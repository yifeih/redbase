//
// File:         rm_internal.h
// Description:  Declarations internal to the record management component
// Authors:      Yifei Huang
//

#ifndef RM_INTERNAL_H
#define RM_INTERNAL_H

#include <cstring>
#include "rm.h"
#include "pf.h"


class RM_RecBitmap {
  
public:
  RM_RecBitmap(int size);
  ~RM_RecBitmap();

  RC SetBit(int bitnum);
  RC ResetBit(int bitnum);
  RC ResetBitmap();

  RC GetFirstZeroBit(int &location);

private:
  int CharToBitmapSize(int size);
  char * bitmap;
  int length;
};


// Define the RM page header
struct RM_PageHeader {
  PageNum nextFreePage;
  RM_RecBitmap recordBitmap;
  int numRecords;

};

#endif