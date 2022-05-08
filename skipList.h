//
// Created by 16541 on 2022/3/26.
//

#ifndef LSM_TREE_SKIPLIST_H
#define LSM_TREE_SKIPLIST_H


#pragma once

#include <vector>
#include <climits>
#include <time.h>
#include <iostream>
#define MAX_LEVEL 8
using namespace std;
enum SKNodeType
{
    HEAD = 1,
    NORMAL,
    NIL
};

struct SKNode
{
    uint64_t key;
    std::string val;
    SKNodeType type;
    std::vector<SKNode *> forwards;
    SKNode(uint64_t _key, std::string _val, SKNodeType _type)
            : key(_key), val(_val), type(_type)
    {
        for (int i = 0; i < MAX_LEVEL; ++i)
        {
            forwards.push_back(nullptr);
        }
    }
};

class SkipList
{
private:
    SKNode *head;
    SKNode *NIL;
    unsigned long long s = 1;
    double my_rand();
    int randomLevel();
    int MaxLevel;

public:
    SkipList()
    {
        head = new SKNode(-1, "", SKNodeType::HEAD);
        NIL = new SKNode(INT_MAX, "", SKNodeType::NIL);
        this->MaxLevel = MAX_LEVEL;
        for (int i = 0; i < MAX_LEVEL; ++i)
        {
            head->forwards[i] = NIL;
        }
    }
    SKNode * getHead(){return this->head;}
    void Insert(uint64_t key, const std::string &s);
    string Search(uint64_t key);
    void Delete(uint64_t key);
    void Display();
    void clear();
    ~SkipList()
    {
        SKNode *n1 = head;
        SKNode *n2;
        while (n1)
        {
            n2 = n1->forwards[0];
            delete n1;
            n1 = n2;
        }
    }
};

#endif //LSM_TREE_SKIPLIST_H
