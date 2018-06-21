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
    int isSystemRequest;   // 判斷此request是system request (1) 或user request (0), default: 0
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
    double ssdCredit;   //當前的ssd credit
    double adjustSsdCredit;   //調整過後的ssd credit，會在一定時間內持續使用 (此值適用在dynamic credit adjustment policy)
    double hddCredit;   //當前的hdd credit
    double adjustHddCredit;   //調整過後的hdd credit，會在一定時間內持續使用 (此值適用在dynamic credit adjustment policy)
    double prevSsdCredit;
    double prevHddCredit;
    unsigned long totalReq;		//Request數量 (include user and system request)
    unsigned long totalSsdReq;		//SSD Request數量 (include user and system ssd request)
    unsigned long totalHddReq;		//HDD Request數量 (include user and system hdd request)
    unsigned long totalUserReq;			//User Request數量
    unsigned long userReadReq;				//User Read Request數量
    unsigned long userReadReqInSecond;		//User Read Request in one second數量 
    unsigned long userReadReqInPeriod;		//User Read Request in one period數量 
    unsigned long userWriteReq;				//User Write Request數量
    unsigned long userWriteReqInSecond;		//User Write Request in one Second數量 
    unsigned long userWriteReqInPeriod;		//User Write Request in one period數量 
    unsigned long totalSysReq;	  //System Request數量
    unsigned long sysSsdReadReq;			//System SSD Read Request數量
    unsigned long sysSsdReadReqInSecond;		//SSD System Read Request in one Second數量 
    unsigned long sysSsdReadReqInPeriod;		//SSD System Read Request in one period數量 
    unsigned long sysSsdWriteReq;			//System SSD Write Request數量
    unsigned long sysSsdWriteReqInSecond;		//SSD System Write Request in one Second數量 
    unsigned long sysSsdWriteReqInPeriod;		//SSD System Write Request in one period數量 
    unsigned long sysHddWriteReq;			//System HDD Write Request數量
    unsigned long sysHddWriteReqInSecond;		//SSD System Write Request in one Second數量 
    unsigned long sysHddWriteReqInPeriod;		//SSD System Write Request in one period數量 
    unsigned long evictCount;			//Eviction次數
    unsigned long evictCountInSecond;			//每秒 Eviction次數
    unsigned long evictCountInPeriod;			//Period Eviction次數
    unsigned long dirtyCount;			//Dirty Page Eviction次數
    unsigned long dirtyCountInSecond;			//每秒 Dirty Page Eviction次數
    unsigned long dirtyCountInPeriod;			//Period Dirty Page Eviction次數
    unsigned long hitCount;				//Hit次數
    unsigned long hitCountInSecond;				//每秒 Hit次數
    unsigned long hitCountInPeriod;				//Period Hit次數
    unsigned long missCount;			//Miss次數
    unsigned long missCountInSecond;			//每秒 Miss次數
    unsigned long missCountInPeriod;			//Period Miss次數
    unsigned long doneSsdSysReq;			//紀錄已經送進SSD simulator做完的system request
    unsigned long doneSsdSysReqInSecond;			//紀錄每秒已經送進SSD simulator做完的system request 
    unsigned long doneSsdSysReqInPeriod;			//紀錄每period已經送進SSD simulator做完的system request 
    unsigned long doneHddSysReq;			//紀錄已經送進HDD simulator做完的system request
    unsigned long doneHddSysReqInSecond;			//紀錄每秒已經送進HDD simulator做完的system request  
    unsigned long doneHddSysReqInPeriod;			//紀錄每period已經送進HDD simulator做完的system request 
    unsigned long doneHddSysReqInAdjustCachePeriod;			//紀錄每adjust cache period已經送進HDD simulator做完的system request 
    unsigned long doneSsdUserReq;			//紀錄已經送進SSD simulator做完的User request
    unsigned long doneSsdUserReqInSecond;			//紀錄每秒已經送進SSD simulator做完的User request 
    unsigned long doneSsdUserReqInPeriod;			//紀錄每period已經送進SSD simulator做完的User request 
    unsigned long doneSsdUserReqInAdjustCachePeriod;			//紀錄每adjust cache period已經送進SSD simulator做完的User request 
    unsigned long doneHddUserReq;			//紀錄已經送進HDD simulator做完的User request
    unsigned long doneHddUserReqInSecond;			//紀錄每秒已經送進HDD simulator做完的User request 
    unsigned long doneHddUserReqInPeriod;			//紀錄每period已經送進HDD simulator做完的User request 
    unsigned long doneHddUserReqInAdjustCachePeriod;			//紀錄每adjust cache period已經送進HDD simulator做完的User request 
    double userSsdReqResTime;					    	//All user ssd requests'  response time for system
    double userSsdReqResTimeInSecond;				//All user ssd requests' response time for system in one second
    double userSsdReqResTimeInPeriod;				//All user ssd requests' response time for system in one period
    double userHddReqResTime;					    	//All user hdd requests'  response time for system
    double userHddReqResTimeInSecond;				//All user hdd requests' response time for system in one second
    double userHddReqResTimeInPeriod;				//All user hdd requests' response time for system in one period
    double userHddReqResTimeInAdjustCachePeriod;				//All user hdd requests' response time for system in one adjust cache period
    double sysSsdReqResTime;					    	//All system ssd requests' response time for system
    double sysSsdReqResTimeInSecond;				//All system ssd requests' response time for system in one second
    double sysSsdReqResTimeInPeriod;				//All system ssd requests' response time for system in one period
    double sysHddReqResTime;					    	//All system hdd requests' response time for system
    double sysHddReqResTimeInSecond;				//All system hdd requests' response time for system in one second
    double sysHddReqResTimeInPeriod;				//All system hdd requests' response time for system in one period
    double sysHddReqResTimeInAdjustCachePeriod;				//All system hdd requests' response time for system in one period
    unsigned long pageNumberInCache;    //此user的cache大小 (有多少個page)
    int comingRequestCounter;         //記錄每一period真實進入的request數量
    QUE *hostQueue;         //User host queue
    QUE *ssdQueue;          //User ssd queue
    QUE *hddQueue;          //User hdd queue
  } userInfo;

  /*整個系統紀錄的資訊, 無視個別user*/
  typedef struct systemInfo {
    unsigned long totalReq;		//Request數量 (include user and system request)
    unsigned long totalSsdReq;		//SSD Request數量 (include user and system ssd request)
    unsigned long totalHddReq;		//HDD Request數量 (include user and system hdd request)
    unsigned long totalUserReq;			//User Request數量
    unsigned long userReadReq;				//User Read Request數量
    unsigned long userReadReqInSecond;		//User Read Request in one second數量 
    unsigned long userReadReqInPeriod;		//User Read Request in one period數量 
    unsigned long userWriteReq;				//User Write Request數量
    unsigned long userWriteReqInSecond;		//User Write Request in one Second數量 
    unsigned long userWriteReqInPeriod;		//User Write Request in one period數量 
    unsigned long totalSysReq;	  //System Request數量
    unsigned long sysSsdReadReq;			//System SSD Read Request數量
    unsigned long sysSsdReadReqInSecond;		//SSD System Read Request in one Second數量 
    unsigned long sysSsdReadReqInPeriod;		//SSD System Read Request in one period數量 
    unsigned long sysSsdWriteReq;			//System SSD Write Request數量
    unsigned long sysSsdWriteReqInSecond;		//SSD System Write Request in one Second數量 
    unsigned long sysSsdWriteReqInPeriod;		//SSD System Write Request in one period數量 
    unsigned long sysHddWriteReq;			//System HDD Write Request數量
    unsigned long sysHddWriteReqInSecond;		//SSD System Write Request in one Second數量 
    unsigned long sysHddWriteReqInPeriod;		//SSD System Write Request in one period數量 
    unsigned long evictCount;			//Eviction次數
    unsigned long evictCountInSecond;			//每秒 Eviction次數
    unsigned long evictCountInPeriod;			//Period Eviction次數
    unsigned long dirtyCount;			//Dirty Page Eviction次數
    unsigned long dirtyCountInSecond;			//每秒 Dirty Page Eviction次數
    unsigned long dirtyCountInPeriod;			//Period Dirty Page Eviction次數
    unsigned long hitCount;				//Hit次數
    unsigned long hitCountInSecond;				//每秒 Hit次數
    unsigned long hitCountInPeriod;				//Period Hit次數
    unsigned long missCount;			//Miss次數
    unsigned long missCountInSecond;			//每秒 Miss次數
    unsigned long missCountInPeriod;			//Period Miss次數
    unsigned long doneSsdSysReq;			//紀錄已經送進SSD simulator做完的system request
    unsigned long doneSsdSysReqInSecond;			//紀錄每秒已經送進SSD simulator做完的system request 
    unsigned long doneSsdSysReqInPeriod;			//紀錄每period已經送進SSD simulator做完的system request 
    unsigned long doneHddSysReq;			//紀錄已經送進HDD simulator做完的system request
    unsigned long doneHddSysReqInSecond;			//紀錄每秒已經送進HDD simulator做完的system request  
    unsigned long doneHddSysReqInPeriod;			//紀錄每period已經送進HDD simulator做完的system request 
    unsigned long doneHddSysReqInAdjustCachePeriod;			//紀錄每adjust cache period已經送進HDD simulator做完的system request 
    unsigned long doneSsdUserReq;			//紀錄已經送進SSD simulator做完的User request
    unsigned long doneSsdUserReqInSecond;			//紀錄每秒已經送進SSD simulator做完的User request 
    unsigned long doneSsdUserReqInPeriod;			//紀錄每period已經送進SSD simulator做完的User request 
    unsigned long doneHddUserReq;			//紀錄已經送進HDD simulator做完的User request
    unsigned long doneHddUserReqInSecond;			//紀錄每秒已經送進HDD simulator做完的User request 
    unsigned long doneHddUserReqInPeriod;			//紀錄每period已經送進HDD simulator做完的User request 
    unsigned long doneHddUserReqInAdjustCachePeriod;			//紀錄每adjust cache period已經送進HDD simulator做完的User request 
    double userSsdReqResTime;					    	//All user ssd requests'  response time for system
    double userSsdReqResTimeInSecond;				//All user ssd requests' response time for system in one second
    double userSsdReqResTimeInPeriod;				//All user ssd requests' response time for system in one period
    double userHddReqResTime;					    	//All user hdd requests'  response time for system
    double userHddReqResTimeInSecond;				//All user hdd requests' response time for system in one second
    double userHddReqResTimeInPeriod;				//All user hdd requests' response time for system in one period
    double userHddReqResTimeInAdjustCachePeriod;				//All user hdd requests' response time for system in one adjust cache period
    double sysSsdReqResTime;					    	//All system ssd requests' response time for system
    double sysSsdReqResTimeInSecond;				//All system ssd requests' response time for system in one second
    double sysSsdReqResTimeInPeriod;				//All system ssd requests' response time for system in one period
    double sysHddReqResTime;					    	//All system hdd requests' response time for system
    double sysHddReqResTimeInSecond;				//All system hdd requests' response time for system in one second
    double sysHddReqResTimeInPeriod;				//All system hdd requests' response time for system in one period
    double sysHddReqResTimeInAdjustCachePeriod;				//All system hdd requests' response time for system in one period
  } systemInfo;
  
  /*建立host的queue list*/
  QUE *build_host_queue();
  /*建立user的queue list*/
  QUE *build_user_queue(int userNum, char *qType);
  /*建立device queue list*/
  QUE *build_device_queue(char *qType);
  /*將request加入到host queue tail*/
  bool insert_req_to_host_que_tail(userInfo *user, REQ *r, systemInfo *sysInfo);
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
  /*檢查queue是否還有request*/ 
  bool is_empty_queue(QUE *Que);
  /*檢查所有user host queue是否還有request*/ 
  bool are_all_user_host_queue_empty(userInfo *user);
  /*檢查所有user ssd queue是否還有request*/ 
  bool are_all_user_ssd_queue_empty(userInfo *user);
  /*檢查所有user hdd queue是否還有request*/ 
  bool are_all_user_hdd_queue_empty(userInfo *user);
  /*印出queue的內容*/ 
  void print_queue_content(QUE *Que, char *queueName);

#endif