#ifndef CPFF_STATISTIC_H
#define CPFF_STATISTIC_H

  #include <stdio.h>
  #include <stdlib.h>
  #include <math.h>
  #include <string.h>

  #include "cpff_parameter.h"
  #include "cpff_structure.h"
	#include "cpff_debug.h"

	void statistics_done_func(systemInfo *sys, userInfo *user, REQ *r, char *reqType, double systemTime);
  void reset_second_value(systemInfo *sysInfo, userInfo *user);
  void reset_period_value(systemInfo *sysInfo, userInfo *user);
  void second_record_statistics(systemInfo *sysInfo, userInfo *user, double systemTime, FILE **secondStatisticRecord);
  void second_csv_statistics(systemInfo *sysInfo, userInfo *user, double systemTime, FILE **systemSecondRecord, FILE **eachUserSecondRecord);
  void period_record_statistics(systemInfo *sysInfo, userInfo *user, double systemTime, FILE **periodStatisticRecord);
  void period_csv_statistics(systemInfo *sysInfo, userInfo *user, double systemTime, FILE **systemPeriodRecord, FILE **eachUserPeriodRecord);
  void final_result_statistics(systemInfo *sysInfo, userInfo *user, FILE **finalResult);
  
#endif