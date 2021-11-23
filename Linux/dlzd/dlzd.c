/***************************************************
Copyright,2009,Huawei WoDian co.,LTD,All	Rights Reserved
�ļ���:dlzd.c
����:leiyong
�汾:0.9
�������:2009��10��
����:�����ն�(�����ն�/������,AT91SAM9260������)main����ļ�
�����б�:
  1.��ں���(main)
�޸���ʷ:
  01,09-10-10,Leiyong created.
  02,10-10-01,Leiyong���485��ɿ��ն��ᴦ��
  03,10-10-01,Leiyong�޸��չ���ʱ�䴦����,��Ϊÿ5���Ӹ���һ�ι���ʱ��
  04,10-11-24,Leiyong����-n����,��ǿ��ɾ���ز�ģ���ڱ��ַ��·��,Ϊ�˷�ֹ����ģ����Ӳ������ַ������
  05,14-09-26,Leiyong,����,ÿ������0��7������һ�ι�������

***************************************************/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>

#include "common.h"
#include "timeUser.h"
#include "lcdGui.h"
#include "convert.h"

#include "ioChannel.h"
#include "hardwareConfig.h"

#include "teRunPara.h"
#include "msSetPara.h"
#include "dataBase.h"
#include "copyMeter.h"
#include "wirelessModem.h"
#include "loadCtrl.h"
#include "msOutput.h"
#include "msInput.h"
#include "userInterface.h"
#include "wlModem.h"
#include "att7022b.h"
#include "reportTask.h"
#include "dataBalance.h"
#include "AFN01.h"
#include "AFN0F.h"

#ifdef PLUG_IN_CARRIER_MODULE
 #include "gdw376-2.h"
#endif

#include <sys/socket.h> 
#include <net/if.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>

extern INT32U avrHeartTime;                //��AVR��Ƭ��ͨ�ŵ�����ʱ��

BOOL          secondChanged=FALSE;         //���Ѹı�
INT8U         prevSecond;
DATE_TIME     powerOffTime;                //�ػ�ʱ��

extern void *threadOfUpRt(void *arg);

/***************************************************
��������:teMaintain
��������:ά���ն��߳�
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
***************************************************/
void teMaintain(void *arg)
{
  INT16U    ifWatchdog=0;
  INT8U     buf[20];
  INT16U    offset;
  INT32U    acVolA,acVolB,acVolC;
  ADC_PARA  adcPara;               //ֱ��ģ�������ò���(AFN04-FN81,FN82,FN83)
  INT8U     saveAdcData=0;
 
 #ifdef LIGHTING 
  struct cpAddrLink *tmpCpLink;
 #endif
  
  while(1)
  {
    //1.�õ�ϵͳʱ��
    getSystemDateTime(&sysTime);
     
    //2.���źŵĸı��ж�
    secondChanged = FALSE;
    if (prevSecond!=sysTime.second)
    {
     	secondChanged = TRUE;
    }
    prevSecond = sysTime.second;
     
    //3.����Ƿ�Ӧ�ø�λ
    detectReset(secondChanged);
     
    //2012-11-08,���ڽ���socketʱҪ��������,�������޷�Ӧ,Ϊ�˲�Ӱ�찴������
    //           ����������Ƶ��������д���
    //4.ά��������·
    //wlModem();
      
    //5.�û��ӿ�
    userInterface(secondChanged);
     
    //7.�ظ���վ(��ʱ֡�������ϱ�֡)
    replyToMs();
     
   #ifdef WDOG_USE_X_MEGA
    if (secondChanged==TRUE)
    {
      xMegaHeartBeat++;

      if (xMegaHeartBeat>5)
      {
      	xMegaHeartBeat = 0;
      	sendXmegaFrame(CPU_HEART_BEAT, buf, 0);
      	xMegaQueue.inTimeFrameSend = TRUE;
      }
    }
      
    sendToXmega();
   #endif
     
    //8.�����йصĴ���
    if (secondChanged==TRUE)
    {
      //���������Ǽ��
      //if (ioctl(fdOfIoChannel,READ_SWITCH_KH,0))
      //{
      //	 printf("��\n");
      //}
      //else
      //{
      //	 printf("��\n");
      //}
        
      if (sysTime.hour==0 && sysTime.minute==10 && sysTime.second == 0)
      {
        signalReport(0, wlRssi);
        if (wlModemFlag.loginSuccess == 1)
        {
         #ifdef PLUG_IN_CARRIER_MODULE
          #ifdef MENU_FOR_CQ_CANON
           switch(moduleType)
           {
             case GPRS_SIM300C:
             case GPRS_GR64:
             case GPRS_MC52I:
             case GPRS_M590E:
             case GPRS_M72D:
      	       guiDisplay(6, 1, "G", 1);
      	       break;
						 
             case LTE_AIR720H:
      	       guiDisplay(6, 1, "4", 1);
      	       break;
      	          
      	     case CDMA_DTGS800:
      	     case CDMA_CM180:
      	       guiDisplay(6, 1, "C", 1);
      	       break;
      	   }
           lcdRefresh(1,16);
          #else
           showInfo("��¼�ɹ�!");
          #endif
         #else    //��G����Ŀ�
          guiLine(20,1,20,15,1);
          guiLine(30,1,30,15,1);
          guiLine(20,1,30,1,1);
          guiLine(20,15,30,15,1);
          lcdRefresh(1,16);
         #endif
        }
      }
        
      if (sysTime.hour==23 && sysTime.minute==59 && sysTime.second == 55)
      {
        //��¼���ۼƹ���ʱ��
        dayPowerOnAmount();
      }
        
      //ÿ5���Ӹ���һ�ι���ʱ��
      if ((sysTime.minute%5)==0 && sysTime.second==0)
      {
        //��¼���ۼƹ���ʱ��
        dayPowerOnAmount();
      }
        
      workSecond++;

      if (workSecond==2)
      {
       #ifdef LOAD_CTRL_MODULE
        statusOfGate |= 0x40;  //���е���
       #endif
      
        //֪ͨЭ�������Ը�λ
       #ifdef WDOG_USE_X_MEGA
        sendXmegaFrame(ADVISE_RESET_XMEGA, buf, 0);
        xMegaQueue.inTimeFrameSend = TRUE;
         
        sendToXmega();
       #endif
      }
       
      if (workSecond==10)
      {
        //12-1.��������
        if (checkIfCascade()==2 || checkIfCascade()==1)
        {
         #ifdef  WDOG_USE_X_MEGA
          buf[0] = 0x02;                         //xMega�ϵ�485����ӿ�2
          buf[1] = cascadeCommPara.ctrlWord;     //�˿�����
          sendXmegaFrame(ADVISE_PORT_CASCADE, buf, 2);
             
          printf("֪ͨ485-2Ϊ�����˿ڲ�����������\n");
         #endif
           
          //�������ն�
          if (checkIfCascade()==2)
          {
            printf("����������Ӧ�ò�����\n");
     
            wlModemFlag.permitSendData = 1;   //������Ӧ�ò�����
          }
        }
      }

      //8.1 1����������
      if (initTasking==FALSE)
      {
        if (reportTask1.numOfTask>0)
        {
          activeReport1();
        }
      
        if (fQueue.activeSendPtr!=fQueue.activeTailPtr && fQueue.activeFrameSend==FALSE)
        {
          if (debugInfo&PRINT_ACTIVE_REPORT)
          {
            printf("���������ϱ�֡ activeSendPtr=%d,activeTailPtr=%d\n",fQueue.activeSendPtr,fQueue.activeTailPtr);
          }
         
          addFrameFlag(TRUE,TRUE);
          fQueue.activeFrameSend = TRUE;  //������ʱ������TCP����
        }
      }
       
      if (ioctl(fdOfIoChannel, DETECT_POWER_FAILURE, 0)==IO_LOW)     //ͣ��
      {
        if (ifPowerOff==FALSE)
        {
          powerOffEvent(FALSE);    //ͣ���¼�
             
          activeReport3();         //�����ϱ��¼�

          dayPowerOnAmount();      //��¼�չ���ʱ���ۼ�
             
          ifPowerOff = TRUE;
             
          //2013-11-22,����ɽ��������˾Ӫ����֪ͨ,����ع���ʱ��ĳ�70��
         #ifdef SDDL_CSM
          powerOffTime = nextTime(sysTime,0,70);
         #else
          powerOffTime = nextTime(sysTime,0,35);
         #endif
    	 	     
    	  	lcdBackLight(LCD_LIGHT_OFF);            //�رձ�����
             
          //����ʱ����Ϊ0
          guiLine(1,17,160,160,0);
          guiDisplay(48,64,"�ն�ͣ��",1);
          lcdRefresh(17,160);
        }
          
      	if (compareTwoTime(powerOffTime, sysTime)==TRUE)
        {
          powerOffTime = nextTime(sysTime,0,10);
             
          ioctl(fdOfIoChannel, SET_BATTERY_ON, IO_LOW);  //�Ͽ��󱸵��
          printf("�ѹػ�\n");
        }
      }
      else                                               //�е�
      {
    	  if (ifPowerOff==TRUE)
    	  {
    	    //#ifdef MENU_FOR_CQ_CANON
    	    defaultMenu();
    	    //#endif
            
          ioctl(fdOfIoChannel, SET_BATTERY_ON, IO_HIGH); //��ͨ�󱸵��
  
    	    ifPowerOff = FALSE;
    	    powerOffEvent(TRUE);                           //�ϵ��¼�
    	      
    	    activeReport3();                               //�����ϱ��¼�

          powerOnStatisTime = sysTime;
    	    alarmLedCtrl(ALARM_LED_OFF);
        }
      }
       
      //���ɿ��Ź�
      //if ((sysTime.minute%2)==0 && (sysTime.second==7))
      //ÿ��11��鿴һ�½����Ƿ�����,ly,2011-04-18
     #ifdef AC_SAMPLE_DEVICE
      if ((sysTime.second%11)==0)
      {
        //ת������ֵ
        acVolA = hexDivision((realAcData[8]>>acSamplePara.vAjustTimes),times2(13),1);
        acVolA = (acVolA>>12 & 0xf)*100 + (acVolA>>8 & 0xf)*10 + (acVolA>>4 & 0xf);
        acVolB = hexDivision((realAcData[9]>>acSamplePara.vAjustTimes),times2(13),1);
        acVolB = (acVolB>>12 & 0xf)*100 + (acVolB>>8 & 0xf)*10 + (acVolB>>4 & 0xf);
        acVolC = hexDivision((realAcData[10]>>acSamplePara.vAjustTimes),times2(13),1);
        acVolC = (acVolC>>12 & 0xf)*100 + (acVolC>>8 & 0xf)*10 + (acVolC>>4 & 0xf);
         
        if (debugInfo&PRINT_AC_SAMPLE)
        {
         	printf("%02d-%02d-%02d %02d:%02d:%02d:����ֵA���ѹ=%d,B���ѹ=%d,C���ѹ=%d\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, acVolA,acVolB,acVolC);
        }
         
        //�Ƚ�,���ĳ���ѹ��45V�Ժ�����10V����,û��У������������ΪУ���������ȷ,�����·�У�����
        if (((acVolA>10 && acVolA<45) || (acVolB>10 && acVolB<45) || (acVolC>10 && acVolC<45)) && (readCheckData!=0x55))
        {
          if (debugInfo&PRINT_AC_SAMPLE)
          {
         	  printf("ĳ���ѹ��45V�Ժ�����10V����,�����·�У�����\n");
         	}

       	  resetAtt7022b(TRUE);
       	}
      }
     #endif
       
     #ifdef PLUG_IN_CARRIER_MODULE
      if (sysTime.minute==0 && sysTime.second==0)
      {
        //��Ѹ��ÿСʱУ��
        if (carrierModuleType==FC_WIRELESS)
        {
          carrierFlagSet.setDateTime = 0;
            
          //ÿ������9��ͬ��������Ϣ
          if (sysTime.hour==21)
          {
            carrierFlagSet.synSlaveNode = 1;
          }
        }
      }
        
      //ÿ15���ӱ���һ��ֱ��ģ����ֵ
      if (sysTime.second==0 && workSecond>15)
      {
        if (selectViceParameter(0x04, 81, 1, (INT8U *)&adcPara, sizeof(ADC_PARA))==TRUE)
        {
          saveAdcData = 0;
          switch(adcPara.adcFreezeDensity)
          {
            case 1:    //15����
              if (sysTime.minute%15==0)
              {
          	    saveAdcData = 1;
          	  }
          	  break;
          	  
          	case 2:    //30����
              if (sysTime.minute%30==0)
              {
          	    saveAdcData = 1;
          	  }
          	  break;

          	case 3:    //60����
              if (sysTime.minute==0)
              {
          	    saveAdcData = 1;
          	  }
          	  break;

          	case 254:  //5����
              if (sysTime.minute%5==0)
              {
          	    saveAdcData = 1;
          	  }
          	  break;

          	case 255:  //1����
          	  saveAdcData = 1;
          	  break;
          }
          	
          if (saveAdcData==1)
          {
          	buf[0] = hexToBcd(adcData);
          	buf[1] = (hexToBcd(adcData)>>8&0xf) | 0xa0;
          	saveMeterData(1, 0, timeHexToBcd(sysTime), buf, DC_ANALOG, 0, 2);
            if (debugInfo&PRINT_DATA_BASE)
            {
          	  printf("%02d-%02d-%02d %02d:%02d:%02d����ֱ��ģ����\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
          	}
          }
        }
          
        dcAnalogStatis(adcData);
      }
     #endif
        
      //sqlite3���ݿ⿴�Ź�,ÿ���ӵ�23�뿴һ�����ݿ��Ƿ�����
      //ly,2012-04-25,add,��Ϊ���ֳ����ָ����豸�·������ɹ�,�����ٲ�ʧ��
      if (sysTime.second==23 && flagOfClearData!=0x55 && flagOfClearData!=0xaa)
      {
        sqlite3Watch();
      }
        
      //2014-09-26,add
      if (0==sysTime.hour && 7==sysTime.minute && flagOfClearData==0x00)
      {
        flagOfClearData = 0x99;
      }
       
      //Modem�������Ź�,2015-10-28
      if(31==sysTime.second && wlModemFlag.loginSuccess == TRUE)
      {
       	//2015-11-06,�޸�Ϊֻ���ڳ���30����δ������ʱ,�����Ź�������
       	if (compareTwoTime(nextTime(lastHeartBeat, 30, 0), sysTime)==TRUE)
       	{
       	 #ifdef RECORD_LOG
          logRun("Modem:heartBeat time out over over 30 minutes");
         #endif
          if (debugInfo&WIRELESS_DEBUG)
          {
            printf("Modem:heartBeat time out over over 30 minutes\n");
          }
           
          ifReset = TRUE;
       	}
      }
			
		 #ifdef LIGHTING
	    //2016-11-21,Add,�����նȴ���ֵ����
		  if (lcHeartBeat<300)
			{
				lcHeartBeat++;
				if (lcHeartBeat>=300)    //����5����û�յ��������ն�ֵ��
				{
    			//��·���Ƶ�
      	  tmpCpLink = xlcLink;
          while(tmpCpLink!=NULL)
          {
    			  if (tmpCpLink->lcDetectOnOff<=1)
    			  {
      			  tmpCpLink->lcOnOff = 0xfe;
      			  tmpCpLink->lcDetectOnOff = 0xfe;
      			  
      			  if (debugInfo&METER_DEBUG)
              {
      	        printf(
      	               "%02d-%02d-%02d %02d:%02d:%02d,��ʱ��δ�յ��������ն�ֵ,����·���Ƶ�%02x%02x%02x%02x%02x%02x��lcDetectOnOff״̬Ϊδ֪\n",
      	                sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, 
      	                  tmpCpLink->addr[5], tmpCpLink->addr[4], tmpCpLink->addr[3], tmpCpLink->addr[2], tmpCpLink->addr[1], tmpCpLink->addr[0]
      	              );
      	      }
      	    }
						
						tmpCpLink = tmpCpLink->next;
    			}
				}
			}
		   
			//2016-11-24,Add,���ơ���·���Ƶ�״̬��������,10���ӷ�һ��
			if (statusHeartBeat<600)
			{
				statusHeartBeat++;
			}
	   #endif

    }
     
    //���Ź�
   #ifdef PLUG_IN_CARRIER_MODULE
    ifWatchdog++;
    if (ifWatchdog==0x1)
    {
   	  ioctl(fdOfIoChannel,SET_WATCH_DOG,1);
    }
    if (ifWatchdog==0x2)
    {
   	  ifWatchdog = 0x0;
   	  ioctl(fdOfIoChannel,SET_WATCH_DOG,0);
   	}
   #endif

    usleep(1000);
  }
}

/**************************************************
��������:main
��������:�����ն�(�����նˡ�������,AT91SAM9260������)main����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��״̬
***************************************************/
int main(int argc,char *argv[])
{
  DATE_TIME     powerOffTime;        //�ػ�ʱ��
  INT8U         prevSecond;
  INT8U         i;
  INT32U        j,k;
  pthread_t     id;
  int           ret;
  int           kkk;
  INT8U         tmpData;  
  INT8U         threadPara;
  INT8U         threadParax[11];     //�̲߳���
  DATE_TIME     nowTime;  
  INT8U         buf[3];              //for test acSample ly,2010-01-27
  INT8U         port;
  pthread_attr_t     attr;
  struct sched_param param;
  int    newprio=20;

  struct sockaddr_in addr;
  int                socketOfSendUdp;

  debugInfo = 0;
  debugInfox = 0;
  if (argc>1)
  {
  	for(i=1;i<argc;i++)
  	{
  	 	if (argv[i][0]=='-' && (argv[i][1]=='h' || argv[i][1]=='H'))
  	 	{
        printf("Usage:dlzd [-h] [-a] [-b] [-c] [-d] [-e] [-l] [-m] [-s] [-u] [-w] [-x] [-ac]\n");
        printf("    -h  ��ʾ������Ϣ\n");
        printf("    -a  ��ʾ�������ϱ��йصĵ�����Ϣ\n");
        printf("    -b  ��ʾ������йصĵ�����Ϣ\n");
        printf("    -c  ��ʾ�뱾��ͨ�Ŷ˿�(�ز�/����)�йصĵ�����Ϣ\n");
        printf("    -d  ��ʾ�뱾�����ݿ��йصĵ�����Ϣ\n");
        printf("    -e  ��ʾ���¼��йصĵ�����Ϣ\n");
        printf("    -l  ��ʾ�븺�ɿ����йصĵ�����Ϣ\n");
        printf("    -n  ǿ��ɾ���ز�ģ������(���ַ��·����Ϣ)\n");
        printf("    -m  ��ʾ�볭���йصĵ�����Ϣ\n");
        printf("    -s  ��ʾ��ESAMоƬ�йصĵ�����Ϣ\n");
        printf("    -u  ��ʾ��Զ�������йصĵ�����Ϣ\n");
        printf("    -w  ��ʾ��Զ��ͨ��ģ���йصĵ�����Ϣ\n");
        printf("    -x  ��ʾ��Э�������йصĵ�����Ϣ\n");
        printf("    -ac ��ʾ�뽻�������йصĵ�����Ϣ\n");
        printf("    -p  ��ʾ������ɼ��йصĵ�����Ϣ\n");
        printf("    -6  ��ʾ485�ӿڵĳ���645֡\n");
        printf("    -v  ��ʾ�뽻��ʾֵ�йصĵ�����Ϣ\n");
        printf("    -3  У�������ݴӴ��ڷ���,������Ǵ�MODEM����\n");
        printf("    -r  ���̿��Ź���������������\n");
        printf("    -t  ��ʾ����̫��������صĵ�����Ϣ\n");
        
        exit(0);
  	 	}
  	 	
  	 	//�������ϱ��йصĵ�����Ϣ
  	 	if (argv[i][0]=='-' && (argv[i][1]=='a' || argv[i][1]=='A'))
  	 	{
  	 		 debugInfo |= PRINT_ACTIVE_REPORT;
  	 	}

  	 	//������йصĵ�����Ϣ
  	 	if (argv[i][0]=='-' && (argv[i][1]=='b' || argv[i][1]=='B'))
  	 	{
  	 		 debugInfo |= PRINT_BALANCE_DEBUG;
  	 	}

  	 	//���ز��йصĵ�����Ϣ
  	 	if (argv[i][0]=='-' && (argv[i][1]=='c' || argv[i][1]=='C'))
  	 	{
  	 		 debugInfo |= PRINT_CARRIER_DEBUG;
  	 	}
  	 	
  	 	//�����ݿ��йصĵ�����Ϣ
  	 	if (argv[i][0]=='-' && (argv[i][1]=='d' || argv[i][1]=='D'))
  	 	{
  	 		 debugInfo |= PRINT_DATA_BASE;
  	 	}

  	 	//���¼��йصĵ�����Ϣ
  	 	if (argv[i][0]=='-' && (argv[i][1]=='e' || argv[i][1]=='E'))
  	 	{
  	 		 debugInfo |= PRINT_EVENT_DEBUG;
  	 	}

  	 	//�븺�ɿ����йصĵ�����Ϣ
  	 	if (argv[i][0]=='-' && (argv[i][1]=='l' || argv[i][1]=='L'))
  	 	{
  	 		 debugInfo |= PRINT_LOAD_CTRL_DEBUG;
  	 	}

  	 	//�볭���йصĵ�����Ϣ
  	 	if (argv[i][0]=='-' && (argv[i][1]=='m' || argv[i][1]=='M'))
  	 	{
  	 		 debugInfo |= METER_DEBUG;
  	 	}

  	 	//��ESAMоƬ�йصĵ�����Ϣ
  	 	if (argv[i][0]=='-' && (argv[i][1]=='s' || argv[i][1]=='S'))
  	 	{
  	 		 debugInfo |= ESAM_DEBUG;
  	 	}

  	 	//��Զ�������йصĵ�����Ϣ
  	 	if (argv[i][0]=='-' && (argv[i][1]=='u' || argv[i][1]=='U'))
  	 	{
  	 		 debugInfo |= PRINT_UPGRADE_DEBUG;
  	 	}

  	 	//������Modem�йصĵ�����Ϣ
  	 	if (argv[i][0]=='-' && (argv[i][1]=='w' || argv[i][1]=='W'))
  	 	{
  	 		 debugInfo |= WIRELESS_DEBUG;
  	 	}

  	 	//��xMega�йصĵ�����Ϣ
  	 	if (argv[i][0]=='-' && (argv[i][1]=='x' || argv[i][1]=='X'))
  	 	{
  	 		 debugInfo |= PRINT_XMEGA_DEBUG;
  	 	}

  	 	//�뽻���йصĵ�����Ϣ
  	 	if (argv[i][0]=='-' && ((argv[i][1]=='a' && argv[i][2]=='c') || (argv[i][1]=='A' && argv[i][2]=='C')))
  	 	{
  	 		 debugInfo |= PRINT_AC_SAMPLE;
  	 	}

  	 	//������ɼ��йصĵ�����Ϣ
  	 	if (argv[i][0]=='-' && (argv[i][1]=='p' || argv[i][1]=='P'))
  	 	{
  	 		 debugInfo |= PRINT_PULSE_DEBUG;
  	 	}

  	 	//����645֡
  	 	if (argv[i][0]=='-' && argv[i][1]=='6')
  	 	{
  	 		debugInfo |= PRINT_645_FRAME;
        
       #ifdef SUPPORT_ETH_COPY
  	 		monitorMp = 0;
  	 		if (argv[i][2]=='m' || argv[i][2]=='M')
  	 	  {
  	 	    switch (strlen(argv[i]))
  	 	    {
  	 	    	case 4:
  	 	    		monitorMp = argv[i][3]-0x30;
  	 	    		break;

  	 	    	case 5:
  	 	    		monitorMp = (argv[i][3]-0x30)*10 + argv[i][4]-0x30;
  	 	    		break;

  	 	    	case 6:
  	 	    		monitorMp = (argv[i][3]-0x30)*100 + (argv[i][4]-0x30)*10 + argv[i][5]-0x30;
  	 	    		break;

  	 	    	case 7:
  	 	    		monitorMp = (argv[i][3]-0x30)*1000 + (argv[i][4]-0x30)*100 + (argv[i][5]-0x30)*10 + argv[i][6]-0x30;
  	 	    		break;
  	 	    }
  	 	  }
  	 	 #endif
  	 	}

  	 	//����ʾֵ���
  	 	if (argv[i][0]=='-' && argv[i][1]=='v')
  	 	{
  	 		debugInfo |= PRINT_AC_VISION;
  	 	}
  	 	
  	 #ifdef PLUG_IN_CARRIER_MODULE
  	 	//ǿ��ɾ���ز�ģ������(���ַ��·����Ϣ)
  	 	if (argv[i][0]=='-' && (argv[i][1]=='n' || argv[i][1]=='N'))
  	 	{
  	 		debugInfo |= DELETE_LOCAL_MODULE;
  	 		carrierFlagSet.forceClearData = 1;
  	 	}
  	 #endif

  	 	//����ʾֵ���
  	 	if (argv[i][0]=='-' && argv[i][1]=='3')
  	 	{
  	 		debugInfox |= 0x01;
  	 	}

  	 	//���̿��Ź�������Ӧ�ó���
  	 	if (argv[i][0]=='-' && (argv[i][1]=='r' || argv[i][1]=='R'))
  	 	{
  	 		debugInfox |= WATCH_RESTART;
  	 	}

  	 	//��̫��������ص�����Ϣ
  	 	if (argv[i][0]=='-' && (argv[i][1]=='t' || argv[i][1]=='T'))
  	 	{
  	 		debugInfox |= ETH_COPY_METER;
  	 	}
  	}
  }
  
  printf("�ն�����...\n");
  
  //debugInfo |= PRINT_CARRIER_DEBUG;
  //debugInfo |= METER_DEBUG;
  //debugInfo |= PRINT_PULSE_DEBUG;        //ly,2011-07-29
  //debugInfo |= PRINT_LOAD_CTRL_DEBUG;    //ly,2011-07-29
  //debugInfo |= PRINT_BALANCE_DEBUG;      //ly,2011-07-29
  //debugInfo |= PRINT_DATA_BASE;          //ly,2011-08-15
  //debugInfo |= PRINT_UPGRADE_DEBUG;      //ly,2011-08-15

  //0.ȡ��ϵͳʱ��,��ʼ��ͳ��ʱ��
  getSystemDateTime(&sysTime);
  powerOnStatisTime = backTime(sysTime, 0, 0, 0, 0, 15); //�ն��ϵ�ʱ��(����ϵͳ����Ҫ15��20������)
  lcdLightDelay = nextTime(sysTime, 0, 30);              //LCD������ʱ

  //1.��ʼ�����ݿ�
  initDataBase();

  //2.�������
  loadParameter();
  
  //3.��ʼ��Ӳ��(һ��������������ʼ������,2012-11-08)
  initHardware();
  
  if (debugInfo&ESAM_DEBUG)
  {
  	ioctl(fdOfIoChannel, ESAM_DEBUG_INFO, 1);
  }
  else
  {
  	ioctl(fdOfIoChannel, ESAM_DEBUG_INFO, 0);
  }

  //3-1-2.����Modem��λ����
  wlModemFlag.numOfWlReset  = 0;
  wlModemFlag.useMainIp     = 1;
  wlModemFlag.lastIpSuccess = 0;
  wlModemFlag.ethRecvThread = 0;
  wlModemFlag.waitModemRet  = nextTime(sysTime, WAIT_MODEM_MINUTES, 0);
  wlModemFlag.numOfRetryTimes = 0;    //2013-05-10,Add

  //3-2.����Modem�ϵ�
  if (bakModuleType==MODEM_PPP)  //2012-11-08,��Ӵ��ж�
  {
  	waitTimeOut = nextTime(sysTime, 3, 0);
  	
  	system("/bin/ppp-off");    //�ر�ppp0
  }
  else
  {
    wlModemPowerOnOff(0);
  }

  //4.��ʼ��LCD
  lcdInit();

  if ((debugInfox&WATCH_RESTART)==0x00)
  {
   #ifdef PLUG_IN_CARRIER_MODULE
    guiStart(8,50,1);
   #else
    guiStart(32,50,1);
   #endif
  }
  
  //6.��ʼ�����Ͷ���
  initSendQueue();

  #ifdef PULSE_GATHER
    fillPulseVar(1);
  #endif
  
	//7.��λATT7022B	
	//ly,2010-10-28,���ڿ�½����̨���Է���,�������ʱֻ��λһ�εĻ�,att7022b���ܸ�λ��̫����,��������θ�λ������У��ֵ
 #ifdef AC_SAMPLE_DEVICE	
	resetAtt7022b(TRUE);
	resetAtt7022b(TRUE);
 #endif

  //8.��ʼ��������Ϣ����
	initReportFlag = 1;
  
  
  //9.�����߳�
  //9-1.������485���߳�
  threadPara = 0x00;
  ret=pthread_create(&id,NULL,copyMeter,&threadPara);
  if(ret!=0)
  {
    printf ("CreatecopyMeter pthread error!\n");
    return 1;
  }

 #ifndef JZQ_CTRL_BOARD_V_1_4
  #ifndef TE_CTRL_BOARD_V_1_3
   //9-2.����485�ӿ�1�����߳�
   threadPara = 0x01;
   ret=pthread_create(&id,NULL,threadOf485x1Received,&threadPara);
   if(ret!=0)
   {
     printf ("Create copy meter receive(port 1) pthread error!\n");
     return 1;
   }

   //9-3.����485�ӿ�2�����߳�
   threadPara = 0x02;
   ret=pthread_create(&id,NULL,threadOf485x2Received,&threadPara);
   if(ret!=0)
   {
     printf ("Create copy meter receive(port 2) pthread error!\n");
     return 1;
   }

  #endif
 #endif
  
  //9-4.�������ؽӿ�����վͨ�Ž����߳�
  threadPara = 0x03;
  ret=pthread_create(&id,NULL,threadOfmsLocalReceive,&threadPara);
  if(ret!=0)
  {
    printf ("Create ms local receive pthread error!\n");
    return 1;
  }

  //9-5.��������Modem���ڽ����߳�
  threadPara = 0x04;
  ret=pthread_create(&id, NULL, threadOfTtys0Receive, &threadPara);
  if(ret!=0)
  {
    printf ("Create pthread wireless receive error!\n");
    return 1;
  }

 #ifdef PLUG_IN_CARRIER_MODULE 
  //9-6.�����ز����ݽ����߳�
  threadPara= 0x05;
  ret=pthread_create(&id, NULL, threadOfCarrierReceive, &threadPara);
  if(ret!=0)
  {
    printf ("Create pthread carrier receive error!\n");
    return 1;
  }
 #endif
  
  //9-7.�������������߳�
  //ly,2011-10-14,����վҪ���������/�����ĵ�������߳���,���ö�̬�����߳�
  flagOfClearData = 0x99;
  pthread_attr_init(&attr);
  pthread_attr_getschedparam(&attr, &param);
  param.sched_priority=1;
  pthread_attr_setschedparam(&attr, &param);
  threadPara= 0x07;
  ret=pthread_create(&id, &attr, threadOfClearData, &threadPara);
  if(ret!=0)
  {
    printf ("Create pthread clear data error!\n");
    return 1;
  }

  //9-8.������⿪�����������߳�
  pthread_attr_init(&attr);
  pthread_attr_getschedparam(&attr, &param);
  param.sched_priority = 88;    //����40,Ŀǰ���,2010-05-27,ly
  pthread_attr_setschedparam(&attr, &param);
  threadPara= 0x08;
  ret=pthread_create(&id,&attr,detectSwitchPulse,&threadPara);
  if(ret!=0)
  {
     printf ("Create pthread detectSwitchPulse error!\n");
     return 1;
  }

  //9-9.����ά���ն��߳�
  ret=pthread_create(&id, NULL, teMaintain, &threadPara);
  if(ret!=0)
  {
    printf ("Create pthread teMaintain error!\n");
    return 1;
  }
  
  //9-10.���ݽ����߳�
  //ly,2011-10-14,�ĵ�����,���ö�̬�����߳�  
  ret=pthread_create(&id, NULL, dataProcRealPoint, &threadParax);
  if(ret!=0)
  {
    printf("���������߳�ʧ��!\n");
    
    return 1;
  }
  
  //9-11.������������
  //ly,2011-10-15,�ĵ�����,���ö�̬�����߳�  
  ret=pthread_create(&id,NULL,threadOfReport2,&threadParax);
  if(ret!=0)
  {
    printf("�����������������߳�ʧ��!\n");
    
    return 1;
  }
  
  //9-12.ר��III�ʹ��������߳�
 #ifndef PLUG_IN_CARRIER_MODULE
  ret=pthread_create(&id, NULL, threadOfCtrl, &threadPara);
  if(ret!=0)
  {
    printf("Create pthread threadOfCtrl error!\n");
    return 1;
  }

  //����·Ϊ��բ״̬
  for(i=1; i<CONTROL_OUTPUT+1; i++)
  {
    gateJumpTime[i] = 0x4;
  }
 #endif



  addr.sin_family = AF_INET;
  addr.sin_port = htons(65431);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  if (addr.sin_addr.s_addr == INADDR_NONE)
  {
    printf("Incorrect ip address!");
    close(socketOfUdp);
  }
  
  if ((socketOfUdp = socket(AF_INET, SOCK_DGRAM, 0))<0)
  {
    perror("socket");
  }

  if (bind(socketOfUdp, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("bind");
  }

  //9-13.����UDP�����߳�
  if(pthread_create(&id, NULL, threadOfRecvUdpData, &threadPara)!=0)
  {
    printf ("Create pthread threadOfRecvUdpData error!\n");
    return 1;
  }



 #ifdef SUPPORT_ETH_COPY
  //9-14.������̫����������߳�
  threadPara= 0x14;
  ret=pthread_create(&id, NULL, threadOfEthCopyServer, &threadPara);
  if(ret!=0)
  {
    printf ("Create pthread threadOfEthCopyServer error!\n");
    
    return 1;
  }

  //9-15.������ȡ��̫������TCP�����߳�
  threadPara = 0x15;
  if(pthread_create(&id, NULL, threadOfEthCopyDataRead, &threadPara)!=0)
  {
    printf("Create pthread threadOfTcpDataRecv error!\n");
  }

  //9-16.������̫�������߳�
  threadPara= 0x16;
  ret=pthread_create(&id, NULL, threadOfEthCopyMeter, &threadPara);
  if(ret!=0)
  {
    printf ("Create pthread threadOfEthCopyMeter error!\n");
    
    return 1;
  }
 #endif



  //10.��¼���ϵ缰��λ����
  if (powerOnOffRecord.powerOnOrOff==0x11 || powerOnOffRecord.powerOnOrOff==0x22)
  {
   	if (powerOnOffRecord.powerOnOrOff==0x22)    //��ͣ��ʱ���¼,��¼ϵͳ�ϵ��¼���¼
   	{
      powerOffEvent(TRUE);
       
      activeReport3();    //�����ϱ��¼�
   	}
  }
  else    //���û��ͣ/�ϵ�ʱ���¼,��¼��ǰʱ��Ϊ�ϵ�ʱ��
  {
   	powerOnOffRecord.powerOnOrOff = 0x11;
   	powerOnOffRecord.powerOnOffTime = sysTime;

    //�ն˵Ĳ�����¼ΪAFN=0x66
    saveParameter(88, 1,(INT8U *)&powerOnOffRecord,sizeof(POWER_ON_OFF));
  }  
  teResetReport();    //��¼�ն˸�λ����
  
  logRun("terminal starting...");

  //11.����������Ƿ�����̫����¼��վ
 #ifdef PLUG_IN_CARRIER_MODULE 
  if (teIpAndPort.ethIfLoginMs==0x55)
  {
   #ifdef JZQ_CTRL_BOARD_V_1_4
   	gatherModuleType = 2;
   #endif

   #ifdef TE_CTRL_BOARD_V_1_3
   	gatherModuleType = 2;
   #endif
   	 
    moduleType = ETHERNET;
   	 
   	operateModem = 1;    //2012-11-8
   	 
   	bakModuleType = 0;   //2013-04-28
  }
 #endif
  
  //12.��������
  //12-1.�������ն�
  if (checkIfCascade()==2)
  {
  	printf("���ն�Ϊ�������ն�\n");

	 #ifdef JZQ_CTRL_BOARD_V_1_4
	  gatherModuleType = 2;
	 #endif

	 #ifdef TE_CTRL_BOARD_V_1_3
	  gatherModuleType = 2;
	 #endif
	 
	  moduleType = CASCADE_TE;
  }
  
  //13.������ʾ
  if (debugInfox&WATCH_RESTART)
  {
  	defaultMenu();
  }
  else
  {
    startDisplay();
  }

  //14.�����̴���
  while(1)
  {
    if (initReportFlag==1)
    {
     	initReportFlag = 2;

     	initReportTask(NULL);
    }
     
   #ifdef WDOG_USE_X_MEGA
    if (gatherModuleType==0)
    {
      sendXmegaFrame(GATHER_IO_DATA, &i, 0);
      sleep(2);
    }

    if (gatherModuleType==1)
    {
      gatherModuleType=2;

      if (bakModuleType!=MODEM_PPP)    //2012-11-14,add
      {
        if (moduleType==GPRS_MC52I)
        {
          uart0Config(2);  //����������Ϊ57600bps
          printf("����Ϊ57600\n");
        }
        else
        {
          uart0Config(0);  //����������Ϊ115200bps
          //printf("����Ϊ115200");
        }
      	 
        if (moduleType==GPRS_MC52I)
        {
          resetWlModem(1);
        }
      }
         
      //��̫��
      if (moduleType==ETHERNET)
      {
				fdOfModem=NULL;    //2017-8-23,add

	    	//������ģ���Դ
      	wlModemPowerOnOff(1);
      }
    }
   #endif
     
    //2012-11-7,modify,ͨ��socket�ĳɷ�����ʽ��,�Ͳ���ÿ�ζ������߳���
    //if (wlModemFlag.ethRecvThread==0 && moduleType==ETHERNET && wlModemFlag.permitSendData==1)
    //{
    //	 printf("������̫�������߳�\n");
    	 
    //	 wlModemFlag.ethRecvThread = 1;
     	 
    //  threadPara= 0x04;
    //  ret=pthread_create(&id,NULL,threadOfTtys0Receive,&threadPara);
    //  if(ret!=0)
    //  {
    //    printf ("Create pthread wireless receive error!\n");
    //    return 1;
    //  }
    //}
    
   #ifdef PLUG_IN_CARRIER_MODULE
    for(port=0;port<4;port++)
   #else
    for(port=0;port<NUM_OF_COPY_METER;port++)
   #endif
    {
      if (((sysTime.second>2 && sysTime.second<10) || (sysTime.second>=15 && sysTime.second<17)) && (copyCtrl[port].ifRealBalance==0) && (copyCtrl[port].thisMinuteProcess==FALSE))
      {
       	copyCtrl[port].thisMinuteProcess = TRUE;
       	
       	if (debugInfo&PRINT_BALANCE_DEBUG)
       	{
       	  printf("%02d-%02d-%02d %02d:%02d:%02d,������:����˿�%dԽ�޳���ʱ�䵽�ڿ�ʼ\n", sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,port+1);
       	}
        
        processOverLimit(port);
        
        activeReport3();  //�����ϱ��¼�

       	if (debugInfo&PRINT_BALANCE_DEBUG)
       	{
       	  printf("%02d-%02d-%02d %02d:%02d:%02d,������:����˿�%dԽ�޳���ʱ�䵽�ڽ���\n",
       	   sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,port+1);
       	}
      }
       
      if (((sysTime.second>=10 && sysTime.second<15) || (sysTime.second>=17 && sysTime.second<20))  && (copyCtrl[port].thisMinuteProcess==TRUE))
      {
      	 copyCtrl[port].thisMinuteProcess=FALSE;
      }
    }
     
   #ifdef PLUG_IN_CARRIER_MODULE
    if (upRtFlag==1)
    {
     	upRtFlag = 2;
     	 
      threadPara= 0x00;
      ret=pthread_create(&id,NULL,threadOfUpRt,&threadPara);
      if(ret!=0)
      {
        printf ("Create pthread update router error!\n");
        return 1;
      }
    }
   #endif
     
    //2012-11-08,���ڽ���socketʱҪ��������,�������޷�Ӧ,Ϊ�˲�Ӱ�찴������
    //           ����������Ƶ��������д���
    //4.ά��������·
    wlModem();

    usleep(1000);
  }
  
  close(fdOfModem);
  close(fdOfttyS2);
  close(fdOfttyS3);
  close(fpOfUpgrade);
  printf("�ն˳����˳�\n");
  
  //�ر����ݿ�
  sqlite3_close(sqlite3Db);
}

