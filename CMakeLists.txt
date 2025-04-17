cmake_minimum_required(VERSION 3.19)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(kea-conf-gen)

set(CMAKE_CXX_STANDARD 17)

# Find GoogleTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Find nlohmann/json
find_package(nlohmann_json REQUIRED)

add_library(kea-conf-gen KeaGenerator.cc)

add_executable(kea-conf-gen-test KeaGenerator_test.cc KeaGenerator.h)
target_link_libraries(kea-conf-gen-test ${GTEST_LIBRARIES} nlohmann_json::nlohmann_json kea-conf-gen)