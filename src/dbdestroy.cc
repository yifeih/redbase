//
// dbcreate.cc
//
// Author: Jason McHugh (mchughj@cs.stanford.edu)
//
// This shell is provided for the student.

#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "rm.h"
#include "sm.h"
#include "redbase.h"
#include <climits>


using namespace std;


//
// main
//
int main(int argc, char *argv[])
{
    char *dbname;
    char command[255] = "rm -r ";
    RC rc;

    // Look for 2 arguments. The first is always the name of the program
    // that was executed, and the second should be the name of the
    // database.
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " dbname \n";
        exit(1);
    }

    // The database name is the second argument
    dbname = argv[1];
    if(strlen(argv[1]) > (sizeof(command)-strlen(command) - 1)){
        cerr << argv[1] << " length exceeds maximum allowed, cannot delete database\n";
        exit(1);
    }

    // Create a subdirectory for the database
    int error = system (strcat(command,dbname));

    if(error){
        cerr << "system call to destroy directory exited with error code " << error << endl;
        exit(1);
    }

    /*
    if (chdir(dbname) < 0) {
        cerr << argv[0] << " chdir error to " << dbname << "\n";
        exit(1);
    }
    */


    return(0);
}
