#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>   //for fork()


#include "cpff_structure.h"
#include "cpff_prize_caching.h"

/* 選擇SSD Cache管理測略 */
#ifdef CPFF_STATIC_CACHING_SPACE
  #include "cpff_static_caching_space.h"
#elif defined CPFF_DYNAMIC_CACHING_SPACE
  #include "cpff_dynamic_caching_space.h"
#elif defined CPFF_COMPETITION_CACHING_SPACE
  #include "cpff_compition_caching_space.h"
#endif

#include "cpff_ipc.h"
#include "cpff_debug.h"