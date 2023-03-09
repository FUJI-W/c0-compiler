//
// Created by Klaus-Mikealson on 2019/10/30.
//

#ifndef MYCOMPILER_SYMTABLEMNGR_H
#define MYCOMPILER_SYMTABLEMNGR_H

#include <map>
#include <string>
#include "Constant.h"
#include "SymTable.h"

using namespace std;

class SymTableMngr {
private:
    SymTable curScope;                  // 当前域
    SymTable globalScope;               // 全局域
    map<string, SymTable> name2LocScope;// 局部域

    /* 生成中间代码 */
    // I. 字符串常量管理
    map<string, string> name2String;
    int strNum;
    string strPrefix = "string";

    // II. 中间临时变量管理
    int tmpNum;
    string tmpPrefix = "TMP_";

    // III. 中间临时常量管理
    int constNum;
    string constPrefix = "CON_";

    /* label管理 */
    //string labelPrefix = "label_";
    //int labelNum;
    //map<string, int> name2Label;        // label地址

public:
    SymTableMngr();
    ~SymTableMngr();

    /* 创建新的作用域并更新 */
    void createScope(string scopeName, SymbolType retType);
    void insertCurScope();

    /* 属性更新 - 为当前作用域增加新的项 */
    void addConstant(string constName, SymbolType type, int constVal);
    void addVariable(string varName, SymbolType type);
    void addParam(string paramName, SymbolType type);
    void addArray(string arrName, SymbolType type, int dimension, int length);
    void addTemp(string tempName, SymbolType type);
    void updScope(SymTable scope);

    /* 属性获取 */
    SymTable getCurScope();
    SymTable getGlobalScope();
    SymTable getScope(string funcName);
    string getCurScopeName();
    int getCurScopeRetValType();
    map<string, SymTable> getName2LocScope();

    /* 语义分析辅助函数 */
    // I. Idfr
    bool isIdfrRedefine(string idfrName);
    bool isIdfrUndefine(string idfrName);     // 这个不仅要查当前域，还要查global域

    bool isIdfrSym(string idfrName);        // 注意查当前和global域
    bool isIdfrConst(string idfrName);      // 注意查当前和global域
    bool isIdfrVar(string idfrName);        // 注意查当前和global域
    bool isIdfrArray(string idfrName);      // 注意查当前和global域
    bool isIdfrParam(string idfrName);
    bool isIdfrFunc(string idfrName);
    bool isIdfrCharType(string idfrName);       // char型表达式的判断

    // II. Func
    bool isFuncRedefine(string funcName);
    bool isHaveRetValueFunc(string funcName);
    bool isNonRetValueFunc(string funcName);

    int getFuncParamNum(string funcName);                       // 在相应的域下，查参数表
    vector<SymTableItem> getFuncParamList(string funcName);
    int getFuncRetValType(string funcName);

    void setHaveRetStatement();
    bool getHaveRetStatement();

    /* 生成中间代码 */
    string genStrCon(string str);
    string genTempVar(SymbolType type);
    string genTempCon(SymbolType type, int constVal);
    string genArrayElement(string arrName, string index);
    map<string, string> getName2String();
    SymTableItem getIdfrItem(string idfrName);
};


#endif //MYCOMPILER_SYMTABLEMNGR_H
