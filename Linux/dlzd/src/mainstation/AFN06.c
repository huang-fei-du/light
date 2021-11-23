/***************************************************
Copyright,2011,Huawei WoDian co.,LTD
文件名：AFN06.c
作者：Leiyong
版本：0.9
完成日期：2011年3月
描述：主站AFN06(身份认证及密钥协商)处理文件。
函数列表：
  01,11-03-11,Leiyong created.
***************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "convert.h"
#include "msSetPara.h"
#include "dataBase.h"
#include "teRunPara.h"
#include "copyMeter.h"
#include "ioChannel.h"
#include "hardwareConfig.h"

#include "AFN00.h"
#include "AFN06.h"

#define NUM_OF_MAC_BLOCK   240

//变量
INT16U offset06;            //接收到的帧中的数据单元偏移量(不计数据标识的字节)
INT8U  esamSerial[8];       //ESAM芯片序列号

/**************************************************
函数名称:delay01etu
功能描述:延时0.1个etu
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
void delay01etu(unsigned int times)
{
   unsigned int i;
   
   while(times>0)
   {
     //for(i=0;i<474;i++)
     for(i=0;i<948;i++)
     {
   	   ;
     }
     
     times--;
   }
}

/*******************************************************
函数名称:AFN06
功能描述:主站"身份认证及密钥协商"(AFN06)处理函数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void AFN06(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom)
{
   INT16U    frameTail06;             //分组尾
   INT16U    tmpI,tmpFrameTail, tmpHead06;
   INT8U     frameCounter,checkSum;   //发送计数器 
   //char      say[20],str[8];
   INT16U    i;
   INT8U     fn, ackTail;
   INT8U     ackAll, nAckAll;       //全部确认,否认标志      
   INT8U     maxCycle;              //最大循环次数
   INT8U     tmpDtCount;            //DT移位计数
   INT8U     tmpDt1;                //临时DT1
   INT8U     *pTpv;                 //TpV指针

   INT16U (*AFN06Fun[134])(INT16U tail,INT8U *pHandle,INT8U dataFrom);
   
   //清空数据
   bzero(ackData, 100);

   for(i=0;i<10;i++)
   {
     AFN06Fun[i] = NULL;
   }
   
   //组1
   //AFN06Fun[0] = AFN06001;
   //AFN06Fun[1] = AFN06002;
   //AFN06Fun[2] = AFN06003;
   //AFN06Fun[3] = AFN06004;
   AFN06Fun[4] = AFN06005;
   AFN06Fun[5] = AFN06006;
   AFN06Fun[6] = AFN06007;
   AFN06Fun[7] = AFN06008;
   
   //组2
   AFN06Fun[8] = AFN06009;
   AFN06Fun[9] = AFN06010;
   
   if (fQueue.tailPtr == 0)
   {
     tmpHead06 = 0;
   }
   else
   {
     tmpHead06 = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
   }
   frameTail06 = tmpHead06+14; 
   
   ackAll = 0;
   nAckAll = 0;
   ackTail = 0;
   maxCycle = 0;
   frameCounter = 0;
   while ((frame.loadLen > 0) && (maxCycle<5))
   {
      maxCycle++;
      
      offset06 = 0;
            
      tmpDt1 = *(pDataHead + 2);
      tmpDtCount = 0;
      while(tmpDtCount < 9)
      {
         tmpDtCount++;
         if ((tmpDt1 & 0x1) == 0x1)
         {
            fn = *(pDataHead + 3) * 8 + tmpDtCount;
            
            //printf("AFN06 Fn=%d\n",fn);
            
            if (fn>10)
            {
      	       maxCycle = 5;
      	       break;
      	    }

            if (AFN06Fun[fn-1] != NULL)
            {
               //逐项确认/否认填写
               ackData[ackTail*5]   = *pDataHead;                         //DA1
               ackData[ackTail*5+1] = *(pDataHead+1);                     //DA2
               ackData[ackTail*5+2] = 0x1<<((fn%8 == 0) ? 7 : (fn%8-1));  //DT1
               ackData[ackTail*5+3] = (fn-1)/8;                           //DT2
               
               if ((tmpFrameTail = AFN06Fun[fn-1](frameTail06,pDataHead,dataFrom))==0)
               { 
               	 return;
               }
               else
               {
                 frameTail06 = tmpFrameTail;                 
               }
            }
         }
         
         tmpDt1 >>= 1;
         
         if (((frameTail06 - tmpHead06) > MAX_OF_PER_FRAME) || (((pDataHead+offset06+4) == pDataEnd) && tmpDtCount==8))
         {
             //不允许主动上报且有事件发生
             if (frame.acd==1 && (callAndReport&0x03)== 0x02 && (frameTail06 - tmpHead06) > 16)
             {
            	   msFrame[frameTail06++] = iEventCounter;
            	   msFrame[frameTail06++] = nEventCounter;
             }

             //根据启动站要求判断是否携带TP
             if (frame.pTp != NULL)
             {
                pTpv = frame.pTp;
                msFrame[frameTail06++] = *pTpv++;
                msFrame[frameTail06++] = *pTpv++;
                msFrame[frameTail06++] = *pTpv++;
                msFrame[frameTail06++] = *pTpv++;
                msFrame[frameTail06++] = *pTpv++;
                msFrame[frameTail06++] = *pTpv;
             }
             
             msFrame[tmpHead06 + 0] = 0x68;   //帧起始字符
           
             tmpI = ((frameTail06 - tmpHead06 -6) << 2) | PROTOCOL_FIELD;
             msFrame[tmpHead06 + 1] = tmpI & 0xFF;   //L
             msFrame[tmpHead06 + 2] = tmpI >> 8;
             msFrame[tmpHead06 + 3] = tmpI & 0xFF;   //L
             msFrame[tmpHead06 + 4] = tmpI >> 8; 
           
             msFrame[tmpHead06 + 5] = 0x68;  //帧起始字符
      
      			 //控制域
             if (frame.acd==1 && (callAndReport&0x03)== 0x02)   //不允许主动上报且有事件发生
             {
                msFrame[tmpHead06 + 6] = 0xa8;  //控制字节10001000
             }
             else
             {
             	  msFrame[tmpHead06 + 6] = 0x88;  //控制字节10001000
             }
             	 
             //地址域
             msFrame[tmpHead06 + 7] = addrField.a1[0];
             msFrame[tmpHead06 + 8] = addrField.a1[1];
             msFrame[tmpHead06 + 9] = addrField.a2[0];
             msFrame[tmpHead06 + 10] = addrField.a2[1];
             msFrame[tmpHead06 + 11] = addrField.a3;
     
             msFrame[tmpHead06 + 12] = 0x06;  //AFN
      
             if ((pDataHead+4) == pDataEnd)
             {
                if (frameCounter == 0)
                {
                  msFrame[tmpHead06 + 13] = 0x60 | rSeq;    //01100000 | rSeq 单帧
                }
                else
                {
                  msFrame[tmpHead06 + 13] = 0x20 | rSeq;    //00100000 | rSeq 最后一帧
                }
             }
             else
             {
               if (frameCounter == 0)
               {
                 msFrame[tmpHead06 + 13] = 0x40 | rSeq;     //01000000 | rSeq  第一帧
               }
               else
               {
                 msFrame[tmpHead06 + 13] = 0x00 | rSeq;     //00000000 | rSeq 中间帧
               }
               frameCounter++;
             }

             if (frame.pTp != NULL)
             {
            	  msFrame[tmpHead06+13] |= 0x80;       //TpV置位
             }
                          
             tmpI = tmpHead06 + 6;
             checkSum = 0;
             while (tmpI < frameTail06)
             {
                checkSum = msFrame[tmpI] + checkSum;
                tmpI++;
             }
             
             msFrame[frameTail06++] = checkSum;
             msFrame[frameTail06++] = 0x16;
             
             fQueue.frame[fQueue.tailPtr].head = tmpHead06;
             fQueue.frame[fQueue.tailPtr].len = frameTail06-tmpHead06;
             
             if (((frameTail06 - tmpHead06) > 16 && frame.pTp==NULL)||((frameTail06 - tmpHead06) > 22 && frame.pTp!=NULL))
             {
               tmpHead06 = frameTail06;
               if ((tmpHead06+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
               	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
               {
                  fQueue.frame[fQueue.tailPtr].next = 0x0;
               	  fQueue.tailPtr = 0;
               	  tmpHead06 = 0;
               }
               else
               {                 
                  fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
                  fQueue.tailPtr++;
               }

               frameTail06 = tmpHead06 + 14;  //frameTail重新置为14填写下一帧
             }
         }
      }
      
      if (maxCycle==5)
      {
      	 break;
      }
      
      //printf("offset06=%d\n",offset06);
      if (frame.loadLen < offset06+4)
      {
      	 break;
      }
      else
      {
         frame.loadLen -= (offset06 + 4);
         pDataHead = pDataHead + offset06 + 4;
      }
   }
}

/*******************************************************
函数名称:AFN04005
功能描述:响应主站身份认证及密钥协商"读取终端随机数(F5)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
INT16U AFN06005(INT16U frameTail,INT8U *pHandle,INT8U dataFrom)
{
	INT8U buf[20];
	INT8U i;

  //数据单元标识
  msFrame[frameTail++] = *pHandle++;  //DA1
  msFrame[frameTail++] = *pHandle++;  //DA2
  msFrame[frameTail++] = *pHandle++;  //DT1
  msFrame[frameTail++] = *pHandle;    //DT2
    
  //数据单元

  //终端随机数
  //读取终端随机数
  getChallenge(buf);  
  //倒序上送
  for(i=0;i<8;i++)
  {
    msFrame[frameTail++] = buf[7-i];
  }
  
  //读取ESAM Serial
  getSerial(buf);
  //倒序上送
  for(i=0;i<8;i++)
  {
    msFrame[frameTail++] = buf[7-i];
  }
  
  offset06 = 0;

  return frameTail;
}

/*******************************************************
函数名称:AFN04006
功能描述:响应主站身份认证及密钥协商"公钥验证(F6)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
INT16U AFN06006(INT16U frameTail,INT8U *pHandle,INT8U dataFrom)
{
	INT8U cmdBuf[270];
	INT8U i;
	INT8U result;
	char  say[30];

	pHandle += 4;
  cmdBuf[0] = 0x02;     //第一字节 - 命令类型
  cmdBuf[1] = 145;     //第二字节 - 本条命令长度
  cmdBuf[2] = 0x80;
  cmdBuf[3] = 0x42;
  cmdBuf[4] = 0x00;
  if (*pHandle==2)
  {
  	cmdBuf[5] = 0x09;
  	strcpy(say,"主站公钥验证");
  }
  else
  {
  	cmdBuf[5] = 0x01;
  	strcpy(say,"主控公钥验证");
  }
  cmdBuf[6] = 0x8c;
  
  cmdBuf[7] = 0x81;
  cmdBuf[8] = 0x08;
  pHandle++;
  
  //目前据观察电科院测试软件,是将读出的数倒序发下来的,因此送到ESAM的顺序又倒过来
  //主站随机数
  for(i=0;i<8;i++)
  {
  	cmdBuf[16-i] = *pHandle++;
  	//cmdBuf[9+i] = *pHandle++;
  }
  cmdBuf[17] = 0x60;
  cmdBuf[18] = 0x80;
  
  //目前据观察电科院测试软件,是将读出的数倒序发下来的,因此送到ESAM的顺序又倒过来
  //数字签名
  for(i=0;i<128;i++)
  {
  	cmdBuf[146-i] = *pHandle++;
  	//cmdBuf[19+i] = *pHandle++;
  }

  result = read(fdOfIoChannel, cmdBuf, 256);
    
  if (result==0)
  {
     swAnalyse(say,cmdBuf[0], cmdBuf[1]);
    
     if (cmdBuf[0]==0x90 && cmdBuf[1]==0x00)
     {
    	  ackOrNack(TRUE, dataFrom);
     }
     else
     {
        AFN00004(dataFrom, 1, NULL);
     }
  }
  else
  {
    if (debugInfo&ESAM_DEBUG)
    {
    	  switch(result)
    	  {
    	    case 1:
    	      printf("公钥验证:接收校验错误\n");
    	      break;
    	      
    	    case 2:
    	      printf("公钥验证:接收第一字节不等INS字节\n");
    	      break;
    	  }
    }
    
    AFN00004(dataFrom, 1, NULL);
  }

  offset06 = 137;
  
  return 0;
}

/*******************************************************
函数名称:AFN04007
功能描述:响应主站身份认证及密钥协商"密钥更新请求(F7)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
INT16U AFN06007(INT16U frameTail,INT8U *pHandle,INT8U dataFrom)
{
	INT32U i;
	INT8U  buf[20];

  //数据单元标识
  msFrame[frameTail++] = *pHandle++;  //DA1
  msFrame[frameTail++] = *pHandle++;  //DA2
  msFrame[frameTail++] = *pHandle++;  //DT1
  msFrame[frameTail++] = *pHandle++;  //DT2
    
  //数据单元
  msFrame[frameTail++] = *pHandle;    //DT2
  
  //读取终端随机数
  getChallenge(buf);  
  //倒序上送
  for(i=0;i<8;i++)
  {
    msFrame[frameTail++] = buf[7-i];
  }
  
  //读取ESAM Serial
  getSerial(buf);
  //倒序上送
  for(i=0;i<8;i++)
  {
    msFrame[frameTail++] = buf[7-i];
  }
  
  offset06 = 1;

  return frameTail;
}

/*******************************************************
函数名称:AFN04008
功能描述:响应主站身份认证及密钥协商"密钥更新(F8)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
INT16U AFN06008(INT16U frameTail,INT8U *pHandle,INT8U dataFrom)
{
	 INT8U  i;
	 INT8U  tmpCmd[5], cmdBuf[500];
	 INT8U  result;
	 INT16U lenOfWrite;    //写入内容长度
	 INT8U  upType;        //密钥更新类型
	 INT8U  numOfKey;      //对称密钥更新指令的密钥个数
	 INT16U tmpTail;
	 char   say[50];

	 pHandle += 4;
	 
	 upType = *pHandle;
	 lenOfWrite = *(pHandle+1) | *(pHandle+2)<<8;
	 
	 pHandle+=3;
   
   offset06 = lenOfWrite + 3;

   //第一条命令
   switch (upType)
   {
   	 case 1:    //主控公钥更新
   	 case 2:    //主站公钥本地更新       
       cmdBuf[0] = 0x06;    //第一字节 - 命令类型
       cmdBuf[0] |= 0x80;   //长度高256字节用0x80这一位
       cmdBuf[1] = 286-256; //第二字节 - 本条命令长度
       cmdBuf[2] = 0x90;
       if (upType==1)
       {
         cmdBuf[3] = 0x40;  //主控公钥更新指令INS为0x40
         sprintf(say, "密钥更新(主控公钥)");
       }
       else
       {
         cmdBuf[3] = 0x34;  //主控公钥本地更新指令INS为0x34
         sprintf(say, "密钥更新(主站公钥本地)");
       }
       cmdBuf[4] = 0x00;
       cmdBuf[5] = 0x00;
       cmdBuf[6] = 0x92;
       cmdBuf[7] = 0x62;
       cmdBuf[8] = 0x90;
       
       //主控(/站)公钥144Bytes
       for(i=0;i<144;i++)
       {
    	   cmdBuf[152-i] = *pHandle++;
       }
       
       //终端随机数8Bytes
       pHandle+=8;
       
       //第二条命令
       cmdBuf[153] = 0x80;
     
       if (upType==1)
       {
         cmdBuf[154] = 0x40; //主控公钥更新指令INS为0x40
       }
       else
       {
         cmdBuf[154] = 0x34; //主控公钥本地更新指令INS为0x34
       }
       cmdBuf[155] = 0x00;
       cmdBuf[156] = 0x00;
       cmdBuf[157] = 0x82;
       cmdBuf[158] = 0x60;
       cmdBuf[159] = 0x80;
       
       //数字签名128Bytes
       for(i=0;i<128;i++)
       {
    	   cmdBuf[287-i] = *pHandle++;
       }
       
       result = read(fdOfIoChannel, cmdBuf, 2);
       break;
       
     case 3:  //主站公钥远程更新
       sprintf(say, "密钥更新(主站公钥远程)");
       cmdBuf[0] = 0x06;    //第一字节 - 命令类型
       cmdBuf[0] |= 0x80;   //长度高256字节用0x80这一位
       cmdBuf[1] = 416-256; //第二字节 - 本条命令长度
       cmdBuf[2] = 0x90;
       cmdBuf[3] = 0x3c;    //主站公钥远程更新指令INS为0x3c
       cmdBuf[4] = 0x00;
       cmdBuf[5] = 0x00;
       cmdBuf[6] = 0xFA;    //Lc
       cmdBuf[7] = 0x63;
       cmdBuf[8] = 0x80;
       
       //会话密鉏128Bytes
       for(i=0;i<128;i++)
       {
    	   cmdBuf[136-i] = *pHandle++;
       }
       cmdBuf[137] = 0x62;
       cmdBuf[138] = 0x90;       

       //主站公钥前118Bytes
       for(i=0;i<118;i++)
       {
    	   cmdBuf[139+i] = *(pHandle+143-i);
       }       
       
       //第二条命令
       cmdBuf[257] = 0x80;     
       cmdBuf[258] = 0x3c; //主站公钥远程更新指令INS为0x3c
       cmdBuf[259] = 0x00;
       cmdBuf[260] = 0x00;
       cmdBuf[261] = 0x9c;

       //主站公钥后26Bytes
       for(i=0;i<26;i++)
       {
    	   cmdBuf[262+i] = *(pHandle+25-i);
       }       
       pHandle+=144;
       
       //终端随机数8Bytes
       pHandle+=8;
       
       cmdBuf[288] = 0x60;
       cmdBuf[289] = 0x80;

       //数字签名128Bytes
       for(i=0;i<128;i++)
       {
    	   cmdBuf[417-i] = *pHandle++;
       }
       
       result = read(fdOfIoChannel, cmdBuf, 2);
     	 break;
     	 
     case 8:    //终端对称密钥更新
       sprintf(say, "密钥更新(终端对称密钥)");
     	 numOfKey = (lenOfWrite-128-128-8)/32;
     	 if (numOfKey<1 || numOfKey>4)
     	 {
     	 	 return 0;
     	 }
     	 
       cmdBuf[0] = 0x06;    //第一字节 - 命令类型
       cmdBuf[0] |= 0x80;   //长度高256字节用0x80这一位
       cmdBuf[1] = 308+(numOfKey-1)*32-256; //第二字节 - 本条命令长度
       cmdBuf[2] = 0x90;
       cmdBuf[3] = 0x3a;    //终端对称密钥更新指令INS为0x3a
       cmdBuf[4] = 0x00;
       cmdBuf[5] = 0x00;
       cmdBuf[6] = 0xa5;    //Lc
       cmdBuf[7] = 0x63;
       cmdBuf[8] = 0x80;
       
       //会话密鉏128Bytes
       for(i=0;i<128;i++)
       {
    	   cmdBuf[136-i] = *pHandle++;
       }
       cmdBuf[137] = 0x64;
       cmdBuf[138] = 0x00;
       cmdBuf[139] = numOfKey*32;

       //第一条密钥32Bytes
       for(i=0;i<32;i++)
       {
    	   cmdBuf[140+i] = *(pHandle+numOfKey*32-1-i);
       }
       
       //第二条命令
       cmdBuf[172] = 0x80;     
       cmdBuf[173] = 0x3a; //终端对称密钥更新指令INS为0x3a
       cmdBuf[174] = 0x00;
       cmdBuf[175] = 0x00;
       cmdBuf[176] = (numOfKey-1)*32+0x82;

       //第二到四条密钥(N-1)32Bytes
       for(i=0;i<(numOfKey-1)*32;i++)
       {
    	   cmdBuf[177+i] = *(pHandle+(numOfKey-1)*32-1-i);
       }
       
       pHandle += numOfKey*32;
       
       //终端随机数8Bytes
       pHandle += 8;
       
       tmpTail = 177+(numOfKey-1)*32;
       
       cmdBuf[tmpTail++] = 0x60;
       cmdBuf[tmpTail++] = 0x80;

       //数字签名128Bytes
       for(i=0;i<128;i++)
       {
    	   cmdBuf[tmpTail+127-i] = *pHandle++;
       }
       
       result = read(fdOfIoChannel, cmdBuf, 2);

     	 break;
     
     default:
     	 return 0;
   }
    
   if (result==0)
   {
     swAnalyse(say, cmdBuf[0], cmdBuf[1]);
     
     if (cmdBuf[0]==0x90 && cmdBuf[1]==0x00)
     {
    	  ackOrNack(TRUE, dataFrom);
     }
     else
     {
       if (cmdBuf[0]==0x90 && (cmdBuf[1]==0x86 || cmdBuf[1]==0x84))
       {
         AFN00004(dataFrom, 2, NULL);
       }
       else
       {
         AFN00004(dataFrom, 1, NULL);
       }
     }
   }
   else
   {
     if (debugInfo&ESAM_DEBUG)
     {
    	  switch(result)
    	  {
    	    case 1:
    	      printf("密钥更新:接收校验错误\n");
    	      break;
    	      
    	    case 2:
    	      printf("密钥更新:接收第一字节不等INS字节\n");
    	      break;
    	  }
     }
    
     AFN00004(dataFrom, 1, NULL);
   }
	 
	 return 0;
}

/*******************************************************
函数名称:AFN04009
功能描述:响应主站身份认证及密钥协商"终端非对称密钥注册(F9)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
INT16U AFN06009(INT16U frameTail,INT8U *pHandle,INT8U dataFrom)
{
	 INT8U  cmdBuf[280];
	 INT32U i;
	 INT8U  tmpCmd[5];
	 INT8U  result;
	 INT8U  *pData;
	 INT8U  ackBuf;
   INT8U  sw1,sw2;
   char   say[30];
   
   sw1 = 0;
   sw2 = 0;

	 pData = pHandle;
	 
   offset06 = 145;

	 pHandle += 4;
   
   cmdBuf[0] = 0x03;    //第一字节 - 命令类型
   cmdBuf[1] = 145;     //第二字节 - 本条命令长度
   cmdBuf[2] = 0x80;
   cmdBuf[3] = 0x36;
   if (*pHandle==0x03)   //请求更新密钥类型,3表示终端非对称密钥对1注册,4表示终端非对称密钥对2注册
   {
   	 cmdBuf[4] = 0x01;
   	 strcpy(say, "终端非对称密钥对1注册");
   }
   else
   {
   	 cmdBuf[4] = 0x02;
   	 strcpy(say, "终端非对称密钥对2注册");
   }

	 pHandle++;
	 
	 cmdBuf[5] = 0x00;
	 cmdBuf[6] = 0x8C;

   cmdBuf[7] = 0x81;
	 cmdBuf[8] = 0x08;
   
   //主站随机数
   for(i=0;i<8;i++)
   {
  	 cmdBuf[16-i] = *pHandle++;
  	 //cmdBuf[9+i] = *pHandle++;
   }
   
   pHandle+=8;   //终端随机数
   
   cmdBuf[17] = 0x60;
   cmdBuf[18] = 0x80;

   //数字签名
   for(i=0;i<128;i++)
   {
  	 cmdBuf[146-i] = *pHandle++;
  	 //cmdBuf[19+i] = *pHandle++;
   }
   
   result = read(fdOfIoChannel, cmdBuf, 256);
    
   if (result==0)
   {
     swAnalyse(say, cmdBuf[256], cmdBuf[257]);
    
     if (cmdBuf[256]==0x90 && cmdBuf[257]==0x00)
     {
       //数据单元标识
       msFrame[frameTail++] = *pData++;  //DA1
       msFrame[frameTail++] = *pData++;  //DA2
       msFrame[frameTail++] = *pData++;  //DT1
       msFrame[frameTail++] = *pData++;  //DT2
       
       msFrame[frameTail++] = *pData++;  //请求类型
       
       //终端公钥1或2
       for(i=256;i>0;i--)
       {
    	   msFrame[frameTail++] = cmdBuf[i-1];
    	 }
    	 
    	 return frameTail;
     }
     else
     {
     	 ;
     }
   }
   else
   {
     if (debugInfo&ESAM_DEBUG)
     {
    	  switch(result)
    	  {
    	    case 1:
    	      printf("终端非对称密钥注册:接收校验错误\n");
    	      break;
    	      
    	    case 2:
    	      printf("终端非对称密钥注册:接收第一字节不等INS字节\n");
    	      break;
    	    
    	    case 3:
    	      printf("终端非对称密钥注册:接收未知错误\n");
    	      break;
    	  }
     }
   }
   
	 return 0;
}

/*******************************************************
函数名称:AFN04010
功能描述:响应主站身份认证及密钥协商"终端非对称密钥更新(F10)"
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE or FALSE
*******************************************************/
INT16U AFN06010(INT16U frameTail,INT8U *pHandle,INT8U dataFrom)
{
	 INT16U i;
	 INT8U  cmdBuf[300];
	 INT8U  result;
	 INT8U *pData;
	 char  say[30];
	 
	 pData = pHandle;
	 
   offset06 = 273;

	 pHandle += 4;
	 
   cmdBuf[0] = 0x04;     //第一字节 - 命令类型
   cmdBuf[0] |= 0x80;    //长度比一字节大的将最高位置1代表256
   cmdBuf[1] = 280-256;  //第二字节 - 本条命令长度
   cmdBuf[2] = 0x90;
   cmdBuf[3] = 0x38;
   if (*pHandle==0x05)   //请求更新公钥类型,5表示终端非对称密钥对1更新,6表示终端非对称密钥对2更新
   {
   	 cmdBuf[4] = 0x01;
   	 strcpy(say, "终端非对称密钥对1更新");
   }
   else
   {
   	 cmdBuf[4] = 0x02;
   	 strcpy(say, "终端非对称密钥对2更新");
   }
	 pHandle++;
	 
	 cmdBuf[5] = 0x00;
	 cmdBuf[6] = 0x8C;
	 cmdBuf[7] = 0x63;
	 cmdBuf[8] = 0x80;
   
   //会话密钥
   for(i=0;i<128;i++)
   {
  	 cmdBuf[136-i] = *pHandle++;
  	 //cmdBuf[9+i] = *pHandle++;
   }
   
   cmdBuf[137] = 0x81;
   cmdBuf[138] = 0x08;

   //主站随机数
   for(i=0;i<8;i++)
   {
  	 cmdBuf[146-i] = *pHandle++;
  	 //cmdBuf[139+i] = *pHandle++;
   }
   
   pHandle+=8;   //终端随机数
   
   cmdBuf[147] = 0x80;
   cmdBuf[148] = 0x38;
   cmdBuf[149] = cmdBuf[4];   //P1
   cmdBuf[150] = 0x00;
   cmdBuf[151] = 0x82;
   cmdBuf[152] = 0x60;
   cmdBuf[153] = 0x80;
   
   //数字签名
   for(i=0;i<128;i++)
   {
  	 cmdBuf[281-i] = *pHandle++;
  	 //cmdBuf[154+i] = *pHandle++;
   }
   
   result = read(fdOfIoChannel, cmdBuf, 278);
   
   
   if (result==0)
   {
     swAnalyse(say, cmdBuf[276], cmdBuf[277]);
    
     if (cmdBuf[276]==0x90 && cmdBuf[277]==0x00)
     {
       //数据单元标识
       msFrame[frameTail++] = *pData++;  //DA1
       msFrame[frameTail++] = *pData++;  //DA2
       msFrame[frameTail++] = *pData++;  //DT1
       msFrame[frameTail++] = *pData++;  //DT2
       
       msFrame[frameTail++] = *pData++;  //请求类型
       
       pData += 128;

       //主站随机数8字节
       memcpy(&msFrame[frameTail], pData, 8);
       frameTail += 8;

       //终端公钥1或2(144字节)
       for(i=0; i<144; i++)
       {
    	   msFrame[frameTail+143-i] = cmdBuf[2+i];
    	 }
    	 frameTail += 144;


       //终端数字签名(128字节)
       for(i=0;i<128;i++)
       {
    	   msFrame[frameTail+127-i] = cmdBuf[148+i];
    	 }
    	 
    	 frameTail += 128;
    	 
    	 return frameTail;
     }
   }
   else
   {
     if (debugInfo&ESAM_DEBUG)
     {
    	  switch(result)
    	  {
    	    case 1:
    	      printf("终端非对称密钥更新:接收校验错误\n");
    	      break;
    	      
    	    case 2:
    	      printf("终端非对称密钥更新:接收第一字节不等INS字节\n");
    	      break;
    	      
    	    case 3:
    	      printf("终端非对称密钥更新:接收未知错误\n");
    	      break;
    	  }
     }
   }
   
	 return 0;
}

/*******************************************************
函数名称:getChallenge
功能描述:取随机数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void getChallenge(INT8U *buf)
{
	INT8U cmdBuf[20];
	INT8U i;
	INT8U result;
  
  for(i=0;i<5;i++)
  {
    buf[0] = 0x01;     //第一字节 - 命令类型
    buf[1] = 0x05;     //第二字节 - 本条命令长度
    buf[2] = 0x00;     //CLA
    buf[3] = 0x84;     //INS
    buf[4] = 0x00;     //P01
    buf[5] = 0x00;     //P02
    buf[6] = 0x08;     //P03
    result = read(fdOfIoChannel, buf, 8);
    
    if (result==0)
    {
      if (buf[8]==0x90 && buf[9]==0x00)
      {
    	  if (debugInfo&ESAM_DEBUG)
    	  {
    	    printf("第%d发送返回,终端随机数:%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",i+1,buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7]);
    	  }
    	  break;
    	}
    }
    else
    {
    	if (debugInfo&ESAM_DEBUG)
    	{
    	  switch(result)
    	  {
    	    case 1:
    	      printf("接收校验错误\n");
    	      break;
    	      
    	    case 2:
    	      printf("接收第一字节不等INS字节\n");
    	      break;
    	  }
    	}
    }
  }
}

/*******************************************************
函数名称:getSerial
功能描述:取序列号
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void getSerial(INT8U *buf)
{
	 buf[0] = 88;     //第一字节 - 命令类型
	 
	 read(fdOfIoChannel, buf, 8);
}

/*******************************************************
函数名称:getChallenge
功能描述:取随机数
调用函数:
被调用函数:
输入参数:ifSingle - =0,单地址,  >0,组地址组号
输出参数:
返回值：void
*******************************************************/
INT8U calcMac(INT8U afn, INT8U ifSingle, INT8U *buf, INT16U lenOfBuf, INT8U *retMac)
{
	INT8U  cmdBuf[1024];
	INT8U  i;
	INT8U  result;
	INT8U  retValue = 0;
	INT8U  numOfBlock;
	INT16U frameTail;
	
	//计算需要级联的块数
	numOfBlock = lenOfBuf/NUM_OF_MAC_BLOCK;
	if (lenOfBuf%NUM_OF_MAC_BLOCK)
	{
		numOfBlock++;
	}

  cmdBuf[0] = 0x05;                   //第一字节 - 命令类型
  cmdBuf[1] = lenOfBuf+numOfBlock*5;  //第二字节 - 本条命令长度  
  frameTail = 2;
  for(i=0;i<numOfBlock;i++)
  {
    if ((i+1)==numOfBlock)
    {
      cmdBuf[frameTail++] = 0x80;     //CLA,级联指令的最后一条
    }
    else
    {
      cmdBuf[frameTail++] = 0x90;     //CLA,级联指令
    }
    
    cmdBuf[frameTail++] = 0xe8;       //INS
    
    if (ifSingle==0)   //单地址
    {
      cmdBuf[frameTail++] = 0x00;     //P01
      switch(afn)
      {
      	case SET_PARAMETER:
          cmdBuf[frameTail] = 0x02;   //P02
          break;
  
      	case CTRL_COMMAND:
          cmdBuf[frameTail] = 0x03;   //P02
          break;
        
        case FILE_TRANSPORT:
          cmdBuf[frameTail] = 0x04;   //P02
          break;
  
        case DATA_FORWARD:
          cmdBuf[frameTail] = 0x05;   //P02
          break;        	
  
      	default:
          cmdBuf[frameTail] = 0x01;   //P02
          break;
      }
    }
    else
    {
			switch(ifSingle)
		  {
		  	case 1:
          cmdBuf[frameTail++] = 0x15;     //P01
		  		break;
		  	case 2:
          cmdBuf[frameTail++] = 0x16;     //P01
		  		break;
		  	case 3:
          cmdBuf[frameTail++] = 0x17;     //P01
		  		break;
		  	case 4:
          cmdBuf[frameTail++] = 0x18;     //P01
		  		break;
		  	case 5:
          cmdBuf[frameTail++] = 0x19;     //P01
		  		break;
		  	case 6:
          cmdBuf[frameTail++] = 0x1a;     //P01
		  		break;
		  	case 7:
          cmdBuf[frameTail++] = 0x1b;     //P01
		  		break;
		  	case 8:
          cmdBuf[frameTail++] = 0x1c;     //P01
		  		break;
		  	
		  	//2011-06-24,北京测试后发现系统广播地址所有的MAC验证都不合格,
		  	//  因为电科院提供的资料上没有系统广播地址的初始向量,问电科院的人才知道系统广播地址的向量是0x1d
		  	case 9:
          cmdBuf[frameTail++] = 0x1d;     //P01
		  		break;
		  		
		    default:
          cmdBuf[frameTail++] = 0x00;     //P01
		    	break;
		  }
		  
    	switch(afn)
    	{
    		case RESET_CMD:      //AFN==01H
          cmdBuf[frameTail] = 0x0b;     //P02
    		  break;

    		case SET_PARAMETER:  //AFN==04H
          cmdBuf[frameTail] = 0x0c;     //P02
          break;

    		case CTRL_COMMAND:   //AFN==05H
          cmdBuf[frameTail] = 0x0d;     //P02
    		  break;
    		  
        case FILE_TRANSPORT:
          cmdBuf[frameTail] = 0x0e;     //P02
          break;
  
        case DATA_FORWARD:
          cmdBuf[frameTail] = 0x0f;     //P02
          break;    		  
    	}
    }
    cmdBuf[frameTail]<<=2;
      
    if (numOfBlock==1)
    {
      cmdBuf[frameTail] |= 0x3;       //仅有一个级联块
    }
    else
    {
    	if (i==0)
    	{
    		 cmdBuf[frameTail] |= 1;      //第一个级联块
    	}
    	else
    	{
    		if ((i+1)<numOfBlock)
    		{
    			 cmdBuf[frameTail] |= 2;    //中间级联块
    		}
    	}
    }
    frameTail++;

    if ((i+1)==numOfBlock)
    {
      cmdBuf[frameTail++] = lenOfBuf%NUM_OF_MAC_BLOCK;   //Lc
      memcpy(&cmdBuf[frameTail], &buf[i*NUM_OF_MAC_BLOCK], lenOfBuf%NUM_OF_MAC_BLOCK);
      frameTail += lenOfBuf%NUM_OF_MAC_BLOCK;
    }
    else
    {
      cmdBuf[frameTail++] = NUM_OF_MAC_BLOCK;   //Lc
      memcpy(&cmdBuf[frameTail], &buf[i*NUM_OF_MAC_BLOCK], NUM_OF_MAC_BLOCK);
      frameTail += NUM_OF_MAC_BLOCK;
    }
  }
  
  result = read(fdOfIoChannel, cmdBuf, 4);
  
  if (result==0)
  {
  	swAnalyse("MAC计算",cmdBuf[4],cmdBuf[5]);
  	
    if (cmdBuf[4]==0x90 && cmdBuf[5]==0x00)
    {
  	  retValue = 1;
  	  memcpy(retMac, cmdBuf, 4);  	  
  	  
  	  if (debugInfo&ESAM_DEBUG)
  	  {
  	    printf("终端计算后的MAC值:%02x-%02x-%02x-%02x\n",retMac[0],retMac[1],retMac[2],retMac[3]);
  	  }
  	}
  }
  else
  {
  	if (debugInfo&ESAM_DEBUG)
  	{
  	  switch(result)
  	  {
  	    case 1:
  	      printf("MAC计算:接收校验错误\n");
  	      break;
  	      
  	    case 2:
  	      printf("MAC计算:接收第一字节不等INS字节\n");
  	      break;
  	  }
  	}
  }
  
  return retValue;
}

/*******************************************************
函数名称:swAnalyse
功能描述:响应状态码分析
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void swAnalyse(char *say, INT8U sw1,INT8U sw2)
{
  if (debugInfo&ESAM_DEBUG)
  {
  	 printf("响应状态码:SW1=%02x,SW2=%02x\n", sw1, sw2);
  	 
  	 if (sw1==0x90 && sw2==0x00)
     {
  	    printf("%s:正确执行\n", say);
  	 }
     
     if (sw1==0x90 && sw2==0x72)
     {
      	printf("%s:指令结构错误\n",say);
     }

  	 if (sw1==0x90 && sw2==0x73)
     {
    	 printf("%s:SM1密钥错误\n", say);
     }

     if (sw1==0x90 && sw2==0x74)
     {
      	printf("%s:验签文件类型不匹配\n", say);
     }
  
     if (sw1==0x90 && sw2==0x75)
     {
      	printf("%s:验签文件未找到\n", say);
     }
     
     if(sw1==0x90 && sw2==0x76)
     {
      	printf("%s:产生RSA密钥对时么钥文件未找到\n", say);
     }
     
     if (sw1==0x90 && sw2==0x77)
     {
      	printf("%s:用来加密的公钥文件不匹配\n", say);
     }
    
     if (sw1==0x90 && sw2==0x78)
     {
       printf("%s:用来加密的公钥文件没找到\n", say);
     }
     
  	 if (sw1==0x90 && sw2==0x79)
     {
    	 printf("%s:用来解密的公鉏文件不匹配\n", say);
     }
  	 if (sw1==0x90 && sw2==0x7a)
     {
    	 printf("%s:用来解密的公钥文件没找到\n", say);
     }
  	 if (sw1==0x90 && sw2==0x7b)
     {
    	 printf("%s:用来签名的私钥文件不匹配\n", say);
     }
  	 if (sw1==0x90 && sw2==0x7)
     {
    	 printf("%s:用来签名的私钥文件没找到\n", say);
     }
     if (sw1==0x90 && sw2==0x82)
     {
       printf("%s:RSA加密错误\n", say);
     }
     if (sw1==0x90 && sw2==0x86)
     {
       printf("%s:RSA验签错误\n",say);
     }
     if (sw1==0x90 && sw2==0x88)
     {
       printf("%s:RSA产生密钥对错误\n",say);
     }
  	 if (sw1==0x90 && sw2==0x84)
     {
    	 printf("%s:RSA解密错误\n",say);
     }

  	 if (sw1==0x90 && sw2==0x8a)
     {
    	 printf("%s:RSA签名错误\n",say);
     }
  	 if (sw1==0x90 && sw2==0x8c)
     {
    	 printf("%s:SM1计算错误\n",say);
     }
     if (sw1==0x67 && sw2==0x00)
     {
    	 printf("%s:数据长度错误\n", say);
     }
     
     if (sw1==0x69 && sw2==0x81)
     {
    	 printf("%s:P1、P2所指的标识符不是响应的公钥文件\n", say);
     }
     if (sw1==0x69 && sw2==0x82)
     {
    	 printf("%s:使用条件不满足\n", say);
     }
     if (sw1==0x69 && sw2==0x83)
     {
    	 printf("%s:公钥文件未找到\n", say);
     }
     
     if (sw1==0x69 && sw2==0x84)
     {
       printf("%s:没有可用随机数\n",say);
     }
     
     if (sw1==0x6a && sw2==0x80)
     {
    	 printf("%s:数据格式错误\n", say);
     }
     



  }
}
       
