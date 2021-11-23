/*************************************************
Copyright,2010,Huawei Wodian co.,LTD
文件名：wirelessModem.h
作者：leiyong
版本：0.9
完成日期:2010年01月24日
描述：无线Modem动态链接库头文件(用户接口)
      
修改历史：
  01,10-01-24,leiyong created.
**************************************************/

#ifndef __wirelessModemH
#define __wirelessModemH

#include "common.h"

struct wirelessPara
{
	 INT8U  loginIp[4];        //登录IP
	 INT16U loginPort;         //登录端口
	 char   apn[17];           //登录APN
	 char   vpnName[33];       //登录APN
	 char   vpnPass[33];       //登录APN
	 INT8U  moduleType;        //模块类型
	 INT8U  *pRecvMsBuf;       //接收主站数据缓存
	 INT32U delay;             //命令间的延时,防止处理器发送太快模块来不及接收数据
	 INT32U *localIp;          //Modem本端IP地址
   INT32U *fdOfEthSocket;    //以太网SOCKET文件描述符

   void (*reportSignal)(INT8U type,INT8U rssi);
	 void (*sendFrame)(INT8U *pack,INT16U length);
	 void (*debugFrame)(INT8U *pack,INT16U length);
	 void (*portConfig)(INT8U rate);	 
   INT8U (*msInput)(INT8U *pFrame,INT16U dataLength,INT8U dataFrom);
};

//函数声明
BOOL  initWirelessSo(struct wirelessPara *para);
INT8U configWirelessModem(void);
INT8U wirelessReceive(INT8U *buf,INT16U len);
void  resetAllFlag(INT8U ipStartReset);
void  wlModemRequestSend(INT8U *buf,INT16U len);

//函数返回值
#define IP_PERMIT        0x1    //链路已经建立,允许传输IP数据
#define LINK_DISCONNECT  0x2    //链路已经断开,需要重新建立链路连接
#define NO_CARRIER       0x3    //No Carrier(无载波)
#define MODEM_RET_FRAME  0x4    //Modem返回数据帧
#define LINK_FAIL        0x5    //链路连接失败
#define REGISTERED_NET   0x6    //注册到网络

#endif  //__wirelessModemH
