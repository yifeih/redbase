#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "rm_internal.h"

RM_FileHandle::RM_FileHandle(){
  pfh = NULL;
  header = NULL;
  header_modified = false;
  pageHeaderSize = 0;
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
  RM_PageHeader *pageHeader = new RM_PageHeader(bitmap);
  pageHeader->recordBitmap = bitmap;
  pageHeader->nextFreePage = header->firstFreePage;
  pageHeader->numRecords = 0;

  char *pageData;
  if(rc = ph.GetData(pageData))
    return rc;
  memcpy(pageData, pageHeader, sizeof(pageHeader));


  return 0;

}

RC RM_FileHandle::FindFreeSlot(PageNum pageNum){
  RC rc;
  PF_PageHandle ph;
  if(rc = pfh->GetThisPage(pageNum, ph));

  return -1;
}

RC RM_FileHandle::GetRec (const RID &rid, RM_Record &rec) const {
  // CHECK FILE HEADER CREDENTIALS
  PageNum page;
  SlotNum slot;
  RC rc;
  if(rc = rid.GetPageNum(page))
    return rc;
  if(rc = rid.GetSlotNum(slot))
    return rc;

  // TODO: check if valid RID

  PF_PageHandle ph;
  if(rc = pfh->GetThisPage(page, ph))
    return rc;
  char * recData;
  if(rc = ph.GetData(recData))
    return rc;

  rec.SetRecord(rid, recData + (header->headerOffset) + slot*(header->recordSize), header->recordSize);
  
  return 0; 
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
