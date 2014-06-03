//
// sm.h
//   Data Manager Component Interface
//

#ifndef SM_H
#define SM_H

// Please do not include any other files than the ones below in this file.

#include <stdlib.h>
#include <string.h>
#include "redbase.h"  // Please don't change these lines
#include "parser.h"
#include "rm.h"
#include "ix.h"
#include "printer.h"
#include "rm_rid.h"
#include <map>
#include <string>
#include <set>

#define MAX_DB_NAME 255

// Define the catalog entry for a relation
typedef struct RelCatEntry{
  char relName[MAXNAME + 1];
  int tupleLength;
  int attrCount;
  int indexCount;
  int indexCurrNum;
  int numTuples;
  bool statsInitialized;
} RelCatEntry;

// Define catalog entry for an attribute
typedef struct AttrCatEntry{
  char relName[MAXNAME + 1];
  char attrName[MAXNAME +1];
  int offset;
  AttrType attrType;
  int attrLength;
  int indexNo;
  int attrNum;
  int numDistinct;
  float maxValue;
  float minValue;
} AttrCatEntry;

// This is used to specify information about an attribute
// during bulk loading time
typedef struct Attr{
  int offset;
  int type;
  int length;
  int indexNo;
  IX_IndexHandle ih;
  bool (*recInsert) (char *, std::string, int);
  int numDistinct;
  float maxValue;
  float minValue;
} Attr;

//
// SM_Manager: provides data management
//
class SM_Manager {
    friend class QL_Manager;
    static const int NO_INDEXES = -1;
    static const PageNum INVALID_PAGE = -1;
    static const SlotNum INVALID_SLOT = -1;
public:
    SM_Manager    (IX_Manager &ixm, RM_Manager &rmm);
    ~SM_Manager   ();                             // Destructor

    RC OpenDb     (const char *dbName);           // Open the database
    RC CloseDb    ();                             // close the database

    RC CreateTable(const char *relName,           // create relation relName
                   int        attrCount,          //   number of attributes
                   AttrInfo   *attributes);       //   attribute data
    RC CreateIndex(const char *relName,           // create an index for
                   const char *attrName);         //   relName.attrName
    RC DropTable  (const char *relName);          // destroy a relation

    RC DropIndex  (const char *relName,           // destroy index on
                   const char *attrName);         //   relName.attrName
    RC Load       (const char *relName,           // load relName from
                   const char *fileName);         //   fileName
    RC Help       ();                             // Print relations in db
    RC Help       (const char *relName);          // print schema of relName

    RC Print      (const char *relName);          // print relName contents

    RC Set        (const char *paramName,         // set parameter to
                   const char *value);            //   value

private:
  // Returns true if given attribute has valid/matching type and length
  bool isValidAttrType(AttrInfo attribute);

  // Inserts an entry about specified relName relation into relcat
  RC InsertRelCat(const char *relName, int attrCount, int recSize);

  // Insert an entry about specified attribute into attrcat
  RC InsertAttrCat(const char *relName, AttrInfo attr, int offset, int attrNum);
  // Retrieve the record and data associated with a relation entry. Return
  // error if one doesnt' exist
  RC GetRelEntry(const char *relName, RM_Record &relRec, RelCatEntry *&entry);

  // Finds the entry associated with a particular attribute
  RC FindAttr(const char *relName, const char *attrName, RM_Record &attrRec, AttrCatEntry *&entry);
  
  // Sets up print for DataAttrInfo from a file, printing relcat and printing attrcat
  RC SetUpPrint(RelCatEntry* rEntry, DataAttrInfo *attributes);
  RC SetUpRelCatAttributes(DataAttrInfo *attributes);
  RC SetUpAttrCatAttributes(DataAttrInfo *attributes);

  // Prepares the Attr array, which helps with loading
  RC PrepareAttr(RelCatEntry *rEntry, Attr* attributes);

  // Given a RelCatEntry, it populates aEntry with information about all its attribute.
  // While doing so, it also updates the attribute-to-relation mapping
  RC GetAttrForRel(RelCatEntry *relEntry, AttrCatEntry *aEntry, std::map<std::string, std::set<std::string> > &attrToRel);
  // Given a list of relations, it retrieves all the relCatEntries associated with them placing
  // them in the list specified by relEntries. It also returns the total # of attributes in all the
  // relations combined, and populates the mapping from relation name to index number in relEntries
  RC GetAllRels(RelCatEntry *relEntries, int nRelations, const char * const relations[], 
    int &attrCount, std::map<std::string, int> &relToInt);

  // Opens a file and loads it
  RC OpenAndLoadFile(RM_FileHandle &relFH, const char *fileName, Attr* attributes, 
    int attrCount, int recLength, int &loadedRecs);
  // Cleans up the Attr array after loading
  RC CleanUpAttr(Attr* attributes, int attrCount);
  float ConvertStrToFloat(char *string);
  RC PrintStats(const char *relName);
  RC CalcStats(const char *relName);

  RC PrintPageStats();
  RC ResetPageStats();

  IX_Manager &ixm;
  RM_Manager &rmm;

  RM_FileHandle relcatFH;
  RM_FileHandle attrcatFH;
  bool printIndex; // Whether to print the index or not when
                   // help is called on a specific table

  bool useQO;

  bool calcStats;
  bool printPageStats;
};

/*
 *
 */
class SM_AttrIterator{
  friend class SM_Manager;
public:
  SM_AttrIterator    ();
  ~SM_AttrIterator   ();  
  RC OpenIterator(RM_FileHandle &fh, char *relName);
  RC GetNextAttr(RM_Record &attrRec, AttrCatEntry *&entry);
  RC CloseIterator();

private:
  bool validIterator;
  RM_FileScan fs;

};

//
// Print-error function
//
void SM_PrintError(RC rc);

#define SM_CANNOTCLOSE          (START_SM_WARN + 0) // invalid RID
#define SM_BADRELNAME           (START_SM_WARN + 1)
#define SM_BADREL               (START_SM_WARN + 2)
#define SM_BADATTR              (START_SM_WARN + 3)
#define SM_INVALIDATTR          (START_SM_WARN + 4)
#define SM_INDEXEDALREADY       (START_SM_WARN + 5)
#define SM_NOINDEX              (START_SM_WARN + 6)
#define SM_BADLOADFILE          (START_SM_WARN + 7)
#define SM_BADSET               (START_SM_WARN + 8)
#define SM_LASTWARN             SM_BADSET

#define SM_INVALIDDB            (START_SM_ERR - 0)
#define SM_ERROR                (START_SM_ERR - 1) // error
#define SM_LASTERROR            SM_ERROR



#endif
