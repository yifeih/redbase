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

QO_Manager::~QO_Manager(){
  // free all alloted space!
  for(int i=0; i < nRels; i++){
    map<int, costElem*>::iterator it;
    for(it= optcost[i].begin(); it != optcost[i].end(); ++it){
      delete it->second;
    }
  }
}

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


RC QO_Manager::Compute(QO_Rel *relOrder){
  RC rc = 0;
  if((rc = InitializeBaseCases()))
    return (rc);

  vector<int> allSubsets;
  for(int i=0; i < nRels; i++){
    int bitmap = 0;
    if((rc = AddRelToBitmap(i, bitmap)))
      return (rc);
    allSubsets.push_back(bitmap);
  }

  for(int i=1; i < nRels; i++){ // for all sizes
    vector<int> allNewSubsets;
    vector<int>::iterator it;
    for(it= allSubsets.begin(); it != allSubsets.end(); ++it){
      for(int j=0; j < nRels; j++){ // go through all ways of adding a new relation
        int relsInJoin = *it;
        int newRelsInJoin = relsInJoin;
        AddRelToBitmap(j, newRelsInJoin);
        bool isInOptCost1 = (optcost[i-1].find(newRelsInJoin) != optcost[i-1].end());
        bool isInOptCost2 = (optcost[i].find(newRelsInJoin) != optcost[i].end());
        if(!IsBitSet(j, relsInJoin) && !isInOptCost1 && !isInOptCost2){
          if((rc = CalculateOptJoin(newRelsInJoin, i))){
            return (rc);
          }
          allNewSubsets.push_back(newRelsInJoin);
        }
      }
    }
    allSubsets.clear();
    allSubsets = allNewSubsets;
  }

  int relsInJoin = 0;
  for(int i = 0; i < nRels; i++){
    if((rc = AddRelToBitmap(i, relsInJoin)))
      return (rc);
  }
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

RC QO_Manager::CalculateOptJoin(int relsInJoin, int relSize){
  RC rc = 0;
  vector<int> relsInJoinVec;
  ConvertBitmapToVec(relsInJoin, relsInJoinVec);

  vector<int>::iterator it;
  costElem *costTable = new costElem();
  costTable->cost = FLT_MAX;
  costTable->indexAttr = -1;
  costTable->indexCond = -1;
  for(it = relsInJoinVec.begin(); it != relsInJoinVec.end(); ++it){
    int subJoin = relsInJoin;
    if((rc = RemoveRelFromBitmap(*it, subJoin)))
      return (rc);

    float cost;
    map<int, attrStat> attrStats;
    float totalTuples;
    int indexAttr = -1;
    int indexCond = -1;
    if((rc = CalculateJoin(subJoin, *it, relSize, cost, totalTuples, attrStats, indexAttr, indexCond)))
      return (rc);
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
  optcost[relSize].insert({relsInJoin, costTable});

  return (0);
}

RC QO_Manager::CalculateJoin(int relsInJoin, int newRel, int relSize, 
  float &cost, float &totalTuples, map<int, attrStat> &attrStats,
  int &indexAttr, int &indexCond){
  RC rc = 0;
  // copy all attributes over. 
  attrStats = optcost[relSize-1][relsInJoin]->attrs;
  //attrs.insert(optcost[0][singleRelBitmap]->attrs.begin(), optcost[0][singleRelBitmap]->attrs.end());

  string relName(rels[newRel].relName);
  int relIndexStart = qlm.relToAttrIndex[relName];
  for(int j=0; j < rels[newRel].attrCount; j++){
    attrStat stat =  { (float)attrs[relIndexStart+j].numDistinct, 
                  attrs[relIndexStart+j].maxValue,
                  attrs[relIndexStart+j].minValue};
    attrStats.insert({relIndexStart + j, stat});
  }
  int useIdx = false;
  float indexTupleNum = FLT_MAX;
  totalTuples = ((float) rels[newRel].numTuples) * optcost[relSize-1][relsInJoin]->numTuples;
  
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
      if(conds[i].op == EQ_OP && (IsValidIndexCond(relsInJoin, i, newRel, indexAttrTemp) == 0)
        && ((float)attrs[indexAttrTemp].numDistinct  < indexTupleNum) && (attrs[indexAttrTemp].indexNo != -1)){
        useIdx = true;
        indexTupleNum = (float)attrs[indexAttrTemp].numDistinct;
        indexAttr = indexAttrTemp;
        indexCond = i;
      }
    }
  }
  if(useIdx == true){
    cost = optcost[relSize-1][relsInJoin]->cost + 
          optcost[relSize-1][relsInJoin]->numTuples * ((float)attrs[indexAttr].numDistinct);
  }
  else{
    int newRelBitmap = 0;
    AddRelToBitmap(newRel, newRelBitmap);
    cost = optcost[relSize-1][relsInJoin]->cost + 
        optcost[relSize-1][relsInJoin]->numTuples * optcost[0][newRelBitmap]->cost;
  }
  return (rc);
}


RC QO_Manager::IsValidIndexCond(int relsJoined, int condIndex, int relIdx, int &attrIndex){
  RC rc = 0;
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


RC QO_Manager::AddRelToBitmap(int relNum, int &bitmap){
  if(relNum > 8*sizeof(int))
    return (QO_INVALIDBIT);
  bitmap |= 1 << relNum;
  return (0);
}

RC QO_Manager::RemoveRelFromBitmap(int relNum, int &bitmap){
  if(relNum > 8*sizeof(int))
    return (QO_INVALIDBIT);
  bitmap &= ~(1 << relNum);
  return (0);
}

RC QO_Manager::ClearBitmap(int &bitmap){
  bitmap = 0;
  return (0);
}

bool QO_Manager::IsBitSet(int relNum, int bitmap){
  if(relNum > 8*sizeof(int))
    return (QO_INVALIDBIT);
  if((bitmap & (1 << relNum)))
    return true;
  return false;
}

RC QO_Manager::ConvertBitmapToVec(int bitmap, vector<int> &relsInJoin){
  for(int i = 0; i < 8*sizeof(int); i++){
    if((bitmap &(1 << i)))
      relsInJoin.push_back(i);
  }
  return (0);
}

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


RC QO_Manager::InitializeBaseCases(){
  RC rc = 0;
  for(int i=0; i < nRels; i++){
    costElem *costEntry = new costElem();
    costEntry->joins = 0;
    costEntry->newRelIndex = i;
    costEntry->indexAttr = -1;
    costEntry->indexCond = -1;

    int relsInJoin = 0;
    bool useIdx = false;
    int indexAttr = -1;
    int indexCond = -1;

    string relName(rels[i].relName);
    int relIndexStart = qlm.relToAttrIndex[relName];
    map<int, attrStat> attrStats;
    for(int j=0; j < rels[i].attrCount; j++){
      attrStat stat = { (float)attrs[relIndexStart+j].numDistinct, 
                    attrs[relIndexStart+j].maxValue,
                    attrs[relIndexStart+j].minValue};
      attrStats.insert({relIndexStart + j, stat});
    }
    float totalTuples = (float) rels[i].numTuples;
    if((rc = CalcCondsForRel(attrStats, relsInJoin, i, totalTuples, useIdx, indexAttr, indexCond)))
      return (rc);

    for(int j=0; j < rels[i].attrCount; j++){
      attrStats[relIndexStart + j].numTuples = min(totalTuples, attrStats[relIndexStart + j].numTuples);
      costEntry->attrs.insert({j+relIndexStart, attrStats[j+relIndexStart]});
    }
    costEntry->numTuples = totalTuples;

    if(useIdx){
      costEntry->cost = (float) attrs[indexAttr].numDistinct;
      costEntry->indexAttr = indexAttr;
      costEntry->indexCond = indexCond;
    }
    else{
      costEntry->cost = CalculateNumPages(rels[i].numTuples, rels[i].tupleLength);
    }
    int relBitmap = 0;
    if((rc = AddRelToBitmap(i, relBitmap)))
      return (rc);
    optcost[0].insert({relBitmap, costEntry});

  }

  return (0);
}


RC QO_Manager::CalcCondsForRel(map<int, attrStat> & attrStats, int relsInJoin, int relIdx,
  float &totalTuples, bool &useIdx, int& indexAttr, int& indexCond){
  RC rc = 0;
  float indexTupleNum = FLT_MAX;
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
      if((rc = CondToAttrIdx(i, indexAttr, unused)))
        return (rc);
      if(conds[i].op == EQ_OP && !conds[i].bRhsIsAttr && ((float)attrs[indexAttr].numDistinct) < indexTupleNum
        && (attrs[indexAttr].indexNo != -1)){
        useIdx = true;
        int unused;
        if((rc = CondToAttrIdx(i, indexAttr, unused)))
          return (rc);
        indexCond = i;
        indexTupleNum = ((float)attrs[indexAttr].numDistinct);
      }
    }
  }
  return (0);
}



RC QO_Manager::ApplyEQCond(map<int, attrStat> &attr_stats, int condIdx, float& numTuples){
  RC rc = 0;
  int attrIdx, attrIdx2;
  if((rc = CondToAttrIdx(condIdx, attrIdx, attrIdx2)))
    return (rc);
  float frac = 0.0;
  if(conds[condIdx].bRhsIsAttr){
    numTuples = numTuples / max(attr_stats[attrIdx].numTuples, attr_stats[attrIdx2].numTuples);
    attr_stats[attrIdx].numTuples = max(attr_stats[attrIdx].numTuples, attr_stats[attrIdx2].numTuples);
    attr_stats[attrIdx2].numTuples = max(attr_stats[attrIdx].numTuples, attr_stats[attrIdx2].numTuples);
    frac = 1.0/max(attr_stats[attrIdx].numTuples, attr_stats[attrIdx2].numTuples);
  }
  else{
    float value;
    ConvertValueToFloat(condIdx, value);
    numTuples = numTuples / attr_stats[attrIdx].numTuples;
    attr_stats[attrIdx].numTuples = 1;
    attr_stats[attrIdx].minValue = value;
    attr_stats[attrIdx].maxValue = value;
    frac = 1.0/attr_stats[attrIdx].numTuples;
  }
  NormalizeStats(attr_stats, frac, attrIdx, attrIdx2);
  return (0);
}

RC QO_Manager::NormalizeStats(map<int, attrStat> &attr_stats, float frac, int attrIdx1, int attrIdx2){
  map<int, attrStat>::iterator it;
  for(it = attr_stats.begin(); it != attr_stats.end(); ++it){
    int attrIdx = it->first;
    if(attrIdx != attrIdx1 && attrIdx != attrIdx2){
      attr_stats[attrIdx].numTuples = attr_stats[attrIdx].numTuples*frac;
    }
  }
  return (0);
}

RC QO_Manager::ApplyLTCond(map<int, attrStat> &attr_stats, int condIdx, float& numTuples){
  RC rc = 0;
  int idx, idx2;
  if((rc = CondToAttrIdx(condIdx, idx, idx2)))
    return (rc);
  float frac = 0.0;
  if(conds[condIdx].bRhsIsAttr){
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
      (attr_stats[idx].maxValue - attr_stats[idx].minValue + 1);
    attr_stats[idx2].numTuples = attr_stats[idx].numTuples*(attr_stats[idx2].maxValue - newMinS)/
      (attr_stats[idx2].maxValue - attr_stats[idx2].minValue + 1);
    attr_stats[idx].maxValue = newMaxR;
    attr_stats[idx].minValue = newMinS;
    frac = max(fracR, fracS);
  }
  else{
    float value;
    ConvertValueToFloat(condIdx, value);
    float fracR = (value - attr_stats[idx].minValue + 1)/(attr_stats[idx].maxValue - attr_stats[idx].minValue + 2);
    numTuples = numTuples*fracR;
    attr_stats[idx].numTuples = attr_stats[idx].numTuples * fracR;
    attr_stats[idx].maxValue = value;
    frac = fracR;
  }
  NormalizeStats(attr_stats, frac, idx, idx2);

  return (0);
}

RC QO_Manager::ApplyGTCond(map<int, attrStat> &attr_stats, int condIdx, float& numTuples){
  RC rc = 0;
  int idx, idx2;
  if((rc = CondToAttrIdx(condIdx, idx, idx2)))
    return (rc);
  float frac = 0.0;
  if(conds[condIdx].bRhsIsAttr){
    swap(idx, idx2);
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
      (attr_stats[idx].maxValue - attr_stats[idx].minValue + 1);
    attr_stats[idx2].numTuples = attr_stats[idx].numTuples*(attr_stats[idx2].maxValue - newMinS)/
      (attr_stats[idx2].maxValue - attr_stats[idx2].minValue + 1);
    attr_stats[idx].maxValue = newMaxR;
    attr_stats[idx].minValue = newMinS;
    frac = max(fracR, fracS);
  }
  else{
    float value;
    ConvertValueToFloat(condIdx, value);
    float fracR = (attr_stats[idx].maxValue - value + 1)/(attr_stats[idx].maxValue - attr_stats[idx].minValue + 2);
    numTuples = numTuples*fracR;
    attr_stats[idx].numTuples = attr_stats[idx].numTuples * fracR;
    attr_stats[idx].minValue = value;
    frac = fracR;
  }
  NormalizeStats(attr_stats, frac, idx, idx2);
  return (0);
}


float QO_Manager::CalculateNumPages(int numTuples, int tupleLength){
  return ceilf((1.0*tupleLength)*numTuples/(1.0*PF_PAGE_SIZE));
}

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


bool QO_Manager::UseCondition(int relsJoined, int condIndex, int relIdx){
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
  else{
    int firstRel;
    AttrToRelIndex(conds[condIndex].lhsAttr, firstRel);
    if(firstRel == relIdx)
      return true;
  }
  return false;
}


float QO_Manager::ConvertStrToFloat(char *string){
  float value = (float) string[0];
  return value;
}

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

