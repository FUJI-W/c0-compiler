//
// Created by Klaus-Mikealson on 2019/11/17.
//

#ifndef MYCOMPILER_MIPSGENERATOR_H
#define MYCOMPILER_MIPSGENERATOR_H

#include "SymTableMngr.h"
#include "TmpCodeTable.h"
#include "Constant.h"

/* Macro Define */
/* Register */
#define zero 0
#define v0 2
#define rd 3   // v1
#define a0 4
#define a1 5
#define a2 6
#define a3 7
#define t0 8
#define t1 9
#define t2 10
#define t3 11
#define t4 12
#define t5 13
#define t6 14
#define t7 15
#define s0 16
#define s1 17
#define s2 18
#define s3 19
#define s4 20
#define s5 21
#define s6 22
#define s7 23
#define rs 24   // t8
#define rt 25   // t9
#define gp 28
#define sp 29
#define fp 30   // fp
#define ra 31

/* Syscall Option */
#define printInt 1
#define readInt 5
#define printChar 11
#define readChar 12
#define printStr 4
#define exit 10

class MIPSGenerator {
private:
    TmpCodeTable &myTmpCodeTable;   // 全局公用中间代码表

    ofstream mipsfile;
    SymTable globalTable;
    SymTable curScope;
    SymTable lastScope;
    map<string, SymTable> name2Scope;
    map<string, string> name2String;

public:
    MIPSGenerator(string fileName, SymTableMngr &mySymTableMngr, TmpCodeTable &myTmpCodeTable);
    ~MIPSGenerator();

    /* 辅助叶子函数 */
    int loadVar2Reg(SymTableItem item, int reg);   // load itemVal to reg
    int saveVar4Reg(SymTableItem item, int reg);   // save reg to itemVal
    int loadArr2Reg(SymTableItem arr, SymTableItem index, int reg);    // load a[i] to reg
    int saveArr4Reg(SymTableItem arr, SymTableItem index, int reg);    // save reg to a[i]
    // 函数嵌套调用
    int loadPreVar2Reg(SymTableItem item, int reg);   // load itemVal to reg
    int savePreVar4Reg(SymTableItem item, int reg);   // save reg to itemVal
    int loadPreArr2Reg(SymTableItem arr, SymTableItem index, int reg);    // load a[i] to reg
    int getParaReg(int paraNum);
    void save31Reg();
    void restore31Reg();
    /* 代码优化 */
    void loadAllParams();
    void saveUsedRegs();
    void restoreUsedRegs();

    /* generate MIPS */
    void generate();
    void genDataSegment();
    void genTextSegment();

    /* 1. 声明语句 */
    //void genFuncDef(FourTuple tuple);
    /* 2. 表达式语句 */
    void genExprCalc(FourTuple tuple);
    void genAddOp(int rdReg, int rsReg, int rtReg);
    void genSubOp(int rdReg, int rsReg, int rtReg);
    void genMulOp(int rdReg, int rsReg, int rtReg);
    void genDivOp(int rdReg, int rsReg, int rtReg);
    void genUseArr(FourTuple tuple);
    /* 3. 赋值语句 */
    void genAssVar(FourTuple tuple);
    void genAssArr(FourTuple tuple);
    /* 4. 条件语句 */
    void genSexprCond(FourTuple tuple, int destReg);
    void genLessCond(FourTuple tuple, int destReg);
    void genLeqlCond(FourTuple tuple, int destReg);
    void genMoreCond(FourTuple tuple, int destReg);
    void genMeqlCond(FourTuple tuple, int destReg);
    void genNeqlCond(FourTuple tuple, int destReg);
    void genEqlCond(FourTuple tuple, int destReg);
    /* 5. 控制语句 */
    /*
    IF_BEGIN,
    ELSE,
    IF_END,
    FOR_BEGIN,
    FOR_END,
    DO_BEGIN,
    DO_END,
    WHILE_BEGIN,
    WHILE_END,
     */
    /* 6. 函数调用语句 */
    void genFuncCall(vector<FourTuple> table, int st, int ed);
    void genFuncCallBegin(FourTuple tuple);
    void genFuncCallEnd(FourTuple tuple);
    void genHaveRetFunCall(FourTuple tuple);
    void genNonRetFunCall(FourTuple tuple);
    void genPushParam(FourTuple tuple);
    void genPushInline(FourTuple tuple);
    int savePara4Reg(SymTableItem item, int reg);   // save reg to itemVal
    void genExprCalcInFunCall(FourTuple tuple);
    void genUseArrInFunCall(FourTuple tuple);
    /* 7. 写语句 */
    void genPrintString(FourTuple tuple);
    void genPrintIntExpr(FourTuple tuple);
    void genPrintCharExpr(FourTuple tuple);
    /* 8. 读语句 */
    void genReadInt(FourTuple tuple);
    void genReadChar(FourTuple tuple);
    /* 9. 返回语句 */
    void genRetVal(FourTuple tuple);
    void genRetNon(FourTuple tuple);
    /* 10. 其他 */
    void genLabel(FourTuple tuple);
    void genJump(FourTuple tuple);
    void genJumpRet();
    void genBeq(FourTuple tuple, int reg1, int reg2);
    void genBne(FourTuple tuple, int reg1, int reg2);
    void genSyscall(int option);
    /* 11. 内联函数 */
    void genInlineFunc(vector<FourTuple> table, int st, int ed);
};


#endif //MYCOMPILER_MIPSGENERATOR_H
