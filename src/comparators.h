#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include <stdlib.h>

static int compare_string(void *value1, void* value2, int attrLength){
  return strncmp((char *) value1, (char *) value2, attrLength);
}

static int compare_int(void *value1, void* value2, int attrLength){
  if((*(int *)value1 < *(int *)value2))
    return -1;
  else if((*(int *)value1 > *(int *)value2))
    return 1;
  else
    return 0;
}

static int compare_float(void *value1, void* value2, int attrLength){
  if((*(float *)value1 < *(float *)value2))
    return -1;
  else if((*(float *)value1 > *(float *)value2))
    return 1;
  else
    return 0;
}

