#include "debug.h"

/**
 * [印出字串]
 * @param {char*} str [顯示字串]
 */
void print_something(char* str) {
    printf(COLOR_YB"[DEBUG]%s\n"COLOR_RESET, str);
}

/**
 * [印出字串與錯誤碼並結束程式]
 * @param {int} rc  [錯誤碼]
 * @param {char*} str [顯示字串]
 */
void print_error(int rc, char* str) {
	printf(COLOR_RB"[ERROR]%s%d\n"COLOR_RESET, str, rc);
	exit(1);
}

/**
 * [印出字串與錯誤碼]
 * @param {int} rc  [錯誤碼]
 * @param {char*} str [顯示字串]
 */
void print_debug(int rc, char* str) {
	printf(COLOR_YB"[DEBUG]%s%d\n"COLOR_RESET, str, rc);
}

/**
 * [印出Request資訊]
 * @param {REQ} r [Request]
 * @param {char*} str [顯示字串]
 */
void print_REQ(REQ *r, char* str) {
	printf("[DEBUG][%s]:arrivalTime=%lf\n", str, r->arrivalTime);
	printf("[DEBUG][%s]:devno=%u\n", str, r->devno);
	printf("[DEBUG][%s]:diskBlkno=%8lu\n", str, r->diskBlkno);
	printf("[DEBUG][%s]:reqSize=%u\n", str, r->reqSize);
	printf("[DEBUG][%s]:reqFlag=%u\n", str, r->reqFlag);
	printf("[DEBUG][%s]:userno=%u\n", str, r->userno);
	printf("[DEBUG][%s]:responseTime=%lf\n", str, r->responseTime);
	printf("[DEBUG][%s]:hasSystemRequest=%d\n", str, r->hasSystemRequest);
}

/**
 * [印出進度]
 * @param {unsigned long} currentREQ [目前數量]
 * @param {unsigned long} totalREQ [整體數量]
 */
void print_progress(unsigned long currentREQ, unsigned long totalREQ, unsigned long currentMeta, unsigned long currentCache) {
    printf ("\rProgress:%9lu / %9lu Meta: %6lu Cache: %8lu", currentREQ, totalREQ, currentMeta, currentCache);
    fflush (stdout);
}