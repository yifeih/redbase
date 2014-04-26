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
#include <cstdio>

/*
 * The following functions are comparison functions that return 0 if 
 * the two objects are equal, <0 if the first value is smaller, and
 * >0 if the second value is smaller.
 * They must take in an attribute type and attribute length, which 
 * determines the basis to compare the values on.
 */
 
bool iequal(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 == *(float*)value2);
    case INT: return (*(int *)value1 == *(int *)value2) ;
    default:
      return (strncmp((char *) value1, (char *) value2, attrLength) == 0); 
  }
}

bool iless_than(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 < *(float*)value2);
    case INT: return (*(int *)value1 < *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) < 0);
  }
}

bool igreater_than(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 > *(float*)value2);
    case INT: return (*(int *)value1 > *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) > 0);
  }
}

bool iless_than_or_eq_to(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 <= *(float*)value2);
    case INT: return (*(int *)value1 <= *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) <= 0);
  }
}

bool igreater_than_or_eq_to(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 >= *(float*)value2);
    case INT: return (*(int *)value1 >= *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) >= 0);
  }
}

bool inot_equal(void * value1, void * value2, AttrType attrtype, int attrLength){
  switch(attrtype){
    case FLOAT: return (*(float *)value1 != *(float*)value2);
    case INT: return (*(int *)value1 != *(int *)value2) ;
    default: 
      return (strncmp((char *) value1, (char *) value2, attrLength) != 0);
  }
}


IX_IndexScan::IX_IndexScan(){
  openScan = false;
  value = NULL;
  initializedValue = false;
  hasBucketPinned = false;
  hasLeafPinned = false;
  scanEnded = true;
  scanStarted = false;
}

IX_IndexScan::~IX_IndexScan(){
  if(scanEnded == false && hasBucketPinned == true)
    indexHandle->pfh.UnpinPage(currBucketNum);
  if(scanEnded == false && hasLeafPinned == true &&(currLeafNum != (indexHandle->header).rootPage))
    indexHandle->pfh.UnpinPage(currLeafNum);
  if(initializedValue == true){
    free(value);
    initializedValue = false;
  }

}


RC IX_IndexScan::OpenScan(const IX_IndexHandle &indexHandle,
                CompOp compOp,
                void *value,
                ClientHint  pinHint){
  RC rc = 0;

  if(openScan == true || compOp == NE_OP)
    return (IX_INVALIDSCAN);

  if(indexHandle.isValidIndexHeader())
    this->indexHandle = const_cast<IX_IndexHandle*>(&indexHandle);
  else
    return (IX_INVALIDSCAN);

  this->value = NULL;
  this->compOp = compOp;
  switch(compOp){
    case EQ_OP : comparator = &iequal; break;
    case LT_OP : comparator = &iless_than; break;
    case GT_OP : comparator = &igreater_than; break;
    case LE_OP : comparator = &iless_than_or_eq_to; break;
    case GE_OP : comparator = &igreater_than_or_eq_to; break;
    case NO_OP : comparator = NULL; break;
    default: return (IX_INVALIDSCAN);
  }

  if(compOp != NO_OP){
  attrLength = ((this->indexHandle)->header).attr_length;
  this->value = (void *) malloc(attrLength);
  memcpy(this->value, value, attrLength);
  initializedValue = true;
  }

  openScan = true;
  scanEnded = false;
  hasBucketPinned = false;
  hasLeafPinned = false;
  scanStarted = false;

  return (rc);
}

RC IX_IndexScan::GetNextEntry(RID &rid){
  RC rc = 0;
  //printf("entered get next entry\n");
  if(scanEnded == true && openScan == true)
    return (IX_EOF);

  if(scanEnded == true || openScan == false)
    return (IX_INVALIDSCAN);

  bool notFound = true;
  // compare
  while(notFound){
  //for(int i= 0; i < 11; i++){
    //printf("while loop \n");
    if(scanEnded == false && openScan == true && scanStarted == false){
      if((rc = indexHandle->GetFirstLeafPage(currLeafPH, currLeafNum)))
        return (rc);
      if((rc = GetFirstEntryInLeaf(currLeafPH))){
        if(rc == IX_EOF){
          scanEnded = true;
        }
        //printf("returning end of file from scan\n");
        return (rc);
      }
      scanStarted = true;
    }
    //printf("leaf slot found: %d \n", leafSlot);

    if(hasBucketPinned){
      RID rid1(bucketEntries[bucketSlot].page, bucketEntries[bucketSlot].slot);
      currRID = rid1;
      //printf("rid: %d %d \n", bucketEntries[bucketSlot].page, bucketEntries[bucketSlot].slot);
    }
    else if(hasLeafPinned){
      RID rid1(leafEntries[leafSlot].page, leafEntries[leafSlot].slot);
      currRID = rid1;
      //printf("rid: %d %d \n", leafEntries[leafSlot].page, leafEntries[leafSlot].slot);
    }


    if(compOp == NO_OP){
      rid = currRID;
      //break;
      notFound = false;
    }
    else if((comparator((void *)currKey, value, attrType, attrLength))){
      //printf("finished comaring, and they equal\n");
      //printf("returned: %d \n", *(int*)currKey);
      rid = currRID;
      //break;
      notFound = false;
    }

    printf("updating scan\n");
    if((IX_EOF == FindNextValue())){
      scanEnded = true;

      if(notFound == true){
        //printf("ending scan with error code\n");
        rc = IX_EOF;
      }
      notFound = false;
      //printf("scan ended \n");
      //return (IX_EOF);
    }


  }
  //printf("rid: %d %d \n", bucketEntries[bucketSlot].page, bucketEntries[bucketSlot].slot);
  //printf("returning from scan\n");
  return (rc);
}

RC IX_IndexScan::FindNextValue(){
  RC rc = 0;
  if(hasBucketPinned){
    //printf("bucket is pinned \n");
    int prevSlot = bucketSlot;
    bucketSlot = bucketEntries[prevSlot].nextSlot;
    if(bucketSlot != NO_MORE_SLOTS){
      //printf("no more buckets\n");
      return (0); // found next bucket slot
    }
    // otherwise, go to next bucket
    PageNum nextBucket = bucketHeader->nextBucket;
    printf("*** unpinning bucket number %d \n", currBucketNum);
    if((rc = (indexHandle->pfh).UnpinPage(currBucketNum) ))
      return (rc);
    
    hasBucketPinned = false;

    if(nextBucket != NO_MORE_PAGES){
      printf("*** pinning bucket number %d \n", nextBucket);
      if((rc = GetFirstBucketEntry(nextBucket, currBucketPH) ))
        return (rc);
      currBucketNum = nextBucket;
      return (0);
    }
  }
  // otherwise, deal with node level
  //printf("go to leaf level\n");
  int prevLeafSlot = leafSlot;
  leafSlot = leafEntries[prevLeafSlot].nextSlot;

  if(leafSlot != NO_MORE_SLOTS && leafEntries[leafSlot].isValid == OCCUPIED_DUP){
    currKey = leafKeys + leafSlot * attrLength;
    currBucketNum = leafEntries[leafSlot].page;
    printf("***pinning bucket number %d \n", currBucketNum);
    
    if((rc = GetFirstBucketEntry(currBucketNum, currBucketPH) ))
      return (rc);
    return (0);
  }
  if(leafSlot != NO_MORE_SLOTS && leafEntries[leafSlot].isValid == OCCUPIED_NEW){
    //printf("hellow found right next value \n");
    currKey = leafKeys + leafSlot * attrLength;
    return (0);
  }

  // otherwise, get the next page
  PageNum nextLeafPage = leafHeader->nextPage;

  // if it's not the root page, unpin it:
  if((currLeafNum != (indexHandle->header).rootPage)){
    if((rc = (indexHandle->pfh).UnpinPage(currLeafNum))){
      //printf("BAD\n");
      return (rc);
    }
  }
  hasLeafPinned = false;

  if(nextLeafPage != NO_MORE_PAGES){
    currLeafNum = nextLeafPage;
    if((rc = (indexHandle->pfh).GetThisPage(currLeafNum, currLeafPH)))
      return (rc);
    if((rc = GetFirstEntryInLeaf(currLeafPH) ))
      return (rc);
    return (0);
  }

  return (IX_EOF);

}

RC IX_IndexScan::GetFirstEntryInLeaf(PF_PageHandle &leafPH){
  //printf("finding first entry\n");
  RC rc = 0;
  hasLeafPinned = true;
  if((rc = leafPH.GetData((char *&) leafHeader)))
    return (rc);

  if(leafHeader->num_keys == 0)
    return (IX_EOF);

  leafEntries = (struct Node_Entry *)((char *)leafHeader + (indexHandle->header).entryOffset_N);
  leafKeys = (char *)leafHeader + (indexHandle->header).keysOffset_N;

  leafSlot = leafHeader->firstSlotIndex;
  if((leafSlot != NO_MORE_SLOTS)){
    //printf("update key %d \n", *(int*)currKey);
    currKey = leafKeys + attrLength*leafSlot;
    //printf("attribute length: %d \n", attrLength);
    //printf("current slot: %d, current value: %d \n", leafSlot, *(int*)currKey);
  }
  else
    return (IX_INVALIDSCAN);
  //printf("reached here\n");
  if(leafEntries[leafSlot].isValid == OCCUPIED_DUP){
    //isBucketPinned = true;
    currBucketNum = leafEntries[leafSlot].page;
    if((rc = (indexHandle->pfh).GetThisPage(currBucketNum, currBucketPH)))
      return (rc);
    if((rc = GetFirstBucketEntry(currBucketNum, currBucketPH)))
      return (rc);
    //printf("shouldnt be here\n");
  }
  //printf("hi\n");
  return (0);
}

RC IX_IndexScan::GetFirstBucketEntry(PageNum nextBucket, PF_PageHandle &bucketPH){
  RC rc = 0;
  if((rc = (indexHandle->pfh).GetThisPage(nextBucket, currBucketPH)))
        return (rc);
  hasBucketPinned = true;
  if((rc = bucketPH.GetData((char *&) bucketHeader)))
    return (rc);

  bucketEntries = (struct Bucket_Entry *) ((char *)bucketHeader + (indexHandle->header).entryOffset_B);
  bucketSlot = bucketHeader->firstSlotIndex;

  return (0);

}

RC IX_IndexScan::CloseScan(){
  RC rc = 0;
  if(openScan == false)
    return (IX_INVALIDSCAN);
  if(scanEnded == false && hasBucketPinned == true)
    indexHandle->pfh.UnpinPage(currBucketNum);
  if(scanEnded == false && hasLeafPinned == true && (currLeafNum != (indexHandle->header).rootPage))
    indexHandle->pfh.UnpinPage(currLeafNum);
  if(initializedValue == true){
    free(value);
    initializedValue = false;
  }
  openScan = false;
  scanStarted = false;

  return (rc);
}
