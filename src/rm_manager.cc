//
// File:        rm_manager.cc
// Description: RM_Manager handles file operations.
// Author:      Yifei Huang (yifei@stanford.edu)
//
#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "rm_internal.h"


RM_Manager::RM_Manager(PF_Manager &pfm) : pfm(pfm){
}

RM_Manager::~RM_Manager(){
}

/*
 * Given a filename and a record size, creates a file. RecordSize
 * must be a valid record size, and fileName cannot be a file that
 * already exists. 
 * This function creates the file headers associated with this file
 */
RC RM_Manager::CreateFile (const char *fileName, int recordSize) { 
  RC rc = 0;
  if(fileName == NULL)
    return (RM_BADFILENAME);
  // basic checks on record size
  if(recordSize <= 0 || recordSize > PF_PAGE_SIZE)
    return RM_BADRECORDSIZE;

  int numRecordsPerPage = RM_FileHandle::CalcNumRecPerPage(recordSize);
  int bitmapSize = RM_FileHandle::NumBitsToCharSize(numRecordsPerPage);
  int bitmapOffset = sizeof(struct RM_PageHeader);

  if( (PF_PAGE_SIZE - bitmapSize - bitmapOffset)/recordSize <= 0)
    return RM_BADRECORDSIZE;

  // Sets up the file header
  /*
  struct RM_FileHeader header;
  header.recordSize = recordSize;
  header.numRecordsPerPage = RM_FileHandle::CalcNumRecPerPage(recordSize);
  header.bitmapSize = RM_FileHandle::NumBitsToCharSize(header.numRecordsPerPage);
  header.bitmapOffset = sizeof(struct RM_PageHeader);
  header.numPages = 1;
  header.firstFreePage = NO_MORE_FREE_PAGES;

  // More stringent checks on record size
  int numRecordsPerPage = (PF_PAGE_SIZE - (header.bitmapSize) - (header.bitmapOffset))/recordSize;
  if(numRecordsPerPage <= 0)
    return RM_BADRECORDSIZE;
  header.numRecordsPerPage = numRecordsPerPage;
  */

  // Creates the file
  if((rc = pfm.CreateFile(fileName)))
    return (rc);

  // Opens the file, creates a new page and copies the header into it
  PF_PageHandle ph;
  PF_FileHandle fh;
  struct RM_FileHeader *header;
  if((rc = pfm.OpenFile(fileName, fh)))
    return (rc);
  PageNum page;
  if((rc = fh.AllocatePage(ph)) || (rc = ph.GetPageNum(page)))
    return (rc);
  char *pData;
  if((rc = ph.GetData(pData))){
    goto cleanup_and_exit;
  }
  header = (struct RM_FileHeader *) pData;
  header->recordSize = recordSize;
  header->numRecordsPerPage = (PF_PAGE_SIZE - bitmapSize - bitmapOffset)/recordSize;
  header->bitmapSize = bitmapSize;
  header->bitmapOffset = bitmapOffset;
  header->numPages = 1;
  header->firstFreePage = NO_MORE_FREE_PAGES;
  //memcpy(pData, &header, sizeof(struct RM_FileHeader));

  // always unpin the page, and close the file before exiting
  cleanup_and_exit:
  RC rc2;
  if((rc2 = fh.MarkDirty(page)) || (rc2 = fh.UnpinPage(page)) || (rc2 = pfm.CloseFile(fh)))
    return (rc2);
  return (rc); 
}

/*
 * Destroys a file
 */
RC RM_Manager::DestroyFile(const char *fileName) {
  if(fileName == NULL)
    return (RM_BADFILENAME);
  RC rc;
  if((rc = pfm.DestroyFile(fileName)))
    return (rc);
  return (0); 
}

/*
 * Sets up the private variables of RM_FileHandle when opening up a file.
 * The function should be given a valid PF_FileHandle and pointer to 
 * header or else an error is thrown
 */
RC RM_Manager::SetUpFH(RM_FileHandle& fileHandle, PF_FileHandle &fh, struct RM_FileHeader* header){
  // set up the private variables
  memcpy(&fileHandle.header, header, sizeof(struct RM_FileHeader));
  fileHandle.pfh = fh;
  fileHandle.header_modified = false;
  fileHandle.openedFH = true;

  // confirms that the header is valid
  if(! fileHandle.isValidFileHeader()){
    fileHandle.openedFH = false;
    return (RM_INVALIDFILE);
  }
  return (0);
}

/*
 * Given a file name and a valid RM_FileHandle, opens up the file. The
 * given FileHandle must not already be associated with another open file
 */
RC RM_Manager::OpenFile   (const char *fileName, RM_FileHandle &fileHandle){
  if(fileName == NULL)
    return (RM_BADFILENAME);
  // if the filehandle is associated with another open file. exit immediately
  if(fileHandle.openedFH == true)
    return (RM_INVALIDFILEHANDLE);

  RC rc;
  // Open the file
  PF_FileHandle fh;
  if((rc = pfm.OpenFile(fileName, fh)))
    return (rc);

  // Gets the first page, and uses it to set up the header in fileHandle
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

  // set up and validate the header of this file
  rc = SetUpFH(fileHandle, fh, header);

  // Unpin the header page
  RC rc2;
  if((rc2 = fh.UnpinPage(page)))
    return (rc2);

  // If any errors occured, close the file!
  if(rc != 0){
    pfm.CloseFile(fh);
  }
  return (rc); 
}

/*
 * This helper function manipulates the private variables of
 * RM_FileHandle associated with an open file to reflect the closing
 * of the file. If the FileHandle is already closed, throw an error.
 */
RC RM_Manager::CleanUpFH(RM_FileHandle &fileHandle){
  if(fileHandle.openedFH == false)
    return (RM_INVALIDFILEHANDLE);
  fileHandle.openedFH = false;
  return (0);
}

/*
 * This closes the a file specified by RM_FileHAndle. IT is an error to
 * close a RM_FileHandle that is already closed, or was never opened.
 * If the header of RM_FileHandle was changed, we need to update its
 * contents
 */
RC RM_Manager::CloseFile  (RM_FileHandle &fileHandle) {
  RC rc;
  PF_PageHandle ph;
  PageNum page;
  char *pData;

  // If header was modified, put the first page into buffer again,
  // and update its contents, marking the page as dirty
  if(fileHandle.header_modified == true){
    if((rc = fileHandle.pfh.GetFirstPage(ph)) || ph.GetPageNum(page))
      return (rc);
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

  // Close the file
  if((rc = pfm.CloseFile(fileHandle.pfh)))
    return (rc);

  // Disassociate the fileHandle from an open file
  if((rc = CleanUpFH(fileHandle)))
    return (rc);

  return (0);
}
