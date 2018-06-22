#include "cpff_dynamic_credit.h"

/**[針對所有Users初始化Credit]
 * @param {userInfo *} user [users' information]
 * @return {int} 0/-1 [success/fail]
 */
int init_credit(userInfo *user, int totalWeight) {
  //Check the sum of user weights 
  if (totalWeight == 0) {
      print_error(totalWeight, "[dynamic_credit.c]Error! Total User Weight = ");
      return -1;
  }
  int i;
  //Distribute user credit to users by the individual user weight
  for(i = 0; i < NUM_OF_USER; i++) {
    user[i].ssdCredit = INIT_CREDIT * ((double)user[i].globalWeight/totalWeight);
    user[i].hddCredit = INIT_CREDIT * ((double)user[i].globalWeight/totalWeight);
    user[i].prevSsdCredit = user[i].ssdCredit;
    user[i].prevHddCredit = user[i].hddCredit;
  }

  //User credit output
  print_credit(user);
  printf(COLOR_GB" [CREDIT]creditInit() finish!\n"COLOR_RESET);
  return 0;
}

/**
 * [針對所有Users補充Credit]
 * @param {userInfo *} user [users' information]
 * @return {int} 0/-1 [success/fail]
 */
 int credit_replenish(userInfo *user, int totalWeight, double cpffSystemTime) {
  /*Check the sum of user weights*/
  if (totalWeight == 0) {
    print_error(totalWeight, "[dynamic_credit.c]Error! Total User Weight = ");
    return -1;
  }
  int i;
  /*若時間已經過了warm up time*/
  if((cpffSystemTime > (double)SSD_WARM_UP_TIME*1000.0)) {
    /*到了調整credit的周期,則調整credit*/
    if(((int)cpffSystemTime - SSD_WARM_UP_TIME*1000)%(STAT_FOR_TIME_PERIODS*1000) == 0) {
      // printf(COLOR_BB"\n\nAdjust Time: %f\n"COLOR_RESET, cpffSystemTime);
      ssd_credit_adjust(user);
      hdd_credit_adjust(user);
      
      for(i = 0; i < NUM_OF_USER; i++) {
        if(user[i].adjustSsdCredit != 0.0) {
          user[i].ssdCredit = user[i].adjustSsdCredit;
          user[i].prevSsdCredit = user[i].adjustSsdCredit;

        } else {
          user[i].ssdCredit = user[i].prevSsdCredit;
        }
        if(user[i].adjustHddCredit != 0.0) {
          user[i].hddCredit = user[i].adjustHddCredit;
          user[i].prevHddCredit = user[i].adjustHddCredit;      
        } else {
          user[i].hddCredit = user[i].prevHddCredit;
        }       
       
        // printf(COLOR_YB"\nAdjust U%d --> SSD: %f\tHDD: %f\n"COLOR_RESET, i, user[i].ssdCredit, user[i].hddCredit);
      }
      
      return 0;
    } else if(((int)cpffSystemTime - SSD_WARM_UP_TIME*1000)-(STAT_FOR_TIME_PERIODS*1000) > 0) {        //依照之前調整的credit量補充credit
      for(i = 0; i < NUM_OF_USER; i++) {
        user[i].ssdCredit = user[i].prevSsdCredit;
        user[i].hddCredit = user[i].prevHddCredit;

        // printf(COLOR_GB"\nU%d --> SSD: %f\tHDD: %f\n"COLOR_RESET, i, user[i].ssdCredit, user[i].hddCredit);
      }
      return 0;
    } else {
      for(i = 0; i < NUM_OF_USER; i++) {      //warm up時間結束，但還不到調整credit的時間段，故此時間的credit和預設ㄧ樣
        user[i].ssdCredit = INIT_CREDIT * ((double)user[i].globalWeight/totalWeight);
        user[i].hddCredit = INIT_CREDIT * ((double)user[i].globalWeight/totalWeight);

        user[i].prevSsdCredit = user[i].ssdCredit;
        user[i].prevHddCredit = user[i].hddCredit;
        // printf(COLOR_RB"\nSS U%d --> SSD: %f\tHDD: %f\n"COLOR_RESET, i, user[i].ssdCredit, user[i].hddCredit);
      }
    }
  } else {
    /*在wamr up期間，credit依照weight分配來進行補充*/
    for(i = 0; i < NUM_OF_USER; i++) {
      user[i].ssdCredit = INIT_CREDIT * ((double)user[i].globalWeight/totalWeight);
      user[i].hddCredit = INIT_CREDIT * ((double)user[i].globalWeight/totalWeight);
    }
  }
  return 0;
}

/**
 * [在dynamic credit policy中，每經過一個adjust period，就會調整ssd credit(使用克拉瑪公式解(目前只能解兩位user))]
 * @param {userInfo *} user [users' information]
 */
void ssd_credit_adjust(userInfo *user) {
  /*若所有user ssd queue內都沒有request，則返回(代表credit的值會和上一輪調整的一樣)*/
  if(are_all_user_ssd_queue_empty(user)) {
    // printf("\n1111\n");
    // user[0].adjustSsdCredit = user[0].prevSsdCredit;   
    // user[1].adjustSsdCredit = user[1].prevSsdCredit;      
    return;
  }

  /*user1沒有做任何ssd request(user or system request)，且user1 ssd queue內也沒有任何request，代表此user1很有可能已經做完所有request*/
  if(user[0].doneSsdUserReqInPeriod == 0 && user[0].doneSsdSysReqInPeriod == 0 && is_empty_queue(user[0].ssdQueue)) {
    user[0].adjustSsdCredit = 1000.0 * MINI_CREDIT_PROPORTION;  
    user[1].adjustSsdCredit = 1000.0 * (1.0 - MINI_CREDIT_PROPORTION); 
    // printf("\n222\n");

    return; 
  }

  /*user2沒有做任何ssd request(user or system request)，且user2 ssd queue內也沒有任何request，代表此user2很有可能已經做完所有request*/
  if(user[1].doneSsdUserReqInPeriod == 0 && user[1].doneSsdSysReqInPeriod == 0 && is_empty_queue(user[1].ssdQueue)) {
    user[0].adjustSsdCredit = 1000.0 * (1.0 - MINI_CREDIT_PROPORTION);   
    user[1].adjustSsdCredit = 1000.0 * MINI_CREDIT_PROPORTION;  
    // printf("\n333\n");
    
    return; 
  }

  /*user1 在上一輪沒有任何ssd user request被執行,則保留MINI_CREDIT_PROPORTION的credit給user1*/
  if(user[0].doneSsdUserReqInPeriod == 0) {
    #ifdef NON_WROK_CONSERVING
      // printf("\n444\n");

      // user[0].adjustSsdCredit = user[0].prevSsdCredit;   
      // user[1].adjustSsdCredit = user[1].prevSsdCredit;    
      return;       //依照上一輪分配，不調整ssd credit
    #elif defined WORK_CONSERVING
      user[0].adjustSsdCredit = 1000.0 * (MINI_CREDIT_PROPORTION + 0.03);  
      user[1].adjustSsdCredit = 1000.0 * (1.0 - (MINI_CREDIT_PROPORTION + 0.03)); 
      return; 
    #endif
  }
  /*user2 在上一輪沒有任何ssd user request被執行,則保留MINI_CREDIT_PROPORTION的credit給user2*/
  if(user[1].doneSsdUserReqInPeriod == 0) {
    #ifdef NON_WROK_CONSERVING
      // printf("\n555\n");

      // user[0].adjustSsdCredit = user[0].prevSsdCredit;   
      // user[1].adjustSsdCredit = user[1].prevSsdCredit;    
      return;       //依照上一輪分配，不調整ssd credit
    #elif defined WORK_CONSERVING 
      user[0].adjustSsdCredit = 1000.0 * (1.0 - (MINI_CREDIT_PROPORTION + 0.03));   
      user[1].adjustSsdCredit = 1000.0 * (MINI_CREDIT_PROPORTION + 0.03);  
      return;
    #endif
  }
  
  /*開始調整SSD credit*/
  double det, proprotionU1, proprotionU2, weightProprotionU1, weightProprotionU2;
  proprotionU1 = (double)user[0].doneSsdUserReqInPeriod / (double)(user[0].doneSsdUserReqInPeriod + user[0].doneSsdSysReqInPeriod);
  proprotionU2 = (double)user[1].doneSsdUserReqInPeriod / (double)(user[1].doneSsdUserReqInPeriod + user[1].doneSsdSysReqInPeriod);
  
  weightProprotionU1 = (double)user[1].globalWeight * proprotionU1;
  weightProprotionU2 = -(double)user[0].globalWeight * proprotionU2;
  
  det = weightProprotionU2 - weightProprotionU1;
  user[0].adjustSsdCredit = 1000.0 * weightProprotionU2 / det;   //user1調整過後的ssd credit
  user[1].adjustSsdCredit = -1000.0 * weightProprotionU1 / det;  //user2調整過後的ssd credit
  
  if(user[0].adjustSsdCredit == 0) {
    user[0].adjustSsdCredit = user[0].prevSsdCredit;
    user[1].adjustSsdCredit = user[1].prevSsdCredit;
    // printf("\n666\n");
  }
    // printf("\n777\n");

}

/**
 * [在dynamic credit policy中，每經過一個adjust period，就會調整hdd credit(使用克拉瑪公式解(目前只能解兩位user))]
 * @param {userInfo *} user [users' information]
 */
void hdd_credit_adjust(userInfo *user) {
  char c;
  /*若所有user hdd queue內都沒有request，則返回(代表credit的值會和上一輪調整的一樣)*/
  if(are_all_user_hdd_queue_empty(user)) {
    // user[0].adjustHddCredit = user[0].prevHddCredit;   
    // user[1].adjustHddCredit = user[1].prevHddCredit; 
    // printf(COLOR_GB"\n111\n"COLOR_RESET);   
    // c = getchar();
    return;
  }
  /*user1沒有做任何hdd request(user or system request)，且user1 hdd queue內也沒有任何request，代表此user1很有可能已經做完所有request*/
  if(user[0].doneHddUserReqInPeriod == 0 && user[0].doneHddSysReqInPeriod == 0 && is_empty_queue(user[0].hddQueue)) {
    user[0].adjustHddCredit = 1000.0 * MINI_CREDIT_PROPORTION;  
    user[1].adjustHddCredit = 1000.0 * (1.0 - MINI_CREDIT_PROPORTION);
    // printf(COLOR_GB"\n222\n"COLOR_RESET);       
    // c = getchar();
    return; 
  }

  /*user2沒有做任何hdd request(user or system request)，且user2 hdd queue內也沒有任何request，代表此user2很有可能已經做完所有request*/
  if(user[1].doneHddUserReqInPeriod == 0 && user[1].doneHddSysReqInPeriod == 0 && is_empty_queue(user[1].hddQueue)) {
    user[0].adjustHddCredit = 1000.0 * (1.0 - MINI_CREDIT_PROPORTION);   
    user[1].adjustHddCredit = 1000.0 * MINI_CREDIT_PROPORTION;  
    // printf(COLOR_GB"\n333\n"COLOR_RESET);       
    // c = getchar();
    
    return; 
  }

  /*user1 在上一輪沒有任何hdd user request被執行,則保留MINI_CREDIT_PROPORTION的credit給user1*/
  if(user[0].doneHddUserReqInPeriod == 0) {
    #ifdef NON_WROK_CONSERVING
      // user[0].adjustHddCredit = user[0].prevHddCredit;   
      // user[1].adjustHddCredit = user[1].prevHddCredit; 
      // printf(COLOR_GB"\n444\n"COLOR_RESET);       
      // c = getchar();
      
      return;       //依照上一輪分配，不調整hdd credit
    #elif defined WORK_CONSERVING
      // printf(COLOR_GB"\n555\n"COLOR_RESET);       
      // c = getchar();
      
      user[0].adjustHddCredit = 1000.0 * (MINI_CREDIT_PROPORTION + 0.03);  
      user[1].adjustHddCredit = 1000.0 * (1.0 - (MINI_CREDIT_PROPORTION + 0.03)); 
      return; 
    #endif
  }
  /*user2 在上一輪沒有任何hdd user request被執行,則保留MINI_CREDIT_PROPORTION的credit給user2*/
  if(user[1].doneHddUserReqInPeriod == 0) {
    
    #ifdef NON_WROK_CONSERVING
      // user[0].adjustHddCredit = user[0].prevHddCredit;   
      // user[1].adjustHddCredit = user[1].prevHddCredit; 
      // printf(COLOR_GB"\n666\n"COLOR_RESET);       
      // c = getchar();
      
      return;       //依照上一輪分配，不調整hdd credit
    #elif defined WORK_CONSERVING 
      // printf(COLOR_GB"\n777\n"COLOR_RESET);       
      // c = getchar();
      
      user[0].adjustHddCredit = 1000.0 * (1.0 - (MINI_CREDIT_PROPORTION + 0.03));   
      user[1].adjustHddCredit = 1000.0 * (MINI_CREDIT_PROPORTION + 0.03);  
      return;
    #endif
  }
  
  /*開始調整HDD credit*/
  double det, proprotionU1, proprotionU2, weightProprotionU1, weightProprotionU2;
  proprotionU1 = (double)user[0].doneHddUserReqInPeriod / (double)(user[0].doneHddUserReqInPeriod + user[0].doneHddSysReqInPeriod);
  proprotionU2 = (double)user[1].doneHddUserReqInPeriod / (double)(user[1].doneHddUserReqInPeriod + user[1].doneHddSysReqInPeriod);
  
  weightProprotionU1 = (double)user[1].globalWeight * proprotionU1;
  weightProprotionU2 = -(double)user[0].globalWeight * proprotionU2;
  
  det = weightProprotionU2 - weightProprotionU1;
  user[0].adjustHddCredit = 1000.0 * weightProprotionU2 / det;   //user1調整過後的hdd credit
  user[1].adjustHddCredit = -1000.0 * weightProprotionU1 / det;  //user2調整過後的hdd credit

  if(user[0].adjustHddCredit == 0) {
    // printf(COLOR_GB"\n888\n"COLOR_RESET);       
    // c = getchar();
    
    user[0].adjustHddCredit = user[0].prevHddCredit;
    user[1].adjustHddCredit = user[1].prevHddCredit;
  }
  // printf("\nC1: %f\tC2: %f\n",user[0].adjustHddCredit, user[1].adjustHddCredit);
  // printf(COLOR_GB"\n999\n"COLOR_RESET);       
  // c = getchar();
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
  statistics_done_func(sys, user, tmp, "SSD", systemTime);

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
  statistics_done_func(sys, user, tmp, "HDD", systemTime);
  
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

