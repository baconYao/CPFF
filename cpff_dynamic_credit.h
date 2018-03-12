#ifndef CPFF_DYNAMIC_CREDIT_H
#define CPFF_DYNAMIC_CREDIT_H

  #include <stdio.h>
  #include <stdlib.h>
  #include <math.h>
  #include <string.h>

  #include "cpff_parameter.h"
  #include "cpff_structure.h"
  #include "cpff_statistic.h"
	#include "cpff_debug.h"
	#include "cpff_ipc.h"
	

	/*CREDIT INITIALIZATION*/
	int init_credit(userInfo *user, int totalWeight);
	/*CREDIT PRE CHARGING*/
	void credit_pre_charge(userInfo *user, REQ *r, char *creditType);
	/*Credit 補償*/ 
	void credit_compensate(userInfo *user, double serviceTime, REQ *r, char *creditType);
	/*Credit 重新補充*/ 
	int credit_replenish(userInfo *user, int totalWeight, double cpffSystemTime);
	/*ssd credit-based scheduler*/
	double ssd_credit_scheduler(systemInfo *sys, userInfo *user, double systemTime, int *ssdCandidate);
	/*hdd credit-based scheduler*/
	double hdd_credit_scheduler(systemInfo *sys, userInfo *user, double systemTime, int *hddCandidate);
  /*調整ssd credit分配*/
  void ssd_credit_adjust(userInfo *user);
  /*調整hdd credit分配*/
  void hdd_credit_adjust(userInfo *user);

	/*印出所有user的credit*/
	void print_credit(userInfo *user);


#endif