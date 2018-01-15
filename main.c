#include "main.h"

QUE *hostQueue;             //claim host queue
userInfo user[NUM_OF_USER];           //建立user
FILE *trace;          //讀取的trace
char *par[6];         //CPFF system arguments

/**
 * [系統初始化，包含建立host queue、user資訊的初始化、讀取trace I/O request到host queue]
 */
void initialize(char *par[]) {
  //Open trace file
  trace = fopen(par[0], "r");
  if (!trace) {
    printf("Trace file open failed\n");
    exit(1);
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
    user[i].totalSysReq = 0;
    user[i].evictCount = 0;
    user[i].dirtyCount = 0;
    user[i].hitCount = 0;
    user[i].missCount = 0;
    user[i].resTime = 0;
    user[i].resTimeInPeriod = 0;
    user[i].cachingSpace = 0;
  }

  /*建立host queue*/
  hostQueue = build_host_queue();

  /*讀取trace file requests*/ 
  REQ *tmp;
  tmp = calloc(1, sizeof(REQ));
  printf("-------------Reading trace requests--------------\n");
  i = 0;
  while(!feof(trace)) {
    fscanf(trace, "%lf%u%lu%u%u%u", &tmp->arrivalTime, &tmp->devno, &tmp->diskBlkno, &tmp->reqSize, &tmp->reqFlag, &tmp->userno);

    /*將request放進host queue*/ 
    if(!insert_req_to_host_que(hostQueue, tmp)) {
      printf("[Error] request to user queue!\n");
      exit(1);
    }

  }
  /*釋放trace File pointer*/ 
  fclose(trace);

  // print_queue_content(hostQueue);
};


int main(int argc, char *argv[]) {
  /*系統起始訊息*/ 
  printf(CYAN_BOLD_ITALIC"CPFF is running.....\n"COLOR_RESET);

  par[0] = argv[1];         //trace file path

  /*初始化*/ 
  initialize(&par[0]);
  
  
  // printf("%lu\n", get_total_reqs());

  return 0;
}