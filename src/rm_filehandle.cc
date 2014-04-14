#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "rm_internal.h"
#include <math.h>
#include <cstdio>

RM_FileHandle::RM_FileHandle(){
  header_modified = false;
  openedFH = false;
}

RM_FileHandle::~RM_FileHandle(){
  /*
  if(pfh != NULL)
    delete pfh;
  if(header != NULL)
    delete header;
    */
  openedFH = false;
}

RM_FileHandle& RM_FileHandle::operator= (const RM_FileHandle &fileHandle){
  if (this != &fileHandle){
    this->openedFH = fileHandle.openedFH;
    this->header_modified = fileHandle.header_modified;
    this->pfh = fileHandle.pfh;
    memcpy(&this->header, &fileHandle.header, sizeof(struct RM_FileHeader));
  }
  return (*this);
}


// RETURNS PINNED PAGE
RC RM_FileHandle::AllocateNewPage(PF_PageHandle &ph, PageNum &page){
  RC rc;
  //PageNum page;
  if((rc = pfh.AllocatePage(ph))){
    return (rc);
  }
  header.numPages++;
  ph.GetPageNum(page);
 
  // Setup Header
  //struct RM_PageHeader pageHeader;
  //pageHeader.nextFreePage = header.firstFreePage;
  //pageHeader.numRecords = 0;

  // Copy header
  char *pageData;
  if((rc = ph.GetData(pageData))){
    return (rc);
  }
  struct RM_PageHeader *pageHeader = (struct RM_PageHeader *) pageData;
  pageHeader->nextFreePage = header.firstFreePage;
  pageHeader->numRecords = 0;
  //memcpy(pageData, &pageHeader, sizeof(pageHeader));

  // Setup Bitmap right after header
  char bitmap[header.bitmapSize];
  if((rc = ResetBitmap(bitmap, header.numRecordsPerPage)))
    return (rc);

  memcpy(pageData+header.bitmapOffset, bitmap, header.bitmapSize);
  header.firstFreePage = page;

  // TODO: write gotos
  //if((rc = pfh.UnpinPage(page)))
  //  return (rc);

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

  bitmap = pData + header.bitmapOffset;
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
  RC rc = 0;
  // Get RID of the record
  PageNum page;
  SlotNum slot;
  if((rc = GetPageNumAndSlot(rid, page, slot)))
    return (rc);

  // TODO: check if valid RID
  // maybe not?

  // Get this page, 
  PF_PageHandle ph;
  if((rc = pfh.GetThisPage(page, ph))){
    return (rc);
  }
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
    goto cleanup_and_exit;

  // Check if there really exists a record here according to the header
  bool recordExists;
  if ((rc = CheckBitSet(bitmap, header.numRecordsPerPage, slot, recordExists)))
    goto cleanup_and_exit;

  if(!recordExists){
    rc = RM_INVALIDRECORD;
    goto cleanup_and_exit;
  }

  // Set the record and return it
  if((rc = rec.SetRecord(rid, bitmap + (header.bitmapSize) + slot*(header.recordSize), 
    header.recordSize)))
    goto cleanup_and_exit;


  cleanup_and_exit:
  RC rc2;
  if((rc2 = pfh.UnpinPage(page)))
    return (rc2);
  return (rc); 
}

RC RM_FileHandle::InsertRec (const char *pData, RID &rid) {
  if (!isValidFH())
    return (RM_INVALIDFILE);
  // CHECK FILE HEADER CREDENTIALS
  RC rc = 0;
  
  if(pData == NULL)
    return RM_INVALIDRECORD;
  //printf("Num rec per page: %d \n", header.numRecordsPerPage);
  
  PF_PageHandle ph;
  PageNum page;
  if (header.firstFreePage == NO_FREE_PAGES){
    AllocateNewPage(ph, page);
    //printf("allocated new page\n");
    //ph.GetPageNum(page);
    //pfh.GetThisPage(page, ph);
  }
  else{
    if((rc = pfh.GetThisPage(header.firstFreePage, ph)))
      return (rc);
    page = header.firstFreePage;
  }

  char *bitmap;
  struct RM_PageHeader *pageheader;
  int slot;
  if((rc = GetPageDataAndBitmap(ph, bitmap, pageheader)))
    goto cleanup_and_exit;

  if((rc = GetFirstZeroBit(bitmap, header.numRecordsPerPage, slot)))
    goto cleanup_and_exit;
  if((rc = SetBit(bitmap, header.numRecordsPerPage, slot)))
    goto cleanup_and_exit;

  memcpy(bitmap + (header.bitmapSize) + slot*(header.recordSize),
    pData, header.recordSize);

  (pageheader->numRecords)++;
  //if(IsPageFull(data + (header->bitmapOffset), header->bitmapSize)){
  if(pageheader->numRecords == header.numRecordsPerPage){
    header.firstFreePage = pageheader->nextFreePage;
  }
  //printf("page: %d, slot: %d \n", page, slot);
  rid = RID(page, slot);
  //printBits(bitmap, header.numRecordsPerPage);

  cleanup_and_exit:
  RC rc2;
  if((rc2 = pfh.MarkDirty(page) ) || (rc2 = pfh.UnpinPage(page)))
    return (rc2);

  // check data is not null
  // Find first free
  // If no free page, insert free page
  // 
  return (rc); 
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
  if((rc = pfh.GetThisPage(page, ph)))
    return (rc);

  char *bitmap;
  struct RM_PageHeader *pageheader;
  if((rc = GetPageDataAndBitmap(ph, bitmap, pageheader)))
    goto cleanup_and_exit;

  // Check if there really exists a record here according to the header
  bool recordExists;
  if ((rc = CheckBitSet(bitmap, header.numRecordsPerPage, slot, recordExists)))
    goto cleanup_and_exit;
  if(!recordExists){
    rc = RM_INVALIDRECORD;
    goto cleanup_and_exit;
  }

  //printf("resetting bit: \n");
  if((rc = ResetBit(bitmap, header.numRecordsPerPage, slot)))
    goto cleanup_and_exit;
  pageheader->numRecords--;
  if(pageheader->numRecords == 0){
    if((rc = pfh.UnpinPage(page)) || (rc = pfh.DisposePage(page))){
      //printf("error here: %d \n", rc);
      header.numPages--;
      return (rc);
    }
    return (0);
  }
  /*
  if(pageheader->numRecords == header.numRecordsPerPage - 1){
    pageheader->nextFreePage = header.firstFreePage;
    header.firstFreePage = page;
  }
  */
  //printBits(bitmap, header.numRecordsPerPage);

  cleanup_and_exit:
  RC rc2;
  if((rc2 = pfh.MarkDirty(page)) || (rc2 = pfh.UnpinPage(page)))
    return (rc2);

  return (rc); 
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
  //printf("update rec page; %d, slot: %d \n", page, slot);
  //TestRec *pRecBufAgain;
  //rec.GetData((char *&)pRecBufAgain);
  //printf("recordFH: [%s, %d, %f] \n", pRecBufAgain->str, pRecBufAgain->num, pRecBufAgain->r);  

  

  char * buffer;
  // Get this page, 
  PF_PageHandle ph;
  if((rc = pfh.GetThisPage(page, ph)))
    return (rc);

  char *bitmap;
  struct RM_PageHeader *pageheader;
  if((rc = GetPageDataAndBitmap(ph, bitmap, pageheader)))
    goto cleanup_and_exit;

  // Check if there really exists a record here according to the header
  bool recordExists;
  if ((rc = CheckBitSet(bitmap, header.numRecordsPerPage, slot, recordExists)))
    goto cleanup_and_exit;
  if(!recordExists){
    rc = RM_INVALIDRECORD;
    goto cleanup_and_exit;
  }

  char * recData;
  if((rc = rec.GetData(recData)))
    goto cleanup_and_exit;
  memcpy(bitmap + (header.bitmapSize) + slot*(header.recordSize),
    recData, header.recordSize);

  //TestRec *pRecBufAgain;
  //pRecBufAgain = (TestRec *) (bitmap + (header.bitmapSize) + slot*(header.recordSize));
  //printf("update rec: [%s, %d, %f] \n", pRecBufAgain->str, pRecBufAgain->num, pRecBufAgain->r);  


  cleanup_and_exit:
  RC rc2;
  //printf("reaches cleanup\n");
  if((rc2 = pfh.MarkDirty(page)) || (rc2 = pfh.UnpinPage(page)))
    return (rc2);
  return (0); 
}


RC RM_FileHandle::ForcePages(PageNum pageNum) {
  // Check if valid page number
  // PF_FileHandle - push pages
  // Get this page, 
  if (!isValidFH())
    return (RM_INVALIDFILE);

  pfh.ForcePages(pageNum);

  return (0); 
}

/*
RC RM_FileHandle::UnpinPage(PageNum page){
  RC rc;
  if((rc =pfh.UnpinPage(page)))
    return (rc);
  return (0);
}*/

RC RM_FileHandle::GetNextRecord(PageNum page, SlotNum slot, RM_Record &rec, PF_PageHandle &ph, bool nextPage){
  RC rc = 0;
  char *bitmap;
  struct RM_PageHeader *pageheader;
  int nextRec;
  PageNum nextRecPage = page;
  SlotNum nextRecSlot;

  if(nextPage){
    //printf("use next page\n");
    if((PF_EOF == pfh.GetNextPage(page, ph)))
      return (RM_EOF);

    if((rc = ph.GetPageNum(nextRecPage)))
      return (rc);

    if((rc = GetPageDataAndBitmap(ph, bitmap, pageheader)))
      return (rc);

    GetNextOneBit(bitmap, header.numRecordsPerPage, 0, nextRec);
  }
  else{
    if((rc = GetPageDataAndBitmap(ph, bitmap, pageheader)))
      return (rc);
    if(GetNextOneBit(bitmap, header.numRecordsPerPage, slot + 1, nextRec) == RM_ENDOFPAGE)
      return (RM_EOF);
  }

  nextRecSlot = nextRec;
  RID rid(nextRecPage, nextRecSlot);
  //printf("next record page: %d, slot: %d \n", nextRecPage, nextRecSlot);
  //printf("Nextrec page: %d, slot %d \n", nextRecPage, nextRecSlot);
  //TestRec *pRecBufAgain;
  //pRecBufAgain = (TestRec *) (bitmap + (header.bitmapSize) + (nextRecSlot)*(header.recordSize));
  //printf("Nextrec rec: [%s, %d, %f] \n", pRecBufAgain->str, pRecBufAgain->num, pRecBufAgain->r);  


  // Set the record and return it
  if((rc = rec.SetRecord(rid, bitmap + (header.bitmapSize) + (nextRecSlot)*(header.recordSize), 
    header.recordSize)))
    return (rc);
  //printf("gets to end of FH GNR\n");
  return (0);
}

/*
RC RM_FileHandle::GetNextRecord(PageNum page, SlotNum slot, RM_Record &rec, PF_PageHandle &ph){
  RC rc = 0;
  char *bitmap;
  struct RM_PageHeader *pageheader;
  
  //if(page == 1 && slot == BEGIN_SCAN){
  //  if((rc = pfh.GetThisPage(page, ph)))
  //    return (rc);
  //}

  if((rc = GetPageDataAndBitmap(ph, bitmap, pageheader)))
      return (rc);

  int nextRec;
  PageNum nextRecPage = page;
  SlotNum nextRecSlot;
  if(slot == (header.numRecordsPerPage - 1) || 
    GetNextOneBit(bitmap, header.numRecordsPerPage, slot + 1, nextRec) == RM_ENDOFPAGE){
    if((rc = pfh.UnpinPage(page)))
      return (rc);

    if((PF_EOF == pfh.GetNextPage(page, ph)))
      return (RM_EOF);

    if((rc = ph.GetPageNum(nextRecPage)))
      return (rc);

    if((rc = GetPageDataAndBitmap(ph, bitmap, pageheader)))
      return (rc);

    GetNextOneBit(bitmap, header.numRecordsPerPage, 0, nextRec);
  }
  nextRecSlot = nextRec;
  RID rid(nextRecPage, nextRecSlot);
  //printf("Nextrec page: %d, slot %d \n", nextRecPage, nextRecSlot);
  //TestRec *pRecBufAgain;
  //pRecBufAgain = (TestRec *) (bitmap + (header.bitmapSize) + (nextRecSlot)*(header.recordSize));
  //printf("Nextrec rec: [%s, %d, %f] \n", pRecBufAgain->str, pRecBufAgain->num, pRecBufAgain->r);  


  // Set the record and return it
  if((rc = rec.SetRecord(rid, bitmap + (header.bitmapSize) + (nextRecSlot)*(header.recordSize), 
    header.recordSize)))
    return (rc);
  //printf("gets to end of FH GNR\n");
  return (0);
  
  return (0);
}*/


bool RM_FileHandle::isValidFH() const{
  if(openedFH == true)
    return true;
  return false;
}

bool RM_FileHandle::isValidFileHeader() const{
  if(!isValidFH()){
    printf("case1 \n");
    return false;
  }
  if(header.recordSize <= 0 || header.numRecordsPerPage <= 0 || header.numPages <= 0){
    printf("case 2\n");
    return false;
  }
  if((header.bitmapOffset + header.bitmapSize + header.recordSize*header.numRecordsPerPage) >
    PF_PAGE_SIZE){
    printf("case 3\n");
    return false;
  }
  return true;
}

int RM_FileHandle::getRecordSize(){
  return this->header.recordSize;
}


//BITMAP Manipulations
RC RM_FileHandle::ResetBitmap(char *bitmap, int size){
  int char_num = NumBitsToCharSize(size);
  for(int i=0; i < char_num; i++)
    bitmap[i] = bitmap[i] ^ bitmap[i];
  return (0);
}

RC RM_FileHandle::SetBit(char *bitmap, int size, int bitnum){
  if (bitnum > size)
    return (RM_INVALIDBITOPERATION);
  int chunk = bitnum /8;
  int offset = bitnum - chunk*8;
  bitmap[chunk] |= 1 << offset;
  //printf("bitmap now: %i", bitmap[chunk]);
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
    if ((bitmap[chunk] & (1 << offset)) == 0){
      location = i;
      return (0);
    }
  }
  return RM_PAGEFULL;
}

RC RM_FileHandle::GetNextOneBit(char *bitmap, int size, int start, int &location){
  for(int i = start; i < size; i++){
    int chunk = i /8;
    int offset = i - chunk*8;
    if (((bitmap[chunk] & (1 << offset)) != 0)){
      location = i;
      return (0);
    }
  }
  return RM_ENDOFPAGE;
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

int RM_FileHandle::CalcNumRecPerPage(int recSize){
  return floor((PF_PAGE_SIZE * 1.0) / (1.0 * recSize + 1.0/8));
}

void RM_FileHandle::printBits(char* bitmap, int size)
{
   for(int bit=0;bit<size; bit++)
   {
      int chunk = bit / 8;
      int offset = bit % 8;
      if((bitmap[chunk] & (1 << offset)) > 0)
        printf("1");
      else
        printf("0");
      //printf("%i", bitmap[chunk] & (1 << offset));
   }
   printf("\n");
}
