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

using namespace std;

/*
 * Initializes the state of the node
 */
QL_NodeSel::QL_NodeSel(QL_Manager &qlm, QL_Node &prevNode) : QL_Node(qlm), prevNode(prevNode) {
  isOpen = false;
  listsInitialized = false;
  tupleLength = 0;
  attrsInRecSize = 0;
  condIndex = 0;
  attrsInRecSize = 0;
}

/*
 * Cleans up select node
 */
QL_NodeSel::~QL_NodeSel(){
  if(listsInitialized == true){
    free(condList);
    free(attrsInRec);
    free(buffer);
    free(condsInNode);
  }
  listsInitialized = false;
}

/*
 * Given a max number of conditions, it makes the space to hold up to
 * this number of conditions. It also initializes
 * the attribute list for this node by copying the attribute list
 * of the previous node. Finally, it makes space for the
 * buffer which is where the record data from previous nodes
 * will be held.
 */
RC QL_NodeSel::SetUpNode(int numConds){
  RC rc = 0;
  int *attrListPtr;
  if((rc = prevNode.GetAttrList(attrListPtr, attrsInRecSize)))
    return (rc);
  attrsInRec = (int *)malloc(attrsInRecSize*sizeof(int));
  for(int i = 0;  i < attrsInRecSize; i++){
    attrsInRec[i] = attrListPtr[i];
  }

  // allot space for these conditions
  condList = (Cond *)malloc(numConds * sizeof(Cond));
  for(int i= 0; i < numConds; i++){
    condList[i] = {0, NULL, true, NULL, 0, 0, INT};
  }
  condsInNode = (int*)malloc(numConds * sizeof(int));
  memset((void*)condsInNode, 0, sizeof(condsInNode));

  prevNode.GetTupleLength(tupleLength);
  buffer = (char *)malloc(tupleLength); // set up buffer
  memset((void*)buffer, 0, sizeof(buffer));
  listsInitialized = true;
  return (0);
}



/*
 * Open the iterator by opening the previous node
 */
RC QL_NodeSel::OpenIt(){
  RC rc = 0;
  if((rc = prevNode.OpenIt()))
    return (rc);
  return (0);
}

/*
 * Get the next record
 */
RC QL_NodeSel::GetNext(char *data){
  RC rc = 0;
  while(true){
    if((rc = prevNode.GetNext(buffer))){
      return (rc);
    }
    // keep retrieving records until the conditions are met
    RC cond = CheckConditions(buffer);
    if(cond == 0)
      break;
  }
  memcpy(data, buffer, tupleLength);
  return (0);
}

/*
 * Close the iterator by closing the previous node's iterator
 */
RC QL_NodeSel::CloseIt(){
  RC rc = 0;
  if((rc = prevNode.CloseIt()))
    return (rc);

  return (0);
}

/*
 * Retrieves the next record from the previous node
 */
RC QL_NodeSel::GetNextRec(RM_Record &rec){
  RC rc = 0;
  while(true){
    if((rc = prevNode.GetNextRec(rec)))
      return (rc);

    char *pData;
    if((rc = rec.GetData(pData)))
      return (rc);

    RC cond = CheckConditions(pData);
    if(cond ==0)
      break;
  }

  return (0);
}


/*
 * Print the node, and instruct it to print its previous nodes
 */
RC QL_NodeSel::PrintNode(int numTabs){
  for(int i=0; i < numTabs; i++){
    cout << "\t";
  }
  cout << "--SEL: " << endl;
  for(int i = 0; i < condIndex; i++){
    for(int j=0; j <numTabs; j++){
      cout << "\t";
    }
    PrintCondition(qlm.condptr[condsInNode[i]]);
    cout << "\n";
  }
  prevNode.PrintNode(numTabs + 1);

  return (0);
}

/*
 * Free all memory associated with this node, and delete the previous node
 */
RC QL_NodeSel::DeleteNodes(){
  prevNode.DeleteNodes();
  delete &prevNode;
  if(listsInitialized == true){
    free(condList);
    free(attrsInRec);
    free(buffer);
    free(condsInNode);
  }
  listsInitialized = false;
  return (0);
}

bool QL_NodeSel::IsRelNode(){
  return false;
}

RC QL_NodeSel::OpenIt(void *data){
  return (QL_BADCALL);
}

RC QL_NodeSel::UseIndex(int attrNum, int indexNumber, void *data){
  return (QL_BADCALL);
}
