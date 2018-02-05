#include "cpff_ipc.h"

/**
 * [根據指定的Key值和Message Queue Flag建立]
 * @param {key_t} key [Key值]
 * @param {int} msqflag [Message Queue Flag]
 * @return {int} 0/-1 [Success(0) or Fail(-1)]
 */
int create_message_queue(key_t key, int msqflag) {
	int msqid = -1;
	msqid = msgget(key, msqflag | 0666);
	if(msqid >= 0) {
		//PrintDebug(msqid, "Message Queue Identifier:");
		return msqid;
	}
	else {
		return -1;
  }
}


/**
 * [根據指定的Key值和Message Queue Flag刪除]
 * @param {key_t} key [Key值]
 * @param {struct msqid_ds*} msqds [Message Queue Data Structure]
 * @return {int} 0/-1 [Success(0) or Fail(-1)]
 */
int remove_message_queue(key_t key, struct msqid_ds *msqds) {
	int msqid;
	msqid = msgget((key_t)key, IPC_CREAT);
	if(msgctl(msqid, IPC_RMID, msqds) == 0) {
		//PrintDebug(msqid, "Remove message queue:");
		return msqid;
	}
	else {
		return -1;
  }
}


/**
 * [根據指定的Key值代表特定的Message Queue和Message Type傳送Request]
 * @param {key_t} key [Key值]
 * @param {REQ*} r [Request]
 * @param {long} msgtype [Message Queue Type]
 * @return {int} 0/-1 [Success(0) or Fail(-1)]
 */
int send_request_by_MSQ(key_t key, REQ *r, long msgtype) {
	int msqid;
	msqid = msgget((key_t)key, IPC_CREAT);

	MSGBUF msg;
	msg.msgType = msgtype;
	msg.req.arrivalTime = r->arrivalTime;
  msg.req.devno = r->devno;
  msg.req.diskBlkno = r->diskBlkno;
  msg.req.reqSize = r->reqSize;
  msg.req.reqFlag = r->reqFlag;
  msg.req.userno = r->userno;
  msg.req.responseTime = r->responseTime;
	if(msgsnd(msqid, (void *)&msg, MSG_SIZE, 0) == 0) {
		//PrintDebug(msqid, "A request sent to MSQ:");
		return 0;
	}
	else {
		return -1;
  }
}


/**
 * [根據指定的Key值代表特定的Message Queue和Message Type接收Request]
 * @param {key_t} key [Key值]
 * @param {REQ*} r [Request]
 * @param {long} msgtype [Message Queue Type]
 * @return {int} 0/-1 [Success(0) or Fail(-1)]
 */
int recv_request_by_MSQ(key_t key, REQ *r, long msgtype) {
	int msqid;
	msqid = msgget((key_t)key, IPC_CREAT);

	MSGBUF buf;
	if(msgrcv(msqid, (void *)&buf, MSG_SIZE, msgtype, 0) >= 0) {/*return bytes sent*/
		//PrintDebug(msqid, "A request received from MSQ:");
		r->arrivalTime = buf.req.arrivalTime;
		r->devno = buf.req.devno;
		r->diskBlkno = buf.req.diskBlkno;
		r->reqSize = buf.req.reqSize;
		r->reqFlag = buf.req.reqFlag;
		r->userno = buf.req.userno;
		r->responseTime = buf.req.responseTime;
		return 0;
	}
	else {
		return -1;
  }
}

/**
 * [測試使用，僅參考，可忽略]
 */
void test_message_queue() {
	//data structure for msg queue
	struct msqid_ds ds;
	if(create_message_queue(KEY_MSQ_DISKSIM_1, IPC_CREAT) == -1) {
		print_error(-1, " MSQ create error in create_message_queue():");
  }

	REQ *r;
	r = calloc(1, sizeof(REQ));
	r->arrivalTime = 0.00;
	r->devno = 0;
	r->diskBlkno = 2495568;
	r->reqSize = 8;
	r->reqFlag = 0;
	r->userno = 0;
	r->responseTime = -1;

	print_REQ(r ,"To sddsim in testMessageQueue()");

	if(send_request_by_MSQ(KEY_MSQ_DISKSIM_1, r, MSG_TYPE_DISKSIM_1) == -1) {
    print_error(-1, "A request not sent to MSQ in send_request_by_MSQ() return:");
	}

	REQ *rp = NULL;
	rp = calloc(1, sizeof(REQ));
	if(recv_request_by_MSQ(KEY_MSQ_DISKSIM_1, rp, MSG_TYPE_DISKSIM_1) == -1) {
    print_error(-1, "A request not received from MSQ in recvRequestByMSQ():");
  }
	
	print_REQ(rp ,"from sddsim in testMessageQueue()");
	
	if(remove_message_queue(KEY_MSQ_DISKSIM_1, &ds) == -1) {
		print_error(KEY_MSQ_DISKSIM_1, "Not remove message queue:(key)");
  }
	
	if(create_message_queue(KEY_MSQ_DISKSIM_2, IPC_CREAT) == -1) {
		print_error(-1, " MSQ create error in create_message_queue() return:");
  }
	if(remove_message_queue(KEY_MSQ_DISKSIM_2, &ds) == -1) {
		print_error(KEY_MSQ_DISKSIM_2, "Not remove message queue:(key)");
  }
}

/**
 * [根據指定的Key值代表特定的Message Queue和Message Type傳送一Message(帶有特定Flag的Request)，其代表告知simulator應進行Shutdown之工作]
 * @param {key_t} key [Key值]
 * @param {long} msgtype [Message Queue Type]
 * @return {int} 0/-1 [Success(0) or Fail(-1)]
 */
int send_finish_control(key_t key, long msgtype) {
	REQ *ctrl;
  ctrl = calloc(1, sizeof(REQ));
  ctrl->reqFlag = MSG_REQUEST_CONTROL_FLAG_FINISH;
  if(send_request_by_MSQ(key, ctrl, msgtype) == -1) {
    print_error(key, "A control request not sent to MSQ in send_request_by_MSQ() return:");
    return -1;
  }
  else {
    return 0;
  }
}


/**
 * [Message queue初始化，使用系統定義的Key值、Type和IPC function]
 */
void init_MSQ() {
  //Create message queue for SSD simulator
  if(create_message_queue(KEY_MSQ_DISKSIM_1, IPC_CREAT) == -1) {
    print_error(-1, " MSQ create error in createMessageQueue():");
  }
  //Create message queue for HDD simulator
  if(create_message_queue(KEY_MSQ_DISKSIM_2, IPC_CREAT) == -1) {
    print_error(-1, " MSQ create error in createMessageQueue():");
  }
}

/**
 * [Message queue刪除，使用系統定義的Key值和IPC function]
 */
 void rm_MSQ() {
  struct msqid_ds ds;
  //Remove message queue for SSD simulator
  if(remove_message_queue(KEY_MSQ_DISKSIM_1, &ds) == -1) {
    print_error(KEY_MSQ_DISKSIM_1, "Not remove message queue:(key)");
  }
  //Remove message queue for HDD simulator
  if(remove_message_queue(KEY_MSQ_DISKSIM_2, &ds) == -1) {
    print_error(KEY_MSQ_DISKSIM_2, "Not remove message queue:(key)");
  }
}


/**
 * [根據Message Queue傳送Request給SSDsim或HDDsim，等待回傳Service time]
 * @param {key_t} key [根據SSDsim或HDDsim的Message Queue之Key值]
 * @param {long} msgtype [指定Message Queue]
 * @param {REQ*} r [欲處理的Request]
 * @return {double} service [Service Time]
 */
double get_service_time(key_t key, long msgtype, REQ *r) {
	//Send IO request
	if(send_request_by_MSQ(key, r, msgtype) == -1) {
		print_error(-1, "A request not sent to MSQ in send_request_by_MSQ() return:");
		
	}

	//Statistics
	// pcst.totalReq++;
	// userst[r->userno-1].totalReq++;
	// sysst.totalReq++;
	// if (key == KEY_MSQ_DISKSIM_1) {
	// 		pcst.ssdReq++;
	// 		userst[r->userno-1].ssdReq++;
	// 		sysst.ssdReq++;
	// }

	double service = -1;    //Record service time

	//For SSDsim
	if (key == KEY_MSQ_DISKSIM_1) {
		REQ *rtn;
		rtn = calloc(1, sizeof(REQ));

		//Receive serviced request
		if(recv_request_by_MSQ(key, rtn, MSG_TYPE_DISKSIM_1_SERVED) == -1)
				print_error(-1, "[PC]A request not received from MSQ in recv_request_by_MSQ():");

		//Record service time
		service = rtn->responseTime;

		//Release request variable and return service time
		free(rtn);
		return service;
	}
	else if (key == KEY_MSQ_DISKSIM_2) {//For HDDsim
		REQ *rtn;
		rtn = calloc(1, sizeof(REQ));

		//Receive serviced request
		if(recv_request_by_MSQ(key, rtn, MSG_TYPE_DISKSIM_2_SERVED) == -1)
				print_error(-1, "[PC]A request not received from MSQ in recv_request_by_MSQ():");

		//Record service time
		service = rtn->responseTime;

		//Release request variable and return service time
		free(rtn);
		return service;
	}
	else {
		print_error(-1, "Send/Receive message with wrong key");
		
	}

	//Return service time
	return service;
}


