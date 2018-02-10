#include "cpff_static_credit.h"

/**[針對所有Users初始化Credit]
 * @param {userInfo *} user [users' information]
 * @return {int} 0/-1 [success/fail]
 */
 int init_credit(userInfo *user, int totalWeight) {
  //Check the sum of user weights 
  if (totalWeight == 0) {
      print_error(totalWeight, "[static_credit.c]Total User Weight = ");
      return -1;
  }
  int i;
  //Distribute user credit to users by the individual user weight
  for(i = 0; i < NUM_OF_USER; i++) {
    userSSDCredit[i] = INIT_CREDIT * ((double)user[i].globalWeight/totalWeight);
    userHDDCredit[i] = INIT_CREDIT * ((double)user[i].globalWeight/totalWeight);
  }

  //User credit output
  print_credit();
  printf(COLOR_GB" [CREDIT]creditInit() finish!\n"COLOR_RESET);
  return 0;
}


/**
 * [消耗Credit，根據User number]
 * @param {unsigned} userno [User number(0-n)]
 * @param {double} value [消耗量]
 * @param {char *} creditType [判斷是哪種type的credit]
 * @return {double} userCredit [Modified user credit]
 */
 double credit_charge(unsigned userno, double value, char *creditType) {
  //Charge credit
  if(!strcmp("SSDCredit", creditType)) {
    userSSDCredit[userno] -= value;
    return userSSDCredit[userno];
  } else {
    userHDDCredit[userno] -= value;
    return userHDDCredit[userno];
  }
}


/**
 * [印出所有user的credit]
 */
 void print_credit() {
  int i;
  for(i = 0; i < NUM_OF_USER; i++) {
      printf(COLOR_GB" [CREDIT]USER %u SSD CREDIT: %lf\n"COLOR_RESET, i+1, userSSDCredit[i]);
      printf(COLOR_GB" [CREDIT]USER %u HDD CREDIT: %lf\n"COLOR_RESET, i+1, userHDDCredit[i]);
  }
}

