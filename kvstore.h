#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <cstring>
#include <map>
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
    std::map<uint64_t, string> map;
    std::map<uint64_t, string>::iterator  it;
    string rootPath = "../SSTable/level";

    explicit KVStore(const std::string &dir);

    ~KVStore() override;

    void put(uint64_t key, const std::string &s) override;

    std::string get(uint64_t key) override;

    bool del(uint64_t key) override;

    void mapPut(uint64_t key, const std::string &s);

    std::string mapGet(uint64_t key);

    bool mapDel(uint64_t key);

    void reset() override;

    void scan(uint64_t key1, uint64_t key2, std::list<std::pair<uint64_t, std::string> > &list) override;

    bool isOverSize(int sizeOfValue);

    void MEMtoSS();

    void mapMEMtoSS();

};
