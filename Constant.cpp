//
// Created by Klaus-Mikealson on 2019/9/28.
//

#include "Constant.h"
#include <string>
#include "MIPSGenerator.h"

using namespace std;

/* 全局开关 */
const bool optimize = true;

/* 词法分析相关 */
ofstream outfile("output.txt");

const int TYPENUM = 36;  // 类型数量
const int KWDNUM = 13;

const string symbol[TYPENUM] = {
        // 标识符与变量值 - 4
        "IDENFR", "INTCON", "CHARCON", "STRCON",
        // const     int      char      void      main     if       else      do      while      for      scanf      printf      return  - 13
        "CONSTTK", "INTTK", "CHARTK", "VOIDTK", "MAINTK", "IFTK", "ELSETK", "DOTK", "WHILETK", "FORTK", "SCANFTK", "PRINTFTK", "RETURNTK",
        // +      -       *      /      <      <=      >     >=     ==     !=        =         ;        ,    - 13
        "PLUS", "MINU", "MULT", "DIV", "LSS", "LEQ", "GRE", "GEQ", "EQL", "NEQ", "ASSIGN", "SEMICN", "COMMA",
        // ()                    []                  {}              - 6
        "LPARENT", "RPARENT", "LBRACK", "RBRACK", "LBRACE", "RBRACE"
};

// 保留字， i+4 即与symbol一一对应
const string keyword[KWDNUM] = {
        "const", "int", "char", "void", "main", "if", "else", "do", "while", "for", "scanf", "printf", "return"
};

/* 语法分析相关 */
string GLOBAL("global");
string MAIN("main");

/* 错误处理 */
string errorFilePath("error.txt");

/* 中间代码生成部分 */
string ENTER("enter");
string NOTHING("nothing");
//string tmpCodeFilePath("17373489_张佳一_优化前中间代码.txt");
string tmpCodeFilePath("17373489_zjy_tmpCode.txt");
//string opedTmpCodefFilePath("17373489_张佳一_优化后中间代码.txt");
string opedTmpCodefFilePath("17373489_zjy_opedTmpCode.txt");

/* 目标代码生成部分 */
string mipsFilePath("mips.txt");

/* 代码优化 */
const int regs[15] = {t1, t2, t3, t4, t5, t6, t7,
                      s0, s1, s2, s3, s4, s5, s6, s7};
const int inlineRegs[3] = {a1, a2, a3};
