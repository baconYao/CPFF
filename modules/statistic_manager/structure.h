/*
*  
*
*/ 

#include "../../configs/parameter.h"

typedef struct userInfo {
  unsigned int globalWeight;    // 全域的weight
  unsigned int credit;      // 此輪的credit
  unsigned long totalReq;				//Request數量
  unsigned long ssdReq;				//SSD Request數量
  unsigned long totalUserReq;			//User Request數量
  unsigned long UserReqInPeriod;		//User Request in one period數量
  unsigned long UserRReq;				//User Read Request數量
  unsigned long totalSysReq;			//System Request數量
  unsigned long evictCount;			//Eviction次數
  unsigned long dirtyCount;			//Dirty Page Eviction次數
  unsigned long hitCount;				//Hit次數
  unsigned long missCount;			//Miss次數
  double resTime;						//Response time for users
  double resTimeInPeriod;				//Response time for users in one period
  double cachingSpace;				//占用SSD的比例
} userInfo;

userInfo userInfoArr[NUM_OF_USER];
userInfo sysst;

// 初始化 userInfo & sysst
void init_user_info();