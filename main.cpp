#include <iostream>
#include "kvstore.h"
#include <fstream>

using namespace std;

double build_SkipList(SkipList *SkipList, string input_file_path) {
    ifstream inputData;
    inputData.open(input_file_path, ios::in);
    if (!inputData) {
        cout << "[error]: file " << input_file_path << " not found." << endl;
        inputData.close();
        return 0;
    }

    string line;
    while (inputData >> line) {
        int bracketPos = line.find('(');
        string op = line.substr(0, bracketPos);
        string param = line.substr(bracketPos + 1, line.size() - bracketPos - 2);
        if (op == "Insert") {
            int commaPos = param.find(',');
            uint64_t key = atoi(param.substr(0, commaPos).c_str());
            string val = (param.substr(commaPos + 1).c_str());
            SkipList->Insert(key, val);
        }
    }
    //HashTable->Display();
    inputData.close();
    return 0;
}


int main() {
    KVStore *kv = new KVStore("../SSTable");

    kv->MEMTable->Insert(3, "good");
    kv->MEMTable->Insert(2, "haha");
//     build_SkipList(&(kv->MEMTable),"../testFile/test1");
//     kv->MEMTable.Display();
    kv->MEMtoSS();
    cout<<kv->disk->get(1);
    delete kv;
    return 0;
}
