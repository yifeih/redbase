// 
// File:          ql_noderel.cc
// Description:   Abstract class for query processing nodes
// Author:        Yifei Huang (yifei@stanford.edu)
//

#include <cstdio>
#include <iostream>
#include <unistd.h>
#include "redbase.h"
#include "sm.h"
#include "rm.h"
#include "ql.h"
#include "ix.h"
#include <string>
#include "ql_node.h"
#include "comparators.h"


using namespace std;

QL_NodeRel::QL_NodeRel(QL_Manager &qlm, RelCatEntry *rEntry) : QL_Node(qlm){
  relName = (char *)malloc(MAXNAME+1);
  memset((void *)relName, 0, sizeof(relName));
  memcpy(this->relName, rEntry->relName, strlen(rEntry->relName) + 1);
  relNameInitialized = true;
  listsInitialized = false;

  isOpen = false;
  tupleLength = rEntry->tupleLength;

  useIndex = false;
  indexNo = 0;
  indexAttr = 0;
  void *value = NULL;
}

QL_NodeRel::~QL_NodeRel(){
  if(relNameInitialized == true){
    free(relName);
  }
  relNameInitialized = false;
  if(listsInitialized == true){
    free(attrsInRec);
  }
  listsInitialized = false;
}


RC QL_NodeRel::OpenIt(){
  RC rc = 0;
  isOpen = true;
  if(useIndex){
    if((rc = qlm.ixm.OpenIndex(relName, indexNo, ih)))
      return (rc);
    if((rc = is.OpenScan(ih, EQ_OP, value)))
      return (rc);
    if((rc = qlm.rmm.OpenFile(relName, fh)))
      return (rc);
  }
  else{
    if((rc = qlm.rmm.OpenFile(relName, fh)))
      return (rc);
    if((rc = fs.OpenScan(fh, INT, 4, 0, NO_OP, NULL)))
      return (rc);
  }
  return (0);
}

RC QL_NodeRel::GetNext(char *data){
  RC rc = 0;
  char *recData;
  RM_Record rec;
  if((rc = RetrieveNextRec(rec, recData))){
    if(rc == RM_EOF || rc == IX_EOF){
      return (QL_EOI);
    }
    return (rc);
  }
  memcpy(data, recData, tupleLength);
  return (0);
}

RC QL_NodeRel::CloseIt(){
  RC rc = 0;
  if(useIndex){
    if((rc = qlm.rmm.CloseFile(fh)))
      return (rc);
    if((rc = is.CloseScan()))
      return (rc);
    if((rc = qlm.ixm.CloseIndex(ih )))
      return (rc);
  }
  else{
    if((rc = fs.CloseScan()) || (rc = qlm.rmm.CloseFile(fh)))
      return (rc);
  }
  isOpen = false;
  return (rc);
}

RC QL_NodeRel::RetrieveNextRec(RM_Record &rec, char *&recData){
  RC rc = 0;
  if(useIndex){
    RID rid;
    if((rc = is.GetNextEntry(rid) ))
      return (rc);
    if((rc = fh.GetRec(rid, rec) ))
      return (rc);
  }
  else{
    if((rc = fs.GetNextRec(rec)))
      return (rc);
  }
  if((rc = rec.GetData(recData)))
    return (rc);
  return (0);
}


RC QL_NodeRel::GetNextRec(RM_Record &rec){
  RC rc = 0;
  char *recData;
  if((rc = RetrieveNextRec(rec, recData))){
    if(rc == RM_EOF || rc == IX_EOF)
      return (QL_EOI);
    return (rc);
  }
  return (0);

  //return (QL_BADCALL);
}


RC QL_NodeRel::UseIndex(int attrNum, int indexNumber, void *data){
  indexNo = indexNumber;
  value = data;
  useIndex = true;
  indexAttr = attrNum;
  return (0);
}

RC QL_NodeRel::SetUpNode(int *attrs, int attrlistSize){
  RC rc = 0;
  attrsInRecSize = attrlistSize;
  attrsInRec = (int*)malloc(attrlistSize*sizeof(int));
  memset((void *)attrsInRec, 0, sizeof(attrsInRec));
  for(int i = 0; i < attrlistSize; i++){
    attrsInRec[i] = attrs[i];
  }
  listsInitialized = true;

  return (rc);
}

RC QL_NodeRel::PrintNode(int numTabs){
  for(int i=0; i < numTabs; i++){
    cout << "\t";
  }
  cout << "--REL: " << relName;
  if(useIndex){
    cout << " using index on attribute " << qlm.attrEntries[indexAttr].attrName <<
      " = ";
    if(qlm.attrEntries[indexAttr].attrType == INT){
      print_int(value, 4);
    }
    else if(qlm.attrEntries[indexAttr].attrType == FLOAT){
      print_float(value, 4);
    }
    else{
      print_string(value, strlen((char *)value));
    }
    cout << "\n";
  }
  else{
    cout << " using filescan." << endl;
  }
  return (0);
}

RC QL_NodeRel::DeleteNodes(){
  if(relNameInitialized == true){
    free(relName);
  }
  relNameInitialized = false;
  if(listsInitialized == true){
    free(attrsInRec);
  }
  listsInitialized = false;
  return (0);
}


