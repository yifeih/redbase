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
    int slotIndexOffset_i; // int values
    int slotIndexOffset_l;

    int validOffset_i; // chars
    int validOffset_l;

    int keysOffset_i; // size of values
    int keysOffset_l;

    int pagesOffset_i; //page nums
    int RIDsOffset_l; // RIDs

    int maxKeys_i; // doesn't include first page
    int maxKeys_l;

    PageNum rootPage;
};

//
// IX_IndexHandle: IX Index File interface
//
class IX_IndexHandle {
    friend class IX_Manager;
    static const int BEGINNING_OF_SLOTS = -1;
    static const int END_OF_SLOTS = -2;
    static const char UNOCCUPIED = 'u';
    static const char OCCUPIED_NEW = 'n';
    static const char OCCUPIED_REPEAT = 'r';
public:
    IX_IndexHandle();
    ~IX_IndexHandle();

    // Insert a new index entry
    RC InsertEntry(void *pData, const RID &rid);

    // Delete a new index entry
    RC DeleteEntry(void *pData, const RID &rid);

    // Force index files to disk
    RC ForcePages();
private:
    RC CreateNewInternalNode(PF_PageHandle &ph);
    RC CreateNewLeafNode(PF_PageHandle &ph);
    RC InsertIntoNonFull(PF_PageHandle &ph, void *pData, const RID &rid);
    RC SplitInternal(PF_PageHandle &old_ph, PF_PageHandle &new_ph);
    RC SplitLeaf(PF_PageHandle &old_ph, PF_PageHandle &new_ph);
    RC GetFirstNewValue(PF_PageHandle &ph, char *&value);

    RC FindHalfwayIndex(char * nextSlotIndex, int size);
    RC FindNextFreeSlot(char * validArray, int& slot, int size);
    RC FindIndexOfInsertion(int& index, PF_PageHandle &ph);
    RC UpdateNextSlotIndex(int* slotindex, int* firstPage, int before, int insert);

    bool isValidIndexHeader();
    static int CalcNumKeysInternal(int attrLength);
    static int CalcNumKeysLeaf(int attrLength);

    // Private variables
    bool isOpenHandle;
    PF_FileHandle pfh;
    bool header_modified;
    PF_PageHandle rootPage;
    struct IX_IndexHeader header;
    int (*comparator) (void * , void *, int);

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
};

//
// IX_Manager: provides IX index file management
//
class IX_Manager {
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
#define IX_EOF                  (START_IX_WARN + 5)
#define IX_LASTWARN             IX_EOF

#define IX_ERROR                (START_IX_ERR - 0) // error
#define IX_LASTERROR            IX_ERROR

#endif
