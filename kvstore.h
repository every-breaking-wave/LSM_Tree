#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <cstring>
#include "SSmsg.h"
#include "kvstore_api.h"
#include "skipList.h"
#include "SSTable.h"
#include "bloomFilter.h"
#include "utils.h"

class KVStore : public KVStoreAPI {
    // You can add your implementation here
private:
    int judgeSize;
public:
    SkipList *MEMTable;
    SSTable *disk;
    string rootPath = "../SSTable/level";

    KVStore(const std::string &dir);

    ~KVStore();

    void put(uint64_t key, const std::string &s) override;

    std::string get(uint64_t key) override;

    bool del(uint64_t key) override;

    void reset() override;

    void scan(uint64_t key1, uint64_t key2, std::list<std::pair<uint64_t, std::string> > &list) override;

    bool isOverSize(int sizeOfValue);

    void MEMtoSS();

};
