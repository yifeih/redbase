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
#include "node_comps.h"

using namespace std;

QL_NodeProj::QL_NodeProj(QL_Manager &qlm, QL_Node &prevNode) : QL_Node(qlm), prevNode(prevNode) {

}

QL_NodeProj::~QL_NodeProj(){

}

RC QL_NodeProj::OpenIt(){

}

RC QL_NodeProj::GetNext(char *data){

}

RC QL_NodeProj::CloseIt(){

}


RC QL_NodeProj::GetNextRec(RM_Record &rec){
  return (QL_BADCALL);
}

RC QL_NodeProj::DeleteNodes(){

}

RC QL_NodeProj::GetAttrList(int *attrList, int &attrListSize){

}

RC QL_NodeProj::GetTupleLength(int &tupleLength){
  
}