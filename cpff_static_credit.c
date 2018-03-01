#include "cpff_static_credit.h"

/**[針對所有Users初始化Credit]
 * @param {userInfo *} user [users' information]
 * @return {int} 0/-1 [success/fail]
 */
int init_credit(userInfo *user, int totalWeight) {
  //Check the sum of user weights 
  if (totalWeight == 0) {
      print_error(totalWeight, "[static_credit.c]Total User Weight = ");
      return -1;
  }
  int i;
  //Distribute user credit to users by the individual user weight
  for(i = 0; i < NUM_OF_USER; i++) {
    userSSDCredit[i] = INIT_CREDIT * ((double)user[i].globalWeight/totalWeight);
    userHDDCredit[i] = INIT_CREDIT * ((double)user[i].globalWeight/totalWeight);
  }

  //User credit output
  print_credit();
  printf(COLOR_GB" [CREDIT]creditInit() finish!\n"COLOR_RESET);
  return 0;
}

/**
 * [針對所有Users補充Credit]
 * @return {int} 0/-1 [success/fail]
 */
 int credit_replenish(userInfo *user, int totalWeight) {
  //Check the sum of user weights 
  if (totalWeight == 0) {
    print_error(totalWeight, "[CREDIT]Error totalWeight = ");
    return -1;
  }
  
  //Replenishment policy: User credit is negative or positive
  int i;
  for(i = 0; i < NUM_OF_USER; i++) {
    userSSDCredit[i] = INIT_CREDIT * ((double)user[i].globalWeight/totalWeight);
    userHDDCredit[i] = INIT_CREDIT * ((double)user[i].globalWeight/totalWeight);
  }
  
  return 0;
}


/**
 * [預先扣除credit]
 * @param {userInfo} user [User pointer]
 * @param {unsigned} userno [User number(0-n)]
 * @param {REQ *} r [the reuqest in the user queue]
 * @param {char *} creditType [判斷是哪種type的credit (SSDCredit or HDDCredit)]
 * @return {double} userCredit [Modified user credit]
 */
void credit_pre_charge(userInfo *user, unsigned userno, REQ *r, char *creditType) {
  if(!strcmp("SSDCredit", creditType)) {
    switch (r->reqFlag) {
      case DISKSIM_READ:
        r->preChargeValue = SSD_READ_PRE_CHAREG_VALUE;
        userSSDCredit[userno] -= SSD_READ_PRE_CHAREG_VALUE;
        break;
      case DISKSIM_WRITE:
        r->preChargeValue = SSD_WRITE_PRE_CHAREG_VALUE;
        userSSDCredit[userno] -= SSD_WRITE_PRE_CHAREG_VALUE;
        break;
      default:
        break;
    }
  } else {
    /*HDD request利用每個user的HDD request average response time當成pre-charge value*/
    switch (r->reqFlag) {
      case DISKSIM_READ:
        if(user[userno].doneHddUserReq != 0 ) {     //此user已經有HDD request完成
          r->preChargeValue = user[userno].userHddReqResTime / (double)user[userno].doneHddUserReq;
          userHDDCredit[userno] -= r->preChargeValue;
          break;
        } else {        //此user沒有HDD request完成，所以用自訂的pre-charge value
          r->preChargeValue = HDD_READ_PRE_CHAREG_VALUE;
          userHDDCredit[userno] -= HDD_READ_PRE_CHAREG_VALUE;
          break;
        }
      case DISKSIM_WRITE:
        if(user[userno].doneHddUserReq != 0 ) {     //此user已經有HDD request完成
          r->preChargeValue = user[userno].userHddReqResTime / (double)user[userno].doneHddUserReq;
          userHDDCredit[userno] -= r->preChargeValue;
          break;
        } else {        //此user沒有HDD request完成，所以用自訂的pre-charge value
          r->preChargeValue = HDD_WRITE_PRE_CHAREG_VALUE;
          userHDDCredit[userno] -= HDD_WRITE_PRE_CHAREG_VALUE;
          break;
        }
        break;
      default:
        break;
    }
  }
}

/**
 * [當實際的request完成後，會和pre-charge扣的credit比較，進行補償]
 * @param {unsigned} userno [User number(0-n)]
 * @param {double} serviceTime [request的service time]
 * @param {REQ *} r [the reuqest which was serviced by Simulator]
 * @param {char *} creditType [判斷是哪種type的credit (SSDCredit or HDDCredit)]
 * @return {double} userCredit [Modified user credit]
 */
void credit_compensate(unsigned userno, double serviceTime, REQ *r, char *creditType) {
  //Charge credit
  if(!strcmp("SSDCredit", creditType)) {
    switch (r->reqFlag) {
      case DISKSIM_READ:
        userSSDCredit[userno] = userSSDCredit[userno] + (r->preChargeValue - serviceTime);
        break;
      case DISKSIM_WRITE:
        userSSDCredit[userno] = userSSDCredit[userno] + (r->preChargeValue - serviceTime);
        break;
      default:
        break;
    }
  } else {
    switch (r->reqFlag) {
      case DISKSIM_READ:
        userHDDCredit[userno] = userHDDCredit[userno] + (r->preChargeValue - serviceTime);
        break;
      case DISKSIM_WRITE:
        userHDDCredit[userno] = userHDDCredit[userno] + (r->preChargeValue - serviceTime);
        break;
      default:
        break;
    }
  }
}

/**
 * [根據Credit策略，將user ssd queue內的request送至ssd device queue內]
 * @param {userInfo *} user [users' information]
 * @param {QUE *} ssdDeviceQueue [ssd device queue pointer]
 */
void ssd_credit_scheduler(userInfo *user, QUE *ssdDeviceQueue) {
  int i;
  for(i = 0; i < NUM_OF_USER; i++) {
    while(1) {
      if(is_empty_queue(user[i].ssdQueue) || userSSDCredit[i] <= 0) {
        break;
      }

      REQ *tmp;
      tmp = calloc(1, sizeof(REQ));
      copy_req(&(user[i].ssdQueue->head->r), tmp);
      /*pre charge ssd credit*/ 
      credit_pre_charge(user, i, tmp, "SSDCredit");

      insert_req_to_device_que_tail(ssdDeviceQueue, tmp);
      /*移除user ssd queue的head指向的request*/
      remove_req_from_queue_head(user[i].ssdQueue);
      /* release variable */
      free(tmp);
    }
  }
}

/**
 * [根據Credit策略，將user hdd queue內的request送至hdd device queue內]
 * @param {userInfo *} user [users' information]
 * @param {QUE *} hddDeviceQueue [hdd device queue pointer]
 */
void hdd_credit_scheduler(userInfo *user, QUE *hddDeviceQueue) {
  int i;
  for(i = 0; i < NUM_OF_USER; i++) {
    while(1) {
      if(is_empty_queue(user[i].hddQueue) || userHDDCredit[i] <= 0) {
        break;
      }

      REQ *tmp;
      tmp = calloc(1, sizeof(REQ));
      copy_req(&(user[i].hddQueue->head->r), tmp);
      /*pre charge ssd credit*/ 
      credit_pre_charge(user, i, tmp, "HDDCredit");

      insert_req_to_device_que_tail(hddDeviceQueue, tmp);
      /*移除user ssd queue的head指向的request*/
      remove_req_from_queue_head(user[i].hddQueue);
      /* release variable */
      free(tmp);
    }
  }
}

/**
 * [印出所有user的credit]
 */
 void print_credit() {
  int i;
  for(i = 0; i < NUM_OF_USER; i++) {
      printf(COLOR_GB" [CREDIT]USER %u SSD CREDIT: %lf\n"COLOR_RESET, i+1, userSSDCredit[i]);
      printf(COLOR_GB" [CREDIT]USER %u HDD CREDIT: %lf\n"COLOR_RESET, i+1, userHDDCredit[i]);
  }
}

