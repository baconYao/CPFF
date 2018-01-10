/*使用者人數，須根據trace的user去更改*/
#define NUM_OF_USER 2

/*SIMULATOR*/
#define DISKSIM_SECTOR   512  //(bytes)
#define SSD_PAGE_SIZE    4096 //(bytes)
#define SSD_PAGE2SECTOR (SSD_PAGE_SIZE/DISKSIM_SECTOR)
#define SSD_PAGES_PER_BLOCK 64
#define SSD_BLOCK_SIZE   (SSD_PAGE_SIZE*SSD_PAGES_PER_BLOCK) //(bytes)
//#define SSD_BLOCK2SECTOR (SSD_BLOCK_SIZE/DISKSIM_SECTOR)


/*terminal output's color*/
#define RED_BOLD "\x1b[;31;1m"
#define BLU_BOLD "\x1b[;34;1m"
#define YEL_BOLD "\x1b[;33;1m"
#define GRN_BOLD "\x1b[;32;1m"
#define CYAN_BOLD_ITALIC "\x1b[;36;1;3m"
#define COLOR_RESET "\x1b[0;m"