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

    // 0. ����ȫ�� Error��
    Error myError(errorFilePath);
    SymTableMngr mySymTableMngr;
    TmpCodeTable myTmpCodeTable;

    // 1. �ʷ�����
    LexicalAnalysis myLexical(myError, infilePath);
    //myLexical.analysis();

    // 2. �﷨���� + �������(���ɽṹ�����м����)
    SemanticAnalysis mySemantic(myTmpCodeTable, mySymTableMngr);    // ���������ӳ���
    SyntaxAnalysis mySyntax(myLexical, myError, mySymTableMngr, mySemantic);
    mySyntax.analysis();

    if (myError.getErrNum() != 0) {
        // ����������, ����벻ͨ��
        cout << "Compile Failed, detected " << myError.getErrNum() << " error(s)." << endl;
        return 0;
    }

    // 3. �м�����Ż�
    myTmpCodeTable.output(tmpCodeFilePath);
    if (optimize) {
        // �м�����Ż�
        Optimizer optimizer(mySymTableMngr, myTmpCodeTable);
        myTmpCodeTable = optimizer.optimize();
        myTmpCodeTable.output(opedTmpCodefFilePath);
    }

    // 4. ����MIPS����
    MIPSGenerator myMIPSGenerator(mipsFilePath, mySymTableMngr, myTmpCodeTable);
    myMIPSGenerator.generate();

    /* END */
    cout << "Thanks for using." << endl;
    return 0;
}