#include <stdio.h>
#include <iomanip>
#include <ios>
#include <iostream>
#include <fstream>
#include <map>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

struct WSData {
    int sum;
    int cnt;
    int min;
    int max;
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

int main(int argc, char* argv[])
{
    map<string, WSData> stats;

    // Use default file ...
    const char* file = "measurements.txt";
    if (argc > 1){
        // ... or the first argument.
        file = argv[1];
    }
    ifstream fh(file);
    if (not fh.is_open()){
        std::cerr << "Unable to open '" << file << "'" << std::endl;
        return 1;
    }

    // Variable to store each line from the file
    string line;
    string station;
    string tempstr;
    int temp;
    // Read each line from the file and print it
    while (getline(fh, line)) {
        stringstream ss (line);
         
        getline(ss, station, ';');
        getline(ss, tempstr);
        temp = parse_float(tempstr.data());

        auto it = stats.find(station);
        if (it != stats.end()){
            stats[station].cnt += 1; 
            stats[station].sum += temp; 
            stats[station].max = max(stats[station].max, temp);
            stats[station].min = min(stats[station].min, temp);
        } else {
            stats[station] = WSData{temp, 1, temp, temp};
        }
    }

    vector<pair<string, WSData>> sortedStats(stats.begin(), stats.end());

    // Sort by max temperature (descending order)
    sort(sortedStats.begin(), sortedStats.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    for(const auto &item : sortedStats) {
        cout << item.first 
            << ": avg=" << fixed << setprecision(1) << (float) item.second.sum/(item.second.cnt * 10)
            << " min=" << (float) item.second.min / 10
            << " max:" << (float) item.second.max / 10
            << "\n";
    }

    // Always close the file when done
    fh.close();

    return 0;
}
