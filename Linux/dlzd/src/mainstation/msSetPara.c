/***************************************************
Copyright,2009,Huawei WoDian co.,LTD,All	Rights Reserved
�ļ���:msSetPara.c
����:leiyong
�汾:0.9
�������:2009��11��
����:�����ն�(�����ն�/������,AT91SAM9260������)��վ���ò����ļ�
�����б�:
  1.
�޸���ʷ:
  01,09-10-27,Leiyong created.

***************************************************/

#include "common.h"
#include "workWithMS.h"
//#include "meterProtocol.h"

ANALYSE_FRAME						 frame;										//���ڱ�����շ����и��ֶ�ֵ��ȫ�ֱ���

COMM_PARA                commPara;                //�ն�ͨ�Ų���(AFN04-FN01)
RELAY_CONFIG             relayConfig;             //�ն��м�ת������(AFN04-FN02)
IP_AND_PORT              ipAndPort;               //��վIP��ַ�Ͷ˿�(AFN04-FN03)
PHONE_AND_SMS            phoneAndSmsNumber;       //��վ�绰����Ͷ������ĺ���(AFN04-FN04)
INT8U                    messageAuth[3];          //�ն���Ϣ��֤(AFN04-FN05)
INT8U                    groupAddr[16];           //�ն����ַ(AFN04-FN6)

TE_IP_AND_PORT          teIpAndPort;             //�ն�IP��ַ�Ͷ˿�(AFN04-FN7)

PRIVATE_NET_METHOD      tePrivateNetMethod;      //�ն�����ͨ�Ź�����ʽ(AFN04-FN8)
EVENT_RECORD_CONFIG     eventRecordConfig;       //�ն��¼���¼��������(AFN04-FN9)
INT16U									 meterDeviceNum;					//���ܱ�/��������װ����������(AFN04-FN10֮����)
INT8U 									 statusInput[2];		      //�ն�״̬���������(AFN04-FN12)

PULSE_CONFIG             pulseConfig;             //�ն��������ò���(AFN04-FN11)
IU_SIMULATE_CONFIG       simuIUConfig;            //��ѹ/����ģ�������ò���(AFN04-FN13)
TOTAL_ADD_GROUP          totalAddGroup;           //�ն��ܼ������ò���(AFN04-FN14)
ENERGY_DIFFERENCE_CONFIG differenceConfig;        //�������Խ������(AFN04-FN15)
VPN                      vpn;                     //����ר���û���������(AFN04-FN16)
INT8U                    protectLimit[2];         //�ն˱�����ֵ(AFN04-FN17)

CONTRL_PARA              ctrlPara;                //�ն˹���ʱ�Σ�ʱ�ι��ظ���ϵ�����µ������ض�ֵ����ϵ��(AFN04-FN18,FN19,FN20)

INT8U                   periodTimeOfCharge[49];  //�ն˵��������ʺͷ�����(AFN04-021)
CHARGE_RATE_NUM         chargeRateNum;           //�ն˵���������(AFN04-022)

TE_COPY_RUN_PARA         teCopyRunPara;          //�ն˳������в�������(AFN04-FN33)

DOWN_RIVER_MODULE_PARA   downRiverModulePara;		//����������ͨ��ģ��Ĳ�������(AFN04-FN34)
KEY_HOUSEHOLD						keyHouseHold;						//̨�����г����ص㻧����(AFN04-FN35)
INT8U       						  upTranslateLimit[4];			//�ն�����ͨ��������������(AFN04-FN36)
CASCADE_COMM_PARA        cascadeCommPara;         //�ն˼���ͨ�Ų���(AFN04-FN37)
TYPE_1_2_DATA_CONFIG     typeDataConfig1;          //�ն˼���ͨ�Ų���(AFN04-FN38) 
TYPE_1_2_DATA_CONFIG     typeDataConfig2;          //�ն˼���ͨ�Ų���(AFN04-FN39)

INT8U                    chargeAlarm[3];          //�ն˴߷Ѹ澯����(AFN04-023)

//���Ʋ�����
STAY_SUPPORT_STATUS      staySupportStatus;                //�ն˱���״̬
INT8U                    reminderFee;                      //�߷Ѹ澯״̬
INT8U                    toEliminate;                      //�޳�״̬                  
CTRL_RUN_STATUS          ctrlRunStatus[8];                 //�ն�Ͷ��״̬
WKD_CTRL_CONFIG          wkdCtrlConfig[8];                 //���ݹ��ز���(AFN04-FN42)
POWERCTRL_COUNT_TIME     powerCtrlCountTime[8];            //���ʿ��ƵĹ��ʼ��㻬��ʱ��(AFN04-FN43)
OBS_CTRL_CONFIG          obsCtrlConfig[8];                 //Ӫҵ��ͣ�ز���(AFN04-FN44)
POWER_DOWN_CONFIG        powerDownCtrl[8];                 //��ǰ�����¸��ز��� 
POWERCTRL_ROUND_FLAG     powerCtrlRoundFlag[8];            //�����ִ��趨(AFN04-FN45)
MONTH_CTRL_CONFIG        monthCtrlConfig[8];               //�µ����ض�ֵ(AFN04-FN46)
CHARGE_CTRL_CONFIG       chargeCtrlConfig[8];              //�������ز���(AFN04-FN47)
ELECTCTRL_ROUND_FLAG     electCtrlRoundFlag[8];            //����ִ��趨(AFN04-FN48)

POWERCTRL_ALARM_TIME     powerCtrlAlarmTime[8];            //���ظ澯ʱ��(AFN04-FN49)

REMOTE_CTRL_CONFIG       remoteCtrlConfig[CONTROL_OUTPUT]; //ң�ز���
REMOTE_EVENT_INFOR       remoteEventInfor[8];              //ң�����ݶ������
PERIOD_CTRL_CONFIG       periodCtrlConfig[8];              //ʱ�ι��ض�ֵ(AFN04-FN41),ʱ�ι��ز���
CTRL_STATUS              ctrlStatus;                       //����״̬
POWER_CTRL_EVENT_INFOR   powerCtrlEventInfor;              //�����ֳ�(�¼�)��Ϣ

INT8U                    voiceAlarm[3];                    //�ն������澯����//��ֹ����(AFN04-FN57)
INT8U                    noCommunicationTime;              //������ͨ��ʱ��(AFN04-058)
METER_GATE               meterGate;                        //���ܱ��쳣�б���ֵ�趨(AFN04-FN59)
WAVE_LIMIT               waveLimit;                        //г����ֵ(AFN04-FN60)
INT8U 									 adcInFlag;                        //ֱ��ģ�����������(AFN04-FN61)

//VNET_WORK_METHOD         vnetMethod;                       //����ר�����繤����ʽ(AFN04-FN62)

REPORT_TASK_PARA         reportTask1, reportTask2;         //��ʱ����1�࣬2��������������(AFN04-FN65,FN66)

CHN_MESSAGE              chnMessage;                       //������Ϣ

//��Լ��չ
INT8U                    teName[20];                       //�ն�����(AFN04-FN97)

INT8U                    sysRunId[21];                     //ϵͳ���б�ʶ��(AFN04-FN98)[�����Լ]
INT8U                    assignCopyTime[6];                //�ն˳�����������ʱ��(AFN04-FN99)[�����Լ]
INT8U                    teApn[4][16];                     //�ն�Ԥ��apn(AFN04-FN100)[�����Լ]
 
 INT8U                   rlPara[4];                        //[�ֵ���չ]���ģ�����,�ֽ�1-��������,�ֽ�2-�����,�ֽ�3-�ź�ǿ��,�ֽ�4-�ŵ�

AC_SAMPLE_PARA           acSamplePara;                     //����У������
INT8U                    mainNodeAddr[6];                  //����ͨ��ģ��(�ز�/����)���ڵ��ַ
INT16U                   deviceNumber;                     //�豸���


INT8U                    paraStatus[31];                   //�ն˲���״̬
INT8U                    eventStatus[8];                   //�¼���־״̬

#ifdef LIGHTING
 PN_GATE                 pnGate;                           //���Ƶ���ֵ
 
 INT8U                   ctrlMode;                         //����ģʽ
                                                           //  1-ʱ�ο�
                                                           //  2-���
                                                           //  3-���+ʱ�ο�
                                                           //  4-��γ�ȿ�

 INT8U                   beforeOnOff[4];                   //�����ǰ-�ӳٷ�����Ч
 
 INT16U                  lcHeartBeat=0;                    //�����������ֵ��������,2016-11-21,Add
 
 INT16U                  statusHeartBeat=0;                //���ơ���·���Ƶ�״̬������������,2016-11-24,Add
#endif


//��������
char *paraName[132] = 
  { 
	  "�ն�ͨ�Ų�������",                     //F1
	  "�ն��м�ת������",                     //F2
	  "��վIP��ַ�Ͷ˿�",                     //F3
	  "��վ�绰����Ͷ������ĺ���",           //F4
	  "�ն���Ϣ��֤��������",                 //F5
	  "�ն����ַ����",                       //F6
	  "�ն˳���������",                       //F7
	  "�ն��¼���¼��������",                 //F8
	  "�ն�����������",                       //F9
	  "�ն˵��ܱ���������װ�����ò���",     //F10
	  "�ն��������ò���",                     //F11
	  "�ն�״̬���������",                   //F12
	  "�ն˵�ѹ������ģ�������ò���",         //F13
	  "�ն��ܼ������ò���",                   //F14
	  "�й��ܵ������Խ���¼���������",     //F15
	  "����ר���û���������",                 //F16
	  "�ն˱�����ֵ",                         //F17
	  "�ն˹���ʱ��",                         //F18
	  "�ն�ʱ�ι��ض�ֵ����ϵ��",             //F19
	  "�ն��µ������ض�ֵ����ϵ��",           //F20
	  "�ն˵���������ʱ�κͷ�����",           //F21
	  "�ն˵��ܷ���",                         //F22
	  "�ն˴߷Ѹ澯����",                     //F23
	  "�ն˳���������",                     //F24
	  "�������������",                       //F25
	  "��������ֵ����",                       //F26
	  "���������ݶ������",                   //F27
	  "�����㹦�������ֶ���ֵ",               //F28
	  "",                                     //F29
	  "",                                     //F30
	  "",                                     //F31
	  "",                                     //F32
	  "�ܼ������ݶ������",                   //F33
	  "",                                     //F34
	  "",                                     //F35
	  "",                                     //F36
	  "",                                     //F37
	  "",                                     //F38
	  "",                                     //F39
	  "",                                     //F40
	  "ʱ�ι��ض�ֵ",                         //F41
	  "���ݹ��ز���",                         //F42
	  "���ʿ��ƵĹ��ʼ��㻬��ʱ��",           //F43
	  "Ӫҵ��ͣ�ز���",                       //F44
	  "�������ִ��趨",                       //F45
	  "�µ����ض�ֵ",                         //F46
	  "������(��)�ز���",                     //F47
	  "����ִ��趨",                         //F48
	  "���ظ澯ʱ��",                         //F49
	  "",                                     //F50
	  "",                                     //F51
	  "",                                     //F52
	  "",                                     //F53
	  "",                                     //F54
	  "",                                     //F55
	  "",                                     //F56
	  "�ն������澯������ֹ����",           //F57
	  "�ն��Զ��������",                     //F58
	  "���ܱ��쳣�б���ֵ�趨",               //F59
	  "г����ֵ",                             //F60
	  "ֱ��ģ�����������",                   //F61
	  "����ר��������ʽ",                     //F62
	  "",                                     //F63
	  "",                                     //F64
	  "��ʱ����һ��������������",             //F65
	  "��ʱ���Ͷ���������������",             //F66
	  "��ʱ����һ����������������ֹͣ����",   //F67
	  "��ʱ���Ͷ�����������������ֹͣ����",   //F68
	  "",                                     //F69
	  "",                                     //F70
	  "",                                     //F71
	  "",                                     //F72
	  "����������",                           //F73
	  "������Ͷ�����в���",                   //F74
	  "��������������",                       //F75
	  "������Ͷ�п��Ʒ�ʽ",                   //F76
	  "",                                     //F77
	  "",                                     //F78
	  "",                                     //F79
	  "",                                     //F80
	  "ֱ��ģ�������",                       //F81
	  "ֱ��ģ������ֵ",                       //F82
	  "ֱ��ģ�����������",                   //F83
	  "",                                     //F84
	  "",                                     //F85
	  "",                                     //F86
	  "",                                     //F87
	  "",                                     //F88
	  "",                                     //F89
	  "",                                     //F90
	  "",                                     //F91
	  "���ֱ��",                           //F92
	  "",                                     //F93
	  "",                                     //F94
	  "",                                     //F95
	  "",                                     //F96
	  "�ն�����",                             //F97
	  "�ն�ID��",                             //F98
	  "",                                     //F99
	  "",                                     //F100
	  "",                                     //F101
	  "",                                     //F102
	  "",                                     //F103
	  "",                                     //F104
	  "",                                     //F105
	  "",                                     //F106
	  "",                                     //F107
	  "",                                     //F108
	  "",                                     //F109
	  "",                                     //F110
	  "",                                     //F111
	  "",                                     //F112
	  "",                                     //F113
	  "",                                     //F114
	  "",                                     //F115
	  "",                                     //F116
	  "",                                     //F117
	  "",                                     //F118
	  "",                                     //F119
	  "",                                     //F120
	  "�ն˵�ַ������������",                 //F121
	  "ָ���ն˳���ʱ��",                     //F122
	  "�Զ����׼DL/T 645������",             //F123
	  "",                                     //F124
	  "",                                     //F125
	  "",                                     //F126
	  "",                                     //F127
	  "",                                     //F128
	  "�ն˽�������У��ֵ",                   //F129
	  "�ն���̫����ַ",                       //F130
	  "���Խ�������У��ǰ����ֵ",             //F131
	  "����ר���û���������[��չ]"            //F132
	};


char *ctrlName[42] =
  { 
	  "ң����բ",                     //F1
	  "�����բ",                     //F2
	  "",                             //F3
	  "",                             //F4
	  "",                             //F5
	  "",                             //F6
	  "",                             //F7
	  "",                             //F8
	  "ʱ�ι���Ͷ��",                 //F9
	  "���ݹ���Ͷ��",                 //F10
	  "Ӫҵ��ͣ����Ͷ��",             //F11
	  "��ǰ�����¸���Ͷ��",           //F12
	  "",                             //F13
	  "",                             //F14
	  "�µ��Ͷ��",                   //F15
	  "�����Ͷ��",                   //F16
	  "ʱ�ι��ؽ��",                 //F17
	  "���ݿؽ��",                   //F18
	  "Ӫҵ��ͣ�ؽ��",               //F19
	  "��ǰ�����¸��ؽ��",           //F20
	  "",                             //F21
	  "",                             //F22
	  "�µ�ؽ��",                   //F23
	  "����ؽ��",                   //F24
	  "�ն˱���Ͷ��",                 //F25
	  "�߷Ѹ澯Ͷ��",                 //F26
	  "�����ն�����վͨ��",           //F27
	  "�ն��޳�Ͷ��",                 //F28
	  "�����ն������ϱ�",             //F29
	  "",                             //F30
	  "��ʱ����",                     //F31
	  "������Ϣ",                     //F32
	  "�ն˱�����",                 //F33
	  "�߷Ѹ澯���",                 //F34
	  "��ֹ�ն�����վͨ��",           //F35
	  "�ն��޳����",                 //F36
	  "��ֹ�ն������ϱ�",             //F37
	  "��������",                     //F38
	  "",                             //F39
	  "",                             //F40
	  "����������Ͷ��",               //F41
	  "�����������г�"                //F42
	};

