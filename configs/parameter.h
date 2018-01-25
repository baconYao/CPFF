#ifndef PARAMETER_H
#define PARAMETER_H


  #define CACHING_SPACE_MANAGER


  /*使用者人數，須根據trace的user去更改*/
  #define NUM_OF_USER 2


  /*SIMULATOR*/
  #define DISKSIM_SECTOR   512  //(bytes)
  #define SSD_PAGE_SIZE    4096 //(bytes)
  #define SSD_PAGE2SECTOR (SSD_PAGE_SIZE/DISKSIM_SECTOR)
  #define SSD_PAGES_PER_BLOCK 64
  #define SSD_BLOCK_SIZE   (SSD_PAGE_SIZE*SSD_PAGES_PER_BLOCK) //(bytes)
  //#define SSD_BLOCK2SECTOR (SSD_BLOCK_SIZE/DISKSIM_SECTOR)


  #define SSD_CACHING_SPACE_BY_PAGES 32768 	// total page numnumber
	//MAX:(8*8*2048*64*8(channels) = 67108864 sectors)(67108864/PAGE2SECTOR = 8388608)
	//Hint: < 6291456 page valid! 
  
  
	#define SSD_N_ELEMENTS 1 //SSD Channels //No multi channel


	/*ipc*/
	//One message is considered as one request. The control message is a flag which used to control simulator
	#define KEY_MSQ_DISKSIM_1 0x0015		//The key of message queue for SSD simulator
	#define KEY_MSQ_DISKSIM_2 0x0026		//The key of message queue for HDD simulator
	#define MSG_TYPE_DISKSIM_1 100			//The type of message for SSD simulator
	#define MSG_TYPE_DISKSIM_1_SERVED 101	//The type of served message for SSD simulator
	#define MSG_TYPE_DISKSIM_2 200			//The type of message for HDD simulator
	#define MSG_TYPE_DISKSIM_2_SERVED 201	//The type of served message for HDD simulator
	#define MSG_REQUEST_CONTROL_FLAG_FINISH 999		//The type of control message for simulator


  /*prize caching*/
	#define MIN_PRIZE 0.0		//The minimal prize in prize caching. It's a threshold which excludes data from cache
	#define ALPHA 0.5			//The percentage of recency in the core function of prize caching

  
  /*terminal output's color*/
  #define RED_BOLD "\x1b[;31;1m"
  #define BLU_BOLD "\x1b[;34;1m"
  #define YEL_BOLD "\x1b[;33;1m"
  #define GRN_BOLD "\x1b[;32;1m"
  #define CYAN_BOLD_ITALIC "\x1b[;36;1;3m"
  #define COLOR_RESET "\x1b[0;m"

#endif