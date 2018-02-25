#ifndef CPFF_STRUCTURE_H
#define CPFF_STRUCTURE_H

  #include <stdlib.h>
  #include <stdio.h>
  #include <string.h>
  #include <stdbool.h>

  #include "cpff_parameter.h"
  #include "disksim_interface.h" //for flag(DISKSIM_READ) used
  
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
    int isSystemRequest;   // 判斷此request是user request (1) 或system request (0), default: 0
    double preChargeValue;   // 此request的pre charge value(初始為0.0)
  } REQ;


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
    unsigned long totalReq;				//Request數量 (include user and system request)
    unsigned long totalSsdReq;		//SSD Request數量 (include user and system ssd request)
    unsigned long totalHddReq;		//HDD Request數量 (include user and system hdd request)
    unsigned long totalUserReq;			//User Request數量
    unsigned long userReadReq;				//User Read Request數量
    unsigned long userWriteReq;				//User Write Request數量
    unsigned long userReadReqInPeriod;		//User Read Request in one period數量 (Value reset to 0 when credit replenish)
    unsigned long sysSsdReadReqInPeriod;		//System Read Request in one period數量 (Value reset to 0 when credit replenish)
    unsigned long userWriteReqInPeriod;		//User Write Request in one period數量 (Value reset to 0 when credit replenish)
    unsigned long sysSsdWriteReqInPeriod;		//System Write Request in one period數量 (Value reset to 0 when credit replenish)
    unsigned long sysHddWriteReqInPeriod;		//System Write Request in one period數量 (Value reset to 0 when credit replenish)
    unsigned long totalSysReq;			  //System Request數量
    unsigned long sysSsdReadReq;			//System SSD Read Request數量
    unsigned long sysSsdWriteReq;			//System SSD Write Request數量
    unsigned long sysHddWriteReq;			//System HDD Write Request數量
    unsigned long evictCount;			//Eviction次數
    unsigned long dirtyCount;			//Dirty Page Eviction次數
    unsigned long hitCount;				//Hit次數
    unsigned long missCount;			//Miss次數
    double resTime;					    	//All Response time for users
    double resTimeInPeriod;				//Response time for users in one period, Value reset to 0 when credit replenish
    double cachingSpace;				//占用SSD的比例 (Maybe changed each period)
    QUE *ssdQueue;
    QUE *hddQueue;
  } userInfo;

  
  /*建立host的queue list*/
  QUE *build_host_queue();

  /*建立user的queue list*/
  QUE *build_user_queue(int userNum, char *qType);

  /*建立device queue list*/
  QUE *build_device_queue(char *qType);

  /*將request加入到host queue tail*/
  bool insert_req_to_host_que_tail(QUE *hostQ, REQ *r);

  /*將request加入到user queue tail*/
  bool insert_req_to_user_que_tail(userInfo *user, char *qType, REQ *r);

  /*將request加入到device queue tail*/
  bool insert_req_to_device_que_tail(QUE *deviceQ, REQ *r);

  /*從queue的head端將request移出*/
  void remove_req_from_queue_head(QUE *Que);

  /*取得所有requests(sub-requests)的總數*/ 
  unsigned long get_total_reqs();

  /*將r的內容複製到copy*/ 
  void copy_req(REQ *r, REQ *copy);

  bool is_empty_queue(QUE *Que);

  bool are_all_user_ssd_queue_empty(userInfo *user);
  
  bool are_all_user_hdd_queue_empty(userInfo *user);

  /*印出queue的內容*/ 
  void print_queue_content(QUE *Que, char *queueName);


  

#endif