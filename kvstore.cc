#include "kvstore.h"


using namespace std;

KVStore::KVStore(const std::string &dir) : KVStoreAPI(dir) {
    this->MEMTable = new SkipList();
    string SSTableStr = dir;
    this->disk = new SSTable(SSTableStr);
    this->judgeSize = HEADER_SIZE;  // 包含 Header和bloomFilter

}

KVStore::~KVStore() {
    // 把内存写入磁盘
    MEMtoSS();
    delete MEMTable;
    delete disk;

}

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
void KVStore::put(uint64_t key, const std::string &s) {
    //判断插入后是否达到2MB（2*1024^2 byte）
    if (isOverSize(s.size())) {
        //超过了，将MEMTable转移到SSTable中
        MEMtoSS();
    }
    judgeSize += MEMTable->Insert(key,s);  // 如果只是更新了val, 把错误的size改变纠正

}

/**
 * Returns the (string) value of the given key.
 * An empty string indicates not found.
 */
std::string KVStore::get(uint64_t key) {
    std::string findStr = MEMTable->Search(key);
    if (findStr == "@NOT@FOUND") { // 在MEMTable中找不到
        findStr = this->disk->get(key);
    }
    if(findStr == MEMTable->getDeleteMark()){ // 在MEMTable 找到了但被标记为删除
        return "";
    }
    return findStr;
}

/**
 * Delete the given key-value pair if it exists.
 * Returns false iff the key is not found.
 */
bool KVStore::del(uint64_t key) {
    // 在 MEMTable
    if(MEMTable->Delete(key))
        return true;
    // 若在 MEMTable为删除态
    if(MEMTable->Search(key) == MEMTable->getDeleteMark())
        return false;
    // 在缓存查找 TODO: Delete触发 SSToMEM？
    string findStr = disk->get(key);
    if(!findStr.empty() && findStr != MEMTable->getDeleteMark()){ // 在MEMTable 找到且不为删除态， 更新 key 的 val 为删除
        MEMTable->Insert(key, this->MEMTable->getDeleteMark());
        return true;
    }
    return false;
}



/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset() {
    this->MEMTable->clear();
    this->disk->setTimeStamp(0);
    this->disk->setFileId(0);
    int level = 0;
    // 先删掉文件，再删掉空文件夹
    for (auto & i : disk->allMsg) {
        for (auto & j : i) {
            delete j;
        }
        i.clear();
    }
    vector<SS_Msg*> msgVec;
    disk->allMsg.emplace_back(msgVec);
    while(true)
    {
        string levelPath = rootPath+ to_string(level);
        vector<string> ret;
        if(utils::dirExists(levelPath))
        {
            utils::scanDir(levelPath,ret);
            for(auto & i : ret)
            {
                utils::rmfile((levelPath+"/"+i).c_str());
            }
            utils::rmdir(levelPath.c_str());
        }
        else
        {
            break;
        }
        level++;
    }
}

/**
 * Return a list including all the key-value pair between key1 and key2.
 * keys in the list should be in an ascending order.
 * An empty string indicates not found.
 */
void KVStore::scan(uint64_t key1, uint64_t key2, std::list<std::pair<uint64_t, std::string> > &list) {
    vector<SS_Msg*> msgVec;
    msgVec.clear();
    uint64_t min = UINT64_MAX;

    // 若目前不存在 sstable/msg
    if(!disk->getTimeStamp()){
        for (uint64_t i = key1; i <= key2; ++i) {
            string str = this->MEMTable->Search(i);
            if(!str.empty() && str != MEMTable->getDeleteMark()){
                list.emplace_back(make_pair(i, str));
            }
        }
        return;
    }
    auto * temMsg = new SS_Msg;
    vector<SKNode*> nodeVec;
    SKNode *node = MEMTable->getHead();
    while (node->key >= key1 && node->key <= key2){
        nodeVec.emplace_back(node);
        node = node->forwards[0];
    }
    temMsg->min = nodeVec[0]->key;
    temMsg->max = nodeVec.back()->key;
    temMsg->numOfPair = nodeVec.size();
    temMsg->time = UINT64_MAX;
    msgVec.emplace_back(temMsg);

    for (auto & i : disk->allMsg) {  // 所有在 scan 范围内的table对应的 msg
        for (auto & j : i) {
            if(j->min <= key2 && j->max >= key1){
                msgVec.emplace_back(j);
            }
        }
    }
    int vecSize = msgVec.size();

    // sort Vector by timeStamp : form big to small
    for (int i = 0; i < vecSize; i++) {
        int max = i;
        for (int j = i + 1; j < vecSize; j++)
            if (msgVec[j]->time > msgVec[max]->time){
                max = j;
            }
        std::swap(msgVec[i], msgVec[max]);
    }

    int minPtr = 0;
    int msgPtr[vecSize];
    for (int i = 0; i < vecSize; ++i) {
        msgPtr[i] = 0;
    }
    for (int i = 0; i < vecSize; ++i) {  // 将每个msg的指针初始化到相应位置
        while(msgVec[i]->keys[msgPtr[i]] < key1){
            msgPtr[i] ++;
        }
    }

    while(true){ // 每次循环获得一个最小值
        min = UINT64_MAX;
        for (int j = 0; j < msgVec.size(); ++j) {
            if((msgVec[j]->numOfPair - 1) >= msgPtr[j] && msgVec[j]->keys[msgPtr[j]] < min){
                min = msgVec[j]->keys[msgPtr[j]];
                minPtr = j;
                continue;
            }
            if(msgVec[j]->keys[msgPtr[j]] == min){ // 说明前面已经出现过time更大的
                msgPtr[j] ++;
            }
        }
        if(min > key2){
            break;
        }
        msgPtr[minPtr] ++;  // 将本次的最小值的msg指针后移
        // 获得val
        list.emplace_back(make_pair(min, this->get(min)));
    }
}

/**
 *  to judge if MEMTable is overSize(>2MB) after new key-value pair is input as SSTable
 */
bool KVStore::isOverSize(int sizeOfValue) {
    // key:8byte, offset:4byte
    return (judgeSize + sizeOfValue + 12) > MAX_SIZE;
}

/**
 * 将存在内存（跳表）中的数据保存到disk中的SSTable,并生成缓存信息Msg，供之后访问使用
 */
void KVStore::MEMtoSS() {
    if(MEMTable->getHead()->forwards[0] == MEMTable->getNil()){
        return;
    }
    //准备工作
    if(! utils::dirExists("../SSTable")){
        utils::mkdir("../SSTable");
    }

    SKNode *node = MEMTable->getHead()->forwards[0];
//    std::string rootPath = "../SSTable/level";
    const char *root = this->rootPath.c_str();
    if (!utils::dirExists(rootPath + "0")) {
        utils::mkdir((rootPath + "0").c_str());
    }
    uint64_t fileNum = disk->newFileId();  // 既是命名也是时间戳
    disk->addFileId();
    int offset = 0;
    bloomFilter *bf = new bloomFilter();
    uint64_t numOfKey = 0;
    uint64_t time = disk->getTimeStamp();
    disk->updateTimeStamp();


    // 生成sst文件
    string filename = rootPath + "0/sstable" + to_string(fileNum) + ".sst";
    fstream out(filename, ios::binary | ios::out);

    // 在底层遍历MEMTable
    vector<pair<uint64_t, string>> pair;  // 键值对vector
    vector<uint32_t> offsets;
    while (node->type != NIL) {
        pair.emplace_back(make_pair(node->key, node->val));
        offsets.emplace_back(offset);
        offset += node->val.size();
        bf->insert(node->key);
        node = node->forwards[0];
        numOfKey++;
    }
    for (int i = 0; i < numOfKey; ++i) {
        offsets[i] += numOfKey * 12 + HEADER_SIZE;  // 基础偏移量
    }


    // Header 用于存放元数据，按顺序分别为该 SSTable 的时间戳（无符号
    // 64 位整型），SSTable 中键值对的数量（无符号 64 位整型），键最小
    // 值和最大值（无符号 64 位整型），共占用 32 B。
    out.write((char *) &(time), sizeof(uint64_t)); // 时间戳
    out.write((char *) &numOfKey, sizeof(uint64_t)); // 键值对数量
    out.write((char *) &pair[0].first, sizeof(uint64_t)); // 最小的key
    out.write((char *) &pair[pair.size() - 1].first, sizeof(uint64_t)); // 最大的key

    // 生成 BloomFilter 大小为 10 KB （10240 字节）
    out.write((char *) bf->bloomTable, sizeof(bf->bloomTable));

    // 索引区，用来存储有序的索引数据，包含所有的键及对应的值在文件中的 offset (无符号 32 位整型)
    for (int i = 0; i < numOfKey; ++i) {
        out.write((char *) &pair[i].first, sizeof(uint64_t));
        out.write((char *) &offsets[i], sizeof(uint32_t));
    }

    // 数据区，用于存储数据(不包含对应的 key)
    for (int i = 0; i < numOfKey; ++i) {
        out.write(pair[i].second.c_str(), pair[i].second.size());
    }
    out.close();


    // 缓存SSTable内的非数据信息
    SS_Msg *msg = new SS_Msg();
    msg->time = time;
    msg->numOfPair = numOfKey;
    msg->max = pair[pair.size() - 1].first;
    msg->min = pair[0].first;
    msg->path = filename;
    for(auto & i : pair){
        msg->keys.emplace_back(i.first);
    }
    msg->offsets = offsets;
    memcpy(&msg->bf->bloomTable, &bf->bloomTable, 10240);
    if(disk->allMsg.empty()){
        vector<SS_Msg*> msgVec;
        disk->allMsg.emplace_back(msgVec);
    }
    if(!disk->allMsg[0].empty()){
        disk->allMsg[0].insert(disk->allMsg[0].begin(),msg);
    } else{
        disk->allMsg[0].emplace_back(msg);
    }

    // 重置内存内容
    MEMTable->clear();
    judgeSize = HEADER_SIZE;

    // 更新disk的相关信息,文件数超标，进行compaction
    vector <string> ret;
    utils::scanDir(rootPath + to_string(0), ret);
    if(ret.size() > 2){
        this->disk->compaction01();
    }

}

void KVStore::mapPut(uint64_t key, const string &s) {
    //判断插入后是否达到2MB（2*1024^2 byte）
    if (isOverSize(s.size())) {
        //超过了，将MEMTable转移到SSTable中
        mapMEMtoSS();
    }
    it = map.find(key);
    if(it != map.end()){
        map.insert(make_pair(key,s));
        judgeSize += s.size() - it->second.size();
        return;
    }
    map.insert(make_pair(key,s));
    judgeSize += 12 + s.size();
}

std::string KVStore::mapGet(uint64_t key) {
    it = map.find(key);
    if(it == map.end()){
        return this->disk->get(key);
    }
    string  findStr = it->second;
    if(findStr == MEMTable->getDeleteMark()){ // 在MEMTable 找到了但被标记为删除
        return "";
    }
    return findStr;
}


bool KVStore::mapDel(uint64_t key) {
    this->it = map.find(key);
    if(it != map.end()){
        map.insert(make_pair(key,MEMTable->getDeleteMark()));
        return true;
    } else if(it->second == MEMTable->getDeleteMark()){
        return false;
    }
    // 在缓存查找 TODO: Delete触发 SSToMEM？
    string findStr = disk->get(key);
    if(!findStr.empty() && findStr != MEMTable->getDeleteMark()){ // 在MEMTable 找到且不为删除态， 更新 key 的 val 为删除
        MEMTable->Insert(key, this->MEMTable->getDeleteMark());
        return true;
    }
    return false;
}

void KVStore::mapMEMtoSS() {
    if(map.empty()){
        return;
    }
    //准备工作
    if(! utils::dirExists("../SSTable")){
        utils::mkdir("../SSTable");
    }

    SKNode *node = MEMTable->getHead()->forwards[0];

    const char *root = this->rootPath.c_str();
    if (!utils::dirExists(rootPath + "0")) {
        utils::mkdir((rootPath + "0").c_str());
    }
    uint64_t fileNum = disk->newFileId();  // 既是命名也是时间戳
    disk->addFileId();
    int offset = 0;
    bloomFilter *bf = new bloomFilter();
    uint64_t numOfKey = 0;
    uint64_t time = disk->getTimeStamp();
    disk->updateTimeStamp();


    // 生成sst文件
    string filename = rootPath + "0/sstable" + to_string(fileNum) + ".sst";
    fstream out(filename, ios::binary | ios::out);

    // 遍历map
    vector<pair<uint64_t, string>> pair;  // 键值对vector
    vector<uint32_t> offsets;

    for (it = map.begin();  it != map.end() ; it++) {
        pair.emplace_back(make_pair(it->first,it->second));
        offsets.emplace_back(offset);
        offset += it->second.size();
        bf->insert(it->first);
        numOfKey++;
    }

    for (int i = 0; i < numOfKey; ++i) {
        offsets[i] += numOfKey * 12 + HEADER_SIZE;  // 基础偏移量
    }

    // Header 用于存放元数据，按顺序分别为该 SSTable 的时间戳（无符号
    // 64 位整型），SSTable 中键值对的数量（无符号 64 位整型），键最小
    // 值和最大值（无符号 64 位整型），共占用 32 B。
    out.write((char *) &(time), sizeof(uint64_t)); // 时间戳
    out.write((char *) &numOfKey, sizeof(uint64_t)); // 键值对数量
    out.write((char *) &pair[0].first, sizeof(uint64_t)); // 最小的key
    out.write((char *) &pair[pair.size() - 1].first, sizeof(uint64_t)); // 最大的key

    // 生成 BloomFilter 大小为 10 KB （10240 字节）
    out.write((char *) bf->bloomTable, sizeof(bf->bloomTable));

    // 索引区，用来存储有序的索引数据，包含所有的键及对应的值在文件中的 offset (无符号 32 位整型)
    for (int i = 0; i < numOfKey; ++i) {
        out.write((char *) &pair[i].first, sizeof(uint64_t));
        out.write((char *) &offsets[i], sizeof(uint32_t));
    }

    // 数据区，用于存储数据(不包含对应的 key)
    for (int i = 0; i < numOfKey; ++i) {
        out.write(pair[i].second.c_str(), pair[i].second.size());
    }
    out.close();


    // 缓存SSTable内的非数据信息
    SS_Msg *msg = new SS_Msg();
    msg->time = time;
    msg->numOfPair = numOfKey;
    msg->max = pair[pair.size() - 1].first;
    msg->min = pair[0].first;
    msg->path = filename;
    for(auto & i : pair){
        msg->keys.emplace_back(i.first);
    }
    msg->offsets = offsets;
    memcpy(&msg->bf->bloomTable, &bf->bloomTable, 10240);
    if(disk->allMsg.empty()){
        vector<SS_Msg*> msgVec;
        disk->allMsg.emplace_back(msgVec);
    }
    if(!disk->allMsg[0].empty()){
        disk->allMsg[0].insert(disk->allMsg[0].begin(),msg);
    } else{
        disk->allMsg[0].emplace_back(msg);
    }

    // 重置内存内容
    map.clear();
    judgeSize = HEADER_SIZE;

    // 更新disk的相关信息,文件数超标，进行compaction
    vector <string> ret;
    utils::scanDir(rootPath + to_string(0), ret);
    if(ret.size() > 2){
        this->disk->compaction01();
    }

}


