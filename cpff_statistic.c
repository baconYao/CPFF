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