//
// qo_manager.cc
//

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
#undef max
#undef min
#include <sstream>
#include <cstdio>
#include <set>
#include <map>
#include <vector>
#include <cfloat>
#include "qo.h"
#include <algorithm>
#include <tgmath.h>

using namespace std;

// Given the QL manager, the # of relations in the select statement.
// the RelCatEntries for the relations in the select statement, the number
// of conditions, and the list of those conditions, the QL_manager
// prepares the optcost struct for dynamic programming
QO_Manager::QO_Manager(QL_Manager &qlm, int nRelations, RelCatEntry *relations, int nAttributes, AttrCatEntry *attributes, 
    int nConditions, const Condition conditions[]) : qlm(qlm) {
  // Construct the new optjoin 
  for(int i=0; i < nRelations; i++){
    map<int, costElem*>  mapper;
    optcost.push_back(mapper);
  }

  nRels = nRelations;
  nAttrs = nAttributes;
  nConds = nConditions;
  rels = relations;
  attrs = attributes;
  conds = conditions;
}

// free all alloted space! All the alloted costElems
QO_Manager::~QO_Manager(){
  for(int i=0; i < nRels; i++){
    map<int, costElem*>::iterator it;
    for(it= optcost[i].begin(); it != optcost[i].end(); ++it){
      delete it->second;
    }
  }
}

// Prints all the statistics inside of optcost
RC QO_Manager::PrintRels(){
  RC rc = 0;
  int relsInJoin = 0;
  for(int i = 0; i < nRels; i++){
    if((rc = AddRelToBitmap(i, relsInJoin)))
      return (rc);
  }

  for(int i=0; i < nRels; i++){

    map<int, costElem*> ::iterator it2;
    for(it2 = optcost[i].begin(); it2 != optcost[i].end(); ++it2){
      vector<int> relsInJoinVec;
      ConvertBitmapToVec(it2->second->joins, relsInJoinVec);
      vector<int>::iterator it;
      printf("REL JOIN: ");
      for(it = relsInJoinVec.begin(); it != relsInJoinVec.end(); ++it){
        cout << *it << ",";
      }
      printf("\n");
      cout << "  newRelIndex: " << it2->second->newRelIndex << endl;
      cout << "  numTuples: " << it2->second->numTuples << endl;
      cout << "  cost: " << it2->second->cost << endl;
      cout << "  indexAttr: " << it2->second->indexAttr << endl;
      cout << "  indexCond: " << it2->second->indexCond << endl;

      map<int, attrStat> attributes = it2->second->attrs;
      map<int, attrStat>::iterator it3;
      for(it3 = attributes.begin(); it3 != attributes.end(); ++it3){
        cout << "    attribute: " << (it3->first) << endl;
        cout << "      numTuples: " << (it3->second.numTuples) << endl;
        cout << "      max: " << (it3->second.maxValue) << endl;
        cout << "      mix: " << (it3->second.minValue) << endl;
      }
    }
  }
  return (0);
}

// Given a pointer to a QO_Rel array, it sets up the values of
// the QO_Rel entries in the order that the relations should be 
// joined in. It also returns the estimated cost and # of tuples
// in the ultimate selection
RC QO_Manager::Compute(QO_Rel *relOrder, float &costEst, float &tupleEst){
  RC rc = 0;
  // initialize the base relations
  if((rc = InitializeBaseCases()))
    return (rc);

  // create a vector containing bitmaps that
  // reflect all subsets of size 1
  vector<int> allSubsets;
  for(int i=0; i < nRels; i++){
    int bitmap = 0;
    if((rc = AddRelToBitmap(i, bitmap)))
      return (rc);
    allSubsets.push_back(bitmap);
  }

  for(int i=1; i < nRels; i++){ // for all sizes of subsets
    vector<int> allNewSubsets; // will hold all subsets of size i+1
                               // at the end of the iteration
    // iterate through all sets of the previous size
    vector<int>::iterator it;
    for(it= allSubsets.begin(); it != allSubsets.end(); ++it){
      for(int j=0; j < nRels; j++){ // go through all ways of adding a new relation
        int relsInJoin = *it;
        int newRelsInJoin = relsInJoin;
        AddRelToBitmap(j, newRelsInJoin);
        bool isInOptCost1 = (optcost[i-1].find(newRelsInJoin) != optcost[i-1].end());
        bool isInOptCost2 = (optcost[i].find(newRelsInJoin) != optcost[i].end());
        // If adding this new relation modifies the set (i.e. relation is not)
        // included in the set of the previous size) and we haven't calculated
        // the optimal way of joining this yet, do so
        if(!IsBitSet(j, relsInJoin) && !isInOptCost1 && !isInOptCost2){
          if((rc = CalculateOptJoin(newRelsInJoin, i))){
            return (rc);
          }
          // add this to the set.
          allNewSubsets.push_back(newRelsInJoin);
        }
      }
    }
    allSubsets.clear();
    allSubsets = allNewSubsets;
    // set the previous set of joins of size i to the set of joins
    // of size i+1
  }

  // create the bitmap containing all relations
  int relsInJoin = 0;
  for(int i = 0; i < nRels; i++){
    if((rc = AddRelToBitmap(i, relsInJoin)))
      return (rc);
  }
  // update the overall estimated costs and tuple numbers
  costEst = optcost[nRels-1][relsInJoin]->cost;
  tupleEst = optcost[nRels-1][relsInJoin]->numTuples;
  // backtrace: start with the set of all relations, and use the
  // "joins" field to get the cost stats of the previous opt join, and
  // move them to the appropriate location in QO_Rel array
  for(int i=0; i < nRels; i++){
    int index = nRels-i-1;
    int nextSubJoin = optcost[index][relsInJoin]->joins;
    relOrder[index].relIdx = optcost[index][relsInJoin]->newRelIndex;
    relOrder[index].indexAttr = optcost[index][relsInJoin]->indexAttr;
    relOrder[index].indexCond = optcost[index][relsInJoin]->indexCond;
    relsInJoin = nextSubJoin;
  }

  return (0);
}

// Given a set of relations (as bitmap) and the relation size, calculate
// the optimal way of joining the relations in that set
RC QO_Manager::CalculateOptJoin(int relsInJoin, int relSize){
  RC rc = 0;
  // create a vector of these relations in the bitmap
  vector<int> relsInJoinVec;
  ConvertBitmapToVec(relsInJoin, relsInJoinVec);

  // Create a cost element, and initialize its values
  vector<int>::iterator it;
  costElem *costTable = new costElem();
  costTable->cost = FLT_MAX;
  costTable->indexAttr = -1;
  costTable->indexCond = -1;
  // iterate through all ways of removing a relation a
  for(it = relsInJoinVec.begin(); it != relsInJoinVec.end(); ++it){
    int subJoin = relsInJoin;
    if((rc = RemoveRelFromBitmap(*it, subJoin)))
      return (rc);

    float cost;
    map<int, attrStat> attrStats;
    float totalTuples;
    int indexAttr = -1;
    int indexCond = -1;
    // Calculate the a join (S-a)
    if((rc = CalculateJoin(subJoin, *it, relSize, cost, totalTuples, attrStats, indexAttr, indexCond)))
      return (rc);
    // if the cost is the smallest so far, update all values
    if(cost < costTable->cost){ 
      costTable->cost = cost;
      costTable->attrs.clear();
      costTable->attrs = attrStats;
      costTable->joins = subJoin;
      costTable->newRelIndex = *it;
      costTable->numTuples = totalTuples;
      costTable->indexAttr = indexAttr;
      costTable->indexCond = indexCond;
    }
  }
  // insert the costElem as the optimal way of arriving
  // at this join
  optcost[relSize].insert({relsInJoin, costTable});

  return (0);
}


// Given a set of relations already in the join (S-a) as a bitmap
// relsInJoin, the index of the new relation to join (a), the
// relation size, and the initial numTuples and attribute statistics,
// it computes the cost, updates tota tuples, updates attribute
// conditions, and whether to use an attribute or not.
RC QO_Manager::CalculateJoin(int relsInJoin, int newRel, int relSize, 
  float &cost, float &totalTuples, map<int, attrStat> &attrStats,
  int &indexAttr, int &indexCond){
  RC rc = 0;
  // copy all attributes over. 
  attrStats = optcost[relSize-1][relsInJoin]->attrs;

  // set up the initial attribute stats for this relation
  string relName(rels[newRel].relName);
  int relIndexStart = qlm.relToAttrIndex[relName];
  for(int j=0; j < rels[newRel].attrCount; j++){
    attrStat stat =  { (float)attrs[relIndexStart+j].numDistinct, 
                  attrs[relIndexStart+j].maxValue,
                  attrs[relIndexStart+j].minValue};
    attrStats.insert({relIndexStart + j, stat});
  }
  // keep track of whether to use the index join or not
  int useIdx = false;
  float indexTupleNum = FLT_MIN;
  // initial total # of tuples
  totalTuples = ((float) rels[newRel].numTuples) * optcost[relSize-1][relsInJoin]->numTuples;
  
  // Iterate through all the conditions to see if they apply to 
  // this join
  for(int i=0; i < nConds; i++){
    if(UseCondition(relsInJoin, i, newRel)){

      switch(conds[i].op){
        case EQ_OP : ApplyEQCond(attrStats, i, totalTuples); break;
        case LT_OP : ApplyLTCond(attrStats, i, totalTuples); break;
        case GT_OP : ApplyGTCond(attrStats, i, totalTuples); break;
        case LE_OP : ApplyLTCond(attrStats, i, totalTuples); break;
        case GE_OP : ApplyGTCond(attrStats, i, totalTuples); break;
        default: break;
      }
      int indexAttrTemp = -1;
      // IF there is an index that can be used with this condition, calculate
      // the number of tuples that satisfy that equality index. We want this
      // number to be as large as possible because that requires less tuples
      // to be extracted from the index scan -> less PAge reads
      if(conds[i].op == EQ_OP && (IsValidIndexCond(relsInJoin, i, newRel, indexAttrTemp) == 0)
        && ((float)attrs[indexAttrTemp].numDistinct  > indexTupleNum) && (attrs[indexAttrTemp].indexNo != -1)){
        useIdx = true;
        indexTupleNum = (float)attrs[indexAttrTemp].numDistinct;
        indexAttr = indexAttrTemp;
        indexCond = i;
      }
    }
  }
  // IF we're using an index, compute the cost of the index join
  float indexcost = FLT_MAX;
  float filecost = 0;
  if(useIdx == true){
    indexcost = optcost[relSize-1][relsInJoin]->cost + 
          optcost[relSize-1][relsInJoin]->numTuples * ((float)qlm.relEntries[newRel].numTuples)/((float)attrs[indexAttr].numDistinct);
    cost = indexcost;
  }
  //else{
    // also compute the cost of the nested loop join
    int newRelBitmap = 0;
    AddRelToBitmap(newRel, newRelBitmap);
    filecost = optcost[relSize-1][relsInJoin]->cost + 
        optcost[relSize-1][relsInJoin]->numTuples * (1 + optcost[0][newRelBitmap]->cost);
    // if the nested loop join cost is smaller, use nested loop
    // join. Otherwise, use index join.
    if(filecost < indexcost){
      cost = filecost;
      indexAttr = -1;
      indexCond = -1;
    }
  //}
  return (rc);
}

// Check whether a given condition (given by its index number) should be applied
// when the relations in bitmap relsJoined are joined with relation relIdx. If it
// is valid, it returns the attr index associated with relIdx in attrIndex.
RC QO_Manager::IsValidIndexCond(int relsJoined, int condIndex, int relIdx, int &attrIndex){
  RC rc = 0;
  // For attr-attr conditions, one attribute must be in relIdx, and another
  // must be in relsJoined
  if(conds[condIndex].bRhsIsAttr){
    int firstRel, secondRel;
    AttrToRelIndex(conds[condIndex].lhsAttr, firstRel);
    AttrToRelIndex(conds[condIndex].rhsAttr, secondRel);
    bool firstBitSet = IsBitSet(firstRel, relsJoined);
    bool secondBitSet = IsBitSet(secondRel, relsJoined);
    if(firstBitSet && secondRel == relIdx){
      if((rc = qlm.GetAttrCatEntryPos(conds[condIndex].rhsAttr, attrIndex)))
        return (rc);
      return (0);
    }
    else if(secondBitSet && firstRel == relIdx){
      if((rc = qlm.GetAttrCatEntryPos(conds[condIndex].lhsAttr, attrIndex)))
        return (rc);
      return (0);
    }
  }
  // otherwise, attr-value conditions must have attribute in relIdx
  else{
    int firstRel;
    AttrToRelIndex(conds[condIndex].lhsAttr, firstRel);
    if(firstRel == relIdx){
      if((rc = qlm.GetAttrCatEntryPos(conds[condIndex].lhsAttr, attrIndex)))
        return (rc);
      return (0);
    }
  }

  return (rc);
}

// Sets a bit in the given relJoin bitmap
RC QO_Manager::AddRelToBitmap(int relNum, int &bitmap){
  if(relNum > 8*sizeof(int))
    return (QO_INVALIDBIT);
  bitmap |= 1 << relNum;
  return (0);
}

// Resets a bit in the given relJoin bitmap
RC QO_Manager::RemoveRelFromBitmap(int relNum, int &bitmap){
  if(relNum > 8*sizeof(int))
    return (QO_INVALIDBIT);
  bitmap &= ~(1 << relNum);
  return (0);
}

// clears a bitmap
RC QO_Manager::ClearBitmap(int &bitmap){
  bitmap = 0;
  return (0);
}

// Checks whether a bit is set in a given bitmap
bool QO_Manager::IsBitSet(int relNum, int bitmap){
  if(relNum > 8*sizeof(int))
    return (QO_INVALIDBIT);
  if((bitmap & (1 << relNum)))
    return true;
  return false;
}


// Converts the bitmap to a vector of ints, with the ints being the
// indices related to the relations in the join (aka join bitmap)
RC QO_Manager::ConvertBitmapToVec(int bitmap, vector<int> &relsInJoin){
  for(int i = 0; i < 8*sizeof(int); i++){
    if((bitmap &(1 << i)))
      relsInJoin.push_back(i);
  }
  return (0);
}

// Converts the vector of relation ints to a bitmap associated
// to that join
RC QO_Manager::ConvertVecToBitmap(vector<int> &relsInJoin, int &bitmap){
  RC rc = 0;
  vector<int>::iterator it;
  bitmap = 0;
  for(it = relsInJoin.begin(); it != relsInJoin.end(); ++it){
    int relNum = *it;
    if((rc = AddRelToBitmap(relNum, bitmap)))
      return (rc);
  }
  return (0);
}


// This initializes the base cases of one relation.
RC QO_Manager::InitializeBaseCases(){
  RC rc = 0;
  for(int i=0; i < nRels; i++){
    // create a costElem ement, and set basic values
    costElem *costEntry = new costElem();
    costEntry->joins = 0;
    costEntry->newRelIndex = i;
    costEntry->indexAttr = -1;
    costEntry->indexCond = -1;

    int relsInJoin = 0;
    bool useIdx = false;
    int indexAttr = -1;
    int indexCond = -1;

    // initialize the attribute statistics to the ones calculated
    // in the attrcat entry
    string relName(rels[i].relName);
    int relIndexStart = qlm.relToAttrIndex[relName];
    map<int, attrStat> attrStats;
    for(int j=0; j < rels[i].attrCount; j++){
      attrStat stat = { (float)attrs[relIndexStart+j].numDistinct, 
                    attrs[relIndexStart+j].maxValue,
                    attrs[relIndexStart+j].minValue};
      attrStats.insert({relIndexStart + j, stat});
    }
    // calls CalcCondsForRel to determine whether any tuples can
    // be used as select conditions to minimize the # of tuples
    // out of this relation
    float totalTuples = (float) rels[i].numTuples;
    if((rc = CalcCondsForRel(attrStats, relsInJoin, i, totalTuples, useIdx, indexAttr, indexCond)))
      return (rc);

    // normalize the numDistinct values for each attribute and insert it
    // into the map in this costElement
    for(int j=0; j < rels[i].attrCount; j++){
      attrStats[relIndexStart + j].numTuples = min(totalTuples, attrStats[relIndexStart + j].numTuples);
      costEntry->attrs.insert({j+relIndexStart, attrStats[j+relIndexStart]});
    }
    costEntry->numTuples = totalTuples;

    // if we are using an index to apply a condition, update the const
    // entry to reflect that
    if(useIdx){
      costEntry->cost = 1 +  ((float)rels[i].numTuples)/((float)attrs[indexAttr].numDistinct);
      costEntry->indexAttr = indexAttr;
      costEntry->indexCond = indexCond;
    }
    else{
      costEntry->cost = CalculateNumPages(rels[i].numTuples, rels[i].tupleLength);
    }
    // Add this relation to the bitmap, and make this the key
    // to this costElement struct
    int relBitmap = 0;
    if((rc = AddRelToBitmap(i, relBitmap)))
      return (rc);
    optcost[0].insert({relBitmap, costEntry});

  }

  return (0);
}

// For a given join, calculate the new attribute stats by applying
// the conditions that apply to the joining single relation.
// It is given the previous attribute stats, the previous join,
// the new attribute to join, the previous total # of tuples. It returns
// the new stats by updating attrStats and totalTuples, and
// specifies whether to use an index, and which index to use it on
RC QO_Manager::CalcCondsForRel(map<int, attrStat> & attrStats, int relsInJoin, int relIdx,
  float &totalTuples, bool &useIdx, int& indexAttr, int& indexCond){
  RC rc = 0;
  // for index joins, it matters how many tuples have that particular
  // value for that attribute, so keep track of the number
  // of distinct values of the index attribute in indexTupleNum
  float indexTupleNum = FLT_MAX;
  // iterate through all conditions to see if they apply to this join
  for(int i=0; i < nConds; i++){
    if(UseCondition(relsInJoin, i, relIdx)){
      switch(conds[i].op){
        case EQ_OP : ApplyEQCond(attrStats, i, totalTuples); break;
        case LT_OP : ApplyLTCond(attrStats, i, totalTuples); break;
        case GT_OP : ApplyGTCond(attrStats, i, totalTuples); break;
        case LE_OP : ApplyLTCond(attrStats, i, totalTuples); break;
        case GE_OP : ApplyGTCond(attrStats, i, totalTuples); break;
        default: break;
      }
      int unused; 
      int attributeNum = 0;
      if((rc = CondToAttrIdx(i, attributeNum, unused)))
        return (rc);
      // if this is an equality condition, and there is an index on one of
      // the attributes, make this the index join attribute if the number of
      // distinct tuples is greater than the previous join attribute candidate
      if(conds[i].op == EQ_OP && !conds[i].bRhsIsAttr && ((float)attrs[attributeNum].numDistinct) > indexTupleNum
        && (attrs[attributeNum].indexNo != -1)){
        // update return values to reflect this index join
        useIdx = true;
        int unused;
        if((rc = CondToAttrIdx(i, attributeNum, unused)))
          return (rc);
        indexCond = i;
        indexAttr = attributeNum;
        indexTupleNum = ((float)attrs[indexAttr].numDistinct);
      }
    }
  }
  return (0);
}


// Given the attribute map, the condition index, and the total number of tuples in the
// relation, it computes the estimated effect of applying the EQ condition
// specified by the condition at the index condIdx. It returns the estimated
// values in attr_stats and numTuples
RC QO_Manager::ApplyEQCond(map<int, attrStat> &attr_stats, int condIdx, float& numTuples){
  RC rc = 0;
  // Retrieves the indices of the condition attributes
  int attrIdx, attrIdx2;
  if((rc = CondToAttrIdx(condIdx, attrIdx, attrIdx2)))
    return (rc);
  float frac = 0.0;
  if(conds[condIdx].bRhsIsAttr){
    // assume containment of value sets
    numTuples = numTuples / max(attr_stats[attrIdx].numTuples, attr_stats[attrIdx2].numTuples);
    attr_stats[attrIdx].numTuples = max(attr_stats[attrIdx].numTuples, attr_stats[attrIdx2].numTuples);
    attr_stats[attrIdx2].numTuples = max(attr_stats[attrIdx].numTuples, attr_stats[attrIdx2].numTuples);
    frac = 1.0/max(attr_stats[attrIdx].numTuples, attr_stats[attrIdx2].numTuples);
  }
  else{
    // calculate the # of tuples
    float value;
    ConvertValueToFloat(condIdx, value);
    numTuples = ceilf(numTuples / attr_stats[attrIdx].numTuples);
    attr_stats[attrIdx].numTuples = 1;
    attr_stats[attrIdx].minValue = value;
    attr_stats[attrIdx].maxValue = value;
    frac = 1.0/attr_stats[attrIdx].numTuples;
  }
  // Normalize non-join attributes
  NormalizeStats(attr_stats, frac, numTuples, attrIdx, attrIdx2);
  return (0);
}

RC QO_Manager::NormalizeStats(map<int, attrStat> &attr_stats, float frac, float numTuples, int attrIdx1, int attrIdx2){
  map<int, attrStat>::iterator it;
  for(it = attr_stats.begin(); it != attr_stats.end(); ++it){
    int attrIdx = it->first;
    if(attrIdx != attrIdx1 && attrIdx != attrIdx2){
      attr_stats[attrIdx].numTuples = min(numTuples, attr_stats[attrIdx].numTuples);
     // attr_stats[attrIdx].numTuples = attr_stats[attrIdx].numTuples*frac;
    }
  }
  return (0);
}
// Given the attribute map, the condition index, and the total number of tuples in the
// relation, it computes the estimated effect of applying the LT or LE condition
// specified by the condition at the index condIdx. It returns the estimated
// values in attr_stats and numTuples
RC QO_Manager::ApplyLTCond(map<int, attrStat> &attr_stats, int condIdx, float& numTuples){
  RC rc = 0;
  // Retrieves the indices of the condition attributes
  int idx, idx2;
  if((rc = CondToAttrIdx(condIdx, idx, idx2)))
    return (rc);
  float frac = 0.0;
  if(conds[condIdx].bRhsIsAttr){
    // calculate the "mid" value of the range of the two values
    float mid = 0.5*(min(attr_stats[idx].maxValue, attr_stats[idx2].maxValue) + 
      max(attr_stats[idx].minValue, attr_stats[idx2].minValue));
    // based on this mid value, calculate the estimated fraction of
    // tuples from each relation that passes the condition. 
    float fracS = max(attr_stats[idx2].maxValue - mid + 1, (float)0.0) / 
      (attr_stats[idx2].maxValue - attr_stats[idx2].minValue + 1);
    float fracR = max(mid - attr_stats[idx].minValue + 1, (float)0.0) / 
      (attr_stats[idx].maxValue - attr_stats[idx].minValue + 1);
    // calculate the new maxR and minS values
    float newMaxR = min(attr_stats[idx].maxValue, attr_stats[idx2].maxValue);
    float newMinS = min(attr_stats[idx].minValue, attr_stats[idx2].minValue);
    // the number of tuples, based on fracS and fracR calculated above
    numTuples = numTuples * max(fracR, fracS);
    // update the join attribute stats
    attr_stats[idx].numTuples = attr_stats[idx].numTuples*(newMaxR - attr_stats[idx].minValue + 1)/
      (attr_stats[idx].maxValue - attr_stats[idx].minValue + 1);
    attr_stats[idx2].numTuples = attr_stats[idx].numTuples*(attr_stats[idx2].maxValue - newMinS)/
      (attr_stats[idx2].maxValue - attr_stats[idx2].minValue + 1);
    attr_stats[idx].maxValue = newMaxR;
    attr_stats[idx].minValue = newMinS;
    frac = max(fracR, fracS);
  }
  else{
    float value;
    ConvertValueToFloat(condIdx, value); // get the max value
    // calculate the fraction of values expected to survive the condition. assume
    // that values for relation are evenly distributed
    float fracR = (value - attr_stats[idx].minValue + 1)/(attr_stats[idx].maxValue - attr_stats[idx].minValue + 2);
    // update values
    numTuples = numTuples*fracR;
    attr_stats[idx].numTuples = attr_stats[idx].numTuples * fracR;
    attr_stats[idx].maxValue = value;
    frac = fracR;
  }
  // Normalize non-join attributes
  NormalizeStats(attr_stats, frac, numTuples, idx, idx2);

  return (0);
}


// Given the attribute map, the condition index, and the total number of tuples in the
// relation, it computes the estimated effect of applying the GT or GE condition
// specified by the condition at the index condIdx. It returns the estimated
// values in attr_stats and numTuples
RC QO_Manager::ApplyGTCond(map<int, attrStat> &attr_stats, int condIdx, float& numTuples){
  RC rc = 0;
  // Retrieves the indices of the condition attributes
  int idx, idx2;
  if((rc = CondToAttrIdx(condIdx, idx, idx2)))
    return (rc);
  float frac = 0.0;
  if(conds[condIdx].bRhsIsAttr){
    swap(idx, idx2); // the following is the same as LT, except attr indices are swapped
    float mid = 0.5*(min(attr_stats[idx].maxValue, attr_stats[idx2].maxValue) + 
      max(attr_stats[idx].minValue, attr_stats[idx2].minValue));
    float fracS = max(attr_stats[idx2].maxValue - mid + 1, (float)0.0) / 
      (attr_stats[idx2].maxValue - attr_stats[idx2].minValue + 1);
    float fracR = max(mid - attr_stats[idx].minValue + 1, (float)0.0) / 
      (attr_stats[idx].maxValue - attr_stats[idx].minValue + 1);
    float newMaxR = min(attr_stats[idx].maxValue, attr_stats[idx2].maxValue);
    float newMinS = min(attr_stats[idx].minValue, attr_stats[idx2].minValue);
    numTuples = numTuples * max(fracR, fracS);
    attr_stats[idx].numTuples = attr_stats[idx].numTuples*(newMaxR - attr_stats[idx].minValue + 1)/
      (attr_stats[idx].maxValue - attr_stats[idx].minValue + 2);
    attr_stats[idx2].numTuples = attr_stats[idx].numTuples*(attr_stats[idx2].maxValue - newMinS)/
      (attr_stats[idx2].maxValue - attr_stats[idx2].minValue + 2);
    attr_stats[idx].maxValue = newMaxR;
    attr_stats[idx].minValue = newMinS;
    frac = max(fracR, fracS);
  }
  else{
    float value;
    ConvertValueToFloat(condIdx, value); // get the min value
    // calculate the fraction of values expected to survive the condition. assume
    // that values for relation are evenly distributed
    float fracR = (attr_stats[idx].maxValue - value + 1)/(attr_stats[idx].maxValue - attr_stats[idx].minValue + 2);
    // update values
    numTuples = numTuples*fracR; 
    attr_stats[idx].numTuples = attr_stats[idx].numTuples * fracR;
    attr_stats[idx].minValue = value;
    frac = fracR;
  }
  // Normalize non-join attributes
  NormalizeStats(attr_stats, frac, numTuples, idx, idx2);
  return (0);
}

// Given the number of tuples and the tuple length of a relation,
// it retuns the estimated # of getpage calls necessary to do a filescan
// over the relation. the +10 is an overhead for the page headers
float QO_Manager::CalculateNumPages(int numTuples, int tupleLength){
  //cout << "tuple length" << endl;
  return ceilf((1.0*tupleLength+10.0)*numTuples/(1.0*PF_PAGE_SIZE));
}

// Given an index of a condition, it returns the attribute indices
// associated with that condition. If the condition is a value-attr condition
// then attrIdx will contain -1
RC QO_Manager::CondToAttrIdx(int condIndex, int &attrIdx1, int &attrIdx2){
  RC rc = 0;
  if((rc = qlm.GetAttrCatEntryPos(conds[condIndex].lhsAttr, attrIdx1)))
    return (rc);
  if(conds[condIndex].bRhsIsAttr){
    if((rc = qlm.GetAttrCatEntryPos(conds[condIndex].rhsAttr, attrIdx2)))
      return (rc);
  }
  else
    attrIdx2 = -1;
  return (0);
}

// Given the index of a attr-value condition, it converts the
// value in the condition to a float depending on what type
// the comparison is of
RC QO_Manager::ConvertValueToFloat(int condIndex, float &value){
  if(conds[condIndex].bRhsIsAttr)
    return (QO_BADCONDITION);
  int attrIndex;
  qlm.GetAttrCatEntryPos(conds[condIndex].lhsAttr, attrIndex);
  if(attrs[attrIndex].attrType == STRING)
    value = ConvertStrToFloat((char*)conds[condIndex].rhsValue.data);
  else if(attrs[attrIndex].attrType == INT)
    value = (float) *((int*) (conds[condIndex].rhsValue.data));
  else
    value = *((float*)(conds[condIndex].rhsValue.data));
  return (0);
}


// Checks whether the condition at condIndex should be applied after/during this
// join, given the relations already in the join (relsJoined) and the
// current relation being joined (relIdx).
// Returns true or false
bool QO_Manager::UseCondition(int relsJoined, int condIndex, int relIdx){
  // If attributes on both sides, one of the attributes must fall within
  // the relation specified by relIdx, and the other must be in a relation
  // specified in relsJoined
  if(conds[condIndex].bRhsIsAttr){
    int firstRel, secondRel;
    AttrToRelIndex(conds[condIndex].lhsAttr, firstRel);
    AttrToRelIndex(conds[condIndex].rhsAttr, secondRel);
    bool firstBitSet = IsBitSet(firstRel, relsJoined);
    bool secondBitSet = IsBitSet(secondRel, relsJoined);
    if(firstBitSet && secondRel == relIdx)
      return true;
    else if(secondBitSet && firstRel == relIdx)
      return true;
    else if(firstRel == relIdx && secondRel == relIdx)
      return true;
  }
  // If attr-value comparison, then the condition must be of the relation
  // specified by relIdx
  else{
    int firstRel;
    AttrToRelIndex(conds[condIndex].lhsAttr, firstRel);
    if(firstRel == relIdx)
      return true;
  }
  return false;
}


// Converts a string to a float. It does so by casting the first
// byte of the string as a float. 
float QO_Manager::ConvertStrToFloat(char *string){
  float value = (float) string[0];
  return value;
}

// Given an RelAttr, returns the relation index to which this
// attribute belongs
RC QO_Manager::AttrToRelIndex(const RelAttr attr, int& relIndex){
  if(attr.relName != NULL){
    string relName(attr.relName);
    relIndex = qlm.relToInt[relName];
  }
  else{
    string attrName(attr.attrName);
    set<string> relSet = qlm.attrToRel[attrName];
    relIndex = qlm.relToInt[*relSet.begin()];
  }

  return (0);
}

