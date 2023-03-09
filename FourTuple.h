//
// Created by Klaus-Mikealson on 2019/11/16.
//

#ifndef MYCOMPILER_FOURTUPLE_H
#define MYCOMPILER_FOURTUPLE_H

#include <string>
#include <sstream>
#include "Constant.h"
#include "SymTableItem.h"

/**
 * 约定:
 *  对于四元组的结构
 *      除了关系运算中 SEXPR 保存在 src1
 *      其余只有单个项的, 均保存在 dest 中
 */
class FourTuple {
private:
    Operator op;
    SymTableItem src1;
    SymTableItem src2;
    SymTableItem dest;

    string name;    // stringName or labelName or funcName
    SymbolType retType;
    int paraNum;    // 值参数传递时, 记录当前是第几个参数, 方便判断个数是否大于4
public:
    FourTuple();
    ~FourTuple();
    FourTuple(Operator op, SymTableItem dest, SymTableItem src1, SymTableItem src2);    // 表达式语句
    FourTuple(Operator op, SymTableItem dest);                     // 声明语句
    FourTuple(Operator op, SymTableItem src1, bool relationalOp);  // 单个Expr
    FourTuple(Operator op, SymTableItem dest, int num, string funcName);  // Push
    FourTuple(Operator op, SymTableItem dest, SymTableItem src1);  // 赋值语句
    FourTuple(Operator op, SymTableItem src1, SymTableItem src2, bool relationalOp);  // 关系运算
    FourTuple(Operator op, string name);    // string or label or funcName
    FourTuple(Operator op);                 // 控制语句 or return;
    FourTuple(Operator op, SymTableItem dest, string funcName); // 有函数调用语句
    FourTuple(Operator op, string funcName, SymbolType retType); // 函数定义

    Operator getOpType();
    SymTableItem getDest();
    SymTableItem getSrc1();
    SymTableItem getSrc2();
    string getName();
    int getParaNum();

    string toString();
};


#endif //MYCOMPILER_FOURTUPLE_H
