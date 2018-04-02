#ifndef CPFF_DYNAMIC_CACHING_SPACE_H
#define CPFF_DYNAMIC_CACHING_SPACE_H

#include <stdlib.h>
#include <stdio.h>
#include "cpff_parameter.h"
#include "cpff_structure.h"
#include "cpff_prize_caching.h"


	#define HASH_TABLE_SIZE 5       //指定hash table的大小
	/*for ghost cache*/
  static long int freeCacheSize[NUM_OF_USER] = {SSD_CACHING_SPACE_BY_PAGES};          //紀錄每個user的free cahce size, 初始化為cache 總大小
  static long int hitCount[NUM_OF_USER] = {0};          //紀錄每個user的hit count
  static long int missCount[NUM_OF_USER] = {0};          //紀錄每個user的miss count
  static unsigned long int distanceCounter[NUM_OF_USER][SSD_CACHING_SPACE_BY_PAGES] = {0};      //紀錄reuse distance的counter
  static unsigned long int totalCounts[NUM_OF_USER] = {0};        //紀錄每個user的request總數      

  /* 定義page的struct */
  typedef struct ghost_cache_page {
    unsigned long int diskBlkno;
    struct ghost_cache_page *prev;
    struct ghost_cache_page *next;
  } ghost_cache_page;

  /* 定義cache的struct */
  typedef struct ghost_cache_list {
    unsigned long int size;       //cache 目前的大小，和freeCacheSize[NUM_OF_USER]總和 = SSD_CACHING_SPACE_BY_PAGES
    ghost_cache_page *head;
    ghost_cache_page *tail;
  } ghost_cache_list;

  /* 宣告ghost cache list */
  ghost_cache_list ghostCache[NUM_OF_USER];

  /* 定義hash data的struct */
  typedef struct hash_node {
    unsigned long int value;    //data value
    struct hash_node *next;
    struct hash_node *prev;
  } hash_node;

  /* 定義hash table的struct */
  typedef struct hash_table {
    int size;                //紀錄此key的list有幾筆data
    hash_node *head;         //指向第一個hash_node
    hash_node *tail;         //指向最後一個hash_node
  } hash_table;

  /* 建立hash table */
  hash_table hTable[NUM_OF_USER][HASH_TABLE_SIZE];

  /* 初始化cache */
  void initialize_ghost_cache();
  /* 處理到來的data */
  void handle_coming_req(unsigned long int diskBlkno, int userNumber);
  /* 查詢目標是否在hash table內 */
  bool search_hash_table( unsigned long int searchValue, int userNumber);
  /* 新增資料進hash table, 須先使用search檢查是否已存在, 若存在, 則不應該新增 */
  void insert_hash_table(unsigned long int insertValue, int userNumber);
  /* 刪除已存在hash table內的資料, 須先使用search檢查是否已存在, 若不存在, 則不應該刪除 */ 
  void delete_hash_table(unsigned long int deleteValue, int userNumber);
  /* 列出hash table的內容 */ 
  void display_hash_table(int userNumber);
  /* 紀錄reuse distance並且將data移動到MRU端 */ 
  void record_and_move_data_to_mru(unsigned long int diskBlkno, int userNumber);
  /* 新增data至ghost cache的MRU端 */ 
  void add_new_data_to_ghost_cache(unsigned long int diskBlkno, int userNumber);
  /* 取得distanceCounter指定位址之前(包含指定位址)的累積的hit counts */
  unsigned long get_accumulative_hit_counts(unsigned userno, unsigned long index);
  /* 列出ghost cache list的內容 */ 
  void display_ghost_cache_list(int userNumber);
  /* 打印訊息 */ 
  void show_result();



	/*STRUCTURE DEFINITION: SSD CACHE*/
	typedef struct SSD_CACHE {
		unsigned long pageno;		//(In cache)SSD Page Number
		unsigned long diskBlkno;	//(In disk) HDD Block Number//Block編號(根據Disksim格式)
		int dirtyFlag;				//標記是否為Dirty page
		int freeFlag;				//標記是否為Free page
		unsigned user;				//User Number for multi users
		double accessTime;			//Cache的存取時間
		struct metaBlock *pcMeta;			//連結PC的Metadata
		struct SSD_CACHE *prev;
		struct SSD_CACHE *next;
	} SSD_CACHE;

	/* 定義cache的struct */
	typedef struct CACHE_LIST {
		unsigned long int size;       //= userCacheCount，和userFreeCount[NUM_OF_USER]總和 = CACHE_SIZE
		SSD_CACHE *head;
		SSD_CACHE *tail;
	} CACHE_LIST;

	/* 宣告user cache list */
	static CACHE_LIST cache_list[NUM_OF_USER];


  /*
   *SSD CACHE TABLE
   *SSD_CACHING_SPACE_BY_PAGES was defined at parameter.h file
   */
	static SSD_CACHE ssdCache[SSD_CACHING_SPACE_BY_PAGES];

	/*USER SSD CACHE LOGICAL PARTITION*/
	static unsigned long userCacheSize[NUM_OF_USER];		// = userFreeCount + userCacheCount

	/*USER FREE CACHE COUNT*/
	static unsigned long userFreeCount[NUM_OF_USER];
	static unsigned long userCacheCount[NUM_OF_USER];

	/*USER CACHE INITIALIZATION*/
	int init_user_cache(userInfo *user, int totalWeight);

	/*INSERT CACHE TABLE BY USER*/
	SSD_CACHE *insert_cache_by_user(unsigned long diskBlk, int reqFlag, unsigned userno, double time, struct metaBlock *meta, userInfo *user);

	/*CACHE EVICTION POLICY:SPECIFIC Block with min prize and User*/
	SSD_CACHE *evict_cache_from_LRU_with_min_prize_by_user(double minPrize, unsigned userno, userInfo *user);

	/*SEARCH CACHE BY USER*/
	SSD_CACHE *search_cache_by_user(unsigned long diskBlk, unsigned userno);

	/*CHECK CACHE FULL BY USER*/
	int is_full_cache_by_user(unsigned userno);

	/*GET FREE CACHE BY USER*/
	unsigned long get_free_cache_by_user(unsigned userno);

	/*GET CACHE COUNT*/
	unsigned long get_cache_cnt();

	/*調整user的cache size*/
  void adjust_user_cache_size();

	/*剔除最小PC且靠近cache list LRU端的page (此函式式給adjust_cache_size使用)*/ 
  void evict_cache_by_adjustment(unsigned userno);

	/*紀錄每個user每second的cache累積量(單位:page)*/
	void second_record_cache(FILE **result, double systemTime);
	
	/*將SSD Page Number轉成Disksim Block(Sector)*/
	unsigned long ssd_page_to_sim_sector(unsigned long ssdPageno);

#endif