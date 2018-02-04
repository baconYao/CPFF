#ifndef CPFF_STRUCTURE_H
#define CPFF_STRUCTURE_H

  #include <stdlib.h>
  #include <stdio.h>
  #include <string.h>
  #include <stdbool.h>

  #include "cpff_parameter.h"

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
    int hasSystemRequest;   // 表示此I/O request是否會產生額外的system request(經由Prize Caching algorithm判定)，之後I/O request從device queue送出到disksim時，才會根據狀態產生System request
  } REQ;

  /* [hasSystemRequest狀態表]
  * value = 0: 預設值，沒有System request
  * value = 1: Read Cache Miss，會產生一個SSD Write 的System request
  * value = 2: Read Cache Miss，且SSD Cache Space不夠(SSD is full)，會產生多個System requests，先SSD Read(sys)，再HDD Write(sys)，再SSD Write(user)
  * value = 3: Write Cache Miss，且SSD Cache Space不夠(SSD is full)，產生多個System Requests，先SSD Read(sys)，再HDD Write(sys)，再SSD Write(user)
  *
  */


  /*STRUCTURE DEFINITION:USER QUEUE*/
  typedef struct que_item {
  struct que_item *back_req;		//指向上一個(較晚進入)
  REQ r;								            //REQUEST STRUCTURE
  struct que_item *front_req;	//指向下一個(較早進入)
  } QUE_ITEM;


  /*定義queue的格式*/
  typedef struct que {
    int size;               //此Queue內request的數量
    QUE_ITEM *head;		//指向佇列結構的頭
    QUE_ITEM *tail;		//指向佇列結構的尾
  } QUE;

  /*定義user的structure格式*/
  typedef struct userInfo {
    unsigned int globalWeight;    // 全域的weight
    unsigned int credit;      // 此輪的credit
    unsigned long totalReq;				//Request數量
    unsigned long ssdReq;				//SSD Request數量
    unsigned long totalUserReq;			//User Request數量
    unsigned long UserReqInPeriod;		//User Request in one period數量
    unsigned long UserRReq;				//User Read Request數量
    unsigned long UserWReq;				//User Write Request數量
    unsigned long totalSysReq;			//System Request數量
    unsigned long evictCount;			//Eviction次數
    unsigned long dirtyCount;			//Dirty Page Eviction次數
    unsigned long hitCount;				//Hit次數
    unsigned long missCount;			//Miss次數
    double resTime;						//Response time for users
    double resTimeInPeriod;				//Response time for users in one period
    double cachingSpace;				//占用SSD的比例
    QUE *ssdQueue;
    QUE *hddQueue;
  } userInfo;

  
  /*建立host的queue list*/
  QUE *build_host_queue();

  /*建立user的queue list*/
  QUE *build_user_queue(int userNum, char *qType);

  /*建立device queue list*/
  QUE *build_device_queue(char *qType);

  /*將request加入到host queue*/
  bool insert_req_to_host_que_tail(QUE *hostQ, REQ *r);

  /*從queue的head端將request移出*/
  REQ *remove_req_from_queue_head(QUE *Que);

  /*將request加入到user_queue*/
  bool insert_req_to_user_que_tail(userInfo *user, char *qType, REQ *r);

  /*取得所有requests(sub-requests)的總數*/ 
  unsigned long get_total_reqs();

  /*將r的內容複製到copy*/ 
  void copyReq(REQ *r, REQ *copy);

  bool is_empty_queue(QUE *Que);

  /*印出queue的內容*/ 
  void print_queue_content(QUE *Que);


  

#endif