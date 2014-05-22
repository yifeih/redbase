// 
// File:          ql_nodesel.cc
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
#include "node_comps.h"

using namespace std;

QL_NodeSel::QL_NodeSel(QL_Manager &qlm, QL_Node &prevNode) : QL_Node(qlm), prevNode(prevNode) {
  isOpen = false;
  listsInitialized = false;
  tupleLength = 0;
  attrsInRecSize = 0;
  condIndex = 0;
  attrsInRecSize = 0;
}

QL_NodeSel::~QL_NodeSel(){
  if(listsInitialized == true){
    free(condList);
    free(attrsInRec);
    free(buffer);
    free(condsInNode);
  }
  listsInitialized = false;
}

RC QL_NodeSel::SetUpNode(int numConds){
  RC rc = 0;
  int *attrListPtr;
  if((rc = prevNode.GetAttrList(attrListPtr, attrsInRecSize)))
    return (rc);
  attrsInRec = (int *)malloc(attrsInRecSize*sizeof(int));
  for(int i = 0;  i < attrsInRecSize; i++){
    attrsInRec[i] = attrListPtr[i];
  }

  condList = (Cond *)malloc(numConds * sizeof(Cond));
  for(int i= 0; i < numConds; i++){
    condList[i] = {0, NULL, true, NULL, 0, 0, INT};
  }
  condsInNode = (int*)malloc(numConds * sizeof(int));
  memset((void*)condsInNode, 0, sizeof(condsInNode));

  prevNode.GetTupleLength(tupleLength);
  buffer = (char *)malloc(tupleLength);
  memset((void*)buffer, 0, sizeof(buffer));
  listsInitialized = true;
  return (0);
}

RC QL_NodeJoin::AddCondition(const Condition condition, int condNum){
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
  condsInNode[condIndex] = condNum;
  condIndex++;

  return (0);
}


RC QL_NodeSel::OpenIt(){

}

RC QL_NodeSel::GetNext(char *data){

}

RC QL_NodeSel::CloseIt(){

}


RC QL_NodeSel::GetNextRec(RM_Record &rec){
  return (QL_BADCALL);
}

RC QL_NodeSel::DeleteNodes(){
  prevNode.DeleteNodes();
  if(listsInitialized == true){
    free(condList);
    free(attrsInRec);
    free(buffer);
    free(condsInNode);
  }
  listsInitialized = false;
  return (0);
}

RC QL_NodeSel::GetAttrList(int *attrList, int &attrListSize){
  attrList = attrsInRec;
  attrListSize = attrsInRecSize;
  return (0);
}

RC QL_NodeSel::GetTupleLength(int &tupleLength){
  tupleLength = this->tupleLength;
  return (0);
}