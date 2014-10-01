//
// ql_manager.cc
//

#include <cstdio>
#include <iostream>
#include <sys/times.h>
#include <sys/types.h>
#include <cassert>
#include <unistd.h>
#include "redbase.h"
#include "ql.h"
#include "sm.h"
#include "ix.h"
#include "rm.h"
#include "ql_node.h"
#include <string>
#include <set>
#include <map>
#include <cfloat>
#include "qo.h"
#undef max
#undef min
#include <algorithm>

using namespace std;

//
// QL_Manager::QL_Manager(SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm)
//
// Constructor for the QL Manager
//
QL_Manager::QL_Manager(SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm) : rmm(rmm), ixm(ixm), smm(smm)
{
  // Can't stand unused variable warnings!
  assert (&smm && &ixm && &rmm);
}

//
// QL_Manager::~QL_Manager()
//
// Destructor for the QL Manager
//
QL_Manager::~QL_Manager()
{

}

/*
 * Reset: Resets the global variables used for 
 */
RC QL_Manager::Reset(){
  relToInt.clear();
  relToAttrIndex.clear();
  attrToRel.clear();
  conditionToRel.clear();
  nAttrs = 0;
  nRels = 0;
  nConds = 0;
  condptr = NULL;
  isUpdate = false;
  return (0);
}

//
// Handle the select clause
//
RC QL_Manager::Select(int nSelAttrs, const RelAttr selAttrs[],
                      int nRelations, const char * const relations[],
                      int nConditions, const Condition conditions[])
{
  int i;
  RC rc = 0;
  if(smm.printPageStats){
    smm.ResetPageStats();
  }

  cout << "Select\n";

  cout << "   nSelAttrs = " << nSelAttrs << "\n";
  for (i = 0; i < nSelAttrs; i++)
    cout << "   selAttrs[" << i << "]:" << selAttrs[i] << "\n";

  cout << "   nRelations = " << nRelations << "\n";
  for (i = 0; i < nRelations; i++)
    cout << "   relations[" << i << "] " << relations[i] << "\n";

  cout << "   nCondtions = " << nConditions << "\n";
  for (i = 0; i < nConditions; i++)
    cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

  // Parse - relations have no duplicates
  if((rc = ParseRelNoDup(nRelations, relations)))
    return (rc);

  // Reset class query parameters
  Reset();
  nRels = nRelations;
  nConds = nConditions;
  condptr = conditions;

  // retrieve all relations
  relEntries = (RelCatEntry *)malloc(sizeof(RelCatEntry)*nRelations);
  memset((void*)relEntries, 0, sizeof(*relEntries));
  if((rc = smm.GetAllRels(relEntries, nRelations, relations, nAttrs, relToInt))){
    free(relEntries);
    return (rc);
  }

  // Retrieve all attributes
  attrEntries = (AttrCatEntry *)malloc(sizeof(AttrCatEntry)*nAttrs);
  int slot = 0;
  for(int i=0; i < nRelations; i++){
      string relString(relEntries[i].relName);
      relToAttrIndex.insert({relString, slot});
      if((rc = smm.GetAttrForRel(relEntries + i, attrEntries + slot, attrToRel))){
        free(relEntries);
        free(attrEntries);
        return (rc);
      }
      slot += relEntries[i].attrCount;
  }
  
  // Make sure the attributes and conditions are valid
  if((rc = ParseSelectAttrs(nSelAttrs, selAttrs))){
    free(relEntries);
    free(attrEntries);
    return (rc);
  }
  if((rc = ParseConditions(nConditions, conditions))){
    free(relEntries);
    free(attrEntries);
    return (rc);
  }
  /*
  QO_Manager *qom = new QO_Manager(*this, nRels, relEntries, nAttrs, attrEntries,
      nConds, condptr);
  QO_Rel * qorels = (QO_Rel*)(malloc(sizeof(QO_Rel)*nRels));
  for(int i=0; i < nRels; i++){
    *(qorels + i) = (QO_Rel){ 0.0, -1.0, -1.0};
  }
  qom->Compute(qorels);
  qom->PrintRels();
  delete qom;
  free(qorels);
  */
  QL_Node *topNode;
  float cost, tupleEst;
  if(smm.useQO){
    //cout << "using QO" << endl;
    QO_Manager *qom = new QO_Manager(*this, nRels, relEntries, nAttrs, attrEntries,
      nConds, condptr);
    QO_Rel * qorels = (QO_Rel*)(malloc(sizeof(QO_Rel)*nRels));
    for(int i=0; i < nRels; i++){
      *(qorels + i) = (QO_Rel){ 0.0, -1.0, -1.0};
    }
    qom->Compute(qorels, cost, tupleEst);
    qom->PrintRels();
    RecalcCondToRel(qorels);
    if((rc = SetUpNodesWithQO(topNode, qorels, nSelAttrs, selAttrs)))
      return (rc);
    /*
    for(int i=0; i < nRels; i++){
      int index = qorels[i].relIdx;
      cout << relEntries[index].relName << endl;
      cout << "  associated conds: ";
      for(int j=0; j < nConds; j++){
        if(conditionToRel[j] == index)
          cout << j;
        }
        cout << endl;
      }
      */

    delete qom;
    free(qorels);
  }
  else{
    // Construct the query tree
    if((rc = SetUpNodes(topNode, nSelAttrs, selAttrs)))
      return (rc);
  }

  

  //QL_Node *topNode;
  // Construct the query tree
  //if((rc = SetUpNodes(topNode, nSelAttrs, selAttrs)))
  //  return (rc);

  // Print the query tree
  

  
  // run select
  if((rc = RunSelect(topNode)))
    return (rc);

  if(smm.useQO){
    cout << "estimated cost: " << cost << endl;
    cout << "estimated # tuples: " << tupleEst << endl;
  }
    

  if(bQueryPlans){
    cout << "PRINTING QUERY PLAN" <<endl;
    topNode->PrintNode(0);
  }

  
  // clean up query tree
  if((rc = CleanUpNodes(topNode)))
    return (rc);

  if(smm.printPageStats){
    cout << endl;
    smm.PrintPageStats();
  }
    
  free(relEntries);
  free(attrEntries);

  return (rc);
}


/*
 * This function, given the top node of the query tree, will run select and
 * print all matching tuples
 */
RC QL_Manager::RunSelect(QL_Node *topNode){
  RC rc = 0;
  // Retrieve the appropriate tuples that one is supposed to
  // print out by asking for the attribute information from the top node
  int finalTupLength;
  topNode->GetTupleLength(finalTupLength);
  int *attrList;
  int attrListSize;
  if((rc = topNode->GetAttrList(attrList, attrListSize)))
    return (rc);

  // Set up the DatAttrInfo for the attributes to print
  DataAttrInfo * attributes = (DataAttrInfo *)malloc(attrListSize* sizeof(DataAttrInfo));
  if((rc = SetUpPrinter(topNode, attributes)))
    return (rc);
  Printer printer(attributes, attrListSize);
  printer.PrintHeader(cout);

  // Open the iterator of the top node, and keep retrieving elements until
  // there are no more
  if((rc = topNode->OpenIt()))
    return (rc);
  RC it_rc = 0;
  char *buffer = (char *)malloc(finalTupLength);
  it_rc = topNode->GetNext(buffer);
  while(it_rc == 0){
    printer.Print(cout, buffer);
    it_rc = topNode->GetNext(buffer);
  }

  free(buffer);
  if((rc = topNode->CloseIt()))
    return (rc);

  printer.PrintFooter(cout);

  free(attributes);

  return (0);
}


/*
 * IsValidAttr checkes whether an attribute is valid or not. It returns
 * true if it is valid, and false if it is not. It does this based
 * on the maps set up when retrieving the relations and attributes involved
 * in the query.
 */
bool QL_Manager::IsValidAttr(const RelAttr attr){
  // If the attribute has a specified relation to it
  if(attr.relName != NULL){
    string relString(attr.relName);
    string attrString(attr.attrName);
    map<string, int>::iterator it = relToInt.find(relString);
    // tries to find the attribute name in the attribute-to-relation mapping
    map<string, set<string> >::iterator itattr = attrToRel.find(attrString);

    // if it was found, try to find the relation name in the set of relations
    // associated with that attribute
    if(it != relToInt.end() && itattr != attrToRel.end()){
      set<string> relNames = (*itattr).second;
      set<string>::iterator setit = relNames.find(relString);
      if(setit != relNames.end())
        return true;
      else
        return false;
    }
    else
      return false;
  }
  // If the attribute does not have a specified relation
  else{ 
    // Check that it is only associated with one relation
    string attrString(attr.attrName);
    set<string> relNames = attrToRel[attrString];
    if(relNames.size() == 1){
      return true;
    }
    else{
      return false;
    }
  }
}


/*
 * Set up the query nodes for the query tree, given the select operations 
 * (since conditions are saved as a pointer in the QL class)
 * It returns the top node in topNode
 */
RC QL_Manager::SetUpNodes(QL_Node *&topNode, int nSelAttrs, const RelAttr selAttrs[]){
  RC rc = 0;
  // Set up node 1:
  if((rc = SetUpFirstNode(topNode)))
    return (rc);

  // For all other relations, join it, left-deep style, with the previously
  // seen relations
  QL_Node* currNode;
  currNode = topNode;
  for(int i = 1; i < nRels; i++){
    if((rc = JoinRelation(topNode, currNode, i)))
      return (rc);
    currNode = topNode;
  }

  // If select *, don't add project nodes
  if((nSelAttrs == 1 && strncmp(selAttrs[0].attrName, "*", strlen(selAttrs[0].attrName)) == 0))
    return (0);
  // Otherwise, add a project node, and give it the attributes to project
  QL_NodeProj *projNode = new QL_NodeProj(*this, *currNode);
  projNode->SetUpNode(nSelAttrs);
  for(int i= 0 ; i < nSelAttrs; i++){
    int attrIndex = 0;
    if((rc = GetAttrCatEntryPos(selAttrs[i], attrIndex)))
      return (rc);
    if((rc = projNode->AddProj(attrIndex)))
      return (rc);
  }
  topNode = projNode;

  return (rc);
}

RC QL_Manager::SetUpNodesWithQO(QL_Node *&topNode, QO_Rel* qorels, int nSelAttrs, const RelAttr selAttrs[]){
  RC rc = 0;
  if((rc = SetUpFirstNodeWithQO(topNode, qorels)))
    return (rc);

  // For all other relations, join it, left-deep style, with the previously
  // seen relations
  QL_Node* currNode;
  currNode = topNode;
  for(int i = 1; i < nRels; i++){
    if((rc = JoinRelationWithQO(topNode, qorels, currNode, i)))
      return (rc);
    currNode = topNode;
  }

  // If select *, don't add project nodes
  if((nSelAttrs == 1 && strncmp(selAttrs[0].attrName, "*", strlen(selAttrs[0].attrName)) == 0))
    return (0);
  // Otherwise, add a project node, and give it the attributes to project
  QL_NodeProj *projNode = new QL_NodeProj(*this, *currNode);
  projNode->SetUpNode(nSelAttrs);
  for(int i= 0 ; i < nSelAttrs; i++){
    int attrIndex = 0;
    if((rc = GetAttrCatEntryPos(selAttrs[i], attrIndex)))
      return (rc);
    if((rc = projNode->AddProj(attrIndex)))
      return (rc);
  }
  topNode = projNode;
  return (rc);
}

/*
 * This is given the current top node (currNode) and an index relation number
 * (index of the relation in relEntries), and joins the relation with the
 * current top node. It then returns the new top node in topNode
 */
RC QL_Manager::JoinRelation(QL_Node *&topNode, QL_Node *currNode, int relIndex){
  RC rc = 0;
  bool useIndex = false;
  // create new relation node, providing the relation entry
  QL_NodeRel *relNode = new QL_NodeRel(*this, relEntries + relIndex);

  // Set up the list of indices corresponding to attributes in the relation
  int *attrList = (int *)malloc(relEntries[relIndex].attrCount * sizeof(int));
  memset((void *)attrList, 0, sizeof(attrList));
  for(int i = 0;  i < relEntries[relIndex].attrCount ; i++){
    attrList[i] = 0;
  }
  string relString(relEntries[relIndex].relName);
  int start = relToAttrIndex[relString]; // get the offset of the first attr in this relation
  for(int i = 0;  i < relEntries[relIndex].attrCount ; i++){
    attrList[i] = start + i;
  }
  // Set up the relation node by providing the attribute list and # of attributes
  relNode->SetUpNode(attrList, relEntries[relIndex].attrCount);
  free(attrList);

  // Count the # of conditions associated with this attribute
  int numConds;
  CountNumConditions(relIndex, numConds);

  
  // create new join node:
  QL_NodeJoin *joinNode = new QL_NodeJoin(*this, *currNode, *relNode);
  if((rc = joinNode->SetUpNode(numConds))) // provide a max count on # of conditions
    return (rc);                           // to add to this join
  topNode = joinNode;

  for(int i = 0; i < nConds; i++){
    // For each condition, associated with this node
    if(conditionToRel[i] == relIndex){
      bool added = false;
      // if it is a R.A=v condition, consider adding it to the relation node
      // by specifying a index scan, but only if there exists an index on
      // this attribute
      if(condptr[i].op == EQ_OP && !condptr[i].bRhsIsAttr && useIndex == false){
        int index = 0;
        if((rc = GetAttrCatEntryPos(condptr[i].lhsAttr, index) ))
          return (rc);
        if((attrEntries[index].indexNo != -1)){
          if((rc = relNode->UseIndex(index, attrEntries[index].indexNo, condptr[i].rhsValue.data) ))
            return (rc);
          added = true;
          useIndex = true;
        }
      }
      else if(condptr[i].op == EQ_OP && condptr[i].bRhsIsAttr && useIndex == false &&
        !relNode->useIndex ) {
        int index1, index2;
        if((rc = GetAttrCatEntryPos(condptr[i].lhsAttr, index1)))
          return (rc);
        if((rc = GetAttrCatEntryPos(condptr[i].rhsAttr, index2)))
          return (rc);
        int relIdx1, relIdx2;
        AttrToRelIndex(condptr[i].lhsAttr, relIdx1);
        AttrToRelIndex(condptr[i].rhsAttr, relIdx2);
        if(relIdx2 == relIndex && attrEntries[index2].indexNo != -1){
          if((rc = joinNode->UseIndexJoin(index1, index2, attrEntries[index2].indexNo)))
            return (rc);
          added = true;
          useIndex = true;
        }
        else if(relIdx1 == relIndex && attrEntries[index1].indexNo != -1){
          if((rc = joinNode->UseIndexJoin(index2, index1, attrEntries[index1].indexNo)))
            return (rc);
          added = true;
          useIndex = true;
        }
      }
      // Otherwise, add this condition to the join node.
      if(! added){
        if((rc = topNode->AddCondition(condptr[i], i)))
          return (rc);
      }
    }
  }

  return (rc);
}

RC QL_Manager::RecalcCondToRel(QO_Rel* qorels){
  set<int> relsSeen;
  for(int i=0; i < nRels; i++){
    int relIndex = qorels[i].relIdx;
    relsSeen.insert(relIndex);
    for(int j=0; j < nConds; j++){
      if(condptr[j].bRhsIsAttr){
        int index1, index2;
        AttrToRelIndex(condptr[j].lhsAttr, index1);
        AttrToRelIndex(condptr[j].rhsAttr, index2);
        bool found1 = (relsSeen.find(index1) != relsSeen.end());
        bool found2 = (relsSeen.find(index2) != relsSeen.end());
        if(found1 && found2 && (relIndex == index1 || relIndex== index2))
          conditionToRel[j] = relIndex;
      }
      else{
        int index1;
        AttrToRelIndex(condptr[j].lhsAttr, index1);
        if(index1 == relIndex)
          conditionToRel[j] = relIndex;
      }
    }
  }
  return (0);
}

RC QL_Manager::AttrToRelIndex(const RelAttr attr, int& relIndex){
  if(attr.relName != NULL){
    string relName(attr.relName);
    relIndex = relToInt[relName];
  }
  else{
    string attrName(attr.attrName);
    set<string> relSet = attrToRel[attrName];
    relIndex = relToInt[*relSet.begin()];
  }

  return (0);
}

RC QL_Manager::JoinRelationWithQO(QL_Node *&topNode, QO_Rel* qorels, QL_Node *currNode, int qoIdx){
  RC rc = 0;
  int relIndex = qorels[qoIdx].relIdx;
  // create new relation node, providing the relation entry
  QL_NodeRel *relNode = new QL_NodeRel(*this, relEntries + relIndex);

  // Set up the list of indices corresponding to attributes in the relation
  int *attrList = (int *)malloc(relEntries[relIndex].attrCount * sizeof(int));
  memset((void *)attrList, 0, sizeof(attrList));
  for(int i = 0;  i < relEntries[relIndex].attrCount ; i++){
    attrList[i] = 0;
  }
  string relString(relEntries[relIndex].relName);
  int start = relToAttrIndex[relString]; // get the offset of the first attr in this relation
  for(int i = 0;  i < relEntries[relIndex].attrCount ; i++){
    attrList[i] = start + i;
  }
  // Set up the relation node by providing the attribute list and # of attributes
  relNode->SetUpNode(attrList, relEntries[relIndex].attrCount);
  free(attrList);

  // Count the # of conditions associated with this attribute
  int numConds;
  CountNumConditions(relIndex, numConds);

  // create new join node:
  QL_NodeJoin *joinNode = new QL_NodeJoin(*this, *currNode, *relNode);
  if((rc = joinNode->SetUpNode(numConds))) // provide a max count on # of conditions
    return (rc);                           // to add to this join
  topNode = joinNode;

  if(qorels[qoIdx].indexAttr != -1){
    int condIdx = qorels[qoIdx].indexCond;
    int index = qorels[qoIdx].indexAttr;
    int index1 = 0;
    int index2 = 0;
    GetAttrCatEntryPos(condptr[condIdx].lhsAttr, index1);
    if(condptr[condIdx].bRhsIsAttr)
      GetAttrCatEntryPos(condptr[condIdx].rhsAttr, index2);
    int otherAttr;
    if(index1 == index)
      otherAttr = index2;
    else
      otherAttr = index1;
    if((attrEntries[index].indexNo != -1) && !condptr[condIdx].bRhsIsAttr){ // add only if there is an index on this attribute
      //cout << "adding index join on attr " << index; 
      if((rc = relNode->UseIndex(index, attrEntries[index].indexNo, condptr[condIdx].rhsValue.data) ))
        return (rc);
    }
    else if((attrEntries[index].indexNo != -1) && condptr[condIdx].bRhsIsAttr){
      //cout << "adding, lhs attr: " << otherAttr << ", rhsATtr: " << index << endl;
      if((rc = joinNode->UseIndexJoin(otherAttr, index, attrEntries[index].indexNo)))
        return (rc);
    }
  }
  for(int i = 0 ; i < nConds; i++){
    if(conditionToRel[i] == relIndex){
      if((rc = topNode->AddCondition(condptr[i], i) ))
        return (rc);
    }
  }
  return (0);
}

/*
 * Sets up the first node of a query by accessing the first element of relEntries.
 * This is used by the Delete and Update functionalities
 */
RC QL_Manager::SetUpFirstNode(QL_Node *&topNode){
  RC rc = 0;
  bool useSelNode = false; // whether a select node was used with this relation
  bool useIndex = false; // whether an index is being used on this relation
  int relIndex = 0; // take the first element of relEntries as the 1st relation

  QL_NodeRel *relNode = new QL_NodeRel(*this, relEntries);
  topNode = relNode;

  // Create the list of attributes associated with this relation
  int *attrList = (int *)malloc(relEntries[relIndex].attrCount * sizeof(int));
  memset((void *)attrList, 0, sizeof(attrList));
  for(int i = 0;  i < relEntries[relIndex].attrCount ; i++){
    attrList[i] = 0;
  }
  string relString(relEntries[relIndex].relName);
  int start = relToAttrIndex[relString];
  for(int i = 0;  i < relEntries[relIndex].attrCount ; i++){
    attrList[i] = start + i;
  }
  // Set up the relation node with this list of attributes
  relNode->SetUpNode(attrList, relEntries[relIndex].attrCount);
  free(attrList);

  // count the number of conditions associated with this node:
  int numConds;
  CountNumConditions(0, numConds);

  // For each of the conditions, add it to the relation
  for(int i = 0 ; i < nConds; i++){
    if(conditionToRel[i] == 0){
      bool added = false;
      // If it is a R.A=v condition, make the relation use index scan if there isn't already
      // an index, and if this isn't a update operation
      if(condptr[i].op == EQ_OP && !condptr[i].bRhsIsAttr && useIndex == false && isUpdate == false){
        int index = 0;
        if((rc = GetAttrCatEntryPos(condptr[i].lhsAttr, index) ))
          return (rc);
        if((attrEntries[index].indexNo != -1)){ // add only if there is an index on this attribute
          if((rc = relNode->UseIndex(index, attrEntries[index].indexNo, condptr[i].rhsValue.data) ))
            return (rc);
          added = true;
          useIndex = true;
        }
      }
      // Otherwise, create a select node and add the condition to the select node (or just add
      // if select node has already been created)
      if(! added && !useSelNode){
        QL_NodeSel *selNode = new QL_NodeSel(*this, *relNode);
        if((rc = selNode->SetUpNode(numConds) ))
          return (rc);
        topNode = selNode;
        useSelNode = true;
      }
      if(! added){
        if((rc = topNode->AddCondition(condptr[i], i) ))
          return (rc);
      }
    }
  }
  return (0);
}

RC QL_Manager::SetUpFirstNodeWithQO(QL_Node *&topNode, QO_Rel* qorels){
  RC rc = 0;
  int relIndex = qorels[0].relIdx;

  QL_NodeRel *relNode = new QL_NodeRel(*this, relEntries + relIndex);
  topNode = relNode;

  // Create the list of attributes associated with this relation
  int *attrList = (int *)malloc(relEntries[relIndex].attrCount * sizeof(int));
  memset((void *)attrList, 0, sizeof(attrList));
  for(int i = 0;  i < relEntries[relIndex].attrCount ; i++){
    attrList[i] = 0;
  }
  string relString(relEntries[relIndex].relName);
  int start = relToAttrIndex[relString];
  for(int i = 0;  i < relEntries[relIndex].attrCount ; i++){
    attrList[i] = start + i;
  }
  relNode->SetUpNode(attrList, relEntries[relIndex].attrCount);
  free(attrList);

  int numConds;
  CountNumConditions(relIndex, numConds);

  if(qorels[0].indexAttr != -1){
    int index = qorels[0].indexAttr;
    int condIdx = qorels[0].indexCond;
    if((attrEntries[index].indexNo != -1)){ // add only if there is an index on this attribute
      //cout << "using index" << endl;
      if((rc = relNode->UseIndex(index, attrEntries[index].indexNo, condptr[condIdx].rhsValue.data) ))
        return (rc);
    }
  }
  if(numConds > 0){
    QL_NodeSel *selNode = new QL_NodeSel(*this, *relNode);
    if((rc = selNode->SetUpNode(numConds) ))
      return (rc);
    topNode = selNode;
  }
  for(int i = 0 ; i < nConds; i++){
    if(conditionToRel[i] == relIndex){
      if((rc = topNode->AddCondition(condptr[i], i) ))
        return (rc);
    }
  }
  return (0);
}

/*
 * Counts the number of conditions associated with an relation index, and returns
 * that number in numConds. It does this by accessing the condition-to-relation-index map
 */
RC QL_Manager::CountNumConditions(int relIndex, int &numConds){
  RC rc = 0;
  numConds = 0;
  for(int i = 0; i < nConds; i++ ){
    if( conditionToRel[i] == relIndex)
      numConds++;
  }
  return (0);
}

/*
 * Returns the pointer to a given attribute specified by a RelAttr. This
 * pointer is returned as one to an AttrCatEntry in entry.
 */
RC QL_Manager::GetAttrCatEntry(const RelAttr attr, AttrCatEntry *&entry){
  RC rc = 0;
  int index = 0;
  if((rc = GetAttrCatEntryPos(attr, index)))
    return (rc);
  entry = attrEntries + index;
  return (0);
}

/*
 * Given an RelAttr, it retrieves the position of the appropriate attribute that it
 * references in attrEntries
 */
RC QL_Manager::GetAttrCatEntryPos(const RelAttr attr, int &index){
  // retrieves the appropriate relation name
  string relString;
  if(attr.relName != NULL){
    string relStringTemp(attr.relName);
    relString = relStringTemp;
  }
  else{
    string attrString(attr.attrName);
    set<string> relNames = attrToRel[attrString];
    set<string>::iterator it = relNames.begin();
    relString = *it;
  }
  int relNum = relToInt[relString];
  int numAttrs = relEntries[relNum].attrCount; // retrieves the # of attriubutes in relation
  int slot = relToAttrIndex[relString]; // retrieves the index of the first attr in this relation in attrEntries
  for(int i=0; i < numAttrs; i++){
    // Finds the index of the attribute whose name matches this one.
    int comp = strncmp(attr.attrName, attrEntries[slot + i].attrName, strlen(attr.attrName));
    if(comp == 0){
      index = slot + i;
      return (0);
    }
  }
  return (QL_ATTRNOTFOUND);
}

/*
 * Checks whether the select attributes are valid. Returns a positive value if there is an 
 * error, and 0 if the attributes are valid
 */
RC QL_Manager::ParseSelectAttrs(int nSelAttrs, const RelAttr selAttrs[]){
  // If it's select *, then it's valid
  if((nSelAttrs == 1 && strncmp(selAttrs[0].attrName, "*", strlen(selAttrs[0].attrName)) == 0))
    return (0);

  for(int i=0; i < nSelAttrs; i++){
    // Check that attribute name is valid:
    if(!IsValidAttr(selAttrs[i]))
      return (QL_ATTRNOTFOUND);  

  }
  return (0);
}

/*
 * Checks whether the conditions are valid. Returns a positive value if there is an error, and 0
 * if the conditions are valid
 */
RC QL_Manager::ParseConditions(int nConditions, const Condition conditions[]){
  RC rc = 0;
  for(int i=0; i < nConditions; i++){
    // Check if the first attribute is valid
    if(!IsValidAttr(conditions[i].lhsAttr)){
      return (QL_ATTRNOTFOUND);
    }
    // If we are comparing attribute to a value
    if(!conditions[i].bRhsIsAttr){
      // check that types are the same
      AttrCatEntry *entry;
      if((rc = GetAttrCatEntry(conditions[i].lhsAttr, entry)))
        return (rc);
      if(entry->attrType != conditions[i].rhsValue.type)
        return (QL_BADCOND);
      // if it is a good condition add it to the condition list, associated it with
      // the specified relation index that this attribute belongs in
      string relString(entry->relName);
      int relNum = relToInt[relString];
      conditionToRel.insert({i, relNum});
    }
    else{
      // If we are comparint attribute to attribute, make sure the second one is valid
      if(!IsValidAttr(conditions[i].rhsAttr))
        return (QL_ATTRNOTFOUND);
      AttrCatEntry *entry1;
      AttrCatEntry *entry2;
      // check that the types are same.
      if((rc = GetAttrCatEntry(conditions[i].lhsAttr,entry1)) || (rc = GetAttrCatEntry(conditions[i].rhsAttr, entry2)))
        return (rc);
      if(entry1->attrType != entry2->attrType)
        return (QL_BADCOND);
      // if it's a good condition, add it to the condition list, associating the condition with the
      // relation with the bigger index
      string relString1(entry1->relName);
      string relString2(entry2->relName);
      int relNum1 = relToInt[relString1];
      int relNum2 = relToInt[relString2];
      conditionToRel.insert({i, max(relNum1, relNum2)});
    }
  }
  return (0);
}


/*
 * Parses through all the relations in the query, making sure there are no duplicates
 * Returns a positive value if there is.
 */
RC QL_Manager::ParseRelNoDup(int nRelations, const char * const relations[]){
  set<string> relationSet;
  for(int i = 0; i < nRelations; i++){
    string relString(relations[i]);
    bool exists = (relationSet.find(relString) != relationSet.end());
    if(exists)
      return (QL_DUPRELATION);
    relationSet.insert(relString);
  }
  return (0);
}

//
// Insert the values into relName
//
RC QL_Manager::Insert(const char *relName,
                      int nValues, const Value values[])
{
  int i;
  RC rc = 0;

  cout << "Insert\n";

  cout << "   relName = " << relName << "\n";
  cout << "   nValues = " << nValues << "\n";
  for (i = 0; i < nValues; i++)
    cout << "   values[" << i << "]:" << values[i] << "\n";

  Reset();
  
  // Make space for relation name:
  relEntries = (RelCatEntry *)malloc(sizeof(RelCatEntry));
  memset((void*)relEntries, 0, sizeof(*relEntries));
  *relEntries = (RelCatEntry) {"\0", 0, 0, 0, 0, 0, false};
  if((rc = SetUpOneRelation(relName))){
    free(relEntries);
    return (rc);
  }
  // CHECK : # of attributes entered match the count for this relation
  if(relEntries->attrCount != nValues){
    free(relEntries);
    return (QL_BADINSERT);
  }

  // Retrieve the attrCatEntries associated with this relation
  attrEntries = (AttrCatEntry *)malloc(relEntries->attrCount * sizeof(AttrCatEntry));
  memset((void*)attrEntries, 0, sizeof(*attrEntries));
  for(int i= 0 ; i < relEntries->attrCount; i++){
    *(attrEntries+i) = (AttrCatEntry) {"\0", "\0", 0, INT, 0, 0, 0, 0, FLT_MIN, FLT_MAX};
  }
  if((rc = smm.GetAttrForRel(relEntries, attrEntries, attrToRel))){
    free(relEntries);
    free(attrEntries);
    return (rc);
  }

  // Check that the attribute types match
  bool badFormat = false;
  for(int i=0; i < nValues; i++){
    if((values[i].type != attrEntries[i].attrType))
      badFormat = true;
    // If it's a string, make sure we're not strying to set a smaller string to a larger one
    if(attrEntries[i].attrType == STRING && (strlen((char *) values[i].data) > attrEntries[i].attrLength))
      badFormat = true;
  }
  if(badFormat){
    free(relEntries);
    free(attrEntries);
    return (QL_BADINSERT);
  }
  // Insert this record into the relation
  rc = InsertIntoRelation(relName, relEntries->tupleLength, nValues, values);

  free(relEntries);
  free(attrEntries);

  return (rc);
}

/*
 * Given a relation name, the tuple length, and the values for the attributes, inserts this
 * tuple into the relation.
 */
RC QL_Manager::InsertIntoRelation(const char *relName, int tupleLength, int nValues, const Value values[]){
  // Open relation
  RC rc = 0;
  
  // Creates the attribute setup for printing
  DataAttrInfo * printAttributes = (DataAttrInfo *)malloc(relEntries->attrCount* sizeof(DataAttrInfo));
  if((rc = SetUpPrinterInsert(printAttributes)))
    return (rc);
  Printer printer(printAttributes, relEntries->attrCount);
  printer.PrintHeader(cout);

  // Open the appropriate file
  RM_FileHandle relFH;
  if((rc = rmm.OpenFile(relName, relFH))){
    return (rc);
  }
  char *recbuf = (char *)malloc(tupleLength);

  // Create record
  CreateRecord(recbuf, attrEntries, nValues, values);

  // Insert into relation
  RID recRID;
  if((rc = relFH.InsertRec(recbuf, recRID))){
    free(recbuf);
    return (rc);
  }
  printer.Print(cout, recbuf);
  // Insert into any indices in the relation
  if((rc = InsertIntoIndex(recbuf, recRID))){
    free(recbuf);
    return (rc);
  }

  printer.PrintFooter(cout);
  free(recbuf);
  free(printAttributes);
  // Close relation, clean up
  rc = rmm.CloseFile(relFH);
  return (rc);

}

/*
 * Used by insert. Iterates through all the attributes, and checks to see if
 * there is an index on them. If so, it opens up the index, and inserts the appropriate
 * part of the tuple into this index.
 * It is given the record, and the RID of thie record
 */
RC QL_Manager::InsertIntoIndex(char *recbuf, RID recRID){
  RC rc = 0;
  for(int i = 0; i < relEntries->attrCount; i++){
    AttrCatEntry aEntry = attrEntries[i];
    if(aEntry.indexNo != -1){
      IX_IndexHandle ih;
      if((rc = ixm.OpenIndex(relEntries->relName, aEntry.indexNo, ih)))
        return (rc);
      if((rc = ih.InsertEntry((void *)(recbuf + aEntry.offset), recRID)))
        return (rc);
      if((rc = ixm.CloseIndex(ih)))
        return (rc);
    }
  }
  return (0);
}

/*
 * Given the pointer to the new record buffer, copies all the values in values[] and puts
 * them into the appropriate locations in recbuf. 
 */
RC QL_Manager::CreateRecord(char *recbuf, AttrCatEntry *aEntries, int nValues, const Value values[]){
  for(int i = 0; i < nValues; i++){
    AttrCatEntry aEntry = aEntries[i];
    memcpy(recbuf + aEntry.offset, (char *)values[i].data, aEntry.attrLength);
  }
  return (0);
}

/*
 * Used by Insert and Delete. Set up the relEntries and attrEntires, and all other
 * QL class variables to account for this one relation.
 */
RC QL_Manager::SetUpOneRelation(const char *relName){
  RC rc = 0;
  RelCatEntry *rEntry;
  RM_Record relRec;
  if((rc = smm.GetRelEntry(relName, relRec, rEntry))){
    return (rc);
  }
  memcpy(relEntries, rEntry, sizeof(RelCatEntry));

  nRels = 1;
  nAttrs = rEntry->attrCount;
  string relString(relName);
  relToInt.insert({relString, 0});
  relToAttrIndex.insert({relString, 0});
  return (0);
}


//
// Delete from the relName all tuples that satisfy conditions
//
RC QL_Manager::Delete(const char *relName,
                      int nConditions, const Condition conditions[])
{
  int i;

  cout << "Delete\n";

  cout << "   relName = " << relName << "\n";
  cout << "   nCondtions = " << nConditions << "\n";
  for (i = 0; i < nConditions; i++)
    cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

  RC rc = 0;
  Reset();
  condptr = conditions;
  nConds = nConditions;
  
  // Make space for relation 
  relEntries = (RelCatEntry *)malloc(sizeof(RelCatEntry));
  memset((void*)relEntries, 0, sizeof(*relEntries));
  *relEntries = (RelCatEntry) {"\0", 0, 0, 0, 0, 0, true};
  if((rc = SetUpOneRelation(relName))){
    free(relEntries);
    return (rc);
  }
  // Make space for the list of attributes in this relation
  attrEntries = (AttrCatEntry *)malloc(relEntries->attrCount * sizeof(AttrCatEntry));
  memset((void*)attrEntries, 0, sizeof(*attrEntries));
  for(int i= 0 ; i < relEntries->attrCount; i++){
    *(attrEntries+i) = (AttrCatEntry) {"\0", "\0", 0, INT, 0, 0, 0, 0, FLT_MIN, FLT_MAX};
  }
  if((rc = smm.GetAttrForRel(relEntries, attrEntries, attrToRel))){
    free(relEntries);
    free(attrEntries);
    return (rc);
  }

  // Makes sure that the conditions are appropriate
  if(rc = ParseConditions(nConditions, conditions)){
    free(relEntries);
    free(attrEntries);
    return (rc);
  }

   // Create the query tree nodes
    QL_Node *topNode;
    if((rc = SetUpFirstNode(topNode)))
    return (rc);

  // Print the query plan
  if(bQueryPlans){
    cout << "PRINTING QUERY PLAN" <<endl;
    topNode->PrintNode(0);
  }
  
  if((rc = RunDelete(topNode))) // Run delete
    return (rc);
  
  if(rc = CleanUpNodes(topNode)) // Clean up the nodes
    return (rc);

  free(relEntries);
  free(attrEntries);

  return 0;
}

/*
 * This is used by RunUpdate. It creates an array of Attr, with one Attr
 * element per attribute given the relation used in the delete command.
 * Each Attr element holds a reference to an IX_IndexHandle element that 
 * can be used to facilitate delete in relations with indices.
 * Delete touches the indices of multiple attributes, so we need to keep
 * a list of all IX_IndexHandles. 
 * It also opens up the specified relation with the passed in RM_FileHandle.
 */
RC QL_Manager::SetUpRun(Attr* attributes, RM_FileHandle &relFH){
  RC rc = 0;
  for(int i=0; i < relEntries->attrCount; i++){
    memset((void*)&attributes[i], 0, sizeof(attributes[i]));
    IX_IndexHandle ih;
    attributes[i] = (Attr) {0, 0, 0, 0, ih, NULL};
  }
  if((rc = smm.PrepareAttr(relEntries, attributes)))
    return (rc);
  if((rc = rmm.OpenFile(relEntries->relName, relFH)))
    return (rc);
  return (0);
}

/*
 * Cleans up the Attr list mentioned in SetUpRun. Also used by update
 */
RC QL_Manager::CleanUpRun(Attr* attributes, RM_FileHandle &relFH){
  RC rc = 0;
  if( (rc = rmm.CloseFile(relFH)))
    return (rc);

  // Destroy and close the pointers in Attr struct
  if((rc = smm.CleanUpAttr(attributes, relEntries->attrCount)))
    return (rc);
  return (0);
}

/*
 * Given the top node of the query tree, it retrieves elements that satisfy its conditions
 * and deletes these tuples, deleting them also from any index
 */
RC QL_Manager::RunDelete(QL_Node *topNode){
  RC rc = 0;
  // Retrieves the list of attributes associated with this relation, and set up the
  // printer
  int finalTupLength, attrListSize;
  topNode->GetTupleLength(finalTupLength);
  int *attrList;
  if((rc = topNode->GetAttrList(attrList, attrListSize)))
    return (rc);
  DataAttrInfo * printAttributes = (DataAttrInfo *)malloc(attrListSize* sizeof(DataAttrInfo));
  if((rc = SetUpPrinter(topNode, printAttributes)))
    return (rc);
  Printer printer(printAttributes, attrListSize);
  printer.PrintHeader(cout);

  // Open the appropriate file, and Attr* list for holding IX_IndexHandles
  // for efficient index updates
  RM_FileHandle relFH;
  Attr* attributes = (Attr *)malloc(sizeof(Attr)*relEntries->attrCount);
  if((rc = SetUpRun(attributes, relFH))){
    smm.CleanUpAttr(attributes, relEntries->attrCount);
    return (rc);
  } 
  
  // Retrieves records
  if((rc = topNode->OpenIt() ))
    return (rc);

  RM_Record rec;
  RID rid;
  while(true){
    if((rc = topNode->GetNextRec(rec))){
      if (rc == QL_EOI){
        break;
      }
      return (rc);
    }
    char *pData;
    if((rc = rec.GetRid(rid)) || (rc = rec.GetData(pData)) )
      return (rc);
    printer.Print(cout, pData); // print out info about the attribute to delete
    if((rc = relFH.DeleteRec(rid))) // delete it
      return (rc);
    
    // Delete it from any index as well
    for(int i=0; i < relEntries->attrCount ; i++){
      if(attributes[i].indexNo != -1){
        if((rc = attributes[i].ih.DeleteEntry((void *)(pData + attributes[i].offset), rid)))
          return (rc);
      }
    }
  }
  
  if((rc = topNode->CloseIt()))
    return (rc);


  if((rc = CleanUpRun(attributes, relFH)))
    return (rc);

  printer.PrintFooter(cout);
  free(printAttributes);

  return (0);
}


/*
 * Deletes the nodes of the tree recursively, starting from the top node
 */
RC QL_Manager::CleanUpNodes(QL_Node *topNode){
  RC rc = 0;
  if((rc = topNode->DeleteNodes()))
    return (rc);
  delete topNode;
  return (0);
}

/*
 * Checks that the attribute to set in Update is a valid set statement
 */
RC QL_Manager::CheckUpdateAttrs(const RelAttr &updAttr,
                      const int bIsValue,
                      const RelAttr &rhsRelAttr,
                      const Value &rhsValue){
  RC rc = 0;
  if(!IsValidAttr(updAttr)) // lhs is a valid attribute
    return (QL_ATTRNOTFOUND);
  // If it is a value, 
  if(bIsValue){
    AttrCatEntry *entry;
    if((rc = GetAttrCatEntry(updAttr, entry)))
      return (rc);
    if(entry->attrType != rhsValue.type) // check types
      return (QL_BADUPDATE);
    if(entry->attrType == STRING){ // check the sizes for strings. The LHS must be smaller or equal to RHS
      int newValueSize = strlen((char *)rhsValue.data); 
      if(newValueSize > entry->attrLength)
        return (QL_BADUPDATE);
    }
  }
  // If it is an another attiribute
  else{
    if(!IsValidAttr(rhsRelAttr))
      return (QL_ATTRNOTFOUND);
    AttrCatEntry *entry1;
    AttrCatEntry *entry2;
    if((rc = GetAttrCatEntry(updAttr, entry1)) || (rc = GetAttrCatEntry(rhsRelAttr, entry2)))
      return (rc);
    if(entry1->attrType != entry2->attrType)
      return (QL_BADUPDATE);
    if(entry1->attrType == STRING){ // check the sizes for strings. The LHS must be smaller or equal to RHS
      if(entry2->attrLength > entry1->attrLength)
        return (QL_BADUPDATE);
    }
  }

  return (rc);
}

/*
 * Given the top node of the query tree, and info about the attribute to update, runs Update
 */
RC QL_Manager::RunUpdate(QL_Node *topNode, const RelAttr &updAttr,
                      const int bIsValue,
                      const RelAttr &rhsRelAttr,
                      const Value &rhsValue){
  RC rc = 0;
  // Retrieves the attributes in this relation for the printer setup
  int finalTupLength, attrListSize;
  topNode->GetTupleLength(finalTupLength);
  int *attrList;
  if((rc = topNode->GetAttrList(attrList, attrListSize)))
    return (rc);
  DataAttrInfo * attributes = (DataAttrInfo *)malloc(attrListSize* sizeof(DataAttrInfo));
  if((rc = SetUpPrinter(topNode, attributes)))
    return (rc);
  Printer printer(attributes, attrListSize);
  printer.PrintHeader(cout);


  // Open the file
  RM_FileHandle relFH;
  if((rc = rmm.OpenFile(relEntries->relName, relFH)))
    return (rc);

  int index1, index2; // Get the attrEntries indices to the RHS and LHS attributes
  if((rc = GetAttrCatEntryPos(updAttr, index1)))
    return (rc);
  if(!bIsValue){
    if((rc = GetAttrCatEntryPos(rhsRelAttr, index2)))
      return (rc);
  }

  // Get the index to the attribute to be update, if there is one
  IX_IndexHandle ih;
  if((attrEntries[index1].indexNo != -1)){
    if((rc = ixm.OpenIndex(relEntries->relName, attrEntries[index1].indexNo, ih)))
      return (rc);
  }

  // Find all tuples that meet the condition
  if((rc = topNode->OpenIt() ))
    return (rc);
  RM_Record rec;
  RID rid;
  while(true){
    if((rc = topNode->GetNextRec(rec))){
      if (rc == QL_EOI){
        break;
      }
      return (rc);
    }
    char *pData;
    if((rc = rec.GetRid(rid)) || (rc = rec.GetData(pData)) )
      return (rc);
    if(attrEntries[index1].indexNo != -1){ // Delete this value from the index
      if((ih.DeleteEntry(pData + attrEntries[index1].offset, rid)))
        return (rc);
    }
    
    // Set the attribute to its new value
    if(bIsValue){
      if(attrEntries[index1].attrType == STRING){
        int valueLength = strlen((char *)rhsValue.data);
        if(attrEntries[index1].attrLength <= (valueLength + 1) )
          memcpy(pData + attrEntries[index1].offset, (char *)rhsValue.data, attrEntries[index1].attrLength);
        else
          memcpy(pData + attrEntries[index1].offset, (char *)rhsValue.data, valueLength + 1);
      }
      else
        memcpy(pData + attrEntries[index1].offset, (char *)rhsValue.data, attrEntries[index1].attrLength);
    }
    else{
      if(attrEntries[index2].attrLength >= attrEntries[index1].attrLength)
        memcpy(pData + attrEntries[index1].offset, pData + attrEntries[index2].offset, attrEntries[index1].attrLength);
      else{
        memcpy(pData + attrEntries[index1].offset, pData + attrEntries[index2].offset, attrEntries[index2].attrLength);
        pData[attrEntries[index1].offset + attrEntries[index2].attrLength] = '\0';
      }
    }

    // Upate this record in the file
    if((rc = relFH.UpdateRec(rec)))
      return (rc);
    printer.Print(cout, pData);
    
    // Update the record in the index
    if(attrEntries[index1].indexNo != -1){
      if((ih.InsertEntry(pData + attrEntries[index1].offset, rid)))
        return (rc);
    }

  }
  if((rc = topNode->CloseIt()))
    return (rc);
    
  if((attrEntries[index1].indexNo != -1)){ // Close file and indices
    if((rc = ixm.CloseIndex(ih)))
      return (rc);
  }
  if((rc = rmm.CloseFile(relFH)))
    return (rc);

  printer.PrintFooter(cout);
  free(attributes);

  return (0);
}

//
// Update from the relName all tuples that satisfy conditions
//
RC QL_Manager::Update(const char *relName,
                      const RelAttr &updAttr,
                      const int bIsValue,
                      const RelAttr &rhsRelAttr,
                      const Value &rhsValue,
                      int nConditions, const Condition conditions[])
{
  int i;

  cout << "Update\n";

  cout << "   relName = " << relName << "\n";
  cout << "   updAttr:" << updAttr << "\n";
  if (bIsValue)
    cout << "   rhs is value: " << rhsValue << "\n";
  else
    cout << "   rhs is attribute: " << rhsRelAttr << "\n";

  cout << "   nCondtions = " << nConditions << "\n";
  for (i = 0; i < nConditions; i++)
    cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

  RC rc = 0;
  Reset();
  condptr = conditions;
  nConds = nConditions;
  isUpdate = true; // this is to prevent the relation node to put a index on the set attribute
  
  // Make room for the relCAt entry
  relEntries = (RelCatEntry *)malloc(sizeof(RelCatEntry));
  memset((void*)relEntries, 0, sizeof(*relEntries));
  *relEntries = (RelCatEntry) {"\0", 0, 0, 0, 0, 0, true};
  if((rc = SetUpOneRelation(relName))){
    free(relEntries);
    return (rc);
  }

  // Set up the attrEntries list
  attrEntries = (AttrCatEntry *)malloc(relEntries->attrCount * sizeof(AttrCatEntry));
  memset((void*)attrEntries, 0, sizeof(*attrEntries));
  for(int i= 0 ; i < relEntries->attrCount; i++){
    *(attrEntries+i) = (AttrCatEntry) {"\0", "\0", 0, INT, 0, 0, 0, 0, FLT_MIN, FLT_MAX};
  }
  if((rc = smm.GetAttrForRel(relEntries, attrEntries, attrToRel))){
    free(relEntries);
    free(attrEntries);
    return (rc);
  }

  // Make sure conditions are valid
  if(rc = ParseConditions(nConditions, conditions)){
    free(relEntries);
    free(attrEntries);
    return (rc);
  }

  // Check that the update value is ok
  if(rc = CheckUpdateAttrs(updAttr, bIsValue, rhsRelAttr, rhsValue)){
    free(relEntries);
    free(attrEntries);
    return (rc);
  }
   // Create query tree
  QL_Node *topNode;
  
  if((rc = SetUpFirstNode(topNode)))
    return (rc);

  // Print query plan
  if(bQueryPlans){
    cout << "PRINTING QUERY PLAN" <<endl;
    topNode->PrintNode(0);
  }
  
  // Run Update
  if((rc = RunUpdate(topNode, updAttr, bIsValue, rhsRelAttr, rhsValue)))
    return (rc);
  if(rc = CleanUpNodes(topNode))
    return (rc);
    

  free(relEntries);
  free(attrEntries);

  return 0;
}


/*
 * Sets up the printer to print the attributes specified by topNode
 */
RC QL_Manager::SetUpPrinter(QL_Node *topNode, DataAttrInfo *attributes){
  RC rc = 0;
  // Retrieve the attribute list from the top node
  int *attrList;
  int attrListSize;
  if((rc = topNode->GetAttrList(attrList, attrListSize)))
    return (rc);
  
  // Set up the attributes to print by tapping into attrEntries 
  for(int i=0; i < attrListSize; i++){
    int index = attrList[i];
    memcpy(attributes[i].relName, attrEntries[index].relName, MAXNAME + 1);
    memcpy(attributes[i].attrName, attrEntries[index].attrName, MAXNAME + 1);
    attributes[i].attrType = attrEntries[index].attrType;
    attributes[i].attrLength = attrEntries[index].attrLength;
    attributes[i].indexNo = attrEntries[index].indexNo;
    int offset, length;
    if((rc = topNode->IndexToOffset(index, offset, length)))
      return (rc);
    attributes[i].offset = offset;
  }

  return (0);
}

/*
 * Sets up the printer to print attributes for insert statements. Because
 * there is only one relation in inserts, we print in the order of the attributes
 * in attrEntries
 */
RC QL_Manager::SetUpPrinterInsert(DataAttrInfo *attributes){
  RC rc = 0;
  for(int i=0; i < relEntries->attrCount ; i++){
    memcpy(attributes[i].relName, attrEntries[i].relName, MAXNAME +1);
    memcpy(attributes[i].attrName, attrEntries[i].attrName, MAXNAME + 1);
    attributes[i].attrType = attrEntries[i].attrType;
    attributes[i].attrLength = attrEntries[i].attrLength;
    attributes[i].indexNo = attrEntries[i].indexNo;
    attributes[i].offset = attrEntries[i].offset;
  }
  return (0);
}

