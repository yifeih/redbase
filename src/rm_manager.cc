#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "rm_internal.h"
#include <cstdio>


RM_Manager::RM_Manager(PF_Manager &pfm) : pfm(pfm){
  // store the page manager
}

RM_Manager::~RM_Manager(){
}

RC RM_Manager::CreateFile (const char *fileName, int recordSize) { 
  RC rc = 0;
  if(recordSize <= 0 || recordSize > PF_PAGE_SIZE)
    return RM_BADRECORDSIZE;


  struct RM_FileHeader header;
  header.recordSize = recordSize;
  header.numRecordsPerPage = RM_FileHandle::CalcNumRecPerPage(recordSize);
  header.bitmapSize = RM_FileHandle::NumBitsToCharSize(header.numRecordsPerPage);
  header.bitmapOffset = sizeof(struct RM_PageHeader);

  header.numPages = 1;
  header.firstFreePage = NO_MORE_FREE_PAGES;

  printf("recSize %d, numRedPerPage: %d, bitmapSize: %d, bitmapOffset: %d\n",
    header.recordSize, header.numRecordsPerPage, header.bitmapSize, header.bitmapOffset);

  int numRecordsPerPage = (PF_PAGE_SIZE - (header.bitmapSize) - (header.bitmapOffset))/recordSize;
  if(numRecordsPerPage <= 0)
    return RM_BADRECORDSIZE;
  header.numRecordsPerPage = numRecordsPerPage;
  if((rc = pfm.CreateFile(fileName)))
    return (rc);

  PF_PageHandle ph;
  PF_FileHandle fh;
  if((rc = pfm.OpenFile(fileName, fh)))
    return (rc);

  PageNum page;
  if((rc = fh.AllocatePage(ph)) || (rc = ph.GetPageNum(page)))
    return (rc);


  char *pData;
  if((rc = ph.GetData(pData))){
    goto cleanup_and_exit;
  }

  memcpy(pData, &header, sizeof(struct RM_FileHeader));

  cleanup_and_exit:
  RC rc2;
  if((rc2 = fh.MarkDirty(page)) || (rc2 = fh.UnpinPage(page)) || (rc2 = pfm.CloseFile(fh)))
    return (rc2);

  // error: check size of record 
  // open file with PF_Manager
  // store record size/filename

  // Call SetUpFileHeader

  return (rc); 
}


RC RM_Manager::DestroyFile(const char *fileName) {
  RC rc;
  if((rc = pfm.DestroyFile(fileName)))
    return (rc);
  // call PF_manager destroy file
  return (0); 
}

RC RM_Manager::SetUpFH(RM_FileHandle& fileHandle, PF_FileHandle &fh, struct RM_FileHeader* header){
  //fileHandle.header = new struct RM_FileHeader();
  memcpy(&fileHandle.header, header, sizeof(struct RM_FileHeader));
  //fileHandle.pfh = new PF_FileHandle();
  //*fileHandle.pfh = *fh;
  fileHandle.pfh = fh;
  fileHandle.header_modified = false;
  fileHandle.openedFH = true;

  if(! fileHandle.isValidFileHeader()){
    printf("Invalid FH here \n");
    fileHandle.openedFH = false;
    return (RM_INVALIDFILE);
  }
  return (0);
}

RC RM_Manager::OpenFile   (const char *fileName, RM_FileHandle &fileHandle){
  if(fileHandle.openedFH == true)
    return (RM_INVALIDFILEHANDLE);

  RC rc;
  PF_FileHandle fh;

  if((rc = pfm.OpenFile(fileName, fh)))
    return (rc);

  PF_PageHandle ph;
  PageNum page;
  if((rc = fh.GetFirstPage(ph)) || (ph.GetPageNum(page))){
    fh.UnpinPage(page);
    pfm.CloseFile(fh);
    return (rc);
  }

  char *pData;
  ph.GetData(pData);

  struct RM_FileHeader * header = (struct RM_FileHeader *) pData;

  rc = SetUpFH(fileHandle, fh, header);

  RC rc2;
  if((rc2 = fh.UnpinPage(page)))
    return (rc2);
  if(rc != 0){
    pfm.CloseFile(fh);
  }

  // call pf_open file
  
    // ERROR - if already exists a PM_FileHandle in this FileHAndle
  // ERROR - if invalid PM_FileHandle
  // Retrieve Header and save
  // ERROR - if header doesnt exist, or first page doesnt exist
  // Set modified boolean
  // Save PM_FileHandler
  // Unpin page

  // Check file header credentials
  // ERROR - invalidFileCredentials

  // return file handle
  return (rc); 
}

RC RM_Manager::CleanUpFH(RM_FileHandle &fileHandle){
  //if(fileHandle.header == NULL || fileHandle.pfh == NULL)
  //  return RM_NULLFILEHANDLE;
  //delete fileHandle.header;
  //delete fileHandle.pfh;
  if(fileHandle.openedFH == false)
    return (RM_INVALIDFILEHANDLE);
  fileHandle.openedFH = false;
  return (0);
}

RC RM_Manager::CloseFile  (RM_FileHandle &fileHandle) {
  // CHECK FILE HEADER CREDENTIALS
  RC rc;

  if(fileHandle.header_modified == true){
    printf("reached modified \n");
    PF_PageHandle ph;
    PageNum page;
    if((rc = fileHandle.pfh.GetFirstPage(ph)) || ph.GetPageNum(page))
      return (rc);
    char *pData;
    if((rc = ph.GetData(pData))){
      RC rc2;
      if((rc2 = fileHandle.pfh.UnpinPage(page)))
        return (rc2);
      return (rc);
    }
    memcpy(pData, &fileHandle.header, sizeof(struct RM_FileHeader));
    if((rc = fileHandle.pfh.MarkDirty(page)) || (rc = fileHandle.pfh.UnpinPage(page)))
      return (rc);

  }
  printf("Closing file\n");

  if((rc = pfm.CloseFile(fileHandle.pfh)))
    return (rc);

  if((rc = CleanUpFH(fileHandle)))
    return (rc);
  // Check if modified
  // If so, get first page
  // write to it
  // mark as dirty
  // unpin
  // flush page to disk

  // set PM_FIleHAndle to NULL

  // call pf_close file
  return (0);
}
