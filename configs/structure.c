#include "structure.h"

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

bool insert_req_to_host_que_tail(QUE *hostQ, REQ *r) {
  /*page_count代表此request共存取多少SSD page, 意思是將request切成多個sub-requests*/
  int page_count;
  page_count = r->reqSize/SSD_PAGE2SECTOR;
  int i;
  
  for(i = 0; i < page_count; i++) {
    if(hostQ->head == NULL) {            //第一個request
      hostQ->head = calloc(1, sizeof(QUE_ITEM));
      hostQ->tail = hostQ->head;
      copyReq(r, &(hostQ->head->r));
      hostQ->head->r.diskBlkno += i * SSD_PAGE2SECTOR;
      hostQ->head->r.reqSize = SSD_PAGE2SECTOR;               //4096/512 = 8
    } else {
      QUE_ITEM *tmp;
      tmp = calloc(1, sizeof(QUE_ITEM));
      copyReq(r, &(tmp->r));
      tmp->r.diskBlkno += i*SSD_PAGE2SECTOR;
      tmp->r.reqSize = SSD_PAGE2SECTOR;
      tmp->front_req = hostQ->tail;
      hostQ->tail->back_req = tmp;
      hostQ->tail = tmp;
    }
    hostQ->size++;
  }
  
  // printf("Request: %lf %u %lu %u %u %u\n", r->arrivalTime, r->devno, r->diskBlkno, r->reqSize, r->reqFlag, r->userno);
  totalRequests += page_count;
  return true;
}

/**
 * [從尾端(tail)將request放進User Queue並轉換以Page為單位的requests]
 * @param {userInfo} user [整個系統的所有user的array]
 * @param {char*} qType [user queue的類型]
 * @param {REQ*} r [系統定義的Req pointer]
 */
bool insert_req_to_user_que_tail(userInfo *user, char *qType, REQ *r) {

  // printf("User number: %u\n", r->userno);       //user number 1 ~ n
  unsigned userno = r->userno - 1;                 //user array從 0 ~ n-1
  
  /*page_count代表此request共存取多少SSD page*/
	int page_count;
	page_count = r->reqSize/SSD_PAGE2SECTOR;
  int i;
  
  if(!strcmp(qType, "SSD")) {               //此request屬於SSD
    for(i = 0; i < page_count; i++) {
      if(user[userno].ssdQueue->head == NULL) {            //第一個request
        user[userno].ssdQueue->head = calloc(1, sizeof(QUE_ITEM));
        user[userno].ssdQueue->tail = user[userno].ssdQueue->head;
        copyReq(r, &(user[userno].ssdQueue->head->r));
        user[userno].ssdQueue->head->r.diskBlkno += i * SSD_PAGE2SECTOR;
        user[userno].ssdQueue->head->r.reqSize = SSD_PAGE2SECTOR;
      } else {
        QUE_ITEM *tmp;
        tmp = calloc(1, sizeof(QUE_ITEM));
        copyReq(r, &(tmp->r));
        tmp->r.diskBlkno += i*SSD_PAGE2SECTOR;
        tmp->r.reqSize = SSD_PAGE2SECTOR;
        tmp->front_req = user[userno].ssdQueue->tail;
        user[userno].ssdQueue->tail->back_req = tmp;
        user[userno].ssdQueue->tail = tmp;
      }
    }
  } else {            //此request屬於HDD
    for(i = 0; i < page_count; i++) {
      if(user[userno].hddQueue->head == NULL) {            //第一個request
        user[userno].hddQueue->head = calloc(1, sizeof(QUE_ITEM));
        user[userno].hddQueue->tail = user[userno].hddQueue->head;
        copyReq(r, &(user[userno].hddQueue->head->r));
        user[userno].hddQueue->head->r.diskBlkno += i * SSD_PAGE2SECTOR;
        user[userno].hddQueue->head->r.reqSize = SSD_PAGE2SECTOR;
      } else {
        QUE_ITEM *tmp;
        tmp = calloc(1, sizeof(QUE_ITEM));
        copyReq(r, &(tmp->r));
        tmp->r.diskBlkno += i*SSD_PAGE2SECTOR;
        tmp->r.reqSize = SSD_PAGE2SECTOR;
        tmp->front_req = user[userno].hddQueue->tail;
        user[userno].hddQueue->tail->back_req = tmp;
        user[userno].hddQueue->tail = tmp;
      }
    }
  }
  
  // printf("Request: %lf %u %lu %u %u %u\n", r->arrivalTime, r->devno, r->diskBlkno, r->reqSize, r->reqFlag, r->userno);
  totalRequests += page_count;
  return true;
}

/**
 * [從頭端(head)將request移出Queue]
 * @param {QUE*} Que[指定的queue]
 * @return {REQ}
 */
REQ *remove_req_from_queue_head(QUE *Que) {
  // printf("Host queue's memory address: %p\n", &Que);
  // printf("user num: %u\n", Que->head->r.userno);
  // printf("user num: %u\n", Que->head->back_req->back_req->r.userno);
  // printf("Que size %d\n", Que->size);
  
  /*Que is empty, return nothing*/
  if(is_empty_queue(Que)) {
    printf(CYAN_BOLD_ITALIC"Queue is empty!\n"COLOR_RESET);
    return NULL;  
  }

  QUE_ITEM *tmp = Que->head;
  // REQ *r = (REQ *) malloc(sizeof(REQ));
  // copyReq(&(tmp->r), r);
  // printf("Req's: %u\n",tmp->r.userno);

  /*Only one request in queue*/ 
  if(Que->size == 1) {
    Que->size--;
    // free(Que->head);
    Que->head = Que->tail = NULL;
    return &(tmp->r);
  }

  Que->head = Que->head->back_req;
  Que->head->front_req = NULL;
  // free(tmp);
  Que->size--;
  return &(tmp->r);
}

/**
 * [複製Request內容, r to copy]
 * @param {REQ*} r [系統定義的Req pointer]
 * @param {REQ*} copy [系統定義的Req pointerr]
 */
 void copyReq(REQ *r, REQ *copy) {
	copy->arrivalTime = r->arrivalTime;
	copy->devno = r->devno;
	copy->diskBlkno = r->diskBlkno;
	copy->reqSize = r->reqSize;
	copy->reqFlag = r->reqFlag;
	copy->userno = r->userno;
	copy->responseTime = r->responseTime;
}

/**
 * [印出queue的內容]
 */
 void print_queue_content(QUE *Que) {
	int i;
	unsigned count;
	QUE_ITEM *tmp;
  count = 0;
  tmp = Que->head;
  while (tmp != NULL) {
    count++;
    printf("%6lu <-> ", tmp->r.diskBlkno);
    tmp = tmp->back_req;
  }
  printf("NULL (%u)\n", count);
}


/**
 * [根據進入User queue的request數量，取得Request總數(應同於Trace筆數)]
 * @return {unsigned long} totalRequests [The num of requests]
 */
 unsigned long get_total_reqs() {
	return totalRequests;
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
