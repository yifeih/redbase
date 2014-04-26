/* 
 *  File     : my_ix_test.cc
 *  Purpose  : Test IX component
 *  Date     : 10/30/2000
 *  Author   : A Arvind (arvinda@cs.stanford.edu)
 */

#include <cstdio>
#include <cstdlib>

#include "redbase.h"
#include "ix.h"
#include <unistd.h>
#include <limits.h>

PF_Manager           pfm;
IX_Manager           ixm(pfm);
IX_IndexHandle       ixh;
IX_IndexScan         ixs;
RC                   rc;

// create an index
Boolean CreateIndex(char *filename){
  // create the index
  if((rc = ixm.CreateIndex(filename, 1, INT, 4))){
    IX_PrintError(rc);
    return FALSE;
  }  
  return TRUE;
}
 
// open the index
Boolean OpenIndex(char *filename){
  if((rc = ixm.OpenIndex(filename, 1, ixh))){
    IX_PrintError(rc);
    return FALSE;
  }  
  return TRUE;
}

// close the index
Boolean CloseIndex(){
  if((rc = ixm.CloseIndex(ixh))){
    IX_PrintError(rc);
    return FALSE;
  }  
  return TRUE;
}

Boolean DestroyIndex(char *filename){
  if((rc = ixm.DestroyIndex(filename, 1))){
    IX_PrintError(rc);
    return FALSE;
  }  
  return TRUE;
}

// open a scan
Boolean OpenScan(){
  if((rc = ixs.OpenScan(ixh, NO_OP, NULL))){
    IX_PrintError(rc);
    return FALSE;
  }  
  return TRUE;
}

Boolean CloseScan(){
  if((rc = ixs.CloseScan())){
    IX_PrintError(rc);
    return FALSE;
  }  
  return TRUE;
}

// Insert 30000 elements in sequential order
void Test1(){
  char *filename = "indexfile";
  RID  rid;
  
  printf("Performing Test1.........\n");
  printf("Creating the index file\n");
  
  if(!CreateIndex(filename))
    exit(1);
  
  
  if(!OpenIndex(filename))
    exit(1);
  
  for(int k = 1 ; k < 30000 ; k++){
    if((rc = ixh.InsertEntry(&k, RID(k,k)))){
      IX_PrintError(rc);
      exit(1);
    }
    if(k % 1000 == 0)
      printf("Inserted %d in the index\n", k);
  }
  
  printf("Closing the index\n");
  if(!CloseIndex())
    exit(1);

  printf("Destroying the indexfile\n");
  if(!DestroyIndex(filename))
    exit(1);

  printf("Test1 passed\n");
  return;
}
 

// Insert 30000 elements in reverse sequential order
void Test2(){
  char *filename = "indexfile";
  RID  rid;
  
  printf("Performing Test2.........\n");
  printf("Creating the index file\n");
  
  if(!CreateIndex(filename))
    exit(1);
  
  if(!OpenIndex(filename))
    exit(1);
  
  for(int k = 30000 ; k > 0 ; k--){
    if((rc = ixh.InsertEntry(&k, RID(k,k)))){
      IX_PrintError(rc);
      exit(1);
    }
    if(k % 1000 == 0)
      printf("Inserted %d in the index\n", k);
  }
    
  printf("Closing the index\n");
  if(!CloseIndex())
    exit(1);

  printf("Destroying the indexfile\n");
  if(!DestroyIndex(filename))
    exit(1);

  printf("Test2 passed\n");
  return;
}

// insert 30000 elements in sequential order and delete them in that order
void Test3(){
  char *filename = "indexfile";
  RID  rid;
  
  printf("Performing Test3.........\n");
  printf("Creating the index file\n");
  
  if(!CreateIndex(filename))
    exit(1);
  
  if(!OpenIndex(filename))
    exit(1);
  
  for(int k = 1 ; k < 30000 ; k++){
    if((rc = ixh.InsertEntry(&k, RID(k,k)))){
      IX_PrintError(rc);
      exit(1);
    }
    if(k % 1000 == 0)
      printf("Inserted %d in the index\n", k);
  }
  
  printf("Closing the index\n");
  if(!CloseIndex())
    exit(1);

  printf("Opening the index for deletion\n");
  if(!OpenIndex(filename))
    exit(1);
  
  // delete all the entries
  for(int k = 1 ; k < 30000 ; k++){
    if((rc = ixh.DeleteEntry(&k, RID(k,k)))){
      printf("The key %d not founf\n", k);
      IX_PrintError(rc);
      exit(1);
    }
    if(k % 1000 == 0)
      printf("Deleted %d in the index\n", k);
  }

  printf("Closing the index\n");
  if(!CloseIndex())
    exit(1);

  printf("Destroying the indexfile\n");
  if(!DestroyIndex(filename))
    exit(1);

  printf("Test3 passed\n");
  return;
}

// insert 30000 elements in sequential order and delete them in the
// reverse order
void Test4(){
  char *filename = "indexfile";
  RID  rid;
  
  printf("Performing Test4.........\n");
  printf("Creating the index file\n");
  
  if(!CreateIndex(filename))
    exit(1);
  
  if(!OpenIndex(filename))
    exit(1);
  
  for(int k = 1 ; k < 30000 ; k++){
    if((rc = ixh.InsertEntry(&k, RID(k,k)))){
      IX_PrintError(rc);
      exit(1);
    }
    if(k % 1000 == 0)
      printf("Inserted %d in the index\n", k);
  }
  
  printf("Closing the index\n");
  if(!CloseIndex())
    exit(1);

  printf("Opening the index for deletion\n");
  if(!OpenIndex(filename))
    exit(1);
  
  // delete all the entries
  for(int k = 29999 ; k > 0 ; k--){
    if((rc = ixh.DeleteEntry(&k, RID(k,k)))){
      printf("The key %d not founf\n", k);
      IX_PrintError(rc);
      exit(1);
    }
    if(k % 1000 == 0)
      printf("Deleted %d in the index\n", k);
  }

  IX_IndexScan scan;
  int value = 1;
  if ((rc = scan.OpenScan(ixh, NO_OP, &value))) {
    return;
  }

  RID rid2;
  if((0 == scan.GetNextEntry(rid2))){
    printf("WRONG\n");
    return;
  }

  if((rc = scan.CloseScan() ))
    return;

  printf("Closing the index\n");
  if(!CloseIndex())
    exit(1);

  printf("Destroying the indexfile\n");
  if(!DestroyIndex(filename))
    exit(1);

  printf("Test4 passed\n");
  return;
}

// insert 30000 elements all of the same value but different rids and
// delete ech one of them
void Test5(){
  char *filename = "indexfile";
  RID  rid;
  int  con = 21;

  printf("Performing Test5.........\n");
  printf("Creating the index file\n");
  
  if(!CreateIndex(filename))
    exit(1);
  
  if(!OpenIndex(filename))
    exit(1);
  
  for(int k = 1 ; k < 3000 ; k++){
    if((rc = ixh.InsertEntry(&con, RID(k,k)))){
      IX_PrintError(rc);
      exit(1);
    }
    if(k % 100 == 0)
      printf("Inserted %d in the index\n", k);
  }
  
  printf("Closing the index\n");
  if(!CloseIndex())
    exit(1);

  printf("Opening the index for deletion\n");
  if(!OpenIndex(filename))
    exit(1);
  
  // delete all the entries
  for(int k = 2999 ; k > 0 ; k--){
    if((rc = ixh.DeleteEntry(&con, RID(k,k)))){
      printf("The key %d not founf\n", k);
      IX_PrintError(rc);
      exit(1);
    }
    if(k % 100 == 0)
      printf("Deleted %d in the index\n", k);
  }

  printf("Closing the index\n");
  if(!CloseIndex())
    exit(1);

  printf("Destroying the indexfile\n");
  if(!DestroyIndex(filename))
    exit(1);

  printf("Test5 passed\n");
  return;
}

// Insert 30000 random numbers
void Test6(){
  char *filename = "indexfile";
  RID  rid;
  int  r;
  Boolean flag[30000];

  printf("Performing Test6.........\n");
  printf("Creating the index file\n");
  
  if(!CreateIndex(filename))
    exit(1);
  
  if(!OpenIndex(filename))
    exit(1);
  
  for(int k = 1 ; k < 30000 ; k++){
    r = lrand48() % INT_MAX;
    if((rc = ixh.InsertEntry(&r, RID(r,k)))){
      IX_PrintError(rc);
      printf("r = %d\n", r);
      exit(1);
    }
    
    // initialize the flag to FALSE
    flag[k] = FALSE;
    if(k % 1000 == 0)
      printf("Inserted %d in the index\n", k);
  }
  
  printf("Opening a scan\n");
  if(!OpenScan())
    exit(1);

  int last = 0;
  int scanCount = 0;
  while((rc = ixs.GetNextEntry(rid)) == 0){
    PageNum pNum;
    SlotNum sNum;
    rid.GetPageNum(pNum);
    rid.GetSlotNum(sNum);
    
    // set the flag to true to indicate that the element has been seen
    flag[sNum] = TRUE;
    
    if(pNum < last){
      printf("Error: The scan not in sorted order\n");
      exit(1);
    }

    if(scanCount % 1000 == 0)
      printf("scanned %d elements\n", scanCount);
    scanCount++;

    last = pNum;
  }
  if(rc != IX_EOF){
    IX_PrintError(rc);
    exit(1);
  }

  printf("Checking if all elements have been seen in the scan\n");
  // check if all the elements have been seen
  for(int i = 1 ; i < 30000 ; i++)
    if(!flag[i]){
      printf("Scan didnot retrieve all elements!\n");
      exit(1);
    }
  
  printf("Closing the scan\n");
  CloseScan();

  printf("Closing the index\n");
  if(!CloseIndex())
    exit(1);

  printf("Destroying the indexfile\n");
  if(!DestroyIndex(filename))
    exit(1);

  printf("Test6 passed\n");
  return;
}

// Insert 30000 sequential numbers  and delete them in a scan
void Test7(){
  char *filename = "indexfile";
  RID  rid;
  int  key;

  printf("Performing Test7.........\n");
  printf("Creating the index file\n");
  
  if(!CreateIndex(filename))
    exit(1);
    
  if(!OpenIndex(filename))
    exit(1);
  
  for(int k = 1 ; k < 30000 ; k++){
    if((rc = ixh.InsertEntry(&k, RID(k,k)))){
      IX_PrintError(rc);
      exit(1);
    }
    if(k % 1000 == 0)
      printf("Inserted %d in the index\n", k);
  }
  
  // open a scan
  printf("Opening a scan\n");
  if(!OpenScan())
    exit(1);  

  while(((rc = ixs.GetNextEntry(rid))) == 0){
    PageNum pNum;
    SlotNum sNum;
    rid.GetPageNum(pNum);
    rid.GetSlotNum(sNum);
    
    // delete the entry
    key = pNum;
    if((rc = ixh.DeleteEntry(&key, rid))){
      IX_PrintError(rc);
      exit(1);
    }

    if(pNum % 1000 == 0)
      printf("Deleted %d in the index\n", pNum);
  }
  if(rc != IX_EOF){
    IX_PrintError(rc);
    exit(1);
  }

  printf("Closing the scan\n");
  CloseScan();
  
  printf("Closing the index\n");
  if(!CloseIndex())
    exit(1);

  printf("Destroying the indexfile\n");
  if(!DestroyIndex(filename))
    exit(1);

  printf("Test7 passed\n");
  return;
}

// insert 30000 random numbers and delete them in a scan
void Test8(){
  char *filename = "indexfile";
  RID  rid;
  int  key;
  int  r;
  
  printf("Performing Test8.........\n");
  printf("Creating the index file\n");
  
  if(!CreateIndex(filename))
    exit(1);
    
  if(!OpenIndex(filename))
    exit(1);
  
  for(int k = 1 ; k < 10000 ; k++){
    r = lrand48() % INT_MAX;
    if((rc = ixh.InsertEntry(&r, RID(r,k)))){
      IX_PrintError(rc);
      exit(1);
    }
    if(k % 1000 == 0)
      printf("Inserted %d in the index\n", k);
  }
  
  // open a scan
  printf("Opening a scan\n");
  if(!OpenScan())
    exit(1);  

  int scanCount = 0;
  while(((rc = ixs.GetNextEntry(rid))) == 0){
    PageNum pNum;
    SlotNum sNum;
    rid.GetPageNum(pNum);
    rid.GetSlotNum(sNum);

    if(scanCount % 1000 == 0)
      printf("scanned %d elements\n", scanCount);
    scanCount++;

    // delete the entry
    key = pNum;
    //printf("key: %d \n", key);
    if((rc = ixh.DeleteEntry(&key, rid))){
      //printf("cannot delete key %d \n", key);
      IX_PrintError(rc);
      exit(1);
    }
  }
  if(rc != IX_EOF){
    IX_PrintError(rc);
    exit(1);
  }

  printf("Closing the scan\n");
  CloseScan();
  
  printf("Closing the index\n");
  if(!CloseIndex())
    exit(1);

  printf("Destroying the indexfile\n");
  if(!DestroyIndex(filename))
    exit(1);

  printf("Test8 passed\n");
  return;
}

#if 1
int main(){
  Test1();
  Test2();
  Test3();
  Test4();
  Test5();
  Test6();
  Test7();
  Test8();
}
#endif

