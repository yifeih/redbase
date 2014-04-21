//
// rm.h
//
// Record Manager component interface
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

//
// RM_Record: RM Record interface. The RM Record object stores the record
// RID and copies of its contents. (The copies are kept in an array of
// chars that RM_Record mallocs)
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

    // Sets the record with an RID, data contents, and its size
    RC SetRecord (RID rec_rid, char *recData, int size);
private:
    RID rid;        // record RID
    char * data;    // pointer to record data. This is stored in the
                    // record object, where its size is malloced
    int size;       // size of the malloc
};

// RM_FileHeader: Header for each file
struct RM_FileHeader {
  int recordSize;           // record size in file
  int numRecordsPerPage;    // calculated max # of recs per page
  int numPages;             // number of pages
  PageNum firstFreePage;    // pointer to first free object

  int bitmapOffset;         // location in bytes of where the bitmap starts
                            // in the page headers
  int bitmapSize;           // size of bitmaps in the page headers
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
    // Converts from the number of bits to the appropriate char size
    // to store those bits
    static int NumBitsToCharSize(int size);

    // Calculate the number of records per page based on the size of the
    // record to be stored
    static int CalcNumRecPerPage(int recSize);

    // Checks that the file header is valid
    bool isValidFileHeader() const;
    int getRecordSize(); // returns the record size

    // Returns the next record in rec, and the corresponding PF_PageHandle
    // in ph, given the current page and slot number from where to start
    // the search, and whether the next page should be used
    RC GetNextRecord(PageNum page, SlotNum slot, RM_Record &rec, PF_PageHandle &ph, bool nextPage);
    
    // Allocates a new page, and returns its page number in page, and the
    // pinned PageHandle in ph
    RC AllocateNewPage(PF_PageHandle &ph, PageNum &page);

    // returns true if this FH is associated with an open file
    bool isValidFH() const;

    // Wrapper around retrieving the bitmap of records and data from a pageHandle
    RC GetPageDataAndBitmap(PF_PageHandle &ph, char *&bitmap, struct RM_PageHeader *&pageheader) const;
    // Wrapper around retrieving the page and slot number from an RID
    RC GetPageNumAndSlot(const RID &rid, PageNum &page, SlotNum &slot) const;

    //These tests are helpers with bit manipulation on the page headers
    // which keep track of which slots have records and which don't
    RC ResetBitmap(char * bitmap, int size);
    RC SetBit(char * bitmap, int size, int bitnum);
    RC ResetBit(char * bitmap, int size, int bitnum);
    RC CheckBitSet(char *bitmap, int size, int bitnum, bool &set) const;
    RC GetFirstZeroBit(char *bitmap, int size, int &location);
    RC GetNextOneBit(char *bitmap, int size, int start, int &location);

    bool openedFH;
    struct RM_FileHeader header;
    PF_FileHandle pfh;
    bool header_modified;
};

//
// RM_FileScan: condition-based scan of records in the file
//
#define BEGIN_SCAN  -1 //default slot number before scan begins
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
    // Retrieves the number of records on ph's page, and returns it in
    // numRecords
    RC GetNumRecOnPage(PF_PageHandle& ph, int &numRecords);

    bool openScan; // whether this instance is currently a valid, open scan

    // save the parameters of the scan:
    RM_FileHandle* fileHandle;
    bool (*comparator) (void * , void *, AttrType, int);
    int attrOffset;
    int attrLength;
    void *value;
    AttrType attrType;
    CompOp compOp;

    // whether the scan has ended or not. This dictages whether to unpin the
    // page that the scan is on (currentPH)
    bool scanEnded;

    // The current state of the scan. currentPH is the page that's pinned
    PageNum scanPage;
    SlotNum scanSlot;
    PF_PageHandle currentPH;
    // Dictates whether to seek a record on the same page, or unpin it and
    // seek a record on the following page
    int numRecOnPage;
    int numSeenOnPage;
    bool useNextPage;
    bool hasPagePinned;
    bool initializedValue;
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
    // helper method for open scan which sets up private variables of
    // RM_FileHandle. 
    RC SetUpFH(RM_FileHandle& fileHandle, PF_FileHandle &fh, struct RM_FileHeader* header);
    // helper method for close scan which sets up private variables of
    // RM_FileHandle
    RC CleanUpFH(RM_FileHandle &fileHandle);

    PF_Manager &pfm; // reference to program's PF_Manager
};

//
// Print-error function
//
void RM_PrintError(RC rc);

#define RM_INVALIDRID           (START_RM_WARN + 0) // invalid RID
#define RM_BADRECORDSIZE        (START_RM_WARN + 1) // record size is invalid
#define RM_INVALIDRECORD        (START_RM_WARN + 2) // invalid record
#define RM_INVALIDBITOPERATION  (START_RM_WARN + 3) // invalid page header bit ops
#define RM_PAGEFULL             (START_RM_WARN + 4) // no more free slots on page
#define RM_INVALIDFILE          (START_RM_WARN + 5) // file is corrupt/not there
#define RM_INVALIDFILEHANDLE    (START_RM_WARN + 6) // filehandle is improperly set up
#define RM_INVALIDSCAN          (START_RM_WARN + 7) // scan is improperly set up
#define RM_ENDOFPAGE            (START_RM_WARN + 8) // end of a page
#define RM_EOF                  (START_RM_WARN + 9) // end of file 
#define RM_BADFILENAME          (START_RM_WARN + 10)
#define RM_LASTWARN             RM_BADFILENAME

#define RM_ERROR                (START_RM_ERR - 0) // error
#define RM_LASTERROR            RM_ERROR



#endif
