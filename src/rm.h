//
// rm.h
//
//   Record Manager component interface
//
// This file does not include the interface for the RID class.  This is
// found in rm_rid.h
//

#ifndef RM_H
#define RM_H

// Please DO NOT include any files other than redbase.h and pf.h in this
// file.  When you submit your code, the test program will be compiled
// with your rm.h and your redbase.h, along with the standard pf.h that
// was given to you.  Your rm.h, your redbase.h, and the standard pf.h
// should therefore be self-contained (i.e., should not depend upon
// declarations in any other file).

// Do not change the following includes
#include "redbase.h"
#include "rm_rid.h"
#include "pf.h"

//TO REMOVE
struct TestRec1 {
    char  str[29];
    int   num;
    float r;
};


//
// RM_Record: RM Record interface
//
class RM_Record {
    static const int INVALID_RECORD_SIZE = -1;
    friend class RM_FileHandle;
public:
    RM_Record ();
    ~RM_Record();
    RM_Record& operator= (const RM_Record &record);

    // Return the data corresponding to the record.  Sets *pData to the
    // record contents.
    RC GetData(char *&pData) const;

    // Return the RID associated with the record
    RC GetRid (RID &rid) const;
    RC SetRecord (RID rec_rid, char *recData, int size);

    RID rid;
    char * data;
    int size;
};

struct RM_FileHeader {
  int recordSize;
  int numRecordsPerPage;
  int numPages;
  PageNum firstFreePage;

  int bitmapOffset;
  int bitmapSize;
};
//
// RM_FileHandle: RM File interface
//
class RM_FileHandle {
    static const PageNum NO_FREE_PAGES = -1;
    friend class RM_Manager;
    friend class RM_FileScan;
public:
    RM_FileHandle ();
    ~RM_FileHandle();
    RM_FileHandle& operator= (const RM_FileHandle &fileHandle);

    // Given a RID, return the record
    RC GetRec     (const RID &rid, RM_Record &rec) const;

    RC InsertRec  (const char *pData, RID &rid);       // Insert a new record

    RC DeleteRec  (const RID &rid);                    // Delete a record
    RC UpdateRec  (const RM_Record &rec);              // Update a record

    // Forces a page (along with any contents stored in this class)
    // from the buffer pool to disk.  Default value forces all pages.
    RC ForcePages (PageNum pageNum = ALL_PAGES);
    //RC UnpinPage(PageNum page);
private:
    static int NumBitsToCharSize(int size);
    static int CalcNumRecPerPage(int recSize);
    bool isValidFileHeader() const;
    int getRecordSize();
    RC GetNextRecord(PageNum page, SlotNum slot, RM_Record &rec, PF_PageHandle &ph, bool nextPage);
    RC AllocateNewPage(PF_PageHandle &ph, PageNum &page);
    bool isValidFH() const;
    RC GetPageDataAndBitmap(PF_PageHandle &ph, char *&bitmap, struct RM_PageHeader *&pageheader) const;
    RC GetPageNumAndSlot(const RID &rid, PageNum &page, SlotNum &slot) const;

    RC ResetBitmap(char * bitmap, int size);
    RC SetBit(char * bitmap, int size, int bitnum);
    RC ResetBit(char * bitmap, int size, int bitnum);
    RC CheckBitSet(char *bitmap, int size, int bitnum, bool &set) const;
    RC GetFirstZeroBit(char *bitmap, int size, int &location);
    RC GetNextOneBit(char *bitmap, int size, int start, int &location);

    bool IsPageFull(char *bitmap, int size);
    void printBits(char * bitmap, int size);

    bool openedFH;
    struct RM_FileHeader header;
    PF_FileHandle pfh;
    bool header_modified;
};

//
// RM_FileScan: condition-based scan of records in the file
//
#define BEGIN_SCAN  -1
class RM_FileScan {
public:
    RM_FileScan  ();
    ~RM_FileScan ();

    RC OpenScan  (const RM_FileHandle &fileHandle,
                  AttrType   attrType,
                  int        attrLength,
                  int        attrOffset,
                  CompOp     compOp,
                  void       *value,
                  ClientHint pinHint = NO_HINT); // Initialize a file scan
    RC GetNextRec(RM_Record &rec);               // Get next matching record
    RC CloseScan ();                             // Close the scan

private:
    RC GetNumRecOnPage(PF_PageHandle& ph, int &numRecords);
    RM_FileHandle* fileHandle;
    bool (*comparator) (void * , void *, AttrType, int);
    int attrOffset;
    int attrLength;
    void *value;
    AttrType attrType;
    bool openScan;
    CompOp compOp;

    PageNum scanPage;
    SlotNum scanSlot;
    PF_PageHandle currentPH;
    bool scanEnded;

    int numRecOnPage;
    int numSeenOnPage;
    bool useNextPage;
};

//
// RM_Manager: provides RM file management
//
class RM_Manager {
public:
    RM_Manager    (PF_Manager &pfm);
    ~RM_Manager   ();

    RC CreateFile (const char *fileName, int recordSize);
    RC DestroyFile(const char *fileName);
    RC OpenFile   (const char *fileName, RM_FileHandle &fileHandle);

    RC CloseFile  (RM_FileHandle &fileHandle);
private:
    RC SetUpFH(RM_FileHandle& fileHandle, PF_FileHandle &fh, struct RM_FileHeader* header);
    RC CleanUpFH(RM_FileHandle &fileHandle);
    PF_Manager &pfm;
};

//
// Print-error function
//
void RM_PrintError(RC rc);

#define RM_INVALIDRID           (START_RM_WARN + 0)
#define RM_BADRECORDSIZE        (START_RM_WARN + 1) // record size is invalid
#define RM_INVALIDRECORD        (START_RM_WARN + 2)
#define RM_INVALIDBITOPERATION  (START_RM_WARN + 3)
#define RM_PAGEFULL             (START_RM_WARN + 4)
#define RM_INVALIDFILE          (START_RM_WARN + 5)
#define RM_INVALIDFILEHANDLE    (START_RM_WARN + 6)
#define RM_INVALIDSCAN          (START_RM_WARN + 7)
#define RM_ENDOFPAGE            (START_RM_WARN + 8)
#define RM_EOF                  (START_RM_WARN + 9) // end of file 
#define RM_LASTWARN             RM_EOF

#define RM_ERROR                (START_RM_ERR - 0) // error
#define RM_LASTERROR            RM_ERROR



#endif
