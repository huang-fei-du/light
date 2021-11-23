/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
�ļ�����AFN0D.c
���ߣ�TianYe
�汾��0.9
������ڣ�2010��1��
��������վAFN0D(�����������)�����ļ���
�����б�
�޸���ʷ��
  01,2006-08-09,TinaYe created.
  02,2010-06-18,Leiyong modify,���������Ͷ������ݸ�Ϊ�̴߳���
  03,2012-10-22,�޸Ĵ����ڸ�����������ʱ����,�ò�ͬ���ܶ��ٻ�����������ĳ�㲻һ��
		 ԭ��:��ȡ��������ʱ����readMeterData��������
***************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "teRunPara.h"
#include "msSetPara.h"
#include "workWithMeter.h"
#include "meterProtocol.h"
#include "copyMeter.h"
#include "dataBase.h"
#include "convert.h"
#include "wlModem.h"

#include "AFN0C.h"
#include "AFN0D.h"

extern INT8U  ackTail;

BOOL   hasCurveVisionData;           //û������ʾֵ����? ly,2009-12-02,add
INT8U  bakDataBuff[1024];            //�������ݻ���

/*******************************************************
��������:AFN0D
��������:��վ"��ʾ��������"(AFN0D)�Ĵ�����
���ú���:
�����ú���:
�������:
�������:
����ֵ��void
*******************************************************/
void AFN0D(INT8U *pDataHead, INT8U *pDataEnd,INT8U dataFrom, INT8U poll)
{
    INT16U    tmpFrameTail;             //����β
    INT16U    tmpI;
    INT8U     fn;
    INT8U     tmpDtCount;               //DT��λ����
    INT8U     tmpDt1;                   //��ʱDT1
    INT8U     *pTpv;                    //TpVָ��
    INT8U     maxCycle;                 //���ѭ������
    INT16U    tmpHead0dActive = 0;      //�����ϱ�AFN0D��ʱ֡ͷ
    INT16U    tmpHead0d   = 0;          //AFN0D��ʱ֡ͷ
    INT16U    frameTail0d = 0;          //AFN0D����β
    INT16U    offset0d;                 //���յ���֡�е����ݵ�Ԫƫ����(�������ݱ�ʶ���ֽ�)
    INT8U     checkSum;
    INT16U    frameLoadLen;
    INT8U     thisThreadSending;        //�������ϱ�����Ҫ����
    INT8U     taskNum;                  //�����
    INT8U     tmpDataBuff[1024];        //��ʱ���ݻ���
    DATE_TIME activeThreadTimeOut;      //�����ϱ�ʱ��Ϊ�̵߳ĳ�ʱʱ��
	  
	  INT8U     *pTmpFrame;
	  
	  #ifdef RECORD_LOG
	   char    logStr[2000];
	   char    sayStr[10];
	   INT16U  i;
    #endif

    
    INT16U (*AFN0DFun[221])(INT8U *buff, INT16U frameTail,INT8U *pHandle,INT8U fn, INT16U *offset0d);

    for (tmpI=0; tmpI<221; tmpI++)
    {
       AFN0DFun[tmpI] = NULL;
    }

    //��1 ���ܱ�ʾֵ�����������������
    AFN0DFun[0] = AFN0D001;
    AFN0DFun[1] = AFN0D002;
    AFN0DFun[2] = AFN0D003;
    AFN0DFun[3] = AFN0D004;
    AFN0DFun[4] = AFN0D005;
    AFN0DFun[5] = AFN0D006;
    AFN0DFun[6] = AFN0D007;
    AFN0DFun[7] = AFN0D008;
    
    //��2 ����ʾֵ���������
    AFN0DFun[8] = AFN0D009;
    AFN0DFun[9] = AFN0D010;
    AFN0DFun[10] = AFN0D011;
    AFN0DFun[11] = AFN0D012;
    
    //��3 ����ʾֵ�����������������
    AFN0DFun[16] = AFN0D017;
    AFN0DFun[17] = AFN0D018;
    AFN0DFun[18] = AFN0D019;
    AFN0DFun[19] = AFN0D020;
    AFN0DFun[20] = AFN0D021;
    AFN0DFun[21] = AFN0D022;
    AFN0DFun[22] = AFN0D023;
    AFN0DFun[23] = AFN0D024;
    
    //��4 ������ͳ������
    AFN0DFun[24] = AFN0D025;
    AFN0DFun[25] = AFN0D026;
    AFN0DFun[26] = AFN0D027;
    AFN0DFun[27] = AFN0D028;
    AFN0DFun[28] = AFN0D029;
    AFN0DFun[29] = AFN0D030;
    AFN0DFun[30] = AFN0D031;
    AFN0DFun[31] = AFN0D032;
    
    //��5 ������ͳ������
    AFN0DFun[32] = AFN0D033;
    AFN0DFun[33] = AFN0D034;
    AFN0DFun[34] = AFN0D035;
    AFN0DFun[35] = AFN0D036;
    AFN0DFun[36] = AFN0D037;
    AFN0DFun[37] = AFN0D038;
    AFN0DFun[38] = AFN0D039;
    
    //��6 �޹�����ͳ������
    AFN0DFun[40] = AFN0D041;
    AFN0DFun[41] = AFN0D042;
    AFN0DFun[42] = AFN0D043;
    AFN0DFun[43] = AFN0D044;
    AFN0DFun[44] = AFN0D045;
    AFN0DFun[45] = AFN0D046;
    
    //��7 �ն�ͳ������
    AFN0DFun[48] = AFN0D049;
    AFN0DFun[49] = AFN0D050;
    AFN0DFun[50] = AFN0D051;
    AFN0DFun[51] = AFN0D052;
    AFN0DFun[52] = AFN0D053;
    AFN0DFun[53] = AFN0D054;
    
    AFN0DFun[54] = AFN0D055;   //�����Լ
    AFN0DFun[55] = AFN0D056;   //�����Լ

    //��8 �ܼ���ͳ������
    AFN0DFun[56] = AFN0D057;
    AFN0DFun[57] = AFN0D058;
    AFN0DFun[58] = AFN0D059;
    AFN0DFun[59] = AFN0D060;
    AFN0DFun[60] = AFN0D061;
    AFN0DFun[61] = AFN0D062;
    
    //��9 �ܼ���Խ��ͳ������
    AFN0DFun[64] = AFN0D065;
    AFN0DFun[65] = AFN0D066;
    
    //��10 �ܼ�������
    AFN0DFun[72] = AFN0D073;
    AFN0DFun[73] = AFN0D074;
    AFN0DFun[74] = AFN0D075;
    AFN0DFun[75] = AFN0D076;
	
	//���ڹ�������,2017-7-18,Add
    AFN0DFun[76] = AFN0D077;
    AFN0DFun[77] = AFN0D079;
    AFN0DFun[78] = AFN0D079;
    AFN0DFun[79] = AFN0D080;
    
    //��11 ��������
    AFN0DFun[80] = AFN0D081;
    AFN0DFun[81] = AFN0D082;
    AFN0DFun[82] = AFN0D083;
    AFN0DFun[83] = AFN0D084;
    AFN0DFun[84] = AFN0D085;
    AFN0DFun[85] = AFN0D086;
    AFN0DFun[86] = AFN0D087;
    AFN0DFun[87] = AFN0D088;
    
    //��12 ��ѹ��������
    AFN0DFun[88] = AFN0D089;
    AFN0DFun[89] = AFN0D090;
    AFN0DFun[90] = AFN0D091;
    AFN0DFun[91] = AFN0D092;
    AFN0DFun[92] = AFN0D093;
    AFN0DFun[93] = AFN0D094;
    AFN0DFun[94] = AFN0D095;

    //2017-7-19,add,Ƶ������
	AFN0DFun[95] = AFN0D096;
    
    //��13 �ܵ��������ܵ���ʾֵ����
    AFN0DFun[96] = AFN0D097;
    AFN0DFun[97] = AFN0D098;
    AFN0DFun[98] = AFN0D099;
    AFN0DFun[99] = AFN0D100;
    AFN0DFun[100] = AFN0D101;
    AFN0DFun[101] = AFN0D102;
    AFN0DFun[102] = AFN0D103;
    AFN0DFun[103] = AFN0D104;
    
    //��14 ������������ѹ��λ�����ߣ�������λ������ 
    AFN0DFun[104] = AFN0D105;
    AFN0DFun[105] = AFN0D106;
    AFN0DFun[106] = AFN0D107;
    AFN0DFun[107] = AFN0D108;
    AFN0DFun[108] = AFN0D109;
    AFN0DFun[109] = AFN0D110;
   
   #ifdef LIGHTING
    AFN0DFun[110] = AFN0D111;
    AFN0DFun[111] = AFN0D112;
   #else
    AFN0DFun[110] = AFN0D111;
   #endif
    
    //��15 г�����ͳ������
    AFN0DFun[112] = AFN0D113;
    AFN0DFun[113] = AFN0D114;
    AFN0DFun[114] = AFN0D115;
    AFN0DFun[115] = AFN0D116;
    AFN0DFun[116] = AFN0D117;
    AFN0DFun[117] = AFN0D118;
    
    //��16 г��Խ��ͳ������
    AFN0DFun[120] = AFN0D121;
    AFN0DFun[121] = AFN0D122;
    AFN0DFun[122] = AFN0D123;
    
    //��17ֱ��ģ��������
    AFN0DFun[128] = AFN0D129;
    AFN0DFun[129] = AFN0D130;
    
    //��18 ֱ��ģ������������
    AFN0DFun[137] = AFN0D138;
    
    //��19 �ĸ������޹��ܵ���ʾֵ����
    AFN0DFun[144] = AFN0D145;
    AFN0DFun[145] = AFN0D146;
    AFN0DFun[146] = AFN0D147;
    AFN0DFun[147] = AFN0D148;

   #ifdef LIGHTING
    //�������һ�ο����ء���ʱ��
    AFN0DFun[148] = AFN0D149;
    AFN0DFun[149] = AFN0D150;
    AFN0DFun[150] = AFN0D151;
    AFN0DFun[151] = AFN0D152;   
   #endif
    
    //��20 �������ʾֵ
    AFN0DFun[152] = AFN0D153;
    AFN0DFun[153] = AFN0D154;
    AFN0DFun[154] = AFN0D155;
    AFN0DFun[155] = AFN0D156;
    AFN0DFun[156] = AFN0D157;
    AFN0DFun[157] = AFN0D158;
    AFN0DFun[158] = AFN0D159;
    AFN0DFun[159] = AFN0D160;
    
    //��21
    AFN0DFun[160] = AFN0D161;
    AFN0DFun[161] = AFN0D162;
    AFN0DFun[162] = AFN0D163;
    AFN0DFun[163] = AFN0D164;
    AFN0DFun[164] = AFN0D165;
    AFN0DFun[165] = AFN0D166;
    AFN0DFun[166] = AFN0D167;
    AFN0DFun[167] = AFN0D168;
    
    //��22
    AFN0DFun[168] = AFN0D169;
    AFN0DFun[169] = AFN0D170;
    AFN0DFun[170] = AFN0D171;
    AFN0DFun[171] = AFN0D172;
    AFN0DFun[172] = AFN0D173;
    AFN0DFun[173] = AFN0D174;
    AFN0DFun[174] = AFN0D175;
    AFN0DFun[175] = AFN0D176;

    //��23
    AFN0DFun[176] = AFN0D177;
    AFN0DFun[177] = AFN0D178;
    AFN0DFun[178] = AFN0D179;
    AFN0DFun[179] = AFN0D180;
    AFN0DFun[180] = AFN0D181;
    AFN0DFun[181] = AFN0D182;
    AFN0DFun[182] = AFN0D183;
    AFN0DFun[183] = AFN0D184;
    
    //��24
    AFN0DFun[184] = AFN0D185;
    AFN0DFun[185] = AFN0D186;
    AFN0DFun[186] = AFN0D187;
    AFN0DFun[187] = AFN0D188;
    AFN0DFun[188] = AFN0D189;
    AFN0DFun[189] = AFN0D190;
    AFN0DFun[190] = AFN0D191;
    AFN0DFun[191] = AFN0D192;
    
    //��25
    AFN0DFun[192] = AFN0D193;
    AFN0DFun[193] = AFN0D194;
    AFN0DFun[194] = AFN0D195;
    AFN0DFun[195] = AFN0D196;
    
    //��26
    AFN0DFun[200] = AFN0D201;
    AFN0DFun[201] = AFN0D202;
    AFN0DFun[202] = AFN0D203;
    AFN0DFun[203] = AFN0D204;
    AFN0DFun[204] = AFN0D205;
    AFN0DFun[205] = AFN0D206;
    AFN0DFun[206] = AFN0D207;
    AFN0DFun[207] = AFN0D208;
    
    //��27
    AFN0DFun[208] = AFN0D209;
    AFN0DFun[212] = AFN0D213;
    AFN0DFun[213] = AFN0D214;
    AFN0DFun[214] = AFN0D215;
    AFN0DFun[215] = AFN0D216;
    
    //��28
    AFN0DFun[216] = AFN0D217;
    AFN0DFun[217] = AFN0D218;

    AFN0DFun[220] = AFN0D221;
    
    if (poll == AFN0B_REQUIRE)
    {
       frameTail0d = 18;
    }
    else
    {
       frameTail0d = 14;
    }
    
    tmpDt1 = 0;
    tmpDtCount = 0;
    for (ackTail = 0; ackTail < 100; ackTail++)
    {
      ackData[ackTail] = 0;
    }
    ackTail = 0;
    
    frameLoadLen = frame.loadLen;
    if (poll==ACTIVE_REPORT)
    {
    	 frameLoadLen = pDataEnd - pDataHead;
    	 fQueue.active0dDataSending = 0;
    	 taskNum = *(pDataEnd+1);
    	 
       hasCurveVisionData = TRUE;   //������ʾֵ����
    }
    
    if (debugInfo&PRINT_ACTIVE_REPORT)
    {
      printf("AFN0D In-Frame:");
      pTmpFrame = pDataHead;
      while(pTmpFrame!=pDataEnd)
      {
    	  printf("%02X ", *pTmpFrame++);
      }
      printf("\n");
    }
    
    
    if (poll==ACTIVE_REPORT)
    {
    	 activeThreadTimeOut = nextTime(sysTime,60,0);    //��Ϊ�̵߳ĳ�ʱʱ��
    }
    thisThreadSending = 0;
    fQueue.active0dTaskId = 0;
    maxCycle = 0;
    while ((frameLoadLen > 0) && (maxCycle<1500))
    {
    	maxCycle++;

    	//printf("�����=%d,loadLen=%d, maxCycle=%d,offset0d=%d,frameTail0d=%d\n",taskNum,frameLoadLen, maxCycle,offset0d,frameTail0d);
    	
    	offset0d = 0;
      tmpDt1 = *(pDataHead+2);
      tmpDtCount = 0;
      while(tmpDtCount<9)
      {
       	if (poll == ACTIVE_REPORT)
       	{
     	  	//��Ϊ�߳���������·��ͨʱ�ᴦ��һ���ȴ�ͨ�ŵ�״̬,�̻߳�Խ��Խ��,,����һ������
     	  	//�����Ϊ�߳�ʱһ��Сʱ����û���,ֱ���˳��߳�
     	  	if (compareTwoTime(activeThreadTimeOut, sysTime))
     	  	{
     	  		 return;
     	  	}

       		if (thisThreadSending>0)
       		{
         		//�����ݵ�һ�ζ���û��
         		if (fQueue.active0dDataSending==0)
         		{
               //�ŵ�����,����
               //if (fQueue.delay==0 && (fQueue.active0dTaskId==taskNum || fQueue.active0dTaskId==0) && wlModemFlag.permitSendData==TRUE && wlModemFlag.loginSuccess==TRUE)
       				 if (fQueue.active0dDataSending==0  && fQueue.delay==0 && fQueue.inTimeFrameSend==FALSE && fQueue.activeFrameSend==FALSE && fQueue.active0dTaskId==0 && wlModemFlag.permitSendData==TRUE && wlModemFlag.loginSuccess==TRUE)
               {
       				   thisThreadSending = TRUE;
       				   fQueue.active0dDataSending = 1;
       				   fQueue.active0dTaskId = taskNum;
       				  
       				   fQueue.delay = 1;
  	 	           fQueue.delayTime = nextTime(sysTime, 0, 15);   //��ʱ15����ٷ���һ֡

       				   memcpy(bakDataBuff, tmpDataBuff, frameTail0d+2);
       			     sendToMs(tmpDataBuff, frameTail0d+2);
    	 	                        
                 if (debugInfo&PRINT_ACTIVE_REPORT)
                 {
                   printf("AFN0D�����ϱ�(�����=%d):��һ�η��ͱ�֡,����=%d\n",taskNum, frameTail0d+2);
                 }
                 
                 #ifdef RECORD_LOG
                  strcpy(logStr,"");
                  sprintf(logStr,"AFN0D�����ϱ�(�����=%d):��һ�η��ͱ�֡,����=%d:",taskNum,frameTail0d+2);
                  for(i=0;i<frameTail0d+2;i++)
                  {
                  	strcpy(sayStr,"");
                  	sprintf(sayStr,"%02x ",tmpDataBuff[i]);
                  	strcat(logStr,sayStr);
                  }
                  logRun(logStr);
                 #endif
               }
               
               usleep(500000);
               continue;
         		}
         		
       		  if (fQueue.active0dTaskId==taskNum)
       		  {
         		  //���͵�һ�κ�ĵȴ�
         		  if (fQueue.active0dDataSending==1)
         		  {
         			   usleep(500000);
         			   continue;
         		  }
         		
           		//û�ȵ���һ�η��͵�ȷ��,�ط�
           		if (fQueue.active0dDataSending==2)
           		{
                 //�ŵ�����,����
                 if (fQueue.delay==0 && (fQueue.active0dTaskId==taskNum || fQueue.active0dTaskId==0) && wlModemFlag.permitSendData==TRUE && wlModemFlag.loginSuccess==TRUE)
                 {
                   fQueue.active0dDataSending = 3;
                   fQueue.active0dTaskId = taskNum;
           			   fQueue.delay = 1;
      	 	         fQueue.delayTime = nextTime(sysTime, 0, 15);   //��ʱ15����ٷ���һ֡
      	 	                        
                   if (debugInfo&PRINT_ACTIVE_REPORT)
                   {
                     printf("AFN0D�����ϱ�(�����=%d):�ڶ��η��ͱ�֡,����=%d\n",taskNum ,frameTail0d+2);
                   }
       				     
       				     memcpy(tmpDataBuff,bakDataBuff,frameTail0d+2);
           			   sendToMs(tmpDataBuff, frameTail0d+2);
           			   
           			   sendDebugFrame(tmpDataBuff, frameTail0d+2);
    
                   #ifdef RECORD_LOG
                    strcpy(logStr,"");
                    sprintf(logStr,"AFN0D�����ϱ�(�����=%d):�ڶ��η��ͱ�֡,����=%d:",taskNum,frameTail0d+2);
                    for(i=0;i<frameTail0d+2;i++)
                    { 
                    	strcpy(sayStr,"");
                    	sprintf(sayStr,"%02x ",tmpDataBuff[i]);
                    	strcat(logStr,sayStr);
                    }
                    logRun(logStr);
                   #endif
      	 	       }
                 
                 usleep(500000);
                 continue;
           		}
           		
           		//���͵ڶ��κ�ĵȴ�
           		if (fQueue.active0dDataSending==3)
           		{
                 usleep(500000);
                 continue;
           		}
           		
           		//��ʱ���յ���վȷ��
           		if (fQueue.active0dDataSending>3)
           		{
           			 fQueue.active0dDataSending = 0;
           			 fQueue.delay = 0;
           			 fQueue.active0dTaskId = 0;
           			 thisThreadSending = 0;
           			 frameTail0d = 14;
           			 
           			 if (debugInfo&PRINT_ACTIVE_REPORT)
           			 {
                   printf("AFN0D�����ϱ�(�����=%d):����\n",taskNum);
                 }
           		}
            }
            
            usleep(500000);
            continue;
          }
      	}
      	
      	tmpDtCount++;
        fn = 0;
        if ((tmpDt1 & 0x1) == 0x1)
        {
        	fn = *(pDataHead+3)*8 + tmpDtCount;
        	
        	if (debugInfo&PRINT_ACTIVE_REPORT)
        	{
        	  printf("AFN0D=%d\n", fn);
        	}

          //ִ�к���
          if (fn <= 221)
          {
            if (AFN0DFun[fn-1] != NULL)
            //2013-11-27,�ڸ�����Է��ּ������������ݲ��ϱ�,�����Զ�����������AFN0D-F247
            //           �������������������ʧ��,���۲컹���ֻ��쳣�˳�����
            //       �޸�Ϊ����2���ж�,�Է�ֹ����Խ������ֶδ���
            //           
            //if ((AFN0DFun[fn-1] != NULL) && (fn <= 221))
            {
              tmpFrameTail = AFN0DFun[fn-1](tmpDataBuff, frameTail0d, pDataHead, fn, &offset0d);
              if (tmpFrameTail == frameTail0d)
              {
           	    ackData[ackTail*5]   = *pDataHead;                         //DA1
           	    ackData[ackTail*5+1] = *(pDataHead+1);                     //DA2
           	    ackData[ackTail*5+2] = 0x1<<((fn%8 == 0) ? 7 : (fn%8-1));  //DT1
           	    ackData[ackTail*5+3] = (fn-1)/8;                           //DT2
           	    ackData[ackTail*5+4] = 0x03;                               //����Ч����
           	    ackTail++;
              }
              else
              {
        			  frameTail0d = tmpFrameTail;
              }
            }
          }
          else
          {
          	//2013-11-27,�������ݲ����͵���һԭ�������else�����offset0dû�и�ֵ
          	//           ��ɱ���AFN0D����һֱ�˲���while,��threadOfReport2һֱ���ڹ���״̬
          	//           ���ղ��ϵ����Ǹ���������ֱ������Խ����˳���������
          	offset0d = 3;
          }
        }
        tmpDt1 >>= 1;

        if (frameTail0d>MAX_OF_PER_FRAME || ((pDataHead+offset0d+4)== pDataEnd && tmpDtCount==8))
        {
        	//�����������ϱ������¼�����
          if (frame.acd==1 && (callAndReport&0x03)== 0x02 && frameTail0d > 16)
          {
     	      tmpDataBuff[frameTail0d++] = iEventCounter;
     	      tmpDataBuff[frameTail0d++] = nEventCounter;
          }

          //��������վҪ���ж��Ƿ�Я��TP
          if (frame.pTp != NULL)
          {
            //ly,2011-10-11,�޸���������ϱ�Tp��bug(ifΪtrue�Ĵ���)
            if (poll==ACTIVE_REPORT)
            {
              tmpDataBuff[frameTail0d++] = pfc++;
              tmpDataBuff[frameTail0d++] = hexToBcd(sysTime.second);
              tmpDataBuff[frameTail0d++] = hexToBcd(sysTime.minute);
              tmpDataBuff[frameTail0d++] = hexToBcd(sysTime.hour);
              tmpDataBuff[frameTail0d++] = hexToBcd(sysTime.day);
              tmpDataBuff[frameTail0d++] = 0x0;
            }
            else
            {
              pTpv = frame.pTp;
              tmpDataBuff[frameTail0d++] = *pTpv++;
              tmpDataBuff[frameTail0d++] = *pTpv++;
              tmpDataBuff[frameTail0d++] = *pTpv++;
              tmpDataBuff[frameTail0d++] = *pTpv++;
              tmpDataBuff[frameTail0d++] = *pTpv++;
              tmpDataBuff[frameTail0d++] = *pTpv;
            }
          }
                             
          tmpDataBuff[0] = 0x68;          //֡��ʼ�ַ�
          
          if (poll==ACTIVE_REPORT)
          {
            //tmpI = ((frameTail0d - 6) << 2) | 0x01;   //������վ����� 2010.06.17
            
            tmpI = ((frameTail0d - 6) << 2) | PROTOCOL_FIELD;
          }
          else
          {
            tmpI = ((frameTail0d - 6) << 2) | PROTOCOL_FIELD;
          }
          tmpDataBuff[1] = tmpI & 0xFF;   //L
          tmpDataBuff[2] = tmpI >> 8;
          tmpDataBuff[3] = tmpI & 0xFF;   //L
          tmpDataBuff[4] = tmpI >> 8; 
           
          tmpDataBuff[5] = 0x68;          //֡��ʼ�ַ�
          
          if (poll==ACTIVE_REPORT)
       	  {
       	   	tmpDataBuff[6] = 0xc4;        //DIR=1,PRM=1,������=0x4
       	  }
          else
          {
           	tmpDataBuff[6] = 0x88;        //�����ַ�10001000(DIR=1,PRM=0,������=0x8)
          }

          if (frame.acd==1 && (callAndReport&0x03)== 0x02)   //�����������ϱ������¼�����
          {
            tmpDataBuff[6] |= 0x20;
          }
     
          //��ַ
          tmpDataBuff[7]  = addrField.a1[0];
          tmpDataBuff[8]  = addrField.a1[1];
          tmpDataBuff[9]  = addrField.a2[0];
          tmpDataBuff[10] = addrField.a2[1];
            
          if (poll == ACTIVE_REPORT)
          {
            tmpDataBuff[11] = 0;
          }
          else
          {
            tmpDataBuff[11] = addrField.a3;
          }
          
          if (poll == AFN0B_REQUIRE)
          {
            tmpDataBuff[12] = 0x0B;
          }
          else
          {
            tmpDataBuff[12] = 0x0D;          //AFN
          }
             
          tmpDataBuff[13] = 0x0;
          if (frame.pTp != NULL)
          {
     	      tmpDataBuff[13] |= 0x80;
          }
            
          if (poll == AFN0B_REQUIRE)
          {
            tmpDataBuff[14] = *(frame.pData+2);
            tmpDataBuff[15] = *(frame.pData+3);
            tmpDataBuff[16] = *(frame.pData+4);
            tmpDataBuff[17] = *(frame.pData+5);
          }
          
          //frameTail0d++;
          tmpDataBuff[frameTail0d+1] = 0x16;
           
          if ((poll == AFN0B_REQUIRE && frameTail0d > 22 && frame.pTp==NULL)
          	 ||((poll != AFN0B_REQUIRE && frameTail0d > 23 && frame.pTp ==NULL)
         	   ||(frameTail0d > 29 && frame.pTp!=NULL)))
          {
            if (poll==ACTIVE_REPORT)
            {
           	  tmpDataBuff[13] |= 0x70;       //�����ϱ�֡Ϊ��֡����Ҫ�Ը�֡���Ľ���ȷ��,ly,2010-10-12,add,��ǰ��Ϊ����ȷ�Ĵ���
           	  tmpDataBuff[13] |= pSeq&0xf;
           	  pSeq++;
             
              tmpI = 6;
              checkSum = 0;
              while (tmpI < frameTail0d)
              {
                checkSum = tmpDataBuff[tmpI]+checkSum;
                tmpI++;
              }
              tmpDataBuff[frameTail0d] = checkSum;
              
       				//���߳�ʵ��������Ҫ����
       				thisThreadSending = 1;

       				//ly,2011-10-11,modify
       				// if (fQueue.active0dDataSending==0 && fQueue.delay==0 && fQueue.active0dTaskId==0 && wlModemFlag.permitSendData==TRUE && wlModemFlag.loginSuccess==TRUE)
       				if (fQueue.active0dDataSending==0  && fQueue.delay==0 && fQueue.inTimeFrameSend==FALSE && fQueue.activeFrameSend==FALSE && fQueue.active0dTaskId==0 && wlModemFlag.permitSendData==TRUE && wlModemFlag.loginSuccess==TRUE)
       				{

       				  thisThreadSending = TRUE;
       				  fQueue.active0dDataSending = 1;
       				  fQueue.active0dTaskId = taskNum;
       				  
       				  fQueue.delay = 1;
  	 	          fQueue.delayTime = nextTime(sysTime, 0, 15);   //��ʱ15����ٷ���һ֡
                
                memcpy(bakDataBuff, tmpDataBuff, frameTail0d+2);
       			    
       			    sendToMs(tmpDataBuff, frameTail0d+2);

       				  if (debugInfo&PRINT_ACTIVE_REPORT)
       				  {
       				    printf("AFN0D�����ϱ�(�����=%d):��֯���ݺ��ͱ�֡��һ��,����=%d\n",taskNum,frameTail0d+2);
       				  }
                
                #ifdef RECORD_LOG
                 strcpy(logStr,"");
                 sprintf(logStr,"AFN0D�����ϱ�(�����=%d):��֯���ݺ��ͱ�֡��һ��,����=%d:",taskNum,frameTail0d+2);
                 for(i=0;i<frameTail0d+2;i++)
                 {
                	 strcpy(sayStr,"");
                	 sprintf(sayStr,"%02x ",tmpDataBuff[i]);
                	 strcat(logStr,sayStr);
                 }
                 logRun(logStr);
                #endif
       				}
            }
            else
            {
              if (fQueue.tailPtr == 0)
              {
                 tmpHead0d = 0;
              }
              else
              {
                 tmpHead0d = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
              }
              fQueue.frame[fQueue.tailPtr].head = tmpHead0d;
              fQueue.frame[fQueue.tailPtr].len  = frameTail0d + 2;
              
              //��������
              memcpy(&msFrame[fQueue.frame[fQueue.tailPtr].head], tmpDataBuff, fQueue.frame[fQueue.tailPtr].len);

          		if ((tmpHead0d+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
              	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
              {
                fQueue.frame[fQueue.tailPtr].next = 0x0;
          			fQueue.tailPtr = 0;
          			tmpHead0d = 0;
              }
              else
              {
                fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
                fQueue.tailPtr++;
              }
           }               

           if (poll == AFN0B_REQUIRE)
           {
              frameTail0d = 18;  //frameTail������λ��д��һ֡
           }
           else
           {
              if (poll!=ACTIVE_REPORT)
              {
                frameTail0d = 14;  //frameTail������λ��д��һ֡
              }
           }
          }
        }
        
        if (poll==ACTIVE_REPORT)
        {
          usleep(1000);
        }
      }
       
      pDataHead += (offset0d + 4);
      if (frameLoadLen < offset0d)
      {
        break;
      }
      else
      {
        frameLoadLen -= (offset0d + 4);
      }
    }
    
    if (ackTail!=0 && poll!=ACTIVE_REPORT)
    {
       AFN00003(ackTail, dataFrom, 0x0D);
    }
    
    if (poll==ACTIVE_REPORT)
    {
      //�в�����û������ʾֵ����(1Сʱ��û��������,������Ƿ񳭱��Ӳ������,��λ����?)
    	if (hasCurveVisionData!=TRUE)
    	{
    	  ifReset = TRUE;
    	}    	 
    }  
}

#ifdef LIGHTING

/*************************************************************
��������:AFN0D149
��������:��Ӧ��վ�����������"�ն��ᵱ�����һ�ο���ʱ��"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
�޸���ʷ:
*************************************************************/
INT16U AFN0D149(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U               dataBuff[512];
	INT8U               da1, da2;
	INT16U              pn, tmpPn = 0;
	//INT8U               i, tariff;
	//INT8U               queryType, dataType, tmpDay;
	DATE_TIME           tmpTime, readTime;
	INT16U              tmpFrameTail;
	//INT8U               meterInfo[10];
  //BOOL                bufHasData;

	tmpFrameTail = frameTail;
	da1 = *pHandle++;
	da2 = *pHandle++;
	pHandle += 2;
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x1) == 0x1)
		{
			pn = tmpPn + (da2 - 1) * 8;
      
			buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
			buff[frameTail++] = (pn - 1) / 8 + 1;												//DA2
			switch(fn)
			{
				case 149:	//�ն��������й�����ʾֵ
					buff[frameTail++] = 0x10;																//DT1
					buff[frameTail++] = 0x12;															  //DT2
					break;

				case 150:	//�ն��������޹�����ʾֵ
					buff[frameTail++] = 0x20;																//DT1
					buff[frameTail++] = 0x12;																//DT2
					break;
										
				case 151:	//�ն��ᷴ���й�����ʾֵ
					buff[frameTail++] = 0x40;																//DT1
					buff[frameTail++] = 0x12;																//DT2
					break;
				
				case 152:	//�ն��ᷴ���޹�����ʾֵ
					buff[frameTail++] = 0x80;																//DT1
					buff[frameTail++] = 0x12;																//DT2
					break;
			}
			
			*offset0d = 3;

			//����������ʱ��
			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour   = 0x23;
			buff[frameTail++] = tmpTime.day   = *(pHandle+0);
			buff[frameTail++] = tmpTime.month = *(pHandle+1);
			buff[frameTail++] = tmpTime.year  = *(pHandle+2);
			
			readTime = tmpTime;
      if (readSlDayData(pn, (fn-148), readTime, dataBuff)==TRUE)
			{
				memcpy(&buff[frameTail], &dataBuff[1] ,5);
			}
			else
			{
				memset(&buff[frameTail], 0xee ,5);
			}
		  frameTail += 5;
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*************************************************************
��������:AFN0D150
��������:��Ӧ��վ�����������"�ն��ᵱ�����һ�ι��Сʱ��"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D150(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D149(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D151
��������:��Ӧ��վ�����������"�ն��ᵱ�����һ�ι������ʱ��"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D151(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D149(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D152
��������:��Ӧ��վ�����������"�ն��ᵱ�����һ�ιص�ʱ��"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D152(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D149(buff, frameTail, pHandle, fn, offset0d);
}

#endif


/*******************************************************
��������:AFN0D001
��������:��Ӧ��վ�����������"�ն���������/�޹�����ʾֵ��
          һ/�������޹�����ʾֵ"������ݸ�ʽ14��11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
�޸���ʷ:
	1.2011-08-27,add,���������ն���ʾֵû�еĻ�,�õ������һ�γɹ��ĳ�����������Ϊ�ն���(��F1��F2)
  2.2012-05-23,add,���������ն���ʾֵû�еĻ�,�õ������һ�γɹ��ĳ�����������Ϊ�ն���(��F1��F2)
  3.2012-05-24,�޸�,��Щ�����(���չ��õĻ���)�صĲ�������Ϊ0xff,�����Բ�����ȷ������,���Ե�0xEE����
*******************************************************/
INT16U AFN0D001(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
  INT16U    pn, tmpPn=0;
  INT8U     tariff, dataType, queryType;
  INT8U     da1, da2;
  INT8U     tmpDay, i, j;
  BOOL      ifHasData, bufHasData;
  INT16U    offset;
  DATE_TIME tmpTime, readTime;
  INT8U     meterInfo[10];
  INT8U     pulsePnData = 0;  

  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
    
  if(da1==0x0)
  {
  	return frameTail;
  }
    
  while(tmpPn < 9)
  {
   	tmpPn++;
   	if((da1 & 0x01) == 0x01)
	  {
	  	pn = tmpPn + (da2 - 1) * 8;
      
      pulsePnData = 0;
      
  		#ifdef PULSE_GATHER
	 	   //�鿴�Ƿ����������������
	  	 if (fn==1 || fn==2)
	  	 {
	        for(j=0;j<NUM_OF_SWITCH_PULSE;j++)
	  	 	  {
	  	 	    //���������Ĳ�����
	  	 	    if (pulse[j].ifPlugIn==TRUE && pulse[j].pn==pn)
	  	 	    {
	   	  	 	  pulsePnData = 1;
	   	  	 	  break;
			 	  	}
			 	  }
	  	 }
	  	#endif

		  tmpTime.second = 0x59;
		  tmpTime.minute = 0x59;
		  tmpTime.hour   = 0x23;
		  if ((fn >= 1 && fn <= 2)||(fn >= 9 && fn <= 10))
		  {
		    *offset0d = 3;
		    tmpTime.day   = *(pHandle+0);   //��
		    tmpTime.month = *(pHandle+1);   //��
		    tmpTime.year  = *(pHandle+2);   //��
		  }
		  else
		  {
		    if(fn >= 17 && fn <= 18)
		    {
		  	  *offset0d = 2;
			    tmpDay = monthDays(2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			    tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
		      tmpTime.month = *(pHandle+0);   //��
		      tmpTime.year  = *(pHandle+1);   //��
		    }
		  }
		  
      queryMeterStoreInfo(pn, meterInfo);

		  buff[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
		  buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
		  switch (fn)
		  {
		   	case 1:
		     	buff[frameTail++] = 0x01;																//DT1
			    buff[frameTail++] = 0x00;																//DT2

          switch (meterInfo[0])
          {
    			  case 1:    //�������ܱ�
    			    queryType = SINGLE_PHASE_DAY;
    	        break;
    	        
    	      case 2:    //���౾�طѿر�
    	      	queryType = SINGLE_LOCAL_CTRL_DAY;
    	      	break;
    
    	      case 3:    //����Զ�̷ѿر�
    	      	queryType = SINGLE_REMOTE_CTRL_DAY;
    	      	break;
	      	
      	    case 4:
      	    	queryType = THREE_DAY;
      	    	break;
      	    	
      	    case 5:
      	      queryType = THREE_LOCAL_CTRL_DAY;
      	      break;
      	      
      	    case 6:
      	      queryType = THREE_REMOTE_CTRL_DAY;
      	      break;

      	    case 8:
      	      queryType = KEY_HOUSEHOLD_DAY;
      	      break;
      	      
      	    default:
			        queryType = DAY_BALANCE;
			        break;
			    }
			    
			    if (meterInfo[0]<4)
			    {
    	      dataType  = ENERGY_DATA;
			    }
			    else
			    {
			      dataType  = DAY_FREEZE_COPY_DATA;
			    }
			    break;

			  case 2:
			  	buff[frameTail++] = 0x02;																//DT1
			    buff[frameTail++] = 0x00;																//DT2
          
          switch (meterInfo[0])
          {
    			  case 1:    //�������ܱ�
    			    queryType = SINGLE_PHASE_DAY;
    	        break;
    	        
    	      case 2:    //���౾�طѿر�
    	      	queryType = SINGLE_LOCAL_CTRL_DAY;
    	      	break;
    
    	      case 3:    //����Զ�̷ѿر�
    	      	queryType = SINGLE_REMOTE_CTRL_DAY;
    	      	break;

      	    case 4:
      	    	queryType = THREE_DAY;
      	    	break;
      	    	
      	    case 5:
      	      queryType = THREE_LOCAL_CTRL_DAY;
      	      break;
      	      
      	    case 6:
      	      queryType = THREE_REMOTE_CTRL_DAY;
      	      break;

      	    case 8:
      	      queryType = KEY_HOUSEHOLD_DAY;
      	      break;
      	      
      	    default:
			        queryType = DAY_BALANCE;
			        break;
			    }
			    
			    if (meterInfo[0]<4)
			    {
    	      dataType  = ENERGY_DATA;
			    }
			    else
			    {
			      dataType  = DAY_FREEZE_COPY_DATA;
			    }
			    break;
			    
		   	case 9:
		     	buff[frameTail++] = 0x01;																//DT1
			    buff[frameTail++] = 0x01;																//DT2
			    queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
			    break;

			  case 10:
			  	buff[frameTail++] = 0x02;																//DT1
			    buff[frameTail++] = 0x01;																//DT2
			    queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
			    break;
			    
			  case 17:
			   	buff[frameTail++] = 0x01;																//DT1
			   	buff[frameTail++] = 0x02;																//DT2
          
          switch (meterInfo[0])
          {
    			  case 1:    //�������ܱ�
    			    queryType = SINGLE_PHASE_MONTH;
    	        break;
    	        
    	      case 2:    //���౾�طѿر�
    	      	queryType = SINGLE_LOCAL_CTRL_MONTH;
    	      	break;
    
    	      case 3:    //����Զ�̷ѿر�
    	      	queryType = SINGLE_REMOTE_CTRL_MONTH;
    	      	break;

      	    case 4:
      	    	queryType = THREE_MONTH;
      	    	break;
      	    	
      	    case 5:
      	      queryType = THREE_LOCAL_CTRL_MONTH;
      	      break;
      	      
      	    case 6:
      	      queryType = THREE_REMOTE_CTRL_MONTH;
      	      break;

      	    case 8:
      	      queryType = KEY_HOUSEHOLD_MONTH;
      	      break;
      	      
      	    default:
			        queryType = MONTH_BALANCE;
			        break;
			    }
			    
			    if (meterInfo[0]<4)
			    {
    	      dataType  = ENERGY_DATA;
			    }
			    else
			    {
			      dataType  = MONTH_FREEZE_COPY_DATA;
			    }
			   	break;

			  case 18:
			   	buff[frameTail++] = 0x02;																//DT1
			   	buff[frameTail++] = 0x02;																//DT2
			   	
          switch (meterInfo[0])
          {
    			  case 1:    //�������ܱ�
    			    queryType = SINGLE_PHASE_MONTH;
    	        break;
    	        
    	      case 2:    //���౾�طѿر�
    	      	queryType = SINGLE_LOCAL_CTRL_MONTH;
    	      	break;
    
    	      case 3:    //����Զ�̷ѿر�
    	      	queryType = SINGLE_REMOTE_CTRL_MONTH;
    	      	break;

      	    case 4:
      	    	queryType = THREE_MONTH;
      	    	break;
      	    	
      	    case 5:
      	      queryType = THREE_LOCAL_CTRL_MONTH;
      	      break;
      	      
      	    case 6:
      	      queryType = THREE_REMOTE_CTRL_MONTH;
      	      break;
      	      
      	    case 8:
      	      queryType = KEY_HOUSEHOLD_MONTH;
      	      break;
      	      
      	    default:
			        queryType = MONTH_BALANCE;
			        break;
			    }
			    
			    if (meterInfo[0]<4)
			    {
    	      dataType = ENERGY_DATA;
			    }
			    else
			    {
			      dataType = MONTH_FREEZE_COPY_DATA;
			    }
			   	break;
			}
			
		  ifHasData = FALSE;
		  bufHasData = FALSE;
		  
		  //��÷��ʸ���
		  tariff = numOfTariff(pn);
       
      readTime = tmpTime;
		  bufHasData = readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0);
		  
		  //����ն���ʾֵû�еĻ�,�õ������һ�γɹ��ĳ�����������Ϊ�ն���
		  //2011-08-27,add,�����Ĵ���
		  //2012-05-23,add,�����(485�ڻ����ز��˿�)�Ĵ���
		  if (bufHasData==FALSE && (fn==1 || fn==2))
		  {
        readTime = tmpTime;
        if (meterInfo[0]>0 && meterInfo[0]<4)
	      {
	      	bufHasData = readMeterData(dataBuff, pn, meterInfo[1], ENERGY_DATA, &readTime, 0);
		      if (bufHasData==FALSE)
	      	{
	      		//readTime = timeBcdToHex(tmpTime);
	      		//2012-12-10,�޸�
	      		readTime = tmpTime;
	      		bufHasData = readBakDayFile(pn, &readTime, dataBuff, 1);
	      	}
	      }
	      else
	      {
		      bufHasData = readMeterData(dataBuff, pn, LAST_TODAY, ENERGY_DATA, &readTime, 0);
		      
		      if (bufHasData==FALSE)
	      	{
	      		//readTime = timeBcdToHex(tmpTime);
	      		//2012-12-10,�޸�
	      		readTime = tmpTime;
	      		bufHasData = readBakDayFile(pn, &readTime, dataBuff, DAY_FREEZE_COPY_DATA);
	      	}
		    }
		  }

		  if (bufHasData==TRUE)
		  {
		  	//���ݶ���ʱ��--���ݸ�ʽ20,21
		  	if(fn==1 || fn==2 || fn==9 || fn==10)
			  {
			    buff[frameTail++] = tmpTime.day;   //��
			  }
			  buff[frameTail++] = tmpTime.month;   //��
			  buff[frameTail++] = tmpTime.year;    //��
						
			  //�ն˳���ʱ��--���ݸ�ʽ15(��ʱ������)
			  buff[frameTail++] = readTime.minute;
			  buff[frameTail++] = readTime.hour;
			  buff[frameTail++] = readTime.day;		    
			  buff[frameTail++] = readTime.month;
			  buff[frameTail++] = readTime.year;
				
			  //������
			  buff[frameTail++] = tariff;
		  	
		  	//�����й�����ʾֵ(��,����1~M)--���ݸ�ʽ14
		  	if(fn==1 || fn==9 || fn==17)
		  	{
		  		offset = POSITIVE_WORK_OFFSET;
		  	}
		  	else	//F2,F10,F18
		  	{
		  		if (meterInfo[0]<4)
		  		{
		  		  offset = NEGTIVE_WORK_OFFSET_S;
		  		}
		  		else
		  		{
		  		  offset = NEGTIVE_WORK_OFFSET;
		  		}
		  	}
			 	for(i = 0; i <= tariff; i++)
			 	{
			   	//2012-05-24,add,0xff�Ĵ���
			   	if((dataBuff[offset] != 0xEE) && (dataBuff[offset]!=0XFF))
			   	{
			  		ifHasData = TRUE;
			   		
            if (pulsePnData==1)
            {
              buff[frameTail++] = dataBuff[offset++];
            }
            else
            {
              buff[frameTail++] = 0x0;
            }
			  	}
			   	else
			   	{
			  		buff[frameTail++] = 0xEE;
			   	}
			   	
				  //2012-05-24,add,0xff�Ĵ���
		   	  if (dataBuff[offset]==0xff)
			   	{
			   		buff[frameTail++] = 0xee;
			   		buff[frameTail++] = 0xee;
			   		buff[frameTail++] = 0xee;
			   		buff[frameTail++] = 0xee;
			   	}
			   	else
			   	{
			   	  buff[frameTail++] = dataBuff[offset++];
			   	  buff[frameTail++] = dataBuff[offset++];
			   	  buff[frameTail++] = dataBuff[offset++];
			   	  buff[frameTail++] = dataBuff[offset++];
			   	}
			   	
			   	//�����ʵĴ���,����1����һ��
			   	if (tariff==1)
			   	{
			   		buff[frameTail++] = buff[frameTail-5];
			   		buff[frameTail++] = buff[frameTail-5];
			   		buff[frameTail++] = buff[frameTail-5];
			   		buff[frameTail++] = buff[frameTail-5];
			   		buff[frameTail++] = buff[frameTail-5];
			   		i++;
			   	}
			 	}

        if ((fn==1 || fn==2 || fn==17 || fn==18) && (meterInfo[0]==8 || meterInfo[0]<4))
        {
        	 memset(&buff[frameTail],0xee,(tariff+1)*12);
        	 frameTail += (tariff+1)*12;
        }
        else
        {
  			  //�����޹�����ʾֵ(��,����1~M)--���ݸ�ʽ11
  			  if(fn==1 || fn==9 || fn==17)
  		  	{
  		  		offset = POSITIVE_NO_WORK_OFFSET;
  		  	}
  		  	else	//F2,F10,F18
  		  	{
  		  		offset = NEGTIVE_NO_WORK_OFFSET;
  		  	}
  			  for(i = 0; i <= tariff; i++)
  			  {
  			   	if((dataBuff[offset]!=0xEE) && (dataBuff[offset]!=0xFF))
  			  	{
  			  		ifHasData = TRUE;
  			   	}
  			   	
            if (pulsePnData==1)
            {
            	offset++;
            }
  			   	
  			   	if (dataBuff[offset]==0xff)
  			   	{
  			   		buff[frameTail++] = 0xee;
  			   		buff[frameTail++] = 0xee;
  			   		buff[frameTail++] = 0xee;
  			   		buff[frameTail++] = 0xee;
  			   	}
  			   	else
  			   	{
  			   	  buff[frameTail++] = dataBuff[offset++];
  			   	  buff[frameTail++] = dataBuff[offset++];
  			  	  buff[frameTail++] = dataBuff[offset++];
  			   	  buff[frameTail++] = dataBuff[offset++];
  			   	}
  
  			   	//�����ʵĴ���,����1����һ��
  			   	if (tariff==1)
  			   	{
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		i++;
  			   	}
  			  }
  			      	
  			  //һ�����޹�����ʾֵ(��,����1~M)--���ݸ�ʽ11
  			  if(fn == 1 || fn == 9 || fn == 17)
  		  	{
  		  		offset = QUA1_NO_WORK_OFFSET;
  		  	}
  		  	else	//F2,F10,F18
  		  	{
  		  		offset = QUA2_NO_WORK_OFFSET;
  		  	}
  			  for(i = 0; i <= tariff; i++)
  			  {
  			   	if(dataBuff[offset] != 0xEE)
  			   	{
  			   		ifHasData = TRUE;
  			   	}
  			   	buff[frameTail++] = dataBuff[offset++];
  			   	buff[frameTail++] = dataBuff[offset++];
  			   	buff[frameTail++] = dataBuff[offset++];
  			   	buff[frameTail++] = dataBuff[offset++];
  
  			   	//�����ʵĴ���,����1����һ��
  			   	if (tariff==1)
  			   	{
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		i++;
  			   	}
  			  }
  			      	
  			  //�������޹�����ʾֵ(��,����1~M)--���ݸ�ʽ11
  			  if(fn == 1 || fn == 9 || fn == 17)
  		  	{
  		  		offset = QUA4_NO_WORK_OFFSET;
  		  	}
  		  	else	//F2,F10,F18
  		  	{
  		  		offset = QUA3_NO_WORK_OFFSET;
  		  	}
  			  for(i = 0; i <= tariff; i++)
  			  {
  			   	if(dataBuff[offset] != 0xEE)
  			   	{
  			   		ifHasData = TRUE;
  			   	}
  			   	buff[frameTail++] = dataBuff[offset++];
  			   	buff[frameTail++] = dataBuff[offset++];
  			   	buff[frameTail++] = dataBuff[offset++];
  			   	buff[frameTail++] = dataBuff[offset++];
  
  			   	//�����ʵĴ���,����1����һ��
  			   	if (tariff==1)
  			   	{
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		buff[frameTail++] = buff[frameTail-4];
  			   		i++;
  			   	}
  			  }
  			      	
  			  #ifdef NO_DATA_USE_PART_ACK_03
  			    if (ifHasData == FALSE)
  			  	{
  			  		if((fn >= 1 && fn <= 2) ||(fn >= 9 && fn <= 10))
  			  		{
  			  			frameTail -= 13 + (tariff + 1) * 17;
  			  		}
  			  		else
  			  		{
  			  			frameTail -= 12 + (tariff + 1) * 17;
  			  		}
  			   	}
  			 	#endif
  			}
			}
			else
			{
			 	#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail -= 4;
			  #else
				  if ((fn >= 1 && fn <= 2) || (fn >= 9 && fn <= 10))
				  {
				    buff[frameTail++] = tmpTime.day;   //��
				  }
				  buff[frameTail++] = tmpTime.month;   //��
				  buff[frameTail++] = tmpTime.year;    //��
							
				  //�ն˳���ʱ��--���ݸ�ʽ15(��ʱ������)
				  buff[frameTail++] = sysTime.minute / 10 << 4 | sysTime.minute % 10;
				  buff[frameTail++] = sysTime.hour / 10 << 4 | sysTime.hour % 10;
				  buff[frameTail++] = sysTime.day / 10 << 4 | sysTime.day % 10;
				  buff[frameTail++] = sysTime.month / 10 << 4 | sysTime.month % 10;
				  buff[frameTail++] = sysTime.year / 10 << 4 | sysTime.year % 10;
					
				  //������
				  buff[frameTail++] = tariff;
				  
			    for(i = 0; i < (tariff + 1) * 17; i++)
			    {
			   	  buff[frameTail++] = 0xee;
			    }
			  #endif
			}
	  }
    da1 >>= 1;
  }
    
  return frameTail;
}

/*******************************************************
��������:AFN0D002
��������:��Ӧ��վ�����������"�ն��ᷴ����/�޹�������ʾֵ��
          ��/�������޹�������ʾֵ"������ݸ�ʽ14��11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D002(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D001(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
��������:AFN0D003
��������:��Ӧ��վ�����������"�ն���������/�޹��������
         ������ʱ��"������ݸ�ʽ23��17��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D003(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LENGTH_OF_REQ_RECORD];
	INT16U pn, tmpPn = 0;
  INT8U tariff, dataType, queryType;
  INT8U da1, da2;
  INT8U i, tmpDay;
  BOOL  ifHasData, bufHasData;
  
  INT16U offset;
    
  DATE_TIME tmpTime, readTime;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x01) == 0x01)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		
  		tmpTime.second = 0x59;
	    tmpTime.minute = 0x59;
	    tmpTime.hour   = 0x23;
	    if (fn==3 || fn==4 || fn==11 || fn==12)
	    {
	      *offset0d = 3;
	      tmpTime.day   = *(pHandle+0);   //��
	      tmpTime.month = *(pHandle+1);   //��
	      tmpTime.year  = *(pHandle+2);   //��
	    }
	    else	//F19,F20
	    {
		    *offset0d = 2;
			  tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
	      tmpTime.month = *(pHandle+0);   //��
	      tmpTime.year  = *(pHandle+1);   //��
	    }
	    
	    buff[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
		  buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
		  switch (fn)
		  {
		   	case 3:
		     	buff[frameTail++] = 0x04;																//DT1
			    buff[frameTail++] = 0x00;																//DT2
			    queryType = DAY_BALANCE;
			    dataType  = DAY_FREEZE_COPY_REQ;
			    break;

			  case 4:
			  	buff[frameTail++] = 0x08;																//DT1
			    buff[frameTail++] = 0x00;																//DT2
			    queryType = DAY_BALANCE;
			    dataType  = DAY_FREEZE_COPY_REQ;
			    break;

		   	case 11:
		     	buff[frameTail++] = 0x04;																//DT1
			    buff[frameTail++] = 0x01;																//DT2
			    queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_REQ;
			    break;

			  case 12:
			  	buff[frameTail++] = 0x08;																//DT1
			    buff[frameTail++] = 0x01;																//DT2
			    queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_REQ;
			    break;

			  case 19:
			   	buff[frameTail++] = 0x04;																//DT1
			   	buff[frameTail++] = 0x02;																//DT2
			   	queryType = MONTH_BALANCE;
			    dataType  = MONTH_FREEZE_COPY_REQ;
			   	break;

			  case 20:
			   	buff[frameTail++] = 0x08;																//DT1
			   	buff[frameTail++] = 0x02;																//DT2
			   	queryType = MONTH_BALANCE;
			    dataType  = MONTH_FREEZE_COPY_REQ;
			   	break;
			}
			
		  ifHasData = FALSE;
		  bufHasData = FALSE;
		  
		  //��÷��ʸ���
		  tariff = numOfTariff(pn);
			
			readTime = tmpTime;
		  bufHasData = readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0);
		  
		  if (bufHasData==FALSE && (fn==3 || fn==4))
		  {
		    readTime = tmpTime;
		    bufHasData = readMeterData(dataBuff, pn, LAST_TODAY, REQ_REQTIME_DATA, &readTime, 0);
		    
		    if (bufHasData==FALSE)
	      {
	      	readTime = timeBcdToHex(tmpTime);
	      	bufHasData = readBakDayFile(pn, &readTime, dataBuff, DAY_FREEZE_COPY_REQ);
	      }
		  }
		  
		  if (bufHasData==TRUE)
		  {
		  	//���ݶ���ʱ��--���ݸ�ʽ20,21
			  if (fn==3 || fn==4 || fn==11 || fn==12)
			  {
			    buff[frameTail++] = tmpTime.day;   //��
			  }
			  buff[frameTail++] = tmpTime.month;   //��
			  buff[frameTail++] = tmpTime.year;    //��
						
			  //�ն˳���ʱ��--���ݸ�ʽ15(��ʱ������)
			  buff[frameTail++] = readTime.minute;
			  buff[frameTail++] = readTime.hour;
			  buff[frameTail++] = readTime.day;		    
			  buff[frameTail++] = readTime.month;
			  buff[frameTail++] = readTime.year;
				
			  //������
			  buff[frameTail++] = tariff;  
		  	
		  	//�����й��������(��,����1~M)--���ݸ�ʽ23
		  	if(fn == 3 || fn == 11 || fn == 19)
		  	{
		  		offset = REQ_POSITIVE_WORK_OFFSET;
		  	}
		  	else	//F4,F12,F20  �����й��������(��,����1~M)--���ݸ�ʽ23
		  	{
		  		offset = REQ_NEGTIVE_WORK_OFFSET;
		  	}
			 	for(i = 0; i <= tariff; i++)
			 	{
			   	if(dataBuff[offset] != 0xEE)
			   	{
			  		ifHasData = TRUE;
			  	}
			   	buff[frameTail++] = dataBuff[offset++];
			   	buff[frameTail++] = dataBuff[offset++];
			   	buff[frameTail++] = dataBuff[offset++];
			   	
			   	//�����ʵĴ���,����1����һ��
			   	if (tariff==1)
			   	{
			   		buff[frameTail++] = buff[frameTail-3];
			   		buff[frameTail++] = buff[frameTail-3];
			   		buff[frameTail++] = buff[frameTail-3];
			   		i++;
			   	}
			 	}
			      	
			  //�����й������������ʱ��(��,����1~M)--���ݸ�ʽ17
			  if(fn == 3 || fn == 11 || fn == 19)
		  	{
		  		offset = REQ_TIME_P_WORK_OFFSET;
		  	}
		  	else	//F4,F12,F20
		  	{
		  		offset = REQ_TIME_N_WORK_OFFSET;
		  	}
			  for(i = 0; i <= tariff; i++)
			  {
			   	if(dataBuff[offset] != 0xEE)
			  	{
			  		ifHasData = TRUE;
			   	}
			   	buff[frameTail++] = dataBuff[offset++];
			   	buff[frameTail++] = dataBuff[offset++];
			  	buff[frameTail++] = dataBuff[offset++];
			   	buff[frameTail++] = dataBuff[offset++];

			   	//�����ʵĴ���,����1����һ��
			   	if (tariff==1)
			   	{
			   		buff[frameTail++] = buff[frameTail-4];
			   		buff[frameTail++] = buff[frameTail-4];
			   		buff[frameTail++] = buff[frameTail-4];
			   		buff[frameTail++] = buff[frameTail-4];
			   		
			   		i++;
			   	}
			   	else
			   	{
			   	  offset++;
			   	}
			  }
			      	
			  //�����޹��������(��,����1~M)--���ݸ�ʽ23
			  if(fn == 3 || fn == 11 || fn == 19)
		  	{
		  		offset = REQ_POSITIVE_NO_WORK_OFFSET;
		  	}
		  	else	//F4,F12,F20 �����޹��������(��,����1~M)--���ݸ�ʽ23
		  	{
		  		offset = REQ_NEGTIVE_NO_WORK_OFFSET;
		  	}
			  for(i = 0; i <= tariff; i++)
			  {
			   	if(dataBuff[offset] != 0xEE)
			   	{
			   		ifHasData = TRUE;
			   	}
			   	buff[frameTail++] = dataBuff[offset++];
			   	buff[frameTail++] = dataBuff[offset++];
			   	buff[frameTail++] = dataBuff[offset++];

			   	//�����ʵĴ���,����1����һ��
			   	if (tariff==1)
			   	{
			   		buff[frameTail++] = buff[frameTail-3];
			   		buff[frameTail++] = buff[frameTail-3];
			   		buff[frameTail++] = buff[frameTail-3];
			   		i++;
			   	}
			  }
			      	
			  //�����޹������������ʱ��(��,����1~M)--���ݸ�ʽ17
			  if(fn == 3 || fn == 11 || fn == 19)
		  	{
		  		offset = REQ_TIME_P_NO_WORK_OFFSET;
		  	}
		  	else	//F4,F12,F20 �����޹������������ʱ��(��,����1~M)--���ݸ�ʽ17
		  	{
		  		offset = REQ_TIME_N_NO_WORK_OFFSET;
		  	}
			  for(i = 0; i <= tariff; i++)
			  {
			   	if(dataBuff[offset] != 0xEE)
			   	{
			   		ifHasData = TRUE;
			   	}
			   	buff[frameTail++] = dataBuff[offset++];
			   	buff[frameTail++] = dataBuff[offset++];
			   	buff[frameTail++] = dataBuff[offset++];
			   	buff[frameTail++] = dataBuff[offset++];
			   	
			   	//�����ʵĴ���,����1����һ��
			   	if (tariff==1)
			   	{
			   		buff[frameTail++] = buff[frameTail-4];
			   		buff[frameTail++] = buff[frameTail-4];
			   		buff[frameTail++] = buff[frameTail-4];
			   		buff[frameTail++] = buff[frameTail-4];
			   		
			   		i++;
			   	}
			   	else
			   	{
			   	  offset++;
			   	}
			  }
			      	
			  #ifdef NO_DATA_USE_PART_ACK_03
			    if (ifHasData == FALSE)
			  	{
			  		if(fn==3 || fn==4 || fn==11 || fn==12)
			  		{
			  			frameTail -= 13 + (tariff + 1) * 14;
			  		}
			  		else
			  		{
			  			frameTail -= 12 + (tariff + 1) * 14;
			  		}
			   	}
			 	#endif
			}
			else
			{
			 	#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail -= 4;
			  #else
			  	if(fn==3 || fn==4 || fn==11 || fn==12)
				  {
				    buff[frameTail++] = tmpTime.day;   //��
				  }
				  buff[frameTail++] = tmpTime.month;   //��
				  buff[frameTail++] = tmpTime.year;    //��
							
				  //�ն˳���ʱ��--���ݸ�ʽ15(��ʱ������)
				  buff[frameTail++] = sysTime.minute / 10 << 4 | sysTime.minute % 10;
				  buff[frameTail++] = sysTime.hour / 10 << 4 | sysTime.hour % 10;
				  buff[frameTail++] = sysTime.day / 10 << 4 | sysTime.day % 10;
				  buff[frameTail++] = sysTime.month / 10 << 4 | sysTime.month % 10;
				  buff[frameTail++] = sysTime.year / 10 << 4 | sysTime.year % 10;
					
				  //������
				  buff[frameTail++] = tariff;
				  
			    for(i = 0; i < (tariff + 1) * 14; i++)
			    {
			   	  buff[frameTail++] = 0xee;
			    }
			  #endif
			}
  	}
  	da1 >>= 1;
  }

	return frameTail;
}

/*******************************************************
��������:AFN0D004
��������:��Ӧ��վ�����������"�ն��ᷴ����/�޹��������
          ������ʱ��"������ݸ�ʽ23��17��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D004(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D003(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
��������:AFN0D005
��������:��Ӧ��վ�����������"�ն��������й�������"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D005(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U  dataBuff[LEN_OF_ENERGY_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U  tariff, dataType, queryType;
	INT8U  da1, da2;
	INT8U  i, tmpDay;
	INT16U offset;
	
  BOOL ifHasData;
  
  DATE_TIME tmpTime, readTime;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		
  		tmpTime.second = 0x59;
	    tmpTime.minute = 0x59;
	    tmpTime.hour   = 0x23;
	    if (fn >= 5 && fn <= 8)	//�ն���
	    {
	      *offset0d = 3;
	      tmpTime.day   = *(pHandle+0);   //��
	      tmpTime.month = *(pHandle+1);   //��
	      tmpTime.year  = *(pHandle+2);   //��
	    }
	    else	//f21~f24 �¶���
	    {
		    *offset0d = 2;
			  tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
	      tmpTime.month = *(pHandle+0);   //��
	      tmpTime.year  = *(pHandle+1);   //��
	    }
	    
	    buff[frameTail++] = 0x01<<((pn%8 == 0) ? 7 : (pn%8-1));  		//DA1
		  buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
		  switch (fn)
		  {
		   	case 5:
		     	buff[frameTail++] = 0x10;																//DT1
			    buff[frameTail++] = 0x00;																//DT2
			    queryType = DAY_BALANCE;
			    dataType = DAY_BALANCE_POWER_DATA;
			    offset = DAY_P_WORK_OFFSET;
			    break;
			    
			  case 6:
			  	buff[frameTail++] = 0x20;																//DT1
			    buff[frameTail++] = 0x00;																//DT2
			    queryType = DAY_BALANCE;
			    dataType = DAY_BALANCE_POWER_DATA;
			    offset = DAY_P_NO_WORK_OFFSET;
			    break;

			  case 7:
			   	buff[frameTail++] = 0x40;																//DT1
			   	buff[frameTail++] = 0x00;																//DT2
			   	queryType = DAY_BALANCE;
			    dataType = DAY_BALANCE_POWER_DATA;
			    offset = DAY_N_WORK_OFFSET;
			   	break;

			  case 8:
			   	buff[frameTail++] = 0x80;																//DT1
			   	buff[frameTail++] = 0x00;																//DT2
			   	queryType = DAY_BALANCE;
			    dataType = DAY_BALANCE_POWER_DATA;
			    offset = DAY_N_NO_WORK_OFFSET;
			   	break;

			  case 21:
		     	buff[frameTail++] = 0x10;																//DT1
			    buff[frameTail++] = 0x02;																//DT2
			    queryType = MONTH_BALANCE;
			    dataType = MONTH_BALANCE_POWER_DATA;
			    offset = MONTH_P_WORK_OFFSET;
			    break;

			  case 22:
			  	buff[frameTail++] = 0x20;																//DT1
			    buff[frameTail++] = 0x02;																//DT2
			    queryType = MONTH_BALANCE;
			    dataType = MONTH_BALANCE_POWER_DATA;
			    offset = MONTH_P_NO_WORK_OFFSET;
			    break;

			  case 23:
			   	buff[frameTail++] = 0x40;																//DT1
			   	buff[frameTail++] = 0x02;																//DT2
			   	queryType = MONTH_BALANCE;
			    dataType = MONTH_BALANCE_POWER_DATA;
			    offset = MONTH_N_WORK_OFFSET;
			   	break;

			  case 24:
			   	buff[frameTail++] = 0x80;																//DT1
			   	buff[frameTail++] = 0x02;																//DT2
			   	queryType = MONTH_BALANCE;
			    dataType = MONTH_BALANCE_POWER_DATA;
			    offset = MONTH_N_NO_WORK_OFFSET;
			   	break;
			}
			
		  ifHasData = FALSE;
		  
		  //��÷��ʸ���
		  tariff = numOfTariff(pn);
			
			readTime = tmpTime;      
		  if(readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0) == TRUE)
		  {
		  	//���ݶ���ʱ��--���ݸ�ʽ20,21
			  if (fn >= 5 && fn <= 8)
			  {
			    buff[frameTail++] = tmpTime.day;   //��
			  }
			  buff[frameTail++] = tmpTime.month;   //��
			  buff[frameTail++] = tmpTime.year;    //��
				
			  //������
			  buff[frameTail++] = tariff;  
		  	

			 	for(i = 0; i <= tariff; i++)
			 	{			   
			   #ifdef DKY_SUBMISSION    //2012-09-20,add
      	  if (dataBuff[offset] != 0xEE)
      	  {
      	   	ifHasData = TRUE;
      	    
      	    buff[frameTail++] = dataBuff[offset+1];
      	    buff[frameTail++] = dataBuff[offset+2];
    	      buff[frameTail++] = dataBuff[offset+3];
    	      buff[frameTail++] = dataBuff[offset+4];
    	    }
    	    else
    	    {
    	    	buff[frameTail++] = 0x00;
    	    	buff[frameTail++] = 0x00;
    	    	buff[frameTail++] = 0x00;
    	    	buff[frameTail++] = 0x00;
    	    }
			   #else
      	  if (dataBuff[offset] != 0xEE)
      	  {
      	   	ifHasData = TRUE;
      	  }
      	  buff[frameTail++] = dataBuff[offset+1];
      	  buff[frameTail++] = dataBuff[offset+2];
    	    buff[frameTail++] = dataBuff[offset+3];
    	    buff[frameTail++] = dataBuff[offset+4];
    	   #endif
    	    
    	    //�����ʵĴ���
    	    if (tariff==1)
    	    {
    	    	 buff[frameTail++] = buff[frameTail-4];
    	    	 buff[frameTail++] = buff[frameTail-4];
    	    	 buff[frameTail++] = buff[frameTail-4];
    	    	 buff[frameTail++] = buff[frameTail-4];

    	    	 i++;
    	    }
    	    else
    	    { 
    	      offset += 7;
    	    }
    	 }
			 	
			 	#ifdef NO_DATA_USE_PART_ACK_03
			    if (ifHasData == FALSE)
			  	{
			  		if(fn >= 5 && fn <= 8)
			  		{
			  			frameTail -= 8 + (tariff + 1) * 4;
			  		}
			  		else
			  		{
			  			frameTail -= 7 + (tariff + 1) * 4;
			  		}
			   	}
			 	#endif
			}
			else
			{
			 	#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail -= 4;
			  #else
				  if (fn >= 5 && fn <= 8)
				  {
				    buff[frameTail++] = tmpTime.day;   //��
				  }
				  buff[frameTail++] = tmpTime.month;   //��
				  buff[frameTail++] = tmpTime.year;    //��
					
				  //������
				  buff[frameTail++] = tariff;
				  
			    for(i = 0; i < (tariff + 1) * 4; i++)
			    {
			   	  //2012-09-20,add
			   	  #ifdef DKY_SUBMISSION
			   	   buff[frameTail++] = 0x00;
			   	  #else
			   	   buff[frameTail++] = 0xee;
			   	  #endif
			    }
			  #endif
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*******************************************************
��������:AFN0D006
��������:��Ӧ��վ�����������"�ն��������޹�������"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D006(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
 	 *offset0d = 7;	 
	 return AFN0D005(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
��������:AFN0D007
��������:��Ӧ��վ�����������"�ն��������޹�������"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D007(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
 	 *offset0d = 7;
	 return AFN0D005(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
��������:AFN0D008
��������:��Ӧ��վ�����������"�ն��������޹�������"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D008(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
 	 *offset0d = 7;
	 return AFN0D005(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
��������:AFN0D009
��������:��Ӧ��վ�����������"�����ն���������\�޹�����ʾֵ
         һ\�������޹�������ʾֵ"������ݸ�ʽ14��11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D009(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D001(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
��������:AFN0D010
��������:��Ӧ��վ�����������"�����ն��ᷴ����\�޹�����ʾֵ
         ��\�������޹�������ʾֵ"������ݸ�ʽ14��11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D010(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D001(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
��������:AFN0D011
��������:��Ӧ��վ�����������"�����ն���������\�޹���
         ������������ʱ��"������ݸ�ʽ23��17��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D011(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D003(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
��������:AFN0D012
��������:��Ӧ��վ�����������"�����ն��ᷴ����\�޹���
         ������������ʱ��"������ݸ�ʽ23��17��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D012(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D003(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
��������:AFN0D017
��������:��Ӧ��վ�����������"�¶���������/�޹�������ʾֵ��
          һ/�������޹�������ʾֵ"������ݸ�ʽ14��11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D017(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D001(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
��������:AFN0D018
��������:��Ӧ��վ�����������"�¶��ᷴ����/�޹�������ʾֵ��
          ��/�������޹�������ʾֵ"������ݸ�ʽ14��11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D018(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D001(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
��������:AFN0D019
��������:��Ӧ��վ�����������"�¶���������\�޹������
         ��������ʱ��"������ݸ�ʽ23��17��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D019(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D003(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
��������:AFN0D020
��������:��Ӧ��վ�����������"�¶��ᷴ����\�޹������
         ��������ʱ��"������ݸ�ʽ23��17��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D020(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D003(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
��������:AFN0D021
��������:��Ӧ��վ�����������"�¶��������й�������"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D021(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D005(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
��������:AFN0D022
��������:��Ӧ��վ�����������"�¶��������й�������"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D022(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D005(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
��������:AFN0D023
��������:��Ӧ��վ�����������"�¶��������й�������"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D023(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D005(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
��������:AFN0D024
��������:��Ӧ��վ�����������"�¶��������й�������"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D024(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D005(buff, frameTail, pHandle, fn, offset0d);
}

/*******************************************************
��������:AFN0D025
��������:��Ӧ��վ�����������"�ն������ܼ���������й����ʼ�����ʱ��
          �й�����Ϊ��ʱ��"������ݸ�ʽ23��18��
���ú���:AFN0D033
�����ú���:
�������:
�������:
����ֵ��֡β��
*******************************************************/
INT16U AFN0D025(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_PARA_BALANCE_RECORD];
	INT8U dataType, queryType;
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U i, tmpDay;
  BOOL ifHasData;
  
  INT16U tmpFrameTail;
    
  DATE_TIME tmpTime, readTime;
  
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		
  		tmpTime.second = 0x59;
  		tmpTime.minute = 0x59;
  		tmpTime.hour = 0x23;
  		if(fn == 25)
  		{
  			tmpTime.day   = *(pHandle+0);		//��
  			tmpTime.month = *(pHandle+1);		//��
  			tmpTime.year  = *(pHandle+2);		//��
  		}
  		else
  		{
  			tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
			  tmpTime.month = *(pHandle+0);		//��
			  tmpTime.year  = *(pHandle+1);		//��
  		}
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
  		if(fn == 25)
  		{
  			*offset0d = 3;
  			buff[frameTail++] = 0x01;																	//DT1
  			buff[frameTail++] = 0x03;																	//DT2
  			queryType = DAY_BALANCE;
  			dataType = DAY_BALANCE_PARA_DATA;
  		}
  		else	//F33
  		{
  			*offset0d = 2;
  			buff[frameTail++] = 0x01;																	//DT1
  			buff[frameTail++] = 0x04;																	//DT2
  			queryType = MONTH_BALANCE;
  			dataType = MONTH_BALANCE_PARA_DATA;
  		}
  		
  		ifHasData = FALSE;
  		
  		readTime = tmpTime;
  		if(readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0) == TRUE)
  		{
  			//�ն���������ʱ��  F25:Td_d(������) F33:Td_m(����)
  			if (fn==25)
  			{
  			  buff[frameTail++] = tmpTime.day;
  			}
  			buff[frameTail++] = tmpTime.month;
  			buff[frameTail++] = tmpTime.year;
  			
  			//����������й�����
  			if(dataBuff[MAX_TOTAL_POWER] != 0xEE)
  			{
  				ifHasData = TRUE;
  			}
  			buff[frameTail++] = dataBuff[MAX_TOTAL_POWER];
  			buff[frameTail++] = dataBuff[MAX_TOTAL_POWER + 1];
  			buff[frameTail++] = dataBuff[MAX_TOTAL_POWER + 2];
  			
  			//����������й����ʷ���ʱ��
  			if(dataBuff[MAX_TOTAL_POWER_TIME] != 0xEE)
  			{
  				ifHasData = TRUE;
  			}
  			buff[frameTail++] = dataBuff[MAX_TOTAL_POWER_TIME];
  			buff[frameTail++] = dataBuff[MAX_TOTAL_POWER_TIME + 1];
  			buff[frameTail++] = dataBuff[MAX_TOTAL_POWER_TIME + 2];
  			  			
  			//A������й�����
  			if(dataBuff[MAX_A_POWER] != 0xEE)
  			{
  				ifHasData = TRUE;
  			}
  			buff[frameTail++] = dataBuff[MAX_A_POWER];
  			buff[frameTail++] = dataBuff[MAX_A_POWER + 1];
  			buff[frameTail++] = dataBuff[MAX_A_POWER + 2];
  			
  			//A������й����ʷ���ʱ��
  			if(dataBuff[MAX_A_POWER_TIME] != 0xEE)
  			{
  				ifHasData = TRUE;
  			}
  			buff[frameTail++] = dataBuff[MAX_A_POWER_TIME];
  			buff[frameTail++] = dataBuff[MAX_A_POWER_TIME + 1];
  			buff[frameTail++] = dataBuff[MAX_A_POWER_TIME + 2];
  			
  			//B������й�����
  			if(dataBuff[MAX_B_POWER] != 0xEE)
  			{
  				ifHasData = TRUE;
  			}
  			buff[frameTail++] = dataBuff[MAX_B_POWER];
  			buff[frameTail++] = dataBuff[MAX_B_POWER + 1];
  			buff[frameTail++] = dataBuff[MAX_B_POWER + 2];
  			
  			//B������й����ʷ���ʱ��
  			if(dataBuff[MAX_B_POWER_TIME] != 0xEE)
  			{
  				ifHasData = TRUE;
  			}
  			buff[frameTail++] = dataBuff[MAX_B_POWER_TIME];
  			buff[frameTail++] = dataBuff[MAX_B_POWER_TIME + 1];
  			buff[frameTail++] = dataBuff[MAX_B_POWER_TIME + 2];
  			
  			//C������й�����
  			if(dataBuff[MAX_C_POWER] != 0xEE)
  			{
  				ifHasData = TRUE;
  			}
  			buff[frameTail++] = dataBuff[MAX_C_POWER];
  			buff[frameTail++] = dataBuff[MAX_C_POWER + 1];
  			buff[frameTail++] = dataBuff[MAX_C_POWER + 2];
  			
  			//C������й����ʷ���ʱ��
  			if(dataBuff[MAX_C_POWER_TIME] != 0xEE)
  			{
  				ifHasData = TRUE;
  			}
  			buff[frameTail++] = dataBuff[MAX_C_POWER_TIME];
  			buff[frameTail++] = dataBuff[MAX_C_POWER_TIME + 1];
  			buff[frameTail++] = dataBuff[MAX_C_POWER_TIME + 2];
  			
  			//�������й�����Ϊ��ʱ��
  			if(dataBuff[TOTAL_ZERO_POWER_TIME] != 0xEE)
  			{
  				ifHasData = TRUE;
  			  buff[frameTail++] = dataBuff[TOTAL_ZERO_POWER_TIME];
  			  buff[frameTail++] = dataBuff[TOTAL_ZERO_POWER_TIME + 1];
  			}
  			else
  			{
          buff[frameTail++] = 0x0;
          buff[frameTail++] = 0x0;
  			}
  			
  			//A���й�����Ϊ��ʱ��
  			if(dataBuff[A_ZERO_POWER_TIME] != 0xEE)
  			{
  				ifHasData = TRUE;
  			  
  			  buff[frameTail++] = dataBuff[A_ZERO_POWER_TIME];
  			  buff[frameTail++] = dataBuff[A_ZERO_POWER_TIME + 1];
  			}
  			else
  			{
          buff[frameTail++] = 0x0;
          buff[frameTail++] = 0x0;
  			}
  			
  			//B���й�����Ϊ��ʱ��
  			if(dataBuff[B_ZERO_POWER_TIME] != 0xEE)
  			{
  				ifHasData = TRUE;
  			  
  			  buff[frameTail++] = dataBuff[B_ZERO_POWER_TIME];
  			  buff[frameTail++] = dataBuff[B_ZERO_POWER_TIME + 1];
  			}
  			else
  			{
  				buff[frameTail++] = 0x0;
  				buff[frameTail++] = 0x0;
  			}
  			
  			//C���й�����Ϊ��ʱ��
  			if(dataBuff[C_ZERO_POWER_TIME] != 0xEE)
  			{
  				ifHasData = TRUE;
  			  
  			  buff[frameTail++] = dataBuff[C_ZERO_POWER_TIME];
  			  buff[frameTail++] = dataBuff[C_ZERO_POWER_TIME + 1];
  			}
  			else
  			{
  				buff[frameTail++] = 0x0;
  				buff[frameTail++] = 0x0;
  			}
  			
  			#ifdef NO_DATA_USE_PART_ACK_03
			    if (ifHasData == FALSE)
			  	{
			  		frameTail = tmpFrameTail;
			   	}
			 	#endif
			}
			else
			{
			 	#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail = tmpFrameTail;
			  #else
				  if (fn == 25)
				  {
				    buff[frameTail++] = tmpTime.day;   //��
				  }
				  buff[frameTail++] = tmpTime.month;   //��
				  buff[frameTail++] = tmpTime.year;    //��
				  
			    for(i = 0; i < 32; i++)
			    {
			   	  buff[frameTail++] = 0xee;
			    }
			  #endif
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*************************************************************
��������:AFN0D026
��������:��Ӧ��վ�����������"���ܼ������������������ʱ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D026(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_PARA_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U dataType, queryType;
	INT8U i, tmpDay;
	BOOL ifHasData;
	
	DATE_TIME tmpTime, readTime;
	
	INT16U tmpFrameTail;
	
	tmpFrameTail = frameTail;
	da1 = *pHandle++;
	da2 = *pHandle++;
	pHandle += 2;
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 8)
	{
		tmpPn++;
		if((da1 & 0x1) == 0x1)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour = 0x23;
			if(fn == 26)
			{
				tmpTime.day   = *(pHandle+0);
				tmpTime.month = *(pHandle+1);
				tmpTime.year  = *(pHandle+2);
			}
			else
  		{
  			tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
			  tmpTime.month = *(pHandle+0);		//��
			  tmpTime.year  = *(pHandle+1);		//��
  		}
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));			//DA!
  		buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
  		if(fn == 26)
  		{
  			*offset0d = 3;
  			buff[frameTail++] = 0x02;																	//DT1
  			buff[frameTail++] = 0x03;																	//DT2
  			queryType = DAY_BALANCE;
  			dataType = DAY_BALANCE_PARA_DATA;
  		}
  		else	//F34
  		{
  			*offset0d = 2;
  			buff[frameTail++] = 0x02;																	//DT1
  			buff[frameTail++] = 0x04;																	//DT2
  			queryType = MONTH_BALANCE;
  			dataType = MONTH_BALANCE_PARA_DATA;
  		}
  		
  		ifHasData = FALSE;
  		
  		readTime = tmpTime;
  		if(readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0) == TRUE)
  		{
  			//����������ʱ��Td_d Td_m
  			if(fn == 26)
  			{
				  buff[frameTail++] = tmpTime.day;   //��
				}
				buff[frameTail++] = tmpTime.month;   //��
				buff[frameTail++] = tmpTime.year;    //��
				
				//�������й��������
				if(dataBuff[MAX_TOTAL_REQ] != 0xEE)
				{
					ifHasData = TRUE;
				}
				buff[frameTail++] = dataBuff[MAX_TOTAL_REQ];
				buff[frameTail++] = dataBuff[MAX_TOTAL_REQ + 1];
				buff[frameTail++] = dataBuff[MAX_TOTAL_REQ + 2];
				
				//�������й������������ʱ��
				if(dataBuff[MAX_TOTAL_REQ_TIME] != 0xEE)
				{
					ifHasData = TRUE;
				}
				buff[frameTail++] = dataBuff[MAX_TOTAL_REQ_TIME];
				buff[frameTail++] = dataBuff[MAX_TOTAL_REQ_TIME + 1];
				buff[frameTail++] = dataBuff[MAX_TOTAL_REQ_TIME + 2];
				
				//A���й��������
				if(dataBuff[MAX_A_REQ] != 0xEE)
				{
					ifHasData = TRUE;
				}
				buff[frameTail++] = dataBuff[MAX_A_REQ];
				buff[frameTail++] = dataBuff[MAX_A_REQ + 1];
				buff[frameTail++] = dataBuff[MAX_A_REQ + 2];
				
				//A���й������������ʱ��
				if(dataBuff[MAX_A_REQ_TIME] != 0xEE)
				{
					ifHasData = TRUE;
				}
				buff[frameTail++] = dataBuff[MAX_A_REQ_TIME];
				buff[frameTail++] = dataBuff[MAX_A_REQ_TIME + 1];
				buff[frameTail++] = dataBuff[MAX_A_REQ_TIME + 2];
				
				//B���й��������
				if(dataBuff[MAX_B_REQ] != 0xEE)
				{
					ifHasData = TRUE;
				}
				buff[frameTail++] = dataBuff[MAX_B_REQ];
				buff[frameTail++] = dataBuff[MAX_B_REQ + 1];
				buff[frameTail++] = dataBuff[MAX_B_REQ + 2];
				
				//B���й������������ʱ��
				if(dataBuff[MAX_B_REQ_TIME] != 0xEE)
				{
					ifHasData = TRUE;
				}
				buff[frameTail++] = dataBuff[MAX_B_REQ_TIME];
				buff[frameTail++] = dataBuff[MAX_B_REQ_TIME + 1];
				buff[frameTail++] = dataBuff[MAX_B_REQ_TIME + 2];
				
				//C���й��������
				if(dataBuff[MAX_C_REQ] != 0xEE)
				{
					ifHasData = TRUE;
				}
				buff[frameTail++] = dataBuff[MAX_C_REQ];
				buff[frameTail++] = dataBuff[MAX_C_REQ + 1];
				buff[frameTail++] = dataBuff[MAX_C_REQ + 2];
				
				//C���й������������ʱ��
				if(dataBuff[MAX_C_REQ_TIME] != 0xEE)
				{
					ifHasData = TRUE;
				}
				buff[frameTail++] = dataBuff[MAX_C_REQ_TIME];
				buff[frameTail++] = dataBuff[MAX_C_REQ_TIME + 1];
				buff[frameTail++] = dataBuff[MAX_C_REQ_TIME + 2];
				
  			#ifdef NO_DATA_USE_PART_ACK_03
			    if (ifHasData == FALSE)
			  	{
			  		frameTail = tmpFrameTail;
			   	}
			 	#endif
			}
			else
			{
			 	#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail = tmpFrameTail;
			  #else
				  if (fn == 26)
				  {
				    buff[frameTail++] = tmpTime.day;   //��
				  }
				  buff[frameTail++] = tmpTime.month;   //��
				  buff[frameTail++] = tmpTime.year;    //��
				  
			    for(i = 0; i < 24; i++)
			    {
			   	  buff[frameTail++] = 0xee;
			    }
			  #endif
			}
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*************************************************************
��������:AFN0D027
��������:��Ӧ��վ�����������"�յ�ѹͳ������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D027(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U     dataBuff[LEN_OF_PARA_BALANCE_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     queryType, dataType;
	INT8U     tmpFrameTail, i, tmpDay;
	BOOL      ifHasData;	
	DATE_TIME tmpTime, readTime;
	INT16U    offset;
	INT16U    tmpData;
	
	tmpFrameTail = frameTail;
	da1 = *pHandle++;
	da2 = *pHandle++;
	pHandle += 2;
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x1) == 0x1)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour = 0x23;
			if(fn == 27)
			{
				tmpTime.day   = *(pHandle+0);
				tmpTime.month = *(pHandle+1);
				tmpTime.year  = *(pHandle+2);
			}
			else		//F35
  		{
  			tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
			  tmpTime.month = *(pHandle+0);		//��
			  tmpTime.year  = *(pHandle+1);		//��
  		}
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));			//DA!
  		buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
  		if(fn == 27)
  		{
  			*offset0d = 3;
  			buff[frameTail++] = 0x04;																	//DT1
  			buff[frameTail++] = 0x03;																	//DT2
  			queryType = DAY_BALANCE;
  			dataType = DAY_BALANCE_PARA_DATA;
  		}
  		else	//F35
  		{
  			*offset0d = 2;
  			buff[frameTail++] = 0x04;																	//DT1
  			buff[frameTail++] = 0x04;																	//DT2
  			queryType = MONTH_BALANCE;
  			dataType = MONTH_BALANCE_PARA_DATA;
  		}
  		
  		ifHasData = FALSE;
  		
  		readTime = tmpTime;
  		if(readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0) == TRUE)
  		{
  			//����������ʱ��Td_d Td_m
  			if(fn == 27)
  			{
				  buff[frameTail++] = tmpTime.day;   //��
				}
				buff[frameTail++] = tmpTime.month;   //��
				buff[frameTail++] = tmpTime.year;    //��
				
				//A��,B��,C���ѹ
				offset = VOL_A_UP_UP_TIME;
				for(i = 0; i < 3; i++)
				{
					//Խ���������ۼ�ʱ��
					if(dataBuff[offset] != 0xEE)
					{
						ifHasData = TRUE;
					  buff[frameTail++] = dataBuff[offset++];
					  buff[frameTail++] = dataBuff[offset++];
					}
					else
					{
					  buff[frameTail++] = 0x0;
					  buff[frameTail++] = 0x0;
					  offset += 2;
					}
					
					//Խ���������ۼ�ʱ��
					if(dataBuff[offset] != 0xEE)
					{
						ifHasData = TRUE;
					  buff[frameTail++] = dataBuff[offset++];
					  buff[frameTail++] = dataBuff[offset++];
					}
					else
					{
					  buff[frameTail++] = 0x00;
					  buff[frameTail++] = 0x00;
					  offset += 2;
					}
					
					//Խ�������ۼ�ʱ��
					if(dataBuff[offset] != 0xEE)
					{
						ifHasData = TRUE;
					  buff[frameTail++] = dataBuff[offset++];
					  buff[frameTail++] = dataBuff[offset++];
					}
					else
					{
					  buff[frameTail++] = 0x00;
					  buff[frameTail++] = 0x00;
					  offset += 2;
					}
					
					//Խ�������ۼ�ʱ��
					if(dataBuff[offset] != 0xEE)
					{
						ifHasData = TRUE;
					  buff[frameTail++] = dataBuff[offset++];
					  buff[frameTail++] = dataBuff[offset++];
					}
					else
					{
					  buff[frameTail++] = 0x00;
					  buff[frameTail++] = 0x00;
					  offset += 2;
					}
					
					//�ϸ����ۼ�ʱ��
					if(dataBuff[offset] != 0xEE)
					{
						ifHasData = TRUE;
					  buff[frameTail++] = dataBuff[offset++];
					  buff[frameTail++] = dataBuff[offset++];
					}
					else
					{
					  buff[frameTail++] = 0x00;
					  buff[frameTail++] = 0x00;
					  offset += 2;
					}
				}
				
				//A��,B��,C���ѹ
				offset = VOL_A_MAX;
				for(i = 0; i < 3; i++)
				{
					//��ѹ���ֵ
					if(dataBuff[offset] != 0xEE)
					{
						ifHasData = TRUE;
					}
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					
					//��ѹ���ֵ����ʱ��
					if(dataBuff[offset] != 0xEE)
					{
						ifHasData = TRUE;
					}
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					
					//��ѹ��Сֵ
					if(dataBuff[offset] != 0xEE)
					{
						ifHasData = TRUE;
					}
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					
					//��ѹ��Сֵ����ʱ��
					if(dataBuff[offset] != 0xEE)
					{
						ifHasData = TRUE;
					}
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					
					offset += 4;
				}
				
        offset = VOL_A_AVER;
        for (i = 0; i < 3; i++)
        {
            tmpData = hexToBcd(dataBuff[offset]|dataBuff[offset+1]<<8);
            buff[frameTail++] = tmpData&0xFF;
            buff[frameTail++] = tmpData>>8&0xFF;
            
            offset += 14;
        }
          				
				#ifdef NO_DATA_USE_PART_ACK_03
			    if (ifHasData == FALSE)
			  	{
			  		frameTail = tmpFrameTail;
			   	}
			 	#endif
			}
			else
			{
			 	#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail = tmpFrameTail;
			  #else
				  if (fn == 27)
				  {
				    buff[frameTail++] = tmpTime.day;   //��
				  }
				  buff[frameTail++] = tmpTime.month;   //��
				  buff[frameTail++] = tmpTime.year;    //��
				  
			    for(i = 0; i < 66; i++)
			    {
			   	  buff[frameTail++] = 0xee;
			    }
			  #endif
			}
		}
		da1 >>= 1;
	}
	
  return frameTail;
}

/*************************************************************
��������:AFN0D028
��������:��Ӧ��վ�����������"�ղ�ƽ���Խ���ۼ�ʱ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D028(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_PARA_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U queryType, dataType;
	INT8U i, tmpDay;
	BOOL ifHasData;
	
	DATE_TIME tmpTime, readTime;
  
  INT16U tmpFrameTail, offset;
  
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x1) == 0x1)
 		{
 			pn = tmpPn + (da2 - 1) * 8;
 			
 			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour   = 0x23;
			if(fn == 28)
			{
				tmpTime.day   = *(pHandle+0);    //��
				tmpTime.month = *(pHandle+1);    //��
				tmpTime.year  = *(pHandle+2);    //��
			}
			else		//F36
  		{
  			tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
			  tmpTime.month = *(pHandle+0);		//��
			  tmpTime.year  = *(pHandle+1);		//��
  		}
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));			//DA!
  		buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
  		if(fn == 28)
  		{
  			*offset0d = 3;
  			buff[frameTail++] = 0x08;																	//DT1
  			buff[frameTail++] = 0x03;																	//DT2
  			queryType = DAY_BALANCE;
  			dataType = DAY_BALANCE_PARA_DATA;
  		}
  		else	//F36
  		{
  			*offset0d = 2;
  			buff[frameTail++] = 0x08;																	//DT1
  			buff[frameTail++] = 0x04;																	//DT2
  			queryType = MONTH_BALANCE;
  			dataType = MONTH_BALANCE_PARA_DATA;
  		}
  		
  		ifHasData = FALSE;
  		
  		readTime = tmpTime;
  		if(readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0) == TRUE)
  		{
  			//����������ʱ��Td_d Td_m
  			if(fn == 28)
  			{
				  buff[frameTail++] = tmpTime.day;   //��
				}
				buff[frameTail++] = tmpTime.month;   //��
				buff[frameTail++] = tmpTime.year;    //��
				
				//������ƽ��Խ�����ۼ�ʱ��
				if (dataBuff[CUR_UNBALANCE_TIME]!=0xee && dataBuff[CUR_UNBALANCE_TIME+1]!=0xee)
				{
				  buff[frameTail++] = dataBuff[CUR_UNBALANCE_TIME];
				  buff[frameTail++] = dataBuff[CUR_UNBALANCE_TIME+1];
				}
				else
				{
					buff[frameTail++] = 0x00;
					buff[frameTail++] = 0x00;
				}
					
				//��ѹ��ƽ��Խ�����ۼ�ʱ��
				if (dataBuff[VOL_UNBALANCE_TIME]!=0xee && dataBuff[VOL_UNBALANCE_TIME+1]!=0xee)
				{
				  buff[frameTail++] = dataBuff[VOL_UNBALANCE_TIME];
				  buff[frameTail++] = dataBuff[VOL_UNBALANCE_TIME+1];
				}
				else
				{
					buff[frameTail++] = 0x00;
					buff[frameTail++] = 0x00;
				}
				
				//������ƽ�����ֵ
				buff[frameTail++] = dataBuff[CUR_UNB_MAX];
				buff[frameTail++] = dataBuff[CUR_UNB_MAX+1];

				//������ƽ�����ֵ��������
				buff[frameTail++] = dataBuff[CUR_UNB_MAX_TIME];
				buff[frameTail++] = dataBuff[CUR_UNB_MAX_TIME+1];
				buff[frameTail++] = dataBuff[CUR_UNB_MAX_TIME+2];
				if (fn==36)
				{
					 buff[frameTail++] = tmpTime.month;
				}

				//��ѹ��ƽ�����ֵ
				buff[frameTail++] = dataBuff[VOL_UNB_MAX];
				buff[frameTail++] = dataBuff[VOL_UNB_MAX+1];

				//��ѹ��ƽ�����ֵ��������
				buff[frameTail++] = dataBuff[VOL_UNB_MAX_TIME];
				buff[frameTail++] = dataBuff[VOL_UNB_MAX_TIME+1];
				buff[frameTail++] = dataBuff[VOL_UNB_MAX_TIME+2];
				if (fn==36)
				{
					buff[frameTail++] = tmpTime.month;
				}
			}
			else
			{
				if (fn == 28)
				{
				  buff[frameTail++] = tmpTime.day;   //��
				}
				buff[frameTail++] = tmpTime.month;   //��
				buff[frameTail++] = tmpTime.year;    //��
				  
			  for(i = 0; i < 14; i++)
			  {
			    buff[frameTail++] = 0xee;
			  }
			}
 		}
 		da1 >>= 1;
 	}
  
  return frameTail;
}

/*************************************************************
��������:AFN0D029
��������:��Ӧ��վ�����������"�յ���Խ��ͳ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D029(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_PARA_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U queryType, dataType;
	INT8U i, tmpDay;
	BOOL ifHasData;
	
	DATE_TIME tmpTime, readTime;
  
  INT16U tmpFrameTail, offset;
  
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x1) == 0x1)
 		{
 			pn = tmpPn + (da2 - 1) * 8;
 			
 			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour = 0x23;
			if(fn == 29)
			{
				tmpTime.day   = *(pHandle+0);    //��
				tmpTime.month = *(pHandle+1);    //��
				tmpTime.year  = *(pHandle+2);    //��
			}
			else		//F37
  		{
  			tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
			  tmpTime.month = *(pHandle+0);		//��
			  tmpTime.year  = *(pHandle+1);		//��
  		}
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
  		if(fn == 29)
  		{
  			*offset0d = 3;
  			buff[frameTail++] = 0x10;																	//DT1
  			buff[frameTail++] = 0x03;																	//DT2
  			queryType = DAY_BALANCE;
  			dataType = DAY_BALANCE_PARA_DATA;
  		}
  		else	//F37
  		{
  			*offset0d = 2;
  			buff[frameTail++] = 0x10;																	//DT1
  			buff[frameTail++] = 0x04;																	//DT2
  			queryType = MONTH_BALANCE;
  			dataType = MONTH_BALANCE_PARA_DATA;
  		}
  		
  		ifHasData = FALSE;
  		
  		readTime = tmpTime;
  		if(readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0) == TRUE)
  		{
  			//����������ʱ��Td_d Td_m
  			if(fn == 29)
  			{
				  buff[frameTail++] = tmpTime.day;   //��
				}
				buff[frameTail++] = tmpTime.month;   //��
				buff[frameTail++] = tmpTime.year;    //��
				
				//A,B,C�����
				offset = CUR_A_UP_UP_TIME;
				for(i = 0; i < 3; i++)
				{
					//����Խ�������ۼ�ʱ��
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					
					//����Խ�����ۼ�ʱ��
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					
					offset += 6;
				}
				
				//�������Խ�����ۼ�ʱ��
				buff[frameTail++] = dataBuff[offset++];
				buff[frameTail++] = dataBuff[offset++];
				
				//A,B,C��������������
				offset = CUR_A_MAX;
				for(i = 0; i < 4; i++)
				{
					//�������ֵ
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
				 
					buff[frameTail++] = dataBuff[offset++];
					
					//���ֵ����ʱ��
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					
					offset += 4;
				}
			}
			else
			{
			 	#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail = tmpFrameTail;
			  #else
				  if (fn == 29)
				  {
				    buff[frameTail++] = tmpTime.day;   //��
				  }
				  buff[frameTail++] = tmpTime.month;   //��
				  buff[frameTail++] = tmpTime.year;    //��
				  
			    for(i = 0; i < 38; i++)
			    {
			   	  buff[frameTail++] = 0xee;
			    }
			  #endif
			}
 		}
 		da1 >>= 1;
 	}
 	
 	return frameTail;
}

/*************************************************************
��������:AFN0D030
��������:��Ӧ��վ�����������"�����ڹ���Խ���ۼ�ʱ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D030(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_PARA_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U dataType, queryType;
	INT8U i, tmpDay;
	BOOL ifHasData;
	
  DATE_TIME tmpTime, readTime;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x1) == 0x1)
 		{
 			pn = tmpPn + (da2 - 1) * 8;
 			
 			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour = 0x23;
			if(fn == 30)
			{
				tmpTime.day   = *(pHandle+0);   //��
				tmpTime.month = *(pHandle+1);   //��
				tmpTime.year  = *(pHandle+2);   //��
			}
			else		//F38
  		{
  			tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
			  tmpTime.month = *(pHandle+0);		//��
			  tmpTime.year  = *(pHandle+1);		//��
  		}
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
  		if(fn == 30)
  		{
  			*offset0d = 3;
  			buff[frameTail++] = 0x20;																	//DT1
  			buff[frameTail++] = 0x03;																	//DT2
  			queryType = DAY_BALANCE;
  			dataType = DAY_BALANCE_PARA_DATA;
  		}
  		else	//F38
  		{
  			*offset0d = 2;
  			buff[frameTail++] = 0x20;																	//DT1
  			buff[frameTail++] = 0x04;																	//DT2
  			queryType = MONTH_BALANCE;
  			dataType = MONTH_BALANCE_PARA_DATA;
  		}
  		
  		ifHasData = FALSE;
  		
  		readTime = tmpTime;
  		if(readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0) == TRUE)
  		{
  			//����������ʱ��Td_d Td_m
  			if(fn == 30)
  			{
				  buff[frameTail++] = tmpTime.day;   //��
				}
				buff[frameTail++] = tmpTime.month;   //��
				buff[frameTail++] = tmpTime.year;    //��
				
				//���ڹ���Խ�������ۼ�ʱ��
				if (dataBuff[APPARENT_POWER_UP_UP_TIME]!=0xee && dataBuff[APPARENT_POWER_UP_UP_TIME+1]!=0xee)
				{
				  buff[frameTail++] = dataBuff[APPARENT_POWER_UP_UP_TIME];
				  buff[frameTail++] = dataBuff[APPARENT_POWER_UP_UP_TIME + 1];
				}
				else
				{
					buff[frameTail++] = 0x00;
					buff[frameTail++] = 0x00;
				}
				
				//���ڹ���Խ�����ۼ�ʱ��
				if (dataBuff[APPARENT_POWER_UP_TIME]!=0xee && dataBuff[APPARENT_POWER_UP_TIME+1]!=0xee)
				{
				  buff[frameTail++] = dataBuff[APPARENT_POWER_UP_TIME];
				  buff[frameTail++] = dataBuff[APPARENT_POWER_UP_TIME + 1];
				}
				else
				{
					buff[frameTail++] = 0x00;
					buff[frameTail++] = 0x00;
				}
			}
			else
			{
				#ifdef NO_DATA_USE_PART_ACK_03
        	frameTail -= 4;
        #else
        	//����������ʱ��Td_d Td_m
	  			if(fn == 30)
	  			{
					  buff[frameTail++] = tmpTime.day;   //��
					}
					buff[frameTail++] = tmpTime.month;   //��
					buff[frameTail++] = tmpTime.year;    //��
          for(i=0;i<4;i++)
          {
          	buff[frameTail++] = 0xee;
          }
        #endif
			}
 		}
 		da1 >>= 1;
 	}
  
  return frameTail;
}

/*************************************************************
��������:AFN0D031
��������:��Ӧ��վ�����������"�ո�����ͳ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D031(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   	//ly,10-12-17,Ϊ���ݹ��繫˾��������,������Ҫ����
   	
   	INT8U  dataBuff[LEN_OF_PARA_BALANCE_RECORD];
  	INT16U pn, tmpPn = 0;
  	INT8U  da1, da2;
  	INT8U  dataType, queryType;
  	INT8U  i, tmpDay;
  	BOOL  ifHasData;
  	
    DATE_TIME tmpTime, readTime;
    
    da1 = *pHandle++;
    da2 = *pHandle++;
    pHandle += 2;
    
    if(da1 == 0x0)
    {
    	return frameTail;
    }
    
    while(tmpPn < 9)
   	{
   		tmpPn++;
   		if((da1 & 0x1) == 0x1)
   		{
   			pn = tmpPn + (da2 - 1) * 8;
   			
   			tmpTime.second = 0x59;
  			tmpTime.minute = 0x59;
  			tmpTime.hour   = 0x23;
  			if(fn == 31)
  			{
  				tmpTime.day   = *(pHandle+0);   //��
  				tmpTime.month = *(pHandle+1);   //��
  				tmpTime.year  = *(pHandle+2);   //��
  				
  				*offset0d = 3;
  			}
  			else		//F39
    		{
    			tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
  		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
  			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
  			  tmpTime.month = *(pHandle+0);		//��
  			  tmpTime.year  = *(pHandle+1);		//��
  				*offset0d = 2;
    		}
    		
    		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
    		buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
  			if(fn == 31)
  			{
    		  buff[frameTail++] = 0x40;																	//DT1
    		  buff[frameTail++] = 0x03;																	//DT2
				  buff[frameTail++] = tmpTime.day;   //��
				}
				else  //F39
				{
    		  buff[frameTail++] = 0x40;																	//DT1
    		  buff[frameTail++] = 0x04;																	//DT2
				}
				buff[frameTail++] = tmpTime.month;   //��
				buff[frameTail++] = tmpTime.year;    //��
    		
    		if (fn==31)
    		{
    		  memset(&buff[frameTail], 0x0, 10);
    		  frameTail += 10;
    		}
    		else
    		{
    		  memset(&buff[frameTail], 0x0, 12);
    		  frameTail += 12;
    		}
    		
    	}
    	da1 >>= 1;
    }
  	
   return frameTail;
}

/*************************************************************
��������:AFN0D032
��������:��Ӧ��վ�����������"�յ��ܱ��������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D032(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_PARA_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U i;
	
	DATE_TIME tmpTime, readTime;
  
  *offset0d = 3;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x1) == 0x1)
 		{
 			pn = tmpPn + (da2 - 1) * 8;
 			
 			tmpTime.second = 0x59;
 			tmpTime.minute = 0x59;
 			tmpTime.hour   = 0x23;
			tmpTime.day    = *(pHandle+0);   //��
			tmpTime.month  = *(pHandle+1);   //��
			tmpTime.year   = *(pHandle+2);   //��
 			
 			buff[frameTail++] = 0x01 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
 			buff[frameTail++] = (pn - 1) / 8 + 1;												//DA2
 		  if (fn==31)
 		  {
 			  buff[frameTail++] = 0x40;				  												//DT1
 		  }
 		  else
 		  {	
 			  buff[frameTail++] = 0x80;																	//DT1
 			}
 			buff[frameTail++] = 0x03;																		//DT2
 			
 			readTime = tmpTime;
 			if(readMeterData(dataBuff, pn, DAY_BALANCE, DAY_BALANCE_PARA_DATA, &readTime, 0) == TRUE)
 			{
 				//�ն���������ʱ��Td_d
 				buff[frameTail++] = tmpTime.day;
 				buff[frameTail++] = tmpTime.month;
 				buff[frameTail++] = tmpTime.year;
 				
 				//�ն˳���ʱ��
 				buff[frameTail++] = tmpTime.minute;
 				buff[frameTail++] = tmpTime.hour;
 				buff[frameTail++] = tmpTime.day;
 				buff[frameTail++] = tmpTime.month;
 				buff[frameTail++] = tmpTime.year;
 				
 				//�ܶ������
 				buff[frameTail++] = dataBuff[OPEN_PHASE_TIMES];
 				buff[frameTail++] = dataBuff[OPEN_PHASE_TIMES + 1];
 				
 				//A��������
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_TIMES];
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_TIMES + 1];
 				
 				//B��������
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_TIMES];
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_TIMES + 1];
 				
 				//C��������
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_TIMES];
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_TIMES + 1];
 				
 				//�����ۼ�ʱ��
 				buff[frameTail++] = dataBuff[OPEN_PHASE_MINUTES];
 				buff[frameTail++] = dataBuff[OPEN_PHASE_MINUTES + 1];
 				buff[frameTail++] = dataBuff[OPEN_PHASE_MINUTES + 2];
 				
 				//A�����ۼ�ʱ��
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_MINUTES];
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_MINUTES + 1];
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_MINUTES + 2];
 				
 				//B�����ۼ�ʱ��
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_MINUTES];
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_MINUTES + 1];
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_MINUTES + 2];
 				
 				//C�����ۼ�ʱ��
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_MINUTES];
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_MINUTES + 1];
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_MINUTES + 2];
 				
 				//���һ�ζ�����ʼʱ��
 				buff[frameTail++] = dataBuff[OPEN_PHASE_LAST_BEG];
 				buff[frameTail++] = dataBuff[OPEN_PHASE_LAST_BEG + 1];
 				buff[frameTail++] = dataBuff[OPEN_PHASE_LAST_BEG + 2];
 				buff[frameTail++] = dataBuff[OPEN_PHASE_LAST_BEG + 3];
 				
 				//A�����������ʼʱ��
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_LAST_BEG + 0];
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_LAST_BEG + 1];
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_LAST_BEG + 2];
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_LAST_BEG + 3];
 				
 				//B�����������ʼʱ��
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_LAST_BEG + 0];
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_LAST_BEG + 1];
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_LAST_BEG + 2];
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_LAST_BEG + 3];
 				
 				//C�����������ʼʱ��
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_LAST_BEG + 0];
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_LAST_BEG + 1];
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_LAST_BEG + 2];
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_LAST_BEG + 3];
 				
 				//���һ�ζ������ʱ��
 				buff[frameTail++] = dataBuff[OPEN_PHASE_LASE_END + 0];
 				buff[frameTail++] = dataBuff[OPEN_PHASE_LASE_END + 1];
 				buff[frameTail++] = dataBuff[OPEN_PHASE_LASE_END + 2];
 				buff[frameTail++] = dataBuff[OPEN_PHASE_LASE_END + 3];
 				
 				//A������������ʱ��
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_LAST_END + 0];
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_LAST_END + 1];
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_LAST_END + 2];
 				buff[frameTail++] = dataBuff[A_OPEN_PHASE_LAST_END + 3];
 				
 				//B������������ʱ��
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_LAST_END + 0];
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_LAST_END + 1];
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_LAST_END + 2];
 				buff[frameTail++] = dataBuff[B_OPEN_PHASE_LAST_END + 3];
 				
 				//C������������ʱ��
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_LAST_END + 0];
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_LAST_END + 1];
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_LAST_END + 2];
 				buff[frameTail++] = dataBuff[C_OPEN_PHASE_LAST_END + 3];
 			}
 			else
 			{
 				#ifdef NO_DATA_USE_PART_ACK_03
        	frameTail -= 4;
        #else
        	//�ն���������ʱ��Td_d
	 				buff[frameTail++] = tmpTime.day;
	 				buff[frameTail++] = tmpTime.month;
	 				buff[frameTail++] = tmpTime.year;
	 				
	 				//�ն˳���ʱ��
	 				buff[frameTail++] = tmpTime.minute;
	 				buff[frameTail++] = tmpTime.hour;
	 				buff[frameTail++] = tmpTime.day;
	 				buff[frameTail++] = tmpTime.month;
	 				buff[frameTail++] = tmpTime.year;
					
          for(i=0;i<52;i++)
          {
          	buff[frameTail++] = 0xee;
          }
        #endif
 			}
 		}
 		da1 >>= 1;
 	}
  
	return frameTail;
}

/*************************************************************
��������:AFN0D033
��������:��Ӧ��վ�����������"���ܼ���������й����ʷ���ʱ�䣬
          �й�����Ϊ��ʱ��"������ݸ�ʽ23��18��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D033(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
  return AFN0D025(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D034
��������:��Ӧ��վ�����������"���ܼ������й��������������ʱ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D034(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D026(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D035
��������:��Ӧ��վ�����������"�µ�ѹͳ������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D035(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D027(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D036
��������:��Ӧ��վ�����������"�²�ƽ���Խ���ۼ�ʱ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D036(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D028(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D037
��������:��Ӧ��վ�����������"�µ���Խ��ͳ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D037(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D029(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D038
��������:��Ӧ��վ�����������"�����ڹ���Խ���ۼ�ʱ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D038(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D030(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D039
��������:��Ӧ��վ�����������"�¸���ͳ����"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D039(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D031(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D041
��������:��Ӧ��վ�����������"�ն���������ۼ�Ͷ��ʱ��ʹ���"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D041(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	//����Ч����
  *offset0d = 3;
  return frameTail;
}

/*************************************************************
��������:AFN0D042
��������:��Ӧ��վ�����������"�ն����ա��µ������ۼƲ������޹�������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D042(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
  //����Ч����
  *offset0d = 3;
  return frameTail;
}

/*************************************************************
��������:AFN0D043
��������:��Ӧ��վ�����������"�ն����չ������������ۼ�ʱ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D043(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_PARA_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U dataType, queryType;
	INT8U i, tmpDay;
	BOOL ifHasData;
	
  DATE_TIME tmpTime, readTime;
  
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x1) == 0x1)
 		{
 			pn = tmpPn + (da2 - 1) * 8;
 			
 			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour = 0x23;
			if(fn == 43)
			{
				tmpTime.day   = *(pHandle+0);   //��
				tmpTime.month = *(pHandle+1);   //��
				tmpTime.year  = *(pHandle+2);   //��
			}
			else		//F38
  		{
  			tmpDay = monthDays((2000 + (*(pHandle+1) & 0xF) + (*(pHandle + 1) >> 4) * 10)
		  												, (*pHandle & 0xF) + (*pHandle >> 4) * 10);
			  tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
			  tmpTime.month = *(pHandle+0);		//��
			  tmpTime.year  = *(pHandle+1);		//��
  		}
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;                      	//DA2
  		if(fn == 43)
  		{
  			*offset0d = 3;
  			buff[frameTail++] = 0x04;																	//DT1
  			buff[frameTail++] = 0x05;																	//DT2
  			queryType = DAY_BALANCE;
  			dataType = DAY_BALANCE_PARA_DATA;
  		}
  		else	//F44
  		{
  			*offset0d = 2;
  			buff[frameTail++] = 0x08;																	//DT1
  			buff[frameTail++] = 0x05;																	//DT2
  			queryType = MONTH_BALANCE;
  			dataType = MONTH_BALANCE_PARA_DATA;
  		}
  		
  		ifHasData = FALSE;
  		
  		readTime = tmpTime;
  		if(readMeterData(dataBuff, pn, queryType, dataType, &readTime, 0) == TRUE)
  		{
  			//����������ʱ��Td_d Td_m
  			if(fn == 43)
  			{
				  buff[frameTail++] = tmpTime.day;   //��
				}
				buff[frameTail++] = tmpTime.month;   //��
				buff[frameTail++] = tmpTime.year;    //��
				
				//����1�ۼ�ʱ��
				if (dataBuff[FACTOR_SEG_1]!=0xee && dataBuff[FACTOR_SEG_1+1]!=0xee)
				{
				  buff[frameTail++] = dataBuff[FACTOR_SEG_1];
				  buff[frameTail++] = dataBuff[FACTOR_SEG_1 + 1];
				}
				else
				{
					buff[frameTail++] = 0x00;
					buff[frameTail++] = 0x00;
				}

				//����2�ۼ�ʱ��
				if (dataBuff[FACTOR_SEG_2]!=0xee && dataBuff[FACTOR_SEG_2+1]!=0xee)
				{
				  buff[frameTail++] = dataBuff[FACTOR_SEG_2];
				  buff[frameTail++] = dataBuff[FACTOR_SEG_2 + 1];
				}
				else
				{
					buff[frameTail++] = 0x00;
					buff[frameTail++] = 0x00;
				}

				//����3�ۼ�ʱ��
				if (dataBuff[FACTOR_SEG_3]!=0xee && dataBuff[FACTOR_SEG_3+1]!=0xee)
				{
				  buff[frameTail++] = dataBuff[FACTOR_SEG_3];
				  buff[frameTail++] = dataBuff[FACTOR_SEG_3 + 1];
				}
				else
				{
					buff[frameTail++] = 0x00;
					buff[frameTail++] = 0x00;
				}
			}
			else
			{
      	//����������ʱ��Td_d Td_m
  			if(fn == 43)
  			{
				  buff[frameTail++] = tmpTime.day;   //��
				}
				buff[frameTail++] = tmpTime.month;   //��
				buff[frameTail++] = tmpTime.year;    //��
        for(i=0;i<6;i++)
        {
        	buff[frameTail++] = 0xee;
        }
			}
 		}
 		da1 >>= 1;
 	}
  
  return frameTail;
}

/*************************************************************
��������:AFN0D044
��������:��Ӧ��վ�����������"�¶����¹������������ۼ�ʱ��"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D044(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
  return AFN0D043(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D045
��������:��Ӧ��վ�����������"�ն���ͭ��,�����й�����ʾֵ"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D045(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	//����Ч����
	*offset0d = 3;
  return frameTail;
}

/*************************************************************
��������:AFN0D046
��������:��Ӧ��վ�����������"�¶���ͭ��,�����й�����ʾֵ"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D046(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	//����Ч����
	*offset0d = 2;
  return frameTail;
}

/*************************************************************
��������:AFN0D049
��������:��Ӧ��վ�����������"�ն����ն��չ���ʱ�䡢�ո�λ�ۼƴ���"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D049(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	TERMINAL_STATIS_RECORD terminalStatisRecord;
	INT8U                  i, tmpDay;
	DATE_TIME              tmpTime, bakTime;
	INT8U                  tmpCount;
	INT32U                 tmpMonthReset,tmpMonthPowerOn;
	
	buff[frameTail++] = *pHandle++;		//DA1
	buff[frameTail++] = *pHandle++;		//DA2
	buff[frameTail++] = *pHandle++;		//DT1
	buff[frameTail++] = *pHandle++;		//DT2
	
	//����������ʱ��Td_d Td_m
	tmpTime.second = 0x59;
	tmpTime.minute = 0x59;
	tmpTime.hour = 0x23;
	if(fn == 49)
	{
		*offset0d = 3;
		buff[frameTail++] = tmpTime.day   = *pHandle++;
		buff[frameTail++] = tmpTime.month = *pHandle++;
		buff[frameTail++] = tmpTime.year  = *pHandle++;
	}
	else		//F51
	{
		*offset0d = 2;
		tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + *(pHandle + 1) & 0xF
													, (*pHandle >> 4) * 10 + *pHandle % 10);
		tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
		
		buff[frameTail++] = tmpTime.month = *pHandle++;
		buff[frameTail++] = tmpTime.year  = *pHandle++;
	}
	
	if(fn == 49)
	{
	  if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
	  {
			//�ն��չ���ʱ��
			buff[frameTail++] = terminalStatisRecord.powerOnMinute & 0xFF;
			buff[frameTail++] = terminalStatisRecord.powerOnMinute >> 8;
			
			//�ն��ո�λ�ۼƴ���
			buff[frameTail++] = terminalStatisRecord.resetTimes & 0xFF;
			buff[frameTail++] = terminalStatisRecord.resetTimes >> 8;
	  }
	  else
	  {
		  #ifdef NO_DATA_USE_PART_ACK_03
          frameTail -= 7;
      #else
        for(i = 0; i < 4; i++)
        {
     	    buff[frameTail++] = 0xee;
        }
      #endif
    }
	}
	else
	{
  	bakTime = timeBcdToHex(tmpTime);
  	tmpCount = monthDays(bakTime.year+2000,bakTime.month);
  	tmpMonthReset   = 0;
  	tmpMonthPowerOn = 0;
  	for(i=1; i<=tmpCount; i++)
  	{
  		tmpTime = bakTime;
  		tmpTime.day = i;
  	 	tmpTime = timeHexToBcd(tmpTime);
      if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
      {
        tmpMonthReset   += terminalStatisRecord.resetTimes;
        tmpMonthPowerOn += terminalStatisRecord.powerOnMinute;
      }
  	}
      
		//�ն��¹���ʱ��
		buff[frameTail++] = tmpMonthPowerOn & 0xFF;
		buff[frameTail++] = tmpMonthPowerOn >> 8;
			
		//�ն��¸�λ�ۼƴ���
		buff[frameTail++] = tmpMonthReset & 0xFF;
		buff[frameTail++] = tmpMonthReset >> 8;
    
  	/*ly,2011-10-11,�޸�Ϊ�����д��
  	tmpCount = monthDays(sysTime.year+2000,sysTime.month);
  	tmpMonthReset   = 0;
  	tmpMonthPowerOn = 0;
  	for(i=1;i<=tmpCount && i<=sysTime.day;i++)
  	{
  		 tmpTime = sysTime;
  		 tmpTime.day = i;
  	 	 tmpTime = timeHexToBcd(tmpTime);
       if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
       {
         tmpMonthReset   += terminalStatisRecord.resetTimes;
         tmpMonthPowerOn += terminalStatisRecord.powerOnMinute;
       }
  	}  	
      
    if (tmpMonthReset!=0)
    {
			 //�ն��¹���ʱ��
			 buff[frameTail++] = tmpMonthPowerOn & 0xFF;
			 buff[frameTail++] = tmpMonthPowerOn >> 8;
			
			 //�ն��¸�λ�ۼƴ���
			 buff[frameTail++] = tmpMonthReset & 0xFF;
			 buff[frameTail++] = tmpMonthReset >> 8;
		}
	  else
	  {
		   #ifdef NO_DATA_USE_PART_ACK_03
        frameTail -= 6;
       #else
        for(i = 0; i < 4; i++)
        {
     	    buff[frameTail++] = 0xee;
        }
       #endif
    }
    */
	}
	
	return frameTail;
}

/*************************************************************
��������:AFN0D050
��������:��Ӧ��վ�����������"�ն����ն��տ���ͳ������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D050(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	if (fn==50)
	{
  	*offset0d = 3;
	}
	else
	{
  	*offset0d = 2;
	}
	
	#ifdef LOAD_CTRL_MODULE
  	TERMINAL_STATIS_RECORD terminalStatisRecord;
  	INT8U                  i, tmpDay;
  	DATE_TIME              tmpTime, bakTime;
  	INT8U                  tmpCount;
  	INT16U                 tmpData[4];
  	
  	buff[frameTail++] = *pHandle++;		//DA1
  	buff[frameTail++] = *pHandle++;		//DA2
  	buff[frameTail++] = *pHandle++;		//DT1
  	buff[frameTail++] = *pHandle++;		//DT2
  	
  	//����������ʱ��Td_d Td_m
  	tmpTime.second = 0x59;
  	tmpTime.minute = 0x59;
  	tmpTime.hour = 0x23;
  	if(fn == 50)
  	{
  		*offset0d = 3;
  		buff[frameTail++] = tmpTime.day   = *pHandle++;
  		buff[frameTail++] = tmpTime.month = *pHandle++;
  		buff[frameTail++] = tmpTime.year  = *pHandle++;
  	}
  	else		//F52
  	{
  		*offset0d = 2;
  		tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + *(pHandle + 1) & 0xF
  													, (*pHandle >> 4) * 10 + *pHandle % 10);
  		tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
  		
  		buff[frameTail++] = tmpTime.month = *pHandle++;
  		buff[frameTail++] = tmpTime.year  = *pHandle++;
  	}
  	
  	if(fn == 50)
  	{
  	  if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
  	  {
  			buff[frameTail++] = terminalStatisRecord.monthCtrlJumpedDay;   //�µ����բ���ۼƴ���
  			buff[frameTail++] = terminalStatisRecord.chargeCtrlJumpedDay;  //�������բ���ۼƴ���
  			buff[frameTail++] = terminalStatisRecord.powerCtrlJumpedDay;   //������բ���ۼƴ���
  			buff[frameTail++] = terminalStatisRecord.remoteCtrlJumpedDay;  //ң����բ���ۼƴ���
  	  }
  	  else
  	  {
        for(i = 0; i < 4; i++)
        {
       	   buff[frameTail++] = 0xee;
        }
      }
  	}
  	else
  	{
    	bakTime = timeBcdToHex(tmpTime);
    	tmpCount = monthDays(bakTime.year+2000, bakTime.month);
    	tmpData[0]   = 0;
    	tmpData[1]   = 0;
    	tmpData[2]   = 0;
    	tmpData[3]   = 0;
    	for(i=1;i<=tmpCount;i++)
    	{
    		 tmpTime = bakTime;
    		 tmpTime.day = i;
    	 	 tmpTime = timeHexToBcd(tmpTime);
         if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
         {
           tmpData[0] += terminalStatisRecord.monthCtrlJumpedDay;
           tmpData[1] += terminalStatisRecord.chargeCtrlJumpedDay;
           tmpData[2] += terminalStatisRecord.powerCtrlJumpedDay;
           tmpData[3] += terminalStatisRecord.remoteCtrlJumpedDay;
         }
    	}
        
  		buff[frameTail++] = tmpData[0];
  		buff[frameTail++] = tmpData[1];
  		buff[frameTail++] = tmpData[2];
  		buff[frameTail++] = tmpData[3];
  	}
  #endif

  return frameTail;
}

/*************************************************************
��������:AFN0D051
��������:��Ӧ��վ�����������"�¶����ն��¹���ʱ�䡢�¸�λ�ۼƴ���"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D051(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D049(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D052
��������:��Ӧ��վ�����������"�¶����ն��µ����ͳ������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D052(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D050(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D053
��������:��Ӧ��վ�����������"�ն�����վ��ͨ������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D053(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	TERMINAL_STATIS_RECORD terminalStatisRecord;
	INT8U  queryType, dataType;
	INT8U  i, tmpDay, tmpCount;
	INT32U tmpData;
	
	DATE_TIME tmpTime,backTime;
	
	buff[frameTail++] = *pHandle++;		//DA1
	buff[frameTail++] = *pHandle++;		//DA2
	buff[frameTail++] = *pHandle++;		//DT1
	buff[frameTail++] = *pHandle++;		//DT2
	
	//����������ʱ��Td_d Td_m
	tmpTime.second = 0x59;
	tmpTime.minute = 0x59;
	tmpTime.hour   = 0x23;
	if(fn == 53)
	{
		*offset0d = 3;
		buff[frameTail++] = tmpTime.day   = *pHandle++;
		buff[frameTail++] = tmpTime.month = *pHandle++;
		buff[frameTail++] = tmpTime.year  = *pHandle++;
	}
	else		//F54
	{
		*offset0d = 2;
		tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + *(pHandle + 1) & 0xF
													, (*pHandle >> 4) * 10 + *pHandle % 10);
		tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
		
		buff[frameTail++] = tmpTime.month = *pHandle++;
		buff[frameTail++] = tmpTime.year  = *pHandle++;
	}
	
	if(fn == 53)
	{
	  if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
	  {
			tmpData = terminalStatisRecord.sendBytes+terminalStatisRecord.receiveBytes;
		  buff[frameTail++] = tmpData&0xff;
		  buff[frameTail++] = tmpData>>8&0xff;
		  buff[frameTail++] = tmpData>>16&0xFF;
		  buff[frameTail++] = tmpData>>32&0xff;
	  }
  	else
  	{
  		#ifdef NO_DATA_USE_PART_ACK_03
        frameTail -= 7;
      #else
        for(i = 0; i < 4; i++)
        {
       	  buff[frameTail++] = 0xee;
        }
      #endif
  	}
	}
	else  //FN=54
	{
   	backTime = timeBcdToHex(tmpTime);
   	tmpCount = monthDays(backTime.year+2000,backTime.month);
   	tmpData = 0;
   	for(i=1;i<=tmpCount;i++)
   	{
   	  tmpTime = backTime;
   	  tmpTime.day = i;
   	  tmpTime = timeHexToBcd(tmpTime);
      if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
      {
         tmpData += terminalStatisRecord.sendBytes+terminalStatisRecord.receiveBytes;
      }
   	}
		buff[frameTail++] = tmpData&0xff;
		buff[frameTail++] = tmpData>>8&0xff;
		buff[frameTail++] = tmpData>>16&0xFF;
		buff[frameTail++] = tmpData>>32&0xff;
	}
		
	return frameTail;
}

/*************************************************************
��������:AFN0D054
��������:��Ӧ��վ�����������"�ն�����վ��ͨ������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D054(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	 return AFN0D053(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D055
��������:��Ӧ��վ�����������"�ն��� �ն��ռ��г���ͳ����Ϣ(�����Լ)"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D055(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	TERMINAL_STATIS_RECORD terminalStatisRecord;
	DATE_TIME              tmpTime;
	INT16U                 i;
	
	buff[frameTail++] = *pHandle++;		//DA1
	buff[frameTail++] = *pHandle++;		//DA2
	buff[frameTail++] = *pHandle++;		//DT1
	buff[frameTail++] = *pHandle++;		//DT2
	
	//����������ʱ��Td_d Td_m
	tmpTime.second = 0x59;
	tmpTime.minute = 0x59;
	tmpTime.hour   = 0x23;
	
	*offset0d = 3;
	buff[frameTail++] = tmpTime.day   = *pHandle++;
	buff[frameTail++] = tmpTime.month = *pHandle++;
	buff[frameTail++] = tmpTime.year  = *pHandle++;
	
	if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
	{
    //ע��������
    buff[frameTail++] = meterDeviceNum&0xff;
    buff[frameTail++] = meterDeviceNum>>8&0xff;
  
    //�ص��û�����
	  buff[frameTail++] = keyHouseHold.numOfHousehold;
	
	  //����ʧ�ܵ������
	  buff[frameTail++] = 0;
	  buff[frameTail++] = 0;
	}
  else
  {
  		#ifdef NO_DATA_USE_PART_ACK_03
        frameTail -= 7;
      #else
        for(i = 0; i < 5; i++)
        {
       	  buff[frameTail++] = 0xee;
        }
      #endif
  }
	
	return frameTail;
}

/*************************************************************
��������:AFN0D056
��������:��Ӧ��վ�����������"�ն��� �ն��ռ��г����м�ͳ����Ϣ(�����Լ)"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D056(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	TERMINAL_STATIS_RECORD terminalStatisRecord;
	DATE_TIME              tmpTime;
	INT16U                 i;
	
	buff[frameTail++] = *pHandle++;		//DA1
	buff[frameTail++] = *pHandle++;		//DA2
	buff[frameTail++] = *pHandle++;		//DT1
	buff[frameTail++] = *pHandle++;		//DT2
	
	//����������ʱ��Td_d Td_m
	tmpTime.second = 0x59;
	tmpTime.minute = 0x59;
	tmpTime.hour   = 0x23;
	
	*offset0d = 3;
	buff[frameTail++] = tmpTime.day   = *pHandle++;
	buff[frameTail++] = tmpTime.month = *pHandle++;
	buff[frameTail++] = tmpTime.year  = *pHandle++;
	
	if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
	{
    //0���м�·�ɱ���
    buff[frameTail++] = meterDeviceNum&0xff;
    buff[frameTail++] = meterDeviceNum>>8&0xff;

    //1���м�·�ɱ���
    buff[frameTail++] = 0x0;
    buff[frameTail++] = 0x0;

    //2���м�·�ɱ���
    buff[frameTail++] = 0x0;
    buff[frameTail++] = 0x0;

    //3���м�·�ɱ���
    buff[frameTail++] = 0x0;
    buff[frameTail++] = 0x0;
    
    //4���м�·�ɱ���
    buff[frameTail++] = 0x0;
    buff[frameTail++] = 0x0;
    
    //5���м�·�ɱ���
    buff[frameTail++] = 0x0;
    buff[frameTail++] = 0x0;
    
    //6���м�·�ɱ���
    buff[frameTail++] = 0x0;
    buff[frameTail++] = 0x0;
    
    //7���м�·�ɱ���
    buff[frameTail++] = 0x0;
    buff[frameTail++] = 0x0;
    
    //8���м�·�ɱ���
    buff[frameTail++] = 0x0;
    buff[frameTail++] = 0x0;
    
    //9���м�·�ɱ���
    buff[frameTail++] = 0x0;
    buff[frameTail++] = 0x0;
	}
  else
  {
  		#ifdef NO_DATA_USE_PART_ACK_03
        frameTail -= 7;
      #else
        for(i = 0; i < 20; i++)
        {
       	  buff[frameTail++] = 0xee;
        }
      #endif
  }
	
	return frameTail;
}

/*************************************************************
��������:AFN0D057
��������:��Ӧ��վ�����������"�ܼ����������С�й����ʼ�
         ����ʱ�䣬�й�����Ϊ��ʱ��"������ݸ�ʽ02��18��BIN��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D057(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_ZJZ_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U queryType, dataType;
	INT8U i, tmpDay;
	
	INT16U offset;
	INT16U tmpFrameTail;
  
  DATE_TIME tmpTime;
  
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x1) == 0x1)
 		{
 			pn = tmpPn + (da2 - 1) * 8;
 			
 			tmpTime.second = 0x59;
 			tmpTime.minute = 0x59;
 			tmpTime.hour = 0x23;
 			if(fn == 57)
 			{
 				*offset0d = 3;
 				tmpTime.day = *pHandle++;
 				tmpTime.month = *pHandle++;
 				tmpTime.year = *pHandle++;
 			}
 			else		//f60
 			{
 				*offset0d = 2;
 				tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + (*(pHandle + 1) & 0xF)
 															, (*pHandle >> 4) * 10 + (*pHandle & 0xF));
 				tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
 				tmpTime.month = *pHandle++;
 				tmpTime.year = *pHandle++;
 			}
 			
 			buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DT1
 			buff[frameTail++] = (pn - 1) / 8 + 1;											//DT2
 			if(fn == 57)
 			{
 				buff[frameTail++] = 0x01;																//DA1
 				queryType = DAY_BALANCE;
 				dataType = GROUP_DAY_BALANCE;
 				offset = GP_DAY_MAX_POWER;
 			}
 			else
 			{
 				buff[frameTail++] = 0x08;																//DA1
 				queryType = MONTH_BALANCE;
 				dataType = GROUP_MONTH_BALANCE;
 				offset = GP_MONTH_MAX_POWER;
 			}
 			buff[frameTail++] = 0x07;																	//DA2
 			
 			//����������ʱ�� f57:Td_d f60:Td_m
 			if(fn == 57)
 			{
 				buff[frameTail++] = tmpTime.day;
 			}
 			buff[frameTail++] = tmpTime.month;
 			buff[frameTail++] = tmpTime.year;
 			
 			if(readMeterData(dataBuff, pn, queryType, dataType, &tmpTime, 0) == TRUE)
 			{
 				//����й�����
 				buff[frameTail++] = dataBuff[offset + 1];
 				buff[frameTail++] = ((dataBuff[offset] & 0x7) << 5) | (dataBuff[offset] & 0x10)
 																		| (dataBuff[offset + 2] & 0xF);
 				offset += 3;
 				
 				//����й����ʷ���ʱ��
 				buff[frameTail++] = dataBuff[offset++];
 				buff[frameTail++] = dataBuff[offset++];
 				buff[frameTail++] = dataBuff[offset++];
 				
 				//��С�й�����
 				buff[frameTail++] = dataBuff[offset + 1];
 				buff[frameTail++] = ((dataBuff[offset] & 0x7) << 5) | (dataBuff[offset] & 0x10)
 																		| (dataBuff[offset + 2] & 0xF);
 				offset += 3;
 				
 				//��С�й����ʷ���ʱ��
 				buff[frameTail++] = dataBuff[offset++];
 				buff[frameTail++] = dataBuff[offset++];
 				buff[frameTail++] = dataBuff[offset++];
 				
 				//�й�����Ϊ���ۼ�ʱ��
 				buff[frameTail++] = dataBuff[offset++];
 				buff[frameTail++] = dataBuff[offset++];
 			}
 			else
 			{
 				#ifdef NO_DATA_USE_PART_ACK_03
	        frameTail = tmpFrameTail;
	      #else
	        for(i=0;i<12;i++)
	        {
	        	buff[frameTail++] = 0xee;
	        }
	      #endif
 			}
 		}
 		da1 >>= 1;
 	}
  
  return frameTail;
}

/*************************************************************
��������:AFN0D058
��������:��Ӧ��վ�����������"�ն����ܼ������ۼ��й�������
          "������ݸ�ʽ03��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D058(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_ZJZ_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U tariff, queryType, dataType;
	INT8U tmpDay;
	
	DATE_TIME tmpTime;
  
  INT16U i, offset;
  INT16U tmpFrameTail;
  
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
 	{
 		tmpPn++;
 		if((da1 & 0x1) == 0x1)
 		{
 			pn = tmpPn + (da2 - 1) * 8;
 			tmpTime.second = 0x59;
 			tmpTime.minute = 0x59;
 			tmpTime.hour = 0x23;
 			if(fn >= 58 && fn <= 59)
 			{
 				*offset0d = 3;
 				tmpTime.day = *pHandle++;
 				tmpTime.month = *pHandle++;
 				tmpTime.year = *pHandle++;
 			}
 			else		//f61 f62
 			{
 				*offset0d = 2;
 				tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + (*(pHandle + 1) & 0xF)
 															, (*pHandle >> 4) * 10 + (*pHandle & 0xF));
 				
 				tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
 				tmpTime.month = *pHandle++;
 				tmpTime.year = *pHandle++;
 			}
 			
 			buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DT1
 			buff[frameTail++] = (pn - 1) / 8 + 1;											//DT2
 			switch(fn)
 			{
 				case 58:
 					buff[frameTail++] = 0x02;															//DA1
 					queryType = DAY_BALANCE;
 					dataType = GROUP_DAY_BALANCE;
 					offset = GP_DAY_WORK;
 					break;
 				case 59:
 					buff[frameTail++] = 0x04;															//DA1
 					queryType = DAY_BALANCE;
 					dataType = GROUP_DAY_BALANCE;
 					offset = GP_DAY_NO_WORK;
 					break;
 				case 61:
 					buff[frameTail++] = 0x10;															//DA1
 					queryType = MONTH_BALANCE;
 					dataType = GROUP_MONTH_BALANCE;
 					offset = GP_MONTH_WORK;
 					break;
 				case 62:
 					buff[frameTail++] = 0x10;															//DA1
 					queryType = MONTH_BALANCE;
 					dataType = GROUP_MONTH_BALANCE;
 					offset = GP_MONTH_NO_WORK;
 					break;
 			}
 			buff[frameTail++] = 0x07;																	//DA2
 			
 			
 			//����������ʱ�� f58, f59:Td_d f61, f62:Td_m
 			if(fn >= 58 && fn <= 59)
 			{
 				buff[frameTail++] = tmpTime.day;
 			}
 			buff[frameTail++] = tmpTime.month;
 			buff[frameTail++] = tmpTime.year;
 			
 			//������
 			buff[frameTail++] = tariff = numOfTariff(pn);
 			
 			if(readMeterData(dataBuff, pn, queryType, dataType, &tmpTime, 0) == TRUE)
 			{
 				//���й�������(f58,f61) ���޹�������(f59,f62)(�ܣ�����1~M)
 				for(i = 0; i <= tariff; i++)
 				{
 					buff[frameTail++] = dataBuff[offset+3];
          buff[frameTail++] = dataBuff[offset+4];
          buff[frameTail++] = dataBuff[offset+5];
          buff[frameTail++] = (dataBuff[offset+6] & 0x0F)
          	                       | dataBuff[offset];
         offset += 7;
 				}
 			}
 			else
 			{
 				#ifdef NO_DATA_USE_PART_ACK_03
          frameTail = tmpFrameTail;
        #else
        	for(i=0;i<(tariff+1)*4;i++)
          {
          	buff[frameTail++] = 0xEE;
          }
        #endif
 			}
 		}
 		da1 >>= 1;
 	}

  return frameTail;
}

/*************************************************************
��������:AFN0D059
��������:��Ӧ��վ�����������"�ն����ܼ������ۼ��޹�������
          "������ݸ�ʽ03��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D059(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D058(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D060
��������:��Ӧ��վ�����������"�¶����ܼ����������С�й�
          ���ʼ��䷢��ʱ��"������ݸ�ʽ02��18��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D060(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{	
	return AFN0D057(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D061
��������:��Ӧ��վ�����������"�¶����ܼ������й�������"������ݸ�ʽ03��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D061(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D058(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D062
��������:��Ӧ��վ�����������"�¶����ܼ������޹�������"������ݸ�ʽBIN��03��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D062(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D058(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D065
��������:��Ӧ��վ�����������"�¶����ܼ��鳬���ʶ�ֵ�����ۼ�ʱ�估���ۼƵ�����"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D065(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
    *offset0d = 2;
    return frameTail;
}

/*************************************************************
��������:AFN0D066
��������:��Ӧ��վ�����������"�¶����ܼ��鳬�µ�������ֵ�����ۼ�ʱ�估���ۼƵ�����"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D066(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
    *offset0d = 2;
    return frameTail;
}

/*************************************************************
��������:AFN0D073
��������:��Ӧ��վ�����������"�ܼ����й���������"������ݸ�ʽ02��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D073(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LEN_OF_ZJZ_BALANCE_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U i, interval;
	INT8U da1, da2;
	INT8U density, dataNum;
	
	DATE_TIME tmpTime, readTime;
	
	INT16U offset, tmpFrameTail;
	
	*offset0d = 7;
	tmpFrameTail = frameTail;
	da1 = *pHandle++;
	da2 = *pHandle++;
	pHandle += 2;
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x1) == 0x1)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
			//��ʼʱ��
			tmpTime.second = 0;
			tmpTime.minute = *pHandle++;
			tmpTime.hour = *pHandle++;
			tmpTime.day = *pHandle++;
			tmpTime.month = *pHandle++;
			tmpTime.year = *pHandle++;
			
			//�����ܶ�
			density = *pHandle++;
			
			switch(density)
			{
				case 1:
					interval = 15;
					break;
				case 2:
					interval = 30;
					break;
				case 3:
					interval = 60;
					break;
				case 254:
					interval = 5;
					break;
				case 255:
					interval = 1;
					break;
				default:	//��������������
					interval = 61;
			}
			
			//��Ч����
			if(interval > 60)
			{
				return tmpFrameTail;
			}
			
			//���ݵ���
			dataNum = *pHandle++;
			
			buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
			buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
			switch(fn)
			{
				case 73:
					buff[frameTail++] = 0x01;															//DT1
					offset = GP_WORK_POWER;			//�ܼ����й���������
					break;
				case 74:
					buff[frameTail++] = 0x02;															//DT1
					offset = GP_NO_WORK_POWER;	//�ܼ����޹���������
					break;
				case 75:
					buff[frameTail++] = 0x04;															//DT1
					offset = GP_DAY_WORK;				//�ܼ����й�����������
					break;
				case 76:
					buff[frameTail++] = 0x08;															//DT1
					offset = GP_DAY_NO_WORK;		//�ܼ����޹�����������
					break;
			}
			buff[frameTail++] = 0x09;																	//DT2
			
			//Td_c
			buff[frameTail++] = tmpTime.minute;
			buff[frameTail++] = tmpTime.hour;
			buff[frameTail++] = tmpTime.day;
			buff[frameTail++] = tmpTime.month;
			buff[frameTail++] = tmpTime.year;
			
			//�����ܶ�m
			buff[frameTail++] = density;
			
			//���ݵ���
			buff[frameTail++] = dataNum;
			
			//ʱ��ת��
			tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
			
			for(i = 0; i < dataNum; i++)
			{
				readTime = timeHexToBcd(tmpTime);
				if(readMeterData(dataBuff, pn, CURVE_DATA_BALANCE, GROUP_REAL_BALANCE, &readTime, hexToBcd(interval)) == TRUE)
				{
					if(fn >= 73 && fn <= 74)
					{
						buff[frameTail++] = dataBuff[offset + 1];
						buff[frameTail++] = ((dataBuff[offset] & 0x7) << 5) | (dataBuff[offset] & 0x10)
																			| (dataBuff[offset + 2] & 0xF);
					}
					else if(fn >= 75 && fn <= 76)
					{
						buff[frameTail++] = dataBuff[offset + 3];
						buff[frameTail++] = dataBuff[offset + 4];
						buff[frameTail++] = dataBuff[offset + 5];
						buff[frameTail++] = ((dataBuff[offset] & 0x1) << 6) | (dataBuff[offset] & 0x10)
																	| (dataBuff[offset + 6]);
					}
				}
				else
				{
					if(fn >= 73 && fn <= 74)
					{
						buff[frameTail++] = 0xEE;
						buff[frameTail++] = 0xEE;
					}
					else
					{
						buff[frameTail++] = 0xEE;
						buff[frameTail++] = 0xEE;
						buff[frameTail++] = 0xEE;
						buff[frameTail++] = 0xEE;
					}
				}
				
				tmpTime = nextTime(tmpTime, interval, 0);
			}
		}
		da1 >>= 1;
	}
	
  return frameTail;
}

/*************************************************************
��������:AFN0D074
��������:��Ӧ��վ�����������"�ܼ����޹���������"������ݸ�ʽ02��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D074(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D073(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D075
��������:��Ӧ��վ�����������"�ܼ����й�����������"������ݸ�ʽ03��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D075(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D073(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D076
��������:��Ӧ��վ�����������"�ܼ����޹�����������"������ݸ�ʽ03��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D076(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D073(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D077
��������:��Ӧ��վ�����������"���������ڹ�������"������ݸ�ʽ09��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D077(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}
/*************************************************************
��������:AFN0D078
��������:��Ӧ��վ�����������"������A�����ڹ�������"������ݸ�ʽ09��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D078(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}
/*************************************************************
��������:AFN0D079
��������:��Ӧ��վ�����������"������B�����ڹ�������"������ݸ�ʽ09��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D079(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}
/*************************************************************
��������:AFN0D080
��������:��Ӧ��վ�����������"������C�����ڹ�������"������ݸ�ʽ09��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D080(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D081
��������:��Ӧ��վ�����������"�������й���������"������ݸ�ʽ09��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
�޸���ʷ:
  1.2012-06-06,
    1)֧���ص��û��������ߵĶ�ȡ
    2)ĳЩ�����B��C���0xff���ݵĴ���
*************************************************************/
INT16U AFN0D081(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
 #ifdef LIGHTING
  METER_DEVICE_CONFIG  meterConfig;
  BOOL      pnSeted=FALSE;
 #endif
  INT8U     meterInfo[10];
	INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     density, dataNum;
	INT8U     i, interval, tmpMinute;
	BOOL      bufHasData = FALSE;

  DATE_TIME tmpTime,readTime;
  
  INT16U    offset, tmpFrameTail;

  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
      
     #ifdef LIGHTING
  	  pnSeted = FALSE;
      if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
      {
  	    pnSeted = TRUE;
      }
     #endif

      queryMeterStoreInfo(pn, meterInfo);

  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
  		switch(fn)
  		{
  			//2017-7-18,���,���ڹ�������77-80
				case 77:		//���������ڹ�������
  				buff[frameTail++] = 0x10;															//DT1
  				offset = POWER_INSTANT_APPARENT;
  				break;
  				
  			case 78:		//������A�����ڹ�������
  				buff[frameTail++] = 0x20;															//DT1
  				offset = POWER_PHASE_A_APPARENT;
  				break;
  				
  			case 79:		//������B�����ڹ�������
  				buff[frameTail++] = 0x40;															//DT1
  				offset = POWER_PHASE_B_APPARENT;
  				break;
  				
  			case 80:		//������C�����ڹ�������
  				buff[frameTail++] = 0x80;															//DT1
  				offset = POWER_PHASE_C_APPARENT;
  				break;
				
				
				
				
  			case 81:		//�������й���������
  				buff[frameTail++] = 0x01;															//DT1
  				offset = POWER_INSTANT_WORK;
  				break;
  				
  			case 82:		//������A���й���������
  				buff[frameTail++] = 0x02;															//DT1
  				offset = POWER_PHASE_A_WORK;
  				break;
  				
  			case 83:		//������B���й���������
  				buff[frameTail++] = 0x04;															//DT1
  				offset = POWER_PHASE_B_WORK;
  				break;
  				
  			case 84:		//������C���й���������
  				buff[frameTail++] = 0x08;															//DT1
  				offset = POWER_PHASE_C_WORK;
  				break;
  				
  			case 85:		//�������޹���������
  				buff[frameTail++] = 0x10;															//DT1
  				offset = POWER_INSTANT_NO_WORK;
  				break;
  				
  			case 86:		//������A���޹���������
  				buff[frameTail++] = 0x20;															//DT1
  				offset = POWER_PHASE_A_NO_WORK;
  				break;
  				
  			case 87:		//������B���޹���������
  				buff[frameTail++] = 0x40;															//DT1
  				offset = POWER_PHASE_B_NO_WORK;
  				break;
  				
  			case 88:		//������C���޹���������
  				buff[frameTail++] = 0x80;															//DT1
  				offset = POWER_PHASE_C_NO_WORK;
  				break;
  		}
			if (fn<81)
			{
  		  buff[frameTail++] = 0x09;															  //DT2			
			}
			else
			{
  		  buff[frameTail++] = 0x0A;																//DT2
			}
  		
  		//����������ʱ��Td_c
  		//��ʼʱ�䣺��ʱ������
  		tmpTime.second = 0;
  		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  		
  		//���ݶ����ܶ�
  		buff[frameTail++] = density = *(pHandle+5);
  		switch(density)
  		{
				case 1:
					interval = 15;
					break;
					
				case 2:
					interval = 30;
					break;
					
				case 3:
					interval = 60;
					break;
					
				case 254:
					interval = 5;
					break;
					
				case 255:
					interval = 1;
					break;
					
				default:	//��������������
					interval = 61;
					break;
			}
			
			//��Ч�����ж�
			if(interval > 60)
			{
				return tmpFrameTail;
			}
			
			//���ݵ���
			buff[frameTail++] = dataNum = *(pHandle+6);
			
			//ʱ��ת��
		 #ifdef LIGHTING
			if (TRUE==pnSeted 
				  && (
				      (31==(meterConfig.rateAndPort&0x1f))    //���ƿ��������Ƶ�
				      || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)    //�������Ƶ�
				     )
				 )
			{
			  tmpTime = timeBcdToHex(tmpTime);
			}
			else
			{
		 #endif
		 
			  tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
		 
		 #ifdef LIGHTING
		  } 
		 #endif
		 
			for(i = 0; i < dataNum; i++)
			{
				//2012-10-22,�޸��������,�ڸ�����������ʱ����,�ò�ͬ���ܶ��ٻ�����������ĳ�㲻һ��
				//    ԭ��:��ȡ��������ʱ����readMeterData��������
				//tmpMinute = hexToBcd(tmpTime.minute+interval);
				tmpMinute = hexToBcd(interval);
				
			 #ifdef LIGHTING
			  if (TRUE==pnSeted 
				    && (
				        (31==(meterConfig.rateAndPort&0x1f))    //���ƿ��������Ƶ�
				        || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)    //�������Ƶ�
				       )
				   )
			  {
				  readTime = tmpTime;
				  bufHasData = readMeterData(dataBuff, pn, HOUR_FREEZE_SLC, 0, &readTime, 0);
				  if (85==fn)
				  {
					  offset = 13;
				  }
				  else
				  {
					  if (81==fn)
					  {
					    offset = 10;
					  }
					  else
					  {
						  bufHasData = FALSE;
					  }
					}
				}
				else
				{
			 #endif
				
				  readTime  = timeHexToBcd(tmpTime);
				  if (meterInfo[0]==8)
				  {
				    bufHasData = readMeterData(dataBuff, pn, CURVE_KEY_HOUSEHOLD, PARA_VARIABLE_DATA, &readTime, tmpMinute);
				  }
				  else
				  {
				    bufHasData = readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, PARA_VARIABLE_DATA, &readTime, tmpMinute);
				  }
			 #ifdef LIGHTING
			  }
			 #endif
				
				if(bufHasData == TRUE)
				{
          if (dataBuff[offset] != 0xFF)
          {
				    buff[frameTail++] = dataBuff[offset+0];
				    buff[frameTail++] = dataBuff[offset+1];
				    buff[frameTail++] = dataBuff[offset+2];
          }
          else
          {
            buff[frameTail++] = 0xee;
            buff[frameTail++] = 0xee;
            buff[frameTail++] = 0xee;
          }
				}
				else
				{
					 buff[frameTail++] = 0xEE;
					 buff[frameTail++] = 0xEE;
					 buff[frameTail++] = 0xEE;
				}
				
				tmpTime = nextTime(tmpTime, interval, 0);
				
				usleep(10000);
			}
  	}
  	da1 >>= 1;
  }
	
  return frameTail;
}

/*************************************************************
��������:AFN0D082
��������:��Ӧ��վ�����������"������A���й���������"������ݸ�ʽ09��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D082(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D083
��������:��Ӧ��վ�����������"������B���й���������"������ݸ�ʽ09��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D083(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D084
��������:��Ӧ��վ�����������"������C���й���������"������ݸ�ʽ09��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D084(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D085
��������:��Ӧ��վ�����������"�������޹���������"������ݸ�ʽ09��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D085(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D086
��������:��Ӧ��վ�����������"������A���޹���������"������ݸ�ʽ09��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D086(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D087
��������:��Ӧ��վ�����������"������B���޹���������"������ݸ�ʽ09��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D087(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D088
��������:��Ӧ��վ�����������"������C���޹���������"������ݸ�ʽ09��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D088(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D081(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D089
��������:��Ӧ��վ�����������"������A���ѹ����"������ݸ�ʽ07��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
�޸���ʷ:
  1.2012-06-06,
    1)֧���ص��ѹ���ߵĶ�ȡ
    2)ĳЩ�����B��C���0xff���ݵĴ���
*************************************************************/
INT16U AFN0D089(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
 #ifdef LIGHTING
  METER_DEVICE_CONFIG  meterConfig;
  BOOL      pnSeted=FALSE;
 #endif
  INT8U     meterInfo[10];
	BOOL      bufHasData = FALSE;
	INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     density, dataNum;
	INT8U     i, interval, tmpMinute;
	
  DATE_TIME tmpTime, readTime;
  
  INT16U    offset, tmpFrameTail;
  
  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		
     #ifdef LIGHTING
  	  pnSeted = FALSE;
      if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
      {
  	    pnSeted = TRUE;
      }
     #endif

      queryMeterStoreInfo(pn, meterInfo);

  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
  		switch(fn)
  		{
  			case 89:		//������A���ѹ����
  				buff[frameTail++] = 0x01;															//DT1
  				offset = VOLTAGE_PHASE_A;
  				break;
  			case 90:		//������B���ѹ����
  				buff[frameTail++] = 0x02;															//DT1
  				offset = VOLTAGE_PHASE_B;
  				break;
  			case 91:		//������C���ѹ����
  				buff[frameTail++] = 0x04;															//DT1
  				offset = VOLTAGE_PHASE_C;
  				break;
  		}
  		buff[frameTail++] = 0x0B;																	//DT2
  		
  		//����������ʱ��Td_c
  		//��ʼʱ�䣺��ʱ������
  		tmpTime.second = 0;
  		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  		density = *(pHandle+5);    //���ݶ����ܶ�
			dataNum = *(pHandle+6);    //���ݵ���

  	 #ifdef LIGHTING
  	  //�������Ƶ�ķ����ܶ���Ϊ15����һ������
  	  if (TRUE==pnSeted && meterConfig.protocol==LIGHTING_DGM)
  	  {
  	 	  if (density==3)
  	 	  {
  	 	  	dataNum *= 4;
  	 	  }
  	 	  if (density==2)
  	 	  {
  	 	  	dataNum *=2;
  	 	  }
  	 	  
  	 	  density = 0x1;
  	  }
  	 #endif
  		
  		//���ݶ����ܶ�
  		buff[frameTail++] = density;

  		switch(density)
  		{
				case 1:
					interval = 15;
					break;
				case 2:
					interval = 30;
					break;
				case 3:
					interval = 60;
					break;
				case 254:
					interval = 5;
					break;
				case 255:
					interval = 1;
					break;
				default:	//��������������
					interval = 61;
			}
			
			//��Ч�����ж�
			if(interval > 60)
			{
				return tmpFrameTail;
			}
			
			//���ݵ���
			buff[frameTail++] = dataNum;
			
			//ʱ��ת��
		 #ifdef LIGHTING
			if (TRUE==pnSeted 
				  && (
				      (31==(meterConfig.rateAndPort&0x1f))    //���ƿ��������Ƶ�
				      || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)    //�������Ƶ�
				     )
				 )
			{
			  tmpTime = timeBcdToHex(tmpTime);
			}
			else
			{
		 #endif
		 
			  tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
		 
		 #ifdef LIGHTING
		  }
		 #endif
			
			for(i = 0; i < dataNum; i++)
			{
				//2012-10-22,�޸��������,�ڸ�����������ʱ����,�ò�ͬ���ܶ��ٻ�����������ĳ�㲻һ��
				//    ԭ��:��ȡ��������ʱ����readMeterData��������
				//tmpMinute = hexToBcd(tmpTime.minute+interval);
				tmpMinute = hexToBcd(interval);

			 #ifdef LIGHTING
			  if (TRUE==pnSeted 
				    && (
				        (31==(meterConfig.rateAndPort&0x1f))    //���ƿ��������Ƶ�
				        || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)    //�������Ƶ�
				       )
				   )
			  {
			    if (89==fn)
			    {
				    readTime  = tmpTime;
				    bufHasData = readMeterData(dataBuff, pn, HOUR_FREEZE_SLC, 0, &readTime, 0);
				    offset = 5;
			    }
			    else
			    {
			  	  bufHasData = FALSE;
			    }
			  }
			  else
			  {
			 #endif
				  
				  readTime  = timeHexToBcd(tmpTime);
				  if (meterInfo[0]==8)
				  {
				    bufHasData = readMeterData(dataBuff, pn, CURVE_KEY_HOUSEHOLD, PARA_VARIABLE_DATA, &readTime, tmpMinute);
				  }
				  else
				  {
				    bufHasData = readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, PARA_VARIABLE_DATA, &readTime, tmpMinute);
				  }
				  
			 #ifdef LIGHTING
				}
			 #endif

				if(bufHasData == TRUE)
				{
					if (dataBuff[offset]!=0xFF)
					{
					  buff[frameTail++] = dataBuff[offset+0];
					  buff[frameTail++] = dataBuff[offset+1];
					}
					else
					{
					  buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;
					}
				}
				else
				{
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
				}
				
				tmpTime = nextTime(tmpTime, interval, 0);
		    
		    usleep(30000);
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*************************************************************
��������:AFN0D090
��������:��Ӧ��վ�����������"������B���ѹ����"������ݸ�ʽ07��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D090(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D089(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D091
��������:��Ӧ��վ�����������"������C���ѹ����"������ݸ�ʽ07��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D091(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D089(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D092
��������:��Ӧ��վ�����������"������A���������"������ݸ�ʽ25��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
�޸���ʷ:
  1.2012-06-06,
    1)֧���ص�������ߵĶ�ȡ
    2)ĳЩ�����B��C���0xff���ݵĴ���
*************************************************************/
INT16U AFN0D092(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
 #ifdef LIGHTING
  METER_DEVICE_CONFIG  meterConfig;
  BOOL      pnSeted=FALSE;
 #endif
  INT8U     meterInfo[10];
	BOOL      bufHasData = FALSE;
	INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     density, dataNum;
	INT8U     i, interval, tmpMinute;
	
  DATE_TIME tmpTime, readTime;
  
  INT16U    offset, tmpFrameTail;
  
  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		
     #ifdef LIGHTING
  	  pnSeted = FALSE;
      if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
      {
  	    pnSeted = TRUE;
      }
     #endif

      queryMeterStoreInfo(pn, meterInfo);

  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));	//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
  		switch(fn)
  		{
  			case 92:		//������A���������
  				buff[frameTail++] = 0x08;															//DT1
  				offset = CURRENT_PHASE_A;
  				break;
  				
  			case 93:		//������B���������
  				buff[frameTail++] = 0x10;															//DT1
  				offset = CURRENT_PHASE_B;
  				break;
  				
  			case 94:		//������C���������
  				buff[frameTail++] = 0x20;															//DT1
  				offset = CURRENT_PHASE_C;
  				break;
  				
  			case 95:		//�����������������
  				buff[frameTail++] = 0x40;															//DT1
  				offset = ZERO_SERIAL_CURRENT;
  				break;
  		}
  		
  		buff[frameTail++] = 0x0B;																	//DT2
  		
  		//����������ʱ��Td_c
  		//��ʼʱ�䣺��ʱ������
  		tmpTime.second = 0;
  		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  		density = *(pHandle+5);    //���ݶ����ܶ�
			dataNum = *(pHandle+6);    //���ݵ���

  	 #ifdef LIGHTING
  	  //�������Ƶ�ķ����ܶ���Ϊ15����һ������
  	  if (TRUE==pnSeted && meterConfig.protocol==LIGHTING_DGM)
  	  {
  	 	  if (density==3)
  	 	  {
  	 	  	dataNum *= 4;
  	 	  }
  	 	  if (density==2)
  	 	  {
  	 	  	dataNum *=2;
  	 	  }
  	 	  
  	 	  density = 0x1;
  	  }
  	 #endif
  	 
  		//���ݶ����ܶ�
  		buff[frameTail++] = density;

  		switch(density)
  		{
				case 1:
					interval = 15;
					break;
				case 2:
					interval = 30;
					break;
				case 3:
					interval = 60;
					break;
				case 254:
					interval = 5;
					break;
				case 255:
					interval = 1;
					break;
				default:	//��������������
					interval = 61;
			}
			
			//��Ч�����ж�
			if(interval > 60)
			{
				return tmpFrameTail;
			}
			
			//���ݵ���
			buff[frameTail++] = dataNum;
			
			//ʱ��ת��
		 #ifdef LIGHTING
			if (TRUE==pnSeted 
				  && (
				      (31==(meterConfig.rateAndPort&0x1f))    //���ƿ��������Ƶ�
				      || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)    //�������Ƶ�
				     )
				 )
			{
			  tmpTime = timeBcdToHex(tmpTime);
			}
			else
			{
		 #endif
		 
			  tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
			  
		 #ifdef LIGHTING
			}
		 #endif
			
			for(i = 0; i < dataNum; i++)
			{
				//2012-10-22,�޸��������,�ڸ�����������ʱ����,�ò�ͬ���ܶ��ٻ�����������ĳ�㲻һ��
				//    ԭ��:��ȡ��������ʱ����readMeterData��������
				//tmpMinute = hexToBcd(tmpTime.minute+interval);
				tmpMinute = hexToBcd(interval);
				
			 #ifdef LIGHTING
			  if (TRUE==pnSeted 
				    && (
				        (31==(meterConfig.rateAndPort&0x1f))    //���ƿ��������Ƶ�
				        || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)    //�������Ƶ�
				       )
				   )
		  	{
				  if (92==fn)
				  {
				    readTime  = tmpTime;
				    bufHasData = readMeterData(dataBuff, pn, HOUR_FREEZE_SLC, 0, &readTime, 0);
				    offset = 7;
				  }
				  else
				  {
					  bufHasData = FALSE;
					}
				}
				else
				{
			 #endif
				
				  readTime  = timeHexToBcd(tmpTime);
				
				  if (meterInfo[0]==8)
				  {
				    bufHasData = readMeterData(dataBuff, pn, CURVE_KEY_HOUSEHOLD, PARA_VARIABLE_DATA, &readTime, tmpMinute);
				  }
				  else
				  {
				    bufHasData = readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, PARA_VARIABLE_DATA, &readTime, tmpMinute);
				  }
				  
			 #ifdef LIGHTING
				}
			 #endif

				if(bufHasData == TRUE)
				{
          if (dataBuff[offset]!=0xFF)
          {
					  buff[frameTail++] = dataBuff[offset+0];
					  buff[frameTail++] = dataBuff[offset+1];
					  buff[frameTail++] = dataBuff[offset+2];
					}
					else
					{
					  buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;						 
					}
				}
				else
				{
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
				}

				tmpTime = nextTime(tmpTime, interval, 0);

		    usleep(30000);
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*************************************************************
��������:AFN0D093
��������:��Ӧ��վ�����������"������B���������"������ݸ�ʽ25��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D093(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D092(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D094
��������:��Ӧ��վ�����������"������C���������"������ݸ�ʽ25��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D094(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D092(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D095
��������:��Ӧ��վ�����������"���������������������"������ݸ�ʽ25��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D095(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D092(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D096
��������:��Ӧ��վ�����������"������Ƶ������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
�޸���ʷ:
  1.2017-07-19,
    1)ӦͬԶҪ����ӱ�����
*************************************************************/
INT16U AFN0D096(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
  INT8U     meterInfo[10];
  BOOL      bufHasData = FALSE;
  INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
  INT16U    pn, tmpPn = 0;
  INT8U     da1, da2;
  INT8U     density, dataNum;
  INT8U     i, interval, tmpMinute;
	
  DATE_TIME tmpTime, readTime;
  
  INT16U    offset, tmpFrameTail;
  
  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  	  pn = tmpPn + (da2 - 1) * 8;
  		

      queryMeterStoreInfo(pn, meterInfo);

  	  buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));    //DA1
  	  buff[frameTail++] = (pn - 1) / 8 + 1;						  //DA2
      
	  	//������Ƶ������
  	  buff[frameTail++] = 0x80;									  //DT1
  	  offset = METER_STATUS_WORD;
 	  	buff[frameTail++] = 0x0B;									  //DT2
  		
  	  //����������ʱ��Td_c
  	  //��ʼʱ�䣺��ʱ������
  	  tmpTime.second = 0;
  	  buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  	  buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  	  buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  	  buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  	  buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  	  density = *(pHandle+5);    //���ݶ����ܶ�
	  	dataNum = *(pHandle+6);    //���ݵ���
  	 
  	  //���ݶ����ܶ�
  	  buff[frameTail++] = density;

		  switch(density)
		  {
				case 1:
				  interval = 15;
				  break;
				case 2:
				  interval = 30;
				  break;
				case 3:
				  interval = 60;
				  break;
				case 254:
				  interval = 5;
				  break;
				case 255:
				  interval = 1;
				  break;
				default:	//��������������
				  interval = 61;
		  }
			
		  //��Ч�����ж�
		  if(interval > 60)
		  {
				return tmpFrameTail;
		  }
			
		  //���ݵ���
		  buff[frameTail++] = dataNum;
				
		  //ʱ��ת��
		  tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
				  
				
		  for(i = 0; i < dataNum; i++)
		  {
				//2012-10-22,�޸��������,�ڸ�����������ʱ����,�ò�ͬ���ܶ��ٻ�����������ĳ�㲻һ��
				//    ԭ��:��ȡ��������ʱ����readMeterData��������
				//tmpMinute = hexToBcd(tmpTime.minute+interval);
				tmpMinute = hexToBcd(interval);
						
				readTime  = timeHexToBcd(tmpTime);
						
				if (meterInfo[0]==8)
				{
				  bufHasData = readMeterData(dataBuff, pn, CURVE_KEY_HOUSEHOLD, PARA_VARIABLE_DATA, &readTime, tmpMinute);
				}
				else
				{
				  bufHasData = readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, PARA_VARIABLE_DATA, &readTime, tmpMinute);
				}
					  

				if(bufHasData == TRUE)
				{
          if (dataBuff[offset]!=0xFF)
          {
						buff[frameTail++] = dataBuff[offset+0];
						buff[frameTail++] = dataBuff[offset+1];
				  }
				  else
				  {
						buff[frameTail++] = 0xEE;
						buff[frameTail++] = 0xEE;
				  }
				}
				else
				{
				  buff[frameTail++] = 0xEE;
				  buff[frameTail++] = 0xEE;
				}

				tmpTime = nextTime(tmpTime, interval, 0);

				usleep(30000);
		  }
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}


/*************************************************************
��������:AFN0D097
��������:��Ӧ��վ�����������"�����������й��ܵ���������"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D097(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U     dataBuff[LEN_OF_ENERGY_BALANCE_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     density, dataNum;
	INT8U     i, interval, tmpMinute;
	
  DATE_TIME tmpTime, readTime;
  
  INT16U    offset, tmpFrameTail;
  
  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
  		switch(fn)
  		{
  			case 97:		//�����������й��ܵ���������
  				buff[frameTail++] = 0x01;															//DT1
  				offset = DAY_P_WORK_OFFSET;
  				break;
  				
  			case 98:		//�����������޹��ܵ���������
  				buff[frameTail++] = 0x02;															//DT1
  				offset = DAY_P_NO_WORK_OFFSET;
  				break;
  				
  			case 99:		//�����㷴���й��ܵ���������
  				buff[frameTail++] = 0x04;															//DT1
  				offset = DAY_N_WORK_OFFSET;
  				break;
  				
  			case 100:		//�����㷴���޹��ܵ���������
  				buff[frameTail++] = 0x08;															//DT1
  				offset = DAY_N_NO_WORK_OFFSET;
  				break;
  		}
  		buff[frameTail++] = 0x0C;																	//DT2
  		
  		//����������ʱ��Td_c
  		//��ʼʱ�䣺��ʱ������
  		tmpTime.second = 0;
  		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  		
  		//���ݶ����ܶ�
  		buff[frameTail++] = density = *(pHandle+5);
  		switch(density)
  		{
				case 1:
					interval = 15;
					break;
				case 2:
					interval = 30;
					break;
				case 3:
					interval = 60;
					break;
				case 254:
					interval = 5;
					break;
				case 255:
					interval = 1;
					break;
				default:	//��������������
					interval = 61;
			}
			
			//��Ч�����ж�
			if(interval > 60)
			{
				return tmpFrameTail;
			}
			
			//���ݵ���
			buff[frameTail++] = dataNum = *(pHandle+6);
			
			//ʱ��ת��
			tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
			
			for(i = 0; i < dataNum; i++)
			{
				//2012-10-22,�޸��������,�ڸ�����������ʱ����,�ò�ͬ���ܶ��ٻ�����������ĳ�㲻һ��
				//    ԭ��:��ȡ��������ʱ����readMeterData��������
				//tmpMinute = hexToBcd(tmpTime.minute+interval);
				tmpMinute = hexToBcd(interval);
				
				readTime  = timeHexToBcd(tmpTime);
				if(readMeterData(dataBuff, pn, CURVE_DATA_BALANCE, REAL_BALANCE_POWER_DATA, &readTime, tmpMinute) == TRUE)
				{
					buff[frameTail++] = dataBuff[offset + 1];
					buff[frameTail++] = dataBuff[offset + 2];
					buff[frameTail++] = dataBuff[offset + 3];
					buff[frameTail++] = dataBuff[offset + 4];
				}
				else
				{
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
				}

				tmpTime = nextTime(tmpTime, interval, 0);

		    usleep(30000);
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*************************************************************
��������:AFN0D098
��������:��Ӧ��վ�����������"�����������޹��ܵ���������"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D098(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D097(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D099
��������:��Ӧ��վ�����������"�����㷴���й��ܵ���������"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D099(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D097(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D100
��������:��Ӧ��վ�����������"�����㷴���޹��ܵ���������"������ݸ�ʽ13��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D100(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D097(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D101
��������:��Ӧ��վ�����������"�����������й��ܵ���ʾֵ����"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
�޸���ʷ:
  1.2012-06-06,�޸�,�ص��û��͵��໧���ʾֵ�����ܶ�ȡ,��ǰ�İ汾�����ܶ�ȡ
*************************************************************/
INT16U AFN0D101(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	METER_DEVICE_CONFIG meterConfig;
	INT8U               dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U              pn, tmpPn = 0;
	INT8U               da1, da2;
	INT8U               density, dataNum;
	INT8U               i, interval, tmpMinute;
  DATE_TIME           tmpTime, readTime;  
  INT16U              offset, tmpFrameTail;
	INT8U               queryType;
  INT8U               meterInfo[10];
  BOOL                carrierMeterFlag = FALSE;

  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
			
			carrierMeterFlag = FALSE;
      if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
      {
      	if ((meterConfig.rateAndPort&0x1f)==PORT_POWER_CARRIER && meterConfig.protocol==DLT_645_2007)
      	{
      	 	carrierMeterFlag = TRUE;
      	}
        queryMeterStoreInfo(pn, meterInfo);
      }

  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));	//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
  		switch(fn)
  		{
  			case 101:		//�����������й��ܵ���ʾֵ����
  				buff[frameTail++] = 0x10;															//DT1
  				offset = POSITIVE_WORK_OFFSET;
  				break;
  				
  			case 102:		//�����������޹��ܵ���ʾֵ����
  				buff[frameTail++] = 0x20;															//DT1
  				offset = POSITIVE_NO_WORK_OFFSET;
  				break;
  				
  			case 103:		//�����㷴���й��ܵ���ʾֵ����
  				buff[frameTail++] = 0x40;															//DT1
  				offset = NEGTIVE_WORK_OFFSET;
  				break;
  				
  			case 104:		//�����㷴���޹��ܵ���ʾֵ����
  				buff[frameTail++] = 0x80;															//DT1
  				offset = NEGTIVE_NO_WORK_OFFSET;
  				break;
  		}
  		buff[frameTail++] = 0x0C;																	//DT2

  		//ȷ����ѯ����
  		queryType = CURVE_DATA_PRESENT;
  		
  		if(fn == 101 || fn == 103)
  		{
    		//�ص��û�
    		if (meterInfo[0]==8)
    		{
    			queryType = CURVE_KEY_HOUSEHOLD;
    		}
    		else
    		{
      		//�����(�ز�������)
      		if (meterInfo[0]<4)
      		{
      	   #ifdef CQDL_CSM
      		  if (carrierMeterFlag == TRUE)
      		  {
      			  queryType = HOUR_FREEZE;
      			  if (fn==103)
      			  {
      				   offset = HOUR_FREEZE_N_WORK;
      			  }
      		  }
      		  else
      		  {
      		 #endif
      		 
      		  	queryType = CURVE_SINGLE_PHASE;
      		  	
      		  	if (fn==103)
      		  	{
      		  		offset = NEGTIVE_WORK_OFFSET_S;
      		  	}
      		  	
      		 #ifdef CQDL_CSM
      		  }
      		 #endif
      		}
      	}
      }
  		
  		//����������ʱ��Td_c
  		//��ʼʱ�䣺��ʱ������
  		tmpTime.second = 0;
  		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  		
  		//���ݶ����ܶ�
  		buff[frameTail++] = density = *(pHandle+5);
  		switch(density)
  		{
				case 1:
					interval = 15;
					break;
				case 2:
					interval = 30;
					break;
				case 3:
					interval = 60;
					break;
				case 254:
					interval = 5;
					break;
				case 255:
					interval = 1;
					break;
				default:	//��������������
					interval = 61;
			}
			
			//��Ч�����ж�
			if(interval > 60)
			{
				return tmpFrameTail;
			}
			
			//���ݵ���
			buff[frameTail++] = dataNum = *(pHandle+6);
			
  	 #ifdef CQDL_CSM
  		if((fn == 101 || fn == 103) && carrierMeterFlag == TRUE)
  		{
  			tmpTime = timeBcdToHex(tmpTime);
  		}
  		else
  		{
  	 #endif
			  
			  //ʱ��ת��
			 #ifdef LIGHTING
			  if (
			  	  (meterConfig.rateAndPort&0x1f)==PORT_POWER_CARRIER
			  	   || meterConfig.protocol==LIGHTING_DGM    //�������Ƶ�
			  	 )
			  {
			    tmpTime = timeBcdToHex(tmpTime);
			  }
			  else
			  {
			 #endif
			   
			    tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
			    
			 #ifdef LIGHTING
			  }
			 #endif
			  
		 #ifdef CQDL_CSM
		 	}
		 #endif
			
			for(i = 0; i < dataNum; i++)
			{
				//2012-10-22,�޸��������,�ڸ�����������ʱ����,�ò�ͬ���ܶ��ٻ�����������ĳ�㲻һ��
				//    ԭ��:��ȡ��������ʱ����readMeterData��������
				//tmpMinute = hexToBcd(tmpTime.minute+interval);
				tmpMinute = hexToBcd(interval);
				
			 #ifdef LIGHTING
			  if (
			  	  (meterConfig.rateAndPort&0x1f)==PORT_POWER_CARRIER
			  	   || meterConfig.protocol==LIGHTING_DGM    //�������Ƶ�
			  	 )
			  {
				  if (101==fn || 102==fn)
				  {
				    if (101==fn)
				    {
				  	  offset = 18;
				    }
				    else
				    {
				  	  offset = 22;
				    }
				    readTime  = tmpTime;
				    if(readMeterData(dataBuff, pn, HOUR_FREEZE_SLC, 0, &readTime, 0) == TRUE)
				    {
					    buff[frameTail++] = dataBuff[offset+0];
					    buff[frameTail++] = dataBuff[offset+1];
					    buff[frameTail++] = dataBuff[offset+2];
					    buff[frameTail++] = dataBuff[offset+3];
				    }
				    else
				    {
					    buff[frameTail++] = 0xEE;
					    buff[frameTail++] = 0xEE;
					    buff[frameTail++] = 0xEE;
					    buff[frameTail++] = 0xEE;
					  }
				  }
				  else
				  {
					  buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;
				  	buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;
				  }
				}
				else
				{
			 #endif
			 	
				  readTime  = timeHexToBcd(tmpTime);
				  if(readMeterData(dataBuff, pn, queryType, ENERGY_DATA, &readTime, tmpMinute) == TRUE)
				  {
					  buff[frameTail++] = dataBuff[offset+0];
					  buff[frameTail++] = dataBuff[offset+1];
					  buff[frameTail++] = dataBuff[offset+2];
					  buff[frameTail++] = dataBuff[offset+3];
				  }
				  else
				  {
					  buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;
				  }
				  
			 #ifdef LIGHTING
			  }
			 #endif

				tmpTime = nextTime(tmpTime, interval, 0);
				
				usleep(100000);
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*************************************************************
��������:AFN0D102
��������:��Ӧ��վ�����������"�����������޹��ܵ���ʾֵ����"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D102(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D101(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D103
��������:��Ӧ��վ�����������"�����㷴���й��ܵ���ʾֵ����"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D103(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D101(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D104
��������:��Ӧ��վ�����������"�����㷴���޹��ܵ���ʾֵ����"������ݸ�ʽ11��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D104(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D101(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D105
��������:��Ӧ��վ�����������"�����㹦����������"������ݸ�ʽ05��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D105(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
 #ifdef LIGHTING
  METER_DEVICE_CONFIG  meterConfig;
  BOOL      pnSeted=FALSE;
 #endif

	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     density, dataNum;
	INT8U     i, interval, tmpMinute;
	
  DATE_TIME tmpTime,readTime;
  
  INT16U    offset, tmpFrameTail;
  
  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;

     #ifdef LIGHTING
  	  pnSeted = FALSE;
      if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
      {
  	    pnSeted = TRUE;
      }
     #endif

  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
  		switch(fn)
  		{
  			case 105:		//�����㹦����������
  				buff[frameTail++] = 0x01;															//DT1
  				offset = TOTAL_POWER_FACTOR;
  				break;
  				
  			case 106:		//������A�๦����������
  				buff[frameTail++] = 0x02;															//DT1
  				offset = FACTOR_PHASE_A;
  				break;
  				
  			case 107:		//������B�๦����������
  				buff[frameTail++] = 0x04;															//DT1
  				offset = FACTOR_PHASE_B;
  				break;
  				
  			case 108:		//������C�๦����������
  				buff[frameTail++] = 0x08;															//DT1
  				offset = FACTOR_PHASE_C;
  				break;
  		}
  		buff[frameTail++] = 0x0D;																	//DT2
  		
  		//����������ʱ��Td_c
  		//��ʼʱ�䣺��ʱ������
  		tmpTime.second = 0;
  		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  		
  		//���ݶ����ܶ�
  		buff[frameTail++] = density = *(pHandle+5);
  		switch(density)
  		{
				case 1:
					interval = 15;
					break;
				case 2:
					interval = 30;
					break;
				case 3:
					interval = 60;
					break;
				case 254:
					interval = 5;
					break;
				case 255:
					interval = 1;
					break;
				default:	//��������������
					interval = 61;
			}
			
			//��Ч�����ж�
			if(interval > 60)
			{
				return tmpFrameTail;
			}
			
			//���ݵ���
			buff[frameTail++] = dataNum = *(pHandle+6);
			
			//ʱ��ת��
		 #ifdef LIGHTING
			if (TRUE==pnSeted 
				  && (
				      (31==(meterConfig.rateAndPort&0x1f))    //���ƿ��������Ƶ�
				      || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)    //�������Ƶ�
				     )
				 )
			{
			  tmpTime = timeBcdToHex(tmpTime);
			}
			else
			{
		 #endif
		 
  			tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
  	 
  	 #ifdef LIGHTING
  	  }
		 #endif
			
			for(i = 0; i < dataNum; i++)
			{
				//2012-10-22,�޸��������,�ڸ�����������ʱ����,�ò�ͬ���ܶ��ٻ�����������ĳ�㲻һ��
				//    ԭ��:��ȡ��������ʱ����readMeterData��������
				//tmpMinute = hexToBcd(tmpTime.minute+interval);
				tmpMinute = hexToBcd(interval);
				
			 #ifdef LIGHTING
			  if (TRUE==pnSeted 
				    && (
				        (31==(meterConfig.rateAndPort&0x1f))    //���ƿ��������Ƶ�
				        || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)    //�������Ƶ�
				       )
				   )
			  {
				  if (105==fn)
				  {
				    readTime  = tmpTime;
				
				    if(readMeterData(dataBuff, pn, HOUR_FREEZE_SLC, 0, &readTime, 0) == TRUE)
				    {
					    buff[frameTail++] = dataBuff[16];
					    buff[frameTail++] = dataBuff[17];
				    }
				    else
				    {
					    buff[frameTail++] = 0xEE;
					    buff[frameTail++] = 0xEE;
					  }
				  }
				  else
				  {
					  buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;
				  }
				}
				else
				{
			 #endif
			 
				  readTime  = timeHexToBcd(tmpTime);
				  if(readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, PARA_VARIABLE_DATA, &readTime, tmpMinute) == TRUE)
				  {
					  buff[frameTail++] = dataBuff[offset+0];
					  buff[frameTail++] = dataBuff[offset+1];
				  }
				  else
				  {
					  buff[frameTail++] = 0xEE;
					  buff[frameTail++] = 0xEE;
				  }
				  
			 #ifdef LIGHTING
			  }
			 #endif

				tmpTime = nextTime(tmpTime, interval, 0);
				
				usleep(30000);
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*************************************************************
��������:AFN0D106
��������:��Ӧ��վ�����������"������A�๦����������"������ݸ�ʽ05��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D106(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D105(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D107
��������:��Ӧ��վ�����������"������B�๦����������"������ݸ�ʽ05��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D107(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D105(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D108
��������:��Ӧ��վ�����������"������C�๦����������"������ݸ�ʽ05��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D108(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D105(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D109
��������:��Ӧ��վ�����������"�������ѹ��λ������"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D109(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     density, dataNum;
	INT8U     i, interval, tmpMinute;
	
  DATE_TIME tmpTime,readTime;
  
  INT16U    offset, tmpFrameTail;
  
  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));	//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
  		switch(fn)
  		{
  			case 109:		//�������ѹ��λ������
  				buff[frameTail++] = 0x10;															//DT1
  				offset = PHASE_ANGLE_V_A;
  				break;
  				
  			case 110:		//�����������λ������
  				buff[frameTail++] = 0x20;															//DT1
  				offset = PHASE_ANGLE_C_A;
  				break;
  		}
  		
  		buff[frameTail++] = 0x0E;																	//DT2
  		
  		//����������ʱ��Td_c
  		//��ʼʱ�䣺��ʱ������
  		tmpTime.second = 0;
  		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  		
  		//���ݶ����ܶ�
  		buff[frameTail++] = density = *(pHandle+5);
  		switch(density)
  		{
				case 1:
					interval = 15;
					break;
				case 2:
					interval = 30;
					break;
				case 3:
					interval = 60;
					break;
				case 254:
					interval = 5;
					break;
				case 255:
					interval = 1;
					break;
				default:	//��������������
					interval = 61;
			}
			
			//��Ч�����ж�
			if(interval > 60)
			{
				return tmpFrameTail;
			}
			
			//���ݵ���
			buff[frameTail++] = dataNum = *(pHandle+6);
			
			//ʱ��ת��
			tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
			
			for(i = 0; i < dataNum; i++)
			{
				//2012-10-22,�޸��������,�ڸ�����������ʱ����,�ò�ͬ���ܶ��ٻ�����������ĳ�㲻һ��
				//    ԭ��:��ȡ��������ʱ����readMeterData��������
				//tmpMinute = hexToBcd(tmpTime.minute+interval);
				tmpMinute = hexToBcd(interval);
				
				readTime  = timeHexToBcd(tmpTime);
				if(readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, PARA_VARIABLE_DATA, &readTime, tmpMinute) == TRUE)
				{
					//memcpy(&buff[frameTail], &dataBuff[offset],6);
					memset(&buff[frameTail], 0x00, 6);
				}
				else
				{
					//Ϊ���ݹ��繫˾��������					
					memset(&buff[frameTail], 0xee, 6);
					frameTail += 6;					
				}
				frameTail += 6;

				tmpTime = nextTime(tmpTime, interval, 0);
				
				usleep(30000);
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*************************************************************
��������:AFN0D110
��������:��Ӧ��վ�����������"�����������λ������"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D110(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
  return AFN0D109(buff, frameTail, pHandle, fn, offset0d);
}

#ifndef LIGHTING

/*************************************************************
��������:AFN0D111
��������:��Ӧ��վ�����������"����������"����
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
�޸���ʷ:
  1.2017-09-22,
    1)��ӿ���������
*************************************************************/
INT16U AFN0D111(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
  INT8U     meterInfo[10];
  BOOL      bufHasData = FALSE;
  INT8U     dataBuff[LENGTH_OF_PARA_RECORD];
  INT16U    pn, tmpPn = 0;
  INT8U     da1, da2;
  INT8U     density, dataNum;
  INT8U     i, interval, tmpMinute;
	
  DATE_TIME tmpTime, readTime;
  
  INT16U    offset, tmpFrameTail;
  
  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  	  pn = tmpPn + (da2 - 1) * 8;
  		

      queryMeterStoreInfo(pn, meterInfo);

  	  buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));    //DA1
  	  buff[frameTail++] = (pn - 1) / 8 + 1;						  //DA2
      
	  //�����㿪��������
  	  buff[frameTail++] = 0x40;									  //DT1
 	  buff[frameTail++] = 0x0D;									  //DT2
  	  offset = METER_STATUS_WORD_2;
  		
  	  //����������ʱ��Td_c
  	  //��ʼʱ�䣺��ʱ������
  	  tmpTime.second = 0;
  	  buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  	  buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  	  buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  	  buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  	  buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  	  density = *(pHandle+5);    //���ݶ����ܶ�
	  dataNum = *(pHandle+6);    //���ݵ���
  	 
  	  //���ݶ����ܶ�
  	  buff[frameTail++] = density;

	  switch(density)
	  {
		case 1:
		  interval = 15;
		  break;
		case 2:
		  interval = 30;
		  break;
		case 3:
		  interval = 60;
		  break;
		case 254:
		  interval = 5;
		  break;
		case 255:
		  interval = 1;
		  break;
		default:	//��������������
		  interval = 61;
	  }
			
	  //��Ч�����ж�
	  if(interval > 60)
	  {
		return tmpFrameTail;
	  }
			
	  //���ݵ���
	  buff[frameTail++] = dataNum;
			
	  //ʱ��ת��
	  tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
			  
			
	  for(i = 0; i < dataNum; i++)
	  {
		//2012-10-22,�޸��������,�ڸ�����������ʱ����,�ò�ͬ���ܶ��ٻ�����������ĳ�㲻һ��
		//    ԭ��:��ȡ��������ʱ����readMeterData��������
		//tmpMinute = hexToBcd(tmpTime.minute+interval);
		tmpMinute = hexToBcd(interval);
				
		readTime  = timeHexToBcd(tmpTime);
				
		if (meterInfo[0]==8)
		{
		  bufHasData = readMeterData(dataBuff, pn, CURVE_KEY_HOUSEHOLD, PARA_VARIABLE_DATA, &readTime, tmpMinute);
		}
		else
		{
		  bufHasData = readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, PARA_VARIABLE_DATA, &readTime, tmpMinute);
		}
				  

		if(bufHasData == TRUE)
		{
		  buff[frameTail++] = dataBuff[offset+0];
		  buff[frameTail++] = dataBuff[offset+1];
		}
		else
		{
		  buff[frameTail++] = 0xEE;
		  buff[frameTail++] = 0xEE;
		}

		tmpTime = nextTime(tmpTime, interval, 0);

		usleep(30000);
	  }
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

#else

/*************************************************************
��������:AFN0D111
��������:��Ӧ��վ�����������"��������������"������ݸ�ʽ05��
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D111(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
  METER_DEVICE_CONFIG  meterConfig;
  BOOL      pnSeted=FALSE;

	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     density, dataNum;
	INT8U     i, interval, tmpMinute;
	
  DATE_TIME tmpTime,readTime;
  
  INT16U    tmpFrameTail;
  
  *offset0d = 7;
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;

  	  pnSeted = FALSE;
      if (selectF10Data(pn, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
      {
  	    pnSeted = TRUE;
      }

  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));	//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
  		switch(fn)
  		{
  			case 111:		//��������������
  				buff[frameTail++] = 0x40;															//DT1
  				break;
  				
  			case 112:		//�������¶�����
  				buff[frameTail++] = 0x80;															//DT1
  				break;
  		}
  		buff[frameTail++] = 0x0D;																	//DT2
  		
  		//����������ʱ��Td_c
  		//��ʼʱ�䣺��ʱ������
  		tmpTime.second = 0;
  		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  		density = *(pHandle+5);    //���ݶ����ܶ�
			dataNum = *(pHandle+6);    //���ݵ���

  	 #ifdef LIGHTING
  	  //�������Ƶ�ķ����ܶ���Ϊ15����һ������
  	  if (TRUE==pnSeted && meterConfig.protocol==LIGHTING_DGM)
  	  {
  	 	  if (density==3)
  	 	  {
  	 	  	dataNum *= 4;
  	 	  }
  	 	  if (density==2)
  	 	  {
  	 	  	dataNum *=2;
  	 	  }
  	 	  
  	 	  density = 0x1;
  	  }
  	 #endif
  	 
  		//���ݶ����ܶ�
  		buff[frameTail++] = density;
  		 
  		switch(density)
  		{
				case 1:
					interval = 15;
					break;
				case 2:
					interval = 30;
					break;
				case 3:
					interval = 60;
					break;
				case 254:
					interval = 5;
					break;
				case 255:
					interval = 1;
					break;
				default:	//��������������
					interval = 61;
			}
			
			//��Ч�����ж�
			if(interval > 60)
			{
				return tmpFrameTail;
			}
			
			//���ݵ���
			buff[frameTail++] = dataNum;
			
			//ʱ��ת��
			if (TRUE==pnSeted 
				  && (
				      (31==(meterConfig.rateAndPort&0x1f))    //���ƿ��������Ƶ�
				      || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)      //�������Ƶ�
				        || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_XL)     //��·���Ƶ�
									|| (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_TH)	 //ʪ�¶Ȳ�����
				     )
				 )
			{
			  tmpTime = timeBcdToHex(tmpTime);
			}
			else
			{
  			tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
  	  }
			
			for(i = 0; i < dataNum; i++)
			{
				tmpMinute = hexToBcd(interval);
				
			  if (TRUE==pnSeted 
				    && (
				        (31==(meterConfig.rateAndPort&0x1f))    //���ƿ��������Ƶ�
				        || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_DGM)      //�������Ƶ�
				          || (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_XL)     //��·���Ƶ�
										|| (31!=(meterConfig.rateAndPort&0x1f) && meterConfig.protocol==LIGHTING_TH)	 //ʪ�¶Ȳ�����
				       )
				   )
			  {
			    readTime  = tmpTime;
			    if(readMeterData(dataBuff, pn, HOUR_FREEZE_SLC, 0, &readTime, 0) == TRUE)
			    {
				    if (111==fn)
				    {
				      //��ʪ�ȴ�������ʪ�����ڴ˴���				      
				      buff[frameTail++] = dataBuff[28];
				      buff[frameTail++] = dataBuff[29];
				    }
				    else    //FN=112
				    {
				      buff[frameTail++] = dataBuff[26];
				      buff[frameTail++] = dataBuff[27];
				    }
			    }
			    else
			    {
				    buff[frameTail++] = 0xEE;
				    buff[frameTail++] = 0xEE;
				  }
				}
				else
				{
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
			  }

				tmpTime = nextTime(tmpTime, interval, 0);
				
				usleep(30000);
			}
  	}
  	da1 >>= 1;
  }
  
  return frameTail;
}

/*************************************************************
��������:AFN0D112
��������:��Ӧ��վ�����������"�������¶�����"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D112(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
  return AFN0D111(buff, frameTail, pHandle, fn, offset0d);
}

#endif

/*************************************************************
��������:AFN0D113
��������:��Ӧ��վ�����������"�ն��������U��2-19��г�����������ֵ������ʱ��"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D113(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   *offset0d = 3;
	 return frameTail;
}

/*************************************************************
��������:AFN0D114
��������:��Ӧ��վ�����������"�ն��������V��2-19��г�����������ֵ������ʱ��"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D114(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   *offset0d = 3;
	 return frameTail;
}

/*************************************************************
��������:AFN0D115
��������:��Ӧ��վ�����������"�ն��������W��2-19��г�����������ֵ������ʱ��"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D115(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   *offset0d = 3;
	 return frameTail;
}

/*************************************************************
��������:AFN0D116
��������:��Ӧ��վ�����������"�ն��������U��2-19��г����ѹ�����ʼ��ܻ����������ֵ������ʱ��"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D116(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   *offset0d = 3;
	 return frameTail;
}

/*************************************************************
��������:AFN0D117
��������:��Ӧ��վ�����������"�ն��������V��2-19��г����ѹ�����ʼ��ܻ����������ֵ������ʱ��"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D117(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   *offset0d = 3;
	 return frameTail;
}

/*************************************************************
��������:AFN0D118
��������:��Ӧ��վ�����������"�ն��������W��2-19��г����ѹ�����ʼ��ܻ����������ֵ������ʱ��"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D118(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   *offset0d = 3;
	 return frameTail;
}

/*************************************************************
��������:AFN0D121
��������:��Ӧ��վ�����������"�ն��������A��г��Խ����ͳ������"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D121(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   *offset0d = 3;
	 return frameTail;
}

/*************************************************************
��������:AFN0D122
��������:��Ӧ��վ�����������"�ն��������B��г��Խ����ͳ������"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D122(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   *offset0d = 3;
	 return frameTail;
}

/*************************************************************
��������:AFN0D123
��������:��Ӧ��վ�����������"�ն��������C��г��Խ����ͳ������"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D123(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
   *offset0d = 3;
	 return frameTail;
}

/*************************************************************
��������:AFN0D129
��������:��Ӧ��վ�����������"�ն���ֱ��ģ����Խ�����ۼ�ʱ�䡢���/��Сֵ������ʱ��"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D129(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	 #ifdef PLUG_IN_CARRIER_MODULE
  	TERMINAL_STATIS_RECORD terminalStatisRecord;
  	INT8U                  i, tmpDay;
  	DATE_TIME              tmpTime, bakTime;
  	INT8U                  tmpCount;
  	INT32U                 tmpMonthUp,tmpMonthDown;
  	INT16U                 tmpMonthMax,tmpMonthMin;
  	INT8U                  tmpMaxTime[3],tmpMinTime[3];
  	INT32U                 tmpData;
  	
  	buff[frameTail++] = *pHandle++;		//DA1
  	buff[frameTail++] = *pHandle++;		//DA2
  	buff[frameTail++] = *pHandle++;		//DT1
  	buff[frameTail++] = *pHandle++;		//DT2
  	
  	//����������ʱ��Td_d Td_m
  	tmpTime.second = 0x59;
  	tmpTime.minute = 0x59;
  	tmpTime.hour = 0x23;
  	if(fn == 129)
  	{
  		*offset0d = 3;
  		buff[frameTail++] = tmpTime.day   = *pHandle++;
  		buff[frameTail++] = tmpTime.month = *pHandle++;
  		buff[frameTail++] = tmpTime.year  = *pHandle++;
  	}
  	else		//F130
  	{
  		*offset0d = 2;
  		tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + *(pHandle + 1) & 0xF
  													, (*pHandle >> 4) * 10 + *pHandle % 10);
  		tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
  		
  		buff[frameTail++] = tmpTime.month = *pHandle++;
  		buff[frameTail++] = tmpTime.year  = *pHandle++;
  	}
  	
  	if(fn == 129)
  	{
  	  if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
  	  {
  			//ֱ��ģ����Խ�����ۼ�ʱ��
  			buff[frameTail++] = terminalStatisRecord.dcOverUp & 0xFF;
  			buff[frameTail++] = terminalStatisRecord.dcOverUp >> 8;
  			
  			//ֱ��ģ����Խ�����ۼ�ʱ��
  			buff[frameTail++] = terminalStatisRecord.dcOverDown & 0xFF;
  			buff[frameTail++] = terminalStatisRecord.dcOverDown >> 8;

  			//ֱ��ģ�������ֵ
  			buff[frameTail++] = terminalStatisRecord.dcMax[0];
  			buff[frameTail++] = terminalStatisRecord.dcMax[1];

  			//ֱ��ģ�������ֵ����ʱ��
  			buff[frameTail++] = terminalStatisRecord.dcMaxTime[0];
  			buff[frameTail++] = terminalStatisRecord.dcMaxTime[1];
  			buff[frameTail++] = terminalStatisRecord.dcMaxTime[2];

  			//ֱ��ģ������Сֵ
  			buff[frameTail++] = terminalStatisRecord.dcMin[0];
  			buff[frameTail++] = terminalStatisRecord.dcMin[1];

  			//ֱ��ģ������Сֵ����ʱ��
  			buff[frameTail++] = terminalStatisRecord.dcMinTime[0];
  			buff[frameTail++] = terminalStatisRecord.dcMinTime[1];
  			buff[frameTail++] = terminalStatisRecord.dcMinTime[2];
  	  }
  	  else
  	  {
        for(i = 0; i < 14; i++)
        {
       	  buff[frameTail++] = 0xee;
        }
      }
  	}
  	else
  	{
    	bakTime = timeBcdToHex(tmpTime);
    	tmpCount = monthDays(bakTime.year+2000,bakTime.month);
    	tmpMonthUp   = 0;
    	tmpMonthDown = 0;
    	tmpMonthMax  = 0xeeee;
    	tmpMonthMin  = 0xeeee;
    	for(i=1; i<=tmpCount; i++)
    	{
    		tmpTime = bakTime;
    		tmpTime.day = i;
    	 	tmpTime = timeHexToBcd(tmpTime);
        if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
        {
          tmpMonthUp   += terminalStatisRecord.dcOverUp;
          tmpMonthDown += terminalStatisRecord.dcOverDown;
          
          //�����ֵ������ʱ��
          tmpData = format2ToHex(terminalStatisRecord.dcMax[0] | terminalStatisRecord.dcMax[1]<<8);
          if (tmpMonthMax==0xeeee)
          {
          	tmpMonthMax = tmpData;
          	tmpMaxTime[0] = terminalStatisRecord.dcMaxTime[0]; 
          	tmpMaxTime[1] = terminalStatisRecord.dcMaxTime[1]; 
          	tmpMaxTime[2] = terminalStatisRecord.dcMaxTime[2]; 
          }
          else
          {
          	if (tmpData>tmpMonthMax)
          	{
          	  tmpMonthMax = tmpData;
          	  tmpMaxTime[0] = terminalStatisRecord.dcMaxTime[0]; 
          	  tmpMaxTime[1] = terminalStatisRecord.dcMaxTime[1]; 
          	  tmpMaxTime[2] = terminalStatisRecord.dcMaxTime[2]; 
          	}
          }
          
          //����Сֵ������ʱ��
          tmpData = format2ToHex(terminalStatisRecord.dcMin[0] | terminalStatisRecord.dcMin[1]<<8);
          if (tmpMonthMin==0xeeee)
          {
          	tmpMonthMin = tmpData;
          	tmpMinTime[0] = terminalStatisRecord.dcMinTime[0]; 
          	tmpMinTime[1] = terminalStatisRecord.dcMinTime[1]; 
          	tmpMinTime[2] = terminalStatisRecord.dcMinTime[2]; 
          }
          else
          {
          	if (tmpData<tmpMonthMin)
          	{
          	  tmpMonthMin = tmpData;
          	  tmpMinTime[0] = terminalStatisRecord.dcMinTime[0]; 
          	  tmpMinTime[1] = terminalStatisRecord.dcMinTime[1]; 
          	  tmpMinTime[2] = terminalStatisRecord.dcMinTime[2]; 
          	}
          }
        }
    	}
      
  		//ֱ��ģ����Խ�������ۼ�ʱ��
  		buff[frameTail++] = tmpMonthUp & 0xFF;
  		buff[frameTail++] = tmpMonthUp >> 8;
  			
  		//ֱ��ģ����Խ�������ۼ�ʱ��
  		buff[frameTail++] = tmpMonthDown & 0xFF;
  		buff[frameTail++] = tmpMonthDown >> 8;
  		
  		//ֱ��ģ���������ֵ������ʱ��
      if (tmpMonthMax==0xeeee)
      {
      	buff[frameTail++] = 0xee;
      	buff[frameTail++] = 0xee;
      	buff[frameTail++] = 0xee;
      	buff[frameTail++] = 0xee;
      	buff[frameTail++] = 0xee;
      }
      else
      {
        buff[frameTail++] = hexToBcd(tmpMonthMax/100);
        buff[frameTail++] = (hexToBcd(tmpMonthMax/100)>>8&0xf) | 0xa0;
      	buff[frameTail++] = tmpMaxTime[0];
      	buff[frameTail++] = tmpMaxTime[1];
      	buff[frameTail++] = tmpMaxTime[2];
      }
      
  		//ֱ��ģ��������Сֵ������ʱ��
      if (tmpMonthMin==0xeeee)
      {
      	buff[frameTail++] = 0xee;
      	buff[frameTail++] = 0xee;
      	buff[frameTail++] = 0xee;
      	buff[frameTail++] = 0xee;
      	buff[frameTail++] = 0xee;
      }
      else
      {
        buff[frameTail++] = hexToBcd(tmpMonthMin/100);
        buff[frameTail++] = (hexToBcd(tmpMonthMin/100)>>8&0xf) | 0xa0;
      	buff[frameTail++] = tmpMinTime[0];
      	buff[frameTail++] = tmpMinTime[1];
      	buff[frameTail++] = tmpMinTime[2];
      }
  	}
   #else
    *offset0d = 3;
   #endif

	 return frameTail;
}

/*************************************************************
��������:AFN0D130
��������:��Ӧ��վ�����������"�¶���ֱ��ģ����Խ�����ۼ�ʱ�䡢���/��Сֵ������ʱ��"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D130(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	 #ifdef PLUG_IN_CARRIER_MODULE
	  return AFN0D129(buff, frameTail, pHandle, fn, offset0d);
	 #else
    *offset0d = 2;
	   return frameTail;
   #endif
}

/*************************************************************
��������:AFN0D138
��������:��Ӧ��վ�����������"ֱ��ģ������������"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D138(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{	 
	#ifdef PLUG_IN_CARRIER_MODULE
  	INT8U     dataBuff[5];
  	INT16U    pn, tmpPn = 0;
  	INT8U     da1, da2;
  	INT8U     density, interval, tmpMinute;
  	INT8U     i, dataNum;
  	
  	INT16U    tmpFrameTail;
  	
  	DATE_TIME tmpTime,readTime;
  	
    *offset0d = 7; 
    tmpFrameTail = frameTail;
    da1 = *pHandle++;
    da2 = *pHandle++;
    pHandle += 2;
    
    if(da1 == 0x0)
    {
    	return frameTail;
    }
    
    while(tmpPn < 9)
    {
    	tmpPn++;
    	if((da1 & 0x1) == 0x1)
    	{
    		pn = tmpPn + (da2 - 1) * 8;
    		
    		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));	 //DA1
    		buff[frameTail++] = (pn - 1) / 8 + 1;											 //DA2
    		buff[frameTail++] = 0x02;															     //DT1
    		buff[frameTail++] = 0x11;																	 //DT2
    		
    		//����������ʱ��Td_c
    		tmpTime.second = 0;
    		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
    		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
    		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
    		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
    		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
    		
    		//�����ܶ�
    		buff[frameTail++] = density = *(pHandle+5);
    		switch(density)
    		{
    			case 1:
    				interval = 15;
    				break;
    			case 2:
    				interval = 30;
    				break;
    			case 3:
    				interval = 60;
    				break;
    			case 254:
    				interval = 5;
    				break;
    			case 255:
    				interval = 1;
    				break;
    			default:	//��������������
    				interval = 61;
    		}
    		
    		if(interval > 60)
    		{
    			return tmpFrameTail;
    		}
    		
    		//���ݵ���
    		buff[frameTail++] = dataNum = *(pHandle+6);
    		
  			//ʱ��ת��
  			tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
  			
  			for(i = 0; i < dataNum; i++)
  			{
  				//tmpMinute = hexToBcd(tmpTime.minute+interval);
  				tmpMinute = hexToBcd(interval);
  				
  				readTime  = timeHexToBcd(tmpTime);
  				if(readMeterData(dataBuff, pn, DC_ANALOG, DC_ANALOG_CURVE_DATA, &readTime, tmpMinute) == TRUE)
  				{
  					buff[frameTail++] = dataBuff[0];
  					buff[frameTail++] = dataBuff[1];
  				}
  				else
  				{
  					buff[frameTail++] = 0xEE;
  					buff[frameTail++] = 0xEE;
  				}
  				
  				tmpTime = nextTime(tmpTime, interval, 0);
  
  				usleep(30000);
  			}
    	}
    	da1 >>= 1;
    }
  #else
   *offset0d = 7; 
  #endif

	 return frameTail;
}

/*************************************************************
��������:AFN0D145
��������:��Ӧ��վ�����������"������һ�����޹��ܵ���ʾֵ����"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D145(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     density, interval, tmpMinute;
	INT8U     i, dataNum;
	
	INT16U    tmpFrameTail;
	INT16U    offset;
	
	DATE_TIME tmpTime,readTime;
	
  *offset0d = 7; 
  tmpFrameTail = frameTail;
  da1 = *pHandle++;
  da2 = *pHandle++;
  pHandle += 2;
  
  if(da1 == 0x0)
  {
  	return frameTail;
  }
  
  while(tmpPn < 9)
  {
  	tmpPn++;
  	if((da1 & 0x1) == 0x1)
  	{
  		pn = tmpPn + (da2 - 1) * 8;
  		
  		buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
  		buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
  		switch(fn)
  		{
  			case 145:		//������һ�����޹��ܵ���ʾֵ����
  				buff[frameTail++] = 0x01;															//DT1
  				offset = QUA1_NO_WORK_OFFSET;
  				break;
  				
  			case 146:		//�������������޹��ܵ���ʾֵ����
  				buff[frameTail++] = 0x02;															//DT1
  				offset = QUA4_NO_WORK_OFFSET;
  				break;
  				
  			case 147:		//������������޹�����ʾֵ����
  				buff[frameTail++] = 0x04;															//DT1
  				offset = QUA2_NO_WORK_OFFSET;
  				break;
  				
  			case 148:		//�������������޹�����ʾֵ����
  				buff[frameTail++] = 0x08;															//DT1
  				offset = QUA3_NO_WORK_OFFSET;
  				break;
  		}
  		buff[frameTail++] = 0x12;																	//DT2
  		
  		//����������ʱ��Td_c
  		tmpTime.second = 0;
  		buff[frameTail++] = tmpTime.minute = *(pHandle+0);
  		buff[frameTail++] = tmpTime.hour   = *(pHandle+1);
  		buff[frameTail++] = tmpTime.day    = *(pHandle+2);
  		buff[frameTail++] = tmpTime.month  = *(pHandle+3);
  		buff[frameTail++] = tmpTime.year   = *(pHandle+4);
  		
  		//�����ܶ�
  		buff[frameTail++] = density = *(pHandle+5);
  		switch(density)
  		{
  			case 1:
  				interval = 15;
  				break;
  			case 2:
  				interval = 30;
  				break;
  			case 3:
  				interval = 60;
  				break;
  			case 254:
  				interval = 5;
  				break;
  			case 255:
  				interval = 1;
  				break;
  			default:	//��������������
  				interval = 61;
  		}
  		
  		if(interval > 60)
  		{
  			return tmpFrameTail;
  		}
  		
  		//���ݵ���
  		buff[frameTail++] = dataNum = *(pHandle+6);
  		
			//ʱ��ת��
			tmpTime = backTime(timeBcdToHex(tmpTime), 0, 0, 0, interval, 0);
			
			for(i = 0; i < dataNum; i++)
			{
				//tmpMinute = hexToBcd(tmpTime.minute+interval);
				tmpMinute = hexToBcd(interval);
				
				readTime  = timeHexToBcd(tmpTime);
				if(readMeterData(dataBuff, pn, CURVE_DATA_PRESENT, ENERGY_DATA, &readTime, tmpMinute) == TRUE)
				{
					buff[frameTail++] = dataBuff[offset+0];
					buff[frameTail++] = dataBuff[offset+1];
					buff[frameTail++] = dataBuff[offset+2];
					buff[frameTail++] = dataBuff[offset+3];
				}
				else
				{
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
					buff[frameTail++] = 0xEE;
				}
				
				tmpTime = nextTime(tmpTime, interval, 0);

				usleep(30000);
			}
  	}
  	da1 >>= 1;
  }
	
	return frameTail;
}

/*************************************************************
��������:AFN0D146
��������:��Ӧ��վ�����������"�������������޹��ܵ���ʾֵ����"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D146(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D145(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D147
��������:��Ӧ��վ�����������"������������޹��ܵ���ʾֵ����"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D147(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D145(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D148
��������:��Ӧ��վ�����������"�������������޹��ܵ���ʾֵ����"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D148(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D145(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D153
��������:��Ӧ��վ�����������"�ն����������������й�����ʾֵ"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D153(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U    pn, tmpPn = 0;
	INT8U     da1, da2;
	INT8U     queryType, dataType;
	INT8U     i, tmpDay;
	
	DATE_TIME tmpTime,readTime;
	
	INT16U    tmpFrameTail;
	INT16U    offset;
	
	tmpFrameTail = frameTail;
	da1 = *pHandle++;
	da2 = *pHandle++;
	pHandle += 2;
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x1) == 0x1)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour = 0x23;
			if(fn == 153 || fn == 155)
			{
				*offset0d = 3;
				tmpTime.day   = *(pHandle+0);
				tmpTime.month = *(pHandle+1);
				tmpTime.year  = *(pHandle+2);
			}
			else	//f157 f159
			{
				*offset0d = 2;
				tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + (*(pHandle + 1) & 0xF)
											, (*pHandle >> 4) * 10 + (*pHandle & 0xF));
				tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
				tmpTime.month = *(pHandle+0);
				tmpTime.year  = *(pHandle+1);
			}
			
			buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
			buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
			switch(fn)
			{
				case 153:		//�ն����������������й�����ʾֵ
					buff[frameTail++] = 0x01;															//DT1
					queryType = DAY_BALANCE;
					dataType  = DAY_FREEZE_COPY_DATA;
					break;
					
				case 155:		//�ն����������෴���й�����ʾֵ
					buff[frameTail++] = 0x04;															//DT1
					queryType = DAY_BALANCE;
					dataType  = DAY_FREEZE_COPY_DATA;
					break;
					
				case 157:		//�¶����������������й�����ʾֵ
					buff[frameTail++] = 0x10;															//DT1
					queryType = MONTH_BALANCE;
					dataType  = MONTH_FREEZE_COPY_DATA;
					break;
					
				case 159:		//�¶����������෴���й�����ʾֵ
					buff[frameTail++] = 0x40;															//DT1
					queryType = MONTH_BALANCE;
					dataType  = MONTH_FREEZE_COPY_DATA;
					break;
			}
			buff[frameTail++] = 0x13;																	//DT2
			
			//����������ʱ��
			if(fn == 153 || fn == 155)
			{
				buff[frameTail++] = tmpTime.day;
			}
			buff[frameTail++] = tmpTime.month;
			buff[frameTail++] = tmpTime.year;
			
			//�ն˳���ʱ��(��ʱ������)
			buff[frameTail++] = tmpTime.minute;
			buff[frameTail++] = tmpTime.hour;
			buff[frameTail++] = tmpTime.day;
			buff[frameTail++] = tmpTime.month;
			buff[frameTail++] = tmpTime.year;
			
			if(readMeterData(dataBuff, pn, queryType, dataType, &tmpTime, 0) == TRUE)
			{
				//���ݸ�ʽ14
				//A��,B��,C�������й�����ʾֵ
				if(fn == 153 || fn == 157)
				{
					offset = POSITIVE_WORK_A_OFFSET;
				}
				else	//f155 f159
				{
					offset = NEGTIVE_WORK_A_OFFSET;
				}
				for(i = 0; i < 3; i++)
				{
					if(dataBuff[offset] != 0xEE)
					{
						buff[frameTail++] = 0x0;
					}
					else
					{
						buff[frameTail++] = 0xEE;
					}
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					offset += 12;
				}
			}
			else
			{
				#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail = tmpFrameTail;
			  #else
				  for(i = 0; i < 15; i++)
			    {
			   	  buff[frameTail++] = 0xEE;
			    }
			  #endif
			}
		}
		da1 >>= 1;
		
		usleep(30000);
	}
	
	return frameTail;
}

/*************************************************************
��������:AFN0D154
��������:��Ӧ��վ�����������"�ն����������������޹�����ʾֵ"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D154(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U dataBuff[LENGTH_OF_ENERGY_RECORD];
	INT16U pn, tmpPn = 0;
	INT8U da1, da2;
	INT8U queryType;
	INT8U i, tmpDay;
	
	DATE_TIME tmpTime;
	
	INT16U tmpFrameTail;
	INT16U offset;
	
	tmpFrameTail = frameTail;
	da1 = *pHandle++;
	da2 = *pHandle++;
	pHandle += 2;
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x1) == 0x1)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour = 0x23;
			if(fn == 154 || fn == 156)
			{
				*offset0d = 3;
				tmpTime.day = *pHandle++;
				tmpTime.month = *pHandle++;
				tmpTime.year = *pHandle++;
			}
			else	//f158
			{
				*offset0d = 2;
				tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + (*(pHandle + 1) & 0xF)
											, (*pHandle >> 4) * 10 + (*pHandle & 0xF));
				tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
				tmpTime.month = *pHandle++;
				tmpTime.year = *pHandle++;
			}
			
			buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
			buff[frameTail++] = (pn - 1) / 8 + 1;											//DA2
			switch(fn)
			{
				case 154:		//�ն����������������޹�����ʾֵ
					buff[frameTail++] = 0x02;															//DT1
					queryType = DAY_BALANCE;
					break;
				case 156:		//�ն����������෴���޹�����ʾֵ
					buff[frameTail++] = 0x08;															//DT1
					queryType = DAY_BALANCE;
					break;
				case 158:		//�¶����������������޹�����ʾֵ
					buff[frameTail++] = 0x20;															//DT1
					queryType = MONTH_BALANCE;
					break;
				case 160:		//�¶����������෴���޹�����ʾֵ
					buff[frameTail++] = 0x80;															//DT1
					queryType = MONTH_BALANCE;
					break;
			}
			buff[frameTail++] = 0x13;																	//DT2
			
			//����������ʱ��
			if(fn == 154 || fn == 156)
			{
				buff[frameTail++] = tmpTime.day;
			}
			buff[frameTail++] = tmpTime.month;
			buff[frameTail++] = tmpTime.year;
			
			//�ն˳���ʱ��(��ʱ������)
			buff[frameTail++] = tmpTime.minute;
			buff[frameTail++] = tmpTime.hour;
			buff[frameTail++] = tmpTime.day;
			buff[frameTail++] = tmpTime.month;
			buff[frameTail++] = tmpTime.year;
			
			if(readMeterData(dataBuff, pn, queryType, ENERGY_DATA, &tmpTime, 0) == TRUE)
			{
				//���ݸ�ʽ14
				//A��,B��,C�������й�����ʾֵ
				if(fn == 154 || fn == 158)
				{
					offset = COMB1_NO_WORK_A_OFFSET;
				}
				else
				{
					offset = COMB2_NO_WORK_A_OFFSET;
				}
				for(i = 0; i < 3; i++)
				{
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					buff[frameTail++] = dataBuff[offset++];
					offset += 12;
				}
			}
			else
			{
				#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail = tmpFrameTail;
			  #else
				  for(i = 0; i < 12; i++)
			    {
			   	  buff[frameTail++] = 0xEE;
			    }
			  #endif
			}
		}
		da1 >>= 1;
		
		usleep(30000);
	}
	
	return frameTail;
}

/*************************************************************
��������:AFN0D155
��������:��Ӧ��վ�����������"�¶����������෴���й�����ʾֵ"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D155(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D153(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D156
��������:��Ӧ��վ�����������"�¶����������෴���޹�����ʾֵ"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D156(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D154(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D157
��������:��Ӧ��վ�����������"�¶����������������й�����ʾֵ"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D157(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D153(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D158
��������:��Ӧ��վ�����������"�¶����������������޹�����ʾֵ"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D158(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D154(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D159
��������:��Ӧ��վ�����������"�¶����������෴���й�����ʾֵ"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D159(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D153(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D160
��������:��Ӧ��վ�����������"�¶����������෴���޹�����ʾֵ"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D160(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D154(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D161
��������:��Ӧ��վ�����������"�ն��������й�����ʾֵ(�ܡ�����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
�޸���ʷ:
  1.2012-05-23,�޸�,���û���ն�������,�������й�,�����й���ȡ��������һ��ʵʱ������Ϊ�ն�������,
                 ��������δ���������.
  2.2012-05-24,�޸�,��Щ�����(�绪�������)�صĲ�������Ϊ0xff,�����Բ�����ȷ������,���Ե�0xEE����
*************************************************************/
INT16U AFN0D161(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	METER_DEVICE_CONFIG meterConfig;
	INT8U               dataBuff[512];
	INT8U               da1, da2;
	INT16U              pn, tmpPn = 0;
	INT8U               i, tariff;
	INT8U               queryType, dataType, tmpDay;
	DATE_TIME           tmpTime, readTime;
	INT16U              offset, tmpFrameTail;
	INT8U               meterInfo[10];
  BOOL                bufHasData;

	tmpFrameTail = frameTail;
	da1 = *pHandle++;
	da2 = *pHandle++;
	pHandle += 2;
	
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x1) == 0x1)
		{
			pn = tmpPn + (da2 - 1) * 8;
      
  		queryMeterStoreInfo(pn, meterInfo);
			switch (meterInfo[0])
			{
			  case 1:    //�������ܱ�
			    if (fn<169)
			    {
			      queryType = SINGLE_PHASE_DAY;
			    }
			    else
			    {
			      queryType = SINGLE_PHASE_MONTH;  			    	
			    }
	        dataType  = ENERGY_DATA;
	        break;
	        
	      case 2:    //���౾�طѿر�
	      	if (fn<169)
	      	{
	      	  queryType = SINGLE_LOCAL_CTRL_DAY;
	      	}
	      	else
	      	{
	      	  queryType = SINGLE_LOCAL_CTRL_MONTH;
	      	}
	      	dataType  = ENERGY_DATA;
	      	break;

	      case 3:    //����Զ�̷ѿر�
	      	if (fn<169)
	      	{
	      	  queryType = SINGLE_REMOTE_CTRL_DAY;
	      	}
	      	else
	      	{
	      	  queryType = SINGLE_REMOTE_CTRL_MONTH;
	      	}
	      	dataType  = ENERGY_DATA;
	      	break;
	      	
	      case 4:    //���౾�طѿر�
	      	if (fn<169)
	      	{
	      	  queryType = THREE_DAY;
	      	  dataType  = DAY_FREEZE_COPY_DATA;
	      	}
	      	else
	      	{
	      	  queryType = THREE_MONTH;
	      	  dataType  = MONTH_FREEZE_COPY_DATA;
	      	}
	      	break;

	      case 5:    //���౾�طѿر�
	      	if (fn<169)
	      	{
	      	  queryType = THREE_LOCAL_CTRL_DAY;
	      	  dataType  = DAY_FREEZE_COPY_DATA;
	      	}
	      	else
	      	{
	      	  queryType = THREE_LOCAL_CTRL_MONTH;
	      	  dataType  = MONTH_FREEZE_COPY_DATA;
	      	}
	      	break;

	      case 6:    //����Զ�̷ѿر�
	      	if (fn<169)
	      	{
	      	  queryType = THREE_LOCAL_CTRL_DAY;
	      	  dataType  = DAY_FREEZE_COPY_DATA;
	      	}
	      	else
	      	{
	      	  queryType = THREE_LOCAL_CTRL_MONTH;
	      	  dataType  = MONTH_FREEZE_COPY_DATA;
	      	}
	      	break;

	      case 8:    //�ص��û�
	      	if (fn<169)
	      	{
	      	  queryType = KEY_HOUSEHOLD_DAY;
	      	  dataType  = DAY_FREEZE_COPY_DATA;
	      	}
	      	else
	      	{
	      	  queryType = KEY_HOUSEHOLD_MONTH;
	      	  dataType  = MONTH_FREEZE_COPY_DATA;
	      	}
	      	break;
	      	
	      default:
	      	if (fn<169)
	      	{
			      queryType = DAY_BALANCE;
	          dataType  = DAY_FREEZE_COPY_DATA;
	        }
	        else
	        {
			      queryType = MONTH_BALANCE;
	          dataType  = MONTH_FREEZE_COPY_DATA;
	        }
	        break;
	    }
	    
	    //�����ֻ��Fn161 Fn163 Fn177 Fn179,���Ҫ��ȡ����4��Fn���������Ӧ��������0xEE
	    if (!(fn==161 || fn==163 || fn==177 || fn==179))
	    {
	    	if (meterInfo[0]>0 && meterInfo[0]<4)
	    	{
			    if (fn<169)
			    {
			      queryType = DAY_BALANCE;
	          dataType  = DAY_FREEZE_COPY_DATA;
	        }
	        else
	        {
			      queryType = MONTH_BALANCE;
	          dataType  = MONTH_FREEZE_COPY_DATA;
	        }
	    	}
	    }
			    
			buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
			buff[frameTail++] = (pn - 1) / 8 + 1;												//DA2
			switch(fn)
			{
				case 161:	//�ն��������й�����ʾֵ
					buff[frameTail++] = 0x01;																//DT1
					buff[frameTail++] = 0x14;															  //DT2
					offset = POSITIVE_WORK_OFFSET;
					break;

				case 162:	//�ն��������޹�����ʾֵ
					buff[frameTail++] = 0x02;																//DT1
					buff[frameTail++] = 0x14;																//DT2
					offset = POSITIVE_NO_WORK_OFFSET;
					break;
										
				case 163:	//�ն��ᷴ���й�����ʾֵ
					buff[frameTail++] = 0x04;																//DT1
					buff[frameTail++] = 0x14;																//DT2
					if (meterInfo[0]>0 && meterInfo[0]<4)
					{
					   offset = NEGTIVE_WORK_OFFSET_S;
					}
					else
					{
						 offset = NEGTIVE_WORK_OFFSET;
					}
					break;
				
				case 164:	//�ն��ᷴ���޹�����ʾֵ
					buff[frameTail++] = 0x08;																//DT1
					buff[frameTail++] = 0x14;																//DT2
					offset = NEGTIVE_NO_WORK_OFFSET;
					break;
					
				case 165:	//�ն���һ�����޹�����ʾֵ
					buff[frameTail++] = 0x10;																//DT1
					buff[frameTail++] = 0x14;																//DT2
					offset = QUA1_NO_WORK_OFFSET;
					break;

				case 166:	//�ն���������޹�����ʾֵ
					buff[frameTail++] = 0x20;																//DT1
					buff[frameTail++] = 0x14;																//DT2
					offset = QUA2_NO_WORK_OFFSET;
					break;

				case 167:	//�ն����������޹�����ʾֵ
					buff[frameTail++] = 0x40;																//DT1
					buff[frameTail++] = 0x14;																//DT2
					offset = QUA3_NO_WORK_OFFSET;
					break;

				case 168:	//�ն����������޹�����ʾֵ
					buff[frameTail++] = 0x80;																//DT1
					buff[frameTail++] = 0x14;																//DT2
					offset = QUA4_NO_WORK_OFFSET;
					break;
					
				case 169:	//�����ն��������й�����ʾֵ
					buff[frameTail++] = 0x01;																//DT1
					buff[frameTail++] = 0x15;																//DT2
					queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
					offset = POSITIVE_WORK_OFFSET;
					break;
					
				case 170:	//�����ն��������޹�����ʾֵ
					buff[frameTail++] = 0x02;																//DT1
					buff[frameTail++] = 0x15;																//DT2
					queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
					offset    = POSITIVE_NO_WORK_OFFSET;
					break;

				case 171:	//�����ն��ᷴ���й�����ʾֵ
					buff[frameTail++] = 0x04;																//DT1
					buff[frameTail++] = 0x15;																//DT2
					queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
					offset    = NEGTIVE_WORK_OFFSET;
					break;

				case 172:	//�����ն��ᷴ���޹�����ʾֵ
					buff[frameTail++] = 0x08;																//DT1
					buff[frameTail++] = 0x15;																//DT2
					queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
					offset    = NEGTIVE_NO_WORK_OFFSET;
					break;

				case 173:	//�����ն���һ�����޹�����ʾֵ
					buff[frameTail++] = 0x10;																//DT1
					buff[frameTail++] = 0x15;																//DT2
					queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
					offset    = QUA1_NO_WORK_OFFSET;
					break;

				case 174:	//�����ն���������޹�����ʾֵ
					buff[frameTail++] = 0x20;																//DT1
					buff[frameTail++] = 0x15;																//DT2
					queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
					offset    = QUA2_NO_WORK_OFFSET;
					break;

				case 175:	//�����ն����������޹�����ʾֵ
					buff[frameTail++] = 0x40;																//DT1
					buff[frameTail++] = 0x15;																//DT2
					queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
					offset    = QUA3_NO_WORK_OFFSET;
					break;

				case 176:	//�����ն����������޹�����ʾֵ
					buff[frameTail++] = 0x80;																//DT1
					buff[frameTail++] = 0x15;																//DT2
					queryType = DAY_BALANCE;
			    dataType  = COPY_FREEZE_COPY_DATA;
					offset    = QUA4_NO_WORK_OFFSET;
					break;

				case 177:	//�¶��������й�����ʾֵ
					buff[frameTail++] = 0x01;																//DT1
					buff[frameTail++] = 0x16;																//DT2
					offset    = POSITIVE_WORK_OFFSET;
					break;

				case 178:	//�¶��������޹�����ʾֵ
					buff[frameTail++] = 0x02;																//DT1
					buff[frameTail++] = 0x16;																//DT2
					offset    = POSITIVE_NO_WORK_OFFSET;
					break;
										
				case 179:	//�¶��ᷴ���й�����ʾֵ
					buff[frameTail++] = 0x04;																//DT1
					buff[frameTail++] = 0x16;																//DT2
					if (meterInfo[0]>0 && meterInfo[0]<4)
					{
  					offset = NEGTIVE_WORK_OFFSET_S;
  			  }
  			  else
  			  {
  					offset = NEGTIVE_WORK_OFFSET;
			    }
					break;
					
				case 180:	//�¶��ᷴ���޹�����ʾֵ
					buff[frameTail++] = 0x08;																//DT1
					buff[frameTail++] = 0x16;																//DT2
					offset    = NEGTIVE_NO_WORK_OFFSET;
					break;

				case 181:	//�¶���һ�����޹�����ʾֵ
					buff[frameTail++] = 0x10;																//DT1
					buff[frameTail++] = 0x16;																//DT2
					offset = QUA1_NO_WORK_OFFSET;
					break;

				case 182:	//�¶���������޹�����ʾֵ
					buff[frameTail++] = 0x20;																//DT1
					buff[frameTail++] = 0x16;																//DT2
					offset    = QUA2_NO_WORK_OFFSET;
					break;

				case 183:	//�¶����������޹�����ʾֵ
					buff[frameTail++] = 0x40;																//DT1
					buff[frameTail++] = 0x16;																//DT2
					offset    = QUA3_NO_WORK_OFFSET;
					break;

				case 184:	//�¶����������޹�����ʾֵ
					buff[frameTail++] = 0x80;																//DT1
					buff[frameTail++] = 0x16;																//DT2
					offset    = QUA4_NO_WORK_OFFSET;
					break;
			}
			
			//����������ʱ��
			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour   = 0x23;
			if(fn>=161 && fn<=176)
			{
				*offset0d = 3;
				buff[frameTail++] = tmpTime.day   = *(pHandle+0);
				buff[frameTail++] = tmpTime.month = *(pHandle+1);
				buff[frameTail++] = tmpTime.year  = *(pHandle+2);
			}
			else
			{
				*offset0d = 2;
				tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + (*(pHandle + 1) & 0xF)
											, (*pHandle >> 4) * 10 + (*pHandle & 0xF));
				tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
				buff[frameTail++] = tmpTime.month = *(pHandle+0);
				buff[frameTail++] = tmpTime.year  = *(pHandle+1);
			}
			tariff = numOfTariff(pn);
			
			readTime = tmpTime;
      bufHasData = readMeterData(dataBuff, pn, queryType, dataType, &tmpTime, 0);
      
		  //2012-05-23,add,����ն���ʾֵû�еĻ�,�õ������һ�γɹ��ĳ�����������Ϊ�ն���
		  if (bufHasData==FALSE && (fn==161 || fn==163))
		  {
        tmpTime = readTime;
        if (meterInfo[0]>0 && meterInfo[0]<4)
	      {
	      	bufHasData = readMeterData(dataBuff, pn, meterInfo[1], ENERGY_DATA, &tmpTime, 0);
	      	
	      	if (bufHasData==FALSE)
	      	{
	      		//tmpTime = timeBcdToHex(tmpTime);
	      		
            //2012-12-10,�޸�
	      		tmpTime = readTime;
	      		bufHasData = readBakDayFile(pn, &tmpTime, dataBuff, 1);
	      	}
	      }
	      else
	      {
		      bufHasData = readMeterData(dataBuff, pn, LAST_TODAY, ENERGY_DATA, &tmpTime, 0);
		      
		      if (bufHasData==FALSE)
	      	{
	      		//tmpTime = timeBcdToHex(tmpTime);
            //2012-12-10,�޸�
            tmpTime = readTime;
	      		bufHasData = readBakDayFile(pn, &tmpTime, dataBuff, DAY_FREEZE_COPY_DATA);
	      	}
		    }
		  }
      
			if(bufHasData == TRUE)
			{
			  //�ն˳���ʱ��
			  buff[frameTail++] = tmpTime.minute;
			  buff[frameTail++] = tmpTime.hour;
			  buff[frameTail++] = tmpTime.day;
			  buff[frameTail++] = tmpTime.month;
			  buff[frameTail++] = tmpTime.year;
			
			  //������
			  buff[frameTail++] = tariff;
				
				for(i = 0; i <= tariff; i++)
				{
					if (fn==161 || fn==163 || fn==169 || fn==171 || fn==177 || fn==179)
					{
					  //2012-5-24,��Щ��ص���0xff,���Բ�������������
					  if((dataBuff[offset]!=0xEE) && (dataBuff[offset]!=0xFF))
					  {
						  buff[frameTail++] = 0x00;
					  }
					  else
					  {
						  buff[frameTail++] = 0xEE;
					  }
					}
					
					//2012-5-24,��Щ��ص���0xff,���Բ�������������
					if (dataBuff[offset]==0xff)
					{
					  buff[frameTail++] = 0xee;
					  buff[frameTail++] = 0xee;
					  buff[frameTail++] = 0xee;
					  buff[frameTail++] = 0xee;
					}
					else
					{
					  buff[frameTail++] = dataBuff[offset++];
					  buff[frameTail++] = dataBuff[offset++];
					  buff[frameTail++] = dataBuff[offset++];
					  buff[frameTail++] = dataBuff[offset++];
					}
					
					if (tariff==1)  //�����ʵĴ���
					{
					  //����1����һ��
					  if (fn==161 || fn==163 || fn==169 || fn==171 || fn==177 || fn==179)
					  {
						  buff[frameTail++] = buff[frameTail-5];
						  buff[frameTail++] = buff[frameTail-5];
						  buff[frameTail++] = buff[frameTail-5];
						  buff[frameTail++] = buff[frameTail-5];
						  buff[frameTail++] = buff[frameTail-5];
					  }
					  else
					  {
						  buff[frameTail++] = buff[frameTail-4];
						  buff[frameTail++] = buff[frameTail-4];
						  buff[frameTail++] = buff[frameTail-4];
						  buff[frameTail++] = buff[frameTail-4];
					  }
					  
					  i++;
					}
					else            //�����
					{
						;
					}
				}
			}
			else
			{
				#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail = tmpFrameTail;
			  #else
			    //�ն˳���ʱ��
			    buff[frameTail++] = tmpTime.minute;
			    buff[frameTail++] = tmpTime.hour;
			    buff[frameTail++] = tmpTime.day;
			    buff[frameTail++] = tmpTime.month;
			    buff[frameTail++] = tmpTime.year;
			
			    //������
			    buff[frameTail++] = tariff;
					
					if (fn==161 || fn==163 || fn==169 || fn==171 || fn==177 || fn==179)
					{
				    for(i = 0; i < (tariff + 1) * 5; i++)
			      {
			   	    buff[frameTail++] = 0xEE;
			      }
			    }
			    else
			    {
				    for(i = 0; i < (tariff + 1) * 4; i++)
			      {
			   	    buff[frameTail++] = 0xEE;
			      }
			    }
			  #endif
			}
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*************************************************************
��������:AFN0D162
��������:��Ӧ��վ�����������"�ն��������޹�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D162(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D163
��������:��Ӧ��վ�����������"�ն��ᷴ���й�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D163(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D164
��������:��Ӧ��վ�����������"�ն��ᷴ���޹�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D164(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D165
��������:��Ӧ��վ�����������"�ն���һ�����޹�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D165(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D166
��������:��Ӧ��վ�����������"�ն���������޹�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D166(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D167
��������:��Ӧ��վ�����������"�ն����������޹�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D167(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D168
��������:��Ӧ��վ�����������"�ն����������޹�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D168(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D169
��������:��Ӧ��վ�����������"�����ն��������й�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D169(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D170
��������:��Ӧ��վ�����������"�����ն��������޹�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D170(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D171
��������:��Ӧ��վ�����������"�����ն��ᷴ���й�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D171(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D172
��������:��Ӧ��վ�����������"�����ն��ᷴ���޹�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D172(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D173
��������:��Ӧ��վ�����������"�����ն���һ�����޹�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D173(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D174
��������:��Ӧ��վ�����������"�����ն���������޹�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D174(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D175
��������:��Ӧ��վ�����������"�����ն����������޹�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D175(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D176
��������:��Ӧ��վ�����������"�����ն����������޹�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D176(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D177
��������:��Ӧ��վ�����������"�¶��������й�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D177(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D178
��������:��Ӧ��վ�����������"�ն��������޹�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D178(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D179
��������:��Ӧ��վ�����������"�¶��ᷴ���й�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D179(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D180
��������:��Ӧ��վ�����������"�ն��ᷴ���޹�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D180(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D181
��������:��Ӧ��վ�����������"�¶���һ�����޹�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D181(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D182
��������:��Ӧ��վ�����������"�¶���������޹�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D182(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D183
��������:��Ӧ��վ�����������"�¶����������޹�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D183(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D184
��������:��Ӧ��վ�����������"�¶����������޹�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D184(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D161(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D185
��������:��Ӧ��վ�����������"�ն��������й��������������ʱ��(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D185(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U     dataBuff[LENGTH_OF_REQ_RECORD];
	INT8U     da1, da2;
	INT16U    pn, tmpPn = 0;
	INT8U     i, tariff;
	INT8U     queryType, dataType, tmpDay;	
	DATE_TIME tmpTime;	
	INT16U    offset1, offset2, tmpFrameTail;
	
	tmpFrameTail = frameTail;
	da1 = *pHandle++;
	da2 = *pHandle++;
	pHandle += 2;
	if(da1 == 0x0)
	{
		return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x1) == 0x1)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
			buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
			buff[frameTail++] = (pn - 1) / 8 + 1;												//DA2
			switch(fn)
			{
				case 185:	//�ն��������й��������������ʱ��(��,����1~M)
					buff[frameTail++] = 0x01;																//DT1
					buff[frameTail++] = 0x17;																//DT2
					queryType = DAY_BALANCE;
					dataType  = DAY_FREEZE_COPY_REQ;
					offset1 = REQ_POSITIVE_WORK_OFFSET;
					offset2 = REQ_TIME_P_WORK_OFFSET;
					break;

				case 186:	//�ն��������޹��������������ʱ��(��,����1~M)
					buff[frameTail++] = 0x02;																//DT1
					buff[frameTail++] = 0x17;																//DT2
					queryType = DAY_BALANCE;
					dataType  = DAY_FREEZE_COPY_REQ;
					offset1 = REQ_POSITIVE_NO_WORK_OFFSET;
					offset2 = REQ_TIME_P_NO_WORK_OFFSET;
					break;

				case 187:	//�ն��ᷴ���й��������������ʱ��(��,����1~M)
					buff[frameTail++] = 0x04;																//DT1
					buff[frameTail++] = 0x17;																//DT2
					queryType = DAY_BALANCE;
					dataType  = DAY_FREEZE_COPY_REQ;
					offset1 = REQ_NEGTIVE_WORK_OFFSET;
					offset2 = REQ_TIME_N_WORK_OFFSET;
					break;

				case 188:	//�ն��ᷴ���޹��������������ʱ��(��,����1~M)
					buff[frameTail++] = 0x08;																//DT1
					buff[frameTail++] = 0x17;																//DT2
					queryType = DAY_BALANCE;
					dataType  = DAY_FREEZE_COPY_REQ;
					offset1 = REQ_NEGTIVE_NO_WORK_OFFSET;
					offset2 = REQ_TIME_N_NO_WORK_OFFSET;
					break;
					
				case 189:	//�����ն��������й��������������ʱ��(��,����1~M)
					buff[frameTail++] = 0x10;																//DT1
					buff[frameTail++] = 0x17;																//DT2
					queryType = DAY_BALANCE;
					dataType  = COPY_FREEZE_COPY_REQ;
					offset1 = REQ_POSITIVE_WORK_OFFSET;
					offset2 = REQ_TIME_P_WORK_OFFSET;
					break;

				case 190:	//�����ն��������޹��������������ʱ��(��,����1~M)
					buff[frameTail++] = 0x20;																//DT1
					buff[frameTail++] = 0x17;																//DT2
					queryType = DAY_BALANCE;
					dataType  = COPY_FREEZE_COPY_REQ;
					offset1 = REQ_POSITIVE_NO_WORK_OFFSET;
					offset2 = REQ_TIME_P_NO_WORK_OFFSET;
					break;

				case 191:	//�����ն��ᷴ���й��������������ʱ��(��,����1~M)
					buff[frameTail++] = 0x40;																//DT1
					buff[frameTail++] = 0x17;																//DT2
					queryType = DAY_BALANCE;
					dataType  = COPY_FREEZE_COPY_REQ;
					offset1 = REQ_NEGTIVE_WORK_OFFSET;
					offset2 = REQ_TIME_N_WORK_OFFSET;
					break;

				case 192:	//�����ն��ᷴ���޹��������������ʱ��(��,����1~M)
					buff[frameTail++] = 0x80;																//DT1
					buff[frameTail++] = 0x17;																//DT2
					queryType = DAY_BALANCE;
					dataType  = COPY_FREEZE_COPY_REQ;
					offset1 = REQ_NEGTIVE_NO_WORK_OFFSET;
					offset2 = REQ_TIME_N_NO_WORK_OFFSET;
					break;

				case 193:	//�¶��������й��������������ʱ��(��,����1~M)
					buff[frameTail++] = 0x01;																//DT1
					buff[frameTail++] = 0x18;																//DT2
					queryType = MONTH_BALANCE;
					dataType  = MONTH_FREEZE_COPY_REQ;
					offset1 = REQ_POSITIVE_WORK_OFFSET;
					offset2 = REQ_TIME_P_WORK_OFFSET;
					break;

				case 194:	//�¶��������޹��������������ʱ��(��,����1~M)
					buff[frameTail++] = 0x02;																//DT1
					buff[frameTail++] = 0x18;																//DT2
					queryType = MONTH_BALANCE;
					dataType  = MONTH_FREEZE_COPY_REQ;
					offset1 = REQ_POSITIVE_NO_WORK_OFFSET;
					offset2 = REQ_TIME_P_NO_WORK_OFFSET;
					break;

				case 195:	//�¶��ᷴ���й��������������ʱ��(��,����1~M)
					buff[frameTail++] = 0x04;																//DT1
					buff[frameTail++] = 0x18;																//DT2
					queryType = MONTH_BALANCE;
					dataType  = MONTH_FREEZE_COPY_REQ;
					offset1 = REQ_NEGTIVE_WORK_OFFSET;
					offset2 = REQ_TIME_N_WORK_OFFSET;
					break;
				case 196:	//�¶��ᷴ���޹��������������ʱ��(��,����1~M)
					buff[frameTail++] = 0x08;																//DT1
					buff[frameTail++] = 0x18;																//DT2
					queryType = MONTH_BALANCE;
					dataType  = MONTH_FREEZE_COPY_REQ;
					offset1 = REQ_NEGTIVE_NO_WORK_OFFSET;
					offset2 = REQ_TIME_N_NO_WORK_OFFSET;
					break;
			}
			
			//����������ʱ��
			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour   = 0x23;
			
			if(fn >= 185 && fn <= 192)
			{
				*offset0d = 3;
				buff[frameTail++] = tmpTime.day   = *(pHandle+0);
				buff[frameTail++] = tmpTime.month = *(pHandle+1);
				buff[frameTail++] = tmpTime.year  = *(pHandle+2);
			}
			else		//f193~f196
			{
				*offset0d = 2;
				tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + (*(pHandle + 1) & 0xF)
											, (*pHandle >> 4) * 10 + (*pHandle & 0xF));
				tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
				buff[frameTail++] = tmpTime.month = *(pHandle+0);
				buff[frameTail++] = tmpTime.year  = *(pHandle+1);
			}
			
			tariff = numOfTariff(pn);
			
			if(readMeterData(dataBuff, pn, queryType, dataType, &tmpTime, 0) == TRUE)
			{
			  //�ն˳���ʱ��
			  buff[frameTail++] = tmpTime.minute;
			  buff[frameTail++] = tmpTime.hour;
			  buff[frameTail++] = tmpTime.day;
			  buff[frameTail++] = tmpTime.month;
			  buff[frameTail++] = tmpTime.year;
			
			  //������
			  buff[frameTail++] = tariff;

				for(i = 0; i <= tariff; i++)
				{
					//�������
					buff[frameTail++] = dataBuff[offset1++];
					buff[frameTail++] = dataBuff[offset1++];
					buff[frameTail++] = dataBuff[offset1++];
					
					//�����������ʱ��
					buff[frameTail++] = dataBuff[offset2++];
					buff[frameTail++] = dataBuff[offset2++];
					buff[frameTail++] = dataBuff[offset2++];
					buff[frameTail++] = dataBuff[offset2++];
					if (tariff==1)   //�����ʵĴ���
					{
						 //����1����һ��
						 //�������
						 buff[frameTail++] = buff[frameTail-7];
						 buff[frameTail++] = buff[frameTail-7];
						 buff[frameTail++] = buff[frameTail-7];
						 
						 //�����������ʱ��
						 buff[frameTail++] = buff[frameTail-7];
						 buff[frameTail++] = buff[frameTail-7];
						 buff[frameTail++] = buff[frameTail-7];
						 buff[frameTail++] = buff[frameTail-7];
						 
						 i++;
					}
					else
					{
					  offset2++;
					}
			  }
			}
			else
			{
				#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail = tmpFrameTail;
			  #else
			    //�ն˳���ʱ��
			    buff[frameTail++] = tmpTime.minute;
			    buff[frameTail++] = tmpTime.hour;
			    buff[frameTail++] = tmpTime.day;
			    buff[frameTail++] = tmpTime.month;
			    buff[frameTail++] = tmpTime.year;
			
			    //������
			    buff[frameTail++] = tariff;

				  for(i = 0; i < (tariff + 1) * 7; i++)
			    {
			   	  buff[frameTail++] = 0xEE;
			    }
			  #endif
			}
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*************************************************************
��������:AFN0D186
��������:��Ӧ��վ�����������"�ն��������޹��������������ʱ��(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D186(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D187
��������:��Ӧ��վ�����������"�ն��ᷴ���й��������������ʱ��(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D187(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D188
��������:��Ӧ��վ�����������"�ն��ᷴ���޹��������������ʱ��(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D188(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D189
��������:��Ӧ��վ�����������"�����ն��������й��������������ʱ��(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D189(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D190
��������:��Ӧ��վ�����������"�����ն��������޹��������������ʱ��(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D190(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D191
��������:��Ӧ��վ�����������"�����ն��ᷴ���й��������������ʱ��(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D191(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D192
��������:��Ӧ��վ�����������"�����ն��ᷴ���޹��������������ʱ��(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D192(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D193
��������:��Ӧ��վ�����������"�¶��������й��������������ʱ��(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D193(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D194
��������:��Ӧ��վ�����������"�¶��������޹��������������ʱ��(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D194(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D195
��������:��Ӧ��վ�����������"�¶��ᷴ���й��������������ʱ��(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D195(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D196
��������:��Ӧ��վ�����������"�¶��ᷴ���޹��������������ʱ��(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D196(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D185(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D201
��������:��Ӧ��վ�����������"��һʱ�����������й�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D201(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
��������:AFN0D202
��������:��Ӧ��վ�����������"�ڶ�ʱ�����������й�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D202(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
��������:AFN0D203
��������:��Ӧ��վ�����������"����ʱ�����������й�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D203(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
��������:AFN0D204
��������:��Ӧ��վ�����������"����ʱ�����������й�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D204(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
��������:AFN0D205
��������:��Ӧ��վ�����������"����ʱ�����������й�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D205(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
��������:AFN0D206
��������:��Ӧ��վ�����������"����ʱ�����������й�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D206(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
��������:AFN0D207
��������:��Ӧ��վ�����������"����ʱ�����������й�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D207(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
��������:AFN0D208
��������:��Ӧ��վ�����������"�ڰ�ʱ�����������й�����ʾֵ(��,����1~M)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D208(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
��������:AFN0D209
��������:��Ӧ��վ�����������"�ն�����ܱ�Զ�̿���ͨ�ϵ�״̬����¼"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D209(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	INT8U               dataBuff[768];
	INT8U               da1, da2;
	INT16U              pn, tmpPn = 0;
	INT8U               i, tariff;
	INT8U               queryType, dataType, tmpDay;
	DATE_TIME           tmpTime;
	INT16U              offset, tmpFrameTail;
	INT8U               meterInfo[10];	
	INT8U               buffHasData;
	
	tmpFrameTail = frameTail;
	da1 = *pHandle++;
	da2 = *pHandle++;
	pHandle += 2;
	
	if(da1 == 0x0)
	{
		 return frameTail;
	}
	
	while(tmpPn < 9)
	{
		tmpPn++;
		if((da1 & 0x1) == 0x1)
		{
			pn = tmpPn + (da2 - 1) * 8;
			
			buff[frameTail++] = 0x1 << (pn % 8 == 0?7:(pn % 8 - 1));		//DA1
			buff[frameTail++] = (pn - 1) / 8 + 1;												//DA2
			switch(fn)
			{
				case 209:
					buff[frameTail++] = 0x01;																//DT1
					buff[frameTail++] = 0x1a;															  //DT2
					offset    = POSITIVE_WORK_OFFSET;
					break;
					
			  case 215:
					buff[frameTail++] = 0x40;																//DT1
					buff[frameTail++] = 0x1a;															  //DT2
			  	break;
			}
			
			//����������ʱ��
			tmpTime.second = 0x59;
			tmpTime.minute = 0x59;
			tmpTime.hour   = 0x23;
			if(fn==209)
			{
				*offset0d = 3;
				buff[frameTail++] = tmpTime.day   = *(pHandle+0);
				buff[frameTail++] = tmpTime.month = *(pHandle+1);
				buff[frameTail++] = tmpTime.year  = *(pHandle+2);
			}
			else
			{
				*offset0d = 2;
				tmpDay = monthDays(2000 + (*(pHandle + 1) >> 4) * 10 + (*(pHandle + 1) & 0xF)
											, (*pHandle >> 4) * 10 + (*pHandle & 0xF));
				tmpTime.day = tmpDay / 10 << 4 | tmpDay % 10;
				buff[frameTail++] = tmpTime.month = *(pHandle+0);
				buff[frameTail++] = tmpTime.year  = *(pHandle+1);
			}
			
			//��ѯ������洢��Ϣ
  		queryMeterStoreInfo(pn, meterInfo);
  		
  		if (fn==209)
  		{
  		  if (meterInfo[0]<4)
  		  {
  		    buffHasData =  readMeterData(dataBuff, pn , meterInfo[2], ENERGY_DATA, &tmpTime, 0);
  		  }
  		  else
  		  {
  		    buffHasData =  readMeterData(dataBuff, pn , meterInfo[2], DAY_FREEZE_COPY_DATA, &tmpTime, 0);
  		  }
  		}
  		else  //FN=215
  		{
  		  if (meterInfo[0]<4)
  		  {
  		    buffHasData =  readMeterData(dataBuff, pn , meterInfo[3], ENERGY_DATA, &tmpTime, 0);
  		  }
  		  else
  		  {
  		    buffHasData =  readMeterData(dataBuff, pn , meterInfo[3], MONTH_FREEZE_COPY_DATA, &tmpTime, 0);
  		  }
  		}

			//�ն˳���ʱ��
			buff[frameTail++] = tmpTime.minute;
			buff[frameTail++] = tmpTime.hour;
			buff[frameTail++] = tmpTime.day;
			buff[frameTail++] = tmpTime.month;
			buff[frameTail++] = tmpTime.year;
			
			if(buffHasData == TRUE)
			{ 
		    switch(fn)
		    {
		    	case 209:
		        switch (meterInfo[0])
		        {
  		        case 1:  //�������ܱ�(û�и�������)
  		        case 4:  //�������ܱ�(û��/δ����������)
  		        	for(i=0;i<11;i++)
  		        	{
  		        		 buff[frameTail++] = 0xee;
  		        	}
  		        	break;
  		        	
  		        case 2:  //���౾�طѿر�
  		        case 3:  //����Զ�̷ѿر�
  		          //���ܱ�ͨ�ϵ�״̬
  		          if (dataBuff[METER_STATUS_WORD_S_3]&0x40)
  		          {
  		            buff[frameTail++] = 0x11;
  		          }
  		          else
  		          {
  		            buff[frameTail++] = 0x0;
  		          }
  		    
  		          //���һ�ε��ܱ�Զ�̿���ͨ��ʱ��
  		          for(i=1;i<6;i++)
  		          {
  		            buff[frameTail+i-1] = dataBuff[LAST_CLOSED_GATE_TIME_S+i];
  		          }
  		          frameTail += 5;
  		    
  		          //���һ�ε��ܱ�Զ�̿��ƶϵ�ʱ��
  		          for(i=1;i<6;i++)
  		          {
  		            buff[frameTail+i-1] = dataBuff[LAST_JUMPED_GATE_TIME_S+i];
  		          }
  		          frameTail += 5;
  		          break;

  		        case 5:  //���౾�طѿر�
  		        case 6:  //����Զ�̷ѿر�
  		          //���ܱ�ͨ�ϵ�״̬
  		          if (dataBuff[METER_STATUS_WORD_T_3]&0x40)
  		          {
  		            buff[frameTail++] = 0x11;
  		          }
  		          else
  		          {
  		            buff[frameTail++] = 0x0;
  		          }
  		    
  		          //���һ�ε��ܱ�Զ�̿���ͨ��ʱ��
  		          for(i=1;i<6;i++)
  		          {
  		            buff[frameTail+i-1] = dataBuff[LAST_CLOSED_GATE_TIME_T+i];
  		          }
  		          frameTail += 5;
  		    
  		          //���һ�ε��ܱ�Զ�̿��ƶϵ�ʱ��
  		          for(i=1;i<6;i++)
  		          {
  		            buff[frameTail+i-1] = dataBuff[LAST_JUMPED_GATE_TIME_T+i];
  		          }
  		          frameTail += 5;
  		          break;
  		          
  		        default:
    		        //���ܱ�ͨ�ϵ�״̬
    		        if (dataBuff[METER_STATUS_WORD_S_3]&0x40)
    		        {
    		          buff[frameTail++] = 0x11;
    		        }
    		        else
    		        {
    		          buff[frameTail++] = 0x0;
    		        }
    		    
    		        //���һ�ε��ܱ�Զ�̿���ͨ��ʱ��
    		        for(i=1;i<6;i++)
    		        {
    		          buff[frameTail+i-1] = dataBuff[LAST_CLOSED_GATE_TIME_S+i];
    		        }
    		        frameTail += 5;
    		    
    		        //���һ�ε��ܱ�Զ�̿��ƶϵ�ʱ��
    		        for(i=1;i<6;i++)
    		        {
    		          buff[frameTail+i-1] = dataBuff[LAST_JUMPED_GATE_TIME_S+i];
    		        }
    		        frameTail += 5;
    		        break;
  		      }
		        break;
		        
		      case 215:
		        switch (meterInfo[0])
		        {
  		        case 1:  //�������ܱ�(û��/δ����������)
  		        case 3:  //����Զ�̷ѿر�(û��/δ����������)
  		        case 4:  //�������ܱ�(û��/δ����������)
  		        case 6:  //����Զ�̷ѿر�(û��/δ����������)
  		        	for(i=1;i<36;i++)
  		        	{
  		        		 buff[frameTail++] = 0xee;
  		        	}
  		        	break;
  		        	
  		        case 2:  //���౾�طѿر�
     		      	//�������
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME_S];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME_S+1];
     		      	
     		      	//ʣ����
     		      	if (dataBuff[CHARGE_REMAIN_MONEY_S]==0xee)
     		      	{
     		      		buff[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		buff[frameTail++] = 0x00;		      		
     		      	}	
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_S];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_S+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_S+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_S+3];
     		      	
     		      	//�ۼƹ�����
     		      	if (dataBuff[CHARGE_TOTAL_MONEY_S]==0xee)
     		      	{
     		      		buff[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		buff[frameTail++] = 0x00;		      		
     		      	}	
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_S];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_S+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_S+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_S+3];
     		      	
     		      	//ʣ�����
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_S];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_S+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_S+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_S+3];
     		      	
     		      	//͸֧����
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_S];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_S+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_S+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_S+3];
     		      	
     		      	//�ۼƹ�����
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_S];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_S+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_S+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_S+3];
     		      	
     		      	//��Ƿ���޵���
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_S];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_S+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_S+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_S+3];
     		      	
     		      	//��������
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_S];
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_S+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_S+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_S+3];
     		      	
     		      	//���ϵ���
     		      	buff[frameTail++] = 0xee;
     		      	buff[frameTail++] = 0xee;
     		      	buff[frameTail++] = 0xee;
     		      	buff[frameTail++] = 0xee;
  		        	break;
  		        	
  		        case 5:  //���౾�طѿر�
     		      	//�������
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME_T];
     		      	msFrame[frameTail++] = dataBuff[CHARGE_TOTAL_TIME_T+1];
     		      	
     		      	//ʣ����
     		      	if (dataBuff[CHARGE_REMAIN_MONEY_T]==0xee)
     		      	{
     		      		buff[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		buff[frameTail++] = 0x00;		      		
     		      	}	
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_T];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_T+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_T+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY_T+3];
     		      	
     		      	//�ۼƹ�����
     		      	if (dataBuff[CHARGE_TOTAL_MONEY_T]==0xee)
     		      	{
     		      		buff[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		buff[frameTail++] = 0x00;		      		
     		      	}	
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_T];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_T+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_T+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY_T+3];
     		      	
     		      	//ʣ�����
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_T];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_T+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_T+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY_T+3];
     		      	
     		      	//͸֧����
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_T];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_T+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_T+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY_T+3];
     		      	
     		      	//�ۼƹ�����
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_T];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_T+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_T+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY_T+3];
     		      	
     		      	//��Ƿ���޵���
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_T];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_T+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_T+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT_T+3];
     		      	
     		      	//��������
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_T];
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_T+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_T+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY_T+3];
     		      	
     		      	//���ϵ���
     		      	buff[frameTail++] = 0xee;
     		      	buff[frameTail++] = 0xee;
     		      	buff[frameTail++] = 0xee;
     		      	buff[frameTail++] = 0xee;
  		        	break;
  		        	
  		        default:
     		      	//�������
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_TIME];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_TIME+1];
     		      	
     		      	//ʣ����
     		      	if (dataBuff[CHARGE_REMAIN_MONEY]==0xee)
     		      	{
     		      		buff[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		buff[frameTail++] = 0x00;		      		
     		      	}	
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_MONEY+3];
     		      	
     		      	//�ۼƹ�����
     		      	if (dataBuff[CHARGE_TOTAL_MONEY]==0xee)
     		      	{
     		      		buff[frameTail++] = 0xee;
     		      	}
     		      	else
     		      	{
     		      		buff[frameTail++] = 0x00;		      		
     		      	}	
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_MONEY+3];
     		      	
     		      	//ʣ�����
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_REMAIN_QUANTITY+3];
     		      	
     		      	//͸֧����
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_QUANTITY+3];
     		      	
     		      	//�ۼƹ�����
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_TOTAL_QUANTITY+3];
     		      	
     		      	//��Ƿ���޵���
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_OVERDRAFT_LIMIT+3];
     		      	
     		      	//��������
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY];
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY+1];
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY+2];
     		      	buff[frameTail++] = dataBuff[CHARGE_ALARM_QUANTITY+3];
     		      	
     		      	//���ϵ���
     		      	buff[frameTail++] = 0xee;
     		      	buff[frameTail++] = 0xee;
     		      	buff[frameTail++] = 0xee;
     		      	buff[frameTail++] = 0xee;
     		      	break;
     		    }
		      	break;
			  }				
			}
			else
			{
				#ifdef NO_DATA_USE_PART_ACK_03
			    frameTail = tmpFrameTail;
			  #else
					switch (fn)
					{
					  case 209:
				      memset(&buff[frameTail],0xee,1);
				      frameTail += 11;
				      break;
				      
				    case 215:
				      memset(&buff[frameTail],0xee,36);
				      frameTail += 36;
				      break;
			    }
			  #endif
			}
		}
		da1 >>= 1;
	}
	
	return frameTail;
}

/*************************************************************
��������:AFN0D213
��������:��Ӧ��վ�����������"���ܱ��ز���������ʱ��"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D213(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
��������:AFN0D214
��������:��Ӧ��վ�����������"���ܱ�����޸Ĵ�����ʱ��"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D214(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
��������:AFN0D215
��������:��Ӧ��վ�����������"���ܱ����õ���Ϣ"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D215(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	return AFN0D209(buff, frameTail, pHandle, fn, offset0d);
}

/*************************************************************
��������:AFN0D216
��������:��Ӧ��վ�����������"���ܱ������Ϣ"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D216(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 2;
	
	return frameTail;
}

/*************************************************************
��������:AFN0D217
��������:��Ӧ��վ�����������"̨�����г����ز����ڵ����������"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D217(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 7;
	
	return frameTail;
}

/*************************************************************
��������:AFN0D218
��������:��Ӧ��վ�����������"̨�����г����ز����ڵ�ɫ��������"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D218(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	*offset0d = 7;
	
	return frameTail;
}

/*************************************************************
��������:AFN0D221
��������:��Ӧ��վ�����������"�ն��ն�����������ͳ������(�����Լ)"
���ú���:
�����ú���:
�������:
�������:
����ֵ��֡β��
*************************************************************/
INT16U AFN0D221(INT8U *buff, INT16U frameTail, INT8U *pHandle,INT8U fn, INT16U *offset0d)
{
	TERMINAL_STATIS_RECORD terminalStatisRecord;	
	INT8U                  i, tmpDay;	
	DATE_TIME              tmpTime;
	
	buff[frameTail++] = *pHandle++;		//DA1
	buff[frameTail++] = *pHandle++;		//DA2
	buff[frameTail++] = *pHandle++;		//DT1
	buff[frameTail++] = *pHandle++;		//DT2
	
	//����������ʱ��Td_d Td_m
	tmpTime.second = 0x59;
	tmpTime.minute = 0x59;
	tmpTime.hour   = 0x23;
	buff[frameTail++] = tmpTime.day   = *pHandle++;
	buff[frameTail++] = tmpTime.month = *pHandle++;
	buff[frameTail++] = tmpTime.year  = *pHandle++;


  if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
  {
		//��С�ź�ǿ��
		buff[frameTail++] = terminalStatisRecord.minSignal;
		
		//��С�ź�ǿ��ʱ��
		buff[frameTail++] = hexToBcd(terminalStatisRecord.minSignalTime[0]);
		buff[frameTail++] = hexToBcd(terminalStatisRecord.minSignalTime[1]);

		//����ź�ǿ��
		buff[frameTail++] = terminalStatisRecord.maxSignal;
		
		//����ź�ǿ��ʱ��
		buff[frameTail++] = hexToBcd(terminalStatisRecord.maxSignalTime[0]);
		buff[frameTail++] = hexToBcd(terminalStatisRecord.maxSignalTime[1]);
  }
  else
  {
	  #ifdef NO_DATA_USE_PART_ACK_03
      frameTail -= 7;
    #else
      for(i = 0; i < 6; i++)
      {
   	    buff[frameTail++] = 0xee;
      }
    #endif
  }

	*offset0d = 3;
  
	return frameTail;	 
}
