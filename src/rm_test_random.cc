
/* Does many insert, delete, update, flush operations chosen 
 * at random. Maintains a separate log to check whether the 
 * contents of the final relation are correct. 	  
 */

#include <cstdio>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <cstdlib>

#include "redbase.h"
#include "pf.h"
#include "rm.h"
#include "math.h"
#include <fstream>
 #include <assert.h>
 #include <vector>

using namespace std;

#define FILENAME   "testrel"         // test file name

//
// Global PF_Manager and RM_Manager variables
//
PF_Manager pfm;
RM_Manager rmm(pfm);

RC rc = 0;

int printError() {
  if(rc) {
    cerr << "rc == " << rc << endl;
    assert(0);
  }
  return 0;
}

ostream& operator<<(ostream & out, RID& rid) {
  PageNum page;
  SlotNum slot;
  rid.GetPageNum(page);
  rid.GetSlotNum(slot);
  cout << "pnum == " << page << " snum == " << slot <<
    endl;
  return out;
}

const int STRING_SIZE = 211;

struct MyRecord {
  
  int key;
  char bar[STRING_SIZE];
  float foo;
};

int main() {
  
  // Delete files from last time
  unlink(FILENAME);
  
  int recordSize = sizeof(MyRecord);
  
  rc = rmm.CreateFile(FILENAME, recordSize);
  printError();
  
  RM_FileHandle fHandle;
  rc = rmm.OpenFile(FILENAME, fHandle);
  printError();
  
  cout << "successfully opened the file" << endl;
  
  vector<RM_Record *> MyRMRecordLog;  
  
  int myRecordKey = 1;
  int numInstances = 0;
  
  enum { INSERTION = 0, FILE_OPERATION = 1, DELETION = 2, UPDATE = 3,
	 FORCE_PAGES_TO_DISK = 4 };
  const int NUM_OPERATIONS = 5;
  
  while(numInstances < 50000) {
    
    numInstances ++;
    
    int operation = rand() % NUM_OPERATIONS;
    
    switch(operation) {
      
    case FORCE_PAGES_TO_DISK:
      {
	if(rand() % 100 == 0) {
	  rc = fHandle.ForcePages(ALL_PAGES);
	  printError();
	}
      }
      break;
	
    case INSERTION:
	// create a new record 
	{
	// just give this a 1 in 5 chance 
	if(rand() % 5)
	  break;
	
	MyRecord * my_recPtr = new MyRecord;
	assert(my_recPtr);
	memset(my_recPtr, (char)(rand() % 10), recordSize);
	my_recPtr->key = myRecordKey ++;
	my_recPtr->foo = (12.45 * rand()) / 23.45;
	
	// note: insertRec requires the record data pointer and *not* the 
	// rm_record pointer
	RID rid;
	rc = fHandle.InsertRec((char *) my_recPtr, rid);
	printError();
	
	// create a RM_Record object ....
	RM_Record * rm_recPtr = new RM_Record;
	assert(rm_recPtr);
	rm_recPtr->SetData((char *) my_recPtr, recordSize);
	delete my_recPtr;
	rm_recPtr->SetRid(rid);
	
	// ... and insert into the log 
	MyRMRecordLog.push_back(rm_recPtr);
	
	cout << "inserted " << rid;
      }
      
      break;
      
    case DELETION:
      {
	
	// just give this a 1 in 10 chance 
	if(rand() % 10)
	  break;
	
	// pick a random record from the log and delete it
	int size = MyRMRecordLog.size();
	if(size) {
	  int index = rand() % size;
	  RID rid;
	  MyRMRecordLog[index]->GetRid(rid);
	  PageNum pnum;
	  rc = rid.GetPageNum(pnum);
	  printError();
	  
	  RM_Record rec;
	  
	  // read the record from file and check if it is the same thing
	  rc = fHandle.GetRec(rid, rec);
	  printError();
	  
	  RID recRid;
	  rc = rec.GetRid(recRid);
	  printError();
	  
	  // assert same rid
	  assert(recRid == rid);
	  
	  char * dataPtr1;
	  rc = rec.GetData(dataPtr1);
	  printError();
	  char * dataPtr2;
	  rc = MyRMRecordLog[index]->GetData(dataPtr2);
	  printError();
	  
	  // assert same data
	  assert(!memcmp(dataPtr1, dataPtr2, recordSize));
	  
	  // erase from log
	  vector<RM_Record *>::iterator it = MyRMRecordLog.begin();
	  for(int i=0; i < index; i++)
	    it ++;
	  delete (*it);
	  MyRMRecordLog.erase(it);
	  
	  // delete the record from the file
	  rc = fHandle.DeleteRec(rid);
	  printError();
	  
	  cout << "deleted " << rid;
	  
	  rc = fHandle.ForcePages(pnum);
	  printError();

	}
      }
      break;
      
    case UPDATE:
      {
	// just give this a 1 in 6 chance 
	if(rand() % 6)
	  break;
	
	// pick a random record from the log and update it
	int size = MyRMRecordLog.size();
	if(size) {
	  int index = rand() % size;
	  char * dataPtr2;
	  rc = MyRMRecordLog[index]->GetData(dataPtr2);
	  printError();
	  
	  // update the local copy 
	  memset(dataPtr2, (char)(rand() % 10), recordSize);
	  
	  // update the record in the file
	  rc = fHandle.UpdateRec(*MyRMRecordLog[index]);
	  printError();
	  
	  RID rid;
	  MyRMRecordLog[index]->GetRid(rid);
	  cout << "updated " << rid;
	}
      }
      break;
      
    case FILE_OPERATION:
      // close and re-open the file 
      {
	// just give this a 1 in 100 chance 
	if(rand() % 100)
	  break;
	
	rc = rmm.CloseFile(fHandle);
	printError();
	
	rc = rmm.OpenFile(FILENAME, fHandle);
	printError();
	
	cout << "closed and opened the file " << endl;
      }
      break;
      
    default:
      assert(0);
      break;
    };
  }
  
  RM_FileScan fs;

  // check the file scan part
  
  // int
#if 0
  int ival = 300;
  rc = fs.OpenScan(fHandle, INT, sizeof(int), offsetof(MyRecord, key),
		   GT_OP, &ival);
#endif  
  
  // float
#if 0
  float fval = 300;
  rc = fs.OpenScan(fHandle, FLOAT, sizeof(float), offsetof(MyRecord, foo),
		   GT_OP, &fval);
#endif
  
  // string 
  char str[STRING_SIZE];
  memset(str, 3, STRING_SIZE);
  rc = fs.OpenScan(fHandle, STRING, STRING_SIZE, offsetof(MyRecord, bar),
		   EQ_OP, str);
  printError();
  
  int numRecs = 0;
  RM_Record myRec;
  while(1) {
    rc = fs.GetNextRec(myRec);
    if(rc == RM_EOF)
      break;
    else {
      printError();
      numRecs ++;
      
      // check if this record is in the log
      RID rid;
      rc = myRec.GetRid(rid);
      printError();
      
      bool assert_true = false;
      vector<RM_Record *>::iterator it = MyRMRecordLog.begin();
      for(; it != MyRMRecordLog.end(); it++) {
	RID recRid;
	rc = (*it)->GetRid(recRid);
	printError();
	
	// rid has to be unique 
	if(rid == recRid) {
	  
	  char * dataPtr1;
	  rc = myRec.GetData(dataPtr1);
	  printError();
	  char * dataPtr2;
	  rc = (*it)->GetData(dataPtr2);
	  printError();
	  
	  // assert same data
	  assert(!memcmp(dataPtr1, dataPtr2, recordSize));
	  
	  assert_true = true;
	  break;
	}
      }
      assert(assert_true);
    }
  }
  // assert(numRecs == (int) MyRMRecordLog.size());
  cout << "numRecs == " << numRecs << endl;
  cout << "log size == " << (int) MyRMRecordLog.size() << endl;
  
  int num = 0;
  int numStrings = 0;
  float * fptr;
  char * dataPtr;
  char * stringPtr;
  
  vector<RM_Record *>::iterator it = MyRMRecordLog.begin();
  for(; it != MyRMRecordLog.end(); it++) {
    
    rc = (*it)->GetData(dataPtr);
    printError();    
    
    fptr = (float *) (dataPtr + offsetof(MyRecord, foo));
    if(*fptr > 300)
      num ++;
    
    stringPtr = (char *) (dataPtr + offsetof(MyRecord, bar));
    if(!memcmp(stringPtr, str, STRING_SIZE))
      numStrings ++;
  }
  cout << "num with float Val > 300 == " << num << endl;
  cout << "num matching strings == " << numStrings << endl;
  
  rc = fs.CloseScan();
  printError();

  rc = rmm.CloseFile(fHandle);
  printError();
  
  cout << "successfully closed the file" << endl;

  rc = rmm.DestroyFile(FILENAME); 
  printError();
  
  cerr << "Successful Completion" << endl;
  
  it = MyRMRecordLog.begin();
  for(; it != MyRMRecordLog.end(); it++) {
    delete (*it);
    *it = NULL;
  }
  exit(0);
  return 0;
}
