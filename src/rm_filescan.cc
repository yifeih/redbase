#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "rm_internal.h"
#include <string>
#include <stdlib.h>
#include <cstdio>

using namespace std;

RM_FileScan::RM_FileScan(){
  openScan = false;
}

RM_FileScan::~RM_FileScan(){}

int compare_string(string &v1, string &v2, void * value1, void * value2, int attrLength){
  v1.copy( (char * )value1, (size_t)attrLength, 0);
  v2.copy( (char * )value2, (size_t)attrLength, 0);
  return v1.compare(v2);
}


bool equal(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 == *(float*)value2);
    case INT: return (*(int *)value1 == *(int *)value2) ;
    default: 
      string v1, v2;
      return (compare_string(v1,v2,value1,value2, attrLength) == 0);
  }
}

bool less_than(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 < *(float*)value2);
    case INT: return (*(int *)value1 < *(int *)value2) ;
    default: 
      string v1, v2;
      return (compare_string(v1,v2,value1,value2, attrLength) < 0);
  }
}

bool greater_than(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 > *(float*)value2);
    case INT: return (*(int *)value1 > *(int *)value2) ;
    default: 
      string v1, v2;
      return (compare_string(v1,v2,value1,value2, attrLength) > 0);
  }
}

bool less_than_or_eq_to(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 <= *(float*)value2);
    case INT: return (*(int *)value1 <= *(int *)value2) ;
    default: 
      string v1, v2;
      return (compare_string(v1,v2,value1,value2, attrLength) <= 0);
  }
}

bool greater_than_or_eq_to(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 >= *(float*)value2);
    case INT: return (*(int *)value1 >= *(int *)value2) ;
    default: 
      string v1, v2;
      return (compare_string(v1,v2,value1,value2, attrLength) >= 0);
  }
}

bool not_equal(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 != *(float*)value2);
    case INT: return (*(int *)value1 != *(int *)value2) ;
    default: 
      string v1, v2;
      return (compare_string(v1,v2,value1,value2, attrLength) != 0);
  }
}


RC RM_FileScan::OpenScan (const RM_FileHandle &fileHandle,
                  AttrType   attrType,
                  int        attrLength,
                  int        attrOffset,
                  CompOp     compOp,
                  void       *value,
                  ClientHint pinHint) {
  if (openScan == true)
    return (RM_INVALIDSCAN);

  if(fileHandle.isValidFileHeader())
    this->fileHandle = const_cast<RM_FileHandle*>(&fileHandle);
  else
    return (RM_INVALIDFILE);

  this->value = value;
  this->compOp = compOp;

  if(this->compOp != NO_OP){
    int recSize = (this->fileHandle)->getRecordSize();
    if((attrOffset + attrLength) > recSize)
      return (RM_INVALIDSCAN);
    this->attrOffset = attrOffset;
    this->attrLength = attrLength;

    if(attrType == FLOAT || attrType == INT){
      this->value = (void *) malloc(4);
      memcpy(this->value, value, 4);
    }
    else if(attrType == STRING){
      this->value = (void *) malloc(attrLength);
      memcpy(this->value, value, attrLength);
    }
    else{
      return (RM_INVALIDSCAN);
    }

    this->attrType = attrType;
  }


  switch(compOp){
    case EQ_OP : comparator = &equal; break;
    case LT_OP : comparator = &less_than; break;
    case GT_OP : comparator = &greater_than; break;
    case LE_OP : comparator = &less_than_or_eq_to; break;
    case GE_OP : comparator = &greater_than_or_eq_to; break;
    case NE_OP : comparator = &not_equal; break;
    case NO_OP : comparator = NULL; break;
    default: return (RM_INVALIDSCAN);
  }

  openScan = true;
  scanEnded = false;

  RC rc;

  // Set up the first page, and 
  if((this->fileHandle)->pfh.GetNextPage(0, currentPH) == PF_EOF)
    scanEnded = true;
  currentPH.GetPageNum(scanPage);
  if((rc = GetNumRecOnPage(currentPH, numRecOnPage)))
    return (rc);
  /*if(rc = (this->fileHandle)->pfh.UnpinPage(scanPage)){
    return (rc);
  }*/

  scanSlot = BEGIN_SCAN;
  numSeenOnPage = 0;
  return (0);
} 

RC RM_FileScan::GetNumRecOnPage(PF_PageHandle &ph, int &numRecords){
  RC rc;
  char *bitmap;
  struct RM_PageHeader *pageheader;
  if((rc = (this->fileHandle)->GetPageDataAndBitmap(ph, bitmap, pageheader)))
      return (rc);
  numRecords = pageheader->numRecords;
  return (0);
}

RC RM_FileScan::GetNextRec(RM_Record &rec) {
  if(scanEnded == true)
    return (RM_EOF);
  RC rc;
  while(true){
    RM_Record temprec;
    if((rc=fileHandle->GetNextRecord(scanPage, scanSlot, temprec, currentPH))){
      if(rc == RM_EOF){
        scanEnded = true;
      }
      return (rc);
    }
    RID rid;
    //printf("rec size in filescan: %d \n", temprec.size);
    temprec.GetRid(rid);

    rid.GetPageNum(scanPage);
    rid.GetSlotNum(scanSlot);
    //printf("Rec retrieved: %d, %d \n", scanPage, scanSlot);
    /*
    if(this->slot == (header.numRecordsPerPage -1)){
      this->page++;
      this->slot = 0;
    }
    */
    char *pData;
    if((rc = temprec.GetData(pData))){
      return (rc);
    }
    if(compOp != NO_OP){
      bool satisfies = (* comparator)(pData + attrOffset, this->value, attrType, attrLength);
      if(satisfies){
        rec = temprec;
      break;
      }
    }
    else{
      rec = temprec;
      break;
    }
  }
  return 0;
}

RC RM_FileScan::CloseScan () {
  RC rc;
  if(openScan == false){
    return (RM_INVALIDSCAN);
  }
  if(scanEnded == false){
    if((rc = fileHandle->pfh.UnpinPage(scanPage)))
      return (rc);
  }
  free(value);
  openScan = false;
  return (0);
}