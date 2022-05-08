//
// Created by 16541 on 2022/3/6.
//

#ifndef BLOOMFILTER_BLOOMFILTER_H
#define BLOOMFILTER_BLOOMFILTER_H

#include <vector>
#include "MurmurHash3.h"

#define  INPUT_SIZE 1000
#define BLOOM_SIZE 10240

class bloomFilter {
public:
    bloomFilter();

    void insert(uint64_t key);

    bool isInserted(uint64_t key);

    void clear();

    bool bloomTable[BLOOM_SIZE] ={false};
private:
    unsigned int hash[4];
};

#endif //BLOOMFILTER_BLOOMFILTER_H
