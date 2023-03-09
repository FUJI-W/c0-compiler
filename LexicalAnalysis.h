//
// Created by Klaus-Mikealson on 2019/9/28.
//

#ifndef MYCOMPILER_LEXICALANALYSIS_H
#define MYCOMPILER_LEXICALANALYSIS_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include "Error.h"

using namespace std;

class LexicalAnalysis {
private:
    Error &myError;             // 全局共用错误处理类

    int globalNum;              // 存放当前整型数值
    char globalChar;            // 存放当前读进的字符
    int globalCharCon;          // 存放当前读进的字符常量
    int globalPtr;              // 读字符指针
    int globalSymbol;           // 存放当前所识别单词的类型，与 SymbolType 一一对应
    string globalToken;         // 存放单词的字符串
    int curLineNum;             // 保存当前行号

    int lastPtr;                // 用于超前扫描时保存当前字符指针
    char lastChar;              // 用于超前扫描时保存当前字符
    int lastSymbol;             // 用于超前扫描时保存当前标识符类型
    string lastToken;           // 用于超前扫描时保存当前单词的字符串
    int lastLineNum;            // 用于超前扫描时保存当前行号

    int fileLength;             // 文件长度
    string fileContent;         // 文件内容

public:
    /* 构造与析构函数 */
    LexicalAnalysis(Error &myError, string infilePath);
    ~LexicalAnalysis();

    /* Main function */
    void readFile(string filePath);
    void analysis();

    /* get成员变量函数 */
    int getGlobalNum();
    char getGlobalChar();
    int getGlobalCharCon();
    int getGlobalPtr();
    int getGlobalSymbol();
    string getGlobalToken();
    int getCurLineNum();
    int getFileLength();
    string getFileContent();

    /* 辅助叶子函数 */
    int nextSym(bool printInfoFlag);
    void nextChar();    // 预读下一个字符，并存入globalChar
    void retract();     // 回退到上一个字符与字符指针
    void clearToken();  // 清空Token字符串
    void catToken();    // 每次把 char 中字符与 token 字符数组中的字符串连接
    void saveCurrentState();
    void restoreLastState();
    int reserver();     // 查找保留字。若返回值为 0，则表示 token 中的字符串是一个标识符，否则为保留字编码。
    int transNum();     // 将 token 中的字符串转换成整形数值
    void insertZero();  // 在当前表达式前面插入一个 0

    /* 词法分析相关 */
    // 判断是否为分隔符
    bool isSeparChar() { return (isSpace() || isNewline() || isTab() || isComma() || isSemi() || isLpar() || isLbrack()); }
    // 判断字符是否为空格、换行符和Tab
    bool isSpace() { return globalChar == ' ';}
    bool isTab() { return globalChar == '\t';}
    bool isNewline() { return (globalChar == '\r' || globalChar == '\n'); }

    // 判断是否为字母和注释，注意 '_'
    bool isLetter() { return (globalChar >= 'a' && globalChar <= 'z') || (globalChar >= 'A' && globalChar <= 'Z') || (globalChar == '_'); }
    bool isDigit() { return globalChar >= '0' && globalChar <= '9'; }

    // 判断是否为 ' "
    bool isSquote() { return (globalChar == '\'');}
    bool isQuote() { return (globalChar == '\"');}

    // 判断是否为 加号+ 减号- 乘号* 除号/ 小于< 大于> 等于= 叹号!
    bool isPlus() { return globalChar == '+';}
    bool isMinus() { return globalChar == '-';}
    bool isStar() { return globalChar == '*';}
    bool isDivi() { return globalChar == '/';}
    bool isLess() { return globalChar == '<';}
    bool isMore() { return globalChar == '>';}
    bool isEqu() { return globalChar == '=';}
    bool isNot() { return globalChar == '!';}

    // 判断是否为 逗号, 分号;
    bool isComma() { return globalChar == ',';}
    bool isSemi() { return globalChar == ';';}

    // 判断是否为左右括号 () [] {}
    bool isLpar() { return globalChar == '(';}
    bool isRpar() { return globalChar == ')';}
    bool isLbrack() { return globalChar == '[';}
    bool isRbrack() { return globalChar == ']';}
    bool isLbrace() { return globalChar == '{';}
    bool isRbrace() { return globalChar == '}';}

    // 词法分析错误处理
    bool isIllegalChar();
};


#endif //MYCOMPILER_LEXICALANALYSIS_H
