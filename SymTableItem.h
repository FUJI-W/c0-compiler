//
// Created by Klaus-Mikealson on 2019/10/30.
//

#ifndef MYCOMPILER_SYMTABLEITEM_H
#define MYCOMPILER_SYMTABLEITEM_H

#include <string>
#include "Constant.h"

using namespace std;

class SymTableItem {
private:
    string symName;		// 符号名
    SymbolType symType;	// 符号类型
    int address;        // 相对地址, 全局变量 - gp, 局部变量 - sp
    int size;           // 记录项的大小,方便分配空间
    bool global;        // 是否为全局变量
    bool constant;      // 是否为常量

    /* extension */
    int dimension;      // 取值 0 或 1 , 数组标识
    int length;         // 若为数组, 则需要初始化数组长度
    int constValue;     // 保存常量值
    int paraNum;        // 记录该参数是第几个参数
public:
    SymTableItem();
    ~SymTableItem();
    SymTableItem(string name, SymbolType type, int addr, bool isGlobal);	                   // General
    SymTableItem(string name, SymbolType type, int addr, int dimen, int len, bool isGlobal);   // array
    SymTableItem(string constName, int constVal, SymbolType type, bool isGlobal);              // const
    SymTableItem(string name, SymbolType type, int addr, int num);	                           // param

    /* 属性访问 */
    string getName();
    int getType();
    int getAddress();
    int getSize();
    bool isGlobal();
    bool isConstant();
    bool isArray();
    int getLength();
    int getConstValue();
};


#endif //MYCOMPILER_SYMTABLEITEM_H
