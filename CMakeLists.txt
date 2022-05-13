cmake_minimum_required(VERSION 3.21)
project(lsm_tree)

set(CMAKE_CXX_STANDARD 14)

add_executable(lsm_tree main.cpp skipList.cpp skipList.h kvstore.cc bloomFilter.cpp bloomFilter.h SSTable.cpp
                SSmsg.cpp SSmsg.h SSTable.h MurmurHash3.h kvstore_api.h kvstore.h correctness.cc
        test.h utils.h)
