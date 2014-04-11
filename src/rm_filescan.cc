#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "rm_internal.h"

RM_FileScan::RM_FileScan(){
  openScan = false;
}

RM_FileScan::~RM_FileScan(){}


int equal(void * value1, void * value2, AttrType attrtype){
  return 0;
}

int less_than(void * value1, void * value2, AttrType attrtype){
  return 0;
}

int greater_than(void * value1, void * value2, AttrType attrtype){
  return 0;
}

int less_than_or_eq_to(void * value1, void * value2, AttrType attrtype){
  return 0;
}

int greater_than_or_eq_to(void * value1, void * value2, AttrType attrtype){
  return 0;
}

int not_equal(void * value1, void * value2, AttrType attrtype){
  return 0;
}


RC RM_FileScan::OpenScan (const RM_FileHandle &fileHandle,
                  AttrType   attrType,
                  int        attrLength,
                  int        attrOffset,
                  CompOp     compOp,
                  void       *value,
                  ClientHint pinHint) {

  this->attrOffset = attrOffset;
  this->attrLength = attrLength;
  this->value = value;
  this->attrType = attrType;
  this->compOp = compOp;

  switch(compOp){
    case EQ_OP : comparator = equal; break;
    case LT_OP : comparator = less_than; break;
    case GT_OP : comparator = greater_than; break;
    case LE_OP : comparator = less_than_or_eq_to; break;
    case GE_OP : comparator = greater_than_or_eq_to; break;
    case NE_OP : comparator = not_equal; break;
    case NO_OP : comparator = NULL; break;
    default: return (RM_INVALIDSCAN_COMP);
  }

  openScan = true;
  return (0);
} 


RC RM_FileScan::GetNextRec(RM_Record &rec) {return -1; }
RC RM_FileScan::CloseScan () {
  openScan = false;
}