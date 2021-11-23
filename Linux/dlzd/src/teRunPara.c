/***************************************************
Copyright,2009,Huawei WoDian co.,LTD,All	Rights Reserved
�ļ���:teRunPara.c
����:leiyong
�汾:0.9
�������:2009��11��
����:�����ն�(�����ն�/������,AT91SAM9260������)�ն����в����ļ�
�����б�:
  1.
�޸���ʷ:
  01,09-10-28,Leiyong created.

***************************************************/

#include "common.h"
#include "timeUser.h"
#include "workWithMS.h"
#include "teRunPara.h"

DATE_TIME      sysTime;                          //ϵͳʱ��
DATE_TIME      powerOnStatisTime;                //�ϵ�ͳ��ʱ��
BOOL           ifPowerOff = FALSE;               //�Ƿ�ͣ��
POWER_ON_OFF   powerOnOffRecord;                 //ͣ/�ϵ����

INT16U         debugInfo = 0;                    //������Ϣ����
INT16U         debugInfox = 0;                   //������Ϣ����-��չ
INT8U          flagOfClearData=0;                //ɾ�����ݱ�־
INT8U          flagOfClearPulse=0;               //ɾ�������м������ݱ�־

INT32U         workSecond=0;                     //����ʱ��(s)
INT32U         ipStartSecond = 0;                //IP����ʱ��(s)

BOOL           ifReset = FALSE;                  //��λ?
INT8U          numOfDayReset;                    //�ո�λ����
INT8U         moduleType,bakModuleType=NO_MODULE;//ͨ��ģ������

#ifdef CQDL_CSM
 //char           vers[5] = "CQ01";                //����汾
 //char           dispenseDate[11] = "2010-10-31"; //���򷢲�����
 char           vers[5] = "CQ02";                //����汾
 char           dispenseDate[11] = "2014-10-20"; //���򷢲�����
 char           hardwareVers[5]  = "CQ01";       //Ӳ���汾
 char           hardwareDate[11] = "2010-10-31"; //Ӳ����������
#else
 char           vers[5] = "2.20";                //����汾
 char           dispenseDate[11] = "2020-11-19"; //���򷢲�����
 #ifdef PLUG_IN_CARRIER_MODULE
  char          hardwareVers[5]  = "v1.6";       //Ӳ���汾
  char          hardwareDate[11] = "2011-01-05"; //Ӳ����������
 #else
  char          hardwareVers[5]  = "v1.5";       //Ӳ���汾
  char          hardwareDate[11] = "2011-07-20"; //Ӳ����������
 #endif
#endif

char           csNameId[13];                     //�������Ƽ�����
INT8U          localCopyForm = 0;                //����ͨ��ģ�鳭����ʽ
INT8U          denizenDataType = 0;              //�����û������������
INT8U          cycleDataType = 0;                //������������
INT8U          mainTainPortMode = 0;             //ά������ģʽ,Ĭ��Ϊά��
INT8U          rs485Port2Fun = 0;                //��2·485�ӿڹ���,Ĭ��Ϊ����
INT8U          lmProtocol = 0;                   //����ͨ��ģ��Э��,Ĭ��ΪQ/GDW376.2


INT8U          statusOfGate = 0;                 //��բָʾ
INT8U          oneSecondTimeUp=0;                //1�붨ʱ����
INT8U          cmdReset;                         //�յ���λ�����ļ���
INT8U          setParaWaitTime;                  //���ò�����ʾͣ��ʱ��

INT8U          lcdDegree;                        //LCD�Աȶ�ֵ

INT8U        msFrame[SIZE_OF_SEND_MS_BUF];        //���͸���վ��֡
INT8U        activeFrame[SIZE_OF_SEND_MS_BUF];    //�����ϱ�����վ��֡
INT8U        recvMsBuf[SIZE_OF_RECV_MS_BUF];      //������վ֡����(���ؿ��ƶ˿ں�Զ�̶˿ڹ���)

INT8U                    transmitState;           //�����״̬

INT8U                    pfc = 0;                 //����֡֡��ż�����
INT8U                    pSeq = 0;                //����֡���
INT8U                    rSeq = 0;                //��Ӧ֡���

//ETH_ADDR                 addrOfEth;               //��̫����ַ
ADDR_FIELD               addrField;               //��ַ��
FRAME_QUEUE              fQueue;                  //֡����

ANALYSE_FRAME            frame;                   //����֡�ṹ

//״̬��
INT8U                    stOfSwitch;              //״̬��״̬ST
INT8U                    cdOfSwitch;              //״̬����λCD

//�¼�������
INT16U                   iEventCounter;           //��Ҫ�¼�������EC1
INT16U                   nEventCounter;           //һ���¼�������EC2
INT8U                    eventReadedPointer[2];   //�¼���¼������ֵ(����¼�ָ���1�ֽ�)���Ѷ��¼���¼ָ��(��2�ֽ�)

INT8U                    teInRunning=0;           //�ն�Ͷ������(�����Լ)

//ȷ��/����/����Ч���ݶ���
INT8U                    ackData[100];

//��ʱ����ʱ���
REPORT_TIME              reportTime1;             //��ʱ����һ������ʱ���
REPORT_TIME_2            reportTime2;             //��ʱ���Ͷ�������ʱ���

INT8U                    initReportFlag = 0;      //��ʼ��������б�־

//����/��ֹͨ���������ϱ�(��2λ��ʾͨ��,��2λ��ʾ�����ϱ�)
INT8U                    callAndReport;

INT8U                    lastInterval;            //ָ������ʱ�����һ�γ�����

INT8U                    meterTimeOverLoad[8]={0,0,0,0,0,0,0,0};  //���ܱ�ʱ�䳬���־(64��������,ÿ��������ռ1λ)

#ifdef LOAD_CTRL_MODULE
 BOOL                    balanceComplete=FALSE;   //�������
 INT8U                   gateJumpTime[CONTROL_OUTPUT]; //��բ�����ֵ
#endif

#ifdef PULSE_GATHER
 ONE_PULSE pulse[NUM_OF_SWITCH_PULSE];           //�������ɼ�
 INT8U     pulseDataBuff[NUM_OF_SWITCH_PULSE*53];//���������ݻ���
#endif

#ifdef WDOG_USE_X_MEGA
 INT8U                   xMegaFrame[2048];       //���͸�xMega��֡
 XMEGA_FRAME_QUEUE       xMegaQueue;
 INT8U                   gatherModuleType=0;     //�ɼ�ģ������
 INT8U                   xMegaHeartBeat=0;       //��xMega��������
#endif

#ifdef TE_CTRL_BOARD_V_1_3
 INT8U                   gateKvalue = 0;         //�Žӵ�״̬
#endif

INT8U                    acMsa;                  //У���ɵ�MSA

INT8U                    upRtFlag = 0;           //����·�ɱ�־

#ifdef PLUG_IN_CARRIER_MODULE
 INT16U                  adcData = 0;            //ֱ��ģ����ʵʱֵ
#endif

int                      socketOfUdp;            //UDP��socket

