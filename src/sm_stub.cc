//
// File:        SM component stubs
// Description: Print parameters of all SM_Manager methods
// Authors:     Yifei Huang (yifei@stanford.edu)
//

#include <cstdio>
#include <iostream>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include "redbase.h"
#include "sm.h"
#include "ix.h"
#include "rm.h"
#include <string>
#include <set>
#include "stddef.h"

using namespace std;

bool recInsert_int(char *location, string value, int length){
  int num;
  istringstream(value) >> num;
  //printf("num: %d \n", num);
  memcpy(location, (char*)&num, length);
  return true;
}

bool recInsert_float(char *location, string value, int length){
  //float num;
  //istringstream(value) >> num;
  float num = (float)atof(value.c_str());
  //printf("num: %f \n", num);
  memcpy(location, (char*)&num, length);
  return true;
}

bool recInsert_string(char *location, string value, int length){
  memcpy(location, value.c_str(), length);
  return true;
}

SM_Manager::SM_Manager(IX_Manager &ixm, RM_Manager &rmm) : rmm(rmm), ixm(ixm){
  printIndex = false;
}

SM_Manager::~SM_Manager()
{

}

RC SM_Manager::OpenDb(const char *dbName)
{
  RC rc = 0;
  if(strlen(dbName) > MAX_DB_NAME){
    return (SM_INVALIDDB);
  }

  if(chdir(dbName) < 0){
    cerr << "Cannot chdir to " << dbName << "\n";
    return (SM_INVALIDDB);
  }

  if((rc = rmm.OpenFile("relcat", relcatFH) )){
    return (SM_INVALIDDB);
  }
  if((rc = rmm.OpenFile("attrcat", attrcatFH))) {
    return (SM_INVALIDDB);
  }

  return (0);
}

RC SM_Manager::CloseDb()
{
  RC rc = 0;
  if((rc = rmm.CloseFile(relcatFH) )){
    printf("relation cat error\n");
  }
  if((rc = rmm.CloseFile(attrcatFH))){
    return (rc);
    printf("attribute cat error \n");
  } 

  return (0);
}

bool SM_Manager::isValidAttrType(AttrInfo attribute){

  AttrType type = attribute.attrType;
  int length = attribute.attrLength;
  if(type == INT && length == 4)
    return true;
  if(type == FLOAT && length == 4)
    return true;
  if(type == STRING && (length > 0) && length < MAXSTRINGLEN)
    return true;

  return false;
}

RC SM_Manager::CreateTable(const char *relName,
                           int        attrCount,
                           AttrInfo   *attributes)
{
  cout << "CreateTable\n"
    << "   relName     =" << relName << "\n"
    << "   attrCount   =" << attrCount << "\n";
  for (int i = 0; i < attrCount; i++)
    cout << "   attributes[" << i << "].attrName=" << attributes[i].attrName
        << "   attrType="
        << (attributes[i].attrType == INT ? "INT" :
            attributes[i].attrType == FLOAT ? "FLOAT" : "STRING")
        << "   attrLength=" << attributes[i].attrLength << "\n";

  RC rc = 0;
  set<string> relAttributes;
  // CHECK FOR ERRORS
  if(attrCount > MAXATTRS || attrCount < 1)
    return (SM_BADREL);
  if(strlen(relName) > MAXNAME)
    return (SM_BADRELNAME);

  int totalRecSize = 0;
  for(int i = 0; i < attrCount; i++){
    if(strlen(attributes[i].attrName) > MAXNAME) // check name size
      return (SM_BADATTR);
    if(! isValidAttrType(attributes[i])) // check type
      return (SM_BADATTR);
    totalRecSize += attributes[i].attrLength; 
    string attrString(attributes[i].attrName); // check attribute dups
    bool exists = (relAttributes.find(attrString) != relAttributes.end());
    if(exists)
      return (SM_BADREL);
    else
      relAttributes.insert(attrString);
  }

  // Passed error check. Now information in relcat and attrcat

  // Create a file for this relation. This will check for duplicate tables
  // of the same name.
  if((rc = rmm.CreateFile(relName, totalRecSize)))
    return (SM_BADRELNAME);

  // For each attribute, insert into attrcat:
  RID rid;
  int currOffset = 0;
  for(int i = 0; i < attrCount; i++){
    AttrInfo attr = attributes[i];
    if((rc = InsertAttrCat(relName, attr, currOffset, i)))
      return (rc);
    currOffset += attr.attrLength;
  }
    
  // Insert into RelCat
  if((rc = InsertRelCat(relName, attrCount, totalRecSize)))
    return (rc);

  if((rc = attrcatFH.ForcePages()) || (rc = relcatFH.ForcePages()))
    return (rc);

  return (0);
}

RC SM_Manager::InsertRelCat(const char *relName, int attrCount, int recSize){
  RC rc = 0;
  RelCatEntry* rEntry = (RelCatEntry *) malloc(sizeof(RelCatEntry));
  memcpy(rEntry->relName, relName, MAXNAME + 1);
  rEntry->tupleLength = recSize;
  rEntry->attrCount = attrCount;
  rEntry->indexCount = 0;
  rEntry->indexCurrNum = 0;

  RID relRID;
  rc = relcatFH.InsertRec((char *&)rEntry, relRID);
  free(rEntry);

  return rc;
}

RC SM_Manager::InsertAttrCat(const char *relName, AttrInfo attr, int offset, int attrNum){
  RC rc = 0;
  AttrCatEntry *aEntry = (AttrCatEntry *)malloc(sizeof(AttrCatEntry));

  memcpy(aEntry->relName, relName, MAXNAME + 1);
  memcpy(aEntry->attrName, attr.attrName, MAXNAME + 1);
  aEntry->offset = offset;
  aEntry->attrType = attr.attrType;
  aEntry->attrLength = attr.attrLength;
  aEntry->indexNo = NO_INDEXES;
  aEntry->attrNum = attrNum;

  RID attrRID;
  rc = attrcatFH.InsertRec((char *&)aEntry, attrRID);
  free(aEntry);
  return rc;

}


RC SM_Manager::DropTable(const char *relName)
{
  cout << "DropTable\n   relName=" << relName << "\n";
  RC rc = 0;

  if(strlen(relName) > MAXNAME)
    return (SM_BADRELNAME);
  // Try to destroy the table. This should detect whether the file is there
  if((rc = rmm.DestroyFile(relName))){
    //printf("return code: %d \n", rc);
    return (SM_BADRELNAME);
  }

  RM_Record relRec;
  RelCatEntry *relEntry;
  if((rc = GetRelEntry(relName, relRec, relEntry)))
    return (rc);
  int numAttr = relEntry->attrCount;


  SM_AttrIterator attrIt;
  if((rc = attrIt.OpenIterator(attrcatFH, const_cast<char*>(relName))))
    return (rc);
  AttrCatEntry *attrEntry;
  RM_Record attrRec;
  for(int i=0; i < numAttr; i++){
    if((rc = attrIt.GetNextAttr(attrRec, attrEntry))){
      return (rc);
    }

    if((attrEntry->indexNo != NO_INDEXES)){
      if((rc = DropIndex(relName, attrEntry->attrName)))
        return (rc);
    }
    RID attrRID;
    if((rc = attrRec.GetRid(attrRID)) || (rc = attrcatFH.DeleteRec(attrRID)))
      return (rc);

  }
  if((rc = attrIt.CloseIterator()))
    return (rc);

  RID relRID;
  if((rc = relRec.GetRid(relRID)) || (rc = relcatFH.DeleteRec(relRID)))
    return (rc);

  return (0);
}

RC SM_Manager::GetRelEntry(const char *relName, RM_Record &relRec, RelCatEntry *&entry){
  // Search for the relName entry
  RC rc = 0;
  RM_FileScan fs;
  if((rc = fs.OpenScan(relcatFH, STRING, MAXNAME+1, 0, EQ_OP, const_cast<char*>(relName))))
    return (rc);
  if((rc = fs.GetNextRec(relRec)))
    return (SM_BADRELNAME);
  
  if((rc = fs.CloseScan()))
    return (rc);

  if((rc = relRec.GetData((char *&)entry)))
    return (rc);

  return (0);
}

RC SM_Manager::GetAttrEntry(RM_FileScan& fs, RM_Record &attrRec, AttrCatEntry *&entry){
  RC rc = 0;
  if((rc = fs.GetNextRec(attrRec)))
    return (rc);
  if((rc = attrRec.GetData((char *&)entry)))
    return (rc);

  return (rc);
}


RC SM_Manager::CreateIndex(const char *relName,
                           const char *attrName)
{
  cout << "CreateIndex\n"
    << "   relName =" << relName << "\n"
    << "   attrName=" << attrName << "\n";

  RC rc = 0;
  RM_Record relRec;
  RelCatEntry *rEntry;
  if((rc = GetRelEntry(relName, relRec, rEntry)))
    return (rc);

  
  RM_Record attrRec;
  AttrCatEntry *aEntry;
  if((rc = FindAttr(relName, attrName, attrRec, aEntry))){
    printf("rawr\n");
    return (rc);
  }

  // check there isnt already an index
  if(aEntry->indexNo != NO_INDEXES)
    return (SM_INDEXEDALREADY);

  printf("gets past index check\n");
  if((rc = ixm.CreateIndex(relName, rEntry->indexCurrNum, aEntry->attrType, aEntry->attrLength)))
    return (rc);

  IX_IndexHandle ih;
  RM_FileHandle fh;
  RM_FileScan fs;
  if((rc = ixm.OpenIndex(relName, rEntry->indexCurrNum, ih)))
    return (rc);
  if((rc = rmm.OpenFile(relName, fh)))
    return (rc);

  // scan through the entire file:
  if((rc = fs.OpenScan(fh, INT, 4, 0, NO_OP, NULL))){
    return (rc);
  }
  RM_Record rec;
  while(fs.GetNextRec(rec) != RM_EOF){
    char *pData;
    RID rid;
    if((rc = rec.GetData(pData) || (rc = rec.GetRid(rid))))
      return (rc);
    if((rc = ih.InsertEntry(pData+ aEntry->offset, rid)))
      return (rc);
  }
  if((rc = fs.CloseScan()) || (rc = rmm.CloseFile(fh)) || (rc = ixm.CloseIndex(ih)))
    return (rc);
  
  // rewrite entry:
  aEntry->indexNo = rEntry->indexCurrNum;
  rEntry->indexCurrNum++;
  rEntry->indexCount++;

  // write both back
  if((rc = relcatFH.UpdateRec(relRec)) || (rc = attrcatFH.UpdateRec(attrRec)))
    return (rc);
  if((rc = relcatFH.ForcePages() || (rc = attrcatFH.ForcePages())))
    return (rc);

  return (0);
}

RC SM_Manager::FindAttr(const char *relName, const char *attrName, RM_Record &attrRec, AttrCatEntry *&entry){
  RC rc = 0;
  RM_Record relRec;
  RelCatEntry * rEntry;
  if((rc = GetRelEntry(relName, relRec, rEntry)))
    return (rc);
  
  SM_AttrIterator attrIt;
  if((rc = attrIt.OpenIterator(attrcatFH, const_cast<char*>(relName))))
    return (rc);
  bool notFound = true;
  while(notFound){
    if((RM_EOF == attrIt.GetNextAttr(attrRec, entry))){
      break;
    }
    if(strncmp(entry->attrName, attrName, MAXNAME + 1) == 0){
      notFound = false;
      break;
    }
  }
  if((rc = attrIt.CloseIterator()))
    return (rc);

  if(notFound == true)
    return (SM_INVALIDATTR);
  return (rc);

}

RC SM_Manager::DropIndex(const char *relName,
                         const char *attrName)
{
  cout << "DropIndex\n"
    << "   relName =" << relName << "\n"
    << "   attrName=" << attrName << "\n";

  RC rc = 0;
  RM_Record relRec;
  RelCatEntry *rEntry;
  if((rc = GetRelEntry(relName, relRec, rEntry)))
    return (rc);

  RM_Record attrRec;
  AttrCatEntry *aEntry;
  if((rc = FindAttr(relName, attrName, attrRec, aEntry))){
    return (rc);
  }

  if((aEntry->indexNo == NO_INDEXES))
    return (SM_NOINDEX);
  
  if((rc = ixm.DestroyIndex(relName, aEntry->indexNo)))
    return (rc);

  // Update entries:
  aEntry->indexNo = NO_INDEXES;
  rEntry->indexCount--;

  // write both back
  if((rc = relcatFH.UpdateRec(relRec)) || (rc = attrcatFH.UpdateRec(attrRec)))
    return (rc);
  if((rc = relcatFH.ForcePages() || (rc = attrcatFH.ForcePages())))
    return (rc);

  return (0);
}

RC SM_Manager::PrepareAttr(RelCatEntry *rEntry, Attr* attributes){
  RC rc = 0; 
  SM_AttrIterator attrIt;
  if((rc = attrIt.OpenIterator(attrcatFH, rEntry->relName)))
    return (rc);
  RM_Record attrRec;
  AttrCatEntry *aEntry;
  for(int i = 0; i < rEntry->attrCount; i++){
    if((rc = attrIt.GetNextAttr(attrRec, aEntry)))
      return (rc);
    int slot = aEntry->attrNum;
    attributes[slot].offset = aEntry->offset;
    attributes[slot].type = aEntry->attrType;
    attributes[slot].length = aEntry->attrLength;
    attributes[slot].indexNo = aEntry->indexNo;
    // prepare index:
    if((aEntry->indexNo != NO_INDEXES)){
      if((rc = ixm.OpenIndex(rEntry->relName, aEntry->indexNo, attributes[slot].ih)))
        return (rc);
    }


    if(aEntry->attrType == INT){
      //printf("setting parser pointer\n");
      attributes[slot].recInsert = &recInsert_int;
    }
    else if(aEntry->attrType == FLOAT)
      attributes[slot].recInsert = &recInsert_float;
    else
      attributes[slot].recInsert = &recInsert_string;
  }
  if((rc = attrIt.CloseIterator()))
    return (rc);
  return (0);
}

RC SM_Manager::Load(const char *relName,
                    const char *fileName)
{
    cout << "Load\n"
         << "   relName =" << relName << "\n"
         << "   fileName=" << fileName << "\n";

  RC rc = 0;
  RM_Record relRec;
  RelCatEntry *rEntry;
  if((rc = GetRelEntry(relName, relRec, rEntry)))
    return (rc);

  Attr* attributes = (Attr *)malloc(sizeof(Attr)*rEntry->attrCount);
  if((rc = PrepareAttr(rEntry, attributes)))
    return (rc);

  RM_FileHandle relFH;
  if((rc = rmm.OpenFile(relName, relFH)))
    return (rc);

  rc = OpenAndLoadFile(relFH, fileName, attributes, rEntry->attrCount,
    rEntry->tupleLength);
  if((rc = CleanUpAttr(attributes, rEntry->attrCount)))
    return (rc);

  if((rc = rmm.CloseFile(relFH)))
    return (rc);
  return (rc);
}

RC SM_Manager::OpenAndLoadFile(RM_FileHandle &relFH, const char *fileName, Attr* attributes, int attrCount, 
  int recLength){
  RC rc = 0;
  char* record = (char *)malloc(recLength);

  ifstream f(fileName);
  if(f.fail()){
    cout << "cannot open file :( " << endl;
    return (SM_BADLOADFILE);
  }

  string line, token;
  string delimiter = ",";
  while (getline(f, line)) {
    for(int i=0; i <attrCount; i++){
      if(line.size() == 0){
        free(record);
        printf("RAWR\n");
        return (SM_BADLOADFILE);
      }
      size_t pos = line.find(delimiter);
      if(pos == string::npos)
        pos = line.size();
      token = line.substr(0, pos);
      cout << token << endl;
      line.erase(0, pos + delimiter.length());
      attributes[i].recInsert(record + attributes[i].offset, token, attributes[i].length);
    }
    RID recRID;
    if((rc = relFH.InsertRec(record, recRID))){
      free(record);
      return (rc);
    }
    for(int i=0; i < attrCount; i++){
      if(attributes[i].indexNo != NO_INDEXES){
        if((rc = attributes[i].ih.InsertEntry(record + attributes[i].offset, recRID)))
          return (rc);
      }
    }
    //printf("record : %d, %d\n", *(int*)record, *(int*)(record+4));
  }

  free(record);

  return (0);
}

RC SM_Manager::CleanUpAttr(Attr* attributes, int attrCount){
  RC rc = 0;
  for(int i=0; i < attrCount; i++){
    if(attributes[i].indexNo != NO_INDEXES){
      if((rc = ixm.CloseIndex(attributes[i].ih)))
        return (rc);
    }
  }
  free(attributes);
  return (rc);
}

RC SM_Manager::Print(const char *relName)
{
  cout << "Print\n"
    << "   relName=" << relName << "\n";

  RC rc = 0;
  RM_Record relRec;
  RelCatEntry *relEntry;
  if((rc = GetRelEntry(relName, relRec, relEntry)))
    return (SM_BADRELNAME);
  int numAttr = relEntry->attrCount;
  DataAttrInfo * attributes = (DataAttrInfo *)malloc(numAttr* sizeof(DataAttrInfo));
  if((rc = SetUpPrint(relEntry, attributes)))
    return (rc);

  Printer printer(attributes, relEntry->attrCount);
  printer.PrintHeader(cout);

  // open file:
  RM_FileHandle fh;
  RM_FileScan fs;
  if((rc = rmm.OpenFile(relName, fh)) || (rc = fs.OpenScan(fh, INT, 4, 0, NO_OP, NULL)))
    return (rc);

  RM_Record rec;
  while(fs.GetNextRec(rec) != RM_EOF){
    char *pData;
    if((rec.GetData(pData)))
      return (rc);
    printer.Print(cout, pData);
  }
  fs.CloseScan();

  printer.PrintFooter(cout);
  free(attributes);

  return (0);
}

RC SM_Manager::SetUpPrint(RelCatEntry* rEntry, DataAttrInfo *attributes){
  RC rc = 0;
  RID attrRID;
  RM_Record attrRec;
  AttrCatEntry *aEntry;

  SM_AttrIterator attrIt;
  if((rc = attrIt.OpenIterator(attrcatFH, rEntry->relName)))
    return (rc);

  for(int i=0; i < rEntry->attrCount; i++){
    if((rc = attrIt.GetNextAttr(attrRec, aEntry))){
      return (rc);
    }
    int slot = aEntry->attrNum;

    memcpy(attributes[slot].relName, aEntry->relName, MAXNAME + 1);
    memcpy(attributes[slot].attrName, aEntry->attrName, MAXNAME + 1);
    attributes[slot].offset = aEntry->offset;
    attributes[slot].attrType = aEntry->attrType;
    attributes[slot].attrLength = aEntry->attrLength;
    attributes[slot].indexNo = aEntry->indexNo;
  }
  if((rc = attrIt.CloseIterator()))
    return (rc);

  return (rc);
}

RC SM_Manager::Set(const char *paramName, const char *value)
{
    cout << "Set\n"
         << "   paramName=" << paramName << "\n"
         << "   value    =" << value << "\n";
    if(strncmp(paramName, "printIndex", 10) == 0 && strncmp(value, "true", 4) ==0){
      printIndex = true;
    }
    else
      printIndex = false;

    return (0);
}

RC SM_Manager::Help()
{
  cout << "Help\n";
  RC rc = 0;
  DataAttrInfo * attributes = (DataAttrInfo *)malloc(4* sizeof(DataAttrInfo));
  if((rc = SetUpRelCatAttributes(attributes)))
    return (rc);
  Printer printer(attributes, 4);
  printer.PrintHeader(cout);

  RM_FileScan fs;
  if((rc = fs.OpenScan(relcatFH, INT, 4, 0, NO_OP, NULL)))
    return (rc);

  RM_Record rec;
  while(fs.GetNextRec(rec) != RM_EOF){
    char *pData;
    if((rec.GetData(pData)))
      return (rc);
    printer.Print(cout, pData);
  }

  fs.CloseScan();
  printer.PrintFooter(cout);
  free(attributes);

  return (0);
}

RC SM_Manager::SetUpRelCatAttributes(DataAttrInfo *attributes){
  int numAttr = 4;
  for(int i= 0; i < numAttr; i++){
    memcpy(attributes[i].relName, "relcat", strlen("relcat") + 1);
    attributes[i].indexNo = 0;
  }
  
  memcpy(attributes[0].attrName, "relName", MAXNAME + 1);
  memcpy(attributes[1].attrName, "tupleLength", MAXNAME + 1);
  memcpy(attributes[2].attrName, "attrCount", MAXNAME + 1);
  memcpy(attributes[3].attrName, "indexCount", MAXNAME + 1);

  attributes[0].offset = (int) offsetof(RelCatEntry,relName);
  attributes[1].offset = (int) offsetof(RelCatEntry,tupleLength);
  attributes[2].offset = (int) offsetof(RelCatEntry,attrCount);
  attributes[3].offset = (int) offsetof(RelCatEntry,indexCount);

  attributes[0].attrType = STRING;
  attributes[1].attrType = INT;
  attributes[2].attrType = INT;
  attributes[3].attrType = INT;

  attributes[0].attrLength = MAXNAME + 1;
  attributes[1].attrLength = 4;
  attributes[2].attrLength = 4;
  attributes[3].attrLength = 4;

  return (0);
}


RC SM_Manager::Help(const char *relName)
{
    cout << "Help\n"
         << "   relName=" << relName << "\n";
  RC rc = 0;
  RM_FileScan fs;
  RM_Record rec;

  // Check that this relation exists:
  
  
  if((rc = fs.OpenScan(relcatFH, STRING, MAXNAME+1, 0, EQ_OP, const_cast<char*>(relName))))
    return (rc);

  if(fs.GetNextRec(rec) == RM_EOF){
    fs.CloseScan();
    return (SM_BADRELNAME);
  }
  fs.CloseScan();
  

  DataAttrInfo * attributes = (DataAttrInfo *)malloc(6* sizeof(DataAttrInfo));
  if((rc = SetUpAttrCatAttributes(attributes)))
    return (rc);
  Printer printer(attributes, 6);
  printer.PrintHeader(cout);


  if((rc = fs.OpenScan(attrcatFH, STRING, MAXNAME+1, 0, EQ_OP, const_cast<char*>(relName))))
    return (rc);

  while(fs.GetNextRec(rec) != RM_EOF){
    char *pData;
    if((rec.GetData(pData)))
      return (rc);
    printer.Print(cout, pData);
  }

  if((rc = fs.CloseScan() ))
    return (rc);

  if((rc = fs.OpenScan(attrcatFH, STRING, MAXNAME+1, 0, EQ_OP, const_cast<char*>(relName))))
    return (rc);
  while(fs.GetNextRec(rec) != RM_EOF){
    char *pData;
    if((rec.GetData(pData)))
      return (rc);
    AttrCatEntry *attr = (AttrCatEntry*)pData;
    if((attr->indexNo != NO_INDEXES)){
      IX_IndexHandle ih;
      if((rc = ixm.OpenIndex(relName, attr->indexNo, ih)))
        return (rc);
      printf("successfully opens \n");
      if((rc = ih.PrintIndex()) || (rc = ixm.CloseIndex(ih)))
        return (rc);
    }
  }

  printer.PrintFooter(cout);
  free(attributes);
  return (0);
}

RC SM_Manager::SetUpAttrCatAttributes(DataAttrInfo *attributes){
  int numAttr = 6;
  for(int i= 0; i < numAttr; i++){
    memcpy(attributes[i].relName, "attrcat", strlen("attrcat") + 1);
    attributes[i].indexNo = 0;
  }
  
  memcpy(attributes[0].attrName, "relName", MAXNAME + 1);
  memcpy(attributes[1].attrName, "attrName", MAXNAME + 1);
  memcpy(attributes[2].attrName, "offset", MAXNAME + 1);
  memcpy(attributes[3].attrName, "attrType", MAXNAME + 1);
  memcpy(attributes[4].attrName, "attrLength", MAXNAME + 1);
  memcpy(attributes[5].attrName, "indexNo", MAXNAME + 1);

  attributes[0].offset = (int) offsetof(AttrCatEntry,relName);
  attributes[1].offset = (int) offsetof(AttrCatEntry,attrName);
  attributes[2].offset = (int) offsetof(AttrCatEntry,offset);
  attributes[3].offset = (int) offsetof(AttrCatEntry,attrType);
  attributes[4].offset = (int) offsetof(AttrCatEntry,attrLength);
  attributes[5].offset = (int) offsetof(AttrCatEntry,indexNo);

  attributes[0].attrType = STRING;
  attributes[1].attrType = STRING;
  attributes[2].attrType = INT;
  attributes[3].attrType = INT;
  attributes[4].attrType = INT;
  attributes[5].attrType = INT;

  attributes[0].attrLength = MAXNAME + 1;
  attributes[1].attrLength = MAXNAME + 1;
  attributes[2].attrLength = 4;
  attributes[3].attrLength = 4;
  attributes[4].attrLength = 4;
  attributes[5].attrLength = 4;

  return (0);
}

/*
void SM_PrintError(RC rc)
{
    cout << "SM_PrintError\n   rc=" << rc << "\n";
}
*/

