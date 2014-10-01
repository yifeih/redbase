//
// ql_node.h
//
// The QL Node interface
//
#ifndef QL_NODE_H
#define QL_NODE_H


/*
 * This holds information about a condition to meet
 */
typedef struct Cond{
  int offset1; // offset of LHS attribute
  // comparator - depends on which operator is used in this condition
  bool (*comparator) (void * , void *, AttrType, int); 
  bool isValue; // whether this is a value or not
  void* data; // the pointer to the data
  int offset2; // the offset of RHS attribute
  int length; // length of LHS attribute
  int length2; // length of RHS value/attribute
  AttrType type; // attribute type
} Cond;

/*
 * The abstract class for nodes
 */
class QL_Node {
  friend class QL_Manager;
  friend class QL_NodeJoin;
public:
  QL_Node(QL_Manager &qlm);
  ~QL_Node();

  virtual RC OpenIt() = 0;
  virtual RC GetNext(char * data) = 0;
  virtual RC GetNextRec(RM_Record &rec) = 0;
  virtual RC CloseIt() = 0;
  virtual RC DeleteNodes() = 0;
  virtual RC PrintNode(int numTabs) = 0;
  virtual bool IsRelNode() = 0;
  virtual RC OpenIt(void *data) = 0;
  virtual RC UseIndex(int attrNum, int indexNumber, void *data) = 0;
  // Prints a condition
  RC PrintCondition(const Condition condition);
  // Given a index of an attribute, returns its offset and length
  RC IndexToOffset(int index, int &offset, int &length);
  // Add a condition to the node
  RC AddCondition(const Condition conditions, int condNum);
  // Check to see if the conditions to this node are met
  RC CheckConditions(char *recData);
  // Get the attribute list for this node, and the list size
  RC GetAttrList(int *&attrList, int &attrListSize);
  // Get the tuple lenght of this node
  RC GetTupleLength(int &tupleLength);
protected:
  QL_Manager &qlm; // Reference to QL manager
  bool isOpen; // Whether the node is open or not
  int tupleLength; // the length of a tuple in this node
  int *attrsInRec; // list of indices of the attribute in this node
  int attrsInRecSize; // size of the attribute list
  bool listsInitialized; // indicator for whether memory has been initialized

  // The condition list
  Cond *condList;
  int condIndex; // the # of conditions currently in this node
  int* condsInNode; // maps the condition from the index in this list, to the 
                    // index in the list in QL
   bool useIndexJoin;
};

/* Project nodes
 */
class QL_NodeProj: public QL_Node {
  friend class QL_Manager;
public:
  QL_NodeProj(QL_Manager &qlm, QL_Node &prevNode);
  ~QL_NodeProj();

  RC OpenIt();
  RC GetNext(char *data);
  RC CloseIt();
  RC GetNextRec(RM_Record &rec);
  RC DeleteNodes();
  RC PrintNode(int numTabs);
  bool IsRelNode();
  RC OpenIt(void *data);
   RC UseIndex(int attrNum, int indexNumber, void *data);

  // Add a projection by specifying the index of the attribute to keep
  RC AddProj(int attrIndex);
  RC ReconstructRec(char *data); // reconstruct the record, and put it in data
  RC SetUpNode(int numAttrToKeep); 
private:
  QL_Node &prevNode; // previous node

  int numAttrsToKeep; // # of attributes to keep

  char *buffer;
};


/* Select nodes
 */
class QL_NodeSel: public QL_Node {
  friend class QL_Manager;
public:
  QL_NodeSel(QL_Manager &qlm, QL_Node &prevNode);
  ~QL_NodeSel();

  RC OpenIt();
  RC GetNext(char *data);
  RC CloseIt();
  RC GetNextRec(RM_Record &rec);
  RC DeleteNodes();
  RC PrintNode(int numTabs);
  bool IsRelNode();
  RC OpenIt(void *data);
   RC UseIndex(int attrNum, int indexNumber, void *data);

  RC AddCondition(const Condition conditions, int condNum);
  RC SetUpNode(int numConds);
private:
  QL_Node& prevNode;

  char *buffer;
};

/* Join nodes
 */
class QL_NodeJoin: public QL_Node {
  friend class QL_Manager;
public:
  QL_NodeJoin(QL_Manager &qlm, QL_Node &node1, QL_Node &node2);
  ~QL_NodeJoin();

  RC OpenIt();
  RC GetNext(char *data);
  RC CloseIt();
  RC GetNextRec(RM_Record &rec);
  RC DeleteNodes();
  RC PrintNode(int numTabs);
  bool IsRelNode();
  RC OpenIt(void *data);
  RC UseIndex(int attrNum, int indexNumber, void *data);

  RC UseIndexJoin(int indexAttr, int subNodeAttr, int indexNumber);
  RC AddCondition(const Condition conditions, int condNum);
  RC SetUpNode(int numConds);
private:
  QL_Node &node1;
  QL_Node &node2;
  int firstNodeSize;
  char * buffer;

  bool gotFirstTuple;

  bool useIndexJoin;
  int indexAttr;
};

/* Relation nodes
 */
class QL_NodeRel: public QL_Node {
  friend class QL_Manager;
  friend class QL_NodeJoin;
public:
  QL_NodeRel(QL_Manager &qlm, RelCatEntry *rEntry);
  ~QL_NodeRel();

  RC OpenIt();
  RC GetNext(char *data);
  RC CloseIt();
  RC GetNextRec(RM_Record &rec);
  RC DeleteNodes();
  RC PrintNode(int numTabs);
  bool IsRelNode();


  RC SetUpNode(int *attrs, int attrlistSize);
  RC UseIndex(int attrNum, int indexNumber, void *data);
  RC OpenIt(void *data);
private:
  RC RetrieveNextRec(RM_Record &rec, char *&recData);
  // relation name, and indicator for whether it's been malloced
  char *relName;
  bool relNameInitialized;

  bool useIndex; // whether to use the index
  int indexNo;  // index number to use
  void *value;  // equality value for index
  int indexAttr; // index of attribute for the index

  RM_FileHandle fh;  // filehandle/scans for retrieving records from relation
  IX_IndexHandle ih;
  RM_FileScan fs;
  IX_IndexScan is;

};





#endif