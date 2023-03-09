//
// Created by Klaus-Mikealson on 2019/9/28.
//

#include "LexicalAnalysis.h"
#include "Constant.h"
#include "Error.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;

LexicalAnalysis::LexicalAnalysis(Error &myError, string infilePath):myError(myError)
{
    // init
    globalNum = 0;
    globalChar = '\0';
    globalCharCon = 0;
    globalPtr = 0;
    globalSymbol = 0;
    curLineNum = 1;

    // 1. read input file
    readFile(infilePath);
    // 2. clear blank characters at head and tail
    // trim();  // 由语法分析知，程序最后不能为空白符，否则不符合语法，故不能trim()
    // 3. check error
    // check();
}

LexicalAnalysis::~LexicalAnalysis() {}

/* Main function */
void LexicalAnalysis::readFile(string infilePath) {
    // read input file to fileContent
    ifstream infile(infilePath);

    stringstream buffer;
    buffer << infile.rdbuf();
    fileContent = buffer.str();
    fileLength = fileContent.size();

    infile.close(); // close input file stream.
}

void LexicalAnalysis::analysis() {
    nextChar();
    while(globalPtr < fileLength)
    {
        nextSym(true);
        //outfile << symbol[globalSymbol] << " " << globalToken << endl;
    }
}

/* get成员变量函数 */
int LexicalAnalysis::getGlobalNum() { return globalNum; }
char LexicalAnalysis::getGlobalChar() { return globalChar; }
int LexicalAnalysis::getGlobalCharCon() { return globalCharCon; }
int LexicalAnalysis::getGlobalPtr() { return globalPtr; }
int LexicalAnalysis::getGlobalSymbol() { return globalSymbol; }
string LexicalAnalysis::getGlobalToken() { return globalToken; }
int LexicalAnalysis::getCurLineNum() { return curLineNum; }
int LexicalAnalysis::getFileLength() { return fileLength; }
string LexicalAnalysis::getFileContent() {return fileContent; }

/* 辅助叶子函数 */
/*
 * :printInfoFlag
 *  ->true:  输出相关信息到outfile文件流
 *  ->false: 用于语法分析的超前扫描
 */
int LexicalAnalysis::nextSym(bool printInfoFlag) {
    // 往后读一个符号并输出
    clearToken();
    while(isSpace() || isTab() || isNewline()) {
        // 若读到 \n ，当前行号增加
        if (globalChar == '\n') {
            curLineNum++;
        }
        nextChar();
    }

    /* 标识符与常量 */
    // 1. 标识符与保留字
    if (isLetter()) {
        while(isLetter() || isDigit()) {
            catToken();
            nextChar();
        }
        //retractPtr();      // 指针回退

        int resultValue = reserver();   // 是否为保留字
        if(resultValue == 0) globalSymbol = IDENFR;
        else    globalSymbol = resultValue;
    }
    // 2. 整型常量
    else if (isDigit()) {
        while(isDigit()) {
            catToken();
            nextChar();
        }
        //retractPtr();

        globalNum = transNum(); // 暂时用不上
        globalSymbol = INTCON;
    }
    // 3. 字符常量
    else if(isSquote()) {
        nextChar();     // 读 ch
#ifdef ERROR
        // 错误处理，判断字符是否符合词法
        // 当printInfoFlag为true时，再打印相关信息，防止重复输出
        if (printInfoFlag && isIllegalChar()) {
            myError.LexicalError(illegalChar, curLineNum);
        }
#endif
        catToken();
        globalCharCon = globalChar;
        globalSymbol = CHARCON;

        nextChar();     // 读 右单引号
#ifdef ERROR
        // 错误处理，定义字符时，没有右单引号
        if (printInfoFlag && globalChar != '\'') {
            myError.LexicalError(lackRsingleQuote, curLineNum);
            retract();  // 回退一个字符, char ch = 'a;
        }
#endif
        nextChar();     // 预读下一个字符
    }
    // 4. 字符串常量
    else if(isQuote()) {
#ifdef ERROR
        bool isLackRquote = false;
        nextChar();
        while(1) {
            if (globalChar == '\n' || globalChar == '\r') {
                // 错误处理，若没有右侧的双引号，则一直读到换行为止
                isLackRquote = true;    // 当printInfoFlag为true时，再打印相关信息，防止重复输出
                break;
            }
            if (globalChar == '\\') {
                globalToken.push_back('\\');
            }

            if(isQuote())   break;
            else    catToken();
            nextChar();
        }
        globalSymbol = STRCON;
        if (printInfoFlag && isLackRquote) {
            // 保证在预读时是不会打印信息的
            myError.LexicalError(lackRquote, curLineNum);
            while (fileContent.at(globalPtr) != ')') {
                // 此步为了将指针回退到 )
                retract();
            }
        }
        nextChar();     // 读 )
#else
        nextChar();
        while(1) {
            if(isQuote())   break;
            else    catToken();
            nextChar();
        }
        globalSymbol = STRCON;
        nextChar();
#endif
    }
    /* 运算符 */
    // 18. +
    else if(isPlus()) { globalSymbol = PLUS; catToken(); nextChar(); }
    // 19. -
    else if(isMinus()) { globalSymbol = MINU; catToken(); nextChar(); }
    // 20. *
    else if(isStar()) { globalSymbol = MULT; catToken(); nextChar(); }
    // 21. /
    else if(isDivi()) { globalSymbol = DIV; catToken(); nextChar(); }
    // 22. <   23. <=
    else if(isLess()) {
        catToken();
        nextChar();

        if(isEqu()) {  // <=
            globalSymbol = LEQ; catToken(); nextChar();
        }
        else {  // <
            globalSymbol = LSS;
        }
    }
    // 24. >    25. >=
    else if(isMore()) {
        catToken();
        nextChar();

        if(isEqu()) {  // >=
            globalSymbol = GEQ; catToken(); nextChar();
        }
        else {  // >
            globalSymbol = GRE;
        }

    }
    // 26. =    27. ==
    else if(isEqu()) {
        catToken();
        nextChar();

        if(isEqu()) {  // ==
            globalSymbol = EQL; catToken(); nextChar();
        }
        else {  // =
            globalSymbol = ASSIGN;
        }
    }
    // 28. !=
    else if(isNot()) {
        catToken();
        nextChar();
        catToken();
        globalSymbol = NEQ;
        nextChar();
    }
    /* 分界符 */
    // 29. ;
    else if(isSemi()) { globalSymbol = SEMICN; catToken(); nextChar(); }
    // 30. ,
    else if(isComma()) { globalSymbol = COMMA; catToken(); nextChar(); }
    // 31. (
    else if(isLpar()) { globalSymbol = LPARENT; catToken(); nextChar(); }
    // 32. )
    else if(isRpar()) { globalSymbol = RPARENT; catToken(); nextChar(); }
    // 33. [
    else if(isLbrack()) { globalSymbol = LBRACK; catToken(); nextChar(); }
    // 34. ]
    else if(isRbrack()) { globalSymbol = RBRACK; catToken(); nextChar(); }
    // 35. {
    else if(isLbrace()) { globalSymbol = LBRACE; catToken(); nextChar(); }
    // 36. }
    else if(isRbrace()) { globalSymbol = RBRACE; catToken(); nextChar(); }
    // other else  -  e.g. '\0'
    else {
#ifdef ERROR
        // 错误处理，出现词法类型中不允许出现的符号
        if (printInfoFlag) {
            myError.LexicalError(illegalSymbol, curLineNum);
            // 读下一个符号
            nextChar();
            nextSym(printInfoFlag);
            return -1;
        }
#endif
    }
    // output info to the outfile stream.
    if (printInfoFlag) {
        outfile << symbol[globalSymbol] << " " << globalToken << endl;
    }
    return 0;
}

void LexicalAnalysis::nextChar() {
    // if it is finished
    if (globalPtr == fileLength) {
        globalChar = '\0';  // 及时地更新char值
        return;
    }
    globalChar = fileContent.at(globalPtr++);
}

void LexicalAnalysis::retract() {
    globalPtr--;
    globalChar = fileContent.at((globalPtr-1));
}

// 清空 token 字符串
void LexicalAnalysis::clearToken() {
    globalToken.clear();
}

// 每次把 char 中字符与 token 字符数组中的字符串连接
void LexicalAnalysis::catToken() {
    globalToken.push_back(globalChar);
}

void LexicalAnalysis::saveCurrentState() {
    lastPtr = globalPtr;
    lastChar = globalChar;
    lastSymbol = globalSymbol;
    lastToken = globalToken;
    lastLineNum = curLineNum;
}
void LexicalAnalysis::restoreLastState() {
    globalPtr = lastPtr;
    globalChar = lastChar;
    globalSymbol = lastSymbol;
    globalToken = lastToken;
    curLineNum = lastLineNum;
}

// 查找保留字。若返回值为 0，则表示 token 中的字符串是一个标识符，否则为保留字编码。
int LexicalAnalysis::reserver() {
    for(int i = 0; i < KWDNUM; i++) {
        // 这里之前直接采用了 keyword->size() 导致出错，因为这个返回的是 keyword[0].length() 即为const的长度
        if (keyword[i].compare(globalToken) == 0) {
            return (i + 4);     // +4 恰好为对应symbol的index
        }
    }
    return 0;
}
// 将 token 中的字符串转换成整形数值
int LexicalAnalysis::transNum() {
    return atoi(globalToken.c_str());
}
// 在当前表达式前面插入一个 0
void LexicalAnalysis::insertZero() {
    globalPtr--;    // 指向当前 globalChar
    globalPtr--;    // 指向符号 + | -
    fileContent.insert(globalPtr, "0");
    /* 更新相关变量 */
    fileLength++;
    lastPtr++;
}
// 词法分析错误处理
bool LexicalAnalysis::isIllegalChar() {
    // 判断字符是否合法
    if (isLetter() || isDigit() || isPlus() || isMinus() || isStar() || isDivi()) {
        return false;
    }
    return true;
}