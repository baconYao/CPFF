#include "cpff_debug.h"

/**
 * [印出字串]
 * @param {char*} str [顯示字串]
 */
void print_something(char* str) {
    printf(CYAN_BOLD_ITALIC"[DEBUG]%s\n"COLOR_RESET, str);
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
	printf("-----------------------------------\n");
	printf("[DEBUG][%s]:arrivalTime=%lf\n", str, r->arrivalTime);
	printf("[DEBUG][%s]:devno=%u\n", str, r->devno);
	printf("[DEBUG][%s]:diskBlkno=%8lu\n", str, r->diskBlkno);
	printf("[DEBUG][%s]:reqSize=%u sectors\n", str, r->reqSize);
	printf("[DEBUG][%s]:reqFlag=%u\n", str, r->reqFlag);
	printf("[DEBUG][%s]:userno=%u\n", str, r->userno);
	printf("[DEBUG][%s]:responseTime=%lf\n", str, r->responseTime);
	printf("[DEBUG][%s]:isSystemRequest=%d\n", str, r->isSystemRequest);
	printf("[DEBUG][%s]:preChargeValue=%f\n", str, r->preChargeValue);
}

/**
 * [印出進度]
 * @param {unsigned long} currentREQ [目前數量]
 * @param {unsigned long} totalREQ [整體數量]
 */
void print_progress(double cpffSystemTime, unsigned long totalREQ, unsigned long doneREQ) {
	printf ("\rCPFF System Time:%f Progress:%lu / %lu", cpffSystemTime, doneREQ, totalREQ);
	fflush (stdout);
}