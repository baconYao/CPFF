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
    user[i].ssdCredit = INIT_CREDIT * ((double)user[i].globalWeight/totalWeight);
    user[i].hddCredit = INIT_CREDIT * ((double)user[i].globalWeight/totalWeight);
  }

  //User credit output
  print_credit(user);
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
    user[i].ssdCredit = INIT_CREDIT * ((double)user[i].globalWeight/totalWeight);
    user[i].hddCredit = INIT_CREDIT * ((double)user[i].globalWeight/totalWeight);
  }
  
  return 0;
}


/**
 * [預先扣除credit]
 * @param {userInfo} user [User pointer]
 * @param {REQ *} r [the reuqest in the user queue]
 * @param {char *} creditType [判斷是哪種type的credit (SSDCredit or HDDCredit)]
 */
void credit_pre_charge(userInfo *user, REQ *r, char *creditType) {
  if(!strcmp("SSDCredit", creditType)) {
    switch (r->reqFlag) {
      case DISKSIM_READ:
        r->preChargeValue = SSD_READ_PRE_CHAREG_VALUE;
        user[r->userno-1].ssdCredit -= SSD_READ_PRE_CHAREG_VALUE;
        break;
      case DISKSIM_WRITE:
        r->preChargeValue = SSD_WRITE_PRE_CHAREG_VALUE;
        user[r->userno-1].ssdCredit -= SSD_WRITE_PRE_CHAREG_VALUE;
        break;
      default:
        break;
    }
  } else {
    /*HDD request利用每個user的HDD request average response time當成pre-charge value*/
    switch (r->reqFlag) {
      case DISKSIM_READ:
        if(user[r->userno-1].doneHddUserReq != 0 ) {     //此user已經有HDD request完成
          r->preChargeValue = user[r->userno-1].userHddReqResTime / (double)user[r->userno-1].doneHddUserReq;
          user[r->userno-1].hddCredit -= r->preChargeValue;
          break;
        } else {        //此user沒有HDD request完成，所以用自訂的pre-charge value
          r->preChargeValue = HDD_READ_PRE_CHAREG_VALUE;
          user[r->userno-1].hddCredit -= HDD_READ_PRE_CHAREG_VALUE;
          break;
        }
      case DISKSIM_WRITE:
        if(user[r->userno-1].doneHddUserReq != 0 ) {     //此user已經有HDD request完成
          r->preChargeValue = user[r->userno-1].userHddReqResTime / (double)user[r->userno-1].doneHddUserReq;
          user[r->userno-1].hddCredit -= r->preChargeValue;
          break;
        } else {        //此user沒有HDD request完成，所以用自訂的pre-charge value
          r->preChargeValue = HDD_WRITE_PRE_CHAREG_VALUE;
          user[r->userno-1].hddCredit -= HDD_WRITE_PRE_CHAREG_VALUE;
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
 * @param {userInfo *} user [User pointer]
 * @param {double} serviceTime [request的service time]
 * @param {REQ *} r [the reuqest which was serviced by Simulator]
 * @param {char *} creditType [判斷是哪種type的credit (SSDCredit or HDDCredit)]
 */
void credit_compensate(userInfo *user, double serviceTime, REQ *r, char *creditType) {
  //Charge credit
  if(!strcmp("SSDCredit", creditType)) {
    user[r->userno-1].ssdCredit = user[r->userno-1].ssdCredit + (r->preChargeValue - serviceTime);
  } else {
    user[r->userno-1].hddCredit = user[r->userno-1].hddCredit + (r->preChargeValue - serviceTime);
  }
}

/**
 * [根據Credit策略，將user ssd queue內的request送至ssd device queue內]
 * @param {userInfo *} user [users' information]
 * @param {double} systemTime [系統當前時間]
 * @param {int *} ssdCandidate [目前可以送request的人選]
 * @return {double} 回傳時間為此request完成的時間，亦即下個request可以執行的時間
 */
double ssd_credit_scheduler(systemInfo *sys, userInfo *user, double systemTime, int *ssdCandidate) {
  /*所有user ssd queue都request，表示沒有ssd request可以被送進SSD sim，故回傳當前system time*/
  if(are_all_user_ssd_queue_empty(user)) {
    return systemTime;
  }
  int counter = 0, i, candiIndex;
  /*找到candidate*/
  for(i = 0; i < NUM_OF_USER; i++) {
    if(ssdCandidate[i] == 1) {
      candiIndex = i;
      break;
    }
  }
  while(1) {
    if(counter == NUM_OF_USER) {       //繞了一圈，沒有適合的user，故跳出，回到main()
      return systemTime;
    } else if(is_empty_queue(user[candiIndex].ssdQueue) || user[candiIndex].ssdCredit <= 0.0) {     //當此user沒有request在user ssd queue或者user的ssd credit不足時，換下一位user執行
      ssdCandidate[candiIndex] = 0;
      if(candiIndex == NUM_OF_USER - 1) {
        candiIndex = 0;
        ssdCandidate[candiIndex] = 1;
        counter++;
        continue;
      }
      candiIndex++;
      ssdCandidate[candiIndex] = 1;
      counter++;
    } else if(!is_empty_queue(user[candiIndex].ssdQueue) && user[candiIndex].ssdCredit > 0.0) {   //有適合的user，跳出迴圈
      break;
    }
  }
  

  REQ *tmp;
  tmp = calloc(1, sizeof(REQ));
  copy_req(&(user[candiIndex].ssdQueue->head->r), tmp);
  /*pre charge ssd credit*/ 
  credit_pre_charge(user, tmp, "SSDCredit");

  double ssdServiceTime = get_service_time(KEY_MSQ_DISKSIM_1, MSG_TYPE_DISKSIM_1, tmp);   //送進SSD sim，獲得此request的service time
  tmp->responseTime = ssdServiceTime;
  credit_compensate(user, ssdServiceTime, tmp, "SSDCredit");   //進行credit的補償
  double ssdReqCompleteTime = ssdServiceTime + systemTime;   //推進下個ssd request可以做的時間
  
  //statistics
  statistics_done_func(sys, user, tmp, "SSD");

  /*移除user ssd queue的head指向的request*/
  remove_req_from_queue_head(user[candiIndex].ssdQueue);
  /* release variable */
  free(tmp);
  
  return ssdReqCompleteTime;
}

/**
 * [根據Credit策略，將user hdd queue內的request送至hdd device queue內]
 * @param {userInfo *} user [users' information]
 * @param {double} systemTime [系統當前時間]
 * @param {int} hddCandidate [目前可以送request的人選]
 * @return {double} 回傳時間為此request完成的時間，亦即下個request可以執行的時間
 */
double hdd_credit_scheduler(systemInfo *sys, userInfo *user, double systemTime, int *hddCandidate) {
  /*所有user ssd queue都request，表示沒有ssd request可以被送進SSD sim，故回傳當前system time*/
  if(are_all_user_hdd_queue_empty(user)) {
    return systemTime;
  }
  int counter = 0, i, candiIndex;
  /*找到candidate*/
  for(i = 0; i < NUM_OF_USER; i++) {
    if(hddCandidate[i] == 1) {
      candiIndex = i;
      break;
    }
  }
  while(1) {
    
    if(counter == NUM_OF_USER) {       //繞了一圈，沒有適合的user，故跳出，回到main()
      return systemTime;
      
    } else if(is_empty_queue(user[candiIndex].hddQueue) || user[candiIndex].hddCredit <= 0.0) { //當此user沒有request在user hdd queue或者user的hdd credit不足時，換下一位user執行
      hddCandidate[candiIndex] = 0;
      if(candiIndex == NUM_OF_USER - 1) {
        candiIndex = 0;
        hddCandidate[candiIndex] = 1;
        counter++;
        continue;
      }
      candiIndex++;
      hddCandidate[candiIndex] = 1;
      counter++;
    } else if(!is_empty_queue(user[candiIndex].hddQueue) && user[candiIndex].hddCredit > 0.0) {   //有適合的user，跳出迴圈
      break;
    }
  }
  
  REQ *tmp;
  tmp = calloc(1, sizeof(REQ));
  copy_req(&(user[candiIndex].hddQueue->head->r), tmp);
  /*pre charge hdd credit*/ 
  credit_pre_charge(user, tmp, "HDDCredit");
  
  double hddServiceTime = get_service_time(KEY_MSQ_DISKSIM_2, MSG_TYPE_DISKSIM_2, tmp);   //送進HDD sim，獲得此request的service time
  tmp->responseTime = hddServiceTime;
  credit_compensate(user, hddServiceTime, tmp, "HDDCredit");     //進行credit的補償
  double hddReqCompleteTime = hddServiceTime + systemTime;   //推進下個hdd device queue 內 request可以做的時間
  //statistics
  statistics_done_func(sys, user, tmp, "HDD");
  
  /*移除user hdd queue的head指向的request*/
  remove_req_from_queue_head(user[candiIndex].hddQueue);
  /* release variable */
  free(tmp);
  
  return hddReqCompleteTime;
}

/**
 * [印出所有user的credit]
 * @param {userInfo *} user [users' information]
 */
 void print_credit(userInfo *user) {
  int i;
  for(i = 0; i < NUM_OF_USER; i++) {
      printf(COLOR_GB" [CREDIT]USER %u SSD CREDIT: %lf\n"COLOR_RESET, i+1, user[i].ssdCredit);
      printf(COLOR_GB" [CREDIT]USER %u HDD CREDIT: %lf\n"COLOR_RESET, i+1, user[i].hddCredit);
  }
}

