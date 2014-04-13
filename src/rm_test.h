#ifndef RM_TEST_H
#define RM_TEST_H


#include "redbase.h"
#include "rm_rid.h"
#include "pf.h"

class Test_Bitmap{
public:
  Test_Bitmap (RM_FileHandle &fh);
  ~Test_Bitmap();
  RC ResetBitmap(char *bitmap, int size);
  RC SetBit(char *bitmap, int size, int bitnum);
  RC ResetBit(char *bitmap, int size, int bitnum);
  RC CheckBitSet(char *bitmap, int size, int bitnum, bool &set);
  RC GetFirstZeroBit(char *bitmap, int size, int &location);
  RC GetNextOneBit(char *bitmap, int size, int start, int &location);
  RC IsPageFull(char *bitmap, int size);
private:
  RM_FileHandle fh;
};

#endif