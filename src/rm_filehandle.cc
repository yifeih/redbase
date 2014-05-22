//
// File:        rm_filehandle.cc
// Description: RM_FileHandle handles record manipulation on the page.
// Author:      Yifei Huang (yifei@stanford.edu)
//

#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "rm_internal.h"
#include <math.h>
#include <cstdio>

/*
 * Constructor for RM_FileHandle
 */
RM_FileHandle::RM_FileHandle(){
  // initially, it is not associated with an open file.
  header_modified = false; 
  openedFH = false;
}

/*
 * Destructor for RM_FileHandle
 */
RM_FileHandle::~RM_FileHandle(){
  openedFH = false; // disassociate from fileHandle from an open file
}

/*
 * Copying the RM_FileHandle object
 */
RM_FileHandle& RM_FileHandle::operator= (const RM_FileHandle &fileHandle){
  // sets all contents equal to another RM_FileHandle object
  if (this != &fileHandle){
    this->openedFH = fileHandle.openedFH;
    this->header_modified = fileHandle.header_modified;
    this->pfh = fileHandle.pfh;
    memcpy(&this->header, &fileHandle.header, sizeof(struct RM_FileHeader));
  }
  return (*this);
}


/*
 * AllocateNewPage returns a newly allocated page in ph, and its 
 * corresponding page number in page.
 * It sets up the page headers, and updates the file headers to
 * reflect this change in the file
 */
RC RM_FileHandle::AllocateNewPage(PF_PageHandle &ph, PageNum &page){
  RC rc;
  // allocate the page
  if((rc = pfh.AllocatePage(ph))){
    return (rc);
  }
  // get the page number of this allocated page
  if((rc = ph.GetPageNum(page)))
    return (rc);
 
  // create the page header
  char *bitmap;
  struct RM_PageHeader *pageheader;
  if((rc = GetPageDataAndBitmap(ph, bitmap, pageheader)))
    return (rc);
  pageheader->nextFreePage = header.firstFreePage;
  pageheader->numRecords = 0;
  if((rc = ResetBitmap(bitmap, header.numRecordsPerPage)))
    return (rc);

  header.numPages++; // update the file header to reflect addition of one
                     // more page
  // update the free pages linked list
  header.firstFreePage = page;
  return (0);
}

/*
 * Given a valid PF_PageHandle, it returns the pointer to its bitmap
 * in "bitmap" and the pointer to its page header in "pageheader"
 */
RC RM_FileHandle::GetPageDataAndBitmap(PF_PageHandle &ph, char *&bitmap, struct RM_PageHeader *&pageheader) const{
  RC rc;
  // retrieve header pointer
  char * pData;
  if((rc = ph.GetData(pData)))
    return (rc);
  pageheader = (struct RM_PageHeader *) pData;

  bitmap = pData + header.bitmapOffset; // retrieve bitmap pointer
  return (0);
}

/*
 * Given a valid RID, it returns the its page and slot numbers in "page"
 * and "slot"
 */
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

/*
 * Given a valid RID, it gets the record associated with it in "rec".
 * However, it first checks taht there is a valid record at that
 * location, and returns an error if there is not.
 */
RC RM_FileHandle::GetRec (const RID &rid, RM_Record &rec) const {
  // only proceed if this filehandle is associated with an open file
  if (!isValidFH())
    return (RM_INVALIDFILE);

  // Retrieves page and slot number from RID
  RC rc = 0;
  PageNum page;
  SlotNum slot;
  if((rc = GetPageNumAndSlot(rid, page, slot)))
    return (rc);

  // Retrieves the appropriate page, and its bitmap and pageheader 
  // contents
  PF_PageHandle ph;
  if((rc = pfh.GetThisPage(page, ph))){
    return (rc);
  }
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

  // always unpin the page before returning
  cleanup_and_exit:
  RC rc2;
  if((rc2 = pfh.UnpinPage(page)))
    return (rc2);
  return (rc); 
}

/*
 * Given some record data, it inserts the record into an available slot
 * in the file, and then returns it RID. If there is no available slot,
 * it creates a new page for it. pData cannot point to NULL.
 */
RC RM_FileHandle::InsertRec (const char *pData, RID &rid) {
  // only proceed if this filehandle is associated with an open file

  if (!isValidFH())
    return (RM_INVALIDFILE);

  RC rc = 0;

  if(pData == NULL) // invalid record input
    return RM_INVALIDRECORD;
  
  // If no more free pages, allocate it. Otherwise, free get the next
  // page with free slots in it
  PF_PageHandle ph;
  PageNum page;
  if (header.firstFreePage == NO_FREE_PAGES){
    AllocateNewPage(ph, page);
  }
  else{
    if((rc = pfh.GetThisPage(header.firstFreePage, ph)))
      return (rc);
    page = header.firstFreePage;
  }

  // retrieve the page's bitmap and header
  char *bitmap;
  struct RM_PageHeader *pageheader;
  int slot;
  if((rc = GetPageDataAndBitmap(ph, bitmap, pageheader)))
    goto cleanup_and_exit;

  // gets the first free slot on the page and set that bit to mark
  // it as occupied
  if((rc = GetFirstZeroBit(bitmap, header.numRecordsPerPage, slot)))
    goto cleanup_and_exit;
  if((rc = SetBit(bitmap, header.numRecordsPerPage, slot)))
    goto cleanup_and_exit;

  // copy the contents. update the header to reflect that one more
  // record has been added
  memcpy(bitmap + (header.bitmapSize) + slot*(header.recordSize),
    pData, header.recordSize);
  (pageheader->numRecords)++;
  rid = RID(page, slot); // set the RID to return the location of record

  // if page is full, update the free-page-list in the file header
  if(pageheader->numRecords == header.numRecordsPerPage){
    header.firstFreePage = pageheader->nextFreePage;
  }

  // always unpin the page before returning
  cleanup_and_exit:
  RC rc2;
  if((rc2 = pfh.MarkDirty(page) ) || (rc2 = pfh.UnpinPage(page)))
    return (rc2);

    
  return (rc); 
}

/*
 * Given a specific rid, delete that record from the file. This must
 * be a valid RID in the file that points to a record that actually
 * exists in the file.
 */
RC RM_FileHandle::DeleteRec (const RID &rid) {
  // only proceed if this filehandle is associated with an open file
  if (!isValidFH())
    return (RM_INVALIDFILE);
  RC rc = 0;

  // Retrieve page and slot number from the RID
  PageNum page;
  SlotNum slot;
  if((rc = GetPageNumAndSlot(rid, page, slot)))
    return (rc);

  // Get this page, its bitmap, and its header
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

  // Reset the bit to indicate a record deletion
  if((rc = ResetBit(bitmap, header.numRecordsPerPage, slot)))
    goto cleanup_and_exit;
  pageheader->numRecords--;
  // update the free list if this page went from full to not full
  if(pageheader->numRecords == header.numRecordsPerPage - 1){
    pageheader->nextFreePage = header.firstFreePage;
    header.firstFreePage = page;
 }

  // always unpin the page before returning
  cleanup_and_exit:
  RC rc2;
  if((rc2 = pfh.MarkDirty(page)) || (rc2 = pfh.UnpinPage(page)))
    return (rc2);

  return (rc); 
}

/*
 * Given a particular record, goes and updates the contents of that
 * record. This record must have a valid RID that is assocaited with a
 * record on the file.
 */
RC RM_FileHandle::UpdateRec (const RM_Record &rec) {
  // only proceed if this filehandle is associated with an open file
  if (!isValidFH())
    return (RM_INVALIDFILE);
  RC rc = 0;

  // retrieves the page and slot number of the record
  RID rid;
  if((rc = rec.GetRid(rid)))
    return (rc);
  PageNum page;
  SlotNum slot;
  if((rc = GetPageNumAndSlot(rid, page, slot)))
    return (rc);

  // gets the page, bitmap and pageheader for this page that holds the record
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
  
  // updates its contents
  char * recData;
  if((rc = rec.GetData(recData)))
    goto cleanup_and_exit;
  memcpy(bitmap + (header.bitmapSize) + slot*(header.recordSize),
    recData, header.recordSize);

  // always unpin the page before returning
  cleanup_and_exit:
  RC rc2;
  if((rc2 = pfh.MarkDirty(page)) || (rc2 = pfh.UnpinPage(page)))
    return (rc2);
  return (rc); 
}


/*
 * Forces a certain page to go to disk. If no page is specified, it
 * forces all pages to update to disk (from the buffer)
 */
RC RM_FileHandle::ForcePages(PageNum pageNum) {
  // only proceed if this filehandle is associated with an open file
  if (!isValidFH())
    return (RM_INVALIDFILE);
  pfh.ForcePages(pageNum);
  return (0); 
}

/*
 * Given a page and slot number, retrieves the next record that follows 
 * from this position, returning the record in rc, and the pagehandle
 * associated with it in ph. 
 * nextPage indicate whether we should look on
 * the current or next page for this. If nextPage is false, it assumes
 * that the pagehadle passed in refers to a valid page handle that is
 * pinned in buffer. 
 */
RC RM_FileHandle::GetNextRecord(PageNum page, SlotNum slot, RM_Record &rec, PF_PageHandle &ph, bool nextPage){
  RC rc = 0;
  char *bitmap;
  struct RM_PageHeader *pageheader;
  int nextRec;
  PageNum nextRecPage = page;
  SlotNum nextRecSlot;

  // If we are looking in the next page, keep running GetNextPage until 
  // we reach a page that has some records in it.
  if(nextPage){
    while(true){
      //printf("Getting next page\n");
      if((PF_EOF == pfh.GetNextPage(nextRecPage, ph)))
        return (RM_EOF); // reached the end of file

      // retrieve page and bitmap information
      if((rc = ph.GetPageNum(nextRecPage)))
        return (rc);
      if((rc = GetPageDataAndBitmap(ph, bitmap, pageheader)))
        return (rc);
      // search for the next record
      if(GetNextOneBit(bitmap, header.numRecordsPerPage, 0, nextRec) != RM_ENDOFPAGE)
        break;
      // if there are no records, on the page, unpin and get the next page
      //printf("Unpinning page\n");
      if((rc = pfh.UnpinPage(nextRecPage)))
        return (rc);
    }
  }
  else{
    // get the bitmap for this page, and the next record location
    if((rc = GetPageDataAndBitmap(ph, bitmap, pageheader)))
      return (rc);
    if(GetNextOneBit(bitmap, header.numRecordsPerPage, slot + 1, nextRec) == RM_ENDOFPAGE)
      return (RM_EOF);
  }
  // set the RID, and data contents of this record
  nextRecSlot = nextRec;
  RID rid(nextRecPage, nextRecSlot);
  if((rc = rec.SetRecord(rid, bitmap + (header.bitmapSize) + (nextRecSlot)*(header.recordSize), 
    header.recordSize)))
    return (rc);

  return (0);
}

/*
 * Returns true if this fileHandle is associated with an open file
 */
bool RM_FileHandle::isValidFH() const{
  if(openedFH == true)
    return true;
  return false;
}

/*
 * Returns true if this file header is a valid file header
 */
bool RM_FileHandle::isValidFileHeader() const{
  if(!isValidFH()){
    return false;
  }
  if(header.recordSize <= 0 || header.numRecordsPerPage <= 0 || header.numPages <= 0){
    return false;
  }
  if((header.bitmapOffset + header.bitmapSize + header.recordSize*header.numRecordsPerPage) >
    PF_PAGE_SIZE){
    return false;
  }
  return true;
}

/*
 * Wrapper around getting the record size of records in this file
 */
int RM_FileHandle::getRecordSize(){
  return this->header.recordSize;
}


//BITMAP Manipulations

/*
 * Given a pointer to a bitmap and its size in bits, sets all bits to 
 * zero
 */
RC RM_FileHandle::ResetBitmap(char *bitmap, int size){
  int char_num = NumBitsToCharSize(size);
  for(int i=0; i < char_num; i++)
    bitmap[i] = bitmap[i] ^ bitmap[i];
  return (0);
}

/*
 * Given a pointer to a bitmap, its size in bits, and a certain
 * bit number, set the specified bit number fo 1
 */
RC RM_FileHandle::SetBit(char *bitmap, int size, int bitnum){
  if (bitnum > size)
    return (RM_INVALIDBITOPERATION);
  int chunk = bitnum /8;
  int offset = bitnum - chunk*8;
  bitmap[chunk] |= 1 << offset;
  return (0);
}

/*
 * Given a pointer to a bitmap, its size in bits, and a certain
 * bit number, set the specified bit number fo 0
 */
RC RM_FileHandle::ResetBit(char *bitmap, int size, int bitnum){
if (bitnum > size)
    return (RM_INVALIDBITOPERATION);
  int chunk = bitnum / 8;
  int offset = bitnum - chunk*8;
  bitmap[chunk] &= ~(1 << offset);
  return (0);
}

/*
 * Given a pointer to a bitmap, its size in bits, and a certain
 * bit number, returns whether the bit number is set to 0 (set = false)
 * or set to 1 (set = true);
 */
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

/*
 * Given a pointer to a bitmap, and its size in bits, returns the location
 * of the first reset bit in "location". If there are no reset bits,
 * return an error.
 */
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

/*
 * Given a pointer to a bitmap, and its size in bits, and a position
 * to start returns the location of the first set bit after the
 * start position in "location". If there are no set bits, return an
 * error
 */
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

/*
 * Given a size in bits, convert it to the number of chars we must use
 * to be able to store all the bits
 */
int RM_FileHandle::NumBitsToCharSize(int size){
  int bitmapSize = size/8;
  if(bitmapSize * 8 < size) bitmapSize++;
  return bitmapSize;
}

/*
 * Calculates the number of records that can fit on a page given the
 * record size
 */
int RM_FileHandle::CalcNumRecPerPage(int recSize){
  return floor((PF_PAGE_SIZE * 1.0) / (1.0 * recSize + 1.0/8));
}
