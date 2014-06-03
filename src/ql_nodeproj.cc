// 
// File:          ql_nodeproj.cc
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
 * Create the project node with a reference to the previous node
 */
QL_NodeProj::QL_NodeProj(QL_Manager &qlm, QL_Node &prevNode) : QL_Node(qlm), prevNode(prevNode) {
  isOpen = false;
  listsInitialized = false;
  attrsInRecSize = 0;
  tupleLength = 0;
}

/*
 * Free memory allocated to this node
 */
QL_NodeProj::~QL_NodeProj(){
  if(listsInitialized == true){
    free(attrsInRec);
    free(buffer);
  }
  listsInitialized = false;
}

/*
 * Set up the node by giving it the number of attributes to keep.
 * It allocates the memory to keep the list of indices referring to
 * attributes to keep
 */
RC QL_NodeProj::SetUpNode(int numAttrToKeep){
  RC rc = 0;
  // malloc the list of attributes to keep
  attrsInRec = (int *)malloc(numAttrToKeep * sizeof(int));
  memset((void*)attrsInRec, 0, sizeof(attrsInRec));
  int attrsInRecSize = 0;

  // malloc the buffer to keep the data for a tuple passed
  // up from the previous node
  int bufLength;
  prevNode.GetTupleLength(bufLength);
  buffer = (char *)malloc(bufLength);
  memset((void*) buffer, 0, sizeof(buffer));
  listsInitialized = true;

  return (0);
}

/*
 * Open the iterator by opening the previous node
 */
RC QL_NodeProj::OpenIt(){
  RC rc = 0;
  if((rc = prevNode.OpenIt()))
    return (rc);
  return (0);
}

/*
 * Get the Data from the previous node, and reconstruct it
 */
RC QL_NodeProj::GetNext(char *data){
  RC rc = 0;
  if((rc = prevNode.GetNext(buffer)))
    return (rc);

  ReconstructRec(data);
  return (0);
}

/*
 * Close the iterator
 */
RC QL_NodeProj::CloseIt(){
  RC rc = 0;
  if((rc = prevNode.CloseIt()))
    return (rc);
  return (0);
}

/*
 * Add a projection attribute to the list of attributes to keep
 */
RC QL_NodeProj::AddProj(int attrIndex){
  RC rc = 0;
  tupleLength += qlm.attrEntries[attrIndex].attrLength;

  attrsInRec[attrsInRecSize] = attrIndex;
  attrsInRecSize++;
  return (0);
}

/*
 * Given data from the previous node store in buffer, it reconstructs it
 * by retrieving only the attributes to keep, and storing the
 * new data in the pointer passed in
 */
RC QL_NodeProj::ReconstructRec(char *data){
  RC rc = 0;
  int currIdx = 0;
  int *attrsInPrevNode;
  int numAttrsInPrevNode;
  if((rc = prevNode.GetAttrList(attrsInPrevNode, numAttrsInPrevNode)))
    return (rc);

  for(int i = 0; i < attrsInRecSize; i++){
    int bufIdx = 0;
    for(int j = 0; j < numAttrsInPrevNode; j++){
      if(attrsInRec[i] == attrsInPrevNode[j]){
        break;
      }
      int prevNodeIdx = attrsInPrevNode[j];
      bufIdx += qlm.attrEntries[prevNodeIdx].attrLength;
    }
    // get offset of index j
    int attrIdx = attrsInRec[i];
    memcpy(data + currIdx, buffer + bufIdx, qlm.attrEntries[attrIdx].attrLength);
    currIdx += qlm.attrEntries[attrIdx].attrLength;
  }


  return (0);
}

/*
 * Print the node, and instruct it to print its previous nodes
 */
RC QL_NodeProj::PrintNode(int numTabs){
  for(int i=0; i < numTabs; i++){
    cout << "\t";
  }
  cout << "--PROJ: \n";
  for(int i = 0; i < attrsInRecSize; i++){
    int index = attrsInRec[i];
    cout << " " << qlm.attrEntries[index].relName << "." << qlm.attrEntries[index].attrName;
  }
  cout << "\n";
  prevNode.PrintNode(numTabs + 1);

  return (0);
}

/*
 * Don't allow record returning for projection nodes
 */
RC QL_NodeProj::GetNextRec(RM_Record &rec){
  return (QL_BADCALL);
}

/*
 * Free all memory associated with this node, and delete the previous node
 */
RC QL_NodeProj::DeleteNodes(){
  prevNode.DeleteNodes();
  delete &prevNode;
  if(listsInitialized == true){
    free(attrsInRec);
    free(buffer);
    //free(attrsToKeep);
  }
  listsInitialized = false;
  return (0);
}

bool QL_NodeProj::IsRelNode(){
  return false;
}

RC QL_NodeProj::OpenIt(void *data){
  return (QL_BADCALL);
}

RC QL_NodeProj::UseIndex(int attrNum, int indexNumber, void *data){
  return (QL_BADCALL);
}
