//
// File:        ix_test.cc
// Description: Test IX component
// Authors:     Jan Jannink
//              Dallan Quass (quass@cs.stanford.edu)
//              Keith Gordon
//
// 1997:  Tester has been modified to reflect the change in the 1997
// interface.  
//

#include <cstdio>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>

#include "redbase.h"
#include "pf.h"
#include "rm.h"
#include "ix.h"

using namespace std;

//
// Defines
//
#define FILENAME       "testrel"        // test file name
#define BADFILE        "/abc/def/xyz"   // bad file name
#define STRLEN         39               // length of strings to index
#define FEW_ENTRIES    20    
#define MANY_ENTRIES   1000             // ~338 Causes page splits
#define NENTRIES       5000             // Size of values array
#define PROG_UNIT      200              // how frequently to give progress
// reports when adding lots of entries
#define RECSIZE        128

//
// Values array we will be using for our tests
//
int values[NENTRIES];

#ifndef offsetof
#       define offsetof(type, field)    ((size_t)&(((type *)0) -> field))
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
// Global component manager variables
//
PF_Manager pfm;
RM_Manager rmm(pfm);
IX_Manager ixm(pfm);

//
// Function declarations
//
RC Test1(void);


void PrintError(RC rc);
void LsFiles(char *fileName);
void ran(int n);
RC InsertRec(RM_FileHandle &fh, char *record, RID &rid);
RC DeleteInts(IX_IndexHandle &ih, int lo, int hi, int exLo, int exHi);
RC InsertInts(IX_IndexHandle &ih, int count, int lo, int hi, int exLo,
      int exHi);
RC InsertIntEntries(IX_IndexHandle &ih, int nEntries);
RC InsertFloatEntries(IX_IndexHandle &ih, int nEntries);
RC InsertStringEntries(IX_IndexHandle &ih, int nEntries);
RC AddRecs(RM_FileHandle &fh, int nRecs);
RC DeleteIntEntries(IX_IndexHandle &ih, int nEntries);
RC DeleteFloatEntries(IX_IndexHandle &ih, int nEntries);
RC DeleteStringEntries(IX_IndexHandle &ih, int nEntries);
RC VerifyIntIndex(IX_IndexHandle &ih, int nStart, int nEntries, 
      int bExists);
RC VerifyStringIndex(IX_IndexHandle &ih, int nStart, int nEntries, 
      int bExists);
RC PrintFile(RM_FileHandle &fh);
RC GetNextRecScan(RM_FileScan &fs, RM_Record &rec);
RC OpenFile(char *fileName, RM_FileHandle &fh);
void PrintRecord(TestRec &recBuf);

RC PrintIndex(IX_IndexHandle &ih);

// 
// Array of pointers to the test functions
//
#define NUM_TESTS       1               // number of tests
int (*tests[])() =                      // RC doesn't work on some compilers
{
   Test1
};

//
// main
//
int main(int argc, char *argv[])
{
   RC rc;
   char *progName = argv[0];   // since we will be changing argv
   int testNum;

   // Write out initial starting message
   printf("Starting IX component test.\n\n");

   // Init randomize function
   srand( (unsigned)time(NULL));

   // Delete files from last time (if found)
   // Don't check the return codes, since we expect to get an error
   // if the files are not found.
   rmm.DestroyFile(FILENAME);
   ixm.DestroyIndex(FILENAME, 0);
   ixm.DestroyIndex(FILENAME, 1);
   ixm.DestroyIndex(FILENAME, 2);
   ixm.DestroyIndex(FILENAME, 3);

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
   printf("Ending IX component test.\n\n");

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
   else if (abs(rc) <= END_IX_WARN)
      IX_PrintError(rc);
   else
      cerr << "Error code out of range: " << rc << "\n";
}

////////////////////////////////////////////////////////////////////
// The following functions may be useful in tests that you devise //
////////////////////////////////////////////////////////////////////

//
// LsFiles
//
// Desc: list the filename's directory entry
// 
void LsFiles(char *fileName)
{
   char command[80];

   sprintf(command, "ls -l *%s*", fileName);
   printf("doing \"%s\"\n", command);
   system(command);
}

//
// Create an array of random nos. between 0 and n-1, without duplicates.
// put the nos. in values[]
//
void ran(int n)
{
   int i, r, t, m;

   // init values array 
   for (i = 0; i < NENTRIES; i++)
      values[i] = i;

   // randomize first n entries in values array
   for (i = 0, m = n; i < n-1; i++) {
      r = (int)(rand() % m--);
      t = values[m];
      values[m] = values[r];
      values[r] = t;
   }
}

//
// InsertIntEntries
//
// Desc: Add a number of integer entries to the index
//
RC InsertIntEntries(IX_IndexHandle &ih, int nEntries)
{
   RC rc;
   int i;
   int value;

   printf("             adding %d int entries\n", nEntries);
   ran(nEntries);
   for(i = 0; i < nEntries; i++) {
      value = values[i] + 1;
      RID rid(value, value*2);
      if ((rc = ih.InsertEntry((void *)&value, rid)))
         return (rc);

      if((i + 1) % PROG_UNIT == 0){
         // cast to long for PC's
         printf("\r\t%d%%    ", (int)((i+1)*100L/nEntries));
         fflush(stdout);
      }
   }
   printf("\r\t%d%%      \n", (int)(i*100L/nEntries));

   // Return ok
   return (0);
}

//
// Desc: Add a number of float entries to the index
//
RC InsertFloatEntries(IX_IndexHandle &ih, int nEntries)
{
   RC rc;
   int i;
   float value;

   printf("             adding %d float entries\n", nEntries);
   ran(nEntries);
   for (i = 0; i < nEntries; i++) {
      value = values[i] + 1;
      RID rid((PageNum)value, (SlotNum)value*2);
      if ((rc = ih.InsertEntry((void *)&value, rid)))
         return (rc);

      if((i + 1) % PROG_UNIT == 0){
         printf("\r\t%d%%    ", (int)((i+1)*100L/nEntries));
         fflush(stdout);
      }
   }
   printf("\r\t%d%%      \n", (int)(i*100L/nEntries));

   // Return ok
   return (0);
}

//
// Desc: Add a number of string entries to the index
//
RC InsertStringEntries(IX_IndexHandle &ih, int nEntries)
{
   RC rc;
   int i;
   char value[STRLEN];

   printf("             adding %d string entries\n", nEntries);
   ran(nEntries);
   for (i = 0; i < nEntries; i++) {
      memset(value, ' ', STRLEN);
      sprintf(value, "number %d", values[i] + 1);
      RID rid(values[i] + 1, (values[i] + 1)*2);
      if ((rc = ih.InsertEntry(value, rid)))
         return (rc);

      if((i + 1) % PROG_UNIT == 0){
         printf("\r\t%d%%    ", (int)((i+1)*100L/nEntries));
         fflush(stdout);
      }
   }
   printf("\r\t%d%%      \n", (int)(i*100L/nEntries));

   // Return ok
   return (0);
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
// AddRecs
//
// Desc: Add a number of integer records to an RM file
//
RC AddRecs(RM_FileHandle &fh, int nRecs)
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

   printf("\nadding %d records\n", nRecs);
   for(i = 0; i < nRecs; i++) {
      memset(recBuf.str, ' ', STRLEN);
      sprintf(recBuf.str, "a%d", i);
      recBuf.num = i;
      recBuf.r = (float)i;
      if ((rc = InsertRec(fh, (char *)&recBuf, rid)) ||
            (rc = rid.GetPageNum(pageNum)) ||
            (rc = rid.GetSlotNum(slotNum)))
         return (rc);
      if((i + 1) % PROG_UNIT == 0){
         printf("\r\t%d%%      ", (int)((i+1)*100L/nRecs));
         fflush(stdout);
      }
   }
   printf("\r\t%d%%      \n", (int)(i*100L/nRecs));

   // Return ok
   return (0);
}

//
// DeleteIntEntries: delete a number of integer entries from an index
//
RC DeleteIntEntries(IX_IndexHandle &ih, int nEntries)
{
   RC  rc;
   int i;
   int value;

   printf("        deleting %d int entries\n", nEntries);
   ran(nEntries);
   for (i = 0; i < nEntries; i++) {
      value = values[i] + 1;
      RID rid(value, value*2);
      if ((rc = ih.DeleteEntry((void *)&value, rid)))
         return (rc);

      if((i + 1) % PROG_UNIT == 0){
         printf("\r\t%d%%      ", (int)((i+1)*100L/nEntries));
         fflush(stdout);
      }
   }
   printf("\r\t%d%%      \n", (int)(i*100L/nEntries));

   return (0);
}

//
// DeleteFloatEntries: delete a number of float entries from an index
//
RC DeleteFloatEntries(IX_IndexHandle &ih, int nEntries)
{
   RC  rc;
   int i;
   float value;

   printf("        deleting %d float entries\n", nEntries);
   ran(nEntries);
   for (i = 0; i < nEntries; i++) {
      value = values[i] + 1;
      RID rid((PageNum)value, (SlotNum)value*2);
      if ((rc = ih.DeleteEntry((void *)&value, rid)))
         return (rc);

      if((i + 1) % PROG_UNIT == 0){
         printf("\r\t%d%%      ", (int)((i+1)*100L/nEntries));
         fflush(stdout);
      }
   }
   printf("\r\t%d%%      \n", (int)(i*100L/nEntries));

   return (0);
}

//
// Desc: Delete a number of string entries from an index
//
RC DeleteStringEntries(IX_IndexHandle &ih, int nEntries)
{
   RC   rc;
   int  i;
   char value[STRLEN+1];

   printf("             deleting %d string entries\n", nEntries);
   ran(nEntries);
   for (i = 0; i < nEntries; i++) {
      memset(value, ' ', STRLEN);
      sprintf(value, "number %d", values[i] + 1);
      RID rid(values[i] + 1, (values[i] + 1)*2);
      if ((rc = ih.DeleteEntry(value, rid)))
         return (rc);

      if((i + 1) % PROG_UNIT == 0){
         printf("\r\t%d%%    ", (int)((i+1)*100L/nEntries));
         fflush(stdout);
      }
   }
   printf("\r\t%d%%      \n", (int)(i*100L/nEntries));

   // Return ok
   return (0);
}

//
// VerifyIntIndex
//   - nStart is the starting point in the values array to check
//   - nEntries is the number of entries in the values array to check
//   - If bExists == 1, verify that an index has entries as added
//     by InsertIntEntries.  
//     If bExists == 0, verify that entries do NOT exist (you can 
//     use this to test deleting entries).
//
RC VerifyIntIndex(IX_IndexHandle &ih, int nStart, int nEntries, int bExists)
{
   RC rc;
   int i;
   RID rid;
   PageNum pageNum;
   SlotNum slotNum;
   IX_IndexScan scan;
   
   // Assume values still contains the array of values inserted/deleted

   printf("Verifying index contents\n");

   for (i = nStart; i < nStart + nEntries; i++) {
      int value = values[i] + 1;

      if ((rc = scan.OpenScan(ih, EQ_OP, &value))) {
         printf("Verify error: opening scan\n");
         return (rc);
      }

      rc = scan.GetNextEntry(rid);
      if (!bExists && rc == 0) {
         printf("Verify error: found non-existent entry %d\n", value);
         return (IX_EOF);  // What should be returned here?
      }
      else if (bExists && rc == IX_EOF) {
         printf("Verify error: entry %d not found\n", value);
         return (IX_EOF);  // What should be returned here?
      }
      else if (rc != 0 && rc != IX_EOF)
         return (rc);

      if (bExists && rc == 0) {
         // Did we get the right entry?
         if ((rc = rid.GetPageNum(pageNum)) ||
               (rc = rid.GetSlotNum(slotNum)))
            return (rc);

         if (pageNum != value || slotNum != (value*2)) {
            printf("Verify error: incorrect rid (%d,%d) found for entry %d\n",
                  pageNum, slotNum, value);             
            printf("Verify error: correct rid is (%d,%d)\n", value, value*2);
            return (IX_EOF);  // What should be returned here?
         }

         // Is there another entry?
         rc = scan.GetNextEntry(rid);
         if (rc == 0) {
            printf("Verify error: found two entries with same value %d\n", value);
            return (IX_EOF);  // What should be returned here?
         }
         else if (rc != IX_EOF)
            return (rc);
      }

      if ((rc = scan.CloseScan())) {
         printf("Verify error: closing scan\n");
         return (rc);
      }
   }
   return (0);
}



//
// VerifyStringIndex
//   - nStart is the starting point in the values array to check
//   - nEntries is the number of entries in the values array to check
//   - If bExists == 1, verify that an index has entries as added
//     by InsertIntEntries.  
//     If bExists == 0, verify that entries do NOT exist (you can 
//     use this to test deleting entries).
//
RC VerifyStringIndex(IX_IndexHandle &ih, int nStart, int nEntries, int bExists)
{
   RC rc;
   int i;
   RID rid;
   IX_IndexScan scan;
   PageNum pageNum;
   SlotNum slotNum;
   char valStr[STRLEN];

   // Assume values still contains the array of values inserted/deleted

   printf("Verifying index contents\n");

   for (i = nStart; i < nStart + nEntries; i++) {
      int value = values[i] + 1;
      memset(valStr, ' ', STRLEN);
      sprintf(valStr, "number %d", value); 

      if ((rc = scan.OpenScan(ih, EQ_OP, valStr))) {
         printf("Verify error: opening scan\n");
         return (rc);
      }

      rc = scan.GetNextEntry(rid);
      if (!bExists && rc == 0) {
         printf("Verify error: found non-existent entry %s\n", valStr);
         return (IX_EOF);  // What should be returned here?
      }
      else if (bExists && rc == IX_EOF) {
         printf("Verify error: entry %s not found\n", valStr);
         return (IX_EOF);  // What should be returned here?
      }
      else if (rc != 0 && rc != IX_EOF)
         return (rc);

      if (bExists && rc == 0) {
         // Did we get the right entry?
         if ((rc = rid.GetPageNum(pageNum)) ||
               (rc = rid.GetSlotNum(slotNum)))
            return (rc);

         if (pageNum != value || slotNum != (value*2)) {
            printf("Verify error: incorrect rid (%d,%d) found for entry %s\n",
                  pageNum, slotNum, valStr);             
            return (IX_EOF);  // What should be returned here?
         }
         // Is there another entry?
         rc = scan.GetNextEntry(rid);
         if (rc == 0) {
            printf("Verify error: found two entries with same value %s\n", valStr);
            return (IX_EOF);  // What should be returned here?
         }
         else if (rc != IX_EOF)
            return (rc);
      }

      if ((rc = scan.CloseScan())) {
         printf("Verify error: closing scan\n");
         return (rc);
      }
   }

   return (0);
}

//
// VerifyFloatIndex
//   - nStart is the starting point in the values array to check
//   - nEntries is the number of entries in the values array to check
//   - If bExists == 1, verify that an index has entries as added
//     by InsertIntEntries.  
//     If bExists == 0, verify that entries do NOT exist (you can 
//     use this to test deleting entries).
//
RC VerifyFloatIndex(IX_IndexHandle &ih, int nStart, int nEntries, int bExists)
{
   RC rc;
   int i;
   RID rid;
   IX_IndexScan scan;
   PageNum pageNum;
   SlotNum slotNum;
   float value;


   // Assume values still contains the array of values inserted/deleted

   printf("Verifying index contents\n");

   for (i = nStart; i < nStart + nEntries; i++) {
      value = values[i] + 1;

      if ((rc = scan.OpenScan(ih, EQ_OP, &value))) {
         printf("Verify error: opening scan\n");
         return (rc);
      }

      rc = scan.GetNextEntry(rid);
      if (!bExists && rc == 0) {
         printf("Verify error: found non-existent entry %f\n", value);
         return (IX_EOF);  // What should be returned here?
      }
      else if (bExists && rc == IX_EOF) {
         printf("Verify error: entry %f not found\n", value);
         return (IX_EOF);  // What should be returned here?
      }
      else if (rc != 0 && rc != IX_EOF)
         return (rc);

      if (bExists && rc == 0) {
         // Did we get the right entry?
         if ((rc = rid.GetPageNum(pageNum)) ||
               (rc = rid.GetSlotNum(slotNum)))
            return (rc);

         if (pageNum != value || slotNum != (value*2)) {
            printf("Verify error: incorrect rid (%d,%d) found for entry %f\n",
                  pageNum, slotNum, value);             
            return (IX_EOF);  // What should be returned here?
         }
         // Is there another entry?
         rc = scan.GetNextEntry(rid);
         if (rc == 0) {
            printf("Verify error: found two entries with same value %f\n", value);
            return (IX_EOF);  // What should be returned here?
         }
         else if (rc != IX_EOF)
            return (rc);
      }

      if ((rc = scan.CloseScan())) {
         printf("Verify error: closing scan\n");
         return (rc);
      }
   }

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

   printf("\nPrinting file contents\n");
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

//
// GetNextRecScan
//
// Get the next record in a record scan
//
RC GetNextRecScan(RM_FileScan &fs, RM_Record &rec)
{
   return(fs.GetNextRec(rec));
}


//
// OpenFile
//
// Desc: call RM_Manager::OpenFile
//
RC OpenFile(char *fileName, RM_FileHandle &fh)
{
   printf("\nOpening %s\n", fileName);
   return (rmm.OpenFile(fileName, fh));
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
// InsertInts
//
// Desc: Insert count ints in the range lo to hi excluding the range
//       exLo to exHi
//
RC InsertInts(IX_IndexHandle &ih, int count, int lo, int hi, int exLo, 
      int exHi)
{
   RC rc;
   int i, j, value;
   static int unique = 0;
   int count2 = 0;

   unique++;
   for (i = 1; i <= count; i++) {
      for (j = lo; j <= hi; j++) {
         value = j * 10;
         RID rid(unique, i*2);
         if ((j < exLo) || (j > exHi)) {
            if ((rc = ih.InsertEntry((void *)&value, rid))) {
               cout << "Inserted " << count2 << " values\n" << endl;
               return rc;
            }
            count2++;
         }
      }
   }
   cout << "Inserted " << count2 << " values\n" << endl;
   return 0;
}


// 
// DeleteInts 
//
// Desc: Delete all entries between lo and hi except those between
//       exLo and exHi inclusively
//
RC DeleteInts(IX_IndexHandle &ih, int lo, int hi, int exLo, int exHi)
{
   RC rc;
   int i, value;
   RID rid;
   int count = 0;
   IX_IndexScan indScn;

   for (i = lo; i <= hi; i++) {
      if ((i < exLo) || (i > exHi)) {
         value = i * 10;
         if ((rc = indScn.OpenScan(ih, EQ_OP, &value)))
            return (rc);
         while ((rc = indScn.GetNextEntry(rid)) != IX_EOF) {
            if ((rc = ih.DeleteEntry(&value, rid))) {
               cout << "Deleted " << count << " entries" << endl;
               return rc;
            }
            count++;
         }
         indScn.CloseScan();
         if (rc !=  IX_EOF) return rc;
      }
   }
   cout << "Deleted " << count << " entries" << endl;
   return 0;
}



//
// Full blown test
//
RC Test1(void)
{                  
   RC rc;
   IX_IndexHandle ihOK1, ihOK2, ihOK3, ihOK4, ihOK5, ihBAD1;
   int OK1 = 5, 
   OK2 = 10,
   OK3 = 15,
   OK4 = 20,
   OK5 = 25;
   int  nDelete = MANY_ENTRIES * 99/100,
   count = 0;
   RM_FileHandle rmFH;

   cout << "** Beginning Test";

   // Destroy the test indexes (not part of test)
   ixm.DestroyIndex(FILENAME, OK1); 
   ixm.DestroyIndex(FILENAME, OK2); 
   ixm.DestroyIndex(FILENAME, OK3);
   ixm.DestroyIndex(FILENAME, OK4);
   ixm.DestroyIndex(FILENAME, OK5);
	

   //  +++ Test the Manager class functionality +++

   //  Shouldn't be able to create an index with a negative number
   //  although the sheet says that the client should guarantee the
   //  index number is nonnegative
   printf("\n *** Illegal Index Create Test (Negative indexNo): %s\n",
         (rc = ixm.CreateIndex(FILENAME, -1, INT, sizeof(int))) 
         ? "PASS" : "FAIL\a");
   if (rc) PrintError(rc);
   else {
      cout << "Comment: Although the specifications indicate that the\n";
      cout << "client will guarantee NONNEGATIVE indexNo, it is a very\n";
      cout << "easy check to verify\n";
   }


   /*
   // Test for handling of NULL filename
   printf("\n *** Illegal Index Create Test (NULL Filename): %s\n",
         (rc = ixm.CreateIndex(NULL, 1, INT, sizeof(int))) 
         ? "PASS" : "FAIL\a");
   if (rc) PrintError(rc);
   else {
      cout << "Suggestion: Check for NULL fileName in CreateIndex\n";
   }
	*/

   // Nonexistent fileName
   printf("\n *** Illegal Index Create Test (Nonexisting Filename): %s\n",
         (rc = ixm.CreateIndex("DON/TEXIST", 1, INT, sizeof(int))) 
         ? "PASS" : "FAIL\a");
   if (rc) PrintError(rc);
   else {
      cout << "Suggestion: You are not checking the return value from the\n";
      cout << "PF component on an OpenFile call\n";
   }


   // Attribute length doesn't match attribute type
   // Bad INT size
   printf("\n *** Illegal Index Create Test (Bad INT length): %s\n",
         (rc = ixm.CreateIndex(FILENAME, 1, INT, 123)) 
         ? "PASS" : "FAIL\a");
   if (rc) PrintError(rc);
   else {
      cout << "Suggestion: Include a test in the CreateIndex method that\n";
      cout << "checks if an INT really is sizeof(int)\n";
   }


   // Bad FLOAT size
   printf("\n *** Illegal Index Create Test (Bad FLOAT length): %s\n",
         (rc = ixm.CreateIndex(FILENAME, 1, FLOAT, 123)) 
         ? "PASS" : "FAIL\a");
   if (rc) PrintError(rc);
   else {
      cout << "Suggestion: Include a test in the CreateIndex method that\n";
      cout << "checks if an FLOAT really is sizeof(float)\n";
   }

   // Bad STRING sizes
   printf("\n *** Illegal Index Create Test (Bad STRING length): %s\n",
         (rc = ixm.CreateIndex(FILENAME, 1, STRING, 0)) 
         ? "PASS" : "FAIL\a");
   if (rc) PrintError(rc);
   else {
      cout << "Suggestion: Include a test in the CreateIndex method that\n";
      cout << "checks if an STRING size is between 1 and 255\n";
   }

   printf("\n *** Illegal Index Create Test (Bad STRING length): %s\n",
         (rc = ixm.CreateIndex(FILENAME, 1, STRING, 300)) 
         ? "PASS" : "FAIL\a");
   if (rc) PrintError(rc);
   else {
      cout << "Suggestion: Include a test in the CreateIndex method that\n";
      cout << "checks if an STRING size is between 1 and 255\n";
   }


   // Bad attribute type
   printf("\n *** Illegal Index Create Test (Bad Type): %s\n",
         (rc = ixm.CreateIndex(FILENAME, 1, (AttrType) 55, 4)) 
         ? "PASS" : "FAIL\a");
   if (rc) PrintError(rc);
   else {
      cout << "Suggestion: Include a test in the CreateIndex method that\n";
      cout << "checks if the attribute type is INT, FLOAT or STRING\n";
   }

   // Create two legal ones
   printf("\n *** Legal Index Create Test: %s\n",
         (rc = ixm.CreateIndex(FILENAME, OK1, INT, 4)) 
         ? "FAIL\a" :"PASS");
   if (rc) PrintError(rc);


   printf("\n *** Legal Index Create Test: %s\n",
         (rc = ixm.CreateIndex(FILENAME, OK2, INT, 4)) 
         ? "FAIL\a" :"PASS");
   if (rc) PrintError(rc);



   // Destruction tests
   // NULL fileName
   /*
   printf("\n *** Illegal Index Destroy Test (NULL fileName): %s\n",
         (rc = ixm.DestroyIndex(NULL, 1)) 
         ? "PASS" : "FAIL\a");
   if (rc) PrintError(rc);
   else {
      cout << "Suggestion: Verify that DestroyIndex checks for a NULL\n";
      cout << "fileName\n";
   }
	*/

   // Non existing fileName
   printf("\n *** Illegal Index Destroy Test (Nonexisting fileName): %s\n",
         (rc = ixm.DestroyIndex("NOFILE", 1)) 
         ? "PASS" : "FAIL\a");
   if (rc) PrintError(rc);
   else {
      cout << "Suggestion: Verify that you are acting on errors from the\n";
      cout << "PF component layer\n";
   }


   // Negative index number
   printf("\n *** Illegal Index Destroy Test (Negative indexNo): %s\n",
         (rc = ixm.DestroyIndex(FILENAME, -1)) 
         ? "PASS" : "FAIL\a");
   if (rc) PrintError(rc);
   else {
      cout << "Comment: Although the specifications indicate that the\n";
      cout << "client will guarantee NONNEGATIVE indexNo, it is a very\n";
      cout << "easy check to verify\n";
   }



   // Unopen Index
   printf("\n *** Illegal Index Destroy Test (Uncreated Index): %s\n",
         (rc = ixm.DestroyIndex(FILENAME, 1)) 
         ? "PASS" : "FAIL\a");
   if (rc) PrintError(rc);
   else {
      cout << "Suggestion: Verify that you are acting on errors from the\n";
      cout << "PF component layer\n";
   }



   // Legal destruction
   printf("\n *** Legal Index Destroy Test: %s\n",
         (rc = ixm.DestroyIndex(FILENAME, OK1)) 
         ? "FAIL\a" : "PASS");
   if (rc) PrintError(rc);


   printf("\n *** Legal Index Destroy Test: %s\n",
         (rc = ixm.DestroyIndex(FILENAME, OK2)) 
         ? "FAIL\a" : "PASS");
   if (rc) PrintError(rc);



   // Recreate the indexes
   printf("\n *** Legal Index Create Test: %s\n",
         (rc = ixm.CreateIndex(FILENAME, OK1, INT, 4)) 
         ? "FAIL\a" :"PASS");
   if (rc) PrintError(rc);


   printf("\n *** Legal Index Create Test: %s\n",
         (rc = ixm.CreateIndex(FILENAME, OK2, INT, 4)) 
         ? "FAIL\a" :"PASS");
   if (rc) PrintError(rc);


   // Index Open tests
   // Nonexisting indexes
   printf("\n *** Illegal Index Open Test (Nonexisting fileName): %s\n",
         (rc = ixm.OpenIndex("NOFILE", 1, ihBAD1)) 
         ? "PASS" : "FAIL\a");
   if (rc) PrintError(rc);
   else {
      cout << "Suggestion: Verify that you are acting on errors from the\n";
      cout << "PF component layer\n";
   }

   /*
   // NULL fileName
   printf("\n *** Illegal Index Open Test (NULL fileName): %s\n",
         (rc = ixm.OpenIndex(NULL, 1, ihOK1)) 
         ? "PASS" : "FAIL\a");
   if (rc) PrintError(rc);
   else {
      cout << "Suggestion: Verify that DestroyIndex checks for a NULL\n";
      cout << "fileName\n";
   }
	*/

   // Non existing index name
   printf("\n *** Illegal Index Open Test (Nonexisting index name): %s\n",
         (rc = ixm.OpenIndex(FILENAME, 1, ihOK1)) 
         ? "PASS" : "FAIL\a");
   if (rc) PrintError(rc);
   else {
      cout << "Suggestion: Verify that you are acting on errors from the\n";
      cout << "PF component layer\n";
   }


   // Legal index open
   printf("\n *** Legal Index Open Test: %s\n",
         (rc = ixm.OpenIndex(FILENAME, OK1, ihOK1)) 
         ? "FAIL\a" : "PASS");
   if (rc) {
      PrintError(rc);
      cout << "Suggestion: See if you are setting the index handle to USED\n";
      cout << "even on an error in opening ... index handle should be \n";
      cout << "reuseable if a previous open caused an error\n";
   }


   // Legal index open (twice by different index handles)
   // Legal index open
   printf("\n *** Legal Index Open Test: %s\n",
         (rc = ixm.OpenIndex(FILENAME, OK1, ihOK2)) 
         ? "FAIL\a" : "PASS");
   if (rc) {
      PrintError(rc);
      cout << "Comment: Two different index handles should be able to open\n";
      cout << "the same index (fileName + indexNo)\n";
   }

   // Illegal to have the same index handle open two indexes
   printf("\n *** Illegal Index Open Test (One handle - two indexes): %s\n",
         (rc = ixm.OpenIndex(FILENAME, OK2, ihOK1)) 
         ? "PASS" : "FAIL\a");
   if (rc) PrintError(rc);
   else {
      cout << "Suggestion: Verify that you are setting and checking some  \n";
      cout << "state to indicate that an index handle already has an open \n";
      cout << "index\n";
   }

   // Illegal Index close tests
   printf("\n *** Illegal Index Close Test (handle w/out index): %s\n",
         (rc = ixm.CloseIndex(ihBAD1)) 
         ? "PASS" : "FAIL\a");
   if (rc) PrintError(rc);
   else {
      cout << "Suggestion: Verify that you are setting and checking some  \n";
      cout << "state to indicate whether an index handle has an open \n";
      cout << "index OR that this state is not being set to TRUE on an\n";
      cout << "OpenIndex call that results in an error\n";
   }

   // Legal index Close
   printf("\n *** Legal Index Close Test: %s\n",
         (rc = ixm.CloseIndex(ihOK2)) 
         ? "FAIL\a" : "PASS");
   if (rc) {
      PrintError(rc);
   }

   // Attempt to close the index twice
   printf("\n *** Illegal Index Close Test (second close): %s\n",
         (rc = ixm.CloseIndex(ihOK2)) 
         ? "PASS" : "FAIL\a");
   if (rc) PrintError(rc);
   else {
      cout << "Suggestion: Verify that you are setting and checking some  \n";
      cout << "state to indicate whether an index handle has an open \n";
      cout << "index OR that this state is being set to FASLE on an\n";
      cout << "CloseIndex";
   }

   // NEW FOR VERSION 3
   // Some Index Scan Tests
   // Supporting Index Handle does not have an open file
   int isVal = 3;
   RID isRid(5,3);
   IX_IndexScan is1;
   /*
   printf("\n *** Illegal Index Scan (file not open): %s\n",
         ((rc = is1.OpenScan(ihBAD1, EQ_OP, &isVal))
          ? "PASS" : "FAIL\a"));
   if (rc) { 
      PrintError(rc);
   } else {
      cout << "Suggestion: Make sure that IndexHandle that supports the\n";
      cout << "Index Scan has an open file. This should be a similar check\n";
      cout << "to that performed during InsertEntry and DeleteEntry\n";
   }
*/

   /*
   // Index scan called on a NULL value
   IX_IndexScan is2;
   printf("\n *** Illegal Index Scan (NULL value): %s\n",
         ((rc = is2.OpenScan(ihOK1, EQ_OP, NULL))
          ? "PASS" : "FAIL\a"));
   if (rc) { 
      PrintError(rc);
   } else {
      cout << "Suggestion: Make sure you check for NULL value.\n";
   }
*/
   // Validation tests courtesy of Dallan
   // Inserting and validating INTS
   printf("\n *** Dallan's Insertion and Validation Test (INT): %s\n",
         ((rc = ixm.CreateIndex(FILENAME, OK5, INT, sizeof(int))) ||
          (rc = ixm.OpenIndex(FILENAME, OK5, ihOK5)) ||
          (rc = InsertIntEntries(ihOK5, MANY_ENTRIES)) ||
          (rc = ixm.CloseIndex(ihOK5)) ||
          (rc = ixm.OpenIndex(FILENAME, OK5, ihOK5)) ||

          // ensure inserted entries are all there
          (rc = VerifyIntIndex(ihOK5, 0, MANY_ENTRIES, TRUE)) ||

          // ensure an entry not inserted is not there
          (rc = VerifyIntIndex(ihOK5, MANY_ENTRIES, 1, FALSE)) ||
          (rc = ixm.CloseIndex(ihOK5)))
         ? "FAIL\a" : "PASS");
   if (rc) {
      PrintError(rc);
   }


   printf("\n *** Destroying Index Test (INT): %s\n",
         (rc = ixm.DestroyIndex(FILENAME, OK5)) 
         ? "FAIL\a" : "PASS");
   if (rc) {
      PrintError(rc);
   }


   //Inserting and validating STRINGS
   printf("\n *** Insertion and Validation Test (STRING): %s\n",
         ((rc = ixm.CreateIndex(FILENAME, OK5, STRING, STRLEN)) ||
          (rc = ixm.OpenIndex(FILENAME, OK5, ihOK5)) ||
          (rc = InsertStringEntries(ihOK5, MANY_ENTRIES)) ||
          (rc = ixm.CloseIndex(ihOK5)) ||
          (rc = ixm.OpenIndex(FILENAME, OK5, ihOK5)) ||

          // ensure inserted entries are all there
          (rc = VerifyStringIndex(ihOK5, 0, MANY_ENTRIES, TRUE)) ||

          // ensure an entry not inserted is not there
          (rc = VerifyStringIndex(ihOK5, MANY_ENTRIES, 1, FALSE)) ||
          (rc = ixm.CloseIndex(ihOK5)))
         ? "FAIL\a" : "PASS");
   if (rc) {
      PrintError(rc);
   }

   printf("\n *** Destroying Index Test (STRING): %s\n",
         (rc = ixm.DestroyIndex(FILENAME, OK5)) 
         ? "FAIL\a" : "PASS");
   if (rc) {
      PrintError(rc);
   }



   //Inserting and validating FLOATS
   printf("\n *** Insertion and Validation Test (FLOAT): %s\n",
         ((rc = ixm.CreateIndex(FILENAME, OK5, FLOAT, sizeof(float))) ||
          (rc = ixm.OpenIndex(FILENAME, OK5, ihOK5)) ||
          (rc = InsertFloatEntries(ihOK5, MANY_ENTRIES)) ||
          (rc = ixm.CloseIndex(ihOK5)) ||
          (rc = ixm.OpenIndex(FILENAME, OK5, ihOK5)) ||

          // ensure inserted entries are all there
          (rc = VerifyFloatIndex(ihOK5, 0, MANY_ENTRIES, TRUE)) ||

          // ensure an entry not inserted is not there
          (rc = VerifyFloatIndex(ihOK5, MANY_ENTRIES, 1, FALSE)) ||
          (rc = ixm.CloseIndex(ihOK5)))
         ? "FAIL\a" : "PASS");
   if (rc) {
      PrintError(rc);
   }
   printf("\n *** Destroying Index Test (FLOAT): %s\n",
         (rc = ixm.DestroyIndex(FILENAME, OK5)) 
         ? "FAIL\a" : "PASS");
   if (rc) {
      PrintError(rc);
   }


   // Same tests with deletions

   printf("\n *** Dallan's Validation Test with Deletion(INT): %s\n",
         ((rc = ixm.CreateIndex(FILENAME, OK5, INT, sizeof(int))) ||
          (rc = ixm.OpenIndex(FILENAME, OK5, ihOK5)) ||
          (rc = InsertIntEntries(ihOK5, MANY_ENTRIES)) ||
          (rc = DeleteIntEntries(ihOK5, nDelete)) ||
          (rc = ixm.CloseIndex(ihOK5)) ||
          (rc = ixm.OpenIndex(FILENAME, OK5, ihOK5)) ||
          // ensure deleted entries are gone
          (rc = VerifyIntIndex(ihOK5, 0, nDelete, FALSE)) ||
          // ensure non-deleted entries still exist
          (rc = VerifyIntIndex(ihOK5, nDelete, MANY_ENTRIES - nDelete, TRUE)) ||
          (rc = ixm.CloseIndex(ihOK5)))
         ? "FAIL\a" : "PASS");
   if (rc) {
      PrintError(rc);
   }
   printf("\n *** Destroying Index Test (INT): %s\n",
         (rc = ixm.DestroyIndex(FILENAME, OK5)) 
         ? "FAIL\a" : "PASS");
   if (rc) {
      PrintError(rc);
   }


   // For FLOATS
   printf("\n *** Validation Test with Deletion(FLOAT): %s\n",
         ((rc = ixm.CreateIndex(FILENAME, OK5, FLOAT, sizeof(float))) ||
          (rc = ixm.OpenIndex(FILENAME, OK5, ihOK5)) ||
          (rc = InsertFloatEntries(ihOK5, MANY_ENTRIES)) ||
          (rc = DeleteFloatEntries(ihOK5, nDelete)) ||
          (rc = ixm.CloseIndex(ihOK5)) ||
          (rc = ixm.OpenIndex(FILENAME, OK5, ihOK5)) ||
          // ensure deleted entries are gone
          (rc = VerifyFloatIndex(ihOK5, 0, nDelete, FALSE)) ||
          // ensure non-deleted entries still exist
          (rc = VerifyFloatIndex(ihOK5, nDelete, MANY_ENTRIES - nDelete, TRUE)) ||
          (rc = ixm.CloseIndex(ihOK5)))
         ? "FAIL\a" : "PASS");
   if (rc) {
      PrintError(rc);
   }
   printf("\n *** Destroying Index Test (FLOAT): %s\n",
         (rc = ixm.DestroyIndex(FILENAME, OK5)) 
         ? "FAIL\a" : "PASS");
   if (rc) {
      PrintError(rc);
   }


   // For strings
   printf("\n *** Validation Test with Deletion(STRING): %s\n",
         ((rc = ixm.CreateIndex(FILENAME, OK5, STRING, STRLEN)) ||
          (rc = ixm.OpenIndex(FILENAME, OK5, ihOK5)) ||
          (rc = InsertStringEntries(ihOK5, MANY_ENTRIES)) ||
          (rc = DeleteStringEntries(ihOK5, nDelete)) ||
          (rc = ixm.CloseIndex(ihOK5)) ||
          (rc = ixm.OpenIndex(FILENAME, OK5, ihOK5)) ||
          // ensure deleted entries are gone
          (rc = VerifyStringIndex(ihOK5, 0, nDelete, FALSE)) ||
          // ensure non-deleted entries still exist
          (rc = VerifyStringIndex(ihOK5, nDelete, MANY_ENTRIES - nDelete, TRUE)) ||
          (rc = ixm.CloseIndex(ihOK5)))
         ? "FAIL\a" : "PASS");
   if (rc) {
      PrintError(rc);
   }
   printf("\n *** Destroying Index Test (STRING): %s\n",
         (rc = ixm.DestroyIndex(FILENAME, OK5)) 
         ? "FAIL\a" : "PASS");
   if (rc) {
      PrintError(rc);
   }


   // Scan and delete

   int value, i, j;
   RID rid;
   IX_IndexScan indScn;


   if ((rc = ixm.CreateIndex(FILENAME, OK5, INT, sizeof(int))) ||
         (rc = ixm.OpenIndex(FILENAME, OK5, ihOK5))) {
      PrintError(rc);
      goto end;
   }


   cout << "\nStarting scan and delete by inserting 50 values of 50 ints\n"; 
   for (i = 1; i < 51; i++) {
      for (j = 1; j < 51; j++) {
         value = j * 10;
         RID rid(i, i*2);
         if ((rc = ihOK5.InsertEntry((void *)&value, rid))) goto end;
      }
   }

   cout << "Deleting all of the indices for half of the values\n";
   for (i = 1; i < 26; i++) {
      value = i * 10;
      if ((rc = indScn.OpenScan(ihOK5, EQ_OP, &value)))
         goto end;
      while ((rc = indScn.GetNextEntry(rid)) == 0) {
         if ((rc = ihOK5.DeleteEntry(&value, rid))) goto end;
      }
      if (rc !=  IX_EOF) goto end;
      if ((rc = indScn.CloseScan()))
         goto end;
   }

   cout << "Verifying count of remaining values\n";
   for (i = 26; i < 51; i++) {
      value = i * 10;
      if ((rc = indScn.OpenScan(ihOK5, EQ_OP, &value)))
         goto end;
      while ((rc = indScn.GetNextEntry(rid)) == 0) {
         count++;
      }
      if (rc !=  IX_EOF) goto end;
      if ((rc = indScn.CloseScan()))
         goto end;
   }
end:


   printf("Scan and Delete Test %s\n", ((count == 1250) ? "PASS" : "FAIL\n"));
   if (rc) PrintError(rc);

   if ((rc = ixm.CloseIndex(ihOK5)) ||
         (rc = ixm.DestroyIndex(FILENAME, OK5))) {
      PrintError(rc);
   }


   /*********************************************************
     THE ACCORDIAN       --   FINAL (so far) TEST
    ************************************************/


   cout << "\n\n** Congratulations if you have made it this far \n";
   cout << "only one test left - THE ACCORDIAN ... Good Luck !!!\n"  << endl;


   if ((rc = ixm.CreateIndex(FILENAME, OK5, INT, sizeof(int))) ||
         (rc = ixm.OpenIndex(FILENAME, OK5, ihOK5))) {
      PrintError(rc);
      goto oops;
   }


   // Subtest 1: add 50 of values 1 - 50
   if ((rc = InsertInts(ihOK5, 50, 1, 50, 0, 0))) goto oops;
   count = 0;
   for (i = 1; i <= 50; i++) {
      value = i * 10;
      if ((rc = indScn.OpenScan(ihOK5, EQ_OP, &value)))
         goto oops;
      while ((rc = indScn.GetNextEntry(rid)) != IX_EOF) {
         if ((value < 10) || (value > 500)) {
            cout << "Unexpected entry found\n" << endl;
            goto oops;
         }
         count++;
      }
      if ((rc = indScn.CloseScan()))
         goto oops;
   }
   printf("\n\nAdd 50 of vals 1-50 : %s\n", 
         ((count == 2500) ? "PASS" : "FAIL\n"));  


   // Subtest 2: delete all values except val = 5
   if ((rc = DeleteInts(ihOK5, 1, 50, 5, 5))) goto oops;
   printf("\nDeleteInts passed\n");
   count = 0;
   for (i = 1; i <= 50; i++) {
      value = i * 10;
      if ((rc = indScn.OpenScan(ihOK5, EQ_OP, &value)))
         goto oops;
      while ((rc = indScn.GetNextEntry(rid)) != IX_EOF) {
         if ((value < 50) || (value > 50)) {
            cout << "Unexpected entry found\n" << endl;
            goto oops;
         }
         count++;
      }
      if ((rc = indScn.CloseScan()))
         goto oops;
   }
   printf("\n\nDel 50 all vals except 5 : %s\n", 
         ((count == 50) ? "PASS" : "FAIL\n"));  


   // Subtest 3: Add 50 values 1-50 (except val = 5)
   if ((rc = InsertInts(ihOK5, 50, 1, 50, 5, 5))) goto oops;
   count = 0;
   for (i = 1; i <= 50; i++) {
      value = i * 10;
      if ((rc = indScn.OpenScan(ihOK5, EQ_OP, &value)))
         goto oops;
      while ((rc = indScn.GetNextEntry(rid)) != IX_EOF) {
         if ((value < 10) || (value > 500)) {
            cout << "Unexpected entry found\n" << endl;
            goto oops;
         }
         count++;
      }
      if ((rc = indScn.CloseScan()))
         goto oops;
   }
   printf("\n\nAdd 50 of vals 1-50 (exc val = 5): %s\n", 
         ((count == 2500) ? "PASS" : "FAIL\n"));  


   // Subtest 4: Del all (except val = 5-6)
   if ((rc = DeleteInts(ihOK5, 1, 50, 5, 6))) goto oops;
   count = 0;
   for (i = 1; i <= 50; i++) {
      value = i * 10;
      if ((rc = indScn.OpenScan(ihOK5, EQ_OP, &value)))
         goto oops;
      while ((rc = indScn.GetNextEntry(rid)) != IX_EOF) {
         if ((value < 50) || (value > 60)) {
            cout << "Unexpected entry found\n" << endl;
            goto oops;
         }
         count++;
      }
      if ((rc = indScn.CloseScan()))
         goto oops;
   }
   printf("\n\nDel 50 all vals except 5-6 : %s\n", 
         ((count == 100) ? "PASS" : "FAIL\n"));  



   // Subtest 5: Add 50 values (except val = 5-6) 
   if ((rc = InsertInts(ihOK5, 50, 1, 50, 5, 6))) goto oops;
   count = 0;
   for (i = 1; i <= 50; i++) {
      value = i * 10;
      if ((rc = indScn.OpenScan(ihOK5, EQ_OP, &value)))
         goto oops;
      while ((rc = indScn.GetNextEntry(rid)) != IX_EOF) {
         if ((value < 10) || (value > 500)) {
            cout << "Unexpected entry found\n" << endl;
            goto oops;
         }
         count++;
      }
      if ((rc = indScn.CloseScan()))
         goto oops;
   }
   printf("\n\nAdd 50 of vals 1-50 (exc val = 5-6): %s\n", 
         ((count == 2500) ? "PASS" : "FAIL\n"));  


   // Subtest 5: Del all (except val = 5-7)
   if ((rc = DeleteInts(ihOK5, 1, 50, 5, 7))) goto oops;
   count = 0;
   for (i = 1; i <= 50; i++) {
      value = i * 10;
      if ((rc = indScn.OpenScan(ihOK5, EQ_OP, &value)))
         goto oops;
      while ((rc = indScn.GetNextEntry(rid)) != IX_EOF) {
         if ((value < 50) || (value > 70)) {
            cout << "Unexpected entry found\n" << endl;
            goto oops;
         }
         count++;
      }
      if ((rc = indScn.CloseScan()))
         goto oops;
   }
   printf("\n\nDel 50 all vals except 5-7 : %s\n", 
         ((count == 150) ? "PASS" : "FAIL\n"));  


   // Subtest add 50 values (exc val = 5-7)
   if ((rc = InsertInts(ihOK5, 50, 1, 50, 5, 7))) goto oops;
   count = 0;
   for (i = 1; i <= 50; i++) {
      value = i * 10;
      if ((rc = indScn.OpenScan(ihOK5, EQ_OP, &value)))
         goto oops;
      while ((rc = indScn.GetNextEntry(rid)) != IX_EOF) {
         if ((value < 10) || (value > 500)) {
            cout << "Unexpected entry found\n" << endl;
            goto oops;
         }
         count++;
      }
      if ((rc = indScn.CloseScan()))
         goto oops;
   }
   printf("\n\nAdd 50 of vals 1-50 (exc val = 5 - 7): %s\n", 
         ((count == 2500) ? "PASS" : "FAIL\n"));  


   // Subtest 5: Del all (except val = 5-8)
   if ((rc = DeleteInts(ihOK5, 1, 50, 5, 8))) goto oops;

   count = 0;
   for (i = 1; i <= 50; i++) {
      value = i * 10;
      if ((rc = indScn.OpenScan(ihOK5, EQ_OP, &value)))
         goto oops;
      while ((rc = indScn.GetNextEntry(rid)) != IX_EOF) {
         if ((value < 50) || (value > 80)) {
            cout << "Unexpected entry found\n" << endl;
            goto oops;
         }
         count++;
      }
      if ((rc = indScn.CloseScan()))
         goto oops;
   }

oops:
   printf("\n\nTHE ACCORDIAN : %s\n", ((count == 200) ? "PASS" : "FAIL\n"));  
   if (rc) PrintError(rc);

   if ((rc = ixm.CloseIndex(ihOK5))) {
      PrintError(rc);
   }

   return (0);
}
