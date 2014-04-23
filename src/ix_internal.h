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

// HEADERS FOR NODES
struct IX_NodeHeader{
  bool isLeafNode;
  bool isEmpty;
  int num_keys;

  int firstSlotIndex;
  int freeSlotIndex;
  PageNum invalid1;
};

struct IX_NodeHeader_I{
  bool isLeafNode;
  bool isEmpty; // has at least one entry (firstPage is filled)
  int num_keys;

  int firstSlotIndex; // first index into slot list
  int freeSlotIndex;
  PageNum firstPage; // first leaf page under this internal node
};

struct IX_NodeHeader_L{
  bool isLeafNode;
  bool isEmpty; // has at least one entry (nextPage is valid)
  int num_keys;

  int firstSlotIndex;
  int freeSlotIndex;
  PageNum nextPage; // next page
};

// HEADERS FOR ENTRIES 
struct Entry{
  char isValid;
  int nextSlot;
};

struct Node_Entry{
  char isValid;
  int nextSlot;
  PageNum pageNum;
};
struct Bucket_Entry{
  char isValid;
  int nextSlot;
  PageNum page;
  SlotNum slot;
};

// HEADER FOR CONTENT PAGES
struct IX_BucketHeader{
  int num_keys;
  int firstSlotIndex;
  int freeSlotIndex;
  PageNum nextBucket;
  PageNum prevBucket;
};



#endif