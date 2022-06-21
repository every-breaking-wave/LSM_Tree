//
// Created by 16541 on 2022/3/6.
//
#include <iostream>
#include "bloomFilter.h"
using namespace std;

bloomFilter::bloomFilter() {

}


void bloomFilter::insert(uint64_t key){
    for (int i = 0; i < 4; ++i) {
        this->hash[i] = 0;
    }
    MurmurHash3_x64_128(&key,sizeof(key),1,hash);
//    cout<<hash;
    for(int i=0;i<4;i++)
    {
       bloomTable[hash[i] % BLOOM_SIZE]=true;
    }
}

bool bloomFilter::isInserted(uint64_t key) {
    for (int i = 0; i < 4; ++i) {
        this->hash[i] = 0;
    }
    MurmurHash3_x64_128(&key,sizeof(key),1,hash);
    for (int i = 0; i < 4; ++i) {
        if(!bloomTable[hash[i] % BLOOM_SIZE])
            return false;       //  只要有一个不在就为false
    }
    return true;
}

void bloomFilter::clear() {
    for (int i = 0; i < BLOOM_SIZE; ++i) {
        bloomTable[i] = false;
    }
}