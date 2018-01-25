#include "prize_caching.h"

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
 */
void init_meta_table() {
  unsigned i; //For loop
  //Initialize metadata table
  for (i = 0; i < NUM_OF_USER; i++) {
      metaTable[i] = NULL;
      #ifdef CACHING_SPACE_MANAGER
          basePrize[i] = 0;
      #else
          basePrize = 0;
      #endif
  }
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
  #ifdef CACHING_SPACE_MANAGER
    return (ALPHA*(((double)readCnt+1)/((double)writeCnt*(double)seqLen+1))+(1-ALPHA)*basePrize[userno-1]);
  #else
    return (ALPHA*(((double)readCnt+1)/((double)writeCnt*(double)seqLen+1))+(1-ALPHA)*basePrize);
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
  #ifdef CACHING_SPACE_MANAGER
    //Maintain the individual metadata tables
    search->next = metaTable[userNum-1];
    metaTable[userNum-1] = search;
  #else
    //Maintain only one metadata table by index:0
    search->next = metaTable[0];
    metaTable[0] = search;
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
  printf(GRN_BOLD);
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
  #ifdef CACHING_SPACE_MANAGER
    //Individual metadata tables
    unsigned unum = userno;
  #else
    //All metadatas in one table by index:0
    unsigned unum = 1;
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
double metadata_search_by_user_with_min_prize(unsigned userno) {
  //Determine which metadata table used in this function
  //Hint: unum is a substitute for different modes
  #ifdef CACHING_SPACE_MANAGER
    //Individual metadata tables
    unsigned unum = userno;
  #else
    //All metadatas in one table by index:0
    unsigned unum = 1;
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


void prize_caching() {
  printf("Prize!!!!!\n");
}