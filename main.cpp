#include <iostream>
#include <string>
#include "Constant.h"
#include "Error.h"
#include "LexicalAnalysis.h"
#include "SyntaxAnalysis.h"
#include "SymTableMngr.h"
#include "TmpCodeTable.h"
#include "SemanticAnalysis.h"
#include "MIPSGenerator.h"
#include "Optimizer.h"

using namespace std;

string infilePath("./testfile.txt");

int main() {
    /* BEGIN */
    cout << "Welcome to my Compiler." << endl;

    // 0. 声明全局 Error类
    Error myError(errorFilePath);
    SymTableMngr mySymTableMngr;
    TmpCodeTable myTmpCodeTable;

    // 1. 词法分析
    LexicalAnalysis myLexical(myError, infilePath);
    //myLexical.analysis();

    // 2. 语法分析 + 语义分析(生成结构化的中间代码)
    SemanticAnalysis mySemantic(myTmpCodeTable, mySymTableMngr);    // 动作语义子程序
    SyntaxAnalysis mySyntax(myLexical, myError, mySymTableMngr, mySemantic);
    mySyntax.analysis();

    if (myError.getErrNum() != 0) {
        // 若检测出错误, 则编译不通过
        cout << "Compile Failed, detected " << myError.getErrNum() << " error(s)." << endl;
        return 0;
    }

    // 3. 中间代码优化
    myTmpCodeTable.output(tmpCodeFilePath);
    if (optimize) {
        // 中间代码优化
        Optimizer optimizer(mySymTableMngr, myTmpCodeTable);
        myTmpCodeTable = optimizer.optimize();
        myTmpCodeTable.output(opedTmpCodefFilePath);
    }

    // 4. 生成MIPS代码
    MIPSGenerator myMIPSGenerator(mipsFilePath, mySymTableMngr, myTmpCodeTable);
    myMIPSGenerator.generate();

    /* END */
    cout << "Thanks for using." << endl;
    return 0;
}