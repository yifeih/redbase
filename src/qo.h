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

// keeps track of attribute statistics
typedef struct attrStat{
  float numTuples; // num distinct values for this attribute
  float maxValue; // maxValue for attribute
  float minValue; // minValue for attribute
} attrStat;

// keeps track of the join statistics
typedef struct costElem{
  int joins;        // bitmap of rels in (S-a)
  int newRelIndex;  // index of a
  float numTuples;  // number of tuples of 
  float cost;       // cost of joining (S-a) with a to get S
  int indexAttr;    // index attribute. is -1 if no index is used
  int indexCond;    // index condition. is -1 if no index is used
  std::map<int, attrStat> attrs;  // map of attribute statistics
} costElem;


class QO_Manager{
  friend class QL_Manager;

public:
  QO_Manager(QL_Manager &qlm, int nRelations, RelCatEntry *relations, int nAttributes, AttrCatEntry *attributes, 
    int nConditions, const Condition conditions[]);
  ~QO_Manager();
  // prints all the entries in optcost
  RC PrintRels();

  // Returns the list of relations in optimal join order, and 
  // estimated cost and tuple #s
  RC Compute(QO_Rel *relOrder, float &costEst, float &tupleEst);

  // Calculates the optimal join for joining relations in bitmap
  // relsInJoin 
  RC CalculateOptJoin(int relsInJoin, int relSize);

  // calculates the cost/stats of joining relsInJoin and newRel
  RC CalculateJoin(int relsInJoin, int newRel, int relSize, float &cost, float &totalTuples, 
       std::map<int, attrStat> &attrs,int &indexAttr, int &indexCond);
  
  // Checks whether a condition should be used for a given
  // set of (S-a) and a relation a
  RC IsValidIndexCond(int relsJoined, int condIndex, int relIdx, int &attrIndex);

  // Bitmap Operations
  RC AddRelToBitmap(int relNum, int &bitmap);
  RC RemoveRelFromBitmap(int relNum, int &bitmap);
  RC ClearBitmap(int &bitmap);
  bool IsBitSet(int relNum, int bitmap);
  // Converts bitmap to vector of ints, where vector of ints refer
  // to bit positions that are set
  RC ConvertBitmapToVec(int bitmap, std::vector<int> &relsInJoin);
  RC ConvertVecToBitmap(std::vector<int> &relsInJoin, int &bitmap);

  // updates optcost to contain the base cases of 1 relation
  RC InitializeBaseCases();

  // calculates the stats
  RC CalcCondsForRel(std::map<int, attrStat> & attrStats, int relsInJoin, int relIdx,
    float &totalTuples, bool &useIdx, int& indexAttr, int&condAttr);
  
  // For each attribute in attribute stats that is not attrIdx1 or
  // attrIdx2, update its value to numTuples if its value is more than
  // numTuples
  RC NormalizeStats(std::map<int, attrStat> &attr_stats, float frac, float numTuples, int attrIdx1, int attrIdx2);
  
  // calculate the stats for applying different types of conditions
  RC ApplyEQCond(std::map<int, attrStat>& attr_stats, int condIdx, float& numTuples);
  RC ApplyLTCond(std::map<int, attrStat>& attr_stats, int condIdx, float& numTuples);
  RC ApplyGTCond(std::map<int, attrStat>& attr_stats, int condIdx, float& numTuples);
  
  RC ComputeAllSubsets(std::vector<std::string> subsets, int size);
  // Calculate the number of GetPAge calls to filescan through
  // a relation given the number of tuples and tuple length
  float CalculateNumPages(int numTuples, int tupleLength);

  // Retrieves the attribute indices involve din a condition
  RC CondToAttrIdx(int condIndex, int &attrIdx1, int &attrIdx2);
  // Converts a value in a condition to a float value
  RC ConvertValueToFloat(int condIndex, float &value);
  // returns true if we are to use the condition or not for this
  // join calculation
  bool UseCondition(int relsJoined, int condIndex, int relIdx);

  // converts strings to floats
  float ConvertStrToFloat(char *string);

  // Given an attribute, returns the index of the relation it belongs in
  RC AttrToRelIndex(const RelAttr attr, int& relIndex);

private:
  QL_Manager &qlm; // ql_manager relference
  int nRels;  // # of rels in selection
  int nAttrs; // # of attributes in selection
  int nConds; // # of conditions in selections
  RelCatEntry *rels; // pointer to array of RelCatEntries for this selection
  AttrCatEntry *attrs; // pointer to array of AttrCatEntries for this selection
  // pointer to the list of conditions for this selection
  const Condition *conds;

  // optcost array for keeping track of statistics
  // during the dynamic programming
  std::vector<std::map<int, costElem*> > optcost;

};




#endif
