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

RC IX_IndexHandle::InsertIntoNonFull(PF_PageHandle &ph, void *pData, const RID &rid){
  RC rc = 0;
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

// Returns the index to headerArr that is free
// ignores the fact that the first value is full
RC IX_IndexHandle::FindNextFreeSlot(struct KeyHeader *headerArr, int& slot, int maxNumSlots){
  slot = BEGINNING_OF_SLOTS;
  for(int i=0; i < maxNumSlots; i++){
    if(headerArr[i].isValid == UNOCCUPIED){
      slot = i;
      return (0);
    }
  }

  //if(slot == BEGINNING_OF_SLOTS)
  //  return (IX_NODEFULL);
  return (IX_NODEFULL);
}

// In parent function:
// If leaf, and returns duplicate, set the isValid value
// Put it in the overflow.
RC IX_IndexHandle::FindIndexOfInsertion(struct KeyHeader *headerArr, char* keyArray, 
  int maxNumSlots, void* pData, int& index, bool& isDup){
  index = BEGINNING_OF_SLOTS;
  int prev_idx = BEGINNING_OF_SLOTS;
  int curr_idx = 0;
  isDup = false;
  while(curr_idx != END_OF_SLOTS){
    char* value = keyArray + header.attr_length*curr_idx;
    int compared = comparator(pData, (void*)value, header.attr_length);
    if(compared == 0)
      isDup = true;
    if (compared > 0){
      break;
    }
    prev_idx = curr_idx;
    curr_idx = headerArr[prev_idx].nextSlot;
  }
  index = prev_idx;
  return (0);
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
  if(header.maxKeys_I <= 0 || header.maxKeys_L <= 0)
    return false;
  if(header.keyHeadersOffset_I != sizeof(struct IX_InternalHeader) || 
    header.keyHeadersOffset_L != sizeof(struct IX_LeafHeader)){
    //printf("keyheaders: %d , %d \n", header.keyHeadersOffset_I, header.keyHeadersOffset_L);
    //printf("part 2 not true \n");
    return false;
  }

  //printf("size of keyHeader_L : %d \n", sizeof(struct KeyHeader_L));
  int attrLength1 = (header.keysOffset_I - header.keyHeadersOffset_I)/(header.maxKeys_I);
  int attrLength2 = (header.keysOffset_L - header.keyHeadersOffset_L)/(header.maxKeys_L);
  //printf("attribute length: %d, %d \n", attrLength1, attrLength2);
  if(attrLength1 == sizeof(struct KeyHeader) && attrLength2 == sizeof(struct KeyHeader))
    return true;
  return false;
  /*
  if(header.maxKeys_i <= 0 || header.maxKeys_l <= 0)
    return false;
  if(header.slotIndexOffset_i != sizeof(struct IX_InternalHeader) || 
    header.slotIndexOffset_l != sizeof(struct IX_LeafHeader))
    return false;
  if((header.validOffset_i != (header.slotIndexOffset_i + sizeof(int)*header.maxKeys_i))
    || (header.validOffset_l != (header.slotIndexOffset_l + sizeof(int)*header.maxKeys_l)))
    return false;
  if((header.keysOffset_i != (header.validOffset_i + header.maxKeys_i)) ||
    (header.keysOffset_l != (header.validOffset_l + header.maxKeys_l)))
    return false;

  int attrLength1 = (header.pagesOffset_i - header.keysOffset_i)/header.maxKeys_i;
  int attrLength2 = (header.RIDsOffset_l - header.keysOffset_l)/header.maxKeys_l;
  if(attrLength1 != attrLength2)
    return false;
  return true;
  */
}

int IX_IndexHandle::CalcNumKeysInternal(int attrLength){
  int body_size = PF_PAGE_SIZE - sizeof(struct IX_InternalHeader);
  return floor(1.0*body_size / ((sizeof(struct KeyHeader) + attrLength)));
  /*
  int body_size = PF_PAGE_SIZE - sizeof(struct IX_InternalHeader);
  return floor(body_size/(sizeof(char) + sizeof(int) + sizeof(PageNum) + attrLength));
  */
}

int IX_IndexHandle::CalcNumKeysLeaf(int attrLength){
  int body_size = PF_PAGE_SIZE - sizeof(struct IX_LeafHeader);
  return floor(1.0*body_size / ((sizeof(struct KeyHeader) + attrLength)));
  //return floor(body_size/(sizeof(char) + sizeof(int) + sizeof(RID) + attrLength));
}


