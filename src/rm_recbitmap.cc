#include "rm_internal.h"

RM_RecBitmap::RM_RecBitmap(int size){
  int numChars = CharToBitmapSize(size);
  bitmap = new char[numChars];
  length = size;
  ResetBitmap();
}

RM_RecBitmap::~RM_RecBitmap(){
  delete [] bitmap;
}

RC RM_RecBitmap::SetBit(int bitnum){ 
  if (bitnum > length)
    return RM_INVALIDBITNUM;
  int chunk = bitnum /8;
  int offset = bitnum - chunk*8;
  bitmap[chunk] |= 1 << offset;
  return 0;
}

RC RM_RecBitmap::ResetBit(int bitnum) {
  if (bitnum > length)
    return RM_INVALIDBITNUM;
  int chunk = bitnum / 8;
  int offset = bitnum - chunk*8;
  bitmap[chunk] &= ~(1 << offset);
  return 0;
}

RC RM_RecBitmap::ResetBitmap() { 
  int bitmap_size = sizeof(bitmap)/sizeof(char);
  for(int i=0; i < bitmap_size; i++)
    bitmap[i] = bitmap[i] ^ bitmap[i];
  return 0;
}

int RM_RecBitmap::CharToBitmapSize(int size){
  int bitmapSize = size/8;
  if(bitmapSize * 8 < size) bitmapSize++;
  return bitmapSize;
}

RC RM_RecBitmap::GetFirstZeroBit(int &location){
  /*
  int bitmap_size = sizeof(bitmap)/sizeof(char);
  for(int i = 0; i < bitmap_size; i++){
    int mask;
    mask = mask ^ mask;
    mask++;
    for(int j = 0; j < 8; j++){
      if(bitmap[i] & )
    }
  }*/

  for(int i = 0; i < length; i++){
    int chunk = i /8;
    int offset = i - chunk*8;
    if (bitmap[chunk] & ~(1 << offset)){
      location = i;
      return 0;
    }
  }
  return RM_NORESETBITS;
}

