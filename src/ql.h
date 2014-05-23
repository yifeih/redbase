//
// ql.h
//   Query Language Component Interface
//

// This file only gives the stub for the QL component

#ifndef QL_H
#define QL_H


#include <string>
#include <set>
#include <map>
#include <stdlib.h>
#include <string.h>
#include "redbase.h"
#include "parser.h"
#include "rm.h"
#include "ix.h"
#include "sm.h"
#include "ql_node.h"

//
// QL_Manager: query language (DML)
//
class QL_Manager {
    friend class QL_Node;
    friend class Node_Rel;
    friend class QL_NodeJoin;
    friend class QL_NodeRel;
    friend class QL_NodeSel;
    friend class QL_NodeProj;
public:
    QL_Manager (SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm);
    ~QL_Manager();                       // Destructor

    RC Select  (int nSelAttrs,           // # attrs in select clause
        const RelAttr selAttrs[],        // attrs in select clause
        int   nRelations,                // # relations in from clause
        const char * const relations[],  // relations in from clause
        int   nConditions,               // # conditions in where clause
        const Condition conditions[]);   // conditions in where clause

    RC Insert  (const char *relName,     // relation to insert into
        int   nValues,                   // # values
        const Value values[]);           // values to insert

    RC Delete  (const char *relName,     // relation to delete from
        int   nConditions,               // # conditions in where clause
        const Condition conditions[]);   // conditions in where clause

    RC Update  (const char *relName,     // relation to update
        const RelAttr &updAttr,          // attribute to update
        const int bIsValue,              // 1 if RHS is a value, 0 if attribute
        const RelAttr &rhsRelAttr,       // attr on RHS to set LHS equal to
        const Value &rhsValue,           // or value to set attr equal to
        int   nConditions,               // # conditions in where clause
        const Condition conditions[]);   // conditions in where clause

private:
  RC Reset();
  bool IsValidAttr(const RelAttr attr);
  RC ParseRelNoDup(int nRelations, const char * const relations[]);
  RC InsertIntoRelation(const char *relName, int tupleLength, int nValues, const Value values[]);
  RC InsertIntoIndex(char *recbuf, RID recRID);
  RC CreateRecord(char *recbuf, AttrCatEntry *aEntries, int nValues, const Value values[]);
  RC ParseSelectAttrs(int nSelAttrs, const RelAttr selAttrs[]);
  RC GetAttrCatEntry(const RelAttr attr, AttrCatEntry *&entry);
  RC GetAttrCatEntryPos(const RelAttr attr, int &index);
  RC ParseConditions(int nConditions, const Condition conditions[]);
  RC SetUpOneRelation(const char *relName);
  RC CreateRelNode(QL_Node *&topNode, int relIndex, int nConditions, const Condition conditions[]);
  RC CheckUpdateAttrs(const RelAttr &updAttr,
                      const int bIsValue,
                      const RelAttr &rhsRelAttr,
                      const Value &rhsValue);
  RC RunDelete(QL_Node *topNode);
  RC RunUpdate(QL_Node *topNode, const RelAttr &updAttr,
                      const int bIsValue,
                      const RelAttr &rhsRelAttr,
                      const Value &rhsValue);
  RC CleanUpRun(Attr* attributes, RM_FileHandle &relFH);
  RC SetUpRun(Attr* attributes, RM_FileHandle &relFH);
  RC CleanUpNodes(QL_Node *topNode);
  RC RunPseudoDelete();
  RC CountNumConditions(int relIndex, int &numConds);
  RC SetUpFirstNode(QL_Node *&topNode);
  RC SetUpNodes(QL_Node *&topNode, int nSelAttrs, const RelAttr selAttrs[]);
  RC JoinRelation(QL_Node *&topNode, QL_Node *currNode, int relIndex);
  RC RunSelect(QL_Node *topNode);
  RC SetUpPrinter(QL_Node *topNode, DataAttrInfo *attributes);

  RM_Manager &rmm;
  IX_Manager &ixm;
  SM_Manager &smm;

  // helpers for select
  std::map<std::string, int> relToInt;
  std::map<std::string, std::set<std::string> > attrToRel;
  std::map<std::string, int> relToAttrIndex;
  std::map<int, int> conditionToRel;

  RelCatEntry *relEntries;
  AttrCatEntry *attrEntries;
  int nAttrs;
  int nRels;
  int nConds;

  const Condition *condptr;
  

};

//
// Print-error function
//
void QL_PrintError(RC rc);

#define QL_BADINSERT            (START_QL_WARN + 0) // invalid RID
#define QL_DUPRELATION          (START_QL_WARN + 1)
#define QL_BADSELECTATTR        (START_QL_WARN + 2)
#define QL_ATTRNOTFOUND         (START_QL_WARN + 3)
#define QL_BADCOND              (START_QL_WARN + 4)
#define QL_BADCALL              (START_QL_WARN + 5)
#define QL_CONDNOTMET           (START_QL_WARN + 6)
#define QL_EOI                  (START_QL_WARN + 7)
#define QL_LASTWARN             QL_EOI

#define QL_INVALIDDB            (START_QL_ERR - 0)
#define QL_ERROR                (START_QL_ERR - 1) // error
#define QL_LASTERROR            QL_ERROR



#endif
