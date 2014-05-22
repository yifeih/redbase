// 
// File:          ql_node.cc
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

QL_Node::QL_Node(QL_Manager &qlm) : qlm(qlm) {

}

QL_Node::~QL_Node(){

}

RC QL_Node::IndexToOffset(int index, int &offset, int &length){
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

RC QL_Node::AddCondition(const Condition condition, int condNum){
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

RC QL_Node::GetAttrList(int *&attrList, int &attrListSize){
  attrList = attrsInRec;
  attrListSize = attrsInRecSize;
  return (0);
}

RC QL_Node::GetTupleLength(int &tupleLength){
  tupleLength = this->tupleLength;
  return (0);
}