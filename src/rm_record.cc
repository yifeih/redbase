

#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "rm_internal.h"
#include <cstdio>


RM_Record::RM_Record(){
  size = INVALID_RECORD_SIZE;
  data = NULL;
}

RM_Record::~RM_Record(){
  if(data != NULL)
    delete [] data;
}

RM_Record& RM_Record::operator= (const RM_Record &record){
  if (this != &record){
    if(this->data != NULL)
      delete [] data;
    this->size = record.size;
    this->data = new char[size];
    memcpy(this->data, record.data, record.size);
    this->rid = record.rid;
  }
  return (*this);
}
RC RM_Record::GetData(char *&pData) const {
  if(data == NULL)
    printf("no data\n");
  if(size == INVALID_RECORD_SIZE)
    printf("invalid rec size \n");
  if(data == NULL || size == INVALID_RECORD_SIZE)
    return (RM_INVALIDRECORD);
  pData = data;
  return (0);
}

RC RM_Record::GetRid (RID &rid) const {
  RC rc;
  if((rc = (this->rid).isValidRID()))
    return rc;
  rid = this->rid;
  return (0);
}

RC RM_Record::SetRecord(RID rec_rid, char *recData, int rec_size){
  RC rc;
  if((rc = rec_rid.isValidRID()))
    return RM_INVALIDRID;
  if(rec_size <= 0 )
    return RM_BADRECORDSIZE;

  rid = rec_rid;
  size = rec_size;
  if (data != NULL)
    delete [] data;
  data = new char[rec_size];
  memcpy(data, recData, size);
  return (0);
}

