#ifndef CPFF_STATIC_CREDIT_H
#define CPFF_STATIC_CREDIT_H

  #include <stdio.h>
  #include <stdlib.h>
  #include <math.h>
  #include <string.h>

  #include "cpff_parameter.h"
  #include "cpff_structure.h"
  #include "cpff_debug.h"

	/*USER CREDIT*/
	static double userSSDCredit[NUM_OF_USER];
	static double userHDDCredit[NUM_OF_USER];

	/*CREDIT INITIALIZATION*/
	int init_credit(userInfo *user, int totalWeight);
	/*CREDIT PRE CHARGING*/
	void credit_pre_charge(unsigned userno, REQ *r, char *creditType);
	/*Credit 補償*/ 
	void credit_compensate(unsigned userno, double serviceTime, REQ *r, char *creditType);
	/*Credit 重新補充*/ 
	int credit_replenish(userInfo *user, int totalWeight);
	/*ssd credit-based scheduler*/
	void ssd_credit_scheduler(userInfo *user, QUE *ssdDeviceQueue);
	/*hdd credit-based scheduler*/
	void hdd_credit_scheduler(userInfo *user, QUE *hddDeviceQueue);

	/*印出所有user的credit*/
	void print_credit();


#endif