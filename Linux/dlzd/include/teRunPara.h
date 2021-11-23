/*************************************************
Copyright,2009,Huawei WoDian co.,LTD
�ļ���:teRunPara.h
����:leiyong
�汾:0.9
�������:2009��10��
����:�����ն�(�����նˡ�������)��վ���ò���ͷ�ļ�
�޸���ʷ:
  01,09-10-28,leiyong created.

**************************************************/

#ifndef __teRunParaH
#define __teRunParaH

#include "common.h"
#include "timeUser.h"
#include "workWithMS.h"

//�ṹ - ͣ/�ϵ����
typedef struct
{
  DATE_TIME           powerOnOffTime;                 //ϵͳ�ϵ�/ͣ��ʱ��
  INT8U               powerOnOrOff;                   //�ϵ绹��ͣ��
}POWER_ON_OFF;

extern DATE_TIME      sysTime;                        //ϵͳʱ��
extern DATE_TIME      powerOnStatisTime;              //�ϵ�ͳ��ʱ��
extern BOOL           ifPowerOff;                     //�Ƿ�ͣ��
extern POWER_ON_OFF   powerOnOffRecord;               //ͣ/�ϵ����

extern INT16U         debugInfo;                      //������Ϣ����
extern INT16U         debugInfox;                     //������Ϣ����-��չ
extern INT8U          flagOfClearData;                //ɾ�����ݱ�־
extern INT8U          flagOfClearPulse;               //ɾ�������м������ݱ�־

extern INT32U         workSecond;                     //����ʱ��(s)
extern INT32U         ipStartSecond;                  //IP����ʱ��(s)

extern BOOL           ifReset;                        //��λ?
extern INT8U          numOfDayReset;                  //�ո�λ����
extern INT8U          moduleType, bakModuleType;      //ͨ��ģ������
extern char           vers[5];                        //����汾
extern char           dispenseDate[11];               //���򷢲�����
extern char           hardwareVers[5];                //Ӳ���汾
extern char           hardwareDate[11];               //Ӳ����������

extern char           csNameId[13];                   //�������Ƽ�����
extern INT8U          localCopyForm;                  //����ͨ��ģ�鳭����ʽ
extern INT8U          denizenDataType;                //�����û������������
extern INT8U          cycleDataType;                  //������������
extern INT8U          mainTainPortMode;               //ά������ģʽ
extern INT8U          rs485Port2Fun;                  //��2·485�ӿڹ���
extern INT8U          lmProtocol;                     //����ͨ��ģ��Э��,Ĭ��ΪQ/GDW376.2

extern INT8U          statusOfGate;                   //��բָʾ
extern INT8U          oneSecondTimeUp;                //1�붨ʱ����
extern INT8U          cmdReset;                       //�յ���λ�����ļ���
extern INT8U          setParaWaitTime;                //���ò�����ʾͣ��ʱ��
extern INT8U          lcdDegree;                      //LCD�Աȶ�ֵ

extern INT8U        msFrame[SIZE_OF_SEND_MS_BUF];     //���͸���վ��֡
extern INT8U        activeFrame[SIZE_OF_SEND_MS_BUF]; //�����ϱ�����վ��֡
extern INT8U        recvMsBuf[SIZE_OF_RECV_MS_BUF];   //������վ֡����(���ؿ��ƶ˿ں�Զ�̶˿ڹ���)

extern INT8U                    transmitState;        //�����״̬

extern INT8U                    pfc;                  //����֡֡��ż�����
extern INT8U                    pSeq;                 //����֡���
extern INT8U                    rSeq;                 //��Ӧ֡���

//extern ETH_ADDR                 addrOfEth;            //��̫����ַ
extern ADDR_FIELD               addrField;            //��ַ��
extern FRAME_QUEUE              fQueue;               //֡����

extern ANALYSE_FRAME            frame;                //����֡�ṹ

//״̬��
extern INT8U                    stOfSwitch;           //״̬��״̬ST
extern INT8U                    cdOfSwitch;           //״̬����λCD

//�¼�������
extern INT16U                   iEventCounter;        //��Ҫ�¼�������EC1
extern INT16U                   nEventCounter;        //һ���¼�������EC2
extern INT8U                    eventReadedPointer[2];//�¼���¼������ֵ(����¼�ָ���1�ֽ�)���Ѷ��¼���¼ָ��(��2�ֽ�)

extern INT8U                    teInRunning;          //�ն�Ͷ������(�����Լ)

//ȷ��/����/����Ч���ݶ���
extern INT8U                    ackData[100];            

//��ʱ����ʱ���
extern REPORT_TIME              reportTime1;          //��ʱ����һ������ʱ���
extern REPORT_TIME_2            reportTime2;          //��ʱ���Ͷ�������ʱ���
extern INT8U                    initReportFlag;       //��ʼ��������б�־

//����/��ֹͨ���������ϱ�(��2λ��ʾͨ��,��2λ��ʾ�����ϱ�)
extern INT8U                    callAndReport;

extern INT8U                    lastInterval;         //ָ������ʱ�����һ�γ�����

extern INT8U                    meterTimeOverLoad[8]; //���ܱ�ʱ�䳬���־(64��������,ÿ��������ռ1λ)

#ifdef LOAD_CTRL_MODULE
 extern BOOL                    balanceComplete;      //�������
 extern INT8U                   gateJumpTime[CONTROL_OUTPUT]; //��բ�����ֵ
#endif

#ifdef PULSE_GATHER
 extern ONE_PULSE pulse[NUM_OF_SWITCH_PULSE];           //�������ɼ�
 extern INT8U     pulseDataBuff[NUM_OF_SWITCH_PULSE*53];//���������ݻ���
#endif

#ifdef WDOG_USE_X_MEGA
 extern INT8U                   xMegaFrame[2048];      //���͸�xMega��֡
 extern XMEGA_FRAME_QUEUE       xMegaQueue;
 extern INT8U                   gatherModuleType;      //�ɼ�ģ������
 extern INT8U                   xMegaHeartBeat;        //��xMega��������
#endif

#ifdef TE_CTRL_BOARD_V_1_3
 extern INT8U                   gateKvalue;            //�Žӵ�״̬
#endif

extern INT8U                    acMsa;

extern INT8U                    upRtFlag;              //����·�ɱ�־

extern INT8U                    _updating_flag;        //����·��������־(1-������,0-δ����,������esRtUpdate.c��)
extern INT8U                    update_step;           //����·������ʱ�Ĳ���(������esRtUpdate.c��)

#ifdef PLUG_IN_CARRIER_MODULE
 extern INT16U                  adcData;               //ֱ��ģ����ʵʱֵ
#endif

extern int                      socketOfUdp;            //UDP��socket

#endif   /*__teRunParaH*/
