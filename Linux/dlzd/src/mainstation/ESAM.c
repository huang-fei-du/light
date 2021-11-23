/***************************************************
Copyright,2011,Huawei WoDian co.,LTD,All	Rights Reserved
文件名：ESAM.c
作者：leiyong
版本：0.9
完成日期：2011年3月
描述：ESAM芯片操作文件。
函数列表：
     1.
修改历史：
  01,11-03-09,Leiyong created.
***************************************************/

#include "teRunPara.h"
#include "ioChannel.h"
#include "hardwareConfig.h"

#include "ESAM.h"

BOOL  hasEsam;             //有ESAM芯片吗?
INT8U esamSerial[8];       //ESAM芯片序列号

/*******************************************************
函数名称:resetEsam
功能描述:复位ESAM芯片
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void resetEsam(void)
{
	 INT8U  i;
	 INT32U numOfCircle;          //时钟周期个数
	 INT32U j;
	 INT8U  recvBuf[20];
	 BOOL   esamSetIoToZ = FALSE;
	 BOOL   byteStart = FALSE;
	 INT16U etuCircle = 0;
	 INT8U  numOfByte;            //接收字节位置
	 INT8U  bits;                 //字节的位数计数
	 INT8U  empty = 0;
	 INT8U  even = 0;
	 INT8U  k;
	 
	 if (ioctl(fdOfIoChannel,ESAM_DETECT,0))
   {
   	  if (debugInfo&ESAM_DEBUG)
   	  {
   	    printf("检测到ESAM安全芯片模块\n");
   	  }
   	  hasEsam = TRUE;

      for(i=0; i<3; i++)                 //循环复位3次,若均失败则退出
   	  {
   	  	memset(recvBuf, 0x00, 20);
   	  	
   	    numOfCircle = 0;
   	    ioctl(fdOfIoChannel, ESAM_CLOCK, 0);
   	    ioctl(fdOfIoChannel, ESAM_RST, 0);   //复位脚拉低
   	    esamSetIoToZ = FALSE;                //ESAM芯片将IO线置于Z状态,也就是说复位起效
        ioctl(fdOfIoChannel, ESAM_IO, 3);    //IO置为输入
   	  	while(numOfCircle<200000)
   	  	{
   	  	  //产生时钟
   	  	  if (numOfCircle%2)
   	  	  {
   	  	 	  ioctl(fdOfIoChannel,ESAM_CLOCK,1);
   	  	  }
   	  	  else
   	  	  {
   	  	 	  ioctl(fdOfIoChannel,ESAM_CLOCK,0);
   	  	  }

   	      if (esamSetIoToZ==FALSE)
   	      {
   	  	 	  if (numOfCircle<=400)
   	  	 	  {
   	  	 	     if (numOfCircle>190)
   	  	 	     {
   	  	 	       //400个时钟周期内ESAM将IO置为Z状态
   	  	 	       if (ioctl(fdOfIoChannel, ESAM_IO, 4))
   	  	 	       {
   	  	 	         ioctl(fdOfIoChannel,ESAM_RST,1);   //复位脚拉高
   	  	 	         esamSetIoToZ = TRUE;
   	  	 	         byteStart = FALSE;
   	  	 	         numOfByte = 0;
   	  	 	       }
   	  	 	     }
   	  	 	  }
   	  	 	  else
   	  	 	  {
   	  	 	  	 break;                             //复位未起作用,重新开始复位过程
   	  	 	  }
   	      }
   	      else
   	      {
   	  	 	  if (byteStart==FALSE)
   	  	 	  {
   	  	 	    if (!ioctl(fdOfIoChannel, ESAM_IO, 4))
   	  	 	    {
   	  	 	  	   byteStart = TRUE;
   	  	 	  	   etuCircle = 744;
   	  	 	  	   recvBuf[numOfByte] = 0;
   	  	 	  	   bits = 0;
   	  	 	  	   even = 0;
   	  	 	       //printf("ESAM应答,numOfCircle=%d\n",numOfCircle);
   	  	 	       continue;   	  	 	       
   	  	 	  	}
   	  	 	  	empty++;
   	  	 	  	empty++;
   	  	 	  	empty++;
   	  	 	  	empty++;
   	  	 	  	empty++;
   	  	 	  	empty++;
   	  	 	  	empty++;   	  	 	  	
   	  	 	  }
   	  	 	  else
   	  	 	  {
   	  	 	  	 if (etuCircle>0)
   	  	 	  	 {
   	  	 	  	   etuCircle--;
   	  	 	  	   empty++;
   	  	 	  	   empty++;
   	  	 	  	   empty++;
   	  	 	  	 }
   	  	 	  	 else
   	  	 	  	 {
   	  	 	  	 	 etuCircle=744;
   	  	 	  	 	 
   	  	 	  	 	 if (bits<9)
   	  	 	  	 	 {
   	  	 	  	 	 	 if (bits<8)
   	  	 	  	 	 	 {
   	  	 	  	 	 	   recvBuf[numOfByte]>>=1;
   	  	 	  	 	 	 }
   	  	 	  	 	 	 else
   	  	 	  	 	 	 {
   	  	 	  	 	 	   empty++;
   	  	 	  	 	 	 }
   	  	 	  	 	 }
   	  	 	  	 	 else
   	  	 	  	 	 {
                   if (bits==9)
                   {
                     ioctl(fdOfIoChannel, ESAM_IO, 2);    //IO置为输出
   	  	 	  	 	 	   if (even%2)
   	  	 	  	 	 	   {
   	  	 	  	 	 	     //printf("奇偶校验错误,even=%d\n", even);
                       ioctl(fdOfIoChannel, ESAM_IO, 0);  //IO输出低
   	  	 	  	 	 	   }
   	  	 	  	 	 	   else
   	  	 	  	 	 	   {
   	  	 	  	 	 	     //printf("奇偶校验位正确,第%d字节%02x,bits=%d\n", numOfByte+1, recvBuf[numOfByte], bits);
                       ioctl(fdOfIoChannel, ESAM_IO, 1);  //IO输出高
                       if (numOfByte==0)
                       {
                       	 //第一字节错误,重新开始复位过程
                       	 if (recvBuf[numOfByte]!=0x3b)
                       	 {
                       	 	  break;
                       	 }
                       }
   	  	 	  	 	 	   }
   	  	 	  	 	 	   
   	  	 	  	 	 	   etuCircle = 200;   //保护时间
   	  	 	  	 	 	 }
   	  	 	  	 	 	 else
   	  	 	  	 	 	 {
   	  	 	  	 	 	 	 //保护时间已过,开始接收第下一个字节
   	  	 	  	       numOfByte++;
   	  	 	  	 	 	 	 if (numOfByte>2)
   	  	 	  	 	 	 	 {
   	  	 	  	 	 	 	 	  if (numOfByte>4+(recvBuf[1] & 0x0F))
   	  	 	  	 	 	 	 	  {
   	  	 	  	 	 	 	 	  	 memcpy(esamSerial, &recvBuf[10], 8);
   	  	 	  	 	 	 	 	  	 
   	  	 	  	 	 	 	 	  	 if (debugInfo&ESAM_DEBUG)
   	  	 	  	 	 	 	 	  	 {
   	  	 	  	 	 	 	 	  	   printf("ESAM Serial:%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",esamSerial[0],esamSerial[1],esamSerial[2],esamSerial[3],esamSerial[4],esamSerial[5],esamSerial[6],esamSerial[7]);
   	  	 	  	 	 	 	 	  	 }
   	  	 	  	 	 	 	 	  	 i = 3;
   	  	 	  	 	 	 	 	  	 break;
   	  	 	  	 	 	 	 	  }
   	  	 	  	 	 	 	 }
   	  	 	  	 	 	 	 
   	  	 	  	       recvBuf[numOfByte] = 0;
   	  	 	  	       byteStart = FALSE;
                     ioctl(fdOfIoChannel, ESAM_IO, 3);    //IO置为输入
   	  	 	           //printf("保护时间已过,开始接收第%d个字节\n",numOfByte+1);
   	  	 	  	 	 	 }
   	  	 	  	 	 }
   	  	 	  	 	 
   	  	 	  	 	 bits++;
   	  	 	  	 }
   	  	 	  	 
   	  	 	  	 if (etuCircle==372)
   	  	 	  	 {
   	  	 	  	 	  if (bits<9)
   	  	 	  	 	  {
   	  	 	  	 	    if (ioctl(fdOfIoChannel, ESAM_IO, 4))
   	  	 	  	 	    {
   	  	 	  	 	  	   recvBuf[numOfByte] |= 0x80;
   	  	 	  	 	  	   even++;
   	  	 	  	 	    }
   	  	 	  	 	    else
   	  	 	  	 	    {
   	  	 	  	 	  	   recvBuf[numOfByte] |= 0x00;
   	  	 	  	 	    }
   	  	 	  	 	  }
   	  	 	  	 	  else
   	  	 	  	 	  {
   	  	 	  	 	    if (bits==9)
   	  	 	  	 	    {
   	  	 	  	 	      if (ioctl(fdOfIoChannel, ESAM_IO, 4))
   	  	 	  	 	      {
   	  	 	  	 	  	    even++;
   	  	 	  	 	  	    empty++;
   	  	 	  	 	      }
   	  	 	  	 	      else
   	  	 	  	 	      {
   	  	 	  	 	  	    empty++;
   	  	 	  	 	  	    empty++;
   	  	 	  	        }
   	  	 	  	 	    }
   	  	 	  	 	    else
   	  	 	  	 	    {
   	  	 	  	 	    	empty++;
   	  	 	  	 	    	empty++;
   	  	 	  	 	    	empty++;
   	  	 	  	 	    }
   	  	 	  	 	  } 
   	  	 	  	 }
   	  	 	  	 else
   	  	 	  	 {
   	  	 	  	 	  empty++;
   	  	 	  	 	  empty++;
   	  	 	  	 	  empty++;
   	  	 	  	 }
   	  	 	  }
   	      }
   	      
   	      for(k=0;k<150;k++)
   	      {
   	        empty++;
   	      }
   	    
   	  	  numOfCircle++;
   	    }
   	    
   	    ioctl(fdOfIoChannel, ESAM_RST, 1);   //复位脚拉高
   	    ioctl(fdOfIoChannel, ESAM_CLOCK, 0);
   	  }
   }
   else
   {
   	 hasEsam = FALSE;
   	 
   	 if (debugInfo&ESAM_DEBUG)
   	 {
   	   printf("未检测到ESAM安全芯片模块\n");
   	 }
   }
}

/*******************************************************
函数名称:putGetBytes
功能描述:发送命令且接收回复数据
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
INT16U putGetBytes(INT8U *cmdBuf, INT8U lenOfCmd, INT8U respondType, INT8U *recvBuf,INT8U lenOfRecv)
{
	 BOOL   status      = 0;           //发送接收状态(0-发送,1-接收)	 
	 INT32U numOfCircle = 0;           //时钟周期个数
	 INT8U  numOfByte   = 0;           //发送/接收字节位置
	 INT16U etuCircle   = 0;
	 INT8U  bits;                      //字节的位数计数
	 INT8U  empty = 0;
	 INT8U  even  = 0;
	 INT8U  nowByte;                   //当前要发送的字节
	 BOOL   byteStart = FALSE;         //发送/接收字节开始标志
	 BOOL   parityError = FALSE;       //奇偶校验错误
	 INT8U  k;

   ioctl(fdOfIoChannel, ESAM_IO, 2); //IO置为输出
   ioctl(fdOfIoChannel, ESAM_IO, 1); //IO置为输出高
   numOfCircle = 0;
   while(numOfCircle<(lenOfCmd+lenOfRecv+5)*11*744)
   {
   	 //产生时钟
   	 if (numOfCircle%2)
   	 {
   	   ioctl(fdOfIoChannel, ESAM_CLOCK, 1);
   	 }
   	 else
   	 {
   	   ioctl(fdOfIoChannel, ESAM_CLOCK, 0);
   	 }
     
     //发送状态
     if (status==0)
     {
 	 	   if (byteStart==FALSE)
 	 	   {
 	 	     if (numOfByte>=lenOfCmd)
 	 	     {
 	 	     	 status = 1;
   	  	 	 
   	  	 	 byteStart = FALSE;
   	  	 	 numOfByte = 0;
 	 	     }
 	 	     else
 	 	     {
 	 	       ioctl(fdOfIoChannel, ESAM_IO, 2);    //IO置为输出
       	   ioctl(fdOfIoChannel, ESAM_IO, 0);    //IO置为输出低,开始位置为0
       	 
       	   nowByte = cmdBuf[numOfByte];
 	 	  	   byteStart = TRUE;
 	 	  	   etuCircle = 744;
 	 	  	   bits = 0;
 	 	  	   even = 0;
 	 	  	   parityError = FALSE;
 	 	       printf("向ESAM发送第%d个命令字%02x,numOfCircle=%d\n", numOfByte+1, cmdBuf[numOfByte], numOfCircle);
 	 	     }
 	 	   }
 	 	   else
 	 	   {
  	     if (etuCircle==744)
  	     {
           if (bits==0)
           {
     	        //empty++;
     	        //empty++;
     	        //empty++;
     	        //empty>>=1;
           }
           else
           {
             if (bits<9)
             {
               if((nowByte&0x01)==0x01)
               {
     	           ioctl(fdOfIoChannel, ESAM_IO, 1);    //IO置为输出高
     	           //printf("第%d位输出高\n",bits);
                 even++;
               }
               else
               {
     	           ioctl(fdOfIoChannel, ESAM_IO, 0);    //IO置为输出低
     	           //printf("第%d位输出低\n",bits);
     	           empty++;
               }
               nowByte>>=1;
             }
             else
             {
               if (bits==9)
               {
                 if(even%2)
                 {
     	             ioctl(fdOfIoChannel, ESAM_IO, 1);    //奇偶校验位高
     	             //printf("校验位输出高\n");
                 }
                 else
                 {
     	             ioctl(fdOfIoChannel, ESAM_IO, 0);    //奇偶校验位低
     	             //printf("校验位输出低\n");
                 }             	 
     	           empty++;
     	           empty>>=1;
     	         }
     	         else
     	         {
     	         	 empty++;
     	         	 empty++;
     	           empty>>=1;
     	         }
     	       }
           }
  	     }
  	     else
  	     {
     	     //empty++;
     	     //empty++;
     	     //empty++;
     	     //empty>>=1;
  	     }
 	  	   
 	  	   if (etuCircle>0)
 	  	   {
 	  	     etuCircle--;
 	  	     
 	  	     if (etuCircle==500 && bits==10)
 	  	     {
 	  	     	 if (!ioctl(fdOfIoChannel, ESAM_IO, 4))
 	  	     	 {
 	  	     	 	  printf("发送指令奇偶校验错误\n");
 	  	     	 	  //parityError = TRUE;
 	  	     	 }
 	  	     	 else
 	  	     	 {
 	  	     	 	  //printf("发送指令奇偶校验正确\n");
 	  	     	 }
 	  	     }
 	  	   }
 	  	   else
 	  	   {
 	 	  	 	 etuCircle=744;
           
           if (bits==9)
           {
             ioctl(fdOfIoChannel, ESAM_IO, 3);    //IO置为输入
             etuCircle=744;
 	  	 	 	 }
 	  	 	 	 else
 	  	 	 	 {
 	  	 	 	 	 //empty++;
 	  	 	 	 	 //empty++;
 	  	 	 	 }
 	  	 	 	 
 	  	 	 	 if (bits==10)
 	  	 	 	 {
 	  	 	 	 	 //if (parityError==FALSE)
 	  	 	 	 	 //{
 	  	 	 	 	    numOfByte++;
 	 	  	        byteStart = FALSE;
 	  	 	 	 	    continue;
 	  	 	 	 	 //}
 	  	 	 	 	 //else
 	  	 	 	 	 //{
 	 	           //ioctl(fdOfIoChannel, ESAM_IO, 2);    //IO置为输出
       	       //ioctl(fdOfIoChannel, ESAM_IO, 1);    //IO置为输出高
 	  	 	 	 	 //}
 	  	 	 	 }
 	  	 	 	 else
 	  	 	 	 {
 	  	 	 	 	 //empty++;
 	  	 	 	 	 //empty++;
 	  	 	 	 }
 	  	 	 	 
 	  	 	 	 if (bits==11)
 	  	 	 	 {
 	  	 	 	 	  numOfByte++;
 	  	 	 	 	  byteStart = FALSE;
 	  	 	 	 }
 	 	  	 	 
 	 	  	 	 bits++;
 	  	   }
       }
     }
     else
     {
  	 	  if (byteStart==FALSE)
  	 	  {
  	 	    if (lenOfRecv<1)
  	 	    {
  	 	    	 break;
  	 	    }
  	 	    if (!ioctl(fdOfIoChannel, ESAM_IO, 4))
  	 	    {
  	 	  	   byteStart = TRUE;
  	 	  	   etuCircle = 744;
  	 	  	   recvBuf[numOfByte] = 0;
  	 	  	   bits = 0;
  	 	  	   even = 0;
  	 	       //printf("ESAM开始应答,numOfCircle=%d\n",numOfCircle);
  	 	       continue;   	  	 	       
  	 	  	}
  	 	  	empty++;
  	 	  	empty++;
  	 	  	empty++;
  	 	  	empty++;
  	 	  	empty++;
  	 	  	empty++;
  	 	  	empty++;   	  	 	  	
  	 	  }
  	 	  else
  	 	  {
  	 	  	 if (etuCircle>0)
  	 	  	 {
  	 	  	   etuCircle--;
  	 	  	   empty++;
  	 	  	   empty++;
  	 	  	   empty++;
  	 	  	 }
  	 	  	 else
  	 	  	 {
  	 	  	 	 etuCircle=744;
  	 	  	 	 
  	 	  	 	 if (bits<9)
  	 	  	 	 {
  	 	  	 	 	 if (bits<8)
  	 	  	 	 	 {
  	 	  	 	 	   recvBuf[numOfByte]>>=1;
  	 	  	 	 	 }
  	 	  	 	 	 else
  	 	  	 	 	 {
  	 	  	 	 	   empty++;
  	 	  	 	 	 }
  	 	  	 	 }
  	 	  	 	 else
  	 	  	 	 {
               if (bits==9)
               {
                 ioctl(fdOfIoChannel, ESAM_IO, 2);    //IO置为输出
  	 	  	 	 	   if (even%2)
  	 	  	 	 	   {
  	 	  	 	 	     printf("奇偶校验错误,even=%d\n", even);
                   ioctl(fdOfIoChannel, ESAM_IO, 0);  //IO输出低
  	 	  	 	 	   }
  	 	  	 	 	   else
  	 	  	 	 	   {
  	 	  	 	 	     printf("奇偶校验位正确,第%d字节%02x,bits=%d\n", numOfByte+1, recvBuf[numOfByte], bits);
                   ioctl(fdOfIoChannel, ESAM_IO, 1);  //IO输出高
  	 	  	 	 	   }
  	 	  	 	 	   
  	 	  	 	 	   etuCircle = 200;   //保护时间
  	 	  	 	 	 }
  	 	  	 	 	 else
  	 	  	 	 	 {
  	 	  	 	 	 	 //保护时间已过,开始接收第下一个字节
  	 	  	       numOfByte++;
  	 	  	 	 	 	 if (numOfByte>lenOfRecv-1)
  	 	  	 	 	 	 {
  	 	  	 	 	 	 	  break;
  	 	  	 	 	 	 }
  	 	  	 	 	 	 
  	 	  	       recvBuf[numOfByte] = 0;
  	 	  	       byteStart = FALSE;
                 ioctl(fdOfIoChannel, ESAM_IO, 3);    //IO置为输入
  	 	           //printf("保护时间已过,开始接收第%d个字节\n",numOfByte+1);
  	 	  	 	 	 }
  	 	  	 	 }
  	 	  	 	 
  	 	  	 	 bits++;
  	 	  	 }
  	 	  	 
  	 	  	 if (etuCircle==372)
  	 	  	 {
  	 	  	 	  if (bits<9)
  	 	  	 	  {
  	 	  	 	    if (ioctl(fdOfIoChannel, ESAM_IO, 4))
  	 	  	 	    {
  	 	  	 	  	   recvBuf[numOfByte] |= 0x80;
  	 	  	 	  	   even++;
  	 	  	 	    }
  	 	  	 	    else
  	 	  	 	    {
  	 	  	 	  	   recvBuf[numOfByte] |= 0x00;
  	 	  	 	    }
  	 	  	 	  }
  	 	  	 	  else
  	 	  	 	  {
  	 	  	 	    if (bits==9)
  	 	  	 	    {
  	 	  	 	      if (ioctl(fdOfIoChannel, ESAM_IO, 4))
  	 	  	 	      {
  	 	  	 	  	    even++;
  	 	  	 	  	    empty++;
  	 	  	          //printf("奇偶校验位=1\n");
  	 	  	 	      }
  	 	  	 	      else
  	 	  	 	      {
  	 	  	 	  	    empty++;
  	 	  	 	  	    empty++;
  	 	  	          //printf("奇偶校验位=0\n");
  	 	  	        }
  	 	  	 	    }
  	 	  	 	    else
  	 	  	 	    {
  	 	  	 	    	empty++;
  	 	  	 	    	empty++;
  	 	  	 	    	empty++;
  	 	  	 	    }
  	 	  	 	  } 
  	 	  	 }
  	 	  	 else
  	 	  	 {
  	 	  	 	  empty++;
  	 	  	 	  empty++;
  	 	  	 	  empty++;
  	 	  	 }
   	    
   	       for(k=0;k<50;k++)
   	       {
   	         empty++;
   	       }
   	    }
     }
   	  
   	 numOfCircle++;
   }
   
   ioctl(fdOfIoChannel, ESAM_CLOCK, 0);
   
   return 1;
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
	INT8U cmdBuf[5], recvBuf[20];
	INT8U i;

  for(i=0;i<10;i++)
  {
    //cmdBuf[0] = 0x00;
    //cmdBuf[1] = 0xC0;
    //cmdBuf[2] = 0x00;
    //cmdBuf[3] = 0x00;
    //cmdBuf[4] = 0x08;
    //putGetBytes(cmdBuf, 5, 0, recvBuf, 2);

    cmdBuf[0] = 0x00;
    cmdBuf[1] = 0x84;
    cmdBuf[2] = 0x00;
    cmdBuf[3] = 0x00;
    cmdBuf[4] = 0x08;
    putGetBytes(cmdBuf, 5, 0, recvBuf, 11);
    //putGetBytes(cmdBuf, 5, 0, recvBuf, 0);
    
    if (recvBuf[0]==0x84 && recvBuf[9]==0x90 && recvBuf[10]==0x00)
    {
    	 if (debugInfo&ESAM_DEBUG)
    	 {
    	   printf("终端随机数:%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",recvBuf[1],recvBuf[2],recvBuf[3],recvBuf[4],recvBuf[5],recvBuf[6],recvBuf[7],recvBuf[8]);
    	 }
    	 break;
    }
    
    usleep(500000);
  }
  
  if (i<10)
  {
  	for(i=0;i<8;i++)
    {
      *buf = recvBuf[8-i];
      buf++;
    }
  }
  else
  {
  	if (debugInfo&ESAM_DEBUG)
  	{
    	 printf("读取终端随机数失败\n");
  	}
  }
}
