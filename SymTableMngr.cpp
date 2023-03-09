//
// Created by Klaus-Mikealson on 2019/10/30.
//

#include <sstream>
#include "SymTableMngr.h"

SymTableMngr::SymTableMngr() {
    SymTable globalSymTable(GLOBAL, VOIDTK);

    /* 更新相关变量 */
    globalScope = globalSymTable;
    curScope = globalScope;
    strNum = 0;
    tmpNum = 0;
    constNum = 0;
}

SymTableMngr::~SymTableMngr() {}

/* 创建新的作用域并更新当前作用域 */
void SymTableMngr::createScope(string scopeName, SymbolType retType) {
    /* 1. 保存当前域 */
    if (curScope.getScopeName().compare(GLOBAL) == 0) {
        // 更新全局域
        globalScope = curScope;
    } else {
        // 更新局部域
        insertCurScope();
    }
    /* 2. 更新相关变量 */
    SymTable newTable(scopeName, retType);
    curScope = newTable;
    insertCurScope();       // 临时
}
void SymTableMngr::insertCurScope() {
    pair<string, SymTable> value(curScope.getScopeName(), curScope);
    name2LocScope.insert(value);
}

/* 属性更新 - 为当前作用域增加新的项 */
void SymTableMngr::addConstant(string constName, SymbolType type, int constVal) {
    curScope.addConstant(constName, type, constVal);
    name2LocScope[curScope.getScopeName()] = curScope;  // 临时
}
void SymTableMngr::addVariable(string varName, SymbolType type) {
    curScope.addVariable(varName, type);
    name2LocScope[curScope.getScopeName()] = curScope;  // 临时
}
void SymTableMngr::addParam(string paramName, SymbolType type) {
    curScope.addParam(paramName, type);
    name2LocScope[curScope.getScopeName()] = curScope;  // 临时
}
void SymTableMngr::addArray(string arrName, SymbolType type, int dimension, int length) {
    curScope.addArray(arrName, type, dimension, length);
    name2LocScope[curScope.getScopeName()] = curScope;  // 临时
}
void SymTableMngr::addTemp(string tempName, SymbolType type) {
    curScope.addTemp(tempName, type);
    name2LocScope[curScope.getScopeName()] = curScope;  // 临时
}
void SymTableMngr::updScope(SymTable scope) {
    name2LocScope[scope.getScopeName()] = scope;
}

/* 属性获取 */
SymTable SymTableMngr::getCurScope() {
    return curScope;
}
SymTable SymTableMngr::getGlobalScope() {
    return globalScope;
}
SymTable SymTableMngr::getScope(string funcName) {
    return name2LocScope[funcName];
}
string SymTableMngr::getCurScopeName() {
    return curScope.getScopeName();
}
int SymTableMngr::getCurScopeRetValType() {
    return curScope.getRetType();
}
map<string, SymTable> SymTableMngr::getName2LocScope() {
    return name2LocScope;
}

/* 语义分析辅助函数 */
// I. Idfr
bool SymTableMngr::isIdfrRedefine(string idfrName) {
    // 这里无需判断函数重名
    // 1. 若标识符为全局域，根据文法，此时函数尚未定义
    // 2. 若标识符为局部域，根据规则，将相应的函数定义覆盖，即在次局部域内无法访问函数，而只能将该idfr当变量或常量使用
    // 判断当前作用域下是否有重复定义的变量
    return curScope.isIdfrDefine(idfrName);
}

bool SymTableMngr::isIdfrUndefine(string idfrName) {
    // 这里的判断顺序应该没什么讲究
    // 1. 先判断是否有函数
    if (name2LocScope.count(idfrName) > 0) {
        return false;
    }
    // 2. 查当前域
    if (curScope.isIdfrDefine(idfrName)) {
        return false;
    }
    // 3. 查global域
    if (curScope.getScopeName().compare(GLOBAL) != 0) {
        // 当前非全局域
        if (globalScope.isIdfrDefine(idfrName)) return false;
    }
    return true;
}

bool SymTableMngr::isIdfrSym(string idfrName) {
    // 这里相当于判断标识符是否定义, 与重定义并不同
    // 1. 查当前域
    if (curScope.isIdfrDefine(idfrName)) {
        return true;
    }
    // 2. 查global域
    if (globalScope.isIdfrDefine(idfrName)) {
        return true;
    }
    return false;
}
bool SymTableMngr::isIdfrConst(string idfrName) {
    // 1. 查当前域
    if (curScope.isIdfrDefine(idfrName)) {
        // 若当前域已定义, 则只需判断其是否为常量即可
        // 因为可能当前域为 int a, 而全局域有一个 const int a;
        return curScope.isIdfrConst(idfrName);
    }
    // 2. 查global域
    return globalScope.isIdfrConst(idfrName);
}
bool SymTableMngr::isIdfrVar(string idfrName) {
    // 1. 查当前域
    if (curScope.isIdfrDefine(idfrName)) {
        // 若当前域已定义, 则只需判断其是否为变量即可
        // 因为可能当前域为 int a[10], 而全局域有一个 const int a;
        return curScope.isIdfrVar(idfrName);
    }
    // 2. 查global域
    return globalScope.isIdfrVar(idfrName);
}
bool SymTableMngr::isIdfrArray(string idfrName) {
    // 1. 查当前域
    if (curScope.isIdfrDefine(idfrName)) {
        // 若当前域已定义, 则只需判断其是否为数组类型即可
        // 因为可能当前域为 int a, 而全局域有一个 int a[10];
        return curScope.isIdfrArray(idfrName);
    }
    // 2. 查global域
    return globalScope.isIdfrArray(idfrName);
}
bool SymTableMngr::isIdfrParam(string idfrName) {
    return curScope.isIdfrParam(idfrName);
}
bool SymTableMngr::isIdfrFunc(string idfrName) {
    return name2LocScope.count(idfrName) > 0;
}
bool SymTableMngr::isIdfrCharType(string idfrName) {
    if (curScope.isIdfrDefine(idfrName)) {
        return curScope.isIdfrChar(idfrName);
    }
    return globalScope.isIdfrChar(idfrName);
}

// II. Func
bool SymTableMngr::isFuncRedefine(string funcName) {
    return name2LocScope.count(funcName) > 0;
}
bool SymTableMngr::isHaveRetValueFunc(string funcName) {
    map<string, SymTable>::iterator iter = name2LocScope.find(funcName);

    if (iter != name2LocScope.end() && iter->second.getRetType() != VOIDTK) {
        return true;
    }
    return false;
}
bool SymTableMngr::isNonRetValueFunc(string funcName) {
    map<string, SymTable>::iterator iter = name2LocScope.find(funcName);

    if (iter != name2LocScope.end() && iter->second.getRetType() == VOIDTK) {
        return true;
    }
    return false;
}

int SymTableMngr::getFuncParamNum(string funcName) {
    SymTable table = name2LocScope[funcName];
    return table.getParamNum();
}
vector<SymTableItem> SymTableMngr::getFuncParamList(string funcName) {
    SymTable table = name2LocScope[funcName];
    return table.getParamList();
}
int SymTableMngr::getFuncRetValType(string funcName) {
    SymTable table = name2LocScope[funcName];
    return table.getRetType();
}

void SymTableMngr::setHaveRetStatement() {
    curScope.setHaveRetStatement();
}
bool SymTableMngr::getHaveRetStatement() {
    return curScope.getHaveRetStatement();
}


/* 生成中间代码 */
string SymTableMngr::genStrCon(string str) {
    string suffix;
    stringstream ss;    //定义流ss
    ss << strNum;       //将数字a转化成流ss
    ss >> suffix;       //将流ss转化成字符串

    string name = strPrefix + suffix;  // string0
    pair<string, string> value(name, str);
    name2String.insert(value);
    strNum++;
    return name;
}
string SymTableMngr::genTempVar(SymbolType type) {
    // 生成中间变量名
    string suffix;
    stringstream ss;    // 定义流ss
    ss << tmpNum;       // 将数字a转化成流ss
    ss >> suffix;       // 将流ss转化成字符串

    string name = tmpPrefix + suffix;  // TMP_0
    addTemp(name, type);
    tmpNum++;
    return name;
}
string SymTableMngr::genTempCon(SymbolType type, int constVal) {
    // 生成中间变量名
    string suffix;
    stringstream ss;    // 定义流ss
    ss << constNum;     // 将数字a转化成流ss
    ss >> suffix;       // 将流ss转化成字符串

    string name = constPrefix + suffix; // CON_0
    addConstant(name, type, constVal);
    constNum++;
    return name;
}


string SymTableMngr::genArrayElement(string arrName, string index) {
    /* 生成数组元素名 */
    stringstream ss;
    ss << arrName << "[" << index << "]";
    return ss.str();
}

map<string, string> SymTableMngr::getName2String() {
    return name2String;
}

SymTableItem SymTableMngr::getIdfrItem(string idfrName) {
    if (curScope.isIdfrDefine(idfrName)) {
        return curScope.getIdfrItem(idfrName);
    }
    return globalScope.getIdfrItem(idfrName);
}
