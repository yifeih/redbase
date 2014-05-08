//
// redbase.cc
//
// Author: Jason McHugh (mchughj@cs.stanford.edu)
//
// This shell is provided for the student.

#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "redbase.h"
#include "rm.h"
#include "sm.h"
#include "ql.h"

using namespace std;

PF_Manager pfm;
RM_Manager rmm(pfm);
IX_Manager ixm(pfm);
SM_Manager smm(ixm, rmm);
QL_Manager qlm(smm, ixm, rmm);
//
// main
//
int main(int argc, char *argv[])
{
    char *dbname;
    RC rc;

    // Look for 2 arguments.  The first is always the name of the program
    // that was executed, and the second should be the name of the
    // database.
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " dbname \n";
        exit(1);
    }

    /*
    char char1[10] = "asdf\0";
    char char2[20] = "asdf\0    ";

    cout << "size of string 1: " << strlen(char1) << endl;
    cout << "size of string 1: " << strlen(char2) << endl;
    int comp = strncmp(char1, char2, 10);
    cout << "compare result: " << comp << endl;
    */

    dbname = argv[1];
    if ((rc = smm.OpenDb(dbname))) {
        PrintError(rc);
        return (1);
    }

    RBparse(pfm, smm, qlm);

    if ((rc = smm.CloseDb())) {
        PrintError(rc);
        return (1);
    }


    // *********************************
    //
    // Fair amount to be filled in here!!
    //
    // *********************************

    cout << "Bye.\n";
}
