//
// Created by Klaus-Mikealson on 2019/9/28.
//

#ifndef MYCOMPILER_ERROR_H
#define MYCOMPILER_ERROR_H

#include <iostream>
#include <string>
#include <fstream>
#include "Constant.h"

using namespace std;

/* 词法分析错误 a */
enum LexicalErrorCode {
    illegalSymbol = 0,  // 1.0 出现词法规则里不允许出现的符号
    lackRsingleQuote,   // 1.1 定义字符时，没有右单引号
    lackRquote,         // 1.2 定义字符串时，没有右双引号
    illegalChar,        // 1.3 char型字符不合法
};

/* 语法分析错误 */
enum SyntaxErrorCode {
    lackSemi = 0,       // 2.0 应为分号 k
    lackRpar,           // 2.1 应为右小括号’)’ l
    lackRbrack,         // 2.2 应为右中括号’]’ m
    lackWhileBehindDoTK,// 2.3 do-while语句中缺少while n
    notIntCon,          // 2.4 int常量定义中=后面只能是整型常量 o
    notCharCon,         // 2.5 char常量定义中=后面只能是字符型常量 o
};

/* 语义分析错误 */
enum SemanticErrorCode {
    idfrReDefine = 0,   // 3.0 标识符重定义 b
    idfrNotDefine,      // 3.1 未定义的名字 c
    paraNumNotMatch,    // 3.2 函数参数个数不匹配 d
    paraTypeNotMatch,   // 3.3 函数参数类型不匹配 e
    illegalCondition,   // 3.4 条件判断中出现不合法的类型 f
    illegalArrayIndex,  // 3.5 数组元素的下标只能是整形表达式 i
    modifyConstant,     // 3.6 不能改变常量的值 j
};

/* 返回值类型错误 */
enum RetValueErrorCode {
    voidButHaveRetValue = 0,   // 4.0 无返回值的函数存在不匹配的return语句 g
    lackReturnStatement,// 4.1 有返回值的函数缺少return语句 h
    retValueNotMatch,   // 4.2 有返回值的函数存在不匹配的return语句 h
};

class Error {
private:
    /* data */
    int err_num;
    ofstream errorfile;
public:
    Error(string filename);
    ~Error();

    int getErrNum();
    void LexicalError(LexicalErrorCode errorCode, int lineNum);
    void SyntaxError(SyntaxErrorCode errorCode, int lineNum);
    void SemanticError(SemanticErrorCode errorCode, int lineNum);
    void retValueError(RetValueErrorCode errorCode, int lineNum);
};



#endif //MYCOMPILER_ERROR_H
