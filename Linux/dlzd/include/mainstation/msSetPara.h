/*************************************************
Copyright,2009,Huawei WoDian co.,LTD
�ļ���:common.h
����:leiyong
�汾:0.9
�������:2009��10��
����:�����ն�(�����նˡ�������)��վ���ò���ͷ�ļ�
�޸���ʷ:
  

**************************************************/
#ifndef __MsSetParaH
#define __MsSetParaH

#include "common.h"
#include "workWithMS.h"

extern ANALYSE_FRAME 					  frame;									 //���ڱ�����շ����и��ֶ�ֵ��ȫ�ֱ���

extern COMM_PARA                commPara;                //�ն�ͨ�Ų���(AFN04-FN01)
extern RELAY_CONFIG             relayConfig;             //�ն��м�ת������(AFN04-FN02)
extern IP_AND_PORT              ipAndPort;               //��վIP��ַ�Ͷ˿�(AFN04-FN03)
extern PHONE_AND_SMS            phoneAndSmsNumber;       //��վ�绰����Ͷ������ĺ���(AFN04-FN04)
extern INT8U                    messageAuth[3];          //�ն���Ϣ��֤(AFN04-FN05)
extern INT8U                    groupAddr[32];           //�ն����ַ(AFN04-FN6)

extern TE_IP_AND_PORT           teIpAndPort;             //�ն�IP��ַ�Ͷ˿�(AFN04-FN7)
extern PRIVATE_NET_METHOD       tePrivateNetMethod;      //�ն�����ͨ�Ź�����ʽ(AFN04-FN8)
extern EVENT_RECORD_CONFIG      eventRecordConfig;       //�ն��¼���¼��������(AFN04-FN9)
extern INT16U									  meterDeviceNum;				   //���ܱ�/��������װ����������
extern INT8U 									  statusInput[2];		       //�ն�״̬���������(AFN04-FN12)

extern PULSE_CONFIG             pulseConfig;             //�ն��������ò���(AFN04-FN11)
extern IU_SIMULATE_CONFIG       simuIUConfig;            //��ѹ/����ģ�������ò���(AFN04-FN13)
extern TOTAL_ADD_GROUP          totalAddGroup;           //�ն��ܼ������ò���(AFN04-FN14)
extern ENERGY_DIFFERENCE_CONFIG differenceConfig;        //�������Խ������(AFN04-FN15)
extern VPN                      vpn;                     //����ר���û���������(AFN04-FN16)
extern INT8U                    protectLimit[2];         //�ն˱�����ֵ(AFN04-FN17)
extern CONTRL_PARA              ctrlPara;                //�ն˹���ʱ�Σ�ʱ�ι��ظ���ϵ�����µ������ض�ֵ����ϵ��(AFN04-FN18,FN19,FN20)

extern INT8U                    periodTimeOfCharge[49];  //�ն˵��������ʺͷ�����(AFN04-021)

extern CHARGE_RATE_NUM          chargeRateNum;           //�ն˵���������(AFN04-022)

extern INT8U                    chargeAlarm[3];          //�ն˴߷Ѹ澯����(AFN04-023)

extern TE_COPY_RUN_PARA         teCopyRunPara;           //�ն˳������в�������(AFN04-FN33)

extern DOWN_RIVER_MODULE_PARA   downRiverModulePara;		 //����������ͨ��ģ��Ĳ�������(AFN04-FN34)
extern KEY_HOUSEHOLD						keyHouseHold;						 //̨�����г����ص㻧����(AFN04-FN35)
extern INT8U       						  upTranslateLimit[4];		 //�ն�����ͨ��������������(AFN04-FN36)
extern CASCADE_COMM_PARA        cascadeCommPara;         //�ն˼���ͨ�Ų���
extern TYPE_1_2_DATA_CONFIG     typeDataConfig1;         //�ն˼���ͨ�Ų���(AFN04-FN38)
extern TYPE_1_2_DATA_CONFIG     typeDataConfig2;         //�ն˼���ͨ�Ų���(AFN04-FN39)

//���Ʋ�����
extern STAY_SUPPORT_STATUS      staySupportStatus;                //�ն˱���״̬
extern INT8U                    reminderFee;                      //�߷Ѹ澯״̬
extern INT8U                    toEliminate;                      //�޳�״̬                  
extern CTRL_RUN_STATUS          ctrlRunStatus[8];                 //�ն�Ͷ��״̬
extern WKD_CTRL_CONFIG          wkdCtrlConfig[8];                 //���ݹ��ز���(AFN04-FN42)
extern POWERCTRL_COUNT_TIME     powerCtrlCountTime[8];            //���ʿ��ƵĹ��ʼ��㻬��ʱ��(AFN04-FN43)
extern OBS_CTRL_CONFIG          obsCtrlConfig[8];                 //Ӫҵ��ͣ�ز���(AFN04-FN44)
extern POWER_DOWN_CONFIG        powerDownCtrl[8];                 //��ǰ�����¸��ز��� 
extern POWERCTRL_ROUND_FLAG     powerCtrlRoundFlag[8];            //�����ִ��趨(AFN04-FN45)
extern MONTH_CTRL_CONFIG        monthCtrlConfig[8];               //�µ����ض�ֵ(AFN04-FN46)
extern CHARGE_CTRL_CONFIG       chargeCtrlConfig[8];              //�������ز���(AFN04-FN47)
extern ELECTCTRL_ROUND_FLAG     electCtrlRoundFlag[8];            //����ִ��趨(AFN04-FN48)

extern POWERCTRL_ALARM_TIME     powerCtrlAlarmTime[8];            //���ظ澯ʱ��(AFN04-FN49)

extern REMOTE_CTRL_CONFIG       remoteCtrlConfig[CONTROL_OUTPUT]; //ң�ز���
extern REMOTE_EVENT_INFOR       remoteEventInfor[8];              //ң�����ݶ������
extern PERIOD_CTRL_CONFIG       periodCtrlConfig[8];              //ʱ�ι��ض�ֵ(AFN04-FN41),ʱ�ι��ز���
extern CTRL_STATUS              ctrlStatus;                       //����״̬
extern POWER_CTRL_EVENT_INFOR   powerCtrlEventInfor;              //�����ֳ�(�¼�)��Ϣ

extern INT8U                    voiceAlarm[3];                    //�ն������澯����//��ֹ����(AFN04-FN57)
extern INT8U                    noCommunicationTime;              //������ͨ��ʱ��(AFN04-058)
extern METER_GATE               meterGate;                        //���ܱ��쳣�б���ֵ�趨(AFN04-FN59)
extern WAVE_LIMIT               waveLimit;                        //г����ֵ(AFN04-FN60)
extern INT8U 									  adcInFlag;                        //ֱ��ģ�����������(AFN04-FN61)

extern REPORT_TASK_PARA         reportTask1, reportTask2;         //��ʱ����1�࣬2��������������(AFN04-FN65,FN66)

extern CHN_MESSAGE              chnMessage;                       //������Ϣ

//��Լ��չ
extern INT8U                    teName[20];                       //�ն�����(AFN04-FN97)

extern INT8U                    sysRunId[20];                     //ϵͳ���б�ʶ��(AFN04-FN98)[�����Լ]
extern INT8U                    assignCopyTime[6];                //�ն˳�����������ʱ��(AFN04-FN99)[�����Լ]
extern INT8U                    teApn[4][16];                     //�ն�Ԥ��apn(AFN04-FN100)[�����Լ]
extern INT8U                    rlPara[4];                        //[�ֵ���չ]���ģ�����,�ֽ�1-��������,�ֽ�2-�����,�ֽ�3-�ź�ǿ��,�ֽ�4-�ŵ�
extern AC_SAMPLE_PARA           acSamplePara;                     //����У������
extern INT8U                    mainNodeAddr[6];                  //����ͨ��ģ��(�ز�/����)���ڵ��ַ
extern INT16U                   deviceNumber;                     //�豸���

extern INT8U                    paraStatus[31];                   //�ն˲���״̬
extern INT8U                    eventStatus[8];                   //�¼���־״̬

extern char *paraName[132];  //��������
extern char *ctrlName[42];   //��������

#ifdef LIGHTING

 extern PN_GATE                 pnGate;                           //���Ƶ���ֵ
 extern INT8U                   ctrlMode;                         //����ģʽ
 extern INT8U                   beforeOnOff[4];                   //�����ǰ-�ӳٷ�����Ч
 
 extern INT16U                  lcHeartBeat;                      //�����������ֵ��������
 
 extern INT16U                  statusHeartBeat;                  //���ơ���·���Ƶ�״̬������������,2016-11-24,Add

#endif

#endif /*__MsSetParaH*/
