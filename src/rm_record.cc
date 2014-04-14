//
// File:        rm_record.cc
// Description: RM_Record represents a record in a file
// Author:      Yifei Huang (yifei@stanford.edu)
//

#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "rm_internal.h"


RM_Record::RM_Record(){
  size = INVALID_RECORD_SIZE; // initial contents of a record
  data = NULL;
}

RM_Record::~RM_Record(){
  if(data != NULL) // delete any data associated with the record
    delete [] data;
}

/*
 * Copies contents of one RM_Record instance to another RM_Record instance
 */
RM_Record& RM_Record::operator= (const RM_Record &record){
  if (this != &record){
    if(this->data != NULL)  // make sure the memory allocation is for the
      delete [] data;       // correct size of the record
    this->size = record.size;
    this->data = new char[size];
    memcpy(this->data, record.data, record.size);
    this->rid = record.rid;
  }
  return (*this);
}

/*
 * Retrieves the pointer to the record data, only if the
 * record is associated with valid record data and size
 */
RC RM_Record::GetData(char *&pData) const {
  if(data == NULL || size == INVALID_RECORD_SIZE)
    return (RM_INVALIDRECORD);
  pData = data;
  return (0);
}

/*
 * Retrieves the RID of a record only if the RID  is valid
 */
RC RM_Record::GetRid (RID &rid) const {
  RC rc;
  if((rc = (this->rid).isValidRID()))
    return rc;
  rid = this->rid;
  return (0);
}

/*
 * Set the contents of this record object with specified data and RID.
 * the RID must be a valid RID.
 */
RC RM_Record::SetRecord(RID rec_rid, char *recData, int rec_size){
  RC rc;
  if((rc = rec_rid.isValidRID()))
    return RM_INVALIDRID;
  if(rec_size <= 0 )
    return RM_BADRECORDSIZE;
  rid = rec_rid;

  if(recData == NULL)
    return RM_INVALIDRECORD;
  size = rec_size;
  if (data != NULL)
    delete [] data;
  data = new char[rec_size];
  memcpy(data, recData, size);
  return (0);
}


