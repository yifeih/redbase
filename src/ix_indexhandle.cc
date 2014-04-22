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

RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid){
  RC rc = 0;

  return (rc);
}

RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid){
  RC rc = 0;
  return (rc);
}

RC IX_IndexHandle::ForcePages(){
  RC rc = 0;
  return (rc);
}

RC IX_IndexHandle::CreateNewInternalNode(PF_PageHandle &ph){
  RC rc = 0;
  return (rc);
}

RC IX_IndexHandle::CreateNewLeafNode(PF_PageHandle &ph){
  RC rc = 0;
  return (rc);
}

// ONLY FOR NODES
RC IX_IndexHandle::InsertIntoNonFull(struct IX_NodeHeader *nodeHeader, void *pData, const RID &rid){
  RC rc = 0;
  if(nodeHeader->isLeafNode){
    int freeslot, index;
    bool isDup;
    if((rc = FindNextFreeSlot((char *)nodeHeader, freeslot, false) ) ||
      (rc = FindIndexOfInsertion((char *)nodeHeader, false, pData, index, isDup)))
      return (rc);
    // if it is not a duplicate, insert a value for it in the leaf node
    /*
    if(!isDup){
      struct Node_Entry *entries = (struct Node_Entry *) (nodeHeader + header.entryOffset_N);
      entries[freeslot].nextSlot = entries[index].nextSlot;
      entries[index].nextSlot = freeslot;
      entries[loc].isValid = OCCUPIED_NEW;
    }*/
    struct Node_Entry entries = (struct Node_Entry *)(nodeHeader + header.entriesOffset_N);
    PageNum page = entries[index].pageNum;
    if((rc = InsertIntoBucket(nodeHeader, index, pData, rid, isDup, page)))
      return (rc);
  }
  else{

  }

  return (rc);
}

RC IX_IndexHandle::CreateNewBucket(PF_PageHandle &ph, PageNum &page){
  RC rc;
  // allocate the page
  if((rc = pfh.AllocatePage(ph)) || (rc = ph.GetPageNum(page))){
    return (rc);
  }

  struct IX_BucketHeader *header;
  if((rc = ph.GetData((char &*)header)))
    return (rc);

  header->num_keys = 0;
  header->firstSlotIndex = NO_MORE_SLOTS;
  header->nextPage = 

}

RC IX_IndexHandle::InsertIntoBucket(struct IX_NodeHeader *nodeHeader, int index, void *pData, 
  const RID &rid, bool isDup, PageNum &page){
  RC rc = 0;
  PF_PageHandle ph;
  char *bData;
  if((rc = pfh.GetThisPage(page, ph)) || (rc = ph.GetData(bData))){
    return (rc);
  }
  int bucketindex, freeslot;
  if(IX_NODEFULL == FindFreeSlot(bData, freeslot, true)){
    // create new page
    // move the last slot over to the next page
  }

  if (isDup)

  return (rc);
}

RC IX_IndexHandle::SplitInternal(PF_PageHandle &old_ph, PF_PageHandle &new_ph){
  RC rc = 0;
  return (rc);
}

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

// WORKS FOR BOTH NODE AND BUCKET ENTRIES
// Returns the index to headerArr that is free
// ignores the fact that the first value is full
RC IX_IndexHandle::FindNextFreeSlot(char * nodeHeader, int& slot, bool isBucket){
  int entrySize, maxEntries;
  char * entryOffset;
  if(isBucket){
    entrySize = sizeof(struct Bucket_Entry);
    entryOffset = nodeHeader + header.entryOffset_B;
    maxEntries = header.maxKeys_B;
  }
  else{
    entrySize = sizeof(struct Node_Entry);
    entryOffset = nodeHeader + header.entryOffset_N;
    maxEntries = header.maxKeys_N;
  }

  slot = BEGINNING_OF_SLOTS;
  for(int i=0; i < maxEntries; i++){
    struct Entry *entry = (struct Entry *)(entryOffset + entrySize*i);
    if(entry->isValid == UNOCCUPIED){
      slot = i;
      return (0);
    }
  }

  return (IX_NODEFULL);
}

// In parent function:
// If leaf, and returns duplicate, set the isValid value
// Put it in the overflow.
// Returns the index offset of the term of the previous one
// that it needs to go in.
RC IX_IndexHandle::FindIndexOfInsertion(char *nodeHeader, 
  bool isBucket, void* pData, int& index, bool& isDup){

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
  int curr_idx = ;
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

// Find second to last entry:
RC IX_IndexHandle::FindSecondLastBucketEntry(struct IX_BucketHeader *bucketHeader, int &index){
  struct Bucket_Entry *entries = (struct Bucket_Entry *) (bucketHeader + header.entriesOffset_B);
  int prev_idx = BEGINNING_OF_SLOTS;
  int curr_idx = bucketHeader->firstSlotIndex;
  while(curr_idx != NO_MORE_SLOTS){
    if(entries[curr_idx].nextSlot = NO_MORE_SLOTS)
      break;
    prev_idx = curr_idx;
    curr_idx = entries[curr_idx].nextSlot;
  }
  index = prev_idx;
  return (0);
  //return (IX_INVALIDBUCKET);
}

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


