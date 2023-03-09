//
// Created by Klaus-Mikealson on 2019/11/16.
//

#ifndef MYCOMPILER_SEMANTICANALYSIS_H
#define MYCOMPILER_SEMANTICANALYSIS_H

#include "TmpCodeTable.h"
#include "SymTableMngr.h"

/**
 * 提供语义动作子程序函数
 * 供语法分析器在相应的语法成分内调用, 以生成结构化的中间代码
 */
class SemanticAnalysis {
private:
    TmpCodeTable &myTmpCodeTable;
    SymTableMngr &mySymTableMngr;
    int ifNum;
    int forNum;
    int doNum;
    int whileNum;
public:
    SemanticAnalysis();
    ~SemanticAnalysis();
    SemanticAnalysis(TmpCodeTable &myTmpCodeTable, SymTableMngr &mySymTableMngr);

    /* 语义动作子程序 - 生成结构化四元组 */
    /* 1. 声明语句 */
    void constDefState(string constName);
    void varDefState(string varName);
    void arrDefState(string arrName);
    void paraDefState(string paraName);
    void funcDefBegin(string funcName, SymbolType retType);
    void funcDefEnd(string funcName);
    /* 2. 表达式语句 */
    void exprState(Operator op, string dest, string src1, string src2);
    void useArr(string dest, string src1, string src2); // dest[src2] = src1
    /* 3. 赋值语句 */
    void assVar(string dest, string src1);              // dest = src1
    void assArr(string dest, string src1, string src2); // dest = src1[src2]
    /* 4. 条件语句 */
    void conditionState(Operator op, string src1);
    void conditionState(Operator op, string src1, string src2);
    /* 5. 控制语句 */
    void controlState(Operator op);      // mark begin or end of CtrlState
    string genCtrlLabelName(Operator op, int suffix);
    int getIfNum();
    int getForNum();
    int getDoNum();
    int getWhileNum();
    /* 6. 函数调用语句 */
    void haveRetFunCallState(string dest, string funcName);    // dest = funcName()
    void nonRetFunCallState(string funcName);                  // funcName()
    void push(string dest, int num, string funcName);          // push dest
    void funCallBegin(string funcName);
    void funCallEnd(string funcName);
    /* 7. 写语句 */
    void printStrState(string stringName);
    void printExprState(string dest, ExprType exprType);
    /* 8. 读语句 */
    void readState(string dest);
    /* 9. 返回语句 */
    void retValState(string dest);
    void retNonState();
    /* 10. 其他 */
    void genLabel(string labelName);
    void jump(string labelName);
    void branch(string labelName, bool beq);
};


#endif //MYCOMPILER_SEMANTICANALYSIS_H
