cmake_minimum_required(VERSION 3.22.1)
project(project-name CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(SRC_SHARED_UTILITIES src/shared)
set(SRC_SAMPLES_DIR src/samples)
set(SRC_WEATHER_STATIONS_SINGLE_THREAD_DIR src/ws-singlethread)
set(SRC_WEATHER_STATIONS_MULTITHREAD_DIR src/ws-multithread)

include_directories(include)

add_executable(brc-ws
    ${SRC_WEATHER_STATIONS_SINGLE_THREAD_DIR}/main.cpp
)

target_compile_features(brc-ws PRIVATE cxx_std_20)
target_compile_options(brc-ws PRIVATE -Wall -Wextra)
target_include_directories(brc-ws PRIVATE include src)

add_executable(brc-ws-threads 
    ${SRC_WEATHER_STATIONS_MULTITHREAD_DIR}/main.cpp
    ${SRC_SHARED_UTILITIES}/thread_pool.cpp
    ${SRC_SHARED_UTILITIES}/hash_table.cpp
)

target_compile_features(brc-ws-threads PRIVATE cxx_std_20)
target_compile_options(brc-ws-threads PRIVATE -Wall -Wextra -std=c++0x -pthread -O3)
target_include_directories(brc-ws-threads PRIVATE include src)

add_executable(brc-ws-samples 
    ${SRC_SAMPLES_DIR}/create-samples.cpp
)
target_compile_features(brc-ws-samples PRIVATE cxx_std_20)
target_compile_options(brc-ws-samples PRIVATE -Wall -Wextra)
target_include_directories(brc-ws-samples PRIVATE include src)
