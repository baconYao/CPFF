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
 * [預先扣除credit]
 * @param {unsigned} userno [User number(0-n)]
 * @param {REQ *} r [the reuqest in the user queue]
 * @param {char *} creditType [判斷是哪種type的credit (SSDCredit or HDDCredit)]
 * @return {double} userCredit [Modified user credit]
 */
void credit_pre_charge(unsigned userno, REQ *r, char *creditType) {
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
    switch (r->reqFlag) {
      case DISKSIM_READ:
        r->preChargeValue = HDD_READ_PRE_CHAREG_VALUE;
        userHDDCredit[userno] -= HDD_READ_PRE_CHAREG_VALUE;
        break;
      case DISKSIM_WRITE:
        r->preChargeValue = HDD_WRITE_PRE_CHAREG_VALUE;
        userHDDCredit[userno] -= HDD_WRITE_PRE_CHAREG_VALUE;
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
        userSSDCredit[userno] += SSD_READ_PRE_CHAREG_VALUE - serviceTime;
        break;
      case DISKSIM_WRITE:
        userSSDCredit[userno] += SSD_WRITE_PRE_CHAREG_VALUE - serviceTime;
        break;
      default:
        break;
    }
  } else {
    switch (r->reqFlag) {
      case DISKSIM_READ:
        userHDDCredit[userno] += HDD_READ_PRE_CHAREG_VALUE - serviceTime;
        break;
      case DISKSIM_WRITE:
        userHDDCredit[userno] += HDD_WRITE_PRE_CHAREG_VALUE - serviceTime;
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

      /*移除user ssd queue的head指向的request*/
      remove_req_from_queue_head(user[i].ssdQueue);

      insert_req_to_device_que_tail(ssdDeviceQueue, tmp);

      /*pre charge ssd credit*/ 
      credit_pre_charge(i, tmp, "SSDCredit");

      print_REQ(tmp, "BBB");
      print_credit();
      print_queue_content(ssdDeviceQueue, "SSD Device Queue");

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

      /*移除user ssd queue的head指向的request*/
      remove_req_from_queue_head(user[i].hddQueue);

      insert_req_to_device_que_tail(hddDeviceQueue, tmp);

      /*pre charge ssd credit*/ 
      credit_pre_charge(i, tmp, "HDDCredit");

      print_REQ(tmp, "AAA");
      print_credit();
      print_queue_content(hddDeviceQueue, "HDD Device Queue");

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

