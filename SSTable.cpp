//
// Created by 16541 on 2022/4/1.
//

#include "SSTable.h"
#include "utils.h"
#include <fstream>
#include <vector>

using namespace std;

SSTable::SSTable() {
    level = 0;
    this->tableInLevel.emplace_back(0);
}
SSTable::~SSTable(){

}

void SSTable::addSS(string path, int level) {
    if (!utils::dirExists(path)) {
        utils::mkdir(path.c_str());
    }
}

void SSTable::put(uint64_t key) {

}

/**
 * 在sst文件寻找键值为key的val，先在缓冲区Msg中遍历寻找sst中的path和位置
 * @param key
 * @return 寻找到的string，若未找到，返回“@NOT@FOUND”
 */
string SSTable::get(uint64_t key) {   // 目前只需要在level0找
    string findStr;
//    printf("find in cache\n");
//    fflush(stdout);
    string path = "../SSTable/level";
    for (int i = 0; i < allMsg.size(); ++i) {
        if (key >= allMsg[i]->min && key <= allMsg[i]->max && allMsg[i]->bf->isInserted(key)) {
            //若满足基本条件，可开始二分查找
            int beg = 0;
            int end = allMsg[i]->numOfPair - 1;
            int mid = allMsg[i]->numOfPair / 2;
            while (beg < mid && mid < end) {
                if (key <= allMsg[i]->keys[mid]) {
                    end = mid;
                    mid = (beg + mid) / 2;
                } else {
                    beg = mid;
                    mid = (end + mid) / 2;
                }
            }
            // 判断要找的string是否在sst末尾
            if (key == allMsg[i]->keys[beg] || key == allMsg[i]->keys[end]) {
                uint32_t endOffset = (mid == allMsg.size() - 1) ? (-1) : allMsg[i]->offsets[i + 1];
                findStr = getStringInSST(allMsg[i]->path, allMsg[i]->offsets[i], endOffset);
            }

        }
    }
    return findStr;
}

/**
 * 读取sst文件，寻找目标字符串
 * @param path : sst文件的路径
 * @param begOffset string开始的位置
 * @param endOffset string结束的位置，若string在末尾，则为-1
 * @return 找到的string值
 */
string SSTable::getStringInSST(string path, uint32_t begOffset, uint32_t endOffset) {
    fstream in(path, ios::binary | ios::in);
//    if(!in.is_open())
//    {
//        //cout<<"can not open SST file: "+path<<endl;
//        return "";
//    }
    char tmp;
    string findStr = "";
    if (!in.seekg(begOffset, ios::beg)) {
        return "";
    }
    if (endOffset != -1) {
        for (uint32_t i = begOffset; i < endOffset; ++i) {
            // read byte by byte
            in.read((char *) (&tmp), sizeof(tmp));
            findStr += tmp;
            printf("%c",tmp);
            fflush(stdout);
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

void SSTable::compaction() {
    // 获取每个分块的min和max
    int min,max;

}

