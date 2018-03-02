#ifndef CPFF_STATIC_CREDIT_H
#define CPFF_STATIC_CREDIT_H

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
	int credit_replenish(userInfo *user, int totalWeight);
	/*ssd credit-based scheduler*/
	double ssd_credit_scheduler(systemInfo *sys, userInfo *user, double systemTime, int *ssdCandidate);
	/*hdd credit-based scheduler*/
	double hdd_credit_scheduler(systemInfo *sys, userInfo *user, double systemTime, int *hddCandidate);

	/*印出所有user的credit*/
	void print_credit(userInfo *user);


#endif