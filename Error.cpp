//
// Created by Klaus-Mikealson on 2019/9/28.
//

#include "Error.h"

Error::Error(string filename) {
    err_num = 0;
    errorfile.open(filename);
}

Error::~Error() {}

int Error::getErrNum() {
    return err_num;
}

/* 词法分析错误 a */
void Error::LexicalError(LexicalErrorCode errorCode, int lineNum) {
    switch (errorCode) {
        case illegalSymbol:
            // 1.0 出现词法规则里不允许出现的符号
            cout << "error in lineNo." << lineNum << " : illegal symbol." << endl;
            errorfile << lineNum << " a" << endl;
            break;
        case lackRsingleQuote:
            // 1.1 定义字符时，没有右单引号
            cout << "error in lineNo." << lineNum << " : lack right single quote." << endl;
            errorfile << lineNum << " a" << endl;
            break;
        case lackRquote:
            // 1.2 定义字符串时，没有右双引号
            cout << "error in lineNo." << lineNum << " : lack right quote." << endl;
            errorfile << lineNum << " a" << endl;
            break;
        case illegalChar:
            // 1.3 char型字符不合法
            cout << "error in lineNo." << lineNum << " : illegal character." << endl;
            errorfile << lineNum << " a" << endl;
            break;
        default:
            cout << "error in lineNo." << lineNum << " : unknown lexical error." << endl;
            break;
    }
    err_num++;
}

void Error::SyntaxError(SyntaxErrorCode errorCode, int lineNum) {
    switch (errorCode) {
        case lackSemi:
            // 2.0 应为分号 k
            cout << "error in lineNo." << lineNum << " : lack semicolon." << endl;
            errorfile << lineNum << " k" << endl;
            break;
        case lackRpar:
            // 2.1 应为右小括号’)’ l
            cout << "error in lineNo." << lineNum << " : lack right parenthesis." << endl;
            errorfile << lineNum << " l" << endl;
            break;
        case lackRbrack:
            // 2.2 应为右中括号’]’ m
            cout << "error in lineNo." << lineNum << " : lack right bracket." << endl;
            errorfile << lineNum << " m" << endl;
            break;
        case lackWhileBehindDoTK:
            // 2.3 do-while语句中缺少while n
            cout << "error in lineNo." << lineNum << " : lack WHILETK behind DOTK." << endl;
            errorfile << lineNum << " n" << endl;
            break;
        case notIntCon:
            // 2.4 int常量定义中=后面只能是整型常量 o
            cout << "error in lineNo." << lineNum << " : int constants must be followed by integer." << endl;
            errorfile << lineNum << " o" << endl;
            break;
        case notCharCon:
            // 2.5 char常量定义中=后面只能是字符型常量 o
            cout << "error in lineNo." << lineNum << " : char constants must be followed by character." << endl;
            errorfile << lineNum << " o" << endl;
            break;
        default:
            cout << "error in lineNo." << lineNum << " : unknown syntax error." << endl;
            break;
    }
    err_num++;
}

void Error::SemanticError(SemanticErrorCode errorCode, int lineNum) {
    switch (errorCode) {
        case idfrReDefine:
            // 3.0 标识符重定义 b
            cout << "error in lineNo." << lineNum << " : detect identifier redefined." << endl;
            errorfile << lineNum << " b" << endl;
            break;
        case idfrNotDefine:
            // 3.1 未定义的名字 c
            cout << "error in lineNo." << lineNum << " : identifier not define." << endl;
            errorfile << lineNum << " c" << endl;
            break;
        case paraNumNotMatch:
            // 3.2 函数参数个数不匹配 d
            cout << "error in lineNo." << lineNum << " : the number of function parameter doesn't match." << endl;
            errorfile << lineNum << " d" << endl;
            break;
        case paraTypeNotMatch:
            // 3.3 函数参数类型不匹配 e
            cout << "error in lineNo." << lineNum << " : the type of function parameter doesn't match." << endl;
            errorfile << lineNum << " e" << endl;
            break;
        case illegalCondition:
            // 3.4 条件判断中出现不合法的类型 f
            cout << "error in lineNo." << lineNum << " : detect illegal expression in condition." << endl;
            errorfile << lineNum << " f" << endl;
            break;
        case illegalArrayIndex:
            // 3.5 数组元素的下标只能是整形表达式 i
            cout << "error in lineNo." << lineNum << " : the array index must be integer." << endl;
            errorfile << lineNum << " i" << endl;
            break;
        case modifyConstant:
            // 3.6 不能改变常量的值 j
            cout << "error in lineNo." << lineNum << " : the constant value cannot be modified." << endl;
            errorfile << lineNum << " j" << endl;
            break;
        default:
            cout << "error in lineNo." << lineNum << " : unknown semantic error." << endl;
            break;
    }
    err_num++;
}

void Error::retValueError(RetValueErrorCode errorCode, int lineNum) {
    switch (errorCode) {
        case voidButHaveRetValue:
            // 4.0 无返回值的函数存在不匹配的return语句 g
            cout << "error in lineNo." << lineNum << " : non-return value function exists unmatched return-statement." << endl;
            errorfile << lineNum << " g" << endl;
            break;
        case lackReturnStatement:
            // 4.1 有返回值的函数缺少return语句 h
            cout << "error in lineNo." << lineNum << " : have-return value function lack return-statement." << endl;
            errorfile << lineNum << " h" << endl;
            break;
        case retValueNotMatch:
            // 4.2 有返回值的函数存在不匹配的return语句 h
            cout << "error in lineNo." << lineNum << " : have-return value function exists unmatched return-statement." << endl;
            errorfile << lineNum << " h" << endl;
            break;
        default:
            cout << "error in lineNo." << lineNum << " : unknown retValue error." << endl;
            break;
    }
    err_num++;
}
