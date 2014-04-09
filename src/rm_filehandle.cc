#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "rm_internal.h"

RM_FileHandle::RM_FileHandle(){
  pfh = NULL;
  header = NULL;
  header_modified = false;
}

RM_FileHandle::~RM_FileHandle(){
  if(pfh != NULL)
    delete pfh;
  if(header != NULL)
    delete header;
}

RC RM_FileHandle::AllocateNewPage(){
  RC rc;

  PF_PageHandle ph;
  if(rc = pfh->AllocatePage(ph)){
    return rc;
  }
 
  RM_RecBitmap bitmap(header->numRecordsPerPage);
  return -1;

}

RC RM_FileHandle::FindFreeRec(PageNum pageNum){
  return -1;
}

RC RM_FileHandle::GetRec (const RID &rid, RM_Record &rec) const {
  // CHECK FILE HEADER CREDENTIALS
  return -1; 
}

RC RM_FileHandle::InsertRec (const char *pData, RID &rid) {
  // CHECK FILE HEADER CREDENTIALS

  // check data is not null
  // Find first free
  // If no free page, insert free page
  // 
  return -1; 
}

RC RM_FileHandle::DeleteRec (const RID &rid) {

  return -1; 
}
RC RM_FileHandle::UpdateRec (const RM_Record &rec) {
  return -1; 
}


RC RM_FileHandle::ForcePages(PageNum pageNum) {
  // Check if valid page number
  // PF_FileHandle - push pages 
  return -1; 
}
