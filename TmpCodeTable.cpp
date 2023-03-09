//
// Created by Klaus-Mikealson on 2019/11/16.
//

#include "TmpCodeTable.h"
#include <fstream>

TmpCodeTable::TmpCodeTable() {}
TmpCodeTable::~TmpCodeTable() {}

void TmpCodeTable::addTuple(FourTuple &tuple) {
    table.push_back(tuple);
}
void TmpCodeTable::addTupleList(vector<FourTuple> &tupleList) {
    for (int i = 0; i < tupleList.size(); ++i) {
        table.push_back(tupleList[i]);
    }
}
void TmpCodeTable::setTable(vector<FourTuple> table) {
    this->table = table;
}
void TmpCodeTable::clear() {
    table.clear();
}

vector<FourTuple> TmpCodeTable::getTable() {
    return table;
}

void TmpCodeTable::output(string filename) {
    ofstream midFile(filename);

    for (vector<FourTuple>::iterator iter = table.begin(); iter != table.end(); iter++) {
        midFile << iter->toString() << endl;
    }
}