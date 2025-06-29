#include <cstddef>
#include <cstdio>
#include <iomanip>
#include <stdio.h>
#include <cstdlib>
#include <iosfwd>
#include <iostream>
#include <iostream>
#include <memory>
#include <string>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <cstring> 
#include "thread_pool.h"

#define TEMP_MAX 1000
#define TEMP_MIN -1000

using namespace std;

struct WSData {
    int sum = 0;
    int cnt = 0;
    int min = TEMP_MAX;
    int max = TEMP_MIN;
};

int parse_float(const char* s) {
    // parse sign
    int mod = 1;
    if (*s == '-') {
        mod = -1;
        s++;
    }

    if (s[1] == '.') {
        return (s[0] * 10 + s[2] - (11 * '0')) * mod;
    }

    return (s[0] * 100 + s[1] * 10 + s[3] - (111 * '0')) * mod;
}

struct Task {
    std::shared_ptr<std::unordered_map<std::string, WSData>> map;
    const char* data;
    size_t start;
    size_t end;
    int tId;
};

size_t adjustToNextNewline(const char* data, size_t end, size_t fileSize) {
    size_t i = end;

    while (i < fileSize) {
        if (data[i] == '\n') {
            return i + 1;
        }
        i++;
    }

    return fileSize;
}

void FileProcessTask(Task args) 
{
    const char *ptr = args.data + args.start;
    const char *end = args.data + args.end;

    char station[100] = {0};
    char temp[5] = {0};
    int temperature;

    const char *sep;
    const char *newline;

    while (ptr < end) {
        // Find ';'
        sep = ptr;
        while (sep < end && *sep != ';') ++sep;

        memcpy(station, ptr, sep - ptr);
        station[sep - ptr] = '\0';

        // Advance past ';'
        if (sep < end) ++sep;

        // Find '\n'
        newline = sep;
        while (newline < end && *newline != '\n') ++newline;

        memcpy(temp, sep, newline - sep);
        temp[newline - sep] = '\0';

        temperature = parse_float(temp);

        auto &entry = (*args.map)[station];
        entry.cnt += 1;
        entry.sum += temperature;
        entry.max = max(entry.max, temperature);
        entry.min = min(entry.min, temperature);

        // Move to next line
        if (newline < end) ++newline;
        ptr = newline;
    }

    return;
}

void MergeMapsTask(unordered_map<string, WSData>& finalStats, const unordered_map<string, WSData>& partialMap) {
    for (const auto& item : partialMap) {
        WSData& entry = finalStats[item.first];
        entry.cnt += item.second.cnt;
        entry.sum += item.second.sum;
        entry.max = max(entry.max, item.second.max);
        entry.min = min(entry.min, item.second.min);
    }
}

void MergeSortTask(std::vector<std::pair<string, WSData>>& arr, int left, int mid, int right) {
    vector<pair<string, WSData>> temp(right - left + 1);
    int i = left, j = mid + 1, k = 0;

    while (i <= mid && j <= right) {
        // Compare based on a max temperature
        if (arr[i].second.max < arr[j].second.max)  
            temp[k++] = arr[i++];
        else
            temp[k++] = arr[j++];
    }
     
    // Copy remaining elements from left and right subarrays
    while (i <= mid) temp[k++] = arr[i++];
    while (j <= right) temp[k++] = arr[j++];
    
    // Copy sorted elements back to the original array
    for (size_t i = 0; i < temp.size(); i++)
        arr[left + i] = temp[i];
}

int main(int argc, char* argv[])
{
    int numThreads = thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2; // Fallback if the function returns 0

    // Use default file ...
    const char* file = "measurements.txt";
    if (argc > 1){
        // ... or the first argument.
        file = argv[1];
    }

    if (argc > 2) {
        numThreads = atoi(argv[2]);
        if (numThreads <= 0) {
            cerr << "Invalid number of threads.\n";
        }
    }

    ThreadPool thread_pool(numThreads);

    int fd = open(file, O_RDONLY); 
    if (fd == -1){
        std::cerr << "Unable to open '" << file << "'" << std::endl;
        return 1;
    }
    
    vector<std::shared_ptr<std::unordered_map<string, WSData>>> stats;

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        std::cerr << "fstat failed." << std::endl;
        return 1;
    }

    size_t fileSize = (size_t)sb.st_size;

    char *mapped = (char*) mmap(NULL, fileSize, PROT_READ, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        std::cerr << "Unable to map '" << file << "'" << std::endl;
        return 1;
    }

    int splitFactor = 10 * numThreads;
    size_t chunkSize = fileSize / splitFactor;

    size_t start = 0;
    size_t end = chunkSize;

    // Task creation and execution is the bottleneck
    for (int i = 0; i < splitFactor; i++) {
        // Open file to adjust the end position to the next newline
        end = adjustToNextNewline(mapped, end, fileSize);

        auto map = std::make_shared<std::unordered_map<string, WSData>>();
        stats.push_back(map);

        // Launch task
        Task task {map, mapped, start, end, i};
        thread_pool.enqueue([task]() { FileProcessTask(task); });

        start = end;  // Next task starts from this position
        end = (i == splitFactor - 2) ? fileSize : start + chunkSize;
    }

    thread_pool.wait();

    size_t n = stats.size();

    for (size_t k = n; k > 1; k = k / 2 + (k % 2)) {
        for (size_t i = 0; i < k / 2; i++) {
            thread_pool.enqueue([&stats, i, k]() {
                MergeMapsTask(*stats[i], *stats[i + k/2 + (k % 2)]);
            });
        }
        thread_pool.wait();
    }

    std::vector<std::pair<std::string, WSData>> statsVector(stats[0]->begin(), stats[0]->end());

    n = statsVector.size();
    int mid, right;

    for (size_t size = 1; size < n; size *= 2) {
        for (size_t left = 0; left < n - size; left += 2 * size) {
            mid = left + size - 1;
            right = min(left + 2 * size - 1, n - 1);

            // Enqueue merge task
            thread_pool.enqueue([&, left, mid, right]() {
                MergeSortTask(statsVector, left, mid, right);
            });
        } 
        thread_pool.wait();
    }
    
    for(const auto &item : statsVector) {
        cout << item.first 
            << ": avg=" << fixed << setprecision(1) << (float)item.second.sum/ (10 * item.second.cnt)
            << " min=" << fixed << setprecision(1) << (float)item.second.min / 10
            << " max:" << fixed << setprecision(1) << (float)item.second.max / 10 
            << "\n";
    }

    munmap(mapped, fileSize);
    close(fd);
}
