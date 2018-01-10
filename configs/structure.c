#include "structure.h"

/**
 * [建立user queue]
 * @param {int} userNum [user的代號]
 * @param {char*} qType [user queue的類型]
 */
USER_QUE *build_user_queue(int userNum, char *qType) {
  printf("User %d's %s queue is building......\n", userNum, qType);  
  USER_QUE *q = (USER_QUE *) malloc(sizeof(USER_QUE));
  q -> head = q -> tail =NULL;
  q -> size = 0;
  return q;
}

/**
 * [從尾端(tail)將request放進User Queue並轉換以Page為單位的requests]
 * @param {userInfo} user [整個系統的所有user的array]
 * @param {char*} qType [user queue的類型]
 * @param {REQ*} r [系統定義的Req pointer]
 */
bool insert_req_to_user_que(userInfo *user, char *qType, REQ *r) {

  // printf("User number: %u\n", r->userno);       //user number 1 ~ n
  unsigned userno = r->userno - 1;                 //user array從 0 ~ n-1
  
  /*page_count代表此request共存取多少SSD page*/
	int page_count;
	page_count = r->reqSize/SSD_PAGE2SECTOR;
  int i;
  
  if(!strcmp(qType, "SSD")) {               //此request屬於SSD
    for(i = 0; i < page_count; i++) {
      if(user[userno].ssdQueue->head == NULL) {            //第一個request
        user[userno].ssdQueue->head = calloc(1, sizeof(USER_QUE_ITEM));
        user[userno].ssdQueue->tail = user[userno].ssdQueue->head;
        copyReq(r, &(user[userno].ssdQueue->head->r));
        user[userno].ssdQueue->head->r.diskBlkno += i * SSD_PAGE2SECTOR;
        user[userno].ssdQueue->head->r.reqSize = SSD_PAGE2SECTOR;
      } else {
        USER_QUE_ITEM *tmp;
        tmp = calloc(1, sizeof(USER_QUE_ITEM));
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
        user[userno].hddQueue->head = calloc(1, sizeof(USER_QUE_ITEM));
        user[userno].hddQueue->tail = user[userno].hddQueue->head;
        copyReq(r, &(user[userno].hddQueue->head->r));
        user[userno].hddQueue->head->r.diskBlkno += i * SSD_PAGE2SECTOR;
        user[userno].hddQueue->head->r.reqSize = SSD_PAGE2SECTOR;
      } else {
        USER_QUE_ITEM *tmp;
        tmp = calloc(1, sizeof(USER_QUE_ITEM));
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
 * [根據進入User queue的request數量，取得Request總數(應同於Trace筆數)]
 * @return {unsigned long} totalRequests [The num of requests]
 */
 unsigned long get_total_reqs() {
	return totalRequests;
}