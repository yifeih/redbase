// 
// File:          ql_nodejoin.cc
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


QL_NodeJoin::QL_NodeJoin(QL_Manager &qlm, QL_Node &node1, QL_Node &node2) : 
  QL_Node(qlm), node1(node1), node2(node2){
  isOpen = false;
  listsInitialized = false;
  attrsInRecSize = 0;
  tupleLength = 0;
  condIndex = 0;
  firstNodeSize = 0;
}

QL_NodeJoin::~QL_NodeJoin(){
  if(listsInitialized = true){
    free(attrsInRec);
    free(condList);
    //free(buffer1);
    //free(buffer2);
    free(buffer);
    free(condsInNode);
  }
}

RC QL_NodeJoin::SetUpNode(int numConds){
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
  condsInNode = (int*)malloc(numConds * sizeof(int));
  memset((void*)condsInNode, 0, sizeof(condsInNode));

  int tupleLength1, tupleLength2;
  node1.GetTupleLength(tupleLength1);
  node2.GetTupleLength(tupleLength2);
  tupleLength = tupleLength1 + tupleLength2;
  firstNodeSize = tupleLength1;
  //buffer1 = (char *)malloc(tupleLength1);
  //buffer2 = (char *)malloc(tupleLength2);
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

RC QL_NodeJoin::OpenIt(){
  RC rc = 0;
  if((rc = node1.OpenIt()) || (rc = node2.OpenIt()))
    return (rc);
  if((rc = node1.GetNext(buffer)))
    return (rc);

  return (0);
}

RC QL_NodeJoin::GetNext(char *data){
  RC rc = 0;
  while(true){
    if((rc = node2.GetNext(buffer + firstNodeSize)) && rc == QL_EOI){
      // no more in buffer2, restart it, and get the next node in buf1
      if((rc = node1.GetNext(buffer)))
        return (rc);
      if((rc = node2.CloseIt()) || (rc = node2.OpenIt()))
        return (rc);
      if((rc = node2.GetNext(buffer + firstNodeSize)))
        return (rc);
    }

    RC comp = CheckConditions(buffer);
    if(comp == 0)
      break;
  }
  memcpy(data, buffer, tupleLength);
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
        return (QL_CONDNOTMET);
    }
    else{
      bool comp = condList[i].comparator((void *)(recData + offset1), condList[i].data, 
        condList[i].type, condList[i].length);
      if(comp == false)
        return (QL_CONDNOTMET);
    }
  }

  return (0);
}

RC QL_NodeJoin::GetNextRec(RM_Record &rec){
  return (QL_BADCALL);
}


RC QL_NodeJoin::CloseIt(){
  RC rc = 0;
  if((rc = node1.CloseIt()) || (rc = node2.CloseIt()))
    return (rc);
}

/*
RC QL_NodeJoin::IndexToOffset(int index, int &offset, int &length){
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

RC QL_NodeJoin::DeleteNodes(){
  // This relation has nothing to destroy
  node1.DeleteNodes();
  node2.DeleteNodes();
  if(listsInitialized == true){
    free(attrsInRec);
    free(condList);
    //free(buffer1);
    //free(buffer2);
    free(condsInNode);
    free(buffer);
  }
  listsInitialized = false;
  return (0);
}

RC QL_NodeJoin::GetAttrList(int *attrList, int &attrListSize){
  attrList = attrsInRec;
  attrListSize = attrsInRecSize;
  return (0);
}

