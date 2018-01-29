#ifndef DEBUG_H
#define DEBUG_H

#include <stdlib.h>
#include <stdio.h>
#include "./../../configs/structure.h"


	/*印出字串*/
  void print_something(char* str);
  
	/*印出字串與錯誤碼並結束程式*/
  void print_error(int rc, char* str);
  
	/*印出字串與錯誤碼*/
  void print_debug(int rc, char* str);
  
	/*印出Request資訊*/
  void print_REQ(REQ *r, char* str);
  
	/*印出進度*/
	void print_progress(unsigned long currentREQ, unsigned long totalREQ, unsigned long currentMeta, unsigned long currentCache);
#endif
