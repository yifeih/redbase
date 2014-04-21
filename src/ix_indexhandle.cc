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

RC IX_IndexHandle::GetFirstNewValue(PF_PageHandle &ph, char *&value){
  RC rc = 0;
  return (rc);
}

RC IX_IndexHandle::FindHalfwayIndex(char * nextSlotIndex, int size){
  RC rc = 0;
  return (rc);
}

RC IX_IndexHandle::FindNextFreeSlot(char * validArray, int& slot, int size){
  RC rc = 0;
  slot = BEGINNING_OF_SLOTS;
  for(int i=0; i < size; i++){
    if(validArray[i] == UNOCCUPIED){
      slot = i;
      return (0);
    }
  }

  //if(slot == BEGINNING_OF_SLOTS)
  //  return (IX_NODEFULL);
  return (IX_NODEFULL);
}

RC IX_IndexHandle::FindIndexOfInsertion(int& index, void* pData, void* keyArray, void*int size){
  RC rc = 0;
  index = BEGINNING_OF_SLOTS;
  for(int i = )


  return (rc);
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
}

int IX_IndexHandle::CalcNumKeysInternal(int attrLength){
  int body_size = PF_PAGE_SIZE - sizeof(struct IX_InternalHeader);
  return floor(body_size/(sizeof(char) + sizeof(int) + sizeof(PageNum) + attrLength));
}

int IX_IndexHandle::CalcNumKeysLeaf(int attrLength){
  int body_size = PF_PAGE_SIZE - sizeof(struct IX_LeafHeader);
  return floor(body_size/(sizeof(char) + sizeof(int) + sizeof(RID) + attrLength));
}


