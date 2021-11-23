/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
文件名：AFN10.c
作者：leiyong
版本：0.9
完成日期：2007年3月
描述：主站AFN10(数据转发)处理文件。
函数列表：
     1.
修改历史：
  01,07-3-22,Tianye created.
  02,07-11-17,leiyong modify,将原来需要查看是否配置测量点后再转发改为直接转发出去
  03,10-04-01,leiyong移植到AT91SAM9260
***************************************************/
#include "common.h"
#include "teRunPara.h"
#include "copyMeter.h"
#include "AFN10.h"

/*******************************************************
函数名称:AFN10
功能描述:主站"数据转发命令"(AFN0C)的处理函数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void AFN10(INT8U *pDataHead, INT8U *pDataEnd,INT8U dataFrom)
{
	 INT8U    fn, pn;
	 INT8U    *pData;
	 INT16U   frameTail;
	 INT8U    tmpPort; 
    
	 pData = pDataHead;
	  
	 pn = findFnPn(*pData, *(pData+1), FIND_PN);
	 fn = findFnPn(*(pData+2), *(pData+3), FIND_FN);
	  
	 //只接受和P0
	 if (pn != 0)
	 {
	   return;
	 }
	 pData+=4;
    
   //端口只接受1,2,3及31
   //端口只接受1,2,3,4及31,2012-03-28改成接爱1 2 3 4了
   tmpPort = *pData++;
   if ((tmpPort>0 && tmpPort<5) || tmpPort==PORT_POWER_CARRIER)
   {
     if (tmpPort==PORT_POWER_CARRIER)
     {
   	   tmpPort = 4;
   	   
   	   //2015-12-05,改成只对载波作这个判断
      #ifdef LIGHTING
       if (0<carrierFlagSet.broadCast)
       {
         ackOrNack(FALSE,dataFrom);    //全部否认
         
         return;
       }
      #endif
     }
     else
     { 
   	   tmpPort--;
     }
     

     printf("%02d-%02d-%02d %02d:%02d:%02d,AFN10,转发端口%d,Fn=%d\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, tmpPort, fn);
     
     //如果该端口当前未处理转发
     if (copyCtrl[tmpPort].pForwardData==NULL)
     {
   	  switch(fn)
   	  {
   	 	  case 1:    //透明转发
       	  copyCtrl[tmpPort].pForwardData = (FORWARD_DATA *)malloc(sizeof(FORWARD_DATA));
          copyCtrl[tmpPort].pForwardData->fn            = 1;
          copyCtrl[tmpPort].pForwardData->dataFrom      = dataFrom;
       	  copyCtrl[tmpPort].pForwardData->ifSend        = FALSE;
       	  copyCtrl[tmpPort].pForwardData->receivedBytes = FALSE;
       	  copyCtrl[tmpPort].pForwardData->forwardResult = RESULT_NONE;
       
       	  //透明转发通信控制字
       	  copyCtrl[tmpPort].pForwardData->ctrlWord = *pData++;
       	  
       	  //透明转发接收等待报文超时时间
       	  if ((*pData&0x80)==0)   //单位为ms
       	  {
       	  	 copyCtrl[tmpPort].pForwardData->frameTimeOut = (*pData&0x7f)/100;
       	  	 if (copyCtrl[tmpPort].pForwardData->frameTimeOut <1)
       	  	 {
       	  	 	 copyCtrl[tmpPort].pForwardData->frameTimeOut = 2;
       	  	 }
       	  }
       	  else                    //单位为s
       	  {
       	  	copyCtrl[tmpPort].pForwardData->frameTimeOut = *pData&0x7f;
       	  }       	  
       	  pData++;
       	  
       	  //透明转发接收等待字节超时时间
       	  copyCtrl[tmpPort].pForwardData->byteTimeOut = *pData++;
       	  
       	  copyCtrl[tmpPort].pForwardData->length = *pData | *(pData+1)<<8;
       	  pData+=2;
       	  
       	  //载波/无线端口,去掉0xFE,否则本地通信模块会无法抄表的,2012-08-20
       	  if (tmpPort==4)
       	  {
       	    while(copyCtrl[tmpPort].pForwardData->length>0)
       	    {
       	      if (*pData==0xfe)
       	      {
       	    	  pData++;
       	    	  copyCtrl[tmpPort].pForwardData->length--;
       	      }
       	      else
       	      {
       	    	  break;
       	      }
       	    }
       	  }
       	  
       	  //透明转发内容
       	  memcpy(copyCtrl[tmpPort].pForwardData->data,pData,copyCtrl[tmpPort].pForwardData->length);
       	  
       	 #ifdef LIGHTING 
       	  if (tmpPort==4)
       	  {
       	    if (0x99==copyCtrl[tmpPort].pForwardData->data[1]
       	    	 || 0x99==copyCtrl[tmpPort].pForwardData->data[2]
       	    	  || 0x99==copyCtrl[tmpPort].pForwardData->data[3]
       	    	   || 0x99==copyCtrl[tmpPort].pForwardData->data[4]
       	    	    || 0x99==copyCtrl[tmpPort].pForwardData->data[5]
       	    	     || 0x99==copyCtrl[tmpPort].pForwardData->data[6]
       	    	 )
       	    {
       	      carrierFlagSet.broadCast = 2;
       	    }
       	  }
       	 #endif
   	 	    break;
   	 	  	
   	 	  case 9:    //转发主站直接对电能表的抄读数据命令
       	  copyCtrl[tmpPort].pForwardData = (FORWARD_DATA *)malloc(sizeof(FORWARD_DATA));
          copyCtrl[tmpPort].pForwardData->fn       = 9;
          copyCtrl[tmpPort].pForwardData->dataFrom = dataFrom;
       	  copyCtrl[tmpPort].pForwardData->ifSend   = FALSE;
       
       	  //透明转发通信控制字(未指定用终端原设定各端口的控制字)
       	  copyCtrl[tmpPort].pForwardData->ctrlWord = 0x0;
       	  
       	  //透明转发接收等待报文超时时间s(未指定)
       	  copyCtrl[tmpPort].pForwardData->frameTimeOut = 30;

       	  //透明转发接收等待字节超时时间(未指定)
       	  copyCtrl[tmpPort].pForwardData->byteTimeOut = 0x0;
       	  
       	  //未指定
       	  copyCtrl[tmpPort].pForwardData->length = 0x0;
       	  copyCtrl[tmpPort].pForwardData->forwardResult = RESULT_NONE;

       	  //转发目标地址,标识类型,数据标识
       	  if (*pData==0xff)    //无指定中继路由
       	  {
       	     memcpy(copyCtrl[tmpPort].pForwardData->data,pData+1,11);
       	  }
       	  else
       	  {
       	     memcpy(copyCtrl[tmpPort].pForwardData->data,pData+((*pData*6)+1),11);
       	  }

   	 	    break;
   	 	  
   	 	  case 10:   //转发主站直接对电能表的遥控跳闸/允许合闸命令
   	 	    break;
   	 	  	
   	 	  case 11:   //转发主站直接对电能表的遥控送电命令
   	 	    break;
   	 	}
     }    
   }
}