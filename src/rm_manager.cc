#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "rm_internal.h"


RM_Manager::RM_Manager(PF_Manager &pfm){
  // store the page manager
  
}

RM_Manager::~RM_Manager(){}

RC RM_Manager::CreateFile (const char *fileName, int recordSize) { 
  // error: check size of record 
  // open file with PF_Manager
  // store record size/filename

  // Call SetUpFileHeader

  return -1; 
}

RC RM_Manager::SetUpFileHeader(){
    // open file with pfh
  // create a file header
  // save header information/header dirty information

  // allocate a new page
  // memcpy into page buffer
  // mark page as dirty
  // unpin page
  // close page with pfh
  return -1;
}

RC RM_Manager::DestroyFile(const char *fileName) {
  // call PF_manager destroy file
  return -1; 
}

RC RM_Manager::OpenFile   (const char *fileName, RM_FileHandle &fileHandle){
  // call pf_open file
  
    // ERROR - if already exists a PM_FileHandle in this FileHAndle
  // ERROR - if invalid PM_FileHandle
  // Retrieve Header and save
  // ERROR - if header doesnt exist, or first page doesnt exist
  // Set modified boolean
  // Save PM_FileHandler
  // Unpin page

  // Check file header credentials
  // ERROR - invalidFileCredentials

  // return file handle
  return -1; 
}

RC RM_Manager::CloseFile  (RM_FileHandle &fileHandle) {
  // CHECK FILE HEADER CREDENTIALS

  // Check if modified
  // If so, get first page
  // write to it
  // mark as dirty
  // unpin
  // flush page to disk

  // set PM_FIleHAndle to NULL

  // call pf_close file
  return -1; 
}

