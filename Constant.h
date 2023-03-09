//
// Created by Klaus-Mikealson on 2019/9/28.
//

#ifndef MYCOMPILER_CONSTANT_H
#define MYCOMPILER_CONSTANT_H

#include <string>
#include <fstream>

using namespace std;

/* 全局开关 */
#define ERROR   // 错误处理
#define DEBUG   // debug模式
extern const bool optimize;   // 优化中间代码

/* 词法分析相关 */
extern ofstream outfile;      // 文件输出流

enum SymbolType {
    // 标识符与变量值
    IDENFR = 0, INTCON, CHARCON, STRCON,
    // const  int    char    void    main    if    else    do    while    for    scanf    printf    return
    CONSTTK, INTTK, CHARTK, VOIDTK, MAINTK, IFTK, ELSETK, DOTK, WHILETK, FORTK, SCANFTK, PRINTFTK, RETURNTK,
    // +    -   *     /     <   <=    >   >=   ==   !=    =       ;       ,
    PLUS, MINU, MULT, DIV, LSS, LEQ, GRE, GEQ, EQL, NEQ, ASSIGN, SEMICN, COMMA,
    // ()               []              {}
    LPARENT, RPARENT, LBRACK, RBRACK, LBRACE, RBRACE
};

extern const int TYPENUM;
extern const int KWDNUM;
extern const string symbol[];
extern const string keyword[];

/* 语法分析相关 */
extern string GLOBAL;
extern string MAIN;

enum ExprType {
    NONEXPR = 0,
    INTEXPR = 1,
    CHAREXPR = 2
};

/* 错误处理部分 */
extern string errorFilePath;

/* 中间代码生成部分 */
extern string ENTER;
extern string NOTHING;
extern string tmpCodeFilePath;
extern string opedTmpCodefFilePath;

enum Operator {
    /* 1. 声明语句 */
    CONSTDEF,
    VARDEF,
    ARRDEF,
    PARADEF,
    FUNCDEFBEGIN,
    FUNCDEFEND,
    /* 2. 表达式语句 */
    ADDOP,
    SUBOP,
    MULOP,
    DIVOP,
    USEARR,
    /* 3. 赋值语句 */
    ASSVAR,
    ASSARR,
    /* 4. 条件语句 */
    SEXPR,
    LESS,
    LEQL,
    MORE,
    MEQL,
    NEQUAL,
    EQUAL,
    /* 5. 控制语句 */
    IF_BEGIN,
    ELSE,
    IF_END,
    FOR_BEGIN,
    FOR_END,
    DO_BEGIN,
    DO_END,
    WHILE_BEGIN,
    WHILE_END,
    /* 6. 函数调用语句 */
    HAVERETFUNCALL,
    NONRETFUNCALL,
    PUSH,
    FUNCALLBEGIN,
    FUNCALLEND,
    /* 7. 写语句 */
    WRITESTRING,
    WRITEINTEXPR,
    WRITECHAREXPR,
    /* 8. 读语句 */
    READINT,
    READCHAR,
    /* 9. 返回语句 */
    RETVAL,
    RETNON,
    /* 10. 其他 */
    GENLABEL,
    JUMP,
    BEQ,
    BNE,
    /* 11. 内联函数 */
    INLINE_FUNC_BEGIN,
    INLINE_FUNC_END,
    INLINE_RET,
    INLINE_CALL
};

enum ValueType {
    INT = 5,
    CHAR = 6,
    VOID = 7
};

/* 目标代码生成部分 */
extern string mipsFilePath;

/* 代码优化 */

extern const int regs[];
extern const int inlineRegs[];

#endif //MYCOMPILER_CONSTANT_H
