//
// File:        pf_error.cc
// Description: PF_PrintError function
// Authors:     Hugo Rivero (rivero@cs.stanford.edu)
//              Dallan Quass (quass@cs.stanford.edu)
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include "rm_internal.h"

using namespace std;

//
// Error table
//
static char *RM_WarnMsg[] = {
  (char*)"invalid RID",
  (char*)"bad record size",
  (char*)"invalid record object",
  (char*)"invalid bit operation",
  (char*)"page is full",
  (char*)"invalid file",
  (char*)"invalid file handle",
  (char*)"invalid file scan",
  (char*)"end of page",
  (char*)"end of file",
  (char*)"invalid filename"
};

static char *RM_ErrorMsg[] = {
  (char*)"no memory",
  (char*)"no buffer space",
  (char*)"incomplete read of page from file",
  (char*)"incomplete write of page to file",
  (char*)"incomplete read of header from file",
  (char*)"incomplete write of header from file",
  (char*)"new page to be allocated already in buffer",
  (char*)"hash table entry not found",
  (char*)"page already in hash table",
  (char*)"invalid file name"
};

//
// PF_PrintError
//
// Desc: Send a message corresponding to a PF return code to cerr
//       Assumes PF_UNIX is last valid PF return code
// In:   rc - return code for which a message is desired
//
void RM_PrintError(RC rc)
{
  // Check the return code is within proper limits
  if (rc >= START_RM_WARN && rc <= RM_LASTWARN)
    // Print warning
    cerr << "RM warning: " << RM_WarnMsg[rc - START_RM_WARN] << "\n";
  // Error codes are negative, so invert everything
  else if (-rc >= -START_RM_ERR && -rc < -RM_LASTERROR)
    // Print error
    cerr << "RM error: " << RM_ErrorMsg[-rc + START_RM_ERR] << "\n";
  else if (rc == PF_UNIX)
#ifdef PC
      cerr << "OS error\n";
#else
      cerr << strerror(errno) << "\n";
#endif
  else if (rc == 0)
    cerr << "RM_PrintError called with return code of 0\n";
  else
    cerr << "RM error: " << rc << " is out of bounds\n";
}
