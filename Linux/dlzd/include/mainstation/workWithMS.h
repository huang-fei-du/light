/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
�ļ�����workWithMS.h
���ߣ�TianYe
�汾��0.9
������ڣ�2006��5��
����������վ�йصĲ���ͷ�ļ�
�����б�
�޸���ʷ:
  01,06-05-29,Leiyong created.
  02,07-07-27,Leiyong modify,��������վ��֡���ӵ�8192�ֽ�,����Ӧ��վ����������������
             (����,��վҪ����32���������F33,F34�����ݾ���Ҫ�õ���ô��ķ��ͻ���)
  03,09-12-08,Leiyong,��ֲ��AT91SAM9260.
  04,10-12-17,Leiyong,���ݹ��繫˾���Է���:F38,F39 1�ࡢ2�������������óɹ����ٲⲻ����
              ԭ��:ԭ���������ǲ���ȷ��,ֻ����һ������ŵ�λ��,����ԼҪ���������15������ŵ�����,
              ����:��Ϊ��������15������ŵ����á� 
  
***************************************************/
#ifndef __INCworkWithMSh
#define __INCworkWithMSh

#include "common.h"
#include "timeUser.h"

#define PROCESSING              1 //���ڴ�������֡
#define PROCESS_DONE            0 //��Դ���У����Դ����µ�֡

#define CHECK_PW_FAILED         1 //����У��δͨ��
#define CHECK_PW_OK             0 //����У��ͨ��

//ͨ�õ�ַ
#define BROADCASTADDR      0xffff

//AFN���ܴ���
#define RESPONSE              0x0 //ȷ��/����,ֻ������֡�г���
#define RESET_CMD             0x1 //��λ
#define LINK_IF_CHECK         0x2 //��·�ӿڼ��
#define RELAY_STATION_COMMAND 0x3 //�м�վ����
#define SET_PARAMETER         0x4 //��������
#define CTRL_COMMAND          0x5 //��������
#define AUTHENTICATION        0x6 //�����֤����ԿЭ��
#define REQUEST_CASCADE_TE    0x8 //���󱻼����ն������ϱ�
#define REQUEST_TE_CONFIG     0x9 //�����ն�����
#define INQUIRE_INDEX         0xA //��ѯ����
#define INQUIRE_QUEST_INDEX   0xB //��ѯ��������
#define INQUIRE_TYPE_1        0xC //����1������(ʵʱ)
#define INQUIRE_TYPE_2        0xD //����2������(��ʷ)
#define INQUIRE_TYPE_3        0xE //����3������(�¼�)
#define FILE_TRANSPORT        0xF //�ļ�����
#define DATA_FORWARD         0x10 //����ת��

#define FIND_FN               0x1 //����Fn
#define FIND_PN               0x2 //����Pn

#define SIZE_OF_SEND_MS_BUF  9600 //������վ��֡��С(32֡*300Bytes)
#define SIZE_OF_RECV_MS_BUF  2048 //������վ��֡��С
#define LEN_OF_SEND_QUEUE     128 //���Ͷ��г���

//��վ��������?
#define DATA_FROM_LOCAL       0x1 //��վ�������Ա��ؿ���RS232��
#define DATA_FROM_GPRS        0x2 //��վ��������GPRS�ӿ�
#define DATA_FROM_IR          0x3 //��վ�������Ժ���
#define DATA_FROM_RS485_2     0x4 //��վ��������RS485-2��(����Ϊά����)

//Ͷ�п��Ʒ�ʽ
#define LOCAL_CONTRL      0x01
#define REMOTE_CONTRL     0x02
#define LOCK_OFF          0x03
#define LOCK_ON           0x04

//���ݲ�ѯ/�����ϱ���־
#define QUERY             0x00    //���ݲ�ѯ��־
#define ACTIVE_REPORT     0x01    //���������ϱ���־
#define AFN0B_REQUIRE     0x10    //��ѯ����

//��ʱ��������������/ֹͣ��־
#define  TASK_STOP        0xaa    //����ֹͣ��־
#define  TASK_START       0x55    //��������־
#define  TASK_INVALID     0x00    //��������������Ч

#define  RELAY_IN_USE     0x00    //�����м�ת��
#define  RELAY_NO_USE     0x01    //��ֹ�м�ת��

//�ṹ - ��ַ��
typedef struct
{
   INT8U a1[2];                   //����������
   INT8U a2[2];                   //�ն˵�ַ
   INT8U a3;                      //��վ��ַ�����ַ��־
}ADDR_FIELD;

//�ṹ - �¼���¼ͷ
typedef struct
{
	 INT8U     erc;                 //�¼���¼����
	 INT8U     length;              //�洢����
	 DATE_TIME time;                //����ʱ�� 
}EVENT_HEAD;

//���ڱ�����շ����и��ֶ�ֵ��ȫ�ֱ���
typedef struct 
{
  INT16U  l1, l2;                 //֡����(2���ֽںϲ���һ��16������)
  INT8U   c;                      //������
  INT16U  a1, a2;                 //��ַ[a1Ϊ����������,a2Ϊ�ն˵�ַ]
  INT8U   a3;                     //��վ��ַ
  INT8U   *pData;                 //�����ݿ�ʼָ��(afn��ʼ)
  INT8U   *pDataEnd;              //�����ݽ���ָ��
  INT8U   afn;                    //AFN
  INT8U   seq;                    //֡�����
  INT8U   *pw;                    //����ָ��
  INT8U   *pTp;                   //֡ʱ���ǩ��ЧTpָ��
  INT8U   cs;                     //֡У���
  INT8U   acd;                    //�¼���־λ
  INT16U  loadLen;                //���û����ݳ���
}ANALYSE_FRAME;

//�ṹ - ֡����
typedef struct
{
	 BOOL   inTimeFrameSend;        //��ʱ֡����
	 INT8U  thisStartPtr;           //���δ���ָ��
	 INT8U  tailPtr;                //βָ��
	 INT8U  sendPtr;                //����ָ֡��
	 struct
	 {
	   INT16U head;
	   INT16U len;                  //֡����
	   INT8U  next;                 //��һ֡
	 }frame[LEN_OF_SEND_QUEUE];
	 
	 BOOL   continueActiveSend;     //����������һ֡�����ϱ�֡
	 BOOL   activeFrameSend;        //�����ϱ�֡����
	 INT8U  activeThisStartPtr;     //�����ϱ����δ���ָ��
	 INT8U  activeTailPtr;          //�����ϱ�βָ��
	 INT8U  activeSendPtr;          //�����ϱ�����ָ֡��	 
	 struct
	 {
	   INT16U head;
	   INT16U len;                  //֡����
	   INT8U  next;                 //��һ֡
	 }activeFrame[LEN_OF_SEND_QUEUE];
	 
	 INT8U     delay;               //֮֡�����ʱ
	 DATE_TIME delayTime;           //��ʱ���;���ʱ��
	 
	 INT8U     active0dDataSending; //0D�����ϱ��������ڷ��ͱ�־
	 INT8U     active0dTaskId;      //0D�����ϱ���������ID
}FRAME_QUEUE;

#ifdef WDOG_USE_X_MEGA
//�ṹ - xMega֡����
typedef struct
{
	 BOOL   inTimeFrameSend;        //��ʱ֡����
	 INT8U  thisStartPtr;           //���δ���ָ��
	 INT8U  tailPtr;                //βָ��
	 INT8U  sendPtr;                //����ָ֡��
	 struct
	 {
	   INT16U head;
	   INT16U len;                  //֡����
	   INT8U  next;                 //��һ֡
	 }frame[16];
	 
	 INT8U     delay;               //֮֡�����ʱ
}XMEGA_FRAME_QUEUE;
#endif

//�ṹ -- �ն�����ͨ�ſ�ͨ�Ų�������(�ն�ͨ�Ų�������)(AFN04-FN01)
typedef struct
{
  INT8U rts;                			//�ն���������ʱʱ��RTS
  INT8U delay;              			//�ն���Ϊ����վ�����ʹ�����ʱʱ��
  
  INT8U timeOutReSendTimes[2];    //�ն˵ȴ��Ӷ�վ��Ӧ�ĳ�ʱʱ����ط�����
  
  INT8U flagOfCon;          			//��Ҫ��վȷ�ϵ�ͨ�ŷ���(con=1)�ı�־
  INT8U heartBeat;          			//��������
}COMM_PARA;

//�ṹ -- �ն�����ͨ�ſ������м�ת������(�ն��м�ת������)(AFN04-FN02)
typedef struct
{
  INT8U  relayAddrNumFlg;         //��ת�����ն˵�ַ����ʹ�ñ�־
  INT8U  relayAddr[16][2];        //��ת���ն˵�ַ
}RELAY_CONFIG;

//�ṹ �� ��վIP��ַ�Ͷ˿�(AFN04-F3)
typedef struct
{
  INT8U ipAddr[4];                //����IP��ַ
  INT8U port[2];                  //���ö˿�
  INT8U ipAddrBak[4];             //����IP��ַ
  INT8U portBak[2];               //���ö˿�
  INT8U apn[16];                  //APN
}IP_AND_PORT;

//�ṹ - ��վ�绰����Ͷ������ĺ���(AFN04-F4)
typedef struct
{
  INT8U phoneNumber[8];           //��վ�绰�������վ�ֻ�����
  INT8U smsNumber[8];             //�������ĺ���
}PHONE_AND_SMS;

//�ṹ - �ն�IP��ַ�Ͷ˿�(AFN04-07)
typedef struct
{
  INT8U teIpAddr[4];              //�ն�IP��ַ
  INT8U mask[4];                  //���������ַ
  INT8U gateWay[4];               //���ص�ַ
  INT8U proxyType;                //��������
  INT8U proxyServer[4];           //�����������ַ
  INT8U proxyPort[2];             //����������˿�
  INT8U proxyLinkType;            //������������ӷ�ʽ
  INT8U userNameLen;              //�û�������m
  INT8U userName[20];             //�û���
  INT8U passwordLen;              //���볤��n
  INT8U password[20];             //����
  INT8U listenPort[2];            //�ն������˿�
  INT8U ethIfLoginMs;             //�Ƿ�ʹ����̫����¼��վ
}TE_IP_AND_PORT;

//�ṹ - �ն�����ͨ�Ź�����ʽ(��̫ר��������ר��)(AFN04-08)
typedef struct
{
  INT8U workMethod;               //����ģʽ
  INT8U redialInterval[2];        //�������ߡ�ʱ������ģʽ�ز����
  INT8U maxRedial;                //��������ģʽ�ز�����
  INT8U closeConnection;          //��������ģʽ������ͨ���Զ�����ʱ��
  INT8U onLinePeriodTime[3];      //ʱ������ģʽ��������ʱ�α�־
}PRIVATE_NET_METHOD;

//�ṹ--�ն��¼���¼����(AFN04-F9)
typedef struct
{
  INT8U  nEvent[8];               //�¼���¼��Ч��־λ(һ���¼�)
  INT8U  iEvent[8];	              //�¼���Ҫ�Եȼ���־λ(��Ҫ�¼�)
}EVENT_RECORD_CONFIG;

//�ṹ--�ն˵��ܱ�/��������װ�����ò���(AFN04-F10)
//�ýṹ��ʾһ������Ϣ
typedef struct
{
  INT16U number;                  //���ܱ�/��������װ�����
  INT16U measurePoint;            //�����������
  INT8U  rateAndPort;             //ͨ�����ʼ��˿ں�
  INT8U  protocol;                //ͨ�Ź�Լ����
  INT8U  addr[6];                 //ͨ�ŵ�ַ
  INT8U  password[6];             //ͨ������
  INT8U  numOfTariff;             //���ʸ���
  INT8U  mixed;                   //�й�����ʾֵ����λ��С��λ
  INT8U  collectorAddr[6];        //�����ɼ���ͨ�ŵ�ַ
  INT8U  bigAndLittleType;        //�û����༰�û�С���
}METER_DEVICE_CONFIG;

//�ṹ--�ն��������ò���(AFN04-011)
typedef struct
{
  INT8U     numOfPulse;           //�������ø���
  struct
  {
    INT8U ifNo;                   //��������˿ں�
    INT8U pn;                     //�����������
    INT8U character;              //��������
    INT8U meterConstant[2];       //�����k
  }perPulseConfig[NUM_OF_SWITCH_PULSE];
  //��ԼҪ����64·����,�����ǵļ�����ֻ��2·����������
}PULSE_CONFIG;

//�ṹ--��ѹ/����ģ�������ò���(AFN04-FN13)
typedef struct
{
  INT16U  numOfSimu;
  struct
  {
    INT8U ifNo;						        //��ѹ/����ģ��������˿ں�
    INT8U pn;							        //�����������
    INT8U character;			        //��ѹ/����ģ��������
  }perIUConfig[64];
}IU_SIMULATE_CONFIG;

//�ṹ--�ն��ܼ������ò���(AFN04-FN14)
typedef struct
{
	INT8U   numberOfzjz;            //�ܼ�������
	struct
  {
	  INT8U zjzNo;                  //�ܼ������
	  INT8U pointNumber;            //���ܼ������������
    INT8U measurePoint[64];       //ÿ�������㼰�ܼӱ�־
  }perZjz[8];                     //ÿ���ܼ���������
}TOTAL_ADD_GROUP;

//�ṹ--�������Խ������(AFN04-FN15)
typedef struct
{
  INT16U     numOfConfig;				  //�й��ܵ����������������
  struct
  {
    INT16U  groupNum;					    //�й��ܵ�����������
    INT8U   toCompare;					  //�Աȵ��ܼ������
    INT8U   toReference;				  //���յ��ܼ������
    INT8U   timeAndFlag;				  //�����ĵ�������ʱ�����估�Աȷ�����־
    INT8U   ralitaveDifference;   //�Խ�����ƫ��ֵ
    INT8U   absoluteDifference[4];//�Խ�޾���ƫ��ֵ
       
    INT8U   startStop;            //Խ����ֹ��־(���ǹ�Լ�涨,�Ǽ�¼������)
  }perConfig[8];//�ṹ--����������Խ������
   
}ENERGY_DIFFERENCE_CONFIG;

//�ṹ - ����ר���û���������(AFN04-FN16)
typedef struct
{
	INT8U vpnName[32];              //����ר���û���
	INT8U vpnPassword[32];          //����ר������
}VPN;

//�ṹ -- ���Ʋ���(AFN04-FN18,FN19,FN20)
typedef struct
{
  INT8U  pCtrlPeriod[12];         //�ն˹���ʱ��
  INT8U  pCtrlIndex;              //ʱ�ι��ض�ֵ����ϵ��
  INT8U  monthEnergCtrlIndex;     //�ն��µ������ض�ֵ����ϵ��
}CONTRL_PARA;

//�ṹ - �ն˵���������(AFN04-FN22)
typedef struct
{
  INT8U chargeNum;        		    //������
  INT8U chargeRate[48][4];        //����
}CHARGE_RATE_NUM;

//�ṹ--�������������(AFN04-F25)
typedef struct
{
  INT16U voltageTimes;            //��ѹ����������
  INT16U currentTimes;            //��������������
  INT16U ratingVoltage;           //���ѹ
  INT8U  maxCurrent;              //�����
  INT8U  powerRating[3];          //�����
  INT8U  linkStyle;               //��Դ���߷�ʽ
}MEASURE_POINT_PARA;

//�ṹ--���������Ʋ���(AFN04-026)
typedef struct
{
  INT16U   vUpLimit;              //��ѹ�ϸ�����
  INT16U   vLowLimit;             //��ѹ�ϸ�����
  INT16U   vPhaseDownLimit;       //��ѹ��������
     
  INT16U   vSuperiodLimit;        //��ѹ������(��ѹ����)
  INT8U    vUpUpTimes;            //��ѹ������Խ�޳���ʱ��
  INT8U    vUpUpResume[2];        //��ѹ������Խ�޻ָ�ϵ��
     
  INT16U   vDownDownLimit;        //��ѹ������(Ƿѹ����)
  INT8U    vDownDownTimes;        //��ѹ������Խ�޳���ʱ��
  INT8U    vDownDownResume[2];    //��ѹ������Խ�޻ָ�ϵ��
  
  INT8U    cSuperiodLimit[3];     //�����������(��������
  INT8U    cUpUpTimes;            //�����������Խ�޳���ʱ��
  INT8U    cUpUpReume[2];         //�����������Խ�޻ָ�ϵ��
     
  INT8U    cUpLimit[3];           //���������(���������)
  INT8U    cUpTimes;              //���������Խ��ʱ��
  INT8U    cUpResume[2];          //���������Խ�޻ָ�ϵ��
     
  INT8U    cZeroSeqLimit[3];      //�����������
  INT8U    cZeroSeqTimes;         //�������Խ�޳���ʱ��
  INT8U    cZeroSeqResume[2];     //�������Խ�޻ָ�ϵ��
  
  INT8U    pSuperiodLimit[3];     //���ڹ���������
  INT8U    pSuperiodTimes;        //���ڹ���Խ�����޳���ʱ��
  INT8U    pSuperiodResume[2];    //���ڹ���Խ�����޻ָ�ϵ��
     
  INT8U    pUpLimit[3];           //���ڹ�������
  INT8U    pUpTimes;              //���ڹ���Խ���޳���ʱ��
  INT8U    pUpResume[2];          //���ڹ���Խ���޻ָ�ϵ��
        
  INT8U    uPhaseUnbalance[2];    //�����ѹ��ƽ����ֵ
  INT8U    uPhaseUnTimes;         //�����ѹ��ƽ��Խ�޳���ʱ��
  INT8U    uPhaseUnResume[2];     //�����ѹ��ƽ��Խ�޻ָ�ϵ��
  
  INT8U    cPhaseUnbalance[2];    //���������ƽ����ֵ
  INT8U    cPhaseUnTimes;         //���������ƽ��Խ�޳���ʱ��
  INT8U    cPhaseUnResume[2];     //���������ƽ��Խ�޻ָ�ϵ��
  
  INT8U    uLostTimeLimit;        //����ʧѹʱ����ֵ
 
}MEASUREPOINT_LIMIT_PARA;

#ifdef LIGHTING

//�ṹ--���Ƶ����Ʋ���(AFN04-052)
typedef struct
{
  INT16U   vSuperiodLimit;        //��ѹ��ѹ����
  INT16U   vDownDownLimit;        //��ѹǷѹ����

  INT8U    cSuperiodLimit[3];     //������������
  INT8U    cDownDownLimit[3];     //����Ƿ������
  
  INT8U    pUpLimit[3];           //��������
  INT8U    pDownLimit[3];         //��������
  
  INT8U    factorDownLimit[2];		//������������
  
  INT8U    overContinued;         //Խ�޳���ʱ��(��)
}PN_LIMIT_PARA;

#endif

//�ṹ - ������ͭ���������(AFN04-027)
typedef struct
{
	INT8U aResistance[2];           //A�����
	INT8U aReactance[2];            //A��翹
	INT8U aConductance[2];          //A��絼
	INT8U aSusceptance[2];          //A�����

	INT8U bResistance[2];           //B�����
	INT8U bReactance[2];            //B��翹
	INT8U bConductance[2];          //B��絼
	INT8U bSusceptance[2];          //B�����

	INT8U cResistance[2];           //C�����
	INT8U cReactance[2];            //C��翹
	INT8U cConductance[2];          //C��絼
	INT8U cSusceptance[2];          //C�����
}COPPER_IRON_LOSS;

//�ṹ - �����㹦�������ֶ���ֵ(AFN04-F28)
typedef struct
{
  INT8U  segLimit1[2];						//���������ֶ���ֵ1
  INT8U  segLimit2[2];						//���������ֶ���ֵ2
}POWER_SEG_LIMIT;

//�ṹ - �ز��ӽڵ㸽���ڵ��ַ(AFN04-F31)
typedef struct
{
  INT8U numOfAuxiliaryNode;       //�ز��ӽڵ㸽���ڵ��ַ����
	INT8U auxiliaryNode[120];       //�ز��ӽڵ㸽���ڵ��ַ
}AUXILIARY_ADDR;

//�ṹ - �ն˳������в�������(AFN04-F33)
typedef struct
{
  INT8U  numOfPara;               //���������
  struct
  {
    //��ԼҪ���·��Ĳ���
    INT8U commucationPort;        //�ն�ͨ���˿ں�
    INT8U copyRunControl[2];      //̨�����г������п�����
    INT8U copyDay[4];             //������-����
    INT8U copyTime[2];            //����ʱ��
    INT8U copyInterval;           //������
    INT8U broadcastCheckTime[3];  //�㲥Уʱ
    INT8U hourPeriodNum;          //������ʱ����m(0<=m<=24)
    INT8U hourPeriod[48][2];      //������ʱ��
 #ifdef SUPPORT_ETH_COPY
  }para[6];                       //2014-4-17,����һ��ETH����˿�(�˿�Ϊ5)
 #else
  }para[5];	                      //����������(������ֻ��5���ӿ�,��������ֻ֧��5�����ݿ�),   //2012-3-27,��4���ӿڱ��5��
 #endif
}TE_COPY_RUN_PARA;

//�ṹ - ����������ͨ��ģ��Ĳ�������(AFN04-F34)
typedef struct
{
  INT8U  numOfPara;    //��������
  struct
  {
 	  INT8U commucationPort;        //�ն�ͨ�Ŷ˿ں�
 	  INT8U commCtrlWord;           //���ն˽ӿڶ˵�ͨ�ſ�����
 	  INT8U commRate[4];            //���ն˽ӿڶ�Ӧ�˵�ͨ������	 	  
  }para[32];  //��������
}DOWN_RIVER_MODULE_PARA;

//�ṹ - ̨�����г����ص㻧����(AFN04-F35)
typedef struct
{
  INT8U  numOfHousehold;          //�ص㻧����
  INT8U  household[40];           //�ص㻧���װ�����
}KEY_HOUSEHOLD;

//�ṹ - �ն˼���ͨ�Ų���(AFN04-037)
typedef struct
{
  INT8U commPort;                 //�ն˼���ͨ�Ŷ˿ں�
  INT8U ctrlWord;                 //�ն˼���ͨ�ſ�����
  INT8U receiveMsgTimeout;        //���ձ��ĳ�ʱʱ��
  INT8U receiveByteTimeout;       //���յȴ��ֽڳ�ʱʱ��
  INT8U cascadeMretryTime;        //������(����վ)����ʧ���ط�����
  INT8U groundSurveyPeriod;       //����Ѳ������
  INT8U flagAndTeNumber;          //������־���ն˸���
  INT8U divisionCode[6];          //�ն�����������
  INT8U cascadeTeAddr[6];         //�����ն˵�ַ
}CASCADE_COMM_PARA;

//�ṹ - 1�༰2��������������(AFN04-038,039)
//10-12-17,Leiyong,���ݹ��繫˾���Է���:F38,F39 1�ࡢ2�������������óɹ����ٲⲻ����
//             ԭ��:ԭ���������ǲ���ȷ��,ֻ����һ������ŵ�λ��,����ԼҪ���������16������ŵ�����,
//             ����:��Ϊ��������16������ŵ����á�
typedef struct
{
  struct
  {
    INT8U groupNum;               //�ô����µ�С��Ÿ���
    struct
    {
	    INT8U infoGroup;            //��Ϣ������
	    INT8U flag[32];             //��Ϣ��Ԫ��־λ
    }littleType[16];	            //С���16��,�û�С����������±��ʾ
  }bigType[16];                   //�����16��,�û�������������±��ʾ
}TYPE_1_2_DATA_CONFIG;

//�ṹ--����Ͷ��״̬
typedef struct
{
  INT8U       ifUsePrdCtrl;       //ʱ�ο�Ͷ��
  INT8U       ifUseWkdCtrl;       //���ݿ�Ͷ��
  INT8U       ifUseObsCtrl;       //��ͣ��Ͷ��
  INT8U       ifUsePwrCtrl;       //��ǰ�����¸���Ͷ��
  INT8U       ifUseMthCtrl;       //�µ��Ͷ��
  INT8U       ifUseChgCtrl;       //�����Ͷ��
}CTRL_RUN_STATUS;

//�ṹ - �������״̬
typedef struct
{
	INT8U  aQueue[8];               //�澯����
	INT8U  pn[8];                   //�ܼ����
	INT8U  allPermitClose[8];       //�����բ�����趨����·��բ
	INT8U  numOfAlarm;              //�澯���еĳ�Ա����
	INT8U  nowAlarm;                //��ǰ�Ķ���ָ��

}CTRL_STATUS;

//�ṹ--�ն˱���״̬
typedef struct
{
  INT8U      ifStaySupport;       //����״̬
  INT8U      ifHold;              //һֱ����
  DATE_TIME  endStaySupport;      //�������ʱ��
}STAY_SUPPORT_STATUS;

//�ṹ--ң�ز���
typedef struct
{
  //ң�ز���
  INT8U      ifUseRemoteCtrl;     //ң��Ͷ��,ly,2011-08-08,add
  INT8U      ifRemoteCtrl;        //�Ƿ�ң��״̬
  INT8U      ifRemoteConfirm;     //�Ƿ�У�ɹ�
  DATE_TIME  confirmTimeOut;      //��Уʱ��
  DATE_TIME  alarmStart;          //�澯ʱ��
  DATE_TIME  remoteStart;         //ң�ؿ�ʼʱ��
  DATE_TIME  remoteEnd;           //ң�ؽ���ʱ��
  INT8U      status;              //״̬(1.�澯,2.����բ,3.�����բ)  2008.12.05 ly added
}REMOTE_CTRL_CONFIG;

//�ṹ--ʱ�ι��ض�ֵ������(AFN04-FN41��AFN05-FN009,etc)
typedef struct
{
  //F41���ض�ֵ
  INT8U      periodNum;           //ʱ�η�����־
  struct
  {
    INT8U    timeCode;            //ʱ�ζ�ֵ��־
    INT8U    limitTime[8][2];
  }period[3];
    
  //ʱ�οؿ��Ʋ���
  struct
  {
    INT8U     limitPeriod;        //ʱ�ι���Ͷ���־
    INT8U     ctrlPeriod;         //ʱ�ι��ض�ֵ������
    INT8U     ifPrdCtrl;          //�Ƿ�ʱ�ι���״̬
    INT8U     prdAlarm;           //ʱ�ι��ظ澯״̬(1.�澯,2.����բ,3.�����բ)
    DATE_TIME alarmEndTime;       //�澯����ʱ��
  }ctrlPara;
}PERIOD_CTRL_CONFIG;

//�ṹ--���ݹ��ز���(AFN04-FN42)
typedef struct
{
  INT8U      ifWkdCtrl;           //�Ƿ��ݿ�״̬
  INT8U      wkdAlarm;            //���ݿظ澯״̬(1.�澯,2.����բ,3.�����բ)
  INT16U     wkdLimit;            //���ݿع�����ֵ  
  INT8U      wkdStartMin;         //������ʼʱ��
  INT8U      wkdStartHour;
  INT8U      wkdTime;             //�޵�ʱ��
  INT8U      wkdDate;             //�޵��ܴ�
  DATE_TIME  alarmEndTime;        //�澯����ʱ��
}WKD_CTRL_CONFIG;

//�ṹ--���ʿ��ƵĹ��ʼ��㻬��ʱ��(F43)
typedef struct
{
  INT8U     def;
  INT8U     countTime;	
}POWERCTRL_COUNT_TIME;

//�ṹ--Ӫҵ��ͣ�ز���(AFN04-FN44)
typedef struct
{
  INT8U      ifObsCtrl;           //�Ƿ�ͣ��״̬
  INT8U      obsAlarm;            //��ͣ���Ƹ澯״̬(1.�澯,2.����բ,3.�����բ)
  INT16U     obsLimit;            //��ͣ�ع�����ֵ
  INT8U      obsStartDay;         //������ʼʱ����
  INT8U      obsStartMonth;       //������ʼʱ����
  INT8U      obsStartYear;        //������ʼʱ����
  INT8U      obsEndDay;           //���ƽ���ʱ����
  INT8U      obsEndMonth;         //���ƽ���ʱ����
  INT8U      obsEndYear;          //���ƽ���ʱ����
  DATE_TIME  alarmEndTime;        //�澯����ʱ��
}OBS_CTRL_CONFIG;

//�ṹ--��ǰ�����¸��ز���(�������������AFN04�ж����,�������¸���Ͷ��(AFN05)ʱ�·���,��Ϊ����������ڹ��ʿ��Ʋ���һ��)
typedef struct
{
  INT8U      ifPwrDownCtrl;       //�Ƿ��¸���״̬
  INT8U      pwrDownAlarm;        //��ͣ���Ƹ澯״̬(1.�澯,2.����բ,3.�����բ)
	  
	INT8U      slipTime;            //��ǰ�����¸��ض�ֵ����ʱ��
	INT8U      floatFactor;         //��ǰ�����¸��ض�ֵ����ϵ��
	INT8U      freezeDelay;         //�غ��ܼ��й����ʶ�����ʱʱ��
	  
	INT32U     powerDownLimit;      //���涨ֵKw
	INT16U     powerLimitWatt;      //���涨ֵwatt
	       
	DATE_TIME  freezeTime;          //���ᵱʱ���ɵ�ʱ��	  
	DATE_TIME  alarmEndTime;        //�澯����ʱ��
	  
	INT8U      downCtrlTime;        //��ǰ�����¸��Ŀ���ʱ��
	INT8U      roundAlarmTime[4];   //��ǰ�����¸��ظ澯ʱ��(��1�ֵ���4��)
	 
}POWER_DOWN_CONFIG;

//�ṹ--�����ִ��趨(F45)
typedef struct
{
  INT8U     ifJumped;             //�Ƿ���բ(BS8,��λ��λ)
  INT8U     flag;
}POWERCTRL_ROUND_FLAG;

//�ṹ--�µ����ض�ֵ(AFN04-FN46)
typedef struct
{
  INT8U      ifMonthCtrl;         //�Ƿ��µ��״̬
  INT8U      monthAlarm;          //�Ƿ��޸澯
  INT32U     monthCtrl;           //�µ�ص�����ֵ
  DATE_TIME  mthAlarmTimeOut;     //�µ�ظ澯��ʱ
}MONTH_CTRL_CONFIG;

//�ṹ--�������ز���(AFN04-FN47)
typedef struct
{
  //���Ʋ���
  INT8U      ifChargeCtrl;        //�Ƿ񹺵��״̬
  INT8U      chargeAlarm;         //���������Ƹ澯״̬(1.�澯,2.����բ,3.�����բ)
  DATE_TIME  alarmTimeOut;        //�澯ʱ��
  
  //���һ���·��Ĺ������ز���
  INT8U      flag;                //׷��ˢ�±�־
  INT32U     numOfBill;           //���絥��
  INT32U     chargeCtrl;          //������ֵ
  INT32U     alarmLimit;          //�澯����
  INT32U     cutDownLimit;        //��բ����
}CHARGE_CTRL_CONFIG;

//�ṹ--����ִ��趨(F48)
typedef struct
{
  INT8U     ifJumped;
  INT8U     flag;
}ELECTCTRL_ROUND_FLAG;

//�ṹ--���ظ澯ʱ��(F49)
typedef struct
{
  INT8U     def;
  INT8U     alarmTime;
}POWERCTRL_ALARM_TIME;

//ң���ֳ���¼
typedef struct
{
	INT8U      remotePower[2];
	INT8U      gate;
	DATE_TIME  remoteTime;
	DATE_TIME  freezeTime;
}REMOTE_EVENT_INFOR;

//�����ֳ���¼
typedef struct
{
  INT8U   numOfCtrl;              //�����¼�δ��¼��
  struct
  {
    INT8U      pn;                //�ܼ����
    DATE_TIME  ctrlTime;          //������բʱ��
    INT8U      gate;              //������բ�ִ�
    INT8U      ctrlType;          //��������
    INT8U      ctrlPower[2];      //��բǰ(�ܼ�)����
    INT8U      limit[2];          //��բʱ���ʶ�ֵ
    DATE_TIME  freezeTime;        //Ӧ����¼�¼�ʱ��
  }perCtrl[6];
}POWER_CTRL_EVENT_INFOR;

//�ṹ - ���ܱ��쳣�б���ֵ�趨(AFN04-F59)
typedef struct
{
	INT8U powerOverGate;            //������������ֵ
	INT8U meterFlyGate;             //���ܱ������ֵ
	INT8U meterStopGate;            //���ܱ�ͣ����ֵ
	INT8U meterCheckTimeGate;       //���ܱ�Уʱ��ֵ
}METER_GATE;

//�ṹ--г����ֵ(AFN04-FN60)
typedef struct
{
  INT8U totalUPercentUpLimit[2];	//�ܻ����ѹ����������
    
  INT8U oddUPercentUpLimit[2];		//���г����ѹ����������
  INT8U evenUPercentUpLimit[2];		//ż��г����ѹ����������
    
  INT8U UPercentUpLimit[18][2];		//г����ѹ����������
    
  INT8U totalIPercentUpLimit[2];	//�ܻ��������Чֵ����
    
  INT8U IPercentUpLimit[18][2];		//г��������Чֵ����
}WAVE_LIMIT;

//�ṹ--��ʱ�������ݲ�������(AFN04-F65,F66,F67,F68)  F65,F67����F65  F66,F68����F68
typedef struct
{
  INT8U       numOfTask;          //�������
  struct
  {
    INT8U     stopFlag;           //������/ֹͣ��־
 
    INT8U     taskNum;            //�������(֧��1~64������)
    INT8U     sendPeriod;         //�������ڼ��������ڵ�λ
    INT8U     week;							  //���������е�����
    DATE_TIME sendStart;          //���ͻ�׼ʱ��
    INT8U     sampleTimes;        //�������ݳ�ȡ����
    INT16U    numOfDataId;        //���ݵ�Ԫ��ʾ����
    struct
    {
      INT8U   fn;                 //���ݵ�Ԫfn
      INT8U   pn1;                //���ݵ�Ԫpn
      INT8U   pn2;
    }dataUnit[1024];
 }task[64];    //������������
}REPORT_TASK_PARA;

//�ṹ--����������(AFN04-FN73, FN76)
typedef struct
{
  struct
  {
	  INT8U compensationMode;				//������ʽ
	  INT8U capacityNum[2];					//����װ������
  }capacity[16];

  INT8U controlMode;						  //���Ʒ�ʽ(FN76)
}CAPACITY_PARA;

//�ṹ--������Ͷ�����в���(AFN04-FN74)
typedef struct
{
  INT8U  targetPowerFactor[2];		//Ŀ�깦������
  INT8U  onPowerLimit[3];					//Ͷ���޹���������
  INT8U  offPowerLimit[3];				//�г��޹���������
  INT8U  delay;										//��ʱʱ��
  INT8U  actInterval;							//����ʱ����
}CAPACITY_RUN_PARA;

//�ṹ--��������������(AFN04-FN75)
typedef struct
{
  INT8U vSuperiod[2];							//����ѹ
  INT8U vSuperiodQuan[2];					//����ѹ�ز�ֵ
  INT8U vLack[2];									//Ƿ��ѹ
  INT8U vLackQuan[2];							//Ƿ��ѹ�ز�ֵ
  INT8U cAbnormalUpLimit[2];			//�ܻ����������������
  INT8U cAbnormalQuan[2];					//�ܻ������������Խ�޻ز�ֵ
  INT8U vAbnormalUpLimit[2];			//�ܻ����ѹ����������
  INT8U vAbnormalQuan[2];					//�ܻ����ѹ������Խ�޻ز�ֵ
}CAPACITY_PROTECT_PARA;

//ֱ��ģ�������ò���(AFN04-FN81,FN82,FN83)
typedef struct
{
  //FN81
  INT8U adcStartValue[2];			    //ֱ��ģ����������ʼֵ
  INT8U adcEndValue[2];				    //ֱ��ģ����������ֵֹ
 
  //FN82
  INT8U adcUpLimit[2];  	        //ֱ��ģ��������
  INT8U adcLowLimit[2];	          //ֱ��ģ��������
 
  //FN83 
  INT8U adcFreezeDensity;         //ֱ��ģ���������ܶ�(ʵ��ʹ�ó���1�ֽ�)
}ADC_PARA;

//�ṹ--��ʱ����1��������������ʱ���
typedef struct
{
  INT8U  numOfTask;
  struct
  {
    INT8U      taskNum;
    DATE_TIME  nextReportTime;    //���������ʱ��
  }taskConfig[64];
}REPORT_TIME;

//�ṹ--��ʱ����2��������������ʱ���
typedef struct
{
  INT8U numOfTask;
  struct
  {
    INT8U      taskNum;
    DATE_TIME  nextReportTime;    //������ʱ�� 
    DATE_TIME  lastReportTime;    //������ʱ��
    DATE_TIME  backLastReportTime;//������ʱ��
  }taskConfig[64];
}REPORT_TIME_2;

//�ṹ--F92���ֱ��<��չ>
typedef struct
{
  INT8U numOfPoint;               //����������
  struct
  {
    INT8U measurePoint;           //�������
    INT8U dlNum[12];              //���ֱ��
  }meterDevice[39];
}DL_NUMBER;

//�ṹ - ��������У��ֵ����
typedef struct
{
  INT8U   lineInType;             //���߷�ʽ(1-��������,2-��������,Ĭ��Ϊ��������)
  INT16U  vAjustTimes;            //��ѹУ��ʹ�õĵ�ѹ�Ŵ���
  INT16U  cAjustTimes;            //����У��ʹ�õĵ����Ŵ���

  INT32U  UADCPga;                //��ѹͨ������Ĵ���ֵ
  INT32U  HFConst;                //��Ƶ������������Ĵ���ֵ
  INT32U  HFDouble;               //���峣���ӱ������Ĵ���ֵ
  INT32U  Irechg;                 //�Ȳ������Ĵ���ֵ
  INT32U  EAddMode;               //���������ۼ�ģʽ�Ĵ���ֵ
  INT32U  EnLineFreq;             //��������ʹ�ܿ��ƼĴ���ֵ
  INT32U  EnHarmonic;             //����������г�������л��Ĵ���ֵ

  INT32U  FailVoltage;            //��ѹ������ֵ
  INT32U  Istartup;               //������������

  INT32U  Iregion1;               //��λ�����������üĴ���ֵ1
  INT32U  Iregion2;               //��λ�����������üĴ���ֵ2
  INT32U  Iregion3;               //��λ�����������üĴ���ֵ3
  INT32U  Iregion4;               //��λ�����������üĴ���ֵ4
  INT32U  PgainA;                 //��������Ĵ���ֵA
  INT32U  PgainB;                 //��������Ĵ���ֵB
  INT32U  PgainC;                 //��������Ĵ���ֵC
  INT32U  PhsreagA;               //��λ�����Ĵ���ֵA
  INT32U  PhsreagB;               //��λ�����Ĵ���ֵB
  INT32U  PhsreagC;               //��λ�����Ĵ���ֵC
  INT32U  UgainA;                 //��ѹУ���Ĵ���ֵA
  INT32U  UgainB;                 //��ѹУ���Ĵ���ֵB
  INT32U  UgainC;                 //��ѹУ���Ĵ���ֵC
  INT32U  IgainA;                 //����У���Ĵ���ֵA
  INT32U  IgainB;                 //����У���Ĵ���ֵB
  INT32U  IgainC;                 //����У���Ĵ���ֵC
}AC_SAMPLE_PARA;

//�ṹ--������Ϣ
typedef struct
{
	INT8U numOfMessage;             //��Ϣ����
	struct
	{
	  INT8U typeAndNum;             //��Ϣ�������Ϣ���
	  INT8U len;                    //��Ϣ����
	  char  chn[202];               //������Ϣ
	}message[8];
}CHN_MESSAGE;

//�ṹ--ת����¼
typedef struct
{
  INT8U     port;                 //ת��ͨ�Ŷ˿ں�
  INT8U     ctrlWord;             //ת��ͨ�ſ�����
  INT8U     frameTimeOut;         //ת�����յȴ����ĳ�ʱʱ��
  INT8U     byteTimeOut;          //ת�����յȴ��ֽڳ�ʱʱ��
  INT8U     length;               //ת������
  INT8U     forwardData[255];     //ת������ָ��
  INT8U     dataFrom;
  BOOL      ifSend;
  
  DATE_TIME nextBytesTime;        //�յ��ֽں�ĵȴ�ʱ��
  BOOL      receivedBytes;        //���յ��ظ�����
  INT8U     data[1024];           //���ݻ���
  INT16U    recvLength;           //���ճ���
}FORWARD_STRUCT;

//�ṹ - �������ɼ�
typedef struct
{
	 BOOL      ifPlugIn;            //�Ƿ����
	 
   INT8U     pn;                  //�����������
   INT8U     character;           //��������
   INT16U    meterConstant;       //�����k

   INT16U    voltageTimes;        //��ѹ����������
   INT16U    currentTimes;        //��������������
	 
	 INT16U    pulseCount;          //��·�������
	 INT16U    pulseCountTariff[14];//��·����ַ��ʼ���
	 
	 INT8U     pulseValue;          //��·����IO�ŵ�ǰֵ
	 INT8U     pulseValueBak;       //��·����IO��ǰһ��ֵ
	 INT8U     pulseValueBak2;      //��·����IO��ǰǰһ��ֵ
	 
	 INT8U     findPulse;           //��������

	 INT16U    prevMinutePulse;     //ǰ1�����������
	 INT16U    minutePulse;         //1�����������
	 struct    timeval calcTv;      //��һ�μ���ʱ��
}ONE_PULSE;

#ifdef LIGHTING

//�ṹ--���ƿ�����/��·����������ʱ��(AFN04-F50)
struct ctrlTimes
{
	
  INT8U            startMonth;    //ʱ�����ʼ�·�
  INT8U            startDay;      //ʱ�����ʼ��
  INT8U            endMonth;      //ʱ��ν����·�
  INT8U            endDay;        //ʱ��ν�����
  
  INT8U            deviceType;    //����������
                                  //  2-���ƿ�����ʱ��
                                  //  3-��·������ʱ��
                                  //  5-��γ�ȿ���������
                                  //  7-���նȿ���
  
  INT8U            noOfTime;      //ʱ�κ�(��һ��ʱ����е�ʱ�κ�)
  
  INT8U            workDay;       //������,2016-08-17,Add
  
  INT8U            hour[6];       //ʱ��1-6ʱ
  INT8U            min[6];        //ʱ��1-6��
  INT8U            bright[6];     //ʱ��1-6���Ȼ���ͨ��
  
  struct ctrlTimes *next;         //��һ�ڵ�ָ��
  
};

//�ṹ - ���Ƶ���ֵ�趨(AFN04-F51)
typedef struct
{
	INT8U failureRetry;         //���ƿ��������ֹ������Դ���
	INT8U boardcastWaitGate;    //�㲥����ȴ�ʱ��
	INT8U checkTimeGate;        //Уʱ��ֵ
	INT8U lddOffGate;           //���ƿ��Ƶ�������ֵ
	INT8U lddtRetry;            //����ĩ�����Դ���
	INT8U offLineRetry;         //�������Դ���
	INT8U lcWave;               //����ն���ֵ
	INT8U leakCurrent;          //©������ֵ
}PN_GATE;

#endif

#endif /*__INCworkWithMSh*/
