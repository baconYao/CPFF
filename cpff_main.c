#include "cpff_main.h"

QUE *hostQueue;                   //claim host queue
QUE *ssdDeviceQueue;              //claim SSD device queue
QUE *hddDeviceQueue;              //claim HDD device queue
userInfo user[NUM_OF_USER];       //建立user
pid_t SSDsimProc, HDDsimProc;     //Sub-process id: SSD and HDD simulator

FILE *trace;          //讀取的trace
char *par[6];         //CPFF system arguments
int totalWeight = 0;  //所有user的global weight累加
double cpffSystemTime = 0.0;      //cpff的系統時間
double SsdReqCompleteTime = 0.0;      //表示被送進SSD sim 的SSD request在系統時間(cpffSystemTime)的甚麼時候做完。 (SsdReqCompleteTime = request servie time + cpffSystemTime)
double HddReqCompleteTime = 0.0;      //表示被送進HDD sim 的HDD request在系統時間(cpffSystemTime)的甚麼時候做完。 (SsdReqCompleteTime = request servie time + cpffSystemTime)
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
  // sendFinishControl(KEY_MSQ_DISKSIM_2, MSG_TYPE_DISKSIM_2);

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

  /*建立host queue*/
  hostQueue = build_host_queue();
  // printf("Host queue's memory address: %p\n", &hostQueue);
  if(hostQueue == NULL) {
    print_error(-1, "Can't build Host Queue");
  }

  /*初始化user資訊*/
  int i = 0;
  unsigned weight = 0;
  for(i = 0; i < NUM_OF_USER; i++) {
    fscanf(trace, "%u", &weight);       //讀取user的global weight
    user[i].globalWeight = weight;
    user[i].ssdQueue = build_user_queue(i+1, "SSD");    //建立user ssd queue
    user[i].hddQueue = build_user_queue(i+1, "HDD");    //建立user hdd queue
    user[i].totalReq = 0;
    user[i].ssdReq = 0;
    user[i].totalUserReq = 0;
    user[i].UserReqInPeriod = 0;
    user[i].UserRReq = 0;
    user[i].UserWReq = 0;
    user[i].totalSysReq = 0;
    user[i].SysSsdReadReq = 0;
    user[i].SysSsdWriteReq = 0;
    user[i].SysHddWriteReq = 0;
    user[i].evictCount = 0;
    user[i].dirtyCount = 0;
    user[i].hitCount = 0;
    user[i].missCount = 0;
    user[i].resTime = 0;
    user[i].resTimeInPeriod = 0;
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
    if(!insert_req_to_host_que_tail(hostQueue, tmp)) {
      print_error(-1, "[Error] request to host queue!");
    }

  }
  /*釋放trace File descriptor*/ 
  fclose(trace);
  free(tmp);
};


/**
 * [cpff主程式]
 */
void execute_CPFF_framework() {
  printf("Press enter to continue program.\n");
  char c = getchar();
  
  
  while(1) {

    /*執行prize caching，根據系統時間(cpffSystemTime)將host queue內的request送至對應的user queue內*/ 
    prize_caching(cpffSystemTime, user, hostQueue);

    /*檢查ssd device queue內是否有request，若沒有，則啟動ssd credit base scheduler從user ssd queue內抓取request到ssd device queue*/
    if(is_empty_queue(ssdDeviceQueue)) {
      //do ssd credit base scheduler
    }

    /*檢查hdd device queue內是否有request，若沒有，則啟動hdd credit base scheduler從user hdd queue內抓取request到hdd device queue*/
    if(is_empty_queue(hddDeviceQueue)) {
      //do hdd credit base scheduler
    }
    
    // if(is_empty_queue(user[0].hddQueue)) {
    //   break;
    // }
    // REQ *tmp;
    // tmp = calloc(1, sizeof(REQ));
    // copy_req(&(user[0].hddQueue->head->r), tmp);
    // print_REQ(tmp, "user1 hdd");

    // credit_pre_charge(0, tmp, "HDDCredit");
    // print_credit();
    // double serT = get_service_time(KEY_MSQ_DISKSIM_2, MSG_TYPE_DISKSIM_2, tmp);
    // printf("HDD Service Time: %f\n", serT);
    // print_REQ(tmp, "BBB");
    // credit_compensate(0, serT, tmp, "HDDCredit");
    // print_credit();
    // free(tmp);
    // remove_req_from_queue_head(user[0].hddQueue);
    c = getchar();
    
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

  // print_queue_content(hostQueue, "Host Queue");
  // prize_caching(5.0, user, hostQueue);
  // print_queue_content(user[0].hddQueue, "User 1 HDD Queue");
  // prize_caching(10.0, user, hostQueue);
  // print_queue_content(user[0].hddQueue, "User 1 HDD Queue");
  // prize_caching(15.5, user, hostQueue);
  // print_queue_content(user[0].hddQueue, "User 1 HDD Queue");
  // printf("total req: %lu\n", get_total_reqs());
  // int i = 0;
  // for(i = 0; i < NUM_OF_USER; i++){
  //   printf("User %d:\n", i+1);
  //   printf("User %d:\n", i+1);
  //   print_queue_content(user[i].ssdQueue, "SSD Queue");
  //   print_queue_content(user[i].hddQueue, "HDD Queue");
  //   printf("Read req: %lu\n", i+1, user[i].UserRReq);
  //   printf("Write req: %lu\n", i+1, user[i].UserWReq);
  // }

  // char c = getchar();

  // while(1) {
  //   if(is_empty_queue(user[0].hddQueue)) {
  //     break;
  //   }
  //   REQ *tmp;
  //   tmp = calloc(1, sizeof(REQ));
  //   copy_req(&(user[0].hddQueue->head->r), tmp);
  //   print_REQ(tmp, "user1 hdd");

  //   credit_pre_charge(0, tmp, "HDDCredit");
  //   print_credit();
  //   double serT = get_service_time(KEY_MSQ_DISKSIM_2, MSG_TYPE_DISKSIM_2, tmp);
  //   printf("HDD Service Time: %f\n", serT);
  //   print_REQ(tmp, "BBB");
  //   credit_compensate(0, serT, tmp, "HDDCredit");
  //   print_credit();
  //   free(tmp);
  //   remove_req_from_queue_head(user[0].hddQueue);
  //   c = getchar();
    
  // }


  // while(1) {
  //   REQ *r = remove_req_from_queue_head(hostQueue);
  //   if(r == NULL) {
  //     break;
  //   }

  //   REQ *tmp, *tmp1;
  //   tmp = calloc(1, sizeof(REQ));
  //   tmp1 = calloc(1, sizeof(REQ));
  //   copy_req(r, tmp);
  //   copy_req(r, tmp1);
  //   printf("HDD Service Time: %f\n", get_service_time(KEY_MSQ_DISKSIM_2, MSG_TYPE_DISKSIM_2, tmp1));
  //   char c = getchar();
  //   printf("SSD Service Time: %f\n", get_service_time(KEY_MSQ_DISKSIM_1, MSG_TYPE_DISKSIM_1, tmp));
  //   free(tmp);
  //   free(tmp1);
  //   c = getchar();
  // }
  
  
  // Waiting for SSDsim and HDDsim process
  wait(NULL);
  wait(NULL);
   //OR
  //printf("Main Process waits for: %d\n", wait(NULL));
  //printf("Main Process waits for: %d\n", wait(NULL));



  return 0;
}