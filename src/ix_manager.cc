//
// File:        ix_indexmanager.cc
// Description: IX_IndexHandle handles indexes
// Author:      Yifei Huang (yifei@stanford.edu)
//

#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "ix_internal.h"
#include <climits>
#include <string>
#include <sstream>
#include <cstdio>
#include "comparators.h"


IX_Manager::IX_Manager(PF_Manager &pfm) : pfm(pfm){
}

IX_Manager::~IX_Manager(){
}

/*
 * This function checks that the parameters passed in for attrType and attrLength
 * make it a valid Index. Return true if so.
 */
bool IX_Manager::IsValidIndex(AttrType attrType, int attrLength){
  if(attrType == INT && attrLength == 4)
    return true;
  else if(attrType == FLOAT && attrLength == 4)
    return true;
  else if(attrType == STRING && attrLength > 0 && attrLength <= MAXSTRINGLEN)
    return true;
  else
    return false;
}

/*
 * This function is given a fileName and index number, and returns the name of the
 * index file to be created as a string in indexname. 
 */
RC IX_Manager::GetIndexFileName(const char *fileName, int indexNo, std::string &indexname){
  //RC rc = 0;
  std::stringstream convert;
  convert << indexNo;
  std::string idx_num = convert.str();
  indexname = std::string(fileName);
  indexname.append(".");
  indexname.append(idx_num);
  if(indexname.size() > PATH_MAX || idx_num.size() > 10) // sizes of file and index number
    return (IX_BADINDEXNAME);                            // cannot exceed certain sizes
  return (0);
}

/*
 * Creates a new index given the filename, the index number, attribute type and length.
 */
RC IX_Manager::CreateIndex(const char *fileName, int indexNo,
                   AttrType attrType, int attrLength){
  if(fileName == NULL || indexNo < 0) // Check that the file name and index number are valid
    return (IX_BADFILENAME);
  RC rc = 0;
  if(! IsValidIndex(attrType, attrLength)) // check that attribute length and type are valid
    return (IX_BADINDEXSPEC);

  // Create index file:
  std::string indexname;
  if((rc = GetIndexFileName(fileName, indexNo, indexname)))
    return (rc);
  if((rc = pfm.CreateFile(indexname.c_str()))){
    return (rc);
  }

  // Make the file
  PF_FileHandle fh;
  PF_PageHandle ph_header;
  PF_PageHandle ph_root;
  if((rc = pfm.OpenFile(indexname.c_str(), fh)))
    return (rc);
  // Calculate the keys per node and keys per bucket
  int numKeys_N = IX_IndexHandle::CalcNumKeysNode(attrLength);
  int numKeys_B = IX_IndexHandle::CalcNumKeysBucket(attrLength);

  // Create the header and root page
  PageNum headerpage;
  PageNum rootpage;
  if((rc = fh.AllocatePage(ph_header)) || (rc = ph_header.GetPageNum(headerpage))
    || (rc = fh.AllocatePage(ph_root)) || (rc = ph_root.GetPageNum(rootpage))){
    return (rc);
  }
  struct IX_IndexHeader *header;
  struct IX_NodeHeader_L *rootheader;
  struct Node_Entry *entries;
  if((rc = ph_header.GetData((char *&) header)) || (rc = ph_root.GetData((char *&) rootheader))){
    goto cleanup_and_exit; // if this fails, then destroy the file
  }

  // setup header page
  header->attr_type = attrType;
  header->attr_length = attrLength;
  header->maxKeys_N= numKeys_N;
  header->maxKeys_B = numKeys_B;
  //printf("max i: %d, max l: %d \n", numKeys_N, numKeys_B);

  header->entryOffset_N = sizeof(struct IX_NodeHeader_I);
  header->entryOffset_B = sizeof(struct IX_BucketHeader);
  header->keysOffset_N = header->entryOffset_N + numKeys_N*sizeof(struct Node_Entry);
  header->rootPage = rootpage;
  //header->firstBucket = NO_MORE_PAGES;

  // Set up the root node
  rootheader->isLeafNode = true;
  rootheader->isEmpty = true;
  rootheader->num_keys = 0;
  rootheader->nextPage = NO_MORE_PAGES;
  rootheader->prevPage = NO_MORE_PAGES;
  rootheader->firstSlotIndex = NO_MORE_SLOTS;
  rootheader->freeSlotIndex = 0;
  entries = (struct Node_Entry *) ((char *)rootheader + header->entryOffset_N);
  for(int i=0; i < header->maxKeys_N; i++){
    entries[i].isValid = UNOCCUPIED;
    entries[i].page = NO_MORE_PAGES;
    if(i == (header->maxKeys_N -1))
      entries[i].nextSlot = NO_MORE_SLOTS;
    else
      entries[i].nextSlot = i+1;
  }
  //printf("NODE CREATION: entries[0].nextSlot: %d \n", entries[0].nextSlot);

  // Mark both pages as dirty, and close the file
  cleanup_and_exit:
  RC rc2;
  if((rc2 = fh.MarkDirty(headerpage)) || (rc2 = fh.UnpinPage(headerpage)) ||
    (rc2 = fh.MarkDirty(rootpage)) || (rc2 = fh.UnpinPage(rootpage)) || (rc2 = pfm.CloseFile(fh)))
    return (rc2);

  return (rc);
}

/*
 * This function destroys a valid index given the file name and index number.
 */
RC IX_Manager::DestroyIndex(const char *fileName, int indexNo){
  RC rc;
  if(fileName == NULL || indexNo < 0)
    return (IX_BADFILENAME);
  std::string indexname;
  if((rc = GetIndexFileName(fileName, indexNo, indexname)))
    return (rc);
  if((rc = pfm.DestroyFile(indexname.c_str())))
    return (rc);
  return (0);
}

/*
 * This function sets up the private variables of an IX_IndexHandle to get it 
 * ready to refer to an open file
 */
RC IX_Manager::SetUpIH(IX_IndexHandle &ih, PF_FileHandle &fh, struct IX_IndexHeader *header){
  RC rc = 0;
  memcpy(&ih.header, header, sizeof(struct IX_IndexHeader));

  // check that this is a valid index file
  if(! IsValidIndex(ih.header.attr_type, ih.header.attr_length))
    return (IX_INVALIDINDEXFILE);

  if(! ih.isValidIndexHeader()){ // check that the header is valid
    return (rc);
  }

  if((rc = fh.GetThisPage(header->rootPage, ih.rootPH))){ // retrieve the root page
    return (rc);
  }

  if(ih.header.attr_type == INT){ // set up the comparator 
    ih.comparator = compare_int;
    ih.printer = print_int;
  }
  else if(ih.header.attr_type == FLOAT){
    ih.comparator = compare_float;
    ih.printer = print_float;
  }
  else{
    ih.comparator = compare_string;
    ih.printer = print_string;
  }

  ih.header_modified = false;
  ih.pfh = fh;
  ih.isOpenHandle = true;
  return (rc);
}


/*
 * This function, given a valid fileName and index Number, opens up the
 * index associated with it, and returns it via the indexHandle variable
 */
RC IX_Manager::OpenIndex(const char *fileName, int indexNo,
                 IX_IndexHandle &indexHandle){
  if(fileName == NULL || indexNo < 0){ // check for valid filename, and valid indexHandle
    return (IX_BADFILENAME);
  }
  if(indexHandle.isOpenHandle == true){
    return (IX_INVALIDINDEXHANDLE);
  }
  RC rc = 0;

  PF_FileHandle fh;
  std::string indexname;
  if((rc = GetIndexFileName(fileName, indexNo, indexname)) || 
    (rc = pfm.OpenFile(indexname.c_str(), fh)))
    return (rc);

  // Get first page, and set up the indexHandle
  PF_PageHandle ph;
  PageNum firstpage;
  char *pData;
  if((rc = fh.GetFirstPage(ph)) || (ph.GetPageNum(firstpage)) || (ph.GetData(pData))){
    fh.UnpinPage(firstpage);
    pfm.CloseFile(fh);
    return (rc);
  }
  struct IX_IndexHeader * header = (struct IX_IndexHeader *) pData;

  rc = SetUpIH(indexHandle, fh, header);
  RC rc2;
  if((rc2 = fh.UnpinPage(firstpage)))
    return (rc2);

  if(rc != 0){
    pfm.CloseFile(fh);
  }
  return (rc); 
}

/*
 * This function modifies the IX_IndexHandle of a file to close it.
 */
RC IX_Manager::CleanUpIH(IX_IndexHandle &indexHandle){
  if(indexHandle.isOpenHandle == false)
    return (IX_INVALIDINDEXHANDLE);
  indexHandle.isOpenHandle = false;
  return (0);
}

/*
 * Given a valid index handle, closes the file associated with it
 */
RC IX_Manager::CloseIndex(IX_IndexHandle &indexHandle){
  RC rc = 0;
  PF_PageHandle ph;
  PageNum page;
  char *pData;

  if(indexHandle.isOpenHandle == false){ // checks that it's a valid index handle
    return (IX_INVALIDINDEXHANDLE);
  }

  // rewrite the root page and unpin it
  PageNum root = indexHandle.header.rootPage;
  if((rc = indexHandle.pfh.MarkDirty(root)) || (rc = indexHandle.pfh.UnpinPage(root)))
    return (rc);

  // Check that the header is modified. If so, write that too.
  if(indexHandle.header_modified == true){
    if((rc = indexHandle.pfh.GetFirstPage(ph)) || ph.GetPageNum(page))
      return (rc);
    if((rc = ph.GetData(pData))){
      RC rc2;
      if((rc2 = indexHandle.pfh.UnpinPage(page)))
        return (rc2);
      return (rc);
    }
    memcpy(pData, &indexHandle.header, sizeof(struct IX_IndexHeader));
    if((rc = indexHandle.pfh.MarkDirty(page)) || (rc = indexHandle.pfh.UnpinPage(page)))
      return (rc);
  }

  // Close the file
  if((rc = pfm.CloseFile(indexHandle.pfh)))
    return (rc);

  if((rc = CleanUpIH(indexHandle)))
    return (rc);


  return (rc);
}