/***************************************************
Copyright,2009,Huawei WoDian co.,LTD,All	Rights Reserved
�ļ�����userInterface.c
���ߣ�leiyong
�汾��0.9
������ڣ�2009��12��
�����������ն��˻��ӿ��ļ�
�����б�
     1.
�޸���ʷ��
  01,09-12-23,Leiyong created.
  02,10-11-23,Leiyong,���������˾���Է���,485�ܱ�δ���볭����ͳ��,��485����ɹ��������ͳ��
  03,10-11-23,Leiyong,���������˾���Է���,����ͳ������BΪ��λ��ʾ,�����Ϊ��KBΪ��λͳ��


***************************************************/
#include <pthread.h>
#include <sys/reboot.h>

#include <stdlib.h>
#include <sys/mount.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/if_arp.h>

#include "string.h"
#include "common.h"
#include "ioChannel.h"
#include "meterProtocol.h"

#include "teRunPara.h"
#include "msSetPara.h"
#include "lcdGui.h"
#include "wlModem.h"
#include "convert.h"
#include "workWithMeter.h"
#include "dataBase.h"
#include "copyMeter.h"
#include "hardwareConfig.h"
#include "att7022b.h"
#include "AFN05.h"
#include "AFN0C.h"
#include "AFN0E.h"

#ifdef LOAD_CTRL_MODULE
 #include "loadCtrl.h"
#endif

#ifdef PLUG_IN_CARRIER_MODULE
 #include "gdw376-2.h"
#endif

#include "userInterface.h"


//�����ն˹�����ʾ����---------------------------------------------------------
INT8U          wlRssi;                                           //����Modem�ź�
DATE_TIME      lcdLightDelay;                                    //LCD������ʱ
INT8U          lcdLightOn = LCD_LIGHT_ON;                        //LCD�����?
INT16U         keyPressCount;                                    //��������,����
#ifdef PLUG_IN_CARRIER_MODULE
 INT16U        keyCountMax = 10;                                 //�����������ֵ
#else
 INT16U        keyCountMax = 12;                                 //�����������ֵ
#endif

INT8U          keyValue;                                         //��ֵ
INT8U          displayMode=0;                                    //��ʾģʽ(Ĭ�ϲ˵�-1,����-2,����-3)
INT8S          menuInLayer=0;                                    //���˵�����
INT8U          layer1MenuLight=0;	  	                           //1��˵��ĵڼ���ѡ��

DATE_TIME searchStart, searchEnd;                                //������ʼʱ��

char           character[70]=". +-@*#0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";//ѡ�����ַ�
INT8U          inputStatus=STATUS_NONE;                          //����״̬
INT8U          inputIndex=0;                                     //��������
INT8U          selectIndex=0;                                    //ѡ����������

INT8U          alarmCount=0;                                     //�澯����
INT8U          pageWait = 0;                                     //ҳ����ʾʱ��,����ҳ����ʾʱ��ص����Ի���
char           tmpVpnUserName[33]="";                            //��ʱVpn�û���
char           tmpVpnPw[33]="";                                  //��ʱVpn����

ABERRANT_ALARM     aberrantAlarm;                                    //�쳣�澯
struct cpAddrLink  *queryMpLink,*tmpMpLink,*tmpPrevMpLink;           //��ѯ��������������
DATE_TIME          menuQueryTime;                                    //��ѯʱ��
INT8U              rowOfLight;                                       //���¼�(��ҳ)����(ר��III���ն�ͨ�Ų������õȵط���)
INT8U              keyLeftRight;                                     //���Ҽ�(��ҳ)����(�ص��û���ͨ�Ų������õȵط���)
char               queryTimeStr[9];                                  //��ѯʱ���ַ���
char               originPassword[7]="000000";                       //ԭʼ����
char               passWord[7] = "000000";                           //����������е������ַ�
INT8U              pwLight=0;                                        //�����ַ��������
char               tmpTeAddr[10]="";                                 //��ʱ�ն˵�ַ
char               commParaItem[5][30];                              //ͨ�Ų�������ʱ����ʱ�ַ�����

char               chrEthPara[4][20];                                       //��ʱ��̫������
char               tmpEthMac[30];

//376.1������������ʾ����--------------------------------------------------------
#ifdef PLUG_IN_CARRIER_MODULE
 struct            carrierMeterInfo *foundMeterHead,*tmpFound,*prevFound,*noFoundMeterHead;//���ֵ��ָ��
 struct            carrierMeterInfo *existMeterHead,*prevExistFound;                       //���ֵ����������ָͬ��
 struct            carrierMeterInfo *tmpNoFoundx;

 INT16U            multiCpUpDown,multiCpMax;                         //������㳭�����·�ҳ,�����ҳ��
 char              singleCopyMp[5]="0001";                           //ָ�������㳭��������ַ���
 char              singleCopyTime[9]="-----";                        //ָ�������㳭����ʱ��
 char              singleCopyEnergy[10]="-----";                     //ָ�������㳭��ʾֵ
 struct            carrierMeterInfo *mpCopyHead, *tmpMpCopy, *prevMpCopy;//���в����㳭��ָ��
 char              realCopyMeterItem[2][18] = {"1��ָ�������㳭��","2��ȫ�������㳭��"};
 
 char              chrMp[5][13];                                     //��ʱ��������Ϣ

 #ifdef MENU_FOR_CQ_CANON  //�����Լ�������˵�����-----------------------------

  METER_DEVICE_CONFIG menuMeterConfig;                               //�˵���ʾ�ò������������Ϣ
  char      weekName[7][7] = {"������","����һ","���ڶ�","������","������","������","������"};
  char      layer1MenuItem[MAX_LAYER1_MENU][12]={"1�������ѯ","2��������ѯ","3���ص��û�","4��ͳ�Ʋ�ѯ","5����������","6���ֳ�����"};
  char      paraSetItem[3][17] = {"5-1 ͨ�Ų�������", "5-2 �޸�����    ", "5-3VPN�û�������"};
  char      debugItem[7][19]   = {"     ʵʱ����     ","ȫ�������㳭����","     �������     ","  �������ܱ��ַ  ","�������ܱ���״̬"," δ�ѵ����ܱ��ַ ","     �����ϱ�     "};
 
  INT8U     layer2MenuLight[MAX_LAYER1_MENU]={0,0,0,0,0,0};          //2��˵��ĵڼ���ѡ��
  INT8U     layer2MenuNum[MAX_LAYER1_MENU]  ={6,5,6,5,2,7};          //2��˵��ĸ��˵����ֵ
  INT8U     layer3MenuNum[MAX_LAYER1_MENU][MAX_LAYER2_MENU]= {{8,0,0,0,0},{0,0,0,0,0},{8,0,0,0,0},{8,0,0,0,0},{5,3,0,0,0},{2,0,0,0,0}};
  INT8U     layer3MenuLight[MAX_LAYER1_MENU][MAX_LAYER2_MENU]= {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};

 #else  //MENU_FOR_CQ_CANON,���ҵ�����Լ----------------------------------------

  #ifdef LIGHTING
   char     layer1MenuItem[MAX_LAYER1_MENU][15]={"���ƿ��Ƶ�״̬","�����������ѯ","   ����ά��   ","��·���Ƶ�״̬","�������Ƶ�״̬","    ���ն�    "};
  #else
   char     layer1MenuItem[MAX_LAYER1_MENU][15]={"������������ʾ","����������鿴","�ն˹�����ά��"};
  #endif

  char      layer2MenuItem[MAX_LAYER1_MENU][MAX_LAYER2_MENU][19]={
	                    {},
	                   #ifdef LIGHTING
	                    {"   ͨ��ͨ������   ","        -         ","  ���Ƶ��������  ","  ������ʱ������  ","   ������������   ","    ���������    ","  ��̫����������  ","����ר���û�������"},
	                    {"��ѯ���ƿ��Ƶ�״̬", "     ������Ϣ     ", "  ����Һ���Աȶ�  ", "     ��λ������    ", "  �������������  ","    ����ң����    "},
	                   #else
	                    {"   ͨ��ͨ������   "," ̨������������ "," �������������� ","   �ն�ʱ������   ","   ������������   ","     �ն˱��     ","  ��̫����������  ","����ר���û�������","   ������������   ","   ���ģ�����   "},
	                    {"     ʵʱ����     ","ȫ�������㳭����","     �������     ","  �������ܱ��ַ  ","�������ܱ���״̬","     ������Ϣ     ","  ����Һ���Աȶ�  "," ����ģ�鳭����ʽ ","     ��λ�ն�     ","   �ն˳�������   "," ·��ģ��������� ","�����û�����������","     ��������     ","  ά����ģʽ����  ", "  ��2·485������  ", " ����ͨ��ģ��Э�� "},
	                   #endif
	                    };
 #ifdef LIGHTING
  char      layer2xMenuItem[2][3][17]= 
  	                                   {{"    ������ѯ    ", "    ��������    "},
  	                                    {"   ���Ƶ����   ", "  ͨ��ͨ������  ", "    ����ʱ��    "}
  	                                   };
 #else
  char      layer2xMenuItem[2][2][17]= 
  	                                   {{"    �����鿴    ", "    ��������    "},
  	                                    {" ��ѯ��������� ", "��ѯͨ��ͨ������"}
  	                                   };
 #endif

  INT8U     layer2MenuLight[MAX_LAYER1_MENU]={0, 0, 0};              //2��˵��ĵڼ���ѡ��

 #ifdef LIGHTING
  INT8U     layer2MenuNum[MAX_LAYER1_MENU]  ={21, 8, 6};             //2��˵��ĸ��˵����ֵ

  struct ctrlTimes *tmpCTimesNode;
  
  extern INT8U downLux[6];                                           //�������´��ĵ�ǰ����ֵ

	//����ң�ز˵�
	char   irMenuItem[8][11] = {
															"  ���俪  ",
															"  ���ȿ�  ",
															"  ��ʪ��  ",
															"  �ؿյ�  ",
															"ѧϰ���俪",
															"ѧϰ���ȿ�",
															"ѧϰ��ʪ��",
															"ѧϰ�ؿյ�",
														 };
 #else
  INT8U     layer2MenuNum[MAX_LAYER1_MENU]  ={21, 9, 16};            //2��˵��ĸ��˵����ֵ
 #endif

  INT8U     layer3MenuNum[MAX_LAYER1_MENU][MAX_LAYER2_MENU]= {{8,0,0,0,0,0},{5,6,6,0,3,0,5,5,6},{2,0,0,0,0,4}};
  INT8U     layer3MenuLight[MAX_LAYER1_MENU][MAX_LAYER2_MENU]= {{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},};
  INT8U     layer2xMenuLight;
  INT8U     mpQueryType, mpQueryPage, mpQueryLight;                  //��ѯ�����������ʱ����

  char      dateTimeItem[15];                                        //��������ʱ����
  char      chrRlPara[4][3];                                         //��ʱ���ģ�����
  char      chrCopyRate[8][6]={"Ĭ��","600","1200","2400","4800","7200","9600","19200"};
  char      chrCopyPort[3][8]={"����","RS485-1","RS485-2"};
  char      chrCopyProtocol[3][13]={"DL/T645-1997","DL/T645-2007","��������"};
  
  DATE_TIME cycleDelay;                                             //������ʱ
  struct cpAddrLink *cycleMpLink, *tmpCycleLink;                    //���Բ�������������
 
 #endif  //MENU_FOR_CQ_CANON
 
#else    //no PLUG_IN_CARRIER_MODULE,�ն�
 //�ṹ - �����ַ����
 struct eventShowLink
 {
	 INT16U               eventNo;                //�¼����
	 
	 struct eventShowLink *next;                  //��һ�ڵ�ָ��
 };

 char       layer1MenuItem[MAX_LAYER1_MENU][15]={"1.ʵʱ����",
 	                                               "2.������ֵ",
 	                                               "3.����״̬",
 	                                               "4.���ܱ�ʾ��",
 	                                               "5.������Ϣ",
 	                                               "6.������Ϣ",
 	                                               "7.�ն���Ϣ"};
 char       layer2MenuItem[MAX_LAYER1_MENU][MAX_LAYER2_MENU][19]={
	                    {"1.��ǰ����",
	                     "2.��ǰ����",
	                     "3.��������",
	                     "4.����״̬",
	                     "5.���ؼ�¼",
	                     "6.��ؼ�¼",
	                     "7.ң�ؼ�¼",
	                     "8.ʧ���¼"
	                    },
	                    {"1.ʱ�οز���",
	                     "2.���ݿز���",
	                     "3.�¸��ز���",
	                     "4.KvKiKp    ",
	                     "5.���ܱ����",
	                     "6.���ò���  ",
	                     "7.ר���û���",
	                     "8.�������  ",
	                     "9.��������  ",
	                     "10.�ն��������",
	                     "11.�������� ",
	                     "12.LCD�Աȶ�",
                       "13.ά����ģʽ",
                       "14.��2·485����",
	                     "15.��̫������"
	                    },
	                    {},
	                    {},
	                    {}
	                    };
 INT8U      layer2MenuLight[MAX_LAYER1_MENU]={0,0,0,0,0,0,0};       //2��˵��ĵڼ���ѡ��
 INT8U      layer2MenuNum[MAX_LAYER1_MENU]  ={8,8,0,0,0,0,3};       //2��˵��ĸ��˵����ֵ
 INT8U      layer3MenuNum[MAX_LAYER1_MENU][MAX_LAYER2_MENU]= {{8,0,0,0,0,0},{5,6,6,0,3,0,0,8,0,0,6},{2,0,0,0,0,3}};
 INT8U      layer3MenuLight[MAX_LAYER1_MENU][MAX_LAYER2_MENU]= {{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},};
 INT8U      numOfPage;                                              //������Ϣ���ܼ���ҳ��
 INT8U      mpPowerCurveLight;                                      //�����㹦�����߸�����
 struct eventShowLink *eventLinkHead,*tmpEventShow;                 //�¼���ȡ����
 char       chrMp[7][13];                                           //��ʱ��������Ϣ
 char       chrCopyRate[8][6]={"Ĭ��","600","1200","2400","4800","7200","9600","19200"};
 char       chrCopyPort[3][8]={"����","RS485-1","RS485-2"};
 char       chrCopyProtocol[8][13]={"DL/T645-1997","DL/T645-2007","��������","ABB����","������ZD��","����EDMI��","����๦�ܱ�","����UI��"};
 
#endif   //PLUG_IN_CARRIER_MODULE




//���ֻ��͹�������***************************************************

/***************************************************
��������:getFileName
��������:ȡ���ļ�·���е��ļ���
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
int getFileName(char *file, char *filename)
{
	unsigned int i;
	
	char *token = NULL;
	char seps[] = "/";
	token = strtok(file, seps);
	while(token != NULL)
	{
		strcpy(filename, token);
		token = strtok(NULL, seps);
	}
	
	return 0;
}

/***************************************************
��������:tipSound
��������:��ʾ��
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
INT8U tipSound(INT8U type)
{
   switch(type)
   {
     case 1:
       ioctl(fdOfIoChannel, SET_ALARM_VOICE, 1);
       sleep(1);
       ioctl(fdOfIoChannel, SET_ALARM_VOICE, 0);
       sleep(1);
       ioctl(fdOfIoChannel, SET_ALARM_VOICE, 1);
       sleep(1);
       ioctl(fdOfIoChannel, SET_ALARM_VOICE, 0);
       break;
     
     case 2:
       ioctl(fdOfIoChannel, SET_ALARM_VOICE, 1);
       usleep(100000);
       ioctl(fdOfIoChannel, SET_ALARM_VOICE, 0);
       usleep(300000);
       ioctl(fdOfIoChannel, SET_ALARM_VOICE, 1);
       usleep(100000);
       ioctl(fdOfIoChannel, SET_ALARM_VOICE, 0);
       break;
   }
}
    

/***************************************************
��������:uDiskUpgrade
��������:U������
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
INT8U uDiskUpgrade(void)
{
  INT8U upOk=1;
  int   len, i;
  char  filenames[20][256];
  FILE  *file;
  char  fileName[256];
  char  tmpProcess[256];
  char  devStr[20];
  INT8U ifFound;
 
  guiLine(1,55,160,105,0);
  guiLine(1,55,1,105,1);
  guiLine(160,55,160,105,1);
  guiLine(1,55,160,55,1);
  guiLine(1,105,160,105,1);
  
  //1.mount
  ifFound = 0;
  for(i=1; i<16; i++)
  {
    sprintf(devStr, "/dev/sda%d", i);
    if (mount(devStr, "/mnt", "vfat", 0, NULL)==0)
    {
      guiDisplay(8, 70 ,"U���Ѳ���,����ļ�", 1);
      lcdRefresh(50, 105);
  
      printf("mount %s /mnt �ɹ�\n", devStr);
      
      ifFound = 1;
      break;
    }
  }

  if (ifFound==0)
  {
    guiDisplay(44, 70, "�����U��", 1);
    lcdRefresh(50, 105);
  
    printf("ִ��mountʧ��,������Ϣ:%s\n", strerror(errno));
  
    tipSound(2);
  
    return 0;
  }

  //2.ȡ�����ļ�
  if((file=fopen("/bin/config.ini","r")) != NULL)
  {
    i = 0;
    while(fgets(filenames[i], 256, file) != NULL)
    {
    	len = strlen(filenames[i]);
    	filenames[i][len - 2] = '\0';
    	i++;
    }
    
    len = i;
  }
  else
  {
    guiLine(3,57,158,103,0);
    guiDisplay(10, 70, "�����ļ���ȡʧ��", 1);
    lcdRefresh(50, 105);
    
  	printf("config.ini is not exist.");

    tipSound(2);

  	goto umountBreak;
  }
  
  //3.�����Ƿ���������ļ�
  for(i=0;i<len;i++)
  {
    strcpy(tmpProcess,filenames[i]);
    getFileName(tmpProcess, fileName);
    sprintf(tmpProcess,"mnt/upfile/%s", fileName);

    if(access(tmpProcess, F_OK) != 0)
    {
      guiLine(3,57,158,103,0);
      guiDisplay(8, 70, "�����ļ�δ����/��ȫ", 1);
      lcdRefresh(50, 105);
      upOk = 0;
      printf("�����ļ�%s������\n", tmpProcess);
      
      tipSound(2);

      goto umountBreak;
    }
  }
    
  guiLine(3, 57, 158, 103, 0);
  guiDisplay(3, 60, "������,����:", 1);
  for(i=0; i<len; i++)
  {
    getFileName(filenames[i], fileName);
    guiLine(3, 77, 158, 103, 0);
    guiDisplay(10, 80, fileName, 1);
    lcdRefresh(50, 105);

    sprintf(tmpProcess, "cp /mnt/upfile/%s %s", fileName, filenames[i]);
    if (system(tmpProcess)==0)
    {
    	printf("����%s�ɹ�\n", tmpProcess);
    }
    else
    {
      guiDisplay(127, 80, "ʧ��", 1);
      lcdRefresh(50, 105);
    	 
    	printf("�����ļ�%sʧ��\n",fileName);
    	upOk = 0;
      
      tipSound(2);

    	break;
    }
  }
  
umountBreak:
  if (umount("/mnt")==0)
  {
    if (upOk==1)
    {
      guiLine(3, 57, 158, 103, 0);
      guiDisplay(3,70,"�������,ϵͳ����..",1);
      lcdRefresh(50,105);

      printf("umount�ɹ�,ϵͳ����\n");
    }
  }
  else
  {

    guiDisplay(10,70,"����U��ʧ��.", 1);
    lcdRefresh(50,105);

    printf("ִ��umountʧ��,������Ϣ:%s\n", strerror(errno));

    tipSound(2);
    
    return 0;
  }
  
  if (upOk==1)
  {
    tipSound(1);
    
    sleep(3);
      
    reboot(RB_AUTOBOOT);
  }
}

/*******************************************************
��������:showInfo
��������:״̬����ʾ��Ϣ(376.1���ҵ�����������Լ)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void showInfo(char *info)
{
	#ifdef PLUG_IN_CARRIER_MODULE
	 #ifndef CQDL_CSM
	  guiLine(1,LCD_LINE_INFO,160,160,0);
	  guiDisplay(1,LCD_LINE_INFO,info,1);
	 
	  lcdRefresh(LCD_LINE_INFO,160);
	 #endif
	#endif
}

/***************************************************
��������:trim
��������:ȥ���ַ�������Ŀո�
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
char * trim(char * s)
{
   INT8U i,j,len,numOfPrevSpace;
   
   //ȥ���ַ���ǰ��Ŀո�
   len = strlen(s);
   
   for(i=0;i<len;i++)
   {
  	  if (s[i]!=0x20)
  	  {
  	  	 break;
  	  }
   }

   if (i>0)
   {
     numOfPrevSpace = i+1;
     for(j=0;j<len;j++)
     {
     	  if (j+numOfPrevSpace<=len)
     	  {
     	     s[j] = s[i++];
     	  }
     	  else
     	  {
     	  	 s[j] = 0x0;
     	  }
     }
   }

   //ȥ���ַ�������Ŀո�
   len = strlen(s);
   for(i=len;i>0;i--)
   {
  	  if (s[i-1]!=0x20)
  	  {
  	  	 break;
  	  }
  	  if (s[i-1]==0x20)
  	  {
  	  	 s[i-1] = 0x0;
  	  }
   }

   len = strlen(s);
   
   return s;
}

/**************************************************
��������:startDisplay
��������:������ʾ
���ú���:
�����ú���:
�������:void *arg
�������:
����ֵ��״̬
***************************************************/
void startDisplay(void)
{
  char     str[6];
 
  lcdClearScreen();

  //����
  guiLine(1, 1, 160, 160, 0);
 
  layer1Menu(0);
 
  switch(moduleType)
  {
    case GPRS_SIM300C:
    case GPRS_GR64:
    case GPRS_MC52I:
    case GPRS_M72D:
      guiAscii(22,1,"G",1);
      break;

    case ETHERNET:
      guiAscii(15,1,"E",1);
      break;
      
    case CDMA_DTGS800:
    case CDMA_CM180:
      guiAscii(22,1,"C",1);
      break;

    case CASCADE_TE:
      guiAscii(14,1,"JL",1);
      break;
		
    case LTE_AIR720H:
      guiAscii(22,1,"4",1);
      break;

    default:
      guiAscii(22,1,"N",1);
      break;
  }
 
 #ifdef PLUG_IN_CARRIER_MODULE
  #ifdef LIGHTING
   guiDisplay(24,30,"����·�Ƽ�����",1);
  #else
   guiDisplay(24,30,"��ѹ����������",1);
  #endif
 
  guiLine(124,1,160,16,0);
  guiAscii(124, 1, digital2ToString(sysTime.hour,str),1);
  guiAscii(139, 1, ":", 1);
  guiAscii(145, 1, digital2ToString(sysTime.minute,str),1);
 
  defaultMenu();


  displayMode = DEFAULT_DISPLAY_MODE;    //Ĭ�ϲ˵�
 #else
  guiAscii(103, 1, digital2ToString(sysTime.hour,str),1);
  guiAscii(118, 1, ":", 1);
  guiAscii(124, 1, digital2ToString(sysTime.minute,str),1);
  guiAscii(139, 1, ":", 1);
  guiAscii(145, 1, digital2ToString(sysTime.second,str),1);

  guiLine(5,16,155,16,1);
 #endif
 
  lcdRefresh(1,160);
}

/**************************************************
��������:signal
��������:���ź�ָʾ
���ú���:
�����ú���:
�������:
�������:
����ֵ��״̬
***************************************************/
void modemSignal(int type)
{
  int i;

  if (type == 8 || type==9)
  {
 	 #ifdef LOAD_CTRL_MODULE
 	  guiLine(1,1,40,15,0);
 	 #else
 	  guiLine(1,1,40,16,0);
 	 #endif
 	  if (type==8)
 	  {
 	   guiAscii(1,1,"NoSIM",1);
 	  }
 	  if (type==9)
 	  {
 	    guiAscii(1,1,"NoUIM",1);
 	  }
  }
  else
  {
	 #ifdef LOAD_CTRL_MODULE
	  guiLine(1,1,40,15,0);
	 #else
	  guiLine(1,1,40,16,0);
	 #endif
	 
   #ifndef PLUG_IN_CARRIER_MODULE
    //�����߲�
    guiLine(4,1,4,7,1);
    guiLine(1,2,1,2,1);
    guiLine(2,3,2,3,1);
    guiLine(3,4,3,4,1); 
    guiLine(7,2,7,2,1);
    guiLine(6,3,6,3,1);
    guiLine(5,4,5,4,1);
   #endif
   
    switch(moduleType)
    {
      case GPRS_SIM300C:
      case GPRS_GR64:
      case GPRS_MC52I:
      case GPRS_M590E:
      case GPRS_M72D:
        guiAscii(22,1,"G",1);
        break;
  
      case ETHERNET:
        guiAscii(22,1,"E",1);
        break;
          
      case CDMA_DTGS800:
      case CDMA_CM180:
        guiAscii(22,1,"C",1);
        break;
        
      case CASCADE_TE:
        guiAscii(14,1,"JL",1);
        break;
			
      case LTE_AIR720H:
        guiAscii(22,1,"4",1);
        break;
     
      default:
        guiAscii(22,1,"N",1);
        break;
    }
    
    for(i=0;i<type;i++)
    {
      if (type==7)
      {
       	break;
      }
       
     #ifndef PLUG_IN_CARRIER_MODULE
      guiLine(2+5*i,9-2*i,2+5*i,14,1);
      guiLine(3+5*i,9-2*i,3+5*i,14,1);
     #else
      guiLine(4+5*i,9-2*i,4+5*i,14,1);
      guiLine(5+5*i,9-2*i,5+5*i,14,1);
     #endif
    }
  }
  
  lcdRefresh(1,16);
}

/**************************************************
��������:signal
��������:���ź�ָʾ
���ú���:
�����ú���:
�������:
�������:
����ֵ��״̬
***************************************************/
void messageTip(int type)
{
  guiLine(42,  3, 55, 14, 0);
	
	if (type==1)
	{
	   guiLine(45,  6, 45,  6, 1);
     guiLine(46,  7, 46,  7, 1);
     guiLine(47,  8, 47,  8, 1);
     guiLine(48,  9, 48,  9, 1);
     guiLine(49, 10, 49, 10, 1);
     guiLine(50, 11, 50, 11, 1);
     guiLine(51, 12, 51, 12, 1);
     guiLine(52, 13, 52, 13, 1);

     guiLine(52,  6, 52,  6, 1);
     guiLine(51,  7, 51,  7, 1);
     guiLine(50,  8, 50,  8, 1);
     guiLine(49,  9, 49,  9, 1);
     guiLine(48, 10, 48, 10, 1);
     guiLine(47, 11, 47, 11, 1);
     guiLine(46, 12, 46, 12, 1);
     guiLine(45, 13, 45, 13, 1);

     guiLine(46,  4, 46,  4, 1);
     guiLine(47,  3, 47,  3, 1);
     guiLine(48,  3, 48,  3, 1);
     guiLine(49,  3, 49,  3, 1);

     guiLine(51,  4, 51,  4, 1);
     guiLine(50,  3, 50,  3, 1);

     guiLine(42,  5, 42, 14, 1);
     guiLine(55,  5, 55, 14, 1);
     guiLine(42,  5, 55,  5, 1);
     guiLine(42, 14, 55, 14, 1);
  }
}

/***************************************************
��������:singleReport
��������:�ź�ǿ�������Сֵ��¼
���ú���:
�����ú���:
�������:��
�������:
����ֵ��void
***************************************************/
void signalReport(INT8U type,INT8U rssi)
{  
  TERMINAL_STATIS_RECORD terminalStatisRecord;  //�ն�ͳ�Ƽ�¼
  DATE_TIME              tmpTime;
  BOOL                   ifNeedRecord;
  char                   say[20],str[5];

  //��ʾ�ź�
  if (type>0)
  {
  	 modemSignal(type);
  	 
  	 return;
  }
  else
  {
  	 wlRssi = rssi;
  	 if (rssi==0)
  	 {
  	   modemSignal(0);
  	 }
  	 else
  	 {
  	   modemSignal(rssi/8+1);
  	 }
  }
  
  if (rssi>0 && rssi<=31)
  {
   #ifdef PLUG_IN_CARRIER_MODULE
    #ifndef MENU_FOR_CQ_CANON
     strcpy(say,"�ź�ǿ��->");
     strcat(say,intToString(rssi,3,str));
     showInfo(say);
    #endif
   #endif
    
    ifNeedRecord = FALSE;

    tmpTime = timeHexToBcd(sysTime);
	  if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == FALSE)
    {
   	  initTeStatisRecord(&terminalStatisRecord);
    }
    
    //��¼������ź�ǿ�ȼ�����źŷ���ʱ��
    if (terminalStatisRecord.maxSignal<rssi)
    {
    	 terminalStatisRecord.maxSignal = rssi;
    	 terminalStatisRecord.maxSignalTime[0] = sysTime.minute;
    	 terminalStatisRecord.maxSignalTime[1] = sysTime.hour;
    	 terminalStatisRecord.maxSignalTime[2] = sysTime.day;
       ifNeedRecord = TRUE;
    }
     
    //��¼����С�ź�ǿ�ȼ���С�źŷ���ʱ��
    if (terminalStatisRecord.minSignal>=rssi)
    {
    	 terminalStatisRecord.minSignal = rssi;
    	 terminalStatisRecord.minSignalTime[0]  = sysTime.minute;
    	 terminalStatisRecord.minSignalTime[1] = sysTime.hour;
    	 terminalStatisRecord.minSignalTime[2] = sysTime.day;
       ifNeedRecord = TRUE;
    }
    
    if (ifNeedRecord == TRUE)
    {
      saveMeterData(0, 0, tmpTime, (INT8U *)&terminalStatisRecord, STATIS_DATA, 0x0,sizeof(TERMINAL_STATIS_RECORD));
    }
  }
}

/*******************************************************
��������:setLcdDegree
��������:����LCD�Աȶ�(376.1���ҵ�����������Լ)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void setLcdDegree(INT8U lightNum)
{
	INT8U i, tmpX;
	char  str[2];
	
	menuInLayer = 3;
  
 #ifdef PLUG_IN_CARRIER_MODULE
	guiLine(1,17,160,144,0);
 #else
	guiLine(1,17,160,160,0);
 #endif
	
	guiLine(1,50,1,110,1);
	guiLine(160,50,160,110,1);
	guiLine(1,110,160,110,1);
	guiLine(1,50,160,50,1);
	guiLine(1,70,160,70,1);

	guiDisplay(24,52,"Һ���Աȶ�����",1);

	guiLine( 10, 80, 10,100,1);

	guiLine(10,80,10+lightNum*7,100,1);
	lcdAdjustDegree(lightNum);
	guiLine(150, 80,150,100,1);
	guiLine( 10, 80,150, 80,1);
	guiLine( 10,100,150,100,1);	
  saveParameter(88, 7,&lcdDegree,1);         //LCD�Աȶ�

 #ifdef PLUG_IN_CARRIER_MODULE
	lcdRefresh(17,144);
 #else
	lcdRefresh(17,160);
 #endif
}



/*******************************************************
��������:alarmProcess
��������:�澯����
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void alarmProcess(void)
{
	 INT8U i,j;
	  
	 if (ifPowerOff==TRUE)    //ͣ��(LED��һ��������˸)
	 {
       alarmCount++;
       if (alarmCount<20)
       {
         alarmLedCtrl(ALARM_LED_OFF);
       }
       else
       {
       	  if (alarmCount<100)
       	  {
       	  	 alarmLedCtrl(ALARM_LED_ON);
       	  }
       	  else
       	  {
       	  	 if (alarmCount<120)
       	  	 {
       	  	 	  alarmLedCtrl(ALARM_LED_OFF);
       	  	 }
       	  	 else
       	  	 {
       	  	    if (alarmCount<140)
       	  	    {
       	  	 	     alarmLedCtrl(ALARM_LED_ON);
       	  	    }
       	  	    else
       	  	    {
       	  	       alarmCount = 0;
       	  	    }
       	  	 }
       	  }
       }
       
  	 	 if (setParaWaitTime==0)      //���ò���
       {
  	      setBeeper(BEEPER_OFF);
  	      setParaWaitTime = 0xfe;
       }
	  	 if (ctrlCmdWaitTime==0)      //��������
       {
  	      setBeeper(BEEPER_OFF);
 	        ctrlCmdWaitTime = 0xfe;
       }
	 }
	 else
	 {
  	 	 if (setParaWaitTime==0)      //���ò���
       {
  	      alarmLedCtrl(ALARM_LED_OFF);
  	      setBeeper(BEEPER_OFF);
  	      setParaWaitTime = 0xfe;

          //�������·�����ں�բ״̬�ҿ���״̬����(ctrlStatus)��ΪNONE_CTRL,
          //��ctrlStatus��ΪNONE_CTRL,ʹ��Ļ�ָ����Ի���
          for(i=0;i<ctrlStatus.numOfAlarm;i++)
          {
             if (ctrlStatus.allPermitClose[i]==1)
             {
          	    for(j=i;j<ctrlStatus.numOfAlarm;j++)
          	    {
          	      ctrlStatus.aQueue[j] = ctrlStatus.aQueue[j+1];
          	      ctrlStatus.allPermitClose[j]=ctrlStatus.allPermitClose[j+1];
          	    }
          	    ctrlStatus.numOfAlarm--;
          	    if (ctrlStatus.numOfAlarm==0)
          	    {
          	    	menuInLayer=1;
          	    }
             }
          }
       }
       else
       {
	  	 	 #ifdef LOAD_CTRL_MODULE 
	  	 	  if (ctrlCmdWaitTime==0)   //��������
          {
    	       if (gateCloseWaitTime==0 || gateCloseWaitTime==0xfe)
    	       {
    	         alarmLedCtrl(ALARM_LED_OFF);
    	         setBeeper(BEEPER_OFF);
    	         ctrlCmdWaitTime = 0xfe;
    	       }
          }
          else
          {
          	 if (gateCloseWaitTime==0)  //�����բ��ʾ������
          	 {
    	         alarmLedCtrl(ALARM_LED_OFF);
    	         setBeeper(BEEPER_OFF);
    	         gateCloseWaitTime = 0xfe;
          	 }
          }
         #endif
       }
	 }
}

/*******************************************************
��������:setVpn
��������:��������ר���û���������
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void setVpn(INT8U lightNum)
{
	INT8U i, row, col;
	char  str[2];
	
	menuInLayer = 3;

	#ifdef PLUG_IN_CARRIER_MODULE
	 #ifndef CQDL_CSM
	  guiLine(1,17,160,144,0);
	 #else
	  guiLine(1,17,160,160,0);
	 #endif
	#else
	 guiLine(1,17,160,160,0);
	#endif
	
	guiLine(1,34,1,128,1);
	guiLine(160,34,160,128,1);
	guiLine(1,128,160,128,1);
	guiLine(1,34,160,34,1);
	guiLine(1,54,160,54,1);

	guiDisplay( 9,36,"����ר���û�������",1);
	guiDisplay( 4,56,"�û���",1);
	guiDisplay(19,92,"����",1);

  str[1] = '\0';
  row = 56;
  col = 53;
  for(i=0;i<32;i++)
  {
    str[0] = tmpVpnUserName[i];
 
    if (i==13)
    {
    	 row = 72;
    	 col = 5;
    }
    if (i==lightNum)
    {
      guiDisplay(col,row,str,1);
    }
    else
    {
      guiDisplay(col,row,str,0);
    }
    col+=8;
  }

  row = 92;
  col = 53;
  for(i=32; i<64; i++)
  {
    str[0] = tmpVpnPw[i-32];
 
    if (i==45)
    {
    	row = 108;
    	col = 5;
    }
    if (i==lightNum)
    {
       guiDisplay(col,row,str,1);
    }
    else
    {
       guiDisplay(col,row,str,0);
    }
    col+=8;
  }
  
	lcdRefresh(17,160);
}

/*******************************************************
��������:setEthPara
��������:������̫������(376.1���ҵ�����������Լ)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void setEthPara(INT8U layer2Light,INT8U layer3Light)
{
	INT8U i, tmpX,tmpY;
	char  str[2];
	
	menuInLayer = 3;

	#ifdef PLUG_IN_CARRIER_MODULE
	 guiLine(1,17,160,144,0);
  #else
	 guiLine(1,17,160,160,0);
  #endif
	
	guiLine(1,17,1,140,1);
	guiLine(160,17,160,140,1);
	guiLine(29,34,29,50,1);	
	guiLine(52,51,52,102,1);	
	guiLine(118,103,118,119,1);	
	guiLine(1,17,160,17,1);
	guiLine(1,34,160,34,1);	
	guiLine(1,51,160,51,1);	
	guiLine(1,68,160,68,1);	
	guiLine(1,85,160,85,1);
	guiLine(1,102,160,102,1);
	guiLine(80,119,80,140,1);
	guiLine(1,119,160,119,1);
	guiLine(1,140,160,140,1);

	guiDisplay(24,18,"��̫����������",1);
	guiDisplay(4, 35,"MAC",1);
	guiDisplay(45, 35,tmpEthMac,1);
	guiDisplay(4, 52,"IP��ַ",1);
	guiDisplay(4, 69,"��  ��",1);
	guiDisplay(4, 86,"��  ��",1);
	guiDisplay(4,103,"��̫����¼��վ",1);

  tmpY += 51;
  
  tmpX = 54;
  for(i=0;i<15;i++)
  {
    str[0] = chrEthPara[0][i];
    str[1] = '\0';
    if (layer2Light==0 && layer3Light==i)
    {
      guiDisplay(tmpX,tmpY,str,0);
      tmpX += 8;
    }
    else
    {
      guiDisplay(tmpX,tmpY,str,1);
      tmpX += 7;
    }
  }
  tmpY += 17;
  
  tmpX = 54;
  for(i=0;i<15;i++)
  {
    str[0] = chrEthPara[1][i];
    str[1] = '\0';
    if (layer2Light==1 && layer3Light==i)
    {
      guiDisplay(tmpX,tmpY,str,0);
      tmpX += 8;
    }
    else
    {
      guiDisplay(tmpX,tmpY,str,1);
      tmpX += 7;
    }
  }
  tmpY += 17;

  tmpX = 54;
  for(i=0;i<15;i++)
  {
    str[0] = chrEthPara[2][i];
    str[1] = '\0';
    if (layer2Light==2 && layer3Light==i)
    {
      guiDisplay(tmpX,tmpY,str,0);
      tmpX += 8;
    }
    else
    {
      guiDisplay(tmpX,tmpY,str,1);
      tmpX += 7;
    }
  }
  tmpY += 17;
  
 #ifdef PLUG_IN_CARRIER_MODULE
  if (chrEthPara[3][0]==0x55)
  {
  	if (layer2Light==3)
  	{
  		guiDisplay(130,tmpY, "��", 0);
  	}
  	else
  	{
  		guiDisplay(130,tmpY, "��", 1);
  	}
  }
  else
  {
  	if (layer2Light==3)
  	{
  		guiDisplay(130, tmpY, "��", 0);
  	}
  	else
  	{
  		guiDisplay(130, tmpY, "��", 1);
  	}
  }
	if (layer2Light==4)
	{
	  guiDisplay(24, 122, "ȷ��", 0);
	}
	else
	{
	  guiDisplay(24, 122, "ȷ��", 1);
	}
	guiDisplay(104,122,"ȡ��",1);
	lcdRefresh(17,144);

 #else
  
  guiDisplay(130,tmpY, "��", 1);  

	if (layer2Light==3)
	{
	  guiDisplay(24, 122, "ȷ��", 0);
	}
	else
	{
	  guiDisplay(24, 122, "ȷ��", 1);
	}
	guiDisplay(104,122,"ȡ��",1);
	
	lcdRefresh(17,160);
 #endif
}

/*******************************************************
��������:adjustEthParaLight
��������:����ETH����������
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void adjustEthParaLight(INT8U leftRight)
{
 	 INT8U tmpData;
 	 
 	 switch (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
 	 {
 	 	  case 0:
 	 	  case 1:
 	 	  case 2:
 	 	  	tmpData = 15;
 	 	  	break;
 	 	  
 	 	  default:
 	 	  	tmpData = 1;
 	 	  	break;
 	 }
 	 if (leftRight==1)  //�Ҽ�
 	 {
 	   if (keyLeftRight>=tmpData-1)
 	   {
 	 	   keyLeftRight = 0;
 	   }
 	   else
 	   {
 	 	   keyLeftRight++;
 	   }
 	 }
 	 else
 	 {
 	   if (keyLeftRight==0)
 	   {
 	 	   keyLeftRight = tmpData-1;
 	   }
 	   else
 	   {
 	 	   keyLeftRight--;
 	   }
 	 }
	 
	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<3)
	 {
	 	 if ((keyLeftRight+1)%4==0 && keyLeftRight<16)
	 	 {
	 	   if (leftRight==1)
	 	   {
	 	     keyLeftRight++;
	 	   }
	 	   else
	 	   {
	 	   	 keyLeftRight--;
	 	   }
	 	 }
	 }
}

/*******************************************************
��������:adjustSetMeterParaLight
��������:�������õ�����������
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void adjustSetMeterParaLight(INT8U leftRight,INT8U type)
{
 	 INT8U tmpData;
 	 
 	 if (type==1)
 	 {
 	   switch (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
 	   {
 	 	   case 0:
 	 	  	 tmpData = 4;
 	 	  	 break;
 	 	  	
 	 	   case 1:
 	 	   case 2:
 	 	   case 3:
 	 	     tmpData = 1;
 	 	     break;
 	 	    
 	 	   case 4:
 	 	  	 tmpData = 12;
 	 	  	 break;
 	 	  #ifndef PLUG_IN_CARRIER_MODULE
 	 	   case 5:
 	 	   case 6:
 	 	  	 tmpData = 2;
 	 	   	 break;
 	 	  #endif
 	 	 }
 	 }
 	 else
 	 {
 	   if (type==6)
 	   {
 	   	 tmpData = 2;
 	   }
 	   else
 	   {
   	   switch (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
   	   {
   	 	   case 0:
   	 	  	 tmpData = 4;
   	 	  	 break;
   	 	  	
   	 	   case 1:
   	 	   case 3:
   	 	   	 tmpData = 12;
   	 	   	 break;
   	 	   	 
   	 	   case 2:
   	 	   case 4:
   	 	     tmpData = 1;
   	 	     break;
  
   	 	   case 6:
   	 	   	 tmpData = 2;
   	 	   	 break;
   	 	 }
   	 }
 	 }
 	 if (leftRight==1)  //�Ҽ�
 	 {
 	   if (keyLeftRight>=tmpData-1)
 	   {
 	 	   keyLeftRight = 0;
 	   }
 	   else
 	   {
 	 	   keyLeftRight++;
 	   }
 	 }
 	 else
 	 {
 	   if (keyLeftRight==0)
 	   {
 	 	   keyLeftRight = tmpData-1;
 	   }
 	   else
 	   {
 	 	   keyLeftRight--;
 	   }
 	 }	 
}

/*******************************************************
��������:adjustCasParaLight
��������:������������������
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void adjustCasParaLight(INT8U leftRight)
{
 	 INT8U tmpData;
 	 
 	 switch (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
 	 {
 	 	  case 0:
 	 	  case 1:
 	 	  	tmpData = 1;
 	 	  	break;
 	 	  
 	 	  default:
 	 	  	tmpData = 9;
 	 	  	break;
 	 }
 	 if (leftRight==1)  //�Ҽ�
 	 {
 	   if (keyLeftRight>=tmpData-1)
 	   {
 	 	   keyLeftRight = 0;
 	   }
 	   else
 	   {
 	 	   keyLeftRight++;
 	   }
 	 }
 	 else
 	 {
 	   if (keyLeftRight==0)
 	   {
 	 	   keyLeftRight = tmpData-1;
 	   }
 	   else
 	   {
 	 	   keyLeftRight--;
 	   }
 	 }
}

/*******************************************************
��������:setCascadePara
��������:���ü�������(376.1���ҵ�����Լ(I�ͼ�����/ר���ն�))
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void setCascadePara(INT8U layer2Light,INT8U layer3Light)
{
	INT8U i, tmpX;
	char  str[2];
	
	menuInLayer = 3;
  
  #ifdef PLUG_IN_CARRIER_MODULE
	 guiLine(1,17,160,144,0);
	#else
	 guiLine(1,17,160,160,0);
	#endif
	
	guiLine(1,17,1,140,1);
	guiLine(160,17,160,140,1);
	guiLine(69,34,69,67,1);	
	guiLine(45,69,45,119,1);	
	
	guiLine(1,17,160,17,1);
	guiLine(1,34,160,34,1);	
	guiLine(1,51,160,51,1);	
	guiLine(1,68,160,68,1);	
	guiLine(1,85,160,85,1);	
	guiLine(1,102,160,102,1);
	guiLine(80,119,80,140,1);
	guiLine(1,119,160,119,1);
	guiLine(1,140,160,140,1);

	guiDisplay(33,18,"������������",1);
	guiDisplay(4, 35,"�����˿�",1);
	guiDisplay(4, 52,"������־",1);
	guiDisplay(4, 69,"�ն�1",1);
	guiDisplay(4, 86,"�ն�2",1);
	guiDisplay(4,103,"�ն�3",1);

  tmpX = 75;
  str[1] = '\0';
  for(i=0;i<1;i++)
  {
    str[0] = chrMp[0][i];
    if (layer2Light==0 && i==layer3Light)
    {
      guiDisplay(tmpX, 35, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 35, str, 1);
    }
    tmpX += 8;
  }
  
  if (layer2Light==1)
  {
    switch (chrMp[1][0])
    {
      case '1':
        guiDisplay(75, 52, "������", 0);
        break;
        
      case '2':
        guiDisplay(75, 52, "��������", 0);
        break;
        
      default:
        guiDisplay(75, 52, "������", 0);
        break;
    }
  }
  else
  {
    switch (chrMp[1][0])
    {
      case '1':
        guiDisplay(75, 52, "������", 1);
        break;
        
      case '2':
        guiDisplay(75, 52, "��������", 1);
        break;
        
      default:
        guiDisplay(75, 52, "������", 1);
        break;
    }
  }

  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<9;i++)
  {
    str[0] = chrMp[2][i];
    if (layer2Light==2 && i==layer3Light)
    {
      guiDisplay(tmpX, 69, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 69, str, 1);
    }
    tmpX += 8;
  }

  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<9;i++)
  {
    str[0] = chrMp[3][i];
    if (layer2Light==3 && i==layer3Light)
    {
      guiDisplay(tmpX, 86, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 86, str, 1);
    }
    tmpX += 8;
  }


  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<9;i++)
  {
    str[0] = chrMp[4][i];
    if (layer2Light==4 && i==layer3Light)
    {
      guiDisplay(tmpX, 103, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 103, str, 1);
    }
    tmpX += 8;
  }
  
	if (layer2Light==5)
	{
	  guiDisplay(24,122,"ȷ��",0);
	}
	else
	{
	  guiDisplay(24,122,"ȷ��",1);
	}
	guiDisplay(104,122,"ȡ��",1);

  #ifdef PLUG_IN_CARRIER_MODULE
	 lcdRefresh(17,144);
	#else
	 lcdRefresh(17,160);
	#endif
}


/*******************************************************
��������:setMainTain
��������:ά����ģʽ����(376.1���ҵ�����������Լ)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void setMainTain(INT8U lightNum)
{
	INT8U i, row, col;
	char  str[2];
	
	menuInLayer = 3;


 #ifdef PLUG_IN_CARRIER_MODULE
	guiLine(1,17,160,144,0);
 #else
	guiLine(1,17,160,160,0);  
 #endif

	guiLine(1,50,1,130,1);
	guiLine(160,50,160,130,1);
	guiLine(1,130,160,130,1);
	guiLine(1,50,160,50,1);
	guiLine(1,70,160,70,1);

	guiDisplay(25,52,"ά����ģʽ����",1);
	switch (lightNum)
	{
	  case 0:
	    guiDisplay(24, 82, "   ά��ģʽ   ", 0);
	    guiDisplay(24,102, "   ����ģʽ   ", 1);
	    break;
	  
	  case 1:
	    guiDisplay(24, 82, "   ά��ģʽ   ", 1);
	    guiDisplay(24,102, "   ����ģʽ   ", 0);
	    break;
	}

 #ifdef PLUG_IN_CARRIER_MODULE
	lcdRefresh(17, 145);
 #else
	lcdRefresh(17, 160);
 #endif
}

/*******************************************************
��������:setRs485Port2
��������:��2·485����(376.1���ҵ�����������Լ)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void setRs485Port2(INT8U lightNum)
{
	INT8U i, row, col;
	char  str[2];
	
	menuInLayer = 3;

 #ifdef PLUG_IN_CARRIER_MODULE
	guiLine(1,17,160,144,0);
 #else
	guiLine(1,17,160,160,0);
 #endif
	guiLine(1,50,1,130,1);
	guiLine(160,50,160,130,1);
	guiLine(1,130,160,130,1);
	guiLine(1,50,160,50,1);
	guiLine(1,70,160,70,1);

	guiDisplay(9,52,"��2·485�ڹ�������",1);
	switch (lightNum)
	{
	  case 0:
	    guiDisplay(24, 82, "   ����ӿ�   ", 0);
	    guiDisplay(24,102, "   ά���ӿ�   ", 1);
	    break;
	  
	  case 1:
	    guiDisplay(24, 82, "   ����ӿ�   ", 1);
	    guiDisplay(24,102, "   ά���ӿ�   ", 0);
	    break;
	}
 
 #ifdef PLUG_IN_CARRIER_MODULE
	lcdRefresh(17, 145);
 #else
	lcdRefresh(17, 160);
 #endif
}

/*******************************************************
��������:setLmProtocol
��������:����ͨ��ģ��Э������(376.1���ҵ�����������Լ)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void setLmProtocol(INT8U lightNum)
{
	INT8U i, row, col;
	char  str[2];
	
	menuInLayer = 3;

	guiLine(1,17,160,144,0);

	guiLine(1,50,1,130,1);
	guiLine(160,50,160,130,1);
	guiLine(1,130,160,130,1);
	guiLine(1,50,160,50,1);
	guiLine(1,70,160,70,1);

	guiDisplay(1,52,"����ͨ��ģ��Э������",1);
	switch (lightNum)
	{
	  case 0:
	    guiDisplay(24, 82, "Q/GDW376.2-2009", 0);
	    guiDisplay(24,102, "   ͸��Э��    ", 1);
	    break;
	  
	  case 1:
	    guiDisplay(24, 82, "Q/GDW376.2-2009", 1);
	    guiDisplay(24,102, "   ͸��Э��    ", 0);
	    break;
	}
 
	lcdRefresh(17, 145);
}


#ifdef LOAD_CTRL_MODULE
/*******************************************************
��������:ctrlAlarmDisplay
��������:���ƹ�����ʾ
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void ctrlAlarmDisplay(void)
{
   INT8U        i,j,tmpLine,tmpClose,prevAlarm,tmpAlarm;
   INT8U        pn;
   INT32U       tmpSecond;
   INT32U       tmpKw;
   INT16U       tmpWatt;
   char         say[20],str[12];
   static INT8U k;
   DATE_TIME    tmpTime,readTime;
   INT8U        tmpSetLine,tmpPeriod;
   INT8U        dataBuff[512];
   INT8U        tmpX;
   
   //�м�����ʱ,�ȴ�5�������澯����
   if (pageWait>5)
   {
   	  return;
   }
   
   //�п���Ͷ������ʱ,Ϊ��ʾ"����Ͷ������",�ȴ�2�������澯����
   if (ctrlCmdWaitTime>3 && ctrlCmdWaitTime!=0xfe)
   {
   	  return;
   }
   
	 if (ctrlStatus.numOfAlarm>0)
	 {
	   tmpClose = 0;
	   tmpSetLine = 0;
	   
	   guiLine(1,17,160,160,0);  //�����ʾ����
	   
	   menuInLayer = 99;         //���ڸ澯����

  	 switch (ctrlStatus.aQueue[ctrlStatus.nowAlarm])
  	 {
       case REMOTE_CTRL:       //ң��
       	 guiDisplay(48,LCD_LINE_1,"ң����բ",0);
       	 guiDisplay(1,LCD_LINE_3,"����",1);
       	 guiDisplay(1,LCD_LINE_4,"�֢�",1);
       	 guiDisplay(1,LCD_LINE_5,"�΢�",1);
       	 guiDisplay(1,LCD_LINE_6,"����",1);
       	 tmpX = 36;
       	 tmpLine = LCD_LINE_3;
       	 
       	 for(i=0;i<CONTROL_OUTPUT;i++)
       	 {
       	 	  switch(remoteCtrlConfig[i].status)
       	 	  {
       	 	  	 case 0:
       	 	  	 	 guiDisplay(tmpX,tmpLine,"δ����",1);
       	 	  	 	 tmpClose++;
       	 	  	 	 break;
       	 	  	 	 
       	 	  	 case CTRL_ALARM:
       	 	  	 	 tmpSecond = delayedSpike(sysTime,remoteCtrlConfig[i].remoteStart);
       	 	  	 	 strcpy(say,"�澯 ");
       	 	  	 	 strcat(say,digital2ToString(tmpSecond/60,str));
       	 	  	 	 strcat(say,":");
       	 	  	 	 strcat(say,digital2ToString(tmpSecond%60,str));
       	 	  	 	 guiDisplay(tmpX,tmpLine,say,1);
       	 	  	 	 break;

       	 	  	 case CTRL_JUMPED:
       	 	  	 	 tmpSecond = delayedSpike(sysTime,remoteCtrlConfig[i].remoteStart);
       	 	  	 	 strcpy(say,"����բ ");
       	 	  	 	 strcat(say,digital2ToString(tmpSecond/3600,str));
       	 	  	 	 strcat(say,":");
       	 	  	 	 strcat(say,digital2ToString(tmpSecond%3600/60,str));
       	 	  	 	 strcat(say,":");
       	 	  	 	 strcat(say,digital2ToString(tmpSecond%3600%60,str));
       	 	  	 	 guiDisplay(tmpX,tmpLine,say,1);
       	 	  	 	 break;

       	 	  	 case CTRL_CLOSE_GATE:
       	 	  	 	 tmpClose++;
       	 	  	 	 guiDisplay(tmpX,tmpLine,"�����բ",1);
       	 	  	 	 break;
       	 	  }
       	 	  
       	 	  tmpLine += 16;
       	 }
 	       
 	       if (tmpClose==CONTROL_OUTPUT)
   	     {
   	  	    ctrlStatus.allPermitClose[ctrlStatus.nowAlarm]=1;   //����·���ں�բ״̬
   	     }
   	     else
   	     {
   	  	    ctrlStatus.allPermitClose[ctrlStatus.nowAlarm]=0;   	 	  
   	     }
       	 break;
       	 
       case POWER_CTRL:       //��ǰ�����¸���
       case OBS_CTRL:         //Ӫҵ��ͣ��
       case WEEKEND_CTRL:     //���ݿ�
       case TIME_CTRL:        //ʱ�ι���
  	 	   pn = ctrlStatus.pn[ctrlStatus.nowAlarm];
  	 	   
	       switch (ctrlStatus.aQueue[ctrlStatus.nowAlarm])
	       {
	         case POWER_CTRL:
	           strcpy(say,"�����¸��� �ܼ���");
	           tmpAlarm = powerDownCtrl[pn-1].pwrDownAlarm;
	           tmpTime  = powerDownCtrl[pn-1].alarmEndTime;
	           tmpKw = powerDownCtrl[pn-1].powerDownLimit;
	           tmpWatt = powerDownCtrl[pn-1].powerLimitWatt;
	           break;

	         case OBS_CTRL:
	           strcpy(say,"Ӫҵ��ͣ�� �ܼ���");
	           tmpAlarm = obsCtrlConfig[pn-1].obsAlarm;
	           tmpTime  = obsCtrlConfig[pn-1].alarmEndTime;
	           tmpKw = obsCtrlConfig[pn-1].obsLimit;
	           break;
	           
	         case WEEKEND_CTRL:
	           strcpy(say,"���ݹ��ء� �ܼ���");
	           tmpAlarm = wkdCtrlConfig[pn-1].wkdAlarm;
	           tmpTime  = wkdCtrlConfig[pn-1].alarmEndTime;
	           tmpKw = wkdCtrlConfig[pn-1].wkdLimit;
	           break;

	         case TIME_CTRL:
	           strcpy(say,"ʱ�ι��ء� �ܼ���");
	           tmpAlarm = periodCtrlConfig[pn-1].ctrlPara.prdAlarm;
	           tmpTime  = periodCtrlConfig[pn-1].ctrlPara.alarmEndTime;
	           
             tmpKw = 0;
             
             //��ȡ��ǰʱ��ε�ʱ������
             if ((tmpPeriod = getPowerPeriod(sysTime)) != 0)
             {
               if (getPowerLimit(pn, periodCtrlConfig[pn-1].ctrlPara.ctrlPeriod, tmpPeriod, (INT8U *)&tmpKw)==FALSE)
               {
               	  tmpKw = 0;
               }
             }
	           break;
	       }
	 	  	 strcat(say,digital2ToString(pn,str));
	       guiDisplay(5,LCD_LINE_1,say,0);
	       
         if (ctrlStatus.aQueue[ctrlStatus.nowAlarm]!=POWER_CTRL)
         {
           tmpKw = countAlarmLimit((INT8U *)&tmpKw, 0x2, 0x0,&tmpWatt);
         }
 	       strcpy(say,"��ֵ");
 	       if (tmpKw>9999999 && tmpWatt==0)
 	       {
 	         tmpKw/=1000;
 	         strcat(say,intToString(tmpKw,3,str));
           strcat(say,"Mw");
 	         guiDisplay(1,LCD_LINE_2,say,1);
 	       }
 	       else
 	       {
 	         strcat(say,intToString(tmpKw,3,str));
 	         if (tmpWatt>0)
 	         {
 	           strcat(say,".");
 	           strcat(say,intToString(tmpWatt,3,str));
 	         }
           strcat(say,"Kw");
 	         guiDisplay(33,LCD_LINE_2,say,1);
 	       }
   	     
   	     tmpKw = 0x0;
  	     if (calcRealPower(dataBuff, pn))
         {
    	     if (dataBuff[GP_WORK_POWER+1] != 0xFF && dataBuff[GP_WORK_POWER+1] != 0xee)
    	     {
             tmpKw  = dataBuff[GP_WORK_POWER+1];
             tmpKw |= (dataBuff[GP_WORK_POWER+2]&0xF)<<8;
             tmpKw |= (dataBuff[GP_WORK_POWER]&0x10)<<8;
             tmpKw |= (dataBuff[GP_WORK_POWER]&0x07)<<13;
             tmpKw  = countAlarmLimit((INT8U*)&tmpKw, 0x2, 0x00,&tmpWatt);
           }
         }
	      
	       strcpy(say,"��ǰ");
	       strcat(say,intToString(tmpKw,3,str));
	       if (tmpWatt>0)
	       {
	         strcat(say,".");
	         tmpWatt /=10;
	         if (tmpWatt<10)
	         {
	         	  strcat(say,"0");
	         }
	         strcat(say,intToString(tmpWatt,3,str));
	       }
         strcat(say,"Kw");
         
	       guiDisplay(33,LCD_LINE_3,say,1);
	       
	       guiDisplay(33,LCD_LINE_5,"  ��",1);
	       guiDisplay(33,LCD_LINE_6,"�֢�",1);
	       guiDisplay(33,LCD_LINE_7,"�΢�",1);
	       guiDisplay(33,LCD_LINE_8,"  ��",1);
	       
	       prevAlarm = 0;
	       for(j=0;j<CONTROL_OUTPUT;j++)
	       {
     	     strcpy(say,"");
  
     	     if (((powerCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x00)
     	     {
     	     	  strcpy(say,"δ�趨");
     	     }
     	     else
     	     {
    	       tmpSetLine++;
    	       if (((powerCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x01)
    	       {
    	       	  strcpy(say,"����բ");
    	       }
    	       else
    	       {
    	       	  switch(tmpAlarm)
    	       	  {
    	       	  	 case CTRL_ALARM:
  	 	  	 	         if (prevAlarm==0)
  	 	  	 	         {
  	 	  	 	           tmpSecond = delayedSpike(sysTime,tmpTime);
  	 	  	 	           strcpy(say,"�澯");
  	 	  	 	           strcat(say,digital2ToString(tmpSecond/60,str));
  	 	  	 	           strcat(say,":");
  	 	  	 	           strcat(say,digital2ToString(tmpSecond%60,str));
  	 	  	 	           prevAlarm++;
  	 	  	 	         }
  	 	  	 	         else
  	 	  	 	         {
  	 	  	 	      	    strcpy(say,"�ȴ�");
  	 	  	 	         }
  	 	  	 	         break;
  	 	  	 	       
  	 	  	 	       case CTRL_JUMPED:
    	       	  	   strcpy(say,"�ȴ�");
    	       	  	   break;
    	       	  	 
    	       	  	 case CTRL_CLOSE_GATE:
    	       	  	   strcpy(say,"�����բ");
    	       	  	   tmpClose++;
                    break;
                    
                  case CTRL_ALARM_CANCEL:
    	       	  	   strcpy(say,"�澯ȡ��");
    	       	  	   tmpClose++;
                  	 break;

                  case CTRL_ALARM_AUTO_CLOSE:
    	       	  	   strcpy(say,"�Զ���բ");
    	       	  	   tmpClose++;
                  	 break;
                  
                  default:
    	       	  	   strcpy(say,"��բ");
    	       	  	   tmpClose++;
                  	 break;
               }
    	       }
    	     }
    	     switch(j)
  	 	   	 {
  	 	   	 	  case 0:
  	 	   	 	  	guiDisplay(65,LCD_LINE_5,say,1);
  	 	   	 	  	break;
  	 	   	 	  case 1:
  	 	   	 	  	guiDisplay(65,LCD_LINE_6,say,1);
  	 	   	 	  	break;
  	 	   	 	  case 2:
  	 	   	 	  	guiDisplay(65,LCD_LINE_7,say,1);
  	 	   	 	  	break;
  	 	   	 	  case 3:
  	 	   	 	  	guiDisplay(65,LCD_LINE_8,say,1);
  	 	   	 	  	break;
  	 	   	 }
	 	   	 }
	 	   	 if (tmpClose == tmpSetLine)
	 	   	 {
	 	   	 	  ctrlStatus.allPermitClose[ctrlStatus.nowAlarm]=1;   //�����趨����·���ں�բ״̬
	 	   	 }
	 	   	 else
	 	   	 {
            ctrlStatus.allPermitClose[ctrlStatus.nowAlarm]=0;
	 	   	 }
       	 break;

       case MONTH_CTRL:       //�µ��
       case CHARGE_CTRL:      //�����
	 	     pn = ctrlStatus.pn[ctrlStatus.nowAlarm];
   	 	   
 	       switch (ctrlStatus.aQueue[ctrlStatus.nowAlarm])
 	       {
 	         case MONTH_CTRL:
 	           strcpy(say,"�µ�ء��� �ܼ���");
 	           tmpAlarm = monthCtrlConfig[pn-1].monthAlarm;
 	           tmpTime  = monthCtrlConfig[pn-1].mthAlarmTimeOut;
 	           tmpKw = monthCtrlConfig[pn-1].monthCtrl;
 	           break;
 	         
 	         case CHARGE_CTRL:
 	           strcpy(say,"����ء��� �ܼ���");
 	           tmpAlarm = chargeCtrlConfig[pn-1].chargeAlarm;
 	           tmpTime  = chargeCtrlConfig[pn-1].alarmTimeOut;
 	           tmpKw = chargeCtrlConfig[pn-1].cutDownLimit;
 	           break;
 	       }
 	 	  	 strcat(say,digital2ToString(pn,str));
 	       guiDisplay(5,LCD_LINE_1,say,0);
 	       
 	       if (ctrlStatus.aQueue[ctrlStatus.nowAlarm]==MONTH_CTRL)
 	       {
 	       	  strcpy(say,"��ֵ");
 	       }
 	       else
 	       {
    	      strcpy(say,"��բ");
    	      
            if (tmpKw&0x10000000)
    	      {
    	 	      strcat(say, "-");
    	      }
    	   }
    	   tmpKw = countAlarmLimit((INT8U *)&tmpKw, 0x3, 0x0,&tmpWatt);
 	       if (tmpKw>999999)
 	       {
 	          tmpKw/=1000;
 	          strcat(say,intToString(tmpKw,3,str));
            strcat(say,"Mwh");
 	       }
 	       else
 	       {
 	          strcat(say,intToString(tmpKw,3,str));
            strcat(say,"Kwh");
 	       }
 	       guiDisplay(33,LCD_LINE_2,say,1);

 	       if (ctrlStatus.aQueue[ctrlStatus.nowAlarm]==MONTH_CTRL)
 	       {
       	   strcpy(say,"��ǰ");
       	   readTime = timeHexToBcd(sysTime);
           if (readMeterData(dataBuff, pn, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &readTime, 0) == TRUE)	
           {
               if (dataBuff[GP_MONTH_WORK+3] != 0xFF)
               {
                 tmpKw = dataBuff[GP_MONTH_WORK+3]
                          | dataBuff[GP_MONTH_WORK+4] << 8 
                          | dataBuff[GP_MONTH_WORK+5] << 16 
                          | (((dataBuff[GP_MONTH_WORK]&0x01)<<6)|(dataBuff[GP_MONTH_WORK]&0x10)|(dataBuff[GP_MONTH_WORK+6]&0x0f))<<24;
               }
           }
 	       }
 	       else
 	       {
            strcpy(say,"ʣ��");
       	    readTime = timeHexToBcd(sysTime);
            if (readMeterData(dataBuff, pn, LEFT_POWER, 0x0, &readTime, 0)==TRUE)
            {
              if (dataBuff[0] != 0xFF || dataBuff[0] != 0xEE)
              {
                //��ǰʣ�����
                tmpKw = dataBuff[0] 
                      | dataBuff[1] << 8 
                      | dataBuff[2] << 16 
                      | (dataBuff[3]&0xf) << 24;
              }
            }
 	       } 	       
    	   tmpKw = countAlarmLimit((INT8U *)&tmpKw, 0x3, 0x0,&tmpWatt);
    	   if (dataBuff[3]&0x10)
    	   {
    	   	 strcat(say, "-");
    	   }
 	       strcat(say,intToString(tmpKw,3,str));
         strcat(say,"Kwh");
         
 	       guiDisplay(33,LCD_LINE_3,say,1);
 	       
 	       guiDisplay(33,LCD_LINE_5,"  ��",1);
 	       guiDisplay(33,LCD_LINE_6,"�֢�",1);
 	       guiDisplay(33,LCD_LINE_7,"�΢�",1);
 	       guiDisplay(33,LCD_LINE_8,"  ��",1);
 	       
 	       prevAlarm = 0;
 	       for(j=0;j<CONTROL_OUTPUT;j++)
 	       {
    	     strcpy(say,"");

    	     if (((electCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x00)
    	     {
    	     	  strcpy(say,"δ�趨");
    	     }
    	     else
    	     {
     	       tmpSetLine++;
     	       if (((electCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x01)
     	       {
     	       	  strcpy(say,"����բ");
     	       }
     	       else
     	       {
     	       	  switch(tmpAlarm)
     	       	  {
     	       	  	 case CTRL_ALARM:
   	 	  	 	         if (prevAlarm==0)
   	 	  	 	         {
   	 	  	 	           tmpSecond = delayedSpike(tmpTime,sysTime);
   	 	  	 	           strcpy(say,"�澯");
   	 	  	 	           strcat(say,digital2ToString(tmpSecond/60,str));
   	 	  	 	           strcat(say,":");
   	 	  	 	           strcat(say,digital2ToString(tmpSecond%60,str));
   	 	  	 	           prevAlarm++;
   	 	  	 	         }
   	 	  	 	         else
   	 	  	 	         {
   	 	  	 	      	    strcpy(say,"�ȴ�");
   	 	  	 	         }
   	 	  	 	         break;
   	 	  	 	       
   	 	  	 	       case CTRL_JUMPED:
     	       	  	   strcpy(say,"�ȴ�");
     	       	  	   break;
     	       	  	 
     	       	  	 case CTRL_CLOSE_GATE:
     	       	  	   strcpy(say,"�����բ");
     	       	  	   tmpClose++;
                     break;
                     
                   case CTRL_ALARM_CANCEL:
     	       	  	   strcpy(say,"�澯ȡ��");
     	       	  	   tmpClose++;
                   	 break;

                   case CTRL_ALARM_AUTO_CLOSE:
     	       	  	   strcpy(say,"�Զ���բ");
     	       	  	   tmpClose++;
                   	 break;

                  default:
    	       	  	   strcpy(say,"��բ");
    	       	  	   tmpClose++;
                  	 break;
                }
     	       }
     	     }
     	     
     	     switch(j)
   	 	   	 {
   	 	   	 	 case 0:
   	 	   	 	   guiDisplay(65,LCD_LINE_5,say,1);
   	 	   	 	   break;
   	 	   	 	 
   	 	   	 	 case 1:
   	 	   	 	   guiDisplay(65,LCD_LINE_6,say,1);
   	 	   	 	   break;
   	 	   	 	 
   	 	   	 	 case 2:
   	 	   	 	   guiDisplay(65,LCD_LINE_7,say,1);
   	 	   	 	   break;
   	 	   	 	 
   	 	   	 	 case 3:
   	 	   	 	   guiDisplay(65,LCD_LINE_8,say,1);
   	 	   	 	   break;         	 	   	 	  	
   	 	   	 }
 	 	   	 }
 	 	   	 if (tmpClose == tmpSetLine)
 	 	   	 {
 	 	   	 	   ctrlStatus.allPermitClose[ctrlStatus.nowAlarm]=1;   //�����趨����·���ں�բ״̬
 	 	   	 }
 	 	   	 else
 	 	   	 {
            ctrlStatus.allPermitClose[ctrlStatus.nowAlarm]=0;
 	 	   	 }
       	 break;
       	 
       case REMINDER_FEE:
       	 guiDisplay(15,LCD_LINE_2,"�߷Ѹ澯",1);
       	 break;
       
       case PAUL_ELECTRICITY:
       	 guiDisplay(40,LCD_LINE_2,"�ն˱���",1);
       	 
       	 tmpSecond = delayedSpike(staySupportStatus.endStaySupport,sysTime);
       	 strcpy(say,"");
       	 strcat(say,digital2ToString(tmpSecond/3600,str));
       	 strcat(say,":");
       	 strcat(say,digital2ToString(tmpSecond%3600/60,str));
       	 strcat(say,":");
       	 strcat(say,digital2ToString(tmpSecond%3600%60,str));
       	 guiDisplay(44,LCD_LINE_3,say,1);
       	 break;
 	   }
 	   
     k++;
     if (k>2)
     {
       k=0;
       ctrlStatus.nowAlarm++;
  	   if (ctrlStatus.nowAlarm>=ctrlStatus.numOfAlarm)
  	   {
  	 	    ctrlStatus.nowAlarm = 0;
  	   }
  	 }
  	 
  	 lcdRefresh(17,160);
   }
}
#endif //LOAD_CTRL_MODULE


//���ֻ��͹�������************************end***************************


/*******************************************************
��������:refreshTitleTime
��������:ˢ��̧ͷ��ʾʱ��(376.1������ͨ��)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void refreshTitleTime(void)
{
  char     str[6];

  #ifdef LOAD_CTRL_MODULE
    guiLine(103,1,160,15,0);
    guiAscii(103, 1, digital2ToString(sysTime.hour,str),1);
    guiAscii(118, 1, ":", 1);
    guiAscii(124, 1, digital2ToString(sysTime.minute,str),1);
    guiAscii(139, 1, ":", 1);
    guiAscii(145, 1, digital2ToString(sysTime.second,str),1);
  #else
    guiLine(124,1,160,16,0);
    guiAscii(124, 1, digital2ToString(sysTime.hour,str),1);
    guiAscii(139, 1, ":", 1);
    guiAscii(145, 1, digital2ToString(sysTime.minute,str),1);
  #endif
  
  lcdRefresh(1,16);
}

/*******************************************************
��������:layer1Menu
��������:һ��˵�(376.1������ͨ��)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void layer1Menu(int	num)
{
	 int i=0;
	 
	 INT8U startX,startY;
   
   #ifdef PLUG_IN_CARRIER_MODULE
	  #ifdef MENU_FOR_CQ_CANON
	   startX = 37;
	   startY = 28;
	   guiLine(1,17,160,160,0);
	  #else
	   startX = 24;
	   startY = LCD_LINE_2;
	   guiLine(1,17,160,144,0);
	  #endif
	 #else
	  startX = 40;
	  startY = 38;
	  guiLine(1,17,160,160,0);
	  
	  guiDisplay(56,19,"���˵�",1);
	 #endif
	 
	 menuInLayer=1;
   
   for(i=0;i<MAX_LAYER1_MENU;i++)
   {
     if (i == num)
     {
        guiDisplay(startX,startY+i*17,layer1MenuItem[i],0);
     }
     else
     {
      	guiDisplay(startX,startY+i*17,layer1MenuItem[i],1);
     }
   }

  #ifdef PLUG_IN_CARRIER_MODULE
   #ifndef MENU_FOR_CQ_CANON
    lcdRefresh(17,145);
   #else
    lcdRefresh(17,160);
   #endif
  #else
   lcdRefresh(17,160);
  #endif
}

/*******************************************************
��������:freeQueryMpLink
��������:�ͷŲ�ѯ�����Ĳ���������
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void freeQueryMpLink(void)
{
	 struct cpAddrLink    *tmpNode;

   while (queryMpLink!=NULL)
   {
     tmpNode = queryMpLink;
     queryMpLink = queryMpLink->next;
     
     free(tmpNode);
   }
}

/*******************************************************
��������:terminalInfo
��������:������������Ϣ
���ú���:
�����ú���:
�������:num
�������:
����ֵ:void
*******************************************************/
void terminalInfo(INT8U num)
{
   INT8U  i;
   INT8U  tmpY;
   char   str[35],say[50];
	 char   sayStr[10];
	 INT32U integer,decimal;
	 INT32U disData;
	 INT8U  sign;
	 INT8U  dataBuff[LENGTH_OF_PARA_RECORD];
   INT8U  visionBuff[LENGTH_OF_ENERGY_RECORD];
   INT8U  reqBuff[LENGTH_OF_REQ_RECORD];
   INT16U offset;

	 #ifdef LOAD_CTRL_MODULE
	  menuInLayer = 2;
	  guiLine(1,17,160,160,0);
	 #else
	  menuInLayer = 3;
	  guiLine(1,17,160,144,0);
	 #endif

	 switch(num)
	 {
	   case 0:      //��ʾ�汾��Ϣ
       strcpy(say, "�ն˵�ַ:");
       #ifdef TE_ADDR_USE_BCD_CODE
        strcat(say,int8uToHex(addrField.a2[1],str));
        strcat(say,int8uToHex(addrField.a2[0],str));
       #else
        strcat(say,intToString(addrField.a2[1]<<8 | addrField.a2[0],3,str));
        strcat(say,"[");
        strcat(say,int8uToHex(addrField.a2[1],str));
        strcat(say,int8uToHex(addrField.a2[0],str));
        strcat(say,"]");
       #endif
	  	 #ifdef PLUG_IN_CARRIER_MODULE
        guiDisplay(1,LCD_LINE_1+8,say,1);
	  	 #else
        guiDisplay(1,LCD_LINE_1+4,say,1);
       #endif
       
       strcpy(say,"����������:");
       strcat(say,digitalToChar(addrField.a1[1]>>4));
       strcat(say,digitalToChar(addrField.a1[1]&0xf));
       strcat(say,digitalToChar(addrField.a1[0]>>4));
       strcat(say,digitalToChar(addrField.a1[0]&0xf));
	  	 #ifdef PLUG_IN_CARRIER_MODULE
        guiDisplay(1,LCD_LINE_2+8,say,1);
       #else
        guiDisplay(1,LCD_LINE_2+4,say,1);
       #endif
       
	  	 	 
	  	 strcpy(say,"����汾:");
	  	 strcat(say,vers);
	  	 
	  	 #ifdef PLUG_IN_CARRIER_MODULE
	  	  guiDisplay(1,LCD_LINE_4,say,1);
	  	 #else
	  	  guiDisplay(1,LCD_LINE_4-8,say,1);
	  	 #endif
	  	 
	  	 strcpy(say,"��������:");
	  	 strcat(say,dispenseDate);
	  	 #ifdef PLUG_IN_CARRIER_MODULE
	  	  guiDisplay(1,LCD_LINE_5,say,1);
	  	 #else
	  	  guiDisplay(1,LCD_LINE_5-8,say,1);
	  	 #endif
	  	 
	  	 strcpy(say,"Ӳ���汾:");
	  	 strcat(say,hardwareVers);
	  	 #ifdef PLUG_IN_CARRIER_MODULE
	  	  guiDisplay(1,LCD_LINE_6,say,1);
	  	 #else
	  	  guiDisplay(1,LCD_LINE_6-8,say,1);
	  	 #endif
	  	 
	  	 strcpy(say,"��������:");
	  	 strcat(say,hardwareDate);
	  	 #ifdef PLUG_IN_CARRIER_MODULE
	  	  guiDisplay(1,LCD_LINE_7,say,1);
	  	 #else
	  	  guiDisplay(1,LCD_LINE_7-8,say,1);
	  	 #endif
	  	 
	  	 #ifndef PLUG_IN_CARRIER_MODULE
	  	  guiDisplay(1,LCD_LINE_8,"��ǰʱ��:",1);
	  	  sprintf(say,"20%02d-%02d-%02d %02d:%02d:%02d",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
	  	  guiDisplay(1,LCD_LINE_9,say,1);
	  	 #endif
	  	 break;
	  	 	 
	   case 1:      //����ָʾ
     	 strcpy(say,"��IP:");
     	 strcat(say,intToIpadd(ipAndPort.ipAddr[0]<<24 | ipAndPort.ipAddr[1]<<16 | ipAndPort.ipAddr[2]<<8 | ipAndPort.ipAddr[3],str));
     	 guiDisplay(1,LCD_LINE_1,say,1);
     	 strcpy(say,"��IP:");
     	 strcat(say,intToIpadd(ipAndPort.ipAddrBak[0]<<24 | ipAndPort.ipAddrBak[1]<<16 | ipAndPort.ipAddrBak[2]<<8 | ipAndPort.ipAddrBak[3],str));
     	 guiDisplay(1,LCD_LINE_2,say,1);
     	 strcpy(say,"�˿�:��");
     	 strcat(say,intToString(ipAndPort.port[1]<<8 | ipAndPort.port[0],3,str));
     	 //guiDisplay(1,LCD_LINE_3,say,1);
     	 strcat(say," ��");
     	 strcat(say,intToString(ipAndPort.portBak[1]<<8 | ipAndPort.portBak[0],3,str));
     	 guiDisplay(1,LCD_LINE_3,say,1);
       switch (moduleType)
       {
         case GPRS_SIM300C:
         case GPRS_GR64:
         case CDMA_DTGS800:
         case GPRS_M590E:
         case GPRS_M72D:
				 case LTE_AIR720H:
           strcpy(say,"APN:");
           strcat(say,(char *)ipAndPort.apn);
           guiAscii(1,LCD_LINE_4,say,1);
           break;
       }
	  	 
	  	 strcpy(say,"");
	  	 switch(moduleType)
	  	 {
	  	 	 case GPRS_SIM300C:
	  	 	 case GPRS_GR64:
	  	 	 case GPRS_M590E:
	  	 	 case GPRS_M72D:
      	   strcpy(say,"GPRS:");
      	   break;
				 
	  	 	 case LTE_AIR720H:
      	   strcpy(say,"LTE:");
      	   break;
	  	 	 	
	  	 	 case CDMA_DTGS800:
	  	 	 case CDMA_CM180:
      	   strcpy(say,"CDMA:");
      	   break;
	  	 }
	  	 if(wlModemFlag.loginSuccess == TRUE)
	  	 {
	  	 	 strcat(say,"�ѵ�¼�ɹ�");
	  	 }
	  	 else
	  	 {
	  	 	 strcat(say,"��¼δ�ɹ�");
	  	 }
	  	 guiDisplay(1,LCD_LINE_5,say,1);

	  	 	 
	  	 strcpy(say,"�ϴ�����:");
	  	 if (lastHeartBeat.month == 0 && lastHeartBeat.day == 0 && lastHeartBeat.hour == 0 && lastHeartBeat.minute == 0 && lastHeartBeat.second == 0)
	  	 {
	  	 	 strcat(say,"δ��������");
	  	 }
	  	 else
	  	 {
	  	 	 strcat(say,digital2ToString(lastHeartBeat.month,str));
	  	 	 strcat(say,"-");
	  	 	 strcat(say,digital2ToString(lastHeartBeat.day,str));
	  	 	 strcat(say," ");
	  	 	 strcat(say,digital2ToString(lastHeartBeat.hour,str));
	  	 	 strcat(say,":");
	  	 	 strcat(say,digital2ToString(lastHeartBeat.minute,str));
	  	 }
	  	 guiDisplay(1,LCD_LINE_6,say,1);

	 	   #ifdef PLUG_IN_CARRIER_MODULE
	 	    guiDisplay(1, LCD_LINE_7, "������IP:", 1);
	 	   #else
	 	    guiDisplay(1, LCD_LINE_7, "�ն�IP:", 1);
	 	   #endif
	 	   
	 	   guiDisplay(20, LCD_LINE_8, intToIpadd(wlLocalIpAddr,str), 1);
	  	 break;
	  	 	 
   	 case 2:
   	   tmpY = LCD_LINE_1+8;
   	  #ifdef LOAD_CTRL_MODULE
   	   for(i=1;i<3;i++)
   	  #else
   	   for(i=1;i<4;i++)
   	  #endif
   	   {
   	     strcpy(say,"�˿�");
   	     if (i<3)
   	     {
   	       strcat(say,intToString(i+1,3,str));
   	     }
   	     else
   	     {
   	     	 strcat(say,"31");
   	     }
   	     strcat(say,"������:");
   	     strcat(say,intToString(teCopyRunPara.para[i].copyInterval,3,str));
   	     guiDisplay(1,tmpY,say,1);
   	     tmpY += 16;
   	     
 	       strcpy(say,"�´�:");
 	       strcat(say,digital2ToString(copyCtrl[i].nextCopyTime.month,str));
 	       strcat(say,"-");
 	       strcat(say,digital2ToString(copyCtrl[i].nextCopyTime.day,str));
	       strcat(say," ");
 	       strcat(say,digital2ToString(copyCtrl[i].nextCopyTime.hour,str));
 	       strcat(say,":");
 	       strcat(say,digital2ToString(copyCtrl[i].nextCopyTime.minute,str));
	       strcat(say,":");
 	       strcat(say,digital2ToString(copyCtrl[i].nextCopyTime.second,str));
 	       guiDisplay(1,tmpY,say,1);
 	       tmpY += 24;
 	     }
	  	 break;
	  
	  #ifdef PLUG_IN_CARRIER_MODULE
	   case 3:
	   	 guiDisplay(17, 17, "����ͨ��ģ����Ϣ",0);
	   	 if (carrierModuleType==NO_CARRIER_MODULE)
	   	 {
	   	  #ifdef LM_SUPPORT_UT
	   	   if (0x55==lmProtocol)
	   	   {
	   	     guiDisplay(1, 65, "͸��Э�鱾��ͨ��ģ��", 1);
	   	   }
	   	   else
	   	   {
	   	  #endif
	   	  
	   	     guiDisplay(1, 65, "δ��⵽����ͨ��ģ��", 1);
	   	  
	   	  #ifdef LM_SUPPORT_UT
	   	   }
	   	  #endif
	   	 }
	   	 else
	   	 {
	   	   strcpy(say,"���̴���:");
	   	   say[9] = carrierFlagSet.productInfo[0];
	   	   say[10] = carrierFlagSet.productInfo[1];
	   	   say[11] = '\0';
	   	  
	   	  #ifndef DKY_SUBMISSION 
	   	   switch(carrierModuleType)
	   	   {
	   	   	 case EAST_SOFT_CARRIER:
	   	   	 	 strcat(say,"����");
	   	   	 	 break;
	   	   	 case CEPRI_CARRIER:
	   	   	 	 strcat(say,"���Ժ");
	   	   	 	 break;
	   	   	 case SR_WIRELESS:
	   	   	 	 strcat(say,"ɣ��");
	   	   	 	 break;
	   	   	 case RL_WIRELESS:
	   	   	 	 strcat(say,"���");
	   	   	 	 break;
	   	   	 case MIA_CARRIER:
	   	   	 	 strcat(say,"����΢");
	   	   	 	 break;
	   	   	 case TC_CARRIER:
	   	   	 	 strcat(say,"����");
	   	   	 	 break;
	   	   	 case LME_CARRIER:
	   	   	 	 strcat(say,"����΢");
	   	   	 	 break;
	   	   	 case FC_WIRELESS:
	   	   	 	 strcat(say,"��Ѹ��");
	   	   	 	 break;
	   	   	 case SC_WIRELESS:
	   	   	 	 strcat(say,"����");
	   	   	 	 break;
	   	   }
	   	  #endif
	   	   guiDisplay(1, 33, say, 1);

	   	   strcpy(say,"оƬ����:");
	   	   say[9] = carrierFlagSet.productInfo[2];
	   	   say[10] = carrierFlagSet.productInfo[3];
	   	   say[11] = '\0';
	   	   guiDisplay(1, 49, say, 1);
	   	 
	   	   sprintf(say,"�汾:%02x%02x", carrierFlagSet.productInfo[8], carrierFlagSet.productInfo[7]);
	   	   guiDisplay(1, 65, say, 1);
	   	   sprintf(say,"�汾����:%02x-%02x-%02x", carrierFlagSet.productInfo[6], carrierFlagSet.productInfo[5], carrierFlagSet.productInfo[4]);
	   	   guiDisplay(1, 81, say, 1);
	   	   guiDisplay(1, 96, "��ǰ������ʽ:", 1);
	   	   if (localCopyForm==0xaa)
	   	   {
	   	     strcpy(say,"·����������");
	   	   }
	   	   else
	   	   {
	   	     strcpy(say,"��������������");
	   	   }
	   	   guiDisplay(33, 112, say, 1);
	   	   
	   	   if (carrierModuleType==SR_WIRELESS || carrierModuleType==FC_WIRELESS)
	   	   {
	   	     if (carrierFlagSet.wlNetOk==3)
	   	     {
	   	     	 guiDisplay(17, 128, "ģ���������", 1);
	   	     }
	   	     else
	   	     {
	   	     	 guiDisplay(17, 128, "ģ������δ���", 1);
	   	     }
	   	   }
	   	 }
	   	 break;
	   	 
	   case 4:
       memset(dataBuff,0xee,LENGTH_OF_PARA_RECORD);
       covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
	   	 //guiDisplay( 1,17,"  ��������ʵʱ����  ",0);

	     tmpY = 17;
	     for(i=0;i<8;i++)
	     {
	    	 sign = 0;
    	   switch(i)
    	   {
    	     case 0:
	    	     disData = dataBuff[POWER_INSTANT_WORK] | dataBuff[POWER_INSTANT_WORK+1]<<8 | dataBuff[POWER_INSTANT_WORK+2]<<16;
    	       strcpy(say,"�й�������=");
    	       break;
    	       
    	     case 1:
	    	     disData = dataBuff[POWER_PHASE_A_WORK] | dataBuff[POWER_PHASE_A_WORK+1]<<8 | dataBuff[POWER_PHASE_A_WORK+2]<<16;
    	       strcpy(say,"A=");
    	       break;
    	       
    	     case 2:
	    	     disData = dataBuff[POWER_PHASE_B_WORK] | dataBuff[POWER_PHASE_B_WORK+1]<<8 | dataBuff[POWER_PHASE_B_WORK+2]<<16;
    	       strcpy(say,"B=");
    	       break;
    	       
    	     case 3:
	    	     disData = dataBuff[POWER_PHASE_C_WORK] | dataBuff[POWER_PHASE_C_WORK+1]<<8 | dataBuff[POWER_PHASE_C_WORK+2]<<16;
    	       strcpy(say,"C=");
    	       break;

    	     case 4:
	    	     disData = dataBuff[POWER_INSTANT_NO_WORK] | dataBuff[POWER_INSTANT_NO_WORK+1]<<8 | dataBuff[POWER_INSTANT_NO_WORK+2]<<16;
    	       strcpy(say,"�޹�������=");
    	       break;
    	       
    	     case 5:
	    	     disData = dataBuff[POWER_PHASE_A_NO_WORK] | dataBuff[POWER_PHASE_A_NO_WORK+1]<<8 | dataBuff[POWER_PHASE_A_NO_WORK+2]<<16;
    	       strcpy(say,"A=");
    	       break;
    	       
    	     case 6:
	    	     disData = dataBuff[POWER_PHASE_B_NO_WORK] | dataBuff[POWER_PHASE_B_NO_WORK+1]<<8 | dataBuff[POWER_PHASE_B_NO_WORK+2]<<16;
    	       strcpy(say,"B=");
    	       break;
    	       
    	     case 7:
	    	     disData = dataBuff[POWER_PHASE_C_NO_WORK] | dataBuff[POWER_PHASE_C_NO_WORK+1]<<8 | dataBuff[POWER_PHASE_C_NO_WORK+2]<<16;
    	       strcpy(say,"C=");
    	       break;
    	   }
	    	 
	    	 if (disData&0x800000)
	    	 {
	    	 	 disData &= 0x7fffff;
	    	 	 sign = 1;
	    	 }

         decimal = (disData>>12 & 0xf)*1000
                   +(disData>>8 & 0xf)*100
                   +(disData>>4 & 0xf)*10
                   +(disData & 0xf);
         integer = (disData>>20 & 0xf)*10+(disData>>16 & 0xf);
         if (sign==1)
         {
         	 strcat(say,"-");
         }
         strcat(say,floatToString(integer,decimal,4,4,str));

    	   if (i==0 || i==4)
    	   {
    	      guiDisplay(1, tmpY,say,1);
    	   }
    	   else
    	   {
    	      guiDisplay(10, tmpY,say,1);    	   	 
    	   }
    	   
    	   tmpY += 16;
	     }
	   	 break;

	   case 5:
       memset(dataBuff,0xee,LENGTH_OF_PARA_RECORD);
       covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
	   	 //guiDisplay( 1,17,"  ��������ʵʱ����  ",0);
	     tmpY = 17;
	     for(i=0;i<8;i++)
	     {
	    	 sign = 0;
    	   switch(i)
    	   {
    	     case 0:
	    	     disData = dataBuff[POWER_INSTANT_APPARENT] | dataBuff[POWER_INSTANT_APPARENT+1]<<8 | dataBuff[POWER_INSTANT_APPARENT+2]<<16;
    	       strcpy(say,"���ڹ�����=");
    	       break;
    	       
    	     case 1:
	    	     disData = dataBuff[POWER_PHASE_A_APPARENT] | dataBuff[POWER_PHASE_A_APPARENT+1]<<8 | dataBuff[POWER_PHASE_A_APPARENT+2]<<16;
    	       strcpy(say,"A=");
    	       break;
    	       
    	     case 2:
	    	     disData = dataBuff[POWER_PHASE_B_APPARENT] | dataBuff[POWER_PHASE_B_APPARENT+1]<<8 | dataBuff[POWER_PHASE_B_APPARENT+2]<<16;
    	       strcpy(say,"B=");
    	       break;
    	       
    	     case 3:
	    	     disData = dataBuff[POWER_PHASE_C_APPARENT] | dataBuff[POWER_PHASE_C_APPARENT+1]<<8 | dataBuff[POWER_PHASE_C_APPARENT+2]<<16;
    	       strcpy(say,"C=");
    	       break;

    	     case 4:
    	       strcpy(say,"����������=");
	    	     disData = dataBuff[TOTAL_POWER_FACTOR] | dataBuff[TOTAL_POWER_FACTOR+1]<<8;
    	       break;
    	       
    	     case 5:
	    	     disData = dataBuff[FACTOR_PHASE_A] | dataBuff[FACTOR_PHASE_A+1]<<8;
    	       strcpy(say,"A=");
    	       break;
    	       
    	     case 6:
	    	     disData = dataBuff[FACTOR_PHASE_B] | dataBuff[FACTOR_PHASE_B+1]<<8;
    	       strcpy(say,"B=");
    	       break;
    	       
    	     case 7:
	    	     disData = dataBuff[FACTOR_PHASE_C] | dataBuff[FACTOR_PHASE_C+1]<<8;
    	       strcpy(say,"C=");
    	       break;
    	   }
	    	 
	    	 if (i<4)
	    	 {
	    	   if (disData&0x800000)
	    	   {
	    	 	   disData &= 0x7fffff;
	    	 	   sign = 1;
	    	   }

           decimal = (disData>>12 & 0xf)*1000
                   +(disData>>8 & 0xf)*100
                   +(disData>>4 & 0xf)*10
                   +(disData & 0xf);
           integer = (disData>>20 & 0xf)*10+(disData>>16 & 0xf);
           if (sign==1)
           {
         	   strcat(say,"-");
           }
           strcat(say,floatToString(integer,decimal,4,4,str));
         }
         else
         {
           if (disData&0x8000)
           {
          	  disData &= 0x7fff;
          	  sign = 1;
           }
           integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10+(disData>>4 & 0xf);
           decimal = (disData & 0xf);
           if (sign==1)
           {
             strcat(say,"-");
           }
           strcat(say,floatToString(integer,decimal,1,1,str));
         }

    	   if (i==0 || i==4)
    	   {
    	      guiDisplay(1, tmpY,say,1);
    	   }
    	   else
    	   {
    	      guiDisplay(10, tmpY,say,1);    	   	 
    	   }
    	   
    	   tmpY += 16;
	     }
	   	 
	   	 break;

	   case 6:
       memset(dataBuff,0xee,LENGTH_OF_PARA_RECORD);
	   	 covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
	   	 //guiDisplay( 1,17,"  ��������ʵʱ����  ",0);
	   	 guiDisplay( 1, 17,"�����ѹ",1);	   	 
	   	 guiDisplay( 1, 81,"�������",1);
	   	 
	   	 tmpY = 33;
       for(i=0;i<6;i++)
       {
         switch(i)
         {
           case 0:
             strcpy(say,"A=");
    	    	 disData = dataBuff[VOLTAGE_PHASE_A] | dataBuff[VOLTAGE_PHASE_A+1]<<8;
             break;
             
           case 1:
             strcpy(say,"B=");
   	    	   disData = dataBuff[VOLTAGE_PHASE_B] | dataBuff[VOLTAGE_PHASE_B+1]<<8;
             break;
             
           case 2:
             strcpy(say,"C=");
  	    	   disData = dataBuff[VOLTAGE_PHASE_C] | dataBuff[VOLTAGE_PHASE_C+1]<<8;
             break;

           case 3:
             strcpy(say,"A=");
    	    	 disData = dataBuff[CURRENT_PHASE_A] | dataBuff[CURRENT_PHASE_A+1]<<8 | dataBuff[CURRENT_PHASE_A+2]<<16;
             break;
             
           case 4:
             strcpy(say,"B=");
 	    	     disData = dataBuff[CURRENT_PHASE_B] | dataBuff[CURRENT_PHASE_B+1]<<8 | dataBuff[CURRENT_PHASE_B+2]<<16;
             break;
             
           case 5:
             strcpy(say,"C=");
  	    	   disData = dataBuff[CURRENT_PHASE_C] | dataBuff[CURRENT_PHASE_C+1]<<8 | dataBuff[CURRENT_PHASE_C+2]<<16;
             break;
         }
         
         if (i<=2)
         {                  
            decimal = disData& 0xf;
            integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10 + (disData>>4 & 0xf);
            strcat(say,floatToString(integer,decimal,1,1,str));
         }
         else
         {
            decimal = bcdToHex(disData&0xffff);
            integer = bcdToHex(disData>>12& 0xff);
            strcat(say,floatToString(integer,decimal,3,2,str));
         }
         
         guiAscii(10,tmpY,say,1);
         
         if (i==2)
         {
         	 tmpY += 32;
         }
         else
         {
         	 tmpY += 16;
         }
       }
	   	 break;
	   	 
	  #else
	  
	   case 3:
       memset(dataBuff,0xee,LENGTH_OF_PARA_RECORD);
       covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
	   	 guiDisplay( 1,17,"  ��������ʵʱ����  ",0);

	     tmpY = 33;
	     for(i=0;i<8;i++)
	     {
	    	 sign = 0;
    	   switch(i)
    	   {
    	     case 0:
	    	     disData = dataBuff[POWER_INSTANT_WORK] | dataBuff[POWER_INSTANT_WORK+1]<<8 | dataBuff[POWER_INSTANT_WORK+2]<<16;
    	       strcpy(say,"�й�������=");
    	       break;
    	       
    	     case 1:
	    	     disData = dataBuff[POWER_PHASE_A_WORK] | dataBuff[POWER_PHASE_A_WORK+1]<<8 | dataBuff[POWER_PHASE_A_WORK+2]<<16;
    	       strcpy(say,"A=");
    	       break;
    	       
    	     case 2:
	    	     disData = dataBuff[POWER_PHASE_B_WORK] | dataBuff[POWER_PHASE_B_WORK+1]<<8 | dataBuff[POWER_PHASE_B_WORK+2]<<16;
    	       strcpy(say,"B=");
    	       break;
    	       
    	     case 3:
	    	     disData = dataBuff[POWER_PHASE_C_WORK] | dataBuff[POWER_PHASE_C_WORK+1]<<8 | dataBuff[POWER_PHASE_C_WORK+2]<<16;
    	       strcpy(say,"C=");
    	       break;

    	     case 4:
	    	     disData = dataBuff[POWER_INSTANT_NO_WORK] | dataBuff[POWER_INSTANT_NO_WORK+1]<<8 | dataBuff[POWER_INSTANT_NO_WORK+2]<<16;
    	       strcpy(say,"�޹�������=");
    	       break;
    	       
    	     case 5:
	    	     disData = dataBuff[POWER_PHASE_A_NO_WORK] | dataBuff[POWER_PHASE_A_NO_WORK+1]<<8 | dataBuff[POWER_PHASE_A_NO_WORK+2]<<16;
    	       strcpy(say,"A=");
    	       break;
    	       
    	     case 6:
	    	     disData = dataBuff[POWER_PHASE_B_NO_WORK] | dataBuff[POWER_PHASE_B_NO_WORK+1]<<8 | dataBuff[POWER_PHASE_B_NO_WORK+2]<<16;
    	       strcpy(say,"B=");
    	       break;
    	       
    	     case 7:
	    	     disData = dataBuff[POWER_PHASE_C_NO_WORK] | dataBuff[POWER_PHASE_C_NO_WORK+1]<<8 | dataBuff[POWER_PHASE_C_NO_WORK+2]<<16;
    	       strcpy(say,"C=");
    	       break;
    	   }
	    	 
	    	 if (disData&0x800000)
	    	 {
	    	 	 disData &= 0x7fffff;
	    	 	 sign = 1;
	    	 }

         decimal = (disData>>12 & 0xf)*1000
                   +(disData>>8 & 0xf)*100
                   +(disData>>4 & 0xf)*10
                   +(disData & 0xf);
         integer = (disData>>20 & 0xf)*10+(disData>>16 & 0xf);
         if (sign==1)
         {
         	 strcat(say,"-");
         }
         strcat(say,floatToString(integer,decimal,4,4,str));

    	   if (i==0 || i==4)
    	   {
    	      guiDisplay(1, tmpY,say,1);
    	   }
    	   else
    	   {
    	      guiDisplay(10, tmpY,say,1);    	   	 
    	   }
    	   
    	   tmpY += 16;
	     }
	   	 break;

	   case 4:
       memset(dataBuff,0xee,LENGTH_OF_PARA_RECORD);
       covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
	   	 guiDisplay( 1,17,"  ��������ʵʱ����  ",0);
	     tmpY = 33;
	     for(i=0;i<8;i++)
	     {
	    	 sign = 0;
    	   switch(i)
    	   {
    	     case 0:
	    	     disData = dataBuff[POWER_INSTANT_APPARENT] | dataBuff[POWER_INSTANT_APPARENT+1]<<8 | dataBuff[POWER_INSTANT_APPARENT+2]<<16;
    	       strcpy(say,"���ڹ�����=");
    	       break;
    	       
    	     case 1:
	    	     disData = dataBuff[POWER_PHASE_A_APPARENT] | dataBuff[POWER_PHASE_A_APPARENT+1]<<8 | dataBuff[POWER_PHASE_A_APPARENT+2]<<16;
    	       strcpy(say,"A=");
    	       break;
    	       
    	     case 2:
	    	     disData = dataBuff[POWER_PHASE_B_APPARENT] | dataBuff[POWER_PHASE_B_APPARENT+1]<<8 | dataBuff[POWER_PHASE_B_APPARENT+2]<<16;
    	       strcpy(say,"B=");
    	       break;
    	       
    	     case 3:
	    	     disData = dataBuff[POWER_PHASE_C_APPARENT] | dataBuff[POWER_PHASE_C_APPARENT+1]<<8 | dataBuff[POWER_PHASE_C_APPARENT+2]<<16;
    	       strcpy(say,"C=");
    	       break;

    	     case 4:
    	       strcpy(say,"����������=");
	    	     disData = dataBuff[TOTAL_POWER_FACTOR] | dataBuff[TOTAL_POWER_FACTOR+1]<<8;
    	       break;
    	       
    	     case 5:
	    	     disData = dataBuff[FACTOR_PHASE_A] | dataBuff[FACTOR_PHASE_A+1]<<8;
    	       strcpy(say,"A=");
    	       break;
    	       
    	     case 6:
	    	     disData = dataBuff[FACTOR_PHASE_B] | dataBuff[FACTOR_PHASE_B+1]<<8;
    	       strcpy(say,"B=");
    	       break;
    	       
    	     case 7:
	    	     disData = dataBuff[FACTOR_PHASE_C] | dataBuff[FACTOR_PHASE_C+1]<<8;
    	       strcpy(say,"C=");
    	       break;
    	   }
	    	 
	    	 if (i<4)
	    	 {
	    	   if (disData&0x800000)
	    	   {
	    	 	   disData &= 0x7fffff;
	    	 	   sign = 1;
	    	   }

           decimal = (disData>>12 & 0xf)*1000
                   +(disData>>8 & 0xf)*100
                   +(disData>>4 & 0xf)*10
                   +(disData & 0xf);
           integer = (disData>>20 & 0xf)*10+(disData>>16 & 0xf);
           if (sign==1)
           {
         	   strcat(say,"-");
           }
           strcat(say,floatToString(integer,decimal,4,4,str));
         }
         else
         {
           if (disData&0x8000)
           {
          	  disData &= 0x7fff;
          	  sign = 1;
           }
           integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10+(disData>>4 & 0xf);
           decimal = (disData & 0xf);
           if (sign==1)
           {
             strcat(say,"-");
           }
           strcat(say,floatToString(integer,decimal,1,1,str));
         }

    	   if (i==0 || i==4)
    	   {
    	      guiDisplay(1, tmpY,say,1);
    	   }
    	   else
    	   {
    	      guiDisplay(10, tmpY,say,1);    	   	 
    	   }
    	   
    	   tmpY += 16;
	     }
	   	 
	   	 break;

	   case 5:
       memset(dataBuff,0xee,LENGTH_OF_PARA_RECORD);
	   	 covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
	   	 guiDisplay( 1,17,"  ��������ʵʱ����  ",0);
	   	 guiDisplay( 1, 33,"�����ѹ",1);	   	 
	   	 guiDisplay( 1, 97,"�������",1);
	   	 
	   	 tmpY = 49;
       for(i=0;i<6;i++)
       {
         switch(i)
         {
           case 0:
             strcpy(say,"A=");
    	    	 disData = dataBuff[VOLTAGE_PHASE_A] | dataBuff[VOLTAGE_PHASE_A+1]<<8;
             break;
             
           case 1:
             strcpy(say,"B=");
   	    	   disData = dataBuff[VOLTAGE_PHASE_B] | dataBuff[VOLTAGE_PHASE_B+1]<<8;
             break;
             
           case 2:
             strcpy(say,"C=");
  	    	   disData = dataBuff[VOLTAGE_PHASE_C] | dataBuff[VOLTAGE_PHASE_C+1]<<8;
             break;

           case 3:
             strcpy(say,"A=");
    	    	 disData = dataBuff[CURRENT_PHASE_A] | dataBuff[CURRENT_PHASE_A+1]<<8 | dataBuff[CURRENT_PHASE_A+2]<<16;
             break;
             
           case 4:
             strcpy(say,"B=");
 	    	     disData = dataBuff[CURRENT_PHASE_B] | dataBuff[CURRENT_PHASE_B+1]<<8 | dataBuff[CURRENT_PHASE_B+2]<<16;
             break;
             
           case 5:
             strcpy(say,"C=");
  	    	   disData = dataBuff[CURRENT_PHASE_C] | dataBuff[CURRENT_PHASE_C+1]<<8 | dataBuff[CURRENT_PHASE_C+2]<<16;
             break;
         }
         
         if (i<=2)
         {                  
            decimal = disData& 0xf;
            integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10 + (disData>>4 & 0xf);
            strcat(say,floatToString(integer,decimal,1,1,str));
         }
         else
         {
            decimal = bcdToHex(disData&0xffff);
            integer = bcdToHex(disData>>12& 0xff);
            strcat(say,floatToString(integer,decimal,3,2,str));
         }
         
         guiAscii(10,tmpY,say,1);
         
         if (i==2)
         {
         	 tmpY += 32;
         }
         else
         {
         	 tmpY += 16;
         }
       }
	   	 break;

	   case 6:
       memset(dataBuff,0xee,LENGTH_OF_PARA_RECORD);
       
	   	 covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
	   	 guiDisplay( 1,17,"  ��������ʵʱ����  ",0);
	   	 guiDisplay( 1, 33,"��ѹ��λ��",1);
	   	 guiDisplay( 1, 97,"������λ��",1);
	   	 tmpY = 49;
       for(i=0;i<6;i++)
       {
         switch(i)
         {
           case 0:
             strcpy(say,"A=");
    	    	 disData = dataBuff[PHASE_ANGLE_V_A] | dataBuff[PHASE_ANGLE_V_A+1]<<8;
             break;
             
           case 1:
             strcpy(say,"B=");
   	    	   disData = dataBuff[PHASE_ANGLE_V_B] | dataBuff[PHASE_ANGLE_V_B+1]<<8;
             break;
             
           case 2:
             strcpy(say,"C=");
  	    	   disData = dataBuff[PHASE_ANGLE_V_C] | dataBuff[PHASE_ANGLE_V_C+1]<<8;
             break;

           case 3:
             strcpy(say,"A=");
    	    	 disData = dataBuff[PHASE_ANGLE_C_A] | dataBuff[PHASE_ANGLE_C_A+1]<<8;
             break;
             
           case 4:
             strcpy(say,"B=");
   	    	   disData = dataBuff[PHASE_ANGLE_C_B] | dataBuff[PHASE_ANGLE_C_B+1]<<8;
             break;
             
           case 5:
             strcpy(say,"C=");
  	    	   disData = dataBuff[PHASE_ANGLE_C_C] | dataBuff[PHASE_ANGLE_C_C+1]<<8;
             break;
         }
         
         if (disData&0x8000)
         {
          	disData &= 0x7fff;
          	sign = 1;
         }
         integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10+(disData>>4 & 0xf);
         decimal = (disData & 0xf);
         if (sign==1)
         {
           strcat(say,"-");
         }
         strcat(say,floatToString(integer,decimal,1,1,str));
         
         guiAscii(10,tmpY,say,1);
         
         if (i==2)
         {
         	 tmpY += 32;
         }
         else
         {
         	 tmpY += 16;
         }
       }
	   	 break;
	  #endif    //no PLUG_IN_CARRIER_MODULE
	   
	   case 7:
	   case 8:
	   case 9:
	   case 10:
	   case 11:
	   case 12:
	   case 13:
	   case 14:
       memset(visionBuff, 0xee, LENGTH_OF_ENERGY_RECORD);       
	   	 covertAcSample(NULL, visionBuff, NULL, 1, sysTime);
	   	 
	   	 switch(num)
	   	 {
	   	   case 7:
	   	     guiDisplay( 1, 17, "      �����й�      ",0);
	   	     guiDisplay( 1, 33, "�й�������(kWh)",1);
	   	     guiDisplay( 1, 49, "  ��ǰ����",1);
	   	     offset = POSITIVE_WORK_OFFSET;
	   	     break;
	   	     
	   	   case 8:
	   	     guiDisplay( 1, 17, "      �����й�      ",0);
	   	     guiDisplay( 1, 33, "�й�������(kWh)",1);
	   	     guiDisplay( 1, 49, "  ��ǰ����",1);
	   	     offset = NEGTIVE_WORK_OFFSET;
	   	   	 break;

	   	   case 9:
	   	     guiDisplay( 1, 17, "      �����޹�      ",0);
	   	     guiDisplay( 1, 33, "�޹�������(kVarh)",1);
	   	     guiDisplay( 1, 49, "  ��ǰ����",1);
	   	     offset = POSITIVE_NO_WORK_OFFSET;
	   	   	 break;

	   	   case 10:
	   	     guiDisplay( 1, 17, "      �����޹�      ",0);
	   	     guiDisplay( 1, 33, "�޹�������(kVarh)",1);
	   	     guiDisplay( 1, 49, "  ��ǰ����",1);
	   	     offset = NEGTIVE_NO_WORK_OFFSET;
	   	   	 break;

	   	   case 11:
	   	     guiDisplay( 1, 17, "       Q1�޹�       ",0);
	   	     guiDisplay( 1, 33, "�޹�������(kVarh)",1);
	   	     guiDisplay( 1, 49, "  ��ǰһ����",1);
	   	     offset = QUA1_NO_WORK_OFFSET;
	   	   	 break;
	   	   	 
	   	   case 12:
	   	     guiDisplay( 1, 17, "       Q4�޹�       ",0);
	   	     guiDisplay( 1, 33, "�޹�������(kVarh)",1);
	   	     guiDisplay( 1, 49, "  ��ǰ������",1);
	   	     offset = QUA4_NO_WORK_OFFSET;
	   	   	 break;
	   	   	 
	   	   case 13:
	   	     guiDisplay( 1, 17, "       Q2�޹�       ",0);
	   	     guiDisplay( 1, 33, "�޹�������(kVarh)",1);
	   	     guiDisplay( 1, 49, "  ��ǰ������",1);
	   	     offset = QUA2_NO_WORK_OFFSET;
	   	   	 break;
	   	   	 
	   	   case 14:
	   	     guiDisplay( 1, 17, "       Q3�޹�       ",0);
	   	     guiDisplay( 1, 33, "�޹�������(kVarh)",1);
	   	     guiDisplay( 1, 49, "  ��ǰ������",1);
	   	     offset = QUA3_NO_WORK_OFFSET;
	   	   	 break;
	   	 }
	   	 
	   	 tmpY = 65;
       for(i=0;i<5;i++)
       {
       	  switch(i)
       	  {
       	  	case 0:
       	      strcpy(sayStr,"��=");
       	      break;
       	      
       	  	case 1:
       	      strcpy(sayStr,"��=");
       	      break;

       	  	case 2:
       	      strcpy(sayStr,"��=");
       	      break;
       	      
       	  	case 3:
       	      strcpy(sayStr,"ƽ=");
       	      break;
       	      
       	  	case 4:
       	      strcpy(sayStr,"��=");
       	      break;
       	  }
       	  
       	  if (dataBuff[POSITIVE_WORK_OFFSET+i*4]!=0xee)
       	  {
             decimal = bcdToHex(visionBuff[offset+i*4]);
             integer = bcdToHex(visionBuff[offset+i*4+1] |visionBuff[offset+i*4+2]<<8
                     | visionBuff[offset+i*4+3]<<16);
             strcpy(str, floatToString(integer,decimal,2,2,str));
             strcat(sayStr,str);
       	  }
       	  
       	  guiDisplay(16,tmpY,sayStr,1);
       	  
       	  if (i==0)
       	  {
       	  	 #ifdef PLUG_IN_CARRIER_MODULE
       	  	  tmpY += 16;
       	  	 #else
       	  	  tmpY += 32;
       	  	 #endif
       	  }
       	  else
       	  {
       	  	 tmpY += 16;
       	  }
       }
	   	 break;

	   case 15:
	   case 16:
       memset(reqBuff, 0xee, LENGTH_OF_REQ_RECORD);
	   	 covertAcSample(NULL, NULL, reqBuff, 1, sysTime);
	   	 
	   	 switch(num)
	   	 {
	   	   case 15:
	   	   	 #ifdef PLUG_IN_CARRIER_MODULE
	    	    guiDisplay( 1, 17, "��ǰ�����й�����:",1);
	   	   	 #else
	   	      guiDisplay( 1, 17, "      �й�����      ",0); 
	    	    guiDisplay( 1, 33, "��ǰ�����й�:",1);
	    	   #endif
       	   strcpy(sayStr, "");
       	   if (reqBuff[REQ_POSITIVE_WORK_OFFSET]!=0xee)
       	   {
             decimal = bcdToHex(reqBuff[REQ_POSITIVE_WORK_OFFSET]|reqBuff[REQ_POSITIVE_WORK_OFFSET+1]<<8);
             integer = bcdToHex(reqBuff[REQ_POSITIVE_WORK_OFFSET+2]);
             strcpy(str, floatToString(integer,decimal,4,4,str));
             strcat(sayStr,str);
             strcat(sayStr,"kWar");
       	   }
       	   #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay(49, 33, sayStr,1);
       	   #else
	   	      guiDisplay(49, 49, sayStr,1);
	   	     #endif
	      
	         strcpy(sayStr,"������ʱ��:");
	         #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay( 1, 49, sayStr,1);
	   	     #else
	   	      guiDisplay( 1, 65, sayStr,1);
	   	     #endif
	   	     
	   	     strcpy(sayStr, "");
	         if (reqBuff[REQ_TIME_P_WORK_OFFSET]!=0xee)
	         {
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_P_WORK_OFFSET+3]),str));
	           strcat(sayStr,"-");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_P_WORK_OFFSET+2]),str));
	           strcat(sayStr," ");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_P_WORK_OFFSET+1]),str));
	           strcat(sayStr,":");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_P_WORK_OFFSET]),str));
	         }

	         #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay(49, 65, sayStr,1);
	    	    guiDisplay( 1, 81, "��ǰ�����й�����:",1);
	         #else
	   	      guiDisplay(49, 81, sayStr,1);
	    	    guiDisplay( 1, 97, "��ǰ�����й�:",1);
	   	     #endif
	   	     
       	   strcpy(sayStr, "");
       	   if (reqBuff[REQ_NEGTIVE_WORK_OFFSET]!=0xee)
       	   {
             decimal = bcdToHex(reqBuff[REQ_NEGTIVE_WORK_OFFSET]|reqBuff[REQ_NEGTIVE_WORK_OFFSET+1]<<8);
             integer = bcdToHex(reqBuff[REQ_NEGTIVE_WORK_OFFSET+2]);
             strcpy(str, floatToString(integer,decimal,4,4,str));
             strcat(sayStr,str);
             strcat(sayStr,"kWar");
       	   }
       	   #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay(49, 97, sayStr,1);
       	   #else
	   	      guiDisplay(49,113, sayStr,1);
	   	     #endif
	      
	         strcpy(sayStr,"������ʱ��:");
	         #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay( 1,113, sayStr,1);
	         #else
	   	      guiDisplay( 1,129, sayStr,1);
	   	     #endif
	   	     strcpy(sayStr, "");
	         if (reqBuff[REQ_TIME_N_WORK_OFFSET]!=0xee)
	         {
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_N_WORK_OFFSET+3]),str));
	           strcat(sayStr,"-");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_N_WORK_OFFSET+2]),str));
	           strcat(sayStr," ");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_N_WORK_OFFSET+1]),str));
	           strcat(sayStr,":");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_N_WORK_OFFSET]),str));
	         }
	         #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay(49, 129, sayStr,1);
	         #else
	   	      guiDisplay(49, 145, sayStr,1);
	   	     #endif
	   	     break;
	   	     
	   	   case 16:
	   	     #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay( 1, 17, "��ǰ�����޹�����:",1);
	   	     #else
	   	      guiDisplay( 1, 17, "      �޹�����      ",0);
	   	      guiDisplay( 1, 33, "��ǰ�����޹�:",1);
	   	     #endif
       	   strcpy(sayStr, "");
       	   if (reqBuff[REQ_POSITIVE_NO_WORK_OFFSET]!=0xee)
       	   {
             decimal = bcdToHex(reqBuff[REQ_POSITIVE_NO_WORK_OFFSET]|reqBuff[REQ_POSITIVE_NO_WORK_OFFSET+1]<<8);
             integer = bcdToHex(reqBuff[REQ_POSITIVE_NO_WORK_OFFSET+2]);
             strcpy(str, floatToString(integer,decimal,4,4,str));
             strcat(sayStr,str);
             strcat(sayStr,"kVar");
       	   }
       	   #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay(49, 33, sayStr,1);
       	   #else
	   	      guiDisplay(49, 49, sayStr,1);
	   	     #endif
	      
	         strcpy(sayStr,"������ʱ��:");
	         #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay( 1, 49, sayStr,1);
	         #else
	   	      guiDisplay( 1, 65, sayStr,1);
	   	     #endif
	   	     strcpy(sayStr, "");
	         if (reqBuff[REQ_TIME_P_NO_WORK_OFFSET]!=0xee)
	         {
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_P_NO_WORK_OFFSET+3]),str));
	           strcat(sayStr,"-");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_P_NO_WORK_OFFSET+2]),str));
	           strcat(sayStr," ");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_P_NO_WORK_OFFSET+1]),str));
	           strcat(sayStr,":");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_P_NO_WORK_OFFSET]),str));
	         }
	         #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay(49, 65, sayStr,1);
	   	     
	   	      guiDisplay( 1, 81, "��ǰ�����޹�����:",1);
	         #else
	   	      guiDisplay(49, 81, sayStr,1);
	   	     
	   	      guiDisplay( 1, 97, "��ǰ�����޹�:",1);
	   	     #endif
       	   strcpy(sayStr, "");
       	   if (reqBuff[REQ_NEGTIVE_NO_WORK_OFFSET]!=0xee)
       	   {
             decimal = bcdToHex(reqBuff[REQ_NEGTIVE_NO_WORK_OFFSET]|reqBuff[REQ_NEGTIVE_NO_WORK_OFFSET+1]<<8);
             integer = bcdToHex(reqBuff[REQ_NEGTIVE_NO_WORK_OFFSET+2]);
             strcpy(str, floatToString(integer,decimal,4,4,str));
             strcat(sayStr,str);
             strcat(sayStr,"kVar");
       	   }
       	   #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay(49, 97, sayStr,1);
       	   #else
	   	      guiDisplay(49,113, sayStr,1);
	   	     #endif
	      
	         strcpy(sayStr,"������ʱ��:");
	         #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay( 1,113, sayStr,1);
	   	     #else
	   	      guiDisplay( 1,129, sayStr,1);
	   	     #endif
	   	     strcpy(sayStr, "");
	         if (reqBuff[REQ_TIME_N_NO_WORK_OFFSET]!=0xee)
	         {
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_N_NO_WORK_OFFSET+3]),str));
	           strcat(sayStr,"-");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_N_NO_WORK_OFFSET+2]),str));
	           strcat(sayStr," ");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_N_NO_WORK_OFFSET+1]),str));
	           strcat(sayStr,":");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_N_NO_WORK_OFFSET]),str));
	         }
	         #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay( 49,129, sayStr,1);
	         #else
	   	      guiDisplay( 49,145, sayStr,1);
	   	     #endif
	   	     break;
	   	 }
	   	 break;
	   	 
	   case 17:
       memset(dataBuff,0xee,LENGTH_OF_PARA_RECORD);
       
	   	 covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
	   	 //guiDisplay( 1,17,"  ��������ʵʱ����  ",0);
	   	 guiDisplay( 1, 17,"��ѹ��λ��",1);
	   	 guiDisplay( 1, 81,"������λ��",1);
	   	 tmpY = 33;
       for(i=0;i<6;i++)
       {
         switch(i)
         {
           case 0:
             strcpy(say,"A=");
    	    	 disData = dataBuff[PHASE_ANGLE_V_A] | dataBuff[PHASE_ANGLE_V_A+1]<<8;
             break;
             
           case 1:
             strcpy(say,"B=");
   	    	   disData = dataBuff[PHASE_ANGLE_V_B] | dataBuff[PHASE_ANGLE_V_B+1]<<8;
             break;
             
           case 2:
             strcpy(say,"C=");
  	    	   disData = dataBuff[PHASE_ANGLE_V_C] | dataBuff[PHASE_ANGLE_V_C+1]<<8;
             break;

           case 3:
             strcpy(say,"A=");
    	    	 disData = dataBuff[PHASE_ANGLE_C_A] | dataBuff[PHASE_ANGLE_C_A+1]<<8;
             break;
             
           case 4:
             strcpy(say,"B=");
   	    	   disData = dataBuff[PHASE_ANGLE_C_B] | dataBuff[PHASE_ANGLE_C_B+1]<<8;
             break;
             
           case 5:
             strcpy(say,"C=");
  	    	   disData = dataBuff[PHASE_ANGLE_C_C] | dataBuff[PHASE_ANGLE_C_C+1]<<8;
             break;
         }
         
         if (disData&0x8000)
         {
          	disData &= 0x7fff;
          	sign = 1;
         }
         integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10+(disData>>4 & 0xf);
         decimal = (disData & 0xf);
         if (sign==1)
         {
           strcat(say,"-");
         }
         strcat(say,floatToString(integer,decimal,1,1,str));
         
         guiAscii(10,tmpY,say,1);
         
         if (i==2)
         {
         	 tmpY += 32;
         }
         else
         {
         	 tmpY += 16;
         }
       }
	   	 break;
	  }
	  
	  lcdRefresh(17,160);
}

/*******************************************************
��������:stringUpDown
��������:�������ϻ��¼�ʱ�ַ����ı䶯
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void stringUpDown(char *processStr, INT8U timeChar, INT8U upDown)
{ 
   if (upDown==1)    //����
   {
    	if (processStr[timeChar]<=0x30)
    	{
    		processStr[timeChar]=0x39;
    	}
    	else
    	{
        processStr[timeChar]--;
    	}
   }
   else              //����
   {
  	 	if (processStr[timeChar]>=0x39)
  	 	{
  	 		processStr[timeChar]=0x30;
  	 	}
  	 	else
  	 	{
        processStr[timeChar]++;
  	 	}
   }
}

/*******************************************************
��������:showInputTime
��������:��ʾ�������ڿ�
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void showInputTime(INT8U layer3Light)
{
   INT8U tmpX, i;
   char  str[2];
   
   guiDisplay(1,60,"��ѡ���ѯ����:",1);
   tmpX = 40;
   for(i=0;i<8;i++)
   {
     str[0] = queryTimeStr[i];
     str[1] = '\0';
     if (i==layer3Light)
     {
       guiDisplay(tmpX,80,str,0);
     }
     else
     {
     	 guiDisplay(tmpX,80,str,1);
     }
     tmpX += 8;
     if (i==3 || i==5)
     {
        guiDisplay(tmpX,80,"-",1);
     	  tmpX += 8;
     }
   }
}

/*******************************************************
��������:checkInputTime
��������:��������������Ƿ�ϸ�
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
BOOL checkInputTime(void)
{
  INT16U tmpData;
  
  menuQueryTime = sysTime;
  
  //�ж���
  tmpData = (queryTimeStr[0]-0x30)*1000+(queryTimeStr[1]-0x30)*100+(queryTimeStr[2]-0x30)*10+(queryTimeStr[3]-0x30);
  if (tmpData<1970 || tmpData>2099)
  {
  	 guiDisplay(20, 110, "������벻��ȷ!", 1);
  	 lcdRefresh(110,130);
  	 return FALSE;
  }
  else
  {
  	 menuQueryTime.year = tmpData-2000; 
  }
  
  //�ж���
  tmpData = (queryTimeStr[4]-0x30)*10+(queryTimeStr[5]-0x30);
  if (tmpData<1 || tmpData>12)
  {
  	 guiDisplay(20, 110, "�·����벻��ȷ!", 1);
  	 lcdRefresh(110,130);
  	 return FALSE;
  }
  else
  {
  	 menuQueryTime.month = tmpData;
  }
  
  //�ж���      	    		 	 	 	  
  tmpData = (queryTimeStr[6]-0x30)*10+(queryTimeStr[7]-0x30);
  if (tmpData<1 || tmpData>31)
  {
  	 guiDisplay(20, 110, "�������벻��ȷ!",1);
  	 lcdRefresh(110,130);
  	 return FALSE;
  }
  else
  {
  	 menuQueryTime.day = tmpData;
  }
  
  return TRUE;
}

/*******************************************************
��������:fillTimeStr
��������:�õ�ǰ��������ѯ�����ַ���
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void fillTimeStr(void)
{
 	 char str[4];
 	 
 	 strcpy(queryTimeStr,"20");
   strcat(queryTimeStr, digital2ToString(sysTime.year,str));
   strcat(queryTimeStr, digital2ToString(sysTime.month,str));
   strcat(queryTimeStr, digital2ToString(sysTime.day,str));
}

/*******************************************************
��������:inputPassWord
��������:��������˵�
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void inputPassWord(INT8U lightNum)
{
   INT8U tmpX, i;
   char  str[2];
   
	#ifdef PLUG_IN_CARRIER_MODULE
	 #ifdef MENU_FOR_CQ_CANON
	  guiLine(1,17,160,160,0); //����
	 #else
	  guiLine(1,17,160,144,0); //����
	 #endif
	#else
	  guiLine(1,17,160,160,0); //����	
	#endif
	 
	 menuInLayer = 20;        //�˵������20��(����ֻ������������)
   
   guiDisplay(40,60,"����������",1);
   
   //����
   guiLine(50,80,50,100,1);
   guiLine(110,80,110,100,1);
   guiLine(50,80,110,80,1);
   guiLine(50,100,110,100,1);
   
   tmpX = 58;
   for(i=0;i<6;i++)
   {
     str[0] = passWord[i];
     str[1] = '\0';
     if (i==lightNum)
     {
       guiDisplay(tmpX,82,str,0);
     }
     else
     {
     	 guiDisplay(tmpX,82,str,1);
     }
     tmpX += 8;
   }
   
  #ifdef PLUG_IN_CARRIER_MODULE
   #ifdef MENU_FOR_CQ_CANON
    lcdRefresh(17,160);
   #else
    lcdRefresh(17,145);
   #endif
  #else
    lcdRefresh(17,160);
  #endif
}

/*******************************************************
��������:inputApn
��������:����APN
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void inputApn(INT8U leftRight)
{
	 char  str[20];
	 INT8U i, tmpX, tmpY;

 #ifdef PLUG_IN_CARRIER_MODULE 
  #ifdef MENU_FOR_CQ_CANON
   tmpY = 33;
   guiLine(1,tmpY,160,160,0); //����
  #else
   tmpY = 17;
   guiLine(1,tmpY,160,144,0); //����
  #endif
   menuInLayer = 4;         //�˵������4��
 #else
   tmpY = 17;
   guiLine(1,tmpY,160,160,0); //����

   menuInLayer = 5;         //�˵������5��
 #endif
   
   //����
   guiLine(1,tmpY+1,1,tmpY+41,1);
   guiLine(160,tmpY+1,160,tmpY+41,1);
   guiLine(1,tmpY+1,160,tmpY+1,1);
   guiLine(1,tmpY+41,160,tmpY+41,1);
   guiLine(1,tmpY+21,160,tmpY+21,1);
   guiDisplay(55,tmpY+3,"����APN",1);

   tmpX = 15;
   tmpY += 23;
   for(i=0;i<16;i++)
   {
     #ifdef PLUG_IN_CARRIER_MODULE
      str[0] = commParaItem[1][i];
     #else
      str[0] = commParaItem[2][i];
     #endif
     str[1] = '\0';
     if (leftRight==i)
     {
       guiDisplay(tmpX,tmpY,str,0);
     }
     else
     {
       guiDisplay(tmpX,tmpY,str,1);
     }
     tmpX += 8;
   }
   
 #ifdef PLUG_IN_CARRIER_MODULE
  #ifdef MENU_FOR_CQ_CANON 
   lcdRefresh(17,160);
  #else
   lcdRefresh(17,145);
  #endif
 #else
  lcdRefresh(17,160);
 #endif
}


/*******************************************************
��������:selectChar
��������:ѡ���ַ�
���ú���:
�����ú���:
�������:num
�������:
����ֵ:void
*******************************************************/
void selectChar(INT8U num)
{
    INT8U i,row,col;
    char  say[2];

    strcpy(character,". +-@*#0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

    inputStatus = STATUS_SELECT_CHAR;
	  
	 #ifdef MENU_FOR_CQ_CANON
	  row = 74;
	  guiLine(4,row,156,160,0);
	 #else
	  row = 57;
	  guiLine(4,row,156,144,0);
	 #endif
	  
	  guiLine(  4,row   ,  4,row+86,1);
	  guiLine(156,row   ,156,row+86,1);
	  guiLine(  4,row   ,156,row   ,1);
	  guiLine(  4,row+86,156,row+86,1);
	  guiLine( 24,row   , 24,row+86,1);
	  
	  guiDisplay(7,row+ 3,"ѡ",1);
	  guiDisplay(7,row+24,"��",1);
	  guiDisplay(7,row+45,"��",1);
	  guiDisplay(7,row+66,"��",1);
	  
	  col = 26;
	  row += 1;
	  say[1] = '\0';
	  for(i=0;i<69;i++)
	  {
	     if (i%14==0 && i!=0)
	     {
	     	  row +=17;
	     	  col = 26;
	     }
	     
	     say[0] = character[i];
	     if (i==num)
	     {
	        guiAscii(col+1,row,say,0);
	     }
	     else
	     {
	        guiAscii(col,row,say,1);
	     }
	     col+=9;
	  }
	 
	 #ifdef MENU_FOR_CQ_CANON 
	  lcdRefresh(73,160);
	 #else
	  lcdRefresh(57,145);
	 #endif
}

//Q/GDW376.1-2009�������˵�
#ifdef PLUG_IN_CARRIER_MODULE

#ifdef LIGHTING
/*******************************************************
��������:ccbStatus
��������:���ƿ��Ƶ�״̬
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void ccbStatus(void)
{
 	char tmpStr[20];
 	
 	guiLine(1,17,160,144,0); //����
 	
	menuInLayer = 2;         //�˵������2��
	
	
	
	if (queryMpLink!=NULL)
	{
	  sprintf(tmpStr,"[%02x%02x%02x%02x%02x%02x]״̬: ", queryMpLink->addr[5], queryMpLink->addr[4], queryMpLink->addr[3], queryMpLink->addr[2], queryMpLink->addr[1], queryMpLink->addr[0]);
	  
	  guiDisplay(1, 17, tmpStr, 0);

	  sprintf(tmpStr, "���Ƶ��:%03d", queryMpLink->mp);
	  guiDisplay(1,33,tmpStr,1);

	  if (queryMpLink->status>100)
	  {
	  	strcpy(tmpStr, "��ǰ����:δ֪");
	  }
	  else
	  {
	    sprintf(tmpStr, "��ǰ����:%d%%", queryMpLink->status);
	  }
	  guiDisplay(1,49,tmpStr,1);
	  
	  guiDisplay(1, 65, "��ȡ״̬ʱ��:",1);
	  sprintf(tmpStr, "%02d-%02d-%02d %02d:%02d:%02d", queryMpLink->statusTime.year, queryMpLink->statusTime.month, queryMpLink->statusTime.day, queryMpLink->statusTime.hour, queryMpLink->statusTime.minute, queryMpLink->statusTime.second);
	  guiDisplay(16, 81, tmpStr, 1);
	  
	  if (queryMpLink->msCtrlCmd<=100)
	  {
	    sprintf(tmpStr, "��վ��������:%d%%", queryMpLink->msCtrlCmd);
	  }
	  else
	  {
	    strcpy(tmpStr, "��վ��������:��");
	  }
	  guiDisplay(1, 97, tmpStr, 1);
	  guiDisplay(1,113, "��վ�����ֹʱ��:",1);
	  sprintf(tmpStr, "%02d-%02d-%02d %02d:%02d:%02d", queryMpLink->msCtrlTime.year, queryMpLink->msCtrlTime.month, queryMpLink->msCtrlTime.day, queryMpLink->msCtrlTime.hour, queryMpLink->msCtrlTime.minute, queryMpLink->msCtrlTime.second);
	  guiDisplay(16,129, tmpStr, 1);
	}
	else
	{
	  guiDisplay(1,17,"���Ƶ㵱ǰ״̬:",1);
	}
	
	lcdRefresh(17, 145);
}

/*******************************************************
��������:xlcStatus
��������:��·������״̬
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void xlcStatus(void)
{
 	char tmpStr[20];
 	
 	guiLine(1,17,160,144,0); //����
 	
	menuInLayer = 2;         //�˵������2��
	
	if (queryMpLink!=NULL)
	{
	  sprintf(tmpStr,"[%02x%02x%02x%02x%02x%02x]%03d#: ", queryMpLink->addr[5], queryMpLink->addr[4], queryMpLink->addr[3], queryMpLink->addr[2], queryMpLink->addr[1], queryMpLink->addr[0],queryMpLink->mp);
	  guiDisplay(1, 17, tmpStr, 0);

	  switch(queryMpLink->bigAndLittleType)
	  {
	  	case 1:
	  		strcpy(tmpStr, "ģʽ:ʱ�ο�");
	  		break;
	  		
	  	case 2:
	  		strcpy(tmpStr, "ģʽ:���");
	  		break;
	  		
	  	case 3:
	  		strcpy(tmpStr, "ģʽ:ʱ�οؽ�Ϲ��");
	  		break;
	  		
	  	case 4:
	  		strcpy(tmpStr, "ģʽ:��γ�ȿ�");
	  		break;

			case 5:
	  		strcpy(tmpStr, "ģʽ:��γ�Ƚ�Ϲ��");
	  		break;
	  		
	  	default:
	  		strcpy(tmpStr, "ģʽ:δ֪");
	  		break;
	  }
	  guiDisplay(1,33,tmpStr,1);

	  if (queryMpLink->collectorAddr[0]!=0xff)
	  {
	    sprintf(tmpStr, "����:%d.%04d", queryMpLink->collectorAddr[0], queryMpLink->collectorAddr[1] | queryMpLink->collectorAddr[2]<<8);
	    guiDisplay(1,49,tmpStr,1);
	  }
	  
	  if (
	  	  queryMpLink->duty[0]!=0x00 
	  	  && 
				 (
				  CTRL_MODE_LA_LO==queryMpLink->bigAndLittleType
					 || CTRL_MODE_LA_LO_LIGHT==queryMpLink->bigAndLittleType
				 )
	  	 )
	  {
	  	sprintf(tmpStr, "��%02d:%02d", queryMpLink->duty[1], queryMpLink->duty[0]);
	    guiDisplay(105, 49, tmpStr,1);
	  }

	  if (queryMpLink->collectorAddr[3]!=0xff)
	  {
	    sprintf(tmpStr, "γ��:%d.%04d", queryMpLink->collectorAddr[3], queryMpLink->collectorAddr[4] | queryMpLink->collectorAddr[5]<<8);
	    guiDisplay(1,65,tmpStr,1);
	  }
	  
	  if (
	  	  queryMpLink->duty[2]!=0x00 
	  	  && 
				 (
				  CTRL_MODE_LA_LO==queryMpLink->bigAndLittleType
					 || CTRL_MODE_LA_LO_LIGHT==queryMpLink->bigAndLittleType
				 )
	  	 )
	  {
	  	sprintf(tmpStr, "��%02d:%02d", queryMpLink->duty[3], queryMpLink->duty[2]);
	    guiDisplay(105, 65, tmpStr,1);
	  }

	  if (queryMpLink->status>1)
	  {
	  	strcpy(tmpStr, "��ǰ:δ֪");
	  }
	  else
	  {
	    if (1==queryMpLink->status)
	    {
	      sprintf(tmpStr, "��ǰ:��բ,��ȡʱ��:");
	    }
	    else
	    {
	      sprintf(tmpStr, "��ǰ:��բ,��ȡʱ��:");
	    }
	  }
	  guiDisplay(1,81,tmpStr,1);

	  sprintf(tmpStr, "%02d-%02d-%02d %02d:%02d:%02d", queryMpLink->statusTime.year, queryMpLink->statusTime.month, queryMpLink->statusTime.day, queryMpLink->statusTime.hour, queryMpLink->statusTime.minute, queryMpLink->statusTime.second);
	  guiDisplay(17, 97, tmpStr, 1);
	  
	  switch (queryMpLink->msCtrlCmd)
	  {
	    case 0:
	      strcpy(tmpStr, "��վ����:��բ");
	      break;

	    case 1:
	      strcpy(tmpStr, "��վ����:��բ");
	      break;
	      
	    default:
	      strcpy(tmpStr, "��վ����:��");
	      break;
	  }
	  strcat(tmpStr, ",��ֹ:");
	  guiDisplay(1, 113, tmpStr, 1);
	  sprintf(tmpStr, "%02d-%02d-%02d %02d:%02d:%02d", queryMpLink->msCtrlTime.year, queryMpLink->msCtrlTime.month, queryMpLink->msCtrlTime.day, queryMpLink->msCtrlTime.hour, queryMpLink->msCtrlTime.minute, queryMpLink->msCtrlTime.second);
	  guiDisplay(17,129, tmpStr, 1);
	}
	else
	{
	  guiDisplay(1,17,"��·���Ƶ㵱ǰ״̬:",1);
	}
	
	lcdRefresh(17, 145);
}

/*******************************************************
��������:xlOpenClose
��������:��·�������ֶ��ֺ�բ�˵�(����������)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void xlOpenClose(INT8U lightNum)
{
	INT8U i, row, col;
	char  str[2];
	
	menuInLayer = 3;

	guiLine( 60, 38,149,140,0);

	guiLine( 60, 38, 60,138, 1);
	guiLine(149, 38,149,138, 1);
	guiLine( 60, 38,149, 38, 1);
	guiLine( 60,138,149,138, 1);

	switch (lightNum)
	{
	  case 0:
	    guiDisplay(65, 40, "��բʮ����", 0);
	    guiDisplay(65, 60, "��բһСʱ", 1);
	    guiDisplay(65, 80, "��բʮ����", 1);
	    guiDisplay(65,100, "��բһСʱ", 1);
	    guiDisplay(65,120, " �Զ����� ", 1);
	    break;
	  
	  case 1:
	    guiDisplay(65, 40, "��բʮ����", 1);
	    guiDisplay(65, 60, "��բһСʱ", 0);
	    guiDisplay(65, 80, "��բʮ����", 1);
	    guiDisplay(65,100, "��բһСʱ", 1);
	    guiDisplay(65,120, " �Զ����� ", 1);
	    break;

		case 2:
	    guiDisplay(65, 40, "��բʮ����", 1);
	    guiDisplay(65, 60, "��բһСʱ", 1);
	    guiDisplay(65, 80, "��բʮ����", 0);
	    guiDisplay(65,100, "��բһСʱ", 1);
	    guiDisplay(65,120, " �Զ����� ", 1);
	    break;
	  
		case 3:
	    guiDisplay(65, 40, "��բʮ����", 1);
	    guiDisplay(65, 60, "��բһСʱ", 1);
	    guiDisplay(65, 80, "��բʮ����", 1);
	    guiDisplay(65,100, "��բһСʱ", 0);
	    guiDisplay(65,120, " �Զ����� ", 1);
	    break;

		case 4:
	    guiDisplay(65, 40, "��բʮ����", 1);
	    guiDisplay(65, 60, "��բһСʱ", 1);
	    guiDisplay(65, 80, "��բʮ����", 1);
	    guiDisplay(65,100, "��բһСʱ", 1);
	    guiDisplay(65,120, " �Զ����� ", 0);
	    break;
	}
 
	lcdRefresh(38, 140);
}

/*******************************************************
��������:xlOpenCloseReply
��������:��·�������ֶ��ֺ�բ�˵�(����������)�ظ�
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void xlOpenCloseReply(char *say)
{
	guiLine(12,57,148,103,0);
	guiDisplay(44,70,say,1);
	lcdRefresh(55, 105);
}

/*******************************************************
��������:irStudyReply
��������:����ѧϰ��ѧϰ(����������)�ظ�
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void irStudyReply(char *say)
{
	guiLine(12,57,148,103,0);
	guiDisplay(44,75,say,1);
	lcdRefresh(55, 105);

	guiLine(10,55,150,105,0);
}


/*******************************************************
��������:ldgmStatus
��������:�������Ƶ�״̬
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void ldgmStatus(void)
{
 	char tmpStr[20];
 	
 	guiLine(1,17,160,144,0); //����
 	
	menuInLayer = 2;         //�˵������2��
	
	if (queryMpLink!=NULL)
	{
	  sprintf(tmpStr,"[%02x%02x%02x%02x%02x%02x]״̬: ", queryMpLink->addr[5], queryMpLink->addr[4], queryMpLink->addr[3], queryMpLink->addr[2], queryMpLink->addr[1], queryMpLink->addr[0]);
	  
	  guiDisplay(1, 17, tmpStr, 0);

	  sprintf(tmpStr, "���Ƶ��:%03d", queryMpLink->mp);
	  guiDisplay(1,33,tmpStr,1);

	  if (queryMpLink->status&0x4)
	  {
	  	  strcpy(tmpStr, "��ǰ״̬:װ���쳣");
	  }
	  else
	  {
	    if (queryMpLink->status&0x2)
	    {
	  	  strcpy(tmpStr, "��ǰ״̬:��·�쳣");
	    }
	    else
	    {
	      sprintf(tmpStr, "��ǰ״̬:��·����");
	    }
	  }
	  guiDisplay(1,49,tmpStr,1);
	  
	  guiDisplay(1, 65, "��ȡ״̬ʱ��:",1);
	  sprintf(tmpStr, "%02d-%02d-%02d %02d:%02d:%02d", queryMpLink->statusTime.year, queryMpLink->statusTime.month, queryMpLink->statusTime.day, queryMpLink->statusTime.hour, queryMpLink->statusTime.minute, queryMpLink->statusTime.second);
	  guiDisplay(16, 81, tmpStr, 1);
	  
	  if (queryMpLink->status&0x1)
	  {
	  	strcpy(tmpStr, "��·����:����");
	  }
	  else
	  {
	    sprintf(tmpStr, "��·����:ֱ��");
	  }
	  guiDisplay(1,97,tmpStr,1);
	  
	  sprintf(tmpStr, "��ǰ����ֵ:%4.3fA", (float)(queryMpLink->duty[0] | queryMpLink->duty[1]<<8 | queryMpLink->duty[2]<<16)/1000);
	  guiDisplay(1,113,tmpStr,1);

    sprintf(tmpStr, "��ǰ��ѹֵ:%3.1fV", (float)(queryMpLink->duty[3] | queryMpLink->duty[4]<<8)/10);
	  guiDisplay(1,129,tmpStr,1);
	}
	else
	{
	  guiDisplay(1,17,"�������Ƶ㵱ǰ״̬:",1);
	}
	
	lcdRefresh(17, 145);
}

/*******************************************************
��������:lsStatus
��������:�նȴ�����״̬
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void lsStatus(void)
{
 	char tmpStr[20];
 	
 	guiLine(1,17,160,144,0); //����
 	
	menuInLayer = 2;         //�˵������2��
	
	if (queryMpLink!=NULL)
	{
	  sprintf(tmpStr,"[%02x%02x%02x%02x%02x%02x]�ն�: ", queryMpLink->addr[5], queryMpLink->addr[4], queryMpLink->addr[3], queryMpLink->addr[2], queryMpLink->addr[1], queryMpLink->addr[0]);
	  guiDisplay(1, 17, tmpStr, 0);

	  sprintf(tmpStr, "���Ƶ��:%03d", queryMpLink->mp);
	  guiDisplay(1,33,tmpStr,1);

	  sprintf(tmpStr, "�ն�:%ldLux", queryMpLink->duty[0] | queryMpLink->duty[1]<<8 | queryMpLink->duty[2]<<16);
	  guiDisplay(1,49,tmpStr,1);
	  
	  guiDisplay(1, 65, "��ȡ�ն�ʱ��:",1);
	  sprintf(tmpStr, "%02d-%02d-%02d %02d:%02d:%02d", queryMpLink->statusTime.year, queryMpLink->statusTime.month, queryMpLink->statusTime.day, queryMpLink->statusTime.hour, queryMpLink->statusTime.minute, queryMpLink->statusTime.second);
	  guiDisplay(16, 81, tmpStr, 1);
	}
	else
	{
	  guiDisplay(1,17,"�նȼ��㵱ǰ״̬:",1);
	}
	
	lcdRefresh(17, 145);
}

/*******************************************************
��������:queryCTimes
��������:��ѯ����ʱ��
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void queryCTimes(struct ctrlTimes *tmpNode)
{
	char  str[30], strx[20];
	INT8U i, tmpY;

	guiLine(1,17,160,144,0);  //����
	menuInLayer = 14;         //�˵������14��
	
	if(tmpNode!=NULL)
	{
    tmpY = 17;
    switch (tmpNode->deviceType)
    {
    	case 2:
        strcpy(str, "��·����");
        break;

			case 5:
        strcpy(str, "��γ�ȿ�");
        break;

    	case 7:
        strcpy(str, "�նȿ���");
        break;
      
      default:
     	  strcpy(str, "���ƿ���");
     	  break;
    }
    sprintf(strx, "%02d-%02d��%02d-%02d", tmpNode->startMonth, tmpNode->startDay, tmpNode->endMonth, tmpNode->endDay);
		strcat(str, strx);
    guiDisplay(1, tmpY, str,1);
    tmpY += 16;
    if (tmpNode->deviceType==7)
		{
		  sprintf(str, "���%d:", tmpNode->noOfTime);			
		}
		else
		{
		  sprintf(str, "ʱ��%d:", tmpNode->noOfTime);
		}
		if (tmpNode->workDay&0x01)
		{
			strcat(str, "һ");
		}
		if (tmpNode->workDay&0x02)
		{
			strcat(str, "��");
		}
		if (tmpNode->workDay&0x04)
		{
			strcat(str, "��");
		}
		if (tmpNode->workDay&0x08)
		{
			strcat(str, "��");
		}
		if (tmpNode->workDay&0x10)
		{
			strcat(str, "��");
		}
		if (tmpNode->workDay&0x20)
		{
			strcat(str, "��");
		}
		if (tmpNode->workDay&0x40)
		{
			strcat(str, "��");
		}
    guiDisplay(1, tmpY, str,1);
    tmpY += 16;
    
    for(i=0; i<6; i++)
    {
      if (tmpNode->hour[i]==0xff)
      {
      	break;
      }
      
      switch (tmpNode->deviceType)
      {
        case 2:
        case 5:
          if (1==tmpNode->bright[i])
          {
            sprintf(str, "%d)%02x:%02x-��ͨ", i+1, tmpNode->hour[i], tmpNode->min[i]);
          }
          else
          {
            sprintf(str, "%d)%02x:%02x-�ж�", i+1, tmpNode->hour[i], tmpNode->min[i]);
          }
          break;
          
        case 7:
          sprintf(str, "%d)%dLux-%d%% ", i+1, (tmpNode->hour[i] | tmpNode->min[i]<<8), tmpNode->bright[i]);
        	break;
          
        default:
          sprintf(str, "%d)%02x:%02x-%d%% ", i+1, tmpNode->hour[i], tmpNode->min[i], tmpNode->bright[i]);
          break;
      }
      guiDisplay(25, tmpY, str,1);
      tmpY += 16;
    }
	}
	 
	lcdRefresh(17, 145);
}	

/*******************************************************
��������:setLearnIr
��������:ң����ѧϰ��������(376.1���ҵ�����������Լ)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void setLearnIr(INT8U lightNum)
{
	INT8U i;
	
	menuInLayer = 3;
  
	guiLine(1,17,160,144,0);

	for(i=0; i<8; i++)
	{
		if (i==lightNum)
		{
			guiDisplay(41, 17+16*i, irMenuItem[i], 0);
		}
		else
		{
			guiDisplay(41, 17+16*i, irMenuItem[i], 1);
		}		
	}
	
	lcdRefresh(17, 144);
}

	 
#endif



/*******************************************************
��������:copyQueryMenu
��������:�����ѯ�˵�
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void copyQueryMenu(INT8U layer2Light,INT8U layer3Light)
{
   INT8U                dataBuff[LENGTH_OF_ENERGY_RECORD];     //��һ�γ������������
 	 INT32U               integer,decimal;
   struct cpAddrLink    *tmpLink;
	 char                 str[15];
	 char                 sayStr[10];
	 INT8U                i, tmpX, tmpCount;
	 DATE_TIME            tmpTime;
	 INT16U               offset;
	 BOOL                 buffHasData;
	 INT8U                meterInfo[10];
	 BOOL                 singleMeter=0;
   
   if (layer2Light>0)
   {
     guiLine(16,56,144,88,0);
     guiLine(16,56,16,88,1);
     guiLine(144,56,144,88,1);
     guiLine(16,88,144,88,1);
     guiLine(16,56,144,56,1);
     guiDisplay(30,64,"��ȡ������...",1);
     lcdRefresh(56,88);
   }
   
 	#ifdef MENU_FOR_CQ_CANON 
 	 guiLine(1,17,160,160,0); //����
 	#else
 	 guiLine(1,17,160,144,0); //����
 	#endif
 	
	 menuInLayer = 2;         //�˵������2��
	 
	 switch(layer2Light)
	 {
	 	 case 0:
	 	   guiDisplay(1, 18, "1", 1);
	 	   guiDisplay(6, 18, "-", 1);
	 	   guiDisplay(11,18, "1", 1);
	 	   
	 	   #ifdef MENU_FOR_CQ_CANON
	 	    guiDisplay(17,17,"�������й������ѯ",1);
	 	   #else
	 	    guiDisplay(17,17,"���������ݲ�ѯ",1);
	 	   #endif
	 	   
	 	   showInputTime(layer3Light);
	 	 	 break;
     
    #ifdef MENU_FOR_CQ_CANON
	 	 case 1:
	 	   guiDisplay(1,17,"1-2�������й���ʾֵ",1);
	 	   offset = POSITIVE_WORK_OFFSET;
	 	   break;

	 	 case 2:
	 	   guiDisplay(1,17,"1-3�������й���ʾֵ",1);
	 	   offset = POSITIVE_WORK_OFFSET+4;
	 	 	 break;
	 	 	 
	 	 case 3:
	 	   guiDisplay(1,17,"1-4�������й���ʾֵ",1);
	 	   offset = POSITIVE_WORK_OFFSET+8;
	 	 	 break;

	 	 case 4:
	 	   guiDisplay(1,17,"1-5�������й�ƽʾֵ",1);
	 	   offset = POSITIVE_WORK_OFFSET+12;
	 	 	 break;

	 	 case 5:
	 	   guiDisplay(1,17,"1-6�������й���ʾֵ",1);
	 	   offset = POSITIVE_WORK_OFFSET+16;
	 	 	 break;
	 	 	 
	 	#else
	 	
	 	 case 1:
	 	   guiDisplay(1,17,"1-2�����й���ʾֵ",1);
	 	   offset = POSITIVE_WORK_OFFSET;
	 	   break;

	 	 case 2:
	 	   guiDisplay(1,17,"1-3�����й���ʾֵ",1);
	 	   offset = POSITIVE_WORK_OFFSET+4;
	 	 	 break;
	 	 	 
	 	 case 3:
	 	   guiDisplay(1,17,"1-4�����й���ʾֵ",1);
	 	   offset = POSITIVE_WORK_OFFSET+8;
	 	 	 break;

	 	 case 4:
	 	   guiDisplay(1,17,"1-5�����й�ƽʾֵ",1);
	 	   offset = POSITIVE_WORK_OFFSET+12;
	 	 	 break;

	 	 case 5:
	 	   guiDisplay(1,17,"1-6�����й���ʾֵ",1);
	 	   offset = POSITIVE_WORK_OFFSET+16;
	 	 	 break;
	 	 	 
	 	 case 6:
	 	   guiDisplay(1,17,"1-7�����й���ʾֵ",1);
	 	   break;

	 	 case 7:
	 	   guiDisplay(1,17,"1-8�����й���ʾֵ",1);
	 	 	 break;
	 	 	 
	 	 case 8:
	 	   guiDisplay(1,17,"1-9�����й���ʾֵ",1);
	 	 	 break;

	 	 case 9:
	 	   guiDisplay(1,17,"1-10�����й�ƽʾֵ",1);
	 	 	 break;

	 	 case 10:
	 	   guiDisplay(1,17,"1-11�����й���ʾֵ",1);
	 	 	 break;
	 	 	 
	 	 case 11:
	 	   guiDisplay(1,17,"1-12�����޹���ʾֵ",1);
	 	   break;

	 	 case 12:
	 	   guiDisplay(1,17,"1-13�����޹���ʾֵ",1);
	 	 	 break;
	 	 	 
	 	 case 13:
	 	   guiDisplay(1,17,"1-14�����޹���ʾֵ",1);
	 	 	 break;

	 	 case 14:
	 	   guiDisplay(1,17,"1-15�����޹�ƽʾֵ",1);
	 	 	 break;

	 	 case 15:
	 	   guiDisplay(1,17,"1-16�����޹���ʾֵ",1);
	 	 	 break;
	 	 	 
	 	 case 16:
	 	   guiDisplay(1,17,"1-17�����޹���ʾֵ",1);
	 	   break;

	 	 case 17:
	 	   guiDisplay(1,17,"1-18�����޹���ʾֵ",1);
	 	 	 break;
	 	 	 
	 	 case 18:
	 	   guiDisplay(1,17,"1-19�����޹���ʾֵ",1);
	 	 	 break;

	 	 case 19:
	 	   guiDisplay(1,17,"1-20�����޹�ƽʾֵ",1);
	 	 	 break;

	 	 case 20:
	 	   guiDisplay(1,17,"1-21�����޹���ʾֵ",1);
	 	 	 break;
	 	#endif
	 }

	#ifdef MENU_FOR_CQ_CANON 
	 if (layer2Light>=1 && layer2Light<=5)
	#else
	 if (layer2Light>=1 && layer2Light<=20)
	#endif
	 {
 	   if (layer3Light>0)
 	   {
 	     guiDisplay(144,33,"��",1);
 	   }
     
     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
     {
 	     guiDisplay(144,144,"��",1);
 	   }
 	   
 	   tmpX = 33;
 	   tmpLink = queryMpLink;
 	   i = 0;
 	   while(tmpLink!=NULL && i<layer3Light)
 	   {
 	    #ifdef MENU_FOR_CQ_CANON
 	     for(tmpCount=0;tmpCount<NUM_MP_PER_PAGE;tmpCount++)
 	    #else
 	     for(tmpCount=0;tmpCount<NUM_MP_PER_PAGE-1;tmpCount++)
 	    #endif
 	     {
 	   	   tmpLink = tmpLink->next;
 	     }
 	     i++;
 	   }
 	   
 	   tmpCount = 0;
 	  #ifdef MENU_FOR_CQ_CANON
 	   while((tmpLink!=NULL) && (tmpCount<NUM_MP_PER_PAGE))
 	  #else
 	   while((tmpLink!=NULL) && (tmpCount<NUM_MP_PER_PAGE-1))
 	  #endif
 	   {
 	   	  strcpy(sayStr,intToString(tmpLink->mp,3,str));
 	   	  strcpy(str,"");
 	   	  if (strlen(sayStr)==1)
 	   	  {
 	   	  	 strcat(str,"00");
 	   	  }
 	   	  if (strlen(sayStr)==2)
 	   	  {
 	   	  	 strcat(str,"0");
 	   	  }	 	   	  
 	   	  strcat(str,sayStr);
 	   	  guiDisplay(30,tmpX,str,1);
 	   	  
 	   	  strcpy(sayStr,"");
 	   	  tmpTime = timeHexToBcd(menuQueryTime);
        
        singleMeter=0;
        queryMeterStoreInfo(tmpLink->mp,meterInfo);
        if (tmpLink->port==PORT_POWER_CARRIER)
        {
          if (layer2Light>=11)
          {
          	guiDisplay(60,tmpX,"    -----",1);

          	goto forgotThis;
          }
          
          singleMeter=1;
          
          #ifdef CQDL_CSM
           if (meterInfo[0]==8)
           {
             buffHasData = readMeterData(dataBuff, tmpLink->mp, meterInfo[2], DAY_FREEZE_COPY_DATA, &tmpTime, 0);
           }
           else
           {
             buffHasData = readMeterData(dataBuff, tmpLink->mp, meterInfo[2], ENERGY_DATA, &tmpTime, 0);
           }
          #else
           if (tmpLink->bigAndLittleType==0 && tmpLink->protocol==DLT_645_2007)
           {
             if (meterInfo[0]==8)
             {
               buffHasData = readMeterData(dataBuff, tmpLink->mp, meterInfo[2], DAY_FREEZE_COPY_DATA, &tmpTime, 0);
             }
             else
             {
               buffHasData = readMeterData(dataBuff, tmpLink->mp, meterInfo[2], ENERGY_DATA, &tmpTime, 0);
               if (buffHasData==FALSE)
               {
                 buffHasData = readMeterData(dataBuff, tmpLink->mp, meterInfo[1], ENERGY_DATA, &tmpTime, 0);
               }
             }
           }
           else
           {
             buffHasData = readMeterData(dataBuff, tmpLink->mp, meterInfo[1], ENERGY_DATA, &tmpTime, 0);
           }
          #endif
        }
        else
        {
          if (meterInfo[0]==1)
          {
            if (layer2Light>=11)
            {
          	  guiDisplay(60,tmpX,"    -----",1);

          	  goto forgotThis;
            }

            singleMeter=1;
            
            buffHasData = readMeterData(dataBuff, tmpLink->mp, meterInfo[1], ENERGY_DATA, &tmpTime, 0);
          }
          else
          {
            buffHasData = readMeterData(dataBuff, tmpLink->mp, LAST_TODAY, ENERGY_DATA, &tmpTime, 0);
          }
        }
       	  
       	
     	  switch(layer2Light)
     	  {
       	 	 case 6:
       	 	   if (singleMeter==1)
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET_S;
       	 	   }
       	 	   else
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET;
       	 	   }
       	 	   break;
       
       	 	 case 7:
       	 	   if (singleMeter==1)
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET_S+4;
       	 	   }
       	 	   else
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET+4;
       	 	   }
       	 	 	 break;
       	 	 	 
       	 	 case 8:
       	 	   if (singleMeter==1)
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET_S+8;
       	 	   }
       	 	   else
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET+8;
       	 	   }
       	 	 	 break;
       
       	 	 case 9:
       	 	   if (singleMeter==1)
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET_S+12;
       	 	   }
       	 	   else
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET+12;
       	 	   }
       	 	 	 break;
       
       	 	 case 10:
       	 	   if (singleMeter==1)
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET_S+16;
       	 	   }
       	 	   else
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET+16;
       	 	   }
       	 	 	 break;
       	 	 	 
       	 	 case 11:
       	 	   offset = POSITIVE_NO_WORK_OFFSET;
       	 	   break;
       
       	 	 case 12:
       	 	   offset = POSITIVE_NO_WORK_OFFSET+4;
       	 	 	 break;
       	 	 	 
       	 	 case 13:
       	 	   offset = POSITIVE_NO_WORK_OFFSET+8;
       	 	 	 break;
       
       	 	 case 14:
       	 	   offset = POSITIVE_NO_WORK_OFFSET+12;
       	 	 	 break;
       
       	 	 case 15:
       	 	   offset = POSITIVE_NO_WORK_OFFSET+16;
       	 	 	 break;
       	 	 	 
       	 	 case 16:
       	 	   offset = NEGTIVE_NO_WORK_OFFSET;
       	 	   break;
       
       	 	 case 17:
       	 	   offset = NEGTIVE_NO_WORK_OFFSET+4;
       	 	 	 break;
       	 	 	 
       	 	 case 18:
       	 	   offset = NEGTIVE_NO_WORK_OFFSET+8;
       	 	 	 break;
       
       	 	 case 19:
       	 	   offset = NEGTIVE_NO_WORK_OFFSET+12;
       	 	 	 break;
       
       	 	 case 20:
       	 	   offset = NEGTIVE_NO_WORK_OFFSET+16;
       	 	 	 break;
       	}      
        
        if (buffHasData==TRUE)
        {
       	   if (dataBuff[offset]!=0xee)
       	   {
              decimal = (dataBuff[offset]>>4 & 0xf) * 10 + (dataBuff[offset] & 0xf);
              integer = (dataBuff[offset+3]>>4 & 0xf)*100000
                          +(dataBuff[offset+3] & 0xf)*10000
                          +(dataBuff[offset+2]>>4 & 0xf)*1000
                          +(dataBuff[offset+2] & 0xf)*100
                          +(dataBuff[offset+1]>>4 & 0xf)*10
                          +(dataBuff[offset+1]& 0xf);
              strcpy(str, floatToString(integer,decimal,2,2,str));
              for(i=0;i<9-strlen(str);i++)
              {
              	strcat(sayStr," ");
              }
              strcat(sayStr,str);
       	   }
       	   else
       	   {
       	   	  strcpy(sayStr,"    -----");
           }
 	   	     guiDisplay(60,tmpX,sayStr,1);
        }
        else
        {
 	   	    guiDisplay(60,tmpX,"    -----",1);
        }

forgotThis:
 	   	  tmpX += 16;
 	   	  tmpLink = tmpLink->next;
 	   	  
 	   	  tmpCount++;
 	   }
	 }
	 
	#ifdef MENU_FOR_CQ_CANON 
	 lcdRefresh(17, 160);
	#else
	 lcdRefresh(17, 145);
	#endif
}

/*******************************************************
��������:commParaSetMenu
��������:ͨ�Ų������ò˵�
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void commParaSetMenu(INT8U layer2Light,INT8U layer3Light)
{
	 char  str[20];
	 INT8U i, tmpX, tmpY;
   
  #ifdef MENU_FOR_CQ_CANON 
   guiLine(1,17,160,160,0); //����
  #else
   guiLine(1,17,160,144,0); //����
	 
	 guiLine(1,17,1,140,1);
	 guiLine(160,17,160,140,1);
	 guiLine(80,119,80,140,1);	  
	 guiLine(1,17,160,17,1);
	 guiLine(1,119,160,119,1);
	 guiLine(1,140,160,140,1);
  #endif
  
   menuInLayer = 3;         //�˵������3��
   #ifdef MENU_FOR_CQ_CANON
    tmpY = 17;
   #else   
    tmpY = 19;
   #endif
   
   if (layer3Light!=0xff)
   {
    #ifdef MENU_FOR_CQ_CANON
     guiDisplay(1,tmpY,"5-1 ͨ�Ų�������",1);
     tmpY += 16;
    #endif
     
     //��������
     guiDisplay(1,tmpY,"��������",1);
     tmpX = 68;
     for(i=0;i<4;i++)
     {
       str[0] = commParaItem[0][i];
       str[1] = '\0';
       if (layer2Light==0 && i==layer3Light)
       {
         guiDisplay(tmpX,tmpY,str,0);
       }
       else
       {
         guiDisplay(tmpX,tmpY,str,1);
       }
       tmpX += 8;
     }
     tmpY += 16;
     
     //�����ŵ�
    #ifdef MENU_FOR_CQ_CANON 
     guiDisplay(1,tmpY,"�����ŵ�",1);
     switch(moduleType)
     {
     	  case GPRS_SIM300C:
     	  case GPRS_GR64:
     	  case GPRS_MC52I:
     	  case GPRS_M590E:
     	  case GPRS_M72D:
     	  	strcpy(str, "GPRS");
     	  	break;
  
     	  case CDMA_DTGS800:
     	  case CDMA_CM180:
     	  	strcpy(str, "CDMA");
     	  	break;
  
				case LTE_AIR720H:
					strcpy(str, "LTE");
					break;
				
     	  case ETHERNET:
     	  	strcpy(str, "��̫��");
     	  	break;
     	  
     	  default:
     	  	strcpy(str,"��ģ��");
     	  	break;
     }
     guiDisplay(68,tmpY,str,1);
     tmpY += 16;
    #endif
     
     //APN����
     guiDisplay(1,  tmpY, "APN",1);
     if (layer2Light==1)
     {
       if (layer3Light<5)
       {       
         strcpy(commParaItem[1],(char *)teApn[layer3Light]);
       }
       guiDisplay(32, tmpY, commParaItem[1], 0);
     }
     else
     {
       guiDisplay(32, tmpY, commParaItem[1], 1);
     }
     tmpY += 16;
     
     guiDisplay(1,tmpY,"��վIP������˿�",1);
     tmpY += 16;
     
     tmpX = 1;
     for(i=0;i<21;i++)
     {
       str[0] = commParaItem[2][i];
       str[1] = '\0';
       if (layer2Light==2 && layer3Light==i)
       {
         guiDisplay(tmpX,tmpY,str,0);
         tmpX += 8;
       }
       else
       {
         guiDisplay(tmpX,tmpY,str,1);
         tmpX += 7;
       }
     }
     tmpY += 16;
  
     guiDisplay(1,tmpY,"����IP������˿�",1);
     tmpY += 16;
     
     tmpX = 1;
     for(i=0;i<21;i++)
     {
       str[0] = commParaItem[3][i];
       str[1] = '\0';
       if (layer2Light==3 && layer3Light==i)
       {
         guiDisplay(tmpX,tmpY,str,0);
         tmpX += 8;
       }
       else
       {
         guiDisplay(tmpX,tmpY,str,1);
         tmpX += 7;
       }
     }
     tmpY += 16;
   }
   else
   {
     tmpY = 40;
     guiDisplay(12,tmpY,"�ٴ�ȷ��Ҫ������?",1);
     tmpY += 20;
   }
   
  #ifdef MENU_FOR_CQ_CANON
   guiLine(26,tmpY,26,tmpY+15,1);
   guiLine(61,tmpY,61,tmpY+15,1);
   guiLine(26,tmpY,61,tmpY,1);
   guiLine(26,tmpY+15,61,tmpY+15,1);

   guiLine(98,tmpY,98,tmpY+15,1);
   guiLine(134,tmpY,134,tmpY+15,1);
   guiLine(98,tmpY,134,tmpY,1);
   guiLine(98,tmpY+15,134,tmpY+15,1);

  #else
   tmpY = 121;
  #endif 
  
   if (layer2Light==4)
   {
     guiDisplay(28,tmpY,"ȷ��",0);     
   }
   else
   {
     guiDisplay(28,tmpY,"ȷ��",1);
   }
   
   guiDisplay(100,tmpY,"ȡ��",1);
   
  #ifdef MENU_FOR_CQ_CANON
   lcdRefresh(17,160);
  #else
   lcdRefresh(17,145);
  #endif
}

/*******************************************************
��������:adjustCommParaLight
��������:����ͨ�Ų���������
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void adjustCommParaLight(INT8U leftRight)
{
 	 INT8U tmpData;
 	 
 	 switch (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
 	 {
 	 	  case 0:
 	 	  	tmpData = 4;
 	 	  	break;
 	 	  	
 	 	  case 1:
 	 	    tmpData = 4;
 	 	    break;
 	 	    
 	 	  case 2:
 	 	  case 3:
 	 	  	tmpData = 21;
 	 	  	break;
 	 }
 	 if (leftRight==1)  //�Ҽ�
 	 {
 	   if (keyLeftRight>=tmpData-1)
 	   {
 	 	   keyLeftRight = 0;
 	   }
 	   else
 	   {
 	 	   keyLeftRight++;
 	   }
 	 }
 	 else
 	 {
 	   if (keyLeftRight==0)
 	   {
 	 	   keyLeftRight = tmpData-1;
 	   }
 	   else
 	   {
 	 	   keyLeftRight--;
 	   }
 	 }
	 
	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2 
	 	 	   || layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
	 {
	 	 if ((keyLeftRight+1)%4==0 && keyLeftRight<16)
	 	 {
	 	   if (leftRight==1)
	 	   {
	 	     keyLeftRight++;
	 	   }
	 	   else
	 	   {
	 	   	 keyLeftRight--;
	 	   }
	 	 }
	 }
}

/*******************************************************
��������:commParaSetMenu
��������:ͨ�Ų������ò˵�
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void modifyPasswordMenu(INT8U layer2Light,INT8U layer3Light)
{
	 char  str[20];
	 INT8U i, tmpX;

	 #ifdef MENU_FOR_CQ_CANON
	  guiLine(1,17,160,160,0); //����
	 #else
	  guiLine(1,17,160,144,0); //����
	  
	  guiLine(1,17,1,140,1);
	  guiLine(160,17,160,140,1);
	  guiLine(80,119,80,140,1);	  
	  guiLine(1,17,160,17,1);
	  guiLine(1,119,160,119,1);
	  guiLine(1,140,160,140,1);
	 #endif
	 
	 menuInLayer = 3;         //�˵������3��	 
	 
   if (layer3Light!=0xff)
   {
     #ifdef MENU_FOR_CQ_CANON
      guiDisplay(1,17,"5-2 �޸�����",1);

      guiDisplay(1,40,"����������",1);
      tmpX = 17;
      for(i=0;i<6;i++)
      {
        str[0] = commParaItem[0][i];
        str[1] = '\0';
        if (layer2Light==0 && i==layer3Light)
        {
          guiDisplay(tmpX,58,str,0);
        }
        else
        {
          guiDisplay(tmpX,58,str,1);
        }
        tmpX += 8;
      }
 
      guiDisplay(1,80,"ȷ��������",1);
      tmpX = 17;
      for(i=0;i<6;i++)
      {
        str[0] = commParaItem[1][i];
        str[1] = '\0';
        if (layer2Light==1 && i==layer3Light)
        {
          guiDisplay(tmpX,98,str,0);
        }
        else
        {
          guiDisplay(tmpX,98,str,1);
        }
        tmpX += 8;
      }
     #else
      guiDisplay(36,20,"����������",1);
      tmpX = 52;
      for(i=0;i<6;i++)
      {
        str[0] = commParaItem[0][i];
        str[1] = '\0';
        if (layer2Light==0 && i==layer3Light)
        {
          guiDisplay(tmpX,38,str,0);
        }
        else
        {
          guiDisplay(tmpX,38,str,1);
        }
        tmpX += 8;
      }
 
      guiDisplay(36,60,"ȷ��������",1);
      tmpX = 52;
      for(i=0;i<6;i++)
      {
        str[0] = commParaItem[1][i];
        str[1] = '\0';
        if (layer2Light==1 && i==layer3Light)
        {
          guiDisplay(tmpX,78,str,0);
        }
        else
        {
          guiDisplay(tmpX,78,str,1);
        }
        tmpX += 8;
      }
     #endif
   }
   else
   {
     #ifdef MENU_FOR_CQ_CANON
      guiDisplay(1,17,"5-3",1);
     #endif
     
     guiDisplay(12,40,"�ٴ�ȷ��Ҫ������?",1);
   }   

  #ifdef MENU_FOR_CQ_CANON 
   guiLine(26,129,26,144,1);
   guiLine(61,129,61,144,1);
   guiLine(26,129,61,129,1);
   guiLine(26,144,61,144,1);

   guiLine(98,129,98,144,1);
   guiLine(134,129,134,144,1);
   guiLine(98,129,134,129,1);
   guiLine(98,144,134,144,1);
  #endif

   if (layer2Light==2)
   {
     #ifdef MENU_FOR_CQ_CANON
      guiDisplay(28,129,"ȷ��",0);
     #else
      guiDisplay(28,121,"ȷ��",0);
     #endif
   }
   else
   {
     #ifdef MENU_FOR_CQ_CANON
      guiDisplay(28,129,"ȷ��",1);
     #else
      guiDisplay(28,121,"ȷ��",1);
     #endif
   }
   
  #ifdef MENU_FOR_CQ_CANON 
   guiDisplay(100,129,"ȡ��",1);
  #else
   guiDisplay(100,121,"ȡ��",1);
  #endif
   
   #ifdef MENU_FOR_CQ_CANON
    lcdRefresh(17,160);
   #else
    lcdRefresh(17,145);
   #endif
}

/*******************************************************
��������:debugMenu
��������:�ֳ����Բ˵�
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void realCopyMeterMenu(int lightNum)
{
	 INT8U i;

	#ifdef MENU_FOR_CQ_CANON 
	 guiLine(1,17,160,160,0); //����
	 guiDisplay(1,17,"6-1 ʵʱ����",1);
	#else
	 guiLine(1,17,160,144,0); //����
	#endif
	 menuInLayer = 3;         //�˵������3��
	 
	 for(i=0;i<2;i++)
	 {
		 if (i==lightNum)
		 {
        guiDisplay(12,50+i*18,realCopyMeterItem[i],0);
		 }
		 else
		 {
        guiDisplay(12,50+i*18,realCopyMeterItem[i],1);
		 }
	 }
	
	#ifdef MENU_FOR_CQ_CANON 
	 lcdRefresh(17, 160);
	#else
	 lcdRefresh(17, 145);
	#endif
}

/*******************************************************
��������:searchMeter
��������:�������
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void searchMeter(INT8U lightNum)
{
	 INT8U tmpX, i;
   char  str[2];
   
  #ifdef MENU_FOR_CQ_CANON 
   if (menuInLayer==3 && layer1MenuLight==5 && layer2MenuLight[layer1MenuLight]==2)
  #else
   if (menuInLayer==3 && layer1MenuLight==2 && layer2MenuLight[layer1MenuLight]==2)
  #endif
   {
   	 #ifdef MENU_FOR_CQ_CANON
   	  guiLine(1,17,160,160,0); //����
      guiDisplay(1,17,"6-5�������",1);
   	 #else
   	  guiLine(1,17,160,144,0); //����
      guiDisplay(1,17,"   �������",1);
   	 #endif
   	 
   	  menuInLayer = 3;         //�˵������3��
      
   
      if (lightNum!=0xff)
      {
        guiLine(45,60,45,78,1);
        guiLine(115,60,115,78,1);
        guiLine(45,60,115,60,1);
        guiLine(45,78,115,78,1);
   
        if (lightNum==0)
        {
          guiDisplay(48,61,"��������",0);
        }
        else
        {
          guiDisplay(48,61,"��������",1);
        }
      }
      else
      {
      	 if (carrierFlagSet.searchMeter==0)
      	 {
      	   guiDisplay(32,120,"����������",1);
      	 }
      	 else
      	 {
      	   guiDisplay(28,120,"���������...",1);   	 	 
      	 }
      }
      
     #ifdef MENU_FOR_CQ_CANON
      lcdRefresh(17,160);
     #else
      lcdRefresh(17,145);
     #endif
   }
}

/*******************************************************
��������:searchMeterReport
��������:�������������
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void searchMeterReport(void)
{
   char   str[15];
   
   if (menuInLayer==3 && layer1MenuLight==5 && layer2MenuLight[layer1MenuLight]==3)
   {
   	  newAddMeter(keyLeftRight);
   }
}

/*******************************************************
��������:newAddMeter
��������:�������ܱ��ַ
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void newAddMeter(INT8U lightNum)
{
	 INT8U  tmpY, i;
	 INT16U tmpNum;
   char   str[10],sayStr[15];
   
	#ifdef MENU_FOR_CQ_CANON 
	 guiLine(1,17,160,160,0); //����
   if (carrierModuleType==SR_WIRELESS || carrierModuleType==HWWD_WIRELESS || carrierModuleType==RL_WIRELESS)
   {
     guiDisplay(1,17,"6-6�����ڵ��ַ",1);
   }
   else
   {
     guiDisplay(1,17,"6-6�������ܱ��ַ",1);
   }
	#else
	 guiLine(1,17,160,144,0); //����
   if (carrierModuleType==SR_WIRELESS || carrierModuleType==RL_WIRELESS || carrierModuleType==HWWD_WIRELESS)
   {
     guiDisplay(1,17,"  δ�����ڵ��ַ",1);
   }
   else
   {
     guiDisplay(1,17,"   �������ܱ��ַ",1);
   }
	#endif
	
	 menuInLayer = 3;         //�˵������3��

   if (lightNum!=0xff)
   {
		 //�������ҳ��
		 multiCpMax = 0;
		 #ifdef MENU_FOR_CQ_CANON
		  tmpFound = foundMeterHead;
		 #else
		   if (carrierModuleType==RL_WIRELESS || carrierModuleType==SR_WIRELESS)
		   {
		     tmpFound = noFoundMeterHead;
		   }
		   else
		   {
		     tmpFound = foundMeterHead;		   	 
		   }
		 #endif

		 while(tmpFound!=NULL)
		 {
			 multiCpMax++;
			 tmpFound = tmpFound->next;
		 }
		 
		#ifdef MENU_FOR_CQ_CANON
		 if ((multiCpMax%(NUM_MP_PER_PAGE-1))!=0)
		 {
		    multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-1)+1;
		 }
		 else
		 {
		   multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-1);
		 }
		#else
		 if ((multiCpMax%(NUM_MP_PER_PAGE-2))!=0)
		 {
		    multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-2)+1;
		 }
		 else
		 {
		   multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-2);
		 }
		#endif

     //��������Ҫ��ʾ��ҳ
		 #ifdef MENU_FOR_CQ_CANON
		  tmpFound = foundMeterHead;
		 #else
		   if (carrierModuleType==RL_WIRELESS || carrierModuleType==SR_WIRELESS)
		   {
		     tmpFound = noFoundMeterHead;
		   }
		   else
		   {
		     tmpFound = foundMeterHead;
		   }
		 #endif

     if (tmpFound!=NULL)
     {
       #ifdef MENU_FOR_CQ_CANON
        for(i=0;i<multiCpUpDown*(NUM_MP_PER_PAGE-1);i++)
       #else
        for(i=0;i<multiCpUpDown*(NUM_MP_PER_PAGE-2);i++)
       #endif
        {
      	  tmpFound = tmpFound->next;      	  
      	  
      	  if (tmpFound==NULL)
      	  {
      	  	 break;
      	  }
        }
     }

     tmpY     = 33;
     tmpNum   = 0;
     while(tmpFound!=NULL)
     {
     	  strcpy(sayStr,"");
     	  for(i=6;i>0;i--)
     	  {
     	    strcat(sayStr,digital2ToString((tmpFound->addr[i-1]/0x10)*10 + tmpFound->addr[i-1]%0x10,str));
     	  }
     	  
     	  guiDisplay(32,tmpY, sayStr, 1);
     	  
     	  tmpY += 16;
     	  tmpNum++;
     	 #ifdef MENU_FOR_CQ_CANON
     	  if (tmpNum>=(NUM_MP_PER_PAGE-1))
     	 #else
     	  if (tmpNum>=(NUM_MP_PER_PAGE-2))
     	 #endif
     	  {
     	  	 break;
     	  }
     	  
     	  tmpFound = tmpFound->next;
     }
     
     if (carrierModuleType==SR_WIRELESS || carrierModuleType==HWWD_WIRELESS || carrierModuleType==RL_WIRELESS)
     {
     	 ;
     }
     else
     {
      #ifdef MENU_FOR_CQ_CANON 
       guiLine(45,144,45,160,1);
       guiLine(115,144,115,160,1);
       guiLine(45,144,115,144,1);
       guiLine(45,160,115,160,1);
  
       if (lightNum==0)
       {
         guiDisplay(48,144,"��������",0);
       }
       else
       {
         guiDisplay(48,144,"��������",1);
       }
      #else
       guiLine(45,129,45,144,1);
       guiLine(115,129,115,144,1);
       guiLine(45,129,115,129,1);
       guiLine(45,144,115,144,1);
  
       if (lightNum==0)
       {
         guiDisplay(48,129,"��������",0);
       }
       else
       {
         guiDisplay(48,129,"��������",1);
       }
      #endif
     }
   }
   
  #ifdef MENU_FOR_CQ_CANON 
   lcdRefresh(17,160);
  #else
   lcdRefresh(17,145);
  #endif
}

/*******************************************************
��������:singleMeterCopy
��������:ָ�������㳭��(����������)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void singleMeterCopy(char *mp,char *time,INT8U *energyData,INT8U lightNum)
{
	 INT8U tmpX, i;
   char  str[2];
   
	 #ifdef MENU_FOR_CQ_CANON
	  guiLine(1,17,160,160,0); //����
	 #else
	  guiLine(1,17,160,144,0); //����
	 #endif
	 
	 menuInLayer = 4;         //�˵������4��
   
  #ifdef MENU_FOR_CQ_CANON
   guiDisplay(1,17,"6-2ָ�������㳭��",1);
  #else
   guiDisplay(1,17,"   ָ�������㳭��",1);
  #endif
   guiDisplay(1,40,"����������",1);
   
   tmpX = 100;
   for(i=0;i<6;i++)
   {
     str[0] = mp[i];
     str[1] = '\0';
     if (i==lightNum)
     {
       guiDisplay(tmpX,40,str,0);
     }
     else
     {
     	 guiDisplay(tmpX,40,str,1);
     }
     tmpX += 8;
   }   
   guiDisplay(1,60,"���ܱ�ʱ��",1);
   guiDisplay(84,60,time,1);
   guiDisplay(1,80,"�й���ʾֵ",1);
   guiDisplay(84,80,energyData,1);
   
   if (pDotCopy!=NULL)
   {
   	 if (pDotCopy->dotRecvItem<pDotCopy->dotTotalItem)
   	 {
        guiDisplay(44,120,"������...",1);
   	 }
   }
   
   #ifdef MENU_FOR_CQ_CANON
    lcdRefresh(17,160);
   #else
    lcdRefresh(17,145);
   #endif
}

#ifdef LIGHTING

/*******************************************************
��������:singleMeterCopy
��������:ָ�����Ƶ�㳭
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void singleCcbCopy(char *mp, char *time, INT8U *energyData, INT8U lightNum)
{
	INT8U tmpX, i;
  char  str[2];
   
	guiLine(1,17,160,144,0); //����
	 
	menuInLayer = 4;         //�˵������4��
   
  guiDisplay(1,17," ��ѯָ�����Ƶ�״̬ ",0);
  guiDisplay(1,40,"������Ƶ��",1);
   
  tmpX = 100;
  for(i=0;i<6;i++)
  {
    str[0] = mp[i];
    str[1] = '\0';
    if (i==lightNum)
    {
      guiDisplay(tmpX, 40, str, 0);
    }
    else
    {
     	guiDisplay(tmpX, 40, str, 1);
    }
    tmpX += 8;
  }
  guiDisplay(1,60,"��ǰ����:",1);
  guiDisplay(84,60,time,1);
  guiDisplay(1,80,"��վ����:",1);
  guiDisplay(84,80,energyData,1);
   
  if (pDotCopy!=NULL)
  {
   	if (pDotCopy->dotRecvItem<pDotCopy->dotTotalItem)
   	{
      guiDisplay(44,120,"��ѯ��...",1);
   	}
  }
   
  lcdRefresh(17,145);
}

#endif

/*******************************************************
��������:singleMeterCopyReport
��������:ָ�������㳭��(����������)����������
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void singleMeterCopyReport(INT8U *data)
{
  #ifdef LIGHTING
   
   if (menuInLayer==4 && layer1MenuLight==2 && layer2MenuLight[layer1MenuLight]==0 && layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
   {
     if (data!=NULL)
     {
       if (data[0]<=100)
       {
       	 sprintf(singleCopyEnergy, "%d%%", data[0]);
       }
       else
       {
       	 strcpy(singleCopyEnergy, "��");
       }

       if (data[7]<100)
       {
       	 sprintf(singleCopyTime, "%d%%", data[7]);
       }
       else
       {
       	 strcpy(singleCopyTime, "δ֪");
       }
       
     }
     else
     {
     	 strcpy(singleCopyTime, "-----");
     	 strcpy(singleCopyEnergy, "-----");
     }
     
     singleCcbCopy(singleCopyMp, singleCopyTime, singleCopyEnergy, keyLeftRight);
   }
   
  #else
  
   char   str[15];
 	 INT32U integer,decimal;
   
  #ifdef MENU_FOR_CQ_CANON
   if (menuInLayer==4 && layer1MenuLight==5 && layer2MenuLight[layer1MenuLight]==0 && layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
  #else
   if (menuInLayer==4 && layer1MenuLight==2 && layer2MenuLight[layer1MenuLight]==0 && layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
  #endif
   {
     if (data!=NULL)
     {
       if (*(data+20)!=0xee)
       {
         strcpy(singleCopyTime, digital2ToString(bcdToHex(*(data+22)),str));
         strcat(singleCopyTime, ":");
         strcat(singleCopyTime,digital2ToString(bcdToHex(*(data+21)),str));
         strcat(singleCopyTime, ":");
         strcat(singleCopyTime,digital2ToString(bcdToHex(*(data+20)),str));
       }
       else
       {
     	   strcpy(singleCopyTime,"-----");
       }
       
       if (*data!=0xee)
       {
         decimal = (*data>>4 & 0xf) * 10 + (*data & 0xf);
         integer = (*(data+3)>>4 & 0xf)*100000
             +(*(data+3) & 0xf)*10000
             +(*(data+2)>>4 & 0xf)*1000
             +(*(data+2) & 0xf)*100
             +(*(data+1)>>4 & 0xf)*10
             +(*(data+1)& 0xf);
         strcpy(singleCopyEnergy, floatToString(integer,decimal,2,2,str));
       }
       else
       {
     	   strcpy(singleCopyEnergy,"-----");
       }
     }
     else
     {
     	 strcpy(singleCopyTime,"-----");
     	 strcpy(singleCopyEnergy,"-----");
     }
     
     singleMeterCopy(singleCopyMp, singleCopyTime, singleCopyEnergy, keyLeftRight);
   }
  #endif
}

/*******************************************************
��������:allMpCopy
��������:ȫ�������㳭��
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void allMpCopy(INT8U lightNum)
{
	 INT16U tmpY, i, j;
   char   str[15],sayStr[15];
 	 INT32U integer,decimal;

   #ifdef MENU_FOR_CQ_CANON
    if ((menuInLayer==2 || menuInLayer==3 || menuInLayer==4) && layer1MenuLight==5 && (layer2MenuLight[layer1MenuLight]==1 || layer2MenuLight[layer1MenuLight]==0))
   #else
    if ((menuInLayer==2 || menuInLayer==3 || menuInLayer==4) && layer1MenuLight==2 && (layer2MenuLight[layer1MenuLight]==1 || layer2MenuLight[layer1MenuLight]==0))
   #endif
    {
      ;
    }
    else
    {
    	return;
    }
   
	 #ifdef MENU_FOR_CQ_CANON
	  guiLine(1,17,160,160,0); //����
	 #else
	  guiLine(1,17,160,144,0); //����	 
	 #endif
	 
	 menuInLayer = 4;          //�˵������4��
   
   if (lightNum<2)
   {
     #ifdef MENU_FOR_CQ_CANON
      guiDisplay(1,17,"6-3ȫ�������㳭��",1);
     #else
      guiDisplay(1,17,"   ȫ�������㳭��",1);
     #endif
      guiDisplay(4,60,"ȫ��������ʱ�ܳ�!",1);
   
      guiLine(26,100,26,118,1);
      guiLine(61,100,61,118,1);
      guiLine(26,100,61,100,1);
      guiLine(26,118,61,118,1);
   
      guiLine(98,100,98,118,1);
      guiLine(134,100,134,118,1);
      guiLine(98,100,134,100,1);
      guiLine(98,118,134,118,1);
   
      if (lightNum==0)
      {
        guiDisplay(28,101,"����",0);
      }
      else
      {
        guiDisplay(28,101,"����",1);
      }
      
      if (lightNum==1)
      {
        guiDisplay(100,101,"ȡ��",0);
      }
      else
      {
        guiDisplay(100,101,"ȡ��",1);
      }
   }
   else   //ȫ�������㳭����
   {
      #ifdef MENU_FOR_CQ_CANON
       guiDisplay(1,17,"6-4ȫ�������㳭��",1);
      #else
       guiDisplay(1,17,"   ȫ�������㳭��",1);
      #endif
      
      guiDisplay(1,33,"������",1);
      guiDisplay(52,33,"ʱ��  �й���",1);
      
      tmpY = 49;
      tmpMpCopy = mpCopyHead;
      if (tmpMpCopy!=NULL)
      {
       #ifdef MENU_FOR_CQ_CANON 
        for(i=0;i<multiCpUpDown*(NUM_MP_PER_PAGE-1);i++)
       #else
        for(i=0;i<multiCpUpDown*(NUM_MP_PER_PAGE-2);i++)
       #endif
        {
      	  tmpMpCopy = tmpMpCopy->next;
      	  
      	  if (tmpMpCopy==NULL)
      	  {
      	  	 break;
      	  }
        }
      }
      
      j=0;
      while(tmpMpCopy!=NULL)
      {
 	   	   strcpy(sayStr,intToString(tmpMpCopy->mp,3,str));
 	   	   strcpy(str,"");
 	   	   if (strlen(sayStr)==1)
 	   	   {
 	   	  	 strcat(str,"00");
 	   	   }
 	   	   if (strlen(sayStr)==2)
 	   	   {
 	   	  	  strcat(str,"0");
 	   	   }	 	   	  
 	   	   strcat(str,sayStr);
 	   	   guiDisplay(12,tmpY,str,1);

         
         if (tmpMpCopy->copyTime[0]==0xee)
         {
         	 strcpy(sayStr,"-----");
         }
         else
         {
           strcpy(sayStr, digital2ToString(tmpMpCopy->copyTime[1],str));
           strcat(sayStr, ":");
           strcat(sayStr, digital2ToString(tmpMpCopy->copyTime[0],str));
         }
         guiDisplay(48, tmpY, sayStr, 1);
                  
         if (tmpMpCopy->copyEnergy[0]==0xee)
         {
         	 strcpy(sayStr,"  -----");
         }
         else
         {
           decimal = (tmpMpCopy->copyEnergy[0]>>4 & 0xf) * 10 + (tmpMpCopy->copyEnergy[0] & 0xf);
           integer = (tmpMpCopy->copyEnergy[3]>>4 & 0xf)*100000
                   +(tmpMpCopy->copyEnergy[3] & 0xf)*10000
                   +(tmpMpCopy->copyEnergy[2]>>4 & 0xf)*1000
                   +(tmpMpCopy->copyEnergy[2] & 0xf)*100
                   +(tmpMpCopy->copyEnergy[1]>>4 & 0xf)*10
                   +(tmpMpCopy->copyEnergy[1]& 0xf);
           strcpy(str, floatToString(integer,decimal,2,2,str));

           strcpy(sayStr,"");
           for(i=0;i<9-strlen(str);i++)
           {
              strcat(sayStr," ");
           }         
           strcat(sayStr,str);
         }
         guiDisplay(88,tmpY,sayStr,1);
      	 
      	 tmpMpCopy = tmpMpCopy->next;      	 
      	 tmpY += 16;
      	 
      	 j++;
      	#ifdef MENU_FOR_CQ_CANON 
      	 if (j>=(NUM_MP_PER_PAGE-1))
      	#else
      	 if (j>=(NUM_MP_PER_PAGE-2))
      	#endif
      	 {
      	 	  break;
      	 }
      }
   }
  
  #ifdef MENU_FOR_CQ_CANON 
   lcdRefresh(17,160);
  #else
   lcdRefresh(17,145);
  #endif
}

/*******************************************************
��������:newAddMeter
��������:�������ܱ��ַ
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void newMeterCpStatus(INT8U lightNum)
{
	 INT8U  tmpY, i, j;
	 INT16U tmpNum;
   char   str[10], sayStr[15];
 	 INT32U integer, decimal;
 	 
  #ifdef MENU_FOR_CQ_CANON 
   if (menuInLayer==3 && layer1MenuLight==5 && layer2MenuLight[layer1MenuLight]==4)
  #else
   if (menuInLayer==3 && layer1MenuLight==2 && layer2MenuLight[layer1MenuLight]==4)
  #endif
   {
  	 #ifdef MENU_FOR_CQ_CANON
  	  guiLine(1,17,160,160,0); //����
      guiDisplay(1,17,"6-7���������״̬",1);
  	 #else
  	  guiLine(1,17,160,144,0); //����
      guiDisplay(1,17,"   ���������״̬",1);
  	 #endif
  	 menuInLayer = 3;         //�˵������3��
  
     guiDisplay(4,33,"���ܱ��ַ ����ʾֵ",1);

		 //�������ҳ��
		 multiCpMax = 0;
		 tmpFound = foundMeterHead;
		 while(tmpFound!=NULL)
		 {
			 multiCpMax++;
			 tmpFound = tmpFound->next;
		 }
		 
		#ifdef MENU_FOR_CQ_CANON
		 if ((multiCpMax%(NUM_MP_PER_PAGE-1))!=0)
		 {
		    multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-1)+1;
		 }
		 else
		 {
		   multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-1);
		 }
	  #else
		 if ((multiCpMax%(NUM_MP_PER_PAGE-2))!=0)
		 {
		    multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-2)+1;
		 }
		 else
		 {
		   multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-2);
		 }
	  #endif
		 
     //��������Ҫ��ʾ��ҳ
     tmpFound = foundMeterHead;
     if (tmpFound!=NULL)
     {
       #ifdef MENU_FOR_CQ_CANON
        for(i=0;i<multiCpUpDown*(NUM_MP_PER_PAGE-1);i++)
       #else
        for(i=0;i<multiCpUpDown*(NUM_MP_PER_PAGE-2);i++)
       #endif
        {
      	  tmpFound = tmpFound->next;      	  
      	  
      	  if (tmpFound==NULL)
      	  {
      	  	 break;
      	  }
        }
     }
     
     j=0;
     tmpY = 49;
     while(tmpFound!=NULL)
     {
       	strcpy(sayStr,"");
       	for(i=6;i>0;i--)
       	{
       	  strcat(sayStr,digital2ToString((tmpFound->addr[i-1]/0x10)*10 + tmpFound->addr[i-1]%0x10,str));
       	}   	  
     	  guiDisplay(1,tmpY,sayStr,1);
        
        if (tmpFound->copyEnergy[0]==0xee)
        {
        	 strcpy(sayStr,"-----");
        }
        else
        {
          decimal = (tmpFound->copyEnergy[0]>>4 & 0xf) * 10 + (tmpFound->copyEnergy[0] & 0xf);
          integer = (tmpFound->copyEnergy[3]>>4 & 0xf)*100000
                  +(tmpFound->copyEnergy[3] & 0xf)*10000
                  +(tmpFound->copyEnergy[2]>>4 & 0xf)*1000
                  +(tmpFound->copyEnergy[2] & 0xf)*100
                  +(tmpFound->copyEnergy[1]>>4 & 0xf)*10
                  +(tmpFound->copyEnergy[1]& 0xf);
          strcpy(sayStr, floatToString(integer,decimal,2,2,str));
        }
        guiDisplay(105,tmpY,sayStr,1);
     	 
     	  tmpFound = tmpFound->next;      	 
     	  tmpY += 16;
     	 
     	  j++;
     	 #ifdef MENU_FOR_CQ_CANON 
     	  if (j>=(NUM_MP_PER_PAGE-1))
     	 #else
     	  if (j>=(NUM_MP_PER_PAGE-2))
     	 #endif
     	  {
     	 	  break;
     	  }
     }   
    
    #ifdef MENU_FOR_CQ_CANON 
     lcdRefresh(17,160);
    #else
     lcdRefresh(17,145);
    #endif
   }
}

#ifdef MENU_FOR_CQ_CANON  //�����Լ�������˵�
/**************************************************
��������:userInterface
��������:�˻��ӿڴ���(376.1�����Լ)
���ú���:
�����ú���:
�������:void *arg
�������:
����ֵ��״̬
***************************************************/
void userInterface(BOOL secondChanged)
{
   char   str[30],strX[30];
   INT16U tmpData;
   INT8U  i;
   struct cpAddrLink * tmpNode;
   
   if (aberrantAlarm.aberrantFlag==1)
   {
     if (secondChanged==TRUE)
     {
       guiLine(44,1,62,16,0);
   
       if (aberrantAlarm.blinkCount==1)
       {
         aberrantAlarm.blinkCount = 2;
         guiDisplay(46, 1, "��", 1);
         guiLine(46,1,62,4,0);
         guiDisplay(46, 2, "��", 1);
       }
       else
       {
   	     guiDisplay(46, 1, digital2ToString(aberrantAlarm.eventNum,str), 1);
   	     aberrantAlarm.blinkCount = 1;
   	   }
   	 }
     
     if (compareTwoTime(aberrantAlarm.timeOut, sysTime))
     {
       guiLine(30,1,62,16,0);
       aberrantAlarm.aberrantFlag = 0;
       lcdRefresh(1,16);
     }
   }
   
   //titleʱ��
   if (sysTime.second==0)
   {
      refreshTitleTime();
   }
   else
   {
   	 if (aberrantAlarm.aberrantFlag==1 && secondChanged==TRUE)
   	 {
   	 	 lcdRefresh(1,16);
   	 } 
   }
   
   if (secondChanged==TRUE)
   {
     if (lcdLightOn==LCD_LIGHT_ON)
     {
       if (compareTwoTime(lcdLightDelay,sysTime) && (!(layer1MenuLight>3 && menuInLayer>1)))
       {
     	   printf("%02d-%02d-%02d %02d:%02d:%02d����ʾ����\n",lcdLightDelay.year,lcdLightDelay.month,
     	         lcdLightDelay.day,lcdLightDelay.hour,lcdLightDelay.minute,lcdLightDelay.second
     	         );
     	   lcdLightOn = LCD_LIGHT_OFF;
     	   lcdBackLight(LCD_LIGHT_OFF);
     	   
     	   printf("����ʾ����\n");
     	  
     	   defaultMenu();
       }
     }
     if (setParaWaitTime!=0 && setParaWaitTime!=0xfe)
     {
    		setParaWaitTime--;
    		if (setParaWaitTime<1)
    		{
         	setBeeper(BEEPER_OFF);         //������
         	alarmLedCtrl(ALARM_LED_OFF);   //ָʾ����
     	    
     	    //lcdBackLight(LCD_LIGHT_OFF);
     	       
     	    //if (menuInLayer==0)
     	    //{
     	    //   defaultMenu();
     	    //}
    		}
     }
   }
   
   if ((keyValue = ioctl(fdOfIoChannel,READ_KEY_VALUE,0))!=0)
   {
      keyPressCount++;
      
      if (keyPressCount>keyCountMax)
      {
        lcdBackLight(LCD_LIGHT_ON);
        lcdLightOn = LCD_LIGHT_ON;
        
        lcdLightDelay = nextTime(sysTime, 0, 30);
        
        keyPressCount = 0;
        
        if (displayMode!=KEYPRESS_DISPLAY_MODE)
        {
           displayMode = KEYPRESS_DISPLAY_MODE;
           layer1Menu(layer1MenuLight);
           return;
        }
        
        switch(keyValue)
        {
      	   case KEY_OK:     //ȷ��
      	   	 switch(menuInLayer)
      	     {
      	    	 case 0:
      	    		 break;
      	    		 	 
      	    	 case 1:     //�˵���1��
      	    		 switch(layer1MenuLight)
      	    		 {
      	    		 	 case 0: //�����ѯ
      	    		 	 	 layer2MenuLight[0] = 0;
      	    		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 fillTimeStr();   //�õ�ǰ��������ѯ�����ַ���
      	    		 	 	 queryMpLink = initPortMeterLink(0xff);
      	    		 	 	 copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 1: //������ѯ
      	    		 	 	 layer2MenuLight[1] = 0;
      	    		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 
      	    		 	 	 queryMpLink = initPortMeterLink(0xff);
      	    		 	 	 
                     if (layer2MenuLight[layer1MenuLight]==0)
                     {
                       layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = meterDeviceNum/(NUM_MP_PER_PAGE-1);
                       if (meterDeviceNum%(NUM_MP_PER_PAGE-1)!=0)
                       {
                     	   layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
                       }
                       layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                     }
      	    		 	 	 
      	    		 	 	 paraQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 2: //�ص��û�
      	    		 	 	 layer2MenuLight[2] = 0;
      	    		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 fillTimeStr();   //�õ�ǰ��������ѯ�����ַ���
   	    		 	 	 	   if (keyHouseHold.numOfHousehold>0)
   	    		 	 	 	   {
                        //�����ص��û���Ž����ص��û�����������
                       tmpPrevMpLink = queryMpLink;                      
   	    		 	 	 	  	 for(i=0;i<keyHouseHold.numOfHousehold;i++)
   	    		 	 	 	  	 {
   	    		 	 	 	  	 	  tmpData = keyHouseHold.household[i*2] | keyHouseHold.household[i*2+1];
                           
                          if (selectF10Data(0, 0, tmpData, (INT8U *)&menuMeterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
                          {
                             tmpMpLink = (struct cpAddrLink *)malloc(sizeof(struct cpAddrLink));
                             tmpMpLink->mpNo = menuMeterConfig.number;       //���������
                             tmpMpLink->mp = menuMeterConfig.measurePoint;   //�������
                             tmpMpLink->protocol = menuMeterConfig.protocol; //Э��
                             memcpy(tmpMpLink->addr,menuMeterConfig.addr,6); //ͨ�ŵ�ַ
                             tmpMpLink->next = NULL;
                             
                       		   //printf("userInterface������%d\n",tmpMpLink->mp);
                       		        
                             if (queryMpLink==NULL)
                             {
                             	 queryMpLink = tmpMpLink;
                             }
                             else
                             {
                             	 tmpPrevMpLink->next = tmpMpLink;
                             }
                             tmpPrevMpLink = tmpMpLink;
                          }
   	    		 	 	 	  	 }
   	    		 	 	 	   }
      	    		 	 	 keyHouseholdMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 3://ͳ�Ʋ�ѯ
      	    		 	 	 layer2MenuLight[3] = 0;
      	    		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;      	    		 	 	 
      	    		 	 	 fillTimeStr();   //�õ�ǰ��������ѯ�����ַ���
      	    		 	 	 statisQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 4://��������
      	    		 	 case 5://�ֳ�����
      	    		 	 	 if (teInRunning==0x01)
      	    		 	 	 {
      	    		 	 	 	  guiLine(1,17,160,160,0); //����
      	    		 	 	 	  guiDisplay(32,50,"��������Ͷ��",1);
      	    		 	 	 	  if (layer1MenuLight==4)
      	    		 	 	 	  {
      	    		 	 	 	    guiDisplay(32,70,"��ֹ���ò���",1);
      	    		 	 	 	  }
      	    		 	 	 	  else
      	    		 	 	 	  {
      	    		 	 	 	    guiDisplay(32,70,"��ֹ�ֳ�����",1);
      	    		 	 	 	  }
      	    		 	 	 	  lcdRefresh(17,160);
      	    		 	 	 	  
      	    		 	 	 	  sleep(2);
      	    		 	 	 	  
      	    		 	 	 	  layer1Menu(layer1MenuLight);
      	    		 	 	 }
      	    		 	 	 else
      	    		 	 	 {
      	    		 	 	   pwLight = 0;
      	    		 	 	   strcpy(passWord,"000000");
      	    		 	 	   inputPassWord(pwLight);
      	    		 	 	 }
      	    		 	 	 break;
      	    		 }
      	    		 break;
      	    		 
      	    	 case 2:     //�˵���2��
      	    		 switch(layer1MenuLight)
      	    		 {
      	    		 	 case 0:  //�����ѯ
      	    		 	 	 //�����1-1(ѡ���ѯ����)��ȷ����Ҫ�ж������Ƿ���ȷ
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]==0)
      	    		 	 	 {
      	    		 	 	 	  //�ж�����������Ƿ���ȷ
      	    		 	 	 	  if (checkInputTime()==FALSE)
      	    		 	 	 	  {
      	    		 	 	 	  	 return;
      	    		 	 	 	  }
      	    		 	 	 }
      	    		 	 	 
      	    		 	 	 //��������ʾ�淶Ҫ��,��ȷ������ʵ�ֲ�ͬ��������Լ���л�
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]>=layer2MenuNum[layer1MenuLight]-1)
      	    		 	 	 {
      	    		 	 	  	layer2MenuLight[layer1MenuLight]=0;
      	    		 	 	 }
      	    		 	 	 else
      	    		 	 	 {
      	    		 	 	  	layer2MenuLight[layer1MenuLight]++;
      	    		 	 	 }
                     
                     if (layer2MenuLight[layer1MenuLight]!=0)
                     {
                       layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = meterDeviceNum/NUM_MP_PER_PAGE;
                       if (meterDeviceNum%NUM_MP_PER_PAGE!=0)
                       {
                     	   layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
                       }
                       layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                     }

      	    		 	 	 copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 1:  //������ѯ
      	    		 	 	 //��������ʾ�淶Ҫ��,��ȷ������ʵ�ֲ�ͬ��������Լ���л�
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]>=layer2MenuNum[layer1MenuLight]-1)
      	    		 	 	 {
      	    		 	 	  	layer2MenuLight[layer1MenuLight]=0;
      	    		 	 	 }
      	    		 	 	 else
      	    		 	 	 {
      	    		 	 	  	layer2MenuLight[layer1MenuLight]++;
      	    		 	 	 }

      	    		 	 	 paraQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);      	    		 	 	 
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 2:  //�ص��û������ѯ
      	    		 	 	 //�����3-1(ѡ���ѯ����)��ȷ����Ҫ�ж������Ƿ���ȷ
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]==0)
      	    		 	 	 {
      	    		 	 	 	  //�ж�����������Ƿ���ȷ
      	    		 	 	 	  if (checkInputTime()==FALSE)
      	    		 	 	 	  {
      	    		 	 	 	  	 return;
      	    		 	 	 	  }
      	    		 	 	 	  
      	    		 	 	 	  if (keyHouseHold.numOfHousehold==0)
      	    		 	 	 	  {
      	    		 	 	 	  	guiDisplay(20, 110, "δ�����ص��û�!", 1);
      	    		 	 	 	  	lcdRefresh(17, 130);
      	    		 	 	 	  	return;
      	    		 	 	 	  }
      	    		 	 	 }
      	    		 	 	 
      	    		 	 	 //��������ʾ�淶Ҫ��,��ȷ������ʵ�ֲ�ͬ��������Լ���л�
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]>=layer2MenuNum[layer1MenuLight]-1)
      	    		 	 	 {
      	    		 	 	  	layer2MenuLight[layer1MenuLight]=0;
      	    		 	 	 }
      	    		 	 	 else
      	    		 	 	 {
      	    		 	 	  	layer2MenuLight[layer1MenuLight]++;
      	    		 	 	 }
                     
                     if (layer2MenuLight[layer1MenuLight]!=0)
                     {
                       keyLeftRight = 0;
                       layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = keyHouseHold.numOfHousehold;
                       layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                     }

      	    		 	 	 keyHouseholdMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 3:  //ͳ�Ʋ�ѯ
      	    		 	 	 //�����4-1(ѡ���ѯ����)��ȷ����Ҫ�ж������Ƿ���ȷ
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]==0)
      	    		 	 	 {
      	    		 	 	 	  //�ж�����������Ƿ���ȷ
      	    		 	 	 	  if (checkInputTime()==FALSE)
      	    		 	 	 	  {
      	    		 	 	 	  	 return;
      	    		 	 	 	  }
      	    		 	 	 }
      	    		 	 	 
      	    		 	 	 //��������ʾ�淶Ҫ��,��ȷ������ʵ�ֲ�ͬ��������Լ���л�
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]>=layer2MenuNum[layer1MenuLight]-1)
      	    		 	 	 {
      	    		 	 	  	layer2MenuLight[layer1MenuLight]=0;
      	    		 	 	 }
      	    		 	 	 else
      	    		 	 	 {
      	    		 	 	  	layer2MenuLight[layer1MenuLight]++;
      	    		 	 	 }

      	    		 	 	 statisQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 4:  //��������
      	    		 	 	 switch(layer2MenuLight[layer1MenuLight])
      	    		 	 	 {
      	    		 	 	 	  case 0:  //ͨ�Ų�������
                          strcpy(commParaItem[0],digitalToChar(addrField.a1[1]>>4));
                          strcat(commParaItem[0],digitalToChar(addrField.a1[1]&0xf));
                          strcat(commParaItem[0],digitalToChar(addrField.a1[0]>>4));
                          strcat(commParaItem[0],digitalToChar(addrField.a1[0]&0xf));
                       
                          for(i=0;i<4;i++)
                          {
                          	 if (strlen((char *)teApn[i])==0)
                          	 {
                          	   //strcpy((char *)teApn[i],(char *)ipAndPort.apn);
                          	 }
                          }
                          strcpy(commParaItem[1],(char *)ipAndPort.apn);
                          
                          intToString(ipAndPort.port[1]<<8 | ipAndPort.port[0],3,strX);
                       	  strcpy(commParaItem[2],intToIpadd(ipAndPort.ipAddr[0]<<24 | ipAndPort.ipAddr[1]<<16 | ipAndPort.ipAddr[2]<<8 | ipAndPort.ipAddr[3],str));
                          strcat(commParaItem[2],":");
                          
                          for(i=0;i<5-strlen(strX);i++)
                          {
                          	  strcat(commParaItem[2],"0");
                          }
                          strcat(commParaItem[2],strX);
                          intToString(ipAndPort.portBak[1]<<8 | ipAndPort.portBak[0],3,strX);
                       	  strcpy(commParaItem[3],intToIpadd(ipAndPort.ipAddrBak[0]<<24 | ipAndPort.ipAddrBak[1]<<16 | ipAndPort.ipAddrBak[2]<<8 | ipAndPort.ipAddrBak[3],str));
                          strcat(commParaItem[3],":");
                          
                          for(i=0;i<5-strlen(strX);i++)
                          {
                          	strcat(commParaItem[3],"0");
                          }
                          strcat(commParaItem[3],strX);
                          
                          keyLeftRight = 0;
                          layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  	commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	 	  	break;
      	    		 	 	 	  	
      	    		 	 	 	  case 1:  //�޸�����
      	    		 	 	 	  	keyLeftRight = 0;
                          layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                          strcpy(commParaItem[0],"000000");
                          strcpy(commParaItem[1],"000000");
      	    		 	 	 	  	modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	 	  	break;
      	    		 	 	 	  	
             	 	 	  		case 2:  //VPN�û�������
                          strcpy(tmpVpnUserName, (char *)vpn.vpnName);                        
                          tmpVpnUserName[32] = '\0';
                          trim(tmpVpnUserName);                                                
                          
       	 	 	  		 	 	    for(i=strlen(tmpVpnUserName); i<32; i++)
       	 	 	  		 	 	    {
       	 	 	  		 	 	    	tmpVpnUserName[i] = ' ';
       	 	 	  		 	 	    }
       	 	 	  		 	 	    tmpVpnUserName[32] = '\0';
  
                          strcpy(tmpVpnPw, (char *)vpn.vpnPassword);
                          tmpVpnPw[32] = '\0';
                          trim(tmpVpnPw);
       	 	 	  		 	 	    for(i=strlen(tmpVpnPw); i<32; i++)
       	 	 	  		 	 	    {
       	 	 	  		 	 	    	tmpVpnPw[i] = ' ';
       	 	 	  		 	 	    }
       	 	 	  		 	 	    tmpVpnPw[32] = '\0';
       	 	 	  		 	 	    keyLeftRight = 0;
                          inputStatus = STATUS_NONE;
                          setVpn(keyLeftRight);
                 	 	 	  	break;
      	    		 	 	 }
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 5:   //�ֳ�����
      	    		 	 	 switch(layer2MenuLight[layer1MenuLight])   //2��˵�����
      	    		 	 	 {
      	    		 	 	 	  case 0:  //ʵʱ����
      	    		 	 	 	  	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  	realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 	  	break;
      	    		 	 	 	  	
      	    		 	 	 	  case 1: //ȫ�������㳭����
      	    		 	 	 	  	allMpCopy(0xff);
      	    		 	 	 	  	break;
      	    		 	 	 	  	
      	    		 	 	 	  case 2:  //�������
      	    		 	 	 	  	if (carrierModuleType==SR_WIRELESS || carrierModuleType==HWWD_WIRELESS || carrierModuleType==RL_WIRELESS || carrierModuleType==NO_CARRIER_MODULE)
      	    		 	 	 	  	{
                             guiLine(10,55,150,105,0);
                             guiLine(10,55,10,105,1);
                             guiLine(150,55,150,105,1);
                             guiLine(10,55,150,55,1);
                             guiLine(10,105,150,105,1);
                             if (carrierModuleType==NO_CARRIER_MODULE)
                             {
                               guiDisplay(15,60,"δʶ����ͨ��ģ��!",1);
                               guiDisplay(15,80,"     �޷��ѱ�!   ",1);
                             }
                             else
                             {
                               guiDisplay(35,60,"����������!",1);
                               guiDisplay(35,80," �����ѱ�! ",1);
                             }
                             lcdRefresh(10,120);
      	    		 	 	 	  	}
      	    		 	 	 	  	else
      	    		 	 	 	  	{
      	    		 	 	 	  	  menuInLayer = 3;
      	    		 	 	 	  	  if (carrierFlagSet.searchMeter==0)
      	    		 	 	 	  	  {
      	    		 	 	 	  	    keyLeftRight = 1;
      	    		 	 	 	  	    searchMeter(keyLeftRight);
      	    		 	 	 	  	  }
      	    		 	 	 	  	  else
      	    		 	 	 	  	  {
      	    		 	 	 	  	    keyLeftRight = 0xff;
      	    		 	 	 	  	    searchMeter(keyLeftRight);
      	    		 	 	 	  	  }
      	    		 	 	 	  	}
      	    		 	 	 	  	break;
      	    		 	 	 	  	
      	    		 	 	 	  case 3:  //�������ܱ��ַ
      	    	 	 	  		 	keyLeftRight = 1;
      	    	 	 	  		 	multiCpUpDown = 0;
      	    	 	 	  		 	newAddMeter(keyLeftRight);
      	    	 	 	  		 	break;
      	    	 	 	  		 	
      	    	 	 	  		case 4:  //�������ܱ���״̬
      	    	 	 	  			menuInLayer++;
      	    	 	 	  			      	    	 	 	  			
      	    	 	 	  			multiCpUpDown = 0;
      	    	 	 	  			newMeterCpStatus(0);
      	    	 	 	  			break;
      	    	 	 	  		 	
      	    	 	 	  		case 5:  //δ�ѵ����ܱ��ַ
      	    	 	 	  		 	keyLeftRight = 1;
      	    	 	 	  		 	multiCpUpDown = 0;
	                      
	                       #ifdef MENU_FOR_CQ_CANON 
                          noFoundMeterHead = NULL;
                          if (carrierFlagSet.searchMeter==0 && carrierFlagSet.ifSearched == 1)
                          {
                            tmpNode = initPortMeterLink(30);
                       	    
                       	    prevFound = noFoundMeterHead;
                       	   
                       	    //�����ò������ѯ
                       	    while(tmpNode!=NULL)
                       	    {
                           	  tmpNoFoundx = foundMeterHead;
                           	  tmpData = 0;
                           	  while(tmpNoFoundx!=NULL)
                           	  {     	 
                           	    if (tmpNoFoundx->addr[0]==tmpNode->addr[0] && tmpNoFoundx->addr[1]==tmpNode->addr[1]
                           	       && tmpNoFoundx->addr[2]==tmpNode->addr[2] && tmpNoFoundx->addr[3]==tmpNode->addr[3]
                           	        && tmpNoFoundx->addr[4]==tmpNode->addr[4] && tmpNoFoundx->addr[5]==tmpNode->addr[5]
                           	       )
                           	    {     	  	 
                                  tmpData = 1;
                                  break;
                                }
                               
                                tmpNoFoundx = tmpNoFoundx->next;
                              }
                    
                           	  if (tmpData==0)
                           	  {
                           	    tmpNoFoundx = existMeterHead;
                           	    while(tmpNoFoundx!=NULL)
                           	    {
                           	      if (tmpNoFoundx->addr[0]==tmpNode->addr[0] && tmpNoFoundx->addr[1]==tmpNode->addr[1]
                           	         && tmpNoFoundx->addr[2]==tmpNode->addr[2] && tmpNoFoundx->addr[3]==tmpNode->addr[3]
                           	           && tmpNoFoundx->addr[4]==tmpNode->addr[4] && tmpNoFoundx->addr[5]==tmpNode->addr[5]
                           	         )
                           	      {     	  	 
                                    tmpData = 1;
                                    break;
                                  }
                               
                                  tmpNoFoundx = tmpNoFoundx->next;
                                }
                              }
                             
                              if (tmpData==0)
                              {
                                tmpFound = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
                                memcpy(tmpFound->addr,tmpNode->addr,6);  //�ӽڵ��ַ
                                tmpFound->copyTime[0]   = 0xee;
                                tmpFound->copyTime[1]   = 0xee;
                                tmpFound->copyEnergy[0] = 0xee;
                                tmpFound->next = NULL;
                             
                                if (noFoundMeterHead==NULL)
                                {
                         	        noFoundMeterHead = tmpFound;
                                }
                                else
                                {
                         	        prevFound->next = tmpFound;
                                }
                               
                                prevFound = tmpFound;
                              }
                             
                              tmpNode = tmpNode->next;
                            }
                          }
                         #endif
                          
      	    	 	 	  			noFoundMeter(0);
      	    	 	 	  			break;
      	    	 	 	  			
      	    	 	 	  		case 6:  //�����ϱ�
      	    	 	 	  			activeReportMenu(1);
      	    	 	 	  			break;
      	    		 	 	 }
      	    		 	 	 break;
      	    		 }
      	    		 break;
      	    		 
      	    	 case 3:   //�˵���3��
      	    	 	 switch(layer1MenuLight)
      	    	 	 {
      	    	 	 	  case 4:  //��������
      	    	 	 	  	switch(layer2MenuLight[layer1MenuLight])
      	    	 	 	  	{
      	    	 	 	  		 case 0:  //ͨ�Ų�������
      	    		 	 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==4)
      	    		 	 	       {
      	    		 	 	          //�ٴ�ȷ��
      	    		 	 	          keyLeftRight = 0xff;
      	    		 	 	          commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	          layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0xff;
      	    		 	 	          
      	    		 	 	          return;
      	    		 	 	       }
      	    		 	 	       
      	    		 	 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] == 0xff)
      	    		 	 	       {
        	  	 	 	      	  	//��IP��ַ
        	  	 	 	      	  	ipAndPort.ipAddr[0] =(commParaItem[2][0]-0x30)*100+(commParaItem[2][1]-0x30)*10+(commParaItem[2][2]-0x30);
        	  	 	 	      	  	ipAndPort.ipAddr[1] =(commParaItem[2][4]-0x30)*100+(commParaItem[2][5]-0x30)*10+(commParaItem[2][6]-0x30);
        	  	 	 	      	  	ipAndPort.ipAddr[2] =(commParaItem[2][8]-0x30)*100+(commParaItem[2][9]-0x30)*10+(commParaItem[2][10]-0x30);
        	  	 	 	      	  	ipAndPort.ipAddr[3] =(commParaItem[2][12]-0x30)*100+(commParaItem[2][13]-0x30)*10+(commParaItem[2][14]-0x30);
        	  	 	 	      	  	
        	  	 	 	      	  	//���˿�
        	  	 	 	      	  	tmpData = (commParaItem[2][16]-0x30)*10000+(commParaItem[2][17]-0x30)*1000
        	  	 	 	      	  	         +(commParaItem[2][18]-0x30)*100+(commParaItem[2][19]-0x30)*10
        	  	 	 	      	  	         +(commParaItem[2][20]-0x30);
        	  	 	 	      	  	ipAndPort.port[1] = tmpData>>8;
        	  	 	 	      	  	ipAndPort.port[0] = tmpData&0xff;

        	  	 	 	      	  	//���õ�ַ
        	  	 	 	      	  	ipAndPort.ipAddrBak[0] =(commParaItem[3][0]-0x30)*100+(commParaItem[3][1]-0x30)*10+(commParaItem[3][2]-0x30);
        	  	 	 	      	  	ipAndPort.ipAddrBak[1] =(commParaItem[3][4]-0x30)*100+(commParaItem[3][5]-0x30)*10+(commParaItem[3][6]-0x30);
        	  	 	 	      	  	ipAndPort.ipAddrBak[2] =(commParaItem[3][8]-0x30)*100+(commParaItem[3][9]-0x30)*10+(commParaItem[3][10]-0x30);
        	  	 	 	      	  	ipAndPort.ipAddrBak[3] =(commParaItem[3][12]-0x30)*100+(commParaItem[3][13]-0x30)*10+(commParaItem[3][14]-0x30);
        	  	 	 	      	  	
        	  	 	 	      	  	//���ö˿�
        	  	 	 	      	  	tmpData = (commParaItem[3][16]-0x30)*10000+(commParaItem[3][17]-0x30)*1000
        	  	 	 	      	  	         +(commParaItem[3][18]-0x30)*100+(commParaItem[3][19]-0x30)*10
        	  	 	 	      	  	         +(commParaItem[3][20]-0x30);
        	  	 	 	      	  	ipAndPort.portBak[1] = tmpData>>8;
        	  	 	 	      	  	ipAndPort.portBak[0] = tmpData&0xff;
        	  	 	 	      	  	        	  	 	 	      	  	
        	  	 	 	      	  	printf("������apn����=%d\n",strlen(commParaItem[1]));
        	  	 	 	      	  	
        	  	 	 	      	  	strcpy((char *)ipAndPort.apn, commParaItem[1]);

        	  	 	 	      	  	//����IP��ַ
        	  	 	 	      	  	saveParameter(0x04, 3,(INT8U *)&ipAndPort,sizeof(IP_AND_PORT));
                        	    
                        	    saveBakKeyPara(3);    //2012-8-9,add

                 	  	 	 	    addrField.a1[1] = (commParaItem[0][0]-0x30)<<4 | (commParaItem[0][1]-0x30);
                 	  	 	 	    addrField.a1[0] = (commParaItem[0][2]-0x30)<<4 | (commParaItem[0][3]-0x30);
                              
                              //��������������
                              saveParameter(0x04, 121,(INT8U *)&addrField,4);
                        	    
                        	    saveBakKeyPara(121);    //2012-8-9,add
                              
                              guiLine(10,55,150,105,0);
                              guiLine(10,55,10,105,1);
                              guiLine(150,55,150,105,1);
                              guiLine(10,55,150,55,1);
                              guiLine(10,105,150,105,1);
                              guiDisplay(12,70,"�޸�ͨ�Ų����ɹ�!",1);
                              lcdRefresh(10,120);
                              
                              menuInLayer--;
                              layer2MenuLight[layer1MenuLight]=0xff;

                              return;
      	    		 	 	       }
      	    		 	 	       
      	    		 	 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
      	    		 	 	       {
      	    		 	 	       	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	       }
      	    		 	 	       else
      	    		 	 	       {
      	    		 	 	       	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	       }
      	    		 	 	       
      	    		 	 	       keyLeftRight = 0;
      	    		 	 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
      	    		 	 	       {
      	    		 	 	       	  keyLeftRight = 0x88;
      	    		 	 	       	  for(i=0;i<4;i++)
      	    		 	 	       	  {
      	    		 	 	       	  	 if (strcmp((char *)teApn[i],commParaItem[1])==0)
      	    		 	 	       	  	 {
      	    		 	 	       	  	 	  keyLeftRight = i;
      	    		 	 	       	  	 	  break;
      	    		 	 	       	  	 }
      	    		 	 	       	  }
      	    		 	 	       }
      	    		 	 	       commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		 	 break;
      	    	 	 	  		 	 
      	    	 	 	  		 case 1: //�޸�����
      	    		 	 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
      	    		 	 	       {
      	    		 	 	       	  for(i=0;i<6;i++)
      	    		 	 	       	  {
      	    		 	 	       	  	 if (commParaItem[0][i]!=commParaItem[1][i])
      	    		 	 	       	  	 {
      	    		 	 	       	  	 	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
      	    		 	 	       	  	 	  keyLeftRight = 0;
      	    		 	 	       	  	 	  guiDisplay(20,113,"�������벻һ��!",1);
      	    		 	 	       	  	 	  lcdRefresh(113,128);
      	    		 	 	       	  	 	  return;
      	    		 	 	       	  	 }
      	    		 	 	       	  }
      	    		 	 	           
      	    		 	 	          //ȷ��������
      	    		 	 	          keyLeftRight = 0xff;
      	    		 	 	          modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	          layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0xff;
      	    		 	 	       	  return;
      	    		 	 	       }
      	    		 	 	       
      	    		 	 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0xff)
      	    		 	 	       {
      	    		 	 	       	  for(i=0;i<6;i++)
      	    		 	 	       	  {
      	    		 	 	       	  	 originPassword[i] = commParaItem[0][i];
      	    		 	 	       	  }
      	    		 	 	       	  originPassword[6] = '\0';
                              
                              saveParameter(88, 8, originPassword, 7);     //�����������
                              
                              guiLine(10,55,150,105,0);
                              guiLine(10,55,10,105,1);
                              guiLine(150,55,150,105,1);
                              guiLine(10,55,150,55,1);
                              guiLine(10,105,150,105,1);
                              guiDisplay(44,70,"�޸�����ɹ�!",1);
                              lcdRefresh(10,120);
                              
                              menuInLayer--;
                              layer2MenuLight[layer1MenuLight]=0xff;
      	    		 	 	       	  return;
      	    		 	 	       }
      	    		 	 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
      	    		 	 	       {
      	    		 	 	       	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	       }
      	    		 	 	       else
      	    		 	 	       {
      	    		 	 	       	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	       }
      	    		 	 	       keyLeftRight = 0;
      	    		 	 	       modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		 	 break;
      	    	 	 	  		 	 
         	    	 	 	     case 2:   //VPN�û�������
                    	 	 	 if (inputStatus == STATUS_SELECT_CHAR)
                    	 	 	 {
                    	 	 	  	inputStatus = STATUS_NONE;
                    	 	 	  	if (keyLeftRight<32)
                    	 	 	  	{
                    	 	 	  	  tmpVpnUserName[keyLeftRight] = character[selectIndex];
                    	 	 	  	}
                    	 	 	  	else
                    	 	 	  	{
                    	 	 	  	  tmpVpnPw[keyLeftRight-32] = character[selectIndex];
                    	 	 	  	}
                    	 	 	  	setVpn(keyLeftRight);
                    	 	 	 }
                    	 	 	 else
                    	 	 	 {
                             tmpVpnUserName[32] = '\0';
                             trim(tmpVpnUserName);
                             tmpVpnPw[32] = '\0';
                             trim(tmpVpnPw);
                             //strcpy((char *)vpn.vpnName, tmpVpnUserName);
                             memcpy(vpn.vpnName, tmpVpnUserName, 16);
                             //strcpy((char *)vpn.vpnPassword, tmpVpnPw);
                             memcpy(vpn.vpnPassword, tmpVpnPw, 16);
   
           	  	 	 	      	 //����vpn�û�������
           	  	 	 	      	 saveParameter(0x04, 16,(INT8U *)&vpn, sizeof(VPN));
   
                             guiLine(10,55,150,105,0);
                             guiLine(10,55,10,105,1);
                             guiLine(150,55,150,105,1);
                             guiLine(10,55,150,55,1);
                             guiLine(10,105,150,105,1);
                             guiDisplay(20, 70, "ר���������޸�!",1);
                             lcdRefresh(10,120);
                                 
                             menuInLayer--;
                    	 	 	 }
                    	 	 	 break;
      	    	 	 	  	}
      	    	 	 	  	break;
      	    	 	 	  	
      	    	 	 	  case 5:  //�ֳ�����
      	    	 	 	  	switch(layer2MenuLight[layer1MenuLight])
      	    	 	 	  	{
      	    	 	 	  		 case 0:  //ʵʱ����
      	    	 	 	  		 	 if ((carrierFlagSet.searchMeter!=0 && carrierModuleType==CEPRI_CARRIER) 
      	    	 	 	  		 	 	  || (carrierFlagSet.setupNetwork!=3 && carrierModuleType==RL_WIRELESS))
      	    	 	 	  		 	 {
      	    	 	 	  		 	 	  guiLine(30, 30, 120, 120, 0);
      	    	 	 	  		 	 	  guiLine( 30, 30,  30, 120, 1);
      	    	 	 	  		 	 	  guiLine(120, 30, 120, 120, 1);
      	    	 	 	  		 	 	  guiLine( 30, 30, 120,  30, 1);
      	    	 	 	  		 	 	  guiLine( 30,120, 120, 120, 1);      	    	 	 	  		 	 	  
      	    	 	 	  		 	 	  guiDisplay(48, 55,"��������",1);
      	    	 	 	  		 	 	  guiDisplay(44, 75,"���Ժ�...",1);
      	    	 	 	  		 	 	  lcdRefresh(30, 120);
      	    	 	 	  		 	 }
      	    	 	 	  		 	 else
      	    	 	 	  		 	 {
        	    	 	 	  		 	 switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
        	    	 	 	  		 	 {
        	    	 	 	  		 	 	  case 0: //ָ�������㳭��
        	    	 	 	  		 	 	  	keyLeftRight = 0;
        	    	 	 	  		 	 	  	strcpy(singleCopyTime,"-----");
               	 	                strcpy(singleCopyEnergy,"-----");
        	    	 	 	  		 	 	  	singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
        	    	 	 	  		 	 	  	break;
        	    	 	 	  		 	 	  	
        	    	 	 	  		 	 	  case 1: //ȫ�������㳭��
        	    	 	 	  		 	 	  	keyLeftRight = 1;
        	    	 	 	  		 	 	  	allMpCopy(keyLeftRight);
        	    	 	 	  		 	 	  	break;
        	    	 	 	  		 	 }
        	    	 	 	  		 }
      	    	 	 	  		 	 break;
      	    	 	 	  		 	 
      	    	 	 	  		 case 2:  //�������
      	    	 	 	  		 	 switch(keyLeftRight)
      	    	 	 	  		 	 {
      	    	 	 	  		 	 	 case 0:  //�����ѱ�
      	    	 	 	  		 	 	   if (carrierFlagSet.searchMeter==0)
      	    	 	 	  		 	 	   {
      	    	 	 	  		 	 	     //�����ǰ�����������ò�������ͬ�ı��ַ
      	    	 	 	  		 	 	     while(existMeterHead!=NULL)
      	    	 	 	  		 	 	     {
      	    	 	 	  		 	 	   	    tmpFound = existMeterHead;
      	    	 	 	  		 	 	   	    existMeterHead = existMeterHead->next;
      	    	 	 	  		 	 	   	    free(tmpFound);
      	    	 	 	  		 	 	     }
      	    	 	 	  		 	 	     existMeterHead = NULL;
      	    	 	 	  		 	 	     prevExistFound = NULL;

      	    	 	 	  		 	 	     //�����ǰ�������ı�
      	    	 	 	  		 	 	     while(foundMeterHead!=NULL)
      	    	 	 	  		 	 	     {
      	    	 	 	  		 	 	   	    tmpFound = foundMeterHead;
      	    	 	 	  		 	 	   	    foundMeterHead = foundMeterHead->next;
      	    	 	 	  		 	 	   	    free(tmpFound);
      	    	 	 	  		 	 	     }
      	    	 	 	  		 	 	     foundMeterHead = NULL;
      	    	 	 	  		 	 	     prevFound = foundMeterHead;
      	    	 	 	  		 	 	     
      	    	 	 	  		 	 	     carrierFlagSet.foundStudyTime = nextTime(sysTime, assignCopyTime[0]|assignCopyTime[1]<<8, 0); //�ѱ�ʱ��
      	    	 	 	  		 	 	   
      	    	 	 	  		 	 	     carrierFlagSet.searchMeter = 1;         //��ʼ�ѱ��־��1
      	    	 	 	  		 	 	   }
      	    	 	 	  		 	 	 	 keyLeftRight = 0xff;
      	    	 	 	  		 	 	   searchMeter(keyLeftRight);

      	    	 	 	  		 	 	 	 break;
      	    	 	 	  		 	 	 	  
      	    	 	 	  		 	 	 case 1:
      	    	 	 	  		 	 	 	 keyLeftRight = 0;
      	    	 	 	  		 	 	 	 searchMeter(keyLeftRight);
      	    	 	 	  		 	 	 	 break;
      	    	 	 	  		 	 	 	 
      	    	 	 	  		 	 	 case 0xff:
      	    	 	 	  		 	 	 	 layer2MenuLight[layer1MenuLight]++;
      	    	 	 	  		 	 	 	 keyLeftRight = 1;
      	    	 	 	  		 	 	 	 newAddMeter(keyLeftRight);
      	    	 	 	  		 	 	 	 break;
      	    	 	 	  		 	 }
      	    	 	 	  		 	 break;
      	    	 	 	  		 	 
      	    	 	 	  		 case 3:  //�������ܱ��ַ
      	    	 	 	  		 	 switch(keyLeftRight)
      	    	 	 	  		 	 {
      	    	 	 	  		 	 	  case 0: //��������(���������ܱ��ʾֵ)
             	 	 	  			      keyLeftRight = 1;
             	 	 	  			      
             	 	 	  			      tmpFound = foundMeterHead;
             	 	 	  			      while(tmpFound!=NULL)
             	 	 	  			      {
             	 	 	  			      	 tmpFound->copyTime[0]   = 0xee;
             	 	 	  			      	 tmpFound->copyTime[1]   = 0xee;
             	 	 	  			      	 tmpFound->copyEnergy[0] = 0xee;
             	 	 	  			      	 
             	 	 	  			      	 tmpFound = tmpFound->next;
             	 	 	  			      }
             	 	 	  			      layer2MenuLight[layer1MenuLight]++;
             	 	 	  			      multiCpUpDown = 0;
             	 	 	  			      newMeterCpStatus(0);
             	 	 	  			      
             	 	 	  			      if (foundMeterHead!=NULL)
             	 	 	  			      {
                            	     pDotCopy = (DOT_COPY *)malloc(sizeof(DOT_COPY));
                            	     prevFound = foundMeterHead;
                            	     pDotCopy->dotCopyMp = prevFound->mp;                            	     
             	 	 	  			       	 memcpy(pDotCopy->addr, prevFound->addr, 6);
                            	     pDotCopy->dotCopying = FALSE;
                            	     pDotCopy->port       = PORT_POWER_CARRIER;
                            	     pDotCopy->dataFrom   = DOT_CP_NEW_METER;
                            	     pDotCopy->outTime    = nextTime(sysTime,0,22);
                            	     pDotCopy->dotResult  = RESULT_NONE;
             	 	 	  			      }
      	    	 	 	  		 	 	  	break;
      	    	 	 	  		 	 	  	
      	    	 	 	  		 	 	  case 1:
      	    	 	 	  		 	 	  	//noFoundMeter(0);
      	    	 	 	  		 	 	  	break;
      	    	 	 	  		 	 	  	
      	    	 	 	  		 	 	  case 0xff:   //���ڳ���ת���������ܱ���״̬ҳ
      	    	 	 	  		 	 	  	break;
      	    	 	 	  		 	 }
      	    	 	 	  		 	 break;
      	    	 	 	  		 	 
      	    	 	 	  		 case 6:
                           guiLine(30, 118, 160, 140, 0);
                           guiDisplay(40,120,"�ϱ���...",1);
                           lcdRefresh(17,160);
   
                           AFN02008();

                           sleep(2);
                           guiLine(30, 118, 160, 140, 0);
                           guiDisplay(40,120,"�ϱ�����",1);
                           lcdRefresh(17,160);
      	    	 	 	  		 	 break;
      	    	 	 	  	}
      	    	 	 	  	break;
      	    	 	 }
      	    	 	 break;
      	    	 	 
            	 case 4:    //�˵���4��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 4: //��������
             	 	 	  	if (inputStatus == STATUS_NONE)
             	 	 	  	{
                         keyLeftRight = 0;
                         layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	 	   commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  	}
             	 	 	  	if (inputStatus == STATUS_SELECT_CHAR)
             	 	 	  	{
             	 	 	  		 inputStatus = STATUS_NONE;
             	 	 	  		 commParaItem[1][inputIndex] = character[selectIndex];
             	 	 	  		 inputApn(inputIndex);
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 5: //�ֳ�����
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //ʵʱ����
             	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
             	 	 	  			{
             	 	 	  				 case 0:
      		                     if (pDotCopy==NULL)
      		                     {
                            	   tmpData = (singleCopyMp[0]-0x30)*1000+(singleCopyMp[1]-0x30)*100
                            	           + (singleCopyMp[2]-0x30)*10 +(singleCopyMp[3]-0x30);
                            	   if (tmpData>2040)
                            	   {
             	 	 	  			         guiDisplay(20,120,"�������������!",1);
             	 	                   lcdRefresh(120,140);
                            	   }
                            	   else
                            	   {             	 	 	  			       
             	 	                   strcpy(singleCopyTime,"-----");
             	 	                   strcpy(singleCopyEnergy,"-----");
             	 	                   singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  			         guiDisplay(44,120,"������...",1);
             	 	                   lcdRefresh(120,140);
             	 	                 
                            	     pDotCopy = (DOT_COPY *)malloc(sizeof(DOT_COPY));
                            	     pDotCopy->dotCopyMp    = tmpData;
                            	     pDotCopy->dotCopying   = FALSE;
                            	     pDotCopy->port         = PORT_POWER_CARRIER;
                            	     pDotCopy->dataFrom     = DOT_CP_SINGLE_MP;
                            	     pDotCopy->outTime      = nextTime(sysTime,0,22);
                            	     pDotCopy->dotResult    = RESULT_NONE;
                            	     pDotCopy->dotTotalItem = 2;
                            	     pDotCopy->dotRecvItem  = 0;
                            	   }
                            	 }
             	 	               break;
             	 	               
             	 	             case 1:
             	 	             	 if (keyLeftRight==0)
             	 	             	 {
                                 while(mpCopyHead!=NULL)
                                 {
                                 	  prevMpCopy = mpCopyHead;
                                 	  mpCopyHead = mpCopyHead->next;
                                 	  free(mpCopyHead);
                                 }
                                 
                                 queryMpLink = initPortMeterLink(30);
             	 	 	  			       tmpMpCopy = mpCopyHead;
             	 	 	  			       multiCpMax = 0;
             	 	 	  			       while (queryMpLink!=NULL)
             	 	 	  			       {
             	 	 	  			       	  tmpMpCopy = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
             	 	 	  			       	  tmpMpCopy->mp = queryMpLink->mp;
             	 	 	  			       	  tmpMpCopy->copyTime[0]   = 0xee;
             	 	 	  			       	  tmpMpCopy->copyTime[1]   = 0xee;
             	 	 	  			       	  tmpMpCopy->copyEnergy[0] = 0xee;
             	 	 	  			       	  tmpMpCopy->next = NULL;
             	 	 	  			       	  
             	 	 	  			       	  if (mpCopyHead==NULL)
                                    {
                                    	mpCopyHead = tmpMpCopy;
                                    }
                                    else
                                    {
                                    	prevMpCopy->next = tmpMpCopy;
                                    }
                                    prevMpCopy = tmpMpCopy;
                                    
             	 	 	  			          tmpPrevMpLink = queryMpLink;
             	 	 	  			          queryMpLink = queryMpLink->next;
                                    
             	 	 	  			          multiCpMax++;
                                    free(tmpPrevMpLink);
             	 	 	  			       }
             	 	 	  			       
             	 	 	  			       if ((multiCpMax%(NUM_MP_PER_PAGE-1))!=0)
             	 	 	  			       {
             	 	 	  			       	  multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-1)+1;
             	 	 	  			       }
             	 	 	  			       else
             	 	 	  			       {
             	 	 	  			       	  multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-1);
             	 	 	  			       }
             	 	 	  			       
             	 	 	  			       if (mpCopyHead!=NULL)
             	 	 	  			       {
                            	      multiCpUpDown = 0;
                            	      
                            	      pDotCopy = (DOT_COPY *)malloc(sizeof(DOT_COPY));
                            	      prevMpCopy = mpCopyHead;
                            	      pDotCopy->dotCopyMp  = prevMpCopy->mp;
                            	      pDotCopy->dotCopying = FALSE;
                            	      pDotCopy->port       = PORT_POWER_CARRIER;
                            	      pDotCopy->dataFrom   = DOT_CP_ALL_MP;
                            	      pDotCopy->outTime    = nextTime(sysTime,0,22);
                            	      pDotCopy->dotResult  = RESULT_NONE;
                            	      pDotCopy->dotTotalItem = 2;
                            	      pDotCopy->dotRecvItem  = 0;
             	 	 	  			       }
             	 	 	  			       
             	 	 	  			       keyLeftRight = 0xff;
             	 	 	  			       allMpCopy(keyLeftRight);
             	 	             	 }
             	 	             	 else
             	 	             	 {
             	 	             	 	  if (keyLeftRight!=0xff)
             	 	             	 	  {
             	 	             	 	    realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	             	 	  }
             	 	             	 }
             	 	             	 break;
             	 	          }
             	 	 	  		 	break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;
      	    		 
      	    	 case 20:  //�˵���20��(��������)��ȷ��
      	    		 for(i=0;i<6;i++)
      	    		 {
      	    		 	  if (originPassword[i]!=passWord[i])
      	    		 	  {
      	    		 	  	 guiDisplay(30,120,"�����������!",1);
      	    		 	  	 lcdRefresh(120,137);
      	    		 	  	 return;
      	    		 	  }
      	    		 }
      	    		 
      	    		 //������������ǰ�ĸ����˵���,ִ����Ӧ�Ķ���
      	    		 switch(layer1MenuLight)
      	    		 {
      	    		 	  case 4:   //��������
      	    		 	 	  layer2MenuLight[layer1MenuLight] = 0;
      	    		 	 	  if (moduleType==CDMA_CM180 || moduleType==CDMA_DTGS800)
      	    		 	 	  {
      	    		 	 	  	layer2MenuNum[layer1MenuLight] = 3;
      	    		 	 	  }
      	    		 	 	  paraSetMenu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	    		 	    break;
      	    		 	  
      	    		 	  case 5:  //�ֳ�����
      	    		 	 	  layer2MenuLight[layer1MenuLight] = 0;
      	    		 	 	  debugMenu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	    		 	  	break;
      	    		 }
      	    		 break;
      	     }
      	     break;

      	   case KEY_CANCEL: //ȡ��
      	     switch(menuInLayer)
      	     {
      	    	 case 1:     //�˵���1��
      	    		 defaultMenu();
      	    		 break;
      	    		 	 
      	    	 case 2:     //�˵���2��
      	    		 freeQueryMpLink();
      	    		 layer1Menu(layer1MenuLight);
      	    		 break;
      	    		 	 
      	    	 case 3:     //�˵���3��
      	    		 switch(layer1MenuLight)
      	    		 {
      	    		 	 case 4://��������
      	    		 	 	 paraSetMenu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	    		 	 	 break;
      	    		 	 	  	
      	    		 	 case 5://�ֳ�����
      	    		 	 	 debugMenu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	    		 	   break;
      	    		 }
      	    		 break;

            	 case 4:    //�˵���4��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 4: //ͨ�Ų�������
      	    		 	 	 	if (inputStatus==STATUS_SELECT_CHAR)
      	    		 	 	 	{
      	    		 	 	 		inputStatus = STATUS_NONE;
      	    		 	 	 		inputApn(inputIndex);
      	    		 	 	 	}
      	    		 	 	 	else
      	    		 	 	 	{
      	    		 	 	 	  commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	 	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 5: //�ֳ�����
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //ʵʱ����
             	 	 	  			realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	break;
             	 	 	  		 	
             	 	 	  		case 1: //ȫ������㳭����
      	    		 	 	      debugMenu(layer2MenuLight[layer1MenuLight],layer1MenuLight);             	 	 	  			
             	 	 	  			break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;      	    		 	 
      	    		 	 
      	    	 case 20:    //�˵���20��(��������)
      	    		 menuInLayer = 1;
     	    		 	 layer1Menu(layer1MenuLight);
      	    		 break;
      	     }
      	     break;
      	    	
           case KEY_DOWN:    //����
           	 switch(menuInLayer)
             {
             	 case 1:    //�˵���1��
             	 	 if (layer1MenuLight>=MAX_LAYER1_MENU-1)
             	 	 {
             	 	 	  layer1MenuLight = 0;
             	 	 }
             	 	 else
             	 	 {
             	 	 	  layer1MenuLight++;
             	 	 }
             	 	 layer1Menu(layer1MenuLight);
             	   break;

             	 case 2:    //�˵���2��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 0:    //�����ѯ
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //1-1�������й������ѯ
             	 	 	  		 	stringUpDown(queryTimeStr, layer3MenuLight[0][0], 1);  //�������ַ���Сһ������
             	 	 	  		 	copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		  break;
             	 	 	  		  
             	 	 	  		default:
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>=layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
                            layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
             	 	 	  		 	}
             	 	 	  		 	copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	break;
             	 	 	  	}
             	 	 	  	break;
             	 	 	  
             	 	 	  case 1:  //������ѯ
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //2-1�����������ѯ
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>=layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
                            layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
             	 	 	  		 	}
             	 	 	  		 	paraQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	break;             	 	 	  		 	
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2:    //�ص��û������ѯ
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //3-1�ص��û������ѯ
             	 	 	  		 	stringUpDown(queryTimeStr, layer3MenuLight[2][0],1);  //�������ַ���Сһ������
             	 	 	  		 	keyHouseholdMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		  break;

             	 	 	  		default:
             	 	 	  		 	keyLeftRight = 0;
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>=layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
                            layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
             	 	 	  		 	}
             	 	 	  		 	keyHouseholdMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  			break;

             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 3:    //ͳ����Ϣ��ѯ
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //4-1����ͳ����Ϣ��ѯ
             	 	 	  		 	stringUpDown(queryTimeStr, layer3MenuLight[3][0],1);  //�������ַ���Сһ������
             	 	 	  		 	statisQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		  break;
             	 	 	    }
             	 	 	    break;

      	    		 	  case 4://��������
      	    		 	 	  if (layer2MenuLight[layer1MenuLight]>=layer2MenuNum[layer1MenuLight]-1)
      	    		 	 	  {
      	    		 	 	 	  layer2MenuLight[layer1MenuLight] = 0;
      	    		 	 	  }
      	    		 	 	  else
      	    		 	 	  {
      	    		 	 	 	  layer2MenuLight[layer1MenuLight]++;
      	    		 	 	  }
      	    		 	 	  paraSetMenu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	    		 	 	  break;
      	    		 	 	  
      	    		 	  case 5://�ֳ�����
      	    		 	 	  if (layer2MenuLight[layer1MenuLight]>=layer2MenuNum[layer1MenuLight]-1)
      	    		 	 	  {
      	    		 	 	 	  layer2MenuLight[layer1MenuLight] = 0;
      	    		 	 	  }
      	    		 	 	  else
      	    		 	 	  {
      	    		 	 	 	  layer2MenuLight[layer1MenuLight]++;
      	    		 	 	  }
      	    		 	 	  debugMenu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	    		 	 	  break;
             	 	 }
             	 	 break;
             	 	 
             	 case 3:    //�˵���3��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 4:  //��������
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		 case 0: //ͨ�Ų�������
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
             	 	 	  		 	   if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
             	 	 	  		 	   {
             	 	 	  		 	 	    //APN�����봦��
             	 	 	  		 	 	    for(i=strlen(commParaItem[1]);i<16;i++)
             	 	 	  		 	 	    {
             	 	 	  		 	 	    	 commParaItem[1][i] = ' ';
             	 	 	  		 	 	    }
             	 	 	  		 	 	    commParaItem[1][16] = '\0';
             	 	 	  		 	 	    inputStatus=STATUS_NONE;
             	 	 	  		 	 	    inputIndex = 0;
             	 	 	  		 	 	    inputApn(inputIndex);
             	 	 	  		 	   }
             	 	 	  		 	   else
             	 	 	  		 	   {
      	    		                stringUpDown(commParaItem[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
      	    		                commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		             }
      	    		           }
             	 	 	  		 	 break;
             	 	 	  		 
             	 	 	  		 case 1: //�޸�����
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
      	    		             stringUpDown(commParaItem[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
      	    		             modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 }
             	 	 	  		 	 break;

             	 	 	  		 case 2:  //VPN�û�������
                 	 	 	  	 if (inputStatus==STATUS_SELECT_CHAR)
                 	 	 	  	 {
                 	 	 	  		 selectIndex += 14;
                 	 	 	  		 if (selectIndex>69)
                 	 	 	  		 {
                 	 	 	  			  selectIndex = 0;
                 	 	 	  		 }
                 	 	 	  		 selectChar(selectIndex);
                 	 	 	  	 }
                 	 	 	  	 if (inputStatus==STATUS_NONE)
                 	 	 	  	 {
                 	 	 	  	   selectIndex = 0;
                             selectChar(selectIndex);
                           }
                 	 	 	  	 break;

             	 	 	  	}
             	 	 	  	break;
             	 	 	  
             	 	 	  case 5:  //�ֳ�����
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		 case 0: //ʵʱ����
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>=layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
             	 	 	  		 	 {
             	 	 	  		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
             	 	 	  		 	 }
             	 	 	  		 	 else
             	 	 	  		 	 {
             	 	 	  		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
             	 	 	  		 	 }
             	 	 	  		 	 realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	 break;
             	 	 	  		 
             	 	 	  		 case 3:  //�������ܱ��ַ
           	 	 	  				 if (multiCpUpDown>=(multiCpMax-1))
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown=0;
           	 	 	  				 }
           	 	 	  				 else
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown++;
           	 	 	  				 }
           	 	 	  				 
           	 	 	  				 newAddMeter(keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 4:   //�������ܱ���״̬
           	 	 	  				 if (multiCpUpDown>=(multiCpMax-1))
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown=0;
           	 	 	  				 }
           	 	 	  				 else
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown++;
           	 	 	  				 }
           	 	 	  				 
           	 	 	  				 newMeterCpStatus(keyLeftRight);
             	 	 	  		 	 break;

             	 	 	  		 case 5:  //δ�������ܱ��ַ
           	 	 	  				 if (multiCpUpDown>=(multiCpMax-1))
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown=0;
           	 	 	  				 }
           	 	 	  				 else
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown++;
           	 	 	  				 }           	 	 	  				 
           	 	 	  				 noFoundMeter(keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;
             	 	 
            	 case 4:    //�˵���4��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 4:
             	 	 	  	if (inputStatus==STATUS_SELECT_CHAR)
             	 	 	  	{
             	 	 	  		selectIndex += 14;
             	 	 	  		if (selectIndex>69)
             	 	 	  		{
             	 	 	  			 selectIndex = 0;
             	 	 	  		}
             	 	 	  		selectChar(selectIndex);
             	 	 	  	}
             	 	 	  	if (inputStatus==STATUS_NONE)
             	 	 	  	{
             	 	 	  	  selectIndex = 0;
                        selectChar(selectIndex);
                      }
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 5: //�ֳ�����
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //ʵʱ����
             	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
             	 	 	  		 	{
             	 	 	  		 	 	case 0:  //ָ�������㳭��
             	 	 	  		 	 		if (pDotCopy==NULL)
             	 	 	  		 	 		{
             	 	 	  		 	 		  stringUpDown(singleCopyMp, keyLeftRight, 1);
             	 	 	  		 	 	    singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	  }
             	 	 	  		 	 	  break;
             	 	 	  		 	 	  
             	 	 	  		 	 	case 1:
                 	 	 	  			if (keyLeftRight==0xff)
                 	 	 	  			{
                 	 	 	  				 if (multiCpUpDown>=(multiCpMax-1))
                 	 	 	  				 {
                 	 	 	  				 	  multiCpUpDown=0;
                 	 	 	  				 }
                 	 	 	  				 else
                 	 	 	  				 {
                 	 	 	  				 	  multiCpUpDown++;
                 	 	 	  				 }
                 	 	 	  				 
                 	 	 	  				 allMpCopy(keyLeftRight);
                 	 	 	  			}
                 	 	 	  			break;
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  		 	
             	 	 	  		case 1:
            	 	 	  			if (keyLeftRight==0xff)
            	 	 	  			{
            	 	 	  				 if (multiCpUpDown>=(multiCpMax-1))
            	 	 	  				 {
            	 	 	  				 	  multiCpUpDown=0;
            	 	 	  				 }
            	 	 	  				 else
            	 	 	  				 {
            	 	 	  				 	  multiCpUpDown++;
            	 	 	  				 }
            	 	 	  				 
            	 	 	  				 allMpCopy(keyLeftRight);
            	 	 	  			}
             	 	 	  			break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;
             	 	 
             	 case 20:   //�˵���20��
             	 	 stringUpDown(passWord, pwLight,1);
             	 	 inputPassWord(pwLight);
             	 	 break;
             }
           	 break;

           case KEY_RIGHT:   //����
           	 switch(menuInLayer)
             {
             	 case 1:    //�˵���1��
             	 	 if (layer1MenuLight>=(MAX_LAYER1_MENU-1))
             	 	 {
             	 	 	  layer1MenuLight = 0;
             	 	 }
             	 	 else
             	 	 {
             	 	 	  layer1MenuLight++;
             	 	 }
             	 	 layer1Menu(layer1MenuLight);
             	   break;
             	   
             	 case 2:    //�˵���2��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 0:    //�����ѯ
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //1-1�������й������ѯ
             	 	 	  		 	if (layer3MenuLight[0][0]>=(layer3MenuNum[0][0]-1))
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[0][0] = 0;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[0][0]++;             	 	 	  		 		
             	 	 	  		 	}
             	 	 	  		 	copyQueryMenu(layer2MenuLight[0],layer3MenuLight[0][0]);
             	 	 	  		  break;
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2:     //�ص��û���ѯ
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //3-1�ص��û������ѯ
             	 	 	  		 	if (layer3MenuLight[2][0]>=(layer3MenuNum[2][0]-1))
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[2][0] = 0;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[2][0]++;             	 	 	  		 		
             	 	 	  		 	}
             	 	 	  		 	keyHouseholdMenu(layer2MenuLight[2],layer3MenuLight[2][0]);
             	 	 	  		  break;
             	 	 	  		  
             	 	 	  		default:
             	 	 	  			if (keyLeftRight>3)
             	 	 	  			{
             	 	 	  				 keyLeftRight = 0;
             	 	 	  			}
             	 	 	  			else
             	 	 	  			{
             	 	 	  				 keyLeftRight++;
             	 	 	  			}
             	 	 	  		 	keyHouseholdMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	break;             	 	 	  			
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 3:     //ͳ�Ʋ�ѯ
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //4-1����ͳ����Ϣ��ѯ
             	 	 	  		 	if (layer3MenuLight[3][0]>=(layer3MenuNum[3][0]-1))
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[3][0] = 0;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[3][0]++;             	 	 	  		 		
             	 	 	  		 	}
             	 	 	  		 	statisQueryMenu(layer2MenuLight[3],layer3MenuLight[3][0]);
             	 	 	  		  break;
             	 	 	    }
             	 	 	    break;
             	 	 }
             	 	 break;
             	 	 
             	 case 3:    //�˵���3��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 4:       //��������
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		 case 0:  //ͨ����������
      	    		         	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
      	    		         	 {                           
                             adjustCommParaLight(1);
      	    		             commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
      	    		           break;
      	    		           
      	    		         case 1:
      	    		         	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
      	    		         	 {
                             if (keyLeftRight>=5)
                       	     {
                       	 	     keyLeftRight = 0;
                       	     }
                       	     else
                       	     {
                       	 	     keyLeftRight++;
                       	     }
      	    		             modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
      	    		           break;
      	    		           
      	    		         case 2:  //����ר���û�������
                 	 	 	  	 if (inputStatus==STATUS_NONE)
                 	 	 	  	 {
                 	 	 	  	   if (keyLeftRight<63)
                 	 	 	  	   {
                 	 	 	  		   keyLeftRight++;
                 	 	 	  	   }
                 	 	 	  	   else
                 	 	 	  	   {
                 	 	 	  		   keyLeftRight = 0;
                 	 	 	  	   }
                 	 	 	  	   setVpn(keyLeftRight);
                 	 	 	  	 }
                 	 	 	  	 else
                 	 	 	  	 {
                 	 	 	  	   if (selectIndex<69)
                 	 	 	  	   {
                 	 	 	  		   selectIndex++;
                 	 	 	  	   }
                 	 	 	  	   else
                 	 	 	  	   {
                 	 	 	  		   selectIndex = 0;
                 	 	 	  	   }
                 	 	 	  	   selectChar(selectIndex);
                 	 	 	  	 }
      	    		           break;

      	    		      }
      	    		      break;
      	    		      
      	    		    case 5:       //�ֳ�����
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 2:  //�������
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  searchMeter(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	break;

             	 	 	  		case 3:  //�������ܱ��ַ
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  newAddMeter(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	break;

             	 	 	  		case 5:  //δ�ѵ����ܱ��ַ
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  noFoundMeter(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  		 	
             	 	 	  		case 6:  //�����ϱ�
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  activeReportMenu(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	break;
      	    		      }
      	    		    	break;
      	    		 }
             	 	 break;
             	 	 
             	 case 4:    //�˵���4��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 4: //��������
             	 	 	  	if (inputStatus==STATUS_NONE)
             	 	 	  	{
             	 	 	  	  if (inputIndex<15)
             	 	 	  	  {
             	 	 	  		   inputIndex++;
             	 	 	  	  }
             	 	 	  	  else
             	 	 	  	  {
             	 	 	  		   inputIndex = 0;
             	 	 	  	  }
             	 	 	  	  inputApn(inputIndex);
             	 	 	  	}
             	 	 	  	else
             	 	 	  	{
             	 	 	  	  if (selectIndex<69)
             	 	 	  	  {
             	 	 	  		   selectIndex++;
             	 	 	  	  }
             	 	 	  	  else
             	 	 	  	  {
             	 	 	  		   selectIndex = 0;
             	 	 	  	  }
             	 	 	  	  selectChar(selectIndex);
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 5: //�ֳ�����
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //ʵʱ����
             	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
             	 	 	  		 	{
             	 	 	  		 	 	case 0:  //ָ�������㳭��
             	 	 	  		 	 		if (pDotCopy==NULL)
             	 	 	  		 	 		{
             	 	 	  		 	 		  if (keyLeftRight>=3)
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight = 0;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    else
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight++;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	  }
             	 	 	  		 	 	  break;
             	 	 	  		 	 	
             	 	 	  		 	 	case 1:  //ȫ�������㳭��
             	 	 	  		 	 		if (keyLeftRight!=0xff)
             	 	 	  		 	 		{
             	 	 	  		 	 		  if (keyLeftRight>=1)
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight = 0;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    else
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight++;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    allMpCopy(keyLeftRight);
             	 	 	  		 	 	  }
             	 	 	  		 	 		break;
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;
             	 	 
             	 case 20:   //�˵���20��(��������)
      	    		 if (pwLight>=5)
      	    		 {
      	    		   pwLight = 0;
      	    		 }
      	    		 else
      	    		 {
      	    		 	 pwLight++;
      	    		 }      	    		 
      	    		 inputPassWord(pwLight);
             	 	 break;
             }
           	 break;

           case KEY_UP:    //����
           	 switch(menuInLayer)
             {
             	 case 1:
             	 	 if (layer1MenuLight==0)
             	 	 {
             	 	 	  layer1MenuLight = MAX_LAYER1_MENU-1;
             	 	 }
             	 	 else
             	 	 {
             	 	 	  layer1MenuLight--;
             	 	 }
             	 	 layer1Menu(layer1MenuLight);
             	   break;
             	   
             	 case 2:    //�˵���2��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 0:    //�����ѯ
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //1-1�������й������ѯ
             	 	 	  		 	stringUpDown(queryTimeStr, layer3MenuLight[0][0], 0);  //�������ַ�����һ���ַ�
             	 	 	  		 	copyQueryMenu(layer2MenuLight[0], layer3MenuLight[0][0]);
             	 	 	  		  break;

             	 	 	  		default:
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
                            layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]--;
             	 	 	  		 	}
             	 	 	  		 	copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	break;
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 1:    //������ѯ
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //2-1�����������ѯ
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
                            layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]--;
             	 	 	  		 	}
             	 	 	  		 	paraQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	break;
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2:    //�ص��û������ѯ
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //3-1�ص��û������ѯ
             	 	 	  		 	stringUpDown(queryTimeStr, layer3MenuLight[2][0], 0);  //�������ַ�����һ���ַ�
             	 	 	  		 	keyHouseholdMenu(layer2MenuLight[2], layer3MenuLight[2][0]);
             	 	 	  		  break;
             	 	 	  		  
             	 	 	  		default:
             	 	 	  		 	keyLeftRight = 0;
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
                            layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]--;
             	 	 	  		 	}
             	 	 	  		 	keyHouseholdMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	break;
             	 	 	    }
             	 	 	    break;
             	 	 	    
             	 	 	  case 3:    //ͳ�Ʋ�ѯ
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //4-1����ͳ����Ϣ��ѯ
             	 	 	  		 	stringUpDown(queryTimeStr, layer3MenuLight[3][0], 0);  //�������ַ�����һ���ַ�
             	 	 	  		 	statisQueryMenu(layer2MenuLight[3], layer3MenuLight[3][0]);
             	 	 	  		  break;
             	 	 	    }
             	 	 	    break;
             	 	 	    
      	    		 	 case 4:     //��������
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]==0)
      	    		 	 	 {      	    		 	 	 	  
      	    		 	 	 	  layer2MenuLight[layer1MenuLight] = layer2MenuNum[layer1MenuLight]-1;
      	    		 	 	 }
      	    		 	 	 else
      	    		 	 	 {
      	    		 	 	 	  layer2MenuLight[layer1MenuLight]--;
      	    		 	 	 }
      	    		 	 	 paraSetMenu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 5:     //�ֳ�����
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]==0)
      	    		 	 	 {      	    		 	 	 	  
      	    		 	 	 	  layer2MenuLight[layer1MenuLight] = layer2MenuNum[layer1MenuLight]-1;
      	    		 	 	 }
      	    		 	 	 else
      	    		 	 	 {
      	    		 	 	 	  layer2MenuLight[layer1MenuLight]--;
      	    		 	 	 }
      	    		 	 	 debugMenu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	    		 	 	 break;
             	 	 }
             	 	 break;
             	 	 
             	 case 3:    //�˵���3��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 4:  //��������
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		 case 0: //ͨ�Ų�������
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
             	 	 	  		 	   if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
             	 	 	  		 	   {
             	 	 	  		 	 	    for(i=strlen(commParaItem[1]);i<16;i++)
             	 	 	  		 	 	    {
             	 	 	  		 	 	    	 commParaItem[1][i] = ' ';
             	 	 	  		 	 	    }
             	 	 	  		 	 	    commParaItem[1][16] = '\0';
             	 	 	  		 	 	    inputStatus=STATUS_NONE;
             	 	 	  		 	 	    inputIndex = 0;
             	 	 	  		 	 	    inputApn(inputIndex); //APN�����봦��
             	 	 	  		 	   }
             	 	 	  		 	   else
             	 	 	  		 	   {
      	    		               stringUpDown(commParaItem[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
      	    		               commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		             }
      	    		           }
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 1: //�޸�����
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {                                 	    		           
      	    		             stringUpDown(commParaItem[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
      	    		             modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 2: //VPN�û�������
                   	 	 	   if (inputStatus==STATUS_SELECT_CHAR)
                   	 	 	   {
                   	 	 	  	 if (selectIndex>=14)
                   	 	 	  	 {
                   	 	 	  		 selectIndex -= 14;
                   	 	 	  	 }
                   	 	 	  	 else
                   	 	 	  	 {
                   	 	 	  		 selectIndex = 68;
                   	 	 	  	 }
                   	 	 	  	 selectChar(selectIndex);
                   	 	 	   }
                   	 	 	   if (inputStatus==STATUS_NONE)
                   	 	 	   {
                   	 	 	  	 selectIndex = 0;
                             selectChar(selectIndex);
                           }
                   	 	 	   break;
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 5:  //�ֳ�����
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		 case 0:  //ʵʱ����
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
             	 	 	  		 	 {
                             layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1;
                           }
             	 	 	  		 	 else
             	 	 	  		 	 {
            	 	 	  		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]--;
             	 	 	  		 	 }
             	 	 	  		 	 realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 3:  //�������ܱ��ַ
           	 	 	  				 if (multiCpUpDown==0)
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown =  multiCpMax-1;
           	 	 	  				 }
           	 	 	  				 else
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown--;
           	 	 	  				 }
           	 	 	  				 
           	 	 	  				 newAddMeter(keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 4:   //�������ܱ���״̬
           	 	 	  				 if (multiCpUpDown==0)
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown = multiCpMax-1;
           	 	 	  				 }
           	 	 	  				 else
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown--;
           	 	 	  				 }
           	 	 	  				 
           	 	 	  				 newMeterCpStatus(keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 5:  //δ�ѵ����ܱ��ַ
           	 	 	  				 if (multiCpUpDown==0)
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown =  multiCpMax-1;
           	 	 	  				 }
           	 	 	  				 else
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown--;
           	 	 	  				 }
           	 	 	  				 
           	 	 	  				 noFoundMeter(keyLeftRight);
           	 	 	  				 break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;

            	 case 4:    //�˵���4��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 4: //��������
             	 	 	  	if (inputStatus==STATUS_SELECT_CHAR)
             	 	 	  	{
             	 	 	  		if (selectIndex>=14)
             	 	 	  		{
             	 	 	  			 selectIndex -= 14;
             	 	 	  		}
             	 	 	  		else
             	 	 	  		{
             	 	 	  			 selectIndex = 68;
             	 	 	  	  }
             	 	 	  		selectChar(selectIndex);
             	 	 	  	}
             	 	 	  	if (inputStatus==STATUS_NONE)
             	 	 	  	{
             	 	 	  	  selectIndex = 0;
                        selectChar(selectIndex);
                      }
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 5: //�ֳ�����
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //ʵʱ����
             	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
             	 	 	  		 	{
             	 	 	  		 	 	case 0:  //ָ�������㳭��
             	 	 	  		 	 		if (pDotCopy==NULL)
             	 	 	  		 	 		{
             	 	 	  		 	 		  stringUpDown(singleCopyMp, keyLeftRight, 0);
             	 	 	  		 	 	    singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	  }
             	 	 	  		 	 	  break;
             	 	 	  		 	 	  
             	 	 	  		 	 	case 1:
                 	 	 	  			if (keyLeftRight==0xff)
                 	 	 	  			{
                 	 	 	  				 if (multiCpUpDown==0)
                 	 	 	  				 {
                 	 	 	  				 	  multiCpUpDown=multiCpMax-1;
                 	 	 	  				 }
                 	 	 	  				 else
                 	 	 	  				 {
                 	 	 	  				 	  multiCpUpDown--;
                 	 	 	  				 }
                 	 	 	  				 
                 	 	 	  				 allMpCopy(keyLeftRight);
                 	 	 	  			}
                 	 	 	  			break;             	 	 	  		 	 		
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  		 	
             	 	 	  		case 1:
             	 	 	  			if (keyLeftRight==0xff)
             	 	 	  			{
             	 	 	  				 if (multiCpUpDown==0)
             	 	 	  				 {
             	 	 	  				 	  multiCpUpDown=multiCpMax-1;
             	 	 	  				 }
             	 	 	  				 else
             	 	 	  				 {
             	 	 	  				 	  multiCpUpDown--;
             	 	 	  				 }
             	 	 	  				 
             	 	 	  				 allMpCopy(keyLeftRight);
             	 	 	  			}
             	 	 	  			break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;
             	 	              	 	 
             	 case 20:   //�˵���20��
             	 	 stringUpDown(passWord, pwLight,0);
             	 	 inputPassWord(pwLight);
             	 	 break;
             }
           	 break;
           	 
           case KEY_LEFT:  //����
           	 switch(menuInLayer)
             {
             	 case 1:
             	 	 if (layer1MenuLight==0)
             	 	 {
             	 	 	  layer1MenuLight = MAX_LAYER1_MENU-1;
             	 	 }
             	 	 else
             	 	 {
             	 	 	  layer1MenuLight--;
             	 	 }
             	 	 layer1Menu(layer1MenuLight);
             	   break;
             	   
             	 case 2:    //�˵���2��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 0:    //�����ѯ
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //1-1�������й������ѯ
             	 	 	  		 	if (layer3MenuLight[0][0]==0)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[0][0] = layer3MenuNum[0][0]-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[0][0]--;
             	 	 	  		 	}
             	 	 	  		 	copyQueryMenu(layer2MenuLight[0],layer3MenuLight[0][0]);
             	 	 	  		  break;
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2:    //�ص��û���ѯ
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //3-1�ص��û������ѯ
             	 	 	  		 	if (layer3MenuLight[2][0]==0)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[2][0] = layer3MenuNum[2][0]-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[2][0]--;
             	 	 	  		 	}
             	 	 	  		 	keyHouseholdMenu(layer2MenuLight[2],layer3MenuLight[2][0]);
             	 	 	  		  break;
             	 	 	  		  
             	 	 	  		default:
             	 	 	  			if (keyLeftRight==0)
             	 	 	  			{
             	 	 	  				 keyLeftRight = 3;
             	 	 	  			}
             	 	 	  			else
             	 	 	  			{
             	 	 	  				 keyLeftRight--;
             	 	 	  			}
             	 	 	  		 	keyHouseholdMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  			break;
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 3:    //ͳ�Ʋ�ѯ
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //4-1����ͳ����Ϣ��ѯ
             	 	 	  		 	if (layer3MenuLight[3][0]==0)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[3][0] = layer3MenuNum[3][0]-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[3][0]--;
             	 	 	  		 	}
             	 	 	  		 	statisQueryMenu(layer2MenuLight[3],layer3MenuLight[3][0]);
             	 	 	  		  break;
             	 	 	  	}
             	 	 	  	break;
             	   }
             	   break;
             	 
             	 case 3:    //�˵���3��
                 switch(layer1MenuLight)
                 {
                 	  case 4:  //��������
                 	  	switch(layer2MenuLight[layer1MenuLight])
                 	  	{
                 	  		 case 0: //ͨ�Ų�������
                           if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
                             adjustCommParaLight(0);
      	    		             commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
      	    		           break;
      	    		        
      	    		         case 1: //�޸�����
                           if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
                             if (keyLeftRight==0)
                       	     {
                       	 	     keyLeftRight = 5;
                       	     }
                       	     else
                       	     {
                       	 	     keyLeftRight--;
                       	     }
      	    		             modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
      	    		           break;
      	    		           
      	    		         case 2:  //����ר���û�������
                	 	 	  	 if (inputStatus==STATUS_NONE)
                	 	 	  	 {
                             if (keyLeftRight==0)
                         	   {
                         	 	   keyLeftRight = 63;
                         	   }
                         	   else
                         	   {
                         	 	   keyLeftRight--;
                         	   }
        	    		           setVpn(keyLeftRight);
                	 	 	  	 }
                	 	 	  	 else
                	 	 	  	 {
                	 	 	  	   if (selectIndex==0)
                	 	 	  	   {
                	 	 	  		    selectIndex=68;
                	 	 	  	   }
                	 	 	  	   else
                	 	 	  	   {
                	 	 	  		    selectIndex--;
                	 	 	  	   }
                	 	 	  	   selectChar(selectIndex);
                	 	 	  	 }
      	    		           break;
      	    		      }
      	    		      break;
      	    		      
      	    		    case 5:       //�ֳ�����
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 2:  //�������
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	   	keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  searchMeter(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  		 	
             	 	 	  		case 3:  //�������ܱ��ַ
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	   	keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  newAddMeter(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  		 	
             	 	 	  		case 6:  //�����ϱ�
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	   	keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  activeReportMenu(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	break;
      	    		      }
      	    		    	break;
      	    		 }
             	 	 break;
             	 	 
             	 case 4:    //�˵���4��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 4: //��������             	 	 	  	
             	 	 	  	if (inputStatus==STATUS_NONE)
             	 	 	  	{
             	 	 	  	  if (inputIndex==0)
             	 	 	  	  {
             	 	 	  		  inputIndex = 15;
             	 	 	  	  }
             	 	 	  	  else
             	 	 	  	  {
             	 	 	  		  inputIndex--;
             	 	 	  	  }
             	 	 	  	  inputApn(inputIndex);
             	 	 	  	}
             	 	 	  	else
             	 	 	  	{
             	 	 	  	  if (selectIndex==0)
             	 	 	  	  {
             	 	 	  		   selectIndex=68;
             	 	 	  	  }
             	 	 	  	  else
             	 	 	  	  {
             	 	 	  		   selectIndex--;
             	 	 	  	  }
             	 	 	  	  selectChar(selectIndex);
             	 	 	  	}
             	 	 	  	break;
             	 	 	  
             	 	 	  case 5: //�ֳ�����
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //ʵʱ����
             	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
             	 	 	  		 	{
             	 	 	  		 	 	case 0:  //ָ�������㳭��
             	 	 	  		 	 		if (pDotCopy==NULL)
             	 	 	  		 	 		{
             	 	 	  		 	 		  if (keyLeftRight==0)
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight = 3;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    else
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight--;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	  }
             	 	 	  		 	 	  break;
             	 	 	  		 	 	  
             	 	 	  		 	 	case 1:  //ȫ�������㳭��
             	 	 	  		 	 		if (keyLeftRight!=0xff)
             	 	 	  		 	 		{
             	 	 	  		 	 		  if (keyLeftRight==0)
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight = 1;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    else
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight--;
             	 	 	  		 	 	    }
             	 	 	  		 	 		  allMpCopy(keyLeftRight);
             	 	 	  		 	 		}
             	 	 	  		 	 		break;
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;
             	 
             	 case 20:   //�˵���20��(��������)
      	    		 if (pwLight==0)
      	    		 {
      	    		   pwLight = 5;
      	    		 }
      	    		 else
      	    		 {
      	    		 	 pwLight--;
      	    		 }
      	    		 inputPassWord(pwLight);
             	 	 break;
             }
           	 break;
        }
      }      
   }
   else
   {
      keyPressCount = 0;
   }
}

/*******************************************************
��������:defaultMenu
��������:Ĭ�ϲ˵�(������,�����Լ376.1�������˵�)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void defaultMenu(void)
{
   char     str[25];
   
   menuInLayer = 0;
   displayMode = DEFAULT_DISPLAY_MODE;

	 guiLine(1,17,160,160,0);
	 
	 if (strlen((char *)teName)>10)
	 {
	   guiDisplay((160-strlen((char *)teName)*8)/2,30,(char *)teName,1);
	   guiDisplay(40,48,"̨��������",1);
	 }
	 else
	 {
	 	 strcpy(str,(char *)teName);
	 	 strcat(str,"̨��������");
	 	 
	   guiDisplay((160-strlen(str)*8)/2,40,str,1);
	 }
   
   guiLine(40,80,120,122,0);
   guiAscii(40, 80, "20",1);
   guiAscii(56, 80, digital2ToString(sysTime.year,str),1);
   guiAscii(72, 80, "-", 1);
   guiAscii(80, 80, digital2ToString(sysTime.month,str),1);
   guiAscii(96, 80, "-", 1);
   guiAscii(104, 80, digital2ToString(sysTime.day,str),1);
   
   guiDisplay(56,98, weekName[dayWeek(2000+sysTime.year, sysTime.month, sysTime.day)],1);
   
   guiLine(4,140,4,160,1);
   guiLine(42,140,42,160,1);
   guiLine(80,140,80,160,1);
   guiLine(118,140,118,160,1);
   guiLine(157,140,157,160,1);   

   guiLine(4,140,157,140,1);
   guiLine(4,160,157,160,1);

   guiDisplay(8,143,"����",1);
   
   if (teInRunning==1)
   {
     guiDisplay(46,143,"Ͷ��",1);
   }
   else
   {
     guiDisplay(46,143,"δͶ",1);
   }
   
   //����ִ��״̬
   if (copyCtrl[4].meterCopying==TRUE)
   {
   	 guiDisplay(84,143,"����",1);
   }
   else
   {
   	 if (carrierFlagSet.searchMeter!=0)
   	 {
   	   guiDisplay(84,143,"�ѱ�",1);
   	 }
   	 else
   	 {
       guiDisplay(84,143,"���",1);
     }
   }
   
   //��վͨ��״̬
   if (wlModemFlag.permitSendData==0)
   {
     guiDisplay(123,143,"����",1);
   }
   else
   {
     guiDisplay(123,143,"����",1);   	 
   }
   
   lcdRefresh(17,160);
}

/*******************************************************
��������:paraQueryMenu
��������:������ѯ�˵�
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void paraQueryMenu(INT8U layer2Light,INT8U layer3Light)
{
   struct cpAddrLink *tmpLink;
	 char              str[30];
	 char              sayStr[30];
	 INT8U             i, tmpX, tmpY, tmpCount;
	 //DATE_TIME         tmpTime;

	 guiLine(1,17,160,160,0); //����
	 menuInLayer = 2;         //�˵������2��
	 
	 switch(layer2Light)
	 {
	 	 case 0:
	 	   guiDisplay(1,17,"2-1�����������ѯ",1);
	 	   guiDisplay(1,33,"�������  ����ַ",1);
	 	   
   	   if (layer3Light>0)
   	   {
   	     guiDisplay(144,33,"��",1);
   	   }
       
       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
       {
   	     guiDisplay(144,144,"��",1);
   	   }
   	   
   	   tmpY = 49;
   	   tmpLink = queryMpLink;
   	   i = 0;
   	   while(tmpLink!=NULL && i<layer3Light)
   	   {
   	     for(tmpCount=0;tmpCount<NUM_MP_PER_PAGE-1;tmpCount++)
   	     {
   	   	   tmpLink = tmpLink->next;
   	     }
   	     i++;
   	   }
   	   tmpCount = 0;
   	   while((tmpLink!=NULL) && (tmpCount<(NUM_MP_PER_PAGE-1)))
   	   {
   	   	  //
   	   	  strcpy(sayStr,intToString(tmpLink->mpNo,3,str));
   	   	  strcpy(str,"");
   	   	  if (strlen(sayStr)==1)
   	   	  {
   	   	  	 strcat(str,"00");
   	   	  }
   	   	  if (strlen(sayStr)==2)
   	   	  {
   	   	  	 strcat(str,"0");
   	   	  }	 	   	  
   	   	  strcat(str,sayStr);
   	   	  guiDisplay(22,tmpY,str,1);

       	  strcpy(str,"");
       	  for(i=6; i>0; i--)
       	  {
       	  	 if(tmpLink->addr[i-1]==0x00)
       	  	 {
       	  	 	  strcat(str, "00");
       	  	 }
       	  	 else
       	  	 {
       	  	 	  strcat(str,digital2ToString((tmpLink->addr[i-1]/0x10)*10
       	  	          +tmpLink->addr[i-1]%0x10,sayStr));
       	  	 }
       	  }
 	   	    guiDisplay(65,tmpY,str,1);

   	   	  tmpY += 16;
   	   	  tmpLink = tmpLink->next;
   	   	  
   	   	  tmpCount++;
   	   }	 	   
	 	   break;
	 	  	
	 	 case 1:
	 	   guiDisplay(1,17,"2-2ͨ�Ų�����ѯ",1);
	 	   //��������
	 	   guiDisplay(1,33,"��������",1);
       strcpy(sayStr,digitalToChar(addrField.a1[1]>>4));
       strcat(sayStr,digitalToChar(addrField.a1[1]&0xf));
       strcat(sayStr,digitalToChar(addrField.a1[0]>>4));
       strcat(sayStr,digitalToChar(addrField.a1[0]&0xf));
	 	   guiDisplay(68,33,sayStr,1);
	 	   
	 	   //�����ŵ�
	 	   guiDisplay(1,49,"�����ŵ�",1);
	 	   switch(moduleType)
	 	   {
	 	   	  case GPRS_SIM300C:
	 	   	  case GPRS_GR64:
	 	   	  case GPRS_M590E:
	 	   	  case GPRS_M72D:
	 	   	  	strcpy(sayStr, "GPRS");
	 	   	  	break;

	 	   	  case CDMA_DTGS800:
	 	   	  case CDMA_CM180:
	 	   	  	strcpy(sayStr, "CDMA");
	 	   	  	break;
					
	 	   	  case LTE_AIR720H:
	 	   	  	strcpy(sayStr, "LTE");
	 	   	  	break;

	 	   	  case ETHERNET:
	 	   	  	strcpy(sayStr, "��̫��");
	 	   	  	break;
	 	   	  
	 	   	  default:
	 	   	  	strcpy(sayStr,"��ģ��");
	 	   	  	break;
	 	   }
	 	   guiDisplay(68,49,sayStr,1);
	 	   
	 	   //APN����
	 	   guiDisplay(1,  65, "APN����",1);
	 	   guiDisplay(68, 65, (char *)ipAndPort.apn, 1);
	 	   
	 	   guiDisplay(1,81,"��վIP������˿�",1);
	  	 strcpy(sayStr,intToIpadd(ipAndPort.ipAddr[0]<<24 | ipAndPort.ipAddr[1]<<16 | ipAndPort.ipAddr[2]<<8 | ipAndPort.ipAddr[3],str));
       strcat(sayStr,":");
	 	   guiDisplay(1,97,sayStr,1);
	 	   tmpX = 1+8*strlen(sayStr);
 	     strcpy(sayStr,intToString(ipAndPort.port[1]<<8 | ipAndPort.port[0],3,str));
	 	   guiDisplay(tmpX,97,sayStr,1);
	 	   
	 	   guiDisplay(1,113,"������IP",1);
	 	   guiDisplay(1,129,intToIpadd(wlLocalIpAddr,str),1);
	 	   guiDisplay(1,145,"����汾",1);
	 	   guiDisplay(68,145,vers,1);
	 	   break;
	 	   
	 	 case 2:
	 	   guiDisplay(1,17,"2-3���������ѯ",1);
	 	   guiDisplay(1,33,"����ʼʱ��  00:01",1);

	 	   strcpy(sayStr,"�ص㻧������");
	 	   strcat(sayStr,intToString(teCopyRunPara.para[0].copyInterval,3,str));
	 	   strcat(sayStr,"����");
	 	   guiDisplay(1,49,sayStr,1);
	 	   
	 	   guiDisplay(1,65,"�ص㻧�ɼ���U",1);
	 	   guiDisplay(105,65,"/",1);
	 	   guiDisplay(112,65,"I",1);
	 	   guiDisplay(119,65,"/",1);
	 	   guiDisplay(126,65,"P",1);
	 	   guiDisplay(133,65,"/",1);
	 	   guiDisplay(140,65,"Q",1);
	 	   guiDisplay(147,65,"/",1);
	 	   guiDisplay(153,65,"R",1);
	 	   break;
	 	   
	 	 case 3:
	 	   guiDisplay(1,17,"2-4�м�·����Ϣ��ѯ",1);
	 	   guiDisplay(1,33,"1���м̵����Ϣ:",1);
	 	   guiDisplay(1,49,"������ �ϼ��м̱��",1);
	 	   break;
	 	   
	 	 case 4:
	 	   guiDisplay(1,17,"2-5�ж�·����Ϣ��ѯ",1);
	 	   guiDisplay(1,33,"2���м̵����Ϣ",1);
	 	   guiDisplay(1,49,"������ �ϼ��м̱��",1);
	 	   break;
	 }
	 
	 lcdRefresh(17, 160);
}

/*******************************************************
��������:keyHouseholdMenu
��������:�ص��û���Ϣ��ѯ�˵�
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void keyHouseholdMenu(INT8U layer2Light,INT8U layer3Light)
{
	 INT8U             dataBuff[LENGTH_OF_ENERGY_RECORD];
 	 INT32U            integer,decimal,disData;
	 char              str[30];
	 char              sayStr[30];
	 INT8U             i, tmpY, tmpCount, tmpX=0;
	 INT8U             dataType;
	 INT8U             tmpMinute;
	 DATE_TIME         tmpTime, readTime;
   struct cpAddrLink *tmpLink;
   INT16U            offset;
   INT8U             sign;
   INT8U             meterInfo[10];
   BOOL              buffHasData;
   INT8U             xx;

	 guiLine(1,17,160,160,0); //����
	 menuInLayer = 2;         //�˵������2��
	 
	 switch(layer2Light)
	 {
	 	 case 0:
	 	   guiDisplay(1,17,"3-1�ص��û������ѯ",1);
	 	   showInputTime(layer3Light);
	 	   break;
	 	   
	 	 case 1:
	 	   guiDisplay(1,17,"3-2�ص��û�����ʾֵ",1);
	 	   guiDisplay(78,33,"����ʾֵ",1);
	 	   dataType = ENERGY_DATA;
	 	   offset   = POSITIVE_WORK_OFFSET;
	 	 	 break;
	 	 	 
	 	 case 2:
	 	   guiDisplay(1,17,"3-3�ص��û��й�����",1);
	 	   guiDisplay(78,33,"�й�����",1);
	 	   dataType = PARA_VARIABLE_DATA;
	 	   offset   = POWER_INSTANT_WORK;
	 	 	 break;

	 	 case 3:
	 	   guiDisplay(1,17,"3-4�ص��û��޹�����",1);
	 	   guiDisplay(78,33,"�޹�����",1);
	 	   dataType = PARA_VARIABLE_DATA;
	 	   offset   = POWER_INSTANT_NO_WORK;
	 	 	 break;

	 	 case 4:
	 	   guiDisplay(1,17,"3-5�ص��û���������",1);
	 	   guiDisplay(70,33,"A�� B�� C��",1);
	 	   dataType = PARA_VARIABLE_DATA;
	 	   offset = CURRENT_PHASE_A;
	 	 	 break;

	 	 case 5:
	 	   guiDisplay(1,17,"3-6�ص��û���ѹ����",1);
	 	   guiDisplay(70,33,"A�� B�� C��",1);
	 	   dataType = PARA_VARIABLE_DATA;
	 	   offset = VOLTAGE_PHASE_A;
	 	 	 break;
	 }

	 lcdRefresh(17, 160);
	 
	 if (layer2Light>=1 && layer2Light<=5)
	 {
 	   if (layer3Light>0)
 	   {
 	     //guiDisplay(144,33,"��",1);
 	   }
     
     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
     {
 	     //guiDisplay(144,144,"��",1);
 	   }
 	   
 	   tmpLink = queryMpLink;
 	   i = 0;
 	   while(tmpLink!=NULL && i<layer3Light)
 	   {
 	   	  tmpLink = tmpLink->next;
 	      i++;
 	   }
 	   if (tmpLink==NULL)
 	   {
 	   	  tmpLink = queryMpLink;
 	   }
 	   
 	   strcpy(sayStr,"������");
 	   if (tmpLink->mp<10)
 	   {
 	   	 strcat(sayStr,"0");
 	   }
 	   strcat(sayStr,intToString(tmpLink->mp,3,str));
 	   guiDisplay(1,33,sayStr,1);

		 tmpTime = menuQueryTime;
		 tmpTime.hour   = keyLeftRight*6;
		 tmpTime.minute = 0x0;
		 tmpTime.second = 0x0;
		 tmpTime = backTime(tmpTime, 0, 0, 0, 15, 0);
	   
	   lcdRefresh(33, 48);
		 
		 tmpY = 52;
		 for(i = 0; i < 6; i++)
		 {
				strcpy(sayStr,"");
				if (tmpTime.hour==23)
				{
					strcpy(sayStr,"00");
				}
				else
				{
				  if (tmpTime.hour<9)
				  {
					   strcat(sayStr,"0");
				  }
				  strcat(sayStr,intToString(tmpTime.hour+1,3,str));
				}
				guiDisplay(17, tmpY, sayStr, 1);
				
				buffHasData = FALSE;
				
				queryMeterStoreInfo(tmpLink->mp,meterInfo);
				readTime  = timeHexToBcd(tmpTime);
				if (meterInfo[0]==8)
				{
          buffHasData = readMeterData(dataBuff, tmpLink->mp, CURVE_KEY_HOUSEHOLD, dataType, &readTime, 0x60);
				}
				else
				{
          buffHasData = readMeterData(dataBuff, tmpLink->mp, CURVE_DATA_PRESENT, dataType, &readTime, 0x60);
				}
				strcpy(sayStr,"");
				if(buffHasData== TRUE)
				{
				   if (dataBuff[offset]!=0xee)
       	   {
  				   switch(layer2Light)
  				   {
  				     case 1:  //����ʾֵ
                 decimal = (dataBuff[offset]>>4 & 0xf) * 10 + (dataBuff[offset] & 0xf);
                 integer = (dataBuff[offset+3]>>4 & 0xf)*100000
                           +(dataBuff[offset+3] & 0xf)*10000
                           +(dataBuff[offset+2]>>4 & 0xf)*1000
                           +(dataBuff[offset+2] & 0xf)*100
                           +(dataBuff[offset+1]>>4 & 0xf)*10
                           +(dataBuff[offset+1]& 0xf);
                 strcpy(str, floatToString(integer,decimal,2,2,str));
                 for(tmpCount=0;tmpCount<10-strlen(str);tmpCount++)
                 {
                	 strcat(sayStr," ");
                 }
                 strcat(sayStr,str);
                 tmpX = 0;
                 break;
                 
               case 2: //�й�����
               case 3: //�޹�����
     	    	     sign = 0;
               	 disData = dataBuff[offset] | dataBuff[offset+1]<<8 | dataBuff[offset+2]<<16;
     	    	     if (dataBuff[offset+2]&0x80)
     	    	     {
     	    	        sign = 1;
     	    	        disData &=0x7fff;
     	    	     }
                 decimal = (disData>>12 & 0xf)*1000
                          +(disData>>8 & 0xf)*100
                          +(disData>>4 & 0xf)*10
                          +(disData & 0xf);
                 integer = (disData>>20 & 0xf)*10+(disData>>16 & 0xf);
                 if (sign==1)
                 {
                 	 strcat(sayStr,"-");
                 }
                 strcat(sayStr,floatToString(integer,decimal,4,2,str));
                 if (strlen(sayStr)<=5)
                 {
                   strcpy(str,"");
                   for(tmpCount=0;tmpCount<5-strlen(sayStr);tmpCount++)
                   {
                	   strcat(str," ");
                   }
                   strcat(str, sayStr);
                   strcpy(sayStr,str);
                 }
                 tmpX = 24;
               	 break;
               	 
               case 4:  //����
               	 for(tmpCount=0; tmpCount<3; tmpCount++)
               	 {
               	   disData = dataBuff[offset+tmpCount*3] | dataBuff[offset+tmpCount*3+1]<<8 | dataBuff[offset+tmpCount*3+2]<<16;
               	   decimal = bcdToHex(disData& 0xfff);
                   integer = bcdToHex(disData>>12& 0xfff);
                   floatToString(integer,decimal,3,1,str);
                   if (strlen(str)==1)
                   {
                   	 strcat(sayStr,"  ");
                   }
                   if (strlen(str)==2)
                   {
                   	 strcat(sayStr," ");
                   }
                   strcat(sayStr,str);
                   if (tmpCount!=2)
                   {
                     strcat(sayStr," ");
                   }
                 }
                 tmpX = 0;
               	 break;

               case 5:  //��ѹ
               	 for(tmpCount=0; tmpCount<3; tmpCount++)
               	 {
               	   disData = dataBuff[offset+tmpCount*2] | dataBuff[offset+tmpCount*2+1]<<8;
               	   decimal = disData& 0xf;
                   integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10 + (disData>>4 & 0xf);
                   floatToString(integer,decimal,0,0,str);
                   if (strlen(str)==1)
                   {
                   	 strcat(sayStr,"  ");
                   }
                   if (strlen(str)==2)
                   {
                   	 strcat(sayStr," ");
                   }
                   strcat(sayStr,str);
                   if (tmpCount!=2)
                   {
                     strcat(sayStr," ");                   
                   }
                 }
                 tmpX = 0;
               	 break;
             }
       	   }
       	   else
       	   {
       	      strcpy(sayStr,"-----");
       	      if (tmpX==0)
       	      {
       	      	 tmpX = 24;
       	      }
           }
				}
				else
				{
				   strcpy(sayStr,"-----");
       	   if (tmpX==0)
       	   {
       	   	 tmpX = 24;
       	   }
       	}
				
				guiDisplay(70+tmpX,tmpY,sayStr,1);

				lcdRefresh(tmpY, tmpY+16);
				
				tmpY += 18;
				tmpTime = nextTime(tmpTime, 60, 0);				
		 }
	 }
	 
	 lcdRefresh(17, 160);
}

/*******************************************************
��������:statisQueryMenu
��������:ͳ����Ϣ��ѯ�˵�
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void statisQueryMenu(INT8U layer2Light,INT8U layer3Light)
{
	 char                   str[30];
	 char                   sayStr[30];
	 INT8U                  i, tmpY, tmpCount, tmpX=0;
	 INT16U                 tmpSuccess,tmpFailure;
	 struct cpAddrLink      *tmpNode;
   TERMINAL_STATIS_RECORD terminalStatisRecord;  //�ն�ͳ�Ƽ�¼
   DATE_TIME              tmpTime;
   INT32U                 tmpData;
   INT8U                  buf[20];
   INT8U                  k;

   if (layer2Light>0)
   {
     guiLine(16,56,144,88,0);
     guiLine(16,56,16,88,1);
     guiLine(144,56,144,88,1);
     guiLine(16,88,144,88,1);
     guiLine(16,56,144,56,1);
     guiDisplay(30,64,"��ȡ������...",1);
     lcdRefresh(56,88);
   }

	 guiLine(1,17,160,160,0); //����
	 menuInLayer = 2;         //�˵������2��

	 switch(layer2Light)
	 {
	 	 case 0:
	 	   guiDisplay(1,17,"4-1����ͳ����Ϣ��ѯ",1);
	 	   showInputTime(layer3Light);
	 	   break;
	 	   
	 	 case 1:
	 	   guiDisplay(1,17,"4-2����ͳ��",1);
	 	   guiDisplay(1,33,"���ܱ�����",1);
	 	   guiDisplay(90,33, intToString(meterDeviceNum,3,str),1);
	 	   guiDisplay(1,49,"�ص��û���",1);
	 	   guiDisplay(90,49, intToString(keyHouseHold.numOfHousehold, 3, str), 1);
	 	   tmpSuccess = 0;
	 	   tmpFailure = 0;
       
       tmpTime = timeHexToBcd(menuQueryTime);
       tmpNode = initPortMeterLink(0xff);

       lcdLightDelay = nextTime(sysTime, 20, 0);

       k = 0;
       while(tmpNode!=NULL)
       {
         if (checkLastDayData(tmpNode->mp, 1, tmpTime, 0)==TRUE)
         {
         	 tmpSuccess++;
         }

       	 tmpNode = tmpNode->next;
      	 
      	 k++;
      	 if (k>=15)
      	 {
      	   k = 0;
      	   
      	   //��ѯ�ڼ䷢��������xMega,����xMega��CPU����λ�ź�
      	   sendXmegaInTimeFrame(CPU_HEART_BEAT, buf, 0);
      	 }

         //�ͷ�CPUʱ��Ƭ�������߳�ִ��
         usleep(100);
       }

	 	   tmpFailure = meterDeviceNum-tmpSuccess;
	 	   
	 	   guiDisplay(1,65,"����ɹ���",1);
       guiDisplay(90,65, intToString(tmpSuccess, 3, str),1);
	 	   guiDisplay(1,81,"����ʧ����",1);
       guiDisplay(90,81, intToString(tmpFailure, 3, str),1);
	 	 	 break;
	 	 	 
	 	 case 2:
	 	   guiDisplay(1,17,"4-3����ʧ���嵥",1);
	 	   guiDisplay(1,33,"�������",1);

	 	   tmpNode = copyCtrl[4].cpLinkHead;
	 	   tmpX    = 1;
	 	   tmpY    = 49;
	 	   tmpFailure = 0;
	 	   while(tmpNode!=NULL)
	 	   {
	 	   	 if (tmpNode->copySuccess==FALSE)
	 	   	 {
 	   	     tmpFailure ++;
 	   	     strcpy(sayStr,intToString(tmpNode->mp,3,str));
 	   	     strcpy(str,"");
 	   	     if (strlen(sayStr)==1)
 	   	     {
 	   	  	   strcat(str," 00");
 	   	     }
 	   	     if (strlen(sayStr)==2)
 	   	     {
 	   	  	   strcat(str," 0");
 	   	     }
 	   	     strcat(str,sayStr);
 	   	     guiDisplay(tmpX,tmpY,str,1);
 	   	     tmpX += 40;
 	   	     if ((tmpFailure%4)==0)
 	   	     {
 	   	     	  tmpY += 16;
 	   	     }
 	   	     
 	   	     //�账����һ��
 	   	     if (tmpY>=160)
 	   	     {
 	   	     	  break;
 	   	     }
	 	   	 }
	 	   	  
	 	   	 tmpNode = tmpNode->next;	 	   	  
	 	   }
	 	 	 break;
	 	 	 
	 	 case 3:
	 	   guiDisplay(1,17,"4-4����ͨ��ͳ��",1);
	 	   guiDisplay(1,33,"����ͨ������",1);
	 	   guiDisplay(1,49,"����ͨ������",1);
	 	   guiDisplay(1,65,"�ź�ǿ��Max",1);
	 	   guiDisplay(1,81,"����ʱ��",1);
	 	   guiDisplay(1,97,"�ź�ǿ��Min",1);
	 	   guiDisplay(1,113,"����ʱ��",1);	 	   
	     
 	   	 tmpTime = timeHexToBcd(menuQueryTime);
	     if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
       {
       	  if ((terminalStatisRecord.sendBytes+terminalStatisRecord.receiveBytes)<1024)
       	  {
       	    strcpy(sayStr, intToString(terminalStatisRecord.sendBytes+terminalStatisRecord.receiveBytes,3,str));
       	    strcat(sayStr, "B");
       	  }
       	  else
       	  {
       	    strcpy(sayStr, intToString((terminalStatisRecord.sendBytes+terminalStatisRecord.receiveBytes)/1024,3,str));
       	    strcat(sayStr, "KB");
       	  }
       	  guiDisplay(104, 33, sayStr, 1);
       	  guiDisplay(104,65,intToString(terminalStatisRecord.maxSignal,3,str),1);
       	  strcpy(sayStr,"");
       	  strcat(sayStr,digital2ToString(terminalStatisRecord.maxSignalTime[1],str));
       	  strcat(sayStr,":");
       	  strcat(sayStr,digital2ToString(terminalStatisRecord.maxSignalTime[0],str));
       	  guiDisplay(104,81,sayStr,1);
       	  
       	  guiDisplay(104,97,intToString(terminalStatisRecord.minSignal,3,str),1);
       	  strcpy(sayStr,"");
       	  strcat(sayStr,digital2ToString(terminalStatisRecord.minSignalTime[1],str));
       	  strcat(sayStr,":");
       	  strcat(sayStr,digital2ToString(terminalStatisRecord.minSignalTime[0],str));
       	  guiDisplay(104,113,sayStr,1);
       	         	  
       	  tmpCount = monthDays(sysTime.year+2000,sysTime.month);
       	  tmpData = 0;
       	  for(i=1;i<=tmpCount && i<=sysTime.day;i++)
       	  {
       	  	 tmpTime = sysTime;
       	  	 tmpTime.day = i;
       	  	 tmpTime = timeHexToBcd(tmpTime);
	           if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
             {
             	  tmpData += terminalStatisRecord.sendBytes+terminalStatisRecord.receiveBytes;
             }
       	  }
       	  if (tmpData<1024)
       	  {
       	    strcpy(sayStr,intToString(tmpData,3,str));
       	    strcat(sayStr, "B");
       	  }
       	  else
       	  {
       	    strcpy(sayStr,intToString(tmpData/1024,3,str));
       	    strcat(sayStr, "KB");
       	  }
       	  guiDisplay(104, 49, sayStr, 1);       	  
       }
	 	 	 break;
	 	 	 
	 	 case 4:
	 	   guiDisplay(1,17,"4-5�м�·����Ϣͳ��",1);
	 	   guiDisplay(1,33,"0���м̵��ܱ���",1);
	 	   guiDisplay(1,49,"1���м̵��ܱ���",1);
	 	   guiDisplay(1,65,"2���м̵��ܱ���",1);
	 	 	 break;
	 }
	 
	 lcdRefresh(17,160);
}

/*******************************************************
��������:paraSetMenu
��������:�������ò˵�
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void paraSetMenu(int lightNum,int layer1Num)
{
	 INT8U i;

	 guiLine(1,17,160,160,0); //����
	 menuInLayer = 2;         //�˵������2��
   
	 for(i=0;i<layer2MenuNum[layer1Num];i++)
	 {
		 if (i==lightNum)
		 {
        guiDisplay(16,40+i*17,paraSetItem[i],0);
		 }
		 else
		 {
        guiDisplay(16,40+i*17,paraSetItem[i],1);
		 }
	 }
	 
	 lcdRefresh(17, 160);
}

/*******************************************************
��������:debugMenu
��������:�ֳ����Բ˵�
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void debugMenu(int lightNum,int layer1Num)
{
	 INT8U i;

	 guiLine(1,17,160,160,0); //����
	 menuInLayer = 2;         //�˵������2��
   
   if (carrierModuleType==SR_WIRELESS || carrierModuleType==HWWD_WIRELESS || carrierModuleType==RL_WIRELESS)
   {
   	 strcpy(debugItem[3], "   �����ڵ��ַ   ");
   	 strcpy(debugItem[5], "  δ�����ڵ��ַ  ");
   }
   
	 for(i=0;i<layer2MenuNum[layer1Num];i++)
	 {
		 if (i==lightNum)
		 {
        guiDisplay(8,32+i*17,debugItem[i],0);
		 }
		 else
		 {
        guiDisplay(8,32+i*17,debugItem[i],1);
		 }
	 }
	 
	 lcdRefresh(17, 160);
}

/*******************************************************
��������:noFoundMeter
��������:δ�ѵ����ܱ��ַ
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void noFoundMeter(INT8U lightNum)
{
	 INT8U  tmpY, i;
	 INT16U tmpNum;
   char   str[10],sayStr[15];
   
	#ifdef MENU_FOR_CQ_CANON 
	 guiLine(1,17,160,160,0); //����
   if (carrierModuleType==SR_WIRELESS || carrierModuleType==HWWD_WIRELESS || carrierModuleType==RL_WIRELESS)
   {
     guiDisplay(1,17,"6-8δ�����ڵ��ַ",1);
   }
   else
   {
     guiDisplay(1,17,"6-8δ�ѵ����ܱ��ַ",1);
     
     if (carrierFlagSet.searchMeter==0)
     {
       if (carrierFlagSet.ifSearched == 0)
       {
         guiDisplay(12,49,"δ�����ѱ�,��ͳ��",1);
       }
     }
     else
     {
       guiDisplay(17,49,"�ѱ���,��ȴ�...",1);
     }
   }
	#else
	 guiLine(1,17,160,144,0); //����
   if (carrierModuleType==SR_WIRELESS || carrierModuleType==HWWD_WIRELESS || carrierModuleType==RL_WIRELESS)
   {
     guiDisplay(1,17,"   δ�����ڵ��ַ",1);
   }
   else
   {
     guiDisplay(1,17,"  δ�ѵ����ܱ��ַ",1);
   }
	#endif
	
	 menuInLayer = 3;         //�˵������3��

   if (lightNum!=0xff)
   {
		 //�������ҳ��
		 multiCpMax = 0;
		 tmpFound = noFoundMeterHead;
		 while(tmpFound!=NULL)
		 {
			 multiCpMax++;
			 tmpFound = tmpFound->next;
		 }
		 
		#ifdef MENU_FOR_CQ_CANON
		 if ((multiCpMax%(NUM_MP_PER_PAGE-1))!=0)
		 {
		    multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-1)+1;
		 }
		 else
		 {
		   multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-1);
		 }
		#else
		 if ((multiCpMax%(NUM_MP_PER_PAGE-2))!=0)
		 {
		    multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-2)+1;
		 }
		 else
		 {
		   multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-2);
		 }
		#endif

     //��������Ҫ��ʾ��ҳ
     tmpFound = noFoundMeterHead;
     if (tmpFound!=NULL)
     {
       #ifdef MENU_FOR_CQ_CANON
        for(i=0;i<multiCpUpDown*(NUM_MP_PER_PAGE-1);i++)
       #else
        for(i=0;i<multiCpUpDown*(NUM_MP_PER_PAGE-2);i++)
       #endif
        {
      	  tmpFound = tmpFound->next;      	  
      	  
      	  if (tmpFound==NULL)
      	  {
      	  	 break;
      	  }
        }
     }

     tmpY     = 33;
     tmpNum   = 0;
     while(tmpFound!=NULL)
     {
     	  strcpy(sayStr,"");
     	  for(i=6;i>0;i--)
     	  {
     	    strcat(sayStr,digital2ToString((tmpFound->addr[i-1]/0x10)*10 + tmpFound->addr[i-1]%0x10,str));
     	  }
     	  
     	  guiDisplay(32,tmpY, sayStr, 1);
     	  
     	  tmpY += 16;
     	  tmpNum++;
     	 #ifdef MENU_FOR_CQ_CANON
     	  if (tmpNum>=(NUM_MP_PER_PAGE-1))
     	 #else
     	  if (tmpNum>=(NUM_MP_PER_PAGE-2))
     	 #endif
     	  {
     	  	 break;
     	  }
     	  
     	  tmpFound = tmpFound->next;
     }
   }
   
  #ifdef MENU_FOR_CQ_CANON 
   lcdRefresh(17,160);
  #else
   lcdRefresh(17,145);
  #endif
}

/*******************************************************
��������:activeReportMenu
��������:�����ϱ��˵�
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void activeReportMenu(INT8U lightNum)
{
	 INT8U tmpX, i;
   char  str[2];
   
	 guiLine(1,17,160,160,0); //����
	 menuInLayer = 3;         //�˵������3��

   guiDisplay(1,17,"6-9�����ϱ�",1);
   
   guiLine(45,60,45,78,1);
   guiLine(115,60,115,78,1);
   guiLine(45,60,115,60,1);
   guiLine(45,78,115,78,1);

   if (lightNum==0)
   {
     guiDisplay(48,61,"������վ",0);
   }
   else
   {
     guiDisplay(48,61,"������վ",1);
   }

   lcdRefresh(17,160);
}


//���ϴ�Լ81�п�ʼ�������켯�����˵�

#else  //MENU_FOR_CQ_CANON , �ӱ��п�ʼ�ǹ��ҵ���������Ҫ��˵�

/**************************************************
��������:userInterface
��������:�˻��ӿڴ���(376.1���ҵ�����������Լ)
���ú���:
�����ú���:
�������:void *arg
�������:
����ֵ��״̬
***************************************************/
void userInterface(BOOL secondChanged)
{
	 METER_DEVICE_CONFIG meterConfig;
   char      str[30],strX[30];
   INT16U    tmpData;
   INT8U     i;
   DATE_TIME tmpTime;
   INT16U    tmpAddr;                    //��ʱ�ն˵�ַ
   char      *tmpChar;

   //ly,2011-4-13,addd
   register int fd, interface, retn = 0;
   struct ifreq buf[MAXINTERFACES];
   struct ifconf ifc;

   if (secondChanged==TRUE)
   {
     if (aberrantAlarm.aberrantFlag==1)
     {
       guiLine(44,1,62,16,0);
   
       if (aberrantAlarm.blinkCount==1)
       {
         aberrantAlarm.blinkCount = 2;
         guiDisplay(46, 1, "��", 1);
         guiLine(46,1,62,4,0);
         guiDisplay(46, 2, "��", 1);
       }
       else
       {
   	     guiDisplay(46, 1, digital2ToString(aberrantAlarm.eventNum, str), 1);
   	     aberrantAlarm.blinkCount = 1;
   	   }
       
       if (compareTwoTime(aberrantAlarm.timeOut, sysTime))
       {
         guiLine(30,1,62,16,0);
         aberrantAlarm.aberrantFlag = 0;
       }
       
       lcdRefresh(1,16);
     }
     
     //titleʱ��
     if (sysTime.second==0)
     {
       refreshTitleTime();
     }

     if (lcdLightOn==LCD_LIGHT_ON)
     {
       if (compareTwoTime(lcdLightDelay, sysTime))
       {
     	  #ifndef LIGHTING
     	   if (displayMode==CYCLE_DISPLAY_MODE)
     	   {
     	  #else
     	   defaultMenu();
     	  #endif
     	     
     	     lcdLightOn = LCD_LIGHT_OFF;
     	     lcdBackLight(LCD_LIGHT_OFF);
     	     
     	  #ifndef LIGHTING
     	   }
     	   else
     	   {
     	     //�������Բ˵�ģʽ
     	     menuInLayer = 0;
     	     defaultMenu();
     	     
           while(cycleMpLink!=NULL)
           {
           	  tmpCycleLink = cycleMpLink;
           	  cycleMpLink  = cycleMpLink->next;
           	  free(tmpCycleLink);
           }
           
           //2012-10-23,�޸�Ϊ�������ݿ�����
           if (cycleDataType==0x55)
           {
             cycleMpLink = initPortMeterLink(0x01);
           }
           else
           {
             cycleMpLink = initPortMeterLink(0xff);
           }
           tmpCycleLink = cycleMpLink;
           if (cycleMpLink==NULL)
           {
     	       lcdLightOn = LCD_LIGHT_OFF;
     	       lcdBackLight(LCD_LIGHT_OFF);
           }
           else
           {           
     	       lcdLightDelay = nextTime(sysTime,1,0);
     	       displayMode = CYCLE_DISPLAY_MODE;
     	       cycleMenu();
     	     }
     	   }
     	  #endif
       }
     }
     
    #ifndef LIGHTING
     if (displayMode==CYCLE_DISPLAY_MODE)
     {
     	 if (compareTwoTime(cycleDelay,sysTime))
     	 {
     	 	 cycleMenu();
     	 }
     }
    #endif
     
     if (setParaWaitTime!=0 && setParaWaitTime!=0xfe)
     {
    		setParaWaitTime--;
    		if (setParaWaitTime<1)
    		{
         	setBeeper(BEEPER_OFF);         //������
         	alarmLedCtrl(ALARM_LED_OFF);   //ָʾ����
     	    lcdBackLight(LCD_LIGHT_OFF);
    		}
     }
     
     //Ĭ�ϲ˵�ʱ��ʾʱ��
     if (displayMode==DEFAULT_DISPLAY_MODE && menuInLayer==0)
     {
       guiLine(40,90,120,122,0);
       guiAscii(40, 90, "20",1);
       guiAscii(56, 90, digital2ToString(sysTime.year,str),1);
       guiAscii(72, 90, "-", 1);
       guiAscii(80, 90, digital2ToString(sysTime.month,str),1);
       guiAscii(96, 90, "-", 1);
       guiAscii(104, 90, digital2ToString(sysTime.day,str),1);
     
       guiAscii(48, 106, digital2ToString(sysTime.hour,str),1);
       guiAscii(64, 106, ":", 1);
       guiAscii(72, 106, digital2ToString(sysTime.minute,str),1);
       guiAscii(90, 106, ":", 1);
       guiAscii(98, 106, digital2ToString(sysTime.second,str),1);
       lcdRefresh(90,122);
     }
   }

   if ((keyValue = ioctl(fdOfIoChannel,READ_KEY_VALUE,0))!=0)
   {
      keyPressCount++;
      
      if (keyPressCount>keyCountMax)
      {
        lcdBackLight(LCD_LIGHT_ON);

        lcdLightOn = LCD_LIGHT_ON;
        
        lcdLightDelay = nextTime(sysTime, 1, 0);

        keyPressCount = 0;
        
        if (displayMode!=KEYPRESS_DISPLAY_MODE)
        {
          displayMode = KEYPRESS_DISPLAY_MODE;
          guiLine(85, 1,117,16,0);
          lcdRefresh(1,16);
          layer1Menu(layer1MenuLight);
          return;
        }
        
        switch(keyValue)
        {
      	  case KEY_OK:   //ȷ��
      	    switch(menuInLayer)
      	    {
      	    	case 1:      //�˵���1��
      	    		switch(layer1MenuLight)
      	    		{
	    		 	      case 0://���������ݲ�ѯ
	    		 	 	     #ifdef LIGHTING
	    		 	 	      
	    		 	 	      queryMpLink = copyCtrl[4].cpLinkHead;
	    		 	 	      if (queryMpLink==NULL)
	    		 	 	      {
                      guiLine(10,55,150,105,0);
                      guiLine(10,55,10,105,1);
                      guiLine(150,55,150,105,1);
                      guiLine(10,55,150,55,1);
                      guiLine(10,105,150,105,1);
                      guiDisplay(12,70,"δ���ÿ��Ƶ����!",1);
                      lcdRefresh(10,120);
	    		 	 	      }
	    		 	 	      else
	    		 	 	      {
	    		 	 	        ccbStatus();
	    		 	 	      }
	    		 	 	      
	    		 	 	     #else
	    		 	 	     
	    		 	 	      layer2MenuLight[0] = 0;
	    		 	 	      layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;      	    		 	 	 
	    		 	 	      fillTimeStr();   //�õ�ǰ��������ѯ�����ַ���
	    		 	 	      queryMpLink = initPortMeterLink(0xff);
	    		 	 	      copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
	    		 	 	      
	    		 	 	     #endif
	    		 	 	      break;
      	    		 	 
      	    		 	case 1:
      	    		 		layer2xMenuLight = 0;
      	    		 		layer2xMenu(0, layer2xMenuLight);
      	    		 		
      	    		 		//pwLight = 0;
      	    		 	 	//strcpy(passWord,"000000");
      	    		 	 	//inputPassWord(pwLight);
      	    		 	 	break;
      	    		 	 	 	 
      	    		 	case 2:
      	    		 	  layer2Menu(layer2MenuLight[layer1MenuLight], layer1MenuLight);
      	    		 	  break;
      	    		 	
      	    		 #ifdef LIGHTING
      	    		 	case 3:
	    		 	 	      queryMpLink = xlcLink;
	    		 	 	      if (queryMpLink==NULL)
	    		 	 	      {
                      guiLine(10,55,150,105,0);
                      guiLine(10,55,10,105,1);
                      guiLine(150,55,150,105,1);
                      guiLine(10,55,150,55,1);
                      guiLine(10,105,150,105,1);
                      guiDisplay(12,70,"δ���ÿ��Ƶ����!",1);
                      lcdRefresh(10,120);
	    		 	 	      }
	    		 	 	      else
	    		 	 	      {
	    		 	 	        xlcStatus();
	    		 	 	      }
      	    		 		
      	    		 		break;
      	    		 		
      	    		 	case 4:
	    		 	 	      queryMpLink = ldgmLink;
	    		 	 	      if (queryMpLink==NULL)
	    		 	 	      {
                      guiLine(10,55,150,105,0);
                      guiLine(10,55,10,105,1);
                      guiLine(150,55,150,105,1);
                      guiLine(10,55,150,55,1);
                      guiLine(10,105,150,105,1);
                      guiDisplay(12,70,"δ���ÿ��Ƶ����!",1);
                      lcdRefresh(10,120);
	    		 	 	      }
	    		 	 	      else
	    		 	 	      {
	    		 	 	        ldgmStatus();
	    		 	 	      }      	    		 		
      	    		 		break;
      	    		 		
      	    		 	case 5:
	    		 	 	      queryMpLink = lsLink;
	    		 	 	      if (queryMpLink==NULL)
	    		 	 	      {
                      guiLine(10,55,150,105,0);
                      guiLine(10,55,10,105,1);
                      guiLine(150,55,150,105,1);
                      guiLine(10,55,150,55,1);
                      guiLine(10,105,150,105,1);
                      
	                    guiDisplay(40, 60, "�����ն�ֵ", 1);
                      sprintf(str, "%ldLux", downLux[0] | downLux[1]<<8 | downLux[2]<<16);
	                    guiDisplay(50, 80, str, 1);
                      //guiDisplay(12,70,"δ���ÿ��Ƶ����!",1);
                      lcdRefresh(10,120);
	    		 	 	      }
	    		 	 	      else
	    		 	 	      {
	    		 	 	        lsStatus();
	    		 	 	      }      	    		 		
      	    		 		break;
      	    		 #endif
      	    		}
      	    		break;
      	    		 	 
      	    	case 2:      //�˵���2��
      	    		switch(layer1MenuLight)
      	    		{
      	    		 	 case 0:  //�����ѯ
      	    		 	 	#ifdef LIGHTING
      	    		 	 	
      	    		 	 	#else 
      	    		 	 	 
      	    		 	 	 //�����1-1(ѡ���ѯ����)��ȷ����Ҫ�ж������Ƿ���ȷ
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]==0)
      	    		 	 	 {
      	    		 	 	 	 //�ж�����������Ƿ���ȷ
      	    		 	 	 	 if (checkInputTime()==FALSE)
      	    		 	 	 	 {
      	    		 	 	 	   return;
      	    		 	 	 	 }
      	    		 	 	 }
      	    		 	 	 
      	    		 	 	 //��������ʾ�淶Ҫ��,��ȷ������ʵ�ֲ�ͬ��������Լ���л�
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]>=layer2MenuNum[layer1MenuLight]-1)
      	    		 	 	 {
      	    		 	 	   layer2MenuLight[layer1MenuLight]=0;
      	    		 	 	 }
      	    		 	 	 else
      	    		 	 	 {
      	    		 	 	   layer2MenuLight[layer1MenuLight]++;
      	    		 	 	 }
                     
                     if (layer2MenuLight[layer1MenuLight]!=0)
                     {
                       layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = meterDeviceNum/(NUM_MP_PER_PAGE-1);
                       if ((meterDeviceNum%(NUM_MP_PER_PAGE-1))!=0)
                       {
                     	   layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
                       }
                       layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                     }

      	    		 	 	 copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 
      	    		 	 	#endif
      	    		 	 	 break;
      	    		 	 	  	
      	    		 	case 1:    //����������鿴
      	    		 	 	switch(layer2MenuLight[1])
      	    		 	 	{
      	    		 	 	  case 0:  //ͨѶͨ������
                        strcpy(commParaItem[0],digitalToChar(addrField.a1[1]>>4));
                        strcat(commParaItem[0],digitalToChar(addrField.a1[1]&0xf));
                        strcat(commParaItem[0],digitalToChar(addrField.a1[0]>>4));
                        strcat(commParaItem[0],digitalToChar(addrField.a1[0]&0xf));
                       
                        for(i=0;i<4;i++)
                        {
                          if (strlen((char *)teApn[i])==0)
                          {
                            strcpy((char *)teApn[i],(char *)ipAndPort.apn);
                          }
                        }
                        strcpy(commParaItem[1],(char *)ipAndPort.apn);
                          
                        intToString(ipAndPort.port[1]<<8 | ipAndPort.port[0],3,strX);
                       	strcpy(commParaItem[2],intToIpadd(ipAndPort.ipAddr[0]<<24 | ipAndPort.ipAddr[1]<<16 | ipAndPort.ipAddr[2]<<8 | ipAndPort.ipAddr[3],str));
                        strcat(commParaItem[2],":");
                          
                        for(i=0;i<5-strlen(strX);i++)
                        {
                          strcat(commParaItem[2],"0");
                        }
                        strcat(commParaItem[2],strX);
                        intToString(ipAndPort.portBak[1]<<8 | ipAndPort.portBak[0],3,strX);
                       	strcpy(commParaItem[3],intToIpadd(ipAndPort.ipAddrBak[0]<<24 | ipAndPort.ipAddrBak[1]<<16 | ipAndPort.ipAddrBak[2]<<8 | ipAndPort.ipAddrBak[3],str));
                        strcat(commParaItem[3],":");
                          
                        for(i=0;i<5-strlen(strX);i++)
                        {
                          strcat(commParaItem[3],"0");
                        }
                        strcat(commParaItem[3],strX);
                          
                        keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	  	break;
      	    		 	 	  	
      	    		 	 	  case 1:  //̨������������
                       #ifndef LIGHTING 
                        strcpy(chrMp[0],"0001");
                        strcpy(chrMp[1],"0");
                        strcpy(chrMp[2],"1");
                        strcpy(chrMp[3],"0");
                        strcpy(chrMp[4],"000000000001");
                        keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  set485Meter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	 	 #endif
      	    		 	 	  	break;
      	    		 	 	  	
      	    		 	 	  case 2:  //��������������
                        strcpy(chrMp[0],"0001");
                        strcpy(chrMp[1],"000000000001");
                        strcpy(chrMp[2],"1");
                        strcpy(chrMp[3],"000000000000");
                        strcpy(chrMp[4],"0");
                        keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  setCarrierMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);      	    		 	 	  	
      	    		 	 	  	break;
      	    		 	 	  	
      	    		 	 	 	case 3:  //�ն�ʱ������
      	    		 	 	 		keyLeftRight = 0;
      	    		 	 	 		strcpy(dateTimeItem,"20");
                        digital2ToString(sysTime.year,str);
                        dateTimeItem[2] = str[0];
                        dateTimeItem[3] = str[1];
                        digital2ToString(sysTime.month,str);
                        dateTimeItem[4] = str[0];
                        dateTimeItem[5] = str[1];
                        digital2ToString(sysTime.day,str);
                        dateTimeItem[6] = str[0];
                        dateTimeItem[7] = str[1];
                        digital2ToString(sysTime.hour,str);
                        dateTimeItem[8] = str[0];
                        dateTimeItem[9] = str[1];
                        digital2ToString(sysTime.minute,str);
                        dateTimeItem[10] = str[0];
                        dateTimeItem[11] = str[1];
                        digital2ToString(sysTime.second,str);
                        dateTimeItem[12] = str[0];
                        dateTimeItem[13] = str[1];
                        dateTimeItem[14] = '\0';
      	    		 	 	 		setTeDateTime(keyLeftRight);
      	    		 	 	 		break;
      	    		 	 	 		
      	    		 	 	 	case 4:  //�޸Ľ�������
      	    		 	 	 	  keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                        strcpy(commParaItem[0],"000000");
                        strcpy(commParaItem[1],"000000");
      	    		 	 	 	  modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	 	  break;
      	    		 	 	  	     
      	    	 	 	  	case 5:  //�ն˱��
      	    	 	 	  		keyLeftRight = 0;
    	  	 	 	  	 	 	 	#ifdef TE_ADDR_USE_BCD_CODE
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a2[1]>>4);
                         tmpTeAddr[0] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a2[1]&0xf);
                         tmpTeAddr[1] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a2[0]>>4);
                         tmpTeAddr[2] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a2[0]&0xf);
                         tmpTeAddr[3] = *tmpChar;          	  	 	 	  	 	 	 	 
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a1[1]>>4);
                         tmpTeAddr[4] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a1[1]&0xf);
                         tmpTeAddr[5] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a1[0]>>4);
                         tmpTeAddr[6] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a1[0]&0xf);
                         tmpTeAddr[7] = *tmpChar;
                         tmpTeAddr[8] = 0x0;
    	  	 	 	  	 	 	 	 modifyTeAddr(0);
    	  	 	 	  	 	 	 	#else
    	  	 	 	  	 	 	 	 tmpAddr = addrField.a2[1]<<8 | addrField.a2[0];
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(tmpAddr/10000);
    	  	 	 	  	 	 	 	 tmpTeAddr[0] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpAddr %= 10000;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(tmpAddr/1000);
    	  	 	 	  	 	 	 	 tmpTeAddr[1] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpAddr %= 1000;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(tmpAddr/100);
    	  	 	 	  	 	 	 	 tmpTeAddr[2] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpAddr %= 100;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(tmpAddr/10);
    	  	 	 	  	 	 	 	 tmpTeAddr[3] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpAddr %= 10;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(tmpAddr);
    	  	 	 	  	 	 	 	 tmpTeAddr[4] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a1[1]>>4);
                         tmpTeAddr[5] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a1[1]&0xf);
                         tmpTeAddr[6] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a1[0]>>4);
                         tmpTeAddr[7] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a1[0]&0xf);
                         tmpTeAddr[8] = *tmpChar;
                         tmpTeAddr[9] = 0x0;
    	  	 	 	  	 	 	 	#endif
    	  	 	 	  	 	 	 	
      	    	 	 	  		setTeAddr(keyLeftRight);
      	    	 	 	  		break;
      	    	 	 	  		
      	    	 	 	  	case 6:  //��̫����������
                        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
                        {
                          ifc.ifc_len = sizeof buf;
                          ifc.ifc_buf = (caddr_t) buf;
                          if (!ioctl(fd, SIOCGIFCONF, (char *) &ifc))
                          {
                            interface = ifc.ifc_len / sizeof(struct ifreq);
                            while (interface-- > 0)
                            {
                              if (strstr(buf[interface].ifr_name,"eth0"))
                              {
                               /*Get HW ADDRESS of the net card */
                                if (!(ioctl(fd, SIOCGIFHWADDR, (char *) &buf[interface]))) 
                                {
                                    sprintf(tmpEthMac, "%02x%02x%02x%02x%02x%02x",
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[0],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[1],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[2],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[3],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[4],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[5]); // ����sprintfת����char *
                                }
                              }
                            }//end of while
                          }
                          else
                          {
                            perror("cpm: ioctl");
                          }
                        }
                        else
                        {
                          perror("cpm: socket");
                        }
                     
                        close(fd);
                       	
                       	strcpy(chrEthPara[0],intToIpadd(teIpAndPort.teIpAddr[0]<<24 | teIpAndPort.teIpAddr[1]<<16 | teIpAndPort.teIpAddr[2]<<8 | teIpAndPort.teIpAddr[3],str));
                       	strcpy(chrEthPara[1],intToIpadd(teIpAndPort.mask[0]<<24 | teIpAndPort.mask[1]<<16 | teIpAndPort.mask[2]<<8 | teIpAndPort.mask[3],str));
                       	strcpy(chrEthPara[2],intToIpadd(teIpAndPort.gateWay[0]<<24 | teIpAndPort.gateWay[1]<<16 | teIpAndPort.gateWay[2]<<8 | teIpAndPort.gateWay[3],str));
                        chrEthPara[3][0] = teIpAndPort.ethIfLoginMs;
                        keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    	 	 	  		setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		break;
      	    	 	 	  		
                      case 7:  //����ר���û�������
                        strcpy(tmpVpnUserName, (char *)vpn.vpnName);                        
                        tmpVpnUserName[32] = '\0';
                        trim(tmpVpnUserName);
                        
     	 	 	  		 	 	    for(i=strlen(tmpVpnUserName); i<32; i++)
     	 	 	  		 	 	    {
     	 	 	  		 	 	    	tmpVpnUserName[i] = ' ';
     	 	 	  		 	 	    }
     	 	 	  		 	 	    tmpVpnUserName[32] = '\0';

                        strcpy(tmpVpnPw, (char *)vpn.vpnPassword);
                        tmpVpnPw[32] = '\0';
                        trim(tmpVpnPw);
     	 	 	  		 	 	    for(i=strlen(tmpVpnPw); i<32; i++)
     	 	 	  		 	 	    {
     	 	 	  		 	 	    	tmpVpnPw[i] = ' ';
     	 	 	  		 	 	    }
     	 	 	  		 	 	    tmpVpnPw[32] = '\0';
     	 	 	  		 	 	    keyLeftRight = 0;
                        inputStatus = STATUS_NONE;
                        setVpn(keyLeftRight);
                        break;
                        
                      case 8:    //�ն˼�����������
                        sprintf(chrMp[0], "%01d", cascadeCommPara.commPort);    //�����˿�
                        if (cascadeCommPara.flagAndTeNumber&0x80)
                        {
                          strcpy(chrMp[1],"2");    //������
                        }
                        else
                        {
                          strcpy(chrMp[1],"1");    //������
                        }
                        switch (cascadeCommPara.flagAndTeNumber&0xf)
                        {
                        	case 1:
                           sprintf(chrMp[2],"%02x%02x%05d",cascadeCommPara.divisionCode[1],cascadeCommPara.divisionCode[0],cascadeCommPara.cascadeTeAddr[0]|cascadeCommPara.cascadeTeAddr[1]<<8);      //�ն�1
                           strcpy(chrMp[3],"000000000");
                           strcpy(chrMp[4],"000000000");
                           break;
                        	case 2:
                           sprintf(chrMp[2],"%02x%02x%05d",cascadeCommPara.divisionCode[1],cascadeCommPara.divisionCode[0],cascadeCommPara.cascadeTeAddr[0]|cascadeCommPara.cascadeTeAddr[1]<<8);      //�ն�1
                           sprintf(chrMp[3],"%02x%02x%05d",cascadeCommPara.divisionCode[3],cascadeCommPara.divisionCode[2],cascadeCommPara.cascadeTeAddr[2]|cascadeCommPara.cascadeTeAddr[3]<<8);      //�ն�1
                           strcpy(chrMp[4],"000000000");
                           break;
                        	case 3:
                           sprintf(chrMp[2],"%02x%02x%05d",cascadeCommPara.divisionCode[1],cascadeCommPara.divisionCode[0],cascadeCommPara.cascadeTeAddr[0]|cascadeCommPara.cascadeTeAddr[1]<<8);      //�ն�1
                           sprintf(chrMp[3],"%02x%02x%05d",cascadeCommPara.divisionCode[3],cascadeCommPara.divisionCode[2],cascadeCommPara.cascadeTeAddr[2]|cascadeCommPara.cascadeTeAddr[3]<<8);      //�ն�1
                           sprintf(chrMp[4],"%02x%02x%05d",cascadeCommPara.divisionCode[5],cascadeCommPara.divisionCode[4],cascadeCommPara.cascadeTeAddr[4]|cascadeCommPara.cascadeTeAddr[5]<<8);      //�ն�1
                           break;
                           
                        	default:
                           strcpy(chrMp[2],"000000000");
                           strcpy(chrMp[3],"000000000");
                           strcpy(chrMp[4],"000000000");
                           
                           strcpy(chrMp[1],"0");      //������
                           break;
                        }
                        
                        keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
                      	break;

      	    		 	 	  case 9:    //���ģ���������
                        sprintf(chrRlPara[0],"%02d",rlPara[0]);
                        sprintf(chrRlPara[1],"%02d",rlPara[1]);
                        sprintf(chrRlPara[2],"%02d",rlPara[2]);
                        sprintf(chrRlPara[3],"%02d",rlPara[3]);
                        keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  setRlPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	  	break;
      	    		 	 	}
      	    		 	 	break;

      	    		 	case 2:    //�ն˹�����ά��
      	    		 	 	switch(layer2MenuLight[layer1MenuLight])   //2��˵�����
      	    		 	 	{
      	    		 	 	 	  case 0:  //ʵʱ����
      	    		 	 	 	   #ifdef LIGHTING    //·�Ƽ�������ָ�������㳭��
    	    	 	 	  		 	  if (carrierModuleType==NO_CARRIER_MODULE || ((carrierModuleType==RL_WIRELESS || carrierModuleType==SR_WIRELESS)  && carrierFlagSet.wlNetOk<3))
      	    		 	 	 	    {
                            guiLine(10,55,150,105,0);
                            guiLine(10,55,10,105,1);
                            guiLine(150,55,150,105,1);
                            guiLine(10,55,150,55,1);
                            guiLine(10,105,150,105,1);
                            if (carrierModuleType==NO_CARRIER_MODULE)
                            {
                              guiDisplay(15,60,"  �ȴ�ʶ��ģ��!",1);
                            }
                            else
                            {
                              guiDisplay(15,60,"�ȴ�ģ���������!",1);
                            }
                            guiDisplay(15,80,"     ���Ժ�   ",1);
                            lcdRefresh(10,120);
      	    		 	 	 	    }
      	    		 	 	 	    else
      	    		 	 	 	    {
      	    	 	 	  		 	  if (0<carrierFlagSet.broadCast)
      	    	 	 	  		 	  {
                              guiLine(10,55,150,105,0);
                              guiLine(10,55,10,105,1);
                              guiLine(150,55,150,105,1);
                              guiLine(10,55,150,55,1);
                              guiLine(10,105,150,105,1);
                              guiDisplay(15,60,"�㲥�������!",1);
                              guiDisplay(15,80,"     ���Ժ�   ",1);
                              lcdRefresh(10,120);
      	    	 	 	  		 	  }
      	    	 	 	  		 	  else
      	    	 	 	  		 	  {
      	    	 	 	  		 	    keyLeftRight = 0;
      	    	 	 	  		 	    strcpy(singleCopyTime,"-----");
             	 	              strcpy(singleCopyEnergy,"-----");
             	 	            
      	    	 	 	  		 	    singleCcbCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
      	    	 	 	  		 	  }
      	    	 	 	  		 	}
      	    	 	 	  		 	
      	    		 	 	 	   #else
      	    		 	 	 	   	
      	    		 	 	 	   	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  	realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 	   
      	    		 	 	 	   #endif
      	    		 	 	 	  	break;
      	    		 	 	 	  	
      	    		 	 	 	  case 1: //ȫ�������㳭����
      	    		 	 	 	   #ifdef LIGHTING    //·�Ƽ����������Ǳ�����Ϣ
      	    		 	 	 	  	
      	    		 	 	 	  	layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 2;
      	    		 	 	 	  	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    	 	 	  			terminalInfo(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    	 	 	  			
      	    		 	 	 	   #else
      	    		 	 	 	  	
      	    		 	 	 	  	allMpCopy(0xff);
      	    		 	 	 	   
      	    		 	 	 	   #endif
      	    		 	 	 	  	break;
      	    		 	 	 	  	
      	    		 	 	 	  case 2:  //�������
      	    		 	 	 	   #ifdef LIGHTING    //·�Ƽ�������ѡ���ǵ���Һ���ȶԶ�
      	    		 	 	 	    
      	    		 	 	 	    setLcdDegree(lcdDegree);
      	    		 	 	 	     
      	    		 	 	 	   #else
      	    		 	 	 	   
      	    		 	 	 	  	if (carrierModuleType==SR_WIRELESS || carrierModuleType==HWWD_WIRELESS || carrierModuleType==RL_WIRELESS || carrierModuleType==NO_CARRIER_MODULE)
      	    		 	 	 	  	{
                             guiLine(10,55,150,105,0);
                             guiLine(10,55,10,105,1);
                             guiLine(150,55,150,105,1);
                             guiLine(10,55,150,55,1);
                             guiLine(10,105,150,105,1);
                             if (carrierModuleType==NO_CARRIER_MODULE)
                             {
                               guiDisplay(15,60,"δʶ����ͨ��ģ��!",1);
                               guiDisplay(15,80,"     �޷��ѱ�!   ",1);
                             }
                             else
                             {
                               guiDisplay(35,60,"����������!",1);
                               guiDisplay(35,80," �����ѱ�! ",1);
                             }
                             lcdRefresh(10,120);
      	    		 	 	 	  	}
      	    		 	 	 	  	else
      	    		 	 	 	  	{
        	    		 	 	 	  	menuInLayer = 3;
        	    		 	 	 	  	if (carrierFlagSet.searchMeter==0)
        	    		 	 	 	  	{
        	    		 	 	 	  	  keyLeftRight = 1;
        	    		 	 	 	  	  searchMeter(keyLeftRight);
        	    		 	 	 	  	}
        	    		 	 	 	  	else
        	    		 	 	 	  	{
        	    		 	 	 	  	  keyLeftRight = 0xff;
        	    		 	 	 	  	  searchMeter(keyLeftRight);
        	    		 	 	 	  	}
        	    		 	 	 	  }
        	    		 	 	 	  
        	    		 	 	 	 #endif
      	    		 	 	 	  	break;
      	    		 	 	 	  	
      	    		 	 	 	  case 3:  //�������ܱ��ַ
      	    	 	 	  		 #ifdef LIGHTING    //·�Ƽ�����������"��λ������"
      	    	 	 	  		  
      	    	 	 	  		  reboot(RB_AUTOBOOT);

      	    	 	 	  		 #else
      	    	 	 	  		 	keyLeftRight = 1;
      	    	 	 	  		 	multiCpUpDown = 0;
      	    	 	 	  		 	newAddMeter(keyLeftRight);
      	    	 	 	  		 #endif
      	    	 	 	  		 	break;
      	    	 	 	  		 	
      	    	 	 	  		case 4:  //�������ܱ���״̬
      	    	 	 	  		 #ifdef LIGHTING    //·�Ƽ����������ǡ����������
      	    	 	 	  		  
      	    	 	 	  		  uDiskUpgrade();

      	    	 	 	  		 #else
      	    	 	 	  		 
      	    	 	 	  		 	menuInLayer++;

      	    	 	 	  			multiCpUpDown = 0;
      	    	 	 	  			newMeterCpStatus(0);
      	    	 	 	  			
      	    	 	 	  		 #endif
      	    	 	 	  			break;
      	    	 	 	  		 	
      	    	 	 	  		case 5:  //������Ϣ
        								 #ifdef LIGHTING
												 	keyLeftRight = 0;
													setLearnIr(keyLeftRight);
												 #else
													if (ifHasAcModule==TRUE)
                          {
      	    		 	 	 	  	  layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 18;
      	    		 	 	 	  	}
      	    		 	 	 	  	else
      	    		 	 	 	  	{
      	    		 	 	 	  	  layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 4;
      	    		 	 	 	  	}
      	    		 	 	 	  	
      	    		 	 	 	  	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    	 	 	  			terminalInfo(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
												 #endif
      	    	 	 	  			break;
      	    	 	 	  			
      	    	 	 	  	  case 6:  //����LCD�Աȶ�
      	    	 	 	  	  	setLcdDegree(lcdDegree);
      	    	 	 	  	  	break;
      	    	 	 	  	  
      	    	 	 	  	  case 7:  //����ͨ��ģ�鳭����ʽ
                      	  if (carrierModuleType==EAST_SOFT_CARRIER
                      	   	   || carrierModuleType==TC_CARRIER
                      	   	     || carrierModuleType==MIA_CARRIER
                      	   	 )
                      	  {
        	    	 	 	  	  	if (localCopyForm==0xaa)
        	    	 	 	  	  	{
        	    	 	 	  	  	  keyLeftRight = 1;
        	    	 	 	  	  	}
        	    	 	 	  	  	else
        	    	 	 	  	  	{
        	    	 	 	  	  		keyLeftRight = 0;
        	    	 	 	  	  	}
        	    	 	 	  	  	setCopyForm(keyLeftRight);
        	    	 	 	  	  }
        	    	 	 	  	  else
        	    	 	 	  	  {
                            guiLine(1,55,150,105,0);
                            guiLine(1,55,1,105,1);
                            guiLine(160,55,160,105,1);
                            guiLine(1,55,160,55,1);
                            guiLine(1,105,160,105,1);
                            
                            if (carrierModuleType==NO_CARRIER_MODULE)
	   	                      {
	   	                      	guiDisplay(1, 70, "δ��⵽����ͨ��ģ��", 1);
	   	                      }
	   	                      else
	   	                      {
                              guiDisplay(17, 60, "�����ͱ���ͨ��ģ��",1);
                              guiDisplay(1, 80, "�������ó�����ʽ!",1);
                            }
                            
                            lcdRefresh(55,110);
                          }
      	    	 	 	  	  	break;
      	    	 	 	  			
      	    	 	 	  		case 8:     //��λ�ն�
      	    	 	 	  			reboot(RB_AUTOBOOT);
      	    	 	 	  			break;
      	    	 	 	  	  
      	    	 	 	  	  case 9:     //�ն˳�������
      	    	 	 	  	  	uDiskUpgrade();
      	    	 	 	  	  	break;
      	    	 	 	  	  
      	    	 	 	  	  case 10:    //����·�ɳ���
      	    	 	 	  	  	upRtFlag = 1;
      	    	 	 	  	  	break;
      	    	 	 	  	  
      	    	 	 	  	  case 11:    //�����û�����������
        	    	 	 	  	  switch (denizenDataType)
        	    	 	 	  	  {
        	    	 	 	  	  	case 0x55:
        	    	 	 	  	  		keyLeftRight = 1;
        	    	 	 	  	  	  break;
        	    	 	 	  	  	  
        	    	 	 	  	  	case 0xaa:
        	    	 	 	  	  	  keyLeftRight = 2;
        	    	 	 	  	  	  break;
        	    	 	 	  	  
        	    	 	 	  	  	default:
        	    	 	 	  	  		keyLeftRight = 0;
        	    	 	 	  	  		break;
        	    	 	 	  	  }
                          setDenizenDataType(keyLeftRight);
      	    	 	 	  	  	break;
      	    	 	 	  	  
      	    	 	 	  	  case 12:    //��������
      	    	 	 	  	  	if (cycleDataType==0x55)
      	    	 	 	  	  	{
      	    	 	 	  	  	  keyLeftRight = 1;
      	    	 	 	  	  	}
      	    	 	 	  	  	else
      	    	 	 	  	  	{
      	    	 	 	  	  	  keyLeftRight = 0;
      	    	 	 	  	  	}
      	    	 	 	  	  	setCycleType(keyLeftRight);
      	    	 	 	  	  	break;
      	    	 	 	  	  
      	    	 	 	  	  case 13:    //ά���ӿ�ģʽ����
      	    	 	 	  	  	if (mainTainPortMode==0x55)
      	    	 	 	  	  	{
      	    	 	 	  	  	  keyLeftRight = 1;
      	    	 	 	  	  	}
      	    	 	 	  	  	else
      	    	 	 	  	  	{
      	    	 	 	  	  	  keyLeftRight = 0;
      	    	 	 	  	  	}
      	    	 	 	  	  	setMainTain(keyLeftRight);
      	    	 	 	  	  	break;

      	    	 	 	  	  case 14:    //��2·485�ڹ�������
      	    	 	 	  	  	if (rs485Port2Fun==0x55)
      	    	 	 	  	  	{
      	    	 	 	  	  	  keyLeftRight = 1;
      	    	 	 	  	  	}
      	    	 	 	  	  	else
      	    	 	 	  	  	{
      	    	 	 	  	  	  keyLeftRight = 0;
      	    	 	 	  	  	}
      	    	 	 	  	  	setRs485Port2(keyLeftRight);
      	    	 	 	  	  	break;
      	    	 	 	  	  
      	    	 	 	  	  case 15:    //����ͨ��ģ��Э������
      	    	 	 	  	  	if (lmProtocol==0x55)
      	    	 	 	  	  	{
      	    	 	 	  	  	  keyLeftRight = 1;
      	    	 	 	  	  	}
      	    	 	 	  	  	else
      	    	 	 	  	  	{
      	    	 	 	  	  	  keyLeftRight = 0;
      	    	 	 	  	  	}
      	    	 	 	  	  	setLmProtocol(keyLeftRight);
      	    	 	 	  	  	break;
      	    		 	 	}
      	    		 	 	break;
										
									case 3:    //��·���Ƶ�״̬
									  keyLeftRight=0;
										xlOpenClose(keyLeftRight);
									  break;
      	    		}
      	    		break;
      	    		
      	    	case 3:   //�˵���3��
      	    	 	switch(layer1MenuLight)
      	    	 	{
      	    	 	 	case 1:    //����������鿴
      	    	 	 	  switch(layer2MenuLight[layer1MenuLight])
      	    	 	 	  {
      	    	 	 	  	case 0:  //ͨ��ͨ������
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==4)
      	    		 	 	    {
      	    		 	 	       //ȷ���޸Ĳ���
      	    		 	 	       keyLeftRight = 0xff;
      	    		 	 	       commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	       layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0xff;
      	    		 	 	          
      	    		 	 	       return;
      	    		 	 	    }

      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] == 0xff)
      	    		 	 	    {
        	  	 	 	      	//��IP��ַ
        	  	 	 	      	ipAndPort.ipAddr[0] =(commParaItem[2][0]-0x30)*100+(commParaItem[2][1]-0x30)*10+(commParaItem[2][2]-0x30);
        	  	 	 	      	ipAndPort.ipAddr[1] =(commParaItem[2][4]-0x30)*100+(commParaItem[2][5]-0x30)*10+(commParaItem[2][6]-0x30);
        	  	 	 	      	ipAndPort.ipAddr[2] =(commParaItem[2][8]-0x30)*100+(commParaItem[2][9]-0x30)*10+(commParaItem[2][10]-0x30);
        	  	 	 	      	ipAndPort.ipAddr[3] =(commParaItem[2][12]-0x30)*100+(commParaItem[2][13]-0x30)*10+(commParaItem[2][14]-0x30);
        	  	 	 	      	  	
        	  	 	 	      	//���˿�
        	  	 	 	      	tmpData = (commParaItem[2][16]-0x30)*10000+(commParaItem[2][17]-0x30)*1000
        	  	 	 	      	         +(commParaItem[2][18]-0x30)*100+(commParaItem[2][19]-0x30)*10
        	  	 	 	      	         +(commParaItem[2][20]-0x30);
        	  	 	 	      	ipAndPort.port[1] = tmpData>>8;
        	  	 	 	      	ipAndPort.port[0] = tmpData&0xff;

        	  	 	 	      	//���õ�ַ
        	  	 	 	      	ipAndPort.ipAddrBak[0] =(commParaItem[3][0]-0x30)*100+(commParaItem[3][1]-0x30)*10+(commParaItem[3][2]-0x30);
        	  	 	 	      	ipAndPort.ipAddrBak[1] =(commParaItem[3][4]-0x30)*100+(commParaItem[3][5]-0x30)*10+(commParaItem[3][6]-0x30);
        	  	 	 	      	ipAndPort.ipAddrBak[2] =(commParaItem[3][8]-0x30)*100+(commParaItem[3][9]-0x30)*10+(commParaItem[3][10]-0x30);
        	  	 	 	      	ipAndPort.ipAddrBak[3] =(commParaItem[3][12]-0x30)*100+(commParaItem[3][13]-0x30)*10+(commParaItem[3][14]-0x30);
        	  	 	 	      	  	
        	  	 	 	      	//���ö˿�
        	  	 	 	      	tmpData = (commParaItem[3][16]-0x30)*10000+(commParaItem[3][17]-0x30)*1000
        	  	 	 	      	         +(commParaItem[3][18]-0x30)*100+(commParaItem[3][19]-0x30)*10
        	  	 	 	      	         +(commParaItem[3][20]-0x30);
        	  	 	 	      	ipAndPort.portBak[1] = tmpData>>8;
        	  	 	 	      	ipAndPort.portBak[0] = tmpData&0xff;
        	  	 	 	      	  	        	  	 	 	      	  	
        	  	 	 	      	strcpy((char *)ipAndPort.apn, commParaItem[1]);

        	  	 	 	      	//����IP��ַ
        	  	 	 	      	saveParameter(0x04, 3,(INT8U *)&ipAndPort,sizeof(IP_AND_PORT));
                        	
                        	saveBakKeyPara(3);    //2012-8-9,add

                 	  	 	 	addrField.a1[1] = (commParaItem[0][0]-0x30)<<4 | (commParaItem[0][1]-0x30);
                 	  	 	 	addrField.a1[0] = (commParaItem[0][2]-0x30)<<4 | (commParaItem[0][3]-0x30);
                              
                          //��������������
                          saveParameter(0x04, 121,(INT8U *)&addrField,4);
                        	saveBakKeyPara(121);    //2012-8-9,add
                              
                          guiLine(10,55,150,105,0);
                          guiLine(10,55,10,105,1);
                          guiLine(150,55,150,105,1);
                          guiLine(10,55,150,55,1);
                          guiLine(10,105,150,105,1);
                          guiDisplay(12,70,"�޸�ͨ�Ų����ɹ�!",1);
                          lcdRefresh(10,120);
                              
                          menuInLayer--;
                          layer2MenuLight[layer1MenuLight]=0xff;
                          
                          //2012-08-02,add,�޸�IP����������IP��¼
                          if (bakModuleType==MODEM_PPP)
                          {
                          	resetPppWlStatus();
                          }
                          else
                          {
                            wlModemPowerOnOff(0);
                          }

                          return;
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
      	    		 	 	    {
      	    		 	 	    	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	    }
      	    		 	 	    else
      	    		 	 	    {
      	    		 	 	      layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	    }

      	    		 	 	    keyLeftRight = 0;
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
      	    		 	 	    {
      	    		 	 	      keyLeftRight = 0x88;
      	    		 	 	      for(i=0;i<4;i++)
      	    		 	 	      {
      	    		 	 	       	if (strcmp((char *)teApn[i],commParaItem[1])==0)
      	    		 	 	       	{
      	    		 	 	       	  keyLeftRight = i;
      	    		 	 	       	  break;
      	    		 	 	       	}
      	    		 	 	      }
      	    		 	 	    }
      	    		 	 	    commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		break;
      	    	 	 	  		
      	    	 	 	  	case 1:  //̨������������
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==5)
      	    		 	 	    {
        	  	 	 	      	//�������
        	  	 	 	      	meterConfig.measurePoint = (chrMp[0][0]-0x30)*1000+(chrMp[0][1]-0x30)*100+(chrMp[0][2]-0x30)*10+(chrMp[0][3]-0x30);
        	  	 	 	      	
        	  	 	 	      	//���
        	  	 	 	      	meterConfig.number = meterConfig.measurePoint;

        	  	 	 	      	//�˿ں�����
        	  	 	 	      	meterConfig.rateAndPort = (chrMp[1][0]-0x30)<<5;
        	  	 	 	      	switch(chrMp[2][0])
        	  	 	 	      	{
        	  	 	 	      		case 0x30:   //���ɶ˿�Ϊ1
        	  	 	 	      		 	meterConfig.rateAndPort |= 0x1;
        	  	 	 	      		 	break;

        	  	 	 	      		case 0x31:   //RS485-1�˿�Ϊ2
        	  	 	 	      		 	meterConfig.rateAndPort |= 0x2;
        	  	 	 	      		 	break;

        	  	 	 	      		case 0x32:   //RS485-2�˿�Ϊ3
        	  	 	 	      		 	meterConfig.rateAndPort |= 0x3;
        	  	 	 	      		 	break;
        	  	 	 	      	}
        	  	 	 	      	
        	  	 	 	      	//Э��
        	  	 	 	      	switch(chrMp[3][0])
        	  	 	 	        {
        	  	 	 	        	 case 0x32:
        	  	 	 	        	 	 meterConfig.protocol = 02;
        	  	 	 	        	 	 break;

        	  	 	 	        	 case 0x31:
        	  	 	 	        	 	 meterConfig.protocol = 30;
        	  	 	 	        	 	 break;
        	  	 	 	        	 	 
        	  	 	 	        	 default:
        	  	 	 	        	 	 meterConfig.protocol = 1;
        	  	 	 	        	 	 break;
        	  	 	 	        }
        	  	 	 	        
     	  	 	 	      	  	//����ַ
     	  	 	 	      	  	meterConfig.addr[5] = (chrMp[4][0]-0x30)<<4 | (chrMp[4][1]-0x30);
     	  	 	 	      	  	meterConfig.addr[4] = (chrMp[4][2]-0x30)<<4 | (chrMp[4][3]-0x30);
     	  	 	 	      	  	meterConfig.addr[3] = (chrMp[4][4]-0x30)<<4 | (chrMp[4][5]-0x30);
     	  	 	 	      	  	meterConfig.addr[2] = (chrMp[4][6]-0x30)<<4 | (chrMp[4][7]-0x30);
     	  	 	 	      	  	meterConfig.addr[1] = (chrMp[4][8]-0x30)<<4 | (chrMp[4][9]-0x30);
     	  	 	 	      	  	meterConfig.addr[0] = (chrMp[4][10]-0x30)<<4 | (chrMp[4][11]-0x30);
     	  	 	 	      	  	
     	  	 	 	      	  	//�ɼ�����ַ
     	  	 	 	      	  	for(i=0;i<6;i++)
     	  	 	 	      	    {
     	  	 	 	      	  	  meterConfig.collectorAddr[i] = 0x0;
     	  	 	 	      	  	}
     	  	 	 	      	  	
     	  	 	 	      	  	//����λ��С��λ����
     	  	 	 	      	  	meterConfig.mixed = 0x05;
     	  	 	 	      	  	
     	  	 	 	      	  	//���ʸ���
     	  	 	 	      	  	meterConfig.numOfTariff = 4;
     	  	 	 	      	  	
     	  	 	 	      	  	//����ż�С���
     	  	 	 	      	  	meterConfig.bigAndLittleType = 0x0;
                              
                          //����
  		                    saveDataF10(meterConfig.measurePoint, meterConfig.rateAndPort&0x1f, meterConfig.addr, meterConfig.number, (INT8U *)&meterConfig, 27);
                              
                          guiLine(10,55,150,105,0);
                          guiLine(10,55,10,105,1);
                          guiLine(150,55,150,105,1);
                          guiLine(10,55,150,55,1);
                          guiLine(10,105,150,105,1);
                          guiDisplay(12,70,"̨��������óɹ�!",1);
                          lcdRefresh(10,120);
                              
                          menuInLayer--;
                          layer2MenuLight[layer1MenuLight]=0xff;

                          return;
      	    		 	 	    }
      	    		 	 	       
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
      	    		 	 	    {
      	    		 	 	    	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	    }
      	    		 	 	    else
      	    		 	 	    {
      	    		 	 	      layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    //�л�������ʱ�������еĲ�������Ϣ
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
      	    		 	 	    {
      	    		 	 	    	 tmpData = (chrMp[0][0]-0x30)*1000 + (chrMp[0][1]-0x30)*100 + (chrMp[0][2]-0x30)*10 + (chrMp[0][3]-0x30);
      	    		 	 	    	 if (tmpData>64 || tmpData<1)
      	    		 	 	    	 {
                              guiLine(10,55,150,105,0);
                              guiLine(10,55,10,105,1);
                              guiLine(150,55,150,105,1);
                              guiLine(10,55,150,55,1);
                              guiLine(10,105,150,105,1);
                              guiDisplay(20,70,"�������������!",1);
                              lcdRefresh(10,120);
      	    		 	 	    	 	  
      	    		 	 	    	 	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
      	    		 	 	    	 	  return;
      	    		 	 	    	 }
                           
                           if (selectF10Data(tmpData, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
                           {
                           	  //����
                           	  chrMp[1][0] = 0x30+(meterConfig.rateAndPort>>5);
                           	  
                           	  //�˿�
                           	  switch(meterConfig.rateAndPort&0x1f)
                           	  {
                           	  	 case 1:
                           	  	 	 chrMp[2][0] = 0x30;
                           	  	 	 break;
                           	  	 	 
                           	  	 case 2:
                           	  	 	 chrMp[2][0] = 0x31;
                           	  	 	 break;

                           	  	 case 3:
                           	  	 	 chrMp[2][0] = 0x32;
                           	  	 	 break;
                           	  }
                           	  
                           	  switch(meterConfig.protocol)
                           	  {
                           	  	 case 2:
                           	  	 	 chrMp[3][0] = 0x32;
                           	  	 	 break;
                           	  	 	 
                           	  	 case 30:
                           	  	 	 chrMp[3][0] = 0x31;
                           	  	 	 break;
                           	  	 	 
                           	  	 default:
                           	  	 	 chrMp[3][0] = 0x30;
                           	  	 	 break;
                           	  }
                           	  
                           	  //���ַ
                  	 	 	   	  chrMp[4][0]  = (meterConfig.addr[5]>>4)+0x30;
                  	 	 	   	  chrMp[4][1]  = (meterConfig.addr[5]&0xf)+0x30;
                  	 	 	   	  chrMp[4][2]  = (meterConfig.addr[4]>>4)+0x30;
                  	 	 	   	  chrMp[4][3]  = (meterConfig.addr[4]&0xf)+0x30;
                  	 	 	   	  chrMp[4][4]  = (meterConfig.addr[3]>>4)+0x30;
                  	 	 	   	  chrMp[4][5]  = (meterConfig.addr[3]&0xf)+0x30;
                  	 	 	   	  chrMp[4][6]  = (meterConfig.addr[2]>>4)+0x30;
                  	 	 	   	  chrMp[4][7]  = (meterConfig.addr[2]&0xf)+0x30;
                  	 	 	   	  chrMp[4][8]  = (meterConfig.addr[1]>>4)+0x30;
                  	 	 	   	  chrMp[4][9]  = (meterConfig.addr[1]&0xf)+0x30;
                  	 	 	   	  chrMp[4][10] = (meterConfig.addr[0]>>4)+0x30;
                  	 	 	   	  chrMp[4][11] = (meterConfig.addr[0]&0xf)+0x30;
                  	 	 	   	  chrMp[4][12] = '\0';
                           }
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    keyLeftRight = 0;
      	    		 	 	    set485Meter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		break;
      	    	 	 	  		
      	    	 	 	  	case 2:  //��������������
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==5)
      	    		 	 	    {
        	  	 	 	      	//�������
        	  	 	 	      	meterConfig.measurePoint = (chrMp[0][0]-0x30)*1000+(chrMp[0][1]-0x30)*100+(chrMp[0][2]-0x30)*10+(chrMp[0][3]-0x30);
        	  	 	 	      	
        	  	 	 	      	//���
        	  	 	 	      	meterConfig.number = meterConfig.measurePoint;

        	  	 	 	      	
        	  	 	 	      	//Э��
        	  	 	 	       #ifdef LIGHTING
        	  	 	 	      	//�˿ں�����
        	  	 	 	      	switch(chrMp[3][0])
        	  	 	 	        {
        	  	 	 	        	case 0x31:
        	  	 	 	      	    meterConfig.rateAndPort = 0x02;
        	  	 	 	      	    break;

        	  	 	 	        	case 0x32:
        	  	 	 	      	    meterConfig.rateAndPort = 0x03;
        	  	 	 	      	    break;
        	  	 	 	      	    
        	  	 	 	        	default:
        	  	 	 	      	    meterConfig.rateAndPort = 31;
        	  	 	 	      	    break;
        	  	 	 	        }
        	  	 	 	       
        	  	 	 	        switch(chrMp[2][0])
        	  	 	 	        {
        	  	 	 	          default:
        	  	 	 	            meterConfig.protocol = 30;
        	  	 	 	            break;

        	  	 	 	          case 0x32:
        	  	 	 	            meterConfig.protocol = 0x2;
        	  	 	 	            break;
        	  	 	 	            
        	  	 	 	          case 0x33:
        	  	 	 	            meterConfig.protocol = 130;
        	  	 	 	            break;
        	  	 	 	        }
        	  	 	 	       
        	  	 	 	       #else	
        	  	 	 	      	//�˿ں�����
        	  	 	 	      	meterConfig.rateAndPort = 0x1f;

        	  	 	 	      	switch(chrMp[2][0])
        	  	 	 	        {
        	  	 	 	        	case 0x31:
        	  	 	 	        	  meterConfig.protocol = 30;
        	  	 	 	        	  break;
        	  	 	 	        	 	 
        	  	 	 	        	default:
        	  	 	 	        	  meterConfig.protocol = 1;
        	  	 	 	        	  break;
        	  	 	 	        }
        	  	 	 	       #endif
        	  	 	 	        
     	  	 	 	      	  	//����ַ
     	  	 	 	      	  	meterConfig.addr[5] = (chrMp[1][0]-0x30)<<4 | (chrMp[1][1]-0x30);
     	  	 	 	      	  	meterConfig.addr[4] = (chrMp[1][2]-0x30)<<4 | (chrMp[1][3]-0x30);
     	  	 	 	      	  	meterConfig.addr[3] = (chrMp[1][4]-0x30)<<4 | (chrMp[1][5]-0x30);
     	  	 	 	      	  	meterConfig.addr[2] = (chrMp[1][6]-0x30)<<4 | (chrMp[1][7]-0x30);
     	  	 	 	      	  	meterConfig.addr[1] = (chrMp[1][8]-0x30)<<4 | (chrMp[1][9]-0x30);
     	  	 	 	      	  	meterConfig.addr[0] = (chrMp[1][10]-0x30)<<4 | (chrMp[1][11]-0x30);
     	  	 	 	      	  	
     	  	 	 	      	   
     	  	 	 	      	   #ifdef LIGHTING
     	  	 	 	      	    
     	  	 	 	      	    //�ɼ�����ַ
     	  	 	 	      	    meterConfig.collectorAddr[5] = 0x0;
     	  	 	 	      	    meterConfig.collectorAddr[4] = 0x0;
     	  	 	 	      	    meterConfig.collectorAddr[3] = 0x0;
     	  	 	 	      	    meterConfig.collectorAddr[2] = 0x0;
     	  	 	 	      	    meterConfig.collectorAddr[1] = 0x0;
     	  	 	 	      	    meterConfig.collectorAddr[0] = 0x0;

     	  	 	 	      	   #else

     	  	 	 	      	  	//�ɼ�����ַ
     	  	 	 	      	  	meterConfig.collectorAddr[5] = (chrMp[3][0]-0x30)<<4 | (chrMp[3][1]-0x30);
     	  	 	 	      	  	meterConfig.collectorAddr[4] = (chrMp[3][2]-0x30)<<4 | (chrMp[3][3]-0x30);
     	  	 	 	      	  	meterConfig.collectorAddr[3] = (chrMp[3][4]-0x30)<<4 | (chrMp[3][5]-0x30);
     	  	 	 	      	  	meterConfig.collectorAddr[2] = (chrMp[3][6]-0x30)<<4 | (chrMp[3][7]-0x30);
     	  	 	 	      	  	meterConfig.collectorAddr[1] = (chrMp[3][8]-0x30)<<4 | (chrMp[3][9]-0x30);
     	  	 	 	      	  	meterConfig.collectorAddr[0] = (chrMp[3][10]-0x30)<<4 | (chrMp[3][11]-0x30);

     	  	 	 	      	  	
     	  	 	 	      	   #endif

     	  	 	 	      	  	//���ʸ���
     	  	 	 	      	  	meterConfig.numOfTariff = chrMp[4][0]-0x30;

     	  	 	 	      	  	//����λ��С��λ����
     	  	 	 	      	  	meterConfig.mixed = 0x05;
     	  	 	 	      	  	
     	  	 	 	      	  	//����ż�С���
     	  	 	 	      	  	meterConfig.bigAndLittleType = 0x0;
                              
                          //����
  		                    saveDataF10(meterConfig.measurePoint, meterConfig.rateAndPort&0x1f, meterConfig.addr, meterConfig.number, (INT8U *)&meterConfig, 27);
                          carrierFlagSet.synSlaveNode = 1;    //����ͬ���ز�ģ��ӽڵ���Ϣ

                          guiLine(10,55,150,105,0);
                          guiLine(10,55,10,105,1);
                          guiLine(150,55,150,105,1);
                          guiLine(10,55,150,55,1);
                          guiLine(10,105,150,105,1);
                         #ifdef LIGHTING
                          guiDisplay(12,70," ���Ƶ����óɹ�!",1);
                         #else 
                          guiDisplay(12,70,"����������óɹ�!",1);
                         #endif
                          lcdRefresh(10,120);
                              
                          menuInLayer--;
                          layer2MenuLight[layer1MenuLight]=0xff;

                          return;
      	    		 	 	    }
      	    		 	 	       
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
      	    		 	 	    {
      	    		 	 	    	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	    }
      	    		 	 	    else
      	    		 	 	    {
      	    		 	 	      layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    //�л�������ʱ�������еĲ�������Ϣ
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
      	    		 	 	    {
      	    		 	 	    	 tmpData = (chrMp[0][0]-0x30)*1000 + (chrMp[0][1]-0x30)*100 + (chrMp[0][2]-0x30)*10 + (chrMp[0][3]-0x30);
      	    		 	 	    	 if (tmpData>2040 || tmpData<1)
      	    		 	 	    	 {
                              guiLine(10,55,150,105,0);
                              guiLine(10,55,10,105,1);
                              guiLine(150,55,150,105,1);
                              guiLine(10,55,150,55,1);
                              guiLine(10,105,150,105,1);
                              guiDisplay(20,70,"�������������!",1);
                              lcdRefresh(10,120);
      	    		 	 	    	 	  
      	    		 	 	    	 	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
      	    		 	 	    	 	  return;
      	    		 	 	    	 }
                           
                           if (selectF10Data(tmpData, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
                           {  
                           	  switch(meterConfig.protocol)
                           	  {
                           	  	 case 2:
                           	  	 	 chrMp[2][0] = 0x32;
                           	  	 	 break;
                           	  	 	 
                           	  	 case 30:
                           	  	 	 chrMp[2][0] = 0x31;
                           	  	 	 break;

                           	  	#ifdef LIGHTING
                           	  	 case 130:
                           	  	 	 chrMp[2][0] = 0x33;    //��·������
                           	  	 	 break;
                           	  	#endif
                           	  	 	 
                           	  	 default:
                           	  	 	 chrMp[2][0] = 0x30;
                           	  	 	 break;
                           	  }
                           	  
                           	  //���ַ
                  	 	 	   	  chrMp[1][0]  = (meterConfig.addr[5]>>4)+0x30;
                  	 	 	   	  chrMp[1][1]  = (meterConfig.addr[5]&0xf)+0x30;
                  	 	 	   	  chrMp[1][2]  = (meterConfig.addr[4]>>4)+0x30;
                  	 	 	   	  chrMp[1][3]  = (meterConfig.addr[4]&0xf)+0x30;
                  	 	 	   	  chrMp[1][4]  = (meterConfig.addr[3]>>4)+0x30;
                  	 	 	   	  chrMp[1][5]  = (meterConfig.addr[3]&0xf)+0x30;
                  	 	 	   	  chrMp[1][6]  = (meterConfig.addr[2]>>4)+0x30;
                  	 	 	   	  chrMp[1][7]  = (meterConfig.addr[2]&0xf)+0x30;
                  	 	 	   	  chrMp[1][8]  = (meterConfig.addr[1]>>4)+0x30;
                  	 	 	   	  chrMp[1][9]  = (meterConfig.addr[1]&0xf)+0x30;
                  	 	 	   	  chrMp[1][10] = (meterConfig.addr[0]>>4)+0x30;
                  	 	 	   	  chrMp[1][11] = (meterConfig.addr[0]&0xf)+0x30;
                  	 	 	   	  chrMp[1][12] = '\0';
                  	 	 	   	  
                  	 	 	   	  //�ɼ�����ַ
                  	 	 	   	  chrMp[3][0]  = (meterConfig.collectorAddr[5]>>4)+0x30;
                  	 	 	   	  chrMp[3][1]  = (meterConfig.collectorAddr[5]&0xf)+0x30;
                  	 	 	   	  chrMp[3][2]  = (meterConfig.collectorAddr[4]>>4)+0x30;
                  	 	 	   	  chrMp[3][3]  = (meterConfig.collectorAddr[4]&0xf)+0x30;
                  	 	 	   	  chrMp[3][4]  = (meterConfig.collectorAddr[3]>>4)+0x30;
                  	 	 	   	  chrMp[3][5]  = (meterConfig.collectorAddr[3]&0xf)+0x30;
                  	 	 	   	  chrMp[3][6]  = (meterConfig.collectorAddr[2]>>4)+0x30;
                  	 	 	   	  chrMp[3][7]  = (meterConfig.collectorAddr[2]&0xf)+0x30;
                  	 	 	   	  chrMp[3][8]  = (meterConfig.collectorAddr[1]>>4)+0x30;
                  	 	 	   	  chrMp[3][9]  = (meterConfig.collectorAddr[1]&0xf)+0x30;
                  	 	 	   	  chrMp[3][10] = (meterConfig.collectorAddr[0]>>4)+0x30;
                  	 	 	   	  chrMp[3][11] = (meterConfig.collectorAddr[0]&0xf)+0x30;
                  	 	 	   	  chrMp[3][12] = '\0';
                  	 	 	   	  
                  	 	 	   	 #ifdef LIGHTING 
                           	  //�˿�
                           	  switch(meterConfig.rateAndPort&0x1f)
                           	  {
                           	  	 case 31:
                           	  	 	 chrMp[3][0] = 0x30;
                           	  	 	 break;
                           	  	 	 
                           	  	 case 2:
                           	  	 	 chrMp[3][0] = 0x31;
                           	  	 	 break;

                           	  	 case 3:
                           	  	 	 chrMp[3][0] = 0x32;
                           	  	 	 break;
                           	  }
                           	 #endif

                  	 	 	   	  //���ʸ���
                  	 	 	   	  chrMp[4][0] = meterConfig.numOfTariff+0x30;
                           }
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    keyLeftRight = 0;
      	    		 	 	    setCarrierMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		break;
      	    	 	 	  		
      	    	 	 	  	case 3: //�ն�ʱ������
    	    		 	 	      tmpTime.year   = (dateTimeItem[2]-0x30)*10+(dateTimeItem[3]-0x30);
    	    		 	 	      tmpTime.month  = (dateTimeItem[4]-0x30)*10+(dateTimeItem[5]-0x30);
    	    		 	 	      tmpTime.day    = (dateTimeItem[6]-0x30)*10+(dateTimeItem[7]-0x30);
    	    		 	 	      tmpTime.hour   = (dateTimeItem[8]-0x30)*10+(dateTimeItem[9]-0x30);
    	    		 	 	      tmpTime.minute = (dateTimeItem[10]-0x30)*10+(dateTimeItem[11]-0x30);
    	    		 	 	      tmpTime.second = (dateTimeItem[12]-0x30)*10+(dateTimeItem[13]-0x30);    	    		 	 	      
    	    		 	 	      tmpData        = (dateTimeItem[0]-0x30)*1000+(dateTimeItem[1]-0x30)*100 + (dateTimeItem[2]-0x30)*10+(dateTimeItem[3]-0x30);
    	    		 	 	      
    	    		 	 	      //�ж����
    	    		 	 	      if (tmpData>2099 || tmpData<1999)
    	    		 	 	      {
    	    		 	 	       	 keyLeftRight = 0;
    	    		 	 	       	 guiDisplay(28,110,"����������!",1);
    	    		 	 	       	 lcdRefresh(110,126);
    	    		 	 	       	 return;
    	    		 	 	      }
    	    		 	 	      
    	    		 	 	      //�ж��·�
    	    		 	 	      if (tmpTime.month>12 || tmpTime.month<1)
    	    		 	 	      {
    	    		 	 	       	 keyLeftRight = 4;
    	    		 	 	       	 guiDisplay(28,110,"�·��������!",1);
    	    		 	 	       	 lcdRefresh(110,126);
    	    		 	 	       	 return;
    	    		 	 	      }
    	    		 	 	      
    	    		 	 	      //�ж�����
    	    		 	 	      tmpData = monthDays(tmpData,tmpTime.month);
    	    		 	 	      if (tmpTime.day>tmpData || tmpTime.day<1)
    	    		 	 	      {
    	    		 	 	       	 keyLeftRight = 6;
    	    		 	 	       	 guiDisplay(28,110,"�����������!",1);
    	    		 	 	       	 lcdRefresh(110,126);
    	    		 	 	       	 return;    	    		 	 	      	 
    	    		 	 	      }

    	    		 	 	      //�ж�Сʱ
    	    		 	 	      if (tmpTime.hour>24)
    	    		 	 	      {
    	    		 	 	       	 keyLeftRight = 8;
    	    		 	 	       	 guiDisplay(28,110,"Сʱ�������!",1);
    	    		 	 	       	 lcdRefresh(110,126);
    	    		 	 	       	 return;    	    		 	 	      	 
    	    		 	 	      }
    	    		 	 	      
    	    		 	 	      //�жϷ���
    	    		 	 	      if (tmpTime.minute>59)
    	    		 	 	      {
    	    		 	 	       	 keyLeftRight = 10;
    	    		 	 	       	 guiDisplay(28,110,"�����������!",1);
    	    		 	 	       	 lcdRefresh(110,126);
    	    		 	 	       	 return;    	    		 	 	      	 
    	    		 	 	      }

    	    		 	 	      //�ж�����
    	    		 	 	      if (tmpTime.second>59)
    	    		 	 	      {
    	    		 	 	       	 keyLeftRight = 12;
    	    		 	 	       	 guiDisplay(28,110,"�����������!",1);
    	    		 	 	       	 lcdRefresh(110,126);
    	    		 	 	       	 return;    	    		 	 	      	 
    	    		 	 	      }
                        
                        setSystemDateTime(tmpTime);
   
                        //ˢ��titleʱ����ʾ
                        refreshTitleTime();

                        //�������ó���ʱ��
                        reSetCopyTime();

                        guiLine(10,55,150,105,0);
                        guiLine(10,55,10,105,1);
                        guiLine(150,55,150,105,1);
                        guiLine(10,55,150,55,1);
                        guiLine(10,105,150,105,1);
                        guiDisplay(28,70,"�޸�ʱ��ɹ�!",1);
                        lcdRefresh(10,120);
                            
                        menuInLayer--;
    	    		 	 	       	  
    	    	 	 	  		 	break;
      	    	 	 	  		
      	    	 	 	  	case 4: //�޸Ľ�������
    	    		 	 	      if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
    	    		 	 	      {
    	    		 	 	       	for(i=0;i<6;i++)
    	    		 	 	       	{
    	    		 	 	       	  if (commParaItem[0][i]!=commParaItem[1][i])
    	    		 	 	       	  {
    	    		 	 	       	  	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
    	    		 	 	       	  	 keyLeftRight = 0;
    	    		 	 	       	  	 guiDisplay(20,100,"�������벻һ��!",1);
    	    		 	 	       	  	 lcdRefresh(100,120);
    	    		 	 	       	  	 return;
    	    		 	 	       	  }
    	    		 	 	       	}
    	    		 	 	           
    	    		 	 	        //ȷ��������
    	    		 	 	        keyLeftRight = 0xff;
    	    		 	 	        modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
    	    		 	 	        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0xff;
    	    		 	 	       	return;
    	    		 	 	      }
    	    		 	 	       
    	    		 	 	      if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0xff)
    	    		 	 	      {
    	    		 	 	       	  for(i=0;i<6;i++)
    	    		 	 	       	  {
    	    		 	 	       	  	 originPassword[i] = commParaItem[0][i];
    	    		 	 	       	  }
    	    		 	 	       	  originPassword[6] = '\0';
                            guiLine(10,55,150,105,0);
                            guiLine(10,55,10,105,1);
                            guiLine(150,55,150,105,1);
                            guiLine(10,55,150,55,1);
                            guiLine(10,105,150,105,1);
                            guiDisplay(28,70,"�޸�����ɹ�!",1);
                            lcdRefresh(10,120);
                            
                            menuInLayer--;
                            layer2MenuLight[layer1MenuLight]=0xff;
    	    		 	 	       	  return;
    	    		 	 	       }
    	    		 	 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
    	    		 	 	       {
    	    		 	 	       	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
    	    		 	 	       }
    	    		 	 	       else
    	    		 	 	       {
    	    		 	 	       	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
    	    		 	 	       }
    	    		 	 	       keyLeftRight = 0;
    	    		 	 	       modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
    	    	 	 	  		 	 break;
    	    	 	 	  		 	 
      	    	 	 	  	case 5: //�ն˱��
   	  	 	 	      	    #ifdef TE_ADDR_USE_BCD_CODE
   	  	 	 	      	  	 addrField.a2[1] = (tmpTeAddr[0]-0x30)<<4 | (tmpTeAddr[1]-0x30);
   	  	 	 	      	  	 addrField.a2[0] = (tmpTeAddr[2]-0x30)<<4 | (tmpTeAddr[3]-0x30);
   	  	 	 	      	  	 addrField.a1[1] = (tmpTeAddr[4]-0x30)<<4 | (tmpTeAddr[5]-0x30);
   	  	 	 	      	  	 addrField.a1[0] = (tmpTeAddr[6]-0x30)<<4 | (tmpTeAddr[7]-0x30);
   	  	 	 	      	    #else
   	  	 	 	      	  	 tmpAddr = (tmpTeAddr[0]-0x30)*10000+(tmpTeAddr[1]-0x30)*1000
   	  	 	 	      	  	         +(tmpTeAddr[2]-0x30)*100+(tmpTeAddr[3]-0x30)*10
   	  	 	 	      	  	         +(tmpTeAddr[4]-0x30);
   	  	 	 	      	  	 addrField.a2[1] = tmpAddr>>8;
   	  	 	 	      	  	 addrField.a2[0] = tmpAddr&0xff;
   	  	 	 	      	  	 addrField.a1[1] = (tmpTeAddr[5]-0x30)<<4 | (tmpTeAddr[6]-0x30);
   	  	 	 	      	  	 addrField.a1[0] = (tmpTeAddr[7]-0x30)<<4 | (tmpTeAddr[8]-0x30);
   	  	 	 	      	    #endif   	  	 	 	      	  	

                        //��������������
                        saveParameter(0x04, 121,(INT8U *)&addrField,4);
                        
                        saveBakKeyPara(121);    //2012-8-9,add
                        
                        guiLine(10,55,150,105,0);
                        guiLine(10,55,10,105,1);
                        guiLine(150,55,150,105,1);
                        guiLine(10,55,150,55,1);
                        guiLine(10,105,150,105,1);
                        guiDisplay(16,70,"�޸��ն˱�ųɹ�!",1);
                        lcdRefresh(10,120);
                        
                        menuInLayer--;
      	    	 	 	  		break;
      	    	 	 	    
      	    	 	 	    case 6:  //��̫����������
    	    		 	 	      if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==4)
    	    		 	 	      {
        	  	 	 	      	//��̫��IP��ַ
        	  	 	 	      	teIpAndPort.teIpAddr[0] =(chrEthPara[0][0]-0x30)*100+(chrEthPara[0][1]-0x30)*10+(chrEthPara[0][2]-0x30);
        	  	 	 	      	teIpAndPort.teIpAddr[1] =(chrEthPara[0][4]-0x30)*100+(chrEthPara[0][5]-0x30)*10+(chrEthPara[0][6]-0x30);
        	  	 	 	      	teIpAndPort.teIpAddr[2] =(chrEthPara[0][8]-0x30)*100+(chrEthPara[0][9]-0x30)*10+(chrEthPara[0][10]-0x30);
        	  	 	 	      	teIpAndPort.teIpAddr[3] =(chrEthPara[0][12]-0x30)*100+(chrEthPara[0][13]-0x30)*10+(chrEthPara[0][14]-0x30);

        	  	 	 	      	//��̫��IP����
        	  	 	 	      	teIpAndPort.mask[0] =(chrEthPara[1][0]-0x30)*100+(chrEthPara[1][1]-0x30)*10+(chrEthPara[1][2]-0x30);
        	  	 	 	      	teIpAndPort.mask[1] =(chrEthPara[1][4]-0x30)*100+(chrEthPara[1][5]-0x30)*10+(chrEthPara[1][6]-0x30);
        	  	 	 	      	teIpAndPort.mask[2] =(chrEthPara[1][8]-0x30)*100+(chrEthPara[1][9]-0x30)*10+(chrEthPara[1][10]-0x30);
        	  	 	 	      	teIpAndPort.mask[3] =(chrEthPara[1][12]-0x30)*100+(chrEthPara[1][13]-0x30)*10+(chrEthPara[1][14]-0x30);

        	  	 	 	      	//��̫������
        	  	 	 	      	teIpAndPort.gateWay[0] =(chrEthPara[2][0]-0x30)*100+(chrEthPara[2][1]-0x30)*10+(chrEthPara[2][2]-0x30);
        	  	 	 	      	teIpAndPort.gateWay[1] =(chrEthPara[2][4]-0x30)*100+(chrEthPara[2][5]-0x30)*10+(chrEthPara[2][6]-0x30);
        	  	 	 	      	teIpAndPort.gateWay[2] =(chrEthPara[2][8]-0x30)*100+(chrEthPara[2][9]-0x30)*10+(chrEthPara[2][10]-0x30);
        	  	 	 	      	teIpAndPort.gateWay[3] =(chrEthPara[2][12]-0x30)*100+(chrEthPara[2][13]-0x30)*10+(chrEthPara[2][14]-0x30);
        	  	 	 	      	
        	  	 	 	      	//�Ƿ�ʹ����̫����¼
        	  	 	 	      	teIpAndPort.ethIfLoginMs = chrEthPara[3][0];

        	  	 	 	      	//����
	                        saveIpMaskGateway(teIpAndPort.teIpAddr,teIpAndPort.mask,teIpAndPort.gateWay);  //���浽rcS��,ly,2011-04-12
	 
	                        saveParameter(0x04, 7, (INT8U *)&teIpAndPort, sizeof(TE_IP_AND_PORT));

													//���汸��007�����ļ�,2020-11-18,Add
													saveBakKeyPara(7);
                              
                          guiLine(6,55,154,105,0);
                          guiLine(6,55,6,105,1);
                          guiLine(154,55,154,105,1);
                          guiLine(6,55,154,55,1);
                          guiLine(6,105,154,105,1);
                          guiDisplay(8,60,"�޸���̫�������ɹ�",1);
                          guiDisplay(16,80,"������Ч��Ҫ����",1);
                          lcdRefresh(10,120);
                              
                          menuInLayer--;
                          layer2MenuLight[layer1MenuLight]=0xff;
    	    		 	 	      }
    	    		 	 	      else
    	    		 	 	      {
    	    		 	 	        if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
    	    		 	 	        {
    	    		 	 	      	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
    	    		 	 	        }
    	    		 	 	        else
    	    		 	 	        {
    	    		 	 	       	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
    	    		 	 	        }
    	    		 	 	        keyLeftRight = 0;
    	    		 	 	        setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
    	    		 	 	      }
      	    	 	 	    	break;
      	    	 	 	    	
      	    	 	 	    case 7:   //VPN�û�������
                 	 	 	  if (inputStatus == STATUS_SELECT_CHAR)
                 	 	 	  {
                 	 	 	  	inputStatus = STATUS_NONE;
                 	 	 	  	if (keyLeftRight<32)
                 	 	 	  	{
                 	 	 	  	  tmpVpnUserName[keyLeftRight] = character[selectIndex];
                 	 	 	  	}
                 	 	 	  	else
                 	 	 	  	{
                 	 	 	  	  tmpVpnPw[keyLeftRight-32] = character[selectIndex];
                 	 	 	  	}
                 	 	 	  	setVpn(keyLeftRight);
                 	 	 	  }
                 	 	 	  else
                 	 	 	  {
                          tmpVpnUserName[32] = '\0';
                          trim(tmpVpnUserName);
                          tmpVpnPw[32] = '\0';
                          trim(tmpVpnPw);
                          //strcpy((char *)vpn.vpnName, tmpVpnUserName);
                          //memcpy(vpn.vpnName, tmpVpnUserName, 16);
                          //ly,2012-03-12,���������ݷ��ִ���
                          memcpy(vpn.vpnName, tmpVpnUserName, 32);
                          //strcpy((char *)vpn.vpnPassword, tmpVpnPw);
                          //memcpy(vpn.vpnPassword, tmpVpnPw, 16);
                          memcpy(vpn.vpnPassword, tmpVpnPw, 32);

        	  	 	 	      	//����vpn�û�������
        	  	 	 	      	saveParameter(0x04, 16,(INT8U *)&vpn, sizeof(VPN));
                        	
                        	saveBakKeyPara(16);    //2012-8-9,add

                          guiLine(10,55,150,105,0);
                          guiLine(10,55,10,105,1);
                          guiLine(150,55,150,105,1);
                          guiLine(10,55,150,55,1);
                          guiLine(10,105,150,105,1);
                          guiDisplay(20, 70, "ר���������޸�!",1);
                          lcdRefresh(10,120);
                              
                          menuInLayer--;
                 	 	 	  }
                 	 	 	  break;
                 	 	 	  
      	    	 	 	  	case 8:   //�����ն˼���
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==5)
      	    		 	 	    {
        	  	 	 	      	//�����˿�
        	  	 	 	      	if (chrMp[0][0]!=0x33)
        	  	 	 	      	{
                             guiLine(10,55,150,105,0);
                             guiLine(10,55,10,105,1);
                             guiLine(150,55,150,105,1);
                             guiLine(10,55,150,55,1);
                             guiLine(10,105,150,105,1);
                             guiDisplay(20,60,"�������!�����˿�ֻ",1);
                             guiDisplay(20,80,"��Ϊ3",1);
                             lcdRefresh(10,120);
      	    		 	 	    	 	 
      	    		 	 	    	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
                             return;
        	  	 	 	      	}
        	  	 	 	      	
        	  	 	 	      	cascadeCommPara.commPort = 0x03; //�˿�3
        	  	 	 	      	cascadeCommPara.ctrlWord = 0x8B;             //Ĭ��4800,8-e-1,Ŀǰ���ֻ��ͨ�������
        	  	 	 	      	cascadeCommPara.receiveMsgTimeout = 0x05;
        	  	 	 	      	cascadeCommPara.receiveByteTimeout = 0x05;
        	  	 	 	      	cascadeCommPara.cascadeMretryTime = 0x01;
        	  	 	 	      	cascadeCommPara.groundSurveyPeriod = 0x05;   //Ѳ������
        	  	 	 	      	
        	  	 	 	      	cascadeCommPara.flagAndTeNumber = 0x00;
        	  	 	 	      	if (chrMp[1][0]==0x30)
        	  	 	 	      	{
        	  	 	 	      	  cascadeCommPara.divisionCode[0] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[1] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[2] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[3] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[4] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[5] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[0] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[1] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[2] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[3] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[4] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[5] = 0x00;
        	  	 	 	      	}
        	  	 	 	      	else
        	  	 	 	      	{
   	  	 	 	      	  	    //�ն˵�ַ1
   	  	 	 	      	  	    tmpAddr = (chrMp[2][4]-0x30)*10000+(chrMp[2][5]-0x30)*1000
   	  	 	 	      	  	            +(chrMp[2][6]-0x30)*100+(chrMp[2][7]-0x30)*10
   	  	 	 	      	  	            +(chrMp[2][8]-0x30);
        	  	 	 	      	  tmpData = (chrMp[2][0]-0x30)<<12 | (chrMp[2][1]-0x30)<<8 | (chrMp[2][2]-0x30)<<4 | (chrMp[2][3]-0x30);
        	  	 	 	      	  if (tmpData!=0x0 && tmpAddr!=0x00)
        	  	 	 	      	  {
        	  	 	 	      	  	cascadeCommPara.flagAndTeNumber++;

   	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[1] = tmpAddr>>8;
   	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[0] = tmpAddr&0xff;
   	  	 	 	      	  	      cascadeCommPara.divisionCode[1] = (chrMp[2][0]-0x30)<<4 | (chrMp[2][1]-0x30);
   	  	 	 	      	  	      cascadeCommPara.divisionCode[0] = (chrMp[2][2]-0x30)<<4 | (chrMp[2][3]-0x30);
        	  	 	 	      	  }

        	  	 	 	      		if (chrMp[1][0]==0x32)
        	  	 	 	      		{
        	  	 	 	      			cascadeCommPara.flagAndTeNumber |= 0x80;
        	  	 	 	      		}
        	  	 	 	      		else
        	  	 	 	      		{
     	  	 	 	      	  	    //�ն˵�ַ2
     	  	 	 	      	  	    tmpAddr = (chrMp[3][4]-0x30)*10000+(chrMp[3][5]-0x30)*1000
     	  	 	 	      	  	            +(chrMp[3][6]-0x30)*100+(chrMp[3][7]-0x30)*10
     	  	 	 	      	  	            +(chrMp[3][8]-0x30);
          	  	 	 	      	  tmpData = (chrMp[3][0]-0x30)<<12 | (chrMp[3][1]-0x30)<<8 | (chrMp[3][2]-0x30)<<4 | (chrMp[3][3]-0x30);
          	  	 	 	      	  if (tmpData!=0x0 && tmpAddr!=0x00)
          	  	 	 	      	  {
          	  	 	 	      	  	cascadeCommPara.flagAndTeNumber++;
  
     	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[3] = tmpAddr>>8;
     	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[2] = tmpAddr&0xff;
     	  	 	 	      	  	      cascadeCommPara.divisionCode[3] = (chrMp[3][0]-0x30)<<4 | (chrMp[3][1]-0x30);
     	  	 	 	      	  	      cascadeCommPara.divisionCode[2] = (chrMp[3][2]-0x30)<<4 | (chrMp[3][3]-0x30);
          	  	 	 	      	  }

     	  	 	 	      	  	    //�ն˵�ַ3
     	  	 	 	      	  	    tmpAddr = (chrMp[4][4]-0x30)*10000+(chrMp[4][5]-0x30)*1000
     	  	 	 	      	  	            +(chrMp[4][6]-0x30)*100+(chrMp[4][7]-0x30)*10
     	  	 	 	      	  	            +(chrMp[4][8]-0x30);
          	  	 	 	      	  tmpData = (chrMp[4][0]-0x30)<<12 | (chrMp[4][1]-0x30)<<8 | (chrMp[4][2]-0x30)<<4 | (chrMp[4][3]-0x30);
          	  	 	 	      	  if (tmpData!=0x0 && tmpAddr!=0x00)
          	  	 	 	      	  {
          	  	 	 	      	  	cascadeCommPara.flagAndTeNumber++;
  
     	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[5] = tmpAddr>>8;
     	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[4] = tmpAddr&0xff;
     	  	 	 	      	  	      cascadeCommPara.divisionCode[5] = (chrMp[4][0]-0x30)<<4 | (chrMp[4][1]-0x30);
     	  	 	 	      	  	      cascadeCommPara.divisionCode[4] = (chrMp[4][2]-0x30)<<4 | (chrMp[4][3]-0x30);
          	  	 	 	      	  }
        	  	 	 	      		}
        	  	 	 	      	}
        	  	 	 	      	
                          saveParameter(0x04, 37, (INT8U *)&cascadeCommPara, sizeof(CASCADE_COMM_PARA));

                          guiLine(10,55,150,105,0);
                          guiLine(10,55,10,105,1);
                          guiLine(150,55,150,105,1);
                          guiLine(10,55,150,55,1);
                          guiLine(10,105,150,105,1);
                          guiDisplay(12,70,"�����������óɹ�!",1);
                          lcdRefresh(10,120);
                              
                          menuInLayer--;
                          return;        	  	 	 	      	
        	  	 	 	      }

      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
      	    		 	 	    {
      	    		 	 	    	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	    }
      	    		 	 	    else
      	    		 	 	    {
      	    		 	 	      layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    keyLeftRight = 0;
      	    		 	 	    setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		break;
      	    	 	 	  		      	    	 	 	  		
      	    	 	 	  	case 9:   //���ģ���������
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==4)
      	    		 	 	    {
        	  	 	 	      	//��������
        	  	 	 	      	if (((chrRlPara[0][0]-0x30)*10+(chrRlPara[0][1]-0x30))>63
        	  	 	 	      		  || ((chrRlPara[1][0]-0x30)*10+(chrRlPara[1][1]-0x30))>63
        	  	 	 	      		  || ((chrRlPara[2][0]-0x30)*10+(chrRlPara[2][1]-0x30))>63
        	  	 	 	      		  || ((chrRlPara[3][0]-0x30)*10+(chrRlPara[3][1]-0x30))>6
        	  	 	 	      		 )
        	  	 	 	      	{
                             guiLine(10,55,150,105,0);
                             guiLine(10,55,10,105,1);
                             guiLine(150,55,150,105,1);
                             guiLine(10,55,150,55,1);
                             guiLine(10,105,150,105,1);
                             guiDisplay(20,60,"�������!���ʷ�",1);
                             guiDisplay(20,80,"Χ1-63,�ŵ�1-6",1);
                             lcdRefresh(10,120);
      	    		 	 	    	 	 
      	    		 	 	    	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
                             return;
        	  	 	 	      	}
        	  	 	 	      	
        	  	 	 	      	rlPara[0] = (chrRlPara[0][0]-0x30)*10+(chrRlPara[0][1]-0x30);
        	  	 	 	      	rlPara[1] = (chrRlPara[1][0]-0x30)*10+(chrRlPara[1][1]-0x30);
        	  	 	 	      	rlPara[2] = (chrRlPara[2][0]-0x30)*10+(chrRlPara[2][1]-0x30);
        	  	 	 	      	rlPara[3] = (chrRlPara[3][0]-0x30)*10+(chrRlPara[3][1]-0x30);
        	  	 	 	      	
        	  	 	 	      	saveParameter(0x04, 135, (INT8U *)&rlPara, 4);
                          
                          guiLine(10,55,150,105,0);
                          guiLine(10,55,10,105,1);
                          guiLine(150,55,150,105,1);
                          guiLine(10,55,150,55,1);
                          guiLine(10,105,150,105,1);
                          guiDisplay(12,70,"��β������óɹ�!",1);
                          lcdRefresh(10,120);
                              
                          menuInLayer--;
                          return;        	  	 	 	      	
        	  	 	 	      }

      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
      	    		 	 	    {
      	    		 	 	    	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	    }
      	    		 	 	    else
      	    		 	 	    {
      	    		 	 	      layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    keyLeftRight = 0;
      	    		 	 	    setRlPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		break;
      	    	 	 	  }
      	    	 	 	  break;
      	    	 	 	
      	    	 	 	case 2:    //�ն˹�����ά��
      	    	 	 	  switch(layer2MenuLight[layer1MenuLight])
      	    	 	 	  {
    	    	 	 	  		 case 0:  //ʵʱ����
    	    	 	 	  		 	 if (carrierModuleType==NO_CARRIER_MODULE || ((carrierModuleType==RL_WIRELESS || carrierModuleType==SR_WIRELESS)  && carrierFlagSet.wlNetOk<3))
      	    		 	 	 	   {
                           guiLine(10,55,150,105,0);
                           guiLine(10,55,10,105,1);
                           guiLine(150,55,150,105,1);
                           guiLine(10,55,150,55,1);
                           guiLine(10,105,150,105,1);
                           if (carrierModuleType==NO_CARRIER_MODULE)
                           {
                             guiDisplay(15,60,"  �ȴ�ʶ��ģ��!",1);
                           }
                           else
                           {
                             guiDisplay(15,60,"�ȴ�ģ���������!",1);
                           }
                           guiDisplay(15,80,"     ���Ժ�   ",1);
                           lcdRefresh(10,120);
      	    		 	 	 	   }
      	    		 	 	 	   else
      	    		 	 	 	   {
      	    	 	 	  		 	 switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
      	    	 	 	  		 	 {
      	    	 	 	  		 	 	  case 0: //ָ�������㳭��
      	    	 	 	  		 	 	  	keyLeftRight = 0;
      	    	 	 	  		 	 	  	strcpy(singleCopyTime,"-----");
             	 	                strcpy(singleCopyEnergy,"-----");
      	    	 	 	  		 	 	  	singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
      	    	 	 	  		 	 	  	break;
      	    	 	 	  		 	 	  	
      	    	 	 	  		 	 	  case 1: //ȫ�������㳭��
      	    	 	 	  		 	 	  	if (pDotCopy==NULL)
      	    	 	 	  		 	 	  	{
      	    	 	 	  		 	 	  	  keyLeftRight = 1;
      	    	 	 	  		 	 	  	  allMpCopy(keyLeftRight);
      	    	 	 	  		 	 	  	}
      	    	 	 	  		 	 	  	else
      	    	 	 	  		 	 	  	{
                                  guiLine(10,55,150,105,0);
                                  guiLine(10,55,10,105,1);
                                  guiLine(150,55,150,105,1);
                                  guiLine(10,55,150,55,1);
                                  guiLine(10,105,150,105,1);
                                  guiDisplay(12,70,"���ڽ��е㳭,�Ժ�",1);
                                  lcdRefresh(10,120);
      	    	 	 	  		 	 	  	}
      	    	 	 	  		 	 	  	break;
      	    	 	 	  		 	 }
      	    	 	 	  		 }
    	    	 	 	  		 	 break;
    	    	 	 	  		 
    	    	 	 	  		 case 1:
    	    	 	 	  		 	#ifdef LIGHTING    //�����������Ǳ�����Ϣ

      	    		 	 	 	   if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-2)
      	    		 	 	 	   {
      	    		 	 	 	   	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
      	    		 	 	 	   }
      	    		 	 	 	   else
      	    		 	 	 	   {
      	    		 	 	 	  	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	 	   }
      	    	 	 	  		 terminalInfo(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    	 	 	  		 
    	    	 	 	  		 	#endif
    	    	 	 	  		 	 break;
    	    	 	 	  		 	 
    	    	 	 	  		 case 2:  //�������
    	    	 	 	  		 	#ifndef LIGHTING
    	    	 	 	  		 	 switch(keyLeftRight)
    	    	 	 	  		 	 {
    	    	 	 	  		 	 	 case 0:  //�����ѱ�
    	    	 	 	  		 	 	   if (carrierFlagSet.searchMeter==0)
    	    	 	 	  		 	 	   {
    	    	 	 	  		 	 	     //�����ǰ�������ı�
    	    	 	 	  		 	 	     while(foundMeterHead!=NULL)
    	    	 	 	  		 	 	     {
    	    	 	 	  		 	 	   	    tmpFound = foundMeterHead;
    	    	 	 	  		 	 	   	    foundMeterHead = foundMeterHead->next;
    	    	 	 	  		 	 	   	    free(tmpFound);
    	    	 	 	  		 	 	     }
    	    	 	 	  		 	 	     foundMeterHead = NULL;
    	    	 	 	  		 	 	     prevFound = foundMeterHead;
    	    	 	 	  		 	 	     carrierFlagSet.foundStudyTime = nextTime(sysTime, assignCopyTime[0]|assignCopyTime[1]<<8, 0); //�ѱ�ʱ��
    	    	 	 	  		 	 	   
    	    	 	 	  		 	 	     carrierFlagSet.searchMeter = 1;         //��ʼ�ѱ��־��1
    	    	 	 	  		 	 	   }
    	    	 	 	  		 	 	 	 keyLeftRight = 0xff;
    	    	 	 	  		 	 	   searchMeter(keyLeftRight);
 
    	    	 	 	  		 	 	 	 break;
    	    	 	 	  		 	 	 	  
    	    	 	 	  		 	 	 case 1:
    	    	 	 	  		 	 	 	 keyLeftRight = 0;
    	    	 	 	  		 	 	 	 searchMeter(keyLeftRight);
    	    	 	 	  		 	 	 	 break;
    	    	 	 	  		 	 	 	 
    	    	 	 	  		 	 	 case 0xff:
    	    	 	 	  		 	 	 	 layer2MenuLight[layer1MenuLight]++;
    	    	 	 	  		 	 	 	 keyLeftRight = 1;
    	    	 	 	  		 	 	 	 newAddMeter(keyLeftRight);
    	    	 	 	  		 	 	 	 break;
    	    	 	 	  		 	 }
    	    	 	 	  		 	#endif
    	    	 	 	  		 	 break;
    	    	 	 	  		 	 
    	    	 	 	  		 case 3:  //�������ܱ��ַ
    	    	 	 	  		 	 switch(keyLeftRight)
    	    	 	 	  		 	 {
    	    	 	 	  		 	 	  case 0: //��������(���������ܱ��ʾֵ)
           	 	 	  			      keyLeftRight = 1;
           	 	 	  			      
           	 	 	  			      tmpFound = foundMeterHead;
           	 	 	  			      while(tmpFound!=NULL)
           	 	 	  			      {
           	 	 	  			      	 tmpFound->copyTime[0]   = 0xee;
           	 	 	  			      	 tmpFound->copyTime[1]   = 0xee;
           	 	 	  			      	 tmpFound->copyEnergy[0] = 0xee;
           	 	 	  			      	 
           	 	 	  			      	 tmpFound = tmpFound->next;
           	 	 	  			      }
           	 	 	  			      layer2MenuLight[layer1MenuLight]++;
           	 	 	  			      multiCpUpDown = 0;
           	 	 	  			      newMeterCpStatus(0);
           	 	 	  			      
           	 	 	  			      if (foundMeterHead!=NULL)
           	 	 	  			      {
                          	     pDotCopy = (DOT_COPY *)malloc(sizeof(DOT_COPY));
                          	     prevFound = foundMeterHead;
                          	     pDotCopy->dotCopyMp = prevFound->mp;
           	 	 	  			       	 memcpy(pDotCopy->addr, prevFound->addr, 6);
                          	     pDotCopy->dotCopying = FALSE;
                          	     pDotCopy->port       = PORT_POWER_CARRIER;
                          	     pDotCopy->dataFrom   = DOT_CP_NEW_METER;
                          	     pDotCopy->outTime    = nextTime(sysTime,0,22);
                          	     pDotCopy->dotResult  = RESULT_NONE;
           	 	 	  			      }
    	    	 	 	  		 	 	  	break;
    	    	 	 	  		 	 	  	
    	    	 	 	  		 	 	  case 1:
    	    	 	 	  		 	 	  	//noFoundMeter(0);
    	    	 	 	  		 	 	  	break;
    	    	 	 	  		 	 	  	
    	    	 	 	  		 	 	  case 0xff:   //���ڳ���ת���������ܱ���״̬ҳ
    	    	 	 	  		 	 	  	break;
    	    	 	 	  		 	 }
    	    	 	 	  		 	 break;
    	    	 	 	  		 	 
    	    	 	 	  		 case 5:
											 	#ifdef LIGHTING
												 //����ң��ѧϰ����RS485����

										     //����ö˿ڵ�ǰδ����ת��
										     if (copyCtrl[1].pForwardData==NULL)
										     {
													 guiLine(10,55,150,105,0);
													 guiLine(10,55,10,105,1);
													 guiLine(150,55,150,105,1);
													 guiLine(10,55,150,55,1);
													 guiLine(10,105,150,105,1);
													 
													 tmpData = 0;
													 if (keyLeftRight<4)
										       {
													   if (selectParameter(5, 160+keyLeftRight, (INT8U *)str, 2)==FALSE)
													   {
														   tmpData = 1;
															 
															 guiDisplay(20,75,"�޺���ѧϰ����.",1);
													   }
										       }

													 if (0==tmpData)
													 {
									       	   copyCtrl[1].pForwardData = (FORWARD_DATA *)malloc(sizeof(FORWARD_DATA));
									           copyCtrl[1].pForwardData->fn            = 1;
									           copyCtrl[1].pForwardData->dataFrom      = DOT_CP_IR;
									       	   copyCtrl[1].pForwardData->ifSend        = FALSE;
									       	   copyCtrl[1].pForwardData->receivedBytes = 0;
									       	   copyCtrl[1].pForwardData->forwardResult = RESULT_NONE;
									       
									       	   //͸��ת��ͨ�ſ�����,2400-8-n-1
									       	   copyCtrl[1].pForwardData->ctrlWord = 0x63;    //2400-8-n-1
									       	  
									       	   //͸��ת�����յȴ��ֽڳ�ʱʱ��
									       	   copyCtrl[1].pForwardData->byteTimeOut = 1;
									       	  
														 copyCtrl[1].pForwardData->data[768] = keyLeftRight;		//���ֽڱ���ѧϰ����������
														 
														 //ѧϰ
														 if (keyLeftRight>3)
														 {
										       	   //͸��ת�����յȴ����ĳ�ʱʱ��
															 //��λΪs
										       	   copyCtrl[1].pForwardData->frameTimeOut = 5;
															 
															 copyCtrl[1].pForwardData->length = 4;
										       	  
										       	   //͸��ת������
										       	   copyCtrl[1].pForwardData->data[0] = 0xfa;
										       	   copyCtrl[1].pForwardData->data[1] = 0xfd;
										       	   copyCtrl[1].pForwardData->data[2] = 0x01;
										       	   copyCtrl[1].pForwardData->data[3] = 0x01;

															 guiDisplay(40,75,"ѧϰ��...",1);
														 }
														 else    //������
													 	 {
														   //͸��ת�����յȴ����ĳ�ʱʱ��
														   //��λΪs
														   copyCtrl[1].pForwardData->frameTimeOut = 1;

															 if (selectParameter(5, 160+keyLeftRight, copyCtrl[1].pForwardData->data, 2)==TRUE)
															 {
																 copyCtrl[1].pForwardData->length = copyCtrl[1].pForwardData->data[0] | copyCtrl[1].pForwardData->data[1]<<8;
																 printf("��������=%d\n", copyCtrl[1].pForwardData->length);
																 
																 selectParameter(5, 160+keyLeftRight, copyCtrl[1].pForwardData->data, copyCtrl[1].pForwardData->length+2);
																 for(tmpData=0; tmpData<copyCtrl[1].pForwardData->length; tmpData++)
																 {
																 	 copyCtrl[1].pForwardData->data[tmpData] = copyCtrl[1].pForwardData->data[tmpData+2];
																 }
																 
																 guiDisplay(40,75,"������...",1);
															 }
													 	 }
													 }
													 lcdRefresh(10,120);
										     }
												 
												#else

												 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-2)
      	    		 	 	 	   {
      	    		 	 	 	   	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
      	    		 	 	 	   }
      	    		 	 	 	   else
      	    		 	 	 	   {
      	    		 	 	 	  	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	 	   }
      	    	 	 	  		 terminalInfo(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
												#endif
      	    	 	 	  		 break;
      	    	 	 	  	
      	    	 	 	  	 case 7:  //����ͨ��ģ�鳭����ʽ����
                         //��������������
                         if (keyLeftRight==1)
                         {
                         	 localCopyForm = 0xaa;
                         }
                        else
                         {
                         	 localCopyForm = 0x55;
                         }
                         saveParameter(88, 55, &localCopyForm, 1);

                         guiLine(10,55,150,105,0);
                         guiLine(10,55,10,105,1);
                         guiLine(150,55,150,105,1);
                         guiLine(10,55,150,55,1);
                         guiLine(10,105,150,105,1);
                         guiDisplay(12,70,"���ó�����ʽ�ɹ�!",1);
                         lcdRefresh(10,120);
                             
                         menuInLayer--;
                         return;
      	    	 	 	  	 	 break;
      	    	 	 	  	 
      	    	 	 	  	 case 11:    //�����û�����������
        	    	 	 	  	 switch (keyLeftRight)
        	    	 	 	  	 {
        	    	 	 	  	  	case 0x01:
        	    	 	 	  	  		denizenDataType = 0x55;
        	    	 	 	  	  	  break;
        	    	 	 	  	  	  
        	    	 	 	  	  	case 0x02:
        	    	 	 	  	  	  denizenDataType = 0xaa;
        	    	 	 	  	  	  break;
        	    	 	 	  	  
        	    	 	 	  	  	default:
        	    	 	 	  	  		denizenDataType = 0;
        	    	 	 	  	  		break;
        	    	 	 	  	 }
        	    	 	 	  	 saveParameter(0x04, 138, (INT8U *)&denizenDataType, 1);
        	    	 	 	  	  
                         guiLine(10,55,150,105,0);
                         guiLine(10,55,10,105,1);
                         guiLine(150,55,150,105,1);
                         guiLine(10,55,150,55,1);
                         guiLine(10,105,150,105,1);
                         guiDisplay(12,70,"����������óɹ�!",1);
                         lcdRefresh(10,120);
                             
                         menuInLayer--;
                         return;
                         break;
                         
      	    	 	 	  	 case 12:    //��������
        	    	 	 	  	 switch (keyLeftRight)
        	    	 	 	  	 {
        	    	 	 	  	   case 0x01:
        	    	 	 	  	  	 cycleDataType = 0x55;
        	    	 	 	  	  	 break;
        	    	 	 	  	  
        	    	 	 	  	   default:
        	    	 	 	  	  	 cycleDataType = 0;
        	    	 	 	  	  	 break;
        	    	 	 	  	 }
        	    	 	 	  	 saveParameter(0x04, 199, (INT8U *)&cycleDataType, 1);
        	    	 	 	  	  
                         guiLine(10,55,150,105,0);
                         guiLine(10,55,10,105,1);
                         guiLine(150,55,150,105,1);
                         guiLine(10,55,150,55,1);
                         guiLine(10,105,150,105,1);
                         guiDisplay(12,70,"�����������óɹ�!",1);
                         lcdRefresh(10,120);
                             
                         menuInLayer--;
                         return;
                         break;

      	    	 	 	  	 case 13:    //ά���ӿ�ģʽ����
        	    	 	 	  	 switch (keyLeftRight)
        	    	 	 	  	 {
        	    	 	 	  	   case 0x01:
        	    	 	 	  	  	 mainTainPortMode = 0x55;
        	    	 	 	  	  	 break;
        	    	 	 	  	  
        	    	 	 	  	   default:
        	    	 	 	  	  	 mainTainPortMode = 0;
        	    	 	 	  	  	 break;
        	    	 	 	  	 }
        	    	 	 	  	  
                         guiLine(10,55,150,105,0);
                         guiLine(10,55,10,105,1);
                         guiLine(150,55,150,105,1);
                         guiLine(10,55,150,55,1);
                         guiLine(10,105,150,105,1);
                         guiDisplay(9,70,"ά����ģʽ���óɹ�",1);
                         lcdRefresh(10,120);
                             
                         menuInLayer--;
                         return;
      	    	 	 	  	   break;

      	    	 	 	  	 case 14:    //��2·485�ڹ�������
        	    	 	 	  	 switch (keyLeftRight)
        	    	 	 	  	 {
        	    	 	 	  	   case 0x01:
        	    	 	 	  	  	 rs485Port2Fun = 0x55;
        	    	 	 	  	  	 
        	    	 	 	  	  	 //���õ�2·485������Ϊ9600-8-e-1
                            #ifdef WDOG_USE_X_MEGA
                             str[0] = 0x02;    //xMega�˿�2
                             str[1] = 0xcb;    //�˿�����,9600-8-e-1
                             sendXmegaFrame(COPY_PORT_RATE_SET,(INT8U *)str, 2);
                              
                             printf("���õ�2·����Ϊ9600-8-e-1\n");
                            #endif

        	    	 	 	  	  	 break;
        	    	 	 	  	  
        	    	 	 	  	   default:
        	    	 	 	  	  	 rs485Port2Fun = 0;
        	    	 	 	  	  	 break;
        	    	 	 	  	 }
        	    	 	 	  	  
                         guiLine(10,55,150,105,0);
                         guiLine(10,55,10,105,1);
                         guiLine(150,55,150,105,1);
                         guiLine(10,55,150,55,1);
                         guiLine(10,105,150,105,1);
                         guiDisplay(9,70,"485��2�������óɹ�",1);
                         lcdRefresh(10,120);
                             
                         menuInLayer--;
                         return;
      	    	 	 	  	   break;
      	    	 	 	  	   
      	    	 	 	  	 case 15:    //����ͨ��ģ��Э������
        	    	 	 	  	 switch (keyLeftRight)
        	    	 	 	  	 {
        	    	 	 	  	   case 0x01:
        	    	 	 	  	  	 lmProtocol = 0x55;
        	    	 	 	  	  	 break;
        	    	 	 	  	  
        	    	 	 	  	   default:
        	    	 	 	  	  	 lmProtocol = 0;
        	    	 	 	  	  	 break;
        	    	 	 	  	 }
                         saveParameter(88, 56, &lmProtocol, 1);

                         guiLine(10,55,150,105,0);
                         guiLine(10,55,10,105,1);
                         guiLine(150,55,150,105,1);
                         guiLine(10,55,150,55,1);
                         guiLine(10,105,150,105,1);
                         guiDisplay(15,70,"ģ��Э�����óɹ�",1);
                         lcdRefresh(10,120);
                             
                         menuInLayer--;
                         return;
      	    	 	 	  	   break;
      	    	 	 	  }
      	    	 	 	  break;
									
									case 3:    //��·���Ƶ�״̬
										if (queryMpLink!=NULL)
										{
										  if (xlcForwardFrame(queryMpLink->port-1, queryMpLink->addr, keyLeftRight)==TRUE)
											{
												guiLine(10,55,150,105,0);
												guiLine(10,55,10,105,1);
												guiLine(150,55,150,105,1);
												guiLine(10,55,150,55,1);
												guiLine(10,105,150,105,1);
												if (keyLeftRight<2)
												{
												  guiDisplay(52,70,"��բ...",1);
												}
												else
												{
												  if (keyLeftRight<4)
													{
													  guiDisplay(52,70,"��բ...",1);
													}
													else
													{
													  guiDisplay(52,70,"�Զ�����...",1);
													}
												}
												lcdRefresh(50,110);
											}
										}
										break;
      	    	 	}
      	    	 	break;
      	    	 	
            	case 4:    //�˵���4��
             	 	switch(layer1MenuLight)
             	 	{
             	 	 	case 1: //����������鿴
             	 	 	  if (inputStatus == STATUS_NONE)
             	 	 	  {
                      keyLeftRight = 0;
                      layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	 	commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  }
             	 	 	  if (inputStatus == STATUS_SELECT_CHAR)
             	 	 	  {
             	 	 	  	inputStatus = STATUS_NONE;
             	 	 	  	commParaItem[1][inputIndex] = character[selectIndex];
             	 	 	  	inputApn(inputIndex);
             	 	 	  }
             	 	 	  break;
             	 	 	  
           	 	 	  case 2: //�ն˹�����ά��
           	 	 	  	switch(layer2MenuLight[layer1MenuLight])
           	 	 	  	{
           	 	 	  		case 0:  //ʵʱ����
           	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
           	 	 	  			{
           	 	 	  				 case 0:
                          	 tmpData = (singleCopyMp[0]-0x30)*1000+(singleCopyMp[1]-0x30)*100
                          	         + (singleCopyMp[2]-0x30)*10 +(singleCopyMp[3]-0x30);
                          	 if (tmpData>2040)
                          	 {
           	 	 	  			      #ifdef LIGHTING
           	 	 	  			       guiDisplay(20,120,"���Ƶ��������!",1);
           	 	 	  			      #else
           	 	 	  			       guiDisplay(20,120,"�������������!",1);
           	 	 	  			      #endif
           	 	                 lcdRefresh(120,140);
                          	 }
                          	 else
                          	 {             	 	 	  			       
           	 	                 strcpy(singleCopyTime,"-----");
           	 	                 strcpy(singleCopyEnergy,"-----");
           	 	                #ifdef LIGHTING
           	 	                 singleCcbCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
           	 	 	  			       guiDisplay(44,120,"��ѯ��...",1);
           	 	                #else 
           	 	                 singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
           	 	 	  			       guiDisplay(44,120,"������...",1);
           	 	 	  			      #endif
           	 	                 lcdRefresh(120,140);
           	 	                 
                          	   pDotCopy = (DOT_COPY *)malloc(sizeof(DOT_COPY));
                          	   pDotCopy->dotCopyMp  = tmpData;
                          	   pDotCopy->dotCopying = FALSE;
                          	   pDotCopy->port       = PORT_POWER_CARRIER;
                          	   pDotCopy->dataFrom   = DOT_CP_SINGLE_MP;
                          	   pDotCopy->outTime    = nextTime(sysTime,0,22);
                          	   pDotCopy->dotResult  = RESULT_NONE;
                          	 }
           	 	               break;
           	 	               
           	 	             case 1:
           	 	             	 if (keyLeftRight==0)
           	 	             	 {
                               while(mpCopyHead!=NULL)
                               {
                               	  prevMpCopy = mpCopyHead;
                               	  mpCopyHead = mpCopyHead->next;
                               	  free(mpCopyHead);
                               }
                               
                               queryMpLink = initPortMeterLink(30);
           	 	 	  			       tmpMpCopy = mpCopyHead;
           	 	 	  			       multiCpMax = 0;
           	 	 	  			       while (queryMpLink!=NULL)
           	 	 	  			       {
           	 	 	  			       	  tmpMpCopy = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
           	 	 	  			       	  tmpMpCopy->mp = queryMpLink->mp;
           	 	 	  			       	  tmpMpCopy->copyTime[0] = 0xee;
           	 	 	  			       	  tmpMpCopy->copyTime[1] = 0xee;
           	 	 	  			       	  tmpMpCopy->copyEnergy[0] = 0xee;
           	 	 	  			       	  tmpMpCopy->next = NULL;
           	 	 	  			       	  
           	 	 	  			       	  if (mpCopyHead==NULL)
                                  {
                                  	mpCopyHead = tmpMpCopy;
                                  }
                                  else
                                  {
                                  	prevMpCopy->next = tmpMpCopy;
                                  }
                                  prevMpCopy = tmpMpCopy;
                                  
           	 	 	  			          tmpPrevMpLink = queryMpLink;
           	 	 	  			          queryMpLink = queryMpLink->next;
                                  
           	 	 	  			          multiCpMax++;
                                  free(tmpPrevMpLink);
           	 	 	  			       }
           	 	 	  			       
           	 	 	  			       if ((multiCpMax%(NUM_MP_PER_PAGE-2))!=0)
           	 	 	  			       {
           	 	 	  			       	  multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-2)+1;
           	 	 	  			       }
           	 	 	  			       else
           	 	 	  			       {
           	 	 	  			       	  multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-2);
           	 	 	  			       }
           	 	 	  			       
           	 	 	  			       if (mpCopyHead!=NULL)
           	 	 	  			       {
                          	      multiCpUpDown = 0;
                          	      
                          	      pDotCopy = (DOT_COPY *)malloc(sizeof(DOT_COPY));
                          	      prevMpCopy = mpCopyHead;
                          	      pDotCopy->dotCopyMp  = prevMpCopy->mp;
                          	      pDotCopy->dotCopying = FALSE;
                          	      pDotCopy->port       = PORT_POWER_CARRIER;
                          	      pDotCopy->dataFrom   = DOT_CP_ALL_MP;
                          	      pDotCopy->outTime    = nextTime(sysTime,0,22);
                          	      pDotCopy->dotResult  = RESULT_NONE;
           	 	 	  			       }
           	 	 	  			       
           	 	 	  			       keyLeftRight = 0xff;
           	 	 	  			       allMpCopy(keyLeftRight);
           	 	             	 }
           	 	             	 else
           	 	             	 {
           	 	             	 	  if (keyLeftRight!=0xff)
           	 	             	 	  {
           	 	             	 	    realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
           	 	             	 	  }
           	 	             	 }
           	 	             	 break;
           	 	          }
           	 	 	  		 	break;
           	 	 	  	}
           	 	 	  	break;
             	 	}
             	 	break;
      	    		 	 
      	    	case 12:  //�˵���12��(������˵���2��(����������鿴)��չ��Ч)
      	    		switch (layer2xMenuLight)
      	    	  {
      	    	  	case 0:  //�����鿴
      	    	  		layer2xMenuLight = 0;
      	    	  		layer2xMenu(1, layer2xMenuLight);
      	    	  		break;
      	    	  		
      	    	  	case 1:  //��������
      	    		 		pwLight = 0;
      	    		 	 	strcpy(passWord, "000000");
      	    		 	 	inputPassWord(pwLight);
      	    		 	 	break;
      	    		}
      	    		break;

      	    	case 13:  //�˵���13��(������˵���2��(����������鿴)��չ��Ч)
      	    		switch (layer2xMenuLight)
      	    	  {
      	    	  	case 0:    //��ѯ���������
      	    	  		queryMpLink = initPortMeterLink(0xff);
      	    	  		mpQueryType = 0;
      	    	  		mpQueryLight = 0;
      	    	  		
                    mpQueryPage = meterDeviceNum/(NUM_MP_PER_PAGE-1);
                    if (meterDeviceNum%(NUM_MP_PER_PAGE-1)!=0)
                    {
                   	  mpQueryPage++;
                    }

      	    	  		mpQueryMenu(mpQueryType, mpQueryLight);
      	    	  		break;
      	    	  		
      	    	  	case 1:    //��ѯͨ��ͨ������
      	    		 		commParaQueryMenu();
      	    		 	 	break;
      	    		 	
      	    		 #ifdef LIGHTING
      	    		 	case 2:    //��ѯ����ʱ��
	                  tmpCTimesNode = cTimesHead;
      	    		 		queryCTimes(tmpCTimesNode);
	                  break;
      	    		 #endif
      	    		}
      	    		break;
      	    		
      	    	case 20:  //�˵���20��(��������)��ȷ��
      	    		for(i=0;i<6;i++)
      	    		{
      	    		 	 if (originPassword[i]!=passWord[i])
      	    		 	 {
      	    		 	  	guiDisplay(30,120,"�����������!",1);
      	    		 	  	lcdRefresh(120,137);
      	    		 	  	return;
      	    		 	 }
      	    		}
      	    		 
      	    		//������������ǰ�ĸ����˵���,ִ����Ӧ�Ķ���
      	    		if (layer1MenuLight==1 || layer1MenuLight==2)
      	    		{
      	    			if (layer2MenuLight[layer1MenuLight]==0xff)
      	    			{
      	    				 layer2MenuLight[layer1MenuLight] = 0;
      	    			}
      	    			
      	    			if (layer1MenuLight==1 && carrierModuleType==RL_WIRELESS)
      	    			{
      	    				layer2MenuNum[1] = 10;
      	    			}
      	    			else
      	    			{
      	    			 #ifndef LIGHTING
      	    				layer2MenuNum[1] = 9;
      	    			 #endif
      	    			}
      	    			layer2Menu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	    		}
      	    }
      	    break;

      	  case KEY_CANCEL:   //ȡ��
      	    switch(menuInLayer)
      	    {
      	    	case 1:
      	    		defaultMenu();
      	    		break;
      	    		 	 
      	    	case 2:     //�˵���2��
      	    	case 12:    //�˵���12��(������˵���2��(����������鿴)��չ��Ч)
      	    	case 13:    //�˵���13��(������˵���2��(����������鿴)��չ��Ч)
      	    	case 20:    //�˵���20��,���������
      	    		layer1Menu(layer1MenuLight);
      	    		break;
      	    		 	 
      	    	case 3:     //�˵���3��
								if (3==layer1MenuLight)
								{
								  xlcStatus();
								}
								else
								{
								  layer2Menu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
								}
      	    		break;
            	
            	case 4:    //�˵���4��
             	 	switch(layer1MenuLight)
             	 	{
             	 	 	case 1: //ͨ�Ų�������
      	    		 	 	if (inputStatus==STATUS_SELECT_CHAR)
      	    		 	 	{
      	    		 	 	 	inputStatus = STATUS_NONE;
      	    		 	 	 	inputApn(inputIndex);
      	    		 	 	}
      	    		 	 	else
      	    		 	 	{
      	    		 	 	 	commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	}
             	 	 	  break;
             	 	 	
             	 	 	case 2: //�ն˹�����ά��
             	 	 	  switch(layer2MenuLight[layer1MenuLight])
             	 	 	  {
             	 	 	  	case 0:    //ʵʱ����
             	 	 	  	 #ifdef LIGHTING
             	 	 	  	  layer2Menu(layer2MenuLight[layer1MenuLight], layer1MenuLight);
             	 	 	  	 #else
             	 	 	  		realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  	 #endif
             	 	 	  		break;
             	 	 	  		 	
             	 	 	  	case 1:    //ȫ������㳭����
      	    		 	 	    layer2Menu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
             	 	 	  		break;
             	 	 	  }
             	 	 	  break;
             	 	}
             	 	break;
             	 
             	case 14:
             	  layer2xMenu(1, layer2xMenuLight);
             	  break;    
      	    }
      	    break;
      	    	
          case KEY_UP:    //����
           	 switch(menuInLayer)
             {
             	 case 1:
             	 	 if (layer1MenuLight==0)
             	 	 {
             	 	 	  layer1MenuLight = MAX_LAYER1_MENU-1;
             	 	 }
             	 	 else
             	 	 {
             	 	 	  layer1MenuLight--;
             	 	 }
             	 	 layer1Menu(layer1MenuLight);
             	   break;
             	   
             	 case 2:    //�˵���2��
             	 	 switch (layer1MenuLight)
             	 	 {
             	 	 	 case 0:    //�����ѯ
             	 	 	   switch(layer2MenuLight[layer1MenuLight])
             	 	 	   {
             	 	 	  		case 0: //1-1�������й������ѯ
             	 	 	  		 #ifdef LIGHTING
             	 	 	  		  ;    //����������Ϸ����ô���,��û��
             	 	 	  		 #else
             	 	 	  		 
             	 	 	  		 	stringUpDown(queryTimeStr, layer3MenuLight[0][0], 0);  //�������ַ�����һ���ַ�
             	 	 	  		 	copyQueryMenu(layer2MenuLight[0], layer3MenuLight[0][0]);
             	 	 	  		 
             	 	 	  		 #endif
             	 	 	  		  break;

             	 	 	  		default:
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
                            layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]--;
             	 	 	  		 	}
             	 	 	  		 	copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	break;
             	 	 	   }
             	 	 	   break;

             	 	 	 case 1:
             	 	 	 case 2:
       	  	 	 	     if (layer2MenuLight[layer1MenuLight] == 0 || layer2MenuLight[layer1MenuLight] == 0xff)
       	  	 	 	     {
       	  	 	 	       layer2MenuLight[layer1MenuLight] = layer2MenuNum[layer1MenuLight]-1;
       	  	 	 	     }
       	  	 	 	     else
       	  	 	 	     {
       	  	 	 	       layer2MenuLight[layer1MenuLight]--;
       	  	 	 	     }
       	  	 	 	     layer2Menu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
       	  	 	 	     break;
             	 	 }
             	 	 break;
             	 	 
             	 case 3:    //�˵���3��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 1:  //����������鿴
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		 case 0: //ͨ�Ų�������
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
             	 	 	  		 	   if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
             	 	 	  		 	   {
             	 	 	  		 	 	    for(i=strlen(commParaItem[1]);i<16;i++)
             	 	 	  		 	 	    {
             	 	 	  		 	 	    	 commParaItem[1][i] = ' ';
             	 	 	  		 	 	    }
             	 	 	  		 	 	    commParaItem[1][16] = '\0';
             	 	 	  		 	 	    inputStatus=STATUS_NONE;
             	 	 	  		 	 	    inputIndex = 0;
             	 	 	  		 	 	    inputApn(inputIndex); //APN�����봦��
             	 	 	  		 	   }
             	 	 	  		 	   else
             	 	 	  		 	   {
      	    		               stringUpDown(commParaItem[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
      	    		               commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		             }
      	    		           }
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 1: //̨������������
  	    		               stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
         	 	 	  		 	     {
         	 	 	  		 	     	 if (chrMp[1][0]>0x37)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[1][0] = 0x30;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
         	 	 	  		 	     {
         	 	 	  		 	     	 if (chrMp[2][0]>0x32)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[2][0] = 0x30;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
         	 	 	  		 	     {
         	 	 	  		 	     	 if (chrMp[3][0]>0x32)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[3][0] = 0x30;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
  	    		               set485Meter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 2: //��������������
         	 	 	  		 	    #ifdef LIGHTING
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
         	 	 	  		 	     {
  	    		                 stringUpDown(chrMp[4], keyLeftRight, 0);
         	 	 	  		 	     	 if (chrMp[4][0]>0x34)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[4][0] = 0x31;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
         	 	 	  		 	     else
         	 	 	  		 	     {
           	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
           	 	 	  		 	     {
    	    		                 stringUpDown(chrMp[3], keyLeftRight, 0);
           	 	 	  		 	     	 if (chrMp[3][0]>0x32)
           	 	 	  		 	     	 {
           	 	 	  		 	     	 	 chrMp[3][0] = 0x30;
           	 	 	  		 	     	 }
           	 	 	  		 	     }
           	 	 	  		 	     else
           	 	 	  		 	     {
             	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==4)
             	 	 	  		 	     {
      	    		                 stringUpDown(chrMp[2], keyLeftRight, 0);
             	 	 	  		 	     	 if (chrMp[2][0]>0x33)
             	 	 	  		 	     	 {
             	 	 	  		 	     	 	 chrMp[2][0] = 0x31;
             	 	 	  		 	     	 }
             	 	 	  		 	     }
             	 	 	  		 	     else
             	 	 	  		 	     {
  	    		                     stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
  	    		                   }
  	    		                 }
         	 	 	  		 	     }
         	 	 	  		 	    #else
  	    		               stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
         	 	 	  		 	     {
         	 	 	  		 	     	 if (chrMp[2][0]>0x31)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[2][0] = 0x30;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
         	 	 	  		 	    #endif
         	 	 	  		 	    
  	    		               setCarrierMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 3: //�ն�ʱ������
      	    		           stringUpDown(dateTimeItem,keyLeftRight,0);
      	    		           setTeDateTime(keyLeftRight);
             	 	 	  		 	 break;

             	 	 	  		 case 4: //�޸Ľ�������
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {                                 	    		           
      	    		             stringUpDown(commParaItem[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
      	    		             modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
             	 	 	  		 	 break;

             	 	 	  		 case 5: //�ն˱������
      	    		           stringUpDown(tmpTeAddr,keyLeftRight,0);
      	    		           setTeAddr(keyLeftRight);
             	 	 	  		 	 break;

             	 	 	  		 case 6: //��̫����������
      	    		           if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
      	    		           {
      	    		           	 if (chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]][0]==0x55)
      	    		           	 {
      	    		           	 	 chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]][0] = 0x0;
      	    		           	 }
      	    		           	 else
      	    		           	 {
      	    		           	   chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]][0] = 0x55;
      	    		           	 }
      	    		           }
      	    		           else
      	    		           {
      	    		             stringUpDown(chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
      	    		           }
      	    		           setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 
             	 	 	  		 case 7: //VPN�û�������
                   	 	 	   if (inputStatus==STATUS_SELECT_CHAR)
                   	 	 	   {
                   	 	 	  	 if (selectIndex>=14)
                   	 	 	  	 {
                   	 	 	  		 selectIndex -= 14;
                   	 	 	  	 }
                   	 	 	  	 else
                   	 	 	  	 {
                   	 	 	  		 selectIndex = 68;
                   	 	 	  	 }
                   	 	 	  	 selectChar(selectIndex);
                   	 	 	   }
                   	 	 	   if (inputStatus==STATUS_NONE)
                   	 	 	   {
                   	 	 	  	 selectIndex = 0;
                             selectChar(selectIndex);
                           }
                   	 	 	   break;

             	 	 	  		 case 8: //������������
  	    		               if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>1 && layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<5)
  	    		               {
  	    		               	 stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
  	    		               }
  	    		               else
  	    		               {
  	    		               	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
  	    		               	 {
  	    		               	 	  if (chrMp[0][0]!='3')
  	    		               	 	  {
  	    		               	 	  	 chrMp[0][0]='3';
  	    		               	 	  }
  	    		               	 }
  	    		               	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
  	    		               	 {
  	    		               	 	 if (chrMp[1][0]>0x31)
  	    		               	 	 {
  	    		               	 	  	chrMp[1][0]='0';
  	    		               	 	 }
  	    		               	 	 else
  	    		               	 	 {
  	    		               	 	  	chrMp[1][0]++;
  	    		               	 	 }
  	    		               	 }
  	    		               }
  	    		               setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 
             	 	 	  		 case 9: //���ģ���������
  	    		               stringUpDown(chrRlPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
  	    		               setRlPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2:  //�ն˹�����ά��
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //ʵʱ����
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
             	 	 	  		 	{
													  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1;
												  }
												  else
												  {
													  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]--;
												  }
												  realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
												  break;             	 	 	  		 	 
             	 	 	  		 	 
             	 	 	  		case 3:  //�������ܱ��ַ
           	 	 	  				if (multiCpUpDown==0)
           	 	 	  				{
           	 	 	  				 	multiCpUpDown =  multiCpMax-1;
           	 	 	  				}
           	 	 	  				else
           	 	 	  				{
           	 	 	  				 	multiCpUpDown--;
           	 	 	  				}
           	 	 	  				 
           	 	 	  				newAddMeter(keyLeftRight);
             	 	 	  		 	break;
             	 	 	  		 	 
             	 	 	  		case 4:   //�������ܱ���״̬
           	 	 	  				if (multiCpUpDown==0)
           	 	 	  				{
           	 	 	  				 	multiCpUpDown = multiCpMax-1;
           	 	 	  				}
           	 	 	  				else
           	 	 	  				{
           	 	 	  				 	multiCpUpDown--;
           	 	 	  				}
           	 	 	  				 
           	 	 	  				newMeterCpStatus(keyLeftRight);
             	 	 	  		 	break;
													
												case 5:
												 #ifdef LIGHTING
												 	if (0==keyLeftRight)
												 	{
												 	  keyLeftRight = 7;
												 	}
													else
												 	{
												 	 	keyLeftRight--;
												 	}
													setLearnIr(keyLeftRight);
												 #endif
													 break;
        	    	 	 	  	 
        	    	 	 	  	case 7:  //����ͨ��ģ�鳭����ʽ
        	    	 	 	  	  if (keyLeftRight==0)
        	    	 	 	  	  {
        	    	 	 	  	    keyLeftRight = 1;
        	    	 	 	  	  }
        	    	 	 	  	  else
        	    	 	 	  	  {
        	    	 	 	  	  	keyLeftRight = 0;
        	    	 	 	  	  }
        	    	 	 	  	  setCopyForm(keyLeftRight);
        	    	 	 	  	  break;

        	    	 	 	  	 case 11:  //�����û�����������
        	    	 	 	  	   if (keyLeftRight==0)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 2;
        	    	 	 	  	   }
        	    	 	 	  	   else
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight--;
        	    	 	 	  	   }
        	    	 	 	  	   setDenizenDataType(keyLeftRight);
        	    	 	 	  	   break;
        	    	 	 	  	 
        	    	 	 	  	 case 12:  //��������
        	    	 	 	  	   if (keyLeftRight==0)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 1;
        	    	 	 	  	   }
        	    	 	 	  	   else
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight--;
        	    	 	 	  	   }
        	    	 	 	  	   setCycleType(keyLeftRight);
        	    	 	 	  	   break;

      	    	 	 	  	   case 13:    //ά���ӿ�ģʽ����
        	    	 	 	  	   if (keyLeftRight==0)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 1;
        	    	 	 	  	   }
        	    	 	 	  	   else
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight--;
        	    	 	 	  	   }
      	    	 	 	  	  	 setMainTain(keyLeftRight);      	    	 	 	  	  	
      	    	 	 	  	  	 break;

      	    	 	 	  	   case 14:    //��2·485�ڹ�������
        	    	 	 	  	   if (keyLeftRight==0)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 1;
        	    	 	 	  	   }
        	    	 	 	  	   else
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight--;
        	    	 	 	  	   }
      	    	 	 	  	  	 setRs485Port2(keyLeftRight);      	    	 	 	  	  	
      	    	 	 	  	  	 break;
      	    	 	 	  	  	 
      	    	 	 	  	   case 15:    //����ͨ��ģ��Э������
        	    	 	 	  	   if (keyLeftRight==0)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 1;
        	    	 	 	  	   }
        	    	 	 	  	   else
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight--;
        	    	 	 	  	   }
      	    	 	 	  	  	 setLmProtocol(keyLeftRight);      	    	 	 	  	  	
      	    	 	 	  	  	 break;
             	 	 	  	}
             	 	 	  	break;
									  
										case 3:    //��·���Ƶ�״̬
									    if (0==keyLeftRight)
											{
												keyLeftRight = 4;	
											}
											else
											{
											  keyLeftRight--;	
											}
										  xlOpenClose(keyLeftRight);
									    break;
             	 	 }
             	 	 break;
             	 	 
            	 case 4:    //�˵���4��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 1: //����������鿴
             	 	 	  	if (inputStatus==STATUS_SELECT_CHAR)
             	 	 	  	{
             	 	 	  		if (selectIndex>=14)
             	 	 	  		{
             	 	 	  			 selectIndex -= 14;
             	 	 	  		}
             	 	 	  		else
             	 	 	  		{
             	 	 	  			 selectIndex = 68;
             	 	 	  	  }
             	 	 	  		selectChar(selectIndex);
             	 	 	  	}
             	 	 	  	if (inputStatus==STATUS_NONE)
             	 	 	  	{
             	 	 	  	  selectIndex = 0;
                        selectChar(selectIndex);
                      }
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2: //�ն˹�����ά��
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //ʵʱ����
             	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
             	 	 	  		 	{
             	 	 	  		 	 	case 0:  //ָ�������㳭��
             	 	 	  		 	 		stringUpDown(singleCopyMp, keyLeftRight, 0);
             	 	 	  		 	 		
             	 	 	  		 	 	 #ifdef LIGHTING	
             	 	 	  		 	 	  singleCcbCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	 #else
             	 	 	  		 	 	  singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	 #endif
             	 	 	  		 	 	  break;
             	 	 	  		 	 	  
             	 	 	  		 	 	case 1:
                 	 	 	  			if (keyLeftRight==0xff)
                 	 	 	  			{
                 	 	 	  				 if (multiCpUpDown==0)
                 	 	 	  				 {
                 	 	 	  				 	  multiCpUpDown=multiCpMax-1;
                 	 	 	  				 }
                 	 	 	  				 else
                 	 	 	  				 {
                 	 	 	  				 	  multiCpUpDown--;
                 	 	 	  				 }
                 	 	 	  				 
                 	 	 	  				 allMpCopy(keyLeftRight);
                 	 	 	  			}
                 	 	 	  			break;             	 	 	  		 	 		
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  		 	
             	 	 	  		case 1:  //ȫ�������㳭����
             	 	 	  		 	if (multiCpUpDown<1)
             	 	 	  		 	{
             	 	 	  		 	 	 multiCpUpDown=multiCpMax-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
             	 	 	  		 	 	 multiCpUpDown--;
             	 	 	  		 	}
             	 	 	  		 	allMpCopy(0xff);
             	 	 	  		 	break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;

      	    	 case 12:  //�˵���12��(������˵���2��(����������鿴)��չ��Ч)
      	    	 case 13:  //�˵���13��(������˵���2��(����������鿴)��չ��Ч)
      	    		 switch(layer1MenuLight)
      	    	   {
             	 	   case 1: //����������鿴
             	 	 		#ifdef LIGHTING
             	 	 		 if (13==menuInLayer)
             	 	 		 {
             	 	 		 	 if (layer2xMenuLight==0)
             	 	 		 	 {
             	 	 		 	 	 layer2xMenuLight = 2;
             	 	 		 	 }
             	 	 		 	 else
             	 	 		 	 {
             	 	 		 	   layer2xMenuLight--;
             	 	 		 	 }
             	 	 		 }
             	 	 		 else
             	 	 		 {
             	 	 		#endif
             	 	 		
             	 	 		   if (layer2xMenuLight==0)
             	 	 		   {
             	 	 		     layer2xMenuLight = 1;
             	 	 		   }
             	 	 		   else
             	 	 		   {
             	 	 			   layer2xMenuLight = 0;
             	 	 		   }
             	 	 		   
             	 	 		#ifdef LIGHTING
             	 	 		 }
             	 	 		#endif
             	 	 		
             	 	 		 if (menuInLayer==12)
             	 	 		 {
             	 	 		   layer2xMenu(0, layer2xMenuLight);
             	 	 		 }
             	 	 		 else
             	 	 		 {
             	 	 		   layer2xMenu(1, layer2xMenuLight);
             	 	 		 }
             	 	 		 break;
      	    	   }
      	    		 break;

      	    	 case 14:  //�˵���14��(������˵���2��(����������鿴)��չ��Ч)
      	    		 switch (layer2xMenuLight)
      	    	   {
      	    	  	 case 0:  //��ѯ���������
      	    	  		 if (mpQueryLight==0)
      	    	  		 {
      	    	  		    mpQueryLight = mpQueryPage-1;
      	    	  		 }
      	    	  		 else
      	    	  		 {
      	    	  		 	  mpQueryLight--;
      	    	  		 }
      	    	  		 mpQueryMenu(mpQueryType, mpQueryLight);
      	    	  		 break;
      	    	  		 
      	    		  #ifdef LIGHTING
      	    		 	 case 2:    //��ѯ����ʱ��
	                   if (tmpCTimesNode->next==NULL)
	                   {
	                     tmpCTimesNode = cTimesHead;
	                   }
	                   else
	                   {
	                     tmpCTimesNode = tmpCTimesNode->next;
	                   }
      	    		 		 queryCTimes(tmpCTimesNode);
	                   break;
      	    		  #endif
      	    	   }
      	    	   break;

             	 case 20:   //�˵���20��
             	 	 stringUpDown(passWord, pwLight,0);
             	 	 inputPassWord(pwLight);
             	 	 break;
             }
           	 break;
          
          case KEY_DOWN:    //����
           	switch(menuInLayer)
            {
             	 case 1:    //�˵���1��
             	 	 if (layer1MenuLight>=MAX_LAYER1_MENU-1)
             	 	 {
             	 	 	  layer1MenuLight = 0;
             	 	 }
             	 	 else
             	 	 {
             	 	 	  layer1MenuLight++;
             	 	 }
             	 	 layer1Menu(layer1MenuLight);
             	   break;

             	 case 2:    //�˵���2��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	 case 0:    //�����ѯ
             	 	 	   switch(layer2MenuLight[layer1MenuLight])
             	 	 	   {
             	 	 	  	 case 0: //1-1�������й������ѯ
             	 	 	  	 	#ifdef LIGHTING
             	 	 	  	 	 
             	 	 	  	 	 if (queryMpLink->next!=NULL)
             	 	 	  	 	 {
             	 	 	  	 	 	 queryMpLink = queryMpLink->next;
             	 	 	  	 	 }
             	 	 	  	 	 else
             	 	 	  	 	 {
             	 	 	  	 	 	 queryMpLink = copyCtrl[4].cpLinkHead;
             	 	 	  	 	 }
             	 	 	  	 	 ccbStatus();
             	 	 	  	 	 
             	 	 	  	 	#else
             	 	 	  	 	
             	 	 	  		 stringUpDown(queryTimeStr, layer3MenuLight[0][0], 1);  //�������ַ���Сһ������
             	 	 	  		 copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		
             	 	 	  		#endif
             	 	 	  		 break;
             	 	 	  	
             	 	 	  	 default:
             	 	 	  		 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>=layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
             	 	 	  		 {
             	 	 	  		 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
             	 	 	  		 }
             	 	 	  		 else
             	 	 	  		 {
                           layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
             	 	 	  		 }
             	 	 	  		 copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 break;
             	 	 	   }
             	 	 	   break;

             	 	 	 case 1:
             	 	 	 case 2:
       	  	 	 	     if (layer2MenuLight[layer1MenuLight]+1>layer2MenuNum[layer1MenuLight]-1)
       	  	 	 	     {
       	  	 	 	       layer2MenuLight[layer1MenuLight] = 0;
       	  	 	 	     }
       	  	 	 	     else
       	  	 	 	     {
       	  	 	 	       layer2MenuLight[layer1MenuLight]++;
       	  	 	 	     }
       	  	 	 	     layer2Menu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
       	  	 	 	     break;
       	  	 	 	     
             	 	 	#ifdef LIGHTING
             	 	 	 case 3:
             	 	 	   if (queryMpLink->next!=NULL)
             	 	 	   {
             	 	 	  	 queryMpLink = queryMpLink->next;
             	 	 	   }
             	 	 	   else
             	 	 	   {
             	 	 	   	 queryMpLink = xlcLink;
             	 	 	   }
             	 	 	   xlcStatus();             	 	 	  	 	 
             	 	 	   break;

             	 	 	 case 4:
             	 	 	   if (queryMpLink->next!=NULL)
             	 	 	   {
             	 	 	  	 queryMpLink = queryMpLink->next;
             	 	 	   }
             	 	 	   else
             	 	 	   {
             	 	 	   	 queryMpLink = ldgmLink;
             	 	 	   }
             	 	 	   ldgmStatus();             	 	 	  	 	 
             	 	 	   break;
             	 	 	   
             	 	 	 case 5:
             	 	 	   if (queryMpLink->next!=NULL)
             	 	 	   {
             	 	 	  	 queryMpLink = queryMpLink->next;
             	 	 	   }
             	 	 	   else
             	 	 	   {
             	 	 	   	 queryMpLink = lsLink;
             	 	 	   }
             	 	 	   lsStatus();
             	 	 	   break;
             	 	 	#endif
             	 	 	  		  
             	 	 }
             	 	 break;
             	 	 
             	 case 3:    //�˵���3��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 1:  //����������鿴
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		 case 0: //ͨ�Ų�������
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
             	 	 	  		 	   if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
             	 	 	  		 	   {
             	 	 	  		 	 	    //APN�����봦��
             	 	 	  		 	 	    for(i=strlen(commParaItem[1]);i<16;i++)
             	 	 	  		 	 	    {
             	 	 	  		 	 	    	 commParaItem[1][i] = ' ';
             	 	 	  		 	 	    }
             	 	 	  		 	 	    commParaItem[1][16] = '\0';
             	 	 	  		 	 	    inputStatus=STATUS_NONE;
             	 	 	  		 	 	    inputIndex = 0;
             	 	 	  		 	 	    inputApn(inputIndex);
             	 	 	  		 	   }
             	 	 	  		 	   else
             	 	 	  		 	   {
      	    		                stringUpDown(commParaItem[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
      	    		                commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		             }
      	    		           }
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 1: //̨������������
  	    		               stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
         	 	 	  		 	     {
         	 	 	  		 	     	 if (chrMp[1][0]>0x37)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[1][0] = 0x37;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
         	 	 	  		 	     {
         	 	 	  		 	     	 if (chrMp[2][0]>0x32)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[2][0] = 0x32;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
         	 	 	  		 	     {
         	 	 	  		 	     	 if (chrMp[3][0]>0x32)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[3][0] = 0x32;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
  	    		               set485Meter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 2: //��������������
         	 	 	  		 	    #ifdef LIGHTING
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
         	 	 	  		 	     {
  	    		                 stringUpDown(chrMp[4], keyLeftRight, 1);
         	 	 	  		 	     	 if (chrMp[4][0]<=0x31)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[4][0] = 0x34;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
         	 	 	  		 	     else
         	 	 	  		 	     {
           	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
           	 	 	  		 	     {
    	    		                 stringUpDown(chrMp[3], keyLeftRight, 1);
           	 	 	  		 	     	 if (chrMp[3][0]<0x30 || chrMp[3][0]>0x32)
           	 	 	  		 	     	 {
           	 	 	  		 	     	 	 chrMp[3][0] = 0x32;
           	 	 	  		 	     	 }
           	 	 	  		 	     }
           	 	 	  		 	     else
           	 	 	  		 	     {
             	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==4)
             	 	 	  		 	     {
      	    		                 stringUpDown(chrMp[2], keyLeftRight, 1);
             	 	 	  		 	     	 if (chrMp[2][0]<0x31)
             	 	 	  		 	     	 {
             	 	 	  		 	     	 	 chrMp[2][0] = 0x33;
             	 	 	  		 	     	 }
             	 	 	  		 	     }
             	 	 	  		 	     else
             	 	 	  		 	     {
             	 	 	  		 	     	 stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
             	 	 	  		 	     }
             	 	 	  		 	   }
         	 	 	  		 	     }
         	 	 	  		 	    #else 
  	    		               stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
         	 	 	  		 	     {
         	 	 	  		 	     	 if (chrMp[2][0]>0x31)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[2][0] = 0x31;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
         	 	 	  		 	    #endif
  	    		               setCarrierMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 3: //�ն�ʱ������
      	    		           stringUpDown(dateTimeItem,keyLeftRight,1);
      	    		           setTeDateTime(keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 4: //�޸Ľ�������
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
      	    		             stringUpDown(commParaItem[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
      	    		             modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 }
             	 	 	  		 	 break;

             	 	 	  		 case 5: //�ն˱������
      	    		           stringUpDown(tmpTeAddr,keyLeftRight,1);
      	    		           setTeAddr(keyLeftRight);
             	 	 	  		 	 break;

             	 	 	  		 case 6: //��̫����������
      	    		           if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
      	    		           {
      	    		           	  if (chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]][0]==0x55)
      	    		           	  {
      	    		           	  	chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]][0] = 0x0;
      	    		           	  }
      	    		           	  else
      	    		           	  {
      	    		           	  	chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]][0] = 0x55;
      	    		           	  }
      	    		           }
      	    		           else
      	    		           {
      	    		             stringUpDown(chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
      	    		           }
      	    		           setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 
             	 	 	  		 case 7:  //VPN�û�������
                 	 	 	  	 if (inputStatus==STATUS_SELECT_CHAR)
                 	 	 	  	 {
                 	 	 	  		 selectIndex += 14;
                 	 	 	  		 if (selectIndex>69)
                 	 	 	  		 {
                 	 	 	  			  selectIndex = 0;
                 	 	 	  		 }
                 	 	 	  		 selectChar(selectIndex);
                 	 	 	  	 }
                 	 	 	  	 if (inputStatus==STATUS_NONE)
                 	 	 	  	 {
                 	 	 	  	   selectIndex = 0;
                             selectChar(selectIndex);
                           }
                 	 	 	  	 break;
                 	 	 	  	 
             	 	 	  		 case 8: //������������
  	    		               if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>1 && layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<5)
  	    		               {
  	    		               	 stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
  	    		               }
  	    		               else
  	    		               {
  	    		               	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
  	    		               	 {
  	    		               	 	  if (chrMp[0][0]!='3')
  	    		               	 	  {
  	    		               	 	  	 chrMp[0][0]='3';
  	    		               	 	  }
  	    		               	 }
  	    		               	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
  	    		               	 {
  	    		               	 	 if (chrMp[1][0]=='0')
  	    		               	 	 {
  	    		               	 	  	chrMp[1][0]='2';
  	    		               	 	 }
  	    		               	 	 else
  	    		               	 	 {
  	    		               	 	  	chrMp[1][0]--;
  	    		               	 	 }
  	    		               	 }
  	    		               }
  	    		               setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 9: //���ģ���������
  	    		               stringUpDown(chrRlPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
  	    		               setRlPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2:  //�ն˹�����ά��
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		 case 0: //ʵʱ����
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>=layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
             	 	 	  		 	 {
             	 	 	  		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
             	 	 	  		 	 }
             	 	 	  		 	 else
             	 	 	  		 	 {
             	 	 	  		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
             	 	 	  		 	 }
             	 	 	  		 	 realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 3:  //�������ܱ��ַ
           	 	 	  				 if (multiCpUpDown>=(multiCpMax-1))
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown=0;
           	 	 	  				 }
           	 	 	  				 else
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown++;
           	 	 	  				 }
           	 	 	  				 
           	 	 	  				 newAddMeter(keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 4:   //�������ܱ���״̬
           	 	 	  				 if (multiCpUpDown>=(multiCpMax-1))
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown=0;
           	 	 	  				 }
           	 	 	  				 else
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown++;
           	 	 	  				 }
           	 	 	  				 
           	 	 	  				 newMeterCpStatus(keyLeftRight);
             	 	 	  		 	 break;
													 
												 case 5:
												 	#ifdef LIGHTING
												 	 if (keyLeftRight>=7)
												 	 {
												 	 	 keyLeftRight = 0;
												 	 }
													 else
												 	 {
												 	 	 keyLeftRight++;
												 	 }
													 setLearnIr(keyLeftRight);
												  #endif
													 break;

												 
        	    	 	 	  	 case 7:  //����ͨ��ģ�鳭����ʽ
        	    	 	 	  	   if (keyLeftRight==0)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 1;
        	    	 	 	  	   }
        	    	 	 	  	   else
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 0;
        	    	 	 	  	   }
        	    	 	 	  	   setCopyForm(keyLeftRight);
        	    	 	 	  	   break;

        	    	 	 	  	 case 11:  //�����û�����������
        	    	 	 	  	   keyLeftRight++;
        	    	 	 	  	   if (keyLeftRight>2)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 0;
        	    	 	 	  	   }
        	    	 	 	  	   setDenizenDataType(keyLeftRight);
        	    	 	 	  	   break;

        	    	 	 	  	 case 12:  //��������
        	    	 	 	  	   keyLeftRight++;
        	    	 	 	  	   if (keyLeftRight>1)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 0;
        	    	 	 	  	   }
        	    	 	 	  	   setCycleType(keyLeftRight);
        	    	 	 	  	   break;

      	    	 	 	  	   case 13:    //ά���ӿ�ģʽ����
        	    	 	 	  	   keyLeftRight++;
        	    	 	 	  	   if (keyLeftRight>1)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 0;
        	    	 	 	  	   }
      	    	 	 	  	  	 setMainTain(keyLeftRight);      	    	 	 	  	  	
      	    	 	 	  	  	 break;

      	    	 	 	  	   case 14:    //��2·485�ڹ�������
        	    	 	 	  	   keyLeftRight++;
        	    	 	 	  	   if (keyLeftRight>1)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 0;
        	    	 	 	  	   }
      	    	 	 	  	  	 setRs485Port2(keyLeftRight);      	    	 	 	  	  	
      	    	 	 	  	  	 break;
      	    	 	 	  	  	 
      	    	 	 	  	   case 15:    //����ͨ��ģ��Э������
        	    	 	 	  	   keyLeftRight++;
        	    	 	 	  	   if (keyLeftRight>1)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 0;
        	    	 	 	  	   }
      	    	 	 	  	  	 setLmProtocol(keyLeftRight);
      	    	 	 	  	  	 break;
             	 	 	  	}
             	 	 	  	break;
											
										case 3:    //��·���Ƶ�״̬
									    if (keyLeftRight>3)
											{
												keyLeftRight=0;	
											}
											else
											{
											  keyLeftRight++;	
											}
										  xlOpenClose(keyLeftRight);
									    break;
             	 	 }
             	 	 break;
             	 	 
            	 case 4:    //�˵���4��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 1:   //����������鿴
             	 	 	  	if (inputStatus==STATUS_SELECT_CHAR)
             	 	 	  	{
             	 	 	  		selectIndex += 14;
             	 	 	  		if (selectIndex>69)
             	 	 	  		{
             	 	 	  			 selectIndex = 0;
             	 	 	  		}
             	 	 	  		selectChar(selectIndex);
             	 	 	  	}
             	 	 	  	if (inputStatus==STATUS_NONE)
             	 	 	  	{
             	 	 	  	  selectIndex = 0;
                        selectChar(selectIndex);
                      }
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2: //�ն˹�����ά��
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //ʵʱ����
             	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
             	 	 	  		 	{
             	 	 	  		 	 	case 0:  //ָ�������㳭��
             	 	 	  		 	 		stringUpDown(singleCopyMp, keyLeftRight, 1);
             	 	 	  		 	 	 #ifdef LIGHTING
             	 	 	  		 	 	  singleCcbCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	 #else
             	 	 	  		 	 	  singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	 #endif
             	 	 	  		 	 	  break;
             	 	 	  		 	 	  
             	 	 	  		 	 	case 1:
                 	 	 	  			if (keyLeftRight==0xff)
                 	 	 	  			{
                 	 	 	  				 if (multiCpUpDown>=(multiCpMax-1))
                 	 	 	  				 {
                 	 	 	  				 	  multiCpUpDown=0;
                 	 	 	  				 }
                 	 	 	  				 else
                 	 	 	  				 {
                 	 	 	  				 	  multiCpUpDown++;
                 	 	 	  				 }
                 	 	 	  				 
                 	 	 	  				 allMpCopy(keyLeftRight);
                 	 	 	  			}
                 	 	 	  			break;
             	 	 	  		 	}
             	 	 	  		 	break;

             	 	 	  		case 1:  //ȫ�������㳭����
             	 	 	  		 	if (multiCpUpDown>=multiCpMax-1)
             	 	 	  		 	{
             	 	 	  		 	 	 multiCpUpDown=0;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
             	 	 	  		 	 	 multiCpUpDown++;
             	 	 	  		 	}
             	 	 	  		 	allMpCopy(0xff);
             	 	 	  		 	break;

             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;
             	 	 
      	    	 case 12:  //�˵���12��(������˵���2��(����������鿴)��չ��Ч)
      	    	 case 13:  //�˵���13��(������˵���2��(����������鿴)��չ��Ч)
      	    	 	 switch(layer1MenuLight)
      	    	   {
             	 	   case 1: //����������鿴
             	 	 		#ifdef LIGHTING
             	 	 		 if (13==menuInLayer)
             	 	 		 {
             	 	 		 	 layer2xMenuLight++;
             	 	 		 	 if (layer2xMenuLight>2)
             	 	 		 	 {
             	 	 		 	 	 layer2xMenuLight = 0;
             	 	 		 	 }
             	 	 		 }
             	 	 		 else
             	 	 		 {
             	 	 		#endif

             	 	 	 	   if (layer2xMenuLight==0)
             	 	 		   {
             	 	 		 	   layer2xMenuLight = 1;
             	 	 		   }
             	 	 		   else
             	 	 		   {
             	 	 			   layer2xMenuLight = 0;
             	 	 		   }
             	 	 		   
             	 	 		#ifdef LIGHTING
             	 	 		 }
             	 	 		#endif
             	 	 		 
             	 	 		 if (menuInLayer==12)
             	 	 		 {
             	 	 		   layer2xMenu(0, layer2xMenuLight);
             	 	 		 }
             	 	 		 else
             	 	 		 {
             	 	 		   layer2xMenu(1, layer2xMenuLight);
             	 	 		 }
             	 	 	 	 break;
      	    	   }
      	    		 break;

      	    	 case 14:  //�˵���14��(������˵���2��(����������鿴)��չ��Ч)
      	    		 switch (layer2xMenuLight)
      	    	   {
      	    	  	 case 0:  //��ѯ���������
      	    	  		 if (mpQueryLight>=(mpQueryPage-1))
      	    	  		 {
      	    	  		   mpQueryLight = 0;
      	    	  		 }
      	    	  		 else
      	    	  		 {
      	    	  		 	 mpQueryLight++;
      	    	  		 }
      	    	  		 mpQueryMenu(mpQueryType, mpQueryLight);
      	    	  		 break;
      	    	  		 
      	    		  #ifdef LIGHTING
      	    		 	 case 2:    //��ѯ����ʱ��
	                   if (tmpCTimesNode->next!=NULL)
	                   {
	                     tmpCTimesNode = tmpCTimesNode->next;
	                   }
	                   else
	                   {
	                     tmpCTimesNode = cTimesHead;
	                   }
      	    		 		 queryCTimes(tmpCTimesNode);
	                   break;
      	    		  #endif

      	    	   }
      	    	   break;

             	 case 20:   //�˵���20��
             	 	 stringUpDown(passWord, pwLight,1);
             	 	 inputPassWord(pwLight);
             	 	 break;
            }
            break;
             
          case KEY_RIGHT:   //����
           	switch(menuInLayer)
            {
             	case 1:    //�˵���1��
             		if (layer1MenuLight>=MAX_LAYER1_MENU-1)
             	 	{
             	 	 	layer1MenuLight = 0;
             	 	}
             	 	else
             	 	{
             	 	 	layer1MenuLight++;
             	 	}
             	 	layer1Menu(layer1MenuLight);
             	  break;

             	case 2:    //�˵���2��
             	 	switch(layer1MenuLight)
             	 	{
             	 	 	case 0:    //�����ѯ
             	 	 	  switch(layer2MenuLight[layer1MenuLight])
             	 	 	  {
             	 	 	  	case 0: //1-1�������й������ѯ
             	 	 	  	 #ifdef LIGHTING
             	 	 	  	  ;
             	 	 	  	 #else	
             	 	 	  		if (layer3MenuLight[0][0]>=(layer3MenuNum[0][0]-1))
             	 	 	  		{
             	 	 	  		 	layer3MenuLight[0][0] = 0;
             	 	 	  		}
             	 	 	  		else
             	 	 	  		{
             	 	 	  		 	layer3MenuLight[0][0]++;             	 	 	  		 		
             	 	 	  		}
             	 	 	  		copyQueryMenu(layer2MenuLight[0],layer3MenuLight[0][0]);
             	 	 	  	 #endif
             	 	 	  		break;
             	 	 	  }
             	 	 	  break;
             	 	}
             	  break;             	   

             	case 3:    //�˵���3��
             	 	switch(layer1MenuLight)
             	 	{
             	 	 	  case 1:       //����������鿴
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		 case 0:  //ͨ����������
      	    		         	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
      	    		         	 {                           
                             adjustCommParaLight(1);
      	    		             commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
      	    		           break;

             	 	 	  		 case 1:  //̨������������
                           adjustSetMeterParaLight(1,1);
      	    		           set485Meter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           break;

             	 	 	  		 case 2:  //��������������
                           adjustSetMeterParaLight(1, 2);
      	    		           setCarrierMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           break;
      	    		           
      	    		         case 3:  //�ն�ʱ������
                           if (keyLeftRight>=13)
                       	   {
                       	 	   keyLeftRight = 0;
                       	   }
                       	   else
                       	   {
                       	 	   keyLeftRight++;
                       	   }
      	    		           setTeDateTime(keyLeftRight);
      	    		           break;
      	    		           
      	    		         case 4:  //���������޸�
      	    		         	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
      	    		         	 {
                             if (keyLeftRight>=5)
                       	     {
                       	 	     keyLeftRight = 0;
                       	     }
                       	     else
                       	     {
                       	 	     keyLeftRight++;
                       	     }
      	    		             modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
      	    		           break;

      	    		         case 5:  //�ն˱������
                           #ifdef TE_ADDR_USE_BCD_CODE
                            if (keyLeftRight>=7)
                           #else
                            if (keyLeftRight>=8)
                           #endif
                       	   {
                       	 	   keyLeftRight = 0;
                       	   }
                       	   else
                       	   {
                       	 	   keyLeftRight++;
                       	   }
      	    		           setTeAddr(keyLeftRight);
      	    		           break;
      	    		           
      	    		         case 6:  //��̫����������
      	    		         	 adjustEthParaLight(1);
      	    		         	 setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);      	    		         	 
      	    		         	 break;
      	    		         
      	    		         case 7:  //����ר���û�������
                 	 	 	  	 if (inputStatus==STATUS_NONE)
                 	 	 	  	 {
                 	 	 	  	   if (keyLeftRight<63)
                 	 	 	  	   {
                 	 	 	  		   keyLeftRight++;
                 	 	 	  	   }
                 	 	 	  	   else
                 	 	 	  	   {
                 	 	 	  		   keyLeftRight = 0;
                 	 	 	  	   }
                 	 	 	  	   setVpn(keyLeftRight);
                 	 	 	  	 }
                 	 	 	  	 else
                 	 	 	  	 {
                 	 	 	  	   if (selectIndex<69)
                 	 	 	  	   {
                 	 	 	  		   selectIndex++;
                 	 	 	  	   }
                 	 	 	  	   else
                 	 	 	  	   {
                 	 	 	  		   selectIndex = 0;
                 	 	 	  	   }
                 	 	 	  	   selectChar(selectIndex);
                 	 	 	  	 }
      	    		           break;

      	    		         case 8:  //�ն˼�����������
      	    		         	 adjustCasParaLight(1);
      	    		           setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		         	 break;

             	 	 	  		 case 9:  //��β�����������
                           adjustSetMeterParaLight(1, 6);
      	    		           setRlPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           break;
      	    		      }      	    		      
      	    		      break;
      	    		      
      	    		    case 2:       //�ն˹�����ά��
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 2:  //�������
             	 	 	  		 #ifdef LIGHTING    //·�Ƽ����������ǡ�����Һ���Աȶȡ�

      	    		 	 	 	  	if (lcdDegree<20)
      	    		 	 	 	  	{
      	    		 	 	 	  		 lcdDegree++;
      	    		 	 	 	  	}
      	    	 	 	  	  	setLcdDegree(lcdDegree);

             	 	 	  		 #else
             	 	 	  		 	
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  searchMeter(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	
             	 	 	  		 #endif
             	 	 	  		 	break;

             	 	 	  		case 3:  //�������ܱ��ַ
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  newAddMeter(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  		 	
             	 	 	  		case 5: 
       	    		 	 	 	   if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-2)
      	    		 	 	 	   {
      	    		 	 	 	   	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
      	    		 	 	 	   }
      	    		 	 	 	   else
      	    		 	 	 	   {
      	    		 	 	 	  	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	 	   }
      	    	 	 	  		 terminalInfo(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    	 	 	  		 break;
             	 	 	  		 	
      	    	 	 	  	  case 6:  //����LCD�Աȶ�
      	    		 	 	 	  	if (lcdDegree<20)
      	    		 	 	 	  	{
      	    		 	 	 	  		 lcdDegree++;
      	    		 	 	 	  	}
      	    	 	 	  	  	setLcdDegree(lcdDegree);
      	    	 	 	  	  	break;
      	    		      }
      	    		    	break;
      	    		}
             	 	break;
             	 	 
             	case 4:    //�˵���4��
             	  switch(layer1MenuLight)
             	  {
             	 	 	  case 1: //����������鿴
             	 	 	  	if (inputStatus==STATUS_NONE)
             	 	 	  	{
             	 	 	  	  if (inputIndex<15)
             	 	 	  	  {
             	 	 	  		   inputIndex++;
             	 	 	  	  }
             	 	 	  	  else
             	 	 	  	  {
             	 	 	  		   inputIndex = 0;
             	 	 	  	  }
             	 	 	  	  inputApn(inputIndex);
             	 	 	  	}
             	 	 	  	else
             	 	 	  	{
             	 	 	  	  if (selectIndex<69)
             	 	 	  	  {
             	 	 	  		   selectIndex++;
             	 	 	  	  }
             	 	 	  	  else
             	 	 	  	  {
             	 	 	  		   selectIndex = 0;
             	 	 	  	  }
             	 	 	  	  selectChar(selectIndex);
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2: //�ն˹�����ά��
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //ʵʱ����
             	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
             	 	 	  		 	{
             	 	 	  		 	 	case 0:  //ָ�������㳭��
             	 	 	  		 	 		if (keyLeftRight>=3)
             	 	 	  		 	 	  {
             	 	 	  		 	 	  	keyLeftRight = 0;
             	 	 	  		 	 	  }
             	 	 	  		 	 	  else
             	 	 	  		 	 	  {
             	 	 	  		 	 	  	keyLeftRight++;
             	 	 	  		 	 	  }
             	 	 	  		 	 	  
             	 	 	  		 	 	 #ifdef LIGHTING
             	 	 	  		 	 	  singleCcbCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	 #else
             	 	 	  		 	 	  singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	 #endif
             	 	 	  		 	 	  break;
             	 	 	  		 	 	
             	 	 	  		 	 	case 1:  //ȫ�������㳭��
             	 	 	  		 	 		if (keyLeftRight!=0xff)
             	 	 	  		 	 		{
             	 	 	  		 	 		  if (keyLeftRight>=1)
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight = 0;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    else
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight++;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    allMpCopy(keyLeftRight);
             	 	 	  		 	 	  }
             	 	 	  		 	 		break;
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  	}
             	 	 	  	break;
             	 	}
             	 	break;
             	 	 
      	    	case 14:  //�˵���14��(������˵���2��(����������鿴)��չ��Ч)
      	    		switch (layer2xMenuLight)
      	    	  {
      	    	  	 case 0:  //��ѯ���������
      	    	  		#ifdef LIGHTING
      	    	  		 if (mpQueryType==0)
      	    	  		 {
      	    	  		   mpQueryType = 2;
      	    	  		 }
      	    	  		 else
      	    	  		 {
      	    	  		 	 mpQueryType = 0;
      	    	  		 }
      	    	  		#else
      	    	  		 if (mpQueryType>1)
      	    	  		 {
      	    	  		   mpQueryType = 0;
      	    	  		 }
      	    	  		 else
      	    	  		 {
      	    	  		 	 mpQueryType++;
      	    	  		 }
      	    	  		#endif
      	    	  		 mpQueryMenu(mpQueryType, mpQueryLight);
      	    	  		 break;
      	    	  }
      	    	  break;
      	    	   
             	case 20:   //�˵���20��(��������)
      	    		if (pwLight>=5)
      	    		{
      	    		   pwLight = 0;
      	    		}
      	    		else
      	    		{
      	    		 	 pwLight++;
      	    		}      	    		 
      	    		inputPassWord(pwLight);
             	 	break;
            }
           	break;
           	 
          case KEY_LEFT:  //����
           	switch(menuInLayer)
            {
             	 case 1:
             	 	 if (layer1MenuLight==0)
             	 	 {
             	 	 	  layer1MenuLight = MAX_LAYER1_MENU-1;
             	 	 }
             	 	 else
             	 	 {
             	 	 	  layer1MenuLight--;
             	 	 }
             	 	 layer1Menu(layer1MenuLight);
             	   break;
             	   
             	 case 2:    //�˵���2��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 0:    //�����ѯ
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //1-1�������й������ѯ
             	 	 	  		 #ifdef LIGHTING
             	 	 	  		  ;
             	 	 	  		 #else
             	 	 	  		 	if (layer3MenuLight[0][0]==0)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[0][0] = layer3MenuNum[0][0]-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[0][0]--;
             	 	 	  		 	}
             	 	 	  		 	copyQueryMenu(layer2MenuLight[0],layer3MenuLight[0][0]);
             	 	 	  		 #endif
             	 	 	  		  break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;
             	   
             	 case 3:    //�˵���3��
                 switch(layer1MenuLight)
                 {
                 	  case 1:  //��������
                 	  	switch(layer2MenuLight[layer1MenuLight])
                 	  	{
                 	  		 case 0: //ͨ�Ų�������
                           if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
                             adjustCommParaLight(0);
      	    		             commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
      	    		           break;

                 	  		 case 1: //̨������������
                           adjustSetMeterParaLight(0,1);
      	    		           set485Meter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           break;

                 	  		 case 2: //��������������
                           adjustSetMeterParaLight(0,2);
      	    		           setCarrierMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           break;
      	    		           
      	    		         case 3: //�ն�ʱ������
                           if (keyLeftRight==0)
                       	   {
                       	 	   keyLeftRight = 13;
                       	   }
                       	   else
                       	   {
                       	 	   keyLeftRight--;
                       	   }
      	    		           setTeDateTime(keyLeftRight);
      	    		           break;
      	    		           
      	    		         case 4: //�޸Ľ�������
                           if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
                             if (keyLeftRight==0)
                       	     {
                       	 	     keyLeftRight = 5;
                       	     }
                       	     else
                       	     {
                       	 	     keyLeftRight--;
                       	     }
      	    		             modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
      	    		           break;

      	    		         case 5: //�ն˱������
                           if (keyLeftRight==0)
                       	   {
                       	 	   #ifdef TE_ADDR_USE_BCD_CODE
                       	 	    keyLeftRight = 7;
                       	 	   #else
                       	 	    keyLeftRight = 8;
                       	 	   #endif
                       	   }
                       	   else
                       	   {
                       	 	   keyLeftRight--;
                       	   }
      	    		           setTeAddr(keyLeftRight);
      	    		           break;
      	    		         
      	    		         case 6:  //��̫����������
      	    		         	 adjustEthParaLight(0);
      	    		         	 setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);      	    		         	 
      	    		         	 break;
      	    		         	 
      	    		         case 7: //����ר���û�������
                	 	 	  	 if (inputStatus==STATUS_NONE)
                	 	 	  	 {
                             if (keyLeftRight==0)
                         	   {
                         	 	   keyLeftRight = 63;
                         	   }
                         	   else
                         	   {
                         	 	   keyLeftRight--;
                         	   }
        	    		           setVpn(keyLeftRight);
                	 	 	  	 }
                	 	 	  	 else
                	 	 	  	 {
                	 	 	  	   if (selectIndex==0)
                	 	 	  	   {
                	 	 	  		    selectIndex=68;
                	 	 	  	   }
                	 	 	  	   else
                	 	 	  	   {
                	 	 	  		    selectIndex--;
                	 	 	  	   }
                	 	 	  	   selectChar(selectIndex);
                	 	 	  	 }
      	    		           break;
      	    		         
      	    		         case 8:  //�ն˼�����������
      	    		         	 adjustCasParaLight(0);
      	    		           setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		         	 break;
      	    		         	 
             	 	 	  		 case 9:  //��β�����������
                           adjustSetMeterParaLight(0, 6);
      	    		           setRlPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           break;
      	    		      }
      	    		      break;
      	    		      
      	    		    case 2:       //�ն˹�����ά��
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 2:  //�������
             	 	 	  		 #ifdef LIGHTING    //·�Ƽ����������ǡ�����Һ���Աȶȡ�

      	    		 	 	 	  	if (lcdDegree>0)
      	    		 	 	 	  	{
      	    		 	 	 	  		lcdDegree--;
      	    		 	 	 	  	}
      	    	 	 	  	  	setLcdDegree(lcdDegree);

             	 	 	  		 #else
             	 	 	  		 	
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	   	keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  searchMeter(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	
             	 	 	  		 #endif
             	 	 	  		 	break;
             	 	 	  		 	
             	 	 	  		case 3:  //�������ܱ��ַ
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	   	keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  newAddMeter(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	break;
    	    	 	 	  		 
    	    	 	 	  		 case 5:
      	    		 	 	 	   if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>1 )
      	    		 	 	 	   {
      	    		 	 	 	   	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]--;
      	    		 	 	 	   }
      	    		 	 	 	   else
      	    		 	 	 	   {
      	    		 	 	 	  	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1;
      	    		 	 	 	   }
      	    	 	 	  		 terminalInfo(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    	 	 	  		 break;
      	    	 	 	  		 
      	    	 	 	  	  case 6:  //����LCD�Աȶ�
      	    		 	 	 	  	if (lcdDegree>0)
      	    		 	 	 	  	{
      	    		 	 	 	  		 lcdDegree--;
      	    		 	 	 	  	}
      	    	 	 	  	  	setLcdDegree(lcdDegree);
      	    	 	 	  	  	break;
      	    		      }
      	    		    	break;
      	    		 }
             	 	 break;
             	 	 
             	 case 4:    //�˵���4��
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 1: //����������鿴
             	 	 	  	if (inputStatus==STATUS_NONE)
             	 	 	  	{
             	 	 	  	  if (inputIndex==0)
             	 	 	  	  {
             	 	 	  		  inputIndex = 15;
             	 	 	  	  }
             	 	 	  	  else
             	 	 	  	  {
             	 	 	  		  inputIndex--;
             	 	 	  	  }
             	 	 	  	  inputApn(inputIndex);
             	 	 	  	}
             	 	 	  	else
             	 	 	  	{
             	 	 	  	  if (selectIndex==0)
             	 	 	  	  {
             	 	 	  		   selectIndex=68;
             	 	 	  	  }
             	 	 	  	  else
             	 	 	  	  {
             	 	 	  		   selectIndex--;
             	 	 	  	  }
             	 	 	  	  selectChar(selectIndex);
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2: //�ն˹�����ά��
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //ʵʱ����
             	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
             	 	 	  		 	{
             	 	 	  		 	 	case 0:  //ָ�������㳭��
             	 	 	  		 	 		if (keyLeftRight==0)
             	 	 	  		 	 	  {
             	 	 	  		 	 	  	keyLeftRight = 3;
             	 	 	  		 	 	  }
             	 	 	  		 	 	  else
             	 	 	  		 	 	  {
             	 	 	  		 	 	  	keyLeftRight--;
             	 	 	  		 	 	  }
             	 	 	  		 	 	 #ifdef LIGHTING
             	 	 	  		 	 	  singleCcbCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	 #else
             	 	 	  		 	 	  singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	 #endif
             	 	 	  		 	 	  break;
             	 	 	  		 	 	  
             	 	 	  		 	 	case 1:  //ȫ�������㳭��
             	 	 	  		 	 		if (keyLeftRight!=0xff)
             	 	 	  		 	 		{
             	 	 	  		 	 		  if (keyLeftRight==0)
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight = 1;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    else
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight--;
             	 	 	  		 	 	    }
             	 	 	  		 	 		  allMpCopy(keyLeftRight);
             	 	 	  		 	 		}
             	 	 	  		 	 		break;
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;
             	 	 
      	    	 case 14:  //�˵���14��(������˵���2��(����������鿴)��չ��Ч)
      	    		 switch (layer2xMenuLight)
      	    	   {
      	    	  	 case 0:  //��ѯ���������
      	    	  		#ifdef LIGHTING
      	    	  		 if (mpQueryType==0)
      	    	  		 {
      	    	  		   mpQueryType = 2;
      	    	  		 }
      	    	  		 else
      	    	  		 {
      	    	  		 	 mpQueryType = 0;
      	    	  		 }
      	    	  		#else
      	    	  		 if (mpQueryType==0)
      	    	  		 {
      	    	  		   mpQueryType = 2;
      	    	  		 }
      	    	  		 else
      	    	  		 {
      	    	  		 	 mpQueryType--;
      	    	  		 }
      	    	  		#endif
      	    	  		 mpQueryMenu(mpQueryType, mpQueryLight);
      	    	  		 break;
      	    	   }
      	    	   break;

             	 case 20:   //�˵���20��(��������)
      	    		 if (pwLight==0)
      	    		 {
      	    		   pwLight = 5;
      	    		 }
      	    		 else
      	    		 {
      	    		 	 pwLight--;
      	    		 }
      	    		 inputPassWord(pwLight);
             	 	 break;
            }
           	break;              	
        }
      }
   }
   else
   {
      keyPressCount = 0;
   }
}

/*******************************************************
��������:defaultMenu
��������:Ĭ�ϲ˵�(������,����376.1��������Լ�˵�)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void defaultMenu(void)
{
   if (displayMode==CYCLE_DISPLAY_MODE)
   {
   	  return;
   }
   
   menuInLayer = 0;
   guiLine(1,17,160,144,0);

  #ifdef LIGHTING
   guiDisplay(40,30,"����������",1);
  #else
   guiDisplay(24,30,"��ѹ����������",1);
  #endif
   
   displayMode = DEFAULT_DISPLAY_MODE;
   lcdRefresh(17,145);
}

/*******************************************************
��������:cycleMenu
��������:���Բ˵�(����376.1��������Լ�˵�)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void cycleMenu(void)
{
   char      str[20];
   char      say[20];
   DATE_TIME tmpTime;   
   INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
   BOOL      buffHasData=FALSE;
 	 INT32U    integer,decimal;
 	 INT16U    offset;
 	 INT32U    disData;
 	 INT8U     sign;

   if (tmpCycleLink==NULL)
   {
   	 return;
   }

   //���������ʾ
   guiLine(85, 1,117,16,0);
   if (tmpCycleLink->mp<100)
   {
     guiLine(100, 1,100,16,1);
     guiLine(117, 1,117,16,1);
     guiLine(100, 1,117, 1,1);
     guiLine(100,16,117,16,1);
     
     strcpy(say,"");
     if (tmpCycleLink->mp<10)
     {
     	 strcat(say,"0");
     }
     strcat(say,intToString(tmpCycleLink->mp,3,str));
     guiDisplay(102,1,say,1);
   }
   else
   {
     if (tmpCycleLink->mp<1000)
     {
       guiLine( 93, 1, 93,16,1);
       guiLine(117, 1,117,16,1);
       guiLine( 93, 1,117, 1,1);
       guiLine( 93,16,117,16,1);
       guiDisplay(94,1,intToString(tmpCycleLink->mp,3,str),1); 
     }
     else
     {
       guiLine( 85, 1, 85,16,1);
       guiLine(117, 1,117,16,1);
       guiLine( 85, 1,117, 1,1);
       guiLine( 85,16,117,16,1);
       guiDisplay(86,1,intToString(tmpCycleLink->mp,3,str),1);
     }
   }

   guiLine(1,17,160,144,0);
   guiLine(1,18,1,140,1);
   guiLine(160,18,160,140,1);
   guiLine(1,18,160,18,1);
   guiLine(1,140,160,140,1);
   
   if (tmpCycleLink->port==31)
   {
     guiDisplay( 2,LCD_LINE_1+6,"�й���=",1);
     guiDisplay( 2,LCD_LINE_2+6,"��ѹA    B    C   ",1);
     guiDisplay( 2,LCD_LINE_3+6,"����A    B    C   ",1);
     guiDisplay( 2,LCD_LINE_4+6,"����������=",1);
   }
   else
   {
  	 tmpTime = queryCopyTime(tmpCycleLink->mp);

     //2014-04-17,�ĳɶ�ȡ��������һ������
     //buffHasData = readMeterData(dataBuff, tmpCycleLink->mp, PRESENT_DATA, PARA_VARIABLE_DATA, &tmpTime, 0);   	 
     buffHasData = readMeterData(dataBuff, tmpCycleLink->mp, LAST_TODAY, PARA_VARIABLE_DATA, &tmpTime, 0);   	 

     if (buffHasData==TRUE)
     {	
       strcpy(say,"�й���=");
       if (dataBuff[POWER_INSTANT_WORK]!=0xee)
       {
         disData = dataBuff[POWER_INSTANT_WORK] | dataBuff[POWER_INSTANT_WORK+1]<<8 | dataBuff[POWER_INSTANT_WORK+2]<<16;
         sign = 0;
         if (dataBuff[POWER_INSTANT_WORK+2]&0x80)
         {
          	sign = 1;
          	disData &=0x7fffff;
         }
         
         decimal = (disData>>12 & 0xf)*1000
                 +(disData>>8 & 0xf)*100
                 +(disData>>4 & 0xf)*10
                 +(disData & 0xf);
         integer = (disData>>20 & 0xf)*10+(disData>>16 & 0xf);
         if (sign==1)
         {
         	 strcat(say,"-");
         }
         strcat(say,floatToString(integer,decimal,4,4,str));
       }       
       guiDisplay( 2,LCD_LINE_1+6,say,1);
       
       strcpy(say,"��ѹA");
       if (dataBuff[VOLTAGE_PHASE_A]!=0xee && dataBuff[VOLTAGE_PHASE_A]!=0xff)
       {
	       disData = dataBuff[VOLTAGE_PHASE_A] | dataBuff[VOLTAGE_PHASE_A+1]<<8;
         decimal = disData& 0xf;
         integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10 + (disData>>4 & 0xf);
         strcat(say,floatToString(integer,0,1,0,str));
       }
       guiDisplay( 2,LCD_LINE_2+6,say,1);

       strcpy(say,"B");
       if (dataBuff[VOLTAGE_PHASE_B]!=0xee && dataBuff[VOLTAGE_PHASE_B]!=0xff)
       {
	       disData = dataBuff[VOLTAGE_PHASE_B] | dataBuff[VOLTAGE_PHASE_B+1]<<8;
         decimal = disData& 0xf;
         integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10 + (disData>>4 & 0xf);
         strcat(say,floatToString(integer,0,1,0,str));
       }
       guiDisplay(76,LCD_LINE_2+6,say,1);

       strcpy(say,"C");
       if (dataBuff[VOLTAGE_PHASE_C]!=0xee && dataBuff[VOLTAGE_PHASE_C]!=0xff)
       {
	       disData = dataBuff[VOLTAGE_PHASE_C] | dataBuff[VOLTAGE_PHASE_C+1]<<8;
         decimal = disData& 0xf;
         integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10 + (disData>>4 & 0xf);
         strcat(say,floatToString(integer,0,1,0,str));
       }
       guiDisplay(120,LCD_LINE_2+6,say,1);
       
       strcpy(say,"����A");
       if (dataBuff[CURRENT_PHASE_A]!=0xee && dataBuff[CURRENT_PHASE_A]!=0xff)
       {
	       disData = dataBuff[CURRENT_PHASE_A] | dataBuff[CURRENT_PHASE_A+1]<<8 | (dataBuff[CURRENT_PHASE_A+2]&0x7f)<<16;
         decimal = (disData>>8 & 0xf)*100+(disData>>4 & 0xf)*10 + (disData& 0xf);
         integer = (disData>>20 & 0xf)*100+(disData>>16 & 0xf)*10 + (disData>>12 & 0xf);
         strcat(say,floatToString(integer,decimal,3,2,str));
       }
       guiDisplay( 2,LCD_LINE_3+6,say,1);

       strcpy(say,"B");
       if (dataBuff[CURRENT_PHASE_B]!=0xee && dataBuff[CURRENT_PHASE_B]!=0xff)
       {
	       disData = dataBuff[CURRENT_PHASE_B] | dataBuff[CURRENT_PHASE_B+1]<<8 | (dataBuff[CURRENT_PHASE_B+2]&0x7f)<<16;
         decimal = (disData>>8 & 0xf)*100+(disData>>4 & 0xf)*10 + (disData& 0xf);
         integer = (disData>>20 & 0xf)*100+(disData>>16 & 0xf)*10 + (disData>>12 & 0xf);
         strcat(say,floatToString(integer,decimal,3,2,str));
       }
       guiDisplay(76,LCD_LINE_3+6,say,1);

       strcpy(say,"C");
       if (dataBuff[CURRENT_PHASE_C]!=0xee && dataBuff[CURRENT_PHASE_C]!=0xff)
       {
	       disData = dataBuff[CURRENT_PHASE_C] | dataBuff[CURRENT_PHASE_C+1]<<8 | (dataBuff[CURRENT_PHASE_C+2]&0x7f)<<16;
         decimal = (disData>>8 & 0xf)*100+(disData>>4 & 0xf)*10 + (disData& 0xf);
         integer = (disData>>20 & 0xf)*100+(disData>>16 & 0xf)*10 + (disData>>12 & 0xf);
         strcat(say,floatToString(integer,decimal,3,2,str));
       }
       guiDisplay(120,LCD_LINE_3+6,say,1);
       
     	 strcpy(say,"����������=");
     	 if (dataBuff[TOTAL_POWER_FACTOR]!=0xee)
     	 {
     	   disData = dataBuff[TOTAL_POWER_FACTOR] | dataBuff[TOTAL_POWER_FACTOR+1]<<8;
         sign = 0;
         if (disData&0x8000)
         {
           disData &= 0x7fff;
           sign = 1; 
         }
         integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10+(disData>>4 & 0xf);
         decimal = (disData & 0xf);
         if (sign==1)
         {
           strcat(say,"-");
         }
         strcat(say,floatToString(integer,decimal,1,1,str));
     	 }
       guiDisplay(2,LCD_LINE_4+6,say,1);
     }
     else
     {
       guiDisplay( 2,LCD_LINE_1+6,"�й���=",1);
       guiDisplay( 2,LCD_LINE_2+6,"��ѹA    B    C   ",1);
       guiDisplay( 2,LCD_LINE_3+6,"����A    B    C   ",1);
       guiDisplay( 2,LCD_LINE_4+6,"����������=",1);
     }     
   }
   
   if (tmpCycleLink->port==31)
   {
  	 tmpTime = timeHexToBcd(sysTime);
     
     if (tmpCycleLink->bigAndLittleType==0x00 && tmpCycleLink->protocol==DLT_645_2007) 
     {
       buffHasData = readMeterData(dataBuff, tmpCycleLink->mp, SINGLE_PHASE_DAY, ENERGY_DATA, &tmpTime, 0);
       if (buffHasData==FALSE)
       {
       	 buffHasData = readMeterData(dataBuff, tmpCycleLink->mp, SINGLE_PHASE_PRESENT, ENERGY_DATA, &tmpTime, 0);
       }
     }
     else
     {
       buffHasData = readMeterData(dataBuff, tmpCycleLink->mp, SINGLE_PHASE_PRESENT, ENERGY_DATA, &tmpTime, 0);
     }
   }
   else
   {
  	 tmpTime = queryCopyTime(tmpCycleLink->mp);
     
     //2014-04-17,�ĳɶ�ȡ��������һ������
     //buffHasData = readMeterData(dataBuff, tmpCycleLink->mp, PRESENT_DATA, ENERGY_DATA, &tmpTime, 0);
     buffHasData = readMeterData(dataBuff, tmpCycleLink->mp, LAST_TODAY, ENERGY_DATA, &tmpTime, 0);     
   }
   offset = POSITIVE_WORK_OFFSET;
   
   if (buffHasData==TRUE)
   {
     strcpy(say,"�й���=");
   	 
   	 if (dataBuff[offset]!=0xee)
   	 {
       decimal = (dataBuff[offset]>>4 & 0xf) * 10 + (dataBuff[offset] & 0xf);
       integer = (dataBuff[offset+3]>>4 & 0xf)*100000
               +(dataBuff[offset+3] & 0xf)*10000
               +(dataBuff[offset+2]>>4 & 0xf)*1000
               +(dataBuff[offset+2] & 0xf)*100
               +(dataBuff[offset+1]>>4 & 0xf)*10
               +(dataBuff[offset+1]& 0xf);
       strcat(say,floatToString(integer,decimal,2,2,str));
     }
     guiDisplay( 2,LCD_LINE_5+6,say,1);
     
     offset+=4;
     strcpy(say,"��=");  	        
   	 if (dataBuff[offset]!=0xee)
   	 {
       decimal = (dataBuff[offset]>>4 & 0xf) * 10 + (dataBuff[offset] & 0xf);
       integer = (dataBuff[offset+3]>>4 & 0xf)*100000
               +(dataBuff[offset+3] & 0xf)*10000
               +(dataBuff[offset+2]>>4 & 0xf)*1000
               +(dataBuff[offset+2] & 0xf)*100
               +(dataBuff[offset+1]>>4 & 0xf)*10
               +(dataBuff[offset+1]& 0xf);
       strcat(say,floatToString(integer,decimal,2,2,str));
       say[8]='\0';
     }
     guiDisplay( 2,LCD_LINE_6+6,say,1);
     
     offset+=4;
     strcpy(say,"��=");  	        
   	 if (dataBuff[offset]!=0xee)
   	 {
       decimal = (dataBuff[offset]>>4 & 0xf) * 10 + (dataBuff[offset] & 0xf);
       integer = (dataBuff[offset+3]>>4 & 0xf)*100000
               +(dataBuff[offset+3] & 0xf)*10000
               +(dataBuff[offset+2]>>4 & 0xf)*1000
               +(dataBuff[offset+2] & 0xf)*100
               +(dataBuff[offset+1]>>4 & 0xf)*10
               +(dataBuff[offset+1]& 0xf);
       strcat(say,floatToString(integer,decimal,2,2,str));
       say[8]='\0';
     }
     guiDisplay(81,LCD_LINE_6+6,say,1);

     offset+=4;
     strcpy(say,"ƽ=");  	        
   	 if (dataBuff[offset]!=0xee)
   	 {     
       decimal = (dataBuff[offset]>>4 & 0xf) * 10 + (dataBuff[offset] & 0xf);
       integer = (dataBuff[offset+3]>>4 & 0xf)*100000
               +(dataBuff[offset+3] & 0xf)*10000
               +(dataBuff[offset+2]>>4 & 0xf)*1000
               +(dataBuff[offset+2] & 0xf)*100
               +(dataBuff[offset+1]>>4 & 0xf)*10
               +(dataBuff[offset+1]& 0xf);
       strcat(say,floatToString(integer,decimal,2,2,str));
       say[8]='\0';
     }
     guiDisplay( 2,LCD_LINE_7+6,say,1);
     
     offset+=4;
     strcpy(say,"��=");
   	 if (dataBuff[offset]!=0xee)
   	 {
       decimal = (dataBuff[offset]>>4 & 0xf) * 10 + (dataBuff[offset] & 0xf);
       integer = (dataBuff[offset+3]>>4 & 0xf)*100000
               +(dataBuff[offset+3] & 0xf)*10000
               +(dataBuff[offset+2]>>4 & 0xf)*1000
               +(dataBuff[offset+2] & 0xf)*100
               +(dataBuff[offset+1]>>4 & 0xf)*10
               +(dataBuff[offset+1]& 0xf);
       strcat(say,floatToString(integer,decimal,2,2,str));
       say[8]='\0';
     }
     guiDisplay(81,LCD_LINE_7+6,say,1);
   }
   else
   {
     guiDisplay( 2,LCD_LINE_5+6,"�й���=",1);
     guiDisplay( 2,LCD_LINE_6+6,"��=",1);   
     guiDisplay(80,LCD_LINE_6+6,"��=",1);   
     guiDisplay( 2,LCD_LINE_7+6,"ƽ=",1);
     guiDisplay(80,LCD_LINE_7+6,"��=",1);   	 
   }

   lcdRefresh(1,144);

   cycleDelay = nextTime(sysTime, 0, 8);
   tmpCycleLink = tmpCycleLink->next;
   
   if (tmpCycleLink==NULL)
   {
   	 tmpCycleLink = cycleMpLink;
   }
}

/*******************************************************
��������:layer2Menu
��������:����˵�(376.1���ҵ�����������Լ)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void layer2Menu(int lightNum,int layer1Num)
{
	INT8U i, j;
	INT8U startItem=0, endItem=0;

	guiLine(1,17,160,144,0);

  menuInLayer = 2;

 #ifndef LIGHTING
	if (layer1Num==2)
  {
   	if (carrierModuleType==RL_WIRELESS || carrierModuleType==SR_WIRELESS)
   	{
   	  strcpy(layer2MenuItem[2][3],"  δ�����ڵ��ַ  ");
   	}
  }
 #endif
   
  if (layer2MenuNum[layer1Num]>8)
  {
   	if (lightNum>7)
   	{
   	 	startItem = lightNum-7;
   	 	endItem = layer2MenuNum[layer1Num]-startItem;
   	}
   	else
   	{
   	  startItem = 0;
   	  endItem = 8;
   	}
  }
  else
  {
   	startItem = 0;
   	endItem = layer2MenuNum[layer1Num];
  }

	for(i=0,j=startItem; i<endItem && i<8; i++,j++)
	{
		if (j==lightNum)
		{
      guiDisplay(8,LCD_LINE_1+i*16,layer2MenuItem[layer1Num][j],0);
		}
		else
		{
      guiDisplay(8,LCD_LINE_1+i*16,layer2MenuItem[layer1Num][j],1);
		}
	}
	 
  lcdRefresh(17,145);    //ˢ��LCD
}

/*******************************************************
��������:layer2xMenu
��������:����˵��ڶ���(����������鿴)���Ӳ˵�(376.1���ҵ�����������Լ)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void layer2xMenu(INT8U type, INT8U lightNum)
{
	 INT8U i;

	 guiLine(1,17,160,144,0);

   if (type==1)
   {
     menuInLayer = 13;
   }
   else
   {
     menuInLayer = 12;
   }
  #ifdef LIGHTING
   if (type==1)
   {
	   for(i=0;i<3;i++)
	   {
		   if (i==lightNum)
		   {
         guiDisplay(17,LCD_LINE_3+i*17,layer2xMenuItem[type][i],0);
		   }
		   else
		   {
         guiDisplay(17,LCD_LINE_3+i*17,layer2xMenuItem[type][i],1);
		   }
		 }
   }
   else
   {
  #endif 
	   
	   for(i=0;i<2;i++)
	   {
		   if (i==lightNum)
		   {
         guiDisplay(17,LCD_LINE_3+i*17,layer2xMenuItem[type][i],0);
		   }
		   else
		   {
         guiDisplay(17,LCD_LINE_3+i*17,layer2xMenuItem[type][i],1);
		   }
		 }
		 
	#ifdef LIGHTING
	 }
	#endif
	 
   lcdRefresh(17,145);               //ˢ��LCD
}

/*******************************************************
��������:commParaQueryMenu
��������:ͨ�Ų�����ѯ�˵�(�����������˵�)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void commParaQueryMenu(void)
{
	 char              str[30];
	 char              sayStr[30];
	 INT8U             tmpX;

	 guiLine(1,17,160,144,0); //����
	 menuInLayer = 14;        //�˵������14��
	 
   guiDisplay(33,17,"ͨ�Ų�����ѯ",1);
   
   //��������
   guiDisplay(1,33,"��������",1);
   strcpy(sayStr,digitalToChar(addrField.a1[1]>>4));
   strcat(sayStr,digitalToChar(addrField.a1[1]&0xf));
   strcat(sayStr,digitalToChar(addrField.a1[0]>>4));
   strcat(sayStr,digitalToChar(addrField.a1[0]&0xf));
   guiDisplay(68,33,sayStr,1);
   
   //�����ŵ�
   guiDisplay(1,49,"�����ŵ�",1);
   switch(moduleType)
   {
   	  case GPRS_SIM300C:
   	  case GPRS_GR64:
   	  case GPRS_M590E:
   	  case GPRS_M72D:
   	  	strcpy(sayStr, "GPRS");
   	  	break;

   	  case CDMA_DTGS800:
   	  case CDMA_CM180:
   	  	strcpy(sayStr, "CDMA");
   	  	break;
			
   	  case LTE_AIR720H:
   	  	strcpy(sayStr, "LTE");
   	  	break;

   	  case ETHERNET:
   	  	strcpy(sayStr, "��̫��");
   	  	break;
   	  
   	  default:
   	  	strcpy(sayStr,"��ģ��");
   	  	break;
   }
   guiDisplay(68,49,sayStr,1);
   
   //APN����
   guiDisplay(1,  65, "APN����",1);
   guiDisplay(68, 65, (char *)ipAndPort.apn, 1);
   
   guiDisplay(1,81,"��վIP������˿�",1);
	 strcpy(sayStr,intToIpadd(ipAndPort.ipAddr[0]<<24 | ipAndPort.ipAddr[1]<<16 | ipAndPort.ipAddr[2]<<8 | ipAndPort.ipAddr[3],str));
   strcat(sayStr,":");
   guiDisplay(1,97,sayStr,1);
   tmpX = 1+8*strlen(sayStr);
    strcpy(sayStr,intToString(ipAndPort.port[1]<<8 | ipAndPort.port[0],3,str));
   guiDisplay(tmpX,97,sayStr,1);
   
   guiDisplay(1,113,"������IP", 1);
   guiDisplay(1,129,intToIpadd(wlLocalIpAddr,str),1);
	 
	 lcdRefresh(17, 145);
}

/*******************************************************
��������:mpQueryMenu
��������:������ѯ�˵�
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void mpQueryMenu(INT8U type, INT8U layer3Light)
{
   struct cpAddrLink *tmpLink;
	 char              str[30];
	 char              sayStr[30];
	 INT8U             i, tmpX, tmpY, tmpCount;

	 guiLine(1,17,160,144,0);  //����
	 menuInLayer = 14;         //�˵������2��
	 
   switch(type)
   {
   	 case 0:  //���ַ
   	 	#ifdef LIGHTING
       guiDisplay(1,17,"���Ƶ� ͨ�ŵ�ַ ʱ��",1);
   	 	#else
       guiDisplay(1,17,"�������  ����ַ",1);
      #endif
       break;

   	 case 1:  //�ɼ���ַ
       guiDisplay(1,17,"������� �ɼ�����ַ",1);
       break;

   	 case 2:  //�˿� Э��
      #ifdef LIGHTING 
       guiDisplay(1,17,"���Ƶ�   �˿� Э��",1);
      #else
       guiDisplay(1,17,"������� �˿� Э��",1);
      #endif
       break;
   }
   
   tmpY = 33;
   tmpLink = queryMpLink;
   i = 0;
   while(tmpLink!=NULL && i<layer3Light)
   {
     for(tmpCount=0;tmpCount<NUM_MP_PER_PAGE-1;tmpCount++)
     {
   	   tmpLink = tmpLink->next;
     }
     i++;
   }
   tmpCount = 0;
   while((tmpLink!=NULL) && (tmpCount<(NUM_MP_PER_PAGE-1)))
   {
   	  strcpy(sayStr,intToString(tmpLink->mp, 3, str));
   	  
   	  strcpy(str,"");
   	  if (strlen(sayStr)==1)
   	  {
   	  	 strcat(str,"00");
   	  }
   	  if (strlen(sayStr)==2)
   	  {
   	  	 strcat(str,"0");
   	  }	 	   	  
   	  strcat(str,sayStr);
   	 #ifdef LIGHTING
   	  guiDisplay(12,tmpY,str,1);   	  
   	 #else 
   	  guiDisplay(22,tmpY,str,1);
   	 #endif

   	  if (type==2)
   	  {
   	  	 sprintf(str,"%02d",tmpLink->port);
   	  	 guiDisplay(80,tmpY,str,1);
   	  	 
   	  	 strcpy(str,"");
   	  	 switch(tmpLink->protocol)
   	  	 {
   	  	 	 case 1:
   	  	     strcat(str,"97");
   	  	     break;

   	  	 	 case 2:
   	  	     strcat(str,"����");
   	  	     break;

   	  	 	 case 30:
   	  	    #ifdef LIGHTING
   	  	     if (tmpLink->port==31)
   	  	     {
   	  	     	 strcat(str,"����");
   	  	     }
   	  	     else
   	  	     {
   	  	     	 strcat(str,"07��");
   	  	     }
   	  	    #else 
   	  	     strcat(str,"07");
   	  	    #endif
   	  	     break;
   	  	  
   	  	  #ifdef LIGHTING
   	  	 	 case 130:
   	  	     strcat(str,"��·");
   	  	     break;

   	  	 	 case 131:
   	  	     strcat(str,"����");
   	  	     break;

   	  	 	 case 132:
   	  	     strcat(str,"�ն�");
   	  	     break;
   	  	  #endif
   	  	   
   	  	   default:
   	  	     sprintf(str,"%d",tmpLink->protocol);
   	  	   	 break;
   	  	 }
   	  	 guiDisplay(114, tmpY, str, 1);
   	  }
   	  else
   	  {
     	  strcpy(str,"");
     	  for(i=6; i>0; i--)
     	  {
        	switch(type)
        	{
        	  	case 0:
          	  	if(tmpLink->addr[i-1]==0x00)
          	  	{
          	  	 	strcat(str, "00");
          	  	}
          	  	else
          	  	{
          	  	 	strcat(str,digital2ToString((tmpLink->addr[i-1]/0x10)*10
          	  	        +tmpLink->addr[i-1]%0x10,sayStr));
          	  	}
        	      break;
        	      
        	    case 1:
          	  	if(tmpLink->collectorAddr[i-1]==0x00)
          	  	{
          	  	 	strcat(str, "00");
          	  	}
          	  	else
          	  	{
          	  	 	strcat(str,digital2ToString((tmpLink->collectorAddr[i-1]/0x10)*10
          	  	        +tmpLink->collectorAddr[i-1]%0x10,sayStr));
          	  	}
        	    	break;
        	}
     	  }
     	  
     	 #ifdef LIGHTING
     	  if (type==0)
     	  {
     	  	//2015-12-07,���ƿ��Ƶ��ctrlTime�ĵ�4λ��ʾ����ʱ����
     	  	sprintf(sayStr,  " %d", tmpLink->ctrlTime&0x0f);
     	  	strcat(str, sayStr);
     	  }
    	  guiDisplay(48, tmpY, str, 1);
     	 #else
    	  guiDisplay(65, tmpY, str, 1);
     	 #endif
    	}

   	  tmpY += 16;
   	  tmpLink = tmpLink->next;
   	  
   	  tmpCount++;
   }
	 
	 lcdRefresh(17, 145);
}

/*******************************************************
��������:setTeDateTime
��������:�����ն�����/ʱ��(376.1���ҵ�����������Լ)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void setTeDateTime(INT8U lightNum)
{
	INT8U i, tmpX;
	char  str[2];
	
	menuInLayer = 3;

	guiLine(1,17,160,144,0);	
	guiLine(1,60,1,100,1);
	guiLine(160,60,160,100,1);
	guiLine(1,100,160,100,1);
	guiLine(1,60,160,60,1);
	guiLine(1,80,160,80,1);

 #ifdef LIGHTING
	guiDisplay(24, 62, "������ʱ������", 1);
 #else	
	guiDisplay(32, 62, "�ն�ʱ������", 1);
 #endif
  
  tmpX = 5;
  for(i=0;i<14;i++)
  {
    str[0] = dateTimeItem[i];
    str[1] = '\0';
    if (lightNum==i)
    {
      guiDisplay(tmpX,82,str,0);
    }
    else
    {
      guiDisplay(tmpX,82,str,1);
    }
    tmpX += 8;
    
    if (i==3 || i==5)
    {
      guiDisplay(tmpX,82,"-",1);
      tmpX += 8;
    }
    if (i==7)
    {
    	tmpX += 8;
    }
    if (i==9 || i==11)
    {
      guiDisplay(tmpX,82,":",1);
      tmpX += 8;
    }
  }
	 
	lcdRefresh(17,145);
}

/*******************************************************
��������:setTeAddr
��������:�����ն˱��(376.1���ҵ�����������Լ)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void setTeAddr(INT8U lightNum)
{
	INT8U i, row, col;
	char  str[2];
	
	menuInLayer = 3;

	guiLine(1,17,160,144,0);	
	guiLine(1,50,1,110,1);
	guiLine(160,50,160,110,1);
	guiLine(1,110,160,110,1);
	guiLine(1,50,160,50,1);
	guiLine(1,70,160,70,1);

 #ifdef LIGHTING
 	guiDisplay(24,52,"�������������",1);
	guiDisplay(15,72,"���������",1);
 #else
 	guiDisplay(32,52,"�ն˱������",1);
	guiDisplay(15,72,"  �ն˱��",1);
 #endif
	guiDisplay(15,92,"����������",1);
  
  str[1] = '\0';
  row = 72;
  col = 100;
  #ifdef TE_ADDR_USE_BCD_CODE
   for(i=0;i<8;i++)
  #else
   for(i=0;i<9;i++)
  #endif
  {  
    str[0] = tmpTeAddr[i];
 
    #ifdef TE_ADDR_USE_BCD_CODE
     if (i==4)
    #else
     if (i==5)
    #endif
    {
    	 row = 92;
    	 col = 100;
    }
    if (i==lightNum)
    {
       guiDisplay(col,row,str,0);
    }
    else
    {
       guiDisplay(col,row,str,1);
    }
    col+=8;
  }

	lcdRefresh(17,145);
}

/*******************************************************
��������:set485Meter
��������:̨������������(485��)(376.1���ҵ�����������Լ)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void set485Meter(INT8U layer2Light,INT8U layer3Light)
{
	INT8U i, tmpX;
	char  str[2];
	
	menuInLayer = 3;

	guiLine(1,17,160,144,0);
	
	guiLine(1,17,1,140,1);
	guiLine(160,17,160,140,1);
	guiLine(52,34,52,119,1);	
	guiLine(1,17,160,17,1);
	guiLine(1,34,160,34,1);	
	guiLine(1,51,160,51,1);	
	guiLine(1,68,160,68,1);	
	guiLine(1,85,160,85,1);
	guiLine(1,102,160,102,1);
	guiLine(80,119,80,140,1);
	guiLine(1,119,160,119,1);
	guiLine(1,140,160,140,1);

	guiDisplay(16,18,"̨������������",1);
	guiDisplay(4, 35,"������",1);
	guiDisplay(4, 52,"��  ��",1);
	guiDisplay(4, 69,"��  ��",1);
	guiDisplay(4, 86,"Э  ��",1);
	guiDisplay(4,103,"���ַ",1);

  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<4;i++)
  {
    str[0] = chrMp[0][i];
    if (layer2Light==0 && i==layer3Light)
    {
      guiDisplay(tmpX, 35, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 35, str, 1);
    }
    tmpX += 8;
  }
  
  if (layer2Light==1)
  {
    if (chrMp[1][0]>0x37)
    {
      guiDisplay(60, 52, "δ֪", 0);
    }
    else
    {
      guiDisplay(60, 52, chrCopyRate[chrMp[1][0]-0x30], 0);
    }
  }
  else
  {
    if (chrMp[1][0]>0x37)
    {
      guiDisplay(60, 52, "δ֪", 1);
    }
    else
    {
      guiDisplay(60, 52, chrCopyRate[chrMp[1][0]-0x30], 1);
    }
  }

  if (layer2Light==2)
  {
    if (chrMp[2][0]>0x32)
    {
    	guiDisplay(60, 69, "δ֪", 0);
    }
    else
    {
      guiDisplay(60, 69, chrCopyPort[chrMp[2][0]-0x30], 0);
    }
  }
  else
  {
    if (chrMp[2][0]>0x32)
    {
    	guiDisplay(60, 69, "δ֪", 1);
    }
    else
    {
      guiDisplay(60, 69, chrCopyPort[chrMp[2][0]-0x30], 1);
    }
  }

  if (layer2Light==3)
  {
    if (chrMp[3][0]>0x32)
    {
    	guiDisplay(60, 86, "δ֪", 0);
    }
    else
    {
      guiDisplay(60, 86, chrCopyProtocol[chrMp[3][0]-0x30], 0);
    }
  }
  else
  {
    if (chrMp[3][0]>0x32)
    {
    	guiDisplay(60, 86, "δ֪", 1);
    }
    else
    {
      guiDisplay(60, 86, chrCopyProtocol[chrMp[3][0]-0x30], 1);
    }
  }
  
  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<12;i++)
  {
    str[0] = chrMp[4][i];
    if (layer2Light==4 && i==layer3Light)
    {
      guiDisplay(tmpX, 103, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 103, str, 1);
    }
    tmpX += 8;
  }

	if (layer2Light==5)
	{
	  guiDisplay(24,122,"ȷ��",0);
	}
	else
	{
	  guiDisplay(24,122,"ȷ��",1);
	}
	guiDisplay(104,122,"ȡ��",1);

	lcdRefresh(17,144);
}

/*******************************************************
��������:setCarrierMeter
��������:��������������(�ز���)(376.1���ҵ�����������Լ)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void setCarrierMeter(INT8U layer2Light,INT8U layer3Light)
{
	INT8U i, tmpX;
 #ifdef LIGHTING
	char  str[20];
 #else
	char  str[2];
 #endif
	
	menuInLayer = 3;

	guiLine(1,17,160,144,0);

	guiLine(1,17,1,140,1);
	guiLine(160,17,160,140,1);
	guiLine(52,34,52,119,1);	
	guiLine(1,17,160,17,1);
	guiLine(1,34,160,34,1);	
	guiLine(1,51,160,51,1);	
	guiLine(1,68,160,68,1);	
	guiLine(1,85,160,85,1);
	guiLine(1,102,160,102,1);
	guiLine(80,119,80,140,1);
	guiLine(1,119,160,119,1);
	guiLine(1,140,160,140,1);

 #ifdef LIGHTING
 
	guiDisplay(24,18,"���Ƶ��������",1);
	guiDisplay(4, 35,"���Ƶ�",1);
	guiDisplay(4, 52," ��ַ",1);
	guiDisplay(4, 69," ʱ��",1);
	guiDisplay(4, 86," �˿�",1);
	guiDisplay(4,103," Э��",1);

  //�������
  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<4;i++)
  {
    str[0] = chrMp[0][i];
    if (layer2Light==0 && i==layer3Light)
    {
      guiDisplay(tmpX, 35, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 35, str, 1);
    }
    tmpX += 8;
  }
  
  //���ַ
  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<12;i++)
  {
    str[0] = chrMp[1][i];
    if (layer2Light==1 && i==layer3Light)
    {
      guiDisplay(tmpX, 52, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 52, str, 1);
    }
    tmpX += 8;
  }

  //�����Ƶ����ʱ�κ�(�÷���������)
  str[0] = chrMp[4][0];
  if (layer2Light==2)
  {
    guiDisplay(60, 69, str, 0);
  }
  else
  {
    guiDisplay(60, 69, str, 1);
  }
  
  strcpy(str, "");
  switch(chrMp[3][0])
  {
  	case 0x31:
  		strcpy(str, "RS485-1");
  		break;
  		
  	case 0x32:
  		strcpy(str, "RS485-2");
  		break;
  		
  	case 0x30:
  		strcpy(str, "�ز��˿�");
  		break;
  }
  if (layer2Light==3)
  {
    guiDisplay(60, 86, str, 0);
  }
  else
  {
    guiDisplay(60, 86, str, 1);
  }

  strcpy(str, "");
  switch(chrMp[2][0])
  {
  	case 0x31:
  		if (0x30==chrMp[3][0])
  		{
  		  strcpy(str, "���ƿ�����");
  		}
  		else
  		{
  		  strcpy(str, "07��");
  		}
  		break;
  		
  	case 0x32:
  		strcpy(str, "97��");
  		break;
  		
  	case 0x33:
  		strcpy(str, "��·����");
  		break;
  }
  if (layer2Light==4)
  {
    guiDisplay(60,103, str, 0);
  }
  else
  {
    guiDisplay(60,103, str, 1);
  }
  
  
	if (layer2Light==5)
	{
	  guiDisplay(24, 122, "ȷ��", 0);
	}
	else
	{
	  guiDisplay(24, 122, "ȷ��", 1);
	}
	guiDisplay(104, 122, "ȡ��", 1);
	
	
 #else
 

	guiDisplay(16,18,"��������������",1);
	guiDisplay(4, 35,"������",1);
	guiDisplay(4, 52,"���ַ",1);
	guiDisplay(4, 69,"Э  ��",1);
	guiDisplay(4, 86,"�ɼ���",1);
	guiDisplay(4,103,"������",1);

  //�������
  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<4;i++)
  {
    str[0] = chrMp[0][i];
    if (layer2Light==0 && i==layer3Light)
    {
      guiDisplay(tmpX, 35, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 35, str, 1);
    }
    tmpX += 8;
  }
  
  //���ַ
  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<12;i++)
  {
    str[0] = chrMp[1][i];
    if (layer2Light==1 && i==layer3Light)
    {
      guiDisplay(tmpX, 52, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 52, str, 1);
    }
    tmpX += 8;
  }
  
  //Э��  
  if (layer2Light==2)
  {
    if (chrMp[2][0]>0x31)
    {
    	guiDisplay(60, 69, "δ֪", 0);
    }
    else
    {
      guiDisplay(60, 69, chrCopyProtocol[chrMp[2][0]-0x30], 0);
    }
  }
  else
  {
    if (chrMp[2][0]>0x32)
    {
    	guiDisplay(60, 69, "δ֪", 1);
    }
    else
    {
      guiDisplay(60, 69, chrCopyProtocol[chrMp[2][0]-0x30], 1);
    }
  }
  
  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<12;i++)
  {
    str[0] = chrMp[3][i];
    if (layer2Light==3 && i==layer3Light)
    {
      guiDisplay(tmpX, 86, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 86, str, 1);
    }
    tmpX += 8;
  }
  
  //������
  str[0] = chrMp[4][0];
  if (layer2Light==4)
  {
    guiDisplay(60, 103, str, 0);
  }
  else
  {
    guiDisplay(60, 103, str, 1);
  }  

	if (layer2Light==5)
	{
	  guiDisplay(24,122,"ȷ��",0);
	}
	else
	{
	  guiDisplay(24,122,"ȷ��",1);
	}
	guiDisplay(104,122,"ȡ��",1);
	
 #endif

	lcdRefresh(17,144);
}

/*******************************************************
��������:setRlPara
��������:�����������ģ�����(376.1���ҵ�����������Լ)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void setRlPara(INT8U layer2Light,INT8U layer3Light)
{
	INT8U i, j, tmpX, tmpY;
	char  str[2];
	
	menuInLayer = 3;

	guiLine(1,17,160,144,0);
	
	guiLine(1,17,1,122,1);
	guiLine(160,17,160,122,1);
	guiLine(68,34,68,102,1);	
	guiLine(1,17,160,17,1);
	guiLine(1,34,160,34,1);	
	guiLine(1,51,160,51,1);	
	guiLine(1,68,160,68,1);	
	guiLine(1,85,160,85,1);
	guiLine(1,102,160,102,1);
	guiLine(1,122,160,122,1);

	guiDisplay(16,18,"���ģ���������",1);
	guiDisplay(4, 35,"��������",1);
	guiDisplay(4, 52,"�����",1);
	guiDisplay(4, 69,"�ź�ǿ��",1);
	guiDisplay(4, 86,"��    ��",1);

  //��������
  tmpY = 35;
  for(j=0;j<4;j++)
  {
    tmpX = 85;
    str[1] = '\0';
    for(i=0;i<2;i++)
    {
      str[0] = chrRlPara[j][i];
      if (layer2Light==j && i==layer3Light)
      {
        guiDisplay(tmpX, tmpY, str, 0);
      }
      else
      {
        guiDisplay(tmpX, tmpY, str, 1);
      }
      tmpX += 8;
    }
    
    tmpY += 17;
  }
  
	if (layer2Light==4)
	{
	  guiDisplay(64,105,"ȷ��",0);
	}
	else
	{
	  guiDisplay(64,105,"ȷ��",1);
	}

	lcdRefresh(17,144);
}

/*******************************************************
��������:setCopyForm
��������:���ñ���ͨ��ģ�鳭����ʽ(376.1���ҵ�����������Լ)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void setCopyForm(INT8U lightNum)
{
	INT8U i, row, col;
	char  str[2];
	
	menuInLayer = 3;

	guiLine(1,17,160,144,0);	
	guiLine(1,50,1,110,1);
	guiLine(160,50,160,110,1);
	guiLine(1,110,160,110,1);
	guiLine(1,50,160,50,1);
	guiLine(1,70,160,70,1);

	guiDisplay(33,52,"������ʽ����",1);
	if (lightNum==0)
	{
	  guiDisplay(24, 72, "��������������", 0);
	  guiDisplay(24, 92, " ·���������� ", 1);
	}
	else
	{
	  guiDisplay(24, 72, "��������������", 1);
	  guiDisplay(24, 92, " ·���������� ", 0);
	}

	lcdRefresh(17, 145);
}

/*******************************************************
��������:setDenizenDataType
��������:���þ����û�����������(376.1���ҵ�����������Լ)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void setDenizenDataType(INT8U lightNum)
{
	INT8U i, row, col;
	char  str[2];
	
	menuInLayer = 3;

	guiLine(1,17,160,144,0);
	guiLine(1,50,1,130,1);
	guiLine(160,50,160,130,1);
	guiLine(1,130,160,130,1);
	guiLine(1,50,160,50,1);
	guiLine(1,70,160,70,1);

	guiDisplay(17,52,"����������������",1);
	switch (lightNum)
	{
	  case 0:
	    guiDisplay(24, 72, "ʵʱ+��������", 0);
	    guiDisplay(24, 92, "��ʵʱ����   ", 1);
	    guiDisplay(24,112, "��ʵʱ��ʾֵ ", 1);
	    break;
	  
	  case 1:
	    guiDisplay(24, 72, "ʵʱ+��������", 1);
	    guiDisplay(24, 92, "��ʵʱ����   ", 0);
	    guiDisplay(24,112, "��ʵʱ��ʾֵ ", 1);
	    break;

	  case 2:
	    guiDisplay(24, 72, "ʵʱ+��������", 1);
	    guiDisplay(24, 92, "��ʵʱ����   ", 1);
	    guiDisplay(24,112, "��ʵʱ��ʾֵ ", 0);
	    break;
	}
 
	lcdRefresh(17, 145);
}

/*******************************************************
��������:setCycleType
��������:����������������(376.1���ҵ�����������Լ)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void setCycleType(INT8U lightNum)
{
	INT8U i, row, col;
	char  str[2];
	
	menuInLayer = 3;

	guiLine(1,17,160,144,0);
	guiLine(1,50,1,130,1);
	guiLine(160,50,160,130,1);
	guiLine(1,130,160,130,1);
	guiLine(1,50,160,50,1);
	guiLine(1,70,160,70,1);

	guiDisplay(33,52,"������������",1);
	switch (lightNum)
	{
	  case 0:
	    guiDisplay(24, 72, "���б��", 0);
	    guiDisplay(24, 92, "��̨����", 1);
	    break;
	  
	  case 1:
	    guiDisplay(24, 72, "���б��", 1);
	    guiDisplay(24, 92, "��̨����", 0);
	    break;
	}

	lcdRefresh(17, 145);
}


#endif //MENU_FOR_CQ_CANON

#else     //PLUG_IN_CARRIER_MODULEר��III�Ͳ˵�

/*******************************************************
��������:adjustCommParaLightIII
��������:����ͨ�Ų���������(ר��III��)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void adjustCommParaLightIII(INT8U leftRight)
{
 	 INT8U tmpData;
 	 
 	 switch (rowOfLight)
 	 {
 	 	  case 0:
 	 	  	tmpData = 4;
 	 	  	break;
 	 	  	
 	 	  case 1:
 	 	   #ifdef TE_ADDR_USE_BCD_CODE
 	 	    tmpData = 4;
 	 	   #else
 	 	    tmpData = 5;
 	 	   #endif
 	 	    break;

 	 	  case 2:
 	 	    tmpData = 1;
 	 	    break;
 	 	    
 	 	  case 3:
 	 	  case 4:
 	 	  	tmpData = 21;
 	 	  	break;
 	 }
 	 
 	 if (leftRight==1)  //�Ҽ�
 	 {
 	   if (keyLeftRight>=tmpData-1)
 	   {
 	 	   keyLeftRight = 0;
 	   }
 	   else
 	   {
 	 	   keyLeftRight++;
 	   }
 	 }
 	 else
 	 {
 	   if (keyLeftRight==0)
 	   {
 	 	   keyLeftRight = tmpData-1;
 	   }
 	   else
 	   {
 	 	   keyLeftRight--;
 	   }
 	 }
	 
	 if (rowOfLight==3 
	 	 	   || rowOfLight==4)
	 {
	 	 if ((keyLeftRight+1)%4==0 && keyLeftRight<16)
	 	 {
	 	   if (leftRight==1)
	 	   {
	 	     keyLeftRight++;
	 	   }
	 	   else
	 	   {
	 	   	 keyLeftRight--;
	 	   }
	 	 }
	 }
}


void setZbMeter(INT8U layer2Light,INT8U layer3Light)
{
	INT8U i, tmpX;
	char  str[2];
	
	menuInLayer = 3;

	guiLine(1,17,160,160,0);
	
	guiLine(1,17,1,156,1);
	guiLine(160,17,160,156,1);
	guiLine(52,34,52,136,1);	
	guiLine(1,17,160,17,1);
	guiLine(1,34,160,34,1);	
	guiLine(1,51,160,51,1);	
	guiLine(1,68,160,68,1);	
	guiLine(1,85,160,85,1);
	guiLine(1,102,160,102,1);	
	guiLine(80,119,80,156,1);
	guiLine(132,119,132,136,1);
	guiLine(1,119,160,119,1);
	guiLine(1,136,160,136,1);	
	guiLine(1,156,160,156,1);
	

	guiDisplay(49,18,"�������",1);
	guiDisplay(4, 35,"������",1);
	guiDisplay(4, 52,"��  ��",1);
	guiDisplay(4, 69,"��  ��",1);
	guiDisplay(4, 86,"Э  ��",1);
	guiDisplay(4,103,"���ַ",1);
	guiDisplay(4,120,"�����",1);
	guiDisplay(82,120,"С���",1);

  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<4;i++)
  {
    str[0] = chrMp[0][i];
    if (layer2Light==0 && i==layer3Light)
    {
      guiDisplay(tmpX, 35, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 35, str, 1);
    }
    tmpX += 8;
  }
  
  if (layer2Light==1)
  {
    if (chrMp[1][0]>0x37)
    {
      guiDisplay(60, 52, "δ֪", 0);
    }
    else
    {
      guiDisplay(60, 52, chrCopyRate[chrMp[1][0]-0x30], 0);
    }
  }
  else
  {
    if (chrMp[1][0]>0x37)
    {
      guiDisplay(60, 52, "δ֪", 1);
    }
    else
    {
      guiDisplay(60, 52, chrCopyRate[chrMp[1][0]-0x30], 1);
    }
  }

  if (layer2Light==2)
  {
    if (chrMp[2][0]>0x32)
    {
    	guiDisplay(60, 69, "δ֪", 0);
    }
    else
    {
      guiDisplay(60, 69, chrCopyPort[chrMp[2][0]-0x30], 0);
    }
  }
  else
  {
    if (chrMp[2][0]>0x32)
    {
    	guiDisplay(60, 69, "δ֪", 1);
    }
    else
    {
      guiDisplay(60, 69, chrCopyPort[chrMp[2][0]-0x30], 1);
    }
  }

  if (layer2Light==3)
  {
    if (chrMp[3][0]>0x37)
    {
    	guiDisplay(60, 86, "δ֪", 0);
    }
    else
    {
      guiDisplay(60, 86, chrCopyProtocol[chrMp[3][0]-0x30], 0);
    }
  }
  else
  {
    if (chrMp[3][0]>0x37)
    {
    	guiDisplay(60, 86, "δ֪", 1);
    }
    else
    {
      guiDisplay(60, 86, chrCopyProtocol[chrMp[3][0]-0x30], 1);
    }
  }
  
  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<12;i++)
  {
    str[0] = chrMp[4][i];
    if (layer2Light==4 && i==layer3Light)
    {
      guiDisplay(tmpX, 103, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 103, str, 1);
    }
    tmpX += 8;
  }

  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<2;i++)
  {
    str[0] = chrMp[5][i];
    if (layer2Light==5 && i==layer3Light)
    {
      guiDisplay(tmpX, 120, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 120, str, 1);
    }
    tmpX += 8;
  }

  tmpX = 140;
  str[1] = '\0';
  for(i=0;i<2;i++)
  {
    str[0] = chrMp[6][i];
    if (layer2Light==6 && i==layer3Light)
    {
      guiDisplay(tmpX, 120, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 120, str, 1);
    }
    tmpX += 8;
  }
  
	if (layer2Light==7)
	{
	  guiDisplay(24,138,"ȷ��",0);
	}
	else
	{
	  guiDisplay(24,138,"ȷ��",1);
	}
	guiDisplay(104,138,"ȡ��",1);

	lcdRefresh(17,160);
}


/**************************************************
��������:userInterface
��������:�˻��ӿڴ���(376.1���ҵ���ר��III�Ͳ˵���Լ)
���ú���:
�����ú���:
�������:void *arg
�������:
����ֵ��״̬
***************************************************/
void userInterface(BOOL secondChanged)
{
	 METER_DEVICE_CONFIG meterConfig;
   char      str[30],strX[30];
   INT8U     i,nextInfo;
   INT16U    j;
   INT16U    tmpAddr;                    //��ʱ�ն˵�ַ
   char      *tmpChar;
   INT16U    tmpData;
   
   //ly,2011-8-26,add
   register int fd, interface, retn = 0;
   struct ifreq buf[MAXINTERFACES];
   struct ifconf ifc;

   if (secondChanged==TRUE)
   {
     #ifdef LOAD_CTRL_MODULE
       ctrlAlarmDisplay();   //���ƹ�����ʾ
     #endif
        
     if (aberrantAlarm.aberrantFlag==1)
     {
       guiLine(60,1,76,15,0);
   
       if (aberrantAlarm.blinkCount==1)
       {
         aberrantAlarm.blinkCount = 2;
         guiDisplay(60, 1, "��", 1);
         guiLine(60,1,62,4,0);
         guiDisplay(60, 2, "��", 1);
       }
       else
       {
   	     guiDisplay(60, 1, digital2ToString(aberrantAlarm.eventNum,str), 1);
   	     aberrantAlarm.blinkCount = 1;
   	   }
       
       if (compareTwoTime(aberrantAlarm.timeOut, sysTime))
       {
         guiLine(60,1,76,15,0);
         aberrantAlarm.aberrantFlag = 0;
       }
       
       lcdRefresh(1, 16);
     }
     
     //titleʱ��
     refreshTitleTime();
     
     if (lcdLightOn==LCD_LIGHT_ON)
     {
       if (compareTwoTime(lcdLightDelay, sysTime))
       {
     	   lcdLightOn = LCD_LIGHT_OFF;
     	   lcdBackLight(LCD_LIGHT_OFF);
     	   
     	   defaultMenu();
       }
     }
     
     if (setParaWaitTime!=0 && setParaWaitTime!=0xfe)
     {
    		setParaWaitTime--;
     }
  	 
  	 if (ctrlCmdWaitTime!=0 && ctrlCmdWaitTime!=0xfe)
  	 {
  			ctrlCmdWaitTime--;
  			if (ctrlCmdWaitTime<1)
  			{
  			 	 //resumeLcd = 1;                 //�ָ�LCD�˵���ʾ��־
  			}
  	 }

  	 if (pageWait!=0)
  	 {
  			pageWait--;
  			if (pageWait<1)
  			{
  			 	//resumeLcd = 1;                  //�ָ����Ի���
     	    defaultMenu();
  			}
  	 }
  	
  	 //�����բ��ʾ��ʱ��
  	 #ifdef LOAD_CTRL_MODULE
  	  if (gateCloseWaitTime!=0 && gateCloseWaitTime!=0xfe)
  	  {
  		  gateCloseWaitTime--;
  	  }
  	 #endif
   }
   
   //4.�澯����
   alarmProcess();

   if ((keyValue = ioctl(fdOfIoChannel, READ_KEY_VALUE, 0))!=0)
   {
      keyPressCount++;
      
      if (keyPressCount>keyCountMax)
      {
        if (menuInLayer!=99)
        {
           pageWait = 10;
        }
        else
        {
           if (keyValue==KEY_OK)
           {
          	  pageWait = 10;
          	  menuInLayer = 1;
           }
        }

        lcdBackLight(LCD_LIGHT_ON);
        lcdLightOn = LCD_LIGHT_ON;
        
        //�������·�����ں�բ״̬�ҿ���״̬����(ctrlStatus)��ΪNONE_CTRL,
        //��ctrlStatus��ΪNONE_CTRL,ʹ��Ļ�ָ����Ի���
        for(i=0;i<ctrlStatus.numOfAlarm;i++)
        {
           if (ctrlStatus.allPermitClose[i]==1)
           {
        	    for(j=i;j<ctrlStatus.numOfAlarm;j++)
        	    {
        	       ctrlStatus.aQueue[j] = ctrlStatus.aQueue[j+1];
        	       ctrlStatus.allPermitClose[j]=ctrlStatus.allPermitClose[j+1];
        	    }
        	    ctrlStatus.numOfAlarm--;
        	    if (ctrlStatus.numOfAlarm==0)
        	    {
        	       menuInLayer=1;
        	    }
        	 }
        }
        lcdLightDelay = nextTime(sysTime, 1, 0);

        keyPressCount = 0;
        
        switch(keyValue)
        {
      	  case KEY_OK:       //ȷ��
      	  	switch(menuInLayer)
      	  	{
      	  		case 1:  //�˵���1��
      	    		switch (layer1MenuLight)
      	    		{
      	    		  case 0:  //ʵʱ����
      	    		  case 1:  //������ֵ
      	    		    if (layer1MenuLight==1)
      	    		    {
      	    		    	if (moduleType==ETHERNET)
      	    		    	{      	    		    		
      	    		    	  layer2MenuNum[1] = 15;
      	    		    	}
      	    		    	else
      	    		    	{
      	    		    	  layer2MenuNum[1] = 14;
      	    		    	}
      	    		    }
      	    		    layer2Menu(layer2MenuLight[layer1MenuLight], layer1MenuLight);
      	    		    break;
      	    		    
      	    		  case 2:  //����״̬
      	    		 	 	layer2MenuLight[2] = 0;
      	    		  	controlStatus(layer2MenuLight[2]);
      	    		  	break;
      	    		  	
      	    		  case 3:  //���ܱ�ʾ��
                    countParameter (0x04, 10, &meterDeviceNum);
                    layer2MenuNum[layer1MenuLight] = meterDeviceNum;
      	    		 	 	layer2MenuLight[layer1MenuLight] = 0;
      	    		 	 	queryMpLink = initPortMeterLink(0xff);
      	    		  	meterVisionValue(layer2MenuLight[layer1MenuLight]);
      	    		  	break;
      	    		  
      	    		  case 4:  //������Ϣ
      	    		  	layer2MenuNum[4] = chnMessage.numOfMessage;
          	  	 	 	layer2MenuLight[4] = 0;
          	  	 	 	numOfPage = 0;
          	  	 	 	chinese(layer2MenuLight[4],numOfPage,0);
      	    		  	break;
      	    		  	
      	    		  case 5:  //������Ϣ
      	    		  	layer2MenuLight[5] = 0;
      	    		  	chargeInfo(0);
      	    		  	break;
      	    		  	
      	    		  case 6:  //�ն���Ϣ
      	    		 	 	layer2MenuLight[6] = 0;
      	    		 	 	if (ifHasAcModule==TRUE)
      	    		 	 	{
      	    		 	 	  layer2MenuNum[6] = 17;
      	    		 	 	}
      	    		 	 	else
      	    		 	 	{
      	    		 	 	  layer2MenuNum[6] = 3;
      	    		 	 	}
      	    	 	 	  terminalInfo(layer2MenuLight[6]);
      	    		  	break;
      	    		}
      	  		 	break;
      	  		
      	  		case 2:   //�˵���2��
      	  			switch(layer1MenuLight)
      	  		  {
      	  		  	case 0:  //ʵʱ����
      	  		  	 	switch(layer2MenuLight[layer1MenuLight])
      	  		  	 	{
      	  		  	 		 case 0:  //��ǰ����
      	  		  	 		 	 layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = totalAddGroup.numberOfzjz+pulseConfig.numOfPulse;
      	  		  	 		 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	  		  	 		 	 currentPower(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	  		  	 		 	 break;
      	  		  	 		 	 
      	  		  	 		 case 1:  //��ǰ����
                         layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = meterDeviceNum;
      	    		 	 	     layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	     queryMpLink = initPortMeterLink(0xff);
      	    		 	 	     currentEnergy(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	  		  	 		 	 break;
      	  		  	 		 	 
      	  		  	 		 case 2:  //��������
          	    		 	 	 mpPowerCurveLight = 0;
          	    		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
          	    		 	 	 fillTimeStr();   //�õ�ǰ��������ѯ�����ַ���
          	    		 	 	 queryMpLink = initPortMeterLink(0xff);
          	    		 	 	 powerCurve(mpPowerCurveLight,layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
          	    		 	 	 break;
      	  		  	 		 	 
      	  		  	 		 case 3:  //����״̬
      	  		  	 		 	 statusOfSwitch();
      	  		  	 		 	 break;
      	  		  	 		 	 
      	  		  	 		 case 4:  //���ؼ�¼
                         searchCtrlEvent(ERC(6));
                         tmpEventShow = eventLinkHead;
                         eventRecordShow(ERC(6));
      	  		  	 		 	 break;

      	  		  	 		 case 5:  //��ؼ�¼
                         searchCtrlEvent(ERC(7));
                         tmpEventShow = eventLinkHead;
                         eventRecordShow(ERC(7));
      	  		  	 		 	 break;

      	  		  	 		 case 6:  //ң�ؼ�¼
                         searchCtrlEvent(ERC(5));
                         tmpEventShow = eventLinkHead;
                         eventRecordShow(ERC(5));
      	  		  	 		 	 break;
      	  		  	 		 	 
      	  		  	 		 case 7:  //ʧ���¼
                         searchCtrlEvent(ERC(14));
                         tmpEventShow = eventLinkHead;
                         eventRecordShow(ERC(14));
      	  		  	 		 	 break;
      	  		  	 	}
      	  		  	 	break;
      	  		  	
      	  		  	case 1:  //������ֵ
      	  		  		switch(layer2MenuLight[layer1MenuLight])
      	  		  		{
      	  		  			 case 0:  //ʱ�οز���
                         layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                         periodPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	  		  			 	 break;
      	  		  			 
      	  		  			 case 1:  //���ݿز���
                         layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	  		  			 	 wkdPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	  		  			 	 break;
      	  		  			 	 
      	  		  			 case 2:  //�¸��ز���
                         layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	  		  			 	 powerDownPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	  		  			 	 break;      	  		  			 	 

      	  		  			 case 3:  //KvKiKp
      	  		  			 	 queryMpLink = initPortMeterLink(0xff);
      	  		  			 	 tmpMpLink = queryMpLink;
      	  		  			 	 kvkikp(tmpMpLink);
      	  		  			 	 break;

      	  		  			 case 4:  //���ܱ����
      	  		  			 	 queryMpLink = initPortMeterLink(0xff);
      	  		  			 	 tmpMpLink = queryMpLink;
      	  		  			 	 meterPara(tmpMpLink);
      	  		  			 	 break;
      	  		  			 	 
      	  		  			 case 5:  //���ò���
      	  		  			 	 configPara(88,88);
      	  		  			 	 break;
      	  		  			 	 
                       case 6:  //����ר���û�������
                         strcpy(tmpVpnUserName, (char *)vpn.vpnName);
                         tmpVpnUserName[32] = '\0';
                         trim(tmpVpnUserName);
                        
     	 	 	  		 	 	     for(i=strlen(tmpVpnUserName); i<32; i++)
     	 	 	  		 	 	     {
     	 	 	  		 	 	    	 tmpVpnUserName[i] = ' ';
     	 	 	  		 	 	     }
     	 	 	  		 	 	     tmpVpnUserName[32] = '\0';

                         strcpy(tmpVpnPw, (char *)vpn.vpnPassword);
                         tmpVpnPw[32] = '\0';
                         trim(tmpVpnPw);
     	 	 	  		 	 	     for(i=strlen(tmpVpnPw); i<32; i++)
     	 	 	  		 	 	     {
     	 	 	  		 	 	    	 tmpVpnPw[i] = ' ';
     	 	 	  		 	 	     }
     	 	 	  		 	 	     tmpVpnPw[32] = '\0';
     	 	 	  		 	 	     keyLeftRight = 0;
                         inputStatus  = STATUS_NONE;
                         setVpn(keyLeftRight);
                         break;
                         
                      case 7:    //�������
                        strcpy(chrMp[0],"0001");
                        strcpy(chrMp[1],"0");
                        strcpy(chrMp[2],"1");
                        strcpy(chrMp[3],"0");
                        strcpy(chrMp[4],"000000000001");
                        strcpy(chrMp[5],"00");
                        strcpy(chrMp[6],"00");
                        keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  setZbMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
                      	break;
                      
                      case 8:    //��������
                      	copyCtrl[0].nextCopyTime = nextTime(sysTime, 0, 5);
                      	copyCtrl[1].nextCopyTime = nextTime(sysTime, 0, 5);
                      	copyCtrl[2].nextCopyTime = nextTime(sysTime, 0, 5);
                        guiLine(10,55,150,105,0);
                        guiLine(10,55,10,105,1);
                        guiLine(150,55,150,105,1);
                        guiLine(10,55,150,55,1);
                        guiLine(10,105,150,105,1);
                        guiDisplay(16,70,"5���Ӻ�ʼ����!",1);
                        lcdRefresh(10,120);
                        menuInLayer--;                        
                      	break;

      	    	 	 	  	case 9:    //�ն˳�������
      	    	 	 	  	  uDiskUpgrade();
      	    	 	 	  	  break;

                      case 10:   //�ն˼�����������
                        sprintf(chrMp[0], "%01d", cascadeCommPara.commPort);   //�����˿�
                        if (cascadeCommPara.flagAndTeNumber&0x80)
                        {
                          strcpy(chrMp[1],"2");      //������
                        }
                        else
                        {
                          strcpy(chrMp[1],"1");      //������
                        }
                        switch (cascadeCommPara.flagAndTeNumber&0xf)
                        {
                        	case 1:
                           sprintf(chrMp[2],"%02x%02x%05d",cascadeCommPara.divisionCode[1],cascadeCommPara.divisionCode[0],cascadeCommPara.cascadeTeAddr[0]|cascadeCommPara.cascadeTeAddr[1]<<8);      //�ն�1
                           strcpy(chrMp[3],"000000000");
                           strcpy(chrMp[4],"000000000");
                           break;
                        	case 2:
                           sprintf(chrMp[2],"%02x%02x%05d",cascadeCommPara.divisionCode[1],cascadeCommPara.divisionCode[0],cascadeCommPara.cascadeTeAddr[0]|cascadeCommPara.cascadeTeAddr[1]<<8);      //�ն�1
                           sprintf(chrMp[3],"%02x%02x%05d",cascadeCommPara.divisionCode[3],cascadeCommPara.divisionCode[2],cascadeCommPara.cascadeTeAddr[2]|cascadeCommPara.cascadeTeAddr[3]<<8);      //�ն�1
                           strcpy(chrMp[4],"000000000");
                           break;
                        	case 3:
                           sprintf(chrMp[2],"%02x%02x%05d",cascadeCommPara.divisionCode[1],cascadeCommPara.divisionCode[0],cascadeCommPara.cascadeTeAddr[0]|cascadeCommPara.cascadeTeAddr[1]<<8);      //�ն�1
                           sprintf(chrMp[3],"%02x%02x%05d",cascadeCommPara.divisionCode[3],cascadeCommPara.divisionCode[2],cascadeCommPara.cascadeTeAddr[2]|cascadeCommPara.cascadeTeAddr[3]<<8);      //�ն�1
                           sprintf(chrMp[4],"%02x%02x%05d",cascadeCommPara.divisionCode[5],cascadeCommPara.divisionCode[4],cascadeCommPara.cascadeTeAddr[4]|cascadeCommPara.cascadeTeAddr[5]<<8);      //�ն�1
                           break;
                           
                        	default:
                           strcpy(chrMp[2],"000000000");
                           strcpy(chrMp[3],"000000000");
                           strcpy(chrMp[4],"000000000");
                           
                           strcpy(chrMp[1],"0");      //������
                           break;
                        }
                        
                        keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
                      	break;
                      	
                      case 11:    //����LCD�Աȶ�
      	    	 	 	  	  setLcdDegree(lcdDegree);
      	    	 	 	  	  break;
      	    	 	 	  	
    	    	 	 	  	  case 12:    //ά���ӿ�ģʽ����
    	    	 	 	  	  	if (mainTainPortMode==0x55)
    	    	 	 	  	  	{
    	    	 	 	  	  	  keyLeftRight = 1;
    	    	 	 	  	  	}
    	    	 	 	  	  	else
    	    	 	 	  	  	{
    	    	 	 	  	  	  keyLeftRight = 0;
    	    	 	 	  	  	}
    	    	 	 	  	  	setMainTain(keyLeftRight);
    	    	 	 	  	  	break;

    	    	 	 	  	  case 13:    //��2·485�ڹ�������
    	    	 	 	  	  	if (rs485Port2Fun==0x55)
    	    	 	 	  	  	{
    	    	 	 	  	  	  keyLeftRight = 1;
    	    	 	 	  	  	}
    	    	 	 	  	  	else
    	    	 	 	  	  	{
    	    	 	 	  	  	  keyLeftRight = 0;
    	    	 	 	  	  	}
    	    	 	 	  	  	setRs485Port2(keyLeftRight);
    	    	 	 	  	  	break;

      	    	 	 	  	case 14:    //��̫����������
                        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
                        {
                          ifc.ifc_len = sizeof buf;
                          ifc.ifc_buf = (caddr_t) buf;
                          if (!ioctl(fd, SIOCGIFCONF, (char *) &ifc))
                          {
                            interface = ifc.ifc_len / sizeof(struct ifreq);
                            while (interface-- > 0)
                            {
                              if (strstr(buf[interface].ifr_name,"eth0"))
                              {
                               /*Get HW ADDRESS of the net card */
                                if (!(ioctl(fd, SIOCGIFHWADDR, (char *) &buf[interface]))) 
                                {
                                    sprintf(tmpEthMac, "%02x%02x%02x%02x%02x%02x",
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[0],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[1],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[2],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[3],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[4],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[5]); // ����sprintfת����char *
                                }
                              }
                            }//end of while
                          }
                          else
                          {
                            perror("cpm: ioctl");
                          }
                        }
                        else
                        {
                          perror("cpm: socket");
                        }
                     
                        close(fd);
                       	
                       	strcpy(chrEthPara[0],intToIpadd(teIpAndPort.teIpAddr[0]<<24 | teIpAndPort.teIpAddr[1]<<16 | teIpAndPort.teIpAddr[2]<<8 | teIpAndPort.teIpAddr[3],str));
                       	strcpy(chrEthPara[1],intToIpadd(teIpAndPort.mask[0]<<24 | teIpAndPort.mask[1]<<16 | teIpAndPort.mask[2]<<8 | teIpAndPort.mask[3],str));
                       	strcpy(chrEthPara[2],intToIpadd(teIpAndPort.gateWay[0]<<24 | teIpAndPort.gateWay[1]<<16 | teIpAndPort.gateWay[2]<<8 | teIpAndPort.gateWay[3],str));
                        chrEthPara[3][0] = teIpAndPort.ethIfLoginMs;
                        keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                        layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 4;
      	    	 	 	  		setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		break;
      	  		  		}
      	  		  		break;

                  case 2:  //����״̬
      	    		 	 	if (layer2MenuLight[layer1MenuLight]+2>totalAddGroup.numberOfzjz)
      	    		 	 	{
      	    		 	 		layer2MenuLight[layer1MenuLight]=0;
      	    		 	 	}
      	    		 	 	else
      	    		 	 	{
      	    		 	 		layer2MenuLight[layer1MenuLight]++;
      	    		 	 	}
                  	controlStatus(layer2MenuLight[layer1MenuLight]);
                  	break;

      	  		  	case 3:  //���ܱ�ʾ��
 	  		  	 		 	  if (layer2MenuNum[layer1MenuLight]<2)
 	  		  	 		 	  {
 	  		  	 		 	 	  layer2MenuLight[layer1MenuLight] = 0;
 	  		  	 		 	  }
 	  		  	 		 	  else
 	  		  	 		 	  {
 	  		  	 		 	    if(layer2MenuLight[layer1MenuLight]>=layer2MenuNum[layer1MenuLight]-1)
 	  		  	 		 	    {
 	  		  	 		 	      layer2MenuLight[layer1MenuLight] = 0;
 	  		  	 		 	    }
 	  		  	 		 	    else
 	  		  	 		 	    {
 	  		  	 		 	   	  layer2MenuLight[layer1MenuLight]++;
 	  		  	 		 	    }
 	  		  	 		 	  }
      	    		  	meterVisionValue(layer2MenuLight[layer1MenuLight]);
      	    		  	break;

      	  		  	case 4:  //������Ϣ
                    nextInfo = 0;
	  	 	 	  	      if (chnMessage.message[layer3MenuLight[2][1]].len<=160)
	  	 	 	  	      {
	  	 	 	  	         nextInfo = 1;
	  	 	 	  	      }
	  	 	 	  	      else
	  	 	 	  	      {
                       if (chnMessage.message[layer3MenuLight[2][1]].len%160!=0)
                       {
	  	 	 	  	      	    if (numOfPage<chnMessage.message[layer3MenuLight[2][1]].len/160)
	  	 	 	  	            {
	  	 	 	  	      	       numOfPage++;
	  	 	 	  	            }
	  	 	 	  	            else
	  	 	 	  	            	nextInfo = 1;
	  	 	 	  	         }
	  	 	 	  	         else
	  	 	 	  	         {
	  	 	 	  	      	    if (numOfPage<chnMessage.message[layer3MenuLight[2][1]].len/160-1)
	  	 	 	  	            {
	  	 	 	  	      	       numOfPage++;
	  	 	 	  	            }
	  	 	 	  	            else
	  	 	 	  	            	nextInfo = 1;
	  	 	 	  	         }
	  	 	 	  	      }

	  	 	 	  	      if (nextInfo==1)
	  	 	 	  	      {
	  	 	 	  	         numOfPage = 0;
	  	 	 	  	         if (layer3MenuLight[2][1]+1>chnMessage.numOfMessage-1)
	  	 	 	  	         {
	  	 	 	  		          layer3MenuLight[2][1] = 0;
	  	 	 	             }
	  	 	 	             else
	  	 	 	             {
	  	 	 	    	          layer3MenuLight[2][1]++;
	  	 	 	    	       }
	  	 	 	    	    }
	  	 	 	    	  	chinese(layer3MenuLight[2][1],numOfPage,0);
      	  		  		break;

      	  		  	case 5:
      	  		  		if(layer2MenuLight[5]+2>totalAddGroup.numberOfzjz)
      	  		  		{
      	  		  			 layer2MenuLight[5] = 0;
      	  		  		}
      	  		  		else
      	  		  		{
      	  		  			 layer2MenuLight[5]++;
      	  		  		}      	  		  		
      	  		  		chargeInfo(layer2MenuLight[5]);
      	  		  		break;
      	  		  	
      	  		    case 6:
      	    		 	 	if (layer2MenuLight[6]+2>layer2MenuNum[6])
      	    		 	 	{
      	    		 	 		layer2MenuLight[6]=0;
      	    		 	 	}
      	    		 	 	else
      	    		 	 	{
      	    		 	 		layer2MenuLight[6]++;
      	    		 	 	}
      	    	 	 	  terminalInfo(layer2MenuLight[6]);
      	  		    	break;
      	  		  }
      	  		  break;
      	  		  
      	  		case 3:  //�˵���3��
      	  			switch(layer1MenuLight)
      	  			{
      	  				 case 0:  //ʵʱ����
      	  				 	 switch(layer2MenuLight[layer1MenuLight])
      	  				 	 {
      	  		  	 		 case 0:  //��ǰ����
      	  		  	 		 	 if (layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<2)
      	  		  	 		 	 {
      	  		  	 		 	 	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	  		  	 		 	 }
      	  		  	 		 	 else
      	  		  	 		 	 {
      	  		  	 		 	   if(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-2)
      	  		  	 		 	   {
      	  		  	 		 	   	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	  		  	 		 	   }
      	  		  	 		 	   else
      	  		  	 		 	   {
      	  		  	 		 	   	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;      	  		  	 		 	   	  
      	  		  	 		 	   }
      	  		  	 		 	 }
      	  		  	 		 	 currentPower(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	  		  	 		 	 break;

      	  		  	 		 case 1:  //��ǰ����
      	  		  	 		 	 if (layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<2)
      	  		  	 		 	 {
      	  		  	 		 	 	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	  		  	 		 	 }
      	  		  	 		 	 else
      	  		  	 		 	 {
      	  		  	 		 	   if(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-2)
      	  		  	 		 	   {
      	  		  	 		 	   	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	  		  	 		 	   }
      	  		  	 		 	   else
      	  		  	 		 	   {
      	  		  	 		 	   	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;      	  		  	 		 	   	  
      	  		  	 		 	   }
      	  		  	 		 	 }
      	  		  	 		 	 currentEnergy(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	  		  	 		 	 break;
      	  		  	 		 	 
      	  		  	 		 case 2:  //��������      	  		  	 		 	 
      	    		 	 	     //�����3-1(ѡ���ѯ����)��ȷ����Ҫ�ж������Ƿ���ȷ
      	    		 	 	     if (mpPowerCurveLight==0)
      	    		 	 	     {
      	    		 	 	 	     //�ж�����������Ƿ���ȷ
      	    		 	 	 	     if (checkInputTime()==FALSE)
      	    		 	 	 	     {
      	    		 	 	 	  	   return;
      	    		 	 	 	     }
      	    		 	 	 	  
      	    		 	 	 	     if (meterDeviceNum==0)
      	    		 	 	 	     {
      	    		 	 	 	  	   guiDisplay(12, 110, "δ���ò�������Ϣ!", 1);
      	    		 	 	 	  	   lcdRefresh(17, 130);
      	    		 	 	 	  	   return;
      	    		 	 	 	     }
      	    		 	 	     }
      	    		 	 	 
      	    		 	 	     //��ȷ������ʵ�ֲ�ͬ��������Լ���л�
      	    		 	 	     if (mpPowerCurveLight>=2)
      	    		 	 	     {
      	    		 	 	  	   mpPowerCurveLight=0;
      	    		 	 	     }
      	    		 	 	     else
      	    		 	 	     {
      	    		 	 	  	   mpPowerCurveLight++;
      	    		 	 	     }
                     
                         if (layer2MenuLight[layer1MenuLight]!=0)
                         {
                           keyLeftRight = 0;
                           layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = meterDeviceNum;
                           layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                         }
      	    		 	 	     powerCurve(mpPowerCurveLight,layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	     break;
      	    		 	 	     
      	    		 	 	   case 4:  //���ؼ�¼
      	    		 	 	   case 5:  //��ؼ�¼
      	    		 	 	   case 6:  //ң�ؼ�¼
      	    		 	 	   case 7:  //ʧ���¼
      	    		 	 	   	 if (eventLinkHead==NULL)
      	    		 	 	   	 {
      	    		 	 	   	 	 tmpEventShow = eventLinkHead;
      	    		 	 	   	 }
      	    		 	 	   	 else
      	    		 	 	   	 {
      	    		 	 	   	   if (tmpEventShow->next!=NULL)
      	    		 	 	   	   {
      	    		 	 	   	 	    tmpEventShow = tmpEventShow->next;
      	    		 	 	   	   }
      	    		 	 	   	   else
      	    		 	 	   	   {
      	    		 	 	   	 	    tmpEventShow = eventLinkHead;
      	    		 	 	   	 	 }
      	    		 	 	   	 }

      	    		 	 	   	 switch(layer2MenuLight[layer1MenuLight])
      	    		 	 	   	 {
      	    		 	 	   	   case 4:
      	    		 	 	   	     eventRecordShow(ERC(6));
      	    		 	 	   	     break;

      	    		 	 	   	   case 5:
      	    		 	 	   	     eventRecordShow(ERC(7));
      	    		 	 	   	     break;

      	    		 	 	   	   case 6:
      	    		 	 	   	     eventRecordShow(ERC(5));
      	    		 	 	   	     break;
      	    		 	 	   	     
      	    		 	 	   	   case 7:
      	    		 	 	   	     eventRecordShow(ERC(14));
      	    		 	 	   	     break;
      	    		 	 	   	 }      	    		 	 	   	 
      	    		 	 	   	 break;
      	  		  	 	 }
      	  		  	 	 break;

      	  				 case 1:  //������ֵ
      	  				 	 switch(layer2MenuLight[layer1MenuLight])
      	  				 	 {
                       case 0:   //ʱ�οز���
                         if ((layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]+2)>totalAddGroup.numberOfzjz)
                         {
                         	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                         }
                         else
                         {
                         	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
                         }
                         periodPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
                         break;

                       case 1:   //���ݿز���
                         if ((layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]+2)>totalAddGroup.numberOfzjz)
                         {
                         	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                         }
                         else
                         {
                         	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
                         }
                         wkdPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
                         break;
                         
                       case 2:  //�¸��ز���
                         if ((layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]+2)>totalAddGroup.numberOfzjz)
                         {
                         	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                         }
                         else
                         {
                         	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
                         }
                         powerDownPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
                       	 break;

      	  				 	 	 case 3:  //KvKiKp
      	  				 	 	 	 if (tmpMpLink!=NULL)
      	  				 	 	 	 {
      	  				 	 	 	   if (tmpMpLink->next!=NULL)
      	  				 	 	 	   {
      	  				 	 	 	 	    tmpMpLink = tmpMpLink->next;
      	  				 	 	 	   }
      	  				 	 	 	   else      	  				 	 	 	 
      	  				 	 	 	   {
      	  				 	 	 	 	    tmpMpLink = queryMpLink;
      	  				 	 	 	   }
      	  				 	 	 	   kvkikp(tmpMpLink);
      	  				 	 	 	 }
      	  				 	 	 	 break;

      	  				 	 	 case 4:  //���ܱ����
      	  				 	 	 	 if (tmpMpLink!=NULL)
      	  				 	 	 	 {
      	  				 	 	 	   if (tmpMpLink->next!=NULL)
      	  				 	 	 	   {
      	  				 	 	 	 	   tmpMpLink = tmpMpLink->next;
      	  				 	 	 	   }
      	  				 	 	 	   else      	  				 	 	 	 
      	  				 	 	 	   {
      	  				 	 	 	 	    tmpMpLink = queryMpLink;
      	  				 	 	 	   }
      	  				 	 	 	   meterPara(tmpMpLink);
      	  				 	 	 	 }
      	  				 	 	 	 break;
      	  				 	 	 	 
      	  				 	 	 case 5: //���ò���
      	    		 	 	     //ly,2011-03-31,ȡ���������������޸����ò���
      	    		 	 	     //pwLight = 0;
      	    		 	 	     //strcpy(passWord,"000000");
      	    		 	 	     //inputPassWord(pwLight);
                   
                         strcpy(commParaItem[0],digitalToChar(addrField.a1[1]>>4));
                         strcat(commParaItem[0],digitalToChar(addrField.a1[1]&0xf));
                         strcat(commParaItem[0],digitalToChar(addrField.a1[0]>>4));
                         strcat(commParaItem[0],digitalToChar(addrField.a1[0]&0xf));
                         
         	 	 	  	 	 	 	 #ifdef TE_ADDR_USE_BCD_CODE
         	 	 	  	 	 	 	  tmpChar = digitalToChar(addrField.a2[1]>>4);
                          tmpTeAddr[0] = *tmpChar;
         	 	 	  	 	 	 	  tmpChar = digitalToChar(addrField.a2[1]&0xf);
                          tmpTeAddr[1] = *tmpChar;
         	 	 	  	 	 	 	  tmpChar = digitalToChar(addrField.a2[0]>>4);
                          tmpTeAddr[2] = *tmpChar;
         	 	 	  	 	 	 	  tmpChar = digitalToChar(addrField.a2[0]&0xf);
                          tmpTeAddr[3] = *tmpChar;          	  	 	 	  	 	 	 	 
                          tmpTeAddr[4] = 0x0;
         	 	 	  	 	 	 	 #else
         	 	 	  	 	 	 	  tmpAddr = addrField.a2[1]<<8 | addrField.a2[0];
         	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr/10000);
         	 	 	  	 	 	 	  tmpTeAddr[0] = *tmpChar;
         	 	 	  	 	 	 	  tmpAddr %= 10000;
         	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr/1000);
         	 	 	  	 	 	 	  tmpTeAddr[1] = *tmpChar;
         	 	 	  	 	 	 	  tmpAddr %= 1000;
         	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr/100);
         	 	 	  	 	 	 	  tmpTeAddr[2] = *tmpChar;
         	 	 	  	 	 	 	  tmpAddr %= 100;
         	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr/10);
         	 	 	  	 	 	 	  tmpTeAddr[3] = *tmpChar;
         	 	 	  	 	 	 	  tmpAddr %= 10;
         	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr);
         	 	 	  	 	 	 	  tmpTeAddr[4] = *tmpChar;
         	 	 	  	 	 	 	  tmpTeAddr[5] = 0x0;
         	 	 	  	 	 	 	 #endif   	 	 	  	 	 	 	 
                         strcpy(commParaItem[1], tmpTeAddr);
          	  	 	 	  	 
                         for(i=0;i<4;i++)
                         {
                         	 if (strlen((char *)teApn[i])==0)
                         	 {
                         	   strcpy((char *)teApn[i],(char *)ipAndPort.apn);
                         	 }
                         }
                         strcpy(commParaItem[2],(char *)ipAndPort.apn);
                         
                         intToString(ipAndPort.port[1]<<8 | ipAndPort.port[0],3,strX);
                      	 strcpy(commParaItem[3],intToIpadd(ipAndPort.ipAddr[0]<<24 | ipAndPort.ipAddr[1]<<16 | ipAndPort.ipAddr[2]<<8 | ipAndPort.ipAddr[3],str));
                         strcat(commParaItem[3],":");
                         
                         for(i=0;i<5-strlen(strX);i++)
                         {
                         	  strcat(commParaItem[3],"0");
                         }
                         strcat(commParaItem[3],strX);
                         intToString(ipAndPort.portBak[1]<<8 | ipAndPort.portBak[0],3,strX);
                      	  strcpy(commParaItem[4],intToIpadd(ipAndPort.ipAddrBak[0]<<24 | ipAndPort.ipAddrBak[1]<<16 | ipAndPort.ipAddrBak[2]<<8 | ipAndPort.ipAddrBak[3],str));
                         strcat(commParaItem[4],":");
                         
                         for(i=0;i<5-strlen(strX);i++)
                         {
                         	 strcat(commParaItem[4],"0");
                         }
                         strcat(commParaItem[4],strX);
                         
                         keyLeftRight = 0;
                         rowOfLight = 0;
            	    			 configPara(rowOfLight, keyLeftRight);
      	    			 
      	  				 	 	 	 break;
      	  				 	 	 	 
      	    	 	 	    case 6:   //VPN�û�������
                 	 	 	  if (inputStatus == STATUS_SELECT_CHAR)
                 	 	 	  {
                 	 	 	  	inputStatus = STATUS_NONE;
                 	 	 	  	if (keyLeftRight<32)
                 	 	 	  	{
                 	 	 	  	  tmpVpnUserName[keyLeftRight] = character[selectIndex];
                 	 	 	  	}
                 	 	 	  	else
                 	 	 	  	{
                 	 	 	  	  tmpVpnPw[keyLeftRight-32] = character[selectIndex];
                 	 	 	  	}
                 	 	 	  	setVpn(keyLeftRight);
                 	 	 	  }
                 	 	 	  else
                 	 	 	  {
                          tmpVpnUserName[32] = '\0';
                          trim(tmpVpnUserName);
                          tmpVpnPw[32] = '\0';
                          trim(tmpVpnPw);
                          //strcpy((char *)vpn.vpnName, tmpVpnUserName);
                          //memcpy(vpn.vpnName, tmpVpnUserName, 16);
                          //ly,2012-03-12,���������ݷ��ִ���
                          memcpy(vpn.vpnName, tmpVpnUserName, 32);
                          //strcpy((char *)vpn.vpnPassword, tmpVpnPw);
                          //memcpy(vpn.vpnPassword, tmpVpnPw, 16);
                          memcpy(vpn.vpnPassword, tmpVpnPw, 32);

        	  	 	 	      	//����vpn�û�������
        	  	 	 	      	saveParameter(0x04, 16,(INT8U *)&vpn, sizeof(VPN));

                        	saveBakKeyPara(16);    //2012-8-9,add

                          guiLine(10,55,150,105,0);
                          guiLine(10,55,10,105,1);
                          guiLine(150,55,150,105,1);
                          guiLine(10,55,150,55,1);
                          guiLine(10,105,150,105,1);
                          guiDisplay(20, 70, "ר���������޸�!",1);
                          lcdRefresh(10,120);

                          menuInLayer--;
                 	 	 	  }
                 	 	 	  break;
                 	 	 	
                 	 	 	case 7:  //�������
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==7)
      	    		 	 	    {
        	  	 	 	      	//�������
        	  	 	 	      	meterConfig.measurePoint = (chrMp[0][0]-0x30)*1000+(chrMp[0][1]-0x30)*100+(chrMp[0][2]-0x30)*10+(chrMp[0][3]-0x30);
        	  	 	 	      	
        	  	 	 	      	//���
        	  	 	 	      	meterConfig.number = meterConfig.measurePoint;

        	  	 	 	      	//�˿ں�����
        	  	 	 	      	meterConfig.rateAndPort = (chrMp[1][0]-0x30)<<5;
        	  	 	 	      	switch(chrMp[2][0])
        	  	 	 	      	{
        	  	 	 	      		case 0x30:   //���ɶ˿�Ϊ1
        	  	 	 	      		 	meterConfig.rateAndPort |= 0x1;
        	  	 	 	      		 	break;

        	  	 	 	      		case 0x31:   //RS485-1�˿�Ϊ2
        	  	 	 	      		 	meterConfig.rateAndPort |= 0x2;
        	  	 	 	      		 	break;

        	  	 	 	      		case 0x32:   //RS485-2�˿�Ϊ3
        	  	 	 	      		 	meterConfig.rateAndPort |= 0x3;
        	  	 	 	      		 	break;
        	  	 	 	      	}
        	  	 	 	      	
        	  	 	 	      	//Э��
        	  	 	 	      	switch(chrMp[3][0])
        	  	 	 	        {
        	  	 	 	        	 case 0x32:
        	  	 	 	        	 	 meterConfig.protocol = 02;
        	  	 	 	        	 	 break;

        	  	 	 	        	 case 0x31:
        	  	 	 	        	 	 meterConfig.protocol = 30;
        	  	 	 	        	 	 break;
        	  	 	 	        	 	 
                             case 0x33:
                             	 meterConfig.protocol = ABB_METER;
                             	 break;

                             case 0x34:
                             	 meterConfig.protocol = SIMENS_ZD_METER;
                             	 break;

                             case 0x35:
                             	 meterConfig.protocol = EDMI_METER;
                             	 break;
														 
                             case 0x36:
                             	 meterConfig.protocol = MODBUS_MW_F;
                             	 break;
														 
                             case 0x37:
                             	 meterConfig.protocol = MODBUS_MW_UI;
                             	 break;
                             	 
        	  	 	 	        	 default:
        	  	 	 	        	 	 meterConfig.protocol = 1;
        	  	 	 	        	 	 break;
        	  	 	 	        }
        	  	 	 	        
     	  	 	 	      	  	//����ַ
     	  	 	 	      	  	meterConfig.addr[5] = (chrMp[4][0]-0x30)<<4 | (chrMp[4][1]-0x30);
     	  	 	 	      	  	meterConfig.addr[4] = (chrMp[4][2]-0x30)<<4 | (chrMp[4][3]-0x30);
     	  	 	 	      	  	meterConfig.addr[3] = (chrMp[4][4]-0x30)<<4 | (chrMp[4][5]-0x30);
     	  	 	 	      	  	meterConfig.addr[2] = (chrMp[4][6]-0x30)<<4 | (chrMp[4][7]-0x30);
     	  	 	 	      	  	meterConfig.addr[1] = (chrMp[4][8]-0x30)<<4 | (chrMp[4][9]-0x30);
     	  	 	 	      	  	meterConfig.addr[0] = (chrMp[4][10]-0x30)<<4 | (chrMp[4][11]-0x30);
     	  	 	 	      	  	
     	  	 	 	      	  	//�ɼ�����ַ
     	  	 	 	      	  	for(i=0;i<6;i++)
     	  	 	 	      	    {
     	  	 	 	      	  	  meterConfig.collectorAddr[i] = 0x0;
     	  	 	 	      	  	}
     	  	 	 	      	  	
     	  	 	 	      	  	//����λ��С��λ����
     	  	 	 	      	  	meterConfig.mixed = 0x05;
     	  	 	 	      	  	
     	  	 	 	      	  	//���ʸ���
     	  	 	 	      	  	meterConfig.numOfTariff = 4;
     	  	 	 	      	  	
     	  	 	 	      	  	//����ż�С���
     	  	 	 	      	  	meterConfig.bigAndLittleType = 0x0;
     	  	 	 	      	  	tmpData = (chrMp[5][0]-0x30)*10+chrMp[5][1]-0x30;
     	  	 	 	      	  	if (tmpData<=15)
     	  	 	 	      	  	{
     	  	 	 	      	  		 meterConfig.bigAndLittleType |= tmpData<<4;
     	  	 	 	      	  	}
     	  	 	 	      	  	tmpData = (chrMp[6][0]-0x30)*10+chrMp[6][1]-0x30;
     	  	 	 	      	  	if (tmpData<=15)
     	  	 	 	      	  	{
     	  	 	 	      	  		 meterConfig.bigAndLittleType |= tmpData;
     	  	 	 	      	  	}
                              
                          //����
  		                    saveDataF10(meterConfig.measurePoint, meterConfig.rateAndPort&0x1f, meterConfig.addr, meterConfig.number, (INT8U *)&meterConfig, 27);
                              
                          guiLine(10,55,150,105,0);
                          guiLine(10,55,10,105,1);
                          guiLine(150,55,150,105,1);
                          guiLine(10,55,150,55,1);
                          guiLine(10,105,150,105,1);
                          guiDisplay(27,70,"������óɹ�!",1);
                          lcdRefresh(10,120);
                              
                          menuInLayer--;

                          return;
      	    		 	 	    }
      	    		 	 	       
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
      	    		 	 	    {
      	    		 	 	    	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	    }
      	    		 	 	    else
      	    		 	 	    {
      	    		 	 	      layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    //�л�������ʱ�������еĲ�������Ϣ
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
      	    		 	 	    {
      	    		 	 	    	 tmpData = (chrMp[0][0]-0x30)*1000 + (chrMp[0][1]-0x30)*100 + (chrMp[0][2]-0x30)*10 + (chrMp[0][3]-0x30);
      	    		 	 	    	 if (tmpData>2040 || tmpData<1)
      	    		 	 	    	 {
                              guiLine(10,55,150,105,0);
                              guiLine(10,55,10,105,1);
                              guiLine(150,55,150,105,1);
                              guiLine(10,55,150,55,1);
                              guiLine(10,105,150,105,1);
                              guiDisplay(20,70,"�������������!",1);
                              lcdRefresh(10,120);
      	    		 	 	    	 	  
      	    		 	 	    	 	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
      	    		 	 	    	 	  return;
      	    		 	 	    	 }
                           
                           if (selectF10Data(tmpData, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
                           {
                           	  //����
                           	  chrMp[1][0] = 0x30+(meterConfig.rateAndPort>>5);
                           	  
                           	  //�˿�
                           	  switch(meterConfig.rateAndPort&0x1f)
                           	  {
                           	  	 case 1:
                           	  	 	 chrMp[2][0] = 0x30;
                           	  	 	 break;
                           	  	 	 
                           	  	 case 2:
                           	  	 	 chrMp[2][0] = 0x31;
                           	  	 	 break;

                           	  	 case 3:
                           	  	 	 chrMp[2][0] = 0x32;
                           	  	 	 break;
                           	  }

                              //char       chrCopyProtocol[3][13]={"DL/T645-1997","DL/T645-2007","��������","ABB����","������ZD��","����EDMI��"};
                           	  switch(meterConfig.protocol)
                           	  {
                           	  	 case 2:
                           	  	 	 chrMp[3][0] = 0x32;
                           	  	 	 break;
                                 	 
                           	  	 case 30:
                           	  	 	 chrMp[3][0] = 0x31;
                           	  	 	 break;

                                 case ABB_METER:
                           	  	 	 chrMp[3][0] = 0x33;
                                 	 break;

                                 case SIMENS_ZD_METER:
                           	  	 	 chrMp[3][0] = 0x34;
                                 	 break;

                                 case EDMI_METER:
                           	  	 	 chrMp[3][0] = 0x35;
                                 	 break;
																 
                                 case MODBUS_MW_F:
                           	  	 	 chrMp[3][0] = 0x36;
                                 	 break;
                           	  	 	 
																 case MODBUS_MW_UI:
																	 chrMp[3][0] = 0x37;
																	 break;
																 
                           	  	 default:
                           	  	 	 chrMp[3][0] = 0x30;
                           	  	 	 break;
                           	  }
                           	  
                           	  //���ַ
                  	 	 	   	  chrMp[4][0]  = (meterConfig.addr[5]>>4)+0x30;
                  	 	 	   	  chrMp[4][1]  = (meterConfig.addr[5]&0xf)+0x30;
                  	 	 	   	  chrMp[4][2]  = (meterConfig.addr[4]>>4)+0x30;
                  	 	 	   	  chrMp[4][3]  = (meterConfig.addr[4]&0xf)+0x30;
                  	 	 	   	  chrMp[4][4]  = (meterConfig.addr[3]>>4)+0x30;
                  	 	 	   	  chrMp[4][5]  = (meterConfig.addr[3]&0xf)+0x30;
                  	 	 	   	  chrMp[4][6]  = (meterConfig.addr[2]>>4)+0x30;
                  	 	 	   	  chrMp[4][7]  = (meterConfig.addr[2]&0xf)+0x30;
                  	 	 	   	  chrMp[4][8]  = (meterConfig.addr[1]>>4)+0x30;
                  	 	 	   	  chrMp[4][9]  = (meterConfig.addr[1]&0xf)+0x30;
                  	 	 	   	  chrMp[4][10] = (meterConfig.addr[0]>>4)+0x30;
                  	 	 	   	  chrMp[4][11] = (meterConfig.addr[0]&0xf)+0x30;
                  	 	 	   	  chrMp[4][12] = '\0';
                  	 	 	   	  
                  	 	 	   	  //�����
                  	 	 	   	  chrMp[5][0] = 0x30+((meterConfig.bigAndLittleType>>4)/10);
                  	 	 	   	  chrMp[5][1] = 0x30+((meterConfig.bigAndLittleType>>4)%10);

                  	 	 	   	  //С���
                  	 	 	   	  chrMp[6][0] = 0x30+((meterConfig.bigAndLittleType&0xf)/10);
                  	 	 	   	  chrMp[6][1] = 0x30+((meterConfig.bigAndLittleType&0xf)%10);                  	 	 	   	  
                           }
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    keyLeftRight = 0;
      	    		 	 	    setZbMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		break;

      	    	 	 	  	case 10:   //�����ն˼���
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==5)
      	    		 	 	    {
        	  	 	 	      	//�����˿�
        	  	 	 	      	if (chrMp[0][0]!=0x33)
        	  	 	 	      	{
                             guiLine(10,55,150,105,0);
                             guiLine(10,55,10,105,1);
                             guiLine(150,55,150,105,1);
                             guiLine(10,55,150,55,1);
                             guiLine(10,105,150,105,1);
                             guiDisplay(20,60,"�������!�����˿�ֻ",1);
                             guiDisplay(20,80,"��Ϊ3",1);
                             lcdRefresh(10,120);
      	    		 	 	    	 	 
      	    		 	 	    	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
                             return;
        	  	 	 	      	}
        	  	 	 	      	
        	  	 	 	      	cascadeCommPara.commPort = 0x03; //�˿�3
        	  	 	 	      	cascadeCommPara.ctrlWord = 0x8B;             //Ĭ��4800,8-e-1,Ŀǰ���ֻ��ͨ�������
        	  	 	 	      	cascadeCommPara.receiveMsgTimeout = 0x05;
        	  	 	 	      	cascadeCommPara.receiveByteTimeout = 0x05;
        	  	 	 	      	cascadeCommPara.cascadeMretryTime = 0x01;
        	  	 	 	      	cascadeCommPara.groundSurveyPeriod = 0x05;   //Ѳ������
        	  	 	 	      	
        	  	 	 	      	cascadeCommPara.flagAndTeNumber = 0x00;
        	  	 	 	      	if (chrMp[1][0]==0x30)
        	  	 	 	      	{
        	  	 	 	      	  cascadeCommPara.divisionCode[0] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[1] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[2] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[3] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[4] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[5] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[0] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[1] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[2] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[3] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[4] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[5] = 0x00;
        	  	 	 	      	}
        	  	 	 	      	else
        	  	 	 	      	{
   	  	 	 	      	  	    //�ն˵�ַ1
   	  	 	 	      	  	    tmpAddr = (chrMp[2][4]-0x30)*10000+(chrMp[2][5]-0x30)*1000
   	  	 	 	      	  	            +(chrMp[2][6]-0x30)*100+(chrMp[2][7]-0x30)*10
   	  	 	 	      	  	            +(chrMp[2][8]-0x30);
        	  	 	 	      	  tmpData = (chrMp[2][0]-0x30)<<12 | (chrMp[2][1]-0x30)<<8 | (chrMp[2][2]-0x30)<<4 | (chrMp[2][3]-0x30);
        	  	 	 	      	  if (tmpData!=0x0 && tmpAddr!=0x00)
        	  	 	 	      	  {
        	  	 	 	      	  	cascadeCommPara.flagAndTeNumber++;

   	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[1] = tmpAddr>>8;
   	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[0] = tmpAddr&0xff;
   	  	 	 	      	  	      cascadeCommPara.divisionCode[1] = (chrMp[2][0]-0x30)<<4 | (chrMp[2][1]-0x30);
   	  	 	 	      	  	      cascadeCommPara.divisionCode[0] = (chrMp[2][2]-0x30)<<4 | (chrMp[2][3]-0x30);
        	  	 	 	      	  }

        	  	 	 	      		if (chrMp[1][0]==0x32)
        	  	 	 	      		{
        	  	 	 	      			cascadeCommPara.flagAndTeNumber |= 0x80;
        	  	 	 	      		}
        	  	 	 	      		else
        	  	 	 	      		{
     	  	 	 	      	  	    //�ն˵�ַ2
     	  	 	 	      	  	    tmpAddr = (chrMp[3][4]-0x30)*10000+(chrMp[3][5]-0x30)*1000
     	  	 	 	      	  	            +(chrMp[3][6]-0x30)*100+(chrMp[3][7]-0x30)*10
     	  	 	 	      	  	            +(chrMp[3][8]-0x30);
          	  	 	 	      	  tmpData = (chrMp[3][0]-0x30)<<12 | (chrMp[3][1]-0x30)<<8 | (chrMp[3][2]-0x30)<<4 | (chrMp[3][3]-0x30);
          	  	 	 	      	  if (tmpData!=0x0 && tmpAddr!=0x00)
          	  	 	 	      	  {
          	  	 	 	      	  	cascadeCommPara.flagAndTeNumber++;
  
     	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[3] = tmpAddr>>8;
     	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[2] = tmpAddr&0xff;
     	  	 	 	      	  	      cascadeCommPara.divisionCode[3] = (chrMp[3][0]-0x30)<<4 | (chrMp[3][1]-0x30);
     	  	 	 	      	  	      cascadeCommPara.divisionCode[2] = (chrMp[3][2]-0x30)<<4 | (chrMp[3][3]-0x30);
          	  	 	 	      	  }

     	  	 	 	      	  	    //�ն˵�ַ3
     	  	 	 	      	  	    tmpAddr = (chrMp[4][4]-0x30)*10000+(chrMp[4][5]-0x30)*1000
     	  	 	 	      	  	            +(chrMp[4][6]-0x30)*100+(chrMp[4][7]-0x30)*10
     	  	 	 	      	  	            +(chrMp[4][8]-0x30);
          	  	 	 	      	  tmpData = (chrMp[4][0]-0x30)<<12 | (chrMp[4][1]-0x30)<<8 | (chrMp[4][2]-0x30)<<4 | (chrMp[4][3]-0x30);
          	  	 	 	      	  if (tmpData!=0x0 && tmpAddr!=0x00)
          	  	 	 	      	  {
          	  	 	 	      	  	cascadeCommPara.flagAndTeNumber++;
  
     	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[5] = tmpAddr>>8;
     	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[4] = tmpAddr&0xff;
     	  	 	 	      	  	      cascadeCommPara.divisionCode[5] = (chrMp[4][0]-0x30)<<4 | (chrMp[4][1]-0x30);
     	  	 	 	      	  	      cascadeCommPara.divisionCode[4] = (chrMp[4][2]-0x30)<<4 | (chrMp[4][3]-0x30);
          	  	 	 	      	  }
        	  	 	 	      		}
        	  	 	 	      	}
        	  	 	 	      	
                          saveParameter(0x04, 37, (INT8U *)&cascadeCommPara, sizeof(CASCADE_COMM_PARA));

                          guiLine(10,55,150,105,0);
                          guiLine(10,55,10,105,1);
                          guiLine(150,55,150,105,1);
                          guiLine(10,55,150,55,1);
                          guiLine(10,105,150,105,1);
                          guiDisplay(12,70,"�����������óɹ�!",1);
                          lcdRefresh(10,120);
                              
                          menuInLayer--;
                          return;        	  	 	 	      	
        	  	 	 	      }

      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
      	    		 	 	    {
      	    		 	 	    	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	    }
      	    		 	 	    else
      	    		 	 	    {
      	    		 	 	      layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    keyLeftRight = 0;
      	    		 	 	    setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		break;

     	    	 	 	  	  case 12:    //ά���ӿ�ģʽ����
       	    	 	 	  	  switch (keyLeftRight)
       	    	 	 	  	  {
       	    	 	 	  	    case 0x01:
       	    	 	 	  	  	  mainTainPortMode = 0x55;
       	    	 	 	  	  	  break;
       	    	 	 	  	  
       	    	 	 	  	    default:
       	    	 	 	  	  	  mainTainPortMode = 0;
       	    	 	 	  	  	  break;
       	    	 	 	  	  }
       	    	 	 	  	  
                        guiLine(10,55,150,105,0);
                        guiLine(10,55,10,105,1);
                        guiLine(150,55,150,105,1);
                        guiLine(10,55,150,55,1);
                        guiLine(10,105,150,105,1);
                        guiDisplay(9,70,"ά����ģʽ���óɹ�",1);
                        lcdRefresh(10,120);
                            
                        menuInLayer--;
                        return;
     	    	 	 	  	    break;

     	    	 	 	  	  case 13:    //��2·485�ڹ�������
       	    	 	 	  	  switch (keyLeftRight)
       	    	 	 	  	  {
       	    	 	 	  	    case 0x01:
       	    	 	 	  	  	  rs485Port2Fun = 0x55;
       	    	 	 	  	  	 
       	    	 	 	  	  	  //���õ�2·485������Ϊ9600-8-e-1
                            #ifdef WDOG_USE_X_MEGA
                             str[0] = 0x02;    //xMega�˿�2
                             str[1] = 0xcb;    //�˿�����,9600-8-e-1
                             sendXmegaFrame(COPY_PORT_RATE_SET,(INT8U *)str, 2);
                             
                             printf("���õ�2·����Ϊ9600-8-e-1\n");
                            #endif

       	    	 	 	  	  	  break;
       	    	 	 	  	  
       	    	 	 	  	    default:
       	    	 	 	  	  	  rs485Port2Fun = 0;
       	    	 	 	  	  	  break;
       	    	 	 	  	  }
       	    	 	 	  	  
                        guiLine(10,55,150,105,0);
                        guiLine(10,55,10,105,1);
                        guiLine(150,55,150,105,1);
                        guiLine(10,55,150,55,1);
                        guiLine(10,105,150,105,1);
                        guiDisplay(9,70,"485��2�������óɹ�",1);
                        lcdRefresh(10,120);
                            
                        menuInLayer--;
                        return;
     	    	 	 	  	    break;
      	    	 	 	  		
      	    	 	 	    case 14:  //��̫����������
    	    		 	 	      if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
    	    		 	 	      {
        	  	 	 	      	//��̫��IP��ַ
        	  	 	 	      	teIpAndPort.teIpAddr[0] =(chrEthPara[0][0]-0x30)*100+(chrEthPara[0][1]-0x30)*10+(chrEthPara[0][2]-0x30);
        	  	 	 	      	teIpAndPort.teIpAddr[1] =(chrEthPara[0][4]-0x30)*100+(chrEthPara[0][5]-0x30)*10+(chrEthPara[0][6]-0x30);
        	  	 	 	      	teIpAndPort.teIpAddr[2] =(chrEthPara[0][8]-0x30)*100+(chrEthPara[0][9]-0x30)*10+(chrEthPara[0][10]-0x30);
        	  	 	 	      	teIpAndPort.teIpAddr[3] =(chrEthPara[0][12]-0x30)*100+(chrEthPara[0][13]-0x30)*10+(chrEthPara[0][14]-0x30);
   
        	  	 	 	      	//��̫��IP����
        	  	 	 	      	teIpAndPort.mask[0] =(chrEthPara[1][0]-0x30)*100+(chrEthPara[1][1]-0x30)*10+(chrEthPara[1][2]-0x30);
        	  	 	 	      	teIpAndPort.mask[1] =(chrEthPara[1][4]-0x30)*100+(chrEthPara[1][5]-0x30)*10+(chrEthPara[1][6]-0x30);
        	  	 	 	      	teIpAndPort.mask[2] =(chrEthPara[1][8]-0x30)*100+(chrEthPara[1][9]-0x30)*10+(chrEthPara[1][10]-0x30);
        	  	 	 	      	teIpAndPort.mask[3] =(chrEthPara[1][12]-0x30)*100+(chrEthPara[1][13]-0x30)*10+(chrEthPara[1][14]-0x30);
   
        	  	 	 	      	//��̫������
        	  	 	 	      	teIpAndPort.gateWay[0] =(chrEthPara[2][0]-0x30)*100+(chrEthPara[2][1]-0x30)*10+(chrEthPara[2][2]-0x30);
        	  	 	 	      	teIpAndPort.gateWay[1] =(chrEthPara[2][4]-0x30)*100+(chrEthPara[2][5]-0x30)*10+(chrEthPara[2][6]-0x30);
        	  	 	 	      	teIpAndPort.gateWay[2] =(chrEthPara[2][8]-0x30)*100+(chrEthPara[2][9]-0x30)*10+(chrEthPara[2][10]-0x30);
        	  	 	 	      	teIpAndPort.gateWay[3] =(chrEthPara[2][12]-0x30)*100+(chrEthPara[2][13]-0x30)*10+(chrEthPara[2][14]-0x30);
        	  	 	 	      	
   
        	  	 	 	      	//����
                          saveIpMaskGateway(teIpAndPort.teIpAddr,teIpAndPort.mask,teIpAndPort.gateWay);  //���浽rcS��,ly,2011-04-12
    
                          saveParameter(0x04, 7, (INT8U *)&teIpAndPort, sizeof(TE_IP_AND_PORT));
                              
                          guiLine(6,55,154,105,0);
                          guiLine(6,55,6,105,1);
                          guiLine(154,55,154,105,1);
                          guiLine(6,55,154,55,1);
                          guiLine(6,105,154,105,1);
                          guiDisplay(8,60,"�޸���̫�������ɹ�",1);
                          guiDisplay(16,80,"������Ч��Ҫ����",1);
                          lcdRefresh(10,120);
                              
                          menuInLayer--;
    	    		 	 	      }
    	    		 	 	      else
    	    		 	 	      {
    	    		 	 	         if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
    	    		 	 	         {
    	    		 	 	      	   layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
    	    		 	 	         }
    	    		 	 	         else
    	    		 	 	         {
    	    		 	 	       	   layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
    	    		 	 	         }
    	    		 	 	         keyLeftRight = 0;
    	    		 	 	         setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
    	    		 	 	      }
      	    	 	 	    	break;
      	  				 	 }
      	  				 	 break;
      	  			}
      	  			break;
      	  	  
      	  	  case 4:  //�˵���4��
    		 	 	    if (rowOfLight == 5)
    		 	 	    {
 	  	 	 	      	//��IP��ַ
 	  	 	 	      	ipAndPort.ipAddr[0] =(commParaItem[3][0]-0x30)*100+(commParaItem[3][1]-0x30)*10+(commParaItem[3][2]-0x30);
 	  	 	 	      	ipAndPort.ipAddr[1] =(commParaItem[3][4]-0x30)*100+(commParaItem[3][5]-0x30)*10+(commParaItem[3][6]-0x30);
 	  	 	 	      	ipAndPort.ipAddr[2] =(commParaItem[3][8]-0x30)*100+(commParaItem[3][9]-0x30)*10+(commParaItem[3][10]-0x30);
 	  	 	 	      	ipAndPort.ipAddr[3] =(commParaItem[3][12]-0x30)*100+(commParaItem[3][13]-0x30)*10+(commParaItem[3][14]-0x30);
 	  	 	 	      	  	
 	  	 	 	      	//���˿�
 	  	 	 	      	tmpData = (commParaItem[3][16]-0x30)*10000+(commParaItem[3][17]-0x30)*1000
 	  	 	 	      	         +(commParaItem[3][18]-0x30)*100+(commParaItem[3][19]-0x30)*10
 	  	 	 	      	         +(commParaItem[3][20]-0x30);
 	  	 	 	      	ipAndPort.port[1] = tmpData>>8;
 	  	 	 	      	ipAndPort.port[0] = tmpData&0xff;

 	  	 	 	      	//���õ�ַ
 	  	 	 	      	ipAndPort.ipAddrBak[0] =(commParaItem[4][0]-0x30)*100+(commParaItem[4][1]-0x30)*10+(commParaItem[4][2]-0x30);
 	  	 	 	      	ipAndPort.ipAddrBak[1] =(commParaItem[4][4]-0x30)*100+(commParaItem[4][5]-0x30)*10+(commParaItem[4][6]-0x30);
 	  	 	 	      	ipAndPort.ipAddrBak[2] =(commParaItem[4][8]-0x30)*100+(commParaItem[4][9]-0x30)*10+(commParaItem[4][10]-0x30);
 	  	 	 	      	ipAndPort.ipAddrBak[3] =(commParaItem[4][12]-0x30)*100+(commParaItem[4][13]-0x30)*10+(commParaItem[4][14]-0x30);
 	  	 	 	      	  	
 	  	 	 	      	//���ö˿�
 	  	 	 	      	tmpData = (commParaItem[4][16]-0x30)*10000+(commParaItem[4][17]-0x30)*1000
 	  	 	 	      	         +(commParaItem[4][18]-0x30)*100+(commParaItem[4][19]-0x30)*10
 	  	 	 	      	         +(commParaItem[4][20]-0x30);
 	  	 	 	      	ipAndPort.portBak[1] = tmpData>>8;
 	  	 	 	      	ipAndPort.portBak[0] = tmpData&0xff;
 	  	 	 	      	  	        	  	 	 	      	  	
 	  	 	 	      	strcpy((char *)ipAndPort.apn, commParaItem[2]);

 	  	 	 	      	//����IP��ַ
 	  	 	 	      	saveParameter(0x04, 3,(INT8U *)&ipAndPort,sizeof(IP_AND_PORT));
                  saveBakKeyPara(3);    //2012-8-9,add

          	  	 	addrField.a1[1] = (commParaItem[0][0]-0x30)<<4 | (commParaItem[0][1]-0x30);
          	  	 	addrField.a1[0] = (commParaItem[0][2]-0x30)<<4 | (commParaItem[0][3]-0x30);
 	 	 	      	    
 	 	 	      	    #ifdef TE_ADDR_USE_BCD_CODE
 	 	 	      	  	 addrField.a2[1] = (commParaItem[1][0]-0x30)<<4 | (commParaItem[1][1]-0x30);
 	 	 	      	  	 addrField.a2[0] = (commParaItem[1][2]-0x30)<<4 | (commParaItem[1][3]-0x30);
 	 	 	      	    #else
 	 	 	      	  	 tmpAddr = (commParaItem[1][0]-0x30)*10000+(commParaItem[1][1]-0x30)*1000
 	 	 	      	  	         +(commParaItem[1][2]-0x30)*100+(commParaItem[1][3]-0x30)*10
 	 	 	      	  	         +(commParaItem[1][4]-0x30);
 	 	 	      	  	 addrField.a2[1] = tmpAddr>>8;
 	 	 	      	  	 addrField.a2[0] = tmpAddr&0xff;
 	 	 	      	    #endif

                  //��������������
                  saveParameter(0x04, 121,(INT8U *)&addrField,4);
                  
                  saveBakKeyPara(121);    //2012-8-9,add

                  guiLine(10,55,150,105,0);
                  guiLine(10,55,10,105,1);
                  guiLine(150,55,150,105,1);
                  guiLine(10,55,150,55,1);
                  guiLine(10,105,150,105,1);
                  guiDisplay(12,70,"�޸�ͨ�Ų����ɹ�!",1);
                  lcdRefresh(10,120);
                       
                  menuInLayer--;
                  
                  //2012-10-22,add,�޸�IP����������IP��¼
                  if (bakModuleType==MODEM_PPP)
                  {
                    resetPppWlStatus();
                  }
                  else
                  {
                  	wlModemPowerOnOff(0);
                  }
                  return;
    		 	 	    }
      	    		 	 	    
 		 	 	        if (rowOfLight >= 5)
 		 	 	        {
 		 	 	       	  rowOfLight = 0;
 		 	 	        }
 		 	 	        else
 		 	 	        {
 		 	 	       	  rowOfLight++;
 		 	 	        }
 		 	 	        keyLeftRight = 0;
 		 	 	        configPara(rowOfLight, keyLeftRight);
      	    		break;
      	    		
             case 5:    //�˵���5��
             	 if (layer1MenuLight==1 && layer2MenuLight[1]==5)
             	 {
           	 	 	  if (inputStatus == STATUS_NONE)
           	 	 	  {
                    keyLeftRight = 0;
             	 	 	  
             	 	 	  for(i=15;i>0;i--)
             	 	 	  {
             	 	 	  	if (commParaItem[2][i]!=' ' && i<15)
             	 	 	  	{
             	 	 	  		 commParaItem[2][i+1] = '\0';
             	 	 	  		 break;
             	 	 	  	}
             	 	 	  }

                    rowOfLight++;
    	    		 	 	 	configPara(rowOfLight, keyLeftRight);
           	 	 	  }
           	 	 	  if (inputStatus == STATUS_SELECT_CHAR)
           	 	 	  {
           	 	 	  	inputStatus = STATUS_NONE;
           	 	 	  	commParaItem[2][inputIndex] = character[selectIndex];
           	 	 	  	inputApn(inputIndex);
           	 	 	  }
             	  }
             	 	break;
      	  			
      	    	case 20:  //�˵���20��(��������)��ȷ��
      	    		//ly,2011-03-31,ȡ���������������޸����ò���
      	    		/*
      	    		for(i=0;i<6;i++)
      	    		{
      	    		 	 if (originPassword[i]!=passWord[i])
      	    		 	 {
      	    		 	  	guiDisplay(30,120,"�����������!",1);
      	    		 	  	lcdRefresh(120,137);
      	    		 	  	return;
      	    		 	 }
      	    		}
      	    		 
      	    		//������������ǰ�ĸ����˵���,ִ����Ӧ�Ķ���
      	    		if (layer1MenuLight==1 || layer2MenuLight[1]==5)
      	    		{
                   strcpy(commParaItem[0],digitalToChar(addrField.a1[1]>>4));
                   strcat(commParaItem[0],digitalToChar(addrField.a1[1]&0xf));
                   strcat(commParaItem[0],digitalToChar(addrField.a1[0]>>4));
                   strcat(commParaItem[0],digitalToChar(addrField.a1[0]&0xf));
                   
   	 	 	  	 	 	 	 #ifdef TE_ADDR_USE_BCD_CODE
   	 	 	  	 	 	 	  tmpChar = digitalToChar(addrField.a2[1]>>4);
                    tmpTeAddr[0] = *tmpChar;
   	 	 	  	 	 	 	  tmpChar = digitalToChar(addrField.a2[1]&0xf);
                    tmpTeAddr[1] = *tmpChar;
   	 	 	  	 	 	 	  tmpChar = digitalToChar(addrField.a2[0]>>4);
                    tmpTeAddr[2] = *tmpChar;
   	 	 	  	 	 	 	  tmpChar = digitalToChar(addrField.a2[0]&0xf);
                    tmpTeAddr[3] = *tmpChar;          	  	 	 	  	 	 	 	 
                    tmpTeAddr[4] = 0x0;
   	 	 	  	 	 	 	 #else
   	 	 	  	 	 	 	  tmpAddr = addrField.a2[1]<<8 | addrField.a2[0];
   	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr/10000);
   	 	 	  	 	 	 	  tmpTeAddr[0] = *tmpChar;
   	 	 	  	 	 	 	  tmpAddr %= 10000;
   	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr/1000);
   	 	 	  	 	 	 	  tmpTeAddr[1] = *tmpChar;
   	 	 	  	 	 	 	  tmpAddr %= 1000;
   	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr/100);
   	 	 	  	 	 	 	  tmpTeAddr[2] = *tmpChar;
   	 	 	  	 	 	 	  tmpAddr %= 100;
   	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr/10);
   	 	 	  	 	 	 	  tmpTeAddr[3] = *tmpChar;
   	 	 	  	 	 	 	  tmpAddr %= 10;
   	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr);
   	 	 	  	 	 	 	  tmpTeAddr[4] = *tmpChar;
   	 	 	  	 	 	 	  tmpTeAddr[5] = 0x0;
   	 	 	  	 	 	 	 #endif   	 	 	  	 	 	 	 
                   strcpy(commParaItem[1], tmpTeAddr);
    	  	 	 	  	 
                   for(i=0;i<4;i++)
                   {
                   	 if (strlen((char *)teApn[i])==0)
                   	 {
                   	   strcpy((char *)teApn[i],(char *)ipAndPort.apn);
                   	 }
                   }
                   strcpy(commParaItem[2],(char *)ipAndPort.apn);
                   
                   intToString(ipAndPort.port[1]<<8 | ipAndPort.port[0],3,strX);
                	 strcpy(commParaItem[3],intToIpadd(ipAndPort.ipAddr[0]<<24 | ipAndPort.ipAddr[1]<<16 | ipAndPort.ipAddr[2]<<8 | ipAndPort.ipAddr[3],str));
                   strcat(commParaItem[3],":");
                   
                   for(i=0;i<5-strlen(strX);i++)
                   {
                   	  strcat(commParaItem[3],"0");
                   }
                   strcat(commParaItem[3],strX);
                   intToString(ipAndPort.portBak[1]<<8 | ipAndPort.portBak[0],3,strX);
                	  strcpy(commParaItem[4],intToIpadd(ipAndPort.ipAddrBak[0]<<24 | ipAndPort.ipAddrBak[1]<<16 | ipAndPort.ipAddrBak[2]<<8 | ipAndPort.ipAddrBak[3],str));
                   strcat(commParaItem[4],":");
                   
                   for(i=0;i<5-strlen(strX);i++)
                   {
                   	 strcat(commParaItem[4],"0");
                   }
                   strcat(commParaItem[4],strX);
                   
                   keyLeftRight = 0;
                   rowOfLight = 0;
      	    			 configPara(rowOfLight, keyLeftRight);
      	    		}
      	    		*/
      	    		break;
      	  	}
      	  	break;
      	  	
      	  case KEY_CANCEL:   //ȡ��
      	  	switch(menuInLayer)
      	  	{
      	  		case 2:  //�˵���2��
      	  			layer1Menu(layer1MenuLight);
      	  		 	break;
      	  		 	
      	  		case 3:  //�˵���3��
             		if ((layer2MenuLight[layer1MenuLight]==4 || layer2MenuLight[layer1MenuLight]==5)&& layer1MenuLight==1)
             		{
             			 freeQueryMpLink();
             		}
             		
             		if ((layer2MenuLight[layer1MenuLight]==4 || layer2MenuLight[layer1MenuLight]==5 || layer2MenuLight[layer1MenuLight]==6 || layer2MenuLight[layer1MenuLight]==7)&& layer1MenuLight==0)
             		{
             			 while (eventLinkHead!=NULL)
             			 {
             			   tmpEventShow = eventLinkHead;
             			   eventLinkHead = eventLinkHead->next;
             			   
             			   free(tmpEventShow);
             			 }             			 
             		}
             		layer2Menu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	  			break;
      	  			
      	  		case 4:   //�˵���4��
      	  			if (layer1MenuLight==1 && layer2MenuLight[1]==5)
      	  			{
             		  layer2Menu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	  			}
      	  		  break;

      	  		case 5:   //�˵���5��
      	  			if (layer1MenuLight==1 && layer2MenuLight[1]==5)
      	  			{
             	 	 	 for(i=15;i>0;i--)
             	 	 	 {
             	 	 	  	if (commParaItem[2][i]!=' ' && i<15)
             	 	 	  	{
             	 	 	  		 commParaItem[2][i+1] = 0x0;
             	 	 	  		 break;
             	 	 	  	}
             	 	 	 }
             	 	 	 
      	  				 configPara(rowOfLight, keyLeftRight);
      	  			}
      	  		  break;      	  		
      	  	}
      	  	break;
      	  
      	  case KEY_UP:       //�ϼ�
           	switch(menuInLayer)
            {
             	case 1:    //�˵���1��
             	 	if (layer1MenuLight==0)
             	 	{
             	 	 	 layer1MenuLight = MAX_LAYER1_MENU-1;
             	 	}
             	 	else
             	 	{
             	 	 	 layer1MenuLight--;
             	 	}
             	 	layer1Menu(layer1MenuLight);
             	  break;
             	  
             	case 2:    //�˵���2��
             		switch(layer1MenuLight)
             		{
             			case 0:
             			case 1:
             				if (layer2MenuLight[layer1MenuLight]==0)
             				{
             					layer2MenuLight[layer1MenuLight] = layer2MenuNum[layer1MenuLight]-1;
             				}
             				else
             				{
             					layer2MenuLight[layer1MenuLight]--;
             				}
             				layer2Menu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
             				break; 
             		}
             		break;
             		
              case 3:    //�˵���3��
              	switch(layer1MenuLight)
                {
                	case 0:  //ʵʱ����
                	 	switch(layer2MenuLight[layer1MenuLight])
                	 	{
                	 		case 2:  //��������
                	 			if (mpPowerCurveLight==0)
             	 	 	  	  {
             	 	 	  		 	stringUpDown(queryTimeStr, layer3MenuLight[0][2], 0);  //�������ַ�����һ���ַ�
             	 	 	  		 	powerCurve(mpPowerCurveLight, layer3MenuLight[0][2]);
             	 	 	  		}
             	 	 	  		else
             	 	 	  		{
             	 	 	  		 	keyLeftRight = 0;
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
                            layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]--;
             	 	 	  		 	}
             	 	 	  		 	powerCurve(mpPowerCurveLight,layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		}
             	 	 	  		break;
             	 	 	  }
             	 	 	  break;
             	 	 	  
             	 	 	case 1:  //������ֵ
                 	  switch(layer2MenuLight[layer1MenuLight])
                 	  {
      	    		      case 6: //����ר���û�������
                	 	 	  if (inputStatus==STATUS_SELECT_CHAR)
                	 	 	  {
                	 	 	  	if (selectIndex>=14)
                	 	 	  	{
                	 	 	  		 selectIndex -= 14;
                	 	 	  	}
                	 	 	  	else
                	 	 	  	{
                	 	 	  	  selectIndex = 68;
                	 	 	  	}
                	 	 	  	selectChar(selectIndex);
                	 	 	  }
                	 	 	  if (inputStatus==STATUS_NONE)
                	 	 	  {
                	 	 	  	selectIndex = 0;
                          selectChar(selectIndex);
                        }
    	    		          break;
    	    		          
        	 	 	  		  case 7: //�������
  		                  stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
    	 	 	  		 	      if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
    	 	 	  		 	      {
    	 	 	  		 	     	  if (chrMp[1][0]>0x37)
    	 	 	  		 	     	  {
    	 	 	  		 	     	  	chrMp[1][0] = 0x30;
    	 	 	  		 	     	  }
    	 	 	  		 	      }
    	 	 	  		 	      if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
    	 	 	  		 	      {
    	 	 	  		 	     	  if (chrMp[2][0]>0x32)
    	 	 	  		 	     	  {
    	 	 	  		 	     	 	  chrMp[2][0] = 0x30;
    	 	 	  		 	     	  }
    	 	 	  		 	      }
    	 	 	  		 	      if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
    	 	 	  		 	      {
    	 	 	  		 	     	  if (chrMp[3][0]>0x37)
    	 	 	  		 	     	  {
    	 	 	  		 	     	 	  chrMp[3][0] = 0x30;
    	 	 	  		 	     	  }
    	 	 	  		 	      }
  		                  setZbMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
        	 	 	  		 	  break;
         	 	 	  		  
         	 	 	  		  case 10: //������������
   		                  if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>1 && layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<5)
   		                  {
   		               	    stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
   		                  }
      		              else
      		              {
      		               	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
      		               	 {
      		               	 	  if (chrMp[0][0]!='3')
      		               	 	  {
      		               	 	  	 chrMp[0][0]='3';
      		               	 	  }
      		               	 }
      		               	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
      		               	 {
      		               	 	 if (chrMp[1][0]>0x31)
      		               	 	 {
      		               	 	  	chrMp[1][0]='0';
      		               	 	 }
      		               	 	 else
      		               	 	 {
      		               	 	  	chrMp[1][0]++;
      		               	 	 }
      		               	 }
      		              }
      		              setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
         	 	 	  		 	  break;

   	    	 	 	  	   case 12:    //ά���ӿ�ģʽ����
     	    	 	 	  	   if (keyLeftRight==0)
     	    	 	 	  	   {
     	    	 	 	  	  	 keyLeftRight = 1;
     	    	 	 	  	   }
     	    	 	 	  	   else
     	    	 	 	  	   {
     	    	 	 	  	  	 keyLeftRight--;
     	    	 	 	  	   }
   	    	 	 	  	  	 setMainTain(keyLeftRight);      	    	 	 	  	  	
   	    	 	 	  	  	 break;

   	    	 	 	  	   case 13:    //��2·485�ڹ�������
     	    	 	 	  	   if (keyLeftRight==0)
     	    	 	 	  	   {
     	    	 	 	  	  	 keyLeftRight = 1;
     	    	 	 	  	   }
     	    	 	 	  	   else
     	    	 	 	  	   {
     	    	 	 	  	  	 keyLeftRight--;
     	    	 	 	  	   }
   	    	 	 	  	  	 setRs485Port2(keyLeftRight);      	    	 	 	  	  	
   	    	 	 	  	  	 break;

         	 	 	  		  case 14: //��̫����������
  	    		            stringUpDown(chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
  	    		            setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
         	 	 	  		 	  break;
    	    		      }
             	 	 		break;
             	 	}
              	break;
              	
              case 4:    //�˵���4��
              	if (layer1MenuLight==1 && layer2MenuLight[1]==5)
              	{
    	 	 	  		 	 if (rowOfLight==2)
    	 	 	  		 	 {
    	 	 	  		 	 	  for(i=strlen(commParaItem[2]);i<16;i++)
    	 	 	  		 	 	  {
    	 	 	  		 	 	    commParaItem[2][i] = ' ';
    	 	 	  		 	 	  }
    	 	 	  		 	 	  commParaItem[2][16] = '\0';
    	 	 	  		 	 	  inputStatus=STATUS_NONE;
    	 	 	  		 	 	  inputIndex = 0;
    	 	 	  		 	 	  inputApn(inputIndex); //APN�����봦��
    	 	 	  		 	 }
    	 	 	  		 	 else
    	 	 	  		 	 {
   		               stringUpDown(commParaItem[rowOfLight],keyLeftRight,0);
   		               configPara(rowOfLight,keyLeftRight);
   		             }
      	    		}
      	    		break;
      	    		
            	case 5:    //�˵���5��
             	 	if (layer1MenuLight==1 && layer2MenuLight[1]==5)
             	 	{
          	 	 	   if (inputStatus==STATUS_SELECT_CHAR)
          	 	 	   {
          	 	 	  		if (selectIndex>=14)
          	 	 	  		{
          	 	 	  			 selectIndex -= 14;
          	 	 	  		}
          	 	 	  		else
          	 	 	  		{
          	 	 	  			 selectIndex = 68;
          	 	 	  	  }
          	 	 	  		selectChar(selectIndex);
          	 	 	   }
          	 	 	   
          	 	 	   if (inputStatus==STATUS_NONE)
          	 	 	   {
          	 	 	  	  selectIndex = 0;
                      selectChar(selectIndex);
                   }
                }
             	 	break;
             
              case 20:   //�˵���20��
             	  stringUpDown(passWord, pwLight,0);
             	  inputPassWord(pwLight);
             	  break;
            }
      	  	break;

      	  case KEY_DOWN:     //�¼�
           	switch(menuInLayer)
            {
             	case 1:
             	 	if (layer1MenuLight>MAX_LAYER1_MENU-2)
             	 	{
             	 	 	 layer1MenuLight = 0;
             	 	}
             	 	else
             	 	{
             	 	 	 layer1MenuLight++;
             	 	}
             	 	layer1Menu(layer1MenuLight);
             	  break;

             	case 2:    //�˵���2��
             		switch(layer1MenuLight)
             		{
             			case 0:
             			case 1:
             				if (layer2MenuLight[layer1MenuLight]>layer2MenuNum[layer1MenuLight]-2)
             				{
             					layer2MenuLight[layer1MenuLight] = 0;
             				}
             				else
             				{
             					layer2MenuLight[layer1MenuLight]++;
             				}
             				layer2Menu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
             				break; 
             		}
             		break;
             		
              case 3:    //�˵���3��
              	switch(layer1MenuLight)
                {
                	case 0:  //ʵʱ����
                	 	switch(layer2MenuLight[layer1MenuLight])
                	 	{
                	 		case 2:  //��������
                	 			if (mpPowerCurveLight==0)
             	 	 	  	  {
             	 	 	  		 	stringUpDown(queryTimeStr, layer3MenuLight[0][2], 1);  //�������ַ���Сһ���ַ�
             	 	 	  		 	powerCurve(mpPowerCurveLight, layer3MenuLight[0][2]);
             	 	 	  		}
             	 	 	  		else
             	 	 	  		{
             	 	 	  		 	keyLeftRight = 0;
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>=layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]= 0;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
                            layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
             	 	 	  		 	}
             	 	 	  		 	powerCurve(mpPowerCurveLight,layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		}
             	 	 	  		break;
             	 	 	  }
             	 	 	  break;

                	case 1:  //������ֵ
                	 	switch(layer2MenuLight[layer1MenuLight])
                	 	{
    	    		         case 6: //����ר���û�������
                 	 	 	   if (inputStatus==STATUS_SELECT_CHAR)
                 	 	 	   {
                 	 	 	  		selectIndex += 14;
                 	 	 	  		if (selectIndex>69)
                 	 	 	  		{
                 	 	 	  			 selectIndex = 0;
                 	 	 	  		}
                 	 	 	  		selectChar(selectIndex);
                 	 	 	   }
                 	 	 	   if (inputStatus==STATUS_NONE)
                 	 	 	   {
                 	 	 	  	  selectIndex = 0;
                            selectChar(selectIndex);
                         }
    	    		           break;
    	    		           
        	 	 	  		   case 7: //�������
   		                   stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
    	 	 	  		 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
    	 	 	  		 	       {
    	 	 	  		 	     	   if (chrMp[1][0]>0x37)
    	 	 	  		 	     	   {
    	 	 	  		 	     	 	   chrMp[1][0] = 0x37;
    	 	 	  		 	     	   }
    	 	 	  		 	       }
    	 	 	  		 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
    	 	 	  		 	       {
    	 	 	  		 	     	   if (chrMp[2][0]>0x32)
    	 	 	  		 	     	   {
    	 	 	  		 	     	 	   chrMp[2][0] = 0x32;
    	 	 	  		 	     	   }
    	 	 	  		 	       }
    	 	 	  		 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
    	 	 	  		 	       {
    	 	 	  		 	     	   if (chrMp[3][0]>0x37)
    	 	 	  		 	     	   {
    	 	 	  		 	     	 	   chrMp[3][0] = 0x37;
    	 	 	  		 	     	   }
    	 	 	  		 	       }
   		                   setZbMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
        	 	 	  		 	   break;
        	 	 	  		 	   
          	 	 	  		 case 10: //������������
       		               if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>1 && layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<5)
       		               {
       		               	 stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
       		               }
       		               else
       		               {
       		               	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
       		               	 {
       		               	 	  if (chrMp[0][0]!='3')
       		               	 	  {
       		               	 	  	 chrMp[0][0]='3';
       		               	 	  }
       		               	 }
       		               	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
       		               	 {
       		               	 	 if (chrMp[1][0]=='0')
       		               	 	 {
       		               	 	  	chrMp[1][0]='2';
       		               	 	 }
       		               	 	 else
       		               	 	 {
       		               	 	  	chrMp[1][0]--;
       		               	 	 }
       		               	 }
       		               }
       		               setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
          	 	 	  		 	 break;
          	 	 	  		 	 
    	    	 	 	  	   case 12:    //ά���ӿ�ģʽ����
      	    	 	 	  	   keyLeftRight++;
      	    	 	 	  	   if (keyLeftRight>1)
      	    	 	 	  	   {
      	    	 	 	  	  	 keyLeftRight = 0;
      	    	 	 	  	   }
    	    	 	 	  	  	 setMainTain(keyLeftRight);      	    	 	 	  	  	
    	    	 	 	  	  	 break;

    	    	 	 	  	   case 13:    //��2·485�ڹ�������
      	    	 	 	  	   keyLeftRight++;
      	    	 	 	  	   if (keyLeftRight>1)
      	    	 	 	  	   {
      	    	 	 	  	  	 keyLeftRight = 0;
      	    	 	 	  	   }
    	    	 	 	  	  	 setRs485Port2(keyLeftRight);      	    	 	 	  	  	
    	    	 	 	  	  	 break;
    	    	 	 	  	  	 
             	 	 	  	 case 14: //��̫����������
      	    		         stringUpDown(chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
      	    		         setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 break;
    	    		      }
    	    		      break;
             	 	}
              	break;
              	
              case 4:    //�˵���4��
              	if (layer1MenuLight==1 && layer2MenuLight[1]==5)
              	{
    	 	 	  		 	 if (rowOfLight==2)
    	 	 	  		 	 {
    	 	 	  		 	 	  for(i=strlen(commParaItem[2]);i<16;i++)
    	 	 	  		 	 	  {
    	 	 	  		 	 	    commParaItem[2][i] = ' ';
    	 	 	  		 	 	  }
    	 	 	  		 	 	  commParaItem[2][16] = '\0';
    	 	 	  		 	 	  inputStatus=STATUS_NONE;
    	 	 	  		 	 	  inputIndex = 0;
    	 	 	  		 	 	  inputApn(inputIndex); //APN�����봦��
    	 	 	  		 	 }
    	 	 	  		 	 else
    	 	 	  		 	 {
   		               stringUpDown(commParaItem[rowOfLight],keyLeftRight,1);
   		               configPara(rowOfLight,keyLeftRight);
   		             }
      	    		}
      	    		break;
      	    		
            	case 5:    //�˵���5��
             	 	if (layer1MenuLight==1 && layer2MenuLight[1]==5)
             	 	{
          	 	 	   if (inputStatus==STATUS_SELECT_CHAR)
          	 	 	   {
          	 	 	  		selectIndex += 14;
          	 	 	  		if (selectIndex>69)
          	 	 	  		{
          	 	 	  			 selectIndex = 0;
          	 	 	  		}
          	 	 	  		selectChar(selectIndex);
          	 	 	   }
          	 	 	   if (inputStatus==STATUS_NONE)
          	 	 	   {
          	 	 	  	  selectIndex = 0;
                     selectChar(selectIndex);
                   }
                }
             	 	break;
      	    		
              case 20:   //�˵���20��
             	  stringUpDown(passWord, pwLight,1);
             	  inputPassWord(pwLight);
             	  break;

            }
      	  	break;

      	  case KEY_LEFT:     //���
      	  	switch(menuInLayer)
      	  	{
      	  		case 2:
      	  			switch(layer1MenuLight)
      	  		  {
      	    		  case 6:  //�ն���Ϣ
      	    		 	 	if (layer2MenuLight[6]>0)
      	    		 	 	{
      	    		 	 	  layer2MenuLight[6]--;
      	    		 	 	}
      	    		 	 	else
      	    		 	 	{
      	    		 	 	  layer2MenuLight[6] = layer2MenuNum[6]-1;
      	    		 	 	}
      	    	 	 	  terminalInfo(layer2MenuLight[6]);
      	    		  	break;
      	    		}
      	    		break;
      	    		
      	  		case 3:
           	  	switch(layer1MenuLight)
           	    {
           	    	case 0:  //ʵʱ����
           	    		switch(layer2MenuLight[layer1MenuLight])
           	    		{
           	    			case 2:
                	 	 	  if (mpPowerCurveLight==0)
                	 	 	  {
                	 	 	  	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
                	 	 	  	{
                	 	 	  		 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 7;
                	 	 	  	}
                	 	 	  	else
                	 	 	  	{
                	 	 	  		 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]--;
                	 	 	  	}
                	 	 	  	powerCurve(mpPowerCurveLight,layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
                	 	 	  }
                	 	 	  else
                	 	 	  {
                	 	 	  	if (keyLeftRight==0)
                	 	 	  	{
                	 	 	  		 keyLeftRight = 3;
                	 	 	  	}
                	 	 	  	else
                	 	 	  	{
                	 	 	  		 keyLeftRight--;
                	 	 	  	}
                	 	 	  	powerCurve(mpPowerCurveLight,layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
                	 	 	  	break;
                	 	 	 }
                  	 	 break;
                  	}
                  	break;
                  
             	 	 	case 1:  //������ֵ
                 	  switch(layer2MenuLight[layer1MenuLight])
                 	  {
      	    		      case 6: //����ר���û�������        	 	 	  	
              	 	 	  	if (inputStatus==STATUS_NONE)
              	 	 	  	{
                          if (keyLeftRight==0)
                       	  {
                       	 	  keyLeftRight = 63;
                       	  }
                       	  else
                       	  {
                       	 	  keyLeftRight--;
                       	  }
      	    		          setVpn(keyLeftRight);
              	 	 	  	}
              	 	 	  	else
              	 	 	  	{
              	 	 	  	  if (selectIndex==0)
              	 	 	  	  {
              	 	 	  		  selectIndex=68;
              	 	 	  	  }
              	 	 	  	  else
              	 	 	  	  {
              	 	 	  		  selectIndex--;
              	 	 	  	  }
              	 	 	  	  selectChar(selectIndex);
              	 	 	  	}
    	    		          break;
    	    		          
    	    		        case 7:  //�������
                        adjustSetMeterParaLight(0,1);
      	    		        setZbMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
    	    		        	break;
      	    		      
      	    		      case 10:  //�ն˼�����������
      	    		        adjustCasParaLight(0);
      	    		        setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		        break;
      	    	 	 	  	
      	    	 	 	  	case 11:  //����LCD�Աȶ�
      	    		 	 	 	  if (lcdDegree>0)
      	    		 	 	 	  {
      	    		 	 	 	  	lcdDegree--;
      	    		 	 	 	  }
      	    	 	 	  	  setLcdDegree(lcdDegree);
      	    	 	 	  	  break;

      	    		      case 14:  //��̫����������
      	    		        adjustEthParaLight(0);
      	    		        setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);      	    		         	 
      	    		        break;
    	    		      }
             	 	 		break;
                }
                break;
              
              case 4:    //�˵���4��
              	if (layer1MenuLight==1 && layer2MenuLight[1]==5)
              	{
              	   adjustCommParaLightIII(0);
      	    		   configPara(rowOfLight, keyLeftRight);
      	    		}
              	break;
              	
             	case 5:    //�˵���5��
             	 	if (layer1MenuLight==1 && layer2MenuLight[1]==5)
             	 	{
           	 	 	  if (inputStatus==STATUS_NONE)
           	 	 	  {
           	 	 	  	  if (inputIndex==0)
           	 	 	  	  {
           	 	 	  		  inputIndex = 15;
           	 	 	  	  }
           	 	 	  	  else
           	 	 	  	  {
           	 	 	  		  inputIndex--;
           	 	 	  	  }
           	 	 	  	  inputApn(inputIndex);
           	 	 	  }
           	 	 	  else
           	 	 	  {
           	 	 	    if (selectIndex==0)
           	 	 	  	{
           	 	 	  		selectIndex=68;
           	 	 	  	}
           	 	 	  	else
           	 	 	  	{
           	 	 	  		selectIndex--;
           	 	 	  	}
           	 	 	  	selectChar(selectIndex);
           	 	 	  }
             	 	}             	 	 	  
             	 	break;
               
              case 20:   //�˵���20��(��������)
       	    		if (pwLight==0)
       	    		{
       	    		  pwLight = 5;
       	    		}
       	    		else
       	    		{
       	    		 	pwLight--;
       	    		}
       	    		inputPassWord(pwLight);
              	break;
            }
      	  	break;

      	  case KEY_RIGHT:    //�Ҽ�
      	  	switch(menuInLayer)
      	    {
      	  		case 2:
      	  			switch(layer1MenuLight)
      	  		  {
      	    		  case 6:  //�ն���Ϣ
      	    		 	 	if (layer2MenuLight[6]>layer2MenuNum[6]-2)
      	    		 	 	{
      	    		 	 	  layer2MenuLight[6] = 0;;
      	    		 	 	}
      	    		 	 	else
      	    		 	 	{
      	    		 	 	  layer2MenuLight[6]++;
      	    		 	 	}
      	    	 	 	  terminalInfo(layer2MenuLight[6]);
      	    		  	break;
      	    		}
      	    		break;

          	  case 3:
          	   	switch(layer1MenuLight)
          	    {
          	    	case 0:  //ʵʱ����
          	    		switch(layer2MenuLight[layer1MenuLight])
          	    		{
          	    			case 2:
                 	 	 	  if (mpPowerCurveLight==0)
                 	 	 	  {
                 	 	 	  	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>=7)
                 	 	 	  	{
                 	 	 	  		 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                 	 	 	  	}
                 	 	 	  	else
                 	 	 	  	{
                 	 	 	  		 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
                 	 	 	  	}
                 	 	 	  	powerCurve(mpPowerCurveLight,layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
                 	 	 	  }
                 	 	 	  else
                 	 	 	  {
                 	 	 	  	if (keyLeftRight>=3)
                 	 	 	  	{
                 	 	 	  		 keyLeftRight = 0;
                 	 	 	  	}
                 	 	 	  	else
                 	 	 	  	{
                 	 	 	  		 keyLeftRight++;
                 	 	 	  	}
                 	 	 	  	powerCurve(mpPowerCurveLight,layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
                 	 	 	  	break;
                 	 	 	 }
                 	 	 	 break;
                 	  }
                 	  break;
                 	  
          	    	case 1:  //������ֵ
          	    		switch(layer2MenuLight[layer1MenuLight])
          	    		{
    	    		        case 6:  //����ר���û�������
               	 	 	  	if (inputStatus==STATUS_NONE)
               	 	 	  	{
               	 	 	  	   if (keyLeftRight<63)
               	 	 	  	   {
               	 	 	  		   keyLeftRight++;
               	 	 	  	   }
               	 	 	  	   else
               	 	 	  	   {
               	 	 	  		   keyLeftRight = 0;
               	 	 	  	   }
               	 	 	  	   setVpn(keyLeftRight);
               	 	 	  	}
               	 	 	  	else
               	 	 	  	{
               	 	 	  	   if (selectIndex<69)
               	 	 	  	   {
               	 	 	  		   selectIndex++;
               	 	 	  	   }
               	 	 	  	   else
               	 	 	  	   {
               	 	 	  		   selectIndex = 0;
               	 	 	  	   }
               	 	 	  	   selectChar(selectIndex);
               	 	 	  	}
    	    		          break;

      	    		      case 7:  //�������
                        adjustSetMeterParaLight(1,1);
      	    		        setZbMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		        break;

      	    		      case 10:  //�ն˼�����������
      	    		        adjustCasParaLight(1);
      	    		        setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		        break;
      	    	 	 	  	
      	    	 	 	  	case 11:  //����LCD�Աȶ�
      	    		 	 	 	  if (lcdDegree<20)
      	    		 	 	 	  {
      	    		 	 	 	  	lcdDegree++;
      	    		 	 	 	  }
      	    	 	 	  	  setLcdDegree(lcdDegree);
      	    	 	 	  	  break;
      	    	 	 	  	  
      	    		      case 14:  //��̫����������
      	    		        adjustEthParaLight(1);
      	    		        setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		        break;
    	    		      }
                }
                break;

              case 4:    //�˵���4��
              	if (layer1MenuLight==1 && layer2MenuLight[1]==5)
              	{
              	   adjustCommParaLightIII(1);
      	    		   configPara(rowOfLight, keyLeftRight);
      	    		}
              	break;
              	
             	case 5:    //�˵���5��
             	 	if (layer1MenuLight==1 && layer2MenuLight[1]==5)
             	 	{
        	 	 	  	if (inputStatus==STATUS_NONE)
        	 	 	  	{
        	 	 	  	  if (inputIndex<15)
        	 	 	  	  {
        	 	 	  		   inputIndex++;
        	 	 	  	  }
        	 	 	  	  else
        	 	 	  	  {
        	 	 	  		   inputIndex = 0;
        	 	 	  	  }
        	 	 	  	  inputApn(inputIndex);
        	 	 	  	}
        	 	 	  	else
        	 	 	  	{
        	 	 	  	  if (selectIndex<69)
        	 	 	  	  {
        	 	 	  		   selectIndex++;
        	 	 	  	  }
        	 	 	  	  else
        	 	 	  	  {
        	 	 	  		   selectIndex = 0;
        	 	 	  	  }
        	 	 	  	  selectChar(selectIndex);
        	 	 	  	}
             	 	}             	 	 	  
             	 	break;


             	case 20:   //�˵���20��(��������)
      	    		if (pwLight<5)
      	    		{
      	    		  pwLight++;
      	    		}
      	    		else
      	    		{
      	    		 	pwLight = 0;
      	    		}
      	    		inputPassWord(pwLight);
             	 	break;
            }
      	  	break;
      	}
      }
   }
}

/*******************************************************
��������:searchCtrlEvent
��������:���������¼�
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void searchCtrlEvent(INT8U erc)
{
  INT16U    j;
  struct    eventShowLink *tmpEventNode,*tmpPrevEvent;
  INT8U     tmpReadBuff[200];
  DATE_TIME tmpTime;
	
	tmpPrevEvent = eventLinkHead;

  for(j=iEventCounter;j>0;j--)
  {
     tmpTime = sysTime;
     if (readMeterData(tmpReadBuff, 1 , EVENT_RECORD, j, &tmpTime, 0)==TRUE)
     {
       if (tmpReadBuff[0]==erc)  //ERC
       {
         tmpEventNode = (struct eventShowLink *)malloc(sizeof(struct eventShowLink));
         tmpEventNode->eventNo = j;     //�¼����
         tmpEventNode->next = NULL;
		        
         if (eventLinkHead==NULL)
         {
      	   eventLinkHead = tmpEventNode;
         }
         else
         {
      	   tmpPrevEvent->next = tmpEventNode;
         }
         tmpPrevEvent = tmpEventNode;
       }
     }
  }
}

/*******************************************************
��������:power2Str
��������:���ݸ�ʽ2����ת�����ַ���
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
char * power2Str(INT8U *pData)
{
	 char      say[35],str[25];
   INT16U    multiply,decimal,integer;

   if (*pData==0xee)
   {
   	  strcpy(say,"");
   	  return say;
   }

   //����(+/-)
   if (*(pData+1) & 0x10)
   {
   	  strcpy(say,"-");
   }
   else
   {
   	  strcpy(say,"");
   }

   if (((*(pData+1)>>5) & 0x7) <5)
   {
   	  switch((*(pData+1)>>5) & 0x7)
   	  {
 	  	  case 0:
 	  	  	multiply = 10000;
 	  	  	break;
 	  	  case 1:
 	  	  	multiply = 1000;
 	  	  	break;
 	  	  case 2:
 	  	  	multiply = 100;
 	  	  	break;
 	  	  case 3:
 	  	  	multiply = 10;
 	  	    break;
 	  	  case 4:
 	  	  	multiply = 1;
 	  	  	break;
   	  }
   	  strcat(say,floatToString(((*(pData+1)& 0xf)*100+(*pData>>4 & 0xf)*10 + (*pData & 0xf))*multiply,0,3,3,str));
    }
   	else
    {
      switch((*(pData+1)>>5) & 0x7)
      {
    	  case 5:
    	  	integer = (*(pData+1) & 0xf)*10+(*pData>>4 & 0xf);
    	  	decimal = *pData & 0xf;
    	  	break;
    	  case 6:
    	  	integer = *(pData+1) & 0xf;
    	  	decimal = (*pData>>4 & 0xf)*10 + (*pData & 0xf);
    	  	if (decimal<100)
    	  	{
    	  		 decimal *= 10;
    	  	}
    	  	break;
    	  case 7:
    	  	integer = 0;
    	  	decimal = (*(pData+1) & 0xf)*100 + (*pData>>4 & 0xf)*10 + (*pData & 0xf);
    	  	break;
      }
      
      strcat(say,floatToString(integer,decimal,3,3,str));
    }
    strcat(say,"kW");
    
    return say;
}

/*******************************************************
��������:eventRecordShow
��������:�¼���¼��ʾ
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void eventRecordShow(INT8U erc)
{
   INT8U     tmpReadBuff[200];
   DATE_TIME tmpTime;
   INT8U     *pData;
   INT8U     row;
   INT32U    tmpData;
   INT8U     type;
 	 
 	 char      say[50],str[25];
	 
	 guiLine(1,17,160,160,0);

   menuInLayer = 3;
   
   switch(erc)
   {
  	 case 5:   //ң�ؼ�¼
  	 	 strcpy(str,"ң�ؼ�¼");
  	 	 break;

  	 case 6:   //���ؼ�¼
  	 	 strcpy(str,"���ؼ�¼");
  	 	 break;

  	 case 7:   //��ؼ�¼
  	 	 strcpy(str,"��ؼ�¼");
  	 	 break;
  	 	 
  	 case 14:
  	 	 strcpy(str,"ʧ���¼");
  	 	 break;
   }
   
   if (tmpEventShow==NULL)
   {
   	  strcpy(say,"��");
   	  strcat(say,str);
   	  strcat(say,"!");
   	  
   	  guiDisplay(10,LCD_LINE_3,say,1);
      lcdRefresh(17, 160);
      
      return;
   }
   
   guiDisplay(49, LCD_LINE_1, str, 0);
   
   tmpTime = sysTime;
   
   printf("eventNo=%d\n",tmpEventShow->eventNo);
   
   if (readMeterData(tmpReadBuff, 1 , EVENT_RECORD, tmpEventShow->eventNo, &tmpTime, 0)==TRUE)
   {
   	  sprintf(say,"%02x-%02x-%02x %02x:%02x:%02x",tmpReadBuff[7],tmpReadBuff[6],tmpReadBuff[5],tmpReadBuff[4],tmpReadBuff[3],tmpReadBuff[2]);
   	  guiDisplay(13, LCD_LINE_3, say, 1);
   	  
   	  pData = &tmpReadBuff[8];
   	  
   	  if (erc==14)
   	  {   	    
   	    guiDisplay(1, LCD_LINE_5, "�ϵ�ʱ��:", 1);   	  	 
   	    sprintf(say,"%02x-%02x-%02x %02x:%02x:%02x",*(pData+5),*(pData+4),*(pData+3),*(pData+2),*(pData+1),*pData);
   	    guiDisplay(13, LCD_LINE_6, say, 1);   	  	 
   	  }
   	  else
   	  {
     	  if (erc!=5)
     	  {
     	    sprintf(say,"�ܼ���:%d",*pData);
     	    guiDisplay(17, LCD_LINE_2, say, 1);
     	    pData++;
     	  }
     	  
     	  strcpy(say,"��բ�ִ�:");
     	  if (*pData&0x1)
     	  {
     	  	 strcat(say,"��");
     	  }
     	  if (*pData&0x2)
     	  {
     	  	 strcat(say,"��");
     	  }
     	  if (*pData&0x4)
     	  {
     	  	 strcat(say,"��");
     	  }
     	  if (*pData&0x8)
     	  {
     	  	 strcat(say,"��");
     	  }
     	  guiDisplay(1, LCD_LINE_4, say, 1);
     	  pData++;
     	  row = LCD_LINE_5;
     	  
     	  switch(erc)
     	  {
     	  	 case 5:    //ң�ؼ�¼
     	  	 	 strcpy(say,"��բǰ����:");
     	  	 	 strcat(say,power2Str(pData));
     	  	 	 guiDisplay(1,row,say,1);
     	  	 	 pData += 2;
     	  	 	 row += 16;
  
     	  	 	 strcpy(say,"��բ����:");
     	  	 	 strcat(say,power2Str(pData));
     	  	 	 guiDisplay(1,row,say,1);
     	  	 	 break;
     	  	 	 
     	  	 case 6:    //���ؼ�¼
     	  	 	 strcpy(say,"�������:");
     	  	 	 if (*pData&0x1)
     	  	 	 {
     	  	 	 	 strcat(say,"ʱ�ο�");
     	  	 	 }
     	  	 	 if (*pData&0x2)
     	  	 	 {
     	  	 	 	 strcat(say,"���ݿ�");
     	  	 	 }
     	  	 	 if (*pData&0x4)
     	  	 	 {
     	  	 	 	 strcat(say,"Ӫҵ��ͣ��");
     	  	 	 }
     	  	 	 if (*pData&0x8)
     	  	 	 {
     	  	 	 	 strcat(say,"�¸���");
     	  	 	 }
     	       guiDisplay(1, row, say, 1);
     	       row += 16;
     	       pData++;
     	  	 	 
     	  	 	 strcpy(say,"��բǰ����:");
     	  	 	 strcat(say,power2Str(pData));
     	  	 	 guiDisplay(1,row,say,1);
     	  	 	 pData += 2;
     	  	 	 row += 16;
  
     	  	 	 strcpy(say,"��բ����:");
     	  	 	 strcat(say,power2Str(pData));
     	  	 	 guiDisplay(1,row,say,1);
     	  	 	 pData += 2;
     	  	 	 row += 16;
  
     	  	 	 strcpy(say,"��բ���ʶ�ֵ:");
     	  	 	 strcat(say,power2Str(pData));
     	  	 	 guiDisplay(1,row,say,1);
     	  	 	 pData += 2;
     	  	 	 row += 16;   	  	 	 
     	  	 	 break;
     	  	 	 
     	  	 case 7:    //��ؼ�¼
     	  	 	 strcpy(say,"������:");
     	  	 	 type = 0;
     	  	 	 if (*pData&0x1)
     	  	 	 {
     	  	 	 	 strcat(say,"�µ��");
     	  	 	 	 type = 1;
     	  	 	 }
     	  	 	 if (*pData&0x2)
     	  	 	 {
     	  	 	 	 strcat(say,"�����");
     	  	 	 	 type = 2;
     	  	 	 }
     	       guiDisplay(1, row, say, 1);
     	       row += 16;
     	       pData++;
     	  	 	 
     	  	 	 if (type==2)
     	  	 	 {
     	  	 	   strcpy(say,"��բʱʣ��:");
     	  	 	 }
     	  	 	 else
     	  	 	 {
     	  	 	   strcpy(say,"�µ�����:");
     	  	 	 }
     	  	 	 tmpData = (*(pData+3)&0xf)<<24 | *(pData+2)<<16 | *(pData+1)<<8 | *pData;
     	  	 	 if (*(pData+3)&0x10)
     	  	 	 {
     	  	 	 	 strcat(say, "-");
     	  	 	 }
     	  	 	 tmpData = bcdToHex(tmpData);
     	  	 	 strcat(say,intToString(tmpData,3,str));
     	  	 	 if (*(pData+3)&0x40)
     	  	 	 {
     	  	 	 	 strcat(say, "MWh");
     	  	 	 }
     	  	 	 else
     	  	 	 {
     	  	 	 	 strcat(say, "kWh");
     	  	 	 }
     	  	 	 guiDisplay(1,row,say,1);
     	  	 	 pData += 4;
     	  	 	 row += 16;
  
     	  	 	 if (type==2)
     	  	 	 {
     	  	 	   strcpy(say,"��բ����:");
     	  	 	 }
     	  	 	 else
     	  	 	 {
     	  	 	   strcpy(say,"�µ�ض�ֵ:");
     	  	 	 }
     	  	 	 tmpData = (*(pData+3)&0xf)<<24 | *(pData+2)<<16 | *(pData+1)<<8 | *pData;
     	  	 	 tmpData = bcdToHex(tmpData);
     	  	 	 if (*(pData+3)&0x10)
     	  	 	 {
     	  	 	 	 strcat(say, "-");
     	  	 	 }
     	  	 	 strcat(say,intToString(tmpData,3,str));
     	  	 	 if (*(pData+3)&0x40)
     	  	 	 {
     	  	 	 	 strcat(say, "MWh");
     	  	 	 }
     	  	 	 else
     	  	 	 {
     	  	 	 	 strcat(say, "kWh");
     	  	 	 }
     	  	 	 guiDisplay(1,row,say,1);
     	  	 	 break;
     	  }
     	}
   }
   else
   {
     guiDisplay(49, LCD_LINE_3, "��ȡ�¼���¼ʧ��", 1);   	 
   }   
   
   lcdRefresh(17, 160);
}

/*******************************************************
��������:defaultMenu
��������:Ĭ�ϲ˵�(������,����376.1ר��III���ն˹�Լ�˵�)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void defaultMenu(void)
{
	 while (eventLinkHead!=NULL)
	 {
	   tmpEventShow = eventLinkHead;
	   eventLinkHead = eventLinkHead->next;
	   
	   free(tmpEventShow);
	 }

	 layer1Menu(layer1MenuLight);
}

/*******************************************************
��������:layer2Menu
��������:����˵�(376.1���ҵ����ն˹�Լ)
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void layer2Menu(int lightNum,int layer1Num)
{
	 INT8U startX, startY;
	 INT8U i, j;
	 INT8U startItem=0, endItem=0;

   if (layer1Num==0)
	 {
	 	 startX = 40;
	 }
	 else
	 {
	 	 startX = 35;
	 }
	 startY = 20;

	 guiLine(1, 17, 160, 160, 0);

   menuInLayer = 2;
   
   if (layer2MenuNum[layer1Num]>8)
   {
   	 if (lightNum>7)
   	 {
   	 	 startItem = lightNum-7;
   	 	 endItem = layer2MenuNum[layer1Num]-startItem;
   	 }
   	 else
   	 {
   	   startItem = 0;
   	   endItem = 8;
   	 }
   }
   else
   {
   	 startItem = 0;
   	 endItem = layer2MenuNum[layer1Num];
   }
   
	 for(i=0,j=startItem; i<endItem; i++,j++)
	 {
		 if (j==lightNum)
		 {
       guiDisplay(startX,startY+i*17,layer2MenuItem[layer1Num][j],0);
		 }
		 else
		 {
       guiDisplay(startX,startY+i*17,layer2MenuItem[layer1Num][j],1);
		 }
	 }

   lcdRefresh(17,160);               //ˢ��LCD
}

/*******************************************************
��������:currentPower
��������:��ǰ����
���ú���:
�����ú���:
�������:item
�������:
����ֵ:void
*******************************************************/
void currentPower(INT8U item)
{
 	  char      say[30],str[25],units[6],sayx[10];
 	  INT16U    multiply;
 	  INT8U     i;
 	  INT8U     tmpPort;
 	  INT32U    integer;
 	  INT32U    decimal;
 	  INT8U     dataBuff[LEN_OF_ZJZ_BALANCE_RECORD];
 	  DATE_TIME tmpTime;
 	  
 	  menuInLayer = 3;
	  guiLine(1,17,160,160,0);
	  
	  if (totalAddGroup.numberOfzjz==0 && pulseConfig.numOfPulse==0)
	  {
  	  guiDisplay(4,LCD_LINE_3,"���ܼ��鼰��������!",1);
  	  lcdRefresh(17,160);
  	  
  	  return;
	  }
	  
	  if (item<totalAddGroup.numberOfzjz)
	  {
  	  strcpy(say,"�ܼ���");
  	  strcat(say,digital2ToString(totalAddGroup.perZjz[item].zjzNo,str));
  	  guiDisplay(48,LCD_LINE_1,say,0);

      tmpTime = timeHexToBcd(sysTime);
      if (readMeterData(dataBuff, totalAddGroup.perZjz[item].zjzNo, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &tmpTime, 0) == TRUE)
	  	{
 	      strcpy(say,"�й�����=");
 	      if (dataBuff[GP_WORK_POWER+1]!=0xEE)
 	      {
   	      //����(+/-)
   	      if (dataBuff[GP_WORK_POWER] & 0x10)
   	      {
   	   	    strcat(say,"-");
   	      }
   	      else
   	      {
   	   	    strcat(say,"");
   	   	  }

   	   	  if ((dataBuff[GP_WORK_POWER] & 0x7) <5)
   	   	  {
   	   	    switch(dataBuff[GP_WORK_POWER] & 0x7)
   	   	    {
   	   	  	  case 0:
   	   	  	  	multiply = 10000;
   	   	  	  	break;
   	   	  	  case 1:
   	   	  	  	multiply = 1000;
   	   	  	  	break;
   	   	  	  case 2:
   	   	  	  	multiply = 100;
   	   	  	  	break;
   	   	  	  case 3:
   	   	  	  	multiply = 10;
   	   	  	    break;
   	   	  	  case 4:
   	   	  	  	multiply = 1;
   	   	  	  	break;
   	   	    }
   	   	    strcat(say,floatToString(((dataBuff[GP_WORK_POWER+2]& 0xf)*100+(dataBuff[GP_WORK_POWER+1]>>4 & 0xf)*10 + (dataBuff[GP_WORK_POWER+1] & 0xf))*multiply,0,3,2,str));
   	   	  }
   	   	  else
   	   	  {
   	   	    switch(dataBuff[GP_WORK_POWER] & 0x7)
   	   	    {
   	   	  	  case 5:
   	   	  	  	integer = (dataBuff[GP_WORK_POWER+2] & 0xf)*10+(dataBuff[GP_WORK_POWER+1]>>4 & 0xf);
   	   	  	  	decimal = (dataBuff[GP_WORK_POWER+1] & 0xf)*100;
   	   	  	  	break;
   	   	  	  	
   	   	  	  case 6:
   	   	  	  	integer = dataBuff[GP_WORK_POWER+2] & 0xf;
   	   	  	  	decimal = (dataBuff[GP_WORK_POWER+1]>>4 & 0xf)*10 + (dataBuff[GP_WORK_POWER+1] & 0xf);
   	   	  	  	if (decimal<100)
   	   	  	  	{
   	   	  	  		 decimal *= 10;
   	   	  	  	}
   	   	  	  	break;
   	   	  	  	
   	   	  	  case 7:
   	   	  	  	integer = 0;
   	   	  	  	decimal = (dataBuff[GP_WORK_POWER+2] & 0xf)*100 + (dataBuff[GP_WORK_POWER+1]>>4 & 0xf)*10 + (dataBuff[GP_WORK_POWER+1] & 0xf);
   	   	  	  	break;
   	   	    }
   	   	    
   	   	    strcat(say,floatToString(integer,decimal,3,2,str));
   	   	  }
   	      strcat(say,"kW");
   	    }   	    
   	    guiDisplay(1,LCD_LINE_3,say,1);
   	    
 	      strcpy(say,"�޹�����=");
 	      if (dataBuff[GP_NO_WORK_POWER+1]!=0xEE)
 	      {
   	      //����(+/-)
   	      if (dataBuff[GP_NO_WORK_POWER] & 0x10)
   	      {
   	   	    strcat(say,"-");
   	      }
   	      else
   	      {
   	   	    strcat(say,"");
   	   	  }

   	   	  if ((dataBuff[GP_NO_WORK_POWER] & 0x7) <5)
   	   	  {
   	   	    switch(dataBuff[GP_NO_WORK_POWER] & 0x7)
   	   	    {
   	   	  	  case 0:
   	   	  	  	multiply = 10000;
   	   	  	  	break;
   	   	  	  case 1:
   	   	  	  	multiply = 1000;
   	   	  	  	break;
   	   	  	  case 2:
   	   	  	  	multiply = 100;
   	   	  	  	break;
   	   	  	  case 3:
   	   	  	  	multiply = 10;
   	   	  	    break;
   	   	  	  case 4:
   	   	  	  	multiply = 1;
   	   	  	  	break;
   	   	    }
   	   	    strcat(say,floatToString(((dataBuff[GP_NO_WORK_POWER+2]& 0xf)*100+(dataBuff[GP_NO_WORK_POWER+1]>>4 & 0xf)*10 + (dataBuff[GP_NO_WORK_POWER+1] & 0xf))*multiply,0,3,2,str));
   	   	  }
   	   	  else
   	   	  {
   	   	    switch(dataBuff[GP_NO_WORK_POWER] & 0x7)
   	   	    {
   	   	  	  case 5:
   	   	  	  	integer = (dataBuff[GP_NO_WORK_POWER+2] & 0xf)*10+(dataBuff[GP_NO_WORK_POWER+1]>>4 & 0xf);
   	   	  	  	decimal = (dataBuff[GP_NO_WORK_POWER+1] & 0xf)*100;
   	   	  	  	break;
   	   	  	  	
   	   	  	  case 6:
   	   	  	  	integer = dataBuff[GP_NO_WORK_POWER+2] & 0xf;
   	   	  	  	decimal = (dataBuff[GP_NO_WORK_POWER+1]>>4 & 0xf)*10 + (dataBuff[GP_NO_WORK_POWER+1] & 0xf);
   	   	  	  	if (decimal<100)
   	   	  	  	{
   	   	  	  		 decimal *= 10;
   	   	  	  	}
   	   	  	  	break;
   	   	  	  	
   	   	  	  case 7:
   	   	  	  	integer = 0;
   	   	  	  	decimal = (dataBuff[GP_NO_WORK_POWER+2] & 0xf)*100 + (dataBuff[GP_NO_WORK_POWER+1]>>4 & 0xf)*10 + (dataBuff[GP_NO_WORK_POWER+1] & 0xf);
   	   	  	  	break;
   	   	    }
   	   	    
   	   	    strcat(say,floatToString(integer,decimal,3,2,str));
   	   	  }
   	      strcat(say,"kvar");
   	    }   	    
   	    guiDisplay(1,LCD_LINE_4,say,1);
      }
      else
      {
   	    guiDisplay(1,LCD_LINE_3,"�й�����=",1);
   	    guiDisplay(1,LCD_LINE_4,"�޹�����=",1);
      }
  	}
  	else
  	{	  
  	  item -= totalAddGroup.numberOfzjz;
  	  if (item<pulseConfig.numOfPulse)
  	  {
    	  if (pulseConfig.numOfPulse<1)
    	  {
    	  	 guiDisplay(1,LCD_LINE_1,"���������ò���!",1);
    	  	 lcdRefresh(2,8);
    	  	 return;
    	  }
    
    	  strcpy(say,"�������˿�");
    	  strcat(say,digital2ToString(pulseConfig.perPulseConfig[item].ifNo,str));
    	  guiDisplay(32,LCD_LINE_1,say,0);
    	  
    	  tmpPort = pulseConfig.perPulseConfig[item].ifNo-1;
    	  strcpy(say,"������=");
    	  strcat(say,digital2ToString(pulseConfig.perPulseConfig[item].pn,str));
    	  strcat(say," ����=");
    	  strcat(say,intToString(pulseConfig.perPulseConfig[item].meterConstant[0] | pulseConfig.perPulseConfig[item].meterConstant[1]<<8,3,str));
    	  guiDisplay(1,LCD_LINE_2,say,1);
    
    	  switch(pulseConfig.perPulseConfig[item].character&0x3)
    	  {
    	  	 case 0:
    	  	 	 strcpy(sayx,"�����й�=");
    	  	 	 strcpy(units,"kW");
    	  	 	 break;
    
    	  	 case 1:
    	  	 	 strcpy(sayx,"�����޹�=");
    	  	 	 strcpy(units,"kvar");
    	  	 	 break;
    	  	 	 
    	  	 case 2:
    	  	 	 strcpy(sayx,"�����й�=");
    	  	 	 strcpy(units,"kW");
    	  	 	 break;
    	  	 	 
    	  	 case 3:
    	  	 	 strcpy(sayx,"�����޹�=");
    	  	 	 strcpy(units,"kvar");
    	  	 	 break;
    	  }
    	  
    	  strcpy(say,"����=");
    	  integer = pulse[tmpPort].prevMinutePulse*pulse[tmpPort].voltageTimes*pulse[tmpPort].currentTimes*60
    	          /pulse[tmpPort].meterConstant;
        decimal = pulse[tmpPort].prevMinutePulse*pulse[tmpPort].voltageTimes*pulse[tmpPort].currentTimes*60
                %pulse[tmpPort].meterConstant*100/pulse[tmpPort].meterConstant;
    	  strcat(say,floatToString(integer,decimal,2,2,str));
        strcat(say,units);
    	  guiDisplay(1,LCD_LINE_4,say,1);
    	  
    	  //ʾֵ����
    	  strcpy(say,sayx);
    	  integer = pulseDataBuff[53*tmpPort] | pulseDataBuff[53*tmpPort+1]<<8 
    	            | pulseDataBuff[53*tmpPort+2]<<16;
    
    	  //ʾֵ*��ѹ���*�������=�������ֵ�����
    	  integer *= (pulse[tmpPort].voltageTimes*pulse[tmpPort].currentTimes);
    
    	  //(����+β�����ֳ˱���/����)= ������������
    	  integer += (pulse[tmpPort].pulseCount*pulse[tmpPort].voltageTimes*pulse[tmpPort].currentTimes
    	             /pulse[tmpPort].meterConstant);
    	  //С��
    	  decimal = (pulse[tmpPort].pulseCount*pulse[tmpPort].voltageTimes*pulse[tmpPort].currentTimes
    	            %pulse[tmpPort].meterConstant*100/pulse[tmpPort].meterConstant);
        strcat(say,floatToString(integer,decimal,2,2,str));
        strcat(say,units);
        strcat(say,"h");
    	  guiDisplay(1,LCD_LINE_6,say,1);
    	  
    
    	  //��������
    	  integer = (pulseDataBuff[53*tmpPort+5] | pulseDataBuff[53*tmpPort+6]<<8)
    	            *pulse[tmpPort].voltageTimes*pulse[tmpPort].currentTimes*60/pulse[tmpPort].meterConstant;
    	  decimal = (pulseDataBuff[53*tmpPort+5] | pulseDataBuff[53*tmpPort+6]<<8)
    	            *pulse[tmpPort].voltageTimes*pulse[tmpPort].currentTimes*60%pulse[tmpPort].meterConstant*100
    	            /pulse[tmpPort].meterConstant;
    	  strcpy(say,"����=");
    	  strcat(say,floatToString(integer,decimal,2,2,str));
        strcat(say,units);    
    	  guiDisplay(1,LCD_LINE_7,say,1);
        
    	  strcpy(say,"����ʱ��");
    	  strcat(say,digital2ToString(pulseDataBuff[53*tmpPort+10],str));
    	  strcat(say,"-");
    	  strcat(say,digital2ToString(pulseDataBuff[53*tmpPort+9],str));
    	  strcat(say," ");
    	  strcat(say,digital2ToString(pulseDataBuff[53*tmpPort+8],str));
    	  strcat(say,":");
    	  strcat(say,digital2ToString(pulseDataBuff[53*tmpPort+7],str));
    	  guiDisplay(1,LCD_LINE_8,say,1);
    	}
  	}
    
	  lcdRefresh(17,160);
}

/*******************************************************
��������:currentEnergy
��������:��ǰ����
���ú���:
�����ú���:
�������:item
�������:
����ֵ:void
*******************************************************/
void currentEnergy(INT16U item)
{
	 INT8U                dataBuff[LEN_OF_ENERGY_BALANCE_RECORD];
 	 INT32U               integer,decimal;
   struct cpAddrLink    *tmpLink;
	 char                 str[15];
	 char                 sayStr[10];
	 DATE_TIME            time;
	 INT16U               i;
	 INT8U                tmpY;

 	 menuInLayer = 3;
	 guiLine(1,17,160,160,0);

 	 tmpLink = queryMpLink;
 	 i = 0;
 	 while(tmpLink!=NULL && i<item)
 	 {
 	   tmpLink = tmpLink->next;
 	   i++;
 	 }
 	 
 	 if (tmpLink!=NULL)
 	 {
   	 if (tmpLink->protocol==AC_SAMPLE)
   	 {
   	 	  tmpLink = tmpLink->next;
   	 	  if (tmpLink==NULL)
   	 	  {
   	 	  	 tmpLink = queryMpLink;
   	 	  	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
   	 	  }
   	 	  else
   	 	  {
   	 	  	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
   	 	  }
   	 }
  	 strcpy(sayStr,"������");
  	 strcat(sayStr,digital2ToString(tmpLink->mp,str));
  	 guiDisplay(48,LCD_LINE_1,sayStr,0);
     
     //��ѯ����������ϴγ���ʱ��
     time = queryCopyTime(tmpLink->mp);
     
     if (readMeterData(dataBuff, tmpLink->mp, LAST_REAL_BALANCE, REAL_BALANCE_POWER_DATA, &time, 0)==TRUE)
     {
        guiLine(1,LCD_LINE_2-1,160,LCD_LINE_2-1,1);
       	
       	tmpY=LCD_LINE_2;
       	for(i=0;i<5;i++)
       	{
       	  switch(i)
       	  {
       	  	case 0:
       	      strcpy(sayStr,"���������й���=");
       	      break;
       	      
       	  	case 1:
       	      strcpy(sayStr,"��=");
       	      break;

       	  	case 2:
       	      strcpy(sayStr,"��=");
       	      break;
       	      
       	  	case 3:
       	      strcpy(sayStr,"ƽ=");
       	      break;
       	      
       	  	case 4:
       	      strcpy(sayStr,"��=");
       	      break;
       	  }
       	  
       	  if (dataBuff[DAY_P_WORK_OFFSET+i*7]!=0xee)
       	  {
             decimal = bcdToHex(dataBuff[DAY_P_WORK_OFFSET+i*7+1] |dataBuff[DAY_P_WORK_OFFSET+i*7+2]<<8);
             integer = bcdToHex(dataBuff[DAY_P_WORK_OFFSET+i*7+3] |dataBuff[DAY_P_WORK_OFFSET+i*7+4]<<8
                     |dataBuff[DAY_P_WORK_OFFSET+i*7+5]<<16 |dataBuff[DAY_P_WORK_OFFSET+i*7+6]<<24);
             strcpy(str, floatToString(integer,decimal,4,2,str));
             strcat(sayStr,str);
       	  }
       	  
       	  if ((i!=0) && (i%2==0))
       	  {
       	    guiDisplay(81,tmpY,sayStr,1);
       	  }
       	  else
       	  {
       	    guiDisplay(1,tmpY,sayStr,1);
       	  }
       	  
       	  if (i==0 || i==2)
       	  {
       	  	 tmpY += 16;
       	  }
       	}
       	
       	//���������޹���
       	strcpy(sayStr,"�����޹���=");
       	if (dataBuff[DAY_P_NO_WORK_OFFSET]!=0xee)
       	{
             decimal = bcdToHex(dataBuff[DAY_P_NO_WORK_OFFSET+1] |dataBuff[DAY_P_NO_WORK_OFFSET+2]<<8);
             integer = bcdToHex(dataBuff[DAY_P_NO_WORK_OFFSET+3] |dataBuff[DAY_P_NO_WORK_OFFSET+4]<<8
                     |dataBuff[DAY_P_NO_WORK_OFFSET+5]<<16 |dataBuff[DAY_P_NO_WORK_OFFSET+6]<<24);
             strcpy(str, floatToString(integer,decimal,4,2,str));
             strcat(sayStr,str);
       	}
       	guiDisplay(1,LCD_LINE_5,sayStr,1);
       	
       	guiLine(1,LCD_LINE_6-1,160,LCD_LINE_6-1,1);
       	
       	tmpY=LCD_LINE_6;
       	for(i=0;i<5;i++)
       	{
       	  switch(i)
       	  {
       	  	case 0:
       	      strcpy(sayStr,"���������й���=");
       	      break;
       	      
       	  	case 1:
       	      strcpy(sayStr,"��=");
       	      break;

       	  	case 2:
       	      strcpy(sayStr,"��=");
       	      break;
       	      
       	  	case 3:
       	      strcpy(sayStr,"ƽ=");
       	      break;
       	      
       	  	case 4:
       	      strcpy(sayStr,"��=");
       	      break;
       	  }
       	  
       	  if (dataBuff[MONTH_P_WORK_OFFSET+i*7]!=0xee)
       	  {
             decimal = bcdToHex(dataBuff[MONTH_P_WORK_OFFSET+i*7+1] |dataBuff[MONTH_P_WORK_OFFSET+i*7+2]<<8);
             integer = bcdToHex(dataBuff[DAY_P_WORK_OFFSET+i*7+3] |dataBuff[DAY_P_WORK_OFFSET+i*7+4]<<8
                     |dataBuff[DAY_P_WORK_OFFSET+i*7+5]<<16 |dataBuff[DAY_P_WORK_OFFSET+i*7+6]<<24);
             strcpy(str, floatToString(integer,decimal,4,2,str));
             strcat(sayStr,str);
       	  }
       	  
       	  if ((i!=0) && (i%2==0))
       	  {
       	    guiDisplay(81,tmpY,sayStr,1);
       	  }
       	  else
       	  {
       	    guiDisplay(1,tmpY,sayStr,1);
       	  }
       	  
       	  if (i==0 || i==2)
       	  {
       	  	 tmpY += 16;
       	  }
       	}
       	//���������޹���
       	strcpy(sayStr,"�����޹���=");
       	if (dataBuff[MONTH_P_NO_WORK_OFFSET]!=0xee)
       	{
             decimal = bcdToHex(dataBuff[MONTH_P_NO_WORK_OFFSET+1] |dataBuff[MONTH_P_NO_WORK_OFFSET+2]<<8);
             integer = bcdToHex(dataBuff[MONTH_P_NO_WORK_OFFSET+3] |dataBuff[MONTH_P_NO_WORK_OFFSET+4]<<8
                     |dataBuff[MONTH_P_NO_WORK_OFFSET+5]<<16 |dataBuff[MONTH_P_NO_WORK_OFFSET+6]<<24);
             strcpy(str, floatToString(integer,decimal,4,2,str));
             strcat(sayStr,str);
       	}
       	guiDisplay(1,LCD_LINE_8+16,sayStr,1);
     }
     else
     {
     	  guiDisplay(1, LCD_LINE_2,"���������й���=",1);
     	  guiDisplay(1, LCD_LINE_3,"��=",1);
     	  guiDisplay(81,LCD_LINE_3,"��=",1);
     	  guiDisplay(1, LCD_LINE_4,"ƽ=",1);
     	  guiDisplay(81,LCD_LINE_4,"��=",1);
     	  guiDisplay(81,LCD_LINE_5,"�޹���=",1);
     	  
     	  guiDisplay(1, LCD_LINE_6,"���������й���=",1);
     	  guiDisplay(1, LCD_LINE_7,"��=",1);
     	  guiDisplay(81,LCD_LINE_7,"��=",1);
     	  guiDisplay(1, LCD_LINE_8,"ƽ=",1);
     	  guiDisplay(81,LCD_LINE_8,"��=",1);
     	  guiDisplay(81,LCD_LINE_8+16,"�޹���=",1);
     }     
   }
   else
   {
     guiDisplay(12, LCD_LINE_3,"�޲��������ò���!",1);   	 
   }
   
   lcdRefresh(17,160);
}

/*******************************************************
��������:powerCurve
��������:��������
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void powerCurve(INT8U layer2Light,INT8U layer3Light)
{
	 INT8U             dataBuff[LENGTH_OF_ENERGY_RECORD];
 	 INT32U            integer,decimal,disData;
	 char              str[30];
	 char              sayStr[30];
	 INT8U             i, tmpY, tmpCount, tmpX=0;
	 INT8U             dataType;
	 INT8U             tmpMinute;
	 DATE_TIME         tmpTime, readTime;
   struct cpAddrLink *tmpLink;
   INT16U            offset;
   INT8U             sign;

	 guiLine(1,17,160,160,0); //����
	 menuInLayer = 3;         //�˵������2��
	 
	 switch(layer2Light)
	 {
	 	 case 0:
	 	   guiDisplay(48,17,"��������",0);
	 	   showInputTime(layer3Light);
	 	   break;
	 	   
	 	 case 1:
       strcpy(sayStr, digital2ToString(sysTime.year,str));
       strcat(sayStr,"-");
       strcat(sayStr, digital2ToString(sysTime.month,str));
       strcat(sayStr,"-");
       strcat(sayStr, digital2ToString(sysTime.day,str));
       strcat(sayStr,"��������");
	 	   guiDisplay(16,17,sayStr,0);
	 	   guiDisplay(78,33,"�й�����",1);
	 	   dataType = PARA_VARIABLE_DATA;
	 	   offset   = POWER_INSTANT_WORK;
	 	 	 break;

	 	 case 2:
       strcpy(sayStr, digital2ToString(sysTime.year,str));
       strcat(sayStr,"-");
       strcat(sayStr, digital2ToString(sysTime.month,str));
       strcat(sayStr,"-");
       strcat(sayStr, digital2ToString(sysTime.day,str));
       strcat(sayStr,"��������");
	 	   guiDisplay(16,17,sayStr,0);
	 	   guiDisplay(78,33,"�޹�����",1);
	 	   dataType = PARA_VARIABLE_DATA;
	 	   offset   = POWER_INSTANT_NO_WORK;
	 	 	 break;
	 }

	 lcdRefresh(17, 160);
	 
	 if (layer2Light>=1 && layer2Light<=2)
	 {
 	   tmpLink = queryMpLink;
 	   i = 0;
 	   while(tmpLink!=NULL && i<layer3Light)
 	   {
 	   	  tmpLink = tmpLink->next;
 	      i++;
 	   }
 	   if (tmpLink==NULL)
 	   {
 	   	  tmpLink = queryMpLink;
 	   }
 	   
 	   strcpy(sayStr,"������");
 	   if (tmpLink->mp<10)
 	   {
 	   	 strcat(sayStr,"0");
 	   }
 	   strcat(sayStr,intToString(tmpLink->mp,3,str));
 	   guiDisplay(1,33,sayStr,1);

		 tmpTime = menuQueryTime;
		 tmpTime.hour   = keyLeftRight*6;
		 tmpTime.minute = 0x0;
		 tmpTime.second = 0x0;
		 tmpTime = backTime(tmpTime, 0, 0, 0, 15, 0);
	   
	   lcdRefresh(33, 48);
		 
		 tmpY = 52;
		 for(i = 0; i < 6; i++)
		 {
				strcpy(sayStr,"");
				if (tmpTime.hour==23)
				{
					strcpy(sayStr,"00");
				}
				else
				{
				  if (tmpTime.hour<9)
				  {
					   strcat(sayStr,"0");
				  }
				  strcat(sayStr,intToString(tmpTime.hour+1,3,str));
				}
				guiDisplay(17, tmpY, sayStr, 1);
				
				readTime  = timeHexToBcd(tmpTime);
				strcpy(sayStr,"");
				if(readMeterData(dataBuff, tmpLink->mp, CURVE_DATA_PRESENT, dataType, &readTime, 0x60) == TRUE)
				{
				   if (dataBuff[offset]!=0xee)
       	   {
  				   switch(layer2Light)
  				   {
               case 1:
               case 2:
     	    	     sign = 0;
               	 disData = dataBuff[offset] | dataBuff[offset+1]<<8 | dataBuff[offset+2]<<16;
     	    	     if (dataBuff[offset+2]&0x80)
     	    	     {
     	    	        sign = 1;
     	    	        disData &=0x7fff;
     	    	     }
                 decimal = (disData>>12 & 0xf)*1000
                          +(disData>>8 & 0xf)*100
                          +(disData>>4 & 0xf)*10
                          +(disData & 0xf);
                 integer = (disData>>20 & 0xf)*10+(disData>>16 & 0xf);
                 if (sign==1)
                 {
                 	 strcat(sayStr,"-");
                 }
                 strcat(sayStr,floatToString(integer,decimal,4,2,str));
                 if (strlen(sayStr)<=5)
                 {
                   strcpy(str,"");
                   for(tmpCount=0;tmpCount<5-strlen(sayStr);tmpCount++)
                   {
                	   strcat(str," ");
                   }
                   strcat(str, sayStr);
                   strcpy(sayStr,str);
                 }
                 tmpX = 24;
               	 break;
             }
       	   }
       	   else
       	   {
       	      strcpy(sayStr,"-----");
       	      if (tmpX==0)
       	      {
       	      	 tmpX = 24;
       	      }
           }
				}
				else
				{
				   strcpy(sayStr,"-----");
       	   if (tmpX==0)
       	   {
       	   	 tmpX = 24;
       	   }
       	}
				
				guiDisplay(70+tmpX,tmpY,sayStr,1);

				lcdRefresh(tmpY, tmpY+16);
				
				tmpY += 18;
				tmpTime = nextTime(tmpTime, 60, 0);				
		 }
	 }
	 
	 lcdRefresh(17, 160);
}

/*******************************************************
��������:meterVisionValue
��������:���ܱ�ʾ��
���ú���:
�����ú���:
�������:
�������:
����ֵ:void
*******************************************************/
void meterVisionValue(INT8U item)
{
	 INT8U                dataBuff[LENGTH_OF_ENERGY_RECORD];
 	 INT32U               integer,decimal;
   struct cpAddrLink    *tmpLink;
	 char                 str[15];
	 char                 sayStr[10];
	 DATE_TIME            time;
	 INT16U               i;
	 INT8U                tmpY;

 	 menuInLayer = 2;
	 guiLine(1,17,160,160,0);

 	 tmpLink = queryMpLink;
 	 i = 0;
 	 while(tmpLink!=NULL && i<item)
 	 {
 	   tmpLink = tmpLink->next;
 	   i++;
 	 }
 	 
 	 if (tmpLink!=NULL)
 	 {
   	 if (tmpLink->protocol==AC_SAMPLE)
   	 {
   	 	  tmpLink = tmpLink->next;
   	 	  if (tmpLink==NULL)
   	 	  {
   	 	  	 tmpLink = queryMpLink;
   	 	  	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
   	 	  }
   	 	  else
   	 	  {
   	 	  	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
   	 	  }
   	 }
   	 
  	 strcpy(sayStr,"������");
  	 strcat(sayStr,digital2ToString(tmpLink->mp,str));
  	 strcat(sayStr,"���ܱ�ʾ��");
  	 guiDisplay(8,LCD_LINE_1,sayStr,0);
     
     //��ѯ����������ϴγ���ʱ��
     time = queryCopyTime(tmpLink->mp);
     if (readMeterData(dataBuff, tmpLink->mp, PRESENT_DATA, ENERGY_DATA, &time, 0)==TRUE)
     {
       	tmpY=LCD_LINE_2+8;
       	for(i=0;i<5;i++)
       	{
       	  switch(i)
       	  {
       	  	case 0:
       	      strcpy(sayStr,"�����й���=");
       	      break;
       	      
       	  	case 1:
       	      strcpy(sayStr,"��=");
       	      break;

       	  	case 2:
       	      strcpy(sayStr,"��=");
       	      break;
       	      
       	  	case 3:
       	      strcpy(sayStr,"ƽ=");
       	      break;
       	      
       	  	case 4:
       	      strcpy(sayStr,"��=");
       	      break;
       	  }
       	  
       	  if (dataBuff[POSITIVE_WORK_OFFSET+i*4]!=0xee)
       	  {
             decimal = bcdToHex(dataBuff[POSITIVE_WORK_OFFSET+i*4]);
             integer = bcdToHex(dataBuff[POSITIVE_WORK_OFFSET+i*4+1] |dataBuff[POSITIVE_WORK_OFFSET+i*4+2]<<8
                     | dataBuff[POSITIVE_WORK_OFFSET+i*4+3]<<16);
             strcpy(str, floatToString(integer,decimal,2,2,str));
             strcat(sayStr,str);
       	  }
       	  
       	  if ((i!=0) && (i%2==0))
       	  {
       	    guiDisplay(81,tmpY,sayStr,1);
       	  }
       	  else
       	  {
       	    guiDisplay(1,tmpY,sayStr,1);
       	  }
       	  
       	  if (i==0 || i==2)
       	  {
       	  	 tmpY += 16;
       	  }
       	}
        
        tmpY+=24;
        strcpy(sayStr,"�����޹���=");       	  
       	if (dataBuff[POSITIVE_NO_WORK_OFFSET]!=0xee)
       	{
           decimal = bcdToHex(dataBuff[POSITIVE_NO_WORK_OFFSET]);
           integer = bcdToHex(dataBuff[POSITIVE_NO_WORK_OFFSET+1] |dataBuff[POSITIVE_NO_WORK_OFFSET+2]<<8
                     | dataBuff[POSITIVE_NO_WORK_OFFSET+3]<<16);
           strcpy(str, floatToString(integer,decimal,2,2,str));
           strcat(sayStr,str);
       	}
       	guiDisplay(1,tmpY,sayStr,1);

        tmpY+=16;
        strcpy(sayStr,"�����޹���=");
       	if (dataBuff[NEGTIVE_NO_WORK_OFFSET]!=0xee)
       	{
           decimal = bcdToHex(dataBuff[NEGTIVE_NO_WORK_OFFSET]);
           integer = bcdToHex(dataBuff[NEGTIVE_NO_WORK_OFFSET+1] |dataBuff[NEGTIVE_NO_WORK_OFFSET+2]<<8
                     | dataBuff[NEGTIVE_NO_WORK_OFFSET+3]<<16);
           strcpy(str, floatToString(integer,decimal,2,2,str));
           strcat(sayStr,str);
       	}
       	guiDisplay(1,tmpY,sayStr,1);
       	
       	
     }
     else
     {
     	  guiDisplay(1, LCD_LINE_2+8,"�����й���=",1);
     	  guiDisplay(1, LCD_LINE_3+8,"��=",1);
     	  guiDisplay(81,LCD_LINE_3+8,"��=",1);
     	  guiDisplay(1, LCD_LINE_4+8,"ƽ=",1);
     	  guiDisplay(81,LCD_LINE_4+8,"��=",1);
     	  guiDisplay(1, LCD_LINE_6,"�����޹���=",1);
     	  guiDisplay(1, LCD_LINE_7,"�����޹���=",1);
     }
     
     time = queryCopyTime(tmpLink->mp);
     if (readMeterData(dataBuff, tmpLink->mp, PRESENT_DATA, REQ_REQTIME_DATA, &time, 0)==TRUE)
     {
       	strcpy(sayStr,"�����й�����=");
       	if (dataBuff[REQ_POSITIVE_WORK_OFFSET]!=0xee)
       	{
           decimal = bcdToHex(dataBuff[REQ_POSITIVE_WORK_OFFSET]|dataBuff[REQ_POSITIVE_WORK_OFFSET+1]<<8);
           integer = bcdToHex(dataBuff[REQ_POSITIVE_WORK_OFFSET+2]);
           strcpy(str, floatToString(integer,decimal,4,4,str));
           strcat(sayStr,str);
       	}
       	guiDisplay(1,LCD_LINE_8,sayStr,1);
	      
	      strcpy(sayStr,"����ʱ��=");
	      if (dataBuff[REQ_TIME_P_WORK_OFFSET]!=0xee)
	      {
	        strcat(sayStr,digital2ToString(bcdToHex(dataBuff[REQ_TIME_P_WORK_OFFSET+3]),str));
	        strcat(sayStr,"-");
	        strcat(sayStr,digital2ToString(bcdToHex(dataBuff[REQ_TIME_P_WORK_OFFSET+2]),str));
	        strcat(sayStr," ");
	        strcat(sayStr,digital2ToString(bcdToHex(dataBuff[REQ_TIME_P_WORK_OFFSET+1]),str));
	        strcat(sayStr,":");
	        strcat(sayStr,digital2ToString(bcdToHex(dataBuff[REQ_TIME_P_WORK_OFFSET]),str));
	      }
	      guiDisplay(1,LCD_LINE_8+16,sayStr,1);
     }
     else
     {
     	  guiDisplay(1, LCD_LINE_8,"�����й�����=",1);
     	  guiDisplay(1, LCD_LINE_8+16,"����ʱ��=",1);
     }     
   }
   else
   {
     guiDisplay(12, LCD_LINE_3,"�޲��������ò���!",1);
   }
   
   lcdRefresh(17,160);
}

/*******************************************************
��������:statusOfSwitch
��������:��ǰ������״̬
���ú���:
�����ú���:
�������:num
�������:
����ֵ:void
*******************************************************/
void statusOfSwitch(void)
{
	 menuInLayer = 3;
	  
	 guiLine(1,17,160,160,0);

 	 if (statusInput[0]&0x1)
 	 {
 	   if (stOfSwitch&0x1)
 	   {
 	     guiDisplay(30,LCD_LINE_2,"������1:��",1);
 	   }
 	   else
 	   {
 	     guiDisplay(30,LCD_LINE_2,"������1:��",1);
 	   }
 	 }
 	 else
 	 {
 	 	  guiDisplay(30,LCD_LINE_2,"������1:δ����",1);
 	 }
 	 
 	 if (statusInput[0]&0x2)
 	 {
 	 	 if (stOfSwitch&0x2)
 	 	 {
 	 	   guiDisplay(30,LCD_LINE_3,"������2:��",1);
 	 	 }
 	 	 else
 	 	 {
 	 	   guiDisplay(30,LCD_LINE_3,"������2:��",1);
 	 	 }
 	 }
 	 else
 	 {
 	 	  guiDisplay(30,LCD_LINE_3,"������2:δ����",1);
 	 }
 	 
 	 if (statusInput[0]&0x4)
 	 {
 	 	 if (stOfSwitch&0x4)
 	     guiDisplay(30,LCD_LINE_4,"������3:��",1);
 	   else
 	     guiDisplay(30,LCD_LINE_4,"������3:��",1);
 	 }
 	 else
 	 {
 	 	  guiDisplay(30,LCD_LINE_4,"������3:δ����",1);
 	 }
 	 
 	 if (statusInput[0]&0x8)
 	 {
 	   if (stOfSwitch&0x8)
 	   {
 	   	  guiDisplay(30,LCD_LINE_5,"������4:��",1);
 	   }
 	   else
 	   {
 	     guiDisplay(30,LCD_LINE_5,"������4:��",1);
 	   }
 	 }
 	 else
 	 {
 	 	  guiDisplay(30,LCD_LINE_5,"������4:δ����",1);
 	 }
 	 
   if (statusInput[0]&0x10)
	 {
	 	 if (stOfSwitch&0x10)
	 	 {
	     guiDisplay(30,LCD_LINE_6,"������5:��",1);
	   }
	   else
	   {
	     guiDisplay(30,LCD_LINE_6,"������5:��",1);
	   }
	 }
	 else
	 {
	   guiDisplay(30,LCD_LINE_6,"������5:δ����",1);
	 }
	   
	 if (statusInput[0]&0x20)
	 {
 	   if (stOfSwitch&0x20)
 	   {
 	     guiDisplay(30,LCD_LINE_7,"������6:��",1);
 	   }
 	   else
 	   {
 	     guiDisplay(30,LCD_LINE_7,"������6:��",1);
 	   }
 	 }
	 else
	 {
	   guiDisplay(30,LCD_LINE_7,"������6:δ����",1);
	 }
 
	 if (getGateKValue()==1)   //�����͵��ſؽڵ�
	 {
	   guiDisplay(30,LCD_LINE_8,"�ſ�:��",1);
	 }
	 else
	 {
	   guiDisplay(30,LCD_LINE_8,"�ſ�:��",1);
	 }
	  
	 lcdRefresh(17,160);
}

/*******************************************************
��������:controlStatus
��������:����״̬(����˵�3)
���ú���:
�����ú���:
�������:num
�������:
����ֵ:void
*******************************************************/
void controlStatus(INT8U num)
{
 	  INT8U i,tmpLine,pn;
 	  char  str[42],str1[15];
	  INT8U tmpUse;

 	  menuInLayer = 2;
	  guiLine(1,17,160,160,0);

	  if(totalAddGroup.numberOfzjz==0)
	  {
	  	 guiDisplay(12,30,"���ܼ������ò���!",1);
	  	 lcdRefresh(17,160);
	  	 return;
	  }
	  
	  pn = totalAddGroup.perZjz[num].zjzNo;
	  strcpy(str,"�ܼ���");
   	strcat(str,digital2ToString(pn,str1));
   	guiDisplay(48,LCD_LINE_1,str,0);
   	
   	for(i=0;i<6;i++)
    {
    	 switch(i)
    	 {
    	 	  case 0:
    	 	  	tmpUse = ctrlRunStatus[pn-1].ifUsePrdCtrl;
    	 	  	strcpy(str,"ʱ�ο�:");
    	 	  	tmpLine = LCD_LINE_2;
    	 	  	break;
    	 	  	
    	 	  case 1:
    	 	  	tmpUse = ctrlRunStatus[pn-1].ifUseWkdCtrl;
    	 	  	strcpy(str,"���ݿ�:");
    	 	  	tmpLine = LCD_LINE_3;
    	 	  	break;
    	 	  	
    	 	  case 2:
    	 	  	tmpUse = ctrlRunStatus[pn-1].ifUseObsCtrl;
    	 	  	strcpy(str,"��ͣ��:");
    	 	  	tmpLine = LCD_LINE_4;
    	 	  	break;
    	 	  	
    	 	  case 3:
    	 	  	tmpUse = ctrlRunStatus[pn-1].ifUsePwrCtrl;
    	 	  	strcpy(str,"�¸���:");
    	 	  	tmpLine = LCD_LINE_5;
    	 	  	break;
    	 	  	
    	 	  case 4:
    	 	  	tmpUse = ctrlRunStatus[pn-1].ifUseMthCtrl;
    	 	  	strcpy(str,"�µ��:");
    	 	  	tmpLine = LCD_LINE_6;
    	 	  	break;
    	 	  	
    	 	  case 5:
    	 	  	tmpUse = ctrlRunStatus[pn-1].ifUseChgCtrl;
    	 	  	strcpy(str,"�����:");
    	 	  	tmpLine = LCD_LINE_7;
    	 	  	break;    	 	  	
    	 }
     	 
     	 if (tmpUse==CTRL_JUMP_IN)
     	 {
     		  strcat(str,"Ͷ��");
     	 }
     	 else
     	 {
     		 if (tmpUse==CTRL_RELEASE || tmpUse==CTRL_RELEASEED)
     		 {
     		 	  strcat(str,"���");
     		 }
     		 else
     		 {
     		 	  strcat(str,"δ��");
     		 }
     	 }
     	 guiDisplay(32,tmpLine,str,1);
    }
    
    guiDisplay(32,LCD_LINE_8,"����:",1);
   	
   	lcdRefresh(17,160);
}

/*******************************************************
��������:periodPara
��������:ʱ�ι��ز���
���ú���:
�����ú���:
�������:num(�ܼ������)
�������:
����ֵ:void
*******************************************************/
void periodPara(INT8U num)
{
  INT8U   pn;
  char    say[42],str1[15];
  INT8U   periodNum;
	INT16U  periodCtrlLimit;
  INT32U  tmpKw;
  INT16U  tmpWatt;

  menuInLayer = 3;
  guiLine(1,17,160,160,0);

  if(totalAddGroup.numberOfzjz==0)
  {
  	 guiDisplay(12,30,"���ܼ������ò���!",1);
  	 lcdRefresh(17,160);
  	 return;
  }
  
  pn = totalAddGroup.perZjz[num].zjzNo;
  strcpy(say,"�ܼ���");
 	strcat(say,digital2ToString(pn,str1));
 	guiDisplay(48,LCD_LINE_1,say,0);
 	
  //if (ctrlRunStatus[pn-1].ifUsePrdCtrl == CTRL_JUMP_IN)
  //{
 	   //Ͷ���ִ�
 	   strcpy(say,"�����ִ�:");
 	   if (powerCtrlRoundFlag[pn-1].flag&0x1)
 	   {
 	   	  strcat(say,"��");
 	   }
 	   if (powerCtrlRoundFlag[pn-1].flag>>1&0x1)
 	   {
 	   	  strcat(say,"��");
 	   }
 	   if (powerCtrlRoundFlag[pn-1].flag>>2&0x1)
 	   {
 	   	  strcat(say,"��");
 	   }
 	   if (powerCtrlRoundFlag[pn-1].flag>>3&0x1)
 	   {
 	   	  strcat(say,"��");
 	   }
 	   guiDisplay(1,LCD_LINE_2,say,1);
 	   
 	   strcpy(say,"Ͷ�뷽��:��");
 	   strcat(say,intToString(periodCtrlConfig[pn-1].ctrlPara.ctrlPeriod+1, 3, str1));
 	   strcat(say,"�׷���");
 	   guiDisplay(1,LCD_LINE_3,say,1);
 	   
 	   strcpy(say,"Ͷ��ʱ��:");
 	   if (periodCtrlConfig[pn-1].ctrlPara.limitPeriod&0x1)
 	   {
 	   	  strcat(say,"1");
 	   }
 	   if (periodCtrlConfig[pn-1].ctrlPara.limitPeriod>>1&0x1)
 	   {
 	   	  strcat(say,"2");
 	   }
 	   if (periodCtrlConfig[pn-1].ctrlPara.limitPeriod>>2&0x1)
 	   {
 	   	  strcat(say,"3");
 	   }
 	   if (periodCtrlConfig[pn-1].ctrlPara.limitPeriod>>3&0x1)
 	   {
 	   	  strcat(say,"4");
 	   }
 	   if (periodCtrlConfig[pn-1].ctrlPara.limitPeriod>>4&0x1)
 	   {
 	   	  strcat(say,"5");
 	   }
 	   if (periodCtrlConfig[pn-1].ctrlPara.limitPeriod>>5&0x1)
 	   {
 	   	  strcat(say,"6");
 	   }
 	   if (periodCtrlConfig[pn-1].ctrlPara.limitPeriod>>6&0x1)
 	   {
 	   	  strcat(say,"7");
 	   }
 	   if (periodCtrlConfig[pn-1].ctrlPara.limitPeriod>>7&0x1)
 	   {
 	   	  strcat(say,"8");
 	   }
 	   guiDisplay(1,LCD_LINE_4,say,1);
 	   
 	   strcpy(say,"��ǰʱ��:");
 	   if ((periodNum=getPowerPeriod(sysTime)) == 0)
 	   {
 	     strcat(say, "��ȡʧ��");
 	   }
 	   else
 	   {
 	   	 sprintf(str1, "%d", periodNum);
 	   	 strcat(say, str1);
 	   }
 	   guiDisplay(1,LCD_LINE_5,say,1);

 	   strcpy(say,"��ǰ��ֵ:");
 	   if (getPowerLimit(pn, periodCtrlConfig[pn-1].ctrlPara.ctrlPeriod, periodNum, (INT8U *)&periodCtrlLimit)==FALSE)
 	   {
 	     strcat(say, "��ȡʧ��");
 	   }
 	   else
 	   {
       tmpKw = periodCtrlLimit;
       tmpKw = countAlarmLimit((INT8U *)&tmpKw, 0x2, 0x0,&tmpWatt);
       if (tmpKw>9999999 && tmpWatt==0)
       {
          tmpKw/=1000;
          strcat(say,intToString(tmpKw,3,str1));
          strcat(say,"Mw");
       }
       else
       {
          strcat(say,intToString(tmpKw,3,str1));
          if (tmpWatt>0)
          {
             strcat(say,".");
             strcat(say,intToString(tmpWatt,3,str1));
          }
          strcat(say,"Kw");
       }
 	   }

 	   guiDisplay(1,LCD_LINE_6,say,1);

  //}
  //else
  //{
  //	 guiDisplay(1,LCD_LINE_3,"ʱ�ι���δͶ��!",1);
  //}
 	
 	lcdRefresh(17,160);
}

/*******************************************************
��������:wkdPara
��������:���ݿز���
���ú���:
�����ú���:
�������:num(�ܼ������)
�������:
����ֵ:void
*******************************************************/
void wkdPara(INT8U num)
{
 	  INT8U   pn;
 	  char    say[42],str1[15];
    INT32U  tmpKw;
    INT16U  tmpWatt;

 	  menuInLayer = 3;
	  guiLine(1,17,160,160,0);

	  if(totalAddGroup.numberOfzjz==0)
	  {
	  	 guiDisplay(12,30,"���ܼ������ò���!",1);
	  	 lcdRefresh(17,160);
	  	 return;
	  }
	  
	  pn = totalAddGroup.perZjz[num].zjzNo;
	  strcpy(say,"�ܼ���");
   	strcat(say,digital2ToString(pn,str1));
   	guiDisplay(48,LCD_LINE_1,say,0);
    
	  if (pn > 0 && pn <= 8 && wkdCtrlConfig[pn-1].wkdStartHour != 0xFF)
	  {
   	   //Ͷ���ִ�
   	   strcpy(say,"�ִ�");
   	   if (powerCtrlRoundFlag[pn-1].flag&0x1)
   	   {
   	   	  strcat(say,"��");
   	   }
   	   if (powerCtrlRoundFlag[pn-1].flag>>1&0x1)
   	   {
   	   	  strcat(say,"��");
   	   }
   	   if (powerCtrlRoundFlag[pn-1].flag>>2&0x1)
   	   {
   	   	  strcat(say,"��");
   	   }
   	   if (powerCtrlRoundFlag[pn-1].flag>>3&0x1)
   	   {
   	   	  strcat(say,"��");
   	   }
   	   guiDisplay(1,LCD_LINE_2,say,1);
   	   
   	   //��ֵ
   	   strcpy(say,"���ݿض�ֵ");
	     tmpKw = wkdCtrlConfig[pn-1].wkdLimit;
	     tmpKw = countAlarmLimit((INT8U *)&tmpKw, 0x2, 0x0,&tmpWatt);
	     if (tmpKw>9999999 && tmpWatt==0)
	     {
	        tmpKw/=1000;
	        strcat(say,intToString(tmpKw,3,str1));
          strcat(say,"Mw");
	     }
	     else
	     {
	        strcat(say,intToString(tmpKw,3,str1));
	        if (tmpWatt>0)
	        {
	           strcat(say,".");
	           strcat(say,intToString(tmpWatt,3,str1));
	        }
          strcat(say,"Kw");
	     }
	     guiDisplay(1,LCD_LINE_3,say,1);
   	   
   	   strcpy(say,"��ʼ:");
   	   strcat(say,digital2ToString((wkdCtrlConfig[pn-1].wkdStartHour&0xf)+(wkdCtrlConfig[pn-1].wkdStartHour>>4&0xf)*10,str1));
   	   strcat(say,":");
   	   strcat(say,digital2ToString((wkdCtrlConfig[pn-1].wkdStartMin&0xf)+(wkdCtrlConfig[pn-1].wkdStartMin>>4&0xf)*10,str1));
	     guiDisplay(1,LCD_LINE_4,say,1);
   	   
   	   strcpy(say,"����:");
   	   strcat(say,intToString(wkdCtrlConfig[pn-1].wkdTime/2,3,str1));
   	   if (wkdCtrlConfig[pn-1].wkdTime%2)
   	   {
   	     strcat(say,".5");
   	   }
   	   strcat(say,"Сʱ");
	     guiDisplay(1,LCD_LINE_5,say,1);
	     
	     strcpy(say,"������:");
	     guiDisplay(1,LCD_LINE_6,say,1);
	     
	     strcpy(say,"");
	     if (wkdCtrlConfig[pn-1].wkdDate>>1&0x1)
	     {
	     	  strcat(say,"һ");
	     }
	     if (wkdCtrlConfig[pn-1].wkdDate>>2&0x1)
	     {
	     	  strcat(say,"��");
	     }
	     if (wkdCtrlConfig[pn-1].wkdDate>>3&0x1)
	     {
	     	  strcat(say,"��");
	     }
	     if (wkdCtrlConfig[pn-1].wkdDate>>4&0x1)
	     {
	     	  strcat(say,"��");
	     }
	     if (wkdCtrlConfig[pn-1].wkdDate>>5&0x1)
	     {
	     	  strcat(say,"��");
	     }
	     if (wkdCtrlConfig[pn-1].wkdDate>>6&0x1)
	     {
	     	  strcat(say,"��");
	     }
	     if (wkdCtrlConfig[pn-1].wkdDate>>7&0x1)
	     {
	     	  strcat(say,"��");
	     }
	     guiDisplay(49,LCD_LINE_7,say,1);
   	}
   	else
   	{
   		 guiDisplay(33,LCD_LINE_3,"δ���ó��ݿز���",1);
   	}
   	
   	lcdRefresh(17,160);
}

/*******************************************************
��������:powerDownPara
��������:��ǰ�����¸��ز���
���ú���:
�����ú���:
�������:num(�ܼ������)
�������:
����ֵ:void
*******************************************************/
void powerDownPara(INT8U num)
{
 	  INT8U   pn;
 	  char    say[42],str1[15];
    INT32U  tmpKw;
    INT16U  tmpWatt;

 	  menuInLayer = 3;
	  guiLine(1,17,160,160,0);

	  if(totalAddGroup.numberOfzjz==0)
	  {
	  	 guiDisplay(12,30,"���ܼ������ò���!",1);
	  	 lcdRefresh(17,160);
	  	 return;
	  }
	  
	  pn = totalAddGroup.perZjz[num].zjzNo;
	  strcpy(say,"�ܼ���");
   	strcat(say,digital2ToString(pn,str1));
   	guiDisplay(48,LCD_LINE_1,say,0);
   	
    //if (ctrlRunStatus[pn-1].ifUsePwrCtrl == CTRL_JUMP_IN)
    //{
   	   //Ͷ���ִ�
   	   strcpy(say,"�ִ�");
   	   if (powerCtrlRoundFlag[pn-1].flag&0x1)
   	   {
   	   	  strcat(say,"��");
   	   }
   	   if (powerCtrlRoundFlag[pn-1].flag>>1&0x1)
   	   {
   	   	  strcat(say,"��");
   	   }
   	   if (powerCtrlRoundFlag[pn-1].flag>>2&0x1)
   	   {
   	   	  strcat(say,"��");
   	   }
   	   if (powerCtrlRoundFlag[pn-1].flag>>3&0x1)
   	   {
   	   	  strcat(say,"��");
   	   }
   	   guiDisplay(1,LCD_LINE_2,say,1);
   	   
   	   strcpy(say,"��ֵ:");
   	   if (powerDownCtrl[pn-1].freezeTime.year!=0xff)
   	   {
   	   	  strcat(say,"�ȴ�����!");
   	   }
   	   else
   	   {
   	   	  tmpKw = powerDownCtrl[pn-1].powerDownLimit;
   	   	  tmpWatt = powerDownCtrl[pn-1].powerLimitWatt;
    	    if (tmpKw>9999999 && tmpWatt==0)
    	    {
    	        tmpKw/=1000;
    	        strcat(say,intToString(tmpKw,3,str1));
              strcat(say,"Mw");
    	    }
    	    else
    	    {
    	        strcat(say,intToString(tmpKw,3,str1));
    	        if (tmpWatt>0)
    	        {
    	           strcat(say,".");
    	           strcat(say,intToString(tmpWatt,3,str1));
    	        }
              strcat(say,"Kw");
    	    }
   	   }
   	   guiDisplay(1,LCD_LINE_3,say,1);
   	   
   	   strcpy(say,"�¸�ϵ��:");
 	     if (powerDownCtrl[pn-1].floatFactor&0x80)
 	     {
 	     	  strcat(say,"�¸�");
 	     }
 	     else
 	     {
 	     	  strcat(say,"�ϸ�");
 	     }
 	     strcat(say,intToString((powerDownCtrl[pn-1].floatFactor&0xF) + (powerDownCtrl[pn-1].floatFactor>>4&0x7)*10,3,str1));
 	     strcat(say,"%");
   	   guiDisplay(1,LCD_LINE_4,say,1);

   	   strcpy(say,"����ʱ��:");
   	   strcat(say,intToString(powerDownCtrl[pn-1].freezeDelay,3,str1));
   	   strcat(say,"��");
   	   guiDisplay(1,LCD_LINE_5,say,1);

   	   strcpy(say,"����ʱ��:");
   	   strcat(say,intToString(powerDownCtrl[pn-1].downCtrlTime,3,str1));
   	   strcat(say,"*0.5Сʱ");
   	   guiDisplay(1,LCD_LINE_6,say,1);
   	   
   	   strcpy(say,"�澯ʱ��:");
   	   guiDisplay(1,LCD_LINE_7,say,1);
   	   strcpy(say,"��1��:");
   	   strcat(say,intToString(powerDownCtrl[pn-1].roundAlarmTime[0],3,str1));
   	   strcat(say,"��");
   	   guiDisplay(1,LCD_LINE_8,say,1);
   	   strcpy(say,"��2��:");
   	   strcat(say,intToString(powerDownCtrl[pn-1].roundAlarmTime[1],3,str1));
   	   strcat(say,"��");
   	   guiDisplay(81,LCD_LINE_8,say,1);
   	   strcpy(say,"��3��:");
   	   strcat(say,intToString(powerDownCtrl[pn-1].roundAlarmTime[2],3,str1));
   	   strcat(say,"��");
   	   guiDisplay(1,LCD_LINE_9,say,1);
   	   strcpy(say,"��4��:");
   	   strcat(say,intToString(powerDownCtrl[pn-1].roundAlarmTime[3],3,str1));
   	   strcat(say,"��");
   	   guiDisplay(81,LCD_LINE_9,say,1);   	   
    //}
    //else
    //{
    //	 guiDisplay(1,LCD_LINE_3,"�����¸���δͶ��!",1);
    //}
   	
   	lcdRefresh(17,160);
}

/*******************************************************
��������:meterPara
��������:���ܱ������Ϣ(����˵�2.5)
���ú���:
�����ú���:
�������:num
�������:
����ֵ:void
*******************************************************/
void meterPara(struct cpAddrLink *mpLink)
{
 	  INT8U j,ifFound,noneZero;
 	  char  str[42],str1[5];

 	  menuInLayer = 3;
	  guiLine(1,17,160,160,0);
	  
	  if(mpLink==NULL)
	  {
	  	 guiDisplay(12,30,"�޲��������ò���!",1);
	  	 lcdRefresh(17,160);
	  	 return;
	  }
	  
	  strcpy(str,"������");
	  strcat(str,intToString(mpLink->mp,3,str1));
	  guiDisplay(48,LCD_LINE_1,str,0);
	  	  
	  strcpy(str,"���:");
	  strcat(str,intToString(mpLink->mpNo,3,str1));
	  guiDisplay(1,LCD_LINE_3,str,1);

	  strcpy(str,"�˿�:");
	  strcat(str,intToString(mpLink->port,3,str1));
	  guiDisplay(1,LCD_LINE_4,str,1);
	  	  
	  strcpy(str,"��Լ:");
	  switch(mpLink->protocol)
	  {
	     case 1:   //DL/T645-1997
	     	 strcat(str,"DL/T645-1997");
	     	 break;

	     case 2:   //����
	     	 strcat(str,"��������װ��");
	     	 break;
	     
	     case SIMENS_ZD_METER:
	     case SIMENS_ZB_METER:
	     	 strcat(str,"������ZD/ZB��");
	     	 break;

	     case ABB_METER:
	     	 strcat(str,"ABB����");
	     	 break;

	     case EDMI_METER:
	     	 strcat(str,"����EDMI��");
	     	 break;
	     	 
	     case 30:   //DL/T645-207
	     	 strcat(str,"DL/T645-2007");
	     	 break;
			 
	     case 57:   //DL/T645-207
	     	 strcat(str,"����๦�ܱ�");
	     	 break;
			 
	     case 58:   //DL/T645-207
	     	 strcat(str,"����UI��");
	     	 break;
	  }
	  guiDisplay(1,LCD_LINE_5,str,1);
	  	  
	  strcpy(str,"���ַ:");
	  noneZero = 0;
	  for(j=6;j>0;j--)
	  {
	  	 if(mpLink->addr[j-1]!=0x00 || noneZero !=0)
	  	 {
	  	 	  noneZero++;
	  	 	  strcat(str,digital2ToString((mpLink->addr[j-1]/0x10)*10+mpLink->addr[j-1]%0x10,str1));
	  	 }
	  }
	  guiDisplay(1,LCD_LINE_6,str,1);
	  
	  lcdRefresh(17,160);
}

/*******************************************************
��������:kvkikp
��������:KvKiKp(����˵�2.4)
���ú���:
�����ú���:
�������:num
�������:
����ֵ:void
*******************************************************/
void kvkikp(struct cpAddrLink *mpLink)
{
 	  char  str[42],str1[5];
	  MEASURE_POINT_PARA pointPara;

 	  menuInLayer = 3;
	  guiLine(1,17,160,160,0);
	  
	  if(mpLink==NULL)
	  {
	  	 guiDisplay(12,30,"�޲��������ò���!",1);
	  	 lcdRefresh(17,160);
	  	 return;
	  }
	  
	  strcpy(str,"������");
	  strcat(str,intToString(mpLink->mp,3,str1));
	  guiDisplay(48,LCD_LINE_1,str,0);
	  
	  if(selectViceParameter(0x04, 25, mpLink->mp, (INT8U *)&pointPara, sizeof(MEASURE_POINT_PARA)) == TRUE)
	  {
	  	strcpy(str,"Kv=");
	  	strcat(str,intToString(pointPara.voltageTimes,3,str1));
	  	guiDisplay(40,LCD_LINE_3,str,1);

	  	strcpy(str,"Ki=");
	  	strcat(str,intToString(pointPara.currentTimes,3,str1));
	  	guiDisplay(40,LCD_LINE_4,str,1);
	  	
	  	strcpy(str,"Kp=");
	  	strcat(str,intToString(pointPara.voltageTimes*pointPara.currentTimes,3,str1));
	  	guiDisplay(40,LCD_LINE_5,str,1);
	  }
	  else
	  {
	  	guiDisplay(40,LCD_LINE_3,"Kvδ����",1);
	  	guiDisplay(40,LCD_LINE_4,"Kiδ����",1);
	  	guiDisplay(40,LCD_LINE_5,"Kpδ����",1);
	  }
	  
	  lcdRefresh(17,160);
}

/*******************************************************
��������:configPara
��������:���ò���(����˵�2.6)
���ú���:
�����ú���:
�������:num
�������:
����ֵ:void
*******************************************************/
void configPara(INT8U rowLight,INT8U colLight)
{	 	   
 	  char  str[40],str1[20];
 	  INT8U tmpX;
 	  INT8U i;

	  guiLine(1,17,160,160,0);

	  if (rowLight==88 && colLight==88)
	  { 
 	    menuInLayer = 3;
  	  
  	  //��������
  	  guiDisplay(1,LCD_LINE_1,"��������:",1);
      strcpy(str,digitalToChar(addrField.a1[1]>>4));
      strcat(str,digitalToChar(addrField.a1[1]&0xf));
      strcat(str,digitalToChar(addrField.a1[0]>>4));
      strcat(str,digitalToChar(addrField.a1[0]&0xf));
  	  guiDisplay(73,LCD_LINE_1,str,1);
  	  
  	  guiDisplay(128,LCD_LINE_1,"�޸�",0);	  
  
      strcpy(str, "�ն˵�ַ:");
      #ifdef TE_ADDR_USE_BCD_CODE
       strcat(str,int8uToHex(addrField.a2[1],str1));
       strcat(str,int8uToHex(addrField.a2[0],str1));
      #else
       strcat(str,intToString(addrField.a2[1]<<8 | addrField.a2[0],3,str1));
       strcat(str,"[");
       strcat(str,int8uToHex(addrField.a2[1],str1));
       strcat(str,int8uToHex(addrField.a2[0],str1));
       strcat(str,"]");
      #endif
      guiDisplay(1,LCD_LINE_2,str,1);
  	   
  	  //APN����
  	  guiDisplay(1,  LCD_LINE_3, "APN����:",1);
  	  guiDisplay(68, LCD_LINE_3, (char *)ipAndPort.apn, 1);
  	   
  	  guiDisplay(1,LCD_LINE_4,"��վIP������˿�:",1);
   	  strcpy(str,intToIpadd(ipAndPort.ipAddr[0]<<24 | ipAndPort.ipAddr[1]<<16 | ipAndPort.ipAddr[2]<<8 | ipAndPort.ipAddr[3],str1));
      strcat(str,":");
  	  guiDisplay(1,LCD_LINE_5,str,1);
  	  tmpX = 1+8*strlen(str);
      strcpy(str,intToString(ipAndPort.port[1]<<8 | ipAndPort.port[0],3,str1));
  	  guiDisplay(tmpX,LCD_LINE_5,str,1);
  
  	  guiDisplay(1,LCD_LINE_6,"��IP������˿�:",1);
   	  strcpy(str,intToIpadd(ipAndPort.ipAddrBak[0]<<24 | ipAndPort.ipAddrBak[1]<<16 | ipAndPort.ipAddrBak[2]<<8 | ipAndPort.ipAddrBak[3],str1));
      strcat(str,":");
  	  guiDisplay(1,LCD_LINE_7,str,1);
  	  tmpX = 1+8*strlen(str);
      strcpy(str,intToString(ipAndPort.portBak[1]<<8 | ipAndPort.portBak[0],3,str1));
  	  guiDisplay(tmpX,LCD_LINE_7,str,1);
  	   
  	  guiDisplay(1,LCD_LINE_8,"�ն�IP:",1);
  	  guiDisplay(1,LCD_LINE_8+16,intToIpadd(wlLocalIpAddr,str1),1);
	  }
	  else   //�޸����ò���
	  {
 	    menuInLayer = 4;
  	  guiDisplay(1,LCD_LINE_1,"��������:",1);
      tmpX = 73;
      for(i=0;i<4;i++)
      {
        str[0] = commParaItem[0][i];
        str[1] = '\0';
        if (rowLight==0 && i==colLight)
        {
          guiDisplay(tmpX,17,str,0);
        }
        else
        {
          guiDisplay(tmpX,17,str,1);
        }
        tmpX += 8;
      }
      
      guiDisplay(1, LCD_LINE_2, "�ն˵�ַ:", 1);
      tmpX = 73;      
      #ifdef TE_ADDR_USE_BCD_CODE
       for(i=0;i<4;i++)
      #else
       for(i=0;i<5;i++)
      #endif
       {
         str[0] = commParaItem[1][i];
         str[1] = '\0';
         if (rowLight==1 && i==colLight)
         {
           guiDisplay(tmpX,33,str,0);
         }
         else
         {
           guiDisplay(tmpX,33,str,1);
         }
         tmpX += 8;
       }
       
       //APN����
       guiDisplay(1,  49, "APN",1);
       if (rowLight==2)
       {
         guiDisplay(32, 49, commParaItem[2], 0);
       }
       else
       {
         guiDisplay(32, 49, commParaItem[2], 1);
       }
     
       guiDisplay(1,65,"��վIP������˿�",1);     
       tmpX = 1;
       for(i=0;i<21;i++)
       {
         str[0] = commParaItem[3][i];
         str[1] = '\0';
         
         if (rowLight==3 && colLight==i)
         {
           guiDisplay(tmpX,81,str,0);
           tmpX += 8;
         }
         else
         {
           guiDisplay(tmpX,81,str,1);
           tmpX += 7;
         }
       }
  
       guiDisplay(1,97,"����IP������˿�",1);
       tmpX = 1;
       for(i=0;i<21;i++)
       {
         str[0] = commParaItem[4][i];
         str[1] = '\0';
         if (rowLight==4 && colLight==i)
         {
           guiDisplay(tmpX,113,str,0);
           tmpX += 8;
         }
         else
         {
           guiDisplay(tmpX,113,str,1);
           tmpX += 7;
         }
       }
       
       guiLine( 60, 133,  60, 152, 1);
       guiLine(100, 133, 100, 152, 1);
       guiLine( 60, 133, 100, 133, 1);
       guiLine( 60, 152, 100, 152, 1);       
       if (rowLight==5)
       {
         guiDisplay(64,135,"ȷ��",0);     
       }
       else
       {
         guiDisplay(64,135,"ȷ��",1);
       }
    }
	  lcdRefresh(17,160);
}

/*******************************************************
��������:chinese
��������:������Ϣ(����˵�3.2)
���ú���:
�����ú���:
�������:num
�������:
����ֵ:void
*******************************************************/
void chinese(INT8U num, INT8U numOfPage)
{
   char  display[35],str[5];
   INT8U ifTail,i,j;
   
   layer1MenuLight = 4;
   menuInLayer = 2;
   
   guiLine(1,17,160,160,0);

   messageTip(0);
   
   if (chnMessage.numOfMessage==0)
   {
    	guiDisplay(32,30,"��������Ϣ!",1);
  	  lcdRefresh(17,160);
    	return;
   }
   
   if ((chnMessage.message[num].typeAndNum>>4)==1)
   {
   	 strcpy(display,"��Ҫ");
   }
   else
   {
   	 strcpy(display,"��ͨ");
   }
   strcat(display,"��Ϣ ");
	 strcat(display,"ҳ");
	 strcat(display,intToString(numOfPage+1,3,str));
	 strcat(display," ");	 
	 strcat(display,intToString(num+1,3,str));
	 strcat(display,"/");
	 strcat(display,intToString(chnMessage.numOfMessage,3,str));
	 guiDisplay(10,LCD_LINE_1,display,1);
   guiLine(1,32,160,32,1);
   ifTail = 1;
   for(i=0;i<8;i++)
   {
      for(j=0;j<20;j++)
      {
      	 if (chnMessage.message[num].chn[numOfPage*160+j+i*20]=='\0')
      	 {
      	 	  ifTail=2;
      	 	  break;
      	 }
      	 else
      	 {
      	    display[j] = chnMessage.message[num].chn[numOfPage*160+j+i*20];
      	 }
      }
      if (j>0)
      {
         display[j] = '\0';
         guiDisplay(1,33+i*16,display,1);
      }
      
      if (ifTail==2)
      {
      	break;
      }
   }
	 lcdRefresh(17,160); 
}

/*******************************************************
��������:chargeInfo
��������:������Ϣ(����˵�2.6)
���ú���:
�����ú���:
�������:num(�ܼ������)
�������:
����ֵ:void
*******************************************************/
void chargeInfo(INT8U num)
{
 	  INT8U     pn;
 	  char      say[42],str1[15];
    INT32U    tmpKw;
    INT16U    tmpWatt;
    INT8U     dataBuff[512];
    DATE_TIME readTime;

 	  menuInLayer = 2;
	  guiLine(1,17,160,160,0);

	  if(totalAddGroup.numberOfzjz==0)
	  {
	  	 guiDisplay(12,LCD_LINE_3,"���ܼ������ò���!",1);
	  	 lcdRefresh(17,160);
	  	 return;
	  }
	  
	  pn = totalAddGroup.perZjz[num].zjzNo;
	  strcpy(say,"�ܼ���");
   	strcat(say,digital2ToString(pn,str1));
   	strcat(say,"������Ϣ");
   	guiDisplay(16,LCD_LINE_1,say,0);
   	
	  if (pn > 0 && pn <= 8 && (chargeCtrlConfig[pn-1].flag == 0x55 || chargeCtrlConfig[pn-1].flag == 0xAA))
	  {
   	   //���絥��
   	   strcpy(say,"���絥��:");
   	   strcat(say,intToString(chargeCtrlConfig[pn-1].numOfBill,3,str1));
   	   guiDisplay(1,LCD_LINE_3,say,1);
   	   
   	   //��������
   	   strcpy(say,"��������:");
	     tmpKw = chargeCtrlConfig[pn-1].alarmLimit;
    	 tmpKw = countAlarmLimit((INT8U *)&tmpKw, 0x3, 0x0,&tmpWatt);
    	 
    	 if (chargeCtrlConfig[pn-1].alarmLimit&0x10000000)
    	 {
    	 	 strcat(say, "-");
    	 }
    	 
 	     if (tmpKw>999999)
 	     {
 	        tmpKw/=1000;
 	        strcat(say,intToString(tmpKw,3,str1));
          strcat(say,"Mwh");
 	     }
 	     else
 	     {
 	        strcat(say,intToString(tmpKw,3,str1));
          strcat(say,"Kwh");
 	     }
   	   guiDisplay(1,LCD_LINE_4,say,1);
 	     
 	     
   	   //��բ����
   	   strcpy(say,"��բ����:");
	     tmpKw = chargeCtrlConfig[pn-1].cutDownLimit;
    	 tmpKw = countAlarmLimit((INT8U *)&tmpKw, 0x3, 0x0,&tmpWatt);
    	 if (chargeCtrlConfig[pn-1].cutDownLimit&0x10000000)
    	 {
    	 	 strcat(say, "-");
    	 }
 	     if (tmpKw>999999)
 	     {
 	        tmpKw/=1000;
 	        strcat(say,intToString(tmpKw,3,str1));
          strcat(say,"Mwh");
 	     }
 	     else
 	     {
 	        strcat(say,intToString(tmpKw,3,str1));
          strcat(say,"Kwh");
 	     }
   	   guiDisplay(1,LCD_LINE_5,say,1);
   	   
       strcpy(say,"ʣ�����:");
       readTime = timeHexToBcd(sysTime);
       if (readMeterData(dataBuff, pn, LEFT_POWER, 0x0, &readTime, 0)==TRUE)
       {
          if (dataBuff[0] != 0xFF || dataBuff[0] != 0xEE)
          {
                //��ǰʣ�����
                tmpKw = dataBuff[0] 
                      | dataBuff[1] << 8 
                      | dataBuff[2] << 16 
                      | dataBuff[3] << 24;
          }
    	    tmpKw = countAlarmLimit((INT8U *)&tmpKw, 0x3, 0x0,&tmpWatt);     	    
     	    if (dataBuff[3]&0x10)
    	    {
    	   	  strcat(say, "-");
    	    }
 	        strcat(say,intToString(tmpKw,3,str1));
          strcat(say,"Kwh");
 	     }
   	   guiDisplay(1,LCD_LINE_6,say,1);
   	}
   	else
   	{
   		 guiDisplay(1,LCD_LINE_3,"δ���ù���ز���!",1);
   	}
   	
   	lcdRefresh(17,160);
}

#endif    //PLUG_IN_CARRIER_MODULE

