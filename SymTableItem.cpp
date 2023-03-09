//
// Created by Klaus-Mikealson on 2019/10/30.
//

#include "SymTableItem.h"

/* Default */
SymTableItem::SymTableItem() {}
SymTableItem::~SymTableItem() {}
/* General */
SymTableItem::SymTableItem(string name, SymbolType type, int addr, bool isGlobal) {
    symName = name;
    symType = type;
    address = addr;
    dimension = 0;
    length = 1;
    //size = type == INTTK? 4:1;  // 计算变量大小
    size = 4;
    global = isGlobal;
    constant = false;
}
/* array */
SymTableItem::SymTableItem(string name, SymbolType type, int addr, int dimen, int len, bool isGlobal) {
    symName = name;
    symType = type;
    address = addr;
    dimension = dimen;
    length = len;
    //size = type == INTTK? 4*length : 1*length;  // 计算数组大小
    size = 4*length;
    global = isGlobal;
    constant = false;
}
/* const */
SymTableItem::SymTableItem(string constName, int constVal, SymbolType type, bool isGlobal) {
    // 常量在计算时直接替换, 不需要分配内存
    symName = constName;
    symType = type;
    //address = addr;
    constValue = constVal;
    dimension = 0;
    length = 1;
    //size = type == INTTK? 4:1;  // 计算常量大小
    size = 4;
    global = isGlobal;
    constant = true;
}
/* param */
SymTableItem::SymTableItem(string name, SymbolType type, int addr, int num) {
    symName = name;
    symType = type;
    address = addr;
    dimension = 0;
    length = 1;
    //size = type == INTTK? 4:1;  // 计算变量大小
    size = 4;
    global = false;
    constant = false;
    paraNum = num;
}

/* 属性访问 */
string SymTableItem::getName() {
    return symName;
}
int SymTableItem::getType() {
    return symType;
}
int SymTableItem::getAddress() {
    return address;
}
int SymTableItem::getSize() {
    return size;
}
bool SymTableItem::isGlobal() {
    return global;
}
bool SymTableItem::isConstant() {
    return constant;
}
bool SymTableItem::isArray() {
    return dimension;
}
int SymTableItem::getLength() {
    return length;
}
int SymTableItem::getConstValue() {
    return constValue;
}
