/*************************************************
Copyright,2010,Huawei Wodian co.,LTD
文件名:wlModem.h
作者：leiyong
版本：0.9
完成日期:2010年1月
描述:无线Modem相关处理头文件
修改历史：
  01,10-01-24,leiyong created.
**************************************************/
#ifndef __wlModemH
#define __wlModemH

#include "common.h"

#define  WAIT_MODEM_MINUTES    5        //等待Modem回复超时时间
#define  RE_LOGIN_TIMES      100        //重登录次数

//无线Modem控制标志集
typedef struct
{
  unsigned int numOfWlReset   :8;        //复位无线Modem次数
  unsigned int useMainIp      :1;        //使用主IP地址
  unsigned int lastIpSuccess  :1;        //上次IP登录成功

	unsigned int power          :2;        //电源及开关机状态
	unsigned int permitSendData :1;        //允许传输IP数据
  unsigned int loginSuccess   :1;        //终端登录主站是否成功
  unsigned int logoutSuccess  :1;        //终端退出登录主站是否成功		
	unsigned int heartSuccess   :1;        //心跳是否成功
	unsigned int sendLoginFrame :1;        //是否发送登录帧
		
  unsigned int ethRecvThread  :1;        //以太网接收线程是否创建
  	
  unsigned int sendToModem    :1;        //有帧发送至模块,等待Modem回复
  DATE_TIME    waitModemRet;             //Modem未回复数据的等待超时时间
  
  unsigned int numOfRetryTimes:3;        //登录、心跳重试次数
  
}WL_MODEM_FLAG;

//外部变量
//变量
extern WL_MODEM_FLAG wlModemFlag;        //无线Modem控制标志
extern DATE_TIME     lastHeartBeat;      //上次心跳时间
extern DATE_TIME     nextHeartBeatTime;  //下一次心跳时间
extern INT32U        wlLocalIpAddr;      //无线Modem本终端IP地址(GPRS/CDMA等获得的IP地址)

extern DATE_TIME     waitTimeOut;        //等待超时时间(发登录帧及心跳帧发送后的超时时间,如果超时要做相就的动作)

extern INT8U         operateModem;       //可以操作modem? 2012-11-7
extern INT8U         dialerOk;           //dialer拨号Ok?  2012-11-7

//函数声明
void  wlModem(void);
void  setModemSoPara(void);
void  *threadOfTtys0Receive(void *arg);
INT8U checkIfCascade(void);
void  resetPppWlStatus(void);
void  *threadOfRecvUdpData(void *arg);
INT8U sendToDialer(unsigned char afn, unsigned char *pData, unsigned char lenOfData);

#endif  //__wlModemH