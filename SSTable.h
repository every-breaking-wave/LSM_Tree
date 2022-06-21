//
// Created by 16541 on 2022/4/1.
//

#ifndef LSM_TREE_SSTABLE_H
#define LSM_TREE_SSTABLE_H

#include <iostream>
#include <vector>
#include "SSmsg.h"
#include <list>
#define HEADER_SIZE 10272
#define MAX_SIZE 2*1024*1024
using namespace std;

class SSTable {
private:
    int maxLevel{};
    string rootPath = "../SSTable/level";
    vector<pair<uint64_t,string>> pairs;
    uint64_t fileId{};
    uint64_t time{};

public:
    vector<vector<SS_Msg*>> allMsg;

    SSTable();
     explicit SSTable(string & dir);

    ~SSTable();

    void initMsg();

    std::string get(uint64_t key);

    bool find(uint64_t key);

    static string getStringInSST(const string& path, uint32_t begOffset, uint32_t endOffset);

    void compaction01();

    void compaction(int upperLevel);

    void mergeSort(vector<SS_Msg*> overlapTables, vector<SS_Msg*> upperTables, int upperLevel);

    void readPairsByMsg(SS_Msg * ssMsg);

    void readStringVal(const string& filename, uint32_t beg,uint32_t end, string & val);

    void merge_sort(vector<pair<uint64_t, string>> forSortingPairs);

    uint64_t newFileId() const  { return fileId;}

    void addFileId()  {fileId++;}

    uint64_t getTimeStamp() const{
        return time;
    }

    void updateTimeStamp(){
        time++;
    }

    void mkSSTablesByPairs(vector<pair<uint64_t,string>> sortedPairs, int lowerLevel, uint64_t timeStamp);

    string getValDirectlyFromSST(uint64_t key);

    void setTimeStamp(uint64_t time){
        this->time = time;
    }

    void setFileId(uint64_t id){
        this->fileId = id;
    }
};


#endif //LSM_TREE_SSTABLE_H
