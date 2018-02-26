#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>   //for fork()


#include "cpff_structure.h"
#include "cpff_prize_caching.h"

/* 選擇SSD Cache管理測略 */
#ifdef STATIC_CACHING_SPACE
  #include "cpff_static_caching_space.h"
#elif defined DYNAMIC_CACHING_SPACE
  #include "cpff_dynamic_caching_space.h"
#elif defined COMPETITION_CACHING_SPACE
  #include "cpff_competition_caching_space.h"
#endif

/* 選擇Credit管理測略 */
#ifdef STATIC_CREDIT
  #include "cpff_static_credit.h"
#elif defined DYNAMIC_CREDIT
  #include "cpff_dynamic_credit.h"
#endif

#include "cpff_ipc.h"
#include "cpff_debug.h"


void init_disksim();
void rm_disksim();
void initialize(char *par[]);
void execute_CPFF_framework();
double shift_cpffSystemTime(double ssdReqCompleteTime, double hddReqCompleteTime);
void statistics_done_func(REQ *r, char *reqType);