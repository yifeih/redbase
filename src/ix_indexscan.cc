//
// File:        ix_indexscan.cc
// Description: IX_IndexHandle handles scanning through the index for a 
//              certain value.
// Author:      Yifei Huang (yifei@stanford.edu)
//

#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "ix_internal.h"

/*
 * The following functions are comparison functions that return 0 if 
 * the two objects are equal, <0 if the first value is smaller, and
 * >0 if the second value is smaller.
 * They must take in an attribute type and attribute length, which 
 * determines the basis to compare the values on.
 */
bool equal(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 == *(float*)value2);
    case INT: return (*(int *)value1 == *(int *)value2) ;
    default:
      return (strncmp((char *) value1, (char *) value2, attrLength) == 0); 
  }
}

bool less_than(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 < *(float*)value2);
    case INT: return (*(int *)value1 < *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) < 0);
  }
}

bool greater_than(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 > *(float*)value2);
    case INT: return (*(int *)value1 > *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) > 0);
  }
}

bool less_than_or_eq_to(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 <= *(float*)value2);
    case INT: return (*(int *)value1 <= *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) <= 0);
  }
}

bool greater_than_or_eq_to(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 >= *(float*)value2);
    case INT: return (*(int *)value1 >= *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) >= 0);
  }
}

bool not_equal(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 != *(float*)value2);
    case INT: return (*(int *)value1 != *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) != 0);
  }
}


IX_IndexScan::IX_IndexScan(){
  openScan = false;
  value = NULL;
  initializedValue = false;
  hasBucketPinned = false;
  hasLeafPinned = false;
  scanEnded = true;
}

IX_IndexScan::~IX_IndexScan(){
  if(scanEnded == false && hasBucketPinned == true)
    indexHandle->pfh.UnpinPage(currBucketNum);
  if(scanEnded == false && hasLeafPinned == true)
    indexHandle->pfh.UnpinPage(currLeafNum);
  if(initializedValue == true){
    free(value);
    initializedValue = false;
  }

}

RC IX_IndexScan::OpenScan(const IX_IndexHandle &indexHandle,
                CompOp compOp,
                void *value,
                ClientHint  pinHint){
  RC rc = 0;

  if(openScan == true || compOp == NE_OP)
    return (IX_INVALIDSCAN);

  if(indexHandle.isValidIndexHeader())
    this->indexHandle = const_cast<IX_IndexHandle*>(&indexHandle);
  else
    return (IX_INVALIDSCAN);

  this->value = NULL;
  this->compOp = compOp;
  switch(compOp){
     case EQ_OP : comparator = &equal; break;
    case LT_OP : comparator = &less_than; break;
    case GT_OP : comparator = &greater_than; break;
    case LE_OP : comparator = &less_than_or_eq_to; break;
    case GE_OP : comparator = &greater_than_or_eq_to; break;
    case NE_OP : comparator = &not_equal; break;
    default: return (IX_INVALIDSCAN);
  }

  int attrLength = ((this->indexHandle)->header).attr_length;
  this->value = (void *) malloc(attrLength);
  memcpy(this->value, value, attrLength);
  initializedValue = true;

  openScan = true;
  scanEnded = false;
  hasBucketPinned = false;
  hasLeafPinned = false;

  return (rc);
}

RC IX_IndexScan::GetNextEntry(RID &rid){
  RC rc = 0;


  return (rc);
}

RC IX_IndexScan::CloseScan(){
  RC rc = 0;
  if(openScan == false)
    return (IX_INVALIDSCAN);
  if(scanEnded == false && hasBucketPinned == true)
    indexHandle->pfh.UnpinPage(currBucketNum);
  if(scanEnded == false && hasLeafPinned == true)
    indexHandle->pfh.UnpinPage(currLeafNum);
  if(initializedValue == true){
    free(value);
    initializedValue = false;
  }
  openScan = false;

  return (rc);
}