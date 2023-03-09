//
// Created by Klaus-Mikealson on 2019/11/16.
//

#ifndef MYCOMPILER_TMPCODETABLE_H
#define MYCOMPILER_TMPCODETABLE_H

#include <vector>
#include "FourTuple.h"

/**
 * 保存并维护结构化的四元组
 */
class TmpCodeTable {
private:
    vector<FourTuple> table;
public:
    TmpCodeTable();
    ~TmpCodeTable();

    /* 属性更新 */
    void addTuple(FourTuple &tuple);
    void addTupleList(vector<FourTuple> &tupleList);
    void setTable(vector<FourTuple> table);
    void clear();
    /* 属性访问 */
    vector<FourTuple> getTable();
    /* 输出中间代码 */
    void output(string filename);
};


#endif //MYCOMPILER_TMPCODETABLE_H
