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

IX_IndexScan::IX_IndexScan(){

}

IX_IndexScan::~IX_IndexScan(){

}

RC IX_IndexScan::OpenScan(const IX_IndexHandle &indexHandle,
                CompOp compOp,
                void *value,
                ClientHint  pinHint){
  RC rc = 0;
  return (rc);
}

RC IX_IndexScan::GetNextEntry(RID &rid){
  RC rc = 0;
  return (rc);
}

RC IX_IndexScan::CloseScan(){
  RC rc = 0;
  return (rc);
}