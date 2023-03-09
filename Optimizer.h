//
// Created by Klaus-Mikealson on 2019/12/12.
//

#ifndef MYCOMPILER_OPTIMIZER_H
#define MYCOMPILER_OPTIMIZER_H


#include <map>
#include "TmpCodeTable.h"
#include "SymTableMngr.h"

class Optimizer {
private:
    SymTableMngr &mySymTableMngr;   // 全局公用符号表管理器
    TmpCodeTable &myTmpCodeTable;   // 全局公用中间代码表
    TmpCodeTable myOpedTmpCodeTable;

    int inline_num = 0;
    string suffix = "_inline_";

public:
    Optimizer(SymTableMngr &mySymTableMngr, TmpCodeTable &myTmpCodeTable);
    ~Optimizer();

    TmpCodeTable optimize();
    // 复制传播
    void optCopyProp();
    void optCopyPropagation(int st, int ed, vector<FourTuple> table);
    // 死代码删除
    void optDeadCode();
    vector<FourTuple> deleteDeadCode(int st, int ed, vector<FourTuple> table);
    // 循环优化
    void optLoop();
    vector<FourTuple> genWhileLoop(int st, int ed, vector<FourTuple> table);
    vector<FourTuple> genForLoop(int st, int ed, vector<FourTuple> table);
    // 代码内联
    void optCodeInline();
    vector<FourTuple> genInlineFunc(int st, int ed, vector<FourTuple> table);
    // 寄存器分配
    void optRegAlloc();

    /* 辅助函数 */
    void controlState(vector<FourTuple>& inlineTable, Operator op, string funcName); // mark begin or end of CtrlState
    void pushState(vector<FourTuple>& inlineTable, FourTuple tuple);
    void retState(vector<FourTuple>& inlineTable, FourTuple tuple);
    void modifyLabel(vector<FourTuple>& inlineTable, FourTuple tuple);
};


#endif //MYCOMPILER_OPTIMIZER_H
