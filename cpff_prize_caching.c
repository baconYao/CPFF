#include "cpff_prize_caching.h"

/**
 * [取得Metadata Block 數量]
 * @param {int} userNum [user的代號 1~n, if 0 =>取得所有user meta block的總和]
 * @return {unsigned long} metaCnt [Metadata Block 數量]
 */
unsigned long get_meta_cnt(int userNum) {
  if( userNum == 0) {
    int i;
    unsigned long t = 0;
    for(i = 0; i < NUM_OF_USER; i++) {
      t += metaCnt[i];
    }
    return t;
  }
  return metaCnt[userNum - 1];
}


/**
 * [初始化 Metadata table]
 * @return {int} 0 default success
 */
int init_meta_table() {
  printf(COLOR_BB"PC metablock table is initializing........");  
  unsigned i; //For loop
  //Initialize metadata table
  for (i = 0; i < NUM_OF_USER; i++) {
    metaTable[i] = NULL;
    #ifdef COMPETITION_CACHING_SPACE
      basePrize = 0;
    #else
      basePrize[i] = 0;
    #endif
  }
  printf("OK\n"COLOR_RESET);
  return 0;
}


/**
 * [計算Prize值]
 * @param {unsigned int} readCnt [Block讀取次數]
 * @param {unsigned int} writeCnt [Block寫入次數]
 * @param {unsigned int} seqLen [多少pages]
 * @return {double} - [Prize值]
 */
double get_prize(unsigned int readCnt, unsigned int writeCnt, unsigned int seqLen, unsigned userno) {
  //Calculate and return prize value
  #ifdef COMPETITION_CACHING_SPACE
    return (ALPHA*(((double)readCnt+1)/((double)writeCnt*(double)seqLen+1))+(1-ALPHA)*basePrize);
  #else
    return (ALPHA*(((double)readCnt+1)/((double)writeCnt*(double)seqLen+1))+(1-ALPHA)*basePrize[userno-1]);
  #endif
}


/**
 * [更新Metadata Block，為取得最新的Prize值]
 * @param {METABLOCK*} metablk [欲更新的Metadata Block]
 * @param {REQ*} tmp [Request]
 */
void meta_block_update(METABLOCK *metablk, REQ *tmp) {
  //Modify readCnt and writeCnt
  switch (tmp->reqFlag) {
    case DISKSIM_READ:
      metablk->readCnt++;
      break;
    case DISKSIM_WRITE:
      metablk->writeCnt++;
      break;
    default:
      break;
  }

  //Update prize
  metablk->prize = get_prize(metablk->readCnt, metablk->writeCnt, metablk->seqLen, tmp->userno);

  //DEBUG:
  //printf("[PRIZE]metaTableUpdate():METABLOCK blkno =%8lu readCnt =%6u writeCnt =%6u seqLen =%3u prize =%3lf\n", metaTable->blkno, metaTable->readCnt, metaTable->writeCnt, metaTable->seqLen, metaTable->prize);
  //metaTablePrint();
}


/**
 * [新增Metadata Block至指定Table]
 * @param {REQ} tmp [Request]
 * @return {METABLOCK*} search/NULL [New Metadata Block]
 */
METABLOCK *add_meta_block_to_table(REQ *tmp) {
  int userNum = tmp->userno;          //user number 1~n

  //New metdata (The unit is block for SSD)
  METABLOCK *search;
  search = (METABLOCK *) calloc(1, sizeof(METABLOCK));

  //Count the number of metadata
  metaCnt[userNum-1]++;

  //Record block number in metadata
  //Hint: Change Disksim block(512bytes) number to SSD block(SSD_BLOCK_SIZE) number
  search->blkno = tmp->diskBlkno/(SSD_PAGE2SECTOR*SSD_PAGES_PER_BLOCK);

  //Record readCnt and writeCnt in metadata
  switch (tmp->reqFlag) {
    case DISKSIM_READ:
      search->readCnt = 1;
      break;
    case DISKSIM_WRITE:
      search->writeCnt = 1;
      break;
    default:
      break;
  }

  //Record seqLen in metadata
  search->seqLen = 0;

  //Record prize in metadata
  search->prize = get_prize(search->readCnt, search->writeCnt, search->seqLen, userNum);

  //Insert metadata into metadata table
  #ifdef COMPETITION_CACHING_SPACE
    //Maintain only one metadata table by index:0
    search->next = metaTable[0];
    metaTable[0] = search;
  #else
    //Maintain the individual metadata tables
    search->next = metaTable[userNum-1];
    metaTable[userNum-1] = search;
  #endif

  //Return this new metadata block
  return search;
}

/**
 * [印出Metadata Block Table]
 */
void meta_table_print() {
  METABLOCK *search;
  unsigned i;
  printf(COLOR_GB);
  printf("----------------------------------------------------------------------------------------------------\n");
  for (i = 0; i < NUM_OF_USER; i++) {
    search = metaTable[i];
    
    printf("-[USER_%u METADATA BLOCK TABLE]\n", i+1);
    while(search != NULL) {
      printf("-   [PRIZE] blkno =%8lu readCnt =%6u writeCnt =%6u seqLen =%3u *prize =%3lf\n", search->blkno, search->readCnt, search->writeCnt, search->seqLen, search->prize);
      search = search->next;
    }
    printf("----------------------------------------------------------------------------------------------------\n");
    printf(COLOR_RESET);
  }
}


/**
 * [根據指定的Block Number(for HDD)搜尋指定的Metadata BlockTable]
 * @param {unsigned long} diskBlk [指定的Block Number(for HDD)(disksim格式)]
 * @param {unsigned} userno [User number(1-n)]
 * @return {METABLOCK*} search/NULL [搜尋Metadata Block結果]
 */
METABLOCK *meta_block_search_by_user(unsigned long diskBlk, unsigned userno) {
  //Determine which metadata table used in this function
  //Hint: unum is a substitute for different modes
  #ifdef COMPETITION_CACHING_SPACE
    //All metadatas in one table by index:0
    unsigned unum = 1;
  #else
    //Individual metadata tables
    unsigned unum = userno;
  #endif

  //Assign the metadata table which pc searches
  METABLOCK *search = NULL;
  search = metaTable[unum-1];
  if (metaTable[unum-1] == NULL){
    return NULL;
  }
  
  //Search the metadata
  while(search != NULL) {
    if (search->blkno == diskBlk/(SSD_PAGE2SECTOR*SSD_PAGES_PER_BLOCK)) {
      return search;
    }
    else{
      search = search->next;
    }
  }

  //Metadata not found
  return NULL;
}

/*SEARCH METADATA BLOCK TABLE FOR USER WITH MINIMAL PRIZE*/
/**
* [於指定的Metadata BlockTable中，搜尋有最小Prize值的Metadata Block]
* @param {unsigned} userno [User number(1-n)]
* @return {double} min->prize [回傳min prize, error:-1]
*/
double meta_block_search_by_user_with_min_prize(unsigned userno) {
  //Determine which metadata table used in this function
  //Hint: unum is a substitute for different modes
  #ifdef COMPETITION_CACHING_SPACE
    //All metadatas in one table by index:0
    unsigned unum = 1;
  #else
    //Individual metadata tables
    unsigned unum = userno;
  #endif

  //Assign the metadata table which pc searches
  METABLOCK *search=NULL;
  search = metaTable[unum-1];
  if (metaTable[unum-1] == NULL){
    return -1;
  }  

  //Search the metadata
  //Hint: This metadata must include some pages in cache 
  METABLOCK *min=NULL;    //Record the metadata with the minimal prize
  //Find the first metadata in the table
  while(search != NULL) {
    //This metadata must include one page in cache at least
    if (search->seqLen > 0) {
        min = search;
        break;
    }
    search = search->next;
  }

  //And, Find the metadata with the minimal prize
  while(search != NULL) {
    if (search->prize < min->prize && search->seqLen > 0) {
        min = search;
    }
    search = search->next;
  }

  //DEBUG:
  //metaTablePrint();
  //printf("[PRIZE]metadataSearchByUserWithMinPrize():Blkno:%lu, Min prize:%lf\n", min->blkno, min->prize);
  
  //Return the minimal prize
  return min->prize;
}


/**
 * [從host queue讀取Request，再根據欲處理的Request，決定系統的快取機制，並決定此request是否有額外的I/O request(即system request) ]
 * @param {double} ttime [系統的time]
 * @param {userInfo *} user [all user]
 * @param {QUE *} hostQueue [hostQueue pointer]
 */
void prize_caching(double cpffSystemTime, userInfo *user, QUE *hostQueue, systemInfo *sysInfo) {

  int flag = 0;           //The flag of page

  while(1) {

    /*host queue內沒有request*/
    if(is_empty_queue(hostQueue)) {
      break;
    }

    /*若request的arrival time > cpffSystemTime，代表此request還沒有資格進入user queue*/
    if(hostQueue->head->r.arrivalTime > cpffSystemTime) {
      break;
    }

    /*從host queue head讀取欲處理的request*/ 
    REQ *tmp;
    tmp = calloc(1, sizeof(REQ));
    copy_req(&(hostQueue->head->r), tmp);

    /*移除host queue的head指向的request*/
    remove_req_from_queue_head(hostQueue);

    /*Check the type of user request*/
    if (tmp->reqFlag == DISKSIM_READ) {
      // printf(COLOR_GB"@@@@@ User %d READ req in\n"COLOR_RESET, tmp->userno);
      
      flag = PAGE_FLAG_CLEAN;
      //Statistics
      sysInfo->totalReq++;
      sysInfo->userReadReqInSecond++;
      sysInfo->userReadReqInPeriod++;
      pcst.userReadReq++;
      pcst.totalReq++;
      pcst.totalUserReq++;
      user[tmp->userno-1].totalReq++;
      user[tmp->userno-1].totalUserReq++;
      user[tmp->userno-1].userReadReq++;
      user[tmp->userno-1].userReadReqInSecond++;
      user[tmp->userno-1].userReadReqInPeriod++;
    }
    else {
      // printf(COLOR_GB"@@@@@ User %d WRITE req in\n"COLOR_RESET, tmp->userno);
      flag = PAGE_FLAG_DIRTY;
      //Statistics
      sysInfo->totalReq++;
      sysInfo->userWriteReqInSecond++;
      sysInfo->userWriteReqInPeriod++;
      pcst.userWriteReq++;
      pcst.totalReq++;
      pcst.totalUserReq++;
      user[tmp->userno-1].totalReq++;
      user[tmp->userno-1].totalUserReq++;
      user[tmp->userno-1].userWriteReq++; 
      user[tmp->userno-1].userWriteReqInSecond++;
      user[tmp->userno-1].userWriteReqInPeriod++;
    }
  
    /*搜尋是否有被cache*/
    SSD_CACHE *cache;
    cache = search_cache_by_user(tmp->diskBlkno, tmp->userno);
  
    /*cache Hit: Page found in cache*/
    if(cache != NULL) {
      //statistics
      // sysInfo->hitCount++;
      // sysInfo->hitCountInSecond++;
      // sysInfo->hitCountInPeriod++;
      pcst.hitCount++;
      // user[tmp->userno-1].hitCount++;
      // user[tmp->userno-1].hitCountInSecond++;
      // user[tmp->userno-1].hitCountInPeriod++;
  
      /*Add new one or update metadata(prize)*/ 
      METABLOCK *meta;
      meta = meta_block_search_by_user(tmp->diskBlkno, tmp->userno);
      if(meta == NULL) {
        meta = add_meta_block_to_table(tmp);
      } else {
        meta_block_update(meta, tmp);
      }
      
      /*Caching*/
      /*因為是Cache hit，所以會update cache table資訊，在此為預更新*/
      if(insert_cache_by_user(tmp->diskBlkno, flag, tmp->userno, cpffSystemTime, meta, user) == NULL) {
        print_error(-1, "[cpff_prize_caching.c (1)]insert_cache_by_user() error(cache hit but return full)");
      }
  
      //轉換成SSD對應大小的diskBlkno
      tmp->diskBlkno = ssd_page_to_sim_sector(cache->pageno);

      //statistic
      sysInfo->totalSsdReq++;
      pcst.totalSsdReq++;
      user[tmp->userno-1].totalSsdReq++;
      

      /*將request 送至 user ssd queue*/ 
      if(!insert_req_to_user_que_tail(user, "SSD", tmp)) {
        print_error(-1, "[cpff_prize_caching.c (2)] Can't move request to user SSD queue");
      }

      // printf(COLOR_GB"@@@@@ User %d Hit\n"COLOR_RESET, tmp->userno);
      

    } else {   /*cache Miss: Page not found in cache*/  
      //Statistics
      // sysInfo->missCount++;
      // sysInfo->missCountInSecond++;
      // sysInfo->missCountInPeriod++;
      pcst.missCount++;
      // user[tmp->userno-1].missCount++;
      // user[tmp->userno-1].missCountInSecond++;
      // user[tmp->userno-1].missCountInPeriod++;

      /*New or update metadata(prize)*/ 
      METABLOCK *meta;
      meta = meta_block_search_by_user(tmp->diskBlkno, tmp->userno);
      if(meta == NULL) {
        meta = add_meta_block_to_table(tmp);
      } else {
        meta_block_update(meta, tmp);
      }

      //Compare MIN_PRIZE and cache or not, MIN_PRIZE是系統定義的最小prize值
      if(meta->prize >= MIN_PRIZE) {
        // 因為cache miss，所以試著將request cache在SSD
        cache = insert_cache_by_user(tmp->diskBlkno, flag, tmp->userno, cpffSystemTime, meta, user);
        
        //Cache is not full(no eviction)
        if (cache != NULL) {
          //Record seqLen in metadata table
          meta->seqLen++;

          //Read Cache Miss: 但cache is not full，所以會產生SSD Write system request
          if(tmp->reqFlag == DISKSIM_READ) {
            
            // printf(COLOR_GB"@@@@@ User %d READ Miss -->SSD isn't full\n"COLOR_RESET, tmp->userno);
            // printf(COLOR_GB"@@@@@ Generate a SSD Write system request\n"COLOR_RESET);

            /*將hdd read request 送至 user hdd queue*/ 
            if(!insert_req_to_user_que_tail(user, "HDD", tmp)) {
              print_error(-1, "[cpff_prize_caching.c (3)] Can't move request to user HDD queue");
            }

            //產生SSD write system request
            REQ *r;
            r = calloc(1, sizeof(REQ));
            copy_req(tmp, r);
            r->isSystemRequest = 1;     // is system request
            r->reqFlag = DISKSIM_WRITE;       // write request
            //轉換成SSD對應大小的diskBlkno
            r->diskBlkno = ssd_page_to_sim_sector(cache->pageno);

            /*將ssd write system request 送至 user ssd queue*/ 
            if(!insert_req_to_user_que_tail(user, "SSD", r)) {
              print_error(-1, "[cpff_prize_caching.c (4)] Can't move request to user SSD queue");
            }

            free(r);

            //Statistics
            sysInfo->totalReq++;      //for system ssd write req
            sysInfo->totalHddReq++;       //for user hdd read req
            sysInfo->totalSsdReq++;       //for system ssd write req
            sysInfo->totalSysReq++;       //for system ssd write req
            sysInfo->sysSsdWriteReq++;    //for system ssd write req
            sysInfo->sysSsdWriteReqInSecond++;   //for system ssd write req
            sysInfo->sysSsdWriteReqInPeriod++;   //for system ssd write req
            pcst.totalReq++;      //for system ssd write req
            pcst.totalHddReq++;       //for user hdd read req
            pcst.totalSsdReq++;       //for system ssd write req
            pcst.totalSysReq++;       //for system ssd write req
            pcst.sysSsdWriteReq++;    //for system ssd write req
            user[tmp->userno-1].totalReq++;   //for system ssd write req
            user[tmp->userno-1].totalHddReq++;    //for user hdd read req
            user[tmp->userno-1].totalSsdReq++;    //for system ssd write req
            user[tmp->userno-1].totalSysReq++;   //for system ssd write req
            user[tmp->userno-1].sysSsdWriteReq++;   //for system ssd write req
            user[tmp->userno-1].sysSsdWriteReqInSecond++;   //for system ssd write req
            user[tmp->userno-1].sysSsdWriteReqInPeriod++;   //for system ssd write req
            

          } else {//Write Cache Miss: 但cache is not full，直接將user request送到user ssd queue   
            //轉換成SSD對應大小的diskBlkno
            tmp->diskBlkno = ssd_page_to_sim_sector(cache->pageno);
            /*將ssd write system request 送至 user ssd queue*/ 
            if(!insert_req_to_user_que_tail(user, "SSD", tmp)) {
              print_error(-1, "[cpff_prize_caching.c (5)] Can't move request to user SSD queue");
            }

            //Statistics
            sysInfo->totalSsdReq++;
            pcst.totalSsdReq++;
            user[tmp->userno-1].totalSsdReq++;
            
            // printf(COLOR_GB"@@@@@ User %d WRITE Miss -->SSD isn't full\n"COLOR_RESET, tmp->userno);

          }
        } else { // Cache is full, 所以要比較SSD中的minimal prize value，決定是否代替(eviction)
          //Find the minimal prize of the cached page
          double minPrize = -1;
          minPrize = meta_block_search_by_user_with_min_prize(tmp->userno);
          if (minPrize == -1) {
            print_error(minPrize, "[cpff_prize_caching.c (6)]Something error:No caching space and no metadata with minPrize! ");
          }

          //The prize of new page >= the minimal prize of the cached page,所以要剔除有minimal prize value的page
          if (meta->prize >= minPrize) {
            /*update Base Prize Value*/
            #ifdef COMPETITION_CACHING_SPACE
              basePrize = minPrize;
            #else
              basePrize[tmp->userno-1] = minPrize;
            #endif

            //Evict the victim page with the minimal prize
            SSD_CACHE *evict;
            evict = evict_cache_from_LRU_with_min_prize_by_user(minPrize, tmp->userno, user);
            if (evict == NULL) {
              print_error(-1, "[cpff_prize_caching.c (7)]Cache eviction error: Victim not found!:");
            }
            // printf(COLOR_GB"@@@@@ User %d Miss -->SSD is full --> evict\n"COLOR_RESET, tmp->userno);
            
            //Modify metadata
            if (evict->pcMeta == NULL) {
              print_error(-1, "[cpff_prize_caching.c (8)]Something error: Meta. of victim not found!:");
            }
            evict->pcMeta->seqLen--;
            
            //Generate IO requests
            //If victim page is dirty, System Read SSDsim & System Write HDDsim
            if (evict->dirtyFlag == PAGE_FLAG_DIRTY) {
              // printf(COLOR_GB"@@@@@ Dirty --> System Read SSDsim & System Write HDDsim\n"COLOR_RESET);
              
              REQ *r1, *r2;
              r1 = calloc(1, sizeof(REQ));      //SSD read system request
              r2 = calloc(1, sizeof(REQ));      //HDD write system request
              copy_req(tmp, r1);
              copy_req(tmp, r2);
              r1->diskBlkno = ssd_page_to_sim_sector(evict->pageno);  //轉換成SSD對應大小的diskBlkno
              r1->reqFlag = DISKSIM_READ;
              r1->isSystemRequest = 2;
              r2->diskBlkno = evict->diskBlkno;
              r2->reqFlag = DISKSIM_WRITE;
              r2->isSystemRequest = 1;

              /*將ssd read system request 送至 user ssd queue*/ 
              if(!insert_req_to_user_que_tail(user, "SSD", r1)) {
                print_error(-1, "[cpff_prize_caching.c (9)] Can't move request to user SSD queue");
              }

              /*將hdd write system request 送至 user hdd queue*/ 
              if(!insert_req_to_user_que_tail(user, "HDD", r2)) {
                print_error(-1, "[cpff_prize_caching.c (10)] Can't move request to user SSD queue");
              }

              //Statistics
              sysInfo->totalReq += 2;    //for sys read ssd and sys write hdd
              sysInfo->totalSsdReq++;    //for sys read ssd
              sysInfo->totalHddReq++;    //for sys write hdd
              sysInfo->totalSysReq += 2;    //for sys read ssd and sys write hdd
              sysInfo->sysSsdReadReq++;    //for sys read ssd
              sysInfo->sysHddWriteReq++;    //for sys write hdd
              // sysInfo->dirtyCount++;
              // sysInfo->dirtyCountInSecond++;
              // sysInfo->dirtyCountInPeriod++;
              sysInfo->sysSsdReadReqInSecond++;    //for sys read ssd
              sysInfo->sysSsdReadReqInPeriod++;    //for sys read ssd
              sysInfo->sysHddWriteReqInSecond++;    //for sys write hdd
              sysInfo->sysHddWriteReqInPeriod++;    //for sys write hdd
              pcst.totalReq += 2;    //for sys read ssd and sys write hdd
              pcst.totalSsdReq++;    //for sys read ssd
              pcst.totalHddReq++;    //for sys write hdd
              pcst.totalSysReq += 2;    //for sys read ssd and sys write hdd
              pcst.sysSsdReadReq++;    //for sys read ssd
              pcst.sysHddWriteReq++;    //for sys write hdd
              pcst.dirtyCount++;
              user[tmp->userno-1].totalReq += 2;    //for sys read ssd and sys write hdd
              user[tmp->userno-1].totalSsdReq++;    //for sys read ssd
              user[tmp->userno-1].totalHddReq++;    //for sys write hdd
              user[tmp->userno-1].totalSysReq += 2;   //for sys read ssd and sys write hdd
              user[tmp->userno-1].sysSsdReadReq++;    //for sys read ssd
              user[tmp->userno-1].sysHddWriteReq++;    //for sys write hdd
              user[tmp->userno-1].sysSsdReadReqInSecond++;    //for sys read ssd
              user[tmp->userno-1].sysSsdReadReqInPeriod++;    //for sys read ssd
              user[tmp->userno-1].sysHddWriteReqInSecond++;    //for sys write hdd
              user[tmp->userno-1].sysHddWriteReqInPeriod++;    //for sys write hdd
              // user[tmp->userno-1].dirtyCount++;
              // user[tmp->userno-1].dirtyCountInSecond++;
              // user[tmp->userno-1].dirtyCountInPeriod++;

              //Release request variable
              free(r1);
              free(r2);
            }
            //Statistics
            // sysInfo->evictCount++;
            // sysInfo->evictCountInSecond++;
            // sysInfo->evictCountInPeriod++;
            pcst.evictCount++;
            // user[tmp->userno-1].evictCount++;
            // user[tmp->userno-1].evictCountInSecond++;
            // user[tmp->userno-1].evictCountInPeriod++;
            

            //Caching
            /*已剔除在SSD內最小pc value的page，接著將此筆request cache在SSD內，在此為預更新*/
            cache = insert_cache_by_user(tmp->diskBlkno, flag, tmp->userno, cpffSystemTime, meta, user);
        
            if(cache != NULL) {
              //Record seqLen in metadata table
              meta->seqLen++;

              //Generate IO requests
              //Read Miss: Read HDDsim & System Write SSDsim
              if (tmp->reqFlag == DISKSIM_READ) {
                REQ *r;
                r = calloc(1, sizeof(REQ));
                copy_req(tmp, r);

                /*將hdd read request 送至 user hdd queue*/ 
                if(!insert_req_to_user_que_tail(user, "HDD", tmp)) {
                  print_error(-1, "[cpff_prize_caching.c (11)] Can't move request to user SSD queue");
                }

                //System Write SSDsim
                r->diskBlkno = ssd_page_to_sim_sector(cache->pageno);     //轉換成SSD對應大小的diskBlkno
                r->reqFlag = DISKSIM_WRITE;
                r->isSystemRequest = 1;

                /*將ssd write system request 送至 user ssd queue*/ 
                if(!insert_req_to_user_que_tail(user, "SSD", r)) {
                  print_error(-1, "[cpff_prize_caching.c (12)] Can't move request to user SSD queue");
                }
                // printf(COLOR_GB"@@@@@ Then User %d READ Miss -->SSD isn't full\n"COLOR_RESET, tmp->userno);
                // printf(COLOR_GB"@@@@@ Generate a SSD Write system request\n"COLOR_RESET);
                //Statistics
                sysInfo->totalReq++;      //for system ssd write req
                sysInfo->totalHddReq++;       //for user hdd read req
                sysInfo->totalSsdReq++;       //for system ssd write req
                sysInfo->totalSysReq++;       //for system ssd write req
                sysInfo->sysSsdWriteReq++;    //for system ssd write req
                sysInfo->sysSsdWriteReqInSecond++;   //for system ssd write req;
                sysInfo->sysSsdWriteReqInPeriod++;   //for system ssd write req;
                pcst.totalReq++;      //for system ssd write req
                pcst.totalHddReq++;       //for user hdd read req
                pcst.totalSsdReq++;       //for system ssd write req
                pcst.totalSysReq++;       //for system ssd write req
                pcst.sysSsdWriteReq++;    //for system ssd write req
                user[tmp->userno-1].totalReq++;   //for system ssd write req
                user[tmp->userno-1].totalHddReq++;    //for user hdd read req
                user[tmp->userno-1].totalSsdReq++;    //for system ssd write req
                user[tmp->userno-1].totalSysReq++;   //for system ssd write req
                user[tmp->userno-1].sysSsdWriteReq++;   //for system ssd write req
                user[tmp->userno-1].sysSsdWriteReqInSecond++;   //for system ssd write req;
                user[tmp->userno-1].sysSsdWriteReqInPeriod++;   //for system ssd write req;

                //Release request variable
                free(r);
              } else {      //Write Miss: Write SSDsim
                tmp->diskBlkno = ssd_page_to_sim_sector(cache->pageno);   //轉換成SSD對應大小的diskBlkno

                /*將ssd write request 送至 user ssd queue*/ 
                if(!insert_req_to_user_que_tail(user, "SSD", tmp)) {
                  print_error(-1, "[cpff_prize_caching.c (13)] Can't move request to user SSD queue");
                }
                //  printf(COLOR_GB"@@@@@Then User %d WRITE Miss -->SSD isn't full\n"COLOR_RESET, tmp->userno);

                //Statistics
                sysInfo->totalSsdReq++;
                pcst.totalSsdReq++;
                user[tmp->userno-1].totalSsdReq++;
              }

            } else {
              print_error(-1, "[cpff_prize_caching.c (14)]After eviction, caching error! ");
            }
          } else {      //pc value < minPrize, 所以不cache
            if (tmp->reqFlag == DISKSIM_READ) {
              /*將hdd read request 送至 user ssd queue*/ 
              if(!insert_req_to_user_que_tail(user, "HDD", tmp)) {
                print_error(-1, "[cpff_prize_caching.c (13)] Can't move request to user SSD queue");
              }
                // printf(COLOR_GB"@@@@@ User %d READ Miss --> pc value < minPrize\n"COLOR_RESET, tmp->userno);
              
            } else {
              /*將hdd write request 送至 user ssd queue*/ 
              if(!insert_req_to_user_que_tail(user, "HDD", tmp)) {
                print_error(-1, "[cpff_prize_caching.c (13)] Can't move request to user SSD queue");
              }
                // printf(COLOR_GB"@@@@@ User %d WRITE Miss --> pc value < minPrize\n"COLOR_RESET, tmp->userno);
              
            }
            sysInfo->totalHddReq++;
            pcst.totalHddReq++;
            user[tmp->userno-1].totalHddReq++;
          }
        }
      }
    }
    //Release request variable
    free(tmp);
  }
}


