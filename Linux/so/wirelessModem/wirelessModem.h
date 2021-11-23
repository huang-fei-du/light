/*************************************************
Copyright,2010,Huawei Wodian co.,LTD
�ļ�����wirelessModem.h
���ߣ�leiyong
�汾��0.9
�������:2010��01��24��
����������Modem��̬���ӿ�ͷ�ļ�(�û��ӿ�)
      
�޸���ʷ��
  01,10-01-24,leiyong created.
**************************************************/

#ifndef __wirelessModemH
#define __wirelessModemH

#include "common.h"

struct wirelessPara
{
	 INT8U  loginIp[4];        //��¼IP
	 INT16U loginPort;         //��¼�˿�
	 char   apn[17];           //��¼APN
	 char   vpnName[33];       //��¼APN
	 char   vpnPass[33];       //��¼APN
	 INT8U  moduleType;        //ģ������
	 INT8U  *pRecvMsBuf;       //������վ���ݻ���
	 INT32U delay;             //��������ʱ,��ֹ����������̫��ģ����������������
	 INT32U *localIp;          //Modem����IP��ַ
   INT32U *fdOfEthSocket;    //��̫��SOCKET�ļ�������

   void (*reportSignal)(INT8U type,INT8U rssi);
	 void (*sendFrame)(INT8U *pack,INT16U length);
	 void (*debugFrame)(INT8U *pack,INT16U length);
	 void (*portConfig)(INT8U rate);	 
   INT8U (*msInput)(INT8U *pFrame,INT16U dataLength,INT8U dataFrom);
};

//��������
BOOL  initWirelessSo(struct wirelessPara *para);
INT8U configWirelessModem(void);
INT8U wirelessReceive(INT8U *buf,INT16U len);
void  resetAllFlag(INT8U ipStartReset);
void  wlModemRequestSend(INT8U *buf,INT16U len);

//��������ֵ
#define IP_PERMIT        0x1    //��·�Ѿ�����,������IP����
#define LINK_DISCONNECT  0x2    //��·�Ѿ��Ͽ�,��Ҫ���½�����·����
#define NO_CARRIER       0x3    //No Carrier(���ز�)
#define MODEM_RET_FRAME  0x4    //Modem��������֡
#define LINK_FAIL        0x5    //��·����ʧ��
#define REGISTERED_NET   0x6    //ע�ᵽ����

#endif  //__wirelessModemH
