#include "cpff_structure.h"

/**
 * [建立host queue]
 * @param {char*} qType [user queue的類型]
 */
 QUE *build_host_queue() {
  printf(COLOR_YB"Host queue is building..............");  
  QUE *q = (QUE *) malloc(sizeof(QUE));
  q -> head = q -> tail =NULL;
  q -> size = 0;
  printf("OK\n"COLOR_RESET);    
  return q;
}

/**
 * [建立user queue]
 * @param {int} userNum [user的代號]
 * @param {char*} qType [user queue的類型]
 */
 QUE *build_user_queue(int userNum, char *qType) {
  printf(COLOR_YB"User %d's %s queue is building......", userNum, qType);  
  QUE *q = (QUE *) malloc(sizeof(QUE));
  q -> head = q -> tail =NULL;
  q -> size = 0;
  printf("OK\n"COLOR_RESET);    
  return q;
}

/**
 * [建立device queue]
 * @param {char*} qType [device queue的類型]
 */
 QUE *build_device_queue(char *qType) {
  printf(COLOR_YB"%s Device queue is building......", qType);  
  QUE *q = (QUE *) malloc(sizeof(QUE));
  q -> head = q -> tail =NULL;
  q -> size = 0;
  printf("OK\n"COLOR_RESET);    
  return q;
}

/**
 * [從尾端(tail)將request放進host Queue並轉換以Page為單位的requests]
 * @param {QUE*} hostQ[整個系統的host queue]
 * @param {REQ*} r [系統定義的Req pointer]
 * return true [將trace所有的reuqests放入queue後即回傳]
 */

bool insert_req_to_host_que_tail(QUE *hostQ, REQ *r, systemInfo *sysInfo) {
  /*page_count代表此request共存取多少SSD page, 意思是將request切成多個sub-requests, each sub-request size is 4 KB*/
  int page_count;
  page_count = r->reqSize/SSD_PAGE2SECTOR;
  int i;
  
  for(i = 0; i < page_count; i++) {
    if(hostQ->head == NULL) {            //第一個request
      hostQ->head = calloc(1, sizeof(QUE_ITEM));
      hostQ->tail = hostQ->head;
      copy_req(r, &(hostQ->head->r));
      hostQ->head->r.diskBlkno += i * SSD_PAGE2SECTOR;
      hostQ->head->r.reqSize = SSD_PAGE2SECTOR;               //4096/512 = 8 sectors
    } else {
      QUE_ITEM *tmp;
      tmp = calloc(1, sizeof(QUE_ITEM));
      copy_req(r, &(tmp->r));
      tmp->r.diskBlkno += i*SSD_PAGE2SECTOR;
      tmp->r.reqSize = SSD_PAGE2SECTOR;
      tmp->front_req = hostQ->tail;
      hostQ->tail->back_req = tmp;
      hostQ->tail = tmp;
    }
    hostQ->size++;
  }
  
  /*Statistics*/
  sysInfo->totalUserReq += page_count;
  if(r->reqFlag == DISKSIM_READ) {
    sysInfo->userReadReq += page_count;
  } else {
    sysInfo->userWriteReq += page_count;
  }

  return true;
}

/**
 * [從尾端(tail)將request放進User Queue]
 * @param {userInfo} user [整個系統的所有user的array]
 * @param {char*} qType [user queue的類型]
 * @param {REQ*} r [系統定義的Req pointer]
 */
bool insert_req_to_user_que_tail(userInfo *user, char *qType, REQ *r) {

  // printf("User number: %u\n", r->userno);       //user number 1 ~ n
  unsigned userno = r->userno - 1;                 //user array從 0 ~ n-1
  
  if(!strcmp(qType, "SSD")) {               //此request屬於SSD
    if(user[userno].ssdQueue->head == NULL) {            //第一個request
      user[userno].ssdQueue->head = calloc(1, sizeof(QUE_ITEM));
      user[userno].ssdQueue->tail = user[userno].ssdQueue->head;
      copy_req(r, &(user[userno].ssdQueue->head->r));
    
    } else {
      QUE_ITEM *tmp;
      tmp = calloc(1, sizeof(QUE_ITEM));
      copy_req(r, &(tmp->r));
      tmp->front_req = user[userno].ssdQueue->tail;
      user[userno].ssdQueue->tail->back_req = tmp;
      user[userno].ssdQueue->tail = tmp;
    }
    user[userno].ssdQueue->size++;
  } else {            //此request屬於HDD
    if(user[userno].hddQueue->head == NULL) {            //第一個request
      user[userno].hddQueue->head = calloc(1, sizeof(QUE_ITEM));
      user[userno].hddQueue->tail = user[userno].hddQueue->head;
      copy_req(r, &(user[userno].hddQueue->head->r));
    } else {
      QUE_ITEM *tmp;
      tmp = calloc(1, sizeof(QUE_ITEM));
      copy_req(r, &(tmp->r));
      tmp->front_req = user[userno].hddQueue->tail;
      user[userno].hddQueue->tail->back_req = tmp;
      user[userno].hddQueue->tail = tmp;
    }
    user[userno].hddQueue->size++;
  }
  
  return true;
}

/**
 * [從尾端(tail)將request放進Device Queue]
 * @param {userInfo} user [整個系統的所有user的array]
 * @param {REQ*} r [系統定義的Req pointer]
 */
bool insert_req_to_device_que_tail(QUE *deviceQ, REQ *r) {

  if(deviceQ->head == NULL) {            //第一個request
    deviceQ->head = calloc(1, sizeof(QUE_ITEM));
    deviceQ->tail = deviceQ->head;
    copy_req(r, &(deviceQ->head->r));
  } else {
    QUE_ITEM *tmp;
    tmp = calloc(1, sizeof(QUE_ITEM));
    copy_req(r, &(tmp->r));
    tmp->front_req = deviceQ->tail;
    deviceQ->tail->back_req = tmp;
    deviceQ->tail = tmp;
  }
  deviceQ->size++;
  return true;
}

/**
 * [從頭端(head)將request移出Queue]
 * @param {QUE*} Que[指定的queue]
 * @return {REQ}
 */
void remove_req_from_queue_head(QUE *Que) {
  // printf("Host queue's memory address: %p\n", &Que);
  // printf("user num: %u\n", Que->head->r.userno);
  // printf("user num: %u\n", Que->head->back_req->back_req->r.userno);
  // printf("Que size %d\n", Que->size);
  
  QUE_ITEM *tmp = Que->head;

  /*Only one request in queue*/ 
  if(Que->size == 1) {
    Que->size--;
    Que->head = Que->tail = NULL;
    free(tmp);
    return;
  }

  Que->head = Que->head->back_req;
  Que->head->front_req = NULL;
  free(tmp);
  Que->size--;
  return;
}

/**
 * [複製Request內容, r to copy]
 * @param {REQ*} r [系統定義的Req pointer]
 * @param {REQ*} copy [系統定義的Req pointerr]
 */
 void copy_req(REQ *r, REQ *copy) {
	copy->arrivalTime = r->arrivalTime;
	copy->devno = r->devno;
	copy->diskBlkno = r->diskBlkno;
	copy->reqSize = r->reqSize;
	copy->reqFlag = r->reqFlag;
	copy->userno = r->userno;
  copy->responseTime = r->responseTime;
  copy->isSystemRequest = r->isSystemRequest;
  copy->preChargeValue = r->preChargeValue;
}

/**
 * [印出queue的內容]
 */
 void print_queue_content(QUE *Que, char *queueName) {
	int i;
	unsigned count;
	QUE_ITEM *tmp;
  count = 0;
  tmp = Que->head;
  printf(COLOR_YB"%s:\n", queueName);
  while (tmp != NULL) {
    count++;
    printf("%6lu <-> ", tmp->r.diskBlkno);
    tmp = tmp->back_req;
  }
  printf("NULL (%u)\n"COLOR_RESET, count);
}

/**
 * [檢查Queue是否為空]
 */
bool is_empty_queue(QUE *Que) {
  if(Que->size > 0) {
    return false;
  }

  return true;         // queue is empty
}

/**
 * [檢查所有的user ssd queue是否為空]
 */
bool are_all_user_ssd_queue_empty(userInfo *user) {
  int i;
  for(i = 0; i < NUM_OF_USER; i++) {
    if(user[i].ssdQueue->size > 0) {
      return false;
    }
  }
  return true;         // all user ssd queue are empty
}

/**
 * [檢查所有的user hdd queue是否為空]
 */
bool are_all_user_hdd_queue_empty(userInfo *user) {
  int i;
  for(i = 0; i < NUM_OF_USER; i++) {
    if(user[i].hddQueue->size > 0) {
      return false;
    }
  }
  return true;         // all user hdd queue are empty
}