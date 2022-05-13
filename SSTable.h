//
// Created by 16541 on 2022/4/1.
//

#ifndef LSM_TREE_SSTABLE_H
#define LSM_TREE_SSTABLE_H

#include <iostream>
#include <vector>
#include "SSmsg.h"

using namespace std;

class SSTable {
    int level;

public:
    vector<SS_Msg *> allMsg;
    vector<int> tableInLevel;

    void addSS(string path, int level);

    SSTable();

    ~SSTable();

    void put(uint64_t key);

    std::string get(uint64_t key);

    string getStringInSST(string path, uint32_t begOffset, uint32_t endOffset);

    void compaction();

    void getMinMax(int level, int );
};


#endif //LSM_TREE_SSTABLE_H
