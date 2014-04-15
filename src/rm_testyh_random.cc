//
// File:        rm_testshell.cc
// Description: Test RM component
// Authors:     Yifei Huang (yifei@stanford.edu)
//
// This test shell contains a number of functions that will be useful
// in testing your RM component code.  In addition, a couple of sample
// tests are provided.  The tests are by no means comprehensive, however,
// and you are expected to devise your own tests to test your code.
//
// 1997:  Tester has been modified to reflect the change in the 1997
// interface.  For example, FileHandle no longer supports a Scan over the
// relation.  All scans are done via a FileScan.

#include <cstdio>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <vector>

#include "redbase.h"
#include "pf.h"
#include "rm.h"
#include <ctime>

using namespace std;

//
// Defines
//
#define FILENAME   "testrel"         // test file name
#define STRLEN      29               // length of string in testrec
#define PROG_UNIT   50               // how frequently to give progress
                                      //   reports when adding lots of recs
#define FEW_RECS   400                // number of records added in

//
// Computes the offset of a field in a record (should be in <stddef.h>)
//
#ifndef offsetof
#       define offsetof(type, field)   ((size_t)&(((type *)0) -> field))
#endif

//
// Structure of the records we will be using for the tests
//
struct TestRec {
    char  str[STRLEN];
    int   num;
    float r;
};

//
// Global PF_Manager and RM_Manager variables
//
#define MAXRECS 10000
PF_Manager pfm;
RM_Manager rmm(pfm);
//vector<int> deleted_indices;
//vector<int> inserted_indeces;
int inserted_indices[MAXRECS];
int deleted_indices[MAXRECS];
RM_Record added_recs[MAXRECS];
int recsInFile;
int maxIndex;

//
// Function declarations
//
RC Test1(void);
RC Test2(void);
RC Test3(void);

void PrintError(RC rc);
void LsFile(char *fileName);
void PrintRecord(TestRec &recBuf);
RC AddRecs(RM_FileHandle &fh, int numRecs);
RC VerifyFile(RM_FileHandle &fh, int numRecs);
RC PrintFile(RM_FileHandle &fh);

RC CreateFile(char *fileName, int recordSize);
RC DestroyFile(char *fileName);
RC OpenFile(char *fileName, RM_FileHandle &fh);
RC CloseFile(char *fileName, RM_FileHandle &fh);
RC InsertRec(RM_FileHandle &fh, char *record, RID &rid);
RC UpdateRec(RM_FileHandle &fh, RM_Record &rec);
RC DeleteRec(RM_FileHandle &fh, RID &rid);
RC GetNextRecScan(RM_FileScan &fs, RM_Record &rec);

//
// Array of pointers to the test functions
//
#define NUM_TESTS       3               // number of tests
int (*tests[])() =                      // RC doesn't work on some compilers
{
    Test1,
    Test2,
    Test3
};

//
// main
//
int main(int argc, char *argv[])
{
    RC   rc;
    char *progName = argv[0];   // since we will be changing argv
    int  testNum;

    // Write out initial starting message
    cerr.flush();
    cout.flush();
    cout << "Starting RM component test.\n";
    cout.flush();


    // Delete files from last time
    unlink(FILENAME);

    

    // If no argument given, do all tests
    if (argc == 1) {
        for (testNum = 0; testNum < NUM_TESTS; testNum++)
            if ((rc = (tests[testNum])())) {

                // Print the error and exit
                PrintError(rc);
                return (1);
            }
    }
    else {

        // Otherwise, perform specific tests
        while (*++argv != NULL) {

            // Make sure it's a number
            if (sscanf(*argv, "%d", &testNum) != 1) {
                cerr << progName << ": " << *argv << " is not a number\n";
                continue;
            }

            // Make sure it's in range
            if (testNum < 1 || testNum > NUM_TESTS) {
                cerr << "Valid test numbers are between 1 and " << NUM_TESTS << "\n";
                continue;
            }

            // Perform the test
            if ((rc = (tests[testNum - 1])())) {

                // Print the error and exit
                PrintError(rc);
                return (1);
            }
        }
    }

    // Write ending message and exit
    cout << "Ending RM component test.\n\n";
    
    return (0);
}

//
// PrintError
//
// Desc: Print an error message by calling the proper component-specific
//       print-error function
//


void PrintError(RC rc)
{
    if (abs(rc) <= END_PF_WARN)
        PF_PrintError(rc);
    else if (abs(rc) <= END_RM_WARN)
        RM_PrintError(rc);
    else
        cerr << "Error code out of range: " << rc << "\n";
}

////////////////////////////////////////////////////////////////////
// The following functions may be useful in tests that you devise //
////////////////////////////////////////////////////////////////////

//
// LsFile
//
// Desc: list the filename's directory entry
//
void LsFile(char *fileName)
{
    char command[80];

    sprintf(command, "ls -l %s", fileName);
    printf("doing \"%s\"\n", command);
    system(command);
}

//
// PrintRecord
//
// Desc: Print the TestRec record components
//
void PrintRecord(TestRec &recBuf)
{
    printf("[%s, %d, %f]\n", recBuf.str, recBuf.num, recBuf.r);
}

//
// AddRecs
//
// Desc: Add a number of records to the file
//
RC AddRecs(RM_FileHandle &fh, int numRecs)
{
    RC      rc;
    int     i;
    TestRec recBuf;
    RID     rid;
    PageNum pageNum;
    SlotNum slotNum;

    // We set all of the TestRec to be 0 initially.  This heads off
    // warnings that Purify will give regarding UMR since sizeof(TestRec)
    // is 40, whereas actual size is 37.
    memset((void *)&recBuf, 0, sizeof(recBuf));

    printf("\nadding %d records\n", numRecs);
    for (i = 0; i < numRecs; i++) {
        memset(recBuf.str, ' ', STRLEN);
        sprintf(recBuf.str, "a%d", i);
        recBuf.num = i;
        recBuf.r = (float)i;
        if ((rc = InsertRec(fh, (char *)&recBuf, rid)) ||
            (rc = rid.GetPageNum(pageNum)) ||
            (rc = rid.GetSlotNum(slotNum)))
            return (rc);

        if ((i + 1) % PROG_UNIT == 0){
            printf("%d  ", i + 1);
            fflush(stdout);
        }
    }
    if (i % PROG_UNIT != 0)
        printf("%d\n", i);
    else
        putchar('\n');

    // Return ok
    return (0);
}


RC RIDToIndex(RID &rid, int &index){
    RC rc;
    PageNum page;
    SlotNum slot;
    if((rc = rid.GetPageNum(page)))
        return (rc);
    if((rc = rid.GetSlotNum(slot)))
        return (rc);

    index = (page-1)*(PF_PAGE_SIZE/sizeof(TestRec)) + slot;

}


//
// Add random records, and keep track of them 
//
RC AddRandRecs(RM_FileHandle &fh, int numRecs)
{
    RC      rc;
    int     i;
    TestRec recBuf;
    RID     rid;
    PageNum pageNum;
    SlotNum slotNum;
    
    memset((void *)&recBuf, 0, sizeof(recBuf));

    for (i = 0; i < numRecs; i++) {
        memset(recBuf.str, ' ', STRLEN);
        sprintf(recBuf.str, "a%d", i);
        recBuf.num = i;
        recBuf.r = (float)i;
        if ((rc = InsertRec(fh, (char *)&recBuf, rid)) ||
            (rc = rid.GetPageNum(pageNum)) ||
            (rc = rid.GetSlotNum(slotNum)))
            return (rc);
        int index = -1;
        if((rc == RIDToIndex(rid, index)))
            return (rc);
        //printf("index added: %d \n", index);
        inserted_indices[index] = 1;
        deleted_indices[index] = 0;

        if((rc = added_recs[index].SetRecord(rid, (char*)&recBuf, sizeof(TestRec))))
            return (rc);
        recsInFile++;
        if(index > maxIndex)
            maxIndex = index;
    }

    // Return ok
    return (0);
}

RC DeleteRandRecs(RM_FileHandle &fh, int numRecs){
    RC rc;
    if(numRecs > recsInFile)
        return (RM_EOF);
    for(int i=0; i < numRecs; i++){
        //printf("recs in file: %d \n", recsInFile);
        int indextodelete = (rand() % recsInFile );
        //printf("indextodelete: %d \n", indextodelete);
        //printf("index: %d \n", i);
        int count = 0;
        int index = -1;
        while(count <= (indextodelete)){
            index++;
            //printf("index: %d, count: %d \n", index, count);
            if(inserted_indices[index] == 1)
                count++;
        }
        //printf("found index: %d \n", index);
        RID rid;
        if((rc = added_recs[index].GetRid(rid))){
            return (rc);
        }
        if((rc = fh.DeleteRec(rid)))
            return (rc);
        inserted_indices[index] = 0;
        deleted_indices[index] = 1;
        recsInFile--;
    }

}

RC VerifyRandFile(RM_FileHandle &fh){
    RC rc;
    RM_FileScan fs;
    RID rid;
    RM_Record rec;
    // Count that there are recsInFile number of records:
    int counter = 0;

   if ((rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num), 
         NO_OP, NULL)))
      return (rc);
   // count them
   for (rc = GetNextRecScan(fs, rec); 
         rc != (RM_EOF); 
         rc = GetNextRecScan(fs, rec), counter++) {
   }
   if ((rc = fs.CloseScan()))
      return(rc);
   if(counter != recsInFile)
      return (RM_EOF);


   // Make sure that all the records deleted are not there:
   for(int i = 0; i < maxIndex; i++){
     if (deleted_indices[i] == 1){
        RID rid;
        RM_Record rec;
        if((rc = added_recs[i].GetRid(rid))){
            return (rc);
        }
        if((fh.GetRec(rid, rec)) == 0)
            return (RM_EOF);
     }
   }

   // Make sure that all the records inserted are there. 
   for(int i = 0; i < maxIndex; i++){
     if (inserted_indices[i] == 1){
        RID rid;
        RM_Record rec;
        if((rc = added_recs[i].GetRid(rid))){
            return (rc);
        }
        if((rc =(fh.GetRec(rid, rec))))
            return (rc);
        TestRec *data1;
        TestRec *data2;
        if((rc = rec.GetData((char*&)data1)))
            return (rc);
        if((rc = added_recs[i].GetData((char*&)data2)))
            return (rc);
        if (strcmp(data1->str,data2->str))
          return (-1);
        if (data1->num != data2->num)
          return (-1);
        if (data1->r != data2->r)
          return (-1);
     }
   }


   return (0);
}

//
// VerifyFile
//
// Desc: verify that a file has records as added by AddRecs
//
RC VerifyFile(RM_FileHandle &fh, int numRecs)
{
    RC        rc;
    int       n;
    TestRec   *pRecBuf;
    RID       rid;
    char      stringBuf[STRLEN];
    char      *found;
    RM_Record rec;

    found = new char[numRecs];
    memset(found, 0, numRecs);

    printf("\nverifying file contents\n");

    RM_FileScan fs;
    if ((rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num),
                        NO_OP, NULL, NO_HINT)))
        return (rc);

    // For each record in the file
    for (rc = GetNextRecScan(fs, rec), n = 0;
         rc == 0;
         rc = GetNextRecScan(fs, rec), n++) {

        // Make sure the record is correct
        if ((rc = rec.GetData((char *&)pRecBuf)) ||
            (rc = rec.GetRid(rid)))
            goto err;

        memset(stringBuf,' ', STRLEN);
        sprintf(stringBuf, "a%d", pRecBuf->num);

        if (pRecBuf->num < 0 || pRecBuf->num >= numRecs ||
            strcmp(pRecBuf->str, stringBuf) ||
            pRecBuf->r != (float)pRecBuf->num) {
            printf("VerifyFile: invalid record = [%s, %d, %f]\n",
                   pRecBuf->str, pRecBuf->num, pRecBuf->r);
            exit(1);
        }

        if (found[pRecBuf->num]) {
            printf("VerifyFile: duplicate record = [%s, %d, %f]\n",
                   pRecBuf->str, pRecBuf->num, pRecBuf->r);
            exit(1);
        }

        found[pRecBuf->num] = 1;
    }

    if (rc != RM_EOF)
        goto err;

    if ((rc=fs.CloseScan()))
        return (rc);

    // make sure we had the right number of records in the file
    if (n != numRecs) {
        printf("%d records in file (supposed to be %d)\n",
               n, numRecs);
        exit(1);
    }

    // Return ok
    rc = 0;

err:
    fs.CloseScan();
    delete[] found;
    return (rc);
}

//
// PrintFile
//
// Desc: Print the contents of the file
//
RC PrintFile(RM_FileScan &fs)
{
    RC        rc;
    int       n;
    TestRec   *pRecBuf;
    RID       rid;
    RM_Record rec;

    printf("\nprinting file contents\n");

    // for each record in the file
    for (rc = GetNextRecScan(fs, rec), n = 0;
         rc == 0;
         rc = GetNextRecScan(fs, rec), n++) {

        // Get the record data and record id
        if ((rc = rec.GetData((char *&)pRecBuf)) ||
            (rc = rec.GetRid(rid)))
            return (rc);

        // Print the record contents
        PrintRecord(*pRecBuf);
    }

    if (rc != RM_EOF)
        return (rc);

    printf("%d records found\n", n);

    // Return ok
    return (0);
}


////////////////////////////////////////////////////////////////////////
// The following functions are wrappers for some of the RM component  //
// methods.  They give you an opportunity to add debugging statements //
// and/or set breakpoints when testing these methods.                 //
////////////////////////////////////////////////////////////////////////

//
// CreateFile
//
// Desc: call RM_Manager::CreateFile
//
RC CreateFile(char *fileName, int recordSize)
{
    printf("\ncreating %s\n", fileName);
    return (rmm.CreateFile(fileName, recordSize));
}

//
// DestroyFile
//
// Desc: call RM_Manager::DestroyFile
//
RC DestroyFile(char *fileName)
{
    printf("\ndestroying %s\n", fileName);
    return (rmm.DestroyFile(fileName));
}

//
// OpenFile
//
// Desc: call RM_Manager::OpenFile
//
RC OpenFile(char *fileName, RM_FileHandle &fh)
{
    printf("\nopening %s\n", fileName);
    return (rmm.OpenFile(fileName, fh));
}

//
// CloseFile
//
// Desc: call RM_Manager::CloseFile
//
RC CloseFile(char *fileName, RM_FileHandle &fh)
{
    if (fileName != NULL)
        printf("\nClosing %s\n", fileName);
    return (rmm.CloseFile(fh));
}

//
// InsertRec
//
// Desc: call RM_FileHandle::InsertRec
//
RC InsertRec(RM_FileHandle &fh, char *record, RID &rid)
{
    return (fh.InsertRec(record, rid));
}

//
// DeleteRec
//
// Desc: call RM_FileHandle::DeleteRec
//
RC DeleteRec(RM_FileHandle &fh, RID &rid)
{
    return (fh.DeleteRec(rid));
}

//
// UpdateRec
//
// Desc: call RM_FileHandle::UpdateRec
//
RC UpdateRec(RM_FileHandle &fh, RM_Record &rec)
{
    return (fh.UpdateRec(rec));
}

//
// GetNextRecScan
//
// Desc: call RM_FileScan::GetNextRec
//
RC GetNextRecScan(RM_FileScan &fs, RM_Record &rec)
{
    return (fs.GetNextRec(rec));
}


RC Test1(void){
   RC rc;
   RM_FileHandle fh1;
   RM_FileHandle fh2;
   RM_FileHandle fh3;

   printf("test3 starting ****************\n");
    // OK
   printf("\n*** File Creation Test with negative rec size: %s\n", 
         (CreateFile(FILENAME, -1)) ? "PASS\a" : "FAIL"); 
   printf("\n*** File Creation Test with too big of rec size: %s\n", 
         (CreateFile(FILENAME, PF_PAGE_SIZE)) ? "PASS\a" : "FAIL");
   printf("\n*** Opening a non-created file: %s\n", 
         (OpenFile(FILENAME, fh1)) ? "PASS\a" : "FAIL");  

   printf("\n*** File Creation Test: %s\n", 
         (CreateFile(FILENAME, sizeof(TestRec))) ? "FAIL\a" : "PASS"); 

   printf("\n*** Opening a created file: %s\n", 
         (OpenFile(FILENAME, fh1)) ? "FAIL\a" : "PASS"); 
   printf("\n*** Opening a created file twice: %s\n", 
         (OpenFile(FILENAME, fh2)) ? "FAIL\a" : "PASS"); 

   printf("\n*** Closing a FH that wasn't opened: %s\n", 
         (CloseFile(FILENAME, fh3)) ? "PASS\a" : "FAIL"); 

   printf("\n*** Closing a FH: %s\n", 
         (CloseFile(FILENAME, fh2)) ? "FAIL\a" : "PASS"); 

   printf("\n*** Closing a FH twice: %s\n", 
         (CloseFile(FILENAME, fh2)) ? "PASS\a" : "FAIL"); 

   printf("\n*** Destroying a file that is open: %s\n", 
         (DestroyFile(FILENAME)) ? "FAIL\a" : "PASS"); 

   printf("\n*** Closing a file that is not there: %s\n", 
         (CloseFile("helloworld", fh2)) ? "PASS\a" : "FAIL"); 
   printf("\n*** Destroy a file that is not there: %s\n", 
         (DestroyFile("helloworld")) ? "PASS\a" : "FAIL"); 

   printf("\n*** Creating another file: %s\n", 
         (CreateFile("helloworld", sizeof(TestRec))) ? "FAIL\a" : "PASS"); 

   printf("\n*** Closing a file that's not opened: %s\n", 
         (CloseFile("helloworld", fh2)) ? "PASS\a" : "FAIL"); 



   printf("\n*** Destroying files: %s\n", 
         (DestroyFile("helloworld")) ? "FAIL\a" : "PASS"); 


   printf("\ntest2 done ********************\n");
   return (0);
}

RC Test2(void){
    RC rc;
    RM_FileHandle fh;
    RM_FileHandle fh2;
    RM_Record rec;
    RID rid;
    printf("\n*** File Creation Test: %s\n", 
         (CreateFile(FILENAME, sizeof(TestRec))) ? "FAIL\a" : "PASS"); 
    printf("\n*** File Open Test: %s\n", 
         (OpenFile(FILENAME, fh)) ? "FAIL\a" : "PASS");
    // OK
    printf("\n*** Add Records Test: %s\n", 
         (AddRecs(fh, FEW_RECS)) ? "FAIL\a" : "PASS");

    RM_FileScan fs;
    rc=fs.OpenScan(fh2,INT,sizeof(int),offsetof(TestRec, num), 
         NO_OP, NULL);
    printf("\n*** Open a filescan from an invalid filehandle: %s\n", 
         (rc) ? "PASS\a" : "FAIL");


    if ((rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num), 
         NO_OP, NULL)))
      return (rc);
    int counter = 0;
    for (rc = GetNextRecScan(fs, rec); 
         rc == 0 && counter < 10; 
         rc = GetNextRecScan(fs, rec), counter++) {

      // Get the record id
      if ((rc = rec.GetRid(rid)))
         return (rc);
      counter++;
   }
   printf("counter: %d \n", counter);
   if ((rc = fs.CloseScan()))
      return(rc);

   if ((rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num), 
         NO_OP, NULL)))
      return (rc);
    counter = 0;
    for (rc = GetNextRecScan(fs, rec); 
         rc == 0 && counter < FEW_RECS; 
         rc = GetNextRecScan(fs, rec), counter++) {

      // Get the record id
      if ((rc = rec.GetRid(rid)))
         return (rc);
      counter++;
   }
   printf("counter: %d \n", counter);
   if ((rc = fs.CloseScan()))
      return(rc);

   printf("\n*** Closing a filescan early and reusing it: %s\n", 
         (counter == FEW_RECS) ? "PASS\a" : "FAIL");

   printf("\n*** Closing a filescan that isn't open: %s\n", 
         (fs.CloseScan()) ? "PASS\a" : "FAIL");

    
    printf("\n*** Destroying a file: %s\n", 
         (DestroyFile(FILENAME)) ? "FAIL\a" : "PASS"); 

    return (0);

}

// Tests random insertion and deletion
RC Test3(void){
    srand (time(NULL));
    RC rc = 0;
    RM_FileHandle fh;
    //RM_FileHandle fh2;
    RM_Record rec;
    RID rid;
    recsInFile = 0;
    maxIndex = 0;
    int numCycles = 100;

    printf("\n*** File Creation Test: %s\n", 
         (CreateFile(FILENAME, sizeof(TestRec))) ? "FAIL\a" : "PASS");
    printf("\n*** File Open Test: %s\n", 
         (OpenFile(FILENAME, fh)) ? "FAIL\a" : "PASS");
    

    printf("\n*** Add 400 Records Test: %s\n", 
         (AddRandRecs(fh, 400)) ? "FAIL\a" : "PASS");

    printf("\n*** Deleting 100 Records Test: %s\n", 
         (DeleteRandRecs(fh, 100)) ? "FAIL\a" : "PASS");

    printf("\n*** Verifying file: %s\n", 
         (VerifyRandFile(fh)) ? "FAIL\a" : "PASS");


    int prevMaxIndex = maxIndex;
    if((rc = AddRandRecs(fh, 100)))
        return (rc);
    printf("\n*** Adding 100 Random Records\n*** Check for reusage: \n%s\n", 
         (prevMaxIndex = maxIndex) ? "PASS\a" : "FAIL");

    // OK

    if((rc = AddRandRecs(fh, 4000)))
        return (rc);
    if((rc = DeleteRandRecs(fh, 1000)))
        return (rc);
    printf("\n*** Adding 4000\n*** Deleting 1000 Random Records\n*** Verifying File: \n%s\n", 
         (VerifyRandFile(fh)) ? "FAIL\a" : "PASS");


    if((rc = AddRandRecs(fh, 1000)))
         return (rc);
    prevMaxIndex = maxIndex;
    printf("\n*** Adding/Deleting 1000 Random Records for %d Cycles: ", numCycles);
    for(int i= 0; i < numCycles; i++){

       if((rc = DeleteRandRecs(fh, 1000)))
          return (rc);
       if((rc = VerifyRandFile(fh)))
          return (rc);
       if((rc = AddRandRecs(fh, 1000)))
         return (rc);
       if((rc = VerifyRandFile(fh)))
          return (rc);
     }
     printf("\n*** Check that no new pages were added: \n%s\n",
        (prevMaxIndex == maxIndex) ? "PASS\a" : "FAIL");


    prevMaxIndex = maxIndex;
    printf("\n*** Adding/Deleting 1 Random Record for %d Cycles: ", numCycles);
    for(int i= 0; i < numCycles; i++){

       if((rc = DeleteRandRecs(fh, 1)))
          return (rc);
       if((rc = VerifyRandFile(fh)))
          return (rc);
       if((rc = AddRandRecs(fh, 1)))
         return (rc);
       if((rc = VerifyRandFile(fh)))
          return (rc);
     }
     printf("\n%s\n",
        (rc == 0) ? "PASS\a" : "FAIL");


     // delete all the records:


    if ((rc = CloseFile(FILENAME, fh)) ||
       (rc = DestroyFile(FILENAME)))
     return (rc);


    return (0);
}

