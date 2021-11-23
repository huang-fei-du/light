/***************************************************
Copyright,2010,Huawei Wodian co.,LTD,All	Rights Reserved
文件名：AFN0C.c
作者：Leiyong
版本：0.9
完成日期：2010年3月
描述：主站AFN09(请求终端配置及信息)处理文件。
函数列表：
     1.
修改历史：
  01,10-3-23,leiyong created.
***************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/if_arp.h>

#include "common.h"
#include "teRunPara.h"
#include "msSetPara.h"
#include "workWithMeter.h"
#include "copyMeter.h"
#include "dataBase.h"
#include "convert.h"
#include "userInterface.h"

#include "AFN09.h"

extern INT8U  ackTail;

/*******************************************************
函数名称:AFN09
功能描述:主站"请求终端配置及信息"(AFN09)的处理函数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void AFN09(INT8U *pDataHead, INT8U *pDataEnd,INT8U dataFrom, INT8U poll)
{
    INT16U   tmpI,tmpFrameTail;
    INT8U    fn;
    INT8U    tmpDtCount;              //DT移位计数
    INT8U    tmpDt1;                  //临时DT1
    INT8U    *pTpv;                   //TpV指针
    INT8U    maxCycle;                //最大循环次数
    INT16U   frameTail09;             //AFN09帧尾
    INT16U   tmpHead09;               //AFN09临时帧头
    INT16U   tmpHead09Active;         //主动上报AFN09临时帧头

    INT16U (*AFN09Fun[11])(INT16U frameTail,INT8U *pHandlem, INT8U fn);
    for (tmpI=0; tmpI<11; tmpI++)
    {
       AFN09Fun[tmpI] = NULL;
    }
       
    //组1
    AFN09Fun[0] = AFN09001;
    AFN09Fun[1] = AFN09002;
    AFN09Fun[2] = AFN09003;
    AFN09Fun[3] = AFN09004;
    AFN09Fun[4] = AFN09005;
    AFN09Fun[5] = AFN09006;
    AFN09Fun[6] = AFN09007;
    AFN09Fun[7] = AFN09008;
    
   #ifdef SDDL_CSM
    AFN09Fun[10] = AFN09011;
   #endif
    
    if (fQueue.tailPtr == 0)
    {
       tmpHead09 = 0;
    }
    else
    {
       tmpHead09 = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
    }

    frameTail09 = tmpHead09 + 14;
    
    for (ackTail = 0; ackTail < 100; ackTail++)
    {
      ackData[ackTail] = 0;
    }
    ackTail = 0;
    
    tmpDt1 = 0;
    tmpDtCount = 0;
    ackTail = 0;
    maxCycle = 0;
    
    while ((frame.loadLen > 0) && (maxCycle<50))
    {
        maxCycle++;
        
        tmpDt1 = *(pDataHead + 2);
        tmpDtCount = 0;
        while(tmpDtCount < 9)
        {
           tmpDtCount++;
           if ((tmpDt1 & 0x1) == 0x1)
           {
           	  fn = *(pDataHead + 3) * 8 + tmpDtCount;
           	  //执行函数
             #ifdef SDDL_CSM
              if (AFN09Fun[fn-1] != NULL && (fn <= 8 || 11==fn))
             #else
              if (AFN09Fun[fn-1] != NULL && fn <= 8)
             #endif
              {
                 tmpFrameTail = AFN09Fun[fn-1](frameTail09, pDataHead, fn);
                 if (tmpFrameTail== frameTail09)
                 {
                 	  ackData[ackTail*5] = *pDataHead;                             //DA1
                 	  ackData[ackTail*5+1] = *(pDataHead+1);                       //DA2
                 	  ackData[ackTail*5+2] = 0x1 << ((fn%8 == 0) ? 7 : (fn%8-1));  //DT1
                 	  ackData[ackTail*5+3] = (fn-1)/8;                             //DT2
                 	  ackData[ackTail*5+4] = 0x01;                                 //无有效数据
                 	  ackTail++;
                 }
                 else
                 {
                 	  frameTail09 = tmpFrameTail;
                 }
              }
           }
           
           tmpDt1 >>= 1;
                      
           if ((frameTail09 - tmpHead09) > MAX_OF_PER_FRAME || (((pDataHead+4) == pDataEnd) && tmpDtCount==8))
           {
              //不允许主动上报且有事件发生
              if (frame.acd==1 && (callAndReport&0x03)== 0x02 && (frameTail09 - tmpHead09) > 16)
              {
              	  msFrame[frameTail09++] = iEventCounter;
              	  msFrame[frameTail09++] = nEventCounter;
              }
              
              //根据启动站要求判断是否携带TP
              if (frame.pTp != NULL)
              {
                 pTpv = frame.pTp;
                 msFrame[frameTail09++] = *pTpv++;
                 msFrame[frameTail09++] = *pTpv++;
                 msFrame[frameTail09++] = *pTpv++;
                 msFrame[frameTail09++] = *pTpv++;
                 msFrame[frameTail09++] = *pTpv++;
                 msFrame[frameTail09++] = *pTpv;
              }
              
              msFrame[tmpHead09 + 0] = 0x68;   //帧起始字符
            
              tmpI = ((frameTail09 - tmpHead09 - 6) << 2) | 0x2;
              msFrame[tmpHead09 + 1] = tmpI & 0xFF;   //L
              msFrame[tmpHead09 + 2] = tmpI >> 8;
              msFrame[tmpHead09 + 3] = tmpI & 0xFF;   //L
              msFrame[tmpHead09 + 4] = tmpI >> 8; 
            
              msFrame[tmpHead09 + 5] = 0x68;  //帧起始字符

       
              msFrame[tmpHead09 + 6] = 0x88;     //控制字节10001000(DIR=1,PRM=0,功能码=0x8)

              if (frame.acd==1 && (callAndReport&0x03)== 0x02)   //不允许主动上报且有事件发生
              {
                  msFrame[tmpHead09 + 6] |= 0x20;
              }
       
              //地址
              msFrame[tmpHead09 + 7] = addrField.a1[0];
              msFrame[tmpHead09 + 8] = addrField.a1[1];
              msFrame[tmpHead09 + 9] = addrField.a2[0];
              msFrame[tmpHead09 + 10] = addrField.a2[1];
              msFrame[tmpHead09 + 11] = addrField.a3;
              
              msFrame[tmpHead09 + 12] = 0x09;  //AFN
       
              msFrame[tmpHead09+13] = 0;
              
              if (frame.pTp != NULL)
              {
              	 msFrame[tmpHead09+13] |= 0x80;       //TpV置位
              }
              
              msFrame[frameTail09+1] = 0x16;
              
              fQueue.frame[fQueue.tailPtr].head = tmpHead09;
              fQueue.frame[fQueue.tailPtr].len = frameTail09 + 2 - tmpHead09;
              
              if (((frameTail09 - tmpHead09) > 16 && frame.pTp==NULL) || ((frameTail09 - tmpHead09) > 22 && frame.pTp!=NULL))
              {
                 tmpHead09 = frameTail09+2;
                 if ((tmpHead09+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
                    	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
                 {
                    fQueue.frame[fQueue.tailPtr].next = 0x0;
                    fQueue.tailPtr = 0;
                    tmpHead09 = 0;
                 }
                 else
                 {                 
                    fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
                    fQueue.tailPtr++;
                 }

                 frameTail09 = tmpHead09 + 14;  //frameTail重新置位填写下一帧
              }
           }
        }
        
        pDataHead += 4;
        if (frame.loadLen<4)
        {
        	break;
        }
        else
        {
          frame.loadLen -= 4;
        }
    }
    
    if (ackTail !=0)
    {
       AFN00003(ackTail, dataFrom, 0x0c);
    }
}

/*******************************************************
函数名称:AFN09001
功能描述:响应主站请求终端配置及信息"终端版本信息"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN09001(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
    char str[6];
    INT8U i;
    
    //数据单元标识
    msFrame[frameTail++] = *pHandle++;    //DA1
    msFrame[frameTail++] = *pHandle++;    //DA2
    msFrame[frameTail++] = 0x01;          //DT1
    msFrame[frameTail++] = 0x00;          //DT2
    
    //数据单元
    //1.厂商代号:4字节ASCII码
	  #ifdef CQDL_CSM
	   msFrame[frameTail++] = 'C';        //厂商代号1(沃电科技"C"的ASCII码)
	   msFrame[frameTail++] = 'Q';        //厂商代号2(沃电科技"Q"的ASCII码)
	   msFrame[frameTail++] = 'H';        //厂商代号3(沃电科技"H"的ASCII码)
	   msFrame[frameTail++] = 'W';        //厂商代号4(沃电科技"W"的ASCII码)
	  #else
	   msFrame[frameTail++] = csNameId[8];//厂商代号1
	   msFrame[frameTail++] = csNameId[9];//厂商代号2
	   msFrame[frameTail++] = csNameId[10];//厂商代号3
	   msFrame[frameTail++] = csNameId[11];//厂商代号4
	  #endif

	 /*
	 #ifdef OEM_BDZNDN 
	  msFrame[frameTail++] = 'Z';        //厂商代号1(保定智能电脑"Z"的ASCII码)
	  msFrame[frameTail++] = 'N';        //厂商代号2(保定智能电脑"N"的ASCII码)
	  msFrame[frameTail++] = 'D';        //厂商代号3(保定智能电脑"D"的ASCII码)
	  msFrame[frameTail++] = 'N';        //厂商代号4(保定智能电脑"N"的ASCII码)
	 #else
	  #ifdef OEM_YZKJ
	   msFrame[frameTail++] = 'Y';        //厂商代号1(盈洲科技"Y"的ASCII码)
	   msFrame[frameTail++] = 'Z';        //厂商代号2(盈洲科技"Z"的ASCII码)
	   msFrame[frameTail++] = 'S';        //厂商代号3(盈洲科技"S"的ASCII码)
	   msFrame[frameTail++] = 'C';        //厂商代号4(盈洲科技"C"的ASCII码)	    
	  #else
	   #ifdef OEM_GSPG	    
	    msFrame[frameTail++] = 'G';        //厂商代号1(甘肃普光"G"的ASCII码)
	    msFrame[frameTail++] = 'S';        //厂商代号2(甘肃普光"S"的ASCII码)
	    msFrame[frameTail++] = 'P';        //厂商代号3(甘肃普光"P"的ASCII码)
	    msFrame[frameTail++] = 'G';        //厂商代号4(甘肃普光"G"的ASCII码)
	   #else
	    #ifdef OEM_LCDZ
	     msFrame[frameTail++] = 'L';        //厂商代号1(鲁成电子"L"的ASCII码)
	     msFrame[frameTail++] = 'C';        //厂商代号2(鲁成电子"C"的ASCII码)
	     msFrame[frameTail++] = 'D';        //厂商代号3(鲁成电子"D"的ASCII码)
	     msFrame[frameTail++] = 'Z';        //厂商代号4(鲁成电子"Z"的ASCII码)
	    #else
	     #ifdef OEM_SCDL
	      msFrame[frameTail++] = 'S';        //厂商代号1(三川电力"S"的ASCII码)
	      msFrame[frameTail++] = 'C';        //厂商代号2(三川电力"C"的ASCII码)
	      msFrame[frameTail++] = 'D';        //厂商代号3(三川电力"D"的ASCII码)
	      msFrame[frameTail++] = 'L';        //厂商代号4(三川电力"L"的ASCII码)
	     #else
	      msFrame[frameTail++] = 'C';        //厂商代号1(沃电科技"C"的ASCII码)
	      msFrame[frameTail++] = 'Q';        //厂商代号2(沃电科技"Q"的ASCII码)
	      msFrame[frameTail++] = 'H';        //厂商代号3(沃电科技"H"的ASCII码)
	      msFrame[frameTail++] = 'W';        //厂商代号4(沃电科技"W"的ASCII码)
	     #endif
	    #endif
	   #endif
	  #endif
	 #endif
	 */

	  //2.设备编号:8字节ASCII码
	 #ifdef CQDL_CSM
	  intToString(deviceNumber, 3, str);
	  
	  //2010年第2批"1002"
	  msFrame[frameTail++] = '1';
	  msFrame[frameTail++] = '0';
	  msFrame[frameTail++] = '0';
	  msFrame[frameTail++] = '2';
	  
	  for(i=0;i<4-strlen(str);i++)
	  {
	    msFrame[frameTail++] = 0x30;
	  }
	 
	  for(i=0;i<strlen(str);i++)
	  {
	    msFrame[frameTail++] = str[i];    //设备编号1(ASCII码)
	  }
	 #else
	  msFrame[frameTail++] = '0';
	  msFrame[frameTail++] = '0';
	  msFrame[frameTail++] = '0';
	  
	  #ifdef TE_ADDR_USE_BCD_CODE
	   intToString((addrField.a2[1]>>4)*1000 + (addrField.a2[1]&0xf)*100 + (addrField.a2[0]>>4)*10 + (addrField.a2[0]&0xf),3,str);
	  #else
	   intToString(addrField.a2[1]<<8 | addrField.a2[0],3,str);
	  #endif

	  for(i=0;i<5-strlen(str);i++)
	  {
	    msFrame[frameTail++] = 0x30;
	  }
	 
	  for(i=0;i<strlen(str);i++)
	  {
	    msFrame[frameTail++] = str[i];    //终端地址作为设备编号1(ASCII码)
	  }
	 #endif
	
	  //3.终端软件版本号:4字节ASCII
	  msFrame[frameTail++] = vers[0];        //终端版本号1("v"的ASCII码)
	  msFrame[frameTail++] = vers[1];        //终端版本号2("1"的ASCII码,升级后1可变)
	  msFrame[frameTail++] = vers[2];        //终端版本号3("."的ASCII码)
	  msFrame[frameTail++] = vers[3];        //终端版本号4("x"的ASCII码,升级x可变)
	
	  //4.终端软件发布日期:日月年
  	msFrame[frameTail++] = (dispenseDate[8]-0x30)<<4 | (dispenseDate[9]-0x30);    //日
  	msFrame[frameTail++] = (dispenseDate[5]-0x30)<<4 | (dispenseDate[6]-0x30);    //月
	  msFrame[frameTail++] = (dispenseDate[2]-0x30)<<4 | (dispenseDate[3]-0x30);    //年
	  
	  //5.终端配置容量信息码:11字节ASCII码
	 
	  //DJGZ221002R
    //5.1
	  #ifdef PLUG_IN_CARRIER_MODULE
      msFrame[frameTail++] = 0x44;        //终端配置容量信息码1("D"的ASCII码)
  	  msFrame[frameTail++] = 0x4A;        //终端配置容量信息码2("J"的ASCII码)
  	#else
      msFrame[frameTail++] = 'F';        //终端配置容量信息码1("F"的ASCII码)
  	  msFrame[frameTail++] = 'K';        //终端配置容量信息码2("K"的ASCII码)  	
  	#endif
	  
	  //5.2模块类型标识
	  switch(moduleType)
	  {
	  	 case GPRS_SIM300C:
	  	 case GPRS_GR64:
	  	 case GPRS_M590E:
	  	 case GPRS_M72D:
	       msFrame[frameTail++] = 0x47;   //终端配置容量信息码3("G"的ASCII码)
	       break;
	       
	  	 case CDMA_DTGS800:
	  	 case CDMA_CM180:
	       msFrame[frameTail++] = 0x43;   //终端配置容量信息码3("C"的ASCII码)
	       break;

	  	 case ETHERNET:
	       msFrame[frameTail++] = 0x45;   //终端配置容量信息码3("E"的ASCII码)
	       break;
	       
       case CASCADE_TE:
	       msFrame[frameTail++] = 'L';   //终端配置容量信息码3("L"的ASCII码)
	       break;
			 
       case LTE_AIR720H:
	       msFrame[frameTail++] = '4';   //终端配置容量信息码3("4"的ASCII码)
	       break;
	  	 
	  	 default:
	       msFrame[frameTail++] = 0x20;    //终端配置容量信息码3(" "的ASCII码)
	       break;
	  } 
    
    //5.3
    #ifdef LOAD_CTRL_MODULE
  	  msFrame[frameTail++] = 'A';        //终端配置容量信息码4("A"的ASCII码)
  	  msFrame[frameTail++] = '4';        //终端配置容量信息码5("4"的ASCII码)
  	  msFrame[frameTail++] = '2';        //终端配置容量信息码6("2"的ASCII码)
    #else
  	  msFrame[frameTail++] = 0x5a;        //终端配置容量信息码4("Z"的ASCII码)
  	  msFrame[frameTail++] = 0x32;        //终端配置容量信息码5("2"的ASCII码)
  	  msFrame[frameTail++] = 0x32;        //终端配置容量信息码6("2"的ASCII码)
  	#endif

   #ifdef CQDL_CSM
   
    msFrame[frameTail++] = 'W';
    msFrame[frameTail++] = 'D';
    msFrame[frameTail++] = '0';
    msFrame[frameTail++] = '0';
    msFrame[frameTail++] = '1';
   
   #else
    //5.4CPU硬件版本确定
    //#ifdef CPU_HW_VER_1_0
     msFrame[frameTail++] = 0x31;      //终端配置容量信息码7("1"的ASCII码)
     msFrame[frameTail++] = 0x30;      //终端配置容量信息码8("0"的ASCII码)
    //#else
    // msFrame[frameTail++] = 0x20;      //终端配置容量信息码7(" "的ASCII码)
    // msFrame[frameTail++] = 0x20;      //终端配置容量信息码8(" "的ASCII码)
    //#endif
    
    //5.5控制单元硬件版本确定
    #ifdef JZQ_CTRL_BOARD_V_0_3
      msFrame[frameTail++] = 0x30;    //终端配置容量信息码09("0"的ASCII码)
      msFrame[frameTail++] = 0x33;    //终端配置容量信息码10("3"的ASCII码)
    #else
     #ifdef JZQ_CTRL_BOARD_V_1_4
      msFrame[frameTail++] = 0x31;    //终端配置容量信息码09("1"的ASCII码)
      msFrame[frameTail++] = 0x34;    //终端配置容量信息码10("4"的ASCII码)
     #else        
      msFrame[frameTail++] = 0x30;    //终端配置容量信息码09("0"的ASCII码)
      msFrame[frameTail++] = 0x30;    //终端配置容量信息码10("0"的ASCII码)
     #endif
    #endif
    
    //5.6时钟是否用硬件外置时间
    if (bakModuleType==MODEM_PPP)  //2012-11-08,添加此判断
    {
      msFrame[frameTail++] = 'P';      //终端配置容量信息码11("P"的ASCII码)使用PPP拨号
    }
    else
    {
     #ifdef RTC_USE_ISL12022M
       msFrame[frameTail++] = 0x49;    //终端配置容量信息码11("I"的ASCII码)EPSON RX8025     
     #else
       msFrame[frameTail++] = 0x20;    //终端配置容量信息码11(" "的ASCII码)      
     #endif
    }
   #endif 
    
    //6.终端通信协议、版本号4Bytes(376.1)
   #ifdef CQDL_CSM
    msFrame[frameTail++] = 'C';
    msFrame[frameTail++] = 'Q';
    msFrame[frameTail++] = '0';
    msFrame[frameTail++] = '4';
   #else
    msFrame[frameTail++] = 0x33;
    msFrame[frameTail++] = 0x37;
    msFrame[frameTail++] = 0x36;
    #ifdef SDDL_CSM
     msFrame[frameTail++] = 0x20;
    #else
     msFrame[frameTail++] = 0x31;
    #endif
   #endif
    
    //7.终端硬件版本号4Bytes ASCII
	  msFrame[frameTail++] = hardwareVers[0];
	  msFrame[frameTail++] = hardwareVers[1];
	  msFrame[frameTail++] = hardwareVers[2];
	  msFrame[frameTail++] = hardwareVers[3];	
    
    //8.终端硬件发布日期:日、月、年
  	msFrame[frameTail++] = (hardwareDate[8]-0x30)<<4 | (hardwareDate[9]-0x30);    //日
  	msFrame[frameTail++] = (hardwareDate[5]-0x30)<<4 | (hardwareDate[6]-0x30);    //月
	  msFrame[frameTail++] = (hardwareDate[2]-0x30)<<4 | (hardwareDate[3]-0x30);    //年
    
    return frameTail;   
}

/*******************************************************
函数名称:AFN09002
功能描述:响应主站请求终端配置及信息"终端支持的输入、输出及通信端口配置"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN09002(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   //ly,2011-5-21,add
   register int fd, interface, retn = 0;
   struct ifreq buf[MAXINTERFACES];
   struct ifconf ifc;
   INT8U  ifFoundMac;

   //数据单元标识
   msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
   msFrame[frameTail++] = 0x02;          //DT1
   msFrame[frameTail++] = 0x00;          //DT2
   
   //脉冲量输入路数
   msFrame[frameTail++] = NUM_OF_SWITCH_PULSE;

   //开关量输入路数
   msFrame[frameTail++] = NUM_OF_SWITCH_PULSE;
   
   //直流模块量输入路数
   msFrame[frameTail++] = NUM_OF_ADC;
   
   //开关量输出路数(轮次)
   #ifdef COLLECTOR
    msFrame[frameTail++] = 0;
   #else
    msFrame[frameTail++] = 0;
   #endif
   
   //支持的抄电能表/交流采样装置最多个数 2040
   msFrame[frameTail++] = 0xf8;
   msFrame[frameTail++] = 0x7;
   
   //支持的终端上行通信最大接收缓存区字节数 9600
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0x08;
   
   //支持的终端上行通信最大发送缓存区字节数
   msFrame[frameTail++] = 0x80;
   msFrame[frameTail++] = 0x25;
   
   //终端MAC地址段段
   //ly,2011-05-21,读出eth0的mac
   ifFoundMac = 0;
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
               ifFoundMac = 1;
               msFrame[frameTail++] = bcdToHex(buf[interface].ifr_hwaddr.sa_data[0]);
               msFrame[frameTail++] = bcdToHex(buf[interface].ifr_hwaddr.sa_data[1]);
               msFrame[frameTail++] = bcdToHex(buf[interface].ifr_hwaddr.sa_data[2]);
               msFrame[frameTail++] = bcdToHex(buf[interface].ifr_hwaddr.sa_data[3]);
               msFrame[frameTail++] = bcdToHex(buf[interface].ifr_hwaddr.sa_data[4]);
               msFrame[frameTail++] = bcdToHex(buf[interface].ifr_hwaddr.sa_data[5]);
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
   
   if (ifFoundMac==0)
   {
      msFrame[frameTail++] = 0x00;
      msFrame[frameTail++] = 0x00;
      msFrame[frameTail++] = 0x00;
      msFrame[frameTail++] = 0x00;
      msFrame[frameTail++] = 0x00;
      msFrame[frameTail++] = 0x00;
   }
   
   //通信端口数量
   msFrame[frameTail++] = 0x03;
   
   //第一个通信端口号及信息字(专变0,标准异步串行口0,直接RS485)
  #ifdef RS485_1_USE_PORT_1
   msFrame[frameTail++] = 0x1;  //端口1
  #else
   msFrame[frameTail++] = 0x2;  //端口2
  #endif
   msFrame[frameTail++] = 0x0;
   
   //第一个通信端口支持的最高波特率115200
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0xc2;
   msFrame[frameTail++] = 0x01;
   msFrame[frameTail++] = 0x00;
   
   //第一个通信端口支持的设备个数 32个
   msFrame[frameTail++] = 0x20;
   msFrame[frameTail++] = 0x00;
   
   //第一个通信端口支持的最大接收缓存区字节数
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0x02;

   //第一个通信端口支持的最大发送缓存区字节数
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0x01;
   
   //第二个通信端口号及信息字(专变0,标准异步串行口0,直接RS485)
  #ifdef RS485_1_USE_PORT_1 
   msFrame[frameTail++] = 0x2;  //端口2
  #else
   msFrame[frameTail++] = 0x3;  //端口3
  #endif
   msFrame[frameTail++] = 0x0;
   
   //第二个通信端口支持的最高波特率115200
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0xc2;
   msFrame[frameTail++] = 0x01;
   msFrame[frameTail++] = 0x00;
   
   //第二个通信端口支持的设备个数 32个
   msFrame[frameTail++] = 0x20;
   msFrame[frameTail++] = 0x00;
   
   //第二个通信端口支持的最大接收缓存区字节数
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0x02;

   //第二个通信端口支持的最大发送缓存区字节数
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0x01;

   //第三个通信端口号及信息字(台区低压集抄,标准异步串行口0,串行接口连接窄带低压载波通信模块,端口31)
   msFrame[frameTail++] = 0x2<<5 | 0x1f;
   msFrame[frameTail++] = 0x2<<5;
   
   //第三个通信端口支持的最高波特率115200
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0xc2;
   msFrame[frameTail++] = 0x01;
   msFrame[frameTail++] = 0x00;
   
   //第三个通信端口支持的设备个数 1500个
   msFrame[frameTail++] = 0xdc;
   msFrame[frameTail++] = 0x05;
   
   //第三个通信端口支持的最大接收缓存区字节数
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0x02;

   //第三个通信端口支持的最大发送缓存区字节数
   msFrame[frameTail++] = 0x00;
   msFrame[frameTail++] = 0x01;
   
   return frameTail;
}

/*******************************************************
函数名称:AFN09003
功能描述:响应主站请求终端配置及信息"终端支持的其它配置"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN09003(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   //数据单元标识
   msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
   msFrame[frameTail++] = 0x04;          //DT1
   msFrame[frameTail++] = 0x00;          //DT2
   
   //支持的测量点最多点数 2040
   msFrame[frameTail++] = 0xf8;
   msFrame[frameTail++] = 0x07;
   
   //支持的总加组最多组数 8
   msFrame[frameTail++] = 0x08;
   
   //支持的任务最多个数 64
   msFrame[frameTail++] = 0x40;
   
   //支持的有功总电能旦差动组最多组数 0
   msFrame[frameTail++] = 0x00;
   
   //支持的最大费率数 8
   msFrame[frameTail++] = 0x08;
	 
	 //支持的测量点数据最大冻结密度  15 30 45 00
	 msFrame[frameTail++] = 0x01;
	 
	 //支持的总加组有功功率数据最大冻结密度 15 30 45 00
	 msFrame[frameTail++] = 0x01;
	 
	 //支持的总加组无功功率数据最大冻结密度 15 30 45 00
	 msFrame[frameTail++] = 0x01;
	 
	 //支持的总加组有功电能量数据最大冻结密度 15 30 45 00
	 msFrame[frameTail++] = 0x01;
	 
	 //支持的总加组无功电能量数据最大冻结密度 15 30 45 00
	 msFrame[frameTail++] = 0x01;
	 
	 //支持的日数据最多存放天数  31天
	 msFrame[frameTail++] = 0x1f;
	 
	 //支持的月数据存放月数 12月
	 msFrame[frameTail++] = 0x0c;
	 
	 //支持的时段功控定值方案最多个数  0
	 msFrame[frameTail++] = 0x0;
	 
	 //支持的谐波检测最高谐波次数 0
	 msFrame[frameTail++] = 0x0;
	 
	 //支持的无功补偿电容组最多组数 0
	 msFrame[frameTail++] = 0x0;

   //支持的台区集中抄表重点用户最多户数 20
   msFrame[frameTail++] = 0x14;
	 
	 //支持的用户大类号标志
	 msFrame[frameTail++] = 0x3f;   //00111111
	 msFrame[frameTail++] = 0x00;
	 
	 //ly,2011-05-13,支持0到15号用户大类下用户小类号个数
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 msFrame[frameTail++] = 0x5;
	 
	 return frameTail;
}

/*******************************************************
函数名称:AFN09004
功能描述:响应主站请求终端配置及信息"终端支持的参数配置"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN09004(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   //数据单元标识
   msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
   msFrame[frameTail++] = 0x08;          //DT1
   msFrame[frameTail++] = 0x00;          //DT2
   
   //支持信息类组数n
   msFrame[frameTail++] = 11;
   
   //支持的第1组信息类组所对应的信息类无标志位
   msFrame[frameTail++] = 0xff;
   
   //支持的第2组信息类组所对应的信息类无标志位
   msFrame[frameTail++] = 0xff;

   //支持的第3组信息类组所对应的信息类无标志位
   msFrame[frameTail++] = 0x7f;

   //支持的第4组信息类组所对应的信息类无标志位
   msFrame[frameTail++] = 0x7f;

   //支持的第5组信息类组所对应的信息类无标志位
   msFrame[frameTail++] = 0x7f;

   //支持的第6组信息类组所对应的信息类无标志位
   msFrame[frameTail++] = 0xff;

   //支持的第7组信息类组所对应的信息类无标志位
   msFrame[frameTail++] = 0x01;

   //支持的第8组信息类组所对应的信息类无标志位
   msFrame[frameTail++] = 0x1f;

   //支持的第9组信息类组所对应的信息类无标志位
   msFrame[frameTail++] = 0x0f;

   //支持的第10组信息类组所对应的信息类无标志位
   msFrame[frameTail++] = 0x0f;

   //支持的第11组信息类组所对应的信息类无标志位
   msFrame[frameTail++] = 0x07;
   
   return frameTail;
}

/*******************************************************
函数名称:AFN09005
功能描述:响应主站请求终端配置及信息"终端支持的控制配置"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN09005(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   //数据单元标识
   msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
   msFrame[frameTail++] = 0x10;          //DT1
   msFrame[frameTail++] = 0x00;          //DT2
   
   //支持信息类组数n
   msFrame[frameTail++] = 0;
   
   return frameTail;
}

/*******************************************************
函数名称:AFN09006
功能描述:响应主站请求终端配置及信息"终端支持的1类数据配置"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN09006(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   //数据单元标识
   msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
   msFrame[frameTail++] = 0x20;          //DT1
   msFrame[frameTail++] = 0x00;          //DT2
   
   //支持信息类组数n
   msFrame[frameTail++] = 0;
   msFrame[frameTail++] = 0;
   
   return frameTail;
}

/*******************************************************
函数名称:AFN09007
功能描述:响应主站请求终端配置及信息"终端支持的2类数据配置"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN09007(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   //数据单元标识
   msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
   msFrame[frameTail++] = 0x40;          //DT1
   msFrame[frameTail++] = 0x00;          //DT2
   
   //支持信息类组数n
   msFrame[frameTail++] = 0;
   msFrame[frameTail++] = 0;
   
   return frameTail;
}

/*******************************************************
函数名称:AFN09008
功能描述:响应主站请求终端配置及信息"终端支持的事件记录配置"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN09008(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
   //数据单元标识
   msFrame[frameTail++] = *pHandle++;    //DA1
   msFrame[frameTail++] = *pHandle++;    //DA2
   msFrame[frameTail++] = 0x80;          //DT1
   msFrame[frameTail++] = 0x00;          //DT2
   
   //支持事件记录标志位
   msFrame[frameTail++] = 0x8f;
   msFrame[frameTail++] = 0xbf;
   msFrame[frameTail++] = 0x99;
   msFrame[frameTail++] = 0xff;
   msFrame[frameTail++] = 0x07;
   msFrame[frameTail++] = 0;
   msFrame[frameTail++] = 0;
   msFrame[frameTail++] = 0;
   
   return frameTail;
}

#ifdef SDDL_CSM

/*******************************************************
函数名称:AFN09011
功能描述:响应主站请求终端配置及信息"F11 终端有效测量点"命令
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
INT16U AFN09011(INT16U frameTail,INT8U *pHandle, INT8U fn)
{
	METER_DEVICE_CONFIG meterConfig;

  INT16U tmpFrameTail;
  INT16U numOfDa;
  INT16U numOfMp;
  INT8U  tmpDa[255];
  INT16U k;
  
  //数据单元标识
  msFrame[frameTail++] = *pHandle++;    //DA1
  msFrame[frameTail++] = *pHandle++;    //DA2
  msFrame[frameTail++] = 0x04;          //DT1
  msFrame[frameTail++] = 0x01;          //DT2
  
  tmpFrameTail = frameTail;
  
  frameTail+=4;

	//DA赋初值
	memset(tmpDa, 0x0, 255);
	numOfMp = 0;
	for(k=1; k<=2040; k++)
	{
  	if (selectF10Data(k, 0, 0, (INT8U *)&meterConfig, sizeof(meterConfig))==TRUE)
  	{
  		numOfMp++;
  				
  		tmpDa[(k-1)/8] |= 0x01<<((k%8 == 0) ? 7 : (k%8-1));
    }
  }
	
	//统计DA个数
	numOfDa = 0;
	for(k=0; k<255; k++)
	{
		if (tmpDa[k]!=0)
		{
			numOfDa++;
			msFrame[frameTail++] = tmpDa[k];
			msFrame[frameTail++] = k+1; 
		}
	}
	
	//测量点个数
	msFrame[tmpFrameTail] = numOfMp&0xff;
	msFrame[tmpFrameTail+1] = numOfMp>>8&0xff;
	
	//DA列表长度
	msFrame[tmpFrameTail+2] = numOfDa&0xff;
	msFrame[tmpFrameTail+3] = numOfDa>>8&0xff;
	
  return frameTail;
}

#endif

