// 
// File:          node_rel.cc
// Description:   Node for relation processing
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
#include "node_comps.h"


using namespace std;


Node_Rel::Node_Rel(QL_Manager &qlm, RelCatEntry* rEntry) : QL_Node(qlm) {
  relName = (char *)malloc(MAXNAME+1);
  memset((void *)relName, 0, sizeof(relName));
  memcpy(this->relName, rEntry->relName, strlen(rEntry->relName) + 1);
  tupleLength = rEntry->tupleLength;
  isOpen = false;
  listsInitialized = false;
  relNameInitialized = true;
  useIndex = false;
  condIndex = 0;
  indexNo = 0;
}

Node_Rel::~Node_Rel(){
  if(listsInitialized == true){
    free(attrsInRec);
    free(condList);
  }
  if(relNameInitialized == true){
    free(relName);
  }
}

RC Node_Rel::SetUpRel(int *attrs, int attrlistSize, int numConds){
  attrsInRecSize = attrlistSize;
  attrsInRec = (int*)malloc(attrlistSize*sizeof(int));
  memset((void *)attrsInRec, 0, sizeof(attrsInRec));
  for(int i = 0; i < attrlistSize; i++){
    attrsInRec[i] = attrs[i];
  }
  condList = (Cond *)malloc(numConds * sizeof(Cond));
  for(int i= 0; i < numConds; i++){
    condList[i] = {0, NULL, true, NULL, 0, 0, INT};
  }
  listsInitialized = true;
    
  return (0);
}

/*
RC Node_Rel::AddCondition(const Condition condition){
  RC rc = 0;
  int index1, index2;
  int offset1, offset2;
  int length1, length2;
  if((rc = qlm.GetAttrCatEntryPos(condition.lhsAttr, index1) ) || (rc = QL_Node::IndexToOffset(index1, offset1, length1)))
    return (rc);
  condList[condIndex].offset1 = offset1;
  condList[condIndex].length = length1;
  condList[condIndex].type = qlm.attrEntries[index1].attrType;

  if(condition.bRhsIsAttr){
    if((rc = qlm.GetAttrCatEntryPos(condition.rhsAttr, index2)) || (rc = QL_Node::IndexToOffset(index2, offset2, length2)))
      return (rc);
    condList[condIndex].offset2 = offset2;
    condList[condIndex].isValue = false;
    if(length2 < condList[condIndex].length)
      condList[condIndex].length = length2;
  }
  else{
    condList[condIndex].isValue = true;
    condList[condIndex].data = condition.rhsValue.data;
  }

  switch(condition.op){
    case EQ_OP : condList[condIndex].comparator = &nequal; break;
    case LT_OP : condList[condIndex].comparator = &nless_than; break;
    case GT_OP : condList[condIndex].comparator = &ngreater_than; break;
    case LE_OP : condList[condIndex].comparator = &nless_than_or_eq_to; break;
    case GE_OP : condList[condIndex].comparator = &ngreater_than_or_eq_to; break;
    case NE_OP : condList[condIndex].comparator = &nnot_equal; break;
    default: return (QL_BADCOND);
  }

  if(condition.op == EQ_OP && !condition.bRhsIsAttr && useIndex == false
    && qlm.attrEntries[index1].indexNo != -1){
    useIndex = true;
    eqCond.offset1 = condList[condIndex].offset1;
    eqCond.comparator = condList[condIndex].comparator;
    eqCond.isValue = condList[condIndex].isValue;
    eqCond.data = condList[condIndex].data;
    eqCond.offset2 = condList[condIndex].offset2;
    eqCond.length = condList[condIndex].length;
    eqCond.type = condList[condIndex].type;
    indexNo = qlm.attrEntries[index1].indexNo;
  }
  else{
    condIndex++;
  }

  return (0);

}*/

RC Node_Rel::OpenIt(){
  RC rc = 0;
  isOpen = true;
  if(useIndex){
    if((rc = qlm.ixm.OpenIndex(relName, indexNo, ih)))
      return (rc);
    if((rc = is.OpenScan(ih, EQ_OP, eqCond.data)))
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

RC Node_Rel::GetNext(char * data){
  RC rc = 0;
  char * recData;
  RM_Record rec;
  while(true){
    if((rc = GetNextRecData(rec, recData) ))
      return (rc);

    RC cond = CheckConditions(recData);
    if(cond == 0)
      break;
  }

  memcpy(data, recData, tupleLength);
  return (0);
}

RC Node_Rel::GetNextRec(RM_Record &rec){
  RC rc = 0;
  char * recData;
  while(true){
    if((rc = GetNextRecData(rec, recData) ))
      return (rc);

    RC cond = CheckConditions(recData);
    if(cond == 0)
      break;
  }
  return (0);
}


RC Node_Rel::CheckConditions(char *recData){
  RC rc = 0;
  for(int i = 0; i < condIndex; i++){
    int offset1 = condList[i].offset1;
    if(! condList[i].isValue){
      int offset2 = condList[i].offset2;
      bool comp = condList[i].comparator((void *)(recData + offset1), (void *)(recData + offset2), 
        condList[i].type, condList[i].length);
      if(comp == false)
        return (-1);
    }
    else{
      bool comp = condList[i].comparator((void *)(recData + offset1), condList[i].data, 
        condList[i].type, condList[i].length);
      if(comp == false)
        return (-1);
    }

  }

  return (0);
}

RC Node_Rel::CloseIt(){
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
  return (0);
}

RC Node_Rel::GetNextRecData(RM_Record &rec, char *&recData){
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

RC Node_Rel::PrintNode(int numTabs){
  return (0);
}

/*
RC Node_Rel::IndexToOffset(int index, int &offset, int &length){
  offset = 0;
  for(int i=0; i < attrsInRecSize; i++){
    if(attrsInRec[i] == index){
      length = qlm.attrEntries[attrsInRec[i]].attrLength;
      return (0);
    }
    offset += qlm.attrEntries[attrsInRec[i]].attrLength;
  }
  return (QL_ATTRNOTFOUND);
}
*/

RC Node_Rel::DeleteNodes(){
  // This relation has nothing to destroy
  if(listsInitialized == true){
    free(attrsInRec);
    free(condList);
  }
  listsInitialized = false;
  if(relNameInitialized == true){
    free(relName);
  }
  relNameInitialized = false;
  return (0);
}

/*

RC Node_Rel::GetAttrList(int *attrList, int &attrListSize){
  attrList = attrsInRec;
  attrListSize = attrsInRecSize;
  return (0);
}

RC Node_Rel::GetTupleLength(int &tupleLength){
  tupleLength = this->tupleLength;
  return (0);
}
*/

