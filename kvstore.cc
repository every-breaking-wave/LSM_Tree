#include "kvstore.h"


using namespace std;

KVStore::KVStore(const std::string &dir) : KVStoreAPI(dir) {
    this->MEMTable = new SkipList();
    this->disk = new SSTable();
    this->judgeSize = 32 + 10240;  // 包含 Header和bloomFilter
    //初始化 disk 目录
    if (!utils::dirExists(dir))
        utils::mkdir(dir.c_str());
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
        MEMTable->Insert(key, s);
        MEMtoSS();
    }
    this->MEMTable->Insert(key, s);
}

/**
 * Returns the (string) value of the given key.
 * An empty string indicates not found.
 */
std::string KVStore::get(uint64_t key) {
    std::string findStr = MEMTable->Search(key);
    if (findStr == "@NOT@FOUND") {
        // 在SSTable中找
        findStr = this->disk->get(key);
    }
    return findStr;
}

/**
 * Delete the given key-value pair if it exists.
 * Returns false iff the key is not found.
 */
bool KVStore::del(uint64_t key) {
    return false;
}

/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset() {
}

/**
 * Return a list including all the key-value pair between key1 and key2.
 * keys in the list should be in an ascending order.
 * An empty string indicates not found.
 */
void KVStore::scan(uint64_t key1, uint64_t key2, std::list<std::pair<uint64_t, std::string> > &list) {
}

/**
 *  to judge if MEMTable is overSize(>2MB) after new key-value pair is input as SSTable
 */
bool KVStore::isOverSize(int sizeOfValue) {
    // key:8byte, offset:4byte
    this->judgeSize += sizeOfValue + 4 + 8;
    return judgeSize > 2 * 1024 * 1024;
}

void KVStore::MEMtoSS() {
    cout<<"tram";
    //准备工作
    SKNode *node = MEMTable->getHead()->forwards[0];
    std::string rootPath = "../SSTable/level0";
    const char *root = rootPath.c_str();
    if (!utils::dirExists(rootPath)) {
        utils::mkdir("../SSTable/level0");
    }
    std::vector<string> ret;
    int fileNum = utils::scanDir(rootPath, ret) + 1;   // 既是命名也是时间戳
    int offset = 0;
    bloomFilter *bf = new bloomFilter();
    uint64_t numOfKey = 0;

    // 生成sst文件
    string filename = rootPath + "/sstable" + to_string(fileNum) + ".sst";
    fstream out(filename, ios::binary | ios::out);

    // 遍历MEMTable
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
    /**
     *  Header 用于存放元数据，按顺序分别为该 SSTable 的时间戳（无符号
     *  64 位整型），SSTable 中键值对的数量（无符号 64 位整型），键最小
     *  值和最大值（无符号 64 位整型），共占用 32 B。
     */
    out.write((char *) &fileNum, sizeof(uint64_t)); // 时间戳
    out.write((char *) &numOfKey, sizeof(uint64_t)); // 键值对数量
    out.write((char *) &pair[0].first, sizeof(uint64_t)); // 最小的key
    out.write((char *) &pair.end()->first, sizeof(uint64_t)); // 最大的key

    /**
     * 生成BloomFilter大小为 10 KB （10240 字节）
     */
    out.write((char *) bf->bloomTable, sizeof(bf->bloomTable));

    /**
     * 索引区，用来存储有序的索引数据，包含所有的键及对应的值在文件中的 offset (无符号 32 位整型)
     */
    for (int i = 0; i < numOfKey; ++i) {
        out.write((char *) &pair[i].first, sizeof(uint64_t));
        out.write((char *) &offsets[i], sizeof(uint32_t));
    }

    /**
     * 数据区，用于存储数据(不包含对应的 key)
     */
    for (int i = 0; i < numOfKey; ++i) {
        out.write((char *) &pair[i].second, pair[i].second.size());
    }
    out.close();

    /**
     * 缓存SSTable内的非数据信息
     */
    SS_Msg *msg = new SS_Msg();
    msg->time = fileNum;
    msg->numOfPair = numOfKey;
    msg->max = pair[pair.size() - 1].first;
    msg->min = pair[0].first;
    msg->path = filename;
    for(int i = 0; i < pair.size(); i++){
        msg->keys.emplace_back(pair[i].first);
    }
    msg->offsets = offsets;
    memcpy(&msg->bf->bloomTable, &bf->bloomTable, 10240);
    disk->allMsg.emplace_back(msg);

}