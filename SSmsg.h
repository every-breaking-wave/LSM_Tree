//
// Created by 16541 on 2022/4/2.
//

#ifndef LSM_TREE_SSMSG_H
#define LSM_TREE_SSMSG_H

#include <iostream>
#include <vector>
#include "bloomFilter.h"

class SS_Msg {
public:
    SS_Msg() { bf = new bloomFilter(); }

    ~SS_Msg() { delete bf; }

    uint64_t time, numOfPair, min, max;
    bloomFilter *bf;
    std::vector<uint64_t> keys;
    std::vector<uint32_t> offsets;
    std::string path;
};


#endif //LSM_TREE_SSMSG_H
