//
// Created by Klaus-Mikealson on 2019/11/16.
//

#include "FourTuple.h"

FourTuple::FourTuple() {}
FourTuple::~FourTuple() {}
FourTuple::FourTuple(Operator op, SymTableItem dest, SymTableItem src1, SymTableItem src2) {
    this->op = op;
    this->dest = dest;
    this->src1 = src1;
    this->src2 = src2;
}
FourTuple::FourTuple(Operator op, SymTableItem dest) {
    this->op = op;
    this->dest = dest;
}
FourTuple::FourTuple(Operator op, SymTableItem src1, bool relationalOp) {
    this->op = op;
    this->src1 = src1;
}
FourTuple::FourTuple(Operator op, SymTableItem dest, int num, string funcName) {
    this->op = op;
    this->dest = dest;
    this->paraNum = num;
    this->name = funcName;
}
FourTuple::FourTuple(Operator op, SymTableItem dest, SymTableItem src1) {
    this->op = op;
    this->dest = dest;
    this->src1 = src1;
}
FourTuple::FourTuple(Operator op, SymTableItem src1, SymTableItem src2, bool relationalOp) {
    this->op = op;
    this->src1 = src1;
    this->src2 = src2;
}
FourTuple::FourTuple(Operator op, string name) {
    this->op = op;
    this->name = name;
}
FourTuple::FourTuple(Operator op) {
    this->op = op;
}
FourTuple::FourTuple(Operator op, SymTableItem dest, string funcName) {
    this->op = op;
    this->dest = dest;
    this->name = funcName;
}
FourTuple::FourTuple(Operator op, string funcName, SymbolType retType) {
    this->op = op;
    this->name = funcName;
    this->retType = retType;
}

/* 属性访问 */
Operator FourTuple::getOpType() {
    return op;
}
SymTableItem FourTuple::getDest() {
    return dest;
}
SymTableItem FourTuple::getSrc1() {
    return src1;
}
SymTableItem FourTuple::getSrc2() {
    return src2;
}
string FourTuple::getName() {
    // for string or label
    return name;
}
int FourTuple::getParaNum() {
    return paraNum;
}

string FourTuple::toString() {
    stringstream ss;

    switch (op) {
        /* 1. 声明语句 */
        case CONSTDEF:
            if (dest.getType() == INTTK) {
                ss << "const int " << dest.getName() << " = " << dest.getConstValue();
            } else {
                ss << "const char " << dest.getName() << " = \'" << char(dest.getConstValue()) << "\'";
            }
            break;
        case VARDEF:
            if (dest.getType() == INTTK) {
                ss << "var int " << dest.getName();
            } else {
                ss << "var char " << dest.getName();
            }
            break;
        case ARRDEF:
            if (dest.getType() == INTTK) {
                ss << "array int " << dest.getName() << "[" << dest.getLength() << "]";
            } else {
                ss << "array char " << dest.getName() << "[" << dest.getLength() << "]";
            }
            break;
        case PARADEF:
            if (dest.getType() == INTTK) {
                ss << "para int " << dest.getName();
            } else {
                ss << "para char " << dest.getName();
            }
            break;
        case FUNCDEFBEGIN:
            ss << "FUNC_DEF_BEGIN" << endl;
            if (retType == INTTK) {
                ss << "func int " << name << "()";
            } else if (retType == CHARTK) {
                ss << "func char " << name << "()";
            } else {
                ss << "func void " << name << "()";
            }
            break;
        case FUNCDEFEND:
            ss << "FUNC_DEF_END";
            break;

        /* 2. 表达式语句 */
        case ADDOP:
            ss << dest.getName() << " = " << src1.getName() << " + " << src2.getName();
            break;
        case SUBOP:
            ss << dest.getName() << " = " << src1.getName() << " - " << src2.getName();
            break;
        case MULOP:
            ss << dest.getName() << " = " << src1.getName() << " * " << src2.getName();
            break;
        case DIVOP:
            ss << dest.getName() << " = " << src1.getName() << " / " << src2.getName();
            break;
        case USEARR:
            // dest = src1[src2]
            ss << dest.getName() << " = " << src1.getName() << "[" << src2.getName() << "]";
            break;

        /* 3. 赋值语句 */
        case ASSVAR:
            ss << dest.getName() << " = " << src1.getName();
            break;
        case ASSARR:
            // dest[src2] = src1
            ss << dest.getName() << "[" << src2.getName() << "]" << " = " << src1.getName();
            break;

        /* 4. 条件语句 */
        case SEXPR:
            ss << src1.getName();
            break;
        case LESS:
            ss << src1.getName() << " < " << src2.getName();
            break;
        case LEQL:
            ss << src1.getName() << " <= " << src2.getName();
            break;
        case MORE:
            ss << src1.getName() << " > " << src2.getName();
            break;
        case MEQL:
            ss << src1.getName() << " >= " << src2.getName();
            break;
        case NEQUAL:
            ss << src1.getName() << " != " << src2.getName();
            break;
        case EQUAL:
            ss << src1.getName() << " == " << src2.getName();
            break;

        /* 5. 控制语句 */
        case IF_BEGIN:
            ss << "IF_BEGIN";
            break;
        case ELSE:
            ss << "ELSE";
        case IF_END:
            ss << "IF_END";
            break;
        case FOR_BEGIN:
            ss << "FOR_BEGIN";
            break;
        case FOR_END:
            ss << "FOR_END";
            break;
        case DO_BEGIN:
            ss << "DO_BEGIN";
            break;
        case DO_END:
            ss << "DO_END";
            break;
        case WHILE_BEGIN:
            ss << "WHILE_BEGIN";
            break;
        case WHILE_END:
            ss << "WHILE_END";
            break;

        /* 6. 函数调用语句 */
        case HAVERETFUNCALL:
            // dest = func()
            ss << "call " << name << endl;
            ss << dest.getName() << " = RET";
            break;
        case NONRETFUNCALL:
            // func()
            ss << "call " << name;
            break;
        case PUSH:
            ss << "push " << dest.getName();
            break;
        case FUNCALLBEGIN:
            ss << "FUNC_CALL_BEGIN";
            break;
        case FUNCALLEND:
            ss << "FUNC_CALL_END";
            break;

        /* 7. 写语句 */
        case WRITESTRING:
            ss << "print " << name;
            break;
        case WRITEINTEXPR:
        case WRITECHAREXPR:
            ss << "print " << dest.getName();
            break;

        /* 8. 读语句 */
        case READINT:
        case READCHAR:
            ss << "read " << dest.getName();
            break;

        /* 9. 返回语句 */
        case RETVAL:
            ss << "ret " << dest.getName();
            break;
        case RETNON:
            ss << "ret " << NOTHING;
            break;

        /* 10. 其他 */
        case GENLABEL:
            ss << "gen label " << name;
            break;
        case JUMP:
            ss << "jump " << name;
            break;
        case BEQ:
            ss << "branch " << name;
            break;
        case BNE:
            ss << "branch " << name;
            break;
        /* 11. 内联函数 */
        case INLINE_FUNC_BEGIN:
            ss << "INLINE FUNC BEGIN";
            break;
        case INLINE_FUNC_END:
            ss << "INLINE FUNC END";
            break;
        case INLINE_RET:
            ss << "inline ret " << dest.getName();
            break;
        case INLINE_CALL:
            ss << dest.getName() << " = inline ret";
        default:
            break;
    }
    return ss.str();
}
