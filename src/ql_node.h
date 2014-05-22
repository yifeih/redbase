//
// ql_node.h
//
// The QL Node interface
//
#ifndef QL_NODE_H
#define QL_NODE_H

typedef struct Cond{
  int offset1;
  bool (*comparator) (void * , void *, AttrType, int);
  bool isValue;
  void* data;
  int offset2;
  int length;
  AttrType type;
} Cond;

class QL_Node {
public:
  QL_Node(QL_Manager &qlm);
  ~QL_Node();

  virtual RC OpenIt() = 0;
  virtual RC GetNext(char * data) = 0;
  virtual RC GetNextRec(RM_Record &rec) = 0;
  virtual RC CloseIt() = 0;
  virtual RC DeleteNodes() = 0;
  virtual RC GetAttrList(int *attrList, int &attrListSize) = 0;
  virtual RC GetTupleLength(int &tupleLength) = 0;
  virtual RC PrintNode(int numTabs) = 0;
  RC IndexToOffset(int index, int &offset, int &length);
protected:
  QL_Manager &qlm;
  bool isOpen;
  int tupleLength;
  int *attrsInRec;
  int attrsInRecSize;
  bool listsInitialized;
};



class QL_NodeProj: public QL_Node {
public:
  QL_NodeProj(QL_Manager &qlm, QL_Node &prevNode);
  ~QL_NodeProj();

  RC OpenIt();
  RC GetNext(char *data);
  RC CloseIt();
  RC GetNextRec(RM_Record &rec);
  RC DeleteNodes();
  RC GetAttrList(int *attrList, int &attrListSize);
  RC GetTupleLength(int &tupleLength);

  RC AddProj(AttrCatEntry * attr);
private:
  QL_Node &prevNode;
};



class QL_NodeSel: public QL_Node {
public:
  QL_NodeSel(QL_Manager &qlm, QL_Node &prevNode);
  ~QL_NodeSel();

  RC OpenIt();
  RC GetNext(char *data);
  RC CloseIt();
  RC GetNextRec(RM_Record &rec);
  RC DeleteNodes();
  RC GetAttrList(int *attrList, int &attrListSize);
  RC GetTupleLength(int &tupleLength);

  RC AddCondition(const Condition conditions, int condNum);
  RC SetUpNode(int numConds);
private:
  //RC IndexToOffset(int index, int &offset, int &length);
  QL_Node& prevNode;
  Cond *condList;
  int condIndex;
  int* condsInNode;
  char *buffer;
};

class QL_NodeJoin: public QL_Node {
public:
  QL_NodeJoin(QL_Manager &qlm, QL_Node &node1, QL_Node &node2);
  ~QL_NodeJoin();

  RC OpenIt();
  RC GetNext(char *data);
  RC CloseIt();
  RC GetNextRec(RM_Record &rec);
  RC DeleteNodes();
  RC GetAttrList(int *attrList, int &attrListSize);
  RC GetTupleLength(int &tupleLength);

  RC AddCondition(const Condition conditions, int condNum);
  RC SetUpNode(int numConds);

private:
  //RC IndexToOffset(int index, int &offset, int &length);
  RC CheckConditions(char *recData);
  QL_Node &node1;
  QL_Node &node2;
  int firstNodeSize;
  //char *buffer1;
  //char *buffer2;
  char * buffer;
  Cond *condList;
  int condIndex;
  int* condsInNode;
};


class QL_NodeRel: public QL_Node {
public:
  QL_NodeRel(QL_Manager &qlm, RelCatEntry *rEntry);
  ~QL_NodeRel();

  RC OpenIt();
  RC GetNext(char *data);
  RC CloseIt();
  RC GetNextRec(RM_Record &rec);
  RC DeleteNodes();
  RC GetAttrList(int *attrList, int &attrListSize);
  RC GetTupleLength(int &tupleLength);

  RC SetUpNode(int *attrs, int attrlistSize);
  RC UseIndex(int indexNumber, void *data);
private:
  //RC IndexToOffset(int index, int &offset, int &length);
  RC RetrieveNextRec(RM_Record &rec, char *&recData);
  char *relName;
  bool relNameInitialized;

  bool useIndex;
  int indexNo;
  void *value;

  RM_FileHandle fh;
  IX_IndexHandle ih;
  RM_FileScan fs;
  IX_IndexScan is;
};



/*
 * This class is used for Update and Delete since it
 * holds a function GetNextRec that does not
 * return an error.
 */
class Node_Rel: public QL_Node {
public:
  Node_Rel(QL_Manager &qlm, RelCatEntry* rEntry);
  ~Node_Rel();

  RC OpenIt();
  RC GetNext(char * data);
  RC CloseIt();
  RC GetNextRec(RM_Record &rec);
  RC DeleteNodes();
  RC GetAttrList(int *attrList, int &attrListSize);
  RC GetTupleLength(int &tupleLength);
  RC PrintNode(int numTabs);

  RC SetUpRel(int *attrs, int attrlistSize, int numConds);
  RC AddCondition(const Condition condition);
private:
  RC IndexToOffset(int index, int &offset, int &length);
  RC GetNextRecData(RM_Record &rec, char *&recData);
  RC CheckConditions(char *recData);

  RM_FileHandle fh;
  IX_IndexHandle ih;
  RM_FileScan fs;
  IX_IndexScan is;
  bool useIndex;
  int indexNo;
  Cond eqCond;
  Cond *condList;
  int* condsInNode;
  int condIndex;
  //char relName[MAXNAME + 1];
  char *relName;
  bool relNameInitialized;

};


#endif