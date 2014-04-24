//
// ix.h
//
//   Index Manager Component Interface
//

#ifndef IX_H
#define IX_H

// Please do not include any other files than the ones below in this file.

#include "redbase.h"  // Please don't change these lines
#include "rm_rid.h"  // Please don't change these lines
#include "pf.h"
#include <string>
#include <stdlib.h>


struct IX_IndexHeader{
    AttrType attr_type;
    int attr_length;

    int entryOffset_N;
    int entryOffset_B;

    int keysOffset_N;
    //int keysOffset_B;

    int maxKeys_N;
    int maxKeys_B;

    PageNum rootPage;
    PageNum firstBucket;
    //bool firstBucketCreated;
};

//
// IX_IndexHandle: IX Index File interface
//
class IX_IndexHandle {
    friend class IX_Manager;
    friend class IX_IndexScan;
    static const int BEGINNING_OF_SLOTS = -2;
    static const int END_OF_SLOTS = -3;
    static const char UNOCCUPIED = 'u';
    static const char OCCUPIED_NEW = 'n';
    static const char OCCUPIED_DUP = 'r';
public:
    IX_IndexHandle();
    ~IX_IndexHandle();

    // Insert a new index entry
    RC InsertEntry(void *pData, const RID &rid);

    // Delete a new index entry
    RC DeleteEntry(void *pData, const RID &rid);

    // Force index files to disk
    RC ForcePages();
    RC PrintBuckets(PageNum page);
    RC PrintLeafNodes(PageNum curr_page);
    RC PrintAllEntries();
    RC PrintRootPage();
    RC PrintAllEntriesString();
    RC PrintLeafNodesString(PageNum curr_page);
    RC CheckAllValuesInt(PageNum curr_page);
private:
    RC CreateNewNode(PF_PageHandle &ph, PageNum &page, char *& nData, bool isLeaf);
    RC CreateNewBucket(PageNum &page);
    //RC CreateNewNode(PF_PageHandle &ph, PageNum &page, bool isLeaf);
    //RC CreateNewLeafNode(PF_PageHandle &ph);
    RC InsertIntoNonFullNode(struct IX_NodeHeader *nHeader, PageNum thisNodeNum, void *pData, const RID &rid);
    RC InsertIntoBucket(PageNum page, const RID &rid);
    //RC SplitInternal(PF_PageHandle &old_ph, PF_PageHandle &new_ph);
    RC SplitLeaf(PF_PageHandle &old_ph, PF_PageHandle &new_ph);
    RC SplitNode(struct IX_NodeHeader *pHeader, struct IX_NodeHeader *oldHeader, PageNum oldPage, int index, 
        int &newKeyIndex, PageNum &newPageNum);
    RC SplitBucket(struct IX_NodeHeader *nHeader, struct IX_BucketHeader *oldBHeader,
  struct IX_BucketHeader *newBHeader, PageNum oldPage, PageNum newPage, int splitIndex);
    //RC GetFirstNewValue(PF_PageHandle &ph, char *&value);

    RC FindHalfwayIndex(char * nextSlotIndex, int size);
    RC FindNextFreeSlot(char * header, int& slot, bool isBucket);
    RC FindNodeInsertIndex(struct IX_NodeHeader *nHeader, 
        void* pData, int& index, bool& isDup);
    RC FindBucketInsertIndex(struct IX_BucketHeader *bHeader,
        void *pData, int &index, bool& searchNext, const RID &rid,
        int &iterloc);
    RC UpdateNextSlotIndex(int* slotindex, int* firstPage, int before, int insert);

    bool isValidIndexHeader() const;
    static int CalcNumKeysNode(int attrLength);
    static int CalcNumKeysBucket(int attrLength);

    RC GetFirstLeafPage(PF_PageHandle &leafPH, PageNum &leafPage);

    // Private variables
    bool isOpenHandle;
    PF_FileHandle pfh;
    bool header_modified;
    PF_PageHandle rootPH;
    struct IX_IndexHeader header;
    int (*comparator) (void * , void *, int);

    bool splittwice;

};

//
// IX_IndexScan: condition-based scan of index entries
//
class IX_IndexScan {
public:
    IX_IndexScan();
    ~IX_IndexScan();

    // Open index scan
    RC OpenScan(const IX_IndexHandle &indexHandle,
                CompOp compOp,
                void *value,
                ClientHint  pinHint = NO_HINT);

    // Get the next matching entry return IX_EOF if no more matching
    // entries.
    RC GetNextEntry(RID &rid);

    // Close index scan
    RC CloseScan();
private:
    bool openScan;
    IX_IndexHandle *indexHandle;
    bool (*comparator) (void *, void*, AttrType, int);
    int attrLength;
    void *value;
    AttrType attrType;
    CompOp compOp;

    bool scanEnded;

    PF_PageHandle currLeafPH;
    PF_PageHandle currBucketPH;
    PageNum currLeafNum;
    PageNum currBucketNum;

    bool hasBucketPinned;
    bool hasLeafPinned;
    bool initializedValue;
};

//
// IX_Manager: provides IX index file management
//
class IX_Manager {
    static const char UNOCCUPIED = 'u';
public:
    IX_Manager(PF_Manager &pfm);
    ~IX_Manager();

    // Create a new Index
    RC CreateIndex(const char *fileName, int indexNo,
                   AttrType attrType, int attrLength);

    // Destroy and Index
    RC DestroyIndex(const char *fileName, int indexNo);

    // Open an Index
    RC OpenIndex(const char *fileName, int indexNo,
                 IX_IndexHandle &indexHandle);

    // Close an Index
    RC CloseIndex(IX_IndexHandle &indexHandle);

private:
    bool IsValidIndex(AttrType attrType, int attrLength);
    RC GetIndexFileName(const char *fileName, int indexNo, std::string &indexname);
    RC SetUpIH(IX_IndexHandle &ih, PF_FileHandle &fh, struct IX_IndexHeader *header);
    RC CleanUpIH(IX_IndexHandle &indexHandle);
    PF_Manager &pfm;
};

//
// Print-error function
//
void IX_PrintError(RC rc);

#define IX_BADINDEXSPEC         (START_IX_WARN + 0) // invalid RID
#define IX_BADINDEXNAME         (START_IX_WARN + 1)
#define IX_INVALIDINDEXHANDLE   (START_IX_WARN + 2)
#define IX_INVALIDINDEXFILE     (START_IX_WARN + 3)
#define IX_NODEFULL             (START_IX_WARN + 4)
#define IX_BADFILENAME          (START_IX_WARN + 5)
#define IX_INVALIDBUCKET        (START_IX_WARN + 6)
#define IX_DUPLICATEENTRY       (START_IX_WARN + 7)
#define IX_INVALIDSCAN          (START_IX_WARN + 8)
#define IX_EOF                  (START_IX_WARN + 9)
#define IX_LASTWARN             IX_EOF

#define IX_ERROR                (START_IX_ERR - 0) // error
#define IX_LASTERROR            IX_ERROR

#endif
