//
// Created by Klaus-Mikealson on 2019/12/12.
//

#include <stack>
#include <algorithm>
#include "Optimizer.h"

Optimizer::Optimizer(SymTableMngr &mySymTableMngr, TmpCodeTable &myTmpCodeTable):
mySymTableMngr(mySymTableMngr), myTmpCodeTable(myTmpCodeTable) {

}
Optimizer::~Optimizer() {}

TmpCodeTable Optimizer::optimize() {
    // 复制传播
    // optCopyProp();
    // 死代码删除
    //optDeadCode();
    // 循环优化
    optLoop();
    // 代码内联
    optCodeInline();
    // 引用计数，分配寄存器
    optRegAlloc();

    return myOpedTmpCodeTable;
}

void Optimizer::optCopyProp() {
    myOpedTmpCodeTable.clear();
    vector<FourTuple> table = myTmpCodeTable.getTable();
    for (int i = 0; i < table.size(); ++i) {
        FourTuple tuple = table[i];
        int st, ed;
        if (tuple.getOpType() == FUNCDEFBEGIN) {
            st = i;
        } else if (tuple.getOpType() == FUNCDEFEND) {
            ed = i;

        }
    }
}

void Optimizer::optCopyPropagation(int st, int ed, vector<FourTuple> table) {
    bool flag;

    SymTable func = mySymTableMngr.getScope(table[st].getName());
    map<string, SymTableItem> name2Var = func.getName2Var();
    for (map<string, SymTableItem>::iterator iter = name2Var.begin(); iter != name2Var.end(); iter++) {
        SymTableItem x = iter->second;
        bool first_def = true;
        for (int i = st; i < ed; ++i) {
            FourTuple tuple = table[i];
            if (tuple.getOpType() == ASSVAR && tuple.getDest().getName().compare(x.getName()) == 0) {
                // 若存在对该变量的变量复制语句 x = y, 且 y 不为临时中间变量
                SymTableItem y = tuple.getSrc1();
                if (func.isIdfrTemp(y.getName())) {
                    continue;
                }


            }
        }

    }
}

void Optimizer::optDeadCode() {
    myOpedTmpCodeTable.clear();
    vector<FourTuple> table = myTmpCodeTable.getTable();
    int st, ed;
    for (int i = 0; i < table.size(); ++i) {
        if (table[i].getOpType() == FUNCDEFBEGIN) {
            st = i;
            for (int j = st+1; j < table.size(); ++j) {
                if (table[j].getOpType() == FUNCDEFEND) {
                    ed = j;
                    table = deleteDeadCode(st, ed, table);
                    break;
                }
            }
        }
    }

    myOpedTmpCodeTable.addTupleList(table);
    myTmpCodeTable = myOpedTmpCodeTable;
}

vector<FourTuple> Optimizer::deleteDeadCode(int st, int ed, vector<FourTuple> table) {
    vector<FourTuple> opTable;
    vector<int> deleteList;
    SymTable func = mySymTableMngr.getScope(table[st].getName());
    map<string, SymTableItem> name2Var = func.getName2Var();
    for (map<string, SymTableItem>::iterator iter = name2Var.begin(); iter != name2Var.end(); iter++) {
        SymTableItem x = iter->second;
        int las_def_index;
        bool first_def = true;
        bool used;
        for (int i = st; i < ed; ++i) {
            FourTuple tuple = table[i];
            switch (tuple.getOpType()) {
                case ADDOP:
                case SUBOP:
                case MULOP:
                case DIVOP:
                case USEARR:
                case ASSVAR:
                case ASSARR:
                    if (tuple.getDest().getName().compare(x.getName()) == 0) {
                        if (first_def) {
                            // 若 x 第一次定义
                            first_def = false;
                            used = false;
                        } else {
                            // 若 x 被多次定义时
                            if (!used) {
                                deleteList.push_back(las_def_index);
                            }
                            used = false;
                        }
                        las_def_index = i;  // 更新上次定义的index
                    } else if (tuple.getSrc1().getName().compare(x.getName()) == 0
                            || tuple.getSrc2().getName().compare(x.getName()) == 0) {
                        used = true;
                    }
                    break;

                case SEXPR:
                case LESS:
                case LEQL:
                case MORE:
                case MEQL:
                case NEQUAL:
                case EQUAL:
                    if (tuple.getSrc1().getName().compare(x.getName()) == 0
                        || tuple.getSrc2().getName().compare(x.getName()) == 0) {
                        used = true;
                    }
                    break;

                case PUSH:
                case WRITEINTEXPR:
                case WRITECHAREXPR:
                case RETVAL:
                    if (tuple.getDest().getName().compare(x.getName()) == 0) {
                        used = true;
                    }
                    break;

                case READINT:
                case READCHAR:
                    if (tuple.getDest().getName().compare(x.getName()) == 0) {
                        if (first_def) {
                            // 若 x 第一次定义
                            first_def = false;
                            used = false;
                        } else {
                            // 若 x 被多次定义时
                            if (!used) {
                                deleteList.push_back(las_def_index);
                            }
                            used = false;
                        }
                        las_def_index = i;  // 更新上次定义的index
                    }
                    break;
                default:
                    break;
            }
        }
        if (!used) {
            deleteList.push_back(las_def_index);
        }
    }

    for (int i = 0; i < st; ++i) {
        opTable.push_back(table[i]);
    }
    for (int i = st; i < ed; ++i) {
        if (std::find(deleteList.begin(), deleteList.end(), i) != deleteList.end() ) {
            continue;
        }
        opTable.push_back(table[i]);
    }
    for (int i = ed; i < table.size(); ++i) {
        opTable.push_back(table[i]);
    }
    return opTable;
}

void Optimizer::optLoop() {
    myOpedTmpCodeTable.clear();
    vector<FourTuple> table = myTmpCodeTable.getTable();
    stack<int> s;
    for (int i = 0; i < table.size(); ++i) {
        FourTuple tuple = table[i];
        switch (tuple.getOpType()) {
            case WHILE_BEGIN:
            case FOR_BEGIN:
                for (int j = i; j < table.size(); ++j) {
                    FourTuple ft = table[j];
                    if (ft.getOpType() == WHILE_BEGIN || ft.getOpType() == FOR_BEGIN) {
                        s.push(j);
                    } else if (ft.getOpType() == WHILE_END || ft.getOpType() == FOR_END) {
                        int st = s.top(); s.pop();
                        int ed = j;
                        if (s.empty()) {
                            // 说明找到了匹配的 st - ed, 生成相应的中间代码
                            if (table[st].getOpType() == WHILE_BEGIN) {
                                // while-loop
                                myOpedTmpCodeTable.addTuple(table[st]);
                                table = genWhileLoop(st, ed, table);
                            } else {
                                // for-loop
                                myOpedTmpCodeTable.addTuple(table[st]);
                                table = genForLoop(st, ed, table);
                            }
                            break;
                        }
                    }
                }
                break;
            default:
                myOpedTmpCodeTable.addTuple(tuple);
                break;
        }
    }

    myTmpCodeTable = myOpedTmpCodeTable;    // 保证下一步优化是在当前优化的基础上进行的
}
vector<FourTuple> Optimizer::genWhileLoop(int st, int ed, vector<FourTuple> table) {
    vector<FourTuple> opTable;
    /* 优化前 */
    int k;
    FourTuple while_begin = table[st];  // st
    FourTuple begin_label = table[st+1];// st+1
    //FourTuple condition = table[st+2];// st+2
    vector<FourTuple> conditions;       // 注意, condition 可能不只有一条四元式
    for (k = st + 2; k < table.size(); ++k) {
        if (table[k].getOpType() == BEQ) {
            break;
        }
        conditions.push_back(table[k]);
    }
    FourTuple beq = table[k];        // st+3

    // statements
    FourTuple jump = table[ed-2];       // ed-2
    FourTuple end_label = table[ed-1];  // ed-1
    FourTuple while_end = table[ed];    // ed

    /* 优化后 */
    for (int i = 0; i < st; ++i) {
        opTable.push_back(table[i]);
    }
    opTable.push_back(while_begin);
    opTable.insert(opTable.end(), conditions.begin(), conditions.end());
    opTable.push_back(beq);
    opTable.push_back(begin_label);
    // statements
    for (int i = k+1; i < ed-2; ++i) {
        opTable.push_back(table[i]);
    }
    opTable.insert(opTable.end(), conditions.begin(), conditions.end());
    FourTuple bne(BNE, begin_label.getName());
    opTable.push_back(bne);
    opTable.push_back(end_label);
    opTable.push_back(while_end);
    for (int i = ed+1; i < table.size(); ++i) {
        opTable.push_back(table[i]);
    }

    return opTable;
}
vector<FourTuple> Optimizer::genForLoop(int st, int ed, vector<FourTuple> table) {
    vector<FourTuple> opTable;
    /* 优化前 */
    int k;
    FourTuple for_begin = table[st];    // st
    //FourTuple init_expr = table[st+1];  // st+1
    vector<FourTuple> init_exprs;       // 注意, init_exprs 可能不只有一条四元式
    for (k = st + 1; k < table.size(); ++k) {
        if (table[k].getOpType() == GENLABEL) {
            break;
        }
        init_exprs.push_back(table[k]);
    }
    FourTuple begin_label = table[k];// st+2
    //FourTuple condition = table[st+3];  // st+3
    vector<FourTuple> conditions;       // 注意, condition 可能不只有一条四元式
    for (k = k + 1; k < table.size(); ++k) {
        if (table[k].getOpType() == BEQ) {
            break;
        }
        conditions.push_back(table[k]);
    }
    FourTuple beq = table[k];        // st+4
    // statements
    FourTuple jump = table[ed-2];       // ed-2
    FourTuple end_label = table[ed-1];  // ed-1
    FourTuple for_end = table[ed];    // ed

    /* 优化后 */
    for (int i = 0; i < st; ++i) {
        opTable.push_back(table[i]);
    }
    opTable.push_back(for_begin);
    opTable.insert(opTable.end(), init_exprs.begin(), init_exprs.end());
    opTable.insert(opTable.end(), conditions.begin(), conditions.end());
    opTable.push_back(beq);
    opTable.push_back(begin_label);
    // statements
    for (int i = k+1; i < ed-2; ++i) {
        opTable.push_back(table[i]);
    }
    opTable.insert(opTable.end(), conditions.begin(), conditions.end());
    FourTuple bne(BNE, begin_label.getName());
    opTable.push_back(bne);
    opTable.push_back(end_label);
    opTable.push_back(for_end);
    for (int i = ed+1; i < table.size(); ++i) {
        opTable.push_back(table[i]);
    }

    return opTable;
}

void Optimizer::optCodeInline() {
    myOpedTmpCodeTable.clear();
    vector<FourTuple> table = myTmpCodeTable.getTable();
    /* 1. 标记内联函数 */
    map<string, pair<int, int>> mark;
    bool flag;    // 内联函数标记, 若有函数调用语句, 则将 flag 置为 false
    int st, ed, retNum;
    for (int i = 0; i < table.size(); ++i) {
        FourTuple tuple = table[i];
        switch (tuple.getOpType()) {
            case FUNCDEFBEGIN:
                st = i;
                flag = true;
                retNum = 0;
                break;
            case FUNCDEFEND:
                if (table[i-1].getOpType() != RETVAL && table[i-1].getOpType() != RETNON) {
                    retNum++;
                }
                ed = i;
                if (flag && retNum <= 1) {
                    string key = tuple.getName();
                    pair<int, int> value(st, ed);
                    pair<string, pair<int, int>> m(key, value);
                    mark.insert(m);
                }
                break;
            case FOR_BEGIN:
            case DO_BEGIN:
            case WHILE_BEGIN:
            case FUNCALLBEGIN:
                flag = false;
                break;
            case RETVAL:
            case RETNON:
                retNum++;
                break;
            default:
                break;
        }
    }
    // 注意删除main
    mark.erase(MAIN);

    /* 2. 内联函数替换 */
    for (int i = 0; i < table.size(); ++i) {
        FourTuple tuple = table[i];
        string name = tuple.getName();
        switch (tuple.getOpType()) {
            case FUNCDEFBEGIN:
                if (mark.count(name) > 0) {
                    // 若为内联函数, 则跳过本函数定义
                    i = mark[tuple.getName()].second;
                } else {
                    myOpedTmpCodeTable.addTuple(tuple);
                }
                break;

            case HAVERETFUNCALL:
            case NONRETFUNCALL:
                if (mark.count(name) > 0) {
                    // 若为内联函数, 替换
                    int st = mark[name].first;
                    int ed = mark[name].second;
                    vector<FourTuple> inlineTable = genInlineFunc(st, ed, table);

                    if (tuple.getOpType() == HAVERETFUNCALL) {
                        // 对于有返回值函数, 需要多加一条load语句
                        FourTuple ft(INLINE_CALL, tuple.getDest());
                        inlineTable.push_back(ft);
                    }
                    myOpedTmpCodeTable.addTupleList(inlineTable);
                } else {
                    myOpedTmpCodeTable.addTuple(tuple);
                }
                break;
            default:
                myOpedTmpCodeTable.addTuple(tuple);
                break;
        }
    }

    myTmpCodeTable = myOpedTmpCodeTable;    // 保证下一步优化是在当前优化的基础上进行的
}

vector<FourTuple> Optimizer::genInlineFunc(int st, int ed, vector<FourTuple> table) {
    vector<FourTuple> inlineTable;
    for (int i = st; i <= ed; ++i) {
        FourTuple tuple = table[i];
        switch (tuple.getOpType()) {
            case FUNCDEFBEGIN:
                controlState(inlineTable, INLINE_FUNC_BEGIN, tuple.getName());
                break;
            case FUNCDEFEND:
                controlState(inlineTable, INLINE_FUNC_END, tuple.getName());
                break;
            case PUSH:
                pushState(inlineTable, tuple);
                break;
            case RETVAL:
            case RETNON:
                retState(inlineTable, tuple);
                break;
            case GENLABEL:
            case JUMP:
            case BEQ:
            case BNE:
                modifyLabel(inlineTable, tuple);
                break;
            default:
                // 其余情况无需变动, 直接生成即可
                inlineTable.push_back(tuple);
                break;
        }
    }

    SymTable fun = mySymTableMngr.getScope(table[st].getName());
    fun.setInline();
    mySymTableMngr.updScope(fun);

    // 更新 inline 计数器
    inline_num++;
    return inlineTable;
}

void Optimizer::optRegAlloc() {
    // 为各个域分配寄存器
    vector<FourTuple> table = myTmpCodeTable.getTable();
    for (int i = 0; i < table.size(); ++i) {
        FourTuple tuple = table[i];
        int st, ed;
        int inline_st, inline_ed;
        SymTable func;
        switch (tuple.getOpType()) {
            case FUNCDEFBEGIN:
                st = i;
                break;
            case FUNCDEFEND:
                ed = i;
                func = mySymTableMngr.getScope(tuple.getName());
                func.initRefCount(st, ed, table);
                func.allocReg(regs, 15);
                mySymTableMngr.updScope(func);
                break;

            case INLINE_FUNC_BEGIN:
                inline_st = i;
                break;
            case INLINE_FUNC_END:
                inline_ed = i;
                func = mySymTableMngr.getScope(tuple.getName());
                func.initRefCount(inline_st+1, inline_ed, table);
                func.allocReg(inlineRegs, 3);
                mySymTableMngr.updScope(func);
                break;

            default:
                break;
        }
    }
}

void Optimizer::controlState(vector<FourTuple>& inlineTable, Operator op, string funcName) {
    // 标记控制语句的开始或结束
    FourTuple ft(op, funcName);
    inlineTable.push_back(ft);
}
void Optimizer::pushState(vector<FourTuple> &inlineTable, FourTuple tuple) {
    // 将last域的变量 sw 到 cur 域对应的地址中
    // 好像不用变哈!
    inlineTable.push_back(tuple);
}
void Optimizer::retState(vector<FourTuple> &inlineTable, FourTuple tuple) {
    // 无返回值无需处理
    if (tuple.getOpType() == RETNON) {
        return;
    }
    // 若有返回值, 只需将当前返回值lw到v0中即可
    FourTuple ft(INLINE_RET, tuple.getDest());
    inlineTable.push_back(ft);
}
void Optimizer::modifyLabel(vector<FourTuple> &inlineTable, FourTuple tuple) {
    // 添加后缀名
    stringstream ss;
    string prefix = tuple.getName();
    ss << prefix << suffix << inline_num;
    string newName = ss.str();

    FourTuple ft(tuple.getOpType(), newName);
    inlineTable.push_back(ft);
}

