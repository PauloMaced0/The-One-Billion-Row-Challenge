#include <stdio.h>
#include <cstdlib>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <sys/types.h>
#include <thread>
#include <unordered_map>
#include "thread_pool.h"
#include <iomanip> 
#include <cstring> 

#define BUFFER_SIZE (1 * 1024 *  1024) // 1 MB
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
    int mod;
    if (*s == '-') {
        mod = -1;
        s++;
    } else {
        mod = 1;
    }

    if (s[1] == '.') {
        return (s[0] * 10 + s[2] - (11 * '0')) * mod;
    }

    return (s[0] * 100 + s[1] * 10 + s[3] - (111 * '0')) * mod;
}

struct Task {
    std::shared_ptr<std::unordered_map<std::string, WSData>> map;
    const char* filename;
    std::streampos start;
    std::streampos end;
    int tId;
};

streampos adjustToNextNewline(ifstream &file, streampos pos, streampos fileSize) {
    file.seekg(pos);
    char c;

    while (file.get(c)) {
        if (c == '\n' || file.tellg() >= fileSize) {
            return file.tellg();
        }
    }

    return fileSize;
}

void FileProcessTask(Task args) 
{
    ifstream file(args.filename, std::ios::binary);
    if (!file) {
        std::cerr << "Thread " << args.tId << ": Failed to open file " << args.filename << "\n";
        return;
    }

    char* buffer = (char*) malloc(sizeof(char) * BUFFER_SIZE);

    file.seekg(args.start);

    uint stringStart;
    streamsize bytesRead;
    
    char station[100] = {0};
    char temp[5] = {0};
    char leftover[100] = {0};
    char* target;
    int temperature;

    while (file.tellg() < args.end) {
        // Ensure we don't read past the end
        bytesRead = min(static_cast<streamsize>(BUFFER_SIZE), streamsize(args.end - file.tellg()));
    
        file.read(buffer, bytesRead);
        bytesRead = file.gcount();  // Get the actual number of bytes read
    
        if (bytesRead <= 0) break;  // Stop if nothing was read
        
        stringStart = 0;
        for (uint i = 0; i < bytesRead; i++) {
            if (buffer[i] == ';' || buffer[i] == '\n') {
                target = (buffer[i] == ';') ? station : temp;
                if (leftover[0] == '\0') {
                    memcpy(target, buffer + stringStart, i - stringStart);
                    target[i - stringStart] = '\0';
                } else {
                    strncat(leftover, buffer + stringStart, i - stringStart);
                    strcpy(target, leftover);
                    leftover[0] = '\0';
                }
                stringStart = i + 1;
                if (buffer[i] == '\n') {
                    temperature = parse_float(temp);

                    WSData& entry = (*args.map)[station];
                    entry.cnt += 1;
                    entry.sum += temperature;
                    entry.max = max(entry.max, temperature);
                    entry.min = min(entry.min, temperature);
                }
            }
        }
        strncpy(leftover, buffer + stringStart, bytesRead - stringStart);
        leftover[bytesRead - stringStart] = '\0';
    }

    free(buffer);
    file.close();

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

    // consider this if multiple input file is implemented
    // int splitFactor = (1 << number of input file) * numThreads;
    int splitFactor = 2 * numThreads;

    ifstream fh(file, ios::binary | ios::ate);
    if (not fh.is_open()){
        std::cerr << "Unable to open '" << file << "'" << std::endl;
        return 1;
    }
    
    vector<std::shared_ptr<std::unordered_map<string, WSData>>> stats;

    streampos fileSize = fh.tellg();
    fh.seekg(0, ios::beg);

    // change 20 to a dynamic value (this is the file slipt factor) 
    streampos chunkSize = fileSize / splitFactor;

    streampos start = 0;
    streampos end = chunkSize;

    for (int i = 0; i < splitFactor; i++) {
        // Open file to adjust the end position to the next newline
        end = adjustToNextNewline(fh, end, fileSize);

        auto map = std::make_shared<std::unordered_map<string, WSData>>();

        // Launch thread
        stats.push_back(map);
        Task task {map, file, start, end, i};
        thread_pool.enqueue([task]() {
            FileProcessTask(task);
        });

        start = end;  // Next thread starts from this position

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

    fh.close();
}
