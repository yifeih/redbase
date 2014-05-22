// 
// File:          node_join.cc
// Description:   Node for join processing
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

Node_Join::Node_Join(QL_Manager &qlm, QL_Node &node1, QL_Node &node2) : 
  qlm(qlm), node1(node1), node2(node2){
  isOpen = false;
  listsInitialized = false;
  condIndex = 0;
}

Node_Join::~Node_Join(){
  if(listsInitialized = true){
    free(attrsInRec);
    free(condList);
    free(buffer1);
    free(buffer2);
  }
}

RC Node_Join::SetUpNode(int numConds){
  RC rc = 0;
  int *attrList1;
  int *attrList2;
  int attrListSize1;
  int attrListSize2;
  if((rc = node1.GetAttrList(attrList1, attrListSize1)) || 
    (rc = node2.GetAttrList(attrList2, attrListSize2)))
  attrsInRecSize = attrListSize1 + attrListSize2;
  
  attrsInRec = (int*)malloc(attrsInRecSize*sizeof(int));
  memset((void *)attrsInRec, 0, sizeof(attrsInRec));
  for(int i = 0; i < attrListSize1; i++){
    attrsInRec[i] = attrList1[i];
  }
  for(int i=0; i < attrListSize2; i++){
    attrsInRec[attrListSize1+i] = attrList2[i];
  }

  condList = (Cond *)malloc(numConds * sizeof(Cond));
  for(int i= 0; i < numConds; i++){
    condList[i] = {0, NULL, true, NULL, 0, 0, INT};
  }

  int tupleLength1, tupleLength2;
  node1.GetTupleLength(tupleLength1);
  node2.GetTupleLength(tupleLength2);
  buffer1 = (char *)malloc(tupleLength1);
  buffer2 = (char *)malloc(tupleLength2);
  listsInitialized = true;
  return (0);
}

RC Node_Join::AddCondition(const Condition condition){
  RC rc = 0;
  int index1, index2;
  int offset1, offset2;
  int length1, length2;
  if((rc = qlm.GetAttrCatEntryPos(condition.lhsAttr, index1) ) || (rc = IndexToOffset(index1, offset1, length1)))
    return (rc);
  condList[condIndex].offset1 = offset1;
  condList[condIndex].length = length1;
  condList[condIndex].type = qlm.attrEntries[index1].attrType;

  if(condition.bRhsIsAttr){
    if((rc = qlm.GetAttrCatEntryPos(condition.rhsAttr, index2)) || (rc = IndexToOffset(index2, offset2, length2)))
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
  condIndex++;

  return (0);
}

RC Node_Join::OpenIt(){
  RC rc = 0;
  if((rc = node1.OpenIt()) || (rc = node2.OpenIt()))
    return (rc);
  if((rc = node1.GetNext(buffer1)))
    return (rc);

  return (0);
}

RC Node_Join::GetNext(char *data){
  RC rc = 0;
  while(true){
    if((rc = node2.GetNext(buffer2))){
      // no more in buffer2, restart it:
      if((rc = node1.GetNext(buffer1)))
        return (rc);
      if((rc = node2.CloseIt()) || (rc = node2.OpenIt()))
        return (rc);
    }
    memasdfasdf

  }

  return (0);
}

RC Node_Join::GetNextRec(RM_Record &rec){
  return (QL_BADCALL);
}


RC Node_Join::CloseIt(){
  RC rc = 0;
  if((rc = node1.CloseIt()) || (rc = node2.CloseIt()))
    return (rc);
}

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

RC Node_Join::DeleteNodes(){
  // This relation has nothing to destroy
  node1.DeleteNodes();
  node2.DeleteNodes();
  if(listsInitialized == true){
    free(attrsInRec);
    free(condList);
    free(buffer1);
    free(buffer2);
  }
  listsInitialized = false;
  return (0);
}

RC Node_Join::GetAttrList(int *attrList, int &attrListSize){
  attrList = attrsInRec;
  attrListSize = attrsInRecSize;
  return (0);
}