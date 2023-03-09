//
// Created by Klaus-Mikealson on 2019/10/30.
//

#ifndef MYCOMPILER_SYMTABLE_H
#define MYCOMPILER_SYMTABLE_H

#include <map>
#include <vector>
#include "Constant.h"
#include "SymTableItem.h"
#include "FourTuple.h"

/**
 * 每张符号表相当于一个域
 * global 代表全局域
 * main   代表主函数域
 * funcName 代表其他函数域
 */

class SymTable {
private:
    bool global;            // 全局域标记
    string scopeName;       // 域名
    SymbolType retType;     // 返回值类型, 全局域和main域返回值均为 void
    map<string, SymTableItem> name2Sym;     // 方便查找该域下的所有标识符
    map<string, SymTableItem> name2Const;   // 常量
    map<string, SymTableItem> name2Var;     // 变量
    map<string, SymTableItem> name2Param;   // 参数, 全局域和main域均无参数
    vector<SymTableItem> paramList;         // 参数, 全局域和main域均无参数
    map<string, SymTableItem> name2Array;   // 一维数组
    map<string, SymTableItem> name2Temp;    // 中间临时变量

    /* 辅助变量 */
    int offset;             // 相对于 sp | gp 的偏移量
    bool haveRetStatement;  // 标记该域是否有返回语句, 用于错误处理的判断
    int paraNum;            // 记录函数的参数个数

    /* 代码优化 */
    map<string, int> refCounter;
    map<string, int> name2Reg;
    map<int, string> reg2Name;
    bool isInline;
    map<string, int> temp2State;    // 0 def状态, 1 used状态, 2 false状态 值被破坏 需要load
    SymTableItem lastTemp;

public:
    SymTable();
    ~SymTable();
    SymTable(string name, SymbolType type);

    /* 属性更新 - 为该作用域增加新的项 */
    void addConstant(string constName, SymbolType type, int constVal);
    void addVariable(string varName, SymbolType type);
    void addParam(string paramName, SymbolType type);
    void addArray(string arrName, SymbolType type, int dimension, int length);
    void addTemp(string tempName, SymbolType type);
    void setHaveRetStatement();
    void setInline();
    bool isInlineFunc();
    void setTempState(string name, int state);
    int getTempState(string name);
    void setLastTemp(SymTableItem lastTemp);
    SymTableItem getLastTemp();
    bool isTempEmpty();

    /* 属性访问 */
    string getScopeName();
    SymbolType getRetType();
    map<string, SymTableItem> getName2Sym();
    map<string, SymTableItem> getName2Const();
    map<string, SymTableItem> getName2Var();
    map<string, SymTableItem> getName2Param();
    map<string, SymTableItem> getName2Array();
    map<string, SymTableItem> getName2Temp();
    bool getHaveRetStatement();

    /* 语义分析辅助函数 */
    // I. Idfr
    bool isIdfrDefine(string idfrName);
    bool isIdfrConst(string idfrName);
    bool isIdfrVar(string idfrName);
    bool isIdfrParam(string idfrName);
    bool isIdfrArray(string idfrName);
    bool isIdfrTemp(string idfrName);
    bool isIdfrChar(string idfrName);	// 用于char型表达式的判断
    SymTableItem getIdfrItem(string idfrName);

    // II. Func
    int getParamNum();
    vector<SymTableItem> getParamList();

    /* 生成中间代码 */
    void updateOffset(int size);
    int getDataSize();

    /* 代码优化 */
    void initRefCount(int st, int ed, vector<FourTuple> table);
    void countRef(SymTableItem item, int weight);
    void allocReg(const int regs[], int length);
    int getAllocedReg(string name);
    map<string, int> getName2Reg();
    map<int, string> getReg2Name();
};


#endif //MYCOMPILER_SYMTABLE_H
