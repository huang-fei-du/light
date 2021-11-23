/***************************************************
Copyright,2010,HuaWei WoDian co.,LTD,All	Rights Reserved
文件名:gdw376-2.c
作者：leiyong
版本：0.9
完成日期：2010年3月
描述:Q/GDW376-2集中器与下行通信模块(载波模块)本地接口通信协议处理文件。

函数列表：
     1.
修改历史：
  01,10-03-09,Leiyong created.
  02,10-09-19,Leiyong,修改接收错误
  03,10-09-19,Leiyong,添加支持无线模块
  04,14-01-09,Leiyong,添加预计应答字节数计算

***************************************************/

#include "stdio.h"
#include "string.h"
#include "gdw376-2.h"

#define PORT_POWER_CARRIER          31               //电力载波接口

INT8U                  *carrierModule;               //载波模块类型
INT8U                  lcModuleTypex=0;              //扩展载波模块类型(ly,2012-02-28,为了识别新版电科院模块而增加,因为新版和老版的处理不相同但又无明显数据项可查询)
INT8U                  mainNodeAddr[6];              //载波模块主节点地址
INT8U                  scMainNodeAddr[6];            //赛康的主节点地址,他的只能读出来用不能设置
GDW376_2_FRAME_ANALYSE recvFrame;                    //接收帧分析
INT8U                  gdw3762RecvBuf[512];          //接收缓存
INT16U                 recvFrameTail;                //接收帧尾

void (*send)(INT8U port,INT8U *pack,INT16U length);  //向端口发送数据函数

const INT8U expectBytes07[12][5] = {
	    {0x05, 0x06, 0x00, 0x01, 22},    //上一日冻结时间
	    {0x05, 0x06, 0x01, 0x01, 37},    //上一日日冻结正向有功电能数据
	    {0x05, 0x06, 0x02, 0x01, 37},    //上一日日冻结反向有功电能数据
      
      {0x00, 0x01, 0xff, 0x00, 37},    //当前正向有功电能示值(总及各费率)
      {0x00, 0x02, 0xff, 0x00, 37},    //当前反向有功电能示值(总及各费率)
      {0x00, 0x01, 0x00, 0x00, 22},    //当前正向有功电能示值总, 2012-5-21,add
      {0x00, 0x02, 0x00, 0x00, 22},    //当前反向有功电能示值总, 2012-5-21,add
      {0x04, 0x00, 0x05, 0xFF, 33},    //电表运行状态字1到7(7*2<>97只有2bytes)
      {0x04, 0x00, 0x01, 0x01, 21},    //日期及周次(4字节=97)
      {0x04, 0x00, 0x01, 0x02, 20},    //电表时间(3字节=97)
      {0x1e, 0x00, 0x01, 0x01, 22},    //上一次跳闸发生时刻(6字节,97没有)
      {0x1d, 0x00, 0x01, 0x01, 22},    //上一次合闸发生时刻(6字节,97没有)
	   };

/*******************************************************
函数名称:initGdw3762So
功能描述:初始化GDW376.2库
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：TRUE-初始化成功
        FALSE-初始化失败
*******************************************************/
BOOL initGdw3762So(GDW376_2_INIT *init)
{
   carrierModule = init->moduleType;
   recvFrame.afn = init->afn;
   recvFrame.fn  = init->fn;
   send = init->send;

   return TRUE;
}

/*******************************************************
函数名称:expectBytes
功能描述:预计应答字节数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
INT8U expectBytes(INT8U *pData)
{
	INT8U i;
	
	if (0x11==*(pData+8))
	{
		for(i=0; i<12; i++)
	  {
	  	if ((expectBytes07[i][3]+0x33)==*(pData+10)
	  		  || (expectBytes07[i][2]+0x33)==*(pData+11)
	  		   || (expectBytes07[i][1]+0x33)==*(pData+12)
	  		    || (expectBytes07[i][0]+0x33)==*(pData+13)
	  		 )
	    {
	    	return expectBytes07[i][4];
	    }
	  }
	}
	
	return 35;    //默认预计字节数为35字节
}

/*******************************************************
函数名称:gdw3762Frameing
功能描述:国家电网企业标准Q/GDW376.2组帧发送
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
INT8U gdw3762Framing(INT8U afn,INT8U fn,INT8U *address,INT8U *pData)
{
	 INT8U  sendBuf[255];
   INT8U  checkSum;
   INT16U i;
   INT16U frameTail;
   
   sendBuf[0]  = GDW376_2_SOP;    //起始字符
  
   //C
   switch(*carrierModule)
   {
   	 case HWWD_WIRELESS:
   	 case SR_WIRELESS:
   	 case RL_WIRELESS:
   	 case FC_WIRELESS:
   	 case SC_WIRELESS:
       sendBuf[3] = 0x4a;
       sendBuf[6] = 0x00;
       break;
       
     case MIA_CARRIER:
     	 if (afn==ROUTE_DATA_READ_3762 || afn==ACK_OR_NACK_3762)
     	 {
         sendBuf[3] = 0x01;
       }
       else
       {
         sendBuf[3] = 0x41;
       }
       sendBuf[6] = 0x00;
     	 break;
     
     default:
     	 if (afn==ROUTE_DATA_READ_3762 || afn==ACK_OR_NACK_3762)
     	 {
         sendBuf[3] = 0x01;
       }
       else
       {
         sendBuf[3] = 0x41;
       }
       sendBuf[6] = 20;    //2014-01-09,从0xff改成20
       break;
   }
    
   //R(6字节)
   sendBuf[5] = 0x00;
   sendBuf[7] = 0x00;
   sendBuf[8] = 0x00;
   sendBuf[9] = 0x00;

   frameTail = 10;
   switch(afn)
   {
     case 0x02:
     case 0x13:
       sendBuf[4] = 0x04;
     
       //主节点地址
     	 if (*carrierModule==SC_WIRELESS)
     	 {
     	   memcpy(&sendBuf[frameTail], scMainNodeAddr, 6);
     	 }
     	 else
     	 {
         memcpy(&sendBuf[frameTail], mainNodeAddr, 6);
       }
       frameTail += 6;

       //目的节点地址
       memcpy(&sendBuf[frameTail], address, 6);
       frameTail += 6;
       break;

     case 0x05:
     	 if (fn==3)
     	 {
         sendBuf[4] = 0x04;
     
         //主节点地址
     	   if (*carrierModule==SC_WIRELESS)
     	   {
     	     memcpy(&sendBuf[frameTail], scMainNodeAddr, 6);
     	   }
     	   else
     	   {
           memcpy(&sendBuf[frameTail], mainNodeAddr, 6);
         }
         frameTail += 6;

         //目的节点地址
         memcpy(&sendBuf[frameTail], address, 6);
         frameTail += 6;
       }
       break;
       
     default:
     	 if ((afn==0x14) && (*carrierModule==TC_CARRIER))
     	 {
         if (address==NULL)
         {
         	 ;
         }
         else
         {
           sendBuf[4] = 0x04;

           //相位
           if (*pData==1)
           {
           	 sendBuf[5] = *(pData+3);
           }
           else
           {
             sendBuf[5] = *(pData+(*(pData+1)+3));
           }

           //主节点地址
           memcpy(&sendBuf[frameTail], mainNodeAddr, 6);
           frameTail += 6;
  
           //目的节点地址
           memcpy(&sendBuf[frameTail], address, 6);
           frameTail += 6;
         }
     	 }
     	 else
     	 {
         sendBuf[4]  = 0x00;
       }
       break;
   }
   
   sendBuf[frameTail++] = afn;
   
   sendBuf[frameTail++] = 0x01<<((fn%8 == 0) ? 7 : (fn%8-1));
   sendBuf[frameTail++] = (fn-1)/8;
      
   switch(afn)
   {
   	 case ACK_OR_NACK_3762:
   	 	 switch(fn)
   	 	 {
   	 	 	  case 1:            //确认
   	 	 	  	sendBuf[5] |= *(pData+4);
   	 	 	  	memcpy(&sendBuf[frameTail], pData, 4);
   	 	 	  	frameTail += 4;
   	 	 	  	break;
   	 	 	  
   	 	 	  case 2:            //否认
   	 	 	  	sendBuf[frameTail++] = *pData;
   	 	 	  	break;
   	 	 }
   	 	 break;
   	 	 
   	 case DATA_FORWARD_3762: //数据转发
   	 	 switch(fn)
   	 	 {
   	 	 	 case 1:
           //规约类型
           sendBuf[frameTail++] = *pData;
           
           //抄表报文长度及报文
           memcpy(&sendBuf[frameTail], pData+1, *(pData+1)+1);
           frameTail += (*(pData+1)+1);
   	 	 	 	 break;
   	 	 }
   	 	 break;
   	 	 
   	 case QUERY_DATA_3762:   //数据查询
   	   
   	 	 break;
   	 
   	 case CTRL_CMD_3762:     //控制命令
   	 	 switch(fn)
   	 	 {
   	 	 	 case 1:             //设置主节点地址
   	   	   memcpy(&sendBuf[frameTail], address, 6);
  	   	   frameTail+=6;
   	 	 	   break;
   	 	 	 
   	 	 	 case 3:             //启动广播
   	 	 	 	 memcpy(&sendBuf[frameTail], pData+1, *pData);
   	 	 	 	 frameTail+=*pData;
   	 	 	 	 break;

   	 	 	 case 4:             //设置信道号(友讯达扩展)
   	   	   memcpy(&sendBuf[frameTail], pData, 7);
  	   	   frameTail+=7;
   	 	 	   break;
   	 	 	 
   	 	 	 case 31:            //友迅达无线模块设置时间命令
   	   	   memcpy(&sendBuf[frameTail], pData, 6);
  	   	   frameTail+=6;   	 	 	 	 
   	 	 	 	 break;
   	 	 }
   	 	 break;
   	 	 
   	 case ROUTE_QUERY_3762:  //路由查询
   	 	 switch(fn)
   	 	 {
   	 	   case 2:             //载波从节点信息
   	 	     sendBuf[frameTail++] = *pData;
   	 	     sendBuf[frameTail++] = *(pData+1);
           
           //ly,2010-12-29,从节点数量
           //1.电科院载波模块一次只能读一个(就是指定从节点数量为10个也只回一个)
           //2.东软载波模块文档上是只能读一个,但实测一次读15个都行
           //3.RL无线模块一次最多回10个
           //4.SR无线模块不能读出这个值
           //5.弥亚微载波模块一次最多查询15个载波从节点信息
           //综合考虑,一次发10个
   	 	     switch(*carrierModule)
   	 	     {
   	 	     	 case EAST_SOFT_CARRIER:
   	 	     	 case RL_WIRELESS:
   	 	     	 case MIA_CARRIER:
   	 	         sendBuf[frameTail++] = 0xa;
   	 	         break;

   	 	     	 case TC_CARRIER:   //鼎信支持一次直接读26个,保险起见读20个一次
   	 	         //2013-12-25,根据鼎信最新的评审表上说明"最大一帧查询数量为29只，为防止串口数据太长导致问题，读取数量应为每帧9只"
   	 	         //           改成每帧9只
   	 	         //sendBuf[frameTail++] = 20;
   	 	         sendBuf[frameTail++] = 9;
   	 	     	 	 break;
   	 	         
   	 	       default:   //默认为1个
   	 	       	 if (lcModuleTypex==CEPRI_CARRIER_3_CHIP)
   	 	       	 {
   	 	           sendBuf[frameTail++] = 10;
   	 	       	 }
   	 	       	 else
   	 	       	 {
   	 	           sendBuf[frameTail++] = 0x1;
   	 	         }
   	 	       	 break;
   	 	     }
   	 	     break;
   	 	     
   	 	   case 6:
   	 	   	 memcpy(&sendBuf[frameTail],pData,3);
   	 	   	 frameTail += 3;
   	 	   	 break;
   	 	 }
   	 	 break;
   	 	 
   	 case ROUTE_SET_3762:     //路由设置
   	   switch(fn)
   	   {
   	   	  case 1:             //添加从节点
   	 	      sendBuf[frameTail++] = 0x1;    //从节点数量
   	   	  	memcpy(&sendBuf[frameTail],address,6);
   	   	  	frameTail+=6;
   	 	      sendBuf[frameTail++] = *pData;
   	 	      sendBuf[frameTail++] = *(pData+1);
   	 	      sendBuf[frameTail++] = *(pData+2);  //从节点规约,由主程序保证规约号 1-97,2-07
   	   	  	break;
   	   	  
   	   	  case 2:             //删除从节点
   	   	  	sendBuf[frameTail++] = 0x1;    //从节点数量
   	   	  	memcpy(&sendBuf[frameTail], pData, 6);
   	   	  	frameTail+=6;
   	   	  	break;
   	   	  	
   	   	  case 3:
            if (*carrierModule==RL_WIRELESS)  //锐拔无线模块用这个来发组网命令,无数据体
            {
            	;
            }
   	   	  	break;
   	   	  	
   	   	  case 4:             //设置路由工作模式
   	   	  	memcpy(&sendBuf[frameTail],pData,3);
   	   	  	frameTail+=3;   	   	  	
   	   	  	break;
   	   	  	
   	   	  case 5:             //激活载波从节点主动注册
   	   	  	memcpy(&sendBuf[frameTail],pData,10);
   	   	  	frameTail += 10;
   	   	  	break;
   	   }
   	   break;
   	   
   	 case ROUTE_DATA_FORWARD_3762:  //路由数据转发
   	 	 switch(fn)
   	 	 {
   	 	 	 case 0x1:   //监控载波从节点(点抄)
           //规约类型
           sendBuf[frameTail++] = *pData;
           
           //从节点数量
           sendBuf[frameTail++] = 0x00;
           
           //抄表报文长度及报文
           memcpy(&sendBuf[frameTail], pData+1, *(pData+1)+1);
           frameTail += (*(pData+1)+1);
           
           //预计应答字节数,2014-01-09,add
           sendBuf[6] = expectBytes(pData+2);
           break;
       }
       break;
     
     case ROUTE_DATA_READ_3762:    //路由数据抄读
     	 switch(fn)
     	 {
     	 	 case 0x01:  //路由请求抄读内容
     	 	 	 //抄读标志
     	 	 	 sendBuf[frameTail++] = *pData;
     	 	 	 
     	 	 	 //数据长度L、数据内容及载波从节点附属节点数量n
           memcpy(&sendBuf[frameTail], pData+1, *(pData+1)+2);
           frameTail += (*(pData+1)+2);

           //预计应答字节数,2014-01-09,add
           sendBuf[6] = expectBytes(pData+2);
     	 	 	 break;
     	 }
     	 break;
       
     case DEBUG_3762:              //内部调试
     	 switch(fn)
     	 {
     	 	  case 0x1:
     	 	  	sendBuf[frameTail++] = 0x01;
     	 	  	break;
     	 }
     	 break;
     
     case RL_EXTEND_3762:         //RL扩展命令
     	 switch(fn)
     	 {
     	 	 case 0x1:
           memcpy(&sendBuf[frameTail], pData, 8);
           frameTail += 8;
     	 	 	 break;

     	 	 case 0x2:
           memcpy(&sendBuf[frameTail], pData, 4);
           frameTail += 4;
     	 	 	 break;
     	 }
     	 break;
     
     case FC_QUERY_DATA_3762:    //友迅达扩展查询数据
     	 switch(fn)
     	 {
     	 	 case 10:    //单帧读取DAU信息
   	 	   	 sendBuf[frameTail++] = 0x03;    //下载节点
   	 	   	 memcpy(&sendBuf[frameTail],pData, 2);
   	 	   	 frameTail += 2;
   	 	   	 sendBuf[frameTail++] = 0x01;    //本次查询数量
     	 	 	 break;
     	 }
     	 break;
     	 
     case FC_NET_CMD_3762:       //友迅达扩展网络命令
     	 switch(fn)
     	 {
     	 	 case 11:    //下载DAU地址
   	 	   	 sendBuf[frameTail++] = 0x01;    //增加节点的总数m(2Bytes)
   	 	   	 sendBuf[frameTail++] = 0x00;    //
   	 	     sendBuf[frameTail++] = 0x01;    //本帧传输的节点数n
   	   	   memcpy(&sendBuf[frameTail], address, 6);
   	   	   frameTail+=6;
     	 	 	 break;
     	 }
     	 break;
   }
   
   checkSum = 0;
   for(i=3;i<frameTail;i++)
   {
   	  checkSum += sendBuf[i];
   }
   sendBuf[frameTail++] = checkSum;
   sendBuf[frameTail++] = GDW376_2_EOP;  //结束字符
   sendBuf[1]  = frameTail;              //长度L 1
   sendBuf[2]  = frameTail>>8;           //长度L 2
   
   //发送
   send(PORT_POWER_CARRIER, sendBuf, frameTail);
   
   return 0;
}

/*******************************************************
函数名称:calcDt
功能描述:计算数据单元标识
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:
*******************************************************/
INT8U calcDt(INT8U *dt)
{
   INT8U tmpData,ret;
   
   tmpData = *dt;
   ret = 0;
   while (ret<8)
   {    
     ret++;
     if ((tmpData&1) == 1)
     {
       break;
     }
     tmpData >>= 1;
   }
   
   return ret+(*(dt+1))*8;   
}

/*******************************************************
函数名称:gdw3762Receiving
功能描述:国家电网企业标准Q/GDW376.2数据帧接收
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:
*******************************************************/
INT8S gdw3762Receiving(INT8U *pData,INT8U *recvLen)
{
   INT16U i;
   INT8U  checkSum;
   char   str[5];
   INT8S  ret = RECV_DATA_UNKNOWN;
   INT8U  phase;
   INT8U  tmpLen;

   for(i=0;i<*recvLen;i++)
   {
   	  gdw3762RecvBuf[recvFrameTail++] = pData[i];
   	  
   	  if (recvFrameTail==1)
   	  {
   	  	 if (gdw3762RecvBuf[0]!=GDW376_2_SOP)
   	  	 {
   	  	 	  recvFrameTail = 0;
   	  	    gdw3762RecvBuf[1] = 0xff;
   	  	    gdw3762RecvBuf[2] = 0xff;
            printf("gdw3762Receiving:未发现起始字符\n");
   	  	    continue;
   	  	 }
   	  }
   	  
   	  if (recvFrameTail==3)
   	  {
   	  	 if ((gdw3762RecvBuf[1]|(gdw3762RecvBuf[2]<<8))>500)
   	  	 {
   	  	 	  recvFrameTail = 0;
   	  	    gdw3762RecvBuf[1] = 0xff;
   	  	    gdw3762RecvBuf[2] = 0xff;
   	  	    
            printf("gdw3762Receiving:接收长度>500\n");
   	  	    continue;
   	  	 }
   	  }
   	  
   	  if (recvFrameTail>3)
   	  {
   	  	 //接收到指定长度的一帧
   	  	 if (recvFrameTail==(gdw3762RecvBuf[1]|(gdw3762RecvBuf[2]<<8)))
   	  	 {
           recvFrame.sop       = gdw3762RecvBuf[0];
           recvFrame.l         = gdw3762RecvBuf[1] | gdw3762RecvBuf[2]<<8;
           recvFrame.c         = gdw3762RecvBuf[3];
           recvFrame.pUserData = gdw3762RecvBuf+4;
           memcpy(recvFrame.r,recvFrame.pUserData,6);
           recvFrame.cs        = gdw3762RecvBuf[recvFrame.l-2];
           recvFrame.eop       = gdw3762RecvBuf[recvFrame.l-1];   	  	 	 
   	  	 	 
   	  	 	 if (recvFrame.eop!=0x16)
   	  	 	 {
   	  	 	 	  printf("lib(Local Module Rx):帧尾错误!\n");
   	  	 	 	  ret = RECV_TAIL_ERROR_3762;
   	  	 	    recvFrameTail = 0;
   	  	      gdw3762RecvBuf[1] = 0xff;
   	  	      gdw3762RecvBuf[2] = 0xff;
   	  	 	 	  break;
   	  	 	 }
   	  	 	 
   	  	 	 //计算校验和
   	  	 	 checkSum = 0;
   	  	 	 for(i=3;i<recvFrame.l-2;i++)
   	  	 	 {
   	  	 	 	  checkSum += gdw3762RecvBuf[i];
   	  	 	 }
   	  	 	 if (recvFrame.cs!=checkSum)
   	  	 	 {
   	  	 	 	  printf("gdw3762Receiving:帧校验和错误!\n");
   	  	 	 	  ret = RECV_CHECKSUM_ERROR_3762;
   	  	 	    
   	  	 	    recvFrameTail = 0;
   	  	      gdw3762RecvBuf[1] = 0xff;
   	  	      gdw3762RecvBuf[2] = 0xff;
   	  	 	 	  break;
   	  	 	 }
   	  	 	 
   	       
   	  	 	 if ((recvFrame.c&0x3f)==1 || (recvFrame.c&0x3f)==0x0a)  //通信方式为1或10(微功率无线)时的处理
   	  	 	 {
   	         if (recvFrame.afn==NULL)
   	         {
   	            printf("gdw3762Receiving:recvFrame.afn空指针,清除接收\n");
   	  	        
   	  	        recvFrameTail = 0;
   	  	        gdw3762RecvBuf[1] = 0xff;
   	  	        gdw3762RecvBuf[2] = 0xff;

   	            return RECV_DATA_UNKNOWN;
   	         }

   	  	 	   ret = RECV_DATA_CORRECT;
   	  	 	   
   	  	 	   //通信模块标识为1,有12个字节的地址
   	  	 	   if (recvFrame.r[0]&0x04)
   	  	 	   {
   	  	 	 	    memcpy(recvFrame.a,recvFrame.pUserData+6,12);
   	  	 	 	    *recvFrame.afn = *(recvFrame.pUserData+6+12);
   	  	 	 	    memcpy(recvFrame.dt,recvFrame.pUserData+6+12+1,2);
   	  	 	 	    recvFrame.pLoadData = recvFrame.pUserData+6+12+1+2;
   	  	 	   }
   	  	 	   else
   	  	 	   {
   	  	 	 	    *recvFrame.afn = *(recvFrame.pUserData+6);
   	  	 	 	    memcpy(recvFrame.dt,recvFrame.pUserData+6+1,2);
   	  	 	 	    recvFrame.pLoadData = recvFrame.pUserData+6+1+2;
   	  	 	   }
   	         
   	  	 	   *recvFrame.fn = calcDt(recvFrame.dt);
   	  	 	   phase = *(recvFrame.pUserData+1);

   	  	 	   //printf("Local AFN=%02X,fn=%d\n",*recvFrame.afn,*recvFrame.fn);
   	  	 	 
     	  	 	 switch(*recvFrame.afn)
     	  	 	 {
     	  	 	 	  case ACK_OR_NACK_3762:   //确认或否认
     	  	 	 	  	switch(*recvFrame.fn)
     	  	 	 	  	{
     	  	 	 	  		 case 1:   //确认
     	  	 	 	  		 	 *pData     = *recvFrame.pLoadData;     //信道状态1
     	  	 	 	  		 	 *(pData+1) = *(recvFrame.pLoadData+1); //信道状态2
     	  	 	 	  		 	 *(pData+2) = *(recvFrame.pLoadData+2); //等待时间1
     	  	 	 	  		 	 *(pData+3) = *(recvFrame.pLoadData+3); //等待时间2
     	  	 	 	  		 	 break;
     	  	 	 	  		 	 
     	  	 	 	  		 case 2:   //否认
     	  	 	 	  		 	 *pData = *recvFrame.pLoadData;         //错误状态字
     	  	 	 	  		 	 break;
     	  	 	 	  	}
     	  	 	 	  	break;
     	  	 	 	  	
                case DATA_FORWARD_3762:
     	  	 	 	  	switch(*recvFrame.fn)
     	  	 	 	  	{
     	  	 	 	  		 case 1:    //转发应答命令
     	  	 	 	  		 	 memcpy(pData,recvFrame.pLoadData,2+*(recvFrame.pLoadData+1));
     	  	 	 	  		 	 break;
     	  	 	 	    }
                	break;
     	  	 	 	  	
     	  	 	 	  case QUERY_DATA_3762:    //查询数据
     	  	 	 	  	switch(*recvFrame.fn)
     	  	 	 	  	{
     	  	 	 	  		 case 1:   //厂商代码和版本信息
     	  	 	 	  		 	 memcpy(pData, recvFrame.pLoadData, 9);
     	  	 	 	  		 	 
     	  	 	 	  		 	 //福星晓程载波模块
     	  	 	 	  		 	 if (*recvFrame.pLoadData=='X' && *(recvFrame.pLoadData+1)=='C')
     	  	 	 	  		 	 {
     	  	 	 	  		 	 	  printf("电科院载波模块\n");
     	  	 	 	  		 	 	  *carrierModule = CEPRI_CARRIER;
     	  	 	 	  		 	 	  
     	  	 	 	  		 	 	  if (recvFrame.pLoadData[6]>=0x11)
     	  	 	 	  		 	 	  {
     	  	 	 	  		 	 	  	lcModuleTypex = CEPRI_CARRIER_3_CHIP;
     	  	 	 	  		 	 	  	printf("新版电科院载波模块\n");
     	  	 	 	  		 	 	  }
     	  	 	 	  		 	 }
     	  	 	 	  		 	 
     	  	 	 	  		 	 if (
     	  	 	 	  		 	 	   (*recvFrame.pLoadData=='S' && *(recvFrame.pLoadData+1)=='E')
     	  	 	 	  		 	 	   || (*recvFrame.pLoadData=='L' && *(recvFrame.pLoadData+1)=='S')  //12-11-8
     	  	 	 	  		 	 	  )
     	  	 	 	  		 	 	
     	  	 	 	  		 	 {  
     	  	 	 	  		 	 	  printf("东软载波模块\n");
     	  	 	 	  		 	 	  *carrierModule = EAST_SOFT_CARRIER;
     	  	 	 	  		 	 }
     	  	 	 	  		 	 
     	  	 	 	  		 	 if (*recvFrame.pLoadData=='W' && *(recvFrame.pLoadData+1)=='D' && *(recvFrame.pLoadData+2)=='W' && *(recvFrame.pLoadData+3)=='L')
     	  	 	 	  		 	 {
     	  	 	 	  		 	 	  printf("华伟沃电无线模块\n");
     	  	 	 	  		 	 	  *carrierModule = HWWD_WIRELESS;
     	  	 	 	  		 	 }
     	  	 	 	  		 	 if (*recvFrame.pLoadData=='S' && *(recvFrame.pLoadData+1)=='R')
     	  	 	 	  		 	 {
     	  	 	 	  		 	 	  printf("SR无线模块\n");
     	  	 	 	  		 	 	  *carrierModule = SR_WIRELESS;
     	  	 	 	  		 	 }
     	  	 	 	  		 	 if (*recvFrame.pLoadData=='R' && *(recvFrame.pLoadData+1)=='L')
     	  	 	 	  		 	 {
     	  	 	 	  		 	 	  printf("锐拔无线模块\n");
     	  	 	 	  		 	 	  *carrierModule = RL_WIRELESS;
     	  	 	 	  		 	 }
     	  	 	 	  		 	 if (*recvFrame.pLoadData=='i' && *(recvFrame.pLoadData+1)=='m')
     	  	 	 	  		 	 {
     	  	 	 	  		 	 	  printf("弥亚微载波模块\n");
     	  	 	 	  		 	 	  *carrierModule = MIA_CARRIER;
     	  	 	 	  		 	 }
     	  	 	 	  		 	 if (*recvFrame.pLoadData=='C' && *(recvFrame.pLoadData+1)=='T')
     	  	 	 	  		 	 {
     	  	 	 	  		 	 	  printf("鼎信载波模块\n");
     	  	 	 	  		 	 	  *carrierModule = TC_CARRIER;
     	  	 	 	  		 	 }
     	  	 	 	  		 	 if (*recvFrame.pLoadData=='L' && *(recvFrame.pLoadData+1)=='M')
     	  	 	 	  		 	 {
     	  	 	 	  		 	 	  printf("力合微载波模块\n");
     	  	 	 	  		 	 	  *carrierModule = LME_CARRIER;
     	  	 	 	  		 	 }
     	  	 	 	  		 	 if (*recvFrame.pLoadData=='F' && *(recvFrame.pLoadData+1)=='C')
     	  	 	 	  		 	 {
     	  	 	 	  		 	 	  printf("友迅达无线模块\n");
     	  	 	 	  		 	 	  *carrierModule = FC_WIRELESS;
     	  	 	 	  		 	 }
     	  	 	 	  		 	 if (*recvFrame.pLoadData=='S' && *(recvFrame.pLoadData+1)=='C')
     	  	 	 	  		 	 {
     	  	 	 	  		 	 	  printf("赛康无线模块\n");
     	  	 	 	  		 	 	  *carrierModule = SC_WIRELESS;
     	  	 	 	  		 	 }
     	  	 	 	  		 	 break;
     	  	 	 	  		 	 
     	  	 	 	  		 case 4:
     	  	 	 	  		 	 memcpy(mainNodeAddr, recvFrame.pLoadData, 6);
     	  	 	 	  		 	 if (*carrierModule==SC_WIRELESS)
     	  	 	 	  		 	 {
     	  	 	 	  		 	   memcpy(scMainNodeAddr, mainNodeAddr, 6);
     	  	 	 	  		 	 }

     	  	 	 	  		 	 break;
     	  	 	 	  	}
     	  	 	 	  	break;
     	  	 	 	  	
     	  	 	 	  case ACTIVE_REPORT_3762:
     	  	 	 	  	switch(*recvFrame.fn)
     	  	 	 	  	{
     	  	 	 	  		case 1:   //上报载波从节点信息
     	  	 	 	  		  memcpy(pData,recvFrame.pLoadData,1+9*(*recvFrame.pLoadData));
     	  	 	 	  		 	break;
     	  	 	 	  		 
     	  	 	 	  		case 2:   //上报抄读内容
     	  	 	 	  			tmpLen = 4+*(recvFrame.pLoadData+3);
     	  	 	 	  		  memcpy(pData, recvFrame.pLoadData, tmpLen);
     	  	 	 	  		  *(pData+tmpLen) = phase;
     	  	 	 	  		 	break;
     	  	 	 	  	}
     	  	 	 	  	break;
     	  	 	 	  	
     	  	 	 	  case ROUTE_QUERY_3762:   //路由查询
     	  	 	 	  	switch(*recvFrame.fn)
     	  	 	 	  	{
     	  	 	 	  		 case 1:    //从节点数量
                       if (*carrierModule==FC_WIRELESS)
     	  	 	 	  		 	 {
     	  	 	 	  		 	   memcpy(pData, recvFrame.pLoadData, 10);
     	  	 	 	  		 	 }
     	  	 	 	  		 	 else
     	  	 	 	  		 	 {
     	  	 	 	  		 	   memcpy(pData, recvFrame.pLoadData, 4);
     	  	 	 	  		 	 }
     	  	 	 	  		 	 break;
     	  	 	 	  		 	 
     	  	 	 	  		 case 2:    //载波从节点信息
     	  	 	 	  		 	 memcpy(pData, recvFrame.pLoadData, 3+8*(*(recvFrame.pLoadData+2)));
     	  	 	 	  		 	 break;
     	  	 	 	  		 	 
     	  	 	 	  		 case 4:    //路由运行状态
     	  	 	 	  		 	 memcpy(pData, recvFrame.pLoadData, 16);
     	  	 	 	  		 	 break;
     	  	 	 	  		 	 
     	  	 	 	  		 case 6:    //主动注册的载波从节点信息
     	  	 	 	  		 	 memcpy(pData, recvFrame.pLoadData, *(recvFrame.pLoadData+2)*8+3);
     	  	 	 	  		 	 break;
     	  	 	 	  	}
     	  	 	 	  	break;
     	  	 	 	  	
     	  	 	 	  case ROUTE_DATA_FORWARD_3762:  //路由数据转发
     	  	 	 	  	switch(*recvFrame.fn)
     	  	 	 	  	{
     	  	 	 	  		 case 1:    //监控载波从节点
     	  	 	 	  		 	 memcpy(pData,recvFrame.pLoadData,2+*(recvFrame.pLoadData+1));
     	  	 	 	  		 	 break;
     	  	 	 	    }
     	  	 	 	  	break;
     	  	 	 	  
     	  	 	 	  case ROUTE_DATA_READ_3762:     //路由数据抄读
     	  	 	 	  	switch(*recvFrame.fn)
     	  	 	 	    {
     	  	 	 	    	 case 1:    //路由请求抄读内容
     	  	 	 	    	 	 memcpy(pData, recvFrame.pLoadData, 9);
     	  	 	 	    	 	 break;
     	  	 	 	    }
     	  	 	 	  	break;
                
                case RL_EXTEND_3762:           //RL扩展命令
                	switch(*recvFrame.fn)
                  {
                  	case 4:    //读未组网节点号
     	  	 	 	  		 	memcpy(pData, recvFrame.pLoadData, 2+(*(recvFrame.pLoadData+1)<<8 | *(recvFrame.pLoadData+0))*6);
                  		break;
                  		
                  	case 6:    //读网络状态
     	  	 	 	  		 	memcpy(pData, recvFrame.pLoadData, 12);
                  	 	break;
                  }
                	break;

     	  	 	 	  case DEBUG_3762:               //WD/SR内部调试
     	  	 	 	  	switch(*recvFrame.fn)
     	  	 	 	  	{
     	  	 	 	  		 case 1:    //主动上报功能控制
     	  	 	 	  		 	 *pData = *recvFrame.pLoadData;
     	  	 	 	  		 	 break;
     	  	 	 	  		 
     	  	 	 	  		 case 3:    //上报建网成功
     	  	 	 	  		 	 memcpy(pData, recvFrame.pLoadData, 20);
     	  	 	 	  		 	 break;
     	  	 	 	  		 
     	  	 	 	  		 case 4:    //上报入网节点
     	  	 	 	  		 case 5:    //上报离网节点
     	  	 	 	  		 	 memcpy(pData, recvFrame.pLoadData, 20);
     	  	 	 	  		 	 break;
     	  	 	 	  	}
     	  	 	 	  	break;
     	  	 	 	  
     	  	 	 	  case FC_QUERY_DATA_3762:  //友迅达扩展查询数据
     	  	 	 	  	switch(*recvFrame.fn)
     	  	 	 	  	{
     	  	 	 	  		 case 2:   //CAC 状态信息
     	  	 	 	  		 	 memcpy(pData, recvFrame.pLoadData, 7);
     	  	 	 	  		 	 break;
     	  	 	 	  		 
     	  	 	 	  		 case 10:  //单帧读取DAU信息
     	  	 	 	  		 	 memcpy(pData, recvFrame.pLoadData, 4+ *(recvFrame.pLoadData+3)*7);
     	  	 	 	  		 	 break;
     	  	 	 	  	}
     	  	 	 	  	break;
     	  	 	 }
   	  	 	 }
   	  	 	 else
   	  	 	 {
   	  	 	 	  printf("通信为7\n");
   	  	 	 }
   	  	 	  
     	  	 *recvLen = i+2;
   	  	 	 
   	  	 	 //处理完成后复位标志
   	  	 	 recvFrameTail = 0;
   	  	 	 gdw3762RecvBuf[1] = 0xff;
   	  	 	 gdw3762RecvBuf[2] = 0xff;
   	  	 	 
   	  	 	 break;
   	  	 }
   	  	 else
   	  	 {
   	  	 	  ret = RECV_NOT_COMPLETE_3762;
   	  	 }
   	  }
   	  
   	  if (recvFrameTail>=510)
   	  {
   	  	 recvFrameTail = 0;
   	  	 gdw3762RecvBuf[1] = 0xff;
   	  	 gdw3762RecvBuf[2] = 0xff;
   	  }
   }

   return ret;
}

