//
// File:        rm_filescan.cc
// Description: RM_FileScan handles scans through a file
// Author:      Yifei Huang (yifei@stanford.edu)
//

#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "rm_internal.h"
#include <stdlib.h>


RM_FileScan::RM_FileScan(){
  openScan = false; // initially a filescan is not valid
  value = NULL;
  initializedValue = false;
  hasPagePinned = false;
  scanEnded = true;
}

RM_FileScan::~RM_FileScan(){
  if(scanEnded == false && hasPagePinned == true && openScan == true){
    fileHandle->pfh.UnpinPage(scanPage);
  }
  if (initializedValue == true){ // free any memory not freed
    free(value);
    initializedValue = false;
  }
}

/*
 * The following functions are comparison functions that return 0 if 
 * the two objects are equal, <0 if the first value is smaller, and
 * >0 if the second value is smaller.
 * They must take in an attribute type and attribute length, which 
 * determines the basis to compare the values on.
 */
bool equal(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 == *(float*)value2);
    case INT: return (*(int *)value1 == *(int *)value2) ;
    default:
      return (strncmp((char *) value1, (char *) value2, attrLength) == 0); 
  }
}

bool less_than(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 < *(float*)value2);
    case INT: return (*(int *)value1 < *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) < 0);
  }
}

bool greater_than(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 > *(float*)value2);
    case INT: return (*(int *)value1 > *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) > 0);
  }
}

bool less_than_or_eq_to(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 <= *(float*)value2);
    case INT: return (*(int *)value1 <= *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) <= 0);
  }
}

bool greater_than_or_eq_to(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 >= *(float*)value2);
    case INT: return (*(int *)value1 >= *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) >= 0);
  }
}

bool not_equal(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 != *(float*)value2);
    case INT: return (*(int *)value1 != *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) != 0);
  }
}

/*
 * Sets up the parameters in FileScan to associate it with certain
 * scan parameters. It is an error to reset the scan without closing
 * it first.
 */
RC RM_FileScan::OpenScan (const RM_FileHandle &fileHandle,
                  AttrType   attrType,
                  int        attrLength,
                  int        attrOffset,
                  CompOp     compOp,
                  void       *value,
                  ClientHint pinHint) {
  // If this is already associated with a scan, return immediately as an error
  if (openScan == true)
    return (RM_INVALIDSCAN);

  // Check that the fileHandle is valid
  if(fileHandle.isValidFileHeader())
    this->fileHandle = const_cast<RM_FileHandle*>(&fileHandle);
  else
    return (RM_INVALIDFILE);

  this->value = NULL;
  // Set the comparator to the appropriate function
  this->compOp = compOp;
  switch(compOp){
    case EQ_OP : comparator = &equal; break;
    case LT_OP : comparator = &less_than; break;
    case GT_OP : comparator = &greater_than; break;
    case LE_OP : comparator = &less_than_or_eq_to; break;
    case GE_OP : comparator = &greater_than_or_eq_to; break;
    case NE_OP : comparator = &not_equal; break;
    case NO_OP : comparator = NULL; break;
    default: return (RM_INVALIDSCAN);
  }

  int recSize = (this->fileHandle)->getRecordSize();
  // If there is a comparison, update the comparison parameters.
  if(this->compOp != NO_OP){
    // Check that the attribute offset and sizes are compatible with given
    // FileHandle
    if((attrOffset + attrLength) > recSize || attrOffset < 0 || attrOffset > MAXSTRINGLEN)
      return (RM_INVALIDSCAN);
    this->attrOffset = attrOffset;
    this->attrLength = attrLength;

    // Allocate the appropraite memory to store the value being compared
    if(attrType == FLOAT || attrType == INT){
      if(attrLength != 4)
        return (RM_INVALIDSCAN);
      this->value = (void *) malloc(4);
      memcpy(this->value, value, 4);
      initializedValue = true;
    }
    else if(attrType == STRING){
      this->value = (void *) malloc(attrLength);
      memcpy(this->value, value, attrLength);
      initializedValue = true;
    }
    else{
      return (RM_INVALIDSCAN);
    }
    this->attrType = attrType;
  }

  // open the scan
  openScan = true;
  scanEnded = false;

  // set up scan parameters:
  numRecOnPage = 0;
  numSeenOnPage = 0;
  useNextPage = true;
  scanPage = 0;
  scanSlot = BEGIN_SCAN;
  numSeenOnPage = 0;
  hasPagePinned = false;
  return (0);
} 

/*
 * Retrieves the number of records on a given page specified by an 
 * open PF_PageHandle. Returns that number in numRecords
 */
RC RM_FileScan::GetNumRecOnPage(PF_PageHandle &ph, int &numRecords){
  RC rc;
  char *bitmap;
  struct RM_PageHeader *pageheader;
  if((rc = (this->fileHandle)->GetPageDataAndBitmap(ph, bitmap, pageheader)))
      return (rc);
  numRecords = pageheader->numRecords;
  return (0);
}

/*
 * Retrieves the next record that satisfies the scan conditions
 */
RC RM_FileScan::GetNextRec(RM_Record &rec) {
  // If the scan has ended, or is not valid, return immediately
  if(scanEnded == true)
    return (RM_EOF);
  if(openScan == false)
    return (RM_INVALIDSCAN);
  hasPagePinned = true;
  
  RC rc;
  while(true){
    // Retrieve next record
    RM_Record temprec;
    if((rc=fileHandle->GetNextRecord(scanPage, scanSlot, temprec, currentPH, useNextPage))){
      if(rc == RM_EOF){
        hasPagePinned = false;
        scanEnded = true;
      }
      return (rc);
    }
    hasPagePinned = true;
    // If we retrieved a record on the next page, reset numRecOnPage to
    // reflect the number of records seen on this new current page
    if(useNextPage){
      GetNumRecOnPage(currentPH, numRecOnPage);
      useNextPage = false;
      numSeenOnPage = 0;
      if(numRecOnPage == 1)
        currentPH.GetPageNum(scanPage);
    }
    numSeenOnPage++; // update # of records seen on this page

    // If we've seen all the record on this page, then next time, we 
    // need to look on the next page, not this page, so unpin the page
    // and set the indicator (useNextPage)
    if(numRecOnPage == numSeenOnPage){
      useNextPage = true;
      //printf("unpin page in filescan\n");
      if(rc = fileHandle->pfh.UnpinPage(scanPage)){
        return (rc);
      }
      hasPagePinned = false;
    }
   
    // Retrieves the RID of the scan to update the progress of the scan
    RID rid;
    temprec.GetRid(rid);
    rid.GetPageNum(scanPage);
    rid.GetSlotNum(scanSlot);

    // Check to see if it satisfies the scan comparison, and if it does,
    // exit the function, returning the record.
    char *pData;
    if((rc = temprec.GetData(pData))){
      return (rc);
    }
    if(compOp != NO_OP){
      bool satisfies = (* comparator)(pData + attrOffset, this->value, attrType, attrLength);
      if(satisfies){
        rec = temprec;
        break;
      }
    }
    else{
      rec = temprec; // if no comparison, just return the record
      break;
    }
  }
  return (0);
}


/*
 * Closes a scan, freeing any memory that was allocated to store the value
 * being compared, and unpinning any pages that are still pinned
 * by the scan.
 */
RC RM_FileScan::CloseScan () {
  RC rc;
  if(openScan == false){
    return (RM_INVALIDSCAN);
  }
  if(hasPagePinned == true){
    //printf("unpinning page\n");
    if((rc = fileHandle->pfh.UnpinPage(scanPage)))
      return (rc);
  }
  if(initializedValue == true){
    free(this->value);
    initializedValue = false;
  }
  openScan = false;
  return (0);
}