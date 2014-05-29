//
// File:        ql_error.cc
// Description: QL_PrintError functions
// Authors:     Hugo Rivero (rivero@cs.stanford.edu)
//              Dallan Quass (quass@cs.stanford.edu)
//              Yifei Huang (yifei@stanford.edu)
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include "ql.h"

using namespace std;

//
// Error table
//
static char *QL_WarnMsg[] = {
  (char*)"bad insert",
  (char*)"duplicate relation",
  (char*)"bad attribute",
  (char*)"attribute not found",
  (char*)"bad select condition",
  (char*)"bad call",
  (char*)"condition not met",
  (char*)"bad update value",
  (char*)"end of iterator"
};

static char *QL_ErrorMsg[] = {
  (char*)"Invalid database",
  (char*)"QL Error"
};

//
// RM_PrintError
//
// Desc: Send a message corresponding to a RM return code to cerr
//       Assumes PF_UNIX is last valid RM return code
// In:   rc - return code for which a message is desired
//
void QL_PrintError(RC rc)
{
  // Check the return code is within proper limits
  if (rc >= START_QL_WARN && rc <= QL_LASTWARN)
    // Print warning
    cerr << "QL warning: " << QL_WarnMsg[rc - START_QL_WARN] << "\n";
  // Error codes are negative, so invert everything
  else if (-rc >= -START_QL_ERR && -rc < -QL_LASTERROR)
    // Print error
    cerr << "QL error: " << QL_ErrorMsg[-rc + START_QL_ERR] << "\n";
  else if (rc == PF_UNIX)
#ifdef PC
      cerr << "OS error\n";
#else
      cerr << strerror(errno) << "\n";
#endif
  else if (rc == 0)
    cerr << "QL_PrintError called with return code of 0\n";
  else
    cerr << "QL error: " << rc << " is out of bounds\n";
}
