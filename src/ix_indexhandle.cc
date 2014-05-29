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



RC IX_IndexHandle::PrintIndex(){
  RC rc;
  PageNum leafPage;
  PF_PageHandle ph;
  struct IX_NodeHeader_L *lheader;
  struct Node_Entry *entries;
  char *keys;
  if((rc = GetFirstLeafPage(ph, leafPage) || (rc = pfh.UnpinPage(leafPage))))
    return (rc);
  while(leafPage != NO_MORE_PAGES){ // print this page
    if((rc = pfh.GetThisPage(leafPage, ph)) || (rc = ph.GetData((char *&)lheader)))
      return (rc);
    entries = (struct Node_Entry*) ( (char *)lheader + header.entryOffset_N);
    keys = (char *)lheader + header.keysOffset_N;

    int prev_idx = BEGINNING_OF_SLOTS;
    int curr_idx = lheader->firstSlotIndex;
    while(curr_idx != NO_MORE_SLOTS){
      printf("\n");
      printer((void *)keys + curr_idx*header.attr_length, header.attr_length);
      if(entries[curr_idx].isValid == OCCUPIED_DUP){
        //printf("is a duplicate\n");
        PageNum bucketPage = entries[curr_idx].page;
        PF_PageHandle bucketPH;
        struct IX_BucketHeader *bheader;
        struct Bucket_Entry *bEntries;
        while(bucketPage != NO_MORE_PAGES){
          if((rc = pfh.GetThisPage(bucketPage, bucketPH)) || (rc = bucketPH.GetData((char *&)bheader))) {
            return (rc);
          }
          bEntries = (struct Bucket_Entry *) ((char *)bheader + header.entryOffset_B); 
          int currIdx = bheader->firstSlotIndex;
          int prevIdx = BEGINNING_OF_SLOTS;
          while(currIdx != NO_MORE_SLOTS){
            //printf("currIdx: %d ", currIdx);
            printf("rid: %d, page %d | ", bEntries[currIdx].page, bEntries[currIdx].slot);
            prevIdx = currIdx;
            currIdx = bEntries[prevIdx].nextSlot;
          }
          PageNum nextBucketPage = bheader->nextBucket;
          if((rc = pfh.UnpinPage(bucketPage)))
            return (rc);
          bucketPage = nextBucketPage;
        }

      }
      else{
        printf("rid: %d, page %d | ", entries[curr_idx].page, entries[curr_idx].slot);
      }
      prev_idx = curr_idx;
      curr_idx = entries[prev_idx].nextSlot;

    }
    PageNum nextPage = lheader->nextPage;
    if(leafPage != header.rootPage){
      if((rc = pfh.UnpinPage(leafPage)))
        return (rc);
    }
    leafPage = nextPage;
  }
  return (0);
}

IX_IndexHandle::IX_IndexHandle(){
  isOpenHandle = false;       // indexhandle is initially closed
  header_modified = false;
}

IX_IndexHandle::~IX_IndexHandle(){

}

/*
 * This function initializes the insert of a certain value and RID into the
 * Index. If the entry already exists, return IX_DUPLICATEENTRY
 */
RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid){
  if(isOpenHandle == false){
  }

  // only insert if this is a valid, open indexHandle
  if(! isValidIndexHeader() || isOpenHandle == false)
    return (IX_INVALIDINDEXHANDLE);

  // Retrieve the root header
  RC rc = 0;
  struct IX_NodeHeader *rHeader;
  if((rc = rootPH.GetData((char *&)rHeader))){
    return (rc);
  }

  // If the root is full, create a new empty root node
  if(rHeader->num_keys == header.maxKeys_N){
    PageNum newRootPage;
    char *newRootData;
    PF_PageHandle newRootPH;
    if((rc = CreateNewNode(newRootPH, newRootPage, newRootData, false))){
      return (rc);
    }
    struct IX_NodeHeader_I *newRootHeader = (struct IX_NodeHeader_I *)newRootData;
    newRootHeader->isEmpty = false;
    newRootHeader->firstPage = header.rootPage; // update the root node

    int unused;
    PageNum unusedPage;
    // Split the current root node into two nodes, and make the parent the new
    // root node
    if((rc = SplitNode((struct IX_NodeHeader *&)newRootData, (struct IX_NodeHeader *&)rHeader, header.rootPage, 
      BEGINNING_OF_SLOTS, unused, unusedPage)))
      return (rc);
    if((rc = pfh.MarkDirty(header.rootPage)) || (rc = pfh.UnpinPage(header.rootPage)))
      return (rc);
    rootPH = newRootPH; // reset root PF_PageHandle
    header.rootPage = newRootPage;
    header_modified = true; // New root page has been set, so the index header has been modified

    // Retrieve the contents of the new Root node
    struct IX_NodeHeader *useMe;
    if((rc = newRootPH.GetData((char *&)useMe))){
      return (rc);
    }
    // Insert into the non-full root node
    if((rc = InsertIntoNonFullNode(useMe, header.rootPage, pData, rid)))
      return (rc);
  }
  else{ // If root is not full, insert into it
    if((rc = InsertIntoNonFullNode(rHeader, header.rootPage, pData, rid))){
      return (rc);
    }
  }

  // Mark the root node as dirty
  if((rc = pfh.MarkDirty(header.rootPage)))
    return (rc);

  return (rc);
}

/*
 * This function deals with splitting a node:
 * pHeader - the header of the parent node
 * oldHeader - the header of the full node to be split
 * oldPage - the PageNum of the old node to be split
 * index - the index into which to insert the new node into in the parent node
 * newKeyIndex - the index of the first key that points to the new node
 * newPageNum - the page number of the new node
 */
RC IX_IndexHandle::SplitNode(struct IX_NodeHeader *pHeader, struct IX_NodeHeader *oldHeader, 
  PageNum oldPage, int index, int & newKeyIndex, PageNum &newPageNum){
  RC rc = 0;
  //printf("********* SPLIT ********* at index %d \n", index);
  bool isLeaf = false;  // Determines if the new page should be a leaf page
  if(oldHeader->isLeafNode == true){
    isLeaf = true;
  }
  PageNum newPage;  // Creates the new page, and acquires its headers
  struct IX_NodeHeader *newHeader;
  PF_PageHandle newPH;
  if((rc = CreateNewNode(newPH, newPage, (char *&)newHeader, isLeaf))){
    return (rc);
  }
  newPageNum = newPage; // returns new page number

  // Retrieve the appropriate pointers to all the nodes' contents
  struct Node_Entry *pEntries = (struct Node_Entry *) ((char *)pHeader + header.entryOffset_N);
  struct Node_Entry *oldEntries = (struct Node_Entry *) ((char *)oldHeader + header.entryOffset_N);
  struct Node_Entry *newEntries = (struct Node_Entry *) ((char *)newHeader + header.entryOffset_N);
  char *pKeys = (char *)pHeader + header.keysOffset_N;
  char *newKeys = (char *)newHeader + header.keysOffset_N;
  char *oldKeys = (char *)oldHeader + header.keysOffset_N;

  // Keep the first header.masKeys_N/2 values in the old node
  int prev_idx1 = BEGINNING_OF_SLOTS;
  int curr_idx1 = oldHeader->firstSlotIndex;
  for(int i=0; i < header.maxKeys_N/2 ; i++){
    prev_idx1 = curr_idx1;
    curr_idx1 = oldEntries[prev_idx1].nextSlot;
  }
  oldEntries[prev_idx1].nextSlot = NO_MORE_SLOTS;

  // This is the key to use in the parent node to point to the new node we're creating
  char *parentKey = oldKeys + curr_idx1*header.attr_length; 
  
  //char * tempchar = (char *)malloc(header.attr_length);
  //memcpy(tempchar, parentKey, header.attr_length);
  //printf("split key: %s \n", tempchar);
  //free(tempchar);

  // If we're not splitting a leaf node, then update the firstPageNum pointer in
  // the new internal node's header.
  if(!isLeaf){ 
    struct IX_NodeHeader_I *newIHeader = (struct IX_NodeHeader_I *)newHeader;
    newIHeader->firstPage = oldEntries[curr_idx1].page;
    newIHeader->isEmpty = false;
    prev_idx1 = curr_idx1;
    curr_idx1 = oldEntries[prev_idx1].nextSlot;
    oldEntries[prev_idx1].nextSlot = oldHeader->freeSlotIndex;
    oldHeader->freeSlotIndex = prev_idx1;
    oldHeader->num_keys--;
  }

  // Now, move the remaining header.maxKeys_N/2 values into the new node
  int prev_idx2 = BEGINNING_OF_SLOTS;
  int curr_idx2 = newHeader->freeSlotIndex;
  while(curr_idx1 != NO_MORE_SLOTS){
    newEntries[curr_idx2].page = oldEntries[curr_idx1].page;
    newEntries[curr_idx2].slot = oldEntries[curr_idx1].slot;
    newEntries[curr_idx2].isValid = oldEntries[curr_idx1].isValid;
    memcpy(newKeys + curr_idx2*header.attr_length, oldKeys + curr_idx1*header.attr_length, header.attr_length);
    if(prev_idx2 == BEGINNING_OF_SLOTS){
      newHeader->freeSlotIndex = newEntries[curr_idx2].nextSlot;
      newEntries[curr_idx2].nextSlot = newHeader->firstSlotIndex;
      newHeader->firstSlotIndex = curr_idx2;
    } 
    else{
      newHeader->freeSlotIndex = newEntries[curr_idx2].nextSlot;
      newEntries[curr_idx2].nextSlot = newEntries[prev_idx2].nextSlot;
      newEntries[prev_idx2].nextSlot = curr_idx2;
    }
    prev_idx2 = curr_idx2;  
    curr_idx2 = newHeader->freeSlotIndex; // update insert index

    prev_idx1 = curr_idx1;
    curr_idx1 = oldEntries[prev_idx1].nextSlot;
    oldEntries[prev_idx1].nextSlot = oldHeader->freeSlotIndex;
    oldHeader->freeSlotIndex = prev_idx1;
    oldHeader->num_keys--;
    newHeader->num_keys++;
  }

  // insert parent key into parent at index specified in parameters
  int loc = pHeader->freeSlotIndex;
  memcpy(pKeys + loc * header.attr_length, parentKey, header.attr_length);
  newKeyIndex = loc;  // return the slot location that points to the new node
  pEntries[loc].page = newPage;
  pEntries[loc].isValid = OCCUPIED_NEW;
  if(index == BEGINNING_OF_SLOTS){
    pHeader->freeSlotIndex = pEntries[loc].nextSlot;
    pEntries[loc].nextSlot = pHeader->firstSlotIndex;
    pHeader->firstSlotIndex = loc;
  }
  else{
    pHeader->freeSlotIndex = pEntries[loc].nextSlot;
    pEntries[loc].nextSlot = pEntries[index].nextSlot;
    pEntries[index].nextSlot = loc;
  }
  pHeader->num_keys++;

  // if is leaf node, update the page pointers to the previous and next leaf node
  if(isLeaf){
    struct IX_NodeHeader_L *newLHeader = (struct IX_NodeHeader_L *) newHeader;
    struct IX_NodeHeader_L *oldLHeader = (struct IX_NodeHeader_L *) oldHeader;
    newLHeader->nextPage = oldLHeader->nextPage;
    newLHeader->prevPage = oldPage;
    oldLHeader->nextPage = newPage;
    if(newLHeader->nextPage != NO_MORE_PAGES){
      PF_PageHandle nextPH;
      struct IX_NodeHeader_L *nextHeader;
      if((rc = pfh.GetThisPage(newLHeader->nextPage, nextPH)) || (nextPH.GetData((char *&)nextHeader)))
        return (rc);
      nextHeader->prevPage = newPage;
      if((rc = pfh.MarkDirty(newLHeader->nextPage)) || (rc = pfh.UnpinPage(newLHeader->nextPage)))
        return (rc);
    }
  }

  // Mark the new page as dirty, and unpin it
  if((rc = pfh.MarkDirty(newPage))||(rc = pfh.UnpinPage(newPage))){
    return (rc);
  }
  return (rc);
}


/*
 * This inserts an RID into a bucket associated with a certain key
 */
RC IX_IndexHandle::InsertIntoBucket(PageNum page, const RID &rid){
  RC rc = 0;
  PageNum ridPage; // Gets the Page and Slot Number from this rid object
  SlotNum ridSlot;
  if((rc = rid.GetPageNum(ridPage)) || (rc = rid.GetSlotNum(ridSlot))){
    return (rc);
  }

  bool notEnd = true; // for searching for an empty spot in the bucket
  PageNum currPage = page;
  PF_PageHandle bucketPH;
  struct IX_BucketHeader *bucketHeader;
  while(notEnd){
    // Get the contents of this bucket
    if((rc = pfh.GetThisPage(currPage, bucketPH)) || (rc = bucketPH.GetData((char *&)bucketHeader))){
      return (rc);
    }
    // Try to find the entry in the database
    struct Bucket_Entry *entries = (struct Bucket_Entry *)((char *)bucketHeader + header.entryOffset_B);
    int prev_idx = BEGINNING_OF_SLOTS;
    int curr_idx = bucketHeader->firstSlotIndex;
    while(curr_idx != NO_MORE_SLOTS){
      // If we found a duplicate entry, then return an error
      if(entries[curr_idx].page == ridPage && entries[curr_idx].slot == ridSlot){
        RC rc2 = 0; 
        if((rc2 = pfh.UnpinPage(currPage)))
          return (rc2);
        return (IX_DUPLICATEENTRY);
      }
      prev_idx = curr_idx;
      curr_idx = entries[prev_idx].nextSlot;
    }
    // If this is the last bucket in the string of buckets, and it's full, create a new bucket
    if(bucketHeader->nextBucket == NO_MORE_PAGES && bucketHeader->num_keys == header.maxKeys_B){
      notEnd = false; // end the search
      PageNum newBucketPage;
      PF_PageHandle newBucketPH;
      if((rc = CreateNewBucket(newBucketPage)))
        return (rc);
      bucketHeader->nextBucket = newBucketPage;
      if((rc = pfh.MarkDirty(currPage)) || (rc = pfh.UnpinPage(currPage))) // unpin previous bucket
        return (rc);
      // Get the contents of the new bucket
      currPage = newBucketPage; 
      if((rc = pfh.GetThisPage(currPage, bucketPH)) || (rc = bucketPH.GetData((char *&)bucketHeader)))
        return (rc);
      entries = (struct Bucket_Entry *)((char *)bucketHeader + header.entryOffset_B);
    }
    // If it's the last bucket in the string of buckets, insert the value in
    if(bucketHeader->nextBucket == NO_MORE_PAGES){
      notEnd = false; // end search
      int loc = bucketHeader->freeSlotIndex;  // Insert into this location
      entries[loc].slot = ridSlot;
      entries[loc].page = ridPage;
      bucketHeader->freeSlotIndex = entries[loc].nextSlot;
      entries[loc].nextSlot = bucketHeader->firstSlotIndex;
      bucketHeader->firstSlotIndex = loc;
      bucketHeader->num_keys++;
    }

    // Update the currPage pointer to the next bucket in the sequence
    PageNum nextPage = bucketHeader->nextBucket;
    if((rc = pfh.MarkDirty(currPage)) || (rc = pfh.UnpinPage(currPage)))
      return (rc);
    currPage = nextPage;
  }
  return (0);
}

/*
 * This inserts a value and RID into a node given its header and page number. 
 */
RC IX_IndexHandle::InsertIntoNonFullNode(struct IX_NodeHeader *nHeader, PageNum thisNodeNum, void *pData, 
  const RID &rid){
  RC rc = 0;

  // Retrieve contents of this node
  struct Node_Entry *entries = (struct Node_Entry *) ((char *)nHeader + header.entryOffset_N);
  char *keys = (char *)nHeader + header.keysOffset_N;

  // If it is a leaf node, then insert into it
  if(nHeader->isLeafNode){
    int prevInsertIndex = BEGINNING_OF_SLOTS;
    bool isDup = false;
    if((rc = FindNodeInsertIndex(nHeader, pData, prevInsertIndex, isDup))) // get appropriate index
      return (rc);
    // If it's not a duplicate, then insert a new key for it, and update
    // the slot and page values. 
    if(!isDup){
      int index = nHeader->freeSlotIndex;
      memcpy(keys + header.attr_length * index, (char *)pData, header.attr_length);
      entries[index].isValid = OCCUPIED_NEW; // mark it as a single entry
      if((rc = rid.GetPageNum(entries[index].page)) || (rc = rid.GetSlotNum(entries[index].slot)))
        return (rc);
      nHeader->isEmpty = false;
      nHeader->num_keys++;
      nHeader->freeSlotIndex = entries[index].nextSlot;
      if(prevInsertIndex == BEGINNING_OF_SLOTS){
        entries[index].nextSlot = nHeader->firstSlotIndex;
        nHeader->firstSlotIndex = index;
      }
      else{
        entries[index].nextSlot = entries[prevInsertIndex].nextSlot;
        entries[prevInsertIndex].nextSlot = index;
      }
    }

    // if it is a duplicate, add a new page if new, or add it to existing bucket:
    else {
      PageNum bucketPage;
      if (isDup && entries[prevInsertIndex].isValid == OCCUPIED_NEW){
        if((rc = CreateNewBucket(bucketPage))) // create a new bucket
          return (rc);
        entries[prevInsertIndex].isValid = OCCUPIED_DUP;
        RID rid2(entries[prevInsertIndex].page, entries[prevInsertIndex].slot);
        // Insert this new RID, and the existing entry into the bucket
        if((rc = InsertIntoBucket(bucketPage, rid2)) || (rc = InsertIntoBucket(bucketPage, rid)))
          return (rc);
        entries[prevInsertIndex].page = bucketPage; // page now points to bucket
      }
      else{
        bucketPage = entries[prevInsertIndex].page;
        if((rc = InsertIntoBucket(bucketPage, rid))) // insert in existing bucket
          return (rc);
      }
      
    }

  }
  else{ // Otherwise, this is a internal node
    // Get its contents, and find the insert location
    struct IX_NodeHeader_I *nIHeader = (struct IX_NodeHeader_I *)nHeader;
    PageNum nextNodePage;
    int prevInsertIndex = BEGINNING_OF_SLOTS;
    bool isDup;
    if((rc = FindNodeInsertIndex(nHeader, pData, prevInsertIndex, isDup)))
      return (rc);
    if(prevInsertIndex == BEGINNING_OF_SLOTS)
      nextNodePage = nIHeader->firstPage;
    else{
      nextNodePage = entries[prevInsertIndex].page;
    }

    // Read this next page to insert into.
    PF_PageHandle nextNodePH;
    struct IX_NodeHeader *nextNodeHeader;
    int newKeyIndex;
    PageNum newPageNum;
    if((rc = pfh.GetThisPage(nextNodePage, nextNodePH)) || (rc = nextNodePH.GetData((char *&)nextNodeHeader)))
      return (rc);
    // If this next node is full, the split the node
    if(nextNodeHeader->num_keys == header.maxKeys_N){
      if((rc = SplitNode(nHeader, nextNodeHeader, nextNodePage, prevInsertIndex, newKeyIndex, newPageNum)))
        return (rc);
      char *value = keys + newKeyIndex*header.attr_length;

      // check which of the two split nodes to insert into.
      int compared = comparator(pData, (void *)value, header.attr_length);
      if(compared >= 0){
        PageNum nextPage = newPageNum;
        if((rc = pfh.MarkDirty(nextNodePage)) || (rc = pfh.UnpinPage(nextNodePage)))
          return (rc);
        if((rc = pfh.GetThisPage(nextPage, nextNodePH)) || (rc = nextNodePH.GetData((char *&) nextNodeHeader)))
          return (rc);
        nextNodePage = nextPage;
      }
    }
    // Insert into the following node, then mark it dirty and unpin it
    if((rc = InsertIntoNonFullNode(nextNodeHeader, nextNodePage, pData, rid)))
      return (rc);
    if((rc = pfh.MarkDirty(nextNodePage)) || (rc = pfh.UnpinPage(nextNodePage)))
      return (rc);

  }
  return (rc);
}

/*
 * This finds the index in a node in which to insert a key into, given the node
 * header and the key to insert. It returns the index to insert into, and whether
 * there already exists a key of this value in this particular node.
 */
RC IX_IndexHandle::FindNodeInsertIndex(struct IX_NodeHeader *nHeader, 
  void *pData, int& index, bool& isDup){
  // Setup 
  struct Node_Entry *entries = (struct Node_Entry *)((char *)nHeader + header.entryOffset_N);
  char *keys = ((char *)nHeader + header.keysOffset_N);

  // Search until we reach a key in the node that is greater than the pData entered
  int prev_idx = BEGINNING_OF_SLOTS;
  int curr_idx = nHeader->firstSlotIndex;
  isDup = false;
  while(curr_idx != NO_MORE_SLOTS){
    char *value = keys + header.attr_length * curr_idx;
    int compared = comparator(pData, (void*) value, header.attr_length);
    if(compared == 0)
      isDup = true;
    if(compared < 0)
      break;
    prev_idx = curr_idx;
    curr_idx = entries[prev_idx].nextSlot;

  } 
  index = prev_idx;
  return (0);
}


/*
 * Given a key and a RID, delete that entry in the index. If it
 * does not exist, return IX_INVALIDENTRY. 
 */
RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid){
  RC rc = 0;
  if(! isValidIndexHeader() || isOpenHandle == false)
    return (IX_INVALIDINDEXHANDLE);

  // get root page
  struct IX_NodeHeader *rHeader;
  if((rc = rootPH.GetData((char *&)rHeader))){
    printf("failing here\n");
    return (rc);
  }

  // If the root page is empty, then no entries can exist
  if(rHeader->isEmpty && (! rHeader->isLeafNode) )
    return (IX_INVALIDENTRY);
  if(rHeader->num_keys== 0 && rHeader->isLeafNode)
    return (IX_INVALIDENTRY);

  // toDelete is an indicator for whether to delete this current node
  // because it has no more contents
  bool toDelete = false; 
  if((rc = DeleteFromNode(rHeader, pData, rid, toDelete))) // Delete the value from this node
    return (rc);

  // If the tree is empty, set the current node to a leaf node.
  if(toDelete){
    rHeader->isLeafNode = true;
  }

  return (rc);
}


/*
 * This function deletes a entry RID/key from a node given its node Header. It returns
 * a boolean toDelete that indicates whether the current node is empty or not, to signal
 * to the caller to delete this node
 */
RC IX_IndexHandle::DeleteFromNode(struct IX_NodeHeader *nHeader, void *pData, const RID &rid, bool &toDelete){
  RC rc = 0;
  toDelete = false;
  if(nHeader->isLeafNode){ // If it's a leaf node, delete it from there
    if((rc = DeleteFromLeaf((struct IX_NodeHeader_L *)nHeader, pData, rid, toDelete))){
      return (rc);
    }
  }
  // else, find the appropriate child node, and delete it from there
  else{
    int prevIndex, currIndex;
    bool isDup;  // Find the index of the chil dnode
    if((rc = FindNodeInsertIndex(nHeader, pData, currIndex, isDup)))
      return (rc);
    struct IX_NodeHeader_I *iHeader = (struct IX_NodeHeader_I *)nHeader;
    struct Node_Entry *entries = (struct Node_Entry *)((char *)nHeader + header.entryOffset_N);
    
    PageNum nextNodePage;
    bool useFirstPage = false;
    if(currIndex == BEGINNING_OF_SLOTS){ // Use the first slot in the internal node
      useFirstPage = true;               // as the child that contains this value
      nextNodePage = iHeader->firstPage;
      prevIndex = currIndex;
    }
    else{ // Otherwise, go down the appropraite page. Also retrieve the index of the
          // page before this index for deletion purposes
      if((rc = FindPrevIndex(nHeader, currIndex, prevIndex)))
        return (rc);
      nextNodePage = entries[currIndex].page;
    }

    // Acquire the contents of this child page
    PF_PageHandle nextNodePH;
    struct IX_NodeHeader *nextHeader;
    if((rc = pfh.GetThisPage(nextNodePage, nextNodePH)) || (rc = nextNodePH.GetData((char *&)nextHeader)))
      return (rc);
    bool toDeleteNext = false; // indicator for deleting the child page
    rc = DeleteFromNode(nextHeader, pData, rid, toDeleteNext); // Delete from this child page

    RC rc2 = 0;
    if((rc2 = pfh.MarkDirty(nextNodePage)) || (rc2 = pfh.UnpinPage(nextNodePage)))
      return (rc2);

    if(rc == IX_INVALIDENTRY) // If the entry was not found, tell the caller
      return (rc);

    // If the entry was successfully deleted, check whether to delete this child node
    if(toDeleteNext){
      if((rc = pfh.DisposePage(nextNodePage))){ // if so, dispose of page
        return (rc);
      }
      if(useFirstPage == false){ // If the deleted page was the first page, put the 
                                  // following page into the firstPage slot
        if(prevIndex == BEGINNING_OF_SLOTS)
          nHeader->firstSlotIndex = entries[currIndex].nextSlot;
        else
          entries[prevIndex].nextSlot = entries[currIndex].nextSlot;
        entries[currIndex].nextSlot = nHeader->freeSlotIndex;
        nHeader->freeSlotIndex = currIndex;
      }
      else{ // Otherwise, just delete this page from the sequence of slot pointers
        int firstslot = nHeader->firstSlotIndex;
        nHeader->firstSlotIndex = entries[firstslot].nextSlot;
        iHeader->firstPage = entries[firstslot].page;
        entries[firstslot].nextSlot = nHeader->freeSlotIndex;
        nHeader->freeSlotIndex = firstslot;
      }
      // update counters of this node's contents
      if(nHeader->num_keys == 0){ // If there are no more keys, and we just deleted
        nHeader->isEmpty = true;  // the first page, return the indicator to delete this
        toDelete = true;          // node
      }
      else
        nHeader->num_keys--;
      
    }

  }

  return (rc);
}

/*
 * Given a node header, and a valid index, returns the index of the slot
 * directly preceding the given one in prevIndex.
 */
RC IX_IndexHandle::FindPrevIndex(struct IX_NodeHeader *nHeader, int thisIndex, int &prevIndex){
  struct Node_Entry *entries = (struct Node_Entry *)((char *)nHeader + header.entryOffset_N);
  int prev_idx = BEGINNING_OF_SLOTS;
  int curr_idx = nHeader->firstSlotIndex;
  while(curr_idx != thisIndex){
    prev_idx = curr_idx;
    curr_idx = entries[prev_idx].nextSlot;
  } 
  prevIndex = prev_idx;
  return (0);
}

/*
 * This function deletes an entry from a leaf given the header of the leaf. It returns
 * in toDelete whether this leaf node is empty, and whether to delete it
 */
RC IX_IndexHandle::DeleteFromLeaf(struct IX_NodeHeader_L *nHeader, void *pData, const RID &rid, bool &toDelete){
  RC rc = 0;
  int prevIndex, currIndex;
  bool isDup; // find index of deletion
  if((rc = FindNodeInsertIndex((struct IX_NodeHeader *)nHeader, pData, currIndex, isDup)))
    return (rc);
  if(isDup == false) // If this entry exists, a key should exist for it in the leaf
    return (IX_INVALIDENTRY);

  // Setup 
  struct Node_Entry *entries = (struct Node_Entry *)((char *)nHeader + header.entryOffset_N);
  char *key = (char *)nHeader + header.keysOffset_N;

  if(currIndex== nHeader->firstSlotIndex) // Set up previous index for deletion of key
    prevIndex = currIndex;                // purposes
  else{
    if((rc = FindPrevIndex((struct IX_NodeHeader *)nHeader, currIndex, prevIndex)))
      return (rc);
  }

  // if only entry, delete it from the leaf
  if(entries[currIndex].isValid == OCCUPIED_NEW){
    PageNum ridPage;
    SlotNum ridSlot;
    if((rc = rid.GetPageNum(ridPage)) || (rc = rid.GetSlotNum(ridSlot))){
      return (rc);
    }
    // If this RID and key value don't match, then the entry is not there. Return IX_INVALIDENTRY
    int compare = comparator((void*)(key + header.attr_length*currIndex), pData, header.attr_length);
    if(ridPage != entries[currIndex].page || ridSlot != entries[currIndex].slot || compare != 0 )
      return (IX_INVALIDENTRY);

    // Otherwise, delete from leaf page
    if(currIndex == nHeader->firstSlotIndex){
      nHeader->firstSlotIndex = entries[currIndex].nextSlot;
    }
    else
      entries[prevIndex].nextSlot = entries[currIndex].nextSlot;
      
    entries[currIndex].nextSlot = nHeader->freeSlotIndex;
    nHeader->freeSlotIndex = currIndex;
    entries[currIndex].isValid = UNOCCUPIED;
    nHeader->num_keys--; // update the key counter
  }
  // if duplicate, delete it from the corresponding bucket
  else if(entries[currIndex].isValid == OCCUPIED_DUP){
    PageNum bucketNum = entries[currIndex].page;
    PF_PageHandle bucketPH;
    struct IX_BucketHeader *bHeader;
    bool deletePage = false;
    RID lastRID;
    PageNum nextBucketNum; // Retrieve the bucket header
    if((rc = pfh.GetThisPage(bucketNum, bucketPH)) || (rc = bucketPH.GetData((char *&)bHeader))){
      return (rc);
    }
    rc = DeleteFromBucket(bHeader, rid, deletePage, lastRID, nextBucketNum);
    RC rc2 = 0;
    if((rc2 = pfh.MarkDirty(bucketNum)) || (rc = pfh.UnpinPage(bucketNum)))
      return (rc2);

    if(rc == IX_INVALIDENTRY) // If it doesnt exist in the bucket, notify the function caller
      return (IX_INVALIDENTRY);

    if(deletePage){ // If we need to delete the bucket, 
      if((rc = pfh.DisposePage(bucketNum) ))
        return (rc);
      // If there are no more buckets, then place the last RID into the leaf page, and
      // update the isValid flag to OCCUPIED_NEW
      if(nextBucketNum == NO_MORE_PAGES){ 
        entries[currIndex].isValid = OCCUPIED_NEW;
        if((rc = lastRID.GetPageNum(entries[currIndex].page)) || 
          (rc = lastRID.GetSlotNum(entries[currIndex].slot)))
          return (rc);
      }
      else // otherwise, set the bucketPointer to the next bucket
        entries[currIndex].page = nextBucketNum;
    }
  }
  if(nHeader->num_keys == 0){ // If the leaf is now empty, 
    toDelete = true;          // return the indicator to delete
    // Update the leaf pointers of its previous and next neighbors
    PageNum prevPage = nHeader->prevPage; 
    PageNum nextPage = nHeader->nextPage;
    PF_PageHandle leafPH;
    struct IX_NodeHeader_L *leafHeader;
    if(prevPage != NO_MORE_PAGES){
      if((rc = pfh.GetThisPage(prevPage, leafPH))|| (rc = leafPH.GetData((char *&)leafHeader)) )
        return (rc);
      leafHeader->nextPage = nextPage;
      if((rc = pfh.MarkDirty(prevPage)) || (rc = pfh.UnpinPage(prevPage)))
        return (rc);
    }
    if(nextPage != NO_MORE_PAGES){
      if((rc = pfh.GetThisPage(nextPage, leafPH))|| (rc = leafPH.GetData((char *&)leafHeader)) )
        return (rc);
      leafHeader->prevPage = prevPage;
      if((rc = pfh.MarkDirty(nextPage)) || (rc = pfh.UnpinPage(nextPage)))
        return (rc);
    }

  }
  return (0);
}


/*
 * Given an RID and a bucketHeader, it deletes the RID from the bucket. It returns
 * an indicator for whether to delete the bucket, an RID signifying the last RID remaining
 * in the bucket, and the next Bucket page which tihs bucket points to.
 */
RC IX_IndexHandle::DeleteFromBucket(struct IX_BucketHeader *bHeader, const RID &rid, 
  bool &deletePage, RID &lastRID, PageNum &nextPage){
  RC rc = 0;
  PageNum nextPageNum = bHeader->nextBucket; 
  nextPage = bHeader->nextBucket; // set the nextBucket pointer

  struct Bucket_Entry *entries = (struct Bucket_Entry *)((char *)bHeader + header.entryOffset_B);

  if((nextPageNum != NO_MORE_PAGES)){ // If there's a bucket after this one, search in it first
    bool toDelete = false; // whether to delete the following bucket
    PF_PageHandle nextBucketPH;
    struct IX_BucketHeader *nextHeader;
    RID last;
    PageNum nextNextPage; // Get this next bucket
    if((rc = pfh.GetThisPage(nextPageNum, nextBucketPH)) || (rc = nextBucketPH.GetData((char *&)nextHeader)))
      return (rc);
    // Recursively call go delete from this bucket
    rc = DeleteFromBucket(nextHeader, rid, toDelete, last, nextNextPage); 

    int numKeysInNext = nextHeader->num_keys;
    RC rc2 = 0;
    if((rc2 = pfh.MarkDirty(nextPageNum)) || (rc2 = pfh.UnpinPage(nextPageNum)))
      return (rc2);

    // If the following bucket only has one key remaining, and there is space in this
    // bucket, then put the lastRID into this bucket, and delete the following bucket
    if(toDelete && bHeader->num_keys < header.maxKeys_B && numKeysInNext == 1){
      int loc = bHeader->freeSlotIndex;
      if((rc2 = last.GetPageNum(entries[loc].page)) || (rc2 = last.GetSlotNum(entries[loc].slot)))
        return (rc2);

      bHeader->freeSlotIndex = entries[loc].nextSlot;
      entries[loc].nextSlot = bHeader->firstSlotIndex;
      bHeader->firstSlotIndex = loc;

      bHeader->num_keys++;
      numKeysInNext = 0;
    }
    if(toDelete && numKeysInNext == 0){
      if((rc2 = pfh.DisposePage(nextPageNum)))
        return (rc2);
      bHeader->nextBucket = nextNextPage; // make this bucket point to the bucket 
                                          // the deleted bucket pointed to
    }

    if(rc == 0) // if we found the value, return
      return (0);
  }

  // Otherwise, search in this bucket
  PageNum ridPage;
  SlotNum ridSlot;
  if((rc = rid.GetPageNum(ridPage))|| (rc = rid.GetSlotNum(ridSlot)))
    return (rc);
  
  // Search through entire values
  int prevIndex = BEGINNING_OF_SLOTS;
  int currIndex = bHeader->firstSlotIndex;
  bool found = false;
  while(currIndex != NO_MORE_SLOTS){
    if(entries[currIndex].page == ridPage && entries[currIndex].slot == ridSlot){
      found = true;
      break;
    }
    prevIndex = currIndex;
    currIndex = entries[prevIndex].nextSlot;
  }

  // if found, delete from it
  if(found){
    if (prevIndex == BEGINNING_OF_SLOTS)
      bHeader->firstSlotIndex = entries[currIndex].nextSlot;
    else
      entries[prevIndex].nextSlot = entries[currIndex].nextSlot;
    entries[currIndex].nextSlot = bHeader->freeSlotIndex;
    bHeader->freeSlotIndex = currIndex;

    bHeader->num_keys--;
    // If there is one or less keys in this bucket, mark it for deletion.
    if(bHeader->num_keys == 1 || bHeader->num_keys == 0){
      int firstSlot = bHeader->firstSlotIndex;
      RID last(entries[firstSlot].page, entries[firstSlot].slot);
      lastRID = last; // Return the last RID to move to the previous bucket
      deletePage = true;
    }

    return (0);

  }
  return (IX_INVALIDENTRY); // if not found, return IX_INVALIDENTRY

}



/*
 * This function creates a new page and sets it up as a node. It returns the open
 * PF_PageHandle, the page number, and the pointer to its data. 
 * isLeaf is a boolean that signifies whether this page should be a leaf or not
 */
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
  nHeader->invalid2 = NO_MORE_PAGES;
  nHeader->firstSlotIndex = NO_MORE_SLOTS;
  nHeader->freeSlotIndex = 0;

  struct Node_Entry *entries = (struct Node_Entry *)((char*)nHeader + header.entryOffset_N);

  for(int i=0; i < header.maxKeys_N; i++){ // Sets up the slot pointers into a 
    entries[i].isValid = UNOCCUPIED;       // linked list in the freeSlotIndex list
    entries[i].page = NO_MORE_PAGES;
    if(i == (header.maxKeys_N -1))
      entries[i].nextSlot = NO_MORE_SLOTS;
    else
      entries[i].nextSlot = i+1;
  }

  return (rc);
}

/*
 * This function creates a new bucket page and sets it up as a bucket. It returns the
 * page number in page
 */
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
  struct IX_BucketHeader *bHeader = (struct IX_BucketHeader *) nData;

  bHeader->num_keys = 0;
  bHeader->firstSlotIndex = NO_MORE_SLOTS;
  bHeader->freeSlotIndex = 0;
  bHeader->nextBucket = NO_MORE_PAGES;

  struct Bucket_Entry *entries = (struct Bucket_Entry *)((char *)bHeader + header.entryOffset_B);

  for(int i=0; i < header.maxKeys_B; i++){ // Sets up the slot pointers to a linked
    if(i == (header.maxKeys_B -1))         // list in the freeSlotIndex list
      entries[i].nextSlot = NO_MORE_SLOTS;
    else
      entries[i].nextSlot = i+1;
  }
  if( (rc =pfh.MarkDirty(page)) || (rc = pfh.UnpinPage(page)))
    return (rc);

  return (rc);
}


/*
 * Forces all pages in the index only if the indexHandle refers to a valid open index
 */
RC IX_IndexHandle::ForcePages(){
  RC rc = 0;
  if (isOpenHandle == false)
    return (IX_INVALIDINDEXHANDLE);
  pfh.ForcePages();
  return (rc);
}

/*
 * Returns the Open PF_PageHandle and the page number of the first leaf page in 
 * this index
 */
RC IX_IndexHandle::GetFirstLeafPage(PF_PageHandle &leafPH, PageNum &leafPage){
  RC rc = 0;
  struct IX_NodeHeader *rHeader;
  if((rc = rootPH.GetData((char *&)rHeader))){ // retrieve header info
    return (rc);
  }

  // if root node is a leaf:
  if(rHeader->isLeafNode == true){
    leafPH = rootPH;
    leafPage = header.rootPage;
    return (0);
  }

  // Otherwise, search down by always going down the first page in each
  // internal node
  struct IX_NodeHeader_I *nHeader = (struct IX_NodeHeader_I *)rHeader;
  PageNum nextPageNum = nHeader->firstPage;
  PF_PageHandle nextPH;
  if(nextPageNum == NO_MORE_PAGES)
    return (IX_EOF);
  if((rc = pfh.GetThisPage(nextPageNum, nextPH)) || (rc = nextPH.GetData((char *&)nHeader)))
    return (rc);
  while(nHeader->isLeafNode == false){ // if it's not a leaf node, unpin it and go
    PageNum prevPage = nextPageNum;    // to its first child
    nextPageNum = nHeader->firstPage;
    if((rc = pfh.UnpinPage(prevPage)))
      return (rc);
    if((rc = pfh.GetThisPage(nextPageNum, nextPH)) || (rc = nextPH.GetData((char *&)nHeader)))
      return (rc);
  }
  leafPage = nextPageNum;
  leafPH = nextPH;

  return (rc);
}

RC IX_IndexHandle::FindRecordPage(PF_PageHandle &leafPH, PageNum &leafPage, void *key){
  RC rc = 0;
  struct IX_NodeHeader *rHeader;
  if((rc = rootPH.GetData((char *&) rHeader))){ // retrieve header info
    return (rc);
  }
  // if root node is leaf
  if(rHeader->isLeafNode == true){
    leafPH = rootPH;
    leafPage = header.rootPage;
    return (0);
  }

  struct IX_NodeHeader_I *nHeader = (struct IX_NodeHeader_I *)rHeader;
  int index = BEGINNING_OF_SLOTS;
  bool isDup = false;
  PageNum nextPageNum;
  PF_PageHandle nextPH;
  if((rc = FindNodeInsertIndex((struct IX_NodeHeader *)nHeader, key, index, isDup)))
    return (rc);
  struct Node_Entry *entries = (struct Node_Entry *)((char *)nHeader + header.entryOffset_N);
  if(index == BEGINNING_OF_SLOTS)
    nextPageNum = nHeader->firstPage;
  else
    nextPageNum = entries[index].page;
  if(nextPageNum == NO_MORE_PAGES)
    return (IX_EOF);
  
  if((rc = pfh.GetThisPage(nextPageNum, nextPH)) || (rc = nextPH.GetData((char *&)nHeader)))
    return (rc);

  while(nHeader->isLeafNode == false){
    if((rc = FindNodeInsertIndex((struct IX_NodeHeader *)nHeader, key, index, isDup)))
      return (rc);
    
    entries = (struct Node_Entry *)((char *)nHeader + header.entryOffset_N);
    PageNum prevPage = nextPageNum;
    if(index == BEGINNING_OF_SLOTS)
      nextPageNum = nHeader->firstPage;
    else
      nextPageNum = entries[index].page;
    //char *keys = (char *)nHeader + header.keysOffset_N; 
    if((rc = pfh.UnpinPage(prevPage)))
      return (rc);
    if((rc = pfh.GetThisPage(nextPageNum, nextPH)) || (rc = nextPH.GetData((char *&)nHeader)))
      return (rc);
  }
  leafPage = nextPageNum;
  leafPH = nextPH;

  return (rc);
}

/*
 * This function check that the header is a valid header based on the sizes of the attributes,
 * the number of keys, and the offsets. It returns true if it is, and false if it's not
 */
bool IX_IndexHandle::isValidIndexHeader() const{
  if(header.maxKeys_N <= 0 || header.maxKeys_B <= 0){
    printf("error 1");
    return false;
  }
  if(header.entryOffset_N != sizeof(struct IX_NodeHeader) || 
    header.entryOffset_B != sizeof(struct IX_BucketHeader)){
    printf("error 2");
    return false;
  }

  int attrLength2 = (header.keysOffset_N - header.entryOffset_N)/(header.maxKeys_N);
  if(attrLength2 != sizeof(struct Node_Entry)){
    printf("error 3");
    return false;
  }
  if((header.entryOffset_B + header.maxKeys_B * sizeof(Bucket_Entry) > PF_PAGE_SIZE) ||
    header.keysOffset_N + header.maxKeys_N * header.attr_length > PF_PAGE_SIZE)
    return false;
  return true;
}


/*
 * Calculates the number of keys in a node that it can hold based on a given 
 * attribute length.
 */
int IX_IndexHandle::CalcNumKeysNode(int attrLength){
  int body_size = PF_PAGE_SIZE - sizeof(struct IX_NodeHeader);
  return floor(1.0*body_size / (sizeof(struct Node_Entry) + attrLength));
}

/*
 * Calculates the number of entries that a bucket.
 */
int IX_IndexHandle::CalcNumKeysBucket(int attrLength){
  int body_size = PF_PAGE_SIZE - sizeof(struct IX_BucketHeader);
  return floor(1.0*body_size / (sizeof(Bucket_Entry)));
}
