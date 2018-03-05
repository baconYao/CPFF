#include "cpff_main.h"

QUE *hostQueue;                   //claim host queue
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
int shiftIdleTimeCounter = 0;     //用來處理IdleTime的計數器
int ssdCandidate[NUM_OF_USER];    //在SSD scheduler決定哪個user可以dispatch request到disksim
int hddCandidate[NUM_OF_USER];    //在HDD scheduler決定哪個user可以dispatch request到disksim
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
  while(1) {
    double ssdServiceTime ,hddServiceTime;
    print_progress(cpffSystemTime, sysInfo.totalReq, sysInfo.doneSsdSysReq+sysInfo.doneHddSysReq+sysInfo.doneSsdUserReq+sysInfo.doneHddUserReq, hostQueue->size);
    
    /*執行prize caching，根據系統時間(cpffSystemTime)將host queue內的request送至對應的user queue內*/ 
    prize_caching(cpffSystemTime, user, hostQueue, &sysInfo);
  
    ssdReqCompleteTime = ssd_credit_scheduler(&sysInfo, user, cpffSystemTime, ssdCandidate); 
    if(ssdReqCompleteTime < cpffSystemTime) {
      ssdReqCompleteTime = cpffSystemTime;
    }

    hddReqCompleteTime = hdd_credit_scheduler(&sysInfo, user, cpffSystemTime, hddCandidate);
    if(hddReqCompleteTime < cpffSystemTime) {
      hddReqCompleteTime = cpffSystemTime;
    }

    /*All requests have been done*/
    if(is_empty_queue(hostQueue) && are_all_user_hdd_queue_empty(user) && are_all_user_ssd_queue_empty(user)) {
      /*在system要結束時(cpffSystemTime可能不會剛好是1000ms的倍數)，也必須統計結果*/
      period_record_statistics(&sysInfo, user, cpffSystemTime, &periodStatisticRecord);
      period_csv_statistics(&sysInfo, user, cpffSystemTime, &systemPeriodRecord, &eachUserPeriodRecord);
      second_record_statistics(&sysInfo, user, cpffSystemTime, &secondStatisticRecord);
      second_csv_statistics(&sysInfo, user, cpffSystemTime, &systemSecondRecord, &eachUserSecondRecord);
      return;   //return to main()
    }

    /*推進系統時間*/
    cpffSystemTime = shift_cpffSystemTime(ssdReqCompleteTime, hddReqCompleteTime);
    
    /*每TIME_PERIOD(1000ms)，重新補充credit*/
    if(cpffSystemTime == nextReplenishCreditTime) {
      /*每隔STAT_FOR_TIME_PERIODS * cpffSystemTime記錄一次*/
      if((int)cpffSystemTime % (STAT_FOR_TIME_PERIODS * 1000) == 0) {
        period_record_statistics(&sysInfo, user, cpffSystemTime, &periodStatisticRecord);
        period_csv_statistics(&sysInfo, user, cpffSystemTime, &systemPeriodRecord, &eachUserPeriodRecord);
        reset_period_value(&sysInfo, user);
      }
      /*每1000ms做的事情*/
      second_record_statistics(&sysInfo, user, cpffSystemTime, &secondStatisticRecord);
      second_csv_statistics(&sysInfo, user, cpffSystemTime, &systemSecondRecord, &eachUserSecondRecord);
      reset_second_value(&sysInfo, user);

      /*補充credit*/
      if(credit_replenish(user, totalWeight) != 0) {
        print_error(-1, "Can't replenish user credit!");
      }
      nextReplenishCreditTime += TIME_PERIOD;   //推進下次補充credit的時間 (+1000ms)
    }
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
  final_result_statistics(&sysInfo, user, &finalResult);

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