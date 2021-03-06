#ifndef CPFF_PRIZE_CACHING_H
#define CPFF_PRIZE_CACHING_H

  #include <stdlib.h>
  #include <stdio.h>
  #include <string.h>
  #include <stdbool.h>

  #include "cpff_parameter.h"
  #include "cpff_structure.h"

  /* 選擇SSD Cache管理測略 */
  #ifdef STATIC_CACHING_SPACE
    #include "cpff_static_caching_space.h"
  #elif defined DYNAMIC_CACHING_SPACE
    #include "cpff_dynamic_caching_space.h"
  #elif defined COMPETITION_CACHING_SPACE
    #include "cpff_competition_caching_space.h"
  #endif


  /*系統定義Base Prize*/
  #ifdef COMPETITION_CACHING_SPACE
    static int basePrize;
  #else
    //維護每個user的base prize value 
    static int basePrize[NUM_OF_USER];
  #endif


  /*STRUCTURE DRFINITION: METADATA BLOCK*/
  typedef struct metaBlock {
    unsigned long blkno;		//以SSD Block size的Block number
    unsigned readCnt;			//讀取次數
    unsigned writeCnt;			//寫入次數
    unsigned seqLen;			//循序存取機會。根據LBPC定義於SSD Block中，計算有多少pages被存取(目前預設一個block的總page數)
    double prize;				//紀錄Prize值
    struct metaBlock *next;		//指向下一個Metadata block
  } METABLOCK;

  //Metadata block table
  static METABLOCK *metaTable[NUM_OF_USER];

  //紀錄每個user的mate block數量
  static unsigned long metaCnt[NUM_OF_USER] = {0};

  //紀錄每個user的min prize value
  static double minPrize[NUM_OF_USER] = {0};
  
  /*STRUCTURE DRFINITION: PC STATISTICS*/
  /**[Record prize caching statistics]
    * 此紀錄係針對整體系統，無視User個別的紀錄
    */
    typedef struct pcStat {
    unsigned long totalReq;		//Request數量 (include user and system request)
    unsigned long totalSsdReq;		//SSD Request數量 (include user and system ssd request)
    unsigned long totalHddReq;		//HDD Request數量 (include user and system hdd request)
    unsigned long totalUserReq;	  //User Request數量
    unsigned long userReadReq;		//User Read Request數量
    unsigned long userWriteReq;		//User Write Request數量
    unsigned long totalSysReq;	  //System Request數量
    unsigned long sysSsdReadReq;			//System SSD Read Request數量
    unsigned long sysSsdWriteReq;			//System SSD Write Request數量
    unsigned long sysHddWriteReq;			//System HDD Write Request數量
    unsigned long evictCount;	//Eviction次數
    unsigned long dirtyCount;	//Dirty Block Eviction次數
    unsigned long hitCount;		//Hit次數
    unsigned long missCount;	//Miss次數
  } PCSTAT;

  /*PC STATISTICS*/
  static PCSTAT pcst = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};

  /*取得Metadata Block 數量*/
  unsigned long get_meta_cnt(int userno);

  /*初始化 Metadata table*/ 
  int init_meta_table();

  /*取得Metadata Block Prize*/
  double get_prize(unsigned int readCnt, unsigned int writeCnt, unsigned int seqLen, unsigned userno);

  /*更新meta block的prize value*/ 
  void meta_block_update(METABLOCK *metablk, REQ *tmp);

  /*新增meta block 到 table*/ 
  METABLOCK *add_meta_block_to_table(REQ *tmp);

  /*印出Metadata Block Table*/
  void meta_table_print();

  /*SEARCH METADATA BLOCK TABLE BY USER*/
  METABLOCK *meta_block_search_by_user(unsigned long diskBlk, unsigned userno);

  /*SEARCH METADATA BLOCK TABLE FOR USER WITH MINIMAL PRIZE*/
  double meta_block_search_by_user_with_min_prize(unsigned userno);

  /*PC algorithm*/ 
  void prize_caching(double time, userInfo *user, systemInfo *sysInfo, FILE **pcHitAccumulativeRecord);

#endif