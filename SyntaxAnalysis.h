//
// Created by Klaus-Mikealson on 2019/10/5.
//

#ifndef MYCOMPILER_SYNTAXANALYSIS_H
#define MYCOMPILER_SYNTAXANALYSIS_H

#include <string>
#include <vector>
#include <stack>
#include "Error.h"
#include "LexicalAnalysis.h"
#include "SymTableMngr.h"
#include "SemanticAnalysis.h"

class SyntaxAnalysis {
private:
    Error &myError;                 // 全局共用错误处理类
    SymTableMngr &mySymTableMngr;   // 全局公用符号表管理器
    LexicalAnalysis &myLexical;     // 词法分析子程序
    SemanticAnalysis &mySemantic;   // 动作语义分析子程序

    string lastIdfr;                // 由于递归子程序本身并无返回值, 只能靠此判断
    SymbolType lastIdfrType;

    /* 中间代码生成 - 表达式相关 */
    stack<string> s;

public:
    SyntaxAnalysis(LexicalAnalysis &myLexical, Error &myError, SymTableMngr &mySymTableMngr, SemanticAnalysis &mySemantic);
    ~SyntaxAnalysis();

    /* Main function */
    void analysis();

    /* 辅助叶子函数 */
    void storeAndPreRead();         // 保存当前状态并预读下一个符号
    void restoreAndPrint();         // 恢复到上一个状态并读取+输出下一个符号

    bool isVariableDefinition();        // 超前扫描鉴别 <变量说明> 与 <有返回值函数的定义>
    bool isMainFunction();              // 超前扫描鉴别 <主函数>   与 <无返回值函数的定义>
    int scanFactor();                   // 超前扫描鉴别 标识符、标识符[<表达式>] 与 有返回值函数调用语句 - 标识符(<值参数表>)

    /* 语法分析错误处理 */
    void semicolon();   // 缺 ;
    void rightPar();    // 缺 )
    void rightBrack();  // 缺 ]
    void skipUntilEndSym(SymbolType rightEndSymbol);

    /* 语义分析辅助函数 */
    // 表达式 - 因子
    void genStdFourTuple(SymbolType type);
    void genUseArrFourTuple(string arrName);      // 使用数组元素的四元式, 即数组元素出现在因子中
    // 赋值语句
    void genAssignFourTuple(string dest, string index);
    // 条件语句
    void genConditionalFourTuple(SymbolType type, bool relationalOp);
    // 函数调用语句
    void genHaveRetValFourTuple(string funcName);
    void genNonRetValFourTuple(string funcName);
    void genPushFourTuple(string funcName, int num);

    // 写语句
    void genPrintStrFourTuple(string str);
    void genPrintExprFourTuple(ExprType exprType);
    // 返回语句
    void genRetValFourTuple();

    /* 递归下降子程序分析 */
    // <程序>    ::= ［<常量说明>］［<变量说明>］{<有返回值函数定义>|<无返回值函数定义>}<主函数>
    void program();

    /*
     * 1.［<常量说明>］- 2
     */
    // <常量说明> ::=  const<常量定义>;{ const<常量定义>;}
    void constantDescription();
    // <常量定义>   ::=   int<标识符>＝<整数>{,<标识符>＝<整数>} | char<标识符>＝<字符>{,<标识符>＝<字符>}
    void constantDefinition();

    /*
     * 2.［<变量说明>］ - 2
     */
    // <变量说明>  ::= <变量定义>;{<变量定义>;}
    void variableDescription();
    // <变量定义>  ::= <类型标识符>(<标识符>|<标识符>'['<无符号整数>']'){,(<标识符>|<标识符>'['<无符号整数>']' )}
    //                                                                        <无符号整数>表示数组元素的个数，其值需大于0
    void variableDefinition();

    /*
     * 3. {<有返回值函数定义>|<无返回值函数定义>} - 4
     */
    // <有返回值函数定义>  ::=  <声明头部>'('<参数表>')' '{'<复合语句>'}'
    void haveReturnValueFunctionDefinition();
    // <无返回值函数定义>  ::= void<标识符>'('<参数表>')''{'<复合语句>'}'
    void nonReturnValueFunctionDefinition();
    // <声明头部>   ::=  int<标识符> |char<标识符>
    string declarationHeader();
    // <参数表>    ::=  <类型标识符><标识符>{,<类型标识符><标识符>}| <空>
    void parameterTable();

    /*
     * 4. <主函数> - 4
     */
    // <主函数>    ::= void main‘(’‘)’ ‘{’<复合语句>‘}’
    void mainFunction();
    // <表达式>    ::= ［＋｜－］<项>{<加法运算符><项>}   //[+|-]只作用于第一个<项>
    int expression();
    // <项>     ::= <因子>{<乘法运算符><因子>}
    int item();
    // <因子>    ::= <标识符>｜<标识符>'['<表达式>']'|'('<表达式>')'｜<整数>|<字符>｜<有返回值函数调用语句>
    int factor();

    /*
     * 5. 语句相关 - 14
     */
    // <复合语句>   ::=  ［<常量说明>］［<变量说明>］<语句列>
    void compoundStatement();
    // <语句>    ::= <条件语句>｜<循环语句>| '{'<语句列>'}'| <有返回值函数调用语句>;
    //                           |<无返回值函数调用语句>;｜<赋值语句>;｜<读语句>;｜<写语句>;｜<空>;|<返回语句>;
    int statement();
    // <赋值语句>   ::=  <标识符>＝<表达式>|<标识符>'['<表达式>']'=<表达式>
    void assignmentStatement();
    // <条件语句>  ::= if '('<条件>')'<语句>［else<语句>］
    void conditionalStatement();
    // <条件>    ::=  <表达式><关系运算符><表达式> //整型表达式之间才能进行关系运算
    //               ｜<表达式>    //表达式为整型，其值为0条件为假，值不为0时条件为真
    void condition();
    // <循环语句>   ::=  while '('<条件>')'<语句>| do<语句>while '('<条件>')' |
    //                  for'('<标识符>＝<表达式>;<条件>;<标识符>＝<标识符>(+|-)<步长>')'<语句>
    void loopStatement();
    // <步长>::= <无符号整数>
    int stepSize();
    // <有返回值函数调用语句> ::= <标识符>'('<值参数表>')'
    void haveReturnValueFunctionCallStatement();
    // <无返回值函数调用语句> ::= <标识符>'('<值参数表>')'
    void nonReturnValueFunctionCallStatement();
    // <值参数表>   ::= <表达式>{,<表达式>}｜<空>
    void valueParameterTable(string funcName);
    // <语句列>   ::= {<语句>}
    void statementSeries();
    // <读语句>    ::=  scanf '('<标识符>{,<标识符>}')'
    void readStatement();
    // <写语句>    ::= printf '(' <字符串>,<表达式> ')'| printf '('<字符串> ')'| printf '('<表达式>')'
    void writeStatement();
    // <返回语句>   ::=  return['('<表达式>')']
    void returnStatement();

    /*
     * 6. 需要输出的终结符 - 3
     */
    // <整数>        ::= ［＋｜－］<无符号整数>
    int integer();
    // <无符号整数>  ::= <非零数字>{<数字>}| 0
    int unsignedInteger();
    // <字符串>   ::=  "{十进制编码为32,33,35-126的ASCII字符}"
    void stringConstant();
    
    /*
     * 7. 非输出符号 - 8
     */
    // <标识符>    ::=  <字母>{<字母>｜<数字>}
    void identifier();
    // <类型标识符>      ::=  int | char
    void typeIdentifier();
    // <字符>    ::=  '<加法运算符>'｜'<乘法运算符>'｜'<字母>'｜'<数字>'
    int character();
    // <加法运算符> ::= +｜-
    void addOperator();
    // <乘法运算符>  ::= *｜/
    void multiOperator();
    // <关系运算符>  ::=  <｜<=｜>｜>=｜!=｜==
    void relationalOperator();
    // <字母>   ::= ＿｜a｜．．．｜z｜A｜．．．｜Z
    void alphabet();
    // <数字>   ::= 0｜<非零数字>
    void digit();
    // <非零数字>  ::= 1｜...｜1
    void nonZeroDigit();
};


#endif //MYCOMPILER_SYNTAXANALYSIS_H
