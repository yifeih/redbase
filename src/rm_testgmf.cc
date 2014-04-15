//
// File:        rm_testgmf.cc
// Description: Test RM component
// Authors:     Greg Fichtenholtz (gregoryf@leland.stanford.edu)
//

#include <cstdio>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <cstdlib>

#include "redbase.h"
#include "pf.h"
#include "rm_internal.h"
#include "rm.h"

using namespace std;

//
// Defines
//
#define FILENAME   "testrel"         // test file name
#define STRLEN      29               // length of string in testrec
#define PROG_UNIT   50               // how frequently to give progress
                                     //   reports when adding lots of recs
#define FEW_RECS    1000             // number of records added in

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

void PrintError(RC rc);
void PrintRecord(TestRec &recBuf);
RC AddRecs(RM_FileHandle &fh, int numRecs);
RC VerifyFile(RM_FileHandle &fh, int numRecs);
RC VerifyFile2(RM_FileHandle &fh, int numRecs);
RC PrintFile(RM_FileHandle &fh);
RC UpdateFile(RM_FileHandle &fh);

RC CreateFile(char *fileName, int recordSize);
RC DestroyFile(char *fileName);
RC OpenFile(char *fileName, RM_FileHandle &fh);
RC CloseFile(char *fileName, RM_FileHandle &fh);
RC InsertRec(RM_FileHandle &fh, char *record, RID &rid);
RC UpdateRec(RM_FileHandle &fh, RM_Record &rec);
RC DeleteRec(RM_FileHandle &fh, RID &rid);
RC GetNextRecScan(RM_FileScan &fs, RM_Record &rec);

extern void PF_Statistics();

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

   PF_Statistics();

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

   found = new char[numRecs];
   memset(found, 0, numRecs);

   printf("\nverifying file contents\n");

   RM_FileScan fs;
   if ((rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num), 
         NO_OP, NULL)))
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
         //printf("VerifyFile: invalid record = [%s, %d, %f]\n",
         //      pRecBuf->str, pRecBuf->num, pRecBuf->r);
         exit(1);
      }

      if (found[pRecBuf->num]) {
         //printf("VerifyFile: duplicate record = [%s, %d, %f]\n",
         //      pRecBuf->str, pRecBuf->num, pRecBuf->r);
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
// VerifyFile2
//
//
RC VerifyFile2(RM_FileHandle &fh, int numRecs)
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

   printf("\nverifying updated file contents\n");

   RID rid1(1,0);
   RM_Record rec1;
   fh.GetRec(rid1, rec1);
   //TestRec *pRecBufAgain;
  //rec1.GetData((char *&) pRecBufAgain);
  //printf("HirecordFH: [%s, %d, %f] \n", pRecBufAgain->str, pRecBufAgain->num, pRecBufAgain->r);  



   RM_FileScan fs;
   if ((rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num), 
         NO_OP, NULL)))
      return (rc);

   // For each record in the file
   for (rc = GetNextRecScan(fs, rec), n = 0; 
         rc == 0; 
         rc = GetNextRecScan(fs, rec), n++) {

      // Make sure the record is correct
      if ((rc = rec.GetData((char *&)pRecBuf)) ||
            (rc = rec.GetRid(rid)))
         goto err;

	  int i1 = numRecs + n;
	  float r1 = (float)i1;
      memset(stringBuf,' ', STRLEN);
      sprintf(stringBuf, "a%d", i1);

      if ( pRecBuf->num != i1 ||
            strcmp(pRecBuf->str, stringBuf) || 
            pRecBuf->r != r1) {
         printf("VerifyFile2: invalid record = [%s, %d, %f]\n",
               pRecBuf->str, pRecBuf->num, pRecBuf->r);
         exit(1);
      }

      if (found[pRecBuf->num-numRecs]) {
         printf("VerifyFile2: duplicate record = [%s, %d, %f]\n",
               pRecBuf->str, pRecBuf->num, pRecBuf->r);
         exit(1);
      }

      found[pRecBuf->num-numRecs] = 1;
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

//
// UpdateFile
//
RC UpdateFile(RM_FileHandle & fh)
{
   RC        rc;
   int       n;
   TestRec   *pRecBuf;
   RID       rid;
   RM_Record rec;

   printf("\nupdating file contents\n");

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

      // Update the record contents
	  pRecBuf->num = FEW_RECS + n;
	  pRecBuf->r   = (float)(pRecBuf->num);
      memset(pRecBuf->str, ' ', STRLEN);
      sprintf(pRecBuf->str, "a%d", pRecBuf->num);	
      //printf("RMTEST update to: [%s, %d, %f] \n", pRecBuf->str, pRecBuf->num, pRecBuf->r);  
    
    //TestRec *pRecBufAgain;
    //rec.GetData((char *&)pRecBufAgain);
    //printf("record: [%s, %d, %f] \n", pRecBufAgain->str, pRecBufAgain->num, pRecBufAgain->r);  

	  if ((rc = UpdateRec(fh, rec)))
		return rc;

   }

   if (rc != RM_EOF)
      return (rc);

   printf("closing scan \n");
   if ((rc = fs.CloseScan()))
	 return rc;
   printf("Scan closed \n");

   printf("%d records updated\n", n);

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
// Test1 
//
RC Test1(void)
{
   RC            rc;
   RM_FileHandle fh;

   printf("test1 starting ****************\n");

   if ((rc = CreateFile(FILENAME, sizeof(TestRec))) ||
         (rc = OpenFile(FILENAME, fh)) ||
         (rc = AddRecs(fh, FEW_RECS)) ||
         (rc = CloseFile(FILENAME, fh))) 
      return (rc);

   printf("\ntest1 done ********************\n");
   return (0);
}                  

//
// Test2 
//
RC Test2(void)
{                               
   RC            rc;
   RM_FileHandle fh;

   printf("test2 starting ****************\n");

   if ((rc = OpenFile(FILENAME, fh)) ||
       (rc = UpdateFile(fh)) ||
       (rc = CloseFile(FILENAME, fh)))
      return (rc);

   printf("\ntest2 done ********************\n");
   return (0);
}


//
// Test3
//
RC Test3(void)
{
   RC            rc;
   RM_FileHandle fh;

   printf("test3 starting ****************\n");

   if ((rc = OpenFile(FILENAME, fh)))
      return (rc);

   // verify file contents
   if ((rc = VerifyFile2(fh, FEW_RECS)))
	 return (rc);


   if ((rc = CloseFile(FILENAME, fh)) ||
	   (rc = DestroyFile(FILENAME)))
	 return (rc);

  printf("\ntest3 done ********************\n");
   return (0);
}
