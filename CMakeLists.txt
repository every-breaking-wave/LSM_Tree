cmake_minimum_required(VERSION 3.21)
project(lsm_tree)

set(CMAKE_CXX_STANDARD 14)

add_executable(lsm_tree main.cpp skipList.cpp skipList.h kvstore.cc bloomFilter.cpp SSTable.cpp)
