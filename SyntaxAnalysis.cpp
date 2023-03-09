//
// Created by Klaus-Mikealson on 2019/10/5.
//

#include "Error.h"
#include "Constant.h"
#include "SyntaxAnalysis.h"
#include "LexicalAnalysis.h"

#include <string>

using namespace std;

SyntaxAnalysis::SyntaxAnalysis(LexicalAnalysis &myLexical, Error &myError, SymTableMngr &mySymTableMngr, SemanticAnalysis &mySemantic):
myLexical(myLexical), myError(myError), mySymTableMngr(mySymTableMngr), mySemantic(mySemantic)
{
    // init
}

SyntaxAnalysis::~SyntaxAnalysis() {}

/* Main function */
void SyntaxAnalysis::analysis()
{
    myLexical.nextChar();
    storeAndPreRead();
    program();
}

/* 辅助叶子函数 */
void SyntaxAnalysis::storeAndPreRead() {
    myLexical.saveCurrentState();
    myLexical.nextSym(false);
}

void SyntaxAnalysis::restoreAndPrint() {
    myLexical.restoreLastState();
    myLexical.nextSym(true);
}

/* 超前扫描鉴别语法类别函数
 * 注意:
 *  - 保证不会输出任何值到文件流
 *  - 会使 symbol 状态回退到上一次预读之前*/
// 超前扫描鉴别 <变量说明> 与 <有返回值函数的定义>
bool SyntaxAnalysis::isVariableDefinition() {
    // 此时 globalSymbol 应为 int | char
    myLexical.nextSym(false);   // 标识符
    myLexical.nextSym(false);   // 若为函数，则 (  若为变量定义，则 [ 或 ;

    if (myLexical.getGlobalSymbol() == LPARENT) {
        // <有返回值的函数定义>
        myLexical.restoreLastState();
        return false;
    } else {
        // <变量定义>
        myLexical.restoreLastState();
        return true;
    }
}
// 超前扫描鉴别 <主函数>   与 <无返回值函数的定义>
bool SyntaxAnalysis::isMainFunction() {
    // 此时 globalSymbol 应为 void
    myLexical.nextSym(false);   // 标识符
    if (myLexical.getGlobalToken().compare(MAIN) == 0) {
        myLexical.restoreLastState();
        return true;
    } else {
        myLexical.restoreLastState();
        return false;
    }
}
// 超前扫描鉴别 标识符、标识符[<表达式>] 与 有返回值函数调用语句 - 标识符(<值参数表>)
int SyntaxAnalysis::scanFactor() {
    // 此时 globalSymbol 应为 identifier
    myLexical.nextSym(false);   // 扫描下一个符号

    int currentSymbol = myLexical.getGlobalSymbol();
    switch (currentSymbol) {
        case LBRACK:    // 标识符[<表达式>]
            myLexical.restoreLastState();
            storeAndPreRead();
            return 1;
        case LPARENT:   // 有返回值函数调用语句 - 标识符(<值参数表>)
            myLexical.restoreLastState();
            storeAndPreRead();
            return 2;
        default:        // 标识符
            myLexical.restoreLastState();
            storeAndPreRead();
            return 0;
    }
}

/* 语法分析错误处理 */
void SyntaxAnalysis::semicolon() {
#ifdef ERROR
    // 判断结尾是否有分号
    if (myLexical.getGlobalSymbol() == SEMICN) {
        restoreAndPrint();      // 输出 ;
        storeAndPreRead();      // 预读下一个符号
    } else {
        // 错误处理
        myLexical.restoreLastState();   // 防止行号出错
        myError.SyntaxError(lackSemi, myLexical.getCurLineNum());
        storeAndPreRead();      // 预读下一个符号
    }
#else
    restoreAndPrint();      // 输出 ;
    storeAndPreRead();      // 预读下一个符号
#endif
}
void SyntaxAnalysis::rightPar() {
#ifdef ERROR
    // 判断是否有右括号 )
    if (myLexical.getGlobalSymbol() == RPARENT) {
        restoreAndPrint();      // 输出 )
        storeAndPreRead();      // 预读下一个符号
    } else {
        // 错误处理
        myLexical.restoreLastState();   // 防止行号出错
        myError.SyntaxError(lackRpar, myLexical.getCurLineNum());
        storeAndPreRead();      // 预读下一个符号
    }
#else
    restoreAndPrint();      // 输出 )
    storeAndPreRead();      // 预读下一个符号
#endif
}
void SyntaxAnalysis::rightBrack() {
#ifdef ERROR
    // 判断是否有右中括号 ]
    if (myLexical.getGlobalSymbol() == RBRACK) {
        restoreAndPrint();      // 输出 ]
        storeAndPreRead();      // 预读下一个符号
    } else {
        // 错误处理
        myLexical.restoreLastState();   // 防止行号出错
        myError.SyntaxError(lackRbrack, myLexical.getCurLineNum());
        storeAndPreRead();      // 预读下一个符号
    }
#else
    restoreAndPrint();      // 输出 ]
    storeAndPreRead();      // 预读下一个符号
#endif
}
void SyntaxAnalysis::skipUntilEndSym(SymbolType rightEndSymbol) {
    while (myLexical.getGlobalSymbol() != rightEndSymbol) {
        myLexical.nextSym(false);
    }
}

/*
 * 表达式类型为char型有且仅有以下三种情况：
 * （１）表达式由<标识符>或＜标识符＞'['＜表达式＞']构成，且<标识符>的类型为char，即char类型的常量和变量、char类型的数组元素。
 * （２）表达式仅由一个<字符常量>构成。
 * （３）表达式仅由一个有返回值函数调用构成，且该被调用的有返回函数返回值为char型
 *  即，<表达式> ::*= <标识符> | <有返回值函数调用语句> | <字符常量>，
 *  该标识符在作用域内为char类型，有返回值调用语句返回char型，:*=表示经过多步推导
 */


/* 语义分析辅助函数 */
// 表达式 - 因子
void SyntaxAnalysis::genStdFourTuple(SymbolType type) {
    /* 生成中间代码 */
    string src2 = s.top();  s.pop();
    string src1 = s.top();  s.pop();
    /* 常量优化 */
    SymTableItem src2Item = mySymTableMngr.getIdfrItem(src2);
    SymTableItem src1Item = mySymTableMngr.getIdfrItem(src1);
    if (src1Item.isConstant() && src2Item.isConstant()) {
        int src1Value = src1Item.getConstValue();
        int src2Value = src2Item.getConstValue();
        int resValue;
        switch (type) {
            case PLUS:
                resValue = src1Value + src2Value;
                break;
            case MINU:
                resValue = src1Value - src2Value;
                break;
            case MULT:
                resValue = src1Value * src2Value;
                break;
            case DIV:
                resValue = src1Value / src2Value;
                break;
            default:
                break;
        }
        string constName = mySymTableMngr.genTempCon(INTTK, resValue);
        s.push(constName);
        return;
    }

    Operator op;
    switch (type) {
        case PLUS:
            op = ADDOP;
            break;
        case MINU:
            op = SUBOP;
            break;
        case MULT:
            op = MULOP;
            break;
        case DIV:
            op = DIVOP;
            break;
        default:
            break;
    }

    string tempName = mySymTableMngr.genTempVar(INTTK); // 生成临时变量名并将其加入符号表中
    mySemantic.varDefState(tempName);                         // 为临时变量生成四元式
    mySemantic.exprState(op, tempName, src1, src2);   // 为<表达式语句>生成四元式
    s.push(tempName);                                 // 将“计算后”的结果压栈
}
void SyntaxAnalysis::genUseArrFourTuple(string arrName) {
    // 获取数组类型
    SymbolType type = SymbolType (mySymTableMngr.getIdfrItem(arrName).getType());
    // 处理数组元素  t0 = a[index]
    string index = s.top(); s.pop();            // 下标名
    string tempName = mySymTableMngr.genTempVar(type);      // 生成中间变量并将其加入符号表中
    mySemantic.varDefState(tempName);                       // 为中间变量生成四元式
    mySemantic.useArr(tempName, arrName, index);            // 为useArray生成四元式
    s.push(tempName);                           // 将"计算后"的结果t0压栈
}
// 赋值语句
void SyntaxAnalysis::genAssignFourTuple(string dest, string index) {
    // dest = src1   or   dest[src2] = src1
    string src1 = s.top(); s.pop();
    /* 常量传播 */
    /*
    SymTableItem src1Item = mySymTableMngr.getIdfrItem(src1);
    if (src1Item.isConstant()) {
        int constValue = src1Item.getConstValue();
        mySymTableMngr.addConstant(dest, (SymbolType) src1Item.getType(), constValue);
        return;
    }
    */
    bool isArr = index.compare("")!=0;
    if (isArr) {
        mySemantic.assArr(dest, src1, index);
    } else {
        mySemantic.assVar(dest, src1);
    }
}
// 控制语句
void SyntaxAnalysis::genConditionalFourTuple(SymbolType type, bool relationalOp) {
    if (!relationalOp) {
        // 若为单个表达式
        string src1 = s.top(); s.pop();
        mySemantic.conditionState(SEXPR, src1);
        return;
    }
    // 若为关系运算, 此时栈中应该正好剩下左右两个expr运算后的结果
    string src2 = s.top(); s.pop();
    string src1 = s.top(); s.pop();
    switch (type) {
        //symbol == LSS || symbol == LEQ || symbol == GRE || symbol == GEQ || symbol == NEQ || symbol == EQL
        case LSS:
            mySemantic.conditionState(LESS, src1, src2);
            break;
        case LEQ:
            mySemantic.conditionState(LEQL, src1, src2);
            break;
        case GRE:
            mySemantic.conditionState(MORE, src1, src2);
            break;
        case GEQ:
            mySemantic.conditionState(MEQL, src1, src2);
            break;
        case NEQ:
            mySemantic.conditionState(NEQUAL, src1, src2);
            break;
        case EQL:
            mySemantic.conditionState(EQUAL, src1, src2);
            break;
        default:
            break;
    }
}
// 函数调用语句
void SyntaxAnalysis::genHaveRetValFourTuple(string funcName) {
    // 处理有返回值函数调用语句  t0 = func(x, y)
    // call func
    // t0 = RET
    SymbolType type = SymbolType (mySymTableMngr.getFuncRetValType(funcName));// 函数返回值可能为字符型
    string tempName = mySymTableMngr.genTempVar(type);  // 生成中间变量并将其加入符号表中
    mySemantic.varDefState(tempName);                   // 为中间变量生成四元式
    mySemantic.haveRetFunCallState(tempName, funcName); // 为funCall生成四元式
    s.push(tempName);                                   // 将“计算后”的结果压栈
}
void SyntaxAnalysis::genNonRetValFourTuple(string funcName) {
    // 处理无返回值函数调用语句  func(x, y)
    mySemantic.nonRetFunCallState(funcName);    // 为funCall生成四元式
}
void SyntaxAnalysis::genPushFourTuple(string funcName, int num) {
    // 处理值参数的push语句
    string valuePara = s.top(); s.pop();
    mySemantic.push(valuePara, num, funcName);
}
// 写语句
void SyntaxAnalysis::genPrintStrFourTuple(string str) {
    // 处理打印字符串
    string strName = mySymTableMngr.genStrCon(str);
    mySemantic.printStrState(strName);
}
void SyntaxAnalysis::genPrintExprFourTuple(ExprType exprType) {
    // 处理打印表达式
    string idfr = s.top();  s.pop();
    mySemantic.printExprState(idfr, exprType);
}
// 返回语句
void SyntaxAnalysis::genRetValFourTuple() {
    // 处理有返回值函数的返回语句
    string dest = s.top(); s.pop();
    mySemantic.retValState(dest);
}

/* 递归下降子程序分析 */
// <程序>    ::= ［<常量说明>］［<变量说明>］{<有返回值函数定义>|<无返回值函数定义>}<主函数>
void SyntaxAnalysis::program()
{
    int symbol;
    // 1. 常量说明, 0-1
    symbol = myLexical.getGlobalSymbol();
    if (symbol == CONSTTK) {
        constantDescription();
        symbol = myLexical.getGlobalSymbol();
    }
    // 2. 变量说明, 0-1
    if (symbol == INTTK || symbol == CHARTK) {
        if (isVariableDefinition()) {
            variableDescription();
            symbol = myLexical.getGlobalSymbol();
        }
    }
    // 3. <有返回值函数定义>|<无返回值函数定义>, 0-n
    while (symbol == INTTK || symbol == CHARTK || symbol == VOIDTK) {
        if (symbol == VOIDTK) {
            if (isMainFunction()) { break; }
            nonReturnValueFunctionDefinition();
        } else{
            haveReturnValueFunctionDefinition();
        }
        symbol = myLexical.getGlobalSymbol();
    }
    // 4. main函数
    mainFunction();

    outfile << "<程序>" << endl;
}

/*
 * 1.［<常量说明>］- 2
 */
// <常量说明> ::=  const<常量定义>;{ const<常量定义>;}
void SyntaxAnalysis::constantDescription()
{
    do {
        restoreAndPrint();        // 输出 const
        storeAndPreRead();
        constantDefinition();
        semicolon();
    } while (myLexical.getGlobalSymbol() == CONSTTK);

    outfile << "<常量说明>" << endl;
}
// <常量定义>   ::=   int<标识符>＝<整数>{,<标识符>＝<整数>} | char<标识符>＝<字符>{,<标识符>＝<字符>}
void SyntaxAnalysis::constantDefinition()
{
    if (myLexical.getGlobalSymbol() == INTTK) {
        do {
            restoreAndPrint();    // 输出 int 或者 ,
            storeAndPreRead();    // 预读标识符
#ifdef ERROR
            // 检查是否重定义
            if (mySymTableMngr.isIdfrRedefine(myLexical.getGlobalToken())) {
                myError.SemanticError(idfrReDefine, myLexical.getCurLineNum());
                skipUntilEndSym(INTCON);
                storeAndPreRead();
                continue;
            }
#endif
            identifier();
            restoreAndPrint();    // 输出 =
            storeAndPreRead();
            int numVal = integer();

            // 填入符号表
            mySymTableMngr.addConstant(lastIdfr, INTTK, numVal);
            // 生成中间代码
            mySemantic.constDefState(lastIdfr);
        } while (myLexical.getGlobalSymbol() == COMMA);
    } else if (myLexical.getGlobalSymbol() == CHARTK) {
        do {
            restoreAndPrint();    // 输出 char 或者 ,
            storeAndPreRead();    // 预读标识符
#ifdef ERROR
            // 检查是否重定义
            if (mySymTableMngr.isIdfrRedefine(myLexical.getGlobalToken())) {
                myError.SemanticError(idfrReDefine, myLexical.getCurLineNum());
                skipUntilEndSym(CHARCON);
                storeAndPreRead();
                continue;
            }
#endif
            identifier();
            restoreAndPrint();  // 输出 =
            storeAndPreRead();
            int charVal = character();
            // 填入符号表
            mySymTableMngr.addConstant(lastIdfr, CHARTK, charVal);
            // 生成中间代码
            mySemantic.constDefState(lastIdfr);
        } while (myLexical.getGlobalSymbol() == COMMA);
    } else {
        cout << "Error in <常量定义>, read in unknown character: " << myLexical.getGlobalSymbol() << endl;
        exit(0);
    }
    outfile << "<常量定义>" << endl;
}

/*
 * 2.［<变量说明>］ - 2
 */
// <变量说明>  ::= <变量定义>;{<变量定义>;}
void SyntaxAnalysis::variableDescription()
{
    // 仅凭第一个符号无法判断是 <变量说明><还是有返回值的函数定义>, 故需超前扫描
    do {
        /*
         * 跳出循环有两种可能
         * 1. int | char 有返回值函数定义，需要判断是否为变量定义，但内部会恢复状态，故需要再次超前扫描
         * 2. void 无返回值函数或主函数，此时是从while出直接跳出，不会恢复，故无需再次超前扫描
         */
        // 首先判断是否为变量定义
        if (!isVariableDefinition()) {
            storeAndPreRead();  // 前面的scanForward被isVariable中恢复了，需要再超前扫描一次
            break;
        }
        variableDefinition();
        semicolon();
    } while (myLexical.getGlobalSymbol() == INTTK || myLexical.getGlobalSymbol() == CHARTK);

    outfile << "<变量说明>" << endl;
}
// <变量定义>  ::= <类型标识符>(<标识符>|<标识符>'['<无符号整数>']'){,(<标识符>|<标识符>'['<无符号整数>']' )}
//                                                                        <无符号整数>表示数组元素的个数，其值需大于0
void SyntaxAnalysis::variableDefinition()
{
    typeIdentifier();       // 类型标识符
    do {
        if (myLexical.getGlobalSymbol() == COMMA) {
            restoreAndPrint();        // 输出 ,
            storeAndPreRead();
        }
        // 由于超前扫描，此时已经读入 标识符
#ifdef ERROR
        // 检查是否重定义
        if (mySymTableMngr.isIdfrRedefine(myLexical.getGlobalToken())) {
            myError.SemanticError(idfrReDefine, myLexical.getCurLineNum());
            storeAndPreRead();    // 跳过标识符，预读 ,  或 [
            // 检查是否为数组重定义的情况
            if (myLexical.getGlobalSymbol() == LBRACK) {
                skipUntilEndSym(RBRACK);
                storeAndPreRead();
            }
            continue;
        }
#endif
        identifier();       // 标识符
        if (myLexical.getGlobalSymbol() == LBRACK) {
            // 若是数组
            restoreAndPrint();    // 输出 [
            storeAndPreRead();          // 读无符号整数
            int length = myLexical.getGlobalNum();  // 记录当前数字
            unsignedInteger();
            rightBrack();
            // 填符号表
            mySymTableMngr.addArray(lastIdfr, lastIdfrType, 1, length);
            // 生成中间代码
            mySemantic.arrDefState(lastIdfr);
        } else {
            // 简单变量 - 填符号表
            mySymTableMngr.addVariable(lastIdfr, lastIdfrType);
            // 生成中间代码
            mySemantic.varDefState(lastIdfr);
        }
    } while (myLexical.getGlobalSymbol() == COMMA);

    outfile << "<变量定义>" << endl;
}
/*
 * 3. {<有返回值函数定义>|<无返回值函数定义>} - 4
 */
// <有返回值函数定义>  ::=  <声明头部>'('<参数表>')' '{'<复合语句>'}'
void SyntaxAnalysis::haveReturnValueFunctionDefinition()
{
    string funcName = declarationHeader();     // 声明头部

    restoreAndPrint();      // 输出 (
    storeAndPreRead();
    parameterTable();       // 参数表
    //restoreAndPrint();    // 输出 )
    rightPar();

    restoreAndPrint();      // 输出 {
    storeAndPreRead();
    compoundStatement();    // 复合语句
    restoreAndPrint();    // 输出 }
#ifdef ERROR
    // 错误处理，判断该函数定义是否有返回值
    if (!mySymTableMngr.getHaveRetStatement()) {
        myError.retValueError(lackReturnStatement, myLexical.getCurLineNum());
    }
#endif
    // 生成中间代码
    mySemantic.funcDefEnd(funcName);
    storeAndPreRead();
    outfile << "<有返回值函数定义>" << endl;
}
// <无返回值函数定义>  ::= void<标识符>'('<参数表>')''{'<复合语句>'}'
void SyntaxAnalysis::nonReturnValueFunctionDefinition()
{
    restoreAndPrint();    // 输出 VOIDTK
    storeAndPreRead();    // 扫描下一个标识符
    identifier();           // 标识符
    string funcName = lastIdfr;
#ifdef ERROR
    // 检查是否重定义
    if (mySymTableMngr.isFuncRedefine(lastIdfr)) {
        // 若函数名重定义，则进行错误局部化处理
        myError.SemanticError(idfrReDefine, myLexical.getCurLineNum());
        // 仍然将其加入符号表，只是改个名字，以便对函数内部进行正常解析，又不会影响原表
        // 因为这个名字是我自己生成的，故后面一定不会再用这个 identifier
        string tmp("_tmp");
        lastIdfr = lastIdfr + tmp;
    }
#endif
    // 填符号表
    mySymTableMngr.createScope(lastIdfr, VOIDTK);
    // 生成中间代码
    mySemantic.funcDefBegin(funcName, VOIDTK);

    restoreAndPrint();    // 输出 (
    storeAndPreRead();
    parameterTable();       // 参数表
    //restoreAndPrint();    // 输出 )
    rightPar();

    restoreAndPrint();      // 输出 {
    storeAndPreRead();
    compoundStatement();    // 复合语句
    restoreAndPrint();      // 输出 }
    // 生成中间代码
    mySemantic.funcDefEnd(funcName);

    storeAndPreRead();
    outfile << "<无返回值函数定义>" << endl;
}
// <声明头部>   ::=  int<标识符> | char<标识符>
string SyntaxAnalysis::declarationHeader()
{
    typeIdentifier();       // 输出类型标识符，并扫描后面的标识符
    identifier();           // 标识符
    string funcName = lastIdfr;
    SymbolType retType = lastIdfrType;
#ifdef ERROR
    // 检查是否重定义
    if (mySymTableMngr.isFuncRedefine(lastIdfr)) {
        // 若函数名重定义，则进行错误局部化处理
        myError.SemanticError(idfrReDefine, myLexical.getCurLineNum());
        // 仍然将其加入符号表，只是改个名字，以便对函数内部进行正常解析，又不会影响原表
        // 因为这个名字是我自己生成的，故后面一定不会再用这个 identifier
        string tmp("_tmp");
        lastIdfr = lastIdfr + tmp;
    }
#endif
    // 填符号表
    mySymTableMngr.createScope(lastIdfr, lastIdfrType);
    // 生成中间代码
    mySemantic.funcDefBegin(funcName, retType);

    outfile << "<声明头部>" << endl;
    return funcName;
}
// <参数表>    ::=  <类型标识符><标识符>{,<类型标识符><标识符>}| <空>
void SyntaxAnalysis::parameterTable()
{
#ifdef ERROR
    // 错误处理，防止 参数表为空且缺少右括号，即void func( {} 这种情况发生
    if (myLexical.getGlobalSymbol() == LBRACE) {
        // 若读到 { , 说明缺少 ) , 直接跳出
        return;
    }
#endif
    if (myLexical.getGlobalSymbol() != RPARENT) {
        // 说明参数表不为空，进一步判断
        do {
            if (myLexical.getGlobalSymbol() == COMMA) {
                restoreAndPrint();    // 输出 ,
                storeAndPreRead();    // 扫描 int | char
            }
            typeIdentifier();   // 类型标识符
            identifier();       // 由于超前扫描，已读入 标识符，即变量名

            // 填符号表
            mySymTableMngr.addParam(lastIdfr, lastIdfrType);
            // 生成中间代码
            mySemantic.paraDefState(lastIdfr);
        } while (myLexical.getGlobalSymbol() == COMMA);
    }
    outfile << "<参数表>" << endl;
}

/*
 * 4. <主函数> - 4
 */
// <主函数>    ::= void main‘(’‘)’ ‘{’<复合语句>‘}’
void SyntaxAnalysis::mainFunction()
{
    restoreAndPrint();        // 输出 VOIDTK
    myLexical.nextSym(true);    // 输出 main
    mySymTableMngr.createScope(MAIN, VOIDTK);
    myLexical.nextSym(true);    // 输出 (
    storeAndPreRead();      // 预读是否有 )
    rightPar();

    // 生成中间代码
    mySemantic.funcDefBegin(MAIN, VOIDTK);

    restoreAndPrint();      // 输出 {
    storeAndPreRead();
    compoundStatement();    // 复合语句
    restoreAndPrint();      // 输出 }

    // 更新符号表
    mySymTableMngr.insertCurScope();    // 将main插入局部域中
    // 生成中间代码
    mySemantic.funcDefEnd(MAIN);
    outfile << "<主函数>" << endl;
}
// <表达式>    ::= ［＋｜－］<项>{<加法运算符><项>}   //[+|-]只作用于第一个<项>
int SyntaxAnalysis::expression()
{
    int res = NONEXPR;
    if (myLexical.getGlobalSymbol() == PLUS || myLexical.getGlobalSymbol() == MINU) {
        res = INTEXPR;
        //restoreAndPrint();          // 输出 + | -
        //storeAndPreRead();
        /* 生成中间代码 */
        // 1. 插入前导0
        myLexical.insertZero();
        // 2. 手动执行 item()
        string constName = mySymTableMngr.genTempCon(INTTK, 0);
        stringstream ss;    ss << constName;    s.push(ss.str());
        goto relationalOp;
    }
    res += item();
    relationalOp:
    while (myLexical.getGlobalSymbol() == PLUS || myLexical.getGlobalSymbol() == MINU)
    {
        SymbolType addOp = SymbolType(myLexical.getGlobalSymbol());
        addOperator();
        item();

        /* 生成中间代码 */
        genStdFourTuple(addOp);
        res = INTEXPR;
    }
    outfile << "<表达式>" << endl;
    return res;
}
// <项>     ::= <因子>{<乘法运算符><因子>}
int SyntaxAnalysis::item()
{
    int res = factor();
    while (myLexical.getGlobalSymbol() == MULT || myLexical.getGlobalSymbol() == DIV)
    {
        SymbolType multOp = SymbolType(myLexical.getGlobalSymbol());
        multiOperator();
        factor();

        /* 生成中间代码 */
        genStdFourTuple(multOp);
        res = INTEXPR;
    }
    outfile << "<项>" << endl;
    return res;
}
// <因子>    ::= <标识符>｜<标识符>'['<表达式>']'|'('<表达式>')'｜<整数>|<字符>｜<有返回值函数调用语句>
int SyntaxAnalysis::factor()
{
    int res = NONEXPR;
    if (myLexical.getGlobalSymbol() == LPARENT) {
        // '('<表达式>')'
        restoreAndPrint();    // 输出 (
        storeAndPreRead();
        expression();
        rightPar();
        res = INTEXPR;
    } else if (myLexical.getGlobalSymbol() == INTCON || myLexical.getGlobalSymbol() == PLUS || myLexical.getGlobalSymbol() == MINU) {
        // <整数>
        int numVal = integer();
        // 生成中间代码
        string constName = mySymTableMngr.genTempCon(INTTK, numVal);
        stringstream ss;    ss << constName;
        s.push(ss.str());
        res = INTEXPR;
    } else if (myLexical.getGlobalSymbol() == CHARCON) {
        // <字符>
        int charVal = character();
        // 生成中间代码
        string constName = mySymTableMngr.genTempCon(CHARTK, charVal);
        stringstream ss;    ss << constName;
        s.push(ss.str());
        res = CHAREXPR;
    } else {    // symbol == IDENFR
        /*
         * 0) 标识符
         * 1) 标识符[<表达式>]
         * 2) 有返回值函数调用语句 - 标识符(<值参数表>)
         */
#ifdef ERROR
        // 错误处理, 且不跳转
        // 如果需要跳转，由于每种情况的右结束符不同，需要对每种情况特判
        if (mySymTableMngr.isIdfrUndefine(myLexical.getGlobalToken())) {
            myError.SemanticError(idfrNotDefine, myLexical.getCurLineNum()); }
#endif
        string idfrName = myLexical.getGlobalToken();    // 保存标识符名
        int sonExprType = NONEXPR;

        if (mySymTableMngr.isIdfrSym(idfrName)) {
            if (mySymTableMngr.isIdfrArray(idfrName)) {
                /* 标识符[<表达式>] */
                // 错误处理, char型表达式判断
                if (mySymTableMngr.isIdfrCharType(idfrName)) { res = CHAREXPR; }
                else { res = INTEXPR; }

                identifier();
                restoreAndPrint();       // 输出 [
                storeAndPreRead();
                sonExprType = expression(); // 表达式
#ifdef ERROR
                if (sonExprType == CHAREXPR) {
                    myError.SemanticError(illegalArrayIndex, myLexical.getCurLineNum()); }
#endif
                rightBrack();
                /* 生成中间代码 - 数组元素 */
                genUseArrFourTuple(idfrName);
            } else {
                /* 标识符 */
                // 错误处理, char型表达式判断
                if (mySymTableMngr.isIdfrCharType(idfrName)) { res = CHAREXPR; }
                else { res = INTEXPR; }

                identifier();
                s.push(idfrName);
            }
        } else if (mySymTableMngr.isHaveRetValueFunc(idfrName)) {
            /* 有返回值函数调用语句 - 标识符(<值参数表>) */
            // 错误处理, char型表达式判断
            if (mySymTableMngr.getFuncRetValType(idfrName) == CHARTK) { res = CHAREXPR; }
            else { res = INTEXPR; }

            haveReturnValueFunctionCallStatement();
        } else {}

    }
    outfile << "<因子>" << endl;
    return res;
}

/*
 * 5. 语句相关 - 14
 */
// <复合语句>   ::=  ［<常量说明>］［<变量说明>］<语句列>
void SyntaxAnalysis::compoundStatement()
{
    if (myLexical.getGlobalSymbol() == CONSTTK) {
        constantDescription();
    }
    if (myLexical.getGlobalSymbol() == INTTK || myLexical.getGlobalSymbol() == CHARTK) {
        variableDescription();
    }
    statementSeries();      // 语句列
    outfile << "<复合语句>" << endl;
}
// <语句>    ::= <条件语句>｜<循环语句>| '{'<语句列>'}'| <赋值语句>;
//                   1          2            3            4
//               |<有返回值函数调用语句>;｜<无返回值函数调用语句>;｜<读语句>;｜<写语句>;|<返回语句>;｜<空>;
//                          5                      6               7         8         9        10
int SyntaxAnalysis::statement()
{
    int symbol = myLexical.getGlobalSymbol();
    // 条件语句 - if
    if (symbol == IFTK) {
        // 注意条件语句与循环语句结尾可能是 } 或 ;  故在内部进行处理，在此处无需输出 ;
        // 对于条件语句来说，内部即为在 <语句> 中处理，上层无需考虑
        conditionalStatement();
        outfile << "<语句>" << endl;
        return 1;
    }
    // 循环语句 - do, while, for
    else if (symbol == DOTK || symbol == WHILETK || symbol == FORTK) {
        // 注意条件语句与循环语句结尾可能是 } 或 ;  故在内部进行处理，在此处无需输出 ;
        // 对于循环语句来说，do-while 与 while, for 两者不同
        //         - do-while 题目给定的文法与C的文法不同，以 ) 结尾，故无需输出 ;   (因而后跟的 ; 将当作空语句处理)
        //         - while 与 for 均以 <语句> 结尾，故 } 或 ; 在 <语句> 内部处理，上层无需考虑
        loopStatement();
        outfile << "<语句>" << endl;
        return 2;
    }
    // 语句列   - {
    else if (symbol == LBRACE) {
        // 注意语句列结尾是 }  故在此处需输出 }
        restoreAndPrint();    // 输出 {

        storeAndPreRead();          // 预读下个语句首字符
        statementSeries();
        restoreAndPrint();    // 输出 }
        storeAndPreRead();
        outfile << "<语句>" << endl;
        return 3;
    }
    // 有返回值函数调用语句   - 标识符
    // 无返回值函数调用语句   - 标识符
    // 赋值语句 - 标识符
    else if(symbol == IDENFR) {
        /**
         * 可优化
         */
#ifdef ERROR
        // 错误处理, 且不跳转
        if (mySymTableMngr.isIdfrUndefine(myLexical.getGlobalToken())) {
            myError.SemanticError(idfrNotDefine, myLexical.getCurLineNum());
            skipUntilEndSym(SEMICN);    // 跳转到语句末 ;
            storeAndPreRead();                         // 预读下一个语句开头
            return -1;
        }
#endif
        // 赋值语句 - <标识符>＝<表达式>|<标识符>'['<表达式>']'=<表达式>
        if (mySymTableMngr.isIdfrSym(myLexical.getGlobalToken())) {
#ifdef ERROR
            // 错误处理, 判断是否是常量
            if (mySymTableMngr.isIdfrConst(myLexical.getGlobalToken())) {
                myError.SemanticError(modifyConstant, myLexical.getCurLineNum());
                //skipUntilEndSym(SEMICN);
                //return -1;
            }
#endif
            assignmentStatement();
            semicolon();

            outfile << "<语句>" << endl;
            return 4;
        }
        // 有返回值函数调用语句   - <标识符>'('<值参数表>')'
        else if (mySymTableMngr.isHaveRetValueFunc(myLexical.getGlobalToken())) {
            haveReturnValueFunctionCallStatement();
            semicolon();

            outfile << "<语句>" << endl;
            return 5;
        }
        // 无返回值函数调用语句   - <标识符>'('<值参数表>')'
        else if (mySymTableMngr.isNonRetValueFunc(myLexical.getGlobalToken())) {
            nonReturnValueFunctionCallStatement();
            semicolon();

            outfile << "<语句>" << endl;
            return 6;
        } else {

        }
    }
    // 读语句   - scanf
    else if (symbol == SCANFTK) {
        readStatement();
        semicolon();

        outfile << "<语句>" << endl;
        return 7;
    }
    // 写语句   - printf
    else if (symbol == PRINTFTK) {
        writeStatement();
        semicolon();

        outfile << "<语句>" << endl;
        return 8;
    }
    // 返回语句 - return
    else if (symbol == RETURNTK) {
        returnStatement();
        semicolon();

        outfile << "<语句>" << endl;
        return 9;
    }
    // 空
    else if (symbol == SEMICN){
        // 注意空语句为单独一个 ;  故在此处需输出 ;
        semicolon();

        outfile << "<语句>" << endl;
        return 10;
    } else {
        return 0;
    }
}
// <赋值语句>   ::=  <标识符>＝<表达式>|<标识符>'['<表达式>']'=<表达式>
void SyntaxAnalysis::assignmentStatement()
{
    identifier();               // <标识符>
    string dest = lastIdfr;
    string index;

    if (myLexical.getGlobalSymbol() == LBRACK) {
        // <标识符>'['<表达式>']'=<表达式>
        restoreAndPrint();        // 输出 [
        storeAndPreRead();

        int exprType = expression();
#ifdef ERROR
        if (exprType == CHAREXPR) {
            myError.SemanticError(illegalArrayIndex, myLexical.getCurLineNum());
        }
#endif
        rightBrack();
        /* 生成中间代码 */
        index = s.top(); s.pop();
    }
    restoreAndPrint();            // 输出 =
    storeAndPreRead();
    expression();
    /* 生成中间代码 */
    genAssignFourTuple(dest, index);

    outfile << "<赋值语句>" << endl;
}
// <条件语句>  ::= if '('<条件>')'<语句>［else<语句>］
void SyntaxAnalysis::conditionalStatement()
{
    /* ** 生成中间代码 ** */
    mySemantic.controlState(IF_BEGIN);      // 标记if开始
    int ifNum = mySemantic.getIfNum();          // 生成后缀
    string if_x = mySemantic.genCtrlLabelName(IF_BEGIN, ifNum); // 生成 if_x
    string else_x = mySemantic.genCtrlLabelName(ELSE, ifNum);   // 生成 else_x
    string endIf_x = mySemantic.genCtrlLabelName(IF_END, ifNum);// 生成 endIf_x
    //mySemantic.genLabel(if_x);                  // gen if_x
    /* ***************** */

    restoreAndPrint();// 输出 if
    myLexical.nextSym(true);    // 输出 (
    storeAndPreRead();
    condition();      // 条件
    rightPar();       // 输出 )

    /* ** 生成中间代码 ** */
    mySemantic.branch(else_x, true);      // branch else_x
    /* ***************** */

    statement();      // 语句

    /* ** 生成中间代码 ** */
    if (myLexical.getGlobalSymbol() == ELSETK) {
        // 若有else语句, 才Jump
        mySemantic.jump(endIf_x);      // jump endif
    }
    mySemantic.genLabel(else_x);
    /* ***************** */
    if (myLexical.getGlobalSymbol() == ELSETK) {
        restoreAndPrint();    // 输出 else

        storeAndPreRead();
        statement();
    }
    /* ** 生成中间代码 ** */
    mySemantic.genLabel(endIf_x);           // gen endif_x
    mySemantic.controlState(IF_END);    // 标记if结束
    /* ***************** */
    outfile << "<条件语句>" << endl;
}
// <条件>    ::=  <表达式><关系运算符><表达式> //整型表达式之间才能进行关系运算
//               ｜<表达式>    //表达式为整型，其值为0条件为假，值不为0时条件为真
void SyntaxAnalysis::condition()
{
    int exprType = expression();
#ifdef ERROR
    if (exprType == CHAREXPR) {
        myError.SemanticError(illegalCondition, myLexical.getCurLineNum());
    }
#endif
    bool relationOp = false;
    // <关系运算符>  ::=  <｜<=｜>｜>=｜!=｜==
    int symbol = myLexical.getGlobalSymbol();
    if (symbol == LSS || symbol == LEQ || symbol == GRE || symbol == GEQ || symbol == NEQ || symbol == EQL) {
        relationOp = true;
        // 可能会缺少右括号 ) , 还是枚举所有的关系运算符比较稳
        relationalOperator();

        int exprType = expression();
#ifdef ERROR
        if (exprType == CHAREXPR) {
            myError.SemanticError(illegalCondition, myLexical.getCurLineNum());
        }
#endif

    }
    /* 生成中间代码 */
    genConditionalFourTuple(SymbolType (symbol), relationOp);
    outfile << "<条件>" << endl;
}
// <循环语句>   ::=  while '('<条件>')'<语句>|
//                    do<语句>while '('<条件>')' |
//                    for'('<标识符>＝<表达式>;<条件>;<标识符>＝<标识符>(+|-)<步长>')'<语句>
void SyntaxAnalysis::loopStatement()
{
    if (myLexical.getGlobalSymbol() == WHILETK) {
        /* ** 生成中间代码 ** */
        mySemantic.controlState(WHILE_BEGIN);   // 标记while开始
        int whileNum = mySemantic.getWhileNum();    // 生成后缀
        string while_x = mySemantic.genCtrlLabelName(WHILE_BEGIN, whileNum);
        string endWhile_x = mySemantic.genCtrlLabelName(WHILE_END, whileNum);
        mySemantic.genLabel(while_x);               // gen while_x
        /* ***************** */

        // 1. while '('<条件>')'<语句>
        restoreAndPrint();    // 输出 while
        myLexical.nextSym(true);    // 输出 (
        storeAndPreRead();
        condition();
        rightPar();
        /* ** 生成中间代码 ** */
        mySemantic.branch(endWhile_x, true);      // branch endWhile_x
        /* ***************** */
        statement();

        /* ** 生成中间代码 ** */
        mySemantic.jump(while_x);               // jump while_x
        mySemantic.genLabel(endWhile_x);        // gen endWhile_x
        mySemantic.controlState(WHILE_END); // 标记while结束
        /* ***************** */
    } else if (myLexical.getGlobalSymbol() == DOTK) {
        /* ** 生成中间代码 ** */
        mySemantic.controlState(DO_BEGIN);  // 标记do-while开始
        int doNum = mySemantic.getDoNum();      // 生成后缀
        string do_x = mySemantic.genCtrlLabelName(DO_BEGIN, doNum);
        string endDo_x = mySemantic.genCtrlLabelName(DO_END, doNum);
        mySemantic.genLabel(do_x);              // gen do_x
        /* ***************** */
        // 2. do<语句>while '('<条件>')'
        // 注意 do-while 与C的文法不同，其最后一定是 ) 故无需在读 ;
        restoreAndPrint();    // 输出 do
        storeAndPreRead();
        statement();
#ifdef ERROR
        if (myLexical.getGlobalSymbol() != WHILETK) {
            myError.SyntaxError(lackWhileBehindDoTK, myLexical.getCurLineNum());
            // 此时读到了 （
        } else {
            restoreAndPrint();    // 输出 while
            storeAndPreRead();      // 预读 (
        }
#else
        restoreAndPrint();    // 输出 while
        storeAndPreRead();      // 预读 (
#endif
        restoreAndPrint();      // 输出 (
        storeAndPreRead();
        condition();
        rightPar();

        /* ** 生成中间代码 ** */
        mySemantic.branch(do_x, false);
        //mySemantic.genLabel(endDo_x);           // gen endDo_x
        mySemantic.controlState(DO_END);    // 标记do-while结束
        /* ***************** */
    } else if (myLexical.getGlobalSymbol() == FORTK) {
#ifdef ERROR
        /* ** 生成中间代码 ** */
        // 务必注意 for 与 do-while, while 的区别
        mySemantic.controlState(FOR_BEGIN); // 标记for开始
        int forNum = mySemantic.getForNum();    // 生成后缀
        string for_x = mySemantic.genCtrlLabelName(FOR_BEGIN, forNum);
        string endFor_x = mySemantic.genCtrlLabelName(FOR_END, forNum);
        /* ***************** */

        // 3. for'('<标识符>＝<表达式>;<条件>;<标识符>＝<标识符>(+|-)<步长>')'<语句>
        restoreAndPrint();    // 输出 for
        myLexical.nextSym(true);    // 输出 (

        // 3.1
        storeAndPreRead();          // 扫描 标识符
        // 若标识符未定义，则跳转到 ;
        if (mySymTableMngr.isIdfrUndefine(myLexical.getGlobalToken())) {
            myError.SemanticError(idfrNotDefine, myLexical.getCurLineNum());
            skipUntilEndSym(SEMICN);
            storeAndPreRead();
        } else {
            identifier();
            string dest = lastIdfr;
            restoreAndPrint();    // 输出 =
            storeAndPreRead();
            expression();
            // 生成中间代码
            genAssignFourTuple(dest,"");

            semicolon();
        }

        // 3.2
        /* ** 生成中间代码 ** */
        mySemantic.genLabel(for_x);             // gen for_x
        /* ***************** */
        condition();
        semicolon();
        /* ** 生成中间代码 ** */
        mySemantic.branch(endFor_x, true);      // branch endWhile_x
        /* ***************** */

        // 3.3
        string dest;
        // e.g. i = j + 1
        if (mySymTableMngr.isIdfrUndefine(myLexical.getGlobalToken())) {
            myError.SemanticError(idfrNotDefine, myLexical.getCurLineNum());
            // 若 i 未定义，则预读 =
            storeAndPreRead();
        } else {
            identifier();
            /* ** 生成中间代码 ** */
            dest = lastIdfr;
            /* ***************** */
        }
        restoreAndPrint();    // 输出 =
        storeAndPreRead();    // 扫描 标识符
        if (mySymTableMngr.isIdfrUndefine(myLexical.getGlobalToken())) {
            myError.SemanticError(idfrNotDefine, myLexical.getCurLineNum());
            // 若 j 未定义，则预读 + | -
            storeAndPreRead();
        } else {
            identifier();
            /* ** 生成中间代码 ** */
            s.push(lastIdfr);   // src1
            /* ***************** */
        }
        restoreAndPrint();    // 输出 + | -
        SymbolType type = myLexical.getGlobalToken()=="+"? PLUS : MINU;
        storeAndPreRead();    // 扫描无符号整数
        int numVal = stepSize();
        rightPar();
        /* ** 生成中间代码 ** */
        string constName = mySymTableMngr.genTempCon(INTTK, numVal);
        s.push(constName);
        /* ***************** */

        statement();

        /* ** 生成中间代码 ** */
        genStdFourTuple(type);                  // t0 = i + 1
        genAssignFourTuple(dest, "");    // i = t0
        mySemantic.jump(for_x);                 // jump for_x
        mySemantic.genLabel(endFor_x);          // gen endFor_x
        mySemantic.controlState(FOR_END);   // 标记for结束
        /* ***************** */
#else
        /* ** 生成中间代码 ** */
        // 务必注意 for 与 do-while, while 的区别
        mySemantic.controlState(FOR_BEGIN); // 标记for开始
        int forNum = mySemantic.getForNum();    // 生成后缀
        string for_x = mySemantic.genCtrlLabelName(FOR_BEGIN, forNum);
        string endFor_x = mySemantic.genCtrlLabelName(FOR_END, forNum);
        /* ***************** */

        // 3. for'('<标识符>＝<表达式>;<条件>;<标识符>＝<标识符>(+|-)<步长>')'<语句>
        restoreAndPrint();    // 输出 for
        myLexical.nextSym(true);    // 输出 (

        // 3.1
        storeAndPreRead();          // 扫描 标识符
        identifier();
        string dest = lastIdfr;
        restoreAndPrint();    // 输出 =
        storeAndPreRead();
        expression();

        // 生成中间代码
        genAssignFourTuple(dest,"");
        semicolon();
        // 3.2
        /* ** 生成中间代码 ** */
        mySemantic.genLabel(for_x);             // gen for_x
        /* ***************** */
        condition();
        semicolon();
        /* ** 生成中间代码 ** */
        mySemantic.branch(endFor_x, true);      // branch endWhile_x
        /* ***************** */
        // 3.3
        // e.g. i = j + 1
        identifier();
        /* ** 生成中间代码 ** */
        dest = lastIdfr;
        /* ***************** */
        restoreAndPrint();    // 输出 =
        storeAndPreRead();    // 扫描 标识符
        identifier();
        /* ** 生成中间代码 ** */
        s.push(lastIdfr);   // src1
        /* ***************** */
        restoreAndPrint();    // 输出 + | -
        SymbolType type = myLexical.getGlobalToken()=="+"? PLUS : MINU;
        storeAndPreRead();    // 扫描无符号整数
        int numVal = stepSize();
        rightPar();
        /* ** 生成中间代码 ** */
        string constName = mySymTableMngr.genTempCon(INTTK, numVal);
        s.push(constName);
        /* ***************** */
        statement();

        /* ** 生成中间代码 ** */
        genStdFourTuple(type);                  // t0 = i + 1
        genAssignFourTuple(dest, "");    // i = t0
        mySemantic.jump(for_x);                 // jump for_x
        mySemantic.genLabel(endFor_x);          // gen endFor_x
        mySemantic.controlState(FOR_END);   // 标记for结束
        /* ***************** */
#endif
    }
    outfile << "<循环语句>" << endl;
}
// <步长>::= <无符号整数>
int SyntaxAnalysis::stepSize()
{
    int numVal = unsignedInteger();
    outfile << "<步长>" << endl;
    return numVal;
}
// <有返回值函数调用语句> ::= <标识符>'('<值参数表>')'
void SyntaxAnalysis::haveReturnValueFunctionCallStatement()
{
    identifier();
    string funcName = lastIdfr;
    /* 生成中间代码 */
    mySemantic.funCallBegin(funcName);

    restoreAndPrint();      // 输出 (
    storeAndPreRead();
    valueParameterTable(funcName);  // 值参数表
    rightPar();             // 输出 )

    /* 生成中间代码 */
    genHaveRetValFourTuple(funcName);
    mySemantic.funCallEnd(funcName);
    outfile << "<有返回值函数调用语句>" << endl;
}
// <无返回值函数调用语句> ::= <标识符>'('<值参数表>')'
void SyntaxAnalysis::nonReturnValueFunctionCallStatement()
{
    identifier();
    string funcName = lastIdfr;
    /* 生成中间代码 */
    mySemantic.funCallBegin(funcName);

    restoreAndPrint();      // 输出 (
    storeAndPreRead();
    valueParameterTable(funcName);  // 值参数表
    rightPar();             // 输出 )

    /* 生成中间代码 */
    genNonRetValFourTuple(funcName);
    mySemantic.funCallEnd(funcName);
    outfile << "<无返回值函数调用语句>" << endl;
}
// <值参数表>   ::= <表达式>{,<表达式>}｜<空>
void SyntaxAnalysis::valueParameterTable(string funcName)
{
#ifdef ERROR
    /**
     * 错误处理
     * bug here, but AC
     * 比较麻烦的是，若本身函数无参数，则如何判断呢
     */
    // 判断函数参数个数与类型是否匹配
    int paramNum = mySymTableMngr.getFuncParamNum(funcName);
    //if (myLexical.getGlobalSymbol() != RPARENT) {
    if (paramNum != 0) {
        // 说明值参数表非空
        if (myLexical.getGlobalSymbol() == RPARENT) {
            // 错误原因：没有传参数
            myError.SemanticError(paraNumNotMatch, myLexical.getCurLineNum());
            return;
        }
        // 判断类型
        vector<SymTableItem> paramList = mySymTableMngr.getFuncParamList(funcName);
        int num = 0;
        do {
            if (myLexical.getGlobalSymbol() == COMMA) {
                restoreAndPrint();    // 输出 ,
                storeAndPreRead();
            }

            int exprType = expression();
#ifdef ERROR
            // 判断类型是否一致
            if (exprType == CHAREXPR) {
                if (paramList[num].getType() != CHARTK) {
                    myError.SemanticError(paraTypeNotMatch, myLexical.getCurLineNum());
                }
            } else {
                if (paramList[num].getType() != INTTK) {
                    myError.SemanticError(paraTypeNotMatch, myLexical.getCurLineNum());
                }
            }
#endif
            /* 生成中间代码 */
            genPushFourTuple(funcName, num);
            num++;
        } while (myLexical.getGlobalSymbol() == COMMA);
        if (num != paramNum) {
            myError.SemanticError(paraNumNotMatch, myLexical.getCurLineNum());
        }
    } else {
        // 说明值参数表为空
        if (myLexical.getGlobalSymbol() != RPARENT) {
            // 错误原因：传了不存在的参数
            // 这样其实是有问题的，e.g. fun(;
            myError.SemanticError(paraNumNotMatch, myLexical.getCurLineNum());
        }
    }
#else
    // 判断函数参数个数与类型是否匹配
    int paramNum = mySymTableMngr.getFuncParamNum(funcName);
    if (paramNum != 0) {
        // 说明值参数表非空
        int num = 0;
        do {
            if (myLexical.getGlobalSymbol() == COMMA) {
                restoreAndPrint();    // 输出 ,
                storeAndPreRead();
            }
            expression();
            /* 生成中间代码 */
            genPushFourTuple(funcName, num);
            num++;
        } while (myLexical.getGlobalSymbol() == COMMA);
    }
#endif
    // 如果是 0 , 说明无参数, 此时已经读到 )  ，直接回到父程序即可
    outfile << "<值参数表>" << endl;
}
// <语句列>   ::= {<语句>}
void SyntaxAnalysis::statementSeries()
{
    /* 注意二者的区别
     *  - 空语句:    <空>;
     *  - 空语句列:  <空>
     */
    int statementType;
    do {
        statementType = statement();
    } while (statementType != 0);

    outfile << "<语句列>" << endl;
}
// <读语句>    ::=  scanf '('<标识符>{,<标识符>}')'
void SyntaxAnalysis::readStatement()
{
    restoreAndPrint();    // 输出 scanf
    myLexical.nextSym(true);    // 输出 (
    storeAndPreRead();
    do {
        if(myLexical.getGlobalSymbol() == COMMA) {
            restoreAndPrint();    // 输出 ,
            storeAndPreRead();    // 扫描下一个identifier
        }
#ifdef ERROR
        // 判断当前idfr是否定义
        if (mySymTableMngr.isIdfrUndefine(myLexical.getGlobalToken())) {
            myError.SemanticError(idfrNotDefine, myLexical.getCurLineNum());
            storeAndPreRead();
        } else {
            identifier();
        }
#else
        identifier();
#endif
        // 生成中间代码
        mySemantic.readState(lastIdfr);
    } while (myLexical.getGlobalSymbol() == COMMA);

    rightPar();          // 输出 )
    outfile << "<读语句>" << endl;
}
// <写语句>    ::= printf '(' <字符串>,<表达式> ')'| printf '('<字符串> ')'| printf '('<表达式>')'
void SyntaxAnalysis::writeStatement()
{
    restoreAndPrint();                    // 输出 print
    myLexical.nextSym(true);  // 输出 (
    storeAndPreRead();
    if (myLexical.getGlobalSymbol() == STRCON) {
        string strCon = myLexical.getGlobalToken();
        stringConstant();          // 字符串

        int exprType = 0;
        bool flag = false;
        if (myLexical.getGlobalSymbol() == COMMA) {
            flag = true;
            // , <表达式>
            restoreAndPrint();    // 输出 ,
            storeAndPreRead();
            exprType = expression();
        }
        /* 生成中间代码 */
        genPrintStrFourTuple(strCon);    // 字符串只会出现在 printf 中
        if (flag) {
            genPrintExprFourTuple((ExprType) exprType);
        }
    } else {
        int exprType = expression();
        /* 生成中间代码 */
        genPrintExprFourTuple((ExprType) exprType);
    }
    // 打印换行符
    mySemantic.printStrState(ENTER);

    rightPar();         // 输出 )
    outfile << "<写语句>" << endl;
}
// <返回语句>   ::=  return['('<表达式>')']
void SyntaxAnalysis::returnStatement()
{
    mySymTableMngr.setHaveRetStatement();
    // 错误处理
    SymbolType curScopeRetValue = SymbolType (mySymTableMngr.getCurScopeRetValType());

    restoreAndPrint();    // 输出 return
    storeAndPreRead();
    if (myLexical.getGlobalSymbol() == LPARENT) {
        // 左括号，说明有返回值
#ifdef ERROR
        if (curScopeRetValue == VOIDTK) {
            // 错误原因：无返回值函数，有返回值
            myError.retValueError(voidButHaveRetValue, myLexical.getCurLineNum());
            skipUntilEndSym(RPARENT);
            storeAndPreRead();
            return;
        }
#endif
        restoreAndPrint();      // 输出 (
        storeAndPreRead();

        int exprType = expression();
#ifdef ERROR
        // 错误原因：有返回值函数，但返回值不匹配
        if (exprType == CHAREXPR) {
            // 若是 char 型
            if (curScopeRetValue != CHARTK) {
                myError.retValueError(retValueNotMatch, myLexical.getCurLineNum());
            }
        } else {
            // 若是 int 型
            if (curScopeRetValue != INTTK) {
                myError.retValueError(retValueNotMatch, myLexical.getCurLineNum());
            }
        }
#endif
        rightPar();         // 输出 )
        /* 生成中间代码 */
        genRetValFourTuple();
    } else {
        // 无左括号，说明无返回值
#ifdef ERROR
        if (curScopeRetValue != VOIDTK) {
            // 错误原因：有返回值函数，无返回值
            myError.retValueError(retValueNotMatch, myLexical.getCurLineNum());
        }
#endif
        /* 生成中间代码 */
        mySemantic.retNonState();
    }
    outfile << "<返回语句>" << endl;
}

/*
 * 6. 需要输出的终结符 - 3
 */
// <整数>        ::= ［＋｜－］<无符号整数>
int SyntaxAnalysis::integer()
{
    bool isNegative;
    int numVal;
    if (myLexical.getGlobalSymbol() == PLUS || myLexical.getGlobalSymbol() == MINU) {
        isNegative = myLexical.getGlobalSymbol() == MINU;   // 判断是否为负数
        restoreAndPrint();    // 输出 + | -
        storeAndPreRead();    // 读入无符号整数
    }
#ifdef ERROR
    if(myLexical.getGlobalSymbol() != INTCON) {
        // 错误处理
        myError.SyntaxError(notIntCon, myLexical.getCurLineNum());
        storeAndPreRead();  // 往后预读
        return -1;
    }
#endif
    numVal = isNegative? -1*unsignedInteger() : unsignedInteger();
    outfile << "<整数>" << endl;
    return numVal;
}
// <无符号整数>  ::= <非零数字>{<数字>}| 0
int SyntaxAnalysis::unsignedInteger()
{
    int numVal = myLexical.getGlobalNum();  // 此时应该已经预读了无符号整数
    restoreAndPrint();    // 输出 无符号整数
    storeAndPreRead();
    outfile << "<无符号整数>" << endl;
    return numVal;
}

// <字符串>   ::=  "{十进制编码为32,33,35-126的ASCII字符}"
void SyntaxAnalysis::stringConstant()
{
    restoreAndPrint();    // 输出字符串
    storeAndPreRead();
    outfile << "<字符串>" << endl;
}

/*
 * 7. 非输出符号 - 8
 * 由于是终结符，且暂时没有进行错误处理，在进入函数时，默认已将相应的符号预读入 globalSymbol
 * 因此直接 restore 输出对应字符，并向后预读一位即可。
 */
// <标识符>    ::=  <字母>{<字母>｜<数字>}
void SyntaxAnalysis::identifier() {
    restoreAndPrint();    // 输出 标识符
    // 此时 globalToken为当前标识符, 更新标识符
    lastIdfr = myLexical.getGlobalToken();
    storeAndPreRead();    // 预读
}
// <类型标识符>      ::=  int | char
void SyntaxAnalysis::typeIdentifier() {
    restoreAndPrint();    // 输出 int | char
    lastIdfrType = SymbolType(myLexical.getGlobalSymbol());
    storeAndPreRead();
}
// <字符>    ::=  '<加法运算符>'｜'<乘法运算符>'｜'<字母>'｜'<数字>'
int SyntaxAnalysis::character() {
#ifdef ERROR
    if (myLexical.getGlobalSymbol() == CHARCON) {
        restoreAndPrint();        // 输出 字符
        storeAndPreRead();
    } else {
        myError.SyntaxError(notCharCon, myLexical.getCurLineNum());
        storeAndPreRead();      // 预读
    }
    return myLexical.getGlobalCharCon();
#else
    restoreAndPrint();        // 输出 字符
    storeAndPreRead();
    return myLexical.getGlobalCharCon();
#endif
}
// <加法运算符> ::= +｜-
void SyntaxAnalysis::addOperator() {
    restoreAndPrint();        // 输出 + | -
    storeAndPreRead();
}
// <乘法运算符>  ::= *｜/
void SyntaxAnalysis::multiOperator() {
    restoreAndPrint();    // 输出 * | /
    storeAndPreRead();
}
// <关系运算符>  ::=  <｜<=｜>｜>=｜!=｜==
void SyntaxAnalysis::relationalOperator() {
    restoreAndPrint();    // 输出关系运算符
    storeAndPreRead();
}
// <字母>   ::= ＿｜a｜．．．｜z｜A｜．．．｜Z
void SyntaxAnalysis::alphabet() {}
// <数字>   ::= 0｜<非零数字>
void SyntaxAnalysis::digit() {}
// <非零数字>  ::= 1｜...｜1
void SyntaxAnalysis::nonZeroDigit() {}
