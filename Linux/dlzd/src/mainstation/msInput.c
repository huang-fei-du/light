/***************************************************
Copyright,2009,Huawei WoDian co.,LTD
�ļ�����msInput.c
���ߣ�wan guihua
�汾��0.1
������ڣ� �� ��
��������վ����֡�ļ���
�����б�
�޸���ʷ��
  01,09-12-10,Leiyong created.
  02,10-03-20,Leiyong�޸�,frame.loadLen���������/��������Ļ���Ӧ��ȥ����16�ֽڶ�����2�ֽ�,Ӧ��USE_16BYTES_PW���
  03,10-08-21,Leiyong����,������Ͷ�˺��ձ���ά����
  04,10-10-02,Leiyong�޸Ĵ���,xMega������ѭ������
              ����ԭ��:���ճ��ȿ����õ�i,��У���Ҳ�õ�i,��ʱ��������ѭ��
***************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "teRunPara.h"
#include "msSetPara.h"
#include "hardwareConfig.h"
#include "dataBase.h"
#include "statistics.h"

#include "wlModem.h"
#include "ioChannel.h"

#include "msInput.h"
#include "copyMeter.h"

extern INT8U  wlRssi;                             //����Modem�ź�

//��������
INT8U         frameProcess;                       //���鴦���б�־
INT16U        crcKey;                             //crc������Կ
INT8U         keyMode;                            //������֤ģʽ

//INT8U      tmp04Data[512];

/*******************************************************
��������:msInput
��������:��վ֡���봦��
���ú���:
�����ú���:
�������:
�������:
����ֵ��ʧ�� 1    �ɹ�  0
*******************************************************/
INT8U msInput(INT8U *pFrame,INT16U dataLength,INT8U dataFrom)
{
    DATE_TIME tpTime;
    INT16S    totalDelayMinute;
    INT16U    tmpCrc;
    INT8U     checkSum;
    INT16U    i;
 	  
    INT32U    tmpData;
    INT8U     tmpAuth;
    INT8U     macCheckData[2048], retMac[20];
    INT16U    lenOfMacCheck;
    INT8U     cmdBuf[20];
    INT8U     eventData[30];
    
    if (debugInfo&WIRELESS_DEBUG)
    {
      printf("msInput:");
    }

   #ifdef WDOG_USE_X_MEGA
    INT8U buf[1024];
    
    if (dataFrom==DATA_FROM_LOCAL && teInRunning==1)
    {
    	 return;
    }
   #endif

   //ly,2011-05-14,������վ��������֡
   /*
   if (pFrame[12]==0x04)
   {
     //������վ֡,����FN2,3,4,7,8,16,36,FN���ص�(191Bytes)
     //68 DE 02 DE 02 68 4A 29 50 72 27 C8 
     //04 
     //74 
     //00 00 CE 00  DA,DT
     //81 34 12 (FN2������)
     //AC 14 28 14 0F 27 AC 14 28 15 A0 26 64 7A 64 6C 2E 63 71 20 20 20 20 20 20 00 00 00 (FN3������)
     //15 96 66 11 11 2F FF FF 15 96 66 11 11 1F FF FF (FN4������)
     //AC 14 28 14 AC 14 28 14 AC 14 28 15 00 00 00 00 00 00 00 00 00 00 0F 27 (FN7������)
     //01 02 00 02 02 7D BF DE (FN8������)
     
     //00 00 80 01 (FN16 DA,DT)
     //64 77 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 31 32 33 34 35 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 (FN16������)
     
     //00 00 08 04 (FN36 DA,DT)
     //00 48 13 00 (F36 ������)
     
     //12 34 56 00 00 00 00 00 00 00 00 00 00 00 00 00 (PW)
     //F0 16

     tmp04Data[0] = 0x68;
     tmp04Data[1] = 0xDE;
     tmp04Data[2] = 0x02;
     tmp04Data[3] = 0xDE;
     tmp04Data[4] = 0x02;
     tmp04Data[5] = 0x68;
     tmp04Data[6] = 0x4A;
     tmp04Data[7] = 0x29;
     tmp04Data[8] = 0x50;
     tmp04Data[9] = 0x72;
     tmp04Data[10] = 0x27;
     tmp04Data[11] = 0xC8;
     tmp04Data[12] = 0x04;
     tmp04Data[13] = 0x74;
     tmp04Data[14] = 0x00;
     tmp04Data[15] = 0x00;
     tmp04Data[16] = 0xCE;
     tmp04Data[17] = 0x00;
     tmp04Data[18] = 0x81;
     tmp04Data[19] = 0x34;
     tmp04Data[20] = 0x12;
     tmp04Data[21] = 0xAC;
     tmp04Data[22] = 0x14;
     tmp04Data[23] = 0x28;
     tmp04Data[24] = 0x14;
     tmp04Data[25] = 0x0F;
     tmp04Data[26] = 0x27;
     tmp04Data[27] = 0xAC;
     tmp04Data[28] = 0x14;
     tmp04Data[29] = 0x28;
     tmp04Data[30] = 0x15;
     tmp04Data[31] = 0xA0;
     tmp04Data[32] = 0x26;
     tmp04Data[33] = 0x64;
     tmp04Data[34] = 0x7A;
     tmp04Data[35] = 0x64;
     tmp04Data[36] = 0x6C;
     tmp04Data[37] = 0x2E;
     tmp04Data[38] = 0x63;
     tmp04Data[39] = 0x71;
     tmp04Data[40] = 0x20;
     tmp04Data[41] = 0x20;
     tmp04Data[42] = 0x20;
     tmp04Data[43] = 0x20;
     tmp04Data[44] = 0x20;
     tmp04Data[45] = 0x20; 
     tmp04Data[46] = 0x00;
     tmp04Data[47] = 0x00;
     tmp04Data[48] = 0x00;
     tmp04Data[49] = 0x15;
     tmp04Data[50] = 0x96;
     tmp04Data[51] = 0x66;
     tmp04Data[52] = 0x11;
     tmp04Data[53] = 0x11;
     tmp04Data[54] = 0x2F;
     tmp04Data[55] = 0xFF;
     tmp04Data[56] = 0xFF;
     tmp04Data[57] = 0x15;
     tmp04Data[58] = 0x96;
     tmp04Data[59] = 0x66;
     tmp04Data[60] = 0x11;
     tmp04Data[61] = 0x11;
     tmp04Data[62] = 0x1F;
     tmp04Data[63] = 0xFF;
     tmp04Data[64] = 0xFF;
     tmp04Data[65] = 0xAC;
     tmp04Data[66] = 0x14;
     tmp04Data[67] = 0x28;
     tmp04Data[68] = 0x14;
     tmp04Data[69] = 0xAC;
     tmp04Data[70] = 0x14;
     tmp04Data[71] = 0x28;
     tmp04Data[72] = 0x14;
     tmp04Data[73] = 0xAC;
     tmp04Data[74] = 0x14;
     tmp04Data[75] = 0x28;
     tmp04Data[76] = 0x15;
     tmp04Data[77] = 0x00;
     tmp04Data[78] = 0x00;
     tmp04Data[79] = 0x00;
     tmp04Data[80] = 0x00;
     tmp04Data[81] = 0x00;
     tmp04Data[82] = 0x00;
     tmp04Data[83] = 0x00;
     tmp04Data[84] = 0x00;
     tmp04Data[85] = 0x00;
     tmp04Data[86] = 0x00;
     tmp04Data[87] = 0x0F;
     tmp04Data[88] = 0x27;
     tmp04Data[89] = 0x01;
     tmp04Data[90] = 0x02;
     tmp04Data[91] = 0x00;
     tmp04Data[92] = 0x02;
     tmp04Data[93] = 0x02;
     tmp04Data[94] = 0x7D;
     tmp04Data[95] = 0xBF;
     tmp04Data[96] = 0xDE;
     tmp04Data[97] = 0x00;
     tmp04Data[98] = 0x00;
     tmp04Data[99] = 0x80;
     tmp04Data[100] = 0x01;
     tmp04Data[101] = 0x64;
     tmp04Data[102] = 0x77;
     tmp04Data[103] = 0x00;
     tmp04Data[104] = 0x00;
     tmp04Data[105] = 0x00;
     tmp04Data[106] = 0x00;
     tmp04Data[107] = 0x00;
     tmp04Data[108] = 0x00;
     tmp04Data[109] = 0x00;
     tmp04Data[110] = 0x00;
     tmp04Data[111] = 0x00;
     tmp04Data[112] = 0x00;
     tmp04Data[113] = 0x00;
     tmp04Data[114] = 0x00;
     tmp04Data[115] = 0x00;
     tmp04Data[116] = 0x00;
     tmp04Data[117] = 0x00;
     tmp04Data[118] = 0x00;
     tmp04Data[119] = 0x00;
     tmp04Data[120] = 0x00;
     tmp04Data[121] = 0x00;
     tmp04Data[122] = 0x00;
     tmp04Data[123] = 0x00;
     tmp04Data[124] = 0x00;
     tmp04Data[125] = 0x00;
     tmp04Data[126] = 0x00;
     tmp04Data[127] = 0x00;
     tmp04Data[128] = 0x00;
     tmp04Data[129] = 0x00;
     tmp04Data[130] = 0x00;
     tmp04Data[131] = 0x00;
     tmp04Data[132] = 0x00;
     tmp04Data[133] = 0x31;
     tmp04Data[134] = 0x32;
     tmp04Data[135] = 0x33;
     tmp04Data[136] = 0x34;
     tmp04Data[137] = 0x35;
     tmp04Data[138] = 0x00;
     tmp04Data[139] = 0x00;
     tmp04Data[140] = 0x00;
     tmp04Data[141] = 0x00;
     tmp04Data[142] = 0x00;
     tmp04Data[143] = 0x00;
     tmp04Data[144] = 0x00;
     tmp04Data[145] = 0x00;
     tmp04Data[146] = 0x00;
     tmp04Data[147] = 0x00;
     tmp04Data[148] = 0x00;
     tmp04Data[149] = 0x00;
     tmp04Data[150] = 0x00;
     tmp04Data[151] = 0x00;
     tmp04Data[152] = 0x00;
     tmp04Data[153] = 0x00;
     tmp04Data[154] = 0x00;
     tmp04Data[155] = 0x00;
     tmp04Data[156] = 0x00;
     tmp04Data[157] = 0x00;
     tmp04Data[158] = 0x00;
     tmp04Data[159] = 0x00;
     tmp04Data[160] = 0x00;
     tmp04Data[161] = 0x00;
     tmp04Data[162] = 0x00;
     tmp04Data[163] = 0x00;
     tmp04Data[164] = 0x00;
     tmp04Data[165] = 0x00;
     tmp04Data[166] = 0x00;
     tmp04Data[167] = 0x08;
     tmp04Data[168] = 0x04;
     tmp04Data[169] = 0x00;
     tmp04Data[170] = 0x48;
     tmp04Data[171] = 0x13;
     tmp04Data[172] = 0x00;
     tmp04Data[173] = 0x12;
     tmp04Data[174] = 0x34;
     tmp04Data[175] = 0x56;
     tmp04Data[176] = 0x00;
     tmp04Data[177] = 0x00;
     tmp04Data[178] = 0x00;
     tmp04Data[179] = 0x00;
     tmp04Data[180] = 0x00;
     tmp04Data[181] = 0x00;
     tmp04Data[182] = 0x00;
     tmp04Data[183] = 0x00;
     tmp04Data[184] = 0x00;
     tmp04Data[185] = 0x00;
     tmp04Data[186] = 0x00;
     tmp04Data[187] = 0x00;
     tmp04Data[188] = 0x00;
     tmp04Data[189] = 0xF0;
     tmp04Data[190] = 0x16; 

     pFrame = tmp04Data;
     dataLength = 191;
   }
   */
   
   //ly,2014-10-16,������վ�·���������֡,����ǰ�б䶯
   /*
   //68 B6 00 B6 00 68 4A 00 50 01 00 4A 04 75 
   //04 01 02 08 
   //41 
   //00 20 10 15 70 14 
   //04 
   //02 
   //02 01 FF 0A 
   //02 01 3F 0B 
   //00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 D6 16 
   tmp04Data[0] = 0x68;
   tmp04Data[1] = 0xB6;
   tmp04Data[2] = 0x00;
   tmp04Data[3] = 0xB6;
   tmp04Data[4] = 0x00;
   tmp04Data[5] = 0x68;
   tmp04Data[6] = 0x4A;
   tmp04Data[7] = 0x00;
   tmp04Data[8] = 0x50;
   tmp04Data[9] = 0x01;
   tmp04Data[10] = 0x00;
   tmp04Data[11] = 0x4A;
   tmp04Data[12] = 0x04;
   tmp04Data[13] = 0x75;
   tmp04Data[14] = 0x04;
   tmp04Data[15] = 0x01;
   tmp04Data[16] = 0x02;
   tmp04Data[17] = 0x08;
   tmp04Data[18] = 0x41;
   tmp04Data[19] = 0x00;
   tmp04Data[20] = 0x20;
   tmp04Data[21] = 0x10;
   tmp04Data[22] = 0x15;
   tmp04Data[23] = 0x70;
   tmp04Data[24] = 0x14;
   tmp04Data[25] = 0x04;
   tmp04Data[26] = 0x02;
   tmp04Data[27] = 0x02;
   tmp04Data[28] = 0x01;
   tmp04Data[29] = 0xFF;
   tmp04Data[30] = 0x0A;
   tmp04Data[31] = 0x02;
   tmp04Data[32] = 0x01;
   tmp04Data[33] = 0x3F;
   tmp04Data[34] = 0x0B;
   tmp04Data[35] = 0x00;
   tmp04Data[36] = 0x00;
   tmp04Data[37] = 0x00;
   tmp04Data[38] = 0x00;
   tmp04Data[39] = 0x00;
   tmp04Data[40] = 0x00;
   tmp04Data[41] = 0x00;
   tmp04Data[42] = 0x00;
   tmp04Data[43] = 0x00;
   tmp04Data[44] = 0x00;
   tmp04Data[45] = 0x00;
   tmp04Data[46] = 0x00;
   tmp04Data[47] = 0x00;
   tmp04Data[48] = 0x00;
   tmp04Data[49] = 0x00;
   tmp04Data[50] = 0x00;
   tmp04Data[51] = 0xD6;
   tmp04Data[52] = 0x16;
   pFrame = tmp04Data;
   dataLength = 53;
   */

    //���ǰһ֡���ڴ���֮�У��������鷵��
    if (frameProcess == PROCESSING)
    {
       return 1;
    }
    
    //��ʼ�������ṹ
    frame.l1 = 0;
    frame.l2 = 0;
    frame.a1 = 0;
    frame.a2 = 0;
    frame.pData = NULL;
    frame.pDataEnd = NULL;
    frame.pw = NULL;
    frame.pTp = NULL;
  
    frame.c = 0;
    frame.a3 = 0;
    frame.afn = 0;
    frame.seq = 0;
    frame.cs = 0;
    frame.acd = 0;
    
    //������շ����и��ֶ�ֵ
    frame.l1    = *(pFrame + 2)<<8 | *(pFrame + 1);
    frame.l2    = *(pFrame + 4)<<8 | *(pFrame + 3);
    frame.c     = *(pFrame + 6);
    frame.a1    = *(pFrame + 8)<<8 | *(pFrame + 7);
    frame.a2    = *(pFrame + 10)<<8 | *(pFrame + 9);
    frame.a3    = *(pFrame + 11);
    frame.pData =   pFrame + 12;
    frame.afn   = *(pFrame + 12);
    frame.seq   = *(pFrame + 13);
    frame.cs    = *(pFrame + dataLength - 2);
    
    //ȷ��tp��pw�ڷ����е�λ��,����Ϊ�û��û�����β
    if (frame.afn == RESET_CMD || frame.afn == SET_PARAMETER || frame.afn == CTRL_COMMAND
    	|| frame.afn == AUTHENTICATION || frame.afn == FILE_TRANSPORT || frame.afn == DATA_FORWARD || frame.afn==AUTHENTICATION)
    {
      #ifdef USE_16BYTES_PW //16�ֽ�PW
      	if (frame.seq>>7)   //����tp����pw
        {
          frame.pTp = pFrame + dataLength - 8;
          frame.pw = pFrame + dataLength - 24;
          frame.pDataEnd = pFrame + dataLength - 24;
          
					//2016-11-23,��ӷ�Խ�紦��
					if ((dataLength-36)<2040)
					{
            //����AFN��SEQ�����ݵ�Ԫ��ʶ�����ݵ�Ԫ
            memcpy(macCheckData, frame.pData, dataLength - 36);

            //����TpV
            memcpy(&macCheckData[dataLength - 36], frame.pTp, 6);
					}

          //MAC����ĳ���
          //ly,2011-03-28,���Ժ�����������MAC������Tp��6���ֽ�
          //lenOfMacCheck = dataLength - 30;
          lenOfMacCheck = dataLength - 36;
        }
        else  //ֻ��pw
        {
          frame.pw = pFrame + dataLength - 18;
          frame.pDataEnd = pFrame + dataLength - 18;
          
					//2016-11-23,��ӷ�Խ�紦��
					if ((dataLength-18)<2040)
					{
            //����AFN��SEQ�����ݵ�Ԫ��ʶ�����ݵ�Ԫ
            memcpy(macCheckData, frame.pData, dataLength - 18);
					}
          
          //MAC����ĳ���
          lenOfMacCheck = dataLength - 30;
        }
        
        //��Ϣ��֤����Ϊ255�Ž���Ӳ����֤
        //if (*frame.pw!=0x00 && *(frame.pw+1)!=0x00 && *(frame.pw+2)!=0x00 && *(frame.pw+3)!=0x00 && messageAuth[0]==255)
        //2012-07-23,�²ɹ���ESAMоƬ��̨�����ʱ����,pw��4���ֽ��п�����0,����������
        if (!(*frame.pw==0x00 && *(frame.pw+1)==0x00 && *(frame.pw+2)==0x00 && *(frame.pw+3)==0x00) && messageAuth[0]==255)
        {
          if (frame.a2 == (addrField.a2[0] | addrField.a2[1]<<8))
          {
            calcMac(frame.afn, 0, macCheckData, lenOfMacCheck, retMac);
          }
          else
          {
          	//�ն����ַ�Ƿ�һ��
          	if ((groupAddr[0] | groupAddr[1]<<8)==frame.a2)
          	{
              calcMac(frame.afn, 1, macCheckData, lenOfMacCheck, retMac);
          	}
          	else
          	{
          		if ((groupAddr[2] | groupAddr[3]<<8)==frame.a2)
          		{
                calcMac(frame.afn, 2, macCheckData, lenOfMacCheck, retMac);          			 
          		}
          		else
          		{
          	 	  if ((groupAddr[4] | groupAddr[5]<<8)==frame.a2)
          	 	  {
                  calcMac(frame.afn, 3, macCheckData, lenOfMacCheck, retMac);          			 
          	 	  }
          	 	  else
          	 	  {
          	 	  	if ((groupAddr[6] | groupAddr[7]<<8)==frame.a2)
          	 	  	{
                    calcMac(frame.afn, 4, macCheckData, lenOfMacCheck, retMac);          			 
          	 	  	}
          	 	  	else
          	 	  	{
          	 	      if ((groupAddr[8] | groupAddr[9]<<8)==frame.a2)
          	 	      {
                      calcMac(frame.afn, 5, macCheckData, lenOfMacCheck, retMac);
          	 	      }
          	 	      else
          	 	      {
          	 	      	if ((groupAddr[10] | groupAddr[11]<<8)==frame.a2)
          	 	      	{
                        calcMac(frame.afn, 6, macCheckData, lenOfMacCheck, retMac);
          	 	      	}
          	 	      	else
          	 	      	{
          	 	      		if ((groupAddr[12] | groupAddr[13]<<8)==frame.a2)
          	 	      		{
                          calcMac(frame.afn, 7, macCheckData, lenOfMacCheck, retMac);
          	 	      		}
          	 	      		else
          	 	      		{
          	 	      			if ((groupAddr[14] | groupAddr[15]<<8)==frame.a2)
          	 	      			{
                            calcMac(frame.afn, 8, macCheckData, lenOfMacCheck, retMac);
          	 	      			}
          	 	      			else
          	 	      			{
          	 	      			  if (frame.a2==0xffff)    //ϵͳ�㲥��ַ
          	 	      			  {
                              calcMac(frame.afn, 9, macCheckData, lenOfMacCheck, retMac);
          	 	      			  }
          	 	      			  else
          	 	      			  {
                              calcMac(frame.afn, 0, macCheckData, lenOfMacCheck, retMac);
                            }
          	 	      			}
          	 	      	  }
          	 	      	}
          	 	      }
          	 	    }
          	 	  }
          	 	}
          	}
          }

  	      if (debugInfo&ESAM_DEBUG)
  	      {
  	        printf("��վ�·���MACֵ:%02x-%02x-%02x-%02x\n", *(frame.pw+3), *(frame.pw+2), *(frame.pw+1), *(frame.pw+0));
  	      }

          if (*frame.pw!=retMac[3] || *(frame.pw+1)!=retMac[2]  || *(frame.pw+2)!=retMac[1]  || *(frame.pw+3)!=retMac[0])
          {
            //��λESAM
            ioctl(fdOfIoChannel, ESAM_RST, 1);
      
            getChallenge(cmdBuf);
            
            //��������
            for(i=0;i<8;i++)
            {
              retMac[4+i] = cmdBuf[7-i];
            }
        
            //��ȡESAM Serial
            getSerial(cmdBuf);
            //��������
            for(i=0;i<8;i++)
            {
              retMac[12+i] = cmdBuf[7-i];
            }
        	  
        	  AFN00004(dataFrom, 3, &retMac[4]);

        		if (debugInfo&ESAM_DEBUG)
        		{
        		 	printf("MAC�Ƚϲ�һ��\n");
        		}
        	  
            //������Ϣ��֤�����¼
            if ((eventRecordConfig.iEvent[2]&0x08)||(eventRecordConfig.nEvent[2] & 0x08))
            {
       	    	eventData[0] = 20;       //ERC
       	    	eventData[1] = 25;       //����
       	    	 
       	    	//����ʱ��
              eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
              eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
              eventData[4] = sysTime.hour  /10<<4 | sysTime.hour  %10;
              eventData[5] = sysTime.day   /10<<4 | sysTime.day   %10;
              eventData[6] = sysTime.month /10<<4 | sysTime.month %10;
              eventData[7] = sysTime.year  /10<<4 | sysTime.year  %10;
               
              //��Ϣ��֤��PW 16Bytes
              memcpy(&eventData[8], frame.pw, 16);
               
              //����վ��ַMSA
              eventData[24] = addrField.a3>>1 & 0xff;
               
              if (eventRecordConfig.iEvent[2] & 0x08)
              {
                writeEvent(eventData, 25, 1, DATA_FROM_GPRS);
              }
              if (eventRecordConfig.nEvent[2] & 0x08)
              {
                writeEvent(eventData, 25, 2, DATA_FROM_LOCAL);
              }
               
              if (debugInfo&PRINT_EVENT_DEBUG)
              {
                printf("frameReport���������ϱ�\n");
              }
                 
              activeReport3();   //�����ϱ��¼�
            }
        	  
        	  goto breakPoint;
        	}
        	else
        	{
        		if (debugInfo&ESAM_DEBUG)
        		{
        		 	printf("MAC�Ƚ�һ��\n");
        		}
        	}
        }        
        
      #else                 //2�ֽ�PW
    	  if (frame.seq>>7)   //����tp����pw
        {
          frame.pTp = pFrame + dataLength - 8;
          frame.pw = pFrame + dataLength -10;
          frame.pDataEnd = pFrame + dataLength - 10;
        }
        else                //ֻ��pw
        {        
          frame.pw = pFrame + dataLength - 4;
          frame.pDataEnd = pFrame + dataLength - 4;
        }
      #endif
    }
    else
    {
      if (frame.seq>>7)  //��tp��pw
      {
        frame.pTp = pFrame + dataLength - 8;
        frame.pDataEnd = pFrame + dataLength - 8;
      }
      else  //��tpҲ��pw
      {        
        frame.pDataEnd = pFrame + dataLength - 2;
      } 
    }
    
    frame.loadLen = frame.pDataEnd-frame.pData - 2;

    //�Խ��շ��������Ч�Լ�⣬���������ֶ�У��,У���У��,Э����У�飬��ʼ/������־У���Լ�ʱ��У��
    //�����ʼ�ֶ�
    if (*pFrame != 0x68 || *(pFrame + 5) != 0x68)
    {
      if (debugInfo&WIRELESS_DEBUG)
      {
        printf("֡ͷ68����\n");
      }

      return 1;
    }

    //���Э����
    if ((frame.l1 & 0x3) != 0x2)
    {
      if (debugInfo&WIRELESS_DEBUG)
      {
        printf("֡Э�����\n");
      }
       
      return 1;
    }

    //�Ƚϳ�����
    if (frame.l1 != frame.l2)
    {
      if (debugInfo&WIRELESS_DEBUG)
      {
        printf("���ȴ���\n");
      }

      return 1;
    }

		//����֡��������յĳ���
    if (((frame.l1>>2) + 8) != dataLength)
    {
      if (debugInfo&WIRELESS_DEBUG)
      {
        printf("����֡��������յĳ��Ȳ�һ��\n");
      }

      return 1;
    }

    //���֡У���
    checkSum = 0;
    for (i = 0; i < frame.l1>>2;i++)
    {
       checkSum += *(pFrame+6+i);
    }

    if (checkSum != *(pFrame+6+i))
    {
      if (debugInfo&WIRELESS_DEBUG)
      {
        printf("У�����\n");
      }

      return 1;
    }

    //��������־
    if (*(pFrame+6+i+ 1) != 0x16)
    {
      if (debugInfo&WIRELESS_DEBUG)
      {
        printf("֡β0x16����\n");
      }

      return 1;
    }

    //�ն˵�ַ�����ַ�뱾���ն˵�ַ�Ƿ�һ��
    if (dataFrom == DATA_FROM_GPRS)
    {
    	//�ն˵�ַ�Ƿ�һ��
      if (frame.a2 != (addrField.a2[0] | addrField.a2[1]<<8))
      {
      	//�ն����ַ�Ƿ�һ��
      	 if ((groupAddr[0] | groupAddr[1]<<8)!= frame.a2 && (groupAddr[2] | groupAddr[3]<<8)!= frame.a2
      	 	   && (groupAddr[4] | groupAddr[5]<<8)!= frame.a2 && (groupAddr[6] | groupAddr[7]<<8)!= frame.a2
      	 	   && (groupAddr[8] | groupAddr[9]<<8)!= frame.a2 && (groupAddr[10] | groupAddr[11]<<8)!= frame.a2
      	 	   && (groupAddr[12] | groupAddr[13]<<8)!= frame.a2 && (groupAddr[14] | groupAddr[15]<<8)!= frame.a2
      	 	   && (groupAddr[16] | groupAddr[17]<<8)!= frame.a2 && (groupAddr[18] | groupAddr[19]<<8)!= frame.a2
      	 	   && (groupAddr[20] | groupAddr[21]<<8)!= frame.a2 && (groupAddr[22] | groupAddr[23]<<8)!= frame.a2
      	 	   && (groupAddr[24] | groupAddr[25]<<8)!= frame.a2 && (groupAddr[26] | groupAddr[27]<<8)!= frame.a2
      	 	   && (groupAddr[28] | groupAddr[29]<<8)!= frame.a2 && (groupAddr[30] | groupAddr[31]<<8)!= frame.a2
      	 	   && frame.a2!=0xffff /*ϵͳ�㲥��ַ*/)
      	 {
      	 	 if (checkIfCascade()==1)   //����Ǽ������ն�
      	 	 {
      	 	   //���Ƿ��Ǳ����ն˵ĵ�ַ
        	 	 if ((frame.a2 == (cascadeCommPara.cascadeTeAddr[0] | cascadeCommPara.cascadeTeAddr[1]<<8))
        	 	      || (frame.a2 == (cascadeCommPara.cascadeTeAddr[2] | cascadeCommPara.cascadeTeAddr[3]<<8))
        	 	       || (frame.a2 == (cascadeCommPara.cascadeTeAddr[4] | cascadeCommPara.cascadeTeAddr[5]<<8))
        	 	    )
        	 	 {
        	 	 	 if (debugInfo&WIRELESS_DEBUG)
        	 	 	 {
        	 	 	   printf("��֡�Ǳ��ն������������ն˵�ַ,ת������������\n");
        	 	 	 }
              
              #ifdef WDOG_USE_X_MEGA
               //�˴�����macCheckData�������,�����Ǵ���MACУ��
               macCheckData[0] = 0x2;
               macCheckData[1] = dataLength&0xff;
               macCheckData[2] = dataLength>>8&0xff;
               memcpy(&macCheckData[3], pFrame, dataLength);
               sendXmegaFrame(CASCADE_DATA, macCheckData, dataLength+3);
              #endif
        	 	 }
           }

           if (debugInfo&WIRELESS_DEBUG)
           {
             printf("�ն˵�ַ��һ�´���\n");
           }

      	 	 return 1;	//����һ�µ�����·���
      	 }
      }
    }    
    
    //�ж�֡�ĸ�����Ϣ������Tp
    if (frame.pTp != NULL)
    {
       //��ȡʱ��
       tpTime.second = *(frame.pTp+1);
       tpTime.minute = *(frame.pTp+2);
       tpTime.hour = *(frame.pTp+3);
       tpTime.day = *(frame.pTp+4);
       
       tpTime.second = (tpTime.second & 0xf) + (tpTime.second >> 4 & 0xf)* 10;
       tpTime.minute = (tpTime.minute & 0xf) + (tpTime.minute >> 4 & 0xf)* 10;
       tpTime.hour = (tpTime.hour & 0xf) + (tpTime.hour >> 4 & 0xf)* 10;
       tpTime.day = (tpTime.day & 0xf) + (tpTime.day >> 4 & 0xf)* 10;
       
       //���TP�еĴ�����ʱʱ��Ϊ0,�Ӷ�վ�������ж�
       if (*(frame.pTp + 5) != 0)
       {
          if (sysTime.day == tpTime.day)
          {
           	totalDelayMinute = (sysTime.hour - tpTime.hour) * 60 + sysTime.minute - tpTime.minute;
          }
          else
          {
             if (sysTime.day == tpTime.day + 1)
             {
                totalDelayMinute = (sysTime.hour + 23 - tpTime.hour) * 60 + sysTime.minute + 60 - tpTime.minute;
             }
             else
             {
               if (debugInfo&WIRELESS_DEBUG)
               {
                 printf("Tp����1\n");
               }
               
               return 1;
             }
          }
          
          if (totalDelayMinute > *(frame.pTp + 5))
          {
            if (debugInfo&WIRELESS_DEBUG)
            {
              printf("Tp����2\n");
            }

            return 1;
          }
       }
    }
    
    //��Ĭ����Կģʽ�£��ն�ֻ������վ��������Կ����    
    tmpAuth = messageAuth[0];
    
    tmpAuth = 0x0;    //��ǰ��������У��(2007.7.18 Leiyong)(���Ҫ������У�飬Ӧ������ɾ��)
        
    if (tmpAuth==0xff)
    {
      if (frame.afn == SET_PARAMETER && findFnPn(*(pFrame+16), *(pFrame+17), FIND_FN) == 5)
      {
         keyMode = 1;
      }
      else
      {
         //ackOrNack(FALSE, dataFrom);
         goto breakPoint;
      }
    }
    else
    {
    	 if (tmpAuth == 0x0)
    	 {
    	 	  keyMode = 3;
    	 }
    	 else
    	 {
 	        keyMode = 2;
    	 }
    }
    
    //���ݴ����б�־��λ
    frameProcess = PROCESSING;
    
    //��ȡ֡�е�MSA�����ַ���е�A3[�ҽ����λ��0,���λ�Ǳ�ʾ����ַ�������ַ��
    addrField.a3 = frame.a3&0xfe;
   
    //��ȡ֡�е�pSeq��Ϊ��Ӧ֡���rSeq
    rSeq = frame.seq & 0xf;
	  
	  crcKeyCount(messageAuth[1], messageAuth[2]);
    
	  //�����һ���յ����ݵ�ʱ����Ϊ�ϴ�����ʱ��
	  if (dataFrom == DATA_FROM_GPRS)
	  {
       /*
       if (wlModemFlag.loginSuccess==0)
       {
       	 wlModemFlag.loginSuccess = 1;

         wlModemFlag.lastIpSuccess = 1;   //�ϴ�IP��¼��Ϊδ�ɹ�
       	 
       	 netStatus |= 0x3;
         netStatus &= 0xf;
         
         //numOfGprsReset = 0;              //��λGPRS������0
         wlFlagsSet.lastIpSuccess = TRUE;
          
         netStatus |= 0x4;
         netStatus &= 0x7;
         //lastCommWithMs = sysTime;

         //if (menuInLayer==0  && setParaWaitTime==0xfe)
         //{
         //   defaultMenu();
         //}         
       }
       */
      
	    #ifndef DKY_SUBMISSION
	     #ifdef LIGHTING
	      
	      //2016-07-04,����������������Ϊ����Ϊ��λ
	      nextHeartBeatTime = nextTime(sysTime, 0, commPara.heartBeat);	      	
	      	
	     #else
	        
	      nextHeartBeatTime = nextTime(sysTime, commPara.heartBeat, 0);

	     #endif
        
        lastHeartBeat = sysTime;
        
        if (debugInfo&WIRELESS_DEBUG)
        {
          printf("�´�����ʱ��Ϊ%02d-%02d-%02d %02d:%02d:%02d,",
                    nextHeartBeatTime.year,
                    nextHeartBeatTime.month,
                    nextHeartBeatTime.day,
                    nextHeartBeatTime.hour,
                    nextHeartBeatTime.minute,
                    nextHeartBeatTime.second
                );
        }
          
	    #endif
	     
       if (wlModemFlag.heartSuccess==0)
       {
         wlModemFlag.heartSuccess = 1;
       
        #ifdef PLUG_IN_CARRIER_MODULE
         #ifndef MENU_FOR_CQ_CANON
          showInfo("��վȷ������!");
         #endif
        #endif
       }
       else
       {
        #ifdef PLUG_IN_CARRIER_MODULE
         #ifndef MENU_FOR_CQ_CANON
          showInfo("����վͨ��...");
         #endif
        #endif
       }
       
       //��¼�ս�����վ����֡����������վ�����ֽ���
       frameReport(2, dataLength, 0);
    }
    
    if (debugInfo&WIRELESS_DEBUG)
    {
      printf("��վ����,AFN=%02X\n",frame.afn);
      for(i=0;i<dataLength;i++)
      {
        printf("%02X ",*(pFrame+i));
      }
      printf("\n");           
    }
   
    //�ж�AFN�����ֱ����Դ������и�λ�����ò������������
    //�����֤����ԿЭ�̣��ļ����䣬����ת����Ҫ��������У��
    switch (frame.afn)
    {
       //ȷ��/����
       case RESPONSE:
       	 AFN00(frame.pData+2, frame.pDataEnd, dataFrom);
       	 break;
       	 
       //��λ
       case RESET_CMD:
       	  if (keyMode<3)
       	  {
       	     tmpCrc = countCRC(pFrame+6);
             if ((tmpCrc & 0xff) == *frame.pw && (tmpCrc>>8 & 0xff) == *(frame.pw+1))
             {
               AFN01(frame.pData+2, frame.pDataEnd, dataFrom);
             }
             else    //���벻��ȷ,����
             {
               ackOrNack(FALSE, dataFrom);
             }
          }
          else
          {
             AFN01(frame.pData+2, frame.pDataEnd, dataFrom);
          }
          break;
          
       //���ò���
       case SET_PARAMETER:
       		//AFN04(frame.pData+2, frame.pDataEnd, dataFrom);
       	  if (keyMode<3)
       	  {
       	     tmpCrc = countCRC(pFrame+6);
             if ((tmpCrc & 0xff) == *frame.pw && (tmpCrc>>8 & 0xff) == *(frame.pw+1))
             {
                AFN04(frame.pData+2, frame.pDataEnd, dataFrom);
             }
             else    //���벻��ȷ,��������
             {
                ackOrNack(FALSE, dataFrom);
             }
          }
          else
          {
             AFN04(frame.pData+2, frame.pDataEnd, dataFrom);
          }
          break;
          
       //��������
       case CTRL_COMMAND:
       	  if (keyMode<3)
       	  {
       	     tmpCrc = countCRC(pFrame+6);
             if ((tmpCrc & 0xff) == *frame.pw && (tmpCrc>>8 & 0xff) == *(frame.pw+1))
             {
                AFN05(frame.pData+2, frame.pDataEnd, dataFrom);
             }
             else    //���벻��ȷ,��������
             {
                ackOrNack(FALSE, dataFrom);
             }
          }
          else
          {
             AFN05(frame.pData+2, frame.pDataEnd, dataFrom);
          }
          break;
          
       //�����֤����ԿЭ��
       case AUTHENTICATION:
         AFN06(frame.pData+2, frame.pDataEnd, dataFrom);
       	 break;
       	 
       case REQUEST_TE_CONFIG:
          AFN09(frame.pData+2, frame.pDataEnd, dataFrom,QUERY);
          break;       	 
          
       //������ѯ
       case INQUIRE_INDEX:
          AFN0A(frame.pData+2, frame.pDataEnd, dataFrom);
     	    break;
     	 
     	 //�����ѯ����
       case INQUIRE_QUEST_INDEX:
          AFN0B(frame.pData+2, frame.pDataEnd, dataFrom);
     	    break;
     	   
       //����һ������
       case INQUIRE_TYPE_1:
          AFN0C(frame.pData+2, frame.pDataEnd, dataFrom,QUERY);
          break;
       
       //�����������
       case INQUIRE_TYPE_2:
       	 AFN0D(frame.pData+2, frame.pDataEnd, dataFrom,QUERY);  
         break;
       
       //������������
       case INQUIRE_TYPE_3:
       	 AFN0E(frame.pData+2, frame.pDataEnd, dataFrom,QUERY);        	  
         break;
       
       case FILE_TRANSPORT:
       	 AFN0F(frame.pData+2, frame.pDataEnd, dataFrom);
       	 break;
       	 
       case DATA_FORWARD:
       	 AFN10(frame.pData+2, frame.pDataEnd, dataFrom);
       	 break;
    }

breakPoint:
    
    //�ӵ�֡���м�֡���Ƕ�֡��־,�Ҽ���У���;
    addFrameFlag(FALSE,FALSE);

	  if (dataFrom == DATA_FROM_GPRS)
	  {
       if (debugInfo&WIRELESS_DEBUG)
       {
       	 printf("��վ������������MODEM,fQueue.sendPtr=%d,fQueue.tailPtr=%d\n", fQueue.sendPtr, fQueue.tailPtr);
       }
       if (fQueue.sendPtr!=fQueue.tailPtr)
       {
          //������ʱ������TCP����
          fQueue.inTimeFrameSend = TRUE;
          
          if (debugInfo&WIRELESS_DEBUG)
          {
       	    printf("��������TCP��ʱ����,fqueue�е�activeFrameSend=%d,continueActiveSend=%d,delay=%d,active0dDataSending=%d\n", fQueue.activeFrameSend, fQueue.continueActiveSend,fQueue.delay,fQueue.active0dDataSending);
          }
       }
    }
    else
    {
	 	   while(fQueue.sendPtr!=fQueue.tailPtr)
	 	   {
	 	   	  if (fQueue.frame[fQueue.sendPtr].len==0 || fQueue.sendPtr==0xff)
	 	   	  {
	 	   	  	 fQueue.sendPtr = 0;
	 	   	  	 fQueue.tailPtr = 0;
	 	   	  	 
	 	   	  	 break;
	 	   	  }
	 	   	  
	 	   	  #ifdef WDOG_USE_X_MEGA
           buf[0] = fQueue.frame[fQueue.sendPtr].len&0xff;
           buf[1] = fQueue.frame[fQueue.sendPtr].len>>8&0xff;
           memcpy(&buf[2], &msFrame[fQueue.frame[fQueue.sendPtr].head], fQueue.frame[fQueue.sendPtr].len);
	 	   	   switch (dataFrom)
	 	   	   {
	 	   	   	 case DATA_FROM_IR:
               sendXmegaFrame(IR_DATA, buf, fQueue.frame[fQueue.sendPtr].len+2);
               break;
             
             case DATA_FROM_RS485_2:
               buf[0] = 0x2;
               buf[1] = fQueue.frame[fQueue.sendPtr].len&0xff;
               buf[2] = fQueue.frame[fQueue.sendPtr].len>>8&0xff;
               memcpy(&buf[3], &msFrame[fQueue.frame[fQueue.sendPtr].head], fQueue.frame[fQueue.sendPtr].len);
               sendXmegaFrame(COPY_DATA, buf, fQueue.frame[fQueue.sendPtr].len+3);
               break;
             
             default:
               sendXmegaFrame(MAINTAIN_DATA, buf, fQueue.frame[fQueue.sendPtr].len+2);
               break;
           }
	 	   	  #else
	 	   	    sendLocalMsFrame(&msFrame[fQueue.frame[fQueue.sendPtr].head], fQueue.frame[fQueue.sendPtr].len);
	 	   	  #endif
	 	   	  
	 	   	  fQueue.sendPtr = fQueue.frame[fQueue.sendPtr].next;
	 	   }
       
       #ifdef WDOG_USE_X_MEGA
  	    if (xMegaQueue.sendPtr!=xMegaQueue.tailPtr)
  	    {
  	      xMegaQueue.inTimeFrameSend = TRUE;
  	    }
  	   #endif
	 	   
	 	   fQueue.sendPtr = fQueue.tailPtr;
	 	   fQueue.thisStartPtr = fQueue.tailPtr;
    }
    
    //�����б�־��λ
    frameProcess = PROCESS_DONE;
		
    return 0;
}

/*******************************************************
��������: countCRC
��������: �ն˼�������֡��CRC
���ú���:     
�����ú���:
�������:   ���ݶ�ָ��
�������:  
����ֵ�� TRUE or FALSE 
*******************************************************/
INT16U countCRC(INT8U *pData)
{
    INT16U  crc;
    INT8U   tmpInt;
    INT8U   len;
    
    crc = 0;
    for (len = 0; len < 12; len++)
    {
      for(tmpInt = 0x01; tmpInt != 0; tmpInt <<= 1)
      {
        if((crc&0x0001) != 0)
        {
        	crc >>= 1; 
        	crc ^= crcKey;
        }
        else 
        {
        	crc >>= 1;
        }
      
        if((*pData & tmpInt) != 0)
        {
        	crc ^= crcKey;
        }
      }
      
      pData++;
    }
    
    return crc;
}

void crcKeyCount(INT8U pw0, INT8U pw1)
{
	 if(keyMode==1)
	 {
	    crcKey = addrField.a1[0] ^ addrField.a2[1] << 8 | addrField.a1[1] ^ addrField.a2[0];
	 }
	 else
	 {
	  	crcKey = pw0 | pw1<<8;
	 }
}

/*******************************************************
��������:findFnPn
��������:����Fn��Pn,����ӳ����
���ú���:
�����ú���:
�������:
�������:
����ֵ��8λFn��Pnֵ
*******************************************************/
INT8U findFnPn(INT8U data1, INT8U data2,INT8U type)
{
  INT8U tmpData1, tmpData2;
  
  if ((data1 == 0) && (data2 == 0))
  {
    return 0;
  }

  tmpData1 = 0;
  while (tmpData1<8)
  {    
    tmpData1++;
    if ((data1&1) == 1)
      break;
    data1 >>= 1;;
  }
  
  if (type == FIND_PN)
  {
    tmpData2 = 0;
    while (data2 != 0)
    {
      if ((data2&1)==1)
      {
        break;
      }
      data2 >>= 1;
      tmpData2++;
    }
    return tmpData2*8+tmpData1;
  }
  else
  {
  	return  data2*8+tmpData1;
  }
}

/*******************************************************
��������:addFrameFlag
��������:�ӵ�֡���м�֡�����һ֡��־������У���
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
void addFrameFlag(BOOL activeReport,BOOL singleFrame)
{
   INT8U     checkSum;
   INT16U    i,tmpj;
   
   /*
   if (fQueue.num>0)
   {
       if (fQueue.num==1) //��֡
       {
       	 msFrame[13] |= 0x60;
       	 msFrame[13] |= rSeq;
         
         tmpj = 6;
         checkSum = 0;
         while (tmpj < fQueue.frame[0].len-2)
         {
            checkSum += msFrame[tmpj];
            tmpj++;
         }                          
         msFrame[fQueue.frame[0].len-2] = checkSum;
       }
       else              //��֡
       {
       	 for(i=0;i<fQueue.num;i++)
       	 {
       	   if (activeReport==TRUE)
       	   {
         	   msFrame[fQueue.frame[i].head+13] |= 0x60;         //�����ϱ���Ϊ��֡
       	   }
       	   else
       	   {
         	   if (i==0)
         	   {
         	     msFrame[fQueue.frame[i].head+13] |= 0x40;       //��֡:��ʼ֡
         	   }
         	   else
         	   {
         	   	  if (i==(fQueue.num-1))
         	   	  {
         	         msFrame[fQueue.frame[i].head+13] |= 0x20;       //��֡:����֡
         	   	  }
         	   	  else
         	   	  {
         	         msFrame[fQueue.frame[i].head+13] |= 0x00;       //��֡:�м�֡
         	   	  }
         	   }
         	 }
         	 msFrame[fQueue.frame[i].head+13] |= (rSeq+i)&0xf;
           
           tmpj = fQueue.frame[i].head + 6;
           checkSum = 0;
           while (tmpj < (fQueue.frame[i].head + fQueue.frame[i].len-2))
           {
              checkSum = msFrame[tmpj]+checkSum;
              tmpj++;
           }

           msFrame[fQueue.frame[i].head + fQueue.frame[i].len-2] = checkSum;
       	 }
       }
   }
   */
   
   if (activeReport!=TRUE)
   {
      if (fQueue.thisStartPtr!=fQueue.tailPtr)
      {
         if (fQueue.tailPtr-fQueue.thisStartPtr==1 || fQueue.frame[fQueue.thisStartPtr].next==fQueue.tailPtr) //��֡
         {
         	  msFrame[fQueue.frame[fQueue.thisStartPtr].head + 13] |= 0x60;
         	  msFrame[fQueue.frame[fQueue.thisStartPtr].head + 13] |= rSeq;
           
            tmpj = fQueue.frame[fQueue.thisStartPtr].head + 6;
            checkSum = 0;
            while (tmpj < fQueue.frame[fQueue.thisStartPtr].head+fQueue.frame[fQueue.thisStartPtr].len-2)
            {
              checkSum += msFrame[tmpj];
              tmpj++;
            }                          
            msFrame[fQueue.frame[fQueue.thisStartPtr].head+fQueue.frame[fQueue.thisStartPtr].len-2] = checkSum;
           
            fQueue.thisStartPtr = fQueue.tailPtr;
         }
         else              //��֡
         {
         	 for(i=0;(i<128) && (fQueue.thisStartPtr!=fQueue.tailPtr);i++)
         	 {
         	   if (i==0)
         	   {
         	      msFrame[fQueue.frame[fQueue.thisStartPtr].head+13] |= 0x40;       //��֡:��ʼ֡
         	   }
         	   else
         	   {
         	   	  if (fQueue.tailPtr-fQueue.thisStartPtr==1 || fQueue.frame[fQueue.thisStartPtr].next==fQueue.tailPtr)
         	   	  {
         	         msFrame[fQueue.frame[fQueue.thisStartPtr].head+13] |= 0x20;       //��֡:����֡
         	   	  }
         	   	  else
         	   	  {
         	         msFrame[fQueue.frame[fQueue.thisStartPtr].head+13] |= 0x00;       //��֡:�м�֡
         	   	  }
         	   }
           	 msFrame[fQueue.frame[fQueue.thisStartPtr].head+13] |= ((rSeq+i)&0xf);
             
             tmpj = fQueue.frame[fQueue.thisStartPtr].head + 6;
             checkSum = 0;
             while (tmpj < (fQueue.frame[fQueue.thisStartPtr].head + fQueue.frame[fQueue.thisStartPtr].len-2))
             {
               checkSum = msFrame[tmpj]+checkSum;
               tmpj++;
             }
  
             msFrame[fQueue.frame[fQueue.thisStartPtr].head + fQueue.frame[fQueue.thisStartPtr].len-2] = checkSum;
             
             fQueue.thisStartPtr = fQueue.frame[fQueue.thisStartPtr].next;
         	 }
         }
      }
   }
   else    //�����ϱ�����
   {
      printf("activeThisStartPtr=%d,activeTailPtr=%d\n",fQueue.activeThisStartPtr,fQueue.activeTailPtr);
      if (fQueue.activeThisStartPtr!=fQueue.activeTailPtr)
      {
         if ((fQueue.activeTailPtr-fQueue.activeThisStartPtr)==1 || fQueue.activeFrame[fQueue.activeThisStartPtr].next==fQueue.activeTailPtr) //��֡
         {
         	  if (singleFrame==TRUE)
         	  {
         	    activeFrame[fQueue.activeFrame[fQueue.activeThisStartPtr].head + 13] |= 0x70;
         	  }
         	  else
         	  {
         	    activeFrame[fQueue.activeFrame[fQueue.activeThisStartPtr].head + 13] |= 0x60;
         	  }
         	  activeFrame[fQueue.activeFrame[fQueue.activeThisStartPtr].head + 13] |= (pSeq&0xf);
         	  pSeq++;
           
            tmpj = fQueue.activeFrame[fQueue.activeThisStartPtr].head + 6;
            checkSum = 0;
            while (tmpj < fQueue.activeFrame[fQueue.activeThisStartPtr].head+fQueue.activeFrame[fQueue.activeThisStartPtr].len-2)
            {
              checkSum += activeFrame[tmpj];
              tmpj++;
            }                          
            
            activeFrame[fQueue.activeFrame[fQueue.activeThisStartPtr].head+fQueue.activeFrame[fQueue.activeThisStartPtr].len-2] = checkSum;
           
            fQueue.activeThisStartPtr = fQueue.activeTailPtr;
         }
         else              //��֡
         {
         	 for(i=0;(i<128) && (fQueue.activeThisStartPtr!=fQueue.activeTailPtr);i++)
         	 {
         	   if (singleFrame==TRUE)
         	   {
           	   activeFrame[fQueue.activeFrame[fQueue.activeThisStartPtr].head+13] |= 0x70;         //�����ϱ�����Ϊ��֡
           	   printf("�ӵ�֡ͷ\n");
         	   }
         	   else
         	   {
           	   if (i==0)
           	   {
           	     activeFrame[fQueue.activeFrame[fQueue.activeThisStartPtr].head+13] |= 0x40;       //��֡:��ʼ֡
           	   }
           	   else
           	   {
           	   	  if ((fQueue.activeTailPtr-fQueue.activeThisStartPtr)==1 || fQueue.activeFrame[fQueue.activeThisStartPtr].next==fQueue.activeTailPtr)
           	   	  {
           	         activeFrame[fQueue.activeFrame[fQueue.activeThisStartPtr].head+13] |= 0x20;       //��֡:����֡
           	   	  }
           	   	  else
           	   	  {
           	         activeFrame[fQueue.activeFrame[fQueue.activeThisStartPtr].head+13] |= 0x00;       //��֡:�м�֡
           	   	  }
           	   }
           	 }
           	 
           	 activeFrame[fQueue.activeFrame[fQueue.activeThisStartPtr].head+13] |= pSeq&0xf;
           	 pSeq++;
             
             tmpj = fQueue.activeFrame[fQueue.activeThisStartPtr].head + 6;
             checkSum = 0;
             while (tmpj < (fQueue.activeFrame[fQueue.activeThisStartPtr].head + fQueue.activeFrame[fQueue.activeThisStartPtr].len-2))
             {
                checkSum = activeFrame[tmpj]+checkSum;
                tmpj++;
             }
  
             activeFrame[fQueue.activeFrame[fQueue.activeThisStartPtr].head + fQueue.activeFrame[fQueue.activeThisStartPtr].len-2] = checkSum;
             
             fQueue.activeThisStartPtr = fQueue.activeFrame[fQueue.activeThisStartPtr].next;
         	 }
         }
      }   	  
   }
}

/**************************************************
��������:threadOfmsLocalReceive
��������:���ض˿�����վͨ�Ž��մ����߳�
���ú���:
�����ú���:
�������:void *arg
�������:
����ֵ��״̬
***************************************************/
void *threadOfmsLocalReceive(void *arg)
{
  INT8U  recvLen=0;
  INT8U  tmpBuf[50];
  INT8U  j=0;
  INT16U lenOfMsLocal=0;                      //���ݳ���
  INT16U msLocalTail = 0;                   //֡β��
  
  #ifdef WDOG_USE_X_MEGA
   INT8U flagOfXmegaFrame=FRAME_NONE;       //xMega֡��־
   INT8U recvLocalBuf[SIZE_OF_RECV_MS_BUF]; //���ձ���ͨ��֡����
  #endif
  
  ANALYSE_XMEGA_FRAME xMegaFrame;
   
  INT8U          checkSum=0;
  INT16U         i=0, recvi=0;
  INT16U         tmpHeadCas=0;
   
  struct timeval tv;
    
  while (1)
  {
     recvLen = read(fdOfLocal, &tmpBuf, 50);

    #ifdef WDOG_USE_X_MEGA
     if (debugInfo&PRINT_XMEGA_DEBUG)
     {
       printf("xMega Rx Data, Len=%d:",recvLen);
       for(i=0;i<recvLen;i++)
       {
     	   printf(" %02x", tmpBuf[i]);
       }
       printf("\n");
     }

     for (recvi=0;recvi<recvLen;recvi++)
     {
       recvLocalBuf[msLocalTail] = tmpBuf[recvi];
       
       if (flagOfXmegaFrame==FRAME_NONE)
       {
         if (msLocalTail==0)
         {
           if (recvLocalBuf[msLocalTail]!=0x68)
           {
             msLocalTail = 0;
             flagOfXmegaFrame = FRAME_NONE;
             
             continue;
           }
         }
         else
         {
           if (msLocalTail==3)
           {
             if (recvLocalBuf[msLocalTail]==0x68)
             {
               flagOfXmegaFrame = FRAME_START;
               lenOfMsLocal = (recvLocalBuf[2]<<8 | recvLocalBuf[1])+6;
             }
             else
             {
               msLocalTail = 0;
               flagOfXmegaFrame = FRAME_NONE;
               
               continue;
             }
           }
         }
       }
       else
       {
         if (flagOfXmegaFrame==FRAME_START)
         {
           if (msLocalTail+1==lenOfMsLocal)
           {
             //֡����,ִ���������
             if (recvLocalBuf[msLocalTail]==0x16)
             {
               msLocalTail++;
               flagOfXmegaFrame = FRAME_END;

               //������շ����и��ֶ�ֵ
               xMegaFrame.len     = *(recvLocalBuf + 1) | *(recvLocalBuf + 2)<<8;
               xMegaFrame.address = *(recvLocalBuf + 4);
               xMegaFrame.afn     = *(recvLocalBuf + 5);
               xMegaFrame.seq     = *(recvLocalBuf + 6);
               xMegaFrame.pData   =   recvLocalBuf + 7;
               xMegaFrame.cs      = *(recvLocalBuf + msLocalTail - 2);
            
               //���֡У���
               checkSum = 0;
               for (i = 0; i < msLocalTail-6; i++)
               {
                  checkSum += *(recvLocalBuf+4+i);
               }
               
               if (checkSum == (xMegaFrame.cs))
               {
                 #ifdef TE_CTRL_BOARD_V_1_3
                  //ר��III�ն��Žӵ�״̬
                  gateKvalue = xMegaFrame.address;
                 #endif
                 
                 switch(xMegaFrame.afn)
                 {
                    case ACK_OR_NACK:  //ȷ�ϻ����
                      switch(*xMegaFrame.pData)
                      {
                         case 0x1:     //ȷ��
                           break;
               
                         case 0x2:     //����
                           break;
                      }
                      break;
              
                    case COPY_DATA:    //��������
                      if (debugInfo&PRINT_XMEGA_DEBUG)
                      {
                        gettimeofday(&tv, NULL);
                    	  printf("%d-%d-%d %d:%d:%d ��=%d,΢��=%d���س�������,�˿�=%d\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,tv.tv_sec, tv.tv_usec,*xMegaFrame.pData+1);
                      }
              
                    	//��2·485����Ϊά����
                    	if (2==*xMegaFrame.pData && 0x55==rs485Port2Fun)
                    	{
                    		msInput(xMegaFrame.pData+3, *(xMegaFrame.pData+1) |*(xMegaFrame.pData+2)<<8, DATA_FROM_RS485_2);                    		
                    	}
                    	else
                    	{
                    	  xMegaCopyData(*xMegaFrame.pData, xMegaFrame.pData+3, *(xMegaFrame.pData+1) |*(xMegaFrame.pData+2)<<8);
                    	}
                    	xMegaQueue.delay = XMEGA_SEND_DELAY - 5;
                      break;
                   
                   case MAINTAIN_DATA: //ά����������
                   	 msInput(xMegaFrame.pData+2, *xMegaFrame.pData |*(xMegaFrame.pData+1)<<8,DATA_FROM_LOCAL);
                   	 break;

                   case IR_DATA:       //���⴮������
                   	 msInput(xMegaFrame.pData+2, *xMegaFrame.pData |*(xMegaFrame.pData+1)<<8,DATA_FROM_IR);
                   	 break;
                   	 
                   case GATHER_IO_DATA://�ɼ�IO����
                   	 moduleType = *xMegaFrame.pData;
                   	 gatherModuleType = 1;
                   	 
                   	 if (debugInfo&WIRELESS_DEBUG)
                   	 {
                   	 	 printf("ģ������=%d\n", moduleType);
                   	 }
                   	 
                   	 //ly,2012-03-10,add
                   	 if (moduleType==GPRS_SIM900A)
                   	 {
                   	 	 moduleType = GPRS_SIM300C;
                   	 	 
                   	 	 if (bakModuleType==NO_MODULE)  //2012-11-08,add�ж�
                   	 	 {
                   	 	   bakModuleType = GPRS_SIM900A;
                   	 	 }
                   	 }
                   	 
                   	 if (moduleType==CDMA_CM180)
                   	 {
      	 	             setModemSoPara();
      	 	           }

                   	 //if (wlRssi==0)
                   	 //{
                   	 //  modemSignal(0);
                   	 //}
                   	 //else
                   	 //{
                   	 //  modemSignal(wlRssi/8+1);
                   	 //}
                   	 break;
                   
                  #ifdef TE_CTRL_BOARD_V_1_3	 
                   case TE_GATE_K:     //ר��III�ն��Žӵ�״̬
                   	 gateKvalue = *xMegaFrame.pData;
                   	 break;
                  #endif
                  
                   case CASCADE_DATA:  //��������
                     if (debugInfo&WIRELESS_DEBUG)
                     {
                       gettimeofday(&tv, NULL);
                    	 printf("%d-%d-%d %d:%d:%d ��=%d,΢��=%d���ؼ����˿�����,�˿�=%d,���ݳ���=%d\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,tv.tv_sec, tv.tv_usec,*xMegaFrame.pData,*(xMegaFrame.pData+1) |*(xMegaFrame.pData+2)<<8);
                     }
                     
                     //�������ն��յ�������������,ֱ��ת������վ
                     if (checkIfCascade()==1)
                     {
                       if (fQueue.tailPtr == 0)
                       {
                          tmpHeadCas = 0;
                       }
                       else
                       {
                          tmpHeadCas = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
                       }
                       
                       fQueue.frame[fQueue.tailPtr].head = tmpHeadCas;
                       fQueue.frame[fQueue.tailPtr].len  = *(xMegaFrame.pData+1) |*(xMegaFrame.pData+2)<<8;
                       
                       memcpy(&msFrame[tmpHeadCas],xMegaFrame.pData+3, *(xMegaFrame.pData+1) |*(xMegaFrame.pData+2)<<8);
  
                       if ((tmpHeadCas+(*(xMegaFrame.pData+1) |*(xMegaFrame.pData+2)<<8)+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
                       	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
                       {
                          fQueue.frame[fQueue.tailPtr].next = 0x0;
                       	  fQueue.tailPtr = 0;
                       }
                       else
                       {                 
                          fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
                          fQueue.tailPtr++;
                       }
                    
                       //ly,2011-10-11,add
                       if (fQueue.sendPtr!=fQueue.tailPtr)
                       {
                         if (debugInfo&WIRELESS_DEBUG)
                         {
                         	 printf("threadOfmsLocalReceive:��������\n");
                         }
                    
                         //������ʱ������TCP����
                         fQueue.inTimeFrameSend = TRUE;
                       }
                     }
                     
                     //���������ն��յ�����������,�ύmsInput����
                     if (checkIfCascade()==2)
                     {
                     	 msInput(xMegaFrame.pData+3, *(xMegaFrame.pData+1) |*(xMegaFrame.pData+2)<<8, DATA_FROM_GPRS);
                     }

                   	 break;
                   	 
                  #ifdef PLUG_IN_CARRIER_MODULE
                   case ADC_DATA:    //������ֱ��ģ������Ϣ
                   	 adcData = (*xMegaFrame.pData | *(xMegaFrame.pData+1)<<8)*20/103;
                   	 
                   	 if (debugInfo&PRINT_XMEGA_DEBUG)
                   	 {
                   	   printf("ֱ��ģ����ֵ=%d(%d)\n", adcData, (*xMegaFrame.pData | *(xMegaFrame.pData+1)<<8));
                   	 }
                   	 break;
                  #endif
                 }
               }
               else
               {
               	 printf("xMega����:У���, ���ճ���=%d\n", lenOfMsLocal);
               }

               lenOfMsLocal = 0xffff;               
               msLocalTail = 0;
               flagOfXmegaFrame = FRAME_NONE;
               
               continue;
             }
             else
             {
               msLocalTail = 0;
               flagOfXmegaFrame = FRAME_NONE;
               
               continue;
             }
           }
         }
       }
       
       if (msLocalTail>=2047)
       {
         flagOfXmegaFrame = FRAME_NONE;         
         msLocalTail = 0;
       }
       else
       {
         msLocalTail++;
       }
     }
     
    #else
    
     for (i=0;i<recvLen;i++)
     {
        recvMsBuf[msLocalTail++] = tmpBuf[i];
        
        if (msLocalTail==1)
        {
        	if (recvMsBuf[0]!=0x68)
          {
          	 msLocalTail  = 0;
          	 lenOfMsLocal = 0;
          	 continue;
          }
        }
        if (msLocalTail==6)
        {
           if (recvMsBuf[5]!=0x68)
           {
         	    msLocalTail = 0;
         	    lenOfMsLocal = 0;
              continue;
           }

           //֡��ʼ,ȡ�����ݷ��������ݵĳ���(length)
           lenOfMsLocal = recvMsBuf[2];
           lenOfMsLocal <<= 8;
           lenOfMsLocal = lenOfMsLocal | recvMsBuf[1];
           lenOfMsLocal = lenOfMsLocal >>2;
           
           lenOfMsLocal += 8;    //��8,�õ�֡�ܳ���
   
           //�������ջ�������С,������֡
           if (lenOfMsLocal > SIZE_OF_RECV_MS_BUF)
           {
              msLocalTail  = 0;
              lenOfMsLocal = 0;
              continue;
           }
        }
              
        if (msLocalTail==lenOfMsLocal)
        {
           msInput(recvMsBuf,lenOfMsLocal,DATA_FROM_LOCAL);
           lenOfMsLocal = 0;
           msLocalTail  = 0;
        }
     }
    #endif
    
    usleep(50);
  }
}

#ifdef USE_ON_BOARD_ETH

/*******************************************************
��������:tcpRecvProcess
��������:TCPӦ�ý��մ�����
���ú���:
�����ú���:
�������:
�������:
����ֵ��
*******************************************************/
err_t  tcpRecvProcess(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
   struct pbuf *q = p;
   INT8U  *pBuffer;
   INT16U len;
   
   //INT8U  say;
   
   pBuffer = recvMsBuf;
   
   //�������ݵ�ppp�Ļ���
   for(q = p; q != NULL; q = q->next)
   {
       /* Send the data from the pbuf to the interface, one pbuf at a
          time. The size of the data in each pbuf is kept in the ->len
          variable. */
      
       memcpy(pBuffer, q->payload, q->len);
       pBuffer += q->len;
   }
   len = recvMsBuf[2];
   len <<= 8;
   len = len | recvMsBuf[1];
   len = len >>2;                
             
   len += 8;    /*��8,�õ�֡�ܳ���*/   
	  
	 //say = 0xaa;
	 //sendDebugFrame(&say,1);
	 
	 //sendDebugFrame((INT8U *)&len,2);
	 
	 msInput(recvMsBuf,len,DATA_FROM_GPRS);
	 
	 pbuf_free(p);   //�ͷ�LwIP�ڴ滺����
	  
	 return ERR_OK;	  
}

#endif
