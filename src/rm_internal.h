//
// File:         rm_internal.h
// Description:  Declarations internal to the record management component
// Authors:      Yifei Huang
//

#ifndef RM_INTERNAL_H
#define RM_INTERNAL_H

#include <cstring>
#include "pf.h"

#define NO_MORE_FREE_PAGES -1 // for the linked list of free pages
                              // to indicate no more free page

// Define the RM page header
struct RM_PageHeader {
  PageNum nextFreePage;
  int numRecords;
};

#include "rm.h"

#endif