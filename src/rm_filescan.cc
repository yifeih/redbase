#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "rm_internal.h"

RM_FileScan::RM_FileScan(){}

RM_FileScan::~RM_FileScan(){}

RC RM_FileScan::OpenScan (const RM_FileHandle &fileHandle,
                  AttrType   attrType,
                  int        attrLength,
                  int        attrOffset,
                  CompOp     compOp,
                  void       *value,
                  ClientHint pinHint) {return -1; } 


RC RM_FileScan::GetNextRec(RM_Record &rec) {return -1; }
RC RM_FileScan::CloseScan () {return -1; }