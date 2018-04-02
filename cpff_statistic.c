#include "cpff_statistic.h"


/*[統計request完成的結果]*/
void statistics_done_func(systemInfo *sys, userInfo *user, REQ *r, char *reqType) {
  if(!strcmp("SSD", reqType)) {     //ssd request
    if(r->isSystemRequest) {      //ssd system request
      sys->doneSsdSysReq++;
      sys->doneSsdSysReqInSecond++;
      sys->doneSsdSysReqInPeriod++;
      user[r->userno-1].doneSsdSysReq++;
      user[r->userno-1].doneSsdSysReqInSecond++;
      user[r->userno-1].doneSsdSysReqInPeriod++;
      sys->sysSsdReqResTime += r->responseTime;
      sys->sysSsdReqResTimeInSecond += r->responseTime;
      sys->sysSsdReqResTimeInPeriod += r->responseTime;
      user[r->userno-1].sysSsdReqResTime += r->responseTime;
      user[r->userno-1].sysSsdReqResTimeInSecond += r->responseTime;
      user[r->userno-1].sysSsdReqResTimeInPeriod += r->responseTime;

      if(r->isSystemRequest == 1) {       //this request is ssd system write request
        sys->missCount++;
        sys->missCountInSecond++;
        sys->missCountInPeriod++;
        user[r->userno-1].missCount++;
        user[r->userno-1].missCountInSecond++;
        user[r->userno-1].missCountInPeriod++;
      } else if(r->isSystemRequest == 2) {    //this request is ssd system read request
        sys->evictCount++;
        sys->evictCountInSecond++;
        sys->evictCountInPeriod++;
        user[r->userno-1].evictCount++;
        user[r->userno-1].evictCountInSecond++;
        user[r->userno-1].evictCountInPeriod++;
      }

      return;
    } else {    //ssd user request
      sys->doneSsdUserReq++;
      sys->doneSsdUserReqInSecond++;
      sys->doneSsdUserReqInPeriod++;
      user[r->userno-1].doneSsdUserReq++;
      user[r->userno-1].doneSsdUserReqInSecond++;
      user[r->userno-1].doneSsdUserReqInPeriod++;
      sys->userSsdReqResTime += r->responseTime;
      sys->userSsdReqResTimeInSecond += r->responseTime;
      sys->userSsdReqResTimeInPeriod += r->responseTime;
      user[r->userno-1].userSsdReqResTime += r->responseTime;
      user[r->userno-1].userSsdReqResTimeInSecond += r->responseTime;
      user[r->userno-1].userSsdReqResTimeInPeriod += r->responseTime;

      sys->hitCount++;
      sys->hitCountInSecond++;
      sys->hitCountInPeriod++;
      user[r->userno-1].hitCount++;
      user[r->userno-1].hitCountInSecond++;
      user[r->userno-1].hitCountInPeriod++;

      return;
    }
  } else {
    if(r->isSystemRequest) {      //hdd system request
      sys->doneHddSysReq++;
      sys->doneHddSysReqInSecond++;
      sys->doneHddSysReqInPeriod++;
      user[r->userno-1].doneHddSysReq++;
      user[r->userno-1].doneHddSysReqInSecond++;
      user[r->userno-1].doneHddSysReqInPeriod++;
      sys->sysHddReqResTime += r->responseTime;
      sys->sysHddReqResTimeInSecond += r->responseTime;
      sys->sysHddReqResTimeInPeriod += r->responseTime;
      user[r->userno-1].sysHddReqResTime += r->responseTime;
      user[r->userno-1].sysHddReqResTimeInSecond += r->responseTime;
      user[r->userno-1].sysHddReqResTimeInPeriod += r->responseTime;
      return;
    } else {    //hdd user request
      sys->doneHddUserReq++;
      sys->doneHddUserReqInSecond++;
      sys->doneHddUserReqInPeriod++;
      user[r->userno-1].doneHddUserReq++;
      user[r->userno-1].doneHddUserReqInSecond++;
      user[r->userno-1].doneHddUserReqInPeriod++;
      sys->userHddReqResTime += r->responseTime;
      sys->userHddReqResTimeInSecond += r->responseTime;
      sys->userHddReqResTimeInPeriod += r->responseTime;
      user[r->userno-1].userHddReqResTime += r->responseTime;
      user[r->userno-1].userHddReqResTimeInSecond += r->responseTime;
      user[r->userno-1].userHddReqResTimeInPeriod += r->responseTime;
      return;
    }
  }
}


/*[This function is used to reset second value every second]*/
void reset_second_value(systemInfo *sysInfo, userInfo *user) {
  /*Reset system's second value*/
  sysInfo->userReadReqInSecond = 0;
  sysInfo->userWriteReqInSecond = 0;
  sysInfo->sysSsdReadReqInSecond = 0;
  sysInfo->sysSsdWriteReqInSecond = 0;
  sysInfo->sysHddWriteReqInSecond = 0;
  sysInfo->evictCountInSecond = 0;
  sysInfo->dirtyCountInSecond = 0;
  sysInfo->hitCountInSecond = 0;
  sysInfo->missCountInSecond = 0;
  sysInfo->doneSsdSysReqInSecond = 0;
  sysInfo->doneHddSysReqInSecond = 0;
  sysInfo->doneSsdUserReqInSecond = 0;
  sysInfo->doneHddUserReqInSecond = 0;
  sysInfo->userSsdReqResTimeInSecond = 0.0;
  sysInfo->userHddReqResTimeInSecond = 0.0;
  sysInfo->sysSsdReqResTimeInSecond = 0.0;
  sysInfo->sysHddReqResTimeInSecond = 0.0;

  /*Reset each user's second value*/
  int i;
  for(i = 0; i < NUM_OF_USER; i++) {
    user[i].userReadReqInSecond = 0;
    user[i].userWriteReqInSecond = 0;
    user[i].sysSsdReadReqInSecond = 0;
    user[i].sysSsdWriteReqInSecond = 0;
    user[i].sysHddWriteReqInSecond = 0;
    user[i].evictCountInSecond = 0;
    user[i].dirtyCountInSecond = 0;
    user[i].hitCountInSecond = 0;
    user[i].missCountInSecond = 0;
    user[i].doneSsdSysReqInSecond = 0;
    user[i].doneHddSysReqInSecond = 0;
    user[i].doneSsdUserReqInSecond = 0;
    user[i].doneHddUserReqInSecond = 0;
    user[i].userSsdReqResTimeInSecond = 0.0;
    user[i].userHddReqResTimeInSecond = 0.0;
    user[i].sysSsdReqResTimeInSecond = 0.0;
    user[i].sysHddReqResTimeInSecond = 0.0;
  }
}

/*[This function is used to reset periods value every period]*/
void reset_period_value(systemInfo *sysInfo, userInfo *user) {
  /*Reset system's period value*/
  sysInfo->userReadReqInPeriod = 0;
  sysInfo->userWriteReqInPeriod = 0;
  sysInfo->sysSsdReadReqInPeriod = 0;
  sysInfo->sysSsdWriteReqInPeriod = 0;
  sysInfo->sysHddWriteReqInPeriod = 0;
  sysInfo->evictCountInPeriod = 0;
  sysInfo->dirtyCountInPeriod = 0;
  sysInfo->hitCountInPeriod = 0;
  sysInfo->missCountInPeriod = 0;
  sysInfo->doneSsdSysReqInPeriod = 0;
  sysInfo->doneHddSysReqInPeriod = 0;
  sysInfo->doneSsdUserReqInPeriod = 0;
  sysInfo->doneHddUserReqInPeriod = 0;
  sysInfo->userSsdReqResTimeInPeriod = 0.0;
  sysInfo->userHddReqResTimeInPeriod = 0.0;
  sysInfo->sysSsdReqResTimeInPeriod = 0.0;
  sysInfo->sysHddReqResTimeInPeriod = 0.0;

  /*Reset each user's period value*/
  int i;
  for(i = 0; i < NUM_OF_USER; i++) {
    user[i].userReadReqInPeriod = 0;
    user[i].userWriteReqInPeriod = 0;
    user[i].sysSsdReadReqInPeriod = 0;
    user[i].sysSsdWriteReqInPeriod = 0;
    user[i].sysHddWriteReqInPeriod = 0;
    user[i].evictCountInPeriod = 0;
    user[i].dirtyCountInPeriod = 0;
    user[i].hitCountInPeriod = 0;
    user[i].missCountInPeriod = 0;
    user[i].doneSsdSysReqInPeriod = 0;
    user[i].doneHddSysReqInPeriod = 0;
    user[i].doneSsdUserReqInPeriod = 0;
    user[i].doneHddUserReqInPeriod = 0;
    user[i].userSsdReqResTimeInPeriod = 0.0;
    user[i].userHddReqResTimeInPeriod = 0.0;
    user[i].sysSsdReqResTimeInPeriod = 0.0;
    user[i].sysHddReqResTimeInPeriod = 0.0;
  }
}


/*[每second就會記錄詳細資訊]*/
void second_record_statistics(systemInfo *sysInfo, userInfo *user, double systemTime, FILE **secondStatisticRecord) {
  if(sysInfo != NULL) {
    
    fprintf(*secondStatisticRecord, "\n-----------------------------\n");
    fprintf(*secondStatisticRecord, "CPFF system time: %f\n", systemTime);
    fprintf(*secondStatisticRecord, "\nSystem Info\n");
    fprintf(*secondStatisticRecord, "All user requests in second, Read: %lu, Write: %lu\n", sysInfo->userReadReqInSecond, sysInfo->userWriteReqInSecond);
    fprintf(*secondStatisticRecord, "All system requests in second, SSD Read: %lu, SSD Write: %lu, HDD Write: %lu\n", sysInfo->sysSsdReadReqInSecond, sysInfo->sysSsdWriteReqInSecond, sysInfo->sysHddWriteReqInSecond);
    fprintf(*secondStatisticRecord, "All evictCountInSecond: %lu, dirtyCountInSecond: %lu, hitCountInSecond: %lu, missCountInSecond: %lu\n", sysInfo->evictCountInSecond, sysInfo->dirtyCountInSecond, sysInfo->hitCountInSecond, sysInfo->missCountInSecond);
    fprintf(*secondStatisticRecord, "All doneSsdSysReqInSecond: %lu, doneHddSysReqInSecond: %lu, doneSsdUserReqInSecond: %lu, doneHddUserReqInSecond: %lu\n", sysInfo->doneSsdSysReqInSecond, sysInfo->doneHddSysReqInSecond, sysInfo->doneSsdUserReqInSecond, sysInfo->doneHddUserReqInSecond);
    fprintf(*secondStatisticRecord, "All sysSsdReqResTimeInSecond: %f, sysHddReqResTimeInSecond: %f, userSsdReqResTimeInSecond: %f, userHddReqResTimeInSecond: %f\n", sysInfo->sysSsdReqResTimeInSecond, sysInfo->sysHddReqResTimeInSecond, sysInfo->userSsdReqResTimeInSecond, sysInfo->userHddReqResTimeInSecond);
    
    int i;
    for(i = 0; i < NUM_OF_USER; i++) {
      fprintf(*secondStatisticRecord, "\nUser %d\n", i+1);
      fprintf(*secondStatisticRecord, "user requests in second, Read: %lu, Write: %lu\n", user[i].userReadReqInSecond, user[i].userWriteReqInSecond);
      fprintf(*secondStatisticRecord, "system requests in second, SSD Read: %lu, SSD Write: %lu, HDD Write: %lu\n", user[i].sysSsdReadReqInSecond, user[i].sysSsdWriteReqInSecond, user[i].sysHddWriteReqInSecond);
      fprintf(*secondStatisticRecord, "evictCountInSecond: %lu, dirtyCountInSecond: %lu, hitCountInSecond: %lu, missCountInSecond: %lu\n", user[i].evictCountInSecond, user[i].dirtyCountInSecond, user[i].hitCountInSecond, user[i].missCountInSecond);
      fprintf(*secondStatisticRecord, "doneSsdSysReqInSecond: %lu, doneHddSysReqInSecond: %lu, doneSsdUserReqInSecond: %lu, doneHddUserReqInSecond: %lu\n", user[i].doneSsdSysReqInSecond, user[i].doneHddSysReqInSecond, user[i].doneSsdUserReqInSecond, user[i].doneHddUserReqInSecond);
      fprintf(*secondStatisticRecord, "sysSsdReqResTimeInSecond: %f, sysHddReqResTimeInSecond: %f, userSsdReqResTimeInSecond: %f, userHddReqResTimeInSecond: %f\n", user[i].sysSsdReqResTimeInSecond, user[i].sysHddReqResTimeInSecond, user[i].userSsdReqResTimeInSecond, user[i].userHddReqResTimeInSecond);
    }
    fprintf(*secondStatisticRecord, "-----------------------------\n");
    
    return;
  }
}

/*[每second寫成csv紀錄檔(systemtime, SSD throughput, HDD throughput, SSD avg response time, HDD avg response time, hit rate)]*/
void second_csv_statistics(systemInfo *sysInfo, userInfo *user, double systemTime, FILE **systemSecondRecord, FILE **eachUserSecondRecord) {
  if(sysInfo != NULL) {
    
    double ssdAvgResponse, hddAvgResponse, ssdThroughput, hddThroughput, hitRate;
    ssdThroughput = ((double)sysInfo->doneSsdUserReqInSecond * 4.0 / 1024) / 1.0;
    hddThroughput = ((double)sysInfo->doneHddUserReqInSecond * 4.0 / 1024) / 1.0;
    // ssdAvgResponse = sysInfo->userSsdReqResTimeInSecond / (double)sysInfo->doneSsdUserReqInSecond;
    // hddAvgResponse = sysInfo->userHddReqResTimeInSecond / (double)sysInfo->doneHddUserReqInSecond;
    hitRate = (double)sysInfo->hitCount / ((double)sysInfo->hitCount + (double)sysInfo->missCount);
    fprintf(*systemSecondRecord, "%f,%f,%f,%f\n", systemTime, ssdThroughput, hddThroughput, hitRate);
    
    int i;
    for(i = 0; i < NUM_OF_USER; i++) {
      ssdThroughput = ((double)user[i].doneSsdUserReqInSecond * 4.0 / 1024) / 1.0;
      hddThroughput = ((double)user[i].doneHddUserReqInSecond * 4.0 / 1024) / 1.0;
      // ssdAvgResponse = user[i].userSsdReqResTimeInSecond / (double)user[i].doneSsdUserReqInSecond;
      // hddAvgResponse = user[i].userHddReqResTimeInSecond / (double)user[i].doneHddUserReqInSecond;
      hitRate = (double)user[i].hitCount / ((double)user[i].hitCount + (double)user[i].missCount);
      fprintf(eachUserSecondRecord[i], "%f,%f,%f,%f,%f,%f\n", systemTime, ssdThroughput, hddThroughput, hitRate, user[i].adjustSsdCredit, user[i].adjustHddCredit);
    }
    return;
  }
}

/*[每隔period就會記錄詳細資訊]*/
void period_record_statistics(systemInfo *sysInfo, userInfo *user, double systemTime, FILE **periodStatisticRecord) {
  if(sysInfo != NULL) {
    
    fprintf(*periodStatisticRecord, "\n-----------------------------\n");
    fprintf(*periodStatisticRecord, "CPFF system time: %f\n", systemTime);
    fprintf(*periodStatisticRecord, "\nSystem Info\n");
    fprintf(*periodStatisticRecord, "All user requests in period, Read: %lu, Write: %lu\n", sysInfo->userReadReqInPeriod, sysInfo->userWriteReqInPeriod);
    fprintf(*periodStatisticRecord, "All system requests in period, SSD Read: %lu, SSD Write: %lu, HDD Write: %lu\n", sysInfo->sysSsdReadReqInPeriod, sysInfo->sysSsdWriteReqInPeriod, sysInfo->sysHddWriteReqInPeriod);
    fprintf(*periodStatisticRecord, "All evictCountInPeriod: %lu, dirtyCountInPeriod: %lu, hitCountInPeriod: %lu, missCountInPeriod: %lu\n", sysInfo->evictCountInPeriod, sysInfo->dirtyCountInPeriod, sysInfo->hitCountInPeriod, sysInfo->missCountInPeriod);
    fprintf(*periodStatisticRecord, "All doneSsdSysReqInPeriod: %lu, doneHddSysReqInPeriod: %lu, doneSsdUserReqInPeriod: %lu, doneHddUserReqInPeriod: %lu\n", sysInfo->doneSsdSysReqInPeriod, sysInfo->doneHddSysReqInPeriod, sysInfo->doneSsdUserReqInPeriod, sysInfo->doneHddUserReqInPeriod);
    fprintf(*periodStatisticRecord, "All sysSsdReqResTimeInPeriod: %f, sysHddReqResTimeInPeriod: %f, userSsdReqResTimeInPeriod: %f, userHddReqResTimeInPeriod: %f\n", sysInfo->sysSsdReqResTimeInPeriod, sysInfo->sysHddReqResTimeInPeriod, sysInfo->userSsdReqResTimeInPeriod, sysInfo->userHddReqResTimeInPeriod);
    
    int i;
    for(i = 0; i < NUM_OF_USER; i++) {
      fprintf(*periodStatisticRecord, "\nUser %d\n", i+1);
      fprintf(*periodStatisticRecord, "user requests in period, Read: %lu, Write: %lu\n", user[i].userReadReqInPeriod, user[i].userWriteReqInPeriod);
      fprintf(*periodStatisticRecord, "system requests in period, SSD Read: %lu, SSD Write: %lu, HDD Write: %lu\n", user[i].sysSsdReadReqInPeriod, user[i].sysSsdWriteReqInPeriod, user[i].sysHddWriteReqInPeriod);
      fprintf(*periodStatisticRecord, "evictCountInPeriod: %lu, dirtyCountInPeriod: %lu, hitCountInPeriod: %lu, missCountInPeriod: %lu\n", user[i].evictCountInPeriod, user[i].dirtyCountInPeriod, user[i].hitCountInPeriod, user[i].missCountInPeriod);
      fprintf(*periodStatisticRecord, "doneSsdSysReqInPeriod: %lu, doneHddSysReqInPeriod: %lu, doneSsdUserReqInPeriod: %lu, doneHddUserReqInPeriod: %lu\n", user[i].doneSsdSysReqInPeriod, user[i].doneHddSysReqInPeriod, user[i].doneSsdUserReqInPeriod, user[i].doneHddUserReqInPeriod);
      fprintf(*periodStatisticRecord, "sysSsdReqResTimeInPeriod: %f, sysHddReqResTimeInPeriod: %f, userSsdReqResTimeInPeriod: %f, userHddReqResTimeInPeriod: %f\n", user[i].sysSsdReqResTimeInPeriod, user[i].sysHddReqResTimeInPeriod, user[i].userSsdReqResTimeInPeriod, user[i].userHddReqResTimeInPeriod);
    }

    fprintf(*periodStatisticRecord, "-----------------------------\n");
    
    return;
  }
}

/*[每隔period寫成csv紀錄檔(systemtime, SSD throughput, HDD throughput, SSD avg response time, HDD avg response time, hit rate)]*/
void period_csv_statistics(systemInfo *sysInfo, userInfo *user, double systemTime, FILE **systemPeriodRecord, FILE **eachUserPeriodRecord) {
  if(sysInfo != NULL) {
    
    double ssdAvgResponse, hddAvgResponse, ssdThroughput, hddThroughput, hitRate;
    ssdThroughput = ((double)sysInfo->doneSsdUserReqInPeriod * 4.0 / 1024.0) / ((double)STAT_FOR_TIME_PERIODS * TIME_PERIOD / 1000.0);
    hddThroughput = ((double)sysInfo->doneHddUserReqInPeriod * 4.0 / 1024.0) / ((double)STAT_FOR_TIME_PERIODS * TIME_PERIOD / 1000.0);
    // ssdAvgResponse = sysInfo->userSsdReqResTimeInPeriod / (double)sysInfo->doneSsdUserReqInPeriod;
    // hddAvgResponse = sysInfo->userHddReqResTimeInPeriod / (double)sysInfo->doneHddUserReqInPeriod;
    hitRate = (double)sysInfo->hitCount / ((double)sysInfo->hitCount + (double)sysInfo->missCount);
    fprintf(*systemPeriodRecord, "%f,%f,%f,%f\n", systemTime, ssdThroughput, hddThroughput, hitRate);
    
    int i;
    for(i = 0; i < NUM_OF_USER; i++) {
      ssdThroughput = ((double)user[i].doneSsdUserReqInPeriod * 4.0 / 1024.0) / ((double)STAT_FOR_TIME_PERIODS * TIME_PERIOD / 1000.0);
      hddThroughput = ((double)user[i].doneHddUserReqInPeriod * 4.0 / 1024.0) / ((double)STAT_FOR_TIME_PERIODS * TIME_PERIOD / 1000.0);
      // ssdAvgResponse = user[i].userSsdReqResTimeInPeriod / (double)user[i].doneSsdUserReqInPeriod;
      // hddAvgResponse = user[i].userHddReqResTimeInPeriod / (double)user[i].doneHddUserReqInPeriod;
      hitRate = (double)user[i].hitCount / ((double)user[i].hitCount + (double)user[i].missCount);
      fprintf(eachUserPeriodRecord[i], "%f,%f,%f,%f,%f,%f\n", systemTime, ssdThroughput, hddThroughput, hitRate, user[i].adjustSsdCredit, user[i].adjustHddCredit);
    }
    return;
  }
}


void final_result_statistics(systemInfo *sysInfo, userInfo *user, FILE **finalResult) {
  if(sysInfo != NULL) {
    
    fprintf(*finalResult, "\n-----------------------------\n");
    fprintf(*finalResult, "\nSystem Info\n");
    fprintf(*finalResult, "All user requests, Read: %lu, Write: %lu\n", sysInfo->userReadReq, sysInfo->userWriteReq);
    fprintf(*finalResult, "All system requests, SSD Read: %lu, SSD Write: %lu, HDD Write: %lu\n", sysInfo->sysSsdReadReq, sysInfo->sysSsdWriteReq, sysInfo->sysHddWriteReq);
    fprintf(*finalResult, "All evictCount: %lu, dirtyCount: %lu, hitCount: %lu, missCount: %lu\n", sysInfo->evictCount, sysInfo->dirtyCount, sysInfo->hitCount, sysInfo->missCount);
    fprintf(*finalResult, "All doneSsdSysReq: %lu, doneHddSysReq: %lu, doneSsdUserReq: %lu, doneHddUserReq: %lu\n", sysInfo->doneSsdSysReq, sysInfo->doneHddSysReq, sysInfo->doneSsdUserReq, sysInfo->doneHddUserReq);
    fprintf(*finalResult, "All sysSsdReqResTime: %f, sysHddReqResTime: %f, userSsdReqResTime: %f, userHddReqResTime: %f\n", sysInfo->sysSsdReqResTime, sysInfo->sysHddReqResTime, sysInfo->userSsdReqResTime, sysInfo->userHddReqResTime);
    
    int i;
    for(i = 0; i < NUM_OF_USER; i++) {
      fprintf(*finalResult, "\nUser %d\n", i+1);
      fprintf(*finalResult, "user requests, Read: %lu, Write: %lu\n", user[i].userReadReq, user[i].userWriteReq);
      fprintf(*finalResult, "system requests, SSD Read: %lu, SSD Write: %lu, HDD Write: %lu\n", user[i].sysSsdReadReq, user[i].sysSsdWriteReq, user[i].sysHddWriteReq);
      fprintf(*finalResult, "evictCount: %lu, dirtyCount: %lu, hitCount: %lu, missCount: %lu\n", user[i].evictCount, user[i].dirtyCount, user[i].hitCount, user[i].missCount);
      fprintf(*finalResult, "doneSsdSysReq: %lu, doneHddSysReq: %lu, doneSsdUserReq: %lu, doneHddUserReq: %lu\n", user[i].doneSsdSysReq, user[i].doneHddSysReq, user[i].doneSsdUserReq, user[i].doneHddUserReq);
      fprintf(*finalResult, "sysSsdReqResTime: %f, sysHddReqResTime: %f, userSsdReqResTime: %f, userHddReqResTime: %f\n", user[i].sysSsdReqResTime, user[i].sysHddReqResTime, user[i].userSsdReqResTime, user[i].userHddReqResTime);
    }

    fprintf(*finalResult, "-----------------------------\n");
    
    return;
  }
}