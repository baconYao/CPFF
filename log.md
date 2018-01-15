# CPFF開發日誌


## 20180115
### configs 資料夾
* 建立queue structure
* 建立user structure
* 建立函式
  * [QUE *] build_host_queue();
  * [QUE *] build_user_queue(int userNum, char *qType);
  * [bool] insert_req_to_host_que(QUE *hostQ, REQ *r);
  * [void] remove_req_from_queue_head(QUE *Que);
  * [bool] insert_req_to_user_que(userInfo *user, char *qType, REQ *r);
  * [unsigned long] get_total_reqs();
  * [void] copyReq(REQ *r, REQ *copy);
  * [bool] is_empty_queue(QUE *Que);
  * [void] print_queue_content(QUE *Que);

### main 程式
* 建立初始化 initialize 函式
  * 讀取trace到host queue
  * 初始化user infomation (包含weight、user queue etc...)
