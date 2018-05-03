#include "cpff_dynamic_caching_space.h"
/**
 * 此檔案為維護本系統之Caching機制，根據Hybrid Storage System設定，將SSD視為HDD之Cache，
 * 因此，Caching Table須同時記錄Data(Block)位於SSD的位址(ssd_blk)和HDD的位址(diskBlk)，以
 * 以確保Caching機制的正確性。
 */

/*****With ssd logical partition*****/

/**
 * [針對Trace指定不同User權重所占用的空間]
 * @return {int} 0/-1 []
 */
int init_user_cache(userInfo *user, int  totalWeight) {
  totalWeight = 0;
  unsigned i;
  for (i = 0; i < NUM_OF_USER; i++) {
    totalWeight += user[i].globalWeight;
  }

  if (totalWeight == 0){
    print_error(0, "[dynamic_caching_space.c] Total User Weight = ");
  }

  #ifdef DYNAMIC_CACHING_SPACE
  for (i = 0; i < NUM_OF_USER; i++) {
    userCacheCount[i] = 0;
    cache_list[i].size = 0;
    cache_list[i].head = cache_list[i].tail = NULL;
    userCacheSize[i] = user[i].globalWeight*SSD_CACHING_SPACE_BY_PAGES/totalWeight;
    userFreeCount[i] = userCacheSize[i];
    user[i].pageNumberInCache = userCacheSize[i];
  }

  printf(" [dynamic_caching_space.c] Total User Weight:%u, Total Cache Size(Pages):%u\n", 2, SSD_CACHING_SPACE_BY_PAGES);

  for (i = 0; i < NUM_OF_USER; i++) {
    printf(" [dynamic_caching_space.c] User%u: Weight:%u , Size(Pages):%lu\n", i+1, 1, userCacheSize[i]);
  }
  #else
   print_error(0, "[dynamic_caching_space.c] You should define DYNAMIC_CACHING_SPACE at parameter.h file ");
  #endif

  for (i = 0; i < SSD_CACHING_SPACE_BY_PAGES; i++) {
    ssdCache[i].diskBlkno = 0;
    ssdCache[i].user = 0;
    ssdCache[i].pageno = i;
    ssdCache[i].prev = ssdCache[i].next = NULL;
  }
  initialize_ghost_cache();
  return 0;
}

/**
 * [針對指定User插入(新增)Caching Block至 User Caching Table]
 * @param {unsigned long} diskBlk [Block Number in HDD]
 * @param {int} reqFlag [此次Caching是否修改Caching Block(參考PAGE_FLAG_XXXX)]
 * @param {unsigned} userno [User number(1-n)]
 * @param {double} time [Cache access time]
 * @param {metaBlock *} meta [PC meta pointer]
 * @param {userInfo *} user [User pointer]
 * @return {int} 0/-1 [FULL(-1) or not(0)]
 */
SSD_CACHE *insert_cache_by_user(unsigned long diskBlk, int reqFlag, unsigned userno, double time, struct metaBlock *meta, userInfo *user) {
  unsigned unum = userno;
  
  SSD_CACHE *search;
  search = search_cache_by_user(diskBlk, unum);

  //新增一筆資料
  if (search == NULL) {
    unsigned long freePage;
    if (is_full_cache_by_user(unum)){
      return NULL;//FULL
    }
    else {
      freePage = get_free_cache_by_user(unum);
    }

    /*賦予page資料*/
    ssdCache[freePage].diskBlkno = diskBlk;
    ssdCache[freePage].dirtyFlag = reqFlag;
    ssdCache[freePage].freeFlag = PAGE_FLAG_NOT_FREE;
    ssdCache[freePage].user = userno;
    ssdCache[freePage].accessTime = time;
    ssdCache[freePage].pcMeta = meta;

    /*紀錄此user cache狀態*/
    userFreeCount[unum-1]--;
    userCacheCount[userno-1]++;

    /*加入cache list的MRU端*/ 
    if(cache_list[unum-1].size == 0) {      //此request是此user的第一筆資料
      cache_list[unum-1].head = cache_list[unum-1].tail = &ssdCache[freePage];      //將cache list的head&tail指向此page
      cache_list[unum-1].size++;
      ssdCache[freePage].prev = ssdCache[freePage].next = NULL;       //將page的prev&next指向NULL
    } else {
      cache_list[unum-1].head->prev = &ssdCache[freePage];
      ssdCache[freePage].next = cache_list[unum-1].head;
      ssdCache[freePage].prev = NULL;
      cache_list[unum-1].head = &ssdCache[freePage];
      cache_list[unum-1].size++;
    }

    return &ssdCache[freePage];
  }
  else {//更新一筆資料
    //更新此次存取的Block flag
    switch (reqFlag) {
      case PAGE_FLAG_CLEAN:
        //Dirty still dirty...
        break;
      case PAGE_FLAG_DIRTY:
        ssdCache[search->pageno].dirtyFlag = reqFlag;
        break;
      default:
        print_error(reqFlag, "[USER CACHE]Caching Error with unknown flag:");
        break;
    }

    ssdCache[search->pageno].accessTime = time;

    /*移動至cache list的MRU端*/
    if(cache_list[unum-1].head->pageno == search->pageno) {      //若此筆資料在cache list的head端，故不需移動
      //do nothing
    } else if(cache_list[unum-1].tail->pageno == search->pageno) {    //若此筆資料在cache list的tail端
      SSD_CACHE *tmp;
      tmp = cache_list[unum-1].tail;

      tmp->next = cache_list[unum-1].head;
      cache_list[unum-1].head->prev = tmp;
      cache_list[unum-1].tail = cache_list[unum-1].tail->prev;
      cache_list[unum-1].tail->next = NULL;
      tmp->prev = NULL;
      cache_list[unum-1].head = tmp;
    } else {            //若此筆資料在cache list的中間
      SSD_CACHE *tmp;
      tmp = &ssdCache[search->pageno];

      tmp->prev->next = tmp->next;
      tmp->next->prev = tmp->prev;
      tmp->prev = NULL;
      tmp->next =  cache_list[unum-1].head;
      cache_list[unum-1].head->prev = tmp;
      cache_list[unum-1].head = tmp;
    }
    
    return &ssdCache[search->pageno];
  }
}

/*CACHE EVICTION POLICY:SPECIFIC Block with min prize and User*/
/**
 * [根據指定的HDD Block Number和User Number進行剔除]
 * @param {double} minPrize [指定欲剔除的page是具有最小Prize的metadata Block]
 * @param {unsigned} userno [User number(1-n)]
 * @param {userInfo *} user [User pointer]
 * @return {SSD_CACHE*} &ssdCache/NULL [回傳欲剔除的cached page;NULL:未找到]
 */
SSD_CACHE *evict_cache_from_LRU_with_min_prize_by_user(double minPrize, unsigned userno, userInfo *user) {
  unsigned unum = userno;
  
  // METABLOCK *min_meta = NULL;
  unsigned long minPriPageno;       //要剔除的page number
  int state=0;//Found:1
  double min;

  SSD_CACHE *tmp;
  tmp = cache_list[unum-1].tail;      //將起始點指定為cache list的tail(LRU端)
  min = minPrize;

  while(tmp != NULL) {
    if(min == tmp->pcMeta->prize) {       //找到要剔除的page
      minPriPageno = tmp->pageno;
      state = 1;
      // printf("\nVictim page: %lu, blk: %lu\n", minPriPageno, tmp->diskBlkno);
      break;
    }
    tmp = tmp->prev;
  }

  //printf("finish evicting...\n");
  if (state == 0) {
    return NULL;
  }
  else {
    if(cache_list[unum-1].head->pageno == minPriPageno) {            //若剔除的page在cache list的head
      tmp = cache_list[unum-1].head;
      cache_list[unum-1].head = cache_list[unum-1].head->next;
      cache_list[unum-1].head->prev = NULL;
      tmp->next = NULL;
    } else if(cache_list[unum-1].tail->pageno == minPriPageno) {     //若剔除的page在cache list的tail
      tmp = cache_list[unum-1].tail;
      cache_list[unum-1].tail = cache_list[unum-1].tail->prev;
      cache_list[unum-1].tail->next = NULL;
      tmp->prev = NULL;
    } else {
      tmp = &ssdCache[minPriPageno];
      tmp->prev->next = tmp->next;
      tmp->next->prev = tmp->prev;
      tmp->next = tmp->prev = NULL;
    }

    cache_list[unum-1].size--;
    ssdCache[minPriPageno].freeFlag = PAGE_FLAG_FREE;     //將剔除的page flag更改為free
    userFreeCount[unum-1]++;
    userCacheCount[ssdCache[minPriPageno].user-1]--;
    return &ssdCache[minPriPageno];
  }
}

/*SEARCH CACHE BY USER*/
/**
 * [指定HDD Block(disksim格式)和User進行搜尋]
 * @param {unsigned long} diskBlk [指定欲搜尋的Block(disksim格式)]
 * @param {unsigned} userno [User number(1-n)]
 * @return {SSD_CACHE*} &ssdCache/NULL [回傳欲搜尋的cached page;NULL:未找到]
 */
SSD_CACHE *search_cache_by_user(unsigned long diskBlk, unsigned userno) {
  unsigned unum = userno;
  SSD_CACHE *tmp;
  tmp = cache_list[unum-1].head;

  while(tmp != NULL) {
    if(tmp->diskBlkno == diskBlk && tmp->freeFlag == PAGE_FLAG_NOT_FREE) {
      return tmp;
    }
    tmp = tmp->next;
  }
  
  return NULL;
}


/*CHECK CACHE FULL BY USER*/
/**
 * [根據指定的User檢查 User Cache是否已滿]
 * @param {unsigned} userno [User number(1-n)]
 * @return CACHE_FULL/CACHE_NOT_FULL [Full:1; Not full:0]
 */
int is_full_cache_by_user(unsigned userno) {
  unsigned unum = userno;

  if (userFreeCount[unum-1] == 0)
    return CACHE_FULL;
  else
    return CACHE_NOT_FULL;
}

/*GET FREE CACHE BY USER*/
/**
 * [根據User Number取得其User Caching Table中一個Free Page]
 * [注意!!欲取得Free Page前必須注意已檢查是否已滿? 建議流程isFullCACHE()->getFreeCACHE()]
 * @param {unsigned} userno [User number(1-n)]
 * @return {unsigned long} pageno [Page Number]
 */
unsigned long get_free_cache_by_user(unsigned userno) {
  unsigned unum = userno;
  unsigned long pageno;
  for (pageno = 0; pageno < SSD_CACHING_SPACE_BY_PAGES; pageno++) {
    if (ssdCache[pageno].freeFlag == PAGE_FLAG_FREE) {
      //printf("Free Page number:%lu\n", pageno);
      return pageno;
    }
  }
}

/*GET CACHE COUNT*/
unsigned long get_cache_cnt() {
  unsigned i;
  unsigned long cnt=0;
  for (i = 0; i < NUM_OF_USER; i++) {
    cnt += (userCacheSize[i]-userFreeCount[i]);
  }
  return cnt;
}

/*
 * [調整user的cache size]
 * @param {userInfo *} user [User pointer]
 */
void adjust_user_cache_size(userInfo *user) {

  int i;
  unsigned long newSize[NUM_OF_USER] = {0};     //調整過後的cache size
  
  /*判斷有幾個user是active*/
  int active = 1, actStatus[NUM_OF_USER] = {1};
  for(i = 0; i < NUM_OF_USER; i++) {
    if(user[i].comingRequestCounter == 0) {
      actStatus[i] = 0;
      active = 0;
    }
  }
  
  /*有user不是active*/
  if(!active) {
    /*將不是active的user的cache space指定為預留的page數量*/
    int actUser, inActUser;
    if(actStatus[0] == 0 && actStatus[1] == 0) {      //all users are inactive
      return;         //不調整cache size (維持原上一輪的分配)
    } else if(actStatus[0] == 1 && actStatus[1] == 0) {   //  user 2 is inactive
      newSize[1] = (unsigned long)(SSD_CACHING_SPACE_BY_PAGES * REMAIN_PAGES);
      newSize[0] = SSD_CACHING_SPACE_BY_PAGES - newSize[1];
      return;
    } else if(actStatus[0] == 0 && actStatus[1] == 1) {   //  user 1 is inactive
      newSize[0] = (unsigned long)(SSD_CACHING_SPACE_BY_PAGES * REMAIN_PAGES);
      newSize[1] = SSD_CACHING_SPACE_BY_PAGES - newSize[0];
      return;
    }
  }

  /* 所有user是active, 接著檢查是active的user的差距是否在忍受範圍內 */ 
  int totalWeight = 0;
  double avgHDDResponseTime[NUM_OF_USER] = {0.0};     //在上一個adjust cache period 的平均每個user hdd request的response time
  for(i = 0; i < NUM_OF_USER; i++) {
    avgHDDResponseTime[i] = (user[i].userHddReqResTimeInAdjustCachePeriod + user[i].sysHddReqResTimeInAdjustCachePeriod) / (double)(user[i].doneHddUserReqInAdjustCachePeriod + user[i].doneHddSysReqInAdjustCachePeriod);
  }
  
  /* 先依據當前的cache size所產生的實際的throughput是否match weight, 若match，則不調整cache size */
  double ssdThroughput, hddThroughput, actualThroughput[NUM_OF_USER] = {0.0};
  for(i = 0; i < NUM_OF_USER; i++) {
    ssdThroughput = ((double)user[i].doneSsdUserReqInAdjustCachePeriod * 4.0 / 1024.0) / ((double)ADJUST_CACHE_TIME_PERIODS * TIME_PERIOD / 1000.0);
    hddThroughput = ((double)user[i].doneHddUserReqInAdjustCachePeriod * 4.0 / 1024.0) / ((double)ADJUST_CACHE_TIME_PERIODS * TIME_PERIOD / 1000.0);
    actualThroughput[i] = ssdThroughput + hddThroughput;
  }

  int check = 0, whoNeedIncreaseCache;
  if(actualThroughput[0] >= actualThroughput[1]) {
    if((actualThroughput[0] - actualThroughput[1]) < TOLERANCE_VALUE) {   
      check = 1;
    }
    whoNeedIncreaseCache = 1;       //user2需要增加cache size, user1要減少
  } else if(actualThroughput[1] >= actualThroughput[0]) {
    if((actualThroughput[1] - actualThroughput[0]) < TOLERANCE_VALUE) {   
      check = 1;
    }   
    whoNeedIncreaseCache = 0;       //user1需要增加cache size, user2要減少    
  }

  if(check) {
    return;   //users預估出來的throughput都在忍受範圍內，故不調整cache size
  }


  /* 開始計算要調整的cache size */
  /*
   *  假設user最終Cache的分配是C1, C2  (C1 + C2 == SSD_CACHING_SPACE_BY_PAGES)，
   *  Step1.  算出(預估)在此cache分配下(C1 or C2)的total response time (TRT), 採用avg 10組的方法預估
   *  Step2.  算出斜率
   *  Step3.  處理 TRTW = total response time * Weight 和 TS = total requests * slope
   *  Step4.  取得當前的cache size (CS)和在當前cache size下所累積的hit count (CHT)
   *  Step5.  給定每個user 係數(coefficient) 和 移動算式取得 和 (equal)
   *  Step6.  使用克拉馬公式解二元一次聯立方程式,解後可的新算出來的 cache size
   */

  /* Step1 */
  double TRT[NUM_OF_USER] = {0.0};    //預估在target cache C1/C2下的Total Response Time
  int round = (int)(ESTIMATE_SPACE / INTERVAL_SPACE), j;
  for(i = 0; i < NUM_OF_USER; i++) {
    for(j = 0; j < round; j++) {
      TRT[i] += get_estimate_total_response_time_by_pageNumberInCache_use_hit_count(user, i+1, avgHDDResponseTime[i], j);
    }
    TRT[i] = TRT[i] / (double)round;
  }

  /* Step2 */
  double slope[NUM_OF_USER] = {0.0};
  for(i = 0; i < NUM_OF_USER; i++) {
    slope[i] = calculate_slope(user, i+1, whoNeedIncreaseCache);
  }

  /* Step3 & Step4*/
  double TRTW[NUM_OF_USER], TS[NUM_OF_USER], CS[NUM_OF_USER], CHT[NUM_OF_USER];
  for(i = 0; i < NUM_OF_USER; i++) {
    /* Step3 */    
    TRTW[i] = TRT[i] * (double)user[i].globalWeight;
    TS[i] = slope[i] * (double)(totalReadReqCounts[i] + totalWriteReqCounts[i]);
    /* Step4*/
    CS[i] = (double)user[i].pageNumberInCache;
    CHT[i] = (double)(get_accumulative_read_hit_counts(i+1, CS[i] - 1) + get_accumulative_write_hit_counts(i+1, CS[i] - 1));
  }

  /* Step5 */
  double coefficient[NUM_OF_USER] = {0.0}, equal;
  coefficient[0] = TRTW[1]*TS[0];
  coefficient[1] = -TRTW[0]*TS[1];
  equal = TRTW[1]*TS[0]*CS[0] - TRTW[0]*TS[1]*CS[1] + TRTW[0]*CHT[1] - TRTW[1]*CHT[0];

  /* Step6 */
  double det, matrix[NUM_OF_USER];
  det = coefficient[1] - coefficient[0];
  matrix[0] = (double)SSD_CACHING_SPACE_BY_PAGES*coefficient[1] - equal;
  matrix[1] = equal - (double)SSD_CACHING_SPACE_BY_PAGES*coefficient[0];
  newSize[0] = (unsigned long)(matrix[0]/det);
  newSize[1] = SSD_CACHING_SPACE_BY_PAGES - newSize[0];

  // printf("\nU1: %f\tU2: %f", avgHDDResponseTime[0], avgHDDResponseTime[1]);
  printf("\nUS1: %lu\tUS2: %lu", user[0].pageNumberInCache, user[1].pageNumberInCache);
  printf("\nU1 actualThr: %f\tU2 actualThr: %f\n", actualThroughput[0], actualThroughput[1]);
  printf("\nwhoNeedIncreaseCache: %d\n", whoNeedIncreaseCache+1);
  // printf("\nreq1: %d\treq2: %d\n", totalReadReqCounts[0] + totalWriteReqCounts[0], totalReadReqCounts[1] + totalWriteReqCounts[1]);
  printf("\nU1 TRT: %f\tU2 TRT: %f\n", TRT[0], TRT[1]);
  // printf("\nU1 TRTW: %f\tU2 TRTW: %f\n", TRTW[0], TRTW[1]);
  printf("\nU1 TS: %f\tU2 TS: %f\n", TS[0], TS[1]);
  printf("\nU1 CS: %f\tU2 CS: %f\n", CS[0], CS[1]);
  printf("\nU1 CHT: %f\tU2 CHT: %f\n", CHT[0], CHT[1]);
  printf("\nU1 coefficient: %f\tU2 coefficient: %f\tequal: %f\n", coefficient[0], coefficient[1], equal);
  printf("\nU1 det: %f\tmatrix1: %f\tmatrix2: %f\n", det, matrix[0], matrix[1]);
  printf("\nU1Size: %lu\tU2Size: %lu", newSize[0], newSize[1]);  
  return;

  /*剔除victim user的page*/
  // for(i = 0; i < userCacheSize[victim-1] - newSize[victim-1]; i++) {
  //   evict_cache_by_adjustment(victim);
  // }
  /*reszie user cache*/
  // for(i = 0; i < NUM_OF_USER; i++) {
  //   userCacheSize[i] = newSize[i];
  // }
}

/*
 * [算斜率]
 * @param {userInfo *} user [User pointer]
 * @param {int} userno [user 1~n]
 * @param {int} whoNeedIncreaseCache [誰要加誰要減]
 * @return {double} [回傳slope value]
 */
double calculate_slope(userInfo *user, int userno, int whoNeedIncreaseCache) {
  unsigned long readHitCount, readHitCount2, writeHitCount, writeHitCount2, missCount, plusPages;
  plusPages = (unsigned long)(ESTIMATE_SPACE / INTERVAL_SPACE) * (unsigned long)(SSD_CACHING_SPACE_BY_PAGES * INTERVAL_SPACE);
  readHitCount = get_accumulative_read_hit_counts(userno, user[userno-1].pageNumberInCache - 1);
  writeHitCount = get_accumulative_write_hit_counts(userno, user[userno-1].pageNumberInCache - 1);
  if(whoNeedIncreaseCache == userno-1) {
    /*超過總cache的大小*/
    if(user[userno-1].pageNumberInCache >= (unsigned long)(SSD_CACHING_SPACE_BY_PAGES * (1-ESTIMATE_SPACE))) {
      readHitCount2 = get_accumulative_read_hit_counts(userno, SSD_CACHING_SPACE_BY_PAGES - 1);
      writeHitCount2 = get_accumulative_write_hit_counts(userno, SSD_CACHING_SPACE_BY_PAGES - 1);
    } else {
      readHitCount2 = get_accumulative_read_hit_counts(userno, user[userno-1].pageNumberInCache + plusPages - 1);
      writeHitCount2 = get_accumulative_write_hit_counts(userno, user[userno-1].pageNumberInCache + plusPages - 1);
    }
  } else {
    /*小於cache的大小*/
    if(user[userno-1].pageNumberInCache <= (unsigned long)(SSD_CACHING_SPACE_BY_PAGES * ESTIMATE_SPACE)) {
      readHitCount2 = get_accumulative_read_hit_counts(userno, (unsigned long)(SSD_CACHING_SPACE_BY_PAGES * INTERVAL_SPACE) - 1);
      writeHitCount2 = get_accumulative_write_hit_counts(userno, (unsigned long)(SSD_CACHING_SPACE_BY_PAGES * INTERVAL_SPACE) - 1);
    } else {
      readHitCount2 = get_accumulative_read_hit_counts(userno, user[userno-1].pageNumberInCache - plusPages - 1);
      writeHitCount2 = get_accumulative_write_hit_counts(userno, user[userno-1].pageNumberInCache - plusPages - 1);
    }
  }
  
  printf("\nU%d  readHitCount: %lu\twriteHitCount: %lu", userno, readHitCount, writeHitCount);  
  printf("U%d  readHitCount2: %lu\twriteHitCount2: %lu\n", userno, readHitCount2, writeHitCount2);  
  

  double hitRatio1, hitRatio2;
  hitRatio1 = (double)(writeHitCount+readHitCount) / (double)(totalReadReqCounts[userno-1]+totalWriteReqCounts[userno-1]);
  hitRatio2 = (double)(writeHitCount2+readHitCount2) / (double)(totalReadReqCounts[userno-1]+totalWriteReqCounts[userno-1]);

  double slope = (hitRatio1 - hitRatio2) / (double)plusPages;
  if(slope < 0) {
    slope = slope * (-1.0);
  }
  
  return slope;
}

/*
 * [user根據當前的cache size取得avg的total response time]
 * @param {userInfo *} user [User pointer]
 * @param {int} userno [user 1~n]
 * @param {double} avgHDDResponseTime [上一輪adjust cache period的average HDD Request response time]
 * @param {int} round []
 * @return {double} [回傳此user預估的totalResponseTime]
 */ 
double get_estimate_total_response_time_by_pageNumberInCache_use_hit_count(userInfo *user, int userno, double avgHDDResponseTime, int round) {
  unsigned long readHitCount, writeHitCount, missCount, plusPages;
  plusPages = (unsigned long)round * (unsigned long)(SSD_CACHING_SPACE_BY_PAGES * INTERVAL_SPACE);
  readHitCount = get_accumulative_read_hit_counts(userno, user[userno-1].pageNumberInCache + plusPages - 1);
  writeHitCount = get_accumulative_write_hit_counts(userno, user[userno-1].pageNumberInCache + plusPages - 1);
  // printf(COLOR_RB"\n%d: %lu\n"COLOR_RESET, userno, user[userno-1].pageNumberInCache + plusPages - 1);
  missCount = (totalReadReqCounts[userno-1] - readHitCount) + (totalWriteReqCounts[userno-1] - writeHitCount);

  double totalResponseTime = 0.0;       //此user在特定cache size下所得到的total response time  (尚未加入Queuing Delay)
  // totalResponseTime = ((double)readHitCount * SSD_READ_PRE_CHAREG_VALUE) + ((double)writeHitCount * SSD_WRITE_PRE_CHAREG_VALUE) + ((double)missCount * avgHDDResponseTime);
  totalResponseTime = ((double)readHitCount * SSD_READ_PRE_CHAREG_VALUE) + ((double)writeHitCount * SSD_WRITE_PRE_CHAREG_VALUE) + ((double)missCount * avgHDDResponseTime);

  return totalResponseTime;      //回傳預估的total response time
}


/*
 * [user根據當前的cache size取得預估的throughput (total requests)]
 * @param {userInfo *} user [User pointer]
 * @param {int} userno [victim user 1~n]
 * @param {double} avgHDDResponseTime [上一輪adjust cache period的average HDD Request response time]
 * @return {double} [回傳此user預估的throughput]
 */ 
double get_estimate_throughput_by_pageNumberInCache(userInfo *user, int userno, double avgHDDResponseTime) {
  unsigned long readHitCount, writeHitCount, missCount;
  readHitCount = get_accumulative_read_hit_counts(userno, user[userno-1].pageNumberInCache-1);
  writeHitCount = get_accumulative_write_hit_counts(userno, user[userno-1].pageNumberInCache-1);
  missCount = (totalReadReqCounts[userno-1] - readHitCount) + (totalWriteReqCounts[userno-1] - writeHitCount);

  double totalResponseTime = 0.0;       //此user在特定cache size下所得到的total response time  (尚未加入Queuing Delay)
  totalResponseTime = ((double)readHitCount * SSD_READ_PRE_CHAREG_VALUE) + ((double)writeHitCount * SSD_WRITE_PRE_CHAREG_VALUE) + ((double)missCount * avgHDDResponseTime);

  return ((double)(totalReadReqCounts[userno-1] + totalWriteReqCounts[userno-1]) / totalResponseTime);      //回傳預估的throughput (request/ ms)
}

/*
 * [剔除最小PC且靠近cache list LRU端的page (此函式式給adjust_cache_size使用)]
 * @param {unsigned} userno [victim user 1~n]
 */ 
void evict_cache_by_adjustment(unsigned userno) {
  unsigned unum = userno;

  // METABLOCK *min_meta = NULL;
  unsigned long minPriPageno;       //要剔除的page number
  double min;

  SSD_CACHE *tmp;
  tmp = cache_list[unum-1].head;      //將起始點指定為cache list的head(MRU端)
  min = tmp->pcMeta->prize;
  minPriPageno = tmp->pageno;
  
  /*找到victim page*/
  while(1) {
    if(min >= tmp->pcMeta->prize) {       //找到要剔除的page
      min = tmp->pcMeta->prize;
      minPriPageno = tmp->pageno;
    }
    if(tmp->next != NULL) {
      tmp = tmp->next;
    } else {
      break;
    }
  }
  /*開始將victim page踢出victim user的cache list*/
  if(cache_list[unum-1].head->pageno == minPriPageno) {            //若剔除的page在cache list的head
    tmp = cache_list[unum-1].head;
    cache_list[unum-1].head = cache_list[unum-1].head->next;
    cache_list[unum-1].head->prev = NULL;
    tmp->next = NULL;
  } else if(cache_list[unum-1].tail->pageno == minPriPageno) {     //若剔除的page在cache list的tail
    tmp = cache_list[unum-1].tail;
    cache_list[unum-1].tail = cache_list[unum-1].tail->prev;
    cache_list[unum-1].tail->next = NULL;
    tmp->prev = NULL;
  } else {
    tmp = &ssdCache[minPriPageno];
    tmp->prev->next = tmp->next;
    tmp->next->prev = tmp->prev;
    tmp->next = tmp->prev = NULL;
  }
  ssdCache[minPriPageno].freeFlag = PAGE_FLAG_FREE;     //將victim page flag更改為free
  
  /*meta->seqLen--*/
  ssdCache[minPriPageno].pcMeta->seqLen--;
  /*若此page是dirty, 需要送HDD write system request*/
  // if(ssdCache[minPriPageno].dirtyFlag == PAGE_FLAG_DIRTY) {

  // }

  cache_list[unum-1].size--;
  userCacheCount[unum-1]--;     //將victim user的cache count-1
  
  if(unum == 1) {   //表示user1是victim
    userFreeCount[1]++;       //所以user2的cache free count+1
  } else {
    userFreeCount[0]++;       //所以user1的cache free count+1
  }
}


/**
 * [寫檔至 Cache_Period_Record, 紀錄每個user每second的cache累積量(單位:page)]
 * @param {FILE*} st [寫檔Pointer]
 */
void second_record_cache(FILE **result, double systemTime) {
  // printf("\nCache: %f,%f\n", (double)userCacheCount[0]/(double)(SSD_CACHING_SPACE_BY_PAGES), (double)userCacheCount[1]/(double)(SSD_CACHING_SPACE_BY_PAGES));
  fprintf(*result, "%f,%f,%f\n", systemTime, (double)userCacheCount[0]/(double)(SSD_CACHING_SPACE_BY_PAGES), (double)userCacheCount[1]/(double)(SSD_CACHING_SPACE_BY_PAGES));
}


/**
 * [將SSD Page Number轉成Disksim Block(Sector)]
 * @param {unsigned long} ssdPageno [SSD Page Number]
 * @return {unsigned long} - [Block(Sector) Number for SSDsim(Disksim)]
 */
unsigned long ssd_page_to_sim_sector(unsigned long ssdPageno) {
	return ssdPageno*SSD_PAGE2SECTOR;
}


/*************************************************************************************/

/*  [Ghost Cache]
 *  利用LRU cache計算workload的reuse distance
 *  以double link list實作LRU cache, 再搭配hash table查詢
 *  概念:
 *    當一個data到來時，先查詢hash table
 *      若有紀錄，代表此data已經在cache內
 *        則將此data移至MRU端，並且將reuse distance記錄至distanceCounter array
 *      若沒紀錄，代表此data不在cache內
 *        若cache沒有滿
 *          則將此data放在cache MRU端，且update hash table
 *        若cahce滿了
 *          則將cache LRU端的資料剔除，再將data放在cache MRU端，最後update hash table
 */

/* 
 * [查詢目標是否在hash table內]
 * @param {unsigned long int } insertValue [要查詢的目標]
 * @param {int } userNumber [user代號 1~n]
 * return true/false    [目標在table內 / 目標不在table內]
 */
bool search_hash_table(unsigned long int searchValue, int userNumber) {
  hash_node *tmp;
  tmp = hTable[userNumber-1][searchValue%HASH_TABLE_SIZE].head;

  /* 檢查此key的list是否有資料存在 */
  if(hTable[userNumber-1][searchValue%HASH_TABLE_SIZE].size == 0) {
    return false;
  }

  while(1) {
    if(tmp->value == searchValue) {
      return true;
    }
    if(tmp->next != NULL) {
      tmp = tmp->next;
    } else {
      break;
    }
  }
  return false;
}

/* 
 * [新增資料進hash table, 須先使用search檢查是否已存在, 若存在, 則不應該新增]
 * @param {unsigned long int } insertValue [要新增的目標]
 * @param {int } userNumber [user代號 1~n]
 */
void insert_hash_table(unsigned long int insertValue, int userNumber) {
  hash_node *node = (hash_node *) malloc(sizeof(hash_node));
  node->value = insertValue;
  node->next = node->prev = NULL;
  
  /* 此list目前沒有資料 */
  if(hTable[userNumber-1][insertValue%HASH_TABLE_SIZE].size == 0) {
    hTable[userNumber-1][insertValue%HASH_TABLE_SIZE].head = hTable[userNumber-1][insertValue%HASH_TABLE_SIZE].tail = node;
    hTable[userNumber-1][insertValue%HASH_TABLE_SIZE].size++;
    return ;
  }
  
  node->prev = hTable[userNumber-1][insertValue%HASH_TABLE_SIZE].tail;
  hTable[userNumber-1][insertValue%HASH_TABLE_SIZE].tail->next = node;
  hTable[userNumber-1][insertValue%HASH_TABLE_SIZE].tail = node;
  hTable[userNumber-1][insertValue%HASH_TABLE_SIZE].size++;
}

/* 
 * [刪除已存在hash table內的資料, 須先使用search檢查是否已存在, 若不存在, 則不應該刪除]
 * @param {unsigned long int } deleteValue [要刪除的目標]
 * @param {int } userNumber [user代號 1~n]
 */
void delete_hash_table(unsigned long int deleteValue, int userNumber) {
  /* 此list目前有一筆資料 */
  if(hTable[userNumber-1][deleteValue%HASH_TABLE_SIZE].size == 1) {
    free(hTable[userNumber-1][deleteValue%HASH_TABLE_SIZE].head);      //釋放記憶體
    hTable[userNumber-1][deleteValue%HASH_TABLE_SIZE].head = hTable[userNumber-1][deleteValue%HASH_TABLE_SIZE].tail = NULL;
    hTable[userNumber-1][deleteValue%HASH_TABLE_SIZE].size--;
    return ;
  }

  hash_node *tmp;     //用來指向要刪除的位址

   /* 若要刪除的資料是在head */
  if(hTable[userNumber-1][deleteValue%HASH_TABLE_SIZE].head->value == deleteValue) {
    tmp = hTable[userNumber-1][deleteValue%HASH_TABLE_SIZE].head;
    hTable[userNumber-1][deleteValue%HASH_TABLE_SIZE].head = tmp->next;
    hTable[userNumber-1][deleteValue%HASH_TABLE_SIZE].head->prev = NULL;
    free(tmp);
    hTable[userNumber-1][deleteValue%HASH_TABLE_SIZE].size--;
    return;
  }

  /* 若要刪除的資料是在tail */
  if(hTable[userNumber-1][deleteValue%HASH_TABLE_SIZE].tail->value == deleteValue) {
    tmp = hTable[userNumber-1][deleteValue%HASH_TABLE_SIZE].tail;
    hTable[userNumber-1][deleteValue%HASH_TABLE_SIZE].tail = tmp->prev;
    hTable[userNumber-1][deleteValue%HASH_TABLE_SIZE].tail->next = NULL;
    free(tmp);
    hTable[userNumber-1][deleteValue%HASH_TABLE_SIZE].size--;
    return;
  }

  /* 刪除屬於list中間的資料, 需慢慢查詢 */
  tmp = hTable[userNumber-1][deleteValue%HASH_TABLE_SIZE].head->next;      //將起始位址設為head的下一個資料
  while(1) {
    if(tmp->value == deleteValue) {
      tmp->prev->next = tmp->next;
      tmp->next->prev = tmp->prev;
      free(tmp);
      hTable[userNumber-1][deleteValue%HASH_TABLE_SIZE].size--;
      break;
    }
    if(tmp->next != hTable[userNumber-1][deleteValue%HASH_TABLE_SIZE].tail) {
      tmp = tmp->next;
    } else {
      break;
    }
  }
}

/* 
 * [重置hash table]
 */
void reset_hash_table() {
  int i, j;
  for(i = 0; i < NUM_OF_USER; i++) {
    for(j = 0; j < HASH_TABLE_SIZE; j++) {
      if(hTable[i][j].size != 0) {              // this entry has records
        if(hTable[i][j].size == 1) {            // only one record
          free(hTable[i][j].head);      //釋放記憶體
          hTable[i][j].head = hTable[i][j].tail = NULL;
          hTable[i][j].size--;
        } else {
          hash_node *tmp;     //用來指向要刪除的位址
          while(hTable[i][j].size > 1) {
            /*delete record from list tail*/
            tmp = hTable[i][j].tail;
            hTable[i][j].tail = hTable[i][j].tail->prev;
            hTable[i][j].tail->next = NULL;
            free(tmp);
            hTable[i][j].size--;
          }
          /*只剩下一個record*/
          free(hTable[i][j].tail);
          hTable[i][j].head = hTable[i][j].tail = NULL;
          hTable[i][j].size--;
        }
      }
    }
  }
}

/* 
 * [列出hash table的內容]
 * @param {int } userNumber [user代號 1~n]
 */
void display_hash_table(int userNumber) {
  int key;
  hash_node *tmp;
  printf("Key\tValue\n");  
  for(key = 0; key < HASH_TABLE_SIZE; key++) {
    printf("%d\t", key);      //print 「key」
    tmp = hTable[userNumber-1][key].head;
    /* 若此key的list沒有資料，則換下一個key的list */
    if(tmp == NULL) {
      printf("NULL\n");
      continue;
    }
    while(1) {
      printf("%lu -> ", tmp->value);
      if(tmp->next != NULL) {
        tmp = tmp->next;
      } else {
        break;
      }
    } 
    printf("NULL\n");
  }
}

/* 
 * [初始化ghost cache]
 */
void initialize_ghost_cache() {
  int i;
  for(i = 0; i < NUM_OF_USER; i++) {
    ghostCache[i].size = 0;
    ghostCache[i].head = ghostCache[i].tail = NULL;
  }
  printf(COLOR_YB"Ghost cache initialize successfully\n"COLOR_RESET);
}

/* 
 * [reset ghost cache and hash table]
 */
void reset_ghost_cache_and_hash_table() {
  int i, j;
  for(i = 0; i <NUM_OF_USER; i++) {
    /*reset user's request counter*/
    totalReadReqCounts[i] = 0;
    totalWriteReqCounts[i] = 0;

    /*reset distance counter*/
    for(j = 0; j < SSD_CACHING_SPACE_BY_PAGES;j++) {
      readDistanceCounter[i][j] = 0;
      writeDistanceCounter[i][j] = 0;
    }
    /*reset ghost cache list*/
    while(ghostCache[i].size > 1) {
      ghost_cache_page *tmp;      //用來指向要刪除的位址
      tmp = ghostCache[i].tail;
      ghostCache[i].tail = ghostCache[i].tail->prev;
      ghostCache[i].tail->next = NULL;
      free(tmp);
      ghostCache[i].size--;
    }
    /* remain one record */
    free(ghostCache[i].head);    
    ghostCache[i].tail = ghostCache[i].head = NULL;
    ghostCache[i].size--;
    
  }
  /* reset hash table */
  reset_hash_table();
  
}

/* 
 * [紀錄reuse distance並且將data移動到ghost MRU端]
 * @param {unsigned long int } diskBlkno [要移動到MRU端的data]
 * @param {int } userNumber [user代號 1~n]
 * @param {unsigned } reqFlag [1:read, 0:write]
 */
void record_and_move_data_to_mru(unsigned long int diskBlkno, int userNumber, unsigned reqFlag) {
  /* data在head, 只記錄reuse distance，不移動data */
  if(ghostCache[userNumber-1].head->diskBlkno == diskBlkno) {
    if(reqFlag) {       // request is read
      readDistanceCounter[userNumber-1][0]++;       //read distance 0 counter ++
    } else {
      writeDistanceCounter[userNumber-1][0]++;      //write distance 0 counter ++
    }
    return;
  }
  
  /* data在tail */
  if(ghostCache[userNumber-1].tail->diskBlkno == diskBlkno) {
    if(reqFlag) {       // request is read
      readDistanceCounter[userNumber-1][ghostCache[userNumber-1].size-1]++;       //read distance 0 counter ++
    } else {
      writeDistanceCounter[userNumber-1][ghostCache[userNumber-1].size-1]++;      //write distance 0 counter ++
    }
    /* 移動data至MRU端 */
    ghost_cache_page *tmp;
    tmp = ghostCache[userNumber-1].tail;
    ghostCache[userNumber-1].tail = ghostCache[userNumber-1].tail->prev;
    ghostCache[userNumber-1].tail->next = NULL;
    tmp->next = ghostCache[userNumber-1].head;
    tmp->prev = NULL;
    ghostCache[userNumber-1].head->prev = tmp;
    ghostCache[userNumber-1].head = tmp;
    return;
  }

  /* data在中間 */
  int i = 0;
  ghost_cache_page *tmp;
  tmp = ghostCache[userNumber-1].head;
  while(tmp != NULL) {
    if(tmp->diskBlkno == diskBlkno) {
      if(reqFlag) {       // request is read
        readDistanceCounter[userNumber-1][i]++;       //read distance 0 counter ++
      } else {
        writeDistanceCounter[userNumber-1][i]++;      //write distance 0 counter ++
      }
      /* 移動data至MRU端 */
      tmp->prev->next = tmp->next;
      tmp->next->prev = tmp->prev;
      tmp->prev = NULL;
      tmp->next = ghostCache[userNumber-1].head;
      ghostCache[userNumber-1].head->prev = tmp;
      ghostCache[userNumber-1].head = tmp;
      return;
    }
    tmp = tmp->next;
    i++;
  }

}

/* 
 * [新增data到ghost cache MRU端，並且update hash table]
 * @param {unsigned long int } diskBlkno [要移動到ghost MRU端的data]
 * @param {int } userNumber [user代號 1~n]
 */
void add_new_data_to_ghost_cache(unsigned long int diskBlkno, int userNumber) {
  /* 檢查cache 有沒有滿 */
  if(ghostCache[userNumber-1].size == SSD_CACHING_SPACE_BY_PAGES) {        //若ghostCache 滿了
    /* 先將hash table update(剔除ghostCache tail的diskBlkno data)*/
    delete_hash_table(ghostCache[userNumber-1].tail->diskBlkno, userNumber);
    /* 將hash table update(將diskBlkno data新增至hash table) */
    insert_hash_table(diskBlkno, userNumber);
    /* 再剔除LRU端的data  */
    ghost_cache_page *tmp;
    tmp = ghostCache[userNumber-1].tail;
    ghostCache[userNumber-1].tail = ghostCache[userNumber-1].tail->prev;
    ghostCache[userNumber-1].tail->next = NULL;
    free(tmp);
    /* 再將new data新增進cache head(MRU端)*/
    ghost_cache_page *new = (ghost_cache_page*)malloc(sizeof(ghost_cache_page));
    new->diskBlkno = diskBlkno;
    new->prev = NULL;
    new->next = ghostCache[userNumber-1].head;
    ghostCache[userNumber-1].head->prev = new;
    ghostCache[userNumber-1].head = new;
  } else {      //若cache沒有滿
    /* 先將hash table update(將diskBlkno data新增至hash table) */
    insert_hash_table(diskBlkno, userNumber);
    /* 若沒有任何data在cache內 */
    if(ghostCache[userNumber-1].size == 0) {
      ghost_cache_page *new = (ghost_cache_page*)malloc(sizeof(ghost_cache_page));
      new->diskBlkno = diskBlkno;
      new->prev = new->next = NULL;
      ghostCache[userNumber-1].head = ghostCache[userNumber-1].tail = new;
    } else {
      /* 將new data新增進ghostCache head(MRU端)*/
      ghost_cache_page *new = (ghost_cache_page*)malloc(sizeof(ghost_cache_page));
      new->diskBlkno = diskBlkno;
      new->prev = NULL;
      new->next = ghostCache[userNumber-1].head;
      ghostCache[userNumber-1].head->prev = new;
      ghostCache[userNumber-1].head = new;
    }
    ghostCache[userNumber-1].size++;
    
  }
}

/*
 * [取得readDistanceCounter指定位址之前(包含指定位址)的累積的read hit counts]
 * @param {unsigned} userno [user 1~n]
 * @param {unsigned long} index [指定array的位址]
 * @return {unsigned long} [累積的hit counts數量]
 */ 
unsigned long get_accumulative_read_hit_counts(unsigned userno, unsigned long index) {
  unsigned long accumulativeHitCounts = 0, i;
  for(i = 0; i <= index; i++) {
    accumulativeHitCounts += readDistanceCounter[userno-1][i];
  }
  return accumulativeHitCounts;
}

/*
 * [取得writeDistanceCounter指定位址之前(包含指定位址)的累積的read hit counts]
 * @param {unsigned} userno [user 1~n]
 * @param {unsigned long} index [指定array的位址]
 * @return {unsigned long} [累積的hit counts數量]
 */ 
unsigned long get_accumulative_write_hit_counts(unsigned userno, unsigned long index) {
  unsigned long accumulativeHitCounts = 0, i;
  for(i = 0; i <= index; i++) {
    accumulativeHitCounts += writeDistanceCounter[userno-1][i];
  }
  return accumulativeHitCounts;
}

/*
 * 透過ghost cache和target throughput來得到預估的cache size
 * @param {unsigned} userno [User number(1-n)]
 */
// unsigned long get_cache_size_by_reference_ghost_with_throughput(double targetThr, int userNum) {
//   return 0;
// }

/* 
 * [處理到來的request]
 * @param {unsigned long int } diskBlkno [data]
 * @param {int } userNumber [user代號 1~n]
 * @param {unsigned } reqFlag [1:read, 0:write]
 */
void handle_coming_req(unsigned long int diskBlkno, int userNumber, unsigned reqFlag) {
  /* 將data丟進hash table查詢 */
  if(search_hash_table(diskBlkno, userNumber)) {      //資料存在hash table，表示也存在cache
    // hitCount[userNumber-1]++;
    record_and_move_data_to_mru(diskBlkno, userNumber, reqFlag);     //紀錄reuse distance並且將data移動到ghost MRU端
  } else {        //資料存不在has table，表示也不存在cache
    // missCount[userNumber-1]++;
    add_new_data_to_ghost_cache(diskBlkno, userNumber);
  }

  if(reqFlag) {           //is read request
    totalReadReqCounts[userNumber-1]++;
  } else {
    totalWriteReqCounts[userNumber-1]++;
  }
}

/* 
 * [列出ghost cache list的內容]
 * @param {int } userNumber [user代號 1~n]
 */
void display_ghost_cache_list(int userNumber) {
  int key;
  ghost_cache_page *tmp;
  tmp = ghostCache[userNumber-1].head;
  printf("\nUser%d ghostCache element number:%lu\n", userNumber, ghostCache[userNumber-1].size);  
  while(tmp != NULL) {
    printf("%lu -> ", tmp->diskBlkno);
    tmp = tmp->next;
  }
  printf("NULL\n");
}

/* 
 * [列出ghost cache list & hash table的records number]
 */
void display_ghost_cache_and_hash_table_number() {
  int i, j, hSize = 0;
  for(i = 0; i < NUM_OF_USER; i++) {
    for(j = 0; j < HASH_TABLE_SIZE; j++) {
      hSize += hTable[i][j].size;
    }
    printf(COLOR_YB"\nGhost size: %lu\tHash table size: %d"COLOR_RESET, ghostCache[i].size, hSize);
    hSize = 0;
  }
  printf("\n");
}

/* 
 * [打印訊息]
 */
void show_user_cache_result() {
  int i;
  printf("\n");
  for(i = 0; i < NUM_OF_USER; i++) {
    printf("U%d Cache size: %lu\t", i+1, userCacheSize[i]);
  }
  printf("\n");
}