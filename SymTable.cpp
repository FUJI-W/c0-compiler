//
// Created by Klaus-Mikealson on 2019/10/30.
//

#include "SymTable.h"
#include <string>
#include <algorithm>

#define align(num) (num%4)==0? num : (num - (num%4) + 4)

SymTable::SymTable() {}
SymTable::~SymTable() {}

SymTable::SymTable(string name, SymbolType type) {
    scopeName = name;
    retType = type;
    offset = 4;     // 0~4 预留给 $ra 寄存器
    global = name.compare(GLOBAL)==0;
    paraNum = 0;

    isInline = false;
}

/* 属性更新 - 为该作用域增加新的项 */
void SymTable::addConstant(string constName, SymbolType type, int constVal) {
    SymTableItem item(constName, constVal, type, global);   // 调用常量构造器
    pair<string, SymTableItem> value(constName, item);
    name2Sym.insert(value);
    name2Const.insert(value);
    //updateOffset(item.getSize());   // 更新 offset
}

void SymTable::addVariable(string varName, SymbolType type) {
    SymTableItem item(varName, type, offset, global);
    pair<string, SymTableItem> value(varName, item);
    name2Sym.insert(value);
    name2Var.insert(value);

    updateOffset(item.getSize());   // 更新 offset
}

void SymTable::addParam(string paramName, SymbolType type) {
    SymTableItem item(paramName, type, offset, paraNum++);
    pair<string, SymTableItem> value(paramName, item);
    name2Sym.insert(value);
    name2Param.insert(value);
    paramList.push_back(item);

    updateOffset(item.getSize());   // 更新 offset
}

void SymTable::addArray(string arrName, SymbolType type, int dimension, int length) {
    SymTableItem item(arrName, type, offset, dimension, length, global);    // 数组构造器
    pair<string, SymTableItem> value(arrName, item);
    name2Sym.insert(value);
    name2Array.insert(value);

    updateOffset(item.getSize());   // 更新 offset
}

void SymTable::addTemp(string tempName, SymbolType type) {
    SymTableItem item(tempName, type, offset, global);
    pair<string, SymTableItem> value(tempName, item);
    name2Sym.insert(value);
    name2Temp.insert(value);

    updateOffset(item.getSize());   // 更新 offset
}

void SymTable::setHaveRetStatement() {
    haveRetStatement = true;
}

void SymTable::setInline() {
    isInline = true;
}

bool SymTable::isInlineFunc() {
    return isInline;
}

void SymTable::setTempState(string name, int state) {
    if (temp2State.count(name) > 0) {
        temp2State[name] = state;
    } else {
        pair<string, int> value(name, state);
        temp2State.insert(value);
    }
}

int SymTable::getTempState(string name) {
    return temp2State[name];
}

void SymTable::setLastTemp(SymTableItem lastTemp) {
    this->lastTemp = lastTemp;
}

SymTableItem SymTable::getLastTemp() {
    return lastTemp;
}

bool SymTable::isTempEmpty() {
    return temp2State.size() == 0;
}

/* 属性访问 */
string SymTable::getScopeName() {
    return scopeName;
}
SymbolType SymTable::getRetType() {
    return retType;
}
map<string, SymTableItem> SymTable::getName2Sym() {
    return name2Sym;
}
map<string, SymTableItem> SymTable::getName2Const() {
    return name2Const;
}
map<string, SymTableItem> SymTable::getName2Var() {
    return name2Var;
}
map<string, SymTableItem> SymTable::getName2Param() {
    return name2Param;
}
map<string, SymTableItem> SymTable::getName2Array() {
    return name2Array;
}
map<string, SymTableItem> SymTable::getName2Temp() {
    return name2Temp;
}
bool SymTable::getHaveRetStatement() {
    return haveRetStatement;
}

/* 语义分析辅助函数 */
// I. Idfr
bool SymTable::isIdfrDefine(string idfrName) {
    return (name2Sym.count(idfrName) > 0);
}
bool SymTable::isIdfrConst(string idfrName) {
    return (name2Const.count(idfrName) > 0);
}
bool SymTable::isIdfrVar(string idfrName) {
    return (name2Var.count(idfrName) > 0);
}
bool SymTable::isIdfrParam(string idfrName) {
    return (name2Param.count(idfrName) > 0);
}
bool SymTable::isIdfrArray(string idfrName) {
    return (name2Array.count(idfrName) > 0);
}
bool SymTable::isIdfrTemp(string idfrName) {
    return (name2Temp.count(idfrName) > 0);
}
SymTableItem SymTable::getIdfrItem(string idfrName) {
    return name2Sym[idfrName];
}
bool SymTable::isIdfrChar(string idfrName) {
    SymTableItem idfr = getIdfrItem(idfrName);
    return idfr.getType()==CHARTK;
}

// II. Func
int SymTable::getParamNum() {
    return name2Param.size();
}
vector<SymTableItem> SymTable::getParamList() {
    return paramList;
}

/* 生成中间代码 */
inline void SymTable::updateOffset(int size) {
    offset += size;
    offset = align(offset);     // 对齐4字节
}

int SymTable::getDataSize() {
    return offset;
}

/* 代码优化 */
void SymTable::initRefCount(int st, int ed, vector<FourTuple> table) {
    int weight = 1;
    for (int i = st; i < ed; ++i) {
        FourTuple tuple = table[i];
        switch (tuple.getOpType()) {
            /* 1. 声明语句 */
            /* 2. 表达式语句 */
            case ADDOP:
            case SUBOP:
            case MULOP:
            case DIVOP:
                countRef(tuple.getDest(), weight);
                countRef(tuple.getSrc1(), weight);
                countRef(tuple.getSrc2(), weight);
                break;
            case USEARR:
                countRef(tuple.getDest(), weight);
                break;
            /* 3. 赋值语句 */
            case ASSVAR:
                countRef(tuple.getDest(), weight);
                countRef(tuple.getSrc1(), weight);
                break;
            case ASSARR:
                // dest[src2] = src1
                countRef(tuple.getDest(), weight);
                countRef(tuple.getSrc1(), weight);
                countRef(tuple.getSrc2(), weight);
                break;
            /* 4. 条件语句 */
            case SEXPR:
                countRef(tuple.getSrc1(), weight);
                break;
            case LESS:
            case LEQL:
            case MORE:
            case MEQL:
            case NEQUAL:
            case EQUAL:
                countRef(tuple.getSrc1(), weight);
                countRef(tuple.getSrc2(), weight);
                break;
            /* 5. 控制语句 */
            // 这样处理没有考虑嵌套循环
            case FOR_BEGIN:
            case DO_BEGIN:
            case WHILE_BEGIN:
                weight = 5;
                break;
            case FOR_END:
            case DO_END:
            case WHILE_END:
                weight = 1;
                break;
            /* 6. 函数调用语句 */
            case PUSH:
                countRef(tuple.getDest(), weight);
                break;
            /* 7. 写语句 */
            case WRITEINTEXPR:
            case WRITECHAREXPR:
                countRef(tuple.getDest(), weight);
                break;
            /* 8. 读语句 */
            case READINT:
            case READCHAR:
                countRef(tuple.getDest(), weight);
                break;
            /* 9. 返回语句 */
            case RETVAL:
                countRef(tuple.getDest(), weight);
                break;
            /* 10. 其他 */
            /* 11. 内联函数 */
            case INLINE_FUNC_BEGIN:
                for (int j = i; j < ed; ++j) {
                    if (table[j].getOpType() == INLINE_FUNC_END) {
                        i = j;
                        break;
                    }
                }
                break;
            case INLINE_RET:    // ret inline
            case INLINE_CALL:   // dest = ret inline
                countRef(tuple.getDest(), weight);
                break;
            default:
                break;
        }
    }
}
void SymTable::countRef(SymTableItem item, int weight) {
    string name = item.getName();
    if (item.isGlobal() ||item.isConstant() || item.isArray() || isIdfrTemp(item.getName())) {
        // 全局变量, 常量, 数组, 以及中间变量
        return;
    }
    // 更新引用次数
    if (refCounter.count(name) > 0) {
        int cnt = refCounter[name] + weight;
        refCounter[name] = cnt;
    } else {
        pair<string, int> value(name, weight);
        refCounter.insert(value);
    }
}

bool cmp(pair<string,int> a, pair<string,int> b) {
    return a.second > b.second;
}
void SymTable::allocReg(const int regs[], int length) {
    // 1. 排序
    vector< pair<string, int> > vec;
    for(map<string, int>::iterator it = refCounter.begin(); it != refCounter.end(); it++){
        vec.push_back( pair<string, int> (it->first, it->second) );
    }
    sort(vec.begin(), vec.end(), cmp);

    // 2. 分配寄存器
    for (int i = 0; i < length && i < vec.size(); ++i) {
        string name = vec[i].first;
        int reg = regs[i];
        pair<string, int> value1(name, reg);
        pair<int, string> value2(reg, name);
        name2Reg.insert(value1);
        reg2Name.insert(value2);
    }
}

int SymTable::getAllocedReg(string name) {
    if (name2Reg.count(name) > 0) {
        return name2Reg[name];
    } else {
        return -1;
    }
}

map<string, int> SymTable::getName2Reg() {
    return name2Reg;
}
map<int, string> SymTable::getReg2Name() {
    return reg2Name;
}
