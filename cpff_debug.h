#ifndef CPFF_DEBUG_H
#define CPFF_DEBUG_H

#include <stdlib.h>
#include <stdio.h>
#include "cpff_structure.h"


	/*印出字串*/
  void print_something(char* str);
  
	/*印出字串與錯誤碼並結束程式*/
  void print_error(int rc, char* str);
  
	/*印出字串與錯誤碼*/
  void print_debug(int rc, char* str);
  
	/*印出Request資訊*/
  void print_REQ(REQ *r, char* str);
  
	/*印出進度*/
	void print_progress(double cpffSystemTime, unsigned long totalREQ, unsigned long doneREQ);
#endif
