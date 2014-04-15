//
// File:        rm_testrecsizes.cc
// Description: Test of the RM component.  Tests databases using
//              different record sizes.  Good for checking how
//              performance varies with record size.  It makes
//              the assumption that the order that records are
//              inserted is the same order they come out of a
//              filescan, at least when no records have been deleted.
//
// Authors:     David Nagy-Farkas (davidnf@cs.stanford.edu)
// 
//              Note: Some snippets taken from provided rm_testkpg.cc
//
//              Test parameters can be varied.  Search for the
//              string "MODIFY" to find these variable parameters
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
#define PROG_UNIT  50               // how frequently to give progress
                                    //  reports when adding lots of recs
#define NUM_RECS   4096             // number of records added in
                                    // MODIFY this number to test more
                                    // or fewer records

// Global PF_Manager and RM_Manager variables
//
PF_Manager pfm;
RM_Manager rmm(pfm);

// Record sizes to run tests on
// MODIFY the sizes in this array to test whatever you desire
//
int RecSize[] = {
   1,               // 1 byte records
   2,               // 2 byte records
   3,               // 3 bytes
   4,               // 4 bytes
   100,             // etc.
   4000
};

//
// PrintError
//
// Desc: Print an error message by calling the proper component-specific
//       print-error function
//
void PrintError(RC rc)
{
  if (abs(rc) <= START_PF_WARN)
    PF_PrintError(rc);
  else if (abs(rc) <= START_RM_WARN)
    RM_PrintError(rc);
  else
    cerr << "Error code out of range: " << rc << "\n";
}

//
// main
//
int main(int argc, char *argv[])
{
   RC             result = OK_RC;
   char*          recbuf;
   char           bufchar;
   int            recSize, numTest, recNum, i;
   RM_FileHandle  fh;
   RID            rid;
   RM_FileScan    scan;
   RM_Record      rec;
   char*          recData;

   // perform tests
   for (numTest = 0; numTest < (int)(sizeof(RecSize)/4); numTest++) {
      recSize = RecSize[numTest];
      cout << "Starting RM test for record size " << recSize << "\n";

      // Delete files from last time
      unlink(FILENAME);

      // create and open a new file
      cout << "Beginning file create... ";
      cout.flush();

      result = rmm.CreateFile(FILENAME, recSize);
      if (result) goto ERROR;
      cout << "Success!\n";
      cout << "Beginning file open... ";
      
      cout.flush();
      result = rmm.OpenFile(FILENAME, fh);
      if (result) goto ERROR;
      cout << "Success!\n";
      cout.flush();

      // add records
      cout << "Adding " << NUM_RECS << " records... ";
      cout.flush();
      recbuf = (char*)malloc(recSize);
      for (recNum = 0; recNum < NUM_RECS; recNum++) {
         // Fill the buffer with a certain byte, which is incremented
         // each time.  We'll use this later to check validity
         bufchar = (char) recNum;
         for (i=0; i<recSize; i++) {
            recbuf[i] = bufchar;
         }

         // Insert the new record
         result = fh.InsertRec(recbuf, rid);

         if (!((recNum + 1) % PROG_UNIT)) {
            cout << recNum + 1 << " ";
            cout.flush();
         }
      }
      cout << NUM_RECS << "Success!\n";
      cout.flush();
      

      cout << "Opening filescan... ";
      cout.flush();
      result = scan.OpenScan(fh, STRING, recSize, 0, NO_OP, NULL);
      if (result) goto ERROR;
      cout << "Success!\n";
      cout.flush();

      cout << "Scanning, verifying, and deleting records... ";
      cout.flush();
      for (recNum = 0; recNum < NUM_RECS; recNum++) {
         // Fill the buffer with a certain byte, which is incremented
         // each time.  Use to check validity
         bufchar = (char) recNum;
         for (i=0; i<recSize; i++) {
            recbuf[i] = bufchar;
         }

         result = scan.GetNextRec(rec);
         if (result) {printf("error 1 \n"); goto ERROR; }

         result = rec.GetData(recData);
         if (result) {printf("error 2 \n"); goto ERROR; }

         // Compare the data
         if (memcmp(recbuf, recData, recSize)) {
            cout << "Data didn't verify for record " << recNum << "!\n";
            cout.flush();
            goto ERROR;
         }

         // Delete the data
         result = rec.GetRid(rid);
         if (result) {printf("error 3 \n"); goto ERROR; }
         result = fh.DeleteRec(rid);
         if (result) {printf("error 4 \n"); goto ERROR; }

         if (!((recNum + 1) % PROG_UNIT)) {
            cout << recNum + 1 << " ";
            cout.flush();
         }
      }

      // Scan should be done, but try GetNextRec just to be sure
      result = scan.GetNextRec(rec);
      if (!result) {
         cout << "Extra records in scan!\n";
         cout.flush();
         result = -1;
         goto END;
      }

      cout << "Success!\n";
      
      cout << "Closing scan... ";
      cout.flush();
      result = scan.CloseScan();
      
      if (result) goto ERROR;
      cout << "Success!\n";
      cout.flush();

      
      free(recbuf);

      cout << "Closing file... ";
      cout.flush();
      result = rmm.CloseFile(fh);
      if (result) goto ERROR;
      cout << "Success! \n";
      cout << "Destroying file... ";
      cout.flush();

      result = rmm.DestroyFile(FILENAME);
      if (result) goto ERROR;
      cout << "Success!\n";
      cout << "TEST SUCCEEDED FOR RECORD SIZE " << recSize << "\n";
      cout.flush();
   }
   cout << "ALL TESTS PASSED!\n";
   cout.flush();

END:
   return result;

ERROR:
   PrintError(result);
   goto END;
}
