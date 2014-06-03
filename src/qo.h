//
// qo.h
//   Query Optimizer Component Interface
//

// This file only gives the stub for the QL component

#ifndef QO_H
#define QO_H


#include <string>
#include <set>
#include <map>
#include <stdlib.h>
#include <string.h>
#include "redbase.h"
#include "parser.h"
#include "rm.h"
#include "ix.h"
#include "sm.h"
#include "ql_node.h"
#include "ql.h"
#undef max
#include <vector>


typedef struct attrStat{
  float numTuples;
  float maxValue;
  float minValue;
} attrStat;

typedef struct costElem{
  int joins;
  int newRelIndex;
  float numTuples;
  float cost;
  int indexAttr;
  int indexCond;
  std::map<int, attrStat> attrs;
} costElem;


class QO_Manager{
  friend class QL_Manager;

public:
  QO_Manager(QL_Manager &qlm, int nRelations, RelCatEntry *relations, int nAttributes, AttrCatEntry *attributes, 
    int nConditions, const Condition conditions[]);
  ~QO_Manager();
  RC PrintRels();
  RC Compute(QO_Rel *relOrder);
  RC CalculateOptJoin(int relsInJoin, int relSize);
  RC CalculateJoin(int relsInJoin, int newRel, int relSize, float &cost, float &totalTuples, 
       std::map<int, attrStat> &attrs,int &indexAttr, int &indexCond);
  RC IsValidIndexCond(int relsJoined, int condIndex, int relIdx, int &attrIndex);

  RC AddRelToBitmap(int relNum, int &bitmap);
  RC RemoveRelFromBitmap(int relNum, int &bitmap);
  RC ClearBitmap(int &bitmap);
  bool IsBitSet(int relNum, int bitmap);
  RC ConvertBitmapToVec(int bitmap, std::vector<int> &relsInJoin);
  RC ConvertVecToBitmap(std::vector<int> &relsInJoin, int &bitmap);


  RC InitializeBaseCases();
  RC CalcCondsForRel(std::map<int, attrStat> & attrStats, int relsInJoin, int relIdx,
    float &totalTuples, bool &useIdx, int& indexAttr, int&condAttr);
  RC NormalizeStats(std::map<int, attrStat> &attr_stats, float frac, int attrIdx1, int attrIdx2);
  RC ApplyEQCond(std::map<int, attrStat>& attr_stats, int condIdx, float& numTuples);
  RC ApplyLTCond(std::map<int, attrStat>& attr_stats, int condIdx, float& numTuples);
  RC ApplyGTCond(std::map<int, attrStat>& attr_stats, int condIdx, float& numTuples);
  RC ComputeAllSubsets(std::vector<std::string> subsets, int size);
  
  float CalculateNumPages(int numTuples, int tupleLength);

  RC CondToAttrIdx(int condIndex, int &attrIdx1, int &attrIdx2);
  RC ConvertValueToFloat(int condIndex, float &value);
  bool UseCondition(int relsJoined, int condIndex, int relIdx);
  float ConvertStrToFloat(char *string);
  RC AttrToRelIndex(const RelAttr attr, int& relIndex);

private:
  QL_Manager &qlm;
  int nRels;
  int nAttrs;
  int nConds;
  RelCatEntry *rels;
  AttrCatEntry *attrs;
  const Condition *conds;

  std::vector<std::map<int, costElem*> > optcost;

};




#endif
