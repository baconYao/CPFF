#ifndef CPFF_PARAMETER_H
#define CPFF_PARAMETER_H


  /*選擇SSD Cache Space管理測略(一次只能選一個)*/
  #define STATIC_CACHING_SPACE
  // #define DYNAMIC_CACHING_SPACE
  // #define COMPETITION_CACHING_SPACE

  /*選擇Credit管理測略(一次只能選一個)*/
  #define STATIC_CREDIT
  // #define DYNAMIC_CREDIT

  /*使用者人數，須根據trace的user去更改*/
  #define NUM_OF_USER 2


  /*SIMULATOR*/
  #define DISKSIM_SECTOR   512  //(bytes)
  #define SSD_PAGE_SIZE    4096 //(bytes)
  #define SSD_PAGE2SECTOR (SSD_PAGE_SIZE/DISKSIM_SECTOR)
  #define SSD_PAGES_PER_BLOCK 64
  #define SSD_BLOCK_SIZE   (SSD_PAGE_SIZE*SSD_PAGES_PER_BLOCK) //(bytes)
  #define SSD_PAGE_READ_TIME 0.025              // unit: ms
  #define SSD_PAGE_WRITE_TIME 0.2               // unit: ms
  #define SSD_BLOCK_ERASE_TIME 1.5              // unit: ms
  #define SSD_CHIP_XFER_LATENCY 0.000025      // unit: ms
  //#define SSD_BLOCK2SECTOR (SSD_BLOCK_SIZE/DISKSIM_SECTOR)


  #define SSD_CACHING_SPACE_BY_PAGES 1048576 	// total pages number
	//MAX:(8*8*2048*64*8(channels) = 67108864 sectors)(67108864/PAGE2SECTOR = 8388608 pages)
  //Hint: < 6291456 page valid!
  /*
   *  SSD Space  
   *  8G: 2097152 pages
   *  4G: 1048576 pages
   *  2G: 524288 pages
   *  1G: 262144 pages
   *  512MB: 131072 pages
   *  256MB: 65536 pages
   *  128MB: 32768 pages
   *  64MB: 16384 pages
   *  32MB: 8192 pages
   *  16MB: 4096 pages
   *  8MB: 2048 pages
   */
  
  
	#define SSD_N_ELEMENTS 1 //SSD Channels //No multi channel

  #define TIME_PERIOD 1000.0 //ms  //VSSD uses 1000.0
  #define STAT_FOR_TIME_PERIODS 10 // 每隔幾個TIME_PERIOD記錄一次

  /*credit*/
  #define INIT_CREDIT (TIME_PERIOD*SSD_N_ELEMENTS)		//The initial credit
  /*ssd credit pre charge ref:http://cighao.com/2016/06/28/disksim-with-ssdmodel-source-analysis-022-time-calculation/*/ 
  #define SSD_READ_PRE_CHAREG_VALUE (SSD_PAGE_READ_TIME+SSD_CHIP_XFER_LATENCY*(512+16)*SSD_PAGE2SECTOR)               //unit: ms
  #define SSD_WRITE_PRE_CHAREG_VALUE (SSD_PAGE_WRITE_TIME+SSD_CHIP_XFER_LATENCY*(512+16)*SSD_PAGE2SECTOR+1.5/64)      //unit: ms
  #define HDD_READ_SEEK_TIME 4.4                // unit: ms
  #define HDD_WRITE_SEEK_TIME 4.9                // unit: ms
  #define HDD_ROTATIONAL_LATENCY_TIME 3         // unit: ms
  #define HDD_TRANSFER_TIME 0.054253            // unit: ms,在此計算的是transfer 一個4KB的HDD request所需的時間
  /*ssd credit pre charge*/ 
  #define HDD_READ_PRE_CHAREG_VALUE HDD_READ_SEEK_TIME
  #define HDD_WRITE_PRE_CHAREG_VALUE HDD_WRITE_SEEK_TIME

	/*ipc*/
	//One message is considered as one request. The control message is a flag which used to control simulator
	#define KEY_MSQ_DISKSIM_1 0x0015		//The key of message queue for SSD simulator
	#define KEY_MSQ_DISKSIM_2 0x0026		//The key of message queue for HDD simulator
	#define MSG_TYPE_DISKSIM_1 100			//The type of message for SSD simulator
	#define MSG_TYPE_DISKSIM_1_SERVED 101	//The type of served message for SSD simulator
	#define MSG_TYPE_DISKSIM_2 200			//The type of message for HDD simulator
	#define MSG_TYPE_DISKSIM_2_SERVED 201	//The type of served message for HDD simulator
	#define MSG_REQUEST_CONTROL_FLAG_FINISH 999		//The type of control message for simulator

	/*cache*/
	#define PAGE_FLAG_FREE 0		//The flag of free page in cache table 
	#define PAGE_FLAG_NOT_FREE 1	//The flag of non-free page in cache table 
	#define PAGE_FLAG_CLEAN 1		//The flag of clean page in cache table 
	#define PAGE_FLAG_DIRTY -1		//The flag of dirty page in cache table 
	#define CACHE_FULL 1		//The flag of cache means that cache is full
	#define CACHE_NOT_FULL 0	//The flag of cache means that cache is not full


  /*prize caching*/
	#define MIN_PRIZE 0.0		//The minimal prize in prize caching. It's a threshold which excludes data from cache
	#define ALPHA 0.5			//The percentage of recency in the core function of prize caching


  /*terminal output's color*/
  #define COLOR_RB "\x1b[;31;1m"  //紅色
  #define COLOR_BB "\x1b[;34;1m"  //藍色
  #define COLOR_YB "\x1b[;33;1m"  //黃色
  #define COLOR_GB "\x1b[;32;1m"  //綠色
  #define CYAN_BOLD_ITALIC "\x1b[;36;1;3m"
  #define COLOR_RESET "\x1b[0;m"

#endif