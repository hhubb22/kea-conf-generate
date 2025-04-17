cmake_minimum_required(VERSION 3.19)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(kea-conf-gen)

set(CMAKE_CXX_STANDARD 17)

add_executable(kea-conf-gen main.cc)