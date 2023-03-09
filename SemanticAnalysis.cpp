//
// Created by Klaus-Mikealson on 2019/11/16.
//

#include "SemanticAnalysis.h"

SemanticAnalysis::~SemanticAnalysis() {}

SemanticAnalysis::SemanticAnalysis(TmpCodeTable &myTmpCodeTable, SymTableMngr &mySymTableMngr):
myTmpCodeTable(myTmpCodeTable), mySymTableMngr(mySymTableMngr) {
    ifNum = 0;
    forNum = 0;
    doNum = 0;
    whileNum = 0;
}

/* 语义动作子程序 - 生成结构化四元组 */
/* 1. 声明语句 */
void SemanticAnalysis::constDefState(string constName) {
    SymTableItem item = mySymTableMngr.getIdfrItem(constName);
    FourTuple tuple(CONSTDEF, item);
    myTmpCodeTable.addTuple(tuple);
}
void SemanticAnalysis::varDefState(string varName) {
    SymTableItem item = mySymTableMngr.getIdfrItem(varName);
    FourTuple tuple(VARDEF, item);
    myTmpCodeTable.addTuple(tuple);
}
void SemanticAnalysis::arrDefState(string arrName) {
    SymTableItem item = mySymTableMngr.getIdfrItem(arrName);
    FourTuple tuple(ARRDEF, item);
    myTmpCodeTable.addTuple(tuple);
}
void SemanticAnalysis::paraDefState(string paraName) {
    SymTableItem item = mySymTableMngr.getIdfrItem(paraName);
    FourTuple tuple(PARADEF, item);
    myTmpCodeTable.addTuple(tuple);
}
void SemanticAnalysis::funcDefBegin(string funcName, SymbolType retType) {
    FourTuple tuple(FUNCDEFBEGIN, funcName, retType);
    myTmpCodeTable.addTuple(tuple);
    /*
     * 此时符号表管理器刚刚更新当前域
     * 注意, 由于是值传递, 此处的func内部信息并不同步, 仅仅有函数名和返回值类型两个信息
     * 但对于函数定义来说, 这两个信息已经足够了
     */
    genLabel(funcName);
}
void SemanticAnalysis::funcDefEnd(string funcName) {
    FourTuple tuple(FUNCDEFEND, funcName, VOIDTK);  // 结束语句只作为标记, 无需type信息
    myTmpCodeTable.addTuple(tuple);
}

/* 2. 表达式语句 */
void SemanticAnalysis::exprState(Operator op, string dest, string src1, string src2) {
    SymTableItem destItem = mySymTableMngr.getIdfrItem(dest);
    SymTableItem src1Item = mySymTableMngr.getIdfrItem(src1);
    SymTableItem src2Item = mySymTableMngr.getIdfrItem(src2);
    FourTuple tuple(op, destItem, src1Item, src2Item);
    myTmpCodeTable.addTuple(tuple);
}
void SemanticAnalysis::useArr(string dest, string src1, string src2) {
    // dest = src1[src2]
    SymTableItem destItem = mySymTableMngr.getIdfrItem(dest);
    SymTableItem src1Item = mySymTableMngr.getIdfrItem(src1);
    SymTableItem src2Item = mySymTableMngr.getIdfrItem(src2);
    FourTuple tuple(USEARR, destItem, src1Item, src2Item);
    myTmpCodeTable.addTuple(tuple);
}

/* 3. 赋值语句 */
void SemanticAnalysis::assVar(string dest, string src1) {
    // dest = src1
    SymTableItem destItem = mySymTableMngr.getIdfrItem(dest);
    SymTableItem src1Item = mySymTableMngr.getIdfrItem(src1);
    FourTuple tuple(ASSVAR, destItem, src1Item);
    myTmpCodeTable.addTuple(tuple);
}
void SemanticAnalysis::assArr(string dest, string src1, string src2) {
    // dest[src2] = src1
    SymTableItem destItem = mySymTableMngr.getIdfrItem(dest);
    SymTableItem src1Item = mySymTableMngr.getIdfrItem(src1);
    SymTableItem src2Item = mySymTableMngr.getIdfrItem(src2);
    FourTuple tuple(ASSARR, destItem, src1Item, src2Item);
    myTmpCodeTable.addTuple(tuple);
}

/* 4. 条件语句 */
void SemanticAnalysis::conditionState(Operator op, string src1) {
    // if(expr)  - op = SEXPR
    SymTableItem src1Item = mySymTableMngr.getIdfrItem(src1);
    FourTuple tuple(op, src1Item, true);  // 条件语句
    myTmpCodeTable.addTuple(tuple);
}
void SemanticAnalysis::conditionState(Operator op, string src1, string src2) {
    SymTableItem src1Item = mySymTableMngr.getIdfrItem(src1);
    SymTableItem src2Item = mySymTableMngr.getIdfrItem(src2);
    FourTuple tuple(op, src1Item, src2Item, true);  // 关系运算
    myTmpCodeTable.addTuple(tuple);
}

/* 5. 控制语句 */
void SemanticAnalysis::controlState(Operator op) {
    // 标记控制语句的开始或结束
    FourTuple tuple(op);
    myTmpCodeTable.addTuple(tuple);
}
string SemanticAnalysis::genCtrlLabelName(Operator op, int suffix) {
    stringstream ss;
    string prefix;
    switch (op) {
        case IF_BEGIN:
            prefix = "if_";
            break;
        case ELSE:
            prefix = "else_";
            break;
        case IF_END:
            prefix = "endIf_";
            break;
        case FOR_BEGIN:
            prefix = "for_";
            break;
        case FOR_END:
            prefix = "endFor_";
            break;
        case DO_BEGIN:
            prefix = "do_";
            break;
        case DO_END:
            prefix = "endDo_";
            break;
        case WHILE_BEGIN:
            prefix = "while_";
            break;
        case WHILE_END:
            prefix = "endWhile_";
            break;
        default:
            break;
    }
    ss << prefix << suffix;
    return ss.str();
}
int SemanticAnalysis::getIfNum() { return ifNum++; }
int SemanticAnalysis::getForNum() { return forNum++; }
int SemanticAnalysis::getDoNum() { return doNum++; }
int SemanticAnalysis::getWhileNum() { return whileNum++; }

/* 6. 函数调用语句 */
void SemanticAnalysis::haveRetFunCallState(string dest, string funcName) {
    // dest = funcName()
    SymTableItem destItem = mySymTableMngr.getIdfrItem(dest);
    FourTuple tuple(HAVERETFUNCALL, destItem, funcName);
    myTmpCodeTable.addTuple(tuple);
}
void SemanticAnalysis::nonRetFunCallState(string funcName) {
    // funcName()
    FourTuple tuple(NONRETFUNCALL, funcName);
    myTmpCodeTable.addTuple(tuple);
}
void SemanticAnalysis::push(string dest, int num, string funcName) {
    // push dest
    SymTableItem destItem = mySymTableMngr.getIdfrItem(dest);
    FourTuple tuple(PUSH, destItem, num, funcName);
    myTmpCodeTable.addTuple(tuple);
}
void SemanticAnalysis::funCallBegin(string funcName) {
    FourTuple tuple(FUNCALLBEGIN, funcName);
    myTmpCodeTable.addTuple(tuple);
}
void SemanticAnalysis::funCallEnd(string funcName) {
    FourTuple tuple(FUNCALLEND, funcName);
    myTmpCodeTable.addTuple(tuple);
}

/* 7. 写语句 */
void SemanticAnalysis::printStrState(string stringName) {
    FourTuple tuple(WRITESTRING, stringName);
    myTmpCodeTable.addTuple(tuple);
}
void SemanticAnalysis::printExprState(string dest, ExprType exprType) {
    SymTableItem destItem = mySymTableMngr.getIdfrItem(dest);
    // 这里应该看得是表达式类型, 而非变量的类型, 比如 ('a') 这种情况，应算作int型
    //Operator op = destItem.getType()==INTTK? WRITEINTEXPR : WRITECHAREXPR;
    Operator op = exprType==INTEXPR? WRITEINTEXPR : WRITECHAREXPR;
    FourTuple tuple(op, destItem);
    myTmpCodeTable.addTuple(tuple);
}

/* 8. 读语句 */
void SemanticAnalysis::readState(string dest) {
    SymTableItem destItem = mySymTableMngr.getIdfrItem(dest);
    Operator op = destItem.getType()==INTTK? READINT : READCHAR;
    FourTuple tuple(op, destItem);
    myTmpCodeTable.addTuple(tuple);
}

/* 9. 返回语句 */
void SemanticAnalysis::retValState(string dest) {
    SymTableItem destItem = mySymTableMngr.getIdfrItem(dest);
    FourTuple tuple(RETVAL, destItem);
    myTmpCodeTable.addTuple(tuple);
}
void SemanticAnalysis::retNonState() {
    FourTuple tuple(RETNON);
    myTmpCodeTable.addTuple(tuple);
}

/* 10. 其他 */
void SemanticAnalysis::genLabel(string labelName) {
    FourTuple tuple(GENLABEL, labelName);
    myTmpCodeTable.addTuple(tuple);
}
void SemanticAnalysis::jump(string labelName) {
    FourTuple tuple(JUMP, labelName);
    myTmpCodeTable.addTuple(tuple);
}
void SemanticAnalysis::branch(string labelName, bool beq) {
    Operator op = beq? BEQ:BNE;
    FourTuple tuple(op, labelName);
    myTmpCodeTable.addTuple(tuple);
}
