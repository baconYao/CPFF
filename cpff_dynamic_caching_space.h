#ifndef CPFF_DYNAMIC_CACHING_SPACE_H
#define CPFF_DYNAMIC_CACHING_SPACE_H

#include <stdlib.h>
#include <stdio.h>
#include "cpff_parameter.h"
#include "cpff_structure.h"
#include "cpff_prize_caching.h"

	/*STRUCTURE DEFINITION: SSD CACHE*/
	typedef struct SSD_cache {
		unsigned long pageno;		//(In cache)SSD Page Number
		unsigned long diskBlkno;	//(In disk) HDD Block Number//Block編號(根據Disksim格式)
		int dirtyFlag;				//標記是否為Dirty page
		int freeFlag;				//標記是否為Free page
		unsigned user;				//User Number for multi users
		double accessTime;			//Cache的存取時間
		struct metaBlock *pcMeta;			//連結PC的Metadata
	} SSD_CACHE;

  /*
   *SSD CACHE TABLE
   *SSD_CACHING_SPACE_BY_PAGES was defined at parameter.h file
   */
	static SSD_CACHE ssdCache[SSD_CACHING_SPACE_BY_PAGES];

	/*****With ssd logical partition*****/

	/*USER SSD CACHE LOGICAL PARTITION*/
	static unsigned long userCacheStart[NUM_OF_USER];
	static unsigned long userCacheSize[NUM_OF_USER];

	/*USER FREE CACHE COUNT*/
	static unsigned long userFreeCount[NUM_OF_USER];
	static unsigned long userCacheCount[NUM_OF_USER];

	/*USER CACHE INITIALIZATION*/
	int init_user_cache(userInfo *user);

	// /*INSERT CACHE TABLE BY USER*/
	SSD_CACHE *insert_cache_by_user(unsigned long diskBlk, int reqFlag, unsigned userno, double time, struct metaBlock *meta, userInfo *user);

	// /*CACHE EVICTION POLICY:SPECIFIC Block with min prize and User*/
	SSD_CACHE *evict_cache_from_LRU_with_min_prize_by_user(double minPrize, unsigned userno, userInfo *user);

	// /*SEARCH CACHE BY USER*/
	SSD_CACHE *search_cache_by_user(unsigned long diskBlk, unsigned userno);

	// /*CHECK CACHE FULL BY USER*/
	int is_full_cache_by_user(unsigned userno);

	// /*GET FREE CACHE BY USER*/
	unsigned long get_free_cache_by_user(unsigned userno);

	// /*CACHING TABLE OUTPUT*/
	void print_cache_by_LRU_and_users();

	// /*GET CACHE COUNT*/
	unsigned long get_cache_cnt();

	// /*寫檔至 Result File*/
	void cache_write_result_file(FILE **result, userInfo *user);
	
#endif