
#include "rm_rid.h"
#include "rm_internal.h"

RID::RID(){
  page = INVALID_PAGE;
  slot = INVALID_SLOT;
}

RID::RID(PageNum pageNum, SlotNum slotNum) {
  page = pageNum;
  slot = slotNum;
}

RID::~RID(){}

RID& RID::operator= (const RID &rid){
  if (this != &rid){
    this->page = rid.page;
    this->slot = rid.slot;
  }
  return (*this);
}

RC RID::GetPageNum(PageNum &pageNum) const {
  if(page == INVALID_PAGE) return RM_INVALIDRID;
  pageNum = page;
  return 0;
}

RC RID::GetSlotNum(SlotNum &slotNum) const {
  if(slot == INVALID_SLOT) return RM_INVALIDRID;
  slotNum = slot;
  return 0;
}

RC RID::isValidRID() const{
  if(page > 0 && slot >= 0)
    return 0;
  else
    return RM_INVALIDRID;
}