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

// RETURNS PINNED PAGE
RC RM_FileHandle::AllocateNewPage(PF_PageHandle &ph, PageNum &page){
  RC rc;

  if((rc = pfh->AllocatePage(ph))){
    return (rc);
  }
  header->numPages++;
  ph.GetPageNum(page);
 
  // Setup Header
  RM_PageHeader *pageHeader = new RM_PageHeader();
  pageHeader->nextFreePage = header->firstFreePage;
  pageHeader->numRecords = 0;

  // Copy header
  char *pageData;
  if((rc = ph.GetData(pageData))){
    return (rc);
  }
  memcpy(pageData, pageHeader, sizeof(pageHeader));

  // Setup Bitmap right after header
  char bitmap[header->bitmapSize];
  if((rc = ResetBitmap(bitmap, header->bitmapSize)))
    return (rc);

  memcpy(pageData+header->bitmapOffset, bitmap, header->bitmapSize);

  return (0);

}

RC RM_FileHandle::GetPageDataAndBitmap(PF_PageHandle &ph, char *&bitmap, struct RM_PageHeader *&pageheader) const{
  // Unpin page now before the error statements. We can unpin now because
  // We do not have to worry about concurrency in this implementation
  RC rc;

  char * pData;
  if((rc = ph.GetData(pData)))
    return (rc);
  
  pageheader = (struct RM_PageHeader *) pData;

  bitmap = pData + header->bitmapOffset;
  return (0);
}

RC RM_FileHandle::GetPageNumAndSlot(const RID &rid, PageNum &page, SlotNum &slot) const {
  RC rc;
  if((rc = rid.isValidRID()))
    return (rc);
  
  if((rc = rid.GetPageNum(page)))
    return (rc);
  if((rc = rid.GetSlotNum(slot)))
    return (rc);

  return (0);
}

RC RM_FileHandle::GetRec (const RID &rid, RM_Record &rec) const {
  if (!isValidFH())
    return (RM_INVALIDFILE);
  // CHECK FILE HEADER CREDENTIALS
  RC rc;
  // Get RID of the record
  PageNum page;
  SlotNum slot;
  if((rc = GetPageNumAndSlot(rid, page, slot)))
    return (rc);

  // TODO: check if valid RID
  // maybe not?

  // Get this page, 
  PF_PageHandle ph;
  if((rc = pfh->GetThisPage(page, ph)))
    return (rc);
  if((rc = pfh->UnpinPage(page)))
    return (rc);
  /*
  // Unpin page now before the error statements. We can unpin now because
  // We do not have to worry about concurrency in this implementation
  if(rc = pfh->UnpinPage(page))
    return (rc);

  char * pData;
  if(rc = ph.GetData(pData))
    return (rc);*/

  char *bitmap;
  struct RM_PageHeader *pageheader;
  if((rc = GetPageDataAndBitmap(ph, bitmap, pageheader)))
    return (rc);

  // Check if there really exists a record here according to the header
  bool recordExists;
  if ((rc = CheckBitSet(bitmap, header->bitmapSize, slot, recordExists)))
    return (rc);

  if(!recordExists)
    return RM_INVALIDRECORD;

  // Set the record and return it
  if((rc = rec.SetRecord(rid, bitmap + (header->bitmapSize) + slot*(header->recordSize), 
    header->recordSize)))
    return (rc);

  return (0); 
}

RC RM_FileHandle::InsertRec (const char *pData, RID &rid) {
  if (!isValidFH())
    return (RM_INVALIDFILE);
  // CHECK FILE HEADER CREDENTIALS
  RC rc;
  
  if(pData == NULL)
    return RM_INVALIDRECORD;
  
  PF_PageHandle ph;
  PageNum page;
  if (header->firstFreePage == NO_FREE_PAGES)
    AllocateNewPage(ph, page);
  else{
    if((rc = pfh->GetThisPage(header->firstFreePage, ph)))
      return (rc);
    page = header->firstFreePage;
  }

  if((rc = pfh->UnpinPage(page)))
    return (rc);


  char *bitmap;
  struct RM_PageHeader *pageheader;
  if((rc = GetPageDataAndBitmap(ph, bitmap, pageheader)))
    return (rc);

  int slot;
  if((rc = GetFirstZeroBit(bitmap, header->bitmapSize, slot)))
    return (rc);
  if((rc = SetBit(bitmap, (header->bitmapSize), slot)))
    return (rc);

  memcpy(bitmap + (header->bitmapSize) + slot*(header->recordSize),
    pData, header->recordSize);

  pageheader->numRecords++;
  //if(IsPageFull(data + (header->bitmapOffset), header->bitmapSize)){
  if(pageheader->numRecords == header->numRecordsPerPage){
    header->firstFreePage = pageheader->nextFreePage;
  }

  if((rc = pfh->MarkDirty(page) ))
    return (rc);

  // check data is not null
  // Find first free
  // If no free page, insert free page
  // 
  return (0); 
}

RC RM_FileHandle::DeleteRec (const RID &rid) {
  if (!isValidFH())
    return (RM_INVALIDFILE);
  // Check valid RID
  RC rc;

  PageNum page;
  SlotNum slot;
  if((rc = GetPageNumAndSlot(rid, page, slot)))
    return (rc);

  // Get this page, 
  PF_PageHandle ph;
  if((rc = pfh->GetThisPage(page, ph)))
    return (rc);
  if((rc = pfh->UnpinPage(page)))
    return (rc);

  char *bitmap;
  struct RM_PageHeader *pageheader;
  if((rc = GetPageDataAndBitmap(ph, bitmap, pageheader)))
    return (rc);

  // Check if there really exists a record here according to the header
  bool recordExists;
  if ((rc = CheckBitSet(bitmap, header->bitmapSize, slot, recordExists)))
    return (rc);
  if(!recordExists)
    return RM_INVALIDRECORD;


  if((rc = ResetBit(bitmap, (header->bitmapSize), slot)))
    return (rc);
  pageheader->numRecords--;

  if((rc = pfh->MarkDirty(page)))
    return (rc);

  return (0); 
}
RC RM_FileHandle::UpdateRec (const RM_Record &rec) {
  if (!isValidFH())
    return (RM_INVALIDFILE);
    // Check valid RID
  RC rc;

  RID rid;
  if((rc = rec.GetRid(rid)))
    return (rc);
  PageNum page;
  SlotNum slot;
  if((rc = GetPageNumAndSlot(rid, page, slot)))
    return (rc);

  // Get this page, 
  PF_PageHandle ph;
  if((rc = pfh->GetThisPage(page, ph)))
    return (rc);
  if((rc = pfh->UnpinPage(page)))
    return (rc);

  char *bitmap;
  struct RM_PageHeader *pageheader;
  if((rc = GetPageDataAndBitmap(ph, bitmap, pageheader)))
    return (rc);

  // Check if there really exists a record here according to the header
  bool recordExists;
  if ((rc = CheckBitSet(bitmap, header->bitmapSize, slot, recordExists)))
    return (rc);
  if(!recordExists)
    return RM_INVALIDRECORD;

  char * recData;
  if((rc = rec.GetData(recData)))
    return (rc);
  memcpy(bitmap + (header->bitmapSize) + slot*(header->recordSize),
    recData, header->recordSize);

  if((rc = pfh->MarkDirty(page)))
    return (rc);
  return (0); 
}


RC RM_FileHandle::ForcePages(PageNum pageNum) {
  // Check if valid page number
  // PF_FileHandle - push pages
  // Get this page, 
  if (!isValidFH())
    return (RM_INVALIDFILE);

  pfh->ForcePages(pageNum);

  return (0); 
}


bool RM_FileHandle::isValidFH() const{
  if(pfh != NULL && header != NULL)
    return true;
  return false;
}

bool RM_FileHandle::isValidFileHeader() const{
  if(!isValidFH())
    return false;
  if(header->recordSize <= 0 || header->numRecordsPerPage <= 0 || header->numPages <= 0)
    return false;
  if(header->bitmapOffset + header->bitmapSize + header->recordSize*header->numRecordsPerPage >
    PF_PAGE_SIZE)
    return false;
  return true;
}


//BITMAP Manipulations
RC RM_FileHandle::ResetBitmap(char *bitmap, int size){
  for(int i=0; i < size; i++)
    bitmap[i] = bitmap[i] ^ bitmap[i];
  return (0);
}

RC RM_FileHandle::SetBit(char *bitmap, int size, int bitnum){
  if (bitnum > size)
    return (RM_INVALIDBITOPERATION);
  int chunk = bitnum /8;
  int offset = bitnum - chunk*8;
  bitmap[chunk] |= 1 << offset;
  return (0);
}

RC RM_FileHandle::ResetBit(char *bitmap, int size, int bitnum){
if (bitnum > size)
    return (RM_INVALIDBITOPERATION);
  int chunk = bitnum / 8;
  int offset = bitnum - chunk*8;
  bitmap[chunk] &= ~(1 << offset);
  return (0);
}

int RM_FileHandle::NumBitsToCharSize(int size){
  int bitmapSize = size/8;
  if(bitmapSize * 8 < size) bitmapSize++;
  return bitmapSize;
}

RC RM_FileHandle::CheckBitSet(char *bitmap, int size, int bitnum, bool &set) const{
  if(bitnum > size)
    return (RM_INVALIDBITOPERATION);
  int chunk = bitnum / 8;
  int offset = bitnum - chunk*8;
  if ((bitmap[chunk] & (1 << offset)) != 0)
    set = true;
  else
    set = false;
  return (0);
}

RC RM_FileHandle::GetFirstZeroBit(char *bitmap, int size, int &location){
  for(int i = 0; i < size; i++){
    int chunk = i /8;
    int offset = i - chunk*8;
    if (bitmap[chunk] & ~(1 << offset)){
      location = i;
      return (0);
    }
  }
  return RM_PAGEFULL;
}

bool RM_FileHandle::IsPageFull(char *bitmap, int size){
  int char_size = NumBitsToCharSize(size);
  int mask = ~ 0;
  for(int i = 0; i < char_size; i++){
    if(~(bitmap[size] & mask) != 0)
      return false;
  }
  return true;
}

