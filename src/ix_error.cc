//
// File:        ix_error.cc
// Description: IX_PrintError functions
// Authors:     Hugo Rivero (rivero@cs.stanford.edu)
//              Dallan Quass (quass@cs.stanford.edu)
//              Yifei Huang (yifei@stanford.edu)
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include "ix_internal.h"

using namespace std;

//
// Error table
//
static char *IX_WarnMsg[] = {
  (char*)"bad index specification",
  (char*)"bad index name",
  (char*)"invalid index handle",
  (char*)"invalid index file",
  (char*)"page is full",
  (char*)"invalid file",
  (char*)"invalid bucket number",
  (char*)"cannot insert duplicate entry",
  (char*)"invalid scan instance",
  (char*)"invalid record entry",
  (char*)"end of file",
  (char*)"IX warning"
};

static char *IX_ErrorMsg[] = {
  (char*)"IX error"
};

//
// RM_PrintError
//
// Desc: Send a message corresponding to a RM return code to cerr
//       Assumes PF_UNIX is last valid RM return code
// In:   rc - return code for which a message is desired
//
void IX_PrintError(RC rc)
{
  // Check the return code is within proper limits
  if (rc >= START_IX_WARN && rc <= IX_LASTWARN)
    // Print warning
    cerr << "IX warning: " << IX_WarnMsg[rc - START_IX_WARN] << "\n";
  // Error codes are negative, so invert everything
  else if (-rc >= -START_IX_ERR && -rc < -IX_LASTERROR)
    // Print error
    cerr << "IX error: " << IX_ErrorMsg[-rc + START_IX_ERR] << "\n";
  else if (rc == PF_UNIX)
#ifdef PC
      cerr << "OS error\n";
#else
      cerr << strerror(errno) << "\n";
#endif
  else if (rc == 0)
    cerr << "IX_PrintError called with return code of 0\n";
  else
    cerr << "IX error: " << rc << " is out of bounds\n";
}