//
// Created by 16541 on 2022/4/1.
//

#include "SSTable.h"
#include "utils.h"
#include <fstream>
#include <vector>

using namespace std;

SSTable::SSTable() = default;


SSTable::SSTable(string &dir) {
    maxLevel = 0;
    fileId = 0;
    time = 0;// 时间戳
    // 初始化文件目录
    if (!utils::dirExists(dir))
        utils::mkdir(dir.c_str());
    // 初始化 SS_Msg
    initMsg();
    maxLevel = allMsg.size() - 1;
    for (int i = 0; i < allMsg.size(); ++i) {
        for (int j = 0; j < allMsg[i].size(); ++j) {
            if(time < allMsg[i][j]->time)
                time = allMsg[i][j]->time;
            // TODO : 获取最大的 fileId
        }
    }

}





SSTable::~SSTable() {

}


/**
 * 在sst文件寻找键值为key的val，先在缓冲区Msg中遍历寻找sst中的path和位置
 * @param key
 * @return 寻找到的string，若未找到，返回 “”
 */
string SSTable::get(uint64_t key) {
    string findStr = "";
    string path = "../SSTable/level";

    for (auto & i : allMsg) {
        for (auto & j : i) {
            if (key >= j->min && key <= j->max && j->bf->isInserted(key)) {
                //若满足基本条件，可开始二分查找
                uint64_t beg = 0;
                uint64_t end = j->numOfPair - 1;
                uint64_t mid;
                while (beg <= end) {
                    mid = beg + (end - beg) / 2;
                    if(key == j->keys[mid]){
                        break;
                    }
                    if (key < j->keys[mid]) {
                        end = mid - 1;
                    } else {
                        beg = mid + 1;
                    }
                }
                // 判断要找的string是否在sst末尾
                if (key == j->keys[mid]) {
                    uint32_t endOffset = (mid == j->offsets.size() - 1) ? (1) : j->offsets[mid + 1];
                    findStr = getStringInSST(j->path, j->offsets[mid], endOffset);
                    return findStr;
                }
            }
        }
    }
    return findStr;
}


/**
 * 在缓存中查找是否有 key
 * @param key
 * @return
 */
bool SSTable::find(uint64_t key) {
    string findStr;
    string path = "../SSTable/level";
    for (auto & i : allMsg) {
        for (auto & j : i) {
            if (key >= j->min && key <= j->max && j->bf->isInserted(key)) {
                //若满足基本条件，可开始二分查找
                uint64_t beg = 0;
                uint64_t end = j->numOfPair - 1;
                uint64_t mid;
                while (beg <= end) {
                    mid = beg + (end - beg) / 2;
                    if(key == j->keys[mid]){  // 找到了
                        return true;
                    }
                    if (key < j->keys[mid]) {
                        end = mid - 1;
                    } else {
                        beg = mid + 1;
                    }
                }
                return false;
            }
        }
    }
}


/**
 * 读取sst文件，寻找目标字符串
 * @param path : sst文件的路径
 * @param begOffset string开始的位置
 * @param endOffset string结束的位置，若string在末尾，则为-1
 * @return 找到的string值
 */
string SSTable::getStringInSST(const string &path, uint32_t begOffset, uint32_t endOffset) {
    fstream in(path, ios::binary | ios::in);
    if(!in.is_open())
    {
        cout<<"can not open SST file: "+path<<endl;
        return "";
    }
    char tmp;
    string findStr;
    if (!in.seekg(begOffset, ios::beg)) {
        return "";
    }
    if (endOffset != 1) {
        for (uint32_t i = begOffset; i < endOffset; ++i) {
            // read byte by byte
            in.read((char *) (&tmp), sizeof(tmp));
            findStr += tmp;
        }
    } else {
        in.seekg(0, ios::end);
        int len = in.tellg();
        in.seekg(begOffset, ios::beg);
        for (uint32_t i = begOffset; i < len - 1; ++i) {
            in.read((char *) (&tmp), sizeof(tmp));
            findStr += tmp;
        }
        in.read((char *) (&tmp), sizeof(tmp));
        if (tmp != ' ' && tmp != '\0') findStr += tmp;
    }
    in.close();
    return findStr;

}

void SSTable::compaction01() {
//    cout << "compaction01" << endl;
    if(maxLevel == 0)
        maxLevel = 1;
    // 获取每个分块的min和max
    uint64_t min = LONG_LONG_MAX, max = 0;
    // 在第0层找到min和max
    for (auto &i: allMsg[0]) {
        min = i->min < min ? i->min : min;
        max = i->max > max ? i->max : max;
    }
    // 从第一层开始compaction
    if (allMsg.size() == 1) {
        vector < SS_Msg * > newLevelVec;
        allMsg.push_back(newLevelVec);
    }
    if (!utils::dirExists(rootPath + "1"))          //不存在level1目录,先生成
    {
        utils::mkdir((rootPath + "1").c_str());
//        level++;
    }

    vector < SS_Msg * > upperTables;
    for (int i = 0; i < allMsg[0].size(); i++) {  // 使 时间戳 小的靠前
        upperTables.emplace_back(allMsg[0][allMsg[0].size() - 1 - i]);
    }
    vector < SS_Msg * > overlapTables;
    vector < SS_Msg * > lowerLeftTables;
    overlapTables.clear();
    lowerLeftTables.clear();
    for (auto &i: allMsg[1]) {
        if (i->max >= min && i->min <= max) {
            overlapTables.emplace_back(i);
        } else {
            lowerLeftTables.emplace_back(i);
        }
    }

    allMsg[0].clear();
    allMsg[1].clear();
    for (auto & leftTable : lowerLeftTables) {
        allMsg[1].emplace_back(leftTable);
    }

    //归并排序
    mergeSort(overlapTables, upperTables, 0);
}

void SSTable::compaction(int upperLevel) {
    // 初始化lowerLevel
    if(maxLevel == upperLevel)
        maxLevel++;
    if (allMsg.size() <= upperLevel + 1) {
        vector < SS_Msg * > newLevelVec;
        allMsg.push_back(newLevelVec);
    }
    if (!utils::dirExists(rootPath + to_string(upperLevel + 1))){
        utils::mkdir((rootPath + to_string(upperLevel + 1)).c_str());
    }
    // 找到 upperLower 需要 compaction 的 tables
    vector <string> ret;
    utils::scanDir(rootPath + to_string(upperLevel), ret);
    int compactionSize = ret.size() - 2 * (upperLevel + 1);
    // 选择排序
    for (int i = 0; i < ret.size(); i++) {
        int min = i;
        for (int j = i + 1; j < ret.size(); j++)
            if (allMsg[upperLevel][j]->time < allMsg[upperLevel][min]->time){
                min = j;
            } else if(allMsg[upperLevel][j]->time == allMsg[upperLevel][min]->time && allMsg[upperLevel][j]->min < allMsg[upperLevel][min]->min){
                min = j;
            }
        std::swap(allMsg[upperLevel][i], allMsg[upperLevel][min]);
    }
    uint64_t min = LONG_LONG_MAX, max = 0;
    for (int i = 0; i < compactionSize; ++i) {
        if(allMsg[upperLevel][i]->min < min) min = allMsg[upperLevel][i]->min;
        if(allMsg[upperLevel][i]->max > max) max = allMsg[upperLevel][i]->max;
    }

    vector<SS_Msg * > upperOverTables;
    vector<SS_Msg * > upperLeftTables;
    for (int i = 0; i < allMsg[upperLevel].size(); ++i) {
        if(i < compactionSize){
            upperOverTables.emplace_back(allMsg[upperLevel][i]);// 前面的 tables 认为是 oversize 的部分
        }
        else{
            upperLeftTables.emplace_back(allMsg[upperLevel][i]);
        }
    }

    // 利用 min， max 选出下一层需要 compaction 的 tables
    vector < SS_Msg * > overlapTables;
    vector < SS_Msg * > lowerLeftTables;
    overlapTables.clear();
    lowerLeftTables.clear();
    for (auto &i: allMsg[upperLevel + 1]) {
        if ((i->max >= min && i->min <= max) ) {
            overlapTables.emplace_back(i);
        } else {
            lowerLeftTables.emplace_back(i);
        }
    }

    // 删除参与了 compaction 的 msg,
    allMsg[upperLevel].clear();
    allMsg[upperLevel + 1].clear();
    for (auto & lowerLeftTable : lowerLeftTables) {
        allMsg[upperLevel + 1].emplace_back(lowerLeftTable);
    }
    for (auto & upperLeftTable : upperLeftTables) {
        allMsg[upperLevel].emplace_back(upperLeftTable);
    }

    mergeSort(overlapTables, upperOverTables, upperLevel);
}

void SSTable::mergeSort(vector<SS_Msg *> overlapTables, vector<SS_Msg *> upperTables, int upperLevel) {
    //清空之前用于归并排序的pairs
    pairs.clear();

    //从SSTable中取出键值对, 并对键值对归并排序, 由于lowerLevel不存在overlap，故无需考虑先后次序，但应在upperLevel之前
    for (auto &overlapTable: overlapTables) {
        readPairsByMsg(overlapTable);
    }
    // 应当先对旧的val进行归并，以进行新值的覆盖
    for (auto & upperTable : upperTables) {
        readPairsByMsg(upperTable);
    }
    uint64_t timeStamp = 0;
    for (auto & upperTable : upperTables) { // 取最大的时间戳
        if(upperTable->time > timeStamp)
            timeStamp = upperTable->time;
    }
    for (auto & overlapTable : overlapTables) {
        if(overlapTable->time > timeStamp)
            timeStamp = overlapTable->time;
    }
    // 生成lowerLevel的新SSTables
    mkSSTablesByPairs(pairs, upperLevel + 1, timeStamp);
}

void SSTable::readPairsByMsg(SS_Msg *ssMsg) {
    string filename = ssMsg->path;
    int offsetSize = ssMsg->offsets.size();
    vector <pair<uint64_t, string>> getPairs; // 根据ssMsg读出 键值对
    getPairs.clear();
    uint64_t key;
    uint32_t beg, end;
    string val;
    // read valString one by ones
    for (uint64_t i = 0; i < ssMsg->numOfPair; i++) {
        beg = ssMsg->offsets[i];
        end = (i == ssMsg->numOfPair - 1) ? UINT32_MAX : (ssMsg->offsets[i + 1]);  // 考虑最后一个val的特殊情况
        key = ssMsg->keys[i];
        readStringVal(filename, beg, end, val);
        getPairs.emplace_back(make_pair(key, val));
    }
    // merge lowerPairs and upperPairs
    merge_sort(getPairs);
    utils::rmfile(filename.c_str());   // merge结束,数据读到内存后,把文件删掉
}

/**
 *
 * @param filename 读取的sst文件名
 * @param offset  一次读取的val的byte数（若为最后一个val则直接读完）
 * @param val  读取出的val，以函数参数的形式返回
 */
void SSTable::readStringVal(const string &filename, uint32_t beg, uint32_t end, string &val) {
    fstream in(filename, ios::binary | ios::in);
    if (!in.is_open()) {
        cout << "merge_table: can not open SST file " + filename << endl;
        return;
    }
    // 没有考虑 index的size
    in.seekg(beg, ios::beg);
    val = "";
    char readChar;
    if (end == UINT32_MAX) {
        in.seekg(0, ios::end);
        end = in.tellg();
        in.seekg(beg, ios::beg);
    }
    uint32_t offset = end - beg;
    for (int i = 0; i < offset - 1; ++i) {
        in.read((char *) (&readChar), sizeof(readChar));
        val += readChar;
    }
    in.read((char *) (&readChar), sizeof(readChar));
    if (readChar != ' ' && readChar != '\0')
        val += readChar;
    in.close();
}

/**
 * 将新从SSTable中读出的vector<pairs>与已有的vector进行归并排序
 * @param forSortingPairs  待加入排序的vector
 */
void SSTable::merge_sort(vector <pair<uint64_t, string>> forSortingPairs) {
    // 归并排序 pairs  & forSortingPairs
    vector <pair<uint64_t, string>> newPairs;
    uint64_t sizeA = forSortingPairs.size();
    uint64_t sizeB = pairs.size();
    int indexA = 0, indexB = 0;
    if (!sizeB) { // pairs 可能为空
        for (int i = 0; i < sizeA; ++i) {
            newPairs.emplace_back(forSortingPairs[i]);
        }
    } else if (!sizeA) {
        for (int i = 0; i < sizeB; ++i) {
            newPairs.emplace_back(pairs[i]);
        }
    } else {
        while (indexA < sizeA && indexB < sizeB) {  // 将最小的pair一次入列
            if (forSortingPairs[indexA].first < pairs[indexB].first) {
                newPairs.emplace_back(forSortingPairs[indexA]);
                indexA++;
            } else if (forSortingPairs[indexA].first > pairs[indexB].first) {
                newPairs.emplace_back(pairs[indexB]);
                indexB++;
            } else { // key值相等，应采用upperLevel的新val
                newPairs.emplace_back(forSortingPairs[indexA]);
                indexA++;
                indexB++;
            }
        }
        // 处理多的pairs
        for (int i = indexA; i < sizeA; i++) newPairs.push_back(forSortingPairs[i]);
        for (int i = indexB; i < sizeB; i++) newPairs.push_back(pairs[i]);
    }
    pairs = newPairs;
}

void SSTable::mkSSTablesByPairs(vector <pair<uint64_t, string>> sortedPairs, int lowerLevel,uint64_t timeStamp) {
    // 取最大的时间戳
    int curSize = HEADER_SIZE;
    int pairSize = sortedPairs.size();
    uint64_t beg = 0, end = 0;

    // 若在最后一层进行compaction， 删除已删除的键值对
    if(lowerLevel == maxLevel){
        for (int i = 0; i < sortedPairs.size(); ++i) {
            if(sortedPairs[i].second == "~DELETED~"){
                sortedPairs.erase(sortedPairs.begin() + i);
            }
        }
    }

    while (end < pairSize) {
        curSize = HEADER_SIZE;
        beg = end;
        while ( end <= (sortedPairs.size() - 1) && curSize + sortedPairs[end].second.size() + 13 < MAX_SIZE ) { // 仍可以继续写入
            curSize += sortedPairs[end].second.size() + 12;
            end++;
        }
        uint64_t fileId = newFileId();

        // 生成sst文件
        string filename = rootPath + to_string(lowerLevel) + "/sstable" + to_string(fileId) + ".sst";
        addFileId();
        fstream out(filename, ios::binary | ios::out);
        uint64_t size = end - beg;

        uint32_t offset = HEADER_SIZE + 12 * size;
        vector <uint32_t> offsets;
        // Header 用于存放元数据，按顺序分别为该 SSTable 的时间戳（无符号
        // 64 位整型），SSTable 中键值对的数量（无符号 64 位整型），键最小
        // 值和最大值（无符号 64 位整型），共占用 32 B。
        out.write((char *) &(timeStamp), sizeof(uint64_t)); // 时间戳
        out.write((char *) &(size), sizeof(uint64_t)); // 键值对数量
        out.write((char *) &sortedPairs[beg].first, sizeof(uint64_t)); // 最小的key
        out.write((char *) &sortedPairs[end - 1].first, sizeof(uint64_t)); // 最大的key

        // 生成 BloomFilter 大小为 10 KB （10240 字节）
        bloomFilter *bf = new bloomFilter;
        for (int i = beg; i < end; ++i) {
            bf->insert(pairs[i].first);
        }
        out.write((char *) bf->bloomTable, sizeof(bf->bloomTable));

        // 索引区，用来存储有序的索引数据，包含所有的键及对应的值在文件中的 offset (无符号 32 位整型)
        for (int i = beg; i < end; ++i) {
            out.write((char *) &sortedPairs[i].first, sizeof(uint64_t));
            out.write((char *) &offset, sizeof(uint32_t));
            offsets.emplace_back(offset);
            offset += sortedPairs[i].second.size();
        }

        string val;
        int writeCount = 0;
        // 数据区，用于存储数据(不包含对应的 key)
        for (int i = beg; i < end; ++i) {
            writeCount++;
            val = sortedPairs[i].second;
            out.write( sortedPairs[i].second.c_str(), sortedPairs[i].second.size());
        }
        out.close();

        // 缓存SSTable内的非数据信息
        SS_Msg *msg = new SS_Msg();
        msg->time = timeStamp;
        msg->numOfPair = end - beg;
        msg->max = pairs[end - 1].first;
        msg->min = pairs[beg].first;
        msg->path = filename;
        for (uint64_t i = beg; i < end; ++i) {
            msg->keys.emplace_back(pairs[i].first);
        }
        msg->offsets = offsets;
        memcpy(&msg->bf->bloomTable, &bf->bloomTable, 10240);
        allMsg[lowerLevel].insert(allMsg[lowerLevel].begin(), msg);

        // 重置内存内容
//        MEMTable->clear();
//        judgeSize = 32 + 1024;

    }
    // 更新disk的相关信息,文件数超标，进行compaction
    vector <string> ret;
    utils::scanDir(rootPath + to_string(lowerLevel), ret);
    if (ret.size() > 2 * (lowerLevel + 1)) {
        compaction(lowerLevel );
    }

}

void SSTable::initMsg() {
    string path="";
    vector<string> ret;
    int curLevel = 0;
    while(true)
    {
        path = rootPath + to_string(curLevel);
        if(utils::dirExists(path))  //存在当前level
        {
            ret.clear();
            utils::scanDir(path,ret);
            vector<SS_Msg*> newMsgVec;
            allMsg.emplace_back(newMsgVec);
            allMsg[curLevel].clear();
            for(auto & i : ret)
            {
                string filename=path+"/"+i;
                fstream in(filename, ios::binary | ios::in);
                if(!in.is_open())
                {
                    cout<<"can not open SST file:"+filename<<endl;
                    return;
                }


                SS_Msg * msg=new SS_Msg;
                in.read((char*)(&msg->time), sizeof(msg->time));    // 开始读文件
                in.read((char*)(&msg->numOfPair), sizeof(msg->numOfPair));
                in.read((char*)(&msg->min), sizeof(msg->min));
                in.read((char*)(&msg->max), sizeof(msg->max));
                in.read((char*)(&msg->bf->bloomTable), sizeof(msg->bf->bloomTable)); //读bloom fliter

                uint64_t key;
                uint32_t offset;
                for(int j = 0; j < msg->numOfPair; j++)
                {
                    in.read((char*)(&key), sizeof(uint64_t));
                    in.read((char*)(&offset), sizeof(uint32_t));
                    msg->keys.emplace_back(key);
                    msg->offsets.emplace_back(offset);
                }
                in.close();
                msg->path = filename;   //buffer中存路径，方便再次打开table
                allMsg[curLevel].emplace_back(msg);
            }
            curLevel++; //找下一层
        }
        else break;
    }
}

string SSTable::getValDirectlyFromSST(uint64_t key) {
    int curLevel = 0;
    string val = "";
    while(curLevel <= maxLevel)
    {
        string levelPath = rootPath+ to_string(curLevel);
        vector<string> ret;
        if(utils::dirExists(levelPath))
        {
            utils::scanDir(levelPath,ret);
            for(auto & i : ret)
            {
                string filename=levelPath + "/" + i;
                fstream in(filename, ios::binary | ios::in);

                if(!in.is_open())
                {
                    cout<<"can not open SST file:"+filename<<endl;
                    return val;
                }
                SS_Msg * msg=new SS_Msg;
                in.read((char*)(&msg->time), sizeof(msg->time));    // 开始读文件
                in.read((char*)(&msg->numOfPair), sizeof(msg->numOfPair));
                in.read((char*)(&msg->min), sizeof(msg->min));
                in.read((char*)(&msg->max), sizeof(msg->max));
                in.read((char*)(&msg->bf->bloomTable), sizeof(msg->bf->bloomTable)); //读bloom fliter

                if(key > msg->max ||key <= msg->min){
                    continue;
                }
                uint32_t offset;
                for(int j = 0; j < msg->numOfPair; j++)
                {
                    in.read((char*)(&key), sizeof(uint64_t));
                    in.read((char*)(&offset), sizeof(uint32_t));
                    msg->keys.emplace_back(key);
                    msg->offsets.emplace_back(offset);
                }
                for (int j = 0; j < msg->keys.size(); ++j) {
                    if(msg->keys[j] == key){
                        int beg = msg->offsets[j];
                        int end = (j == msg->numOfPair - 1) ? UINT32_MAX : (msg->offsets[j + 1]);  // 考虑最后一个val的特殊情况
                        readStringVal(filename, beg, end, val);
                        return val;
                    }
                }
                in.close();
                msg->path = filename;   //buffer中存路径，方便再次打开table
            }
        }
        curLevel++;
    }
}

