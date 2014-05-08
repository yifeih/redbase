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

//int isDir(const char *file){
//    struct stat buf;
//    stat(file, &buf);
//    return S_ISREG(buf.st_mode);
//}

//
// main
//
int main(int argc, char *argv[])
{
    char *dbname;
    char command[255] = "mkdir ";
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
        cerr << argv[1] << " length exceeds maximum allowed, cannot create database\n";
        exit(1);
    }

    // Create a subdirectory for the database
    int error = system (strcat(command,dbname));

    if(error){
        cerr << "system call to create directory exited with error code " << error << endl;
        exit(1);
    }

    if (chdir(dbname) < 0) {
        cerr << argv[0] << " chdir error to " << dbname << "\n";
        exit(1);
    }

    // Create the system catalogs...

    PF_Manager pfm;
    RM_Manager rmm(pfm);

    // Calculate the size of entries in relcat:
    int relCatRecSize = sizeof(RelCatEntry);
    int attrCatRecSize = sizeof(AttrCatEntry);

    if((rc = rmm.CreateFile("relcat", relCatRecSize))){
        cerr << "Trouble creating relcat. Exiting" <<endl;
        exit(1);
    }

    if((rc = rmm.CreateFile("attrcat", attrCatRecSize))){
        cerr << "Trouble creating attrcat. Exiting" <<endl;
        exit(1);
    }



    return(0);
}
