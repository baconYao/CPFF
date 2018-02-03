#ifndef CPFF_IPC_H
#define CPFF_IPC_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "cpff_debug.h"
#include "cpff_structure.h"

	/*STRUCTURE DEFINITION: MESSAGE*/
	typedef struct msgbuf {
		long msgType;
		REQ req;
	} MSGBUF;

	//MESSAGE SIZE
	#define MSG_SIZE sizeof(MSGBUF)-sizeof(long)

	/*CREATE MESSAGE QUEUE*/
	int create_message_queue(key_t key, int msqflag);
	/*REMOVE MESSAGE QUEUE*/
	int remove_message_queue(key_t key, struct msqid_ds *msqds);
	/*SEND REQUEST BY MESSAGE QUEUE*/
	int send_request_by_MSQ(key_t key, REQ *r, long msgtype);
	/*RECEIVE REQUEST BY MESSAGE QUEUE*/
	int recv_request_by_MSQ(key_t key, REQ *r, long msgtype);
	/*******
	EXAMPLE:
	if(sendRequestByMSQ(KEY_MSQ_DISKSIM_2, tmp, MSG_TYPE_DISKSIM_2) == -1)
	    PrintError(-1, "A request not sent to MSQ in sendRequestByMSQ() return:");
	sendSimulateControl(KEY_MSQ_DISKSIM_2, MSG_TYPE_DISKSIM_2);

	if(recvRequestByMSQ(KEY_MSQ_DISKSIM_2, rtn, MSG_TYPE_DISKSIM_2_SERVED) == -1)
	    PrintError(-1, "A served request not received from MSQ in recvRequestByMSQ():");
	*******/

	/*TEST MESSAGE QUEUE*/
	//[測試使用，僅參考，可忽略]
	void test_message_queue();

	/*SEND FINISH CONTROL MESSAGE BY MESSAGE QUEUE*/
  int send_finish_control(key_t key, long msgtype); //Called by rmDisksim()
  
  void init_MSQ();

  void rm_MSQ();

#endif
