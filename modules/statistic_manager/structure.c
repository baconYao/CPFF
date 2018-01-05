#include "structure.h"

/*
 * [初始化User Info]
 */
void init_user_info() {
    sysst.totalReq = 0;
    sysst.ssdReq = 0;
    sysst.totalUserReq = 0;
    sysst.UserReqInPeriod = 0;
    sysst.UserRReq = 0;
    sysst.totalSysReq = 0;
    sysst.evictCount = 0;
    sysst.dirtyCount = 0;
    sysst.hitCount = 0;
    sysst.missCount = 0;
    sysst.resTime = 0;
    sysst.resTimeInPeriod = 0;
    sysst.cachingSpace = 0;
    unsigned long i;
    for (i = 0; i < NUM_OF_USER; i++) {
        userInfoArr[i].globalWeight = 0;
        userInfoArr[i].credit = 0;
        userInfoArr[i].totalReq = 0;
        userInfoArr[i].ssdReq = 0;
        userInfoArr[i].totalUserReq = 0;
        userInfoArr[i].UserReqInPeriod = 0;
        userInfoArr[i].UserRReq = 0;
        userInfoArr[i].totalSysReq = 0;
        userInfoArr[i].evictCount = 0;
        userInfoArr[i].dirtyCount = 0;
        userInfoArr[i].hitCount = 0;
        userInfoArr[i].missCount = 0;
        userInfoArr[i].resTime = 0;
        userInfoArr[i].resTimeInPeriod = 0;
        userInfoArr[i].cachingSpace = 0;
    }
}