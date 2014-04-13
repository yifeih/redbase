#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "rm_internal.h"

Test_Bitmap::Test_Bitmap(RM_FileHandle &fh){
  this->fh = fh;
}
RC Test_Bitmap::ResetBitmap(char *bitmap, int size){
  fh.ResetBitmap(bitmap, size);
}
RC Test_Bitmap::SetBit(char *bitmap, int size, int bitnum){
  fh.SetBit(bitmap, size, bitnum);
}
RC Test_Bitmap::ResetBit(char *bitmap, int size, int bitnum){
  fh.ResetBit(bitmap, size, bitnum);
}
RC Test_Bitmap::CheckBitSet(char *bitmap, int size, int bitnum, bool &set){
  fh.CheckBitSet(bitmap, size, bitnum, set);
}
RC Test_Bitmap::GetFirstZeroBit(char *bitmap, int size, int &location){
  fh.GetFirstZeroBit(bitmap, size, location);
}
RC Test_Bitmap::GetNextOneBit(char *bitmap, int size, int start, int &location){
  GetNextOneBit(bitmap, size, start, location);
}
RC Test_Bitmap::IsPageFull(char *bitmap, int size){
  IsPageFull(bitmap, size);
}