//
// ql_manager_stub.cc
//

// Note that for the SM component (HW3) the QL is actually a
// simple stub that will allow everything to compile.  Without
// a QL stub, we would need two parsers.

#include <cstdio>
#include <iostream>
#include <sys/times.h>
#include <sys/types.h>
#include <cassert>
#include <unistd.h>
#include "redbase.h"
#include "ql.h"
#include "sm.h"
#include "ix.h"
#include "rm.h"
#include "ql_node.h"
#include <string>
#include <set>
#include <map>

using namespace std;

//
// QL_Manager::QL_Manager(SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm)
//
// Constructor for the QL Manager
//
QL_Manager::QL_Manager(SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm) : rmm(rmm), ixm(ixm), smm(smm)
{
  // Can't stand unused variable warnings!
  assert (&smm && &ixm && &rmm);
}

//
// QL_Manager::~QL_Manager()
//
// Destructor for the QL Manager
//
QL_Manager::~QL_Manager()
{

}

RC QL_Manager::Reset(){
  relToInt.clear();
  relToAttrIndex.clear();
  attrToRel.clear();
  conditionToRel.clear();
  nAttrs = 0;
  nRels = 0;
  nConds = 0;
  condptr = NULL;
  return (0);
}

//
// Handle the select clause
//
RC QL_Manager::Select(int nSelAttrs, const RelAttr selAttrs[],
                      int nRelations, const char * const relations[],
                      int nConditions, const Condition conditions[])
{
  int i;
  RC rc = 0;

  cout << "Select\n";

  cout << "   nSelAttrs = " << nSelAttrs << "\n";
  for (i = 0; i < nSelAttrs; i++)
    cout << "   selAttrs[" << i << "]:" << selAttrs[i] << "\n";

  cout << "   nRelations = " << nRelations << "\n";
  for (i = 0; i < nRelations; i++)
    cout << "   relations[" << i << "] " << relations[i] << "\n";

  cout << "   nCondtions = " << nConditions << "\n";
  for (i = 0; i < nConditions; i++)
    cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

  // Parse - relations have no duplicates
  if((rc = ParseRelNoDup(nRelations, relations)))
    return (rc);

  Reset();
  nRels = nRelations;
  nConds = nConditions;
  condptr = conditions;

  // retrieve all relations
  relEntries = (RelCatEntry *)malloc(sizeof(RelCatEntry)*nRelations);
  memset((void*)relEntries, 0, sizeof(*relEntries));
  if((rc = smm.GetAllRels(relEntries, nRelations, relations, nAttrs, relToInt))){
    free(relEntries);
    return (rc);
  }

  // Retrieve all attributes
  attrEntries = (AttrCatEntry *)malloc(sizeof(AttrCatEntry)*nAttrs);
  int slot = 0;
  for(int i=0; i < nRelations; i++){
      string relString(relEntries[i].relName);
      relToAttrIndex.insert({relString, slot});
      if((rc = smm.GetAttrForRel(relEntries + i, attrEntries + slot, attrToRel))){
        free(relEntries);
        free(attrEntries);
        return (rc);
      }
      slot += relEntries[i].attrCount;
  }
  
  if((rc = ParseSelectAttrs(nSelAttrs, selAttrs))){
    free(relEntries);
    free(attrEntries);
    return (rc);
  }
  if((rc = ParseConditions(nConditions, conditions))){
    free(relEntries);
    free(attrEntries);
    return (rc);
  }

  
  QL_Node *topNode;
  if((rc = SetUpNodes(topNode, nSelAttrs, selAttrs)))
    return (rc);
  //QL_Node *topNode;
  //CreateNodes(topNode);

  if(bQueryPlans){
    cout << "PRINTING QUERY PLAN" <<endl;
    topNode->PrintNode(0);
  }

  // run
  if((rc = RunSelect(topNode)))
    return (rc);
  
  if((rc = CleanUpNodes(topNode)))
    return (rc);
    

  free(relEntries);
  free(attrEntries);

  return (rc);
}



RC QL_Manager::RunSelect(QL_Node *topNode){
  RC rc = 0;
  int finalTupLength;
  topNode->GetTupleLength(finalTupLength);
  int *attrList;
  int attrListSize;
  if((rc = topNode->GetAttrList(attrList, attrListSize)))
    return (rc);

  DataAttrInfo * attributes = (DataAttrInfo *)malloc(attrListSize* sizeof(DataAttrInfo));
  if((rc = SetUpPrinter(topNode, attributes)))
    return (rc);
  Printer printer(attributes, attrListSize);
  printer.PrintHeader(cout);

  if((rc = topNode->OpenIt()))
    return (rc);
  RC it_rc = 0;
  char *buffer = (char *)malloc(finalTupLength);
  it_rc = topNode->GetNext(buffer);
  while(it_rc == 0){
    printer.Print(cout, buffer);
    it_rc = topNode->GetNext(buffer);
  }

  free(buffer);
  if((rc = topNode->CloseIt()))
    return (rc);

  printer.PrintFooter(cout);

  free(attributes);

  return (0);
}

RC QL_Manager::SetUpPrinter(QL_Node *topNode, DataAttrInfo *attributes){
  RC rc = 0;
  int *attrList;
  int attrListSize;
  if((rc = topNode->GetAttrList(attrList, attrListSize)))
    return (rc);
  
  for(int i=0; i < attrListSize; i++){
    int index = attrList[i];
    memcpy(attributes[i].relName, attrEntries[index].relName, MAXNAME + 1);
    memcpy(attributes[i].attrName, attrEntries[index].attrName, MAXNAME + 1);
    attributes[i].attrType = attrEntries[index].attrType;
    attributes[i].attrLength = attrEntries[index].attrLength;
    attributes[i].indexNo = attrEntries[index].indexNo;
    int offset, length;
    if((rc = topNode->IndexToOffset(index, offset, length)))
      return (rc);
    attributes[i].offset = offset;
  }

  return (0);
}

bool QL_Manager::IsValidAttr(const RelAttr attr){
  if(attr.relName != NULL){
    string relString(attr.relName);
    string attrString(attr.attrName);
    map<string, int>::iterator it = relToInt.find(relString);
    map<string, set<string> >::iterator itattr = attrToRel.find(attrString);
    if(it != relToInt.end() && itattr != attrToRel.end()){
      set<string> relNames = (*itattr).second;
      set<string>::iterator setit = relNames.find(relString);
      if(setit != relNames.end())
        return true;
      else
        return false;
    }
    else
      return false;
  }
  else{ // attribute does not specify relation
    string attrString(attr.attrName);
    set<string> relNames = attrToRel[attrString];
    if(relNames.size() == 1){
      return true;
    }
    else{
      return false;
    }
  }
}

RC QL_Manager::SetUpNodes(QL_Node *&topNode, int nSelAttrs, const RelAttr selAttrs[]){
  RC rc = 0;
  // Set up node 1:
  if((rc = SetUpFirstNode(topNode)))
    return (rc);

  QL_Node* currNode;
  currNode = topNode;
  for(int i = 1; i < nRels; i++){
    if((rc = JoinRelation(topNode, currNode, i)))
      return (rc);
    currNode = topNode;
  }

  // add project nodes:
  if((nSelAttrs == 1 && strncmp(selAttrs[0].attrName, "*", strlen(selAttrs[0].attrName)) == 0))
    return (0);
  QL_NodeProj *projNode = new QL_NodeProj(*this, *currNode);
  projNode->SetUpNode(nSelAttrs);
  for(int i= 0 ; i < nSelAttrs; i++){
    int attrIndex = 0;
    if((rc = GetAttrCatEntryPos(selAttrs[i], attrIndex)))
      return (rc);
    if((rc = projNode->AddProj(attrIndex)))
      return (rc);
  }
  topNode = projNode;

  return (rc);
}

RC QL_Manager::JoinRelation(QL_Node *&topNode, QL_Node *currNode, int relIndex){
  RC rc = 0;
  bool useIndex = false;
  // create new relation node
  QL_NodeRel *relNode = new QL_NodeRel(*this, relEntries + relIndex);
  int *attrList = (int *)malloc(relEntries[relIndex].attrCount * sizeof(int));
  memset((void *)attrList, 0, sizeof(attrList));
  for(int i = 0;  i < relEntries[relIndex].attrCount ; i++){
    attrList[i] = 0;
  }
  string relString(relEntries[relIndex].relName);
  int start = relToAttrIndex[relString];
  for(int i = 0;  i < relEntries[relIndex].attrCount ; i++){
    attrList[i] = start + i;
  }
  relNode->SetUpNode(attrList, relEntries[relIndex].attrCount);
  free(attrList);

  int numConds;
  CountNumConditions(relIndex, numConds);

  
  // create new join node:
  QL_NodeJoin *joinNode = new QL_NodeJoin(*this, *currNode, *relNode);
  if((rc = joinNode->SetUpNode(numConds)))
    return (rc);
  topNode = joinNode;

  for(int i = 0; i < nConds; i++){
    if(conditionToRel[i] == relIndex){
      bool added = false;
      if(condptr[i].op == EQ_OP && !condptr[i].bRhsIsAttr && useIndex == false){
        int index = 0;
        if((rc = GetAttrCatEntryPos(condptr[i].lhsAttr, index) ))
          return (rc);
        if((attrEntries[index].indexNo != -1)){
          if((rc = relNode->UseIndex(index, attrEntries[index].indexNo, condptr[i].rhsValue.data) ))
            return (rc);
          added = true;
          useIndex = true;
        }
      }
      if(! added){
        if((rc = topNode->AddCondition(condptr[i], i)))
          return (rc);
      }
    }
  }

  return (rc);
}

RC QL_Manager::SetUpFirstNode(QL_Node *&topNode){
  RC rc = 0;
  bool useSelNode = false;
  bool useIndex = false;
  int relIndex = 0;

  QL_NodeRel *relNode = new QL_NodeRel(*this, relEntries);
  topNode = relNode;
  int *attrList = (int *)malloc(relEntries[relIndex].attrCount * sizeof(int));
  memset((void *)attrList, 0, sizeof(attrList));
  for(int i = 0;  i < relEntries[relIndex].attrCount ; i++){
    attrList[i] = 0;
  }
  string relString(relEntries[relIndex].relName);
  int start = relToAttrIndex[relString];
  for(int i = 0;  i < relEntries[relIndex].attrCount ; i++){
    attrList[i] = start + i;
  }
  relNode->SetUpNode(attrList, relEntries[relIndex].attrCount);
  free(attrList);

  // count the number of conditions associated with this node:
  int numConds;
  CountNumConditions(0, numConds);

  for(int i = 0 ; i < nConds; i++){
    if(conditionToRel[i] == 0){
      bool added = false;
      if(condptr[i].op == EQ_OP && !condptr[i].bRhsIsAttr && useIndex == false){
        int index = 0;
        if((rc = GetAttrCatEntryPos(condptr[i].lhsAttr, index) ))
          return (rc);
        if((attrEntries[index].indexNo != -1)){
          if((rc = relNode->UseIndex(index, attrEntries[index].indexNo, condptr[i].rhsValue.data) ))
            return (rc);
          added = true;
          useIndex = true;
        }
      }
      if(! added && !useSelNode){
        QL_NodeSel *selNode = new QL_NodeSel(*this, *relNode);
        if((rc = selNode->SetUpNode(numConds) ))
          return (rc);
        topNode = selNode;
        useSelNode = true;
      }
      if(! added){
        if((rc = topNode->AddCondition(condptr[i], i) ))
          return (rc);
      }
    }


  }
  return (0);
}

RC QL_Manager::CountNumConditions(int relIndex, int &numConds){
  RC rc = 0;
  numConds = 0;
  for(int i = 0; i < nConds; i++ ){
    if( conditionToRel[i] == relIndex)
      numConds++;
  }
  return (0);
}

RC QL_Manager::GetAttrCatEntry(const RelAttr attr, AttrCatEntry *&entry){
  RC rc = 0;
  int index = 0;
  if((rc = GetAttrCatEntryPos(attr, index)))
    return (rc);
  entry = attrEntries + index;
  return (0);
}

RC QL_Manager::GetAttrCatEntryPos(const RelAttr attr, int &index){
  string relString;
  if(attr.relName != NULL){
    string relStringTemp(attr.relName);
    relString = relStringTemp;
  }
  else{
    string attrString(attr.attrName);
    set<string> relNames = attrToRel[attrString];
    set<string>::iterator it = relNames.begin();
    relString = *it;
    //cout << attrString << " " << relString;
  }
  int relNum = relToInt[relString];
  int numAttrs = relEntries[relNum].attrCount;
  int slot = relToAttrIndex[relString];
  //printf("numAttrs %d, relNum %d \n", numAttrs, relNum);
  for(int i=0; i < numAttrs; i++){
    int comp = strncmp(attr.attrName, attrEntries[slot + i].attrName, strlen(attr.attrName));
    //printf("attrEntries[slot + i].attrName, %s \n", attrEntries[slot + i].attrName);
    //printf("attrEntries[slot + i].attrName, %s \n", attr.attrName);
    if(comp == 0){
      //printf("compop = 0");
      index = slot + i;
      return (0);
    }
  }
  return (QL_ATTRNOTFOUND);
}

RC QL_Manager::ParseSelectAttrs(int nSelAttrs, const RelAttr selAttrs[]){
  if((nSelAttrs == 1 && strncmp(selAttrs[0].attrName, "*", strlen(selAttrs[0].attrName)) == 0))
    return (0);

  for(int i=0; i < nSelAttrs; i++){
    // Check that attribute name is valid:
    if(!IsValidAttr(selAttrs[i]))
      return (QL_ATTRNOTFOUND);  

  }
  return (0);
}

RC QL_Manager::ParseConditions(int nConditions, const Condition conditions[]){
  RC rc = 0;
  for(int i=0; i < nConditions; i++){
    // Check if both are valid attributes:
    if(!IsValidAttr(conditions[i].lhsAttr)){
      return (QL_ATTRNOTFOUND);
    }
    if(!conditions[i].bRhsIsAttr){
      // check that types are the same
      AttrCatEntry *entry;
      if((rc = GetAttrCatEntry(conditions[i].lhsAttr, entry)))
        return (rc);
      if(entry->attrType != conditions[i].rhsValue.type)
        return (QL_BADCOND);
      // if it is a good condition add it to the condition list
      string relString(entry->relName);
      int relNum = relToInt[relString];
      conditionToRel.insert({i, relNum});
    }
    else{
      if(!IsValidAttr(conditions[i].rhsAttr))
        return (QL_ATTRNOTFOUND);
      AttrCatEntry *entry1;
      AttrCatEntry *entry2;
      if((rc = GetAttrCatEntry(conditions[i].lhsAttr,entry1)) || (rc = GetAttrCatEntry(conditions[i].rhsAttr, entry2)))
        return (rc);
      if(entry1->attrType != entry2->attrType)
        return (QL_BADCOND);
      // if it's a good condition, add it to the condition list
      string relString1(entry1->relName);
      string relString2(entry2->relName);
      int relNum1 = relToInt[relString1];
      int relNum2 = relToInt[relString2];
      conditionToRel.insert({i, max(relNum1, relNum2)});
    }
  }
  return (0);
}



RC QL_Manager::ParseRelNoDup(int nRelations, const char * const relations[]){
  set<string> relationSet;
  for(int i = 0; i < nRelations; i++){
    string relString(relations[i]);
    bool exists = (relationSet.find(relString) != relationSet.end());
    if(exists)
      return (QL_DUPRELATION);
    relationSet.insert(relString);
  }
  return (0);
}

//
// Insert the values into relName
//
RC QL_Manager::Insert(const char *relName,
                      int nValues, const Value values[])
{
  int i;
  RC rc = 0;

  cout << "Insert\n";

  cout << "   relName = " << relName << "\n";
  cout << "   nValues = " << nValues << "\n";
  for (i = 0; i < nValues; i++)
    cout << "   values[" << i << "]:" << values[i] << "\n";

  Reset();
  
  // Make space for relation name:
  relEntries = (RelCatEntry *)malloc(sizeof(RelCatEntry));
  memset((void*)relEntries, 0, sizeof(*relEntries));
  *relEntries = (RelCatEntry) {"\0", 0, 0, 0, 0};
  if((rc = SetUpOneRelation(relName))){
    free(relEntries);
    return (rc);
  }
  // CHECK : # of attributes entered match
  if(relEntries->attrCount != nValues){
    free(relEntries);
    return (QL_BADINSERT);
  }

  attrEntries = (AttrCatEntry *)malloc(relEntries->attrCount * sizeof(AttrCatEntry));
  memset((void*)attrEntries, 0, sizeof(*attrEntries));
  for(int i= 0 ; i < relEntries->attrCount; i++){
    *(attrEntries+i) = (AttrCatEntry) {"\0", "\0", 0, INT, 0, 0, 0};
  }

  if((rc = smm.GetAttrForRel(relEntries, attrEntries, attrToRel))){
    free(relEntries);
    free(attrEntries);
    return (rc);
  }

  // Check that the attribute types match
  bool badFormat = false;
  for(int i=0; i < nValues; i++){
    if((values[i].type != attrEntries[i].attrType))
      badFormat = true;
    if(attrEntries[i].attrType == STRING && (strlen((char *) values[i].data) > attrEntries[i].attrLength))
      badFormat = true;
  }
  if(badFormat){
    free(relEntries);
    free(attrEntries);
    return (QL_BADINSERT);
  }

  rc = InsertIntoRelation(relName, relEntries->tupleLength, nValues, values);

  free(relEntries);
  free(attrEntries);

  return (rc);
}

RC QL_Manager::InsertIntoRelation(const char *relName, int tupleLength, int nValues, const Value values[]){
  // Open relation
  RC rc = 0;
  RM_FileHandle relFH;
  if((rc = rmm.OpenFile(relName, relFH))){
    return (rc);
  }
  char *recbuf = (char *)malloc(tupleLength);

  // Create record
  CreateRecord(recbuf, attrEntries, nValues, values);

  // Insert into relation
  RID recRID;
  if((rc = relFH.InsertRec(recbuf, recRID))){
    free(recbuf);
    return (rc);
  }
  // Insert into index
  if((rc = InsertIntoIndex(recbuf, recRID))){
    free(recbuf);
    return (rc);
  }

  free(recbuf);
  // Close relation, clean up
  rc = rmm.CloseFile(relFH);
  return (rc);

}

RC QL_Manager::InsertIntoIndex(char *recbuf, RID recRID){
  RC rc = 0;
  for(int i = 0; i < relEntries->attrCount; i++){
    AttrCatEntry aEntry = attrEntries[i];
    if(aEntry.indexNo != -1){
      IX_IndexHandle ih;
      if((rc = ixm.OpenIndex(relEntries->relName, aEntry.indexNo, ih)))
        return (rc);
      if((rc = ih.InsertEntry((void *)(recbuf + aEntry.offset), recRID)))
        return (rc);
      if((rc = ixm.CloseIndex(ih)))
        return (rc);
    }
  }
  return (0);
}

RC QL_Manager::CreateRecord(char *recbuf, AttrCatEntry *aEntries, int nValues, const Value values[]){
  for(int i = 0; i < nValues; i++){
    AttrCatEntry aEntry = aEntries[i];
    memcpy(recbuf + aEntry.offset, (char *)values[i].data, aEntry.attrLength);
  }
  return (0);
}

RC QL_Manager::SetUpOneRelation(const char *relName){
  RC rc = 0;
  RelCatEntry *rEntry;
  RM_Record relRec;
  if((rc = smm.GetRelEntry(relName, relRec, rEntry))){
    return (rc);
  }
  memcpy(relEntries, rEntry, sizeof(RelCatEntry));

  nRels = 1;
  nAttrs = rEntry->attrCount;
  string relString(relName);
  relToInt.insert({relString, 0});
  relToAttrIndex.insert({relString, 0});
  return (0);
}


//
// Delete from the relName all tuples that satisfy conditions
//
RC QL_Manager::Delete(const char *relName,
                      int nConditions, const Condition conditions[])
{
  int i;

  cout << "Delete\n";

  cout << "   relName = " << relName << "\n";
  cout << "   nCondtions = " << nConditions << "\n";
  for (i = 0; i < nConditions; i++)
    cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

  RC rc = 0;
  Reset();
  condptr = conditions;
  nConds = nConditions;
  
  // Make space for relation name:
  relEntries = (RelCatEntry *)malloc(sizeof(RelCatEntry));
  memset((void*)relEntries, 0, sizeof(*relEntries));
  *relEntries = (RelCatEntry) {"\0", 0, 0, 0, 0};
  if((rc = SetUpOneRelation(relName))){
    free(relEntries);
    return (rc);
  }

  attrEntries = (AttrCatEntry *)malloc(relEntries->attrCount * sizeof(AttrCatEntry));
  memset((void*)attrEntries, 0, sizeof(*attrEntries));
  for(int i= 0 ; i < relEntries->attrCount; i++){
    *(attrEntries+i) = (AttrCatEntry) {"\0", "\0", 0, INT, 0, 0, 0};
  }

  if((rc = smm.GetAttrForRel(relEntries, attrEntries, attrToRel))){
    free(relEntries);
    free(attrEntries);
    return (rc);
  }

  if(rc = ParseConditions(nConditions, conditions)){
    free(relEntries);
    free(attrEntries);
    return (rc);
  }

  /*
  QL_Node *topNode;
  if((rc = CreateRelNode(topNode, 0, nConditions, conditions)))
    return (rc);
    */
    QL_Node *topNode;
    if((rc = SetUpFirstNode(topNode)))
    return (rc);

  if(bQueryPlans){
    cout << "PRINTING QUERY PLAN" <<endl;
    topNode->PrintNode(0);
  }
  
  if((rc = RunDelete(topNode)))
    return (rc);
  
  if(rc = CleanUpNodes(topNode))
    return (rc);

  free(relEntries);
  free(attrEntries);

  return 0;
}

RC QL_Manager::SetUpRun(Attr* attributes, RM_FileHandle &relFH){
  RC rc = 0;
  for(int i=0; i < relEntries->attrCount; i++){
    memset((void*)&attributes[i], 0, sizeof(attributes[i]));
    IX_IndexHandle ih;
    attributes[i] = (Attr) {0, 0, 0, 0, ih, NULL};
  }
  if((rc = smm.PrepareAttr(relEntries, attributes)))
    return (rc);
  if((rc = rmm.OpenFile(relEntries->relName, relFH)))
    return (rc);
  return (0);
}

RC QL_Manager::CleanUpRun(Attr* attributes, RM_FileHandle &relFH){
  RC rc = 0;
  if( (rc = rmm.CloseFile(relFH)))
    return (rc);

  // Destroy and close the pointers in Attr struct
  if((rc = smm.CleanUpAttr(attributes, relEntries->attrCount)))
    return (rc);
  return (0);
}

RC QL_Manager::RunDelete(QL_Node *topNode){
  RC rc = 0;
  int finalTupLength, attrListSize;
  topNode->GetTupleLength(finalTupLength);
  int *attrList;
  if((rc = topNode->GetAttrList(attrList, attrListSize)))
    return (rc);

  DataAttrInfo * printAttributes = (DataAttrInfo *)malloc(attrListSize* sizeof(DataAttrInfo));
  if((rc = SetUpPrinter(topNode, printAttributes)))
    return (rc);
  Printer printer(printAttributes, attrListSize);
  printer.PrintHeader(cout);


  RM_FileHandle relFH;
  Attr* attributes = (Attr *)malloc(sizeof(Attr)*relEntries->attrCount);
  if((rc = SetUpRun(attributes, relFH))){
    smm.CleanUpAttr(attributes, relEntries->attrCount);
    return (rc);
  } 
  
  if((rc = topNode->OpenIt() ))
    return (rc);

  RM_Record rec;
  RID rid;
  while(true){
    if((rc = topNode->GetNextRec(rec))){
      if (rc == QL_EOI){
        break;
      }
      return (rc);
    }
    char *pData;
    if((rc = rec.GetRid(rid)) || (rc = rec.GetData(pData)) )
      return (rc);
    printer.Print(cout, pData);
    if((rc = relFH.DeleteRec(rid)))
      return (rc);

    for(int i=0; i < relEntries->attrCount ; i++){
      if(attributes[i].indexNo != -1){
        if((rc = attributes[i].ih.DeleteEntry((void *)(pData + attributes[i].offset), rid)))
          return (rc);
      }
    }
  }
  
  if((rc = topNode->CloseIt()))
    return (rc);


  if((rc = CleanUpRun(attributes, relFH)))
    return (rc);

  printer.PrintFooter(cout);
  free(printAttributes);


  return (0);
}



RC QL_Manager::CleanUpNodes(QL_Node *topNode){
  RC rc = 0;
  if((rc = topNode->DeleteNodes()))
    return (rc);
  delete topNode;
  return (0);
}

RC QL_Manager::CheckUpdateAttrs(const RelAttr &updAttr,
                      const int bIsValue,
                      const RelAttr &rhsRelAttr,
                      const Value &rhsValue){
  RC rc = 0;
  if(!IsValidAttr(updAttr))
    return (QL_ATTRNOTFOUND);
  if(bIsValue){
    AttrCatEntry *entry;
    if((rc = GetAttrCatEntry(updAttr, entry)))
      return (rc);
    if(entry->attrType != rhsValue.type) // check types
      return (QL_BADUPDATE);
    if(entry->attrType == STRING){ // check the sizes
      int newValueSize = strlen((char *)rhsValue.data);
      if(newValueSize > entry->attrLength)
        return (QL_BADUPDATE);
    }
  }
  else{
    if(!IsValidAttr(rhsRelAttr))
      return (QL_ATTRNOTFOUND);
    AttrCatEntry *entry1;
    AttrCatEntry *entry2;
    if((rc = GetAttrCatEntry(updAttr, entry1)) || (rc = GetAttrCatEntry(rhsRelAttr, entry2)))
      return (rc);
    if(entry1->attrType != entry2->attrType)
      return (QL_BADUPDATE);
    if(entry1->attrType == STRING){ // check the sizes
      if(entry2->attrLength > entry1->attrLength)
        return (QL_BADUPDATE);
    }
  }

  return (rc);
}

RC QL_Manager::RunUpdate(QL_Node *topNode, const RelAttr &updAttr,
                      const int bIsValue,
                      const RelAttr &rhsRelAttr,
                      const Value &rhsValue){
  RC rc = 0;
  int finalTupLength, attrListSize;
  topNode->GetTupleLength(finalTupLength);
  int *attrList;
  if((rc = topNode->GetAttrList(attrList, attrListSize)))
    return (rc);

  DataAttrInfo * attributes = (DataAttrInfo *)malloc(attrListSize* sizeof(DataAttrInfo));
  if((rc = SetUpPrinter(topNode, attributes)))
    return (rc);
  Printer printer(attributes, attrListSize);
  printer.PrintHeader(cout);



  RM_FileHandle relFH;
  if((rc = rmm.OpenFile(relEntries->relName, relFH)))
    return (rc);

  int index1, index2;
  // set up update params:
  if((rc = GetAttrCatEntryPos(updAttr, index1)))
    return (rc);
  if(!bIsValue){
    if((rc = GetAttrCatEntryPos(rhsRelAttr, index2)))
      return (rc);
  }

  IX_IndexHandle ih;
  if((attrEntries[index1].indexNo != -1)){
    if((rc = ixm.OpenIndex(relEntries->relName, attrEntries[index1].indexNo, ih)))
      return (rc);
  }

  
  if((rc = topNode->OpenIt() ))
    return (rc);
  RM_Record rec;
  RID rid;
  while(true){
    if((rc = topNode->GetNextRec(rec))){
      if (rc == QL_EOI){
        break;
      }
      return (rc);
    }
    char *pData;
    if((rc = rec.GetRid(rid)) || (rc = rec.GetData(pData)) )
      return (rc);
    if(attrEntries[index1].indexNo != -1){
      if((ih.DeleteEntry(pData + attrEntries[index1].offset, rid)))
        return (rc);
    }

    if(bIsValue){
      memcpy(pData + attrEntries[index1].offset, (char *)rhsValue.data, attrEntries[index1].attrLength);
    }
    else{
      memcpy(pData + attrEntries[index1].offset, pData + attrEntries[index2].offset, attrEntries[index1].attrLength);
    }
    if((rc = relFH.UpdateRec(rec)))
      return (rc);
    printer.Print(cout, pData);
    
    if(attrEntries[index1].indexNo != -1){
      if((ih.InsertEntry(pData + attrEntries[index1].offset, rid)))
        return (rc);
    }

  }
  if((rc = topNode->CloseIt()))
    return (rc);
    
  if((attrEntries[index1].indexNo != -1)){
    if((rc = ixm.CloseIndex(ih)))
      return (rc);
  }
  if((rc = rmm.CloseFile(relFH)))
    return (rc);

  printer.PrintFooter(cout);
  free(attributes);

  return (0);
}

//
// Update from the relName all tuples that satisfy conditions
//
RC QL_Manager::Update(const char *relName,
                      const RelAttr &updAttr,
                      const int bIsValue,
                      const RelAttr &rhsRelAttr,
                      const Value &rhsValue,
                      int nConditions, const Condition conditions[])
{
  int i;

  cout << "Update\n";

  cout << "   relName = " << relName << "\n";
  cout << "   updAttr:" << updAttr << "\n";
  if (bIsValue)
    cout << "   rhs is value: " << rhsValue << "\n";
  else
    cout << "   rhs is attribute: " << rhsRelAttr << "\n";

  cout << "   nCondtions = " << nConditions << "\n";
  for (i = 0; i < nConditions; i++)
    cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

  RC rc = 0;
  Reset();
  condptr = conditions;
  nConds = nConditions;
  
  // Make space for relation name:
  relEntries = (RelCatEntry *)malloc(sizeof(RelCatEntry));
  memset((void*)relEntries, 0, sizeof(*relEntries));
  *relEntries = (RelCatEntry) {"\0", 0, 0, 0, 0};
  if((rc = SetUpOneRelation(relName))){
    free(relEntries);
    return (rc);
  }

  attrEntries = (AttrCatEntry *)malloc(relEntries->attrCount * sizeof(AttrCatEntry));
  memset((void*)attrEntries, 0, sizeof(*attrEntries));
  for(int i= 0 ; i < relEntries->attrCount; i++){
    *(attrEntries+i) = (AttrCatEntry) {"\0", "\0", 0, INT, 0, 0, 0};
  }

  if((rc = smm.GetAttrForRel(relEntries, attrEntries, attrToRel))){
    free(relEntries);
    free(attrEntries);
    return (rc);
  }

  if(rc = ParseConditions(nConditions, conditions)){
    free(relEntries);
    free(attrEntries);
    return (rc);
  }

  // Check that the update value is ok
  if(rc = CheckUpdateAttrs(updAttr, bIsValue, rhsRelAttr, rhsValue)){
    free(relEntries);
    free(attrEntries);
    return (rc);
  }

  

  QL_Node *topNode;
  /*
  if((rc = CreateRelNode(topNode, 0, nConditions, conditions)))
    return (rc);
    */
  if((rc = SetUpFirstNode(topNode)))
    return (rc);

  if(bQueryPlans){
    cout << "PRINTING QUERY PLAN" <<endl;
    topNode->PrintNode(0);
  }
  
  if((rc = RunUpdate(topNode, updAttr, bIsValue, rhsRelAttr, rhsValue)))
    return (rc);
  if(rc = CleanUpNodes(topNode))
    return (rc);
    

  free(relEntries);
  free(attrEntries);

  return 0;
}


