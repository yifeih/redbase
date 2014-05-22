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