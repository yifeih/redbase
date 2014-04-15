//
// File:        rm_rid.cc
// Description: rm_rid represents a RID
// Author:      Yifei Huang (yifei@stanford.edu)
//
#include "rm_rid.h"
#include "rm_internal.h"

RID::RID(){
  page = INVALID_PAGE; // initially the RID refers to invalid page/slot
  slot = INVALID_SLOT;
}

RID::RID(PageNum pageNum, SlotNum slotNum) {
  page = pageNum;
  slot = slotNum;
}

RID::~RID(){}

/*
 * Copies contents of one RID to another
 */
RID& RID::operator= (const RID &rid){
  if (this != &rid){
    this->page = rid.page;
    this->slot = rid.slot;
  }
  return (*this);
}

bool RID::operator== (const RID &rid) const{
  return (this->page == rid.page && this->slot == rid.slot);
}

/*
 * Returns the page number of an RID 
 */
RC RID::GetPageNum(PageNum &pageNum) const {
  //if(page == INVALID_PAGE) return RM_INVALIDRID;
  pageNum = page;
  return 0;
}

/*
 * Returns the page number of an RID o
 */
RC RID::GetSlotNum(SlotNum &slotNum) const {
  //if(slot == INVALID_SLOT) return RM_INVALIDRID;
  slotNum = slot;
  return 0;
}

/*
 * Checks that this is a valid RID
 */
RC RID::isValidRID() const{
  if(page > 0 && slot >= 0)
    return 0;
  else
    return RM_INVALIDRID;
}