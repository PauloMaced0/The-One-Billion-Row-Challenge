cmake_minimum_required(VERSION 3.22.1)
project(project-name CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(SRC_SHARED_UTILITIES src/shared)
set(SRC_SAMPLES_DIR src/samples)
set(SRC_WEATHER_STATIONS_SINGLE_THREAD_DIR src/weather-stations-singlethread)
set(SRC_WEATHER_STATIONS_MULTITHREAD_DIR src/weather-stations-multithread)

include_directories(include)

add_executable(cle-ws
    ${SRC_WEATHER_STATIONS_SINGLE_THREAD_DIR}/main.cpp
)

target_compile_features(cle-ws PRIVATE cxx_std_20)
target_compile_options(cle-ws PRIVATE -Wall -Wextra)
target_include_directories(cle-ws PRIVATE include src)

add_executable(cle-ws-threads 
    ${SRC_WEATHER_STATIONS_MULTITHREAD_DIR}/main.cpp
    ${SRC_SHARED_UTILITIES}/thread_pool.cpp
)

target_compile_features(cle-ws-threads PRIVATE cxx_std_20)
target_compile_options(cle-ws-threads PRIVATE -Wall -Wextra -std=c++0x -pthread -O3)
target_include_directories(cle-ws-threads PRIVATE include src)

add_executable(cle-ws-samples 
    ${SRC_SAMPLES_DIR}/create-samples.cpp
)
target_compile_features(cle-ws-samples PRIVATE cxx_std_20)
target_compile_options(cle-ws-samples PRIVATE -Wall -Wextra)
target_include_directories(cle-ws-samples PRIVATE include src)
