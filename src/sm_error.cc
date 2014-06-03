//
// File:        rm_error.cc
// Description: RM_PrintError functions
// Authors:     Hugo Rivero (rivero@cs.stanford.edu)
//              Dallan Quass (quass@cs.stanford.edu)
//              Yifei Huang (yifei@stanford.edu)
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include "sm.h"

using namespace std;

//
// Error table
//
static char *SM_WarnMsg[] = {
  (char*)"cannot close db",
  (char*)"bad relation name",
  (char*)"bad relation specification",
  (char*)"bad attribute",
  (char*)"invalid attribute",
  (char*)"attribute indexed already",
  (char*)"attribute has no index",
  (char*)"invalid/bad load file",
  (char*)"bad set statement"
};

static char *SM_ErrorMsg[] = {
  (char*)"Invalid database",
  (char*)"SM Error"
};

//
// RM_PrintError
//
// Desc: Send a message corresponding to a RM return code to cerr
//       Assumes PF_UNIX is last valid RM return code
// In:   rc - return code for which a message is desired
//
void SM_PrintError(RC rc)
{
  // Check the return code is within proper limits
  if (rc >= START_SM_WARN && rc <= SM_LASTWARN)
    // Print warning
    cerr << "SM warning: " << SM_WarnMsg[rc - START_SM_WARN] << "\n";
  // Error codes are negative, so invert everything
  else if (-rc >= -START_SM_ERR && -rc < -SM_LASTERROR)
    // Print error
    cerr << "SM error: " << SM_ErrorMsg[-rc + START_SM_ERR] << "\n";
  else if (rc == PF_UNIX)
#ifdef PC
      cerr << "OS error\n";
#else
      cerr << strerror(errno) << "\n";
#endif
  else if (rc == 0)
    cerr << "SM_PrintError called with return code of 0\n";
  else
    cerr << "SM error: " << rc << " is out of bounds\n";
}
