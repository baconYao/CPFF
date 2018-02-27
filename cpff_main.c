#include "cpff_main.h"

QUE *hostQueue;                   //claim host queue
QUE *ssdDeviceQueue;              //claim SSD device queue
QUE *hddDeviceQueue;              //claim HDD device queue
systemInfo sysInfo;               //Record system information
userInfo user[NUM_OF_USER];       //建立user
pid_t SSDsimProc, HDDsimProc;     //Sub-process id: SSD and HDD simulator

FILE *trace;          //讀取的trace
FILE *systemResultFile;     //The final record
// FILE *eachUserPeriodRecord[NUM_OF_USER];   //Every users' records in each period
char *par[6];         //CPFF system arguments
int totalWeight = 0;  //所有user的global weight累加
double cpffSystemTime = 0.0;      //cpff的系統時間
double ssdReqCompleteTime = 0.0;      //表示被送進SSD sim 的SSD request在系統時間(cpffSystemTime)的甚麼時候做完。 (ssdReqCompleteTime = request servie time + cpffSystemTime)
double hddReqCompleteTime = 0.0;      //表示被送進HDD sim 的HDD request在系統時間(cpffSystemTime)的甚麼時候做完。 (hddReqCompleteTime = request servie time + cpffSystemTime)
double nextReplenishCreditTime = TIME_PERIOD;    //credit重新補充的時間，每次加1000.0(ms)，預設為1000.0ms = 1 second
bool doSsdRequest = false;      //表示是否可將SSD device queue的request送到SSD sim執行 (預設為false)
bool doHddRequest = false;      //表示是否可將HDD device queue的request送到HDD sim執行 (預設為false)

/**
 * [Disksim的初始化，利用兩個Process各自執行Disksim，作為SSDsim和HDDsim，
 * 接續MESSAGE QUEUE INITIALIZATION]
 */
void init_disksim() {
  pid_t procid;
  //Fork process to execute SSD simulator
  procid = fork();
  if (procid == 0) {
    SSDsimProc = getpid();
    //printf("SSDsimProc ID: %d\n", SSDsimProc);
    exec_SSDsim("SSDsim", par[1], par[2]);
    exit(0);
  }
  else if (procid < 0) {
    print_error(-1, "SSDsim process fork() error");
    exit(1);
  }

  //Fork process to execute HDD simulator
  procid = fork();
  if (procid == 0) {
    HDDsimProc = getpid();
    //printf("HDDsimProc ID: %d\n", HDDsimProc);
    exec_HDDsim("HDDsim", par[3], par[4]);
    exit(0);
  }
  else if (procid < 0) {
    print_error(-1, "HDDsim process fork() error");
    exit(1);
  }

  //After the initialization of simulators, initialize message queue
  init_MSQ();
}

/**
 * [Disksim的關閉，傳送Control message告知其Process進行Shutdown，並等待回傳結果message]
 */
void rm_disksim() {
  REQ *ctrl, *ctrl_rtn;
  ctrl = calloc(1, sizeof(REQ));
  ctrl_rtn = calloc(1, sizeof(REQ));      //Receive message after control message
  ctrl->reqFlag = MSG_REQUEST_CONTROL_FLAG_FINISH; //Assign a finish flag (ipc)
  
  //Send a control message to finish SSD simulator
  send_finish_control(KEY_MSQ_DISKSIM_1, MSG_TYPE_DISKSIM_1);

  //Receive the last message from SSD simulator
  if(recv_request_by_MSQ(KEY_MSQ_DISKSIM_1, ctrl_rtn, MSG_TYPE_DISKSIM_1_SERVED) == -1) {
    print_error(-1, "A served request not received from MSQ in recvRequestByMSQ():");
  }
  // printf(COLOR_YB"[YUSIM]SSDsim response time = %lf\n"COLOR_N, ctrl_rtn->responseTime);
  // fprintf(result, "[YUSIM]SSDsim response time = %lf\n", ctrl_rtn->responseTime);

  //Send a control message to finish HDD simulator
  send_finish_control(KEY_MSQ_DISKSIM_2, MSG_TYPE_DISKSIM_2);

  //Receive the last message from HDD simulator
  if(recv_request_by_MSQ(KEY_MSQ_DISKSIM_2, ctrl_rtn, MSG_TYPE_DISKSIM_2_SERVED) == -1) {
    print_error(-1, "A served request not received from MSQ in recvRequestByMSQ():");
  }
  // printf(COLOR_YB"[YUSIM]HDDsim response time = %lf\n"COLOR_N, ctrl_rtn->responseTime);
  // fprintf(result, "[YUSIM]HDDsim response time = %lf\n", ctrl_rtn->responseTime);

  //After that, remove message queues
  rm_MSQ();
}

/**
 * [系統初始化]
 */
void initialize(char *par[]) {

  #ifdef STATIC_CACHING_SPACE
    printf(COLOR_RB"Caching Space Policy: STATIC_CACHING_SPACE\n"COLOR_RESET);
  #elif defined DYNAMIC_CACHING_SPACE
    printf(COLOR_RB"Caching Space Policy: DYNAMIC_CACHING_SPACE\n"COLOR_RESET);
  #elif defined COMPETITION_CACHING_SPACE
    printf(COLOR_RB"Caching Space Policy: COMPETITION_CACHING_SPACE\n"COLOR_RESET);
  #endif
  
  #ifdef STATIC_CREDIT
    printf(COLOR_RB"Credit Policy: STATIC_CREDIT\n"COLOR_RESET);
  #elif defined DYNAMIC_CREDIT
    printf(COLOR_RB"Credit Policy: DYNAMIC_CREDIT\n"COLOR_RESET);
  #endif

  /*初始化 HDDsim & SSDsim*/
  init_disksim();
  sleep(3);

  //Open trace file
  trace = fopen(par[0], "r");
  if (!trace) {
    print_error(-1, "Trace file open failed ");
  }

  char *dir="./cpff_statistics_dir/System_Record.json";
  if((systemResultFile = fopen(dir, "w")) == NULL) {
    print_error(-1, "Can't open the cpff_statistics_dir/System_Record.txt file");
  }
  

  /*建立host queue*/
  hostQueue = build_host_queue();
  // printf("Host queue's memory address: %p\n", &hostQueue);
  if(hostQueue == NULL) {
    print_error(-1, "Can't build Host Queue");
  }

  /*初始化system資訊*/
  sysInfo.totalReq = 0;			
  sysInfo.totalSsdReq = 0;			
  sysInfo.totalHddReq = 0;			
  sysInfo.totalUserReq = 0;			
  sysInfo.userReadReq = 0;				
  sysInfo.userWriteReq = 0;	
  sysInfo.userReadReqInPeriod = 0;
  sysInfo.sysSsdReadReqInPeriod = 0;
  sysInfo.userWriteReqInPeriod = 0;
  sysInfo.sysSsdWriteReqInPeriod = 0;
  sysInfo.sysHddWriteReqInPeriod = 0;			
  sysInfo.totalSysReq = 0;				
  sysInfo.sysSsdReadReq = 0;				
  sysInfo.sysSsdWriteReq = 0;				
  sysInfo.sysHddWriteReq = 0;				
  sysInfo.evictCount = 0;				
  sysInfo.dirtyCount = 0;				
  sysInfo.hitCount = 0;				
  sysInfo.missCount = 0;				
  sysInfo.doneSsdSysReq = 0;			
  sysInfo.doneHddSysReq = 0;			
  sysInfo.doneSsdSysReqInPeriod = 0;			
  sysInfo.doneHddSysReqInPeriod = 0;			
  sysInfo.doneSsdUserReq = 0;			
  sysInfo.doneHddUserReq = 0;			
  sysInfo.doneSsdUserReqInPeriod = 0;			
  sysInfo.doneHddUserReqInPeriod = 0;			
  sysInfo.userSsdReqResTime = 0.0;				    	
  sysInfo.userHddReqResTime = 0.0;				    	
  sysInfo.userSsdReqResTimeInPeriod = 0.0;					    	
  sysInfo.userHddReqResTimeInPeriod = 0.0;					    	
  sysInfo.sysSsdReqResTime = 0.0;					    	
  sysInfo.sysHddReqResTime = 0.0;					    	
  sysInfo.sysSsdReqResTimeInPeriod = 0.0;				
  sysInfo.sysHddReqResTimeInPeriod = 0.0;				

  /*初始化user資訊*/
  int i = 0;
  unsigned weight = 0;
  for(i = 0; i < NUM_OF_USER; i++) {
    fscanf(trace, "%u", &weight);       //讀取user的global weight
    user[i].globalWeight = weight;
    user[i].ssdQueue = build_user_queue(i+1, "SSD");    //建立user ssd queue
    user[i].hddQueue = build_user_queue(i+1, "HDD");    //建立user hdd queue
    user[i].totalReq = 0;
    user[i].totalSsdReq = 0;
    user[i].totalHddReq = 0;
    user[i].totalUserReq = 0;
    user[i].userReadReqInPeriod = 0;
    user[i].sysSsdReadReqInPeriod = 0;
    user[i].userWriteReqInPeriod = 0;
    user[i].sysSsdWriteReqInPeriod = 0;
    user[i].sysHddWriteReqInPeriod = 0;
    user[i].userReadReq = 0;
    user[i].userWriteReq = 0;
    user[i].totalSysReq = 0;
    user[i].sysSsdReadReq = 0;
    user[i].sysSsdWriteReq = 0;
    user[i].sysHddWriteReq = 0;
    user[i].doneSsdSysReq = 0;
    user[i].doneHddSysReq = 0;
    user[i].doneSsdSysReqInPeriod = 0;
    user[i].doneHddSysReqInPeriod = 0;
    user[i].doneSsdUserReq = 0;
    user[i].doneHddUserReq = 0;
    user[i].doneSsdUserReqInPeriod = 0;
    user[i].doneHddUserReqInPeriod = 0;
    user[i].evictCount = 0;
    user[i].dirtyCount = 0;
    user[i].hitCount = 0;
    user[i].missCount = 0;
    user[i].userSsdReqResTime = 0.0;
    user[i].userHddReqResTime = 0.0;
    user[i].userSsdReqResTimeInPeriod = 0.0;
    user[i].userHddReqResTimeInPeriod = 0.0;
    user[i].sysSsdReqResTime = 0.0;
    user[i].sysHddReqResTime = 0.0;
    user[i].sysSsdReqResTimeInPeriod = 0.0;
    user[i].sysHddReqResTimeInPeriod = 0.0;
    user[i].cachingSpace = 0;

    totalWeight += weight;              //累加所有user的globalWeight
  }

  /*建立device queue*/
  ssdDeviceQueue = build_device_queue("SSD");
  hddDeviceQueue = build_device_queue("HDD");

  /*初始化 PC metablock table*/
  if(init_meta_table() != 0) {
    print_error(-1, "Can't build user cache!");
  }

  /*初始化 user cache space*/
  if(init_user_cache(user, totalWeight) != 0) {
    print_error(-1, "Can't build user cache!");
  }
  
  /*初始化 user credit*/  
  if(init_credit(user, totalWeight) != 0) {
    print_error(-1, "Can't initialize user credit!");
  }

  
  /*讀取trace file requests*/ 
  REQ *tmp;
  tmp = calloc(1, sizeof(REQ));
  printf("\n-------------Reading trace requests--------------\n");
  i = 0;
  while(!feof(trace)) {
    fscanf(trace, "%lf%u%lu%u%u%u", &tmp->arrivalTime, &tmp->devno, &tmp->diskBlkno, &tmp->reqSize, &tmp->reqFlag, &tmp->userno);
    tmp->isSystemRequest = 0;    //default value: 0
    tmp->preChargeValue = 0.0;    //default value: 0.0

    /*將request放進host queue*/ 
    if(!insert_req_to_host_que_tail(hostQueue, tmp, &sysInfo)) {
      print_error(-1, "[Error] request to host queue!");
    }
  }
  /*釋放trace File descriptor*/ 
  fclose(trace);
  free(tmp);

  printf("Total user requests: %lu\tUser read requests: %lu\tUser write requests: %lu\n", sysInfo.totalUserReq, sysInfo.userReadReq, sysInfo.userWriteReq);
};

/**
 * [cpff主程式]
 */
void execute_CPFF_framework() {
  printf("Press enter to continue program.\n");
  char c = getchar();
  
  int index = 1;        //the final value is same as the sysInfo.totalReq
  while(1) {
    double ssdServiceTime, hddServiceTime;
    
    // printf(COLOR_BB"\nIndex: %d\tCPFF System Time: %f\n"COLOR_RESET, index++, cpffSystemTime);
    // print_credit();
    
    /*執行prize caching，根據系統時間(cpffSystemTime)將host queue內的request送至對應的user queue內*/ 
    prize_caching(cpffSystemTime, user, hostQueue, &sysInfo);

    /*檢查ssd device queue內是否有request，若沒有，則啟動ssd credit base scheduler從user ssd queue內抓取request到ssd device queue*/
    if(is_empty_queue(ssdDeviceQueue)) {
      //do ssd credit base scheduler, dispatch request to ssd device queue
      ssd_credit_scheduler(user, ssdDeviceQueue);
      // print_queue_content(ssdDeviceQueue, "ssdDeviceQueue");
    }

    /*檢查hdd device queue內是否有request，若沒有，則啟動hdd credit base scheduler從user hdd queue內抓取request到hdd device queue*/
    if(is_empty_queue(hddDeviceQueue)) {
      //do hdd credit base scheduler, dispatch request to hdd device queue
      hdd_credit_scheduler(user, hddDeviceQueue);
      // print_queue_content(hddDeviceQueue, "hddDeviceQueue");
    }
    
    // printf("===============================================\n");

    /* we can send ssd request to SSD sim */
    if(cpffSystemTime == ssdReqCompleteTime) {
      if(!is_empty_queue(ssdDeviceQueue)) {
        REQ *ssdTmp;  
        ssdTmp = &(ssdDeviceQueue->head->r);    //取得SSD device queue內的head pointer指向的request
        ssdServiceTime = get_service_time(KEY_MSQ_DISKSIM_1, MSG_TYPE_DISKSIM_1, ssdTmp);   //送進SSD sim，獲得此request的service time
        ssdTmp->responseTime = ssdServiceTime;
        credit_compensate(ssdTmp->userno-1, ssdServiceTime, ssdTmp, "SSDCredit");   //進行credit的補償
        ssdReqCompleteTime = ssdServiceTime + cpffSystemTime;   //推進下個ssd device queue 內 request可以做的時間
        
        //statistics
        statistics_done_func(ssdTmp, "SSD");

        remove_req_from_queue_head(ssdDeviceQueue);
      }
    }

    /* we can send hdd request to HDD sim */
    if(cpffSystemTime == hddReqCompleteTime) {
      if(!is_empty_queue(hddDeviceQueue)) {
        REQ *hddTmp;
        hddTmp = &(hddDeviceQueue->head->r);    //取得HDD device queue內的head pointer指向的request
        hddServiceTime = get_service_time(KEY_MSQ_DISKSIM_2, MSG_TYPE_DISKSIM_2, hddTmp);   //送進HDD sim，獲得此request的service time
        hddTmp->responseTime = hddServiceTime;
        credit_compensate(hddTmp->userno-1, hddServiceTime, hddTmp, "HDDCredit");     //進行credit的補償
        hddReqCompleteTime = hddServiceTime + cpffSystemTime;   //推進下個hdd device queue 內 request可以做的時間
        
        //statistics
        statistics_done_func(hddTmp, "HDD");
        
        remove_req_from_queue_head(hddDeviceQueue);
      }
    }

    // print_cache_by_LRU_and_users();

    /*推進系統時間*/
    cpffSystemTime = shift_cpffSystemTime(ssdReqCompleteTime, hddReqCompleteTime);
    
    /*每TIME_PERIOD，重新補充credit*/
    if(cpffSystemTime == nextReplenishCreditTime) {
      printf("\nTotal request: %lu\n", sysInfo.totalReq);
      printf("User 1 Total request: %lu\n", user[0].totalReq);
      printf("User 2 Total request: %lu\n", user[1].totalReq);
      printf("\nTotal user requests: %lu\t(Read: %lu\tWrite: %lu)\n", sysInfo.totalUserReq, sysInfo.userReadReq, sysInfo.userWriteReq);
      printf("Total user 1 requests: %lu\t(Read: %lu\tWrite: %lu)\n", user[0].totalUserReq, user[0].userReadReq, user[0].userWriteReq);
      printf("Total user 2 requests: %lu\t(Read: %lu\tWrite: %lu)\n", user[1].totalUserReq, user[1].userReadReq, user[1].userWriteReq);
      printf("\nTotal system request: %lu\n", sysInfo.totalSysReq);
      printf("User 1 Total system request: %lu\n", user[0].totalSysReq);
      printf("User 2 Total system request: %lu\n", user[1].totalSysReq);
      printf("\nTotal SSD request: %lu\n", sysInfo.totalSsdReq);
      printf("User 1 Total SSD request: %lu\n", user[0].totalSsdReq);
      printf("User 2 Total SSD request: %lu\n", user[1].totalSsdReq);
      printf("\nTotal HDD request: %lu\n", sysInfo.totalHddReq);
      printf("User 1 Total HDD request: %lu\n", user[0].totalHddReq);
      printf("User 2 Total HDD request: %lu\n", user[1].totalHddReq);
      printf("\nTotal SSD system read request: %lu\n", sysInfo.sysSsdReadReq);
      printf("1 Total SSD system read request: %lu\n", user[0].sysSsdReadReq);
      printf("2 Total SSD system read request: %lu\n", user[1].sysSsdReadReq);
      printf("\nTotal SSD system write request: %lu\n", sysInfo.sysSsdWriteReq);
      printf("1 Total SSD system write request: %lu\n", user[0].sysSsdWriteReq);
      printf("2 Total SSD system write request: %lu\n", user[1].sysSsdWriteReq);
      printf("\nTotal HDD system write request: %lu\n", sysInfo.sysHddWriteReq);
      printf("1 Total HDD system write request: %lu\n", user[0].sysHddWriteReq);
      printf("2 Total HDD system write request: %lu\n", user[1].sysHddWriteReq);
      // printf("doneSsdSysReq: %lu\n", sysInfo.doneSsdSysReq);
      // printf("doneSsdSysReqInPeriod: %lu\n", sysInfo.doneSsdSysReqInPeriod);
      // printf("doneHddSysReq: %lu\n", sysInfo.doneHddSysReq);
      // printf("doneHddSysReqInPeriod: %lu\n\n", sysInfo.doneHddSysReqInPeriod);
      // printf("doneSsdUserReq: %lu\n", sysInfo.doneSsdUserReq);
      // printf("doneHddUserReq: %lu\n", sysInfo.doneHddUserReq);
      // printf("doneSsdUserReqInPeriod: %lu\n", sysInfo.doneSsdUserReqInPeriod);
      // printf("doneHddUserReqInPeriod: %lu\n\n", sysInfo.doneHddUserReqInPeriod);
      // printf("userSsdReqResTime: %f\n", sysInfo.userSsdReqResTime);
      // printf("userHddReqResTime: %f\n", sysInfo.userHddReqResTime);
      // printf("userSsdReqResTimeInPeriod: %f\n", sysInfo.userSsdReqResTimeInPeriod);
      // printf("userHddReqResTimeInPeriod: %f\n", sysInfo.userHddReqResTimeInPeriod);
      record_statistics(&sysInfo, user, cpffSystemTime);
      print_credit();
      if(credit_replenish(user, totalWeight) != 0) {
        print_error(-1, "Can't replenish user credit!");
      }
      nextReplenishCreditTime += TIME_PERIOD;
      printf(CYAN_BOLD_ITALIC"Credit Replenish!!!!\n"COLOR_RESET);
      print_credit();
      c = getchar();
    }

    /*All requests have been done*/
    if(is_empty_queue(hostQueue) && is_empty_queue(hddDeviceQueue) && is_empty_queue(ssdDeviceQueue) && are_all_user_hdd_queue_empty(user) && are_all_user_ssd_queue_empty(user)) {
      return;
    }
    // c = getchar();
    
  }
}

/*[推進系統時間]*/
double shift_cpffSystemTime(double ssdReqCompleteTime, double hddReqCompleteTime) {
  double minimal = nextReplenishCreditTime;
  /*若只要ssd device queue 或 所有user ssd queue其中一個不為空的，則ssdReqCompleteTime必須列入系統時間推進的參考。*/
  if(!is_empty_queue(ssdDeviceQueue) || !are_all_user_ssd_queue_empty(user)) {
    if(minimal > ssdReqCompleteTime) {
      minimal = ssdReqCompleteTime;
    }
  }
  /*若只要hdd device queue 或 所有user hdd queue其中一個不為空的，則hddReqCompleteTime必須列入系統時間推進的參考。*/
  if(!is_empty_queue(hddDeviceQueue) || !are_all_user_hdd_queue_empty(user)) {
    if(minimal > hddReqCompleteTime) {
      minimal = hddReqCompleteTime;
    }
  }
 
  if(!is_empty_queue(hostQueue)) {
    if(minimal > hostQueue->head->r.arrivalTime) {
      minimal = hostQueue->head->r.arrivalTime;
    }
  }
  
  return minimal;
}

/*[統計完成的結果]*/
void statistics_done_func(REQ *r, char *reqType) {
  if(!strcmp("SSD", reqType)) {     //ssd request
    if(r->isSystemRequest) {      //ssd system request
      sysInfo.doneSsdSysReq++;
      sysInfo.doneSsdSysReqInPeriod++;
      user[r->userno-1].doneSsdSysReq++;
      user[r->userno-1].doneSsdSysReqInPeriod++;
      sysInfo.sysSsdReqResTime += r->responseTime;
      sysInfo.sysSsdReqResTimeInPeriod += r->responseTime;
      user[r->userno-1].sysSsdReqResTime += r->responseTime;
      user[r->userno-1].sysSsdReqResTimeInPeriod += r->responseTime;
      return;
    } else {    //ssd user request
      sysInfo.doneSsdUserReq++;
      sysInfo.doneSsdUserReqInPeriod++;
      user[r->userno-1].doneSsdUserReq++;
      user[r->userno-1].doneSsdUserReqInPeriod++;
      sysInfo.userSsdReqResTime += r->responseTime;
      sysInfo.userSsdReqResTimeInPeriod += r->responseTime;
      user[r->userno-1].userSsdReqResTime += r->responseTime;
      user[r->userno-1].userSsdReqResTimeInPeriod += r->responseTime;
      return;
    }
  } else {
    if(r->isSystemRequest) {      //hdd system request
      sysInfo.doneHddSysReq++;
      sysInfo.doneHddSysReqInPeriod++;
      user[r->userno-1].doneHddSysReq++;
      user[r->userno-1].doneHddSysReqInPeriod++;
      sysInfo.sysHddReqResTime += r->responseTime;
      sysInfo.sysHddReqResTimeInPeriod += r->responseTime;
      user[r->userno-1].sysHddReqResTime += r->responseTime;
      user[r->userno-1].sysHddReqResTimeInPeriod += r->responseTime;
      return;
    } else {    //hdd user request
      sysInfo.doneHddUserReq++;
      sysInfo.doneHddUserReqInPeriod++;
      user[r->userno-1].doneHddUserReq++;
      user[r->userno-1].doneHddUserReqInPeriod++;
      sysInfo.userHddReqResTime += r->responseTime;
      sysInfo.userHddReqResTimeInPeriod += r->responseTime;
      user[r->userno-1].userHddReqResTime += r->responseTime;
      user[r->userno-1].userHddReqResTimeInPeriod += r->responseTime;
      return;
    }
  }
}

/*[寫檔紀錄]*/
void record_statistics(systemInfo *sysInfo, userInfo *user, double systemTime) {
  if(sysInfo != NULL) {
    
    fprintf(systemResultFile, "\n-----------------------------\n");
    fprintf(systemResultFile, "Total request: %lu\n", sysInfo->totalReq);
    fprintf(systemResultFile, "Total user requests: %lu\t(Read: %lu\tWrite: %lu)\n", sysInfo->totalUserReq, sysInfo->userReadReq, sysInfo->userWriteReq);
    fprintf(systemResultFile, "Total system requests: %lu\t(Read: %lu\tWrite: %lu)\n", sysInfo->totalSysReq);
    fprintf(systemResultFile, "-----------------------------\n");
    
    return;
  }
}

int main(int argc, char *argv[]) {

  //Check arguments
	if (argc != 8) {
    fprintf(stderr, "usage: %s <Trace file> <param file for SSD> <output file for SSD> <param file for HDD> <output file for HDD> <output file for STAT> <output file for result>\n", argv[0]);
    exit(1);
  }

  /*系統起始訊息*/ 
  printf(CYAN_BOLD_ITALIC"CPFF is running.....\n\n"COLOR_RESET);

  par[0] = argv[1];         //trace file path
  par[1] = argv[2];         
  par[2] = argv[3];
  par[3] = argv[4];
  par[4] = argv[5];
  par[5] = argv[6];
  par[6] = argv[7];

  /*初始化*/ 
  initialize(&par[0]);

  execute_CPFF_framework();

  /*Remove Disksim(SSD and HDD simulators)*/
  rm_disksim();
  
  // Waiting for SSDsim and HDDsim process
  wait(NULL);
  wait(NULL);
   //OR
  //printf("Main Process waits for: %d\n", wait(NULL));
  //printf("Main Process waits for: %d\n", wait(NULL));
  
  fclose(systemResultFile);

  return 0;
}