/*************************************************
Copyright,2009,Huawei WoDian co.,LTD
�ļ�����userInterface.h
���ߣ�leiyong
�汾��0.9
�������:2009��12��
�����������ն�(�����նˡ�������)�˻��ӿڴ���ͷ�ļ�
�޸���ʷ��
  01,09-12-23,leiyong created.

**************************************************/
#ifndef __INCUserInterfaceH
#define __INCUserInterfaceH

#include "common.h"

//��ʾģʽ
#define  DEFAULT_DISPLAY_MODE       1         //Ĭ�ϲ˵���ʽ
#define  CYCLE_DISPLAY_MODE         2         //���Բ˵���ʽ
#define  KEYPRESS_DISPLAY_MODE      3         //�����˵���ʽ

#define  NUM_MP_PER_PAGE            8         //ÿҳ����ʾ�Ĳ��������

#define  STATUS_NONE              0x0         //��δ��������
#define  STATUS_INPUT_PASS        0x1         //������������
#define  PASS_STATUS_CHECK_OK     0x2         //����������ȷ
#define  PASS_STATUS_CHECK_ERROR  0x3         //�����������
#define  STATUS_SELECT_DIGITAL    0x4         //ѡ������
#define  STATUS_SELECT_CHAR       0x5         //ѡ���ַ�
#define  STATUS_SELECT_PROTOCOL   0x6         //ѡ��Э��
#define  STATUS_INPUT_NEW_PASS    0x7         //����������
#define  STATUS_INPUT_TE_ADDR     0x8         //�����ն˵�ַ
#define  STATUS_INPUT_ADD_POINT   0x9         //�����������Ϣ
#define  STATUS_INPUT_EDIT_POINT  0xA         //�޸Ĳ�������Ϣ
#define  STATUS_INPUT_VPN         0xB         //����VPN

#ifdef PLUG_IN_CARRIER_MODULE   //������
 #ifdef MENU_FOR_CQ_CANON
  #define MAX_LAYER1_MENU           6         //һ��˵����ֵ
  #define MAX_LAYER2_MENU           6         //����˵����ֵ
 #else
  #ifdef LIGHTING
   #define MAX_LAYER1_MENU          6         //һ��˵����ֵ
  #else
   #define MAX_LAYER1_MENU          3         //һ��˵����ֵ
  #endif
  #define MAX_LAYER2_MENU          16         //����˵����ֵ
 #endif
#else    //ר��III���ն�
  #define MAX_LAYER1_MENU           7         //һ��˵����ֵ
  #define MAX_LAYER2_MENU          15         //����˵����ֵ
#endif

#define MAXINTERFACES 16

//�ṹ - �ز�����Ϣ
struct carrierMeterInfo
{
	 INT16U mp;                                 //�������
	 INT16U mpNo;                               //�ز��ӽڵ����
	 INT8U  info[2];                            //�ӽڵ���Ϣ
	 INT8U  addr[6];                            //�ز��ӽڵ��ַ
	 INT8U  protocol;                           //�ز��ӽڵ��Լ����
	 INT8U  copyTime[2];
	 INT8U  copyEnergy[4];
	 
	 struct carrierMeterInfo *next;	            //��һ�ڵ�
};

//�ṹ - �쳣�澯��ʾ
typedef struct
{
	 unsigned int aberrantFlag    :1;           //�쳣��־(�Ƿ����쳣����)
	 unsigned int blinkCount      :3;           //��˸����
	 
	 INT8U        eventNum;                     //�¼����
	 DATE_TIME    timeOut;                      //�쳣��˸����ʱ��

}ABERRANT_ALARM;


//�ⲿ����
extern INT8U          displayMode;            //��ʾģʽ(Ĭ�ϲ˵�-1,����-2,����-3)
extern struct         carrierMeterInfo *mpCopyHead,*tmpMpCopy,*prevMpCopy;                    //���в����㳭��ָ��
extern struct         carrierMeterInfo *foundMeterHead,*tmpFound,*prevFound,*noFoundMeterHead;//���ֵ��ָ��
extern struct         carrierMeterInfo *existMeterHead,*prevExistFound;                       //���ֵ����������ָͬ��

extern DATE_TIME      foundTimeOut;           //���ֵ��ʱʱ��
extern ABERRANT_ALARM aberrantAlarm;          //�쳣�澯

//��������
void netInfo(void);
void terminalAddr(void);
void terminalStatus(INT8U num);

//���ҵ�����ʾ�淶����
void showInfo(char *info);
void refreshTitleTime(void);

//������ʾ�淶����
void copyQueryMenu(INT8U layer2Light,INT8U layer3Light);
void paraQueryMenu(INT8U layer2Light,INT8U layer3Light);
void keyHouseholdMenu(INT8U layer2Light,INT8U layer3Light);
void statisQueryMenu(INT8U layer2Light,INT8U layer3Light);
void paraSetMenu(int lightNum,int layer1Num);
void debugMenu(int lightNum,int layerNum);
void realCopyMeterMenu(int lightNum);
void inputPassWord(INT8U lightNum);
void modifyPasswordMenu(INT8U layer2Light,INT8U layer3Light);
void singleMeterCopy(char *mp,char *time,INT8U *energyData,INT8U lightNum);
void singleMeterCopyReport(INT8U *data);
void allMpCopy(INT8U lightNum);
void searchMeter(INT8U lightNum);
void newAddMeter(INT8U lightNum);
void activeReportMenu(INT8U lightNum);
void freeQueryMpLink(void);
void fillTimeStr(void);
void stringUpDown(char *processStr, INT8U timeChar, INT8U upDown);
BOOL checkInputTime(void);
void showInputTime(INT8U layer3Light);
void adjustCommParaLight(INT8U leftRight);
void searchMeterReport(void);
void newMeterCpStatus(INT8U lightNum);
void inputApn(INT8U leftRight);
void searchCtrlEvent(INT8U erc);
void eventRecordShow(INT8U erc);
void setRlPara(INT8U layer2Light,INT8U layer3Light);
void setVpn(INT8U lightNum);
void setCopyForm(INT8U lightNum);

//ר��III���ն˺���
void configPara(INT8U rowLight,INT8U colLight);


extern INT8U          wlRssi;                 //����Modem�ź�

//���ֻ��͹����ⲿ����
extern DATE_TIME      lcdLightDelay;          //LCD������ʱ
extern INT8S          menuInLayer;            //���˵�����
extern INT8U          lcdLightOn;             //LCD�����?

extern DATE_TIME      searchStart, searchEnd; //������ʼʱ��

//���ֻ��͹�������
void userInterface(BOOL secondChanged);
void startDisplay(void);
void modemSignal(int type);
void signalReport(INT8U type,INT8U rssi);
void layer1Menu(int	num);
void layer2Menu(int lightNum,int layer1Num);
void messageTip(int type);

#endif    /*__INCUserInterfaceH*/