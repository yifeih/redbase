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


using namespace std;

/*
 * Create the node by constructing with the two nodes referring to 
 * nodes to join.
 */
QL_NodeJoin::QL_NodeJoin(QL_Manager &qlm, QL_Node &node1, QL_Node &node2) : 
  QL_Node(qlm), node1(node1), node2(node2){
  isOpen = false;
  listsInitialized = false;
  attrsInRecSize = 0;
  tupleLength = 0;
  condIndex = 0;
  firstNodeSize = 0;
  gotFirstTuple = false;
  useIndexJoin = false;
}

/*
 * delete all memory
 */
QL_NodeJoin::~QL_NodeJoin(){
  if(listsInitialized = true){
    free(attrsInRec);
    free(condList);
    free(buffer);
    free(condsInNode);
  }
}

/*
 * Set up the node with a max number of conditions that the 
 * join must meet
 */
RC QL_NodeJoin::SetUpNode(int numConds){
  RC rc = 0;
  // First sets up the attribute list for this node
  // by retrieving the attribute lists for both the previous nodes
  int *attrList1;
  int *attrList2;
  int attrListSize1;
  int attrListSize2;
  if((rc = node1.GetAttrList(attrList1, attrListSize1)) || 
    (rc = node2.GetAttrList(attrList2, attrListSize2)))
    return (rc);
  attrsInRecSize = attrListSize1 + attrListSize2;
  
  attrsInRec = (int*)malloc(attrsInRecSize*sizeof(int));
  memset((void *)attrsInRec, 0, sizeof(attrsInRec));
  for(int i = 0; i < attrListSize1; i++){
    attrsInRec[i] = attrList1[i];
  }
  for(int i=0; i < attrListSize2; i++){
    attrsInRec[attrListSize1+i] = attrList2[i];
  }

  // Malloc the list of conditions to be met
  condList = (Cond *)malloc(numConds * sizeof(Cond));
  for(int i= 0; i < numConds; i++){
    condList[i] = {0, NULL, true, NULL, 0, 0, INT};
  }
  condsInNode = (int*)malloc(numConds * sizeof(int));

  int tupleLength1, tupleLength2;
  node1.GetTupleLength(tupleLength1);
  node2.GetTupleLength(tupleLength2);
  tupleLength = tupleLength1 + tupleLength2;
  firstNodeSize = tupleLength1;

  // Create the buffer to hold the tuples from the previous nodes
  buffer = (char *)malloc(tupleLength);
  memset((void*)buffer, 0, sizeof(buffer));
  listsInitialized = true;
  return (0);
}


/*
 * Open the iterator by opening previous nodes
 */
RC QL_NodeJoin::OpenIt(){
  RC rc = 0;
  if(!useIndexJoin){
    if((rc = node1.OpenIt()) || (rc = node2.OpenIt()))
      return (rc);
  }
  else{
    if((rc = node1.OpenIt() ))
      return (rc);
  }
  gotFirstTuple = false;
 
  return (0);
}

/*
 * Returns the next tuple join that satisfies the conditions
 */
RC QL_NodeJoin::GetNext(char *data){
  RC rc = 0;
  // Retrieve the first tuple, marking the start of the iterator
  if(gotFirstTuple == false && ! useIndexJoin){
    if((rc = node1.GetNext(buffer)))
      return (rc);
  }
  else if(gotFirstTuple == false && useIndexJoin){
    if((rc = node1.GetNext(buffer)))
      return (rc);
    int offset, length;
    IndexToOffset(indexAttr, offset, length);
    if((rc = node2.OpenIt(buffer + offset)))
      return (rc);
  }
  gotFirstTuple = true;
  while(true && !useIndexJoin){
    if((rc = node2.GetNext(buffer + firstNodeSize)) && rc == QL_EOI){
      // no more in buffer2, restart it, and get the next node in buf1
      if((rc = node1.GetNext(buffer)))
        return (rc);
      if((rc = node2.CloseIt()) || (rc = node2.OpenIt()))
        return (rc);
      if((rc = node2.GetNext(buffer + firstNodeSize)))
        return (rc);
    }
    // keep retrieving until condition is met
    RC comp = CheckConditions(buffer);
    if(comp == 0)
      break;
  }
  while(true && useIndexJoin){
    if((rc = node2.GetNext(buffer + firstNodeSize)) ){
      // no more in buffer 2, restart it and get the next node in buf1
      int found = false;
      while(found == false){
      if((rc = node1.GetNext(buffer)))
        return (rc);
      int offset, length;
      IndexToOffset(indexAttr, offset, length);
      if((rc = node2.CloseIt()) || (rc = node2.OpenIt(buffer + offset)))
        return (rc);
      if((rc = node2.GetNext(buffer + firstNodeSize)) && rc == QL_EOI){
       //return (rc);
        found = false;
      }
      else
        found = true;

      }
    }
    RC comp = CheckConditions(buffer);
    if (comp == 0)
      break;
  }
  memcpy(data, buffer, tupleLength);
  return (0);
}

/*
 * Don't allow join nodes to retrieve records
 */
RC QL_NodeJoin::GetNextRec(RM_Record &rec){
  return (QL_BADCALL);
}

/*
 * Close the iterator by closing previous nodes
 */
RC QL_NodeJoin::CloseIt(){
  RC rc = 0;
  if((rc = node1.CloseIt()) || (rc = node2.CloseIt()))
    return (rc);
  gotFirstTuple = false;
  return (0);
}

/*
 * Print the node, and instruct it to print its previous nodes
 */
RC QL_NodeJoin::PrintNode(int numTabs){
  for(int i=0; i < numTabs; i++){
    cout << "\t";
  }
  cout << "--JOIN: \n";
  for(int i = 0; i < condIndex; i++){
    for(int j=0; j <numTabs; j++){
      cout << "\t";
    }
    PrintCondition(qlm.condptr[condsInNode[i]]);
    cout << "\n";
  }
  node1.PrintNode(numTabs + 1);
  node2.PrintNode(numTabs + 1);
  return (0);
}

/*
 * Free all memory associated with this node, and delete the previous node
 */
RC QL_NodeJoin::DeleteNodes(){
  node1.DeleteNodes();
  node2.DeleteNodes();
  delete &node1;
  delete &node2;
  if(listsInitialized == true){
    free(attrsInRec);
    free(condList);
    free(condsInNode);
    free(buffer);
  }
  listsInitialized = false;
  return (0);
}

RC QL_NodeJoin::UseIndexJoin(int indexAttr, int subNodeAttr, int indexNumber){
  useIndexJoin = true;
  this->indexAttr = indexAttr;
  node2.UseIndex(subNodeAttr, indexNumber, NULL);
  node2.useIndexJoin= true;
  return (0);
}

bool QL_NodeJoin::IsRelNode(){
  return false;
}

RC QL_NodeJoin::OpenIt(void *data){
  return (QL_BADCALL);
}

RC QL_NodeJoin::UseIndex(int attrNum, int indexNumber, void *data){
  return (QL_BADCALL);
}

