//
// File:        ix_indexhandle.cc
// Description: IX_IndexHandle handles manipulations within the index
// Author:      Yifei Huang (yifei@stanford.edu)
//

#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "ix_internal.h"
#include <math.h>
#include "comparators.h"
#include <cstdio>


IX_IndexHandle::IX_IndexHandle(){
  isOpenHandle = false;
  header_modified = false;
}

IX_IndexHandle::~IX_IndexHandle(){

}

RC IX_IndexHandle::PrintBucketEntries(){
  printf("PRINTING BUCKETS RAWR: \n");
  RC rc = 0;
  PageNum bucketNum = header.firstBucket;
  printf("bucket page number: %d \n", bucketNum);
  
  while(bucketNum != NO_MORE_PAGES){
    PF_PageHandle bucketPH;
    struct IX_BucketHeader *bHeader;
    if((rc = pfh.GetThisPage(bucketNum, bucketPH)) || (rc = bucketPH.GetData((char *&)bHeader)))
      return (rc);
    struct Bucket_Entry *entries = (struct Bucket_Entry *)((char *)bHeader + header.entryOffset_B);
    char *keys = (char *)bHeader + header.keysOffset_B;
    int prev_idx = BEGINNING_OF_SLOTS;
    int curr_idx = bHeader->firstSlotIndex;
    while(curr_idx != NO_MORE_SLOTS){
      printf("Key: %d, RID: %d, %d , slot: %d, ", *(int *)(keys + curr_idx * header.attr_length), 
        entries[curr_idx].page, entries[curr_idx].slot, curr_idx);
      if(entries[curr_idx].isValid == OCCUPIED_NEW)
        printf(" new \n");
      else
        printf(" repeat \n");
      prev_idx = curr_idx;
      curr_idx = entries[prev_idx].nextSlot;
    }
    int freeSlot = bHeader->freeSlotIndex;
    while(freeSlot != NO_MORE_SLOTS){
      printf("%d, ", freeSlot);
      freeSlot = entries[freeSlot].nextSlot;
    }

    pfh.UnpinPage(bucketNum);
    bucketNum = bHeader->nextBucket;
    printf("Going to next bucket %d \n", bucketNum);
  }

  return (rc);
}

RC IX_IndexHandle::PrintRootNode(){
  RC rc =0;
  struct IX_NodeHeader *nHeader;
  if((rc = rootPH.GetData((char *&)nHeader)))
    return (rc);
  char *keys = (char *)nHeader + header.keysOffset_N;
  struct Node_Entry *entries = (struct Node_Entry *)((char *)nHeader + header.entryOffset_N);
  int prev_idx = BEGINNING_OF_SLOTS;
  int curr_idx = nHeader->firstSlotIndex;
  while(curr_idx != NO_MORE_SLOTS){
    printf("Key: %d, BucketPage: %d \n", *(int*) (keys + curr_idx * header.attr_length), entries[curr_idx].pageNum);
    prev_idx = curr_idx;
    curr_idx = entries[prev_idx].nextSlot;
  }
  return (rc);
}

RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid){
  RC rc = 0;
  struct IX_NodeHeader *rHeader;
  if((rc = rootPH.GetData((char *&)rHeader)))
    return (rc);

  if(rHeader->num_keys == header.maxKeys_N){
    PageNum newRootPage;
    char *newRootData;
    PF_PageHandle newRootPH;
    if((rc = CreateNewNode(newRootPH, newRootPage, newRootData, false)))
      return (rc);
    struct IX_NodeHeader *newRootHeader = (struct IX_NodeHeader *)newRootData;
    struct Node_Entry *newRootEntries = (struct Node_Entry *)(newRootData + header.entryOffset_N);
    char * newRootKeys = newRootData + header.keysOffset_N;
    memcpy(newRootKeys, (char *)rHeader + header.keysOffset_N, header.attr_length);
    int location = newRootHeader->freeSlotIndex;
    newRootHeader->freeSlotIndex = newRootEntries[location].nextSlot;
    newRootEntries[location].isValid = OCCUPIED_NEW;
    newRootEntries[location].nextSlot = newRootHeader->firstSlotIndex;
    newRootEntries[location].pageNum = header.rootPage;
    newRootHeader->isEmpty = false;
    newRootHeader->num_keys++;
    newRootHeader->firstSlotIndex = location;

    if((rc = SplitNode(newRootData, rootPH, 0)))
      return (rc);
  }
  else{
    //printf("reached insert entry\n");
    if((rc = InsertIntoNonFullNode((char *&)rHeader,pData, rid)))
      return (rc);
  }

  return (rc);
}

RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid){
  RC rc = 0;
  return (rc);
}

// Returns the page handle with the header set up
RC IX_IndexHandle::CreateNewNode(PF_PageHandle &ph, PageNum &page, char *&nData, bool isLeaf){
  RC rc = 0;
  if((rc = pfh.AllocatePage(ph)) || (rc = ph.GetPageNum(page))){
    return (rc);
  }
  if((rc = ph.GetData(nData)))
    return (rc);
  struct IX_NodeHeader *nHeader = (struct IX_NodeHeader *)nData;

  nHeader->isLeafNode = isLeaf;
  nHeader->isEmpty = true;
  nHeader->num_keys = 0;
  nHeader->invalid1 = NO_MORE_PAGES;
  nHeader->firstSlotIndex = NO_MORE_SLOTS;
  nHeader->freeSlotIndex = 0;

  struct Node_Entry *entries = (struct Node_Entry *)((char*)nHeader + header.entryOffset_N);

  for(int i=0; i < header.maxKeys_N; i++){
    entries[i].isValid = UNOCCUPIED;
    entries[i].pageNum = NO_MORE_PAGES;
    if(i == (header.maxKeys_N -1))
      entries[i].nextSlot = NO_MORE_SLOTS;
    else
      entries[i].nextSlot = i+1;
  }
  printf("NODE CREATION: entries[0].nextSlot: %d \n", entries[0].nextSlot);

  return (rc);
}

RC IX_IndexHandle::CreateNewBucket(PageNum &page){
  char *nData;
  PF_PageHandle ph;
  RC rc = 0;
  if((rc = pfh.AllocatePage(ph)) || (rc = ph.GetPageNum(page))){
    return (rc);
  }
  if((rc = ph.GetData(nData))){
    RC rc2;
    if((rc2 = pfh.UnpinPage(page)))
      return (rc2);
    return (rc);
  }
  if(header.firstBucketCreated == false){
    header.firstBucketCreated = true;
    header.firstBucket = page;
    header_modified = true;
  }
  struct IX_BucketHeader *bHeader = (struct IX_BucketHeader *) nData;

  bHeader->num_keys = 0;
  bHeader->firstSlotIndex = NO_MORE_SLOTS;
  bHeader->freeSlotIndex = 0;
  bHeader->nextBucket = NO_MORE_PAGES;
  bHeader->prevBucket = NO_MORE_PAGES;

  struct Bucket_Entry *entries = (struct Bucket_Entry *)((char *)bHeader + header.entryOffset_B);

  for(int i=0; i < header.maxKeys_B; i++){
    entries[i].isValid = UNOCCUPIED;
    entries[i].page = NO_MORE_PAGES;
    if(i == (header.maxKeys_B -1))
      entries[i].nextSlot = NO_MORE_SLOTS;
    else
      entries[i].nextSlot = i+1;
  }
  printf("NEW BUCKET PAGE %d \n", page);
  if( (rc =pfh.MarkDirty(page)) || (rc = pfh.UnpinPage(page)))
    return (rc);

  return (rc);
}

RC IX_IndexHandle::ForcePages(){
  RC rc = 0;
  return (rc);
}

RC IX_IndexHandle::SplitNode(char *pData, PF_PageHandle& oldPH, int index){
  RC rc = 0;
  struct IX_NodeHeader *oldHeader;
  PageNum oldPage;
  if((rc = oldPH.GetData((char *&)oldHeader)) || oldPH.GetPageNum(oldPage))
    return (rc);

  PF_PageHandle newPH;
  struct IX_NodeHeader *newHeader;
  PageNum newPage;
  if((rc = CreateNewNode(newPH, newPage, (char *&)newHeader, oldHeader->isLeafNode)))
    return (rc);

  struct Node_Entry *nEntries = (struct Node_Entry *)((char *) newHeader + header.entryOffset_N);
  struct Node_Entry *oEntries = (struct Node_Entry *)((char *) oldHeader + header.entryOffset_N);
  char *nKeys = (char *)newHeader + header.keysOffset_N;
  char *oKeys = (char *)oldHeader + header.keysOffset_N;

  struct IX_NodeHeader *pHeader = (struct IX_NodeHeader *) pData;
  struct Node_Entry *pEntries = (struct Node_Entry *) (pData + header.entryOffset_N);
  char *pKeys = pData + header.keysOffset_N;

  // set first parent value
  int prev_idx1 = BEGINNING_OF_SLOTS;
  int curr_idx1 = oldHeader->firstSlotIndex;

  for(int i=0; i < (header.maxKeys_N/2); i ++){
    prev_idx1 = curr_idx1;
    curr_idx1 = oEntries[prev_idx1].nextSlot;
  }
  oEntries[prev_idx1].nextSlot = NO_MORE_SLOTS;


  int plocation = pHeader->freeSlotIndex;
  pHeader->freeSlotIndex = pEntries[plocation].nextSlot;
  pEntries[plocation].nextSlot = pEntries[index].nextSlot;
  pEntries[index].nextSlot = plocation;

  pEntries[plocation].isValid = OCCUPIED_NEW;
  pEntries[plocation].pageNum = oEntries[curr_idx1].pageNum;
  memcpy(pKeys + plocation*header.attr_length, oKeys + curr_idx1*header.attr_length, header.attr_length);

  int prev_idx2 = BEGINNING_OF_SLOTS;
  int curr_idx2 = newHeader->freeSlotIndex;
  while(curr_idx1 != NO_MORE_SLOTS){
    nEntries[curr_idx2].pageNum = oEntries[curr_idx1].pageNum;
    nEntries[curr_idx2].isValid = oEntries[curr_idx1].isValid;
    memcpy(nKeys + curr_idx2*header.attr_length, oKeys + curr_idx1*header.attr_length, header.attr_length);
    if(curr_idx2 == 0){
      newHeader->freeSlotIndex = nEntries[curr_idx2].nextSlot;
      nEntries[curr_idx2].nextSlot = newHeader->firstSlotIndex;
      newHeader->firstSlotIndex = curr_idx2;
    } 
    else{
      newHeader->freeSlotIndex = nEntries[curr_idx2].nextSlot;
      nEntries[curr_idx2].nextSlot = nEntries[prev_idx2].nextSlot;
      nEntries[prev_idx2].nextSlot = curr_idx2;
    }
    prev_idx2 = curr_idx2;
    curr_idx2 = newHeader->freeSlotIndex;

    oEntries[curr_idx1].isValid = UNOCCUPIED;
    //entries1[curr_idx1].nextSlot = oldBHeader->freeSlotIndex;
    prev_idx1 = curr_idx1;
    curr_idx1 = oEntries[prev_idx1].nextSlot;
    oEntries[prev_idx1].nextSlot = oldHeader->freeSlotIndex;
    oldHeader->freeSlotIndex = prev_idx1;
    oldHeader->num_keys--;
    newHeader->num_keys++;
  }
  char *firstEntry = (nKeys + newHeader->freeSlotIndex * header.attr_length);
  //int bucketIndex, iterlocation;
  //bool isDup;
  //if((rc = FindNodeInsertIndex(newHeader, (void*) firstEntry, bucketIndex, isDup, iterlocation)))
  //  return (rc);
  if(oldHeader->isLeafNode){
    int bucketIndex, iterloc;
    bool isDup;
    PageNum newBucketPage;
    PF_PageHandle newBucketPH, oldBucketPH;
    const RID rid;
    struct IX_BucketHeader *newBucketHeader;
    struct IX_BucketHeader *oldBucketHeader;
    PageNum oldBucketPage = nEntries[newHeader->firstSlotIndex].pageNum;
    if((rc = pfh.GetThisPage(oldBucketPage, oldBucketPH) ||
      (rc = oldBucketPH.GetData((char *&)oldBucketHeader))))
      return (rc);
    if((rc = FindBucketInsertIndex(oldBucketHeader, (void *)firstEntry, bucketIndex, isDup, 
      rid, iterloc)))
      return (rc);
    if(iterloc != -1){
      if((rc = CreateNewBucket(newBucketPage))){
        RC rc2;
        if((rc2 = pfh.UnpinPage(oldPage)) || (rc2 = pfh.UnpinPage(newPage)))
          return (rc2);
        return (rc);
      }
      if((rc = pfh.GetThisPage(newBucketPage, newBucketPH)) || (rc = newBucketPH.GetData((char *&) newBucketHeader)))
        return (rc);
    
      if((rc = SplitBucket(newHeader, oldBucketHeader, newBucketHeader, 
        oldBucketPage, newBucketPage, iterloc+1)))
        return (rc);
    }

  }

  return (rc);
}

/*
RC IX_IndexHandle::CreateNewLeafNode(PF_PageHandle &ph){
  RC rc = 0;
  return (rc);
}*/

// ONLY FOR NODES
RC IX_IndexHandle::InsertIntoNonFullNode(char *&nData, void *pData, const RID &rid){
  RC rc = 0;
  struct IX_NodeHeader *nHeader = (struct IX_NodeHeader *)nData;
  struct Node_Entry *entries = (struct Node_Entry *)(nData + header.entryOffset_N);
  char *keys = (char *)nData + header.keysOffset_N;
  //int iterloc;

  if (nHeader->isLeafNode){ // insert into leaf node
    PageNum bucketPage;
    bool isDup;
    int loc, index;
    if(nHeader->isEmpty){
      if((rc = CreateNewBucket(bucketPage)))
        return (rc);
      index = BEGINNING_OF_SLOTS;
      loc = nHeader->freeSlotIndex;
      isDup = false;
    }
    else{
      loc = nHeader->freeSlotIndex;
      FindNodeInsertIndex(nHeader, pData, index, isDup);
      if(index == BEGINNING_OF_SLOTS)
        bucketPage = entries[0].pageNum;
      else
        bucketPage = entries[index].pageNum;
    }
    if(isDup == false){
      //printf("Adding into Node Page: \n");
      //printf("entries[0].nextSlot: %d \n", entries[0].nextSlot);
      if(nHeader->isEmpty == true)
        nHeader->isEmpty = false;
      nHeader->num_keys++;
      nHeader->freeSlotIndex = entries[loc].nextSlot;
      if(index == BEGINNING_OF_SLOTS){
        entries[loc].nextSlot = nHeader->firstSlotIndex;
        nHeader->firstSlotIndex = loc;
      }
      else{
        entries[loc].nextSlot = entries[index].nextSlot;
        entries[index].nextSlot = loc;
      }
      //printf("   added into slot: %d, next slot: %d, first slot: %d, next free slot: %d \n",
      //  loc, entries[loc].nextSlot, nHeader->firstSlotIndex, nHeader->freeSlotIndex);
      entries[loc].isValid = OCCUPIED_NEW;
      entries[loc].pageNum = bucketPage;
      memcpy(keys + header.attr_length*loc, (char *) pData, header.attr_length);
    }
    if((rc = InsertIntoBucket(nHeader, bucketPage, pData, rid, isDup)))
      return (rc);
    if(isDup == false){
      entries[loc].pageNum = bucketPage;
      //printf("entries bucket: %d \n", bucketPage);
    }
    //if(isDup == false){
    //  // update the pointer in leaf node
      
    //}
  }

  // is not a leaf page
  else{

  }
  return (rc);
}

RC IX_IndexHandle::InsertIntoBucket(struct IX_NodeHeader *nHeader, PageNum &page, void *pData, const RID &rid, bool isDup){
  RC rc = 0;
  
  PF_PageHandle bucketPH;
  struct IX_BucketHeader *bHeader;
  int index = END_OF_SLOTS;
  bool searchNext = true;
  int iterloc;
  if((rc = pfh.GetThisPage(page, bucketPH)) || (rc = bucketPH.GetData((char *&)bHeader)))
    return (rc);
  if((rc = FindBucketInsertIndex(bHeader, pData, index, searchNext, rid, iterloc)))
    return (rc);
  PageNum nextPage = bHeader->nextBucket;

  while(nextPage != NO_MORE_PAGES && searchNext){
    PF_PageHandle bucketPH2;
    struct IX_BucketHeader *bHeader2;
    int index2;
    if((rc = pfh.GetThisPage(nextPage, bucketPH2)) || (rc = bucketPH2.GetData((char *&)bHeader2)))
      return (rc);
    if((rc = FindBucketInsertIndex(bHeader2, pData, index2, searchNext, rid, iterloc)))
      return (rc);
    //nextPage = bHeader2->nextBucket;
    if(searchNext == false && index2 == BEGINNING_OF_SLOTS){
      if((rc = pfh.UnpinPage(nextPage))) // use old page
        return (rc);
      break;
    }
    else if(searchNext == false || bHeader2->nextBucket == NO_MORE_PAGES){
      if((rc = pfh.UnpinPage(page))) // use new page
        return (rc);
      index = index2;
      page = nextPage;
      bHeader = bHeader2;
      break;
    }
    if((rc = pfh.UnpinPage(page))){
      return (rc);
    }
    page = nextPage;
    nextPage = bHeader2->nextBucket;
    bHeader = bHeader2;
    bucketPH = bucketPH2;
  }
  //printf("page %d, index: %d\n", page, index);

  struct Bucket_Entry *entries = (struct Bucket_Entry *)((char *)bHeader + header.entryOffset_B);
  // Insert into bucket
  int loc = bHeader->freeSlotIndex;
  //printf("next location: %d \n", loc);
  bHeader->freeSlotIndex = entries[loc].nextSlot;
  if(index == BEGINNING_OF_SLOTS){
    entries[loc].nextSlot = bHeader->firstSlotIndex;
    bHeader->firstSlotIndex = loc;
    //printf("first entry: %d, second entry: %d, freeslot: %d \n ", bHeader->firstSlotIndex, 
    //  entries[bHeader->firstSlotIndex].nextSlot, bHeader->freeSlotIndex);
  }
  else{
    entries[loc].nextSlot = entries[index].nextSlot;
    entries[index].nextSlot = loc;
    //printf("entries[%d].nextslot: %d \n", index, loc);
  }
  if(isDup)
    entries[loc].isValid = OCCUPIED_DUP;
  else
    entries[loc].isValid = OCCUPIED_NEW;
  if((rc = rid.GetPageNum(entries[loc].page)) || (rc = rid.GetSlotNum(entries[loc].slot))){
    RC rc2;
    if((rc2 = pfh.UnpinPage(page)))
      return (rc2);
    return (rc);
  }
  //printf("-------- page /slot: %d %d \n", entries[loc].page, entries[loc].slot);
  //printf("Adding Entry: %d %d \n", entries[loc].page, entries[loc].slot);
  memcpy((char *)bHeader + header.keysOffset_B + loc * header.attr_length, (char *)pData, header.attr_length);
  bHeader->num_keys++;

  PageNum oBPageNum = page;
  if(bHeader->num_keys == header.maxKeys_B){
    PageNum nBPageNum;
    PF_PageHandle nBucketPH;
    struct IX_BucketHeader *nBHeader;
    if((rc = CreateNewBucket(nBPageNum))){
      RC rc2;
      if((rc2 = pfh.UnpinPage(page)))
        return (rc2);
      return (rc);
    }
    if((rc = pfh.GetThisPage(nBPageNum, nBucketPH)) || (rc = nBucketPH.GetData((char *&) nBHeader)))
      return (rc);
    if((rc = SplitBucket(nHeader, bHeader, nBHeader, oBPageNum, nBPageNum, header.maxKeys_B/2)))
      return (rc);
    char *firstKey = (char *)nBHeader + header.keysOffset_B;
    struct Bucket_Entry *nBEntries = (struct Bucket_Entry *)((char *)nBHeader + header.entryOffset_B);
    int compare = comparator(pData, (void *)firstKey, header.attr_length);
    if(compare > 0 || (compare == 0 && nBEntries[0].isValid == OCCUPIED_NEW)){
      page = nBPageNum;
    }
    if((rc = pfh.MarkDirty(nBPageNum)) || (rc = pfh.UnpinPage(nBPageNum)))
      return (rc);
  }


  if((rc = pfh.MarkDirty(oBPageNum)) || (rc = pfh.UnpinPage(oBPageNum)))
    return (rc);

  return (0);
}


RC IX_IndexHandle::SplitBucket(struct IX_NodeHeader *nHeader, struct IX_BucketHeader *oldBHeader,
  struct IX_BucketHeader *newBHeader, PageNum oldPage, PageNum newPage, int splitIndex){
  //printf("****SPLITTING BUCKET into new bucket %d \n", newPage);
  RC rc = 0;
  //char *firstNewKeyOfSplit;
  //bool firstKeyFound = false;
  // Setup
  newBHeader->nextBucket = oldBHeader->nextBucket;
  if(newBHeader->nextBucket != NO_MORE_PAGES){
    PF_PageHandle ph;
    PageNum page;
    struct IX_BucketHeader *bHeader;
    if((rc = pfh.GetThisPage(newBHeader->nextBucket, ph)) || (rc = ph.GetPageNum(page))
      || ph.GetData((char *&)bHeader))
      return (rc);
    bHeader->prevBucket = newPage;
    if((rc = pfh.MarkDirty(page)) || (rc = pfh.UnpinPage(page)))
      return (rc);
  }
  oldBHeader->nextBucket = newPage;
  newBHeader->prevBucket = oldPage;

  struct Bucket_Entry *entries1 = (struct Bucket_Entry *)((char *)oldBHeader + header.entryOffset_B);
  struct Bucket_Entry *entries2 = (struct Bucket_Entry *)((char *)newBHeader + header.entryOffset_B);
  struct Node_Entry *nEntries = (struct Node_Entry *)((char *)nHeader + header.entryOffset_N);
  char *keys1 = ((char *)oldBHeader + header.keysOffset_B);
  char *keys2 = ((char *)newBHeader + header.keysOffset_B);
  //char *nKeys = ((char *)nHeader + header.keysOffset_N);

  int prev_idx1 = BEGINNING_OF_SLOTS;
  int curr_idx1 = oldBHeader->firstSlotIndex;
  for(int i = 0; i < splitIndex; i++){
    prev_idx1 = curr_idx1;
    curr_idx1 = entries1[prev_idx1].nextSlot;
  }
  //entries[prev_idx1].nextSlot = oldBHeader->freeSlotIndex;
  //oldBHeader->freeSlotIndex = entries1[prev_idx1].nextSlot;
  entries1[prev_idx1].nextSlot = NO_MORE_SLOTS;

  int prev_idx2 = BEGINNING_OF_SLOTS;
  int curr_idx2 = newBHeader->freeSlotIndex;
  while(curr_idx1 != NO_MORE_SLOTS){
    //entries1[curr_idx1].nextSlot = oldBHeader->firstSlotIndex;
    //oldBHeader->firstSlotIndex = curr_idx1;
    //if(firstKeyFound == false && entries1[curr_idx].isValid == OCCUPIED_NEW)
    //  firstNewKeyOfSplit = keys1[curr_idx];
    // copy
    if(entries1[curr_idx1].isValid == OCCUPIED_NEW){
      int slot;
      bool isDup;
      FindNodeInsertIndex(nHeader, (void*) (keys1 + curr_idx1 * header.attr_length), slot, isDup);
      nEntries[slot].pageNum = newPage;
    }
    entries2[curr_idx2].page = entries1[curr_idx1].page;
    entries2[curr_idx2].slot = entries1[curr_idx1].slot;
    entries2[curr_idx2].isValid = entries1[curr_idx1].isValid;
    memcpy(keys2 + curr_idx2*header.attr_length, keys1 + curr_idx1*header.attr_length, header.attr_length);
    if(curr_idx2 == 0){
      newBHeader->freeSlotIndex = entries2[curr_idx2].nextSlot;
      entries2[curr_idx2].nextSlot = newBHeader->firstSlotIndex;
      newBHeader->firstSlotIndex = curr_idx2;
    } 
    else{
      newBHeader->freeSlotIndex = entries2[curr_idx2].nextSlot;
      entries2[curr_idx2].nextSlot = entries2[prev_idx2].nextSlot;
      entries2[prev_idx2].nextSlot = curr_idx2;
    }
    prev_idx2 = curr_idx2;
    curr_idx2 = newBHeader->freeSlotIndex;

    entries1[curr_idx1].isValid = UNOCCUPIED;
    //entries1[curr_idx1].nextSlot = oldBHeader->freeSlotIndex;
    prev_idx1 = curr_idx1;
    curr_idx1 = entries1[prev_idx1].nextSlot;
    entries1[prev_idx1].nextSlot = oldBHeader->freeSlotIndex;
    oldBHeader->freeSlotIndex = prev_idx1;
    oldBHeader->num_keys--;
    newBHeader->num_keys++;
    //oldBHeader->freeSlotIndex = curr_idx1;
  }
  //entries2[idx2-1].nextSlot == NO_MORE_SLOTS;

  return (0);
}

/*
RC IX_IndexHandle::InsertIntoNonFull(struct IX_NodeHeader *nodeHeader, void *pData, const RID &rid){
  RC rc = 0;
  if(nodeHeader->isLeafNode){
    int freeslot, index;
    bool isDup;
    if((rc = FindNextFreeSlot((char *)nodeHeader, freeslot, false) ) ||
      (rc = FindIndexOfInsertion((char *)nodeHeader, false, pData, index, isDup)))
      return (rc);
    // if it is not a duplicate, insert a value for it in the leaf node
    
    if(!isDup){
      struct Node_Entry *entries = (struct Node_Entry *) (nodeHeader + header.entryOffset_N);
      entries[freeslot].nextSlot = entries[index].nextSlot;
      entries[index].nextSlot = freeslot;
      entries[loc].isValid = OCCUPIED_NEW;
    }
    struct Node_Entry entries = (struct Node_Entry *)(nodeHeader + header.entriesOffset_N);
    PageNum page = entries[index].pageNum;
    if((rc = InsertIntoBucket(nodeHeader, index, pData, rid, isDup, page)))
      return (rc);
  }
  else{

  }

  return (rc);
}
*/


RC IX_IndexHandle::SplitLeaf(PF_PageHandle &old_ph, PF_PageHandle &new_ph){
  RC rc = 0;
  return (rc);
}

/*
RC IX_IndexHandle::GetFirstNewValue(PF_PageHandle &ph, char *&value){
  RC rc = 0;
  return (rc);
}
*/

RC IX_IndexHandle::FindHalfwayIndex(char * nextSlotIndex, int size){
  RC rc = 0;
  return (rc);
}

RC IX_IndexHandle::FindNodeInsertIndex(struct IX_NodeHeader *nHeader, 
  void *pData, int& index, bool& isDup){
  struct Node_Entry *entries = (struct Node_Entry *)((char *)nHeader + header.entryOffset_N);
  char *keys = ((char *)nHeader + header.keysOffset_N);
  //printf("STARTING FINDNODE INSERT INDEX: \n");
  int prev_idx = BEGINNING_OF_SLOTS;
  int curr_idx = nHeader->firstSlotIndex;
  //iterloc = -1;
  //printf("current idx: %d, %d \n", nHeader->firstSlotIndex, *(int *) (keys + header.attr_length*nHeader->firstSlotIndex));
  isDup = false;
  while(curr_idx != NO_MORE_SLOTS){
    char *value = keys + header.attr_length * curr_idx;
    int compared = comparator(pData, (void*) value, header.attr_length);
    //printf("N comparing %d, %d. result: %d \n", *(int *)pData, *(int *)value, compared);
    if(compared == 0)
      isDup = true;
    if(compared < 0)
      break;
    prev_idx = curr_idx;
    curr_idx = entries[prev_idx].nextSlot;

  } 
  index = prev_idx;
  //printf("node found index: %d \n", index);
  return (0);
}

// Return 
RC IX_IndexHandle::FindBucketInsertIndex(struct IX_BucketHeader *bHeader,
  void *pData, int &index, bool& searchNext, const RID &rid, int &iterloc){
  RC rc = 0;
  PageNum ridpage;
  SlotNum ridslot;
  if((rc = rid.GetPageNum (ridpage)) || (rc = rid.GetSlotNum(ridslot)))
    return (rc);
  struct Bucket_Entry *entries = (struct Bucket_Entry *)((char *)bHeader + header.entryOffset_B);
  char *keys = ((char *)bHeader + header.keysOffset_B);
  //printf("STARTING FINDBUCKET INSERT INDEX: \n");
  int prev_idx = BEGINNING_OF_SLOTS;
  int curr_idx = bHeader->firstSlotIndex;
  iterloc = -1;
  while(curr_idx != NO_MORE_SLOTS){
    //printf("currIndex: %d \n", curr_idx);
    char *value = keys + header.attr_length * curr_idx;
    int compared = comparator(pData, (void*) value, header.attr_length);
    //printf("B comparing %d, %d. result: %d \n", *(int *)pData, *(int *)value, compared);
    if(compared == 0 && entries[curr_idx].slot == ridslot && entries[curr_idx].page == ridpage)
      return (IX_DUPLICATEENTRY);
    if(compared < 0){
      searchNext = false;
      break;
    }
    iterloc ++;
    prev_idx = curr_idx;
    curr_idx = entries[prev_idx].nextSlot;
  } 
  index = prev_idx;
  //printf("bucket found index: %d \n", index);
  return (0);
}

// In parent function:
// If leaf, and returns duplicate, set the isValid value
// Put it in the overflow.
// Returns the index offset of the term of the previous one
// that it needs to go in.
/*
RC IX_IndexHandle::FindNodeInsertIndex(char *nodeHeader, 
  void* pData, int& index, bool& isDup){

  int entrySize;
  char *entryOffset;
  char *keyOffset;
  if(isBucket){
    entrySize = sizeof(struct Bucket_Entry);
    entryOffset = nodeHeader + header.entryOffset_B;
    keyOffset = nodeHeader + header.keysOffset_B;
  }
  else{
    entrySize = sizeof(struct Node_Entry);
    entryOffset = nodeHeader + header.entryOffset_N;
    keyOffset = nodeHeader + header.keysOffset_N;
  }

  index = BEGINNING_OF_SLOTS;
  int prev_idx = BEGINNING_OF_SLOTS;
  int curr_idx = 0;
  isDup = false;
  while(curr_idx != END_OF_SLOTS){
    char *value = keyOffset + header.attr_length * curr_idx;
    int compared = comparator(pData, (void*) value, header.attr_length);
    if (compared == 0)
      isDup = true;
    if(compared > 0)
      break;
    prev_idx = curr_idx;
    struct Entry * entry = (struct Entry *)(entryOffset + entrySize * prev_idx);
    curr_idx = entry->nextSlot;
  }
  index = prev_idx;
  return (0);

}
*/


RC IX_IndexHandle::UpdateNextSlotIndex(int* slotindex, int* firstPage, int before, int insert){
  RC rc = 0;
  if(before == BEGINNING_OF_SLOTS){
    *firstPage = insert;
    slotindex[0] = before;
  }
  slotindex[insert] = slotindex[before];
  slotindex[before] = insert;

  return (rc);
}

bool IX_IndexHandle::isValidIndexHeader(){
  //printf("maxkeys: %d, %d \n", header.maxKeys_I, header.maxKeys_L);
  if(header.maxKeys_N <= 0 || header.maxKeys_B <= 0)
    return false;
  if(header.entryOffset_N != sizeof(struct IX_NodeHeader_L) || 
    header.entryOffset_B != sizeof(struct IX_BucketHeader)){
    //printf("keyheaders: %d , %d \n", header.keyHeadersOffset_I, header.keyHeadersOffset_L);
    //printf("part 2 not true \n");
    return false;
  }

  //printf("size of keyHeader_L : %d \n", sizeof(struct KeyHeader_L));
  int attrLength1 = (header.keysOffset_B - header.entryOffset_B)/(header.maxKeys_B);
  int attrLength2 = (header.keysOffset_N - header.entryOffset_N)/(header.maxKeys_N);
  //printf("attribute length: %d, %d \n", attrLength1, attrLength2);
  if(attrLength1 != sizeof(struct Bucket_Entry) || attrLength2 != sizeof(struct Node_Entry))
    return false;
  if((header.entryOffset_B + header.maxKeys_B * header.attr_length > PF_PAGE_SIZE) ||
    header.entryOffset_N + header.maxKeys_N * header.attr_length > PF_PAGE_SIZE)
    return false;
  return true;
}

int IX_IndexHandle::CalcNumKeysNode(int attrLength){
  int body_size = PF_PAGE_SIZE - sizeof(struct IX_NodeHeader_I);
  return floor(1.0*body_size / (sizeof(struct Node_Entry) + attrLength));
}

int IX_IndexHandle::CalcNumKeysBucket(int attrLength){
  int body_size = PF_PAGE_SIZE - sizeof(struct IX_BucketHeader);
  return floor(1.0*body_size / (sizeof(Bucket_Entry) + attrLength));
}


