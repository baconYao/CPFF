#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "parameter.h"

/*計算進入User queue的request數量，取得Request總數(應同於Trace筆數)*/
static unsigned long totalRequests = 0;

/*定義request的格式*/
typedef struct req {
  double arrivalTime;		//抵達時間
  unsigned devno;			//裝置編號(預設為0)
  unsigned long diskBlkno;//Block編號(根據Disksim格式)
  unsigned reqSize;		//Block連續數量(至少為1)
  unsigned reqFlag;		//讀:1;寫:0
  unsigned userno;		//使用者編號(1~n)
  double responseTime;	//反應時間(初始為0)
} REQ;

/*STRUCTURE DEFINITION:USER QUEUE*/
typedef struct user_que_item {
struct user_que_item *back_req;		//指向上一個(較晚進入)
REQ r;								            //REQUEST STRUCTURE
struct user_que_item *front_req;	//指向下一個(較早進入)
} USER_QUE_ITEM;

/*定義user queue的格式*/
typedef struct user_que {
  int size;               //此Queue內request的數量
  USER_QUE_ITEM *head;		//指向佇列結構的頭
  USER_QUE_ITEM *tail;		//指向佇列結構的尾
} USER_QUE;


/*定義user的structure格式*/
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
  USER_QUE *ssdQueue;
  USER_QUE *hddQueue;
} userInfo;


/*建立user的queue list*/
USER_QUE *build_user_queue(int userNum, char *qType);

/*將request加入到user_queue*/
bool insert_req_to_user_que(userInfo *user, char *qType, REQ *r);

unsigned long get_total_reqs();

void copyReq(REQ *r, REQ *copy);