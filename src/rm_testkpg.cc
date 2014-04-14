//
// File:        rm_test.cc
// Description: Test RM component
// Authors:     Jan Jannink
//              Dallan Quass (quass@cs.stanford.edu)
//              Keith P. Gordon
//              Jason McHugh (mchughj@cs.stanford.edu)
//
// This test shell contains a number of functions that will be useful
// in testing your RM component code.  In addition, a couple of sample
// tests are provided.  The tests are by no means comprehensive, however,
// and you are expected to devise your own tests to test your code.
//
// 1997:  Tester has been modified to reflect the change in the 1997
// interface.  For example, FileHandle no longer supports a Scan over the
// relation.  All scans are done via a FileScan.
//

#include <cstdio>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <cstdlib>

#include "redbase.h"
#include "pf.h"
#include "rm.h"

using namespace std;

//
// Defines
//
#define FILENAME   "testrel"        // test file name
#define STRLEN     29               // length of string in testrec
#define PROG_UNIT  50               // how frequently to give progress
                                    //   reports when adding lots of recs
#define FEW_RECS   4000             // number of records added in
#define LESS_RECS  1000

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
PF_Manager pfm;
RM_Manager rmm(pfm);

//
// Function declarations
//
RC Test1(void);
RC Test2(void);
RC Test3(void);
RC Test4(void);


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
#define NUM_TESTS       4               // number of tests
int (*tests[])() =                      // RC doesn't work on some compilers
{
  Test1, 
  Test2,
  Test3,
  Test4
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
   for(i = 0; i < numRecs; i++) {
      memset(recBuf.str, ' ', STRLEN);
      sprintf(recBuf.str, "a%d", i);
      recBuf.num = i;
      recBuf.r = (float)i;
      if ((rc = InsertRec(fh, (char *)&recBuf, rid)) ||
            (rc = rid.GetPageNum(pageNum)) ||
            (rc = rid.GetSlotNum(slotNum)))
         return (rc);

      if((i + 1) % PROG_UNIT == 0){
         printf("%d  ", i + 1);
         fflush(stdout);
      }
   }
   if(i % PROG_UNIT != 0)
      printf("%d\n", i);
   else
      putchar('\n');

   // Return ok
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

   RM_FileScan fs;
   if ((rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num), 
         NO_OP, NULL)))
      return (rc);

   found = new char[numRecs];
   memset(found, 0, numRecs);

   printf("\nverifying file contents\n");

   // for each record in the file
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

   // make sure we had the right number of records in the file
   if (n != numRecs) {
      printf("%d records in file (supposed to be %d)\n",
            n, numRecs); 
      exit(1); 
   }

   // Return ok
   rc = 0;

err:  
   delete[] found;
   return (rc);
}

//
// Test RM_EOF
//
// Desc: Test the RM_EOF component by trying to get a record
// where the pageNum is ridiculously high
//
RC Test_EOF(RM_FileHandle &fh)
{
  RID       rid(10000,0);
  RM_Record rec;
  RC rc;

  rc = fh.GetRec(rid, rec);
  
  if (rc!=0)
     return 0;
  else 
     return 1;
}

//
// Test GetBadRecord
//
// Desc: Get a record off page 0,  shouldn't let me because 
// this is header page.
//
RC GetBadRecord(RM_FileHandle &fh)
{
   RID rid(0,3);
   RM_Record rec;
   RC rc;
   PageNum pageNum;
   SlotNum slotNum;

   if (( rc = fh.GetRec(rid, rec)))
      return (rc);

   if ((rc=rid.GetPageNum(pageNum)))
      return rc;
   if ((rc=rid.GetSlotNum(slotNum)))
      return rc;

   if (pageNum!=1 && slotNum!=0)
      return (-1);

   return (0);
}

//
// Test GetWeirdRecord
//
// Desc: Get the next record from (0,5).  Should be the same
// record as (1,0) this is header page.
//
RC GetWeirdRecord(RM_FileHandle &fh)
{
   RID rid(0,5);
   RM_Record rec;
   RC rc;

   if (( rc = fh.GetRec(rid, rec)))
      return (rc);

   return (0);
}

//
// Test Comparison
//
// This ensures that 2 file scans return the same thing.
//
//
RC Comparison(RM_FileHandle &fh)
{
   RM_Record rec1;
   RM_Record rec2;
   RC rc1 = 0, rc2 = 0, rc;
   RM_FileScan fileScan1, fileScan2;
   TestRec *data1;
   TestRec *data2;

   if ((rc=fileScan1.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num), 
         NO_OP, NULL)))
      return (rc);
   if ((rc=fileScan2.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num), 
         NO_OP, NULL)))
      return (rc);

   if ((rc1 = fileScan1.GetNextRec(rec1))) 
      return (rc);
   if ((rc2 = fileScan2.GetNextRec(rec2)))
      return (rc);

   while (rc1 != RM_EOF || rc2 != RM_EOF) {

      if (!(rc1 == RM_EOF)) 
         if ((rc1 = rec1.GetData((char *&) data1)))
            return (-1);
      if (!(rc2 == RM_EOF)) 
         if ((rc2 = rec2.GetData((char *&) data2)))
            return (-1);

      if (strcmp(data1->str,data2->str))
         return (-1);
      if (data1->num != data2->num)
         return (-1);
      if (data1->r != data2->r)
         return (-1);
      rc2 = fileScan2.GetNextRec(rec2);
      rc1 = fileScan1.GetNextRec(rec1);
   }

   if (rc1!=RM_EOF || rc2!=RM_EOF)
      return (-1);

   return (0);

}
//
// PrintFile
//
// Desc: Print the contents of the file
//
RC PrintFile(RM_FileHandle &fh)
{
   RC        rc;
   int       n;
   TestRec   *pRecBuf;
   RID       rid;
   RM_Record rec;

   printf("\nprinting file contents\n");
   // Define a scan to go through all of the elements

   RM_FileScan fs;
   if ((rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num), 
         NO_OP, NULL)))
      return (rc);

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

/////////////////////////////////////////////////////////////////////
// Sample test functions follow.                                   //
/////////////////////////////////////////////////////////////////////

//
// Test1 tests simple creation, opening, closing, and deletion of files
//
RC Test1(void)
{                  
   RC            rc;
   RM_FileHandle fh;
   RID          rid1;
   RID          rid;
   RM_Record    rec;

   printf("TEST 1 BEGINNING ...................\n");
   printf("\n*** File Creation Test: %s\n", 
         (CreateFile(FILENAME, sizeof(TestRec))) ? "FAIL\a" : "PASS");  

   printf("\n*** File Open Test: %s\n", 
         (OpenFile(FILENAME, fh)) ? "FAIL\a" : "PASS");

   // Add record tests...
   printf("\n*** Add Records Test: %s\n", 
         (AddRecs(fh, FEW_RECS)) ? "FAIL\a" : "PASS");

   // Test RM_EOF..
   printf("\n*** Testing the EOF return code: %s\n",
         (Test_EOF(fh)) ? "FAIL\a" : "PASS");

   printf("\n*** Invalid GetRec call: %s\n",
         (GetBadRecord(fh)) ? "PASS" : "FAIL\a");

   printf("\n*** Call to GetRec with RID (0,5): %s\n",
         (GetWeirdRecord(fh)) ? "PASS" : "FAIL\a" );

   printf("\n*** Comparison between 2 FileScans: %s\n",
         (Comparison(fh)) ? "FAIL\a" : "PASS");

   int counter = 0;
   printf("\n*** Record Deletion (complete pages) Test : ");

   RM_FileScan fs;
   if ((rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num), 
         NO_OP, NULL)))
      return (rc);
   // count them
   int counterbefore = 0;
   for (rc = GetNextRecScan(fs, rec); 
         rc == 0 && counter < 4000; 
         rc = GetNextRecScan(fs, rec), counter++) {

      // Get the record id
      if ((rc = rec.GetRid(rid)))
         return (rc);
      counterbefore++;
   }
   printf("counterbefore: %d \n", counterbefore);
   if ((rc = fs.CloseScan()))
      return(rc);
   
   
   if ((rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num), 
         NO_OP, NULL)))
      return (rc);

   // Delete them
   counter = 0;
   for (rc = GetNextRecScan(fs, rec); 
         rc == 0 && counter < 2000; 
         rc = GetNextRecScan(fs, rec), counter++) {

      // Get the record id
      if ((rc = rec.GetRid(rid))){
         printf("error in RID\n");
         return (rc);
      }
      if ((rc = fh.DeleteRec(rid))) {
         printf("error in deletion\n");
         return (rc);
         break;
      }
   }
   printf("num deleted: %d \n", counter);

   if ((rc = fs.CloseScan()))
      return(rc);

   if ((rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num), 
         NO_OP, NULL)))
      return (rc);

   counter = 0;
   // Count them
   for (rc = GetNextRecScan(fs, rec); 
         rc == 0; 
         rc = GetNextRecScan(fs, rec), counter++) {

      // Get the record id
      if ((rc = rec.GetRid(rid)))
         return (rc);
   }

   printf("counter %d \n", counter);

   printf("%s\n",((rc == RM_EOF)&&(counter==2000)) ? "PASS" : "FAIL\a");

   printf("\n*** Reuse Space (Add to new pages) Test: %s\n", 
         (AddRecs(fh, LESS_RECS)) ? "FAIL\a" : "PASS");

   printf("\n*** Reuse Space (Should fill up old pages) Test: %s\n", 
         (AddRecs(fh, LESS_RECS)) ? "FAIL\a" : "PASS");

   
   printf("\n*** File Close Test: %s\n", 
         (CloseFile(FILENAME, fh)) ? "FAIL\a" : "PASS");
   //if((rc = CloseFile(FILENAME, fh)))
   //   return (rc);     

   printf("\n*** File Destroy Test: %s\n", 
         (DestroyFile(FILENAME)) ? "FAIL\a" : "PASS");     

   return (0);
}

//
// Test2 tests adding a few records to a file.
//
RC Test2(void)
{                               
   RC rc;
   RM_FileHandle fh, fh2;
   RID rid1, rid2, rid3, rid4, rid;
   RM_Record rec, rec2, rec3, rec4, rec1;
   int numSel = 0;
   TestRec *data;


   printf("TEST 2 BEGINNING ...................\n");
   // not created
   printf("\n*** Illegal File Open Test: %s\n", 
         (OpenFile(FILENAME, fh)) ? "PASS" :"FAIL\a");  
   // not open or created
   printf("\n*** Illegal File Close Test: %s\n", 
         (CloseFile(FILENAME, fh)) ? "PASS" :"FAIL\a");  
   // OK
   printf("\n*** File Creation Test: %s\n", 
         (CreateFile(FILENAME, sizeof(TestRec))) ? "FAIL\a" : "PASS");  
   // already exists
   printf("\n*** Duplicate Creation Test: %s\n", 
         (CreateFile(FILENAME, sizeof(TestRec))) ? "PASS" : "FAIL\a"); 
   // OK
   printf("\n*** File Open Test: %s\n", 
         (OpenFile(FILENAME, fh)) ? "FAIL\a" : "PASS");

   // fh already has open file
   printf("\n*** Illegal File Open Test: %s\n", 
         (OpenFile(FILENAME, fh)) ? "PASS" :"FAIL\a");  
   // OK
   printf("\n*** Add Records Test: %s\n", 
         (AddRecs(fh, FEW_RECS)) ? "FAIL\a" : "PASS");
   //OK
   printf("\n*** Verify File Test: %s\n", 
         (VerifyFile(fh, FEW_RECS)) ? "FAIL\a" : "PASS");
   // OK
   printf("\n*** File Close Test: %s\n", 
         (CloseFile(FILENAME, fh)) ? "FAIL\a" : "PASS"); 

   printf("\n*** File Open Test: %s\n", 
         (OpenFile(FILENAME, fh)) ? "FAIL\a" : "PASS");

   if ((rc = CloseFile(FILENAME, fh))) return rc; 

   if ((rc = OpenFile(FILENAME, fh))) return rc;

   rc = OK_RC;
   // FILESCAN TESTS
   int compInt = 3985; 
   printf("\n*** File Scan INT EQ_OP Test: ");
   RM_FileScan fileScan;
   fileScan.OpenScan(fh, INT, sizeof(int), offsetof(TestRec, num), 
         EQ_OP, &compInt);
   while (rc != RM_EOF) {
      if ((rc = fileScan.GetNextRec(rec)) && (rc != RM_EOF)) goto err2;
      if (rc == RM_EOF) break;
      if ((rc = rec.GetData((char *&) data))) goto err2;
      numSel++;
   }
   printf("%s\n",  (numSel == 1) ? "PASS" : "FAIL\a");
   printf("Found:%d.  Supposed to find: 1.\n",numSel);
   goto bp;
err2:
   printf("FAIL\a\n");
bp:

   numSel = 0;
   rc = OK_RC;
   printf("\n*** File Scan INT GE_OP Test: ");
   fileScan.CloseScan();
   fileScan.OpenScan(fh, INT, sizeof(int), offsetof(TestRec, num), 
         GE_OP, &compInt);
   while (rc != RM_EOF) {
      if ((rc = fileScan.GetNextRec(rec)) && (rc != RM_EOF)) goto err3;
      if (rc == RM_EOF) break;
      if ((rc = rec.GetData((char *&) data))) goto err3;
      numSel++;
   }
   printf("%s\n",  (numSel == 15) ? "PASS" : "FAIL\a");
   printf ("Found:%d.  Supposed to find: 15.\n",numSel);
   goto bp3;
err3:
   printf("FAIL\a\n");
bp3:

   numSel = 0;
   rc = OK_RC;
   printf("\n*** File Scan INT LT_OP Test: ");
   fileScan.CloseScan();
   fileScan.OpenScan(fh, INT, sizeof(int), offsetof(TestRec, num), 
         LT_OP, &compInt);
   while (rc != RM_EOF) {
      if ((rc = fileScan.GetNextRec(rec)) && (rc != RM_EOF)) goto err4;
      if (rc == RM_EOF) break;
      if ((rc = rec.GetData((char *&) data))) goto err4;
      numSel++;
   }
   printf("%s\n",  (numSel == 3985) ? "PASS" : "FAIL\a");
   printf ("Found:%d.  Supposed to find: 3985.\n",numSel);
   goto bp4;
err4:
   printf("FAIL\a\n");
bp4:

   compInt = 55000;
   numSel = 0;
   rc = OK_RC;
   printf("\n*** File Scan INT EQ_OP Test: ");
   fileScan.CloseScan();
   fileScan.OpenScan(fh, INT, sizeof(int), offsetof(TestRec, num), 
         EQ_OP, &compInt);
   while (rc != RM_EOF) {
      if ((rc = fileScan.GetNextRec(rec)) && (rc != RM_EOF)) goto err5;
      if (rc == RM_EOF) break;
      if ((rc = rec.GetData((char *&) data))) goto err5;
      numSel++;
   }
   printf("%s\n",  (numSel == 0) ? "PASS" : "FAIL\a");
   printf ("Found:%d.  Supposed to find: 0.\n",numSel);
   goto bp5;
err5:
   printf("FAIL\a\n");
bp5:

   rc = OK_RC;
   float compFlt = 99.00;
   numSel = 0;
   printf("\n*** File Scan FLOAT EQ_OP Test: ");
   fileScan.CloseScan();
   fileScan.OpenScan(fh, FLOAT, sizeof(int), offsetof(TestRec, r), 
         EQ_OP, &compFlt);
   while (rc != RM_EOF) {
      if ((rc = fileScan.GetNextRec(rec)) && (rc != RM_EOF)) goto err7;
      if (rc == RM_EOF) break;
      if ((rc = rec.GetData((char *&) data))) goto err7;
      numSel++;
   }
   printf("%s\n",  (numSel == 1) ? "PASS" : "FAIL\a");
   printf ("Found:%d.  Supposed to find: 1.\n",numSel);
   goto bp7;
err7:
   printf("FAIL\a\n");
bp7:


   rc = OK_RC;
   numSel = 0;
   printf("\n*** File Scan FLOAT NE_OP Test: ");
   fileScan.CloseScan();
   fileScan.OpenScan(fh, FLOAT, sizeof(int), offsetof(TestRec, r), 
         NE_OP, &compFlt);
   while (rc != RM_EOF) {
      if ((rc = fileScan.GetNextRec(rec)) && (rc != RM_EOF)) goto err8;
      if (rc == RM_EOF) break;
      if ((rc = rec.GetData((char *&) data))) goto err8;
      numSel++;
   }
   printf("%s\n",  (numSel == 3999) ? "PASS" : "FAIL\a");
   printf ("Found:%d.  Supposed to find: 3999.\n",numSel);
   goto bp8;
err8:
   printf("FAIL\a\n");
bp8:


   rc = OK_RC;
   numSel = 0;
   printf("\n*** File Scan FLOAT LE_OP Test: ");
   fileScan.CloseScan();
   fileScan.OpenScan(fh, FLOAT, sizeof(int), offsetof(TestRec, r), 
         LE_OP, &compFlt);
   while (rc != RM_EOF) {
      if ((rc = fileScan.GetNextRec(rec)) && (rc != RM_EOF)) goto err8a;
      if (rc == RM_EOF) break;
      if ((rc = rec.GetData((char *&) data))) goto err8a;
      numSel++;
   }
   printf("%s\n",  (numSel == 100) ? "PASS" : "FAIL\a");
   printf ("Found:%d.  Supposed to find: 100.\n",numSel);
   goto bp8a;
err8a:
   printf("FAIL\a\n");
bp8a:


   rc = OK_RC;
   numSel = 0;
   printf("\n*** File Scan FLOAT GE_OP Test: ");
   fileScan.CloseScan();
   fileScan.OpenScan(fh, FLOAT, sizeof(int), offsetof(TestRec, r), 
         GE_OP, &compFlt);
   while (rc != RM_EOF) {
      if ((rc = fileScan.GetNextRec(rec)) && (rc != RM_EOF)) goto err9;
      if (rc == RM_EOF) break;
      if ((rc = rec.GetData((char *&) data))) goto err9;
      numSel++;
   }
   printf("%s\n",  (numSel == 3901) ? "PASS" : "FAIL\a");
   printf ("Found:%d.  Supposed to find: 3901.\n",numSel);
   goto bp9;
err9:
   printf("FAIL\a\n");
bp9:


   rc = OK_RC;
   numSel = 0;
   printf("\n*** File Scan STRING LE_OP Test: ");
   char compStr[] = "a92                           ";
   printf("RAWR: %s \n", compStr);
   fileScan.CloseScan();
   fileScan.OpenScan(fh, STRING, STRLEN, offsetof(TestRec, str), 
         LE_OP, &compStr);
   while (rc != RM_EOF) {
      if ((rc = fileScan.GetNextRec(rec)) && (rc != RM_EOF)) goto err10;
      if (rc == RM_EOF) break;
      if ((rc = rec.GetData((char *&) data))) goto err10;
      /*
      TestRec *buf;
      rec.GetData((char *&)buf);
      printf("record: [%s, %d, %f] \n", buf->str, buf->num, buf->r);  
      */
      numSel++;
   }
   printf("%s\n",  (numSel == 3913) ? "PASS" : "FAIL\a");
   printf ("Found:%d.  Supposed to find: 3913.\n",numSel);
   goto bp10;
err10:
   printf("FAIL\a\n");
bp10:


   rc = OK_RC;
   numSel = 0;
   printf("\n*** File Scan STRING GT_OP Test: ");
   fileScan.CloseScan();
   fileScan.OpenScan(fh, STRING, STRLEN, offsetof(TestRec, str), 
         GT_OP, &compStr);
   while (rc != RM_EOF) {
      if ((rc = fileScan.GetNextRec(rec)) && (rc != RM_EOF)) goto err11;
      if (rc == RM_EOF) break;
      if ((rc = rec.GetData((char *&) data))) goto err11;
      numSel++;
   }
   printf("%s\n",  (numSel == 87) ? "PASS" : "FAIL\a");
   printf ("Found:%d.  Supposed to find: 87.\n",numSel);
   goto bp11;
err11:
   printf("FAIL\a\n");
bp11:


   rc = OK_RC;
   numSel = 0;
   printf("\n*** File Scan STRING NO_OP Test: ");
   fileScan.CloseScan();
   fileScan.OpenScan(fh, STRING, STRLEN, offsetof(TestRec, str), 
         NO_OP, NULL);
   while (rc != RM_EOF) {
      if ((rc = fileScan.GetNextRec(rec)) && (rc != RM_EOF)) goto err12;
      if (rc == RM_EOF) break;
      if ((rc = rec.GetData((char *&) data))) goto err12;
      numSel++;
   }
   printf("%s\n",  (numSel == 4000) ? "PASS" : "FAIL\a");
   printf ("Found:%d.  Supposed to find: 4000.\n",numSel);
   goto bp12;
err12:
   printf("FAIL\a\n");
bp12:

   rc = OK_RC;
   numSel = 0;
   printf("\n*** File Scan STRING NE_OP Test: ");
   fileScan.CloseScan();
   fileScan.OpenScan(fh, STRING, STRLEN, offsetof(TestRec, str), 
         NE_OP, &compStr);
   while (rc != RM_EOF) {
      if ((rc = fileScan.GetNextRec(rec)) && (rc != RM_EOF)) goto err13;
      if (rc == RM_EOF) break;
      if ((rc = rec.GetData((char *&) data))) goto err13;
      numSel++;
   }
   printf("%s\n",  (numSel == 4000) ? "PASS" : "FAIL\a");
   printf ("Found:%d.  Supposed to find: 4000.\n",numSel);
   goto bp13;
err13:
   printf("FAIL\a\n");
bp13:

   fileScan.CloseScan();
   rc = fileScan.OpenScan(fh, (AttrType) 5, STRLEN, 
         offsetof(TestRec, str), NE_OP, &compStr);
   printf("\n*** Filescan wrong type test : %s \n",
         (rc != 0) ? "PASS" :"FAIL\a");


   fileScan.CloseScan();
   rc = fileScan.OpenScan(fh, STRING, STRLEN, offsetof(TestRec, str), 
         (CompOp) 23, &compStr);
   printf("\n*** Filescan wrong operation test : %s \n",
         (rc != 0) ? "PASS" :"FAIL\a");


   fileScan.CloseScan();
   rc = fileScan.OpenScan(fh, STRING, STRLEN, offsetof(TestRec, str) + 100, 
         NE_OP, &compStr);
   printf("\n*** Filescan memory violation test : %s \n",
         (rc != 0) ? "PASS" :"FAIL\a");

   fileScan.CloseScan();
   rc = fileScan.OpenScan(fh, STRING, STRLEN, -3, 
         NE_OP, &compStr);
   printf("\n*** Filescan memory violation test : %s \n",
         (rc != 0) ? "PASS" :"FAIL\a");

   fileScan.CloseScan();
   rc = fileScan.OpenScan(fh, STRING, 300, offsetof(TestRec, str), 
         NE_OP, &compStr);
   printf("\n*** Filescan wrong type length test : %s \n",
         (rc != 0) ? "PASS" :"FAIL\a");

   fileScan.CloseScan();
   rc = fileScan.OpenScan(fh, INT, 7, offsetof(TestRec, str), 
         NE_OP, &compInt);
   printf("\n*** Filescan wrong type length test : %s \n",
         (rc != 0) ? "PASS" :"FAIL\a");

   int counter = 0;
   printf("\n*** Record Deletion Test : ");

   RM_FileScan fs;
   if ((rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num), 
         NO_OP, NULL)))
      return (rc);

   // Delete them
   for (rc = GetNextRecScan(fs, rec); 
         rc == 0; 
         rc = GetNextRecScan(fs, rec), counter++) {

      // Get the record id
      if ((rc = rec.GetRid(rid)))
         return (rc);
      if ((counter % 2) == 0) 
         if ((rc = fh.DeleteRec(rid))) break;
   }

   if ((rc = fs.CloseScan()))
      return (rc);

   if ((rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num), 
         NO_OP, NULL)))
      return (rc);

   counter = 0;
   // Count them
   for (rc = GetNextRecScan(fs, rec); 
         rc == 0; 
         rc = GetNextRecScan(fs, rec), counter++) {

      // Get the record id
      if ((rc = rec.GetRid(rid)))
         return (rc);
   }

   printf("%s\n",  ((rc == RM_EOF) && (counter == 2000)) ? "PASS" : "FAIL\a");

   rc = OK_RC;
   numSel = 0;
   printf("\n*** File Scan STRING LE_OP Test: ");
   fileScan.CloseScan();
   fileScan.OpenScan(fh, STRING, STRLEN, offsetof(TestRec, str), 
         LE_OP, &compStr);
   while (rc != RM_EOF) {
      if ((rc = fileScan.GetNextRec(rec)) && (rc != RM_EOF)) goto err22;
      if (rc == RM_EOF) break;
      if ((rc = rec.GetData((char *&) data))) goto err22;
      numSel++;
   }
   printf("%s\n",  (numSel == 1956) ? "PASS" : "FAIL\a");
   printf ("Found:%d.  Supposed to find: 1956.\n",numSel);
   goto bp22;
err22:
   printf("FAIL\a\n");
bp22:

   printf("\n*** Reuse Space (Add) Test: %s\n", 
         (AddRecs(fh, LESS_RECS)) ? "FAIL\a" : "PASS");

   rc = OK_RC;
   numSel = 0;
   char comp2Str[] = "b           ";
   printf("\n*** Reuse Space (Verify) Test: ");
   fileScan.CloseScan();
   fileScan.OpenScan(fh, STRING, STRLEN, offsetof(TestRec, str), 
         LT_OP, &comp2Str);
   while (rc != RM_EOF) {
      if ((rc = fileScan.GetNextRec(rec)) && (rc != RM_EOF)) goto err30;
      if (rc == RM_EOF) break;
      if ((rc = rec.GetData((char *&) data))) goto err30;
      numSel++;
   }
   printf("%s\n",  (numSel == 3000) ? "PASS" : "FAIL\a");
   goto bp30;
err30:
   printf("FAIL\a\n");
bp30:

   printf("\n*** File Close Test: %s\n", 
         (CloseFile(FILENAME, fh)) ? "FAIL\a" : "PASS");     

   printf("\n*** File Destroy Test: %s\n", 
         (DestroyFile(FILENAME)) ? "FAIL\a" : "PASS");     


   printf("\nTESTS 2 COMPLETE  ********************\n");
   return (0);
}


//
// Test3
//
RC Test3(void)
{                  
   RC            rc;
   RM_FileHandle fh;
   RID          rid1;
   RID          rid;
   RM_Record    rec;
   TestRec *data;

   printf("TEST 3 BEGINNING ...................\n");
   printf("\n*** File Creation Test: %s\n", 
         (CreateFile(FILENAME, sizeof(TestRec))) ? "FAIL\a" : "PASS");  

   printf("\n*** File Open Test: %s\n", 
         (OpenFile(FILENAME, fh)) ? "FAIL\a" : "PASS");

   // Add record tests...
   printf("\n*** Add Records Test: %s\n", 
         (AddRecs(fh, FEW_RECS)) ? "FAIL\a" : "PASS");

   rc = OK_RC;
   // FILESCAN TESTS
   int compInt = 2000; 
   int numSel = 0;
   printf("\n*** File Scan INT EQ_OP Test with DELETE: ");
   RM_FileScan fileScan;
   fileScan.OpenScan(fh, INT, sizeof(int), offsetof(TestRec, num), 
         GE_OP, &compInt);
   while (rc != RM_EOF) {
      if ((rc = fileScan.GetNextRec(rec)) && (rc != RM_EOF)) goto err33;
      if (rc == RM_EOF) break;
      if ((rc = rec.GetData((char *&) data))) goto err33;
      if ((rc = rec.GetRid(rid))) goto err33;
      if ((rc = fh.DeleteRec(rid))) goto err33;
      numSel++;
   }
   printf("%s\n",  (numSel == 2000) ? "PASS" : "FAIL\a");
   printf("Found:%d.  Supposed to find: 2000.\n",numSel);
   goto bp33;
err33:
   printf("FAIL\a\n");
bp33:

   RM_FileScan fs;
   if ((rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num), 
         NO_OP, NULL)))
      return (rc);

   printf("\n*** Verify the number of records: ");
   int counter = 0;
   // Count them
   for (rc = GetNextRecScan(fs, rec); 
         rc == 0; 
         rc = GetNextRecScan(fs, rec), counter++) {

      // Get the record id
      if ((rc = rec.GetRid(rid)))
         return (rc);
   }

   printf("%s\n",((rc == RM_EOF)&&(counter == 2000))?"PASS" : "FAIL\a");
   printf ("Found:%d.  Supposed to find: 2000.\n",counter);

   RID rid3(120,0);
   printf("\n*** Get a Record from Deleted Page: %s\n", 
         (fh.GetRec(rid3, rec)) ? "PASS" : "FAIL\a");

   printf("\n*** File Close Test: %s\n", 
         (CloseFile(FILENAME, fh)) ? "FAIL\a" : "PASS");     

   printf("\n*** File Destroy Test: %s\n", 
         (DestroyFile(FILENAME)) ? "FAIL\a" : "PASS");     



   return (0);

}

RC Test4(void){
  char buf1[] = "a92          ";
  char buf2[] = "a91";
  char buf3[] = "a94";

  printf("comparing 'a92      ' with 'a94'\n");
  if(strncmp(buf1, buf2, 29) < 0)
   printf("first is smaller \n");
  else
   printf("second is smaller\n");

  return (0);
}
