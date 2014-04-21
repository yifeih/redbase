//
// ix_internal.h
//
// Index Manager Component Interface - Internal Structures
//

#ifndef IX_INTERNAL_H
#define IX_INTERNAL_H

#include "ix.h"
#include <cstring>

#define NO_MORE_PAGES -1
#define NO_MORE_SLOTS -1

struct IX_Header{
  bool isLeafNode;
};

struct IX_InternalHeader {
  bool isLeafNode;
  bool hasFirstKey; // has at least one entry (firstPage is filled)
  int num_keys;

  PageNum firstLeafPage; // first leaf page under this internal node
  PageNum firstPage;
  int firstKeyIndex;
};

struct IX_LeafHeader{
  bool isLeafNode;
  bool hasFirstKey; // has at least one entry (nextPage is valid)
  int num_keys;

  PageNum nextPage; // next page
  PageNum overflowPage;
  int firstKeyIndex;
};

struct KeyHeader{
  char isValid;
  int nextSlot;
  PageNum pageNum;
  SlotNum slot;
};



#endif