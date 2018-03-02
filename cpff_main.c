#include "cpff_main.h"

QUE *hostQueue;                   //claim host queue
// QUE *ssdDeviceQueue;              //claim SSD device queue
// QUE *hddDeviceQueue;              //claim HDD device queue
systemInfo sysInfo;               //Record system information
userInfo user[NUM_OF_USER];       //建立user
pid_t SSDsimProc, HDDsimProc;     //Sub-process id: SSD and HDD simulator

FILE *trace;          //讀取的trace
FILE *param;          //paramfile
FILE *secondStatisticRecord;     //每個second會做的記錄檔(system and all user information)
FILE *periodStatisticRecord;     //每個period會做的記錄檔(system and all user information)
FILE *systemSecondRecord;   //系統每second的記錄檔
FILE *systemPeriodRecord;   //系統每period的記錄檔
FILE *eachUserSecondRecord[NUM_OF_USER];   //每個user的每second記錄檔
FILE *eachUserPeriodRecord[NUM_OF_USER];   //每個user的每period記錄檔
FILE *finalResult;   //每個user的每period記錄檔
char *par[6];         //CPFF system arguments
int totalWeight = 0;  //所有user的global weight累加
int shiftIdleTimeCounter = 0;
int ssdCandidate[NUM_OF_USER];
int hddCandidate[NUM_OF_USER];
double cpffSystemTime = 0.0;      //cpff的系統時間

double nextReplenishCreditTime = TIME_PERIOD;    //credit重新補充的時間，每次加1000.0(ms)，預設為1000.0ms = 1 second

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

  //Open param file
  param = fopen("./cpff_statistics_dir/Param.txt", "w");
  if (!param) {
    print_error(-1, "Param file open failed ");
  }

  #ifdef STATIC_CACHING_SPACE
    printf(COLOR_RB"Caching Space Policy: STATIC_CACHING_SPACE\n"COLOR_RESET);
    fprintf(param, "Caching Space Policy: STATIC_CACHING_SPACE\n");
  #elif defined DYNAMIC_CACHING_SPACE
    printf(COLOR_RB"Caching Space Policy: DYNAMIC_CACHING_SPACE\n"COLOR_RESET);
    fprintf(param, "Caching Space Policy: DYNAMIC_CACHING_SPACE\n");
  #elif defined COMPETITION_CACHING_SPACE
    printf(COLOR_RB"Caching Space Policy: COMPETITION_CACHING_SPACE\n"COLOR_RESET);
    fprintf(param, "Caching Space Policy: COMPETITION_CACHING_SPACE\n");
  #endif
  
  #ifdef STATIC_CREDIT
    printf(COLOR_RB"Credit Policy: STATIC_CREDIT\n"COLOR_RESET);
    fprintf(param, "Credit Policy: STATIC_CREDIT\n");
  #elif defined DYNAMIC_CREDIT
    printf(COLOR_RB"Credit Policy: DYNAMIC_CREDIT\n"COLOR_RESET);
    fprintf(param, "Credit Policy: DYNAMIC_CREDIT\n");
  #endif

  fprintf(param, "Cache Space: %d pages\n", SSD_CACHING_SPACE_BY_PAGES);
  
  /*初始化 HDDsim & SSDsim*/
  init_disksim();
  sleep(3);

  //Open trace file
  trace = fopen(par[0], "r");
  if (!trace) {
    print_error(-1, "Trace file open failed ");
  }
  fprintf(param, "Trace: %s\n", par[0]);        //Record trace name into param file
  
  //Open period record file
  if((periodStatisticRecord = fopen("./cpff_statistics_dir/Period_Statistic_Record.txt", "w")) == NULL) {
    print_error(-1, "Can't open the cpff_statistics_dir/Period_Statistic_Record.txt file");
  }

  //Open second record file
  if((secondStatisticRecord = fopen("./cpff_statistics_dir/Second_Statistic_Record.txt", "w")) == NULL) {
    print_error(-1, "Can't open the cpff_statistics_dir/Seriod_Statistic_Record.txt file");
  }

  //Open system period record file
  if((systemPeriodRecord = fopen("./cpff_statistics_dir/System_Period_Record.csv", "w")) == NULL) {
    print_error(-1, "Can't open the cpff_statistics_dir/System_Period_Record.csv file");
  }

  //Open system second record file
  if((systemSecondRecord = fopen("./cpff_statistics_dir/System_Second_Record.csv", "w")) == NULL) {
    print_error(-1, "Can't open the cpff_statistics_dir/System_Second_Record.csv file");
  }

  //Open each user period record files
  int i = 0;
  for(i = 0; i < NUM_OF_USER; i++) {
    int userno = i+1;
    char userStr[20];
    sprintf(userStr,"%d",userno);
    char dir[80]="./cpff_statistics_dir/User_";
    strcat(dir, userStr);
    strcat(dir, "_Period_Statistic_Record.csv");
    if((eachUserPeriodRecord[i] = fopen(dir, "w")) == NULL) {
      print_error(-1, "Can't open the user period csv file");
    }
  }

  //Open each user second record files
  for(i = 0; i < NUM_OF_USER; i++) {
    int userno = i+1;
    char userStr[20];
    sprintf(userStr,"%d",userno);
    char dir[80]="./cpff_statistics_dir/User_";
    strcat(dir, userStr);
    strcat(dir, "_Second_Statistic_Record.csv");
    if((eachUserSecondRecord[i] = fopen(dir, "a")) == NULL) {
      print_error(-1, "Can't open the user second csv file");
    }
  }

  /*初始化candidate*/
  for(i = 0; i < NUM_OF_USER; i++) {
    if(i == 0) {
      ssdCandidate[i] = 1;
      hddCandidate[i] = 1;
    } else {
      ssdCandidate[i] = 0;
      hddCandidate[i] = 0;
    }
  }

  /*建立host queue*/
  hostQueue = build_host_queue();
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
  sysInfo.userReadReqInSecond = 0;
  sysInfo.userReadReqInPeriod = 0;
  sysInfo.sysSsdReadReqInSecond = 0;
  sysInfo.sysSsdReadReqInPeriod = 0;
  sysInfo.userWriteReqInSecond = 0;
  sysInfo.userWriteReqInPeriod = 0;
  sysInfo.sysSsdWriteReqInSecond = 0;
  sysInfo.sysSsdWriteReqInPeriod = 0;
  sysInfo.sysHddWriteReqInSecond = 0;			
  sysInfo.sysHddWriteReqInPeriod = 0;			
  sysInfo.totalSysReq = 0;				
  sysInfo.sysSsdReadReq = 0;				
  sysInfo.sysSsdWriteReq = 0;				
  sysInfo.sysHddWriteReq = 0;				
  sysInfo.evictCount = 0;				
  sysInfo.evictCountInSecond = 0;				
  sysInfo.evictCountInPeriod = 0;				
  sysInfo.dirtyCount = 0;				
  sysInfo.dirtyCountInSecond = 0;				
  sysInfo.dirtyCountInPeriod = 0;				
  sysInfo.hitCount = 0;				
  sysInfo.hitCountInSecond = 0;				
  sysInfo.hitCountInPeriod = 0;				
  sysInfo.missCount = 0;				
  sysInfo.missCountInSecond = 0;				
  sysInfo.missCountInPeriod = 0;				
  sysInfo.doneSsdSysReq = 0;			
  sysInfo.doneHddSysReq = 0;			
  sysInfo.doneSsdSysReqInSecond = 0;			
  sysInfo.doneSsdSysReqInPeriod = 0;			
  sysInfo.doneHddSysReqInSecond = 0;			
  sysInfo.doneHddSysReqInPeriod = 0;			
  sysInfo.doneSsdUserReq = 0;			
  sysInfo.doneHddUserReq = 0;			
  sysInfo.doneSsdUserReqInSecond = 0;			
  sysInfo.doneSsdUserReqInPeriod = 0;			
  sysInfo.doneHddUserReqInSecond = 0;			
  sysInfo.doneHddUserReqInPeriod = 0;			
  sysInfo.userSsdReqResTime = 0.0;				    	
  sysInfo.userHddReqResTime = 0.0;				    	
  sysInfo.userSsdReqResTimeInSecond = 0.0;					    	
  sysInfo.userSsdReqResTimeInPeriod = 0.0;					    	
  sysInfo.userHddReqResTimeInSecond = 0.0;					    	
  sysInfo.userHddReqResTimeInPeriod = 0.0;					    	
  sysInfo.sysSsdReqResTime = 0.0;					    	
  sysInfo.sysHddReqResTime = 0.0;					    	
  sysInfo.sysSsdReqResTimeInSecond = 0.0;				
  sysInfo.sysSsdReqResTimeInPeriod = 0.0;				
  sysInfo.sysHddReqResTimeInSecond = 0.0;				
  sysInfo.sysHddReqResTimeInPeriod = 0.0;				

  /*初始化user資訊*/
  unsigned weight = 0;
  for(i = 0; i < NUM_OF_USER; i++) {
    fscanf(trace, "%u", &weight);       //讀取user的global weight
    fprintf(param, "User %d's weight: %u\n", i+1, weight);      //Record user weight into param file
    user[i].globalWeight = weight;
    user[i].ssdCredit = 0.0;
    user[i].hddCredit = 0.0;
    user[i].ssdQueue = build_user_queue(i+1, "SSD");    //建立user ssd queue
    user[i].hddQueue = build_user_queue(i+1, "HDD");    //建立user hdd queue
    user[i].totalReq = 0;
    user[i].totalSsdReq = 0;
    user[i].totalHddReq = 0;
    user[i].totalUserReq = 0;
    user[i].userReadReqInSecond = 0;
    user[i].userReadReqInPeriod = 0;
    user[i].sysSsdReadReqInSecond = 0;
    user[i].sysSsdReadReqInPeriod = 0;
    user[i].userWriteReqInSecond = 0;
    user[i].userWriteReqInPeriod = 0;
    user[i].sysSsdWriteReqInSecond = 0;
    user[i].sysSsdWriteReqInPeriod = 0;
    user[i].sysHddWriteReqInSecond = 0;
    user[i].sysHddWriteReqInPeriod = 0;
    user[i].userReadReq = 0;
    user[i].userWriteReq = 0;
    user[i].totalSysReq = 0;
    user[i].sysSsdReadReq = 0;
    user[i].sysSsdWriteReq = 0;
    user[i].sysHddWriteReq = 0;
    user[i].doneSsdSysReq = 0;
    user[i].doneHddSysReq = 0;
    user[i].doneSsdSysReqInSecond = 0;
    user[i].doneSsdSysReqInPeriod = 0;
    user[i].doneHddSysReqInSecond = 0;
    user[i].doneHddSysReqInPeriod = 0;
    user[i].doneSsdUserReq = 0;
    user[i].doneHddUserReq = 0;
    user[i].doneSsdUserReqInSecond = 0;
    user[i].doneSsdUserReqInPeriod = 0;
    user[i].doneHddUserReqInSecond = 0;
    user[i].doneHddUserReqInPeriod = 0;
    user[i].evictCount = 0;
    user[i].evictCountInSecond = 0;
    user[i].evictCountInPeriod = 0;
    user[i].dirtyCount = 0;
    user[i].dirtyCountInSecond = 0;
    user[i].dirtyCountInPeriod = 0;
    user[i].hitCount = 0;
    user[i].hitCountInSecond = 0;
    user[i].hitCountInPeriod = 0;
    user[i].missCount = 0;
    user[i].missCountInSecond = 0;
    user[i].missCountInPeriod = 0;
    user[i].userSsdReqResTime = 0.0;
    user[i].userHddReqResTime = 0.0;
    user[i].userSsdReqResTimeInSecond = 0.0;
    user[i].userSsdReqResTimeInPeriod = 0.0;
    user[i].userHddReqResTimeInSecond = 0.0;
    user[i].userHddReqResTimeInPeriod = 0.0;
    user[i].sysSsdReqResTime = 0.0;
    user[i].sysHddReqResTime = 0.0;
    user[i].sysSsdReqResTimeInSecond = 0.0;
    user[i].sysSsdReqResTimeInPeriod = 0.0;
    user[i].sysHddReqResTimeInSecond = 0.0;
    user[i].sysHddReqResTimeInPeriod = 0.0;
    user[i].cachingSpace = 0;

    totalWeight += weight;              //累加所有user的globalWeight
  }

  // /*建立device queue*/
  // ssdDeviceQueue = build_device_queue("SSD");
  // hddDeviceQueue = build_device_queue("HDD");

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
  /*釋放File descriptor variable*/ 
  fclose(trace);
  fclose(param);

  free(tmp);

  printf("Total user requests: %lu\tUser read requests: %lu\tUser write requests: %lu\n", sysInfo.totalUserReq, sysInfo.userReadReq, sysInfo.userWriteReq);
};

/**
 * [cpff主程式]
 */
void execute_CPFF_framework() {
  printf("Press enter to continue program.\n");
  char c = getchar();
  double ssdReqCompleteTime = hostQueue->head->r.arrivalTime;      //表示被送進SSD sim 的SSD request在系統時間(cpffSystemTime)的甚麼時候做完。 (ssdReqCompleteTime = request servie time + cpffSystemTime)
  double hddReqCompleteTime = hostQueue->head->r.arrivalTime;      //表示被送進HDD sim 的HDD request在系統時間(cpffSystemTime)的甚麼時候做完。 (hddReqCompleteTime = request servie time + cpffSystemTime)
  cpffSystemTime = hostQueue->head->r.arrivalTime;
  // printf(COLOR_RB"\nssdReqCompleteTime: %f\thddReqCompleteTime: %f\thost: %f\n"COLOR_RESET,ssdReqCompleteTime, hddReqCompleteTime, hostQueue->head->r.arrivalTime);
  while(1) {
    double ssdServiceTime ,hddServiceTime;
    print_progress(cpffSystemTime, sysInfo.totalReq, sysInfo.doneSsdSysReq+sysInfo.doneHddSysReq+sysInfo.doneSsdUserReq+sysInfo.doneHddUserReq, hostQueue->size);

    
    // printf("U1 SSD Q: %d (Done: %d)\nU1 HDD Q: %d (Done: %d)\nU2 SSD Q: %d (Done: %d)\nU2 HDD Q: %d (Done: %d)\n", user[0].ssdQueue->size, user[0].doneSsdSysReq+user[0].doneSsdUserReq, user[0].hddQueue->size, user[0].doneHddSysReq+user[0].doneHddUserReq, user[1].ssdQueue->size, user[1].doneSsdSysReq+user[1].doneSsdUserReq, user[1].hddQueue->size, user[1].doneHddSysReq+user[1].doneHddUserReq);    
    
    /*執行prize caching，根據系統時間(cpffSystemTime)將host queue內的request送至對應的user queue內*/ 
    prize_caching(cpffSystemTime, user, hostQueue, &sysInfo);
    // c = getchar();

  
    ssdReqCompleteTime = ssd_credit_scheduler(&sysInfo, user, cpffSystemTime, ssdCandidate); 
    if(ssdReqCompleteTime < cpffSystemTime) {
      ssdReqCompleteTime = cpffSystemTime;
    }

    hddReqCompleteTime = hdd_credit_scheduler(&sysInfo, user, cpffSystemTime, hddCandidate);
    if(hddReqCompleteTime < cpffSystemTime) {
      hddReqCompleteTime = cpffSystemTime;
    }

    // if(!is_empty_queue(hostQueue)) {
    //   printf(COLOR_RB"!!!ssdReqCompleteTime: %f\thddReqCompleteTime: %f\thost: %f\n\n", ssdReqCompleteTime, hddReqCompleteTime, hostQueue->head->r.arrivalTime);
    // } else {
    //   printf(COLOR_RB"!!!ssdReqCompleteTime: %f\thddReqCompleteTime: %f\n\n", ssdReqCompleteTime, hddReqCompleteTime);
    // }
    // c = getchar();
    // printf(COLOR_BB"\nHost: %d\nU1 SSD Q: %d (Done: %d)\nU1 HDD Q: %d (Done: %d)\nU2 SSD Q: %d (Done: %d)\nU2 HDD Q: %d (Done: %d)\n"COLOR_RESET, hostQueue->size, user[0].ssdQueue->size, user[0].doneSsdSysReq+user[0].doneSsdUserReq, user[0].hddQueue->size, user[0].doneHddSysReq+user[0].doneHddUserReq, user[1].ssdQueue->size, user[1].doneSsdSysReq+user[1].doneSsdUserReq, user[1].hddQueue->size, user[1].doneHddSysReq+user[1].doneHddUserReq);    
    // print_credit(user);
    

    /*All requests have been done*/
    if(is_empty_queue(hostQueue) && are_all_user_hdd_queue_empty(user) && are_all_user_ssd_queue_empty(user)) {
      /*在system要結束時(cpffSystemTime可能不會剛好是1000ms的倍數)，也必須統計結果*/
      period_record_statistics(&sysInfo, user, cpffSystemTime);
      period_csv_statistics(&sysInfo, user, cpffSystemTime);
      second_record_statistics(&sysInfo, user, cpffSystemTime);
      second_csv_statistics(&sysInfo, user, cpffSystemTime);
      return;   //return to main()
    }

    /*推進系統時間*/
    cpffSystemTime = shift_cpffSystemTime(ssdReqCompleteTime, hddReqCompleteTime);
    
    /*每TIME_PERIOD(1000ms)，重新補充credit*/
    if(cpffSystemTime == nextReplenishCreditTime) {
      /*每隔STAT_FOR_TIME_PERIODS * cpffSystemTime記錄一次*/
      if((int)cpffSystemTime % (STAT_FOR_TIME_PERIODS * 1000) == 0) {
        period_record_statistics(&sysInfo, user, cpffSystemTime);
        period_csv_statistics(&sysInfo, user, cpffSystemTime);
        reset_period_value(&sysInfo, user);
      }
      // print_credit(user);
      /*每1000ms做的事情*/
      second_record_statistics(&sysInfo, user, cpffSystemTime);
      second_csv_statistics(&sysInfo, user, cpffSystemTime);
      reset_second_value(&sysInfo, user);

      /*補充credit*/
      if(credit_replenish(user, totalWeight) != 0) {
        print_error(-1, "Can't replenish user credit!");
      }
      nextReplenishCreditTime += TIME_PERIOD;   //推進下次補充credit的時間 (+1000ms)
      // printf(CYAN_BOLD_ITALIC"Credit Replenish!!!!\n"COLOR_RESET);
      // print_credit(user);
      
      // c = getchar();
    }

    
    // c = getchar();
    
  }
}

/*[推進系統時間]*/
double shift_cpffSystemTime(double ssdReqCompleteTime, double hddReqCompleteTime) {
  double minimal = nextReplenishCreditTime;
  /*若只要所有user ssd queue其中一個不為空的，則ssdReqCompleteTime必須列入系統時間推進的參考。*/
  if(!are_all_user_ssd_queue_empty(user)) {
    if(minimal > ssdReqCompleteTime) {
      minimal = ssdReqCompleteTime;
      // printf("S\n");
    }
  }
  /*若只要所有user hdd queue其中一個不為空的，則hddReqCompleteTime必須列入系統時間推進的參考。*/
  if(!are_all_user_hdd_queue_empty(user)) {
    if(minimal > hddReqCompleteTime) {
      minimal = hddReqCompleteTime;
      // printf("H\n");      
    }
  }
 
  if(!is_empty_queue(hostQueue)) {
    if(minimal > hostQueue->head->r.arrivalTime) {
      minimal = hostQueue->head->r.arrivalTime;
      // printf("Host\n");      
    }
  }

  /*處理Idle Time情況*/
  if(minimal == cpffSystemTime) {
    shiftIdleTimeCounter++;
    /*Shift Idle Time, 選擇離cpffSystemTime最近的時間，並且回傳*/
    if(shiftIdleTimeCounter == 3) {
      // printf("\n\nShift Idle Time\n\n");
      // char c = getchar();
      int arrValidElement = 4;
      double arr[4];
      if(is_empty_queue(hostQueue)) {
        arr[0] = nextReplenishCreditTime - cpffSystemTime;
        arr[1] = ssdReqCompleteTime - cpffSystemTime;
        arr[2] = hddReqCompleteTime - cpffSystemTime;
        arrValidElement = 3;
      } else {
        arr[0] = nextReplenishCreditTime - cpffSystemTime;
        arr[1] = ssdReqCompleteTime - cpffSystemTime;
        arr[2] = hddReqCompleteTime - cpffSystemTime;
        arr[3] = hostQueue->head->r.arrivalTime - cpffSystemTime;
      }
      // printf("arrValidElement: %d\n", arrValidElement);
      int i, j;
      double tmp;
      for(i = 0; i < arrValidElement; i++) {
        for(j = arrValidElement -1 ; j > i; j--) {
          tmp = arr[j];
          arr[j] = arr[j-1];
          arr[j-1] = tmp;
        }
      }
      for(i = 0; i < arrValidElement; i++) {
        if(arr[i] != 0) {
          shiftIdleTimeCounter = 0;
          return arr[i] + cpffSystemTime;
        }
      }
    }
  } else {
    shiftIdleTimeCounter = 0;
  }
  return minimal;
}


/*[每second就會記錄詳細資訊]*/
void second_record_statistics(systemInfo *sysInfo, userInfo *user, double systemTime) {
  if(sysInfo != NULL) {
    
    fprintf(secondStatisticRecord, "\n-----------------------------\n");
    fprintf(secondStatisticRecord, "CPFF system time: %f\n", systemTime);
    fprintf(secondStatisticRecord, "\nSystem Info\n");
    fprintf(secondStatisticRecord, "All user requests in second, Read: %lu, Write: %lu\n", sysInfo->userReadReqInSecond, sysInfo->userWriteReqInSecond);
    fprintf(secondStatisticRecord, "All system requests in second, SSD Read: %lu, SSD Write: %lu, HDD Write: %lu\n", sysInfo->sysSsdReadReqInSecond, sysInfo->sysSsdWriteReqInSecond, sysInfo->sysHddWriteReqInSecond);
    fprintf(secondStatisticRecord, "All evictCountInSecond: %lu, dirtyCountInSecond: %lu, hitCountInSecond: %lu, missCountInSecond: %lu\n", sysInfo->evictCountInSecond, sysInfo->dirtyCountInSecond, sysInfo->hitCountInSecond, sysInfo->missCountInSecond);
    fprintf(secondStatisticRecord, "All doneSsdSysReqInSecond: %lu, doneHddSysReqInSecond: %lu, doneSsdUserReqInSecond: %lu, doneHddUserReqInSecond: %lu\n", sysInfo->doneSsdSysReqInSecond, sysInfo->doneHddSysReqInSecond, sysInfo->doneSsdUserReqInSecond, sysInfo->doneHddUserReqInSecond);
    fprintf(secondStatisticRecord, "All sysSsdReqResTimeInSecond: %f, sysHddReqResTimeInSecond: %f, userSsdReqResTimeInSecond: %f, userHddReqResTimeInSecond: %f\n", sysInfo->sysSsdReqResTimeInSecond, sysInfo->sysHddReqResTimeInSecond, sysInfo->userSsdReqResTimeInSecond, sysInfo->userHddReqResTimeInSecond);
    
    int i;
    for(i = 0; i < NUM_OF_USER; i++) {
      fprintf(secondStatisticRecord, "\nUser %d\n", i+1);
      fprintf(secondStatisticRecord, "user requests in second, Read: %lu, Write: %lu\n", user[i].userReadReqInSecond, user[i].userWriteReqInSecond);
      fprintf(secondStatisticRecord, "system requests in second, SSD Read: %lu, SSD Write: %lu, HDD Write: %lu\n", user[i].sysSsdReadReqInSecond, user[i].sysSsdWriteReqInSecond, user[i].sysHddWriteReqInSecond);
      fprintf(secondStatisticRecord, "evictCountInSecond: %lu, dirtyCountInSecond: %lu, hitCountInSecond: %lu, missCountInSecond: %lu\n", user[i].evictCountInSecond, user[i].dirtyCountInSecond, user[i].hitCountInSecond, user[i].missCountInSecond);
      fprintf(secondStatisticRecord, "doneSsdSysReqInSecond: %lu, doneHddSysReqInSecond: %lu, doneSsdUserReqInSecond: %lu, doneHddUserReqInSecond: %lu\n", user[i].doneSsdSysReqInSecond, user[i].doneHddSysReqInSecond, user[i].doneSsdUserReqInSecond, user[i].doneHddUserReqInSecond);
      fprintf(secondStatisticRecord, "sysSsdReqResTimeInSecond: %f, sysHddReqResTimeInSecond: %f, userSsdReqResTimeInSecond: %f, userHddReqResTimeInSecond: %f\n", user[i].sysSsdReqResTimeInSecond, user[i].sysHddReqResTimeInSecond, user[i].userSsdReqResTimeInSecond, user[i].userHddReqResTimeInSecond);
    }
    fprintf(secondStatisticRecord, "-----------------------------\n");
    
    return;
  }
}

/*[每second寫成csv紀錄檔(systemtime, SSD throughput, HDD throughput, SSD avg response time, HDD avg response time, hit rate)]*/
void second_csv_statistics(systemInfo *sysInfo, userInfo *user, double systemTime) {
  if(sysInfo != NULL) {
    
    double ssdAvgResponse, hddAvgResponse, ssdThroughput, hddThroughput, hitRate;
    ssdThroughput = ((double)sysInfo->doneSsdUserReqInSecond * 4.0 / 1024) / 1.0;
    hddThroughput = ((double)sysInfo->doneHddUserReqInSecond * 4.0 / 1024) / 1.0;
    ssdAvgResponse = sysInfo->userSsdReqResTimeInSecond / (double)sysInfo->doneSsdUserReqInSecond;
    hddAvgResponse = sysInfo->userHddReqResTimeInSecond / (double)sysInfo->doneHddUserReqInSecond;
    hitRate = (double)sysInfo->hitCountInSecond / ((double)sysInfo->hitCountInSecond + (double)sysInfo->missCountInSecond);
    fprintf(systemSecondRecord, "%f,%f,%f,%f,%f,%f\n", systemTime, ssdThroughput, hddThroughput, ssdAvgResponse, hddAvgResponse, hitRate);
    
    int i;
    for(i = 0; i < NUM_OF_USER; i++) {
      ssdThroughput = ((double)user[i].doneSsdUserReqInSecond * 4.0 / 1024) / 1.0;
      hddThroughput = ((double)user[i].doneHddUserReqInSecond * 4.0 / 1024) / 1.0;
      ssdAvgResponse = user[i].userSsdReqResTimeInSecond / (double)user[i].doneSsdUserReqInSecond;
      hddAvgResponse = user[i].userHddReqResTimeInSecond / (double)user[i].doneHddUserReqInSecond;
      hitRate = (double)user[i].hitCountInSecond / ((double)user[i].hitCountInSecond + (double)user[i].missCountInSecond);
      fprintf(eachUserSecondRecord[i], "%f,%f,%f,%f,%f,%f\n", systemTime, ssdThroughput, hddThroughput, ssdAvgResponse, hddAvgResponse, hitRate);
    }
    return;
  }
}

/*[This function is used to reset second value every second]*/
void reset_second_value(systemInfo *sysInfo, userInfo *user) {
  /*Reset system's second value*/
  sysInfo->userReadReqInSecond = 0;
  sysInfo->userWriteReqInSecond = 0;
  sysInfo->sysSsdReadReqInSecond = 0;
  sysInfo->sysSsdWriteReqInSecond = 0;
  sysInfo->sysHddWriteReqInSecond = 0;
  sysInfo->evictCountInSecond = 0;
  sysInfo->dirtyCountInSecond = 0;
  sysInfo->hitCountInSecond = 0;
  sysInfo->missCountInSecond = 0;
  sysInfo->doneSsdSysReqInSecond = 0;
  sysInfo->doneHddSysReqInSecond = 0;
  sysInfo->doneSsdUserReqInSecond = 0;
  sysInfo->doneHddUserReqInSecond = 0;
  sysInfo->userSsdReqResTimeInSecond = 0.0;
  sysInfo->userHddReqResTimeInSecond = 0.0;
  sysInfo->sysSsdReqResTimeInSecond = 0.0;
  sysInfo->sysHddReqResTimeInSecond = 0.0;

  /*Reset each user's second value*/
  int i;
  for(i = 0; i < NUM_OF_USER; i++) {
    user[i].userReadReqInSecond = 0;
    user[i].userWriteReqInSecond = 0;
    user[i].sysSsdReadReqInSecond = 0;
    user[i].sysSsdWriteReqInSecond = 0;
    user[i].sysHddWriteReqInSecond = 0;
    user[i].evictCountInSecond = 0;
    user[i].dirtyCountInSecond = 0;
    user[i].hitCountInSecond = 0;
    user[i].missCountInSecond = 0;
    user[i].doneSsdSysReqInSecond = 0;
    user[i].doneHddSysReqInSecond = 0;
    user[i].doneSsdUserReqInSecond = 0;
    user[i].doneHddUserReqInSecond = 0;
    user[i].userSsdReqResTimeInSecond = 0.0;
    user[i].userHddReqResTimeInSecond = 0.0;
    user[i].sysSsdReqResTimeInSecond = 0.0;
    user[i].sysHddReqResTimeInSecond = 0.0;
  }
}

/*[每隔period就會記錄詳細資訊]*/
void period_record_statistics(systemInfo *sysInfo, userInfo *user, double systemTime) {
  if(sysInfo != NULL) {
    
    fprintf(periodStatisticRecord, "\n-----------------------------\n");
    fprintf(periodStatisticRecord, "CPFF system time: %f\n", systemTime);
    fprintf(periodStatisticRecord, "\nSystem Info\n");
    fprintf(periodStatisticRecord, "All user requests in period, Read: %lu, Write: %lu\n", sysInfo->userReadReqInPeriod, sysInfo->userWriteReqInPeriod);
    fprintf(periodStatisticRecord, "All system requests in period, SSD Read: %lu, SSD Write: %lu, HDD Write: %lu\n", sysInfo->sysSsdReadReqInPeriod, sysInfo->sysSsdWriteReqInPeriod, sysInfo->sysHddWriteReqInPeriod);
    fprintf(periodStatisticRecord, "All evictCountInPeriod: %lu, dirtyCountInPeriod: %lu, hitCountInPeriod: %lu, missCountInPeriod: %lu\n", sysInfo->evictCountInPeriod, sysInfo->dirtyCountInPeriod, sysInfo->hitCountInPeriod, sysInfo->missCountInPeriod);
    fprintf(periodStatisticRecord, "All doneSsdSysReqInPeriod: %lu, doneHddSysReqInPeriod: %lu, doneSsdUserReqInPeriod: %lu, doneHddUserReqInPeriod: %lu\n", sysInfo->doneSsdSysReqInPeriod, sysInfo->doneHddSysReqInPeriod, sysInfo->doneSsdUserReqInPeriod, sysInfo->doneHddUserReqInPeriod);
    fprintf(periodStatisticRecord, "All sysSsdReqResTimeInPeriod: %f, sysHddReqResTimeInPeriod: %f, userSsdReqResTimeInPeriod: %f, userHddReqResTimeInPeriod: %f\n", sysInfo->sysSsdReqResTimeInPeriod, sysInfo->sysHddReqResTimeInPeriod, sysInfo->userSsdReqResTimeInPeriod, sysInfo->userHddReqResTimeInPeriod);
    
    int i;
    for(i = 0; i < NUM_OF_USER; i++) {
      fprintf(periodStatisticRecord, "\nUser %d\n", i+1);
      fprintf(periodStatisticRecord, "user requests in period, Read: %lu, Write: %lu\n", user[i].userReadReqInPeriod, user[i].userWriteReqInPeriod);
      fprintf(periodStatisticRecord, "system requests in period, SSD Read: %lu, SSD Write: %lu, HDD Write: %lu\n", user[i].sysSsdReadReqInPeriod, user[i].sysSsdWriteReqInPeriod, user[i].sysHddWriteReqInPeriod);
      fprintf(periodStatisticRecord, "evictCountInPeriod: %lu, dirtyCountInPeriod: %lu, hitCountInPeriod: %lu, missCountInPeriod: %lu\n", user[i].evictCountInPeriod, user[i].dirtyCountInPeriod, user[i].hitCountInPeriod, user[i].missCountInPeriod);
      fprintf(periodStatisticRecord, "doneSsdSysReqInPeriod: %lu, doneHddSysReqInPeriod: %lu, doneSsdUserReqInPeriod: %lu, doneHddUserReqInPeriod: %lu\n", user[i].doneSsdSysReqInPeriod, user[i].doneHddSysReqInPeriod, user[i].doneSsdUserReqInPeriod, user[i].doneHddUserReqInPeriod);
      fprintf(periodStatisticRecord, "sysSsdReqResTimeInPeriod: %f, sysHddReqResTimeInPeriod: %f, userSsdReqResTimeInPeriod: %f, userHddReqResTimeInPeriod: %f\n", user[i].sysSsdReqResTimeInPeriod, user[i].sysHddReqResTimeInPeriod, user[i].userSsdReqResTimeInPeriod, user[i].userHddReqResTimeInPeriod);
    }

    fprintf(periodStatisticRecord, "-----------------------------\n");
    
    return;
  }
}

void final_result_statistics(systemInfo *sysInfo, userInfo *user) {
  if(sysInfo != NULL) {
    
    fprintf(finalResult, "\n-----------------------------\n");
    fprintf(finalResult, "\nSystem Info\n");
    fprintf(finalResult, "All user requests in period, Read: %lu, Write: %lu\n", sysInfo->userReadReq, sysInfo->userWriteReq);
    fprintf(finalResult, "All system requests in period, SSD Read: %lu, SSD Write: %lu, HDD Write: %lu\n", sysInfo->sysSsdReadReq, sysInfo->sysSsdWriteReq, sysInfo->sysHddWriteReq);
    fprintf(finalResult, "All evictCount: %lu, dirtyCount: %lu, hitCount: %lu, missCount: %lu\n", sysInfo->evictCount, sysInfo->dirtyCount, sysInfo->hitCount, sysInfo->missCount);
    fprintf(finalResult, "All doneSsdSysReq: %lu, doneHddSysReq: %lu, doneSsdUserReq: %lu, doneHddUserReq: %lu\n", sysInfo->doneSsdSysReq, sysInfo->doneHddSysReq, sysInfo->doneSsdUserReq, sysInfo->doneHddUserReq);
    fprintf(finalResult, "All sysSsdReqResTime: %f, sysHddReqResTime: %f, userSsdReqResTime: %f, userHddReqResTime: %f\n", sysInfo->sysSsdReqResTime, sysInfo->sysHddReqResTime, sysInfo->userSsdReqResTime, sysInfo->userHddReqResTime);
    
    int i;
    for(i = 0; i < NUM_OF_USER; i++) {
      fprintf(finalResult, "\nUser %d\n", i+1);
      fprintf(finalResult, "user requests in period, Read: %lu, Write: %lu\n", user[i].userReadReq, user[i].userWriteReq);
      fprintf(finalResult, "system requests in period, SSD Read: %lu, SSD Write: %lu, HDD Write: %lu\n", user[i].sysSsdReadReq, user[i].sysSsdWriteReq, user[i].sysHddWriteReq);
      fprintf(finalResult, "evictCount: %lu, dirtyCount: %lu, hitCount: %lu, missCount: %lu\n", user[i].evictCount, user[i].dirtyCount, user[i].hitCount, user[i].missCount);
      fprintf(finalResult, "doneSsdSysReq: %lu, doneHddSysReq: %lu, doneSsdUserReq: %lu, doneHddUserReq: %lu\n", user[i].doneSsdSysReq, user[i].doneHddSysReq, user[i].doneSsdUserReq, user[i].doneHddUserReq);
      fprintf(finalResult, "sysSsdReqResTime: %f, sysHddReqResTime: %f, userSsdReqResTime: %f, userHddReqResTime: %f\n", user[i].sysSsdReqResTime, user[i].sysHddReqResTime, user[i].userSsdReqResTime, user[i].userHddReqResTime);
    }

    fprintf(finalResult, "-----------------------------\n");
    
    return;
  }
}

/*[每隔period寫成csv紀錄檔(systemtime, SSD throughput, HDD throughput, SSD avg response time, HDD avg response time, hit rate)]*/
void period_csv_statistics(systemInfo *sysInfo, userInfo *user, double systemTime) {
  if(sysInfo != NULL) {
    
    double ssdAvgResponse, hddAvgResponse, ssdThroughput, hddThroughput, hitRate;
    ssdThroughput = ((double)sysInfo->doneSsdUserReqInPeriod * 4.0 / 1024.0) / ((double)STAT_FOR_TIME_PERIODS * TIME_PERIOD / 1000.0);
    hddThroughput = ((double)sysInfo->doneHddUserReqInPeriod * 4.0 / 1024.0) / ((double)STAT_FOR_TIME_PERIODS * TIME_PERIOD / 1000.0);
    ssdAvgResponse = sysInfo->userSsdReqResTimeInPeriod / (double)sysInfo->doneSsdUserReqInPeriod;
    hddAvgResponse = sysInfo->userHddReqResTimeInPeriod / (double)sysInfo->doneHddUserReqInPeriod;
    hitRate = (double)sysInfo->hitCountInPeriod / ((double)sysInfo->hitCountInPeriod + (double)sysInfo->missCountInPeriod);
    fprintf(systemPeriodRecord, "%f,%f,%f,%f,%f,%f\n", systemTime, ssdThroughput, hddThroughput, ssdAvgResponse, hddAvgResponse, hitRate);
    
    int i;
    for(i = 0; i < NUM_OF_USER; i++) {
      ssdThroughput = ((double)user[i].doneSsdUserReqInPeriod * 4.0 / 1024.0) / ((double)STAT_FOR_TIME_PERIODS * TIME_PERIOD / 1000.0);
      hddThroughput = ((double)user[i].doneHddUserReqInPeriod * 4.0 / 1024.0) / ((double)STAT_FOR_TIME_PERIODS * TIME_PERIOD / 1000.0);
      ssdAvgResponse = user[i].userSsdReqResTimeInPeriod / (double)user[i].doneSsdUserReqInPeriod;
      hddAvgResponse = user[i].userHddReqResTimeInPeriod / (double)user[i].doneHddUserReqInPeriod;
      hitRate = (double)user[i].hitCountInPeriod / ((double)user[i].hitCountInPeriod + (double)user[i].missCountInPeriod);
      fprintf(eachUserPeriodRecord[i], "%f,%f,%f,%f,%f,%f\n", systemTime, ssdThroughput, hddThroughput, ssdAvgResponse, hddAvgResponse, hitRate);
    }
    return;
  }
}

/*[This function is used to reset periods value every period]*/
void reset_period_value(systemInfo *sysInfo, userInfo *user) {
  /*Reset system's period value*/
  sysInfo->userReadReqInPeriod = 0;
  sysInfo->userWriteReqInPeriod = 0;
  sysInfo->sysSsdReadReqInPeriod = 0;
  sysInfo->sysSsdWriteReqInPeriod = 0;
  sysInfo->sysHddWriteReqInPeriod = 0;
  sysInfo->evictCountInPeriod = 0;
  sysInfo->dirtyCountInPeriod = 0;
  sysInfo->hitCountInPeriod = 0;
  sysInfo->missCountInPeriod = 0;
  sysInfo->doneSsdSysReqInPeriod = 0;
  sysInfo->doneHddSysReqInPeriod = 0;
  sysInfo->doneSsdUserReqInPeriod = 0;
  sysInfo->doneHddUserReqInPeriod = 0;
  sysInfo->userSsdReqResTimeInPeriod = 0.0;
  sysInfo->userHddReqResTimeInPeriod = 0.0;
  sysInfo->sysSsdReqResTimeInPeriod = 0.0;
  sysInfo->sysHddReqResTimeInPeriod = 0.0;

  /*Reset each user's period value*/
  int i;
  for(i = 0; i < NUM_OF_USER; i++) {
    user[i].userReadReqInPeriod = 0;
    user[i].userWriteReqInPeriod = 0;
    user[i].sysSsdReadReqInPeriod = 0;
    user[i].sysSsdWriteReqInPeriod = 0;
    user[i].sysHddWriteReqInPeriod = 0;
    user[i].evictCountInPeriod = 0;
    user[i].dirtyCountInPeriod = 0;
    user[i].hitCountInPeriod = 0;
    user[i].missCountInPeriod = 0;
    user[i].doneSsdSysReqInPeriod = 0;
    user[i].doneHddSysReqInPeriod = 0;
    user[i].doneSsdUserReqInPeriod = 0;
    user[i].doneHddUserReqInPeriod = 0;
    user[i].userSsdReqResTimeInPeriod = 0.0;
    user[i].userHddReqResTimeInPeriod = 0.0;
    user[i].sysSsdReqResTimeInPeriod = 0.0;
    user[i].sysHddReqResTimeInPeriod = 0.0;
  }
}

int main(int argc, char *argv[]) {

  //Check arguments
	if (argc != 6) {
    fprintf(stderr, "usage: %s <Trace file> <param file for SSD> <output file for SSD> <param file for HDD> <output file for HDD>\n", argv[0]);
    exit(1);
  }

  /*系統起始訊息*/ 
  printf(CYAN_BOLD_ITALIC"CPFF is running.....\n\n"COLOR_RESET);

  par[0] = argv[1];         //trace file path
  par[1] = argv[2];         
  par[2] = argv[3];
  par[3] = argv[4];
  par[4] = argv[5];

  /*初始化*/ 
  initialize(&par[0]);

  execute_CPFF_framework();
  print_progress(cpffSystemTime, sysInfo.totalReq, sysInfo.doneSsdSysReq+sysInfo.doneHddSysReq+sysInfo.doneSsdUserReq+sysInfo.doneHddUserReq, hostQueue->size);

  //Open finalResult record file
  if((finalResult = fopen("./cpff_statistics_dir/Final_Statistic_Record.txt", "w")) == NULL) {
    print_error(-1, "Can't open the cpff_statistics_dir/Final_Statistic_Record.txt file");
  }
  final_result_statistics(&sysInfo, user);

  /*Remove Disksim(SSD and HDD simulators)*/
  rm_disksim();
  
  // Waiting for SSDsim and HDDsim process
  wait(NULL);
  wait(NULL);
   //OR
  //printf("Main Process waits for: %d\n", wait(NULL));
  //printf("Main Process waits for: %d\n", wait(NULL));
  
  fclose(finalResult);
  fclose(secondStatisticRecord);
  fclose(periodStatisticRecord);
  fclose(systemSecondRecord);
  fclose(systemPeriodRecord);
  int i;
  for(i = 0; i < NUM_OF_USER; i++) {
    fclose(eachUserSecondRecord[i]);
    fclose(eachUserPeriodRecord[i]);
  }

  return 0;
}