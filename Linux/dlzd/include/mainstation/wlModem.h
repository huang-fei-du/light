/*************************************************
Copyright,2010,Huawei Wodian co.,LTD
�ļ���:wlModem.h
���ߣ�leiyong
�汾��0.9
�������:2010��1��
����:����Modem��ش���ͷ�ļ�
�޸���ʷ��
  01,10-01-24,leiyong created.
**************************************************/
#ifndef __wlModemH
#define __wlModemH

#include "common.h"

#define  WAIT_MODEM_MINUTES    5        //�ȴ�Modem�ظ���ʱʱ��
#define  RE_LOGIN_TIMES      100        //�ص�¼����

//����Modem���Ʊ�־��
typedef struct
{
  unsigned int numOfWlReset   :8;        //��λ����Modem����
  unsigned int useMainIp      :1;        //ʹ����IP��ַ
  unsigned int lastIpSuccess  :1;        //�ϴ�IP��¼�ɹ�

	unsigned int power          :2;        //��Դ�����ػ�״̬
	unsigned int permitSendData :1;        //������IP����
  unsigned int loginSuccess   :1;        //�ն˵�¼��վ�Ƿ�ɹ�
  unsigned int logoutSuccess  :1;        //�ն��˳���¼��վ�Ƿ�ɹ�		
	unsigned int heartSuccess   :1;        //�����Ƿ�ɹ�
	unsigned int sendLoginFrame :1;        //�Ƿ��͵�¼֡
		
  unsigned int ethRecvThread  :1;        //��̫�������߳��Ƿ񴴽�
  	
  unsigned int sendToModem    :1;        //��֡������ģ��,�ȴ�Modem�ظ�
  DATE_TIME    waitModemRet;             //Modemδ�ظ����ݵĵȴ���ʱʱ��
  
  unsigned int numOfRetryTimes:3;        //��¼���������Դ���
  
}WL_MODEM_FLAG;

//�ⲿ����
//����
extern WL_MODEM_FLAG wlModemFlag;        //����Modem���Ʊ�־
extern DATE_TIME     lastHeartBeat;      //�ϴ�����ʱ��
extern DATE_TIME     nextHeartBeatTime;  //��һ������ʱ��
extern INT32U        wlLocalIpAddr;      //����Modem���ն�IP��ַ(GPRS/CDMA�Ȼ�õ�IP��ַ)

extern DATE_TIME     waitTimeOut;        //�ȴ���ʱʱ��(����¼֡������֡���ͺ�ĳ�ʱʱ��,�����ʱҪ����͵Ķ���)

extern INT8U         operateModem;       //���Բ���modem? 2012-11-7
extern INT8U         dialerOk;           //dialer����Ok?  2012-11-7

//��������
void  wlModem(void);
void  setModemSoPara(void);
void  *threadOfTtys0Receive(void *arg);
INT8U checkIfCascade(void);
void  resetPppWlStatus(void);
void  *threadOfRecvUdpData(void *arg);
INT8U sendToDialer(unsigned char afn, unsigned char *pData, unsigned char lenOfData);

#endif  //__wlModemH