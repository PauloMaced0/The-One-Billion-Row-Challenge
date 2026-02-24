#include <chrono>
#include <cstddef>
#include <cstdio>
#include <iomanip>
#include <ios>
#include <stdio.h>
#include <cstdlib>
#include <iosfwd>
#include <iostream>
#include <iostream>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring> 
#include <vector>
#include "thread_pool.h"
#include "hash_table.h"

using namespace std;

#define HASH_TABLE_SIZE 4096

struct Task {
    const char* data;
    int tId;
    size_t start;
    size_t end;
    HashTableWS* map;
};

const char* parse_float(int* temp, const char* s) {
    // parse sign
    int mod = 1;
    if (*s == '-') {
        mod = -1;
        s++;
    }

    if (s[1] == '.') {
        *temp = (s[0] * 10 + s[2] - (11 * '0')) * mod; 
        return s + 4;
    }

    *temp = (s[0] * 100 + s[1] * 10 + s[3] - (111 * '0')) * mod; 
    return s + 5;
}

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
    const char *sep;

    HashTableWS* map = args.map;

    hash_table_node* node = NULL;
    int temperature;
    int len;

    while (ptr < end) {
        // Find ';'
        sep = ptr;
        while (*sep != ';') ++sep;

        len = sep - ptr;

        sep = parse_float(&temperature, ++sep);

        node = map->find(ptr, len);
        if (node == NULL) {
            map->insert(ptr, temperature, len);
        } else {
            map->update(node, temperature);
        }

        ptr = sep;
    }

    return;
}

void MergeMapsTask(HashTableWS& finalStats, HashTableWS& partialMap) {
    for (auto& item : partialMap) {
        hash_table_node* entry = finalStats.find(item.data.station, (int)strlen(item.data.station));
        if (entry != NULL) {
            entry->data.cnt += item.data.cnt;
            entry->data.sum += item.data.sum;
            entry->data.max = max(entry->data.max, item.data.max);
            entry->data.min = min(entry->data.min, item.data.min);
        }
    }
}

void MergeSortTask(vector<ws_data>& arr, int left, int mid, int right) {
    vector<ws_data> temp(right - left + 1);
    int i = left, j = mid + 1, k = 0;

    while (i <= mid && j <= right) {
        // Compare based on a max temperature
        if (strcmp(arr[i].station, arr[j].station) < 0)  
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
    int numThreads = 8;

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
            return 1;
        }
    }

    ThreadPool thread_pool(numThreads);

    int fd = open(file, O_RDONLY); 
    if (fd == -1){
        std::cerr << "Unable to open '" << file << "'" << std::endl;
        return 1;
    }
    
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

    int splitFactor = 200 * numThreads;
    size_t chunkSize = fileSize / splitFactor;

    size_t start = 0;
    size_t end = chunkSize;

    HashTableWS** tables = new HashTableWS*[splitFactor];
    
    // Task creation and execution is the bottleneck
    for (int i = 0; i < splitFactor; i++) {
        // Open file to adjust the end position to the next newline
        end = adjustToNextNewline(mapped, end, fileSize);

        tables[i] = new HashTableWS(HASH_TABLE_SIZE);

        // Launch task
        Task task {mapped, i, start, end, tables[i]};
        thread_pool.enqueue([task]() { FileProcessTask(task); });

        start = end;  // Next task starts from this position
        end = (i == splitFactor - 2) ? fileSize : start + chunkSize;
    }

    thread_pool.wait();

    for (size_t k = splitFactor; k > 1; k = k / 2 + (k % 2)) {
        for (size_t i = 0; i < k / 2; i++) {
            thread_pool.enqueue([&tables, i, k]() {
                MergeMapsTask(*tables[i], *tables[i + k/2 + (k % 2)]);
            });
        }
        thread_pool.wait();
    }

    vector<ws_data> statsVector;

    for (auto& item: *tables[0]) {
        statsVector.push_back(item.data);
    }
    
    size_t n = statsVector.size();

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
        cout << item.station
            << ": avg=" << fixed << setprecision(1) << (float)item.sum/ (10 * item.cnt)
            << " min=" << fixed << setprecision(1) << (float)item.min / 10
            << " max:" << fixed << setprecision(1) << (float)item.max / 10 
            << "\n";
    }

    delete [] tables;

    munmap(mapped, fileSize);
    close(fd);
}
