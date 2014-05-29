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
#include "comparators.h"

using namespace std;

QL_Node::QL_Node(QL_Manager &qlm) : qlm(qlm) {

}

QL_Node::~QL_Node(){

}

/*
 * Given a condition, it prints it int he format:
 * relName.attrName <OP> <value or attribute>
 */
RC QL_Node::PrintCondition(const Condition condition){
  RC rc = 0;

  // Print the LHS attribute
  if(condition.lhsAttr.relName == NULL){
    cout << "NULL";
  }
  else
    cout << condition.lhsAttr.relName;
  cout << "." << condition.lhsAttr.attrName;

  // Print the operator
  switch(condition.op){
    case EQ_OP : cout << "="; break;
    case LT_OP : cout << "<"; break;
    case GT_OP : cout << ">"; break;
    case LE_OP : cout << "<="; break;
    case GE_OP : cout << ">="; break;
    case NE_OP : cout << "!="; break;
    default: return (QL_BADCOND);
  }
  // If the RHS is an attribute, print it
  if(condition.bRhsIsAttr){
    if(condition.rhsAttr.relName == NULL){
      cout << "NULL";
    }
    else
      cout << condition.rhsAttr.relName;
    cout << "." << condition.rhsAttr.attrName;
  }
  else{ // selects the correct printer format for this attribute
    if(condition.rhsValue.type == INT){
      print_int(condition.rhsValue.data, 4);
    }
    else if(condition.rhsValue.type == FLOAT){
      print_float(condition.rhsValue.data, 4);
    }
    else{
      print_string(condition.rhsValue.data, strlen((const char *)condition.rhsValue.data));
    }
  }

  return (0);
}


/*
 * Given the index number of the attribute in attrEntries in the QL,
 * it returns the attribute's offset and length
 */
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
    condList[condIndex].length2 = length2;
    /*
    if(length2 > condList[condIndex].length)
      condList[condIndex].length = length2;
      */
  }
  else{
    condList[condIndex].isValue = true;
    condList[condIndex].data = condition.rhsValue.data;
    condList[condIndex].length2 = strlen((char *)condition.rhsValue.data);
    //printf("is value %d \n", *(int *)condList[condIndex].data);
    //printf("the length of value is %d \n", condList[condIndex].length2);
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
  //printf("adding condition to check tuple at offset %d \n", condList[condIndex].offset1);
  condsInNode[condIndex] = condNum;
  condIndex++;

  return (0);
}


/*
 * Checks that all conditions are met given the data for a tuple.
 * Returns 0 if conditions are met
 */
RC QL_Node::CheckConditions(char *recData){
  RC rc = 0;
  for(int i = 0; i < condIndex; i++){
    int offset1 = condList[i].offset1;
    // If we are comparing this to a value
    if(! condList[i].isValue){
      // If it's not a string, or string of equal length, just compare
      if(condList[i].type != STRING || condList[i].length == condList[i].length2){
        int offset2 = condList[i].offset2;
        bool comp = condList[i].comparator((void *)(recData + offset1), (void *)(recData + offset2), 
          condList[i].type, condList[i].length);

        if(comp == false){
          return (QL_CONDNOTMET);
        }
      }
      // Depending on if the value or attribute is shorter, null terminate the shorter one and compare
      else if(condList[i].length < condList[i].length2){
        int offset2 = condList[i].offset2;
        char *shorter = (char*)malloc(condList[i].length + 1);
        memset((void *)shorter, 0, condList[i].length + 1);
        memcpy(shorter, recData + offset1, condList[i].length);
        shorter[condList[i].length] = '\0';
        bool comp = condList[i].comparator(shorter, (void*)(recData + offset2), condList[i].type, condList[i].length + 1);
        free(shorter);
        if(comp == false)
          return (QL_CONDNOTMET);
      }
      else{
        int offset2 = condList[i].offset2;
        char *shorter = (char*)malloc(condList[i].length2 + 1);
        memset((void*)shorter, 0, condList[i].length2 + 1);
        memcpy(shorter, recData + offset2, condList[i].length2);
        shorter[condList[i].length2] = '\0';
        bool comp = condList[i].comparator((void*)(recData + offset1), shorter, condList[i].type, condList[i].length2 +1);
        free(shorter);
        if(comp == false)
          return (QL_CONDNOTMET);
      }
    }
    // Else, we are comparing it to another attribute
    else{
      // If it's not a string, or string of equal length, just compare
      if(condList[i].type != STRING || condList[i].length == condList[i].length2){
        bool comp = condList[i].comparator((void *)(recData + offset1), condList[i].data, 
          condList[i].type, condList[i].length);

        if(comp == false)
          return (QL_CONDNOTMET);
      }
      // Depending on which one is shorter, null terminate the shorter one and compare
      else if(condList[i].length < condList[i].length2){
        char *shorter = (char*)malloc(condList[i].length + 1);
        memset((void *)shorter, 0, condList[i].length + 1);
        memcpy(shorter, recData + offset1, condList[i].length);
        shorter[condList[i].length] = '\0';
        bool comp = condList[i].comparator(shorter, condList[i].data, condList[i].type, condList[i].length + 1);
        free(shorter);
        if(comp == false)
          return (QL_CONDNOTMET);
      }
      else{
        char *shorter = (char*)malloc(condList[i].length2 + 1);
        memset((void*)shorter, 0, condList[i].length2 + 1);
        memcpy(shorter, condList[i].data, condList[i].length2);
        shorter[condList[i].length2] = '\0';
        bool comp = condList[i].comparator((void*)(recData + offset1), shorter, condList[i].type, condList[i].length2 +1);
        free(shorter);
        if(comp == false)
          return (QL_CONDNOTMET);
      }
    }
  }

  return (0);
}

/*
 * Returns the pointer to its attribute list, and the attribute list size
 */
RC QL_Node::GetAttrList(int *&attrList, int &attrListSize){
  attrList = attrsInRec;
  attrListSize = attrsInRecSize;
  return (0);
}

/*
 * Returns the tuple Length
 */
RC QL_Node::GetTupleLength(int &tupleLength){
  tupleLength = this->tupleLength;
  return (0);
}