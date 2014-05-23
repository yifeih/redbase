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

QL_NodeProj::QL_NodeProj(QL_Manager &qlm, QL_Node &prevNode) : QL_Node(qlm), prevNode(prevNode) {
  isOpen = false;
  listsInitialized = false;
  attrsInRecSize = 0;
  tupleLength = 0;
  //numAttrsToKeep = 0;
  //attrsListIdx = 0;
}

QL_NodeProj::~QL_NodeProj(){
  if(listsInitialized == true){
    free(attrsInRec);
    free(buffer);
    //free(attrsToKeep);
  }
  listsInitialized = false;
}

RC QL_NodeProj::SetUpNode(int numAttrToKeep){
  RC rc = 0;
  /*
  int *attrListPtr;
  if((rc = prevNode.GetAttrList(attrListPtr, attrsInRecSize)))
    return (rc);
  attrsInRec = (int *)malloc(attrsInRecSize*sizeof(int));
  for(int i = 0;  i < attrsInRecSize; i++){
    attrsInRec[i] = attrListPtr[i];
  }

  attrsToKeep = (int *)malloc(numAttrToKeep*sizeof(int));
  memset((void*)attrsToKeep, 0, sizeof(attrsToKeep));

  int bufLength;
  prevNode.GetTupleLength(bufLength);
  buffer = (char *)malloc(bufLength);
  memset((void*)buffer, 0, sizeof(buffer));
  listsInitialized = true;
  */
  attrsInRec = (int *)malloc(numAttrToKeep * sizeof(int));
  memset((void*)attrsInRec, 0, sizeof(attrsInRec));
  int attrsInRecSize = 0;

  int bufLength;
  prevNode.GetTupleLength(bufLength);
  buffer = (char *)malloc(bufLength);
  memset((void*) buffer, 0, sizeof(buffer));
  listsInitialized = true;

  return (0);
}

RC QL_NodeProj::OpenIt(){
  RC rc = 0;
  if((rc = prevNode.OpenIt()))
    return (rc);
  return (0);
}

RC QL_NodeProj::GetNext(char *data){
  RC rc = 0;
  if((rc = prevNode.GetNext(buffer)))
    return (rc);

  ReconstructRec(data);
  return (0);
}

RC QL_NodeProj::CloseIt(){
  RC rc = 0;
  if((rc = prevNode.CloseIt()))
    return (rc);
  return (0);
}

RC QL_NodeProj::AddProj(int attrIndex){
  RC rc = 0;
  tupleLength += qlm.attrEntries[attrIndex].attrLength;

  attrsInRec[attrsInRecSize] = attrIndex;
  attrsInRecSize++;
  return (0);
}

RC QL_NodeProj::ReconstructRec(char *data){
  RC rc = 0;
  int currIdx = 0;
  int bufIdx = 0;
  int *attrsInPrevNode;
  int numAttrsInPrevNode;
  if((rc = prevNode.GetAttrList(attrsInPrevNode, numAttrsInPrevNode)))
    return (rc);
  for(int i = 0; i < numAttrsInPrevNode; i++){
    bool toKeep = false;
    for(int j = 0; j < attrsInRecSize; j++){
      if (i == attrsInRec[j])
        toKeep = true;
    }
    if(toKeep){
      memcpy(data + currIdx, buffer + bufIdx, qlm.attrEntries[i].attrLength);
      currIdx += qlm.attrEntries[i].attrLength;
    }
    bufIdx += qlm.attrEntries[i].attrLength;

  }

  return (0);
}

RC QL_NodeProj::PrintNode(int numTabs){
  for(int i=0; i < numTabs; i++){
    cout << "\t";
  }
  cout << "-Project Node: ";
  for(int i = 0; i < attrsInRecSize; i++){
    int index = attrsInRec[i];
    cout << " " << qlm.attrEntries[index].relName << "." << qlm.attrEntries[index].attrName;
  }
  cout << "\n";
  prevNode.PrintNode(numTabs + 1);

  return (0);
}

RC QL_NodeProj::GetNextRec(RM_Record &rec){
  return (QL_BADCALL);
}

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
