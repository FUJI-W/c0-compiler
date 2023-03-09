//
// Created by Klaus-Mikealson on 2019/11/17.
//

#include <stack>
#include "MIPSGenerator.h"

MIPSGenerator::MIPSGenerator(string fileName, SymTableMngr &mySymTableMngr, TmpCodeTable &myTmpCodeTable):
myTmpCodeTable(myTmpCodeTable)
{
    mipsfile.open(fileName);

    globalTable = mySymTableMngr.getGlobalScope();
    name2Scope = mySymTableMngr.getName2LocScope();
    name2String = mySymTableMngr.getName2String();
    //curScope = name2Scope[MAIN];
}

MIPSGenerator::~MIPSGenerator() {}

/* leaf func */
/**
 * 优化思路为：
 * 若相应的item已经分配了寄存器, 则直接返回对应的寄存器号即可
 * 否则, 读取内存, 并加载到给定的 reg 寄存器中, 返回该reg
 */
int MIPSGenerator::loadVar2Reg(SymTableItem item, int reg) {
    /* 代码优化 */
    if (curScope.isIdfrTemp(item.getName())) {
        // 若为中间变量
        int state = curScope.getTempState(item.getName());
        if (state == 0) {
            // def状态
            curScope.setTempState(item.getName(), 1);
            return t0;
        } else {
            // false状态, 值被破坏
            //mipsfile << "lw $t0, " << item.getAddress() << "($sp)" << endl;
            mipsfile << "lw $" << reg << ", " << item.getAddress() << "($sp)" << endl;
            return reg;
        }
    }

    int allocedReg = curScope.getAllocedReg(item.getName());
    if (allocedReg != -1) {
        return allocedReg;
    }

    if (item.isConstant()) {
        // 1. 常量
        mipsfile << "li $" << reg << ", " << item.getConstValue() << endl;
    } else {
        // 2. 变量
        int pReg = item.isGlobal()? gp:sp;
        mipsfile << "lw $" << reg << ", " << item.getAddress() << "($" << pReg << ")" << endl;
    }
    return reg;
}
int MIPSGenerator::saveVar4Reg(SymTableItem item, int reg) {
    /* 代码优化 */
    if (curScope.isIdfrTemp(item.getName())) {
        // 若为中间变量
        SymTableItem lastTemp = curScope.getLastTemp();
        if (!curScope.isTempEmpty() && curScope.getTempState(lastTemp.getName()) == 0) {
            // 且上一次t0尚未使用, 则需要保存值, 并标记为状态2
            mipsfile << "sw $t0," << lastTemp.getAddress() << "($sp)" << endl;
            curScope.setTempState(lastTemp.getName(), 2);
        }
        // 标记当前变量为def状态, 并更新上一次tempItem
        curScope.setTempState(item.getName(), 0);
        curScope.setLastTemp(item);
        // move
        mipsfile << "move $t0, $" << reg << endl;
        return t0;
    }

    int allocedReg = curScope.getAllocedReg(item.getName());
    if (allocedReg != -1) {
        mipsfile << "move $" << allocedReg << ", $" << reg << endl;
        return allocedReg;
    }

    int pReg = item.isGlobal()? gp:sp;
    mipsfile << "sw $" << reg << ", " << item.getAddress() << "($" << pReg << ")" << endl;
    return reg;
}

int MIPSGenerator::loadArr2Reg(SymTableItem arr, SymTableItem index, int reg) {
    // reg = a[i]
    if (index.isConstant()) {
        int iOffset = 4 * index.getConstValue();
        int pReg = arr.isGlobal()? gp:sp;
        mipsfile << "addi $" << rs << ", $" << pReg << ", " << arr.getAddress() + iOffset << endl;
    } else {
        // 1. 计算数组的相对地址 rs = offset
        // -> 1.1 rs = index*eleSize
        int eleSize = 4;
        int iReg = loadVar2Reg(index, rs);                      // rs = index
        mipsfile << "li $" << rt << ", " << eleSize << endl;    // rt = eleSize
        genMulOp(rs, iReg, rt);                                 // rs = rs*rt
        // -> 1.2 rs = rs + arr
        mipsfile << "addi $" << rs << ", $" << rs << ", " << arr.getAddress() << endl;

        // 2. 计算数组的绝对地址 addr = pReg + offset ->  rs = sp|gp + rs
        int pReg = arr.isGlobal()? gp:sp;
        genAddOp(rs, pReg, rs);
    }

    // 3. 加载数组元素的值
    mipsfile << "lw $" << reg << ", " << 0 << "($" << rs << ")" << endl;
    return reg;
}
int MIPSGenerator::saveArr4Reg(SymTableItem arr, SymTableItem index, int reg) {
    // a[i] = reg
    if (index.isConstant()) {
        int iOffset = 4 * index.getConstValue();
        int pReg = arr.isGlobal()? gp:sp;
        mipsfile << "addi $" << rs << ", $" << pReg << ", " << arr.getAddress() + iOffset << endl;
    } else {
        // 1. 计算数组的相对地址 rs = offset
        // -> 1.1 rs = index*eleSize
        int eleSize = 4;
        int iReg = loadVar2Reg(index, rs);                      // rs = index
        mipsfile << "li $" << rt << ", " << eleSize << endl;    // rt = eleSize
        genMulOp(rs, iReg, rt);                                 // rs = rs*rt
        // -> 1.2 rs = rs + arr
        mipsfile << "addi $" << rs << ", $" << rs << ", " << arr.getAddress() << endl;

        // 2. 计算数组的绝对地址 addr = pReg + offset ->  rs = sp|gp + rs
        int pReg = arr.isGlobal()? gp:sp;
        genAddOp(rs, pReg, rs);
    }

    // 3. 保存数组元素的值
    mipsfile << "sw $" << reg << ", " << 0 << "($" << rs << ")" << endl;
    return reg;
}
// 函数嵌套调用
int MIPSGenerator::loadPreVar2Reg(SymTableItem item, int reg) {
    /* 代码优化 */
    if (lastScope.isIdfrTemp(item.getName())) {
        // 若为中间变量
        int state = lastScope.getTempState(item.getName());
        if (state == 0) {
            // def状态
            lastScope.setTempState(item.getName(), 1);
            return t0;
        } else {
            // false状态, 值被破坏
            //mipsfile << "lw $t0, " << curScope.getDataSize() + item.getAddress() << "($sp)" << endl;
            mipsfile << "lw $" << reg << ", " << curScope.getDataSize() + item.getAddress() << "($sp)" << endl;
            return reg;
        }
    }

    int allocedReg = lastScope.getAllocedReg(item.getName());
    if (allocedReg != -1) {
        return allocedReg;
    }

    // 函数调用语句 push 操作, 需要加载当前域的值参到函数域中
    if (item.isConstant()) {
        // 1. 常量
        mipsfile << "li $" << reg << ", " << item.getConstValue() << endl;
    } else if (item.isGlobal()) {
        // 2. 全局变量
        mipsfile << "lw $" << reg << ", " << item.getAddress() << "($gp)" << endl;
    } else {
        // 3. 局部变量
        mipsfile << "lw $" << reg << ", " << curScope.getDataSize() + item.getAddress() << "($sp)" << endl;
    }
    return reg;
}
int MIPSGenerator::savePreVar4Reg(SymTableItem item, int reg) {
    /* 代码优化 */
    if (lastScope.isIdfrTemp(item.getName())) {
        // 若为中间变量
        SymTableItem lastTemp = lastScope.getLastTemp();
        if (!lastScope.isTempEmpty() && lastScope.getTempState(lastTemp.getName()) == 0) {
            // 且上一次t0尚未使用, 则需要保存值, 并标记为状态2
            mipsfile << "sw $t0, " << curScope.getDataSize() + lastTemp.getAddress() << "($sp)" << endl;
            lastScope.setTempState(lastTemp.getName(), 2);
        }
        // 标记当前变量为def状态, 并更新上一次tempItem
        lastScope.setTempState(item.getName(), 0);
        lastScope.setLastTemp(item);
        // move
        mipsfile << "move $t0, $" << reg << endl;
        return t0;
    }

    int allocedReg = lastScope.getAllocedReg(item.getName());
    if (allocedReg != -1) {
        mipsfile << "move $" << allocedReg << ", $" << reg << endl;
        return allocedReg;
    }

    // 有返回值函数调用语句执行完毕后, 返回值保存在当前域的变量中
    if (item.isGlobal()) {
        // 1. 全局变量
        mipsfile << "sw $" << reg << ", " << item.getAddress() << "($gp)" << endl;
    } else {
        // 2. 局部变量
        mipsfile << "sw $" << reg << ", " << curScope.getDataSize() + item.getAddress() << "($sp)" << endl;
    }
    return reg;
}
int MIPSGenerator::loadPreArr2Reg(SymTableItem arr, SymTableItem index, int reg) {
    // reg = a[i]
    if (index.isConstant()) {
        int iOffset = 4 * index.getConstValue();
        int pReg = arr.isGlobal()? gp:sp;
        mipsfile << "addi $" << rs << ", $" << pReg << ", " << arr.getAddress() + iOffset << endl;
    } else {
        // 1. 计算数组的相对地址 rs = offset
        // -> 1.1 rs = index*eleSize
        int eleSize = 4;
        int iReg = loadPreVar2Reg(index, rs);                      // rs = index
        mipsfile << "li $" << rt << ", " << eleSize << endl;    // rt = eleSize
        genMulOp(rs, iReg, rt);                                 // rs = rs*rt
        // -> 1.2 rs = rs + arr
        mipsfile << "addi $" << rs << ", $" << rs << ", " << arr.getAddress() << endl;

        // 2. 计算数组的绝对地址 addr = pReg + offset
        int pReg = arr.isGlobal()? gp:sp;
        genAddOp(rs, pReg, rs);
    }

    // 3. 加载数组元素的值
    if (arr.isGlobal()) {
        mipsfile << "lw $" << reg << ", 0($" << rs << ")" << endl;
    } else {
        mipsfile << "lw $" << reg << ", " << curScope.getDataSize() << "($" << rs << ")" << endl;
    }

    return reg;
}
int MIPSGenerator::getParaReg(int paraNum) {
    return -1;  // 暂时不使用参数寄存器
    switch (paraNum) {
        case 0: return a0;
        case 1: return a1;
        case 2: return a2;
        case 3: return a3;
        default: return -1;  // 参数个数大于4
    }
}
void MIPSGenerator::save31Reg() {
    mipsfile << "sw $ra, " << curScope.getDataSize() << "($sp)" << endl;
}
void MIPSGenerator::restore31Reg() {
    mipsfile << "lw $ra, " << curScope.getDataSize() << "($sp)" << endl;
}

/* 代码优化 */
void MIPSGenerator::loadAllParams() {
    // 注意加载的是当前域
    vector<SymTableItem> paramList = curScope.getParamList();
    for (int i = 0; i < paramList.size(); ++i) {
        SymTableItem param = paramList[i];
        int reg = curScope.getAllocedReg(param.getName());
        if (reg != -1) {
            mipsfile << "lw $" << reg << ", " << param.getAddress() << "($sp)" << endl;
        }
    }
}
void MIPSGenerator::saveUsedRegs() {
    // 注意保存的是上次的域, 且在当前域被分配了的寄存器
    map<int, string> reg2Name = lastScope.getReg2Name();
    int max_reg_num = reg2Name.size();
    int used_reg_num = curScope.getName2Reg().size();   // 被调用函数域分配的寄存器个数

    for (int i = 0; i < used_reg_num && i < max_reg_num; ++i) {
        int reg = regs[i];
        SymTableItem item = lastScope.getIdfrItem(reg2Name[reg]);

        mipsfile << "sw $" << reg << ", " << curScope.getDataSize() + item.getAddress() << "($sp)" << endl;
    }
    // jal之前要保存t0寄存器
    SymTableItem lastTemp = lastScope.getLastTemp();
    if (!lastScope.isTempEmpty() && lastScope.getTempState(lastTemp.getName()) == 0) {
        mipsfile << "sw $t0," << curScope.getDataSize() + lastTemp.getAddress() << "($sp)" << endl;
        lastScope.setTempState(lastTemp.getName(), 2);
    }
}
void MIPSGenerator::restoreUsedRegs() {
    // 注意恢复的是上次的域, 且在当前域被分配了的寄存器
    map<int, string> reg2Name = lastScope.getReg2Name();
    int max_reg_num = reg2Name.size();
    int used_reg_num = curScope.getName2Reg().size();   // 被调用函数域分配的寄存器个数

    for (int i = 0; i < used_reg_num && i < max_reg_num; ++i) {
        int reg = regs[i];
        SymTableItem item = lastScope.getIdfrItem(reg2Name[reg]);

        mipsfile << "lw $" << reg << ", " << curScope.getDataSize() + item.getAddress() << "($sp)" << endl;
    }
}

/* generate MIPS */
void MIPSGenerator::generate() {
    genDataSegment();
    genTextSegment();

    vector<FourTuple> table = myTmpCodeTable.getTable();
    stack<int> s;
    for (int i = 0; i < table.size(); ++i) {
        FourTuple tuple = table[i];
        switch (tuple.getOpType()) {
            /* 1. 声明语句 */
            case FUNCDEFBEGIN:
                lastScope = curScope;
                curScope = name2Scope[tuple.getName()];
                break;
            case FUNCDEFEND:
                // 防止函数最后没有返回语句
                if (i > 0 && table[i-1].getOpType()!=RETVAL && table[i-1].getOpType()!=RETNON)
                    genJumpRet();
                curScope = lastScope;
                break;
            /* 2. 表达式语句 */
            case ADDOP:
            case SUBOP:
            case MULOP:
            case DIVOP:
                genExprCalc(tuple); break;
            case USEARR: genUseArr(tuple); break;
            /* 3. 赋值语句 */
            case ASSVAR: genAssVar(tuple); break;
            case ASSARR: genAssArr(tuple); break;
            /* 4. 条件语句 */
            case SEXPR: genSexprCond(tuple, rd); break;
            case LESS: genLessCond(tuple, rd); break;
            case LEQL: genLeqlCond(tuple, rd); break;
            case MORE: genMoreCond(tuple, rd); break;
            case MEQL: genMeqlCond(tuple, rd); break;
            case NEQUAL: genNeqlCond(tuple, rd); break;
            case EQUAL: genEqlCond(tuple, rd); break;
            /* 5. 控制语句 */
            /* 6. 函数调用语句 */
            case FUNCALLBEGIN:
                for (int j = i; j < table.size(); ++j) {
                    FourTuple tuple = table[j];
                    if (tuple.getOpType() == FUNCALLBEGIN) {
                        s.push(j);      // 标记函数调用开头
                    } else if (tuple.getOpType() == FUNCALLEND) {
                        int st = s.top(); s.pop();
                        int ed = j;
                        genFuncCall(table, st, ed);     // 函数调用叶子处理函数
                        i = ed;
                    }

                    if (s.empty()) break;
                }
                break;
            /* 7. 写语句 */
            case WRITESTRING: genPrintString(tuple); break;
            case WRITEINTEXPR: genPrintIntExpr(tuple); break;
            case WRITECHAREXPR: genPrintCharExpr(tuple); break;
            /* 8. 读语句 */
            case READINT: genReadInt(tuple); break;
            case READCHAR: genReadChar(tuple); break;
            /* 9. 返回语句 */
            case RETVAL: genRetVal(tuple); break;
            case RETNON: genRetNon(tuple); break;
            /* 10. 其他 */
            case GENLABEL: genLabel(tuple); break;
            case JUMP: genJump(tuple); break;
            case BEQ: genBeq(tuple, rd, zero); break;
            case BNE: genBne(tuple, rd, zero); break;
            default:
                break;
        }
    }
}
void MIPSGenerator::genDataSegment() {
    mipsfile << ".data" << endl;
    /* 1. 全局变量分配空间 */
    int size = globalTable.getDataSize();
    mipsfile << "heap: .space " << size << endl;

    /* 2. 为字符串常量分配空间 */
    mipsfile << ENTER << ": .asciiz \"\\n\"" << endl;
    for (map<string, string>::iterator iter = name2String.begin(); iter != name2String.end(); iter++) {
        mipsfile << iter->first << ": .asciiz \"" << iter->second << "\"" << endl;
    }
}
void MIPSGenerator::genTextSegment() {
    mipsfile << ".text" << endl;
    /* jump and link main */
    //mipsfile << "subi $sp, $sp, " << name2Scope[MAIN].getDataSize() << endl;    // 为main函数栈空间
    mipsfile << "addi $sp, $sp, " << -1*name2Scope[MAIN].getDataSize() << endl;    // 为main函数栈空间
    mipsfile << "jal main" << endl;
    /* exit */
    genSyscall(exit);
}

/* 1. 声明语句 */
/* 2. 表达式语句 */
void MIPSGenerator::genExprCalc(FourTuple tuple) {
    // + - * /
    int lReg = loadVar2Reg(tuple.getSrc1(), rs);
    int rReg = loadVar2Reg(tuple.getSrc2(), rt);
    int dReg = t0;
    // 事实上, dest一定为中间变量
    SymTableItem dest = tuple.getDest();
    SymTableItem lastTemp = curScope.getLastTemp();
    if (!curScope.isTempEmpty() && curScope.getTempState(lastTemp.getName()) == 0) {
        // 且上一次t0尚未使用, 则需要保存值, 并标记为状态2
        mipsfile << "sw $t0," << lastTemp.getAddress() << "($sp)" << endl;
        curScope.setTempState(lastTemp.getName(), 2);
    }
    // 标记当前中间变量为def状态, 并更新上一次tempItem
    curScope.setTempState(dest.getName(), 0);
    curScope.setLastTemp(dest);

    switch (tuple.getOpType()) {
        case ADDOP: genAddOp(dReg, lReg, rReg); break;
        case SUBOP: genSubOp(dReg, lReg, rReg); break;
        case MULOP: genMulOp(dReg, lReg, rReg); break;
        case DIVOP: genDivOp(dReg, lReg, rReg); break;
        default:
            break;
    }
}
void MIPSGenerator::genAddOp(int rdReg, int rsReg, int rtReg) {
    mipsfile << "add $" << rdReg << ", $" << rsReg << ", $" << rtReg << endl;
}
void MIPSGenerator::genSubOp(int rdReg, int rsReg, int rtReg) {
    mipsfile << "sub $" << rdReg << ", $" << rsReg << ", $" << rtReg << endl;
}
void MIPSGenerator::genMulOp(int rdReg, int rsReg, int rtReg) {
    //mipsfile << "mult $" << rsReg << ", $" << rtReg << endl;
    //mipsfile << "mflo $" << rdReg << endl;
    mipsfile << "mul $" << rdReg << ", $" << rsReg << ", $" << rtReg << endl;
}
void MIPSGenerator::genDivOp(int rdReg, int rsReg, int rtReg) {
    mipsfile << "div $" << rsReg << ", $" << rtReg << endl;
    mipsfile << "mflo $" << rdReg << endl;
    //mipsfile << "div $" << rdReg << ", $" << rsReg << ", $" << rtReg << endl;
}
void MIPSGenerator::genUseArr(FourTuple tuple) {
    // dest = src1[src2]  ->  rd = rs[rt]
    // 1. 加载数组元素的值到 rd
    SymTableItem arr = tuple.getSrc1();
    SymTableItem index = tuple.getSrc2();

    // 事实上, dest一定为中间变量
    SymTableItem dest = tuple.getDest();
    SymTableItem lastTemp = curScope.getLastTemp();
    if (!curScope.isTempEmpty() && curScope.getTempState(lastTemp.getName()) == 0) {
        // 且上一次t0尚未使用, 则需要保存值, 并标记为状态2
        mipsfile << "sw $t0," << lastTemp.getAddress() << "($sp)" << endl;
        curScope.setTempState(lastTemp.getName(), 2);
    }
    // 标记当前中间变量为def状态, 并更新上一次tempItem
    curScope.setTempState(dest.getName(), 0);
    curScope.setLastTemp(dest);

    loadArr2Reg(arr, index, t0);
}
/* 3. 赋值语句 */
void MIPSGenerator::genAssVar(FourTuple tuple) {
    // dest = src1
    SymTableItem dest = tuple.getDest();
    SymTableItem src1 = tuple.getSrc1();
    // 一般情况下, 这里的 dest 是不为临时变量的
    int lReg = loadVar2Reg(src1, rd);
    saveVar4Reg(dest, lReg);
}
void MIPSGenerator::genAssArr(FourTuple tuple) {
    // dest[src2] = src1 -> rd[rt] = rs
    // 1. 加载相应的变量到 rd
    SymTableItem src1 = tuple.getSrc1();
    int lReg = loadVar2Reg(src1, rd);   // 未分配就用rd, 否则，用分配的寄存器。

    // 2. 赋给相应的数组元素
    SymTableItem arr = tuple.getDest();
    SymTableItem index = tuple.getSrc2();
    saveArr4Reg(arr, index, lReg);      // 保证计算时不破坏 rd 寄存器
}
/* 4. 条件语句 */
void MIPSGenerator::genSexprCond(FourTuple tuple, int destReg) {
    SymTableItem src1 = tuple.getSrc1();
    int lReg = loadVar2Reg(src1, rs);
    mipsfile << "sne $" << destReg << ", $" << lReg << ", $" << zero << endl;
}
void MIPSGenerator::genLessCond(FourTuple tuple, int destReg) {
    SymTableItem src1 = tuple.getSrc1();
    SymTableItem src2 = tuple.getSrc2();
    int lReg = loadVar2Reg(src1, rs);
    int rReg = loadVar2Reg(src2, rt);
    mipsfile << "slt $" << destReg << ", $" << lReg << ", $" << rReg << endl;
}
void MIPSGenerator::genLeqlCond(FourTuple tuple, int destReg) {
    SymTableItem src1 = tuple.getSrc1();
    SymTableItem src2 = tuple.getSrc2();
    int lReg = loadVar2Reg(src1, rs);
    int rReg = loadVar2Reg(src2, rt);
    mipsfile << "sle $" << destReg << ", $" << lReg << ", $" << rReg << endl;
}
void MIPSGenerator::genMoreCond(FourTuple tuple, int destReg) {
    SymTableItem src1 = tuple.getSrc1();
    SymTableItem src2 = tuple.getSrc2();
    int lReg = loadVar2Reg(src1, rs);
    int rReg = loadVar2Reg(src2, rt);
    mipsfile << "sgt $" << destReg << ", $" << lReg << ", $" << rReg << endl;
}
void MIPSGenerator::genMeqlCond(FourTuple tuple, int destReg) {
    SymTableItem src1 = tuple.getSrc1();
    SymTableItem src2 = tuple.getSrc2();
    int lReg = loadVar2Reg(src1, rs);
    int rReg = loadVar2Reg(src2, rt);
    mipsfile << "sge $" << destReg << ", $" << lReg << ", $" << rReg << endl;
}
void MIPSGenerator::genNeqlCond(FourTuple tuple, int destReg) {
    SymTableItem src1 = tuple.getSrc1();
    SymTableItem src2 = tuple.getSrc2();
    int lReg = loadVar2Reg(src1, rs);
    int rReg = loadVar2Reg(src2, rt);
    mipsfile << "sne $" << destReg << ", $" << lReg << ", $" << rReg << endl;
}
void MIPSGenerator::genEqlCond(FourTuple tuple, int destReg) {
    SymTableItem src1 = tuple.getSrc1();
    SymTableItem src2 = tuple.getSrc2();
    int lReg = loadVar2Reg(src1, rs);
    int rReg = loadVar2Reg(src2, rt);
    mipsfile << "seq $" << destReg << ", $" << lReg << ", $" << rReg << endl;
}
/* 5. 控制语句 */
/* 6. 函数调用语句 */
void MIPSGenerator::genFuncCall(vector<FourTuple> table, int st, int ed) {
    bool isInline = name2Scope[table[st].getName()].isInlineFunc();
    // 1. 处理函数调用开始语句
    genFuncCallBegin(table[st]);
    // 2. 处理函数调用语句
    /* 正常情况下, st -> FUNCALLBEGIN,  ed -> FUNCALLEND
     * 故 st-ed 之间只有 表达式计算与 函数调用语句 */
    stack<int> s;
    for (int i = st + 1; i < ed; ++i) {
        FourTuple tuple = table[i];
        if (tuple.getOpType() == FUNCALLBEGIN) {
            while (1) {
                if (table[i].getOpType() == FUNCALLBEGIN) {
                    s.push(i);
                }
                if (table[i].getOpType() == FUNCALLEND) {
                    s.pop();
                }

                if (s.empty()) break;
                i++;
            }
            continue;
        }
        // 其他情况, 处理表达式计算与 Push 语句
        switch (tuple.getOpType()) {
            /* 2. 表达式语句 */
            case ADDOP:
            case SUBOP:
            case MULOP:
            case DIVOP:
                genExprCalcInFunCall(tuple); break;
            case USEARR: genUseArrInFunCall(tuple); break;
            /* 6. 函数调用语句 */
            case HAVERETFUNCALL: genHaveRetFunCall(tuple); break;
            case NONRETFUNCALL: genNonRetFunCall(tuple); break;
            case PUSH:
                if (isInline)
                    genPushInline(tuple);
                else
                    genPushParam(tuple);
                break;
            /* 11. 内联函数 */
            case INLINE_FUNC_BEGIN:
                // 一个函数调用内部只可能有一个内联函数
                for (int j = i; j < ed; ++j) {
                    FourTuple tuple = table[j];
                    if (tuple.getOpType() == INLINE_FUNC_END) {
                        genInlineFunc(table, i, j);
                        i = j;
                        break;
                    }
                }
                break;
            default:
                break;
        }
    }
    // 3. 处理函数调用结束语句
    genFuncCallEnd(table[ed]);
}
void MIPSGenerator::genFuncCallBegin(FourTuple tuple) {
    // 为被调用函数开辟栈空间, 并将当前域的栈指针填入 prev abp 域
    lastScope = curScope;
    curScope = name2Scope[tuple.getName()];
    //mipsfile << "subi $sp, $sp, " << curScope.getDataSize() << endl;
    mipsfile << "addi $sp, $sp, " << -1*curScope.getDataSize() << endl;
}
void MIPSGenerator::genFuncCallEnd(FourTuple tuple) {
    // 释放被调用函数的栈空间, 并恢复 sp 的值
    mipsfile << "addi $sp, $sp, " << curScope.getDataSize() << endl;
    curScope = lastScope;
}
void MIPSGenerator::genHaveRetFunCall(FourTuple tuple) {
    /* 0. 已push值参 */
    SymTable func = name2Scope[tuple.getName()];
    /* 1. 保存当前状态 - 保存当前域所有变量到相应内存地址 */
    saveUsedRegs();
    save31Reg();
    /* 2. 执行函数 */
    mipsfile << "jal " << func.getScopeName() << endl;
    /* 3. 恢复到上一次的状态 */
    restore31Reg();
    restoreUsedRegs();
    /* 4. dest = RET */
    // 实际上, 这里dest一定为临时变量
    SymTableItem dest = tuple.getDest();
    SymTableItem lastTemp = lastScope.getLastTemp();
    if (!lastScope.isTempEmpty() && lastScope.getTempState(lastTemp.getName()) == 0) {
        // 且上一次t0尚未使用, 则需要保存值, 并标记为状态2
        mipsfile << "sw $t0, " << curScope.getDataSize() + lastTemp.getAddress() << "($sp)" << endl;
        lastScope.setTempState(lastTemp.getName(), 2);
    }
    // 标记当前变量为def状态, 并更新上一次tempItem
    lastScope.setTempState(dest.getName(), 0);
    lastScope.setLastTemp(dest);

    mipsfile << "move $t0, $v0" << endl;

}
void MIPSGenerator::genNonRetFunCall(FourTuple tuple) {
    /* 0. 已push值参 */
    SymTable func = name2Scope[tuple.getName()];
    /* 1. 保存当前状态 - 保存当前域所有变量到相应内存地址 */
    saveUsedRegs();
    save31Reg();
    /* 2. 执行函数 */
    mipsfile << "jal " << func.getScopeName() << endl;
    /* 3. 恢复到上一次的状态 */
    restore31Reg();
    restoreUsedRegs();
}
void MIPSGenerator::genPushParam(FourTuple tuple) {
    SymTableItem valPara = tuple.getDest();
    // 选择参数寄存器
    int paraNum = tuple.getParaNum();   // 第几个参数
    int paraReg = getParaReg(paraNum);  // 对应选择第几个寄存器
    if(paraReg > 0) {
        // 绝大多数情况, 参数个数小于4, 利用 $ax 寄存器传值即可
        //loadVar2Reg(valPara, paraReg);
    } else {
        // 若参数个数大于4, 则将值参存入相应的参数地址中
        SymTable func = name2Scope[tuple.getName()];
        vector<SymTableItem> paramList = func.getParamList();
        SymTableItem param = paramList[paraNum];

        int reg = loadPreVar2Reg(valPara, rd);   // rd = val
        savePara4Reg(param, reg);
    }
}
void MIPSGenerator::genPushInline(FourTuple tuple) {
    SymTableItem valPara = tuple.getDest();
    // 选择参数寄存器
    int paraNum = tuple.getParaNum();   // 第几个参数
    int paraReg = getParaReg(paraNum);  // 对应选择第几个寄存器
    if(paraReg > 0) {
        // 绝大多数情况, 参数个数小于4, 利用 $ax 寄存器传值即可
        //loadVar2Reg(valPara, paraReg);
    } else {
        // 若参数个数大于4, 则将值参存入相应的参数地址中
        SymTable func = name2Scope[tuple.getName()];
        vector<SymTableItem> paramList = func.getParamList();
        SymTableItem param = paramList[paraNum];

        int srcReg = loadPreVar2Reg(valPara, rd);   // rd = val
        saveVar4Reg(param, srcReg);
    }
}
int MIPSGenerator::savePara4Reg(SymTableItem item, int reg) {
    mipsfile << "sw $" << reg << ", " << item.getAddress() << "($sp)" << endl;
    return reg;
}
void MIPSGenerator::genExprCalcInFunCall(FourTuple tuple) {
    // + - * /
    int lReg = loadPreVar2Reg(tuple.getSrc1(), rs);
    int rReg = loadPreVar2Reg(tuple.getSrc2(), rt);
    int dReg = t0;
    // 事实上, dest一定为中间变量
    SymTableItem dest = tuple.getDest();
    SymTableItem lastTemp = lastScope.getLastTemp();
    if (!lastScope.isTempEmpty() && lastScope.getTempState(lastTemp.getName()) == 0) {
        // 且上一次t0尚未使用, 则需要保存值, 并标记为状态2
        mipsfile << "sw $t0," << curScope.getDataSize() + lastTemp.getAddress() << "($sp)" << endl;
        lastScope.setTempState(lastTemp.getName(), 2);
    }
    // 标记当前中间变量为def状态, 并更新上一次tempItem
    lastScope.setTempState(dest.getName(), 0);
    lastScope.setLastTemp(dest);

    switch (tuple.getOpType()) {
        case ADDOP: genAddOp(dReg, lReg, rReg); break;
        case SUBOP: genSubOp(dReg, lReg, rReg); break;
        case MULOP: genMulOp(dReg, lReg, rReg); break;
        case DIVOP: genDivOp(dReg, lReg, rReg); break;
        default:
            break;
    }
}
void MIPSGenerator::genUseArrInFunCall(FourTuple tuple) {
    // dest = src1[src2]  ->  rd = rs[rt]
    // 1. 加载数组元素的值到 rd
    SymTableItem arr = tuple.getSrc1();
    SymTableItem index = tuple.getSrc2();
    // 事实上, dest一定为中间变量
    SymTableItem dest = tuple.getDest();
    SymTableItem lastTemp = lastScope.getLastTemp();
    if (!lastScope.isTempEmpty() && lastScope.getTempState(lastTemp.getName()) == 0) {
        // 且上一次t0尚未使用, 则需要保存值, 并标记为状态2
        mipsfile << "sw $t0," << curScope.getDataSize() + lastTemp.getAddress() << "($sp)" << endl;
        lastScope.setTempState(lastTemp.getName(), 2);
    }
    // 标记当前中间变量为def状态, 并更新上一次tempItem
    lastScope.setTempState(dest.getName(), 0);
    lastScope.setLastTemp(dest);

    loadPreArr2Reg(arr, index, t0);
}

/* 7. 写语句 */
void MIPSGenerator::genPrintString(FourTuple tuple) {
    mipsfile << "la $a0, " << tuple.getName() << endl;
    genSyscall(printStr);
}
void MIPSGenerator::genPrintIntExpr(FourTuple tuple) {
    SymTableItem item = tuple.getDest();
    // 1. a0 = var
    int reg = loadVar2Reg(item, a0);
    if (reg != a0) {
        mipsfile << "move $a0, $" << reg << endl;
    }
    // 2. print
    genSyscall(printInt);
}
void MIPSGenerator::genPrintCharExpr(FourTuple tuple) {
    SymTableItem item = tuple.getDest();
    // 1. a0 = var
    int reg = loadVar2Reg(item, a0);
    if (reg != a0) {
        mipsfile << "move $a0, $" << reg << endl;
    }
    // 2. print
    genSyscall(printChar);
}
/* 8. 读语句 */
void MIPSGenerator::genReadInt(FourTuple tuple) {
    SymTableItem item = tuple.getDest();
    // 1. 读入int
    genSyscall(readInt);
    // 2. 赋给相应变量
    saveVar4Reg(item, v0);
}
void MIPSGenerator::genReadChar(FourTuple tuple) {
    SymTableItem item = tuple.getDest();
    // 1. 读入char
    genSyscall(readChar);
    // 2. 赋给相应变量
    saveVar4Reg(item, v0);
}
/* 9. 返回语句 */
void MIPSGenerator::genRetVal(FourTuple tuple) {
    // return (t0)
    int reg = loadVar2Reg(tuple.getDest(), v0);
    if (reg != v0) {
        mipsfile << "move $v0, $" << reg << endl;
    }
    mipsfile << "jr $" << ra << endl;
}
void MIPSGenerator::genRetNon(FourTuple tuple) {
    // return ;
    mipsfile << "jr $" << ra << endl;
}
/* 10. 其他 */
void MIPSGenerator::genLabel(FourTuple tuple) {
    mipsfile << tuple.getName() << ":" << endl;
    if (name2Scope.count(tuple.getName()) > 0) {
        // 若该函数的参数被分配了寄存器, 则先将参数加载到相应的寄存器中
        loadAllParams();
    }
}
void MIPSGenerator::genJump(FourTuple tuple) {
    mipsfile << "j " << tuple.getName() << endl;
}
void MIPSGenerator::genJumpRet() {
    mipsfile << "jr $" << ra << endl;
}
void MIPSGenerator::genBeq(FourTuple tuple, int reg1, int reg2) {
    mipsfile << "beq $" << reg1 << ", $" << reg2 << ", " << tuple.getName() << endl;
}
void MIPSGenerator::genBne(FourTuple tuple, int reg1, int reg2) {
    mipsfile << "bne $" << reg1 << ", $" << reg2 << ", " << tuple.getName() << endl;
}
void MIPSGenerator::genSyscall(int option) {
    mipsfile << "li $v0, " << option << endl;
    mipsfile << "syscall" << endl;
}

/* 11. 内联函数 */
void MIPSGenerator::genInlineFunc(vector<FourTuple> table, int st, int ed) {
    SymTableItem lastTemp = lastScope.getLastTemp();
    if (!lastScope.isTempEmpty() && lastScope.getTempState(lastTemp.getName()) == 0) {
        mipsfile << "sw $t0," << curScope.getDataSize() + lastTemp.getAddress() << "($sp)" << endl;
        lastScope.setTempState(lastTemp.getName(), 2);
    }

    int reg;
    for (int i = st; i <= ed; ++i) {
        FourTuple tuple = table[i];
        switch (tuple.getOpType()) {
            /* 1. 声明语句 */
            /* 2. 表达式语句 */
            case ADDOP:
            case SUBOP:
            case MULOP:
            case DIVOP:
                genExprCalc(tuple); break;
            case USEARR: genUseArr(tuple); break;
            /* 3. 赋值语句 */
            case ASSVAR: genAssVar(tuple); break;
            case ASSARR: genAssArr(tuple); break;
            /* 4. 条件语句 */
            case SEXPR: genSexprCond(tuple, rd); break;
            case LESS: genLessCond(tuple, rd); break;
            case LEQL: genLeqlCond(tuple, rd); break;
            case MORE: genMoreCond(tuple, rd); break;
            case MEQL: genMeqlCond(tuple, rd); break;
            case NEQUAL: genNeqlCond(tuple, rd); break;
            case EQUAL: genEqlCond(tuple, rd); break;
            /* 5. 控制语句 */
            /* 6. 函数调用语句 */
            /* 7. 写语句 */
            case WRITESTRING: genPrintString(tuple); break;
            case WRITEINTEXPR: genPrintIntExpr(tuple); break;
            case WRITECHAREXPR: genPrintCharExpr(tuple); break;
            /* 8. 读语句 */
            case READINT: genReadInt(tuple); break;
            case READCHAR: genReadChar(tuple); break;
            /* 9. 返回语句 */
            /* 10. 其他 */
            case GENLABEL: genLabel(tuple); break;
            case JUMP: genJump(tuple); break;
            case BEQ: genBeq(tuple, rd, zero); break;
            case BNE: genBne(tuple, rd, zero); break;
            /* 11. 内联函数 */
            /*
            case INLINE_RET:
                reg = loadVar2Reg(tuple.getDest(), v0);
                if (reg != v0) {
                    mipsfile << "move $v0, $" << reg << endl;
                }
                break;
                */
            default:
                break;
        }
    }

    if (table[ed+1].getOpType() == INLINE_CALL) {
        //savePreVar4Reg(table[ed+1].getDest(), v0);
        // table[ed-1] 一定是 inline ret
        reg = loadVar2Reg(table[ed-1].getDest(), v0);
        savePreVar4Reg(table[ed+1].getDest(), reg);
    }
}
