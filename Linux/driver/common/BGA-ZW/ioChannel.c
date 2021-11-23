/***************************************************
Copyright,2009,Huawei WoDian co.,LTD,All Rights Reserved
文件名:ioChannel.c
作者:leiyong
版本:0.9
完成日期:2009年12月
描述:电力终端(负控终端/集中器,AT91SAM9260处理器)I/O通道驱动文件
     I/O通道包括:GPRS开机信号、键盘、YX、ADC

函数列表:

修改历史:
  01,09-12-20,Leiyong created.
***************************************************/

#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>
#include <linux/init.h>         /* Needed for the module-macros */
#include <linux/fs.h>
#include <asm/arch/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <../arch/arm/mach-at91rm9200/clock.h>

#include <asm/arch/at91_pio.h>
#include <asm/arch/at91_aic.h>
#include <asm/arch/at91_pmc.h>


#include "ioChannel.h"

//TC模式寄存器(CMR - Timer Counter Channel Mode Register Bits Definition)
#define TC_CLKS_MCK2          0x0
#define TC_EEVT_XC0           0x400
#define TC_CPCTRG             0x4000
#define TC_WAVE               0x8000
#define TC_ACPA_TOGGLE_OUTPUT 0x30000
#define TC_ACPC_TOGGLE_OUTPUT 0xC0000
#define TC_ASWTRG_SET_OUTPUT  0x400000
#define TC_BCPB_TOGGLE_OUTPUT 0x3000000
#define TC_BCPC_TOGGLE_OUTPUT 0xC000000
#define TC_BSWTRG_SET_OUTPUT  0x40000000

//TC控制寄存器(CCR - Timer Counter Control Register Bits Definition)
#define TC_CLKEN              0x1
#define TC_CLKDIS             0x2
#define TC_SWTRG              0x4

//TC寄存器偏移
#define AT91_TC_CCR           0x00
#define AT91_TC_CMR           0x04
#define AT91_TC_CMR           0x04
#define AT91_TC_RC            0x1c
#define AT91_TC_RB            0x18
#define AT91_TC_RA            0x14

//ESAM管脚操作
#define easmChipRstDirOut(rst)  at91_set_gpio_output(rst, 1) //ESAM复位脚方向输出
#define easmChipRstLow(rst)     at91_set_gpio_value(rst,0)   //ESAM芯片复位低
#define easmChipRstHigh(rst)    at91_set_gpio_value(rst,1)   //ESAM芯片复位高
#define easmChipDaDirIn(io)     at91_set_gpio_input(io, 0)   //ESAM I/O方向输入
#define easmChipDaDirOut(io)    at91_set_gpio_output(io, 1)  //ESAM I/O方向输出
#define easmChipDaInSt(io)      at91_get_gpio_value(io)      //ESAM I/O输入状态
#define easmChipDaLow(io)       at91_set_gpio_value(io, 0)   //ESAM I/O输出低
#define easmChipDaHigh(io)      at91_set_gpio_value(io, 1)   //ESAM I/O输出高

unsigned char hasEsam;            //有ESAM芯片?
void __iomem *tcBase;             //TC重映射基地址
int           esamChipRst;        //ESAM芯片的复位脚
int           esamChipIo;         //ESAM芯片的I/O脚
unsigned char Even;
unsigned char Test; 
unsigned char Bits; 
unsigned char State;
unsigned char esamBuf[50];
unsigned char esamSerial[8];      //ESAM芯片序列号
unsigned char esamError;
unsigned char esamDeguInfo = 1;
    
#define DEVICE_NAME "ioChannel"

static int   ioChannelMajor;
struct cdev  ioChannelCdev;
struct class *classIoChannel;

unsigned char watchDogx=0;
unsigned char machineType=0;

/*
 * Interrupt handler
 */
//unsigned char lyy;
//static irqreturn_t esam_interrupt(int irq, void *dev_id)
//{ 
//	readl(tc0+0x20);

//	lyy++;
//	at91_set_gpio_output(AT91_PIN_PB22, lyy%2);
	 
//	return IRQ_HANDLED;
//}

//static irqreturn_t PA6_intHandle(int irq, void *dev_id)
//{   
//  printk("检测到PA6中断\n");
  
//  return IRQ_RETVAL(IRQ_HANDLED);   
//} 

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

/**************************************************
函数名称:ackRecv
功能描述:接收一个应答数据
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：接收的字节
***************************************************/
unsigned char ackRecv(void) 
{     
   unsigned RecvData = 0;
   Even = 0;
   Test = 0; 
   Bits = 0; 
   State = 1;
    
   easmChipDaDirIn(esamChipIo);  //EasmChip数据管脚设置为输入
   delay01etu(10);      //起始位的延时

   while(State)
   {
     Bits++;
     if(Bits<9)
     {
        RecvData >>= 1;
        if(easmChipDaInSt(esamChipIo))
        {
           RecvData |= 0x80;
           Even++;
        }
        else
        {
           RecvData |= 0x00;
           Test++;
        }
        delay01etu(10); //1etu
     }
     else
     {
        if(easmChipDaInSt(esamChipIo))
        {
          Even++;
        }
        else
        {
        	Test++;
        }
        State = 0;
        
        delay01etu(6);   //0.5etu
     }
   } 
   
   easmChipDaDirOut(esamChipIo);  //--------------------------------------------------------
   if(Even%2)            //偶校验错
   { 
     easmChipDaLow(esamChipIo);   //ESAM_IO = 0;
     esamError = 1;      //接收偶校验错
   }
   else 
   {
     easmChipDaHigh(esamChipIo);  //偶校验对,ESAM_IO = 1;
   }
    
   delay01etu(9);        //1etu保护时间

   return RecvData; 
}

/**************************************************
函数名称:receiveByte
功能描述:检测字节起始并接收一个字节
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：接收的字节
***************************************************/
unsigned char receiveByte(void)
{ 
   unsigned int  Temp;  
   unsigned char RecvData=0;

   //起始字符超时时间
   Temp = 50000;

   easmChipDaDirIn(esamChipIo);
   while(--Temp) 
   {
     if(!easmChipDaInSt(esamChipIo))
     {
       RecvData = ackRecv();
       break;
     }
   }
    
   //if(Temp==0)
   //{
     //SysErrInfo = 0xFF; 
   //}

   return RecvData;
}

/**************************************************
函数名称:sendByte
功能描述:向ESAM发送一个字节
调用函数:
被调用函数:
输入参数:待发送的数据
输出参数:
返回值:void
***************************************************/
void sendByte(unsigned char SendBuf)
{    
   Even = 0;
   Test = 0;
    
   easmChipDaDirOut(esamChipIo);//CARD_IO_DDR = 1;output
   easmChipDaHigh(esamChipIo);  //
   delay01etu(9);     //1etu
   
   easmChipDaLow(esamChipIo);   //CARD_IO = 0;start Bit=0
   delay01etu(9);     //1etu
   for(Bits = 0; Bits < 8; Bits++)
   {
     if((SendBuf & 0x01) == 0x01)
     { 
       Even++;
       easmChipDaHigh(esamChipIo); //CARD_IO = 1;
     }
     else
     {
       Test++;
       easmChipDaLow(esamChipIo);  //CARD_IO = 0;
     }
     SendBuf >>= 1;
     delay01etu(9);      //1etu
   }
   
   if(Even%2)
   {
     easmChipDaHigh(esamChipIo);   //CARD_IO = 1  
   }
   else
   {
     easmChipDaLow(esamChipIo);    //CARD_IO = 0 
   }
   delay01etu(9);         //1etu
        
   easmChipDaHigh(esamChipIo);     //CARD_IO_DIR = 0;input
   easmChipDaDirIn(esamChipIo);    //EasmChip数据管脚设置为输入
   delay01etu(9);         //1etu
}

//-----***-void CosCommand(bool SysKind,unsigned char IccCase, unsigned char Length)-***-----//
//---功能描述：系统命令发送。 
//---输入条件：SysKind=0--->EasmChip的操作，SysKind=1-->CpuCard的操作，IccCase命令类别，Length命令长度。 
//---输出项目：无。

/**************************************************
函数名称:esamPutGet
功能描述:向ESAM发送数据并接收应答数据
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
***************************************************/
void esamPutGet(unsigned char *buf, unsigned int lenOfSend, unsigned char cmdType, unsigned char lenOfRecv)
{   
   unsigned int  i, k;
   unsigned char ackBuf;
   unsigned char sw1,sw2;
   unsigned char tmpCmd[5];
   
   //unsigned char tmpBuf[500];
   unsigned int  insStartPtr, Lc;
   
   sw1 = 0;
   sw2 = 0;
   
   ackBuf = 0;
   
   //发送命令及数据
   Lc = buf[4];   //本条命令数据长度Lc
   insStartPtr = 0;
   for(i=0; i<5; i++)
   {
     sendByte(buf[i]);
   }
   
   //接收第一字节
   ackBuf = receiveByte();
   if(ackBuf==buf[1])
   {
     //根据命令类型接收后续字节
     switch(cmdType)
     {
       case 1:   //如取随机数
         for(i = 0; i < lenOfRecv + 2; i++) 
         {
           buf[i] = receiveByte();
         }
         
  	     if (esamDeguInfo)
  	     {
           printk("esamPutGet类型1接收字节:\n");
           for(i=0;i<lenOfRecv+2;i++)
           {
             printk("%02x ", buf[i]);
           }
           printk("\n");
         }
         break;

       
       case 2:   //如公钥验证
       	 delay01etu(60);
       	 
       	 //发送数据体
         for(i = 5; i < lenOfSend; i++) 
         {
           sendByte(buf[i]);
         }
         
         //接收响应数据
         i=1;
         while(i<0xfffffffe)
         {
         	  buf[0]=receiveByte();
         	  if (buf[0]>0x60)
         	  {
         	  	 buf[1]=receiveByte();
         	  	 break;
         	  }
         	  i++;
         	  delay01etu(12);   //延迟接收一个字节
         }

  	     if (esamDeguInfo)
  	     {
           printk("esamPutGet类型2接收字节:%02x%02x\n",buf[0],buf[1]);
         }
         break;
       
       
       case 3:    //如终端非对称密钥对注册命令
       	 delay01etu(60);
       	 
       	 //发送数据体
         for(i = 5; i < lenOfSend; i++) 
         {
           sendByte(buf[i]);
         }
         
         //接收响应数据(正常返回61FA)
         i=1;
         while(i<0xfffffffe)
         {
         	  sw1=receiveByte();
         	  if (sw1>0x60)
         	  {
         	  	 sw2=receiveByte();
         	  	 break;
         	  }
         	  delay01etu(12);   //延迟接收一个字节
         	  i++;
         }
  	     
         if (sw1==0x61 && sw2==0xfa)
         {
         	 delay01etu(50);

         	 tmpCmd[0] = 0x00;
         	 tmpCmd[1] = 0xc0;
         	 tmpCmd[2] = 0x00;
         	 tmpCmd[3] = 0x00;
         	 tmpCmd[4] = 0xfa;
           
           //发送取响应数据命令
           for(i=0; i<5; i++)
           {
             sendByte(tmpCmd[i]);
           }
           
           sw1 = receiveByte();  //去除一字节
           for(i = 0; i < 0xfa+2; i++) 
           {
             buf[i] = receiveByte();
           }
  	       
  	       if (esamDeguInfo)
  	       {
             printk("非对称密钥对注册响应数据:");
             for(i = 0; i < 0xfa+2; i++) 
             {
               printk("%02x ", buf[i]);
             }
             printk("\n");
           }
           
           if (buf[250]==0x61 && buf[251]==0x06)
           {
         	   delay01etu(50);
         	   
         	   tmpCmd[0] = 0x00;
         	   tmpCmd[1] = 0xc0;
         	   tmpCmd[2] = 0x00;
         	   tmpCmd[3] = 0x00;
         	   tmpCmd[4] = 0x06;
           
             //发送取响应数据命令
             for(i=0; i<5; i++)
             {
               sendByte(tmpCmd[i]);
             }
           
             sw1 = receiveByte();  //去除一字节
             for(i = 0xfa; i<0xfa+0x6+2; i++) 
             {
               buf[i] = receiveByte();
             }
  	         
  	         if (esamDeguInfo)
  	         {
               printk("非对称密钥对注册第二次响应数据:");
               for(i = 0xfa; i < 0xfa+0x6+2; i++) 
               {
                 printk("%02x ", buf[i]);
               }
               printk("\n");
             }
           }
         }
         
         break;
         
       case 4:    //如终端非对称密钥对更新命令
       	 delay01etu(60);
       	 
       	 //发送数据体
         for(i = 5; i < 145; i++) 
         {
           sendByte(buf[i]);
         }
         
         sw1 = receiveByte();
         sw2 = receiveByte();

       	 if (sw1==0x90 && sw2==0x00)
       	 {
       	   delay01etu(100);
       	   
       	   //发送第二条命令指令
           for(i = 145; i < 150; i++) 
           {
             sendByte(buf[i]);
           }
           
           ackBuf = receiveByte();
           if(ackBuf==buf[146])
           {
           	  delay01etu(60);
         	    
         	    //发送第二条命令数据
              for(i = 150; i < 280; i++) 
              {
                sendByte(buf[i]);
              }
              
              //接收响应数据(正常返回61FA)
              i=1;
              while(i<0xfffffffe)
              {
         	      sw1=receiveByte();
         	      if (sw1>0x60)
         	      {
         	  	    sw2=receiveByte();
         	  	    break;
         	      }
         	      delay01etu(12);   //延迟接收一个字节
         	      i++;
              }
  	          
              if (sw1==0x61 && sw2==0xfa)
              {
              	 delay01etu(50);
              	 
              	 tmpCmd[0] = 0x00;
              	 tmpCmd[1] = 0xc0;
              	 tmpCmd[2] = 0x00;
              	 tmpCmd[3] = 0x00;
              	 tmpCmd[4] = 0xfa;
                
                 //发送取响应数据命令
                 for(i=0; i<5; i++)
                 {
                   sendByte(tmpCmd[i]);
                 }
                
                 sw1 = receiveByte();  //去除一字节
                 for(i = 0; i < 0xfa+2; i++) 
                 {
                   buf[i] = receiveByte();
                 }
       	       
       	         if (esamDeguInfo)
       	         {
                    printk("非对称密钥对更新响应数据:");
                    for(i = 0; i < 0xfa+2; i++) 
                    {
                      printk("%02x ", buf[i]);
                    }
                    printk("\n");
                 }
                
                 if (buf[250]==0x61 && buf[251]==0x1a)
                 {
              	   tmpCmd[0] = 0x00;
              	   tmpCmd[1] = 0xc0;
              	   tmpCmd[2] = 0x00;
              	   tmpCmd[3] = 0x00;
              	   tmpCmd[4] = 0x1a;
                
                   //发送取响应数据命令
                   for(i=0; i<5; i++)
                   {
                     sendByte(tmpCmd[i]);
                   }
                
                   sw1 = receiveByte();  //去除一字节
                   for(i = 0xfa; i<0xfa+0x1a+2; i++) 
                   {
                     buf[i] = receiveByte();
                   }
       	         
       	           if (esamDeguInfo)
       	           {
                     printk("非对称密钥对更新第二次响应数据:");
                     for(i = 0xfa; i < 0xfa+0x1a+2; i++) 
                     {
                       printk("%02x ", buf[i]);
                     }
                     printk("\n");
                   }
                 }
              }
           }
         }
 
         break;
         
       case 5:    //MAC计算
       	 k=0;
       	 for(k=0;k<20;k++)
       	 {
       	   delay01etu(60);
       	 
       	   //发送数据体
       	   Lc = buf[insStartPtr+4];
           for(i = 0; i < Lc; i++) 
           {
             sendByte(buf[insStartPtr+5+i]);
           }
         
           sw1 = receiveByte();
           sw2 = receiveByte();
           
           if (buf[insStartPtr]==0x80)
           {
             if (sw1==0x61 && sw2==0x4)
             {
        	     delay01etu(50);
        	   
        	     tmpCmd[0] = 0x00;
        	     tmpCmd[1] = 0xc0;
        	     tmpCmd[2] = 0x00;
        	     tmpCmd[3] = 0x00;
        	     tmpCmd[4] = 0x04;
          
               //发送取响应数据命令
               for(i=0; i<5; i++)
               {
                 sendByte(tmpCmd[i]);
               }
          
               sw1 = receiveByte();  //去除一字节
               for(i = 0; i<4+2; i++) 
               {
                 buf[i] = receiveByte();
               }
             }
             break;
           }
           else    //第一个/中间级联块
           {
             if (sw1==0x90 && sw2==0x00)   //中间块执行成功
             {
         	     delay01etu(60);
         	   
               insStartPtr += 5+Lc;
               for(i=insStartPtr; i<insStartPtr+5; i++)
               {
                 sendByte(buf[i]);
               }
             
               ackBuf = receiveByte();
               if (ackBuf!=buf[insStartPtr+1])
               {
               	  break;
               }
             }
             else
             {
             	  break;
             }
           }
         }
         break;
         
       case 6:    //如密钥更新命令
       	 delay01etu(60);
       	 
       	 //发送数据体
         insStartPtr = 5+Lc;
         for(i = 5; i < 5+Lc; i++) 
         {
           sendByte(buf[i]);
         }
         //接收响应数据
         sw1 = receiveByte();
         sw2 = receiveByte();
       	 if (sw1==0x90 && sw2==0x00)
       	 {
       	   delay01etu(100);
       	   
       	   //发送第二条命令指令
           Lc = buf[insStartPtr+4];  //第二条指令Lc
           for(i = insStartPtr; i < insStartPtr+5; i++) 
           {
             sendByte(buf[i]);
           }
                      
           ackBuf = receiveByte();
           if(ackBuf==buf[insStartPtr+1])
           {
           	  delay01etu(60);
         	    
         	    //发送第二条命令数据
              insStartPtr += 5;
              for(i = insStartPtr; i < insStartPtr+Lc; i++) 
              {
                sendByte(buf[i]);
              }
              
              //接收响应数据(正常返回9000)
              i=1;
              while(i<300)
              {
         	      buf[0]=receiveByte();
         	      if (buf[0]>0x60)
         	      {
         	  	    buf[1]=receiveByte();
         	  	    break;
         	      }
         	      delay01etu(12);   //延迟接收一个字节
         	      i++;
              }

  	          if (esamDeguInfo)
  	          {
                printk("密钥更新响应数据:%02x%02x\n", buf[0], buf[1]);
              }
           }
           else
           {
           	  printk("密钥更新第二条命令ackBuf=%02x\n",ackBuf);
           }
       	 }
     }
   }
   else
   {
   	 if (cmdType==5)  //计算MAC
   	 {
       buf[1] = receiveByte();
       buf[0] = ackBuf;
   	   printk("ackBuf=%02x,sw2=%02x\n", ackBuf, buf[1]);   	   
   	 }
   	 else
   	 {
   	   esamError = 2;   //接收第一字节不等INS字节
   	 
   	   printk("ackBuf=%02x\n", ackBuf);
   	 }
   }
}

/**************************************************
函数名称:resetEsam
功能描述:复位ESAM芯片
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：0x00-成功,0x00-失败
***************************************************/
unsigned char resetEsam(void)
{
   unsigned char  i;
   unsigned short j;  //, k;
   unsigned int Temp; 
   
   //循环复位3次,若均失败则返回0xFF
   for(i = 0; i < 5; i++)
   {
     esamError = 0;
     
     for(j = 0; j < 13; j++) 
     {
       esamBuf[j] = 0;
     }
     
     //复位应答超时计数器
     Temp = 400000;
	   
	   easmChipRstDirOut(esamChipRst);
     easmChipRstLow(esamChipRst);
     easmChipDaDirIn(esamChipIo);
      
     //上电复位延时35个ETU
     for(j=0;j<35;j++)
     {
       delay01etu(10);
     }
     
     //完成复位
     easmChipRstHigh(esamChipRst);     
     
     //超时时400/fi-40000/fi
     while(--Temp)
     {
       if(!easmChipDaInSt(esamChipIo))
       {
         esamBuf[0] = ackRecv();
         if(esamBuf[0] == 0x3b)
         {
           esamBuf[1] = receiveByte();
           
           for(j = 2; j <= 4+(esamBuf[1] & 0x0F); j++) 
           {
             esamBuf[j] = receiveByte();
           }
         }         
         break;
       }
     }
     
     //复位应答超时处理
     if(Temp == 0)
     {
        easmChipRstLow(esamChipRst);  //RESET = 0
     }
     else
     { 
       if((esamBuf[0] == 0x3B) && (esamBuf[1] == 0x7d) && i>0 && !esamError)
       {
         break;
       }
     }
     
   }
   
   easmChipDaDirIn(esamChipIo);

   if(i < 5) 
   {
  	 memcpy(esamSerial, &esamBuf[10], 8);
  	 
  	 printk("ESAM chip found,Serial:%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x, reset times=%d\n",esamSerial[0],esamSerial[1],esamSerial[2],esamSerial[3],esamSerial[4],esamSerial[5],esamSerial[6],esamSerial[7],i+1);

     return 0x00;
   }
   else 
   {
     return 0xff;
   }
}

/**************************************************
函数名称:initConn9260Qfp
功能描述:初始电力载波集中器IO通道(控制采集单元硬件版本v0.4,CPU为9260的QFP封装)
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
void initConn9260Qfp(void)
{
	/***************************1.无线Modem*********************************/
	/*1.1无线Modem开机信号线 - PA11*/
	if (at91_set_gpio_output(AT91_PIN_PA9, 0)!=0)///PA11
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA9 Output failed!\n");
	}
	/*1.2无线Modem开关电源线 - PB30*/
	if (at91_set_gpio_output(AT91_PIN_PB30, 0)!=0)///PB30
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB30 Output failed!\n");
	}
	/*1.3无线Modem复位线 - PA9*/
	if (at91_set_gpio_output(AT91_PIN_PA22, 1)!=0)///PA9
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA22 Output failed!\n");
	}
	/*1.4无线Modem模块型号识别state0 - PA10*/
	/*1.5无线Modem模块型号识别state1 - PA9*/
	/*1.6无线Modem模块型号识别state2 - PA8*/
	/*1.7无线Modem模块型号识别state3 - PA7*/
	/*1.8无线Modem模块型号识别state4 - PA6*/
	/*在控制单元V0.4上,以上这5根线在xMega上*/

	/***************************2.按键**************************************/
	/*2.1上键 - PA10*/
	if (at91_set_gpio_input(AT91_PIN_PC3, 1)!=0)///PA10
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC3 Input  failed!\n");
	}

	/*2.2下键 - PA25*/
	if (at91_set_gpio_input(AT91_PIN_PA25, 1)!=0)///
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA25 Input  failed!\n");
	}

	/*2.3左键 - PA22*/
	if (at91_set_gpio_input(AT91_PIN_PA11, 1)!=0)///PA22
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA11 Input  failed!\n");
	}

	/*2.4右键 - PC1*/
	if (at91_set_gpio_input(AT91_PIN_PC1, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC1 Input failed!\n");
	}	

	/*2.5确认键 - PA8*/
	if (at91_set_gpio_input(AT91_PIN_PA30, 1)!=0)///PA8
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA30 Input failed!\n");
	}	

	/*2.6取消键 - PA5*/
	if (at91_set_gpio_input(AT91_PIN_PA5, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA5 Input failed!\n");
	}
	
	/***************************3.遥信**************************************/
	/*3.1 遥信1(YX1) - PA6*/
	if (at91_set_gpio_input(AT91_PIN_PA6, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA6 Input  failed!\n");
	}
	/*3.2 遥信2(YX2) - PB31*/
	if (at91_set_gpio_input(AT91_PIN_PB31, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB31 Input  failed!\n");
	}
	
	/***************************4.停电检测**********************************/
	/*停电检测引脚 - PC15*/
	if (at91_set_gpio_input(AT91_PIN_PC15, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC15 Input  failed!\n");
	}
	
	/***************************5.载波模块控制引脚**************************/
	/*5.1载波模块设置(Z/SET) - PA4*/
	if (at91_set_gpio_output(AT91_PIN_PA4, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA4 Output failed!\n");
	}
	/*5.2载波模块复位(Z/RST) - PB28*/
	if (at91_set_gpio_output(AT91_PIN_PB28, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB28 Output failed!\n");
	}

	/***************************6.告警指示灯和告警音************************/
	/*6.1告警指示 - PB22*/
	if (at91_set_gpio_output(AT91_PIN_PC2, 1)!=0)///PB22
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC2 Output failed!\n");
	}
	/*6.2告警音 - PB24*/
	if (at91_set_gpio_output(AT91_PIN_PB12, 0)!=0)///PB24
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB12 Output failed!\n");
	}

	/***************************7.停电管理*********************************/
	/*停电管理 - PB19*/
	if (at91_set_gpio_output(AT91_PIN_PB19, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB19 Output failed!\n");
	}
	
	/***************************8.开盖检测*********************************/
	/*开盖检测 - PB19*/
	if (at91_set_gpio_input(AT91_PIN_PA31, 1)!=0)///PA26
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA31 Output failed!\n");
	}
	
	at91_set_A_periph(AT91_PIN_PB8, 1);		/* TXD2 */
	at91_set_A_periph(AT91_PIN_PB9, 0);		/* RXD2 */
	    
	printk("初始化集中器IO(控制单元硬件版本v0.4,CPU9260-QFP)\n");
}

/**************************************************
函数名称:initOldTe
功能描述:初始老终端IO通道(包括FKGA42-LT003,FCGA02-LT002,FCGB02-LT001)
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
void initOldTe(void)
{
	/***************************1.无线Modem*********************************/
	/*1.2无线Modem开关电源线 - PC0*/
	if (at91_set_gpio_output(AT91_PIN_PC0, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC0 Output failed!\n");
	}
	/*1.3无线Modem复位线(开机线IGT) - PA22*/
	if (at91_set_gpio_output(AT91_PIN_PA22, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA11 Output failed!\n");
	}
	/*1.4无线Modem模块型号识别state0 - PC4*/
	if (at91_set_gpio_input(AT91_PIN_PC4, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC4 Output failed!\n");
	}
	/*1.5无线Modem模块型号识别state1 - PC8*/
	if (at91_set_gpio_input(AT91_PIN_PC8, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC8 Output failed!\n");
	}
	/*1.6无线Modem模块型号识别state2 - PC6*/
	if (at91_set_gpio_input(AT91_PIN_PC6, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC6 Output failed!\n");
	}
	/*1.7无线Modem模块型号识别state3 - PC10*/
	if (at91_set_gpio_input(AT91_PIN_PC10, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC10 Output failed!\n");
	}

	/***************************2.按键**************************************/
	/*2.1确认键 - PA7*/
	if (at91_set_gpio_input(AT91_PIN_PA7, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB17 Input  failed!\n");
	}

	/*2.2选择键 - PA6*/
	if (at91_set_gpio_input(AT91_PIN_PA6, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB16 Input  failed!\n");
	}

	/***************************3.遥信**************************************/
	/*3.1 遥信1(YX1) - PC2*/
	if (at91_set_gpio_input(AT91_PIN_PC2, 1)!=0)/////NO PIN
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC2 Input  failed!\n");
	}
	/*3.2 遥信2(YX2) - PB20*/
	if (at91_set_gpio_input(AT91_PIN_PB20, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB20 Input  failed!\n");
	}
	/*3.3 遥信3(YX3) - PB17*/
	if (at91_set_gpio_input(AT91_PIN_PB17, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB17 Input  failed!\n");
	}
	/*3.4 遥信4(YX4) - PB18*/
	if (at91_set_gpio_input(AT91_PIN_PB18, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB18 Input  failed!\n");
	}
	/*3.5 遥信5(YX5) - PB10*/
	if (at91_set_gpio_input(AT91_PIN_PB10, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB10 Input  failed!\n");
	}
	/*3.6 遥信6(YX6) - PB11*/
	if (at91_set_gpio_input(AT91_PIN_PB11, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB11 Input  failed!\n");
	}
	/*3.7 遥信7(YX7) - PA31*/
	if (at91_set_gpio_input(AT91_PIN_PA31, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA31 Input  failed!\n");
	}
	/*3.8 遥信8(YX8) - PA30*/
	if (at91_set_gpio_input(AT91_PIN_PA30, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA30 Input  failed!\n");
	}
	
	/*3.9门控输入 - PB21*/
	if (at91_set_gpio_input(AT91_PIN_PB21, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB21 Input  failed!\n");
	}
	
	/***************************4.停电检测**********************************/
	/*停电检测引脚 - PC15*/
	if (at91_set_gpio_input(AT91_PIN_PC15, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC15 Input  failed!\n");
	}
	
	/***************************5.告警指示灯和告警音************************/
	/*5.1告警音(蜂鸣器) - PB12*/
	if (at91_set_gpio_output(AT91_PIN_PB12, 0)!=0)////NO PIN
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB12 Output failed!\n");
	}

	/***************************6.后背电池管理*********************************/
	/*6.1 5V继电器控制 - PB19*/
	if (at91_set_gpio_output(AT91_PIN_PB19, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB19 Output failed!\n");
	}
	/*6.2 充电管理 - PB30*/
	if (at91_set_gpio_output(AT91_PIN_PB30, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB30 Output failed!\n");
	}

	/***************************7.控制继电器控制***********************************/
	/*7.1继电器控制1 - PB29*/
	if (at91_set_gpio_output(AT91_PIN_PB29, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC4 Output failed!\n");
	}
	/*7.2继电器控制2 - PA5*/
	if (at91_set_gpio_output(AT91_PIN_PA5, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC6 Output failed!\n");
	}
	/*7.3继电器控制3 - PA25*/
	if (at91_set_gpio_output(AT91_PIN_PA25, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC7 Output failed!\n");
	}
	/*7.4继电器控制4 - PA11*/
	if (at91_set_gpio_output(AT91_PIN_PA11, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA26 Output failed!\n");
	}
	
	printk("初始化GDW129-2005终端IO\n");
}

/**************************************************
函数名称:initType3TeQfp
功能描述:初始化专变III型终端IO通道(QFP封装CPU)
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
void initType3TeQfp(void)
{
	/***************************1.无线Modem*********************************/
	/*1.1无线Modem开机信号线 - PA11*/
	if (at91_set_gpio_output(AT91_PIN_PA9, 0)!=0)///PA11
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA9 Output failed!\n");
	}
	/*1.2无线Modem开关电源线 - PB30*/
	if (at91_set_gpio_output(AT91_PIN_PB30, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC0 Output failed!\n");
	}
	/*1.3无线Modem复位线(开机线IGT) - PA9*/
	if (at91_set_gpio_output(AT91_PIN_PA22, 0)!=0)///PA9
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA22 Output failed!\n");
	}
	/*1.4无线Modem模块型号识别state0 - PA26*/
	if (at91_set_gpio_input(AT91_PIN_PA31, 0)!=0)///PA26
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA31 Output failed!\n");
	}
	/*1.5无线Modem模块型号识别state1 - PA10*/
	if (at91_set_gpio_input(AT91_PIN_PC3, 0)!=0)///PA10
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC3 Output failed!\n");
	}
	/*1.6无线Modem模块型号识别state2 - PC1*/
	if (at91_set_gpio_input(AT91_PIN_PC1, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC1 Output failed!\n");
	}
	/*1.7无线Modem模块型号识别state3 - PA8*/
	if (at91_set_gpio_input(AT91_PIN_PA30, 0)!=0)///PA8
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA30 Output failed!\n");
	}

	/***************************2.按键**************************************/
	/*2.1上键 - PB21*/
	if (at91_set_gpio_input(AT91_PIN_PB21, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB21 Input  failed!\n");
	}

	/*2.2下键 - PA25*/
	if (at91_set_gpio_input(AT91_PIN_PA25, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA25 Input  failed!\n");
	}

	/*2.3左键 - PB29*/
	if (at91_set_gpio_input(AT91_PIN_PB29, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB29 Input  failed!\n");
	}

	/*2.4右键 - PB28*/
	if (at91_set_gpio_input(AT91_PIN_PB28, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB28 Input failed!\n");
	}	

	/*2.5确认键 - PA5*/
	if (at91_set_gpio_input(AT91_PIN_PA5, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA5 Input failed!\n");
	}	

	/*2.6取消键 - PA11*/
	if (at91_set_gpio_input(AT91_PIN_PA11, 1)!=0)///PA22
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA11 Input failed!\n");
	}

	/***************************3.遥信**************************************/
	/*3.1 遥信1(YX1) - PA6*/
	if (at91_set_gpio_input(AT91_PIN_PA6, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA6 Input  failed!\n");
	}
	/*3.2 遥信2(YX2) - PB31*/
	if (at91_set_gpio_input(AT91_PIN_PB31, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB31 Input  failed!\n");
	}
	
	/*3.3 遥信3(YX3) - PB20*/
	if (at91_set_gpio_input(AT91_PIN_PB20, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB20 Input  failed!\n");
	}
	/*3.4 遥信4(YX4) - PB22*/
	if (at91_set_gpio_input(AT91_PIN_PC2, 1)!=0)///PB22
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC2 Input  failed!\n");
	}
	
	/*3.5 遥信5(YX5) - PB17*/
	if (at91_set_gpio_input(AT91_PIN_PB17, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB17 Input  failed!\n");
	}
	/*3.6 遥信6(YX6) - PB11*/
	if (at91_set_gpio_input(AT91_PIN_PB18, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB18 Input  failed!\n");
	}

	/***************************4.停电检测**********************************/
	/*停电检测引脚 - PC15*/
	if (at91_set_gpio_input(AT91_PIN_PC15, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC15 Input  failed!\n");
	}
	
	/***************************5.告警指示灯和告警音************************/
	/*5.1告警音(蜂鸣器) - PB12*/
	if (at91_set_gpio_output(AT91_PIN_PB12, 0)!=0)///PB24
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB12 Output failed!\n");
	}

	/***************************6.后背电池管理*********************************/
	/*6.1 5V继电器控制 - PB19*/
	if (at91_set_gpio_output(AT91_PIN_PB19, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB19 Output failed!\n");
	}

	/***************************7.控制继电器控制***********************************/
	/*7.1继电器控制1 - PC4*/
	if (at91_set_gpio_output(AT91_PIN_PC4, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC4 Output failed!\n");
	}
	/*7.2继电器控制2 - PC8*/
	if (at91_set_gpio_output(AT91_PIN_PC8, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC8 Output failed!\n");
	}
	/*7.3继电器控制3 - PC6*/
	if (at91_set_gpio_output(AT91_PIN_PC6, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC6 Output failed!\n");
	}
	/*7.4继电器控制4 - PC10*/
	if (at91_set_gpio_output(AT91_PIN_PC10, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC10 Output failed!\n");
	}

	/*7.5控制输入时钟 - PA4*/
	if (at91_set_gpio_output(AT91_PIN_PA4, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA4 Output failed!\n");
	}	
	
	printk("初始化GDW376-2009专变III型(QFP)终端IO\n");
}

/**************************************************
函数名称:initType3Tev13Qfp
功能描述:初始化专变III型终端控制单元1.3IO通道(QFP封装CPU)
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
void initType3Tev13Qfp(void)
{
	int err;

	/***************************1.无线Modem*********************************/
	/*1.1无线Modem开机信号线 - PA10*/
	if (at91_set_gpio_output(AT91_PIN_PA10, 0)!=0)///PA10
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC3 Output failed!\n");
	}
	/*1.2无线Modem开关电源线 - PC1*/
	if (at91_set_gpio_output(AT91_PIN_PC1, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC1 Output failed!\n");
	}
	/*1.3无线Modem复位线(开机线IGT) - PA9*/
	if (at91_set_gpio_output(AT91_PIN_PA22, 0)!=0)///PA9
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA22 Output failed!\n");
	}
	/*1.4无线Modem模块型号识别state0 - PA26*/
	/*1.5无线Modem模块型号识别state1 - PA10*/
	/*1.6无线Modem模块型号识别state2 - PC1*/
	/*1.7无线Modem模块型号识别state3 - PA8*/
	/*v1.3模块型号识别在xMega上*/

	/***************************2.按键**************************************/
	/*2.1上键 - PB21*/
	if (at91_set_gpio_input(AT91_PIN_PB21, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB21 Input  failed!\n");
	}

	/*2.2下键 - PA25*/
	if (at91_set_gpio_input(AT91_PIN_PA25, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA25 Input  failed!\n");
	}

	/*2.3左键 - PB29*/
	if (at91_set_gpio_input(AT91_PIN_PB29, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB29 Input  failed!\n");
	}

	/*2.4右键 - PB28*/
	if (at91_set_gpio_input(AT91_PIN_PB28, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB28 Input failed!\n");
	}	

	/*2.5确认键 - PA5*/
	if (at91_set_gpio_input(AT91_PIN_PA5, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA5 Input failed!\n");
	}	

	/*2.6取消键 - PA11*/
	if (at91_set_gpio_input(AT91_PIN_PA11, 1)!=0)///PA22
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA11 Input failed!\n");
	}

	/***************************3.遥信**************************************/
	/*3.1 遥信1(YX1) - PA6*/
	if (at91_set_gpio_input(AT91_PIN_PA6, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA6 Input  failed!\n");
	}

  /*中断示例
  at91_set_gpio_input(AT91_PIN_PA6, 1);
  at91_set_deglitch(AT91_PIN_PA6, 1);
  at91_sys_write(1 + PIO_IDR,  1<<6);
  at91_sys_write(1 + PIO_IER,  (~(1<<6)));
  at91_sys_write(AT91_PMC_PCER, 1 << 2);
  	
  
  err = request_irq(AT91_PIN_PA6,PA6_intHandle,AT91_AIC_SRCTYPE_LOW,"PA6",(void*)0);
	
	if(err)    
  {   
    printk("中断申请失败\n");
    
    free_irq(AT91_PIN_PA6,(void*)0);
  }
  */ 
	
	/*3.2 遥信2(YX2) - PB31*/
	if (at91_set_gpio_input(AT91_PIN_PB31, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB31 Input  failed!\n");
	}
	
	/*3.3 遥信3(YX3) - PA26*/
	if (at91_set_gpio_input(AT91_PIN_PA31, 1)!=0)///PA26
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA31 Input  failed!\n");
	}
	/*3.4 遥信4(YX4) - PA8*/
	if (at91_set_gpio_input(AT91_PIN_PA30, 1)!=0)///PA8
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA30 Input  failed!\n");
	}
	
	/*3.5 遥信5(YX5) - PB17*/
	if (at91_set_gpio_input(AT91_PIN_PB17, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB17 Input  failed!\n");
	}
	/*3.6 遥信6(YX6) - PB11*/
	if (at91_set_gpio_input(AT91_PIN_PB18, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB18 Input  failed!\n");
	}

	/***************************4.停电检测**********************************/
	/*停电检测引脚 - PC15*/
	if (at91_set_gpio_input(AT91_PIN_PC15, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC15 Input  failed!\n");
	}
	
	/***************************5.告警指示灯和告警音************************/
	/*5.1告警音(蜂鸣器) - PB24*/
	if (at91_set_gpio_output(AT91_PIN_PB12, 0)!=0)///PB24
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB12 Output failed!\n");
	}

	/***************************6.后背电池管理*********************************/
	/*6.1 5V继电器控制 - PB19*/
	if (at91_set_gpio_output(AT91_PIN_PB19, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB19 Output failed!\n");
	}

	/***************************7.控制继电器控制***********************************/
	/*7.1继电器控制1 - PC4*/
	//if (at91_set_gpio_output(AT91_PIN_PA4, 1)!=0)
	//{
	//   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC4 Output failed!\n");
	//}
	/*7.2继电器控制2 - PC8*/
	//if (at91_set_gpio_output(AT91_PIN_PC8, 1)!=0)
	//{
	//   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC8 Output failed!\n");
	//}
	/*7.3继电器控制3 - PC6*/
	//if (at91_set_gpio_output(AT91_PIN_PC12, 1)!=0)///PB16
	//{
	//   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC12 Output failed!\n");
	//}
	/*7.4继电器控制4 - PC10*/
	//if (at91_set_gpio_output(AT91_PIN_PC10, 1)!=0)
	//{
	//   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA4 Output failed!\n");
	//}

	/*7.1继电器控制1 - PC4*/
	if (at91_set_gpio_output(AT91_PIN_PA4, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC4 Output failed!\n");
	}
	/*7.2继电器控制2 - PC8*/
	if (at91_set_gpio_output(AT91_PIN_PC8, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC8 Output failed!\n");
	}
	/*7.3继电器控制3 - PC6*/
	if (at91_set_gpio_output(AT91_PIN_PB16, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB16 Output failed!\n");
	}
	
	/*7.5控制输入时钟 - PA4*/
	if (at91_set_gpio_output(AT91_PIN_PC4, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC4 Output failed!\n");
	}

	/*7.4继电器 - EN*/
	if (at91_set_gpio_output(AT91_PIN_PC10, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA4 Output failed!\n");
	}
	
	at91_set_A_periph(AT91_PIN_PB6, 1);		/* TXD1 */
	at91_set_A_periph(AT91_PIN_PB7, 0);		/* RXD1 */

	printk("初始化GDW376-2009专变III型控制单元硬件v1.3(QFP)终端IO\n");
}

/**************************************************
函数名称:ioChannel_ioctl
功能描述:IO通道驱动IO控制
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static int ioChannel_ioctl(struct inode *inode, struct file *file,unsigned int cmd, unsigned long arg)
{
   u8 tmpData = 0;
   switch(cmd)
   {
			case ESAM_DEBUG_INFO:				
			  if (arg==1)
			  {
			    esamDeguInfo = 1;
			  }
			  else
			  {
			    esamDeguInfo = 1;
			  }
			  break;
			  
			case MACHINE_TYPE:
				machineType = arg;
				printk("设置iochannel机型为%d\n", machineType);
				switch(machineType)
			  {
			  	 case 2:    //老终端
			  	 	 initOldTe();
			  	 	 break;

			  	 case 5:    //专变III型新终端(CPU为QFP)
			  	 	 initType3TeQfp();
			  	 	 break;
			  	 	 
			  	 case 6: 	  //集中器(控制单元硬件版本v0.4,CPU9260-QFP)
			  	 	 initConn9260Qfp();
			  	 	 break;
			  	 
			  	 case 7: 	  //专变III型终端(控制单元硬件版本v1.3,CPU9260-QFP)
			  	 	 initType3Tev13Qfp();
			  	 	 break;

			  	 default:   //默认为集中器
			  	 	 initConcentrator();
			  	 	 break;
			  }
				break;
				
			default:
				switch(machineType)
			  {
			  	default:    //默认为集中器(控制单元硬件版本V0.3,CPU迈冲核心板BGA)
				    switch(cmd)
				    {
         			case READ_KEY_VALUE:   /*读取I/O脚的值*/
         				if (!at91_get_gpio_value(AT91_PIN_PB17))
         				{
         					 tmpData = KEY_UP;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PB16))
         				{
         					 tmpData = KEY_DOWN;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PA26))
         				{
         					 tmpData = KEY_LEFT;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PA27))
         				{
         					 tmpData = KEY_RIGHT;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PA25))
         				{
         					 tmpData = KEY_CANCEL;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PA28))
         				{
         					 tmpData = KEY_OK;
         				}
         				break;
         
         			case READ_MODULE_TYPE:    //读取无线Modem类型
         				if (at91_get_gpio_value(AT91_PIN_PC3))  /*state0*/                                                   //PA10
         				{
         					 tmpData |= 0x1;
         				}
         				if (at91_get_gpio_value(AT91_PIN_PA22))   /*state1*/                                                   //PA9
         				{
         					 tmpData |= 0x2;
         				}
         				if (at91_get_gpio_value(AT91_PIN_PA30))   /*state2*/                                                   //PA8
         				{
         					 tmpData |= 0x4;
         				}
         				if (at91_get_gpio_value(AT91_PIN_PA7))   /*state3*/
         				{
         					 tmpData |= 0x8;
         				}
         				if (at91_get_gpio_value(AT91_PIN_PA6))   /*state4*/
         				{
         					 tmpData |= 0x10;
         				}
         				break;
         				
         			case WIRELESS_POWER_ON_OFF:   /*打开/关闭无线Modem电源*/
         				at91_set_gpio_value(AT91_PIN_PB31,arg);
         				break;
               
         			case WIRELESS_IGT:            /*无线Modem开关机,press key*/
         				at91_set_gpio_value(AT91_PIN_PB20,arg);
         				break;
         
         			case WIRELESS_RESET:          /*无线Modem复位*/
         				at91_set_gpio_value(AT91_PIN_PC2,arg);
                                                         //PB22
         				break;
         
               case READ_YX_VALUE:           /*读取遥信值*/
         				if (at91_get_gpio_value(AT91_PIN_PC2))  /*YX1*/
         				{
         					 tmpData |= 0x1;
         				}
         				if (at91_get_gpio_value(AT91_PIN_PC3))   /*YX2*/
         				{
         					 tmpData |= 0x2;
         				}      	
               	break;
               	
               case DETECT_POWER_FAILURE:    /*停电检测*/
         				if (at91_get_gpio_value(AT91_PIN_PC4))  /*YX1*/
         				{
         					 tmpData = IO_HIGH;
         				}
         				else
         				{
         					 tmpData = IO_LOW;
         				}
               	break;
               	
         			case SET_ALARM_LIGHT:         /*设置告警灯*/
         				at91_set_gpio_value(AT91_PIN_PB30,arg);
         				break;
               	
         			case SET_ALARM_VOICE:         /*设置告警音*/
         				at91_set_gpio_value(AT91_PIN_PA28,arg);
                                                         //PC7
         				break;
         				
         			case SET_BATTERY_ON:          /*设置后备电池通断*/
         				at91_set_gpio_value(AT91_PIN_PC6,arg);
         				break;
         		  
               case SET_CARRIER_MODULE:     /*设置载波模块/set*/
         				at91_set_gpio_value(AT91_PIN_PA5,arg);
               	break;
               	
         		  case RST_CARRIER_MODULE:     /*复位载波模块*/
         				//硬件电路反向,不符合常理,不通用,软件反过来
         				if (arg==0)
         				{
         				  at91_set_gpio_value(AT91_PIN_PB18,1);
         				}
         				else
         				{
         				  at91_set_gpio_value(AT91_PIN_PB18,0);
         				}
         		  	break;
         		  	
         		  case SET_WATCH_DOG:          /**/
         		  	if (watchDogx==0)
         		  	{
         		  		watchDogx = 1;
         		  		at91_set_gpio_output(AT91_PIN_PC5, 0);
         		  		at91_set_gpio_output(AT91_PIN_PC0, 0);
         				}
         				at91_set_gpio_value(AT91_PIN_PC5,arg);
         				at91_set_gpio_value(AT91_PIN_PC0,arg);
         		  	break;
         
         			default:
         				break;
            }
            break;
            
          case 2:     //老终端
          	switch(cmd)
          	{
         			case READ_KEY_VALUE:   /*读取I/O脚的值*/
         				if (!at91_get_gpio_value(AT91_PIN_PA7))
         				{
         					 tmpData = KEY_OK;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PA6))
         				{
         					 tmpData = KEY_CANCEL;
         				}
         				break;
         
         			case READ_MODULE_TYPE:    //读取无线Modem类型
         				if (at91_get_gpio_value(AT91_PIN_PC4))  /*state0*/
         				{
         					 tmpData |= 0x1;
         				}
         				if (at91_get_gpio_value(AT91_PIN_PC8))  /*state1*/
         				{
         					 tmpData |= 0x2;
         				}
         				if (at91_get_gpio_value(AT91_PIN_PC6))  /*state2*/
         				{
         					 tmpData |= 0x4;
         				}
         				if (at91_get_gpio_value(AT91_PIN_PC10)) /*state3*/
         				{
         					 tmpData |= 0x8;
         				}
         				break;
         				
         			case WIRELESS_POWER_ON_OFF:   /*打开/关闭无线Modem电源*/
         				at91_set_gpio_value(AT91_PIN_PC0,arg);
         				break;
               
         			case WIRELESS_IGT:            /*无线Modem开关机,press key*/
         				at91_set_gpio_value(AT91_PIN_PA11,arg);
                                                         //PA22
         				break;
         
         			case WIRELESS_RESET:          /*无线Modem复位*/
         				at91_set_gpio_value(AT91_PIN_PA11,arg);
                                                          //PA22
         				break;
         
               case READ_YX_VALUE:           /*读取遥信值*/
         				if (at91_get_gpio_value(AT91_PIN_PC2))  /*YX1*/
         				{
         					 tmpData |= 0x1;
         				}
         				if (at91_get_gpio_value(AT91_PIN_PB20))   /*YX2*/
         				{
         					 tmpData |= 0x2;
         				}      	
         				if (at91_get_gpio_value(AT91_PIN_PB17))   /*YX3*/
         				{
         					 tmpData |= 0x4;
         				}      	
         				if (at91_get_gpio_value(AT91_PIN_PB18))   /*YX4*/
         				{
         					 tmpData |= 0x8;
         				}      	
         				if (at91_get_gpio_value(AT91_PIN_PB10))   /*YX5*/
         				{
         					 tmpData |= 0x10;
         				}      	
         				if (at91_get_gpio_value(AT91_PIN_PB11))   /*YX6*/
         				{
         					 tmpData |= 0x20;
         				}      	
         				if (at91_get_gpio_value(AT91_PIN_PA31))   /*YX7*/
         				{
         					 tmpData |= 0x40;
         				}      	
         				if (at91_get_gpio_value(AT91_PIN_PA30))   /*YX8*/
         				{
         					 tmpData |= 0x80;
         				}
               	break;      	
               	
               case READ_GATEK_VALUE:           /*读取门控值*/
         				if (at91_get_gpio_value(AT91_PIN_PB21))
         				{
         					 tmpData |= 0x1;
         				}
         				break;
               	
               case DETECT_POWER_FAILURE:    /*停电检测*/
         				if (at91_get_gpio_value(AT91_PIN_PC15))
         				{
         					 tmpData = IO_HIGH;
         				}
         				else
         				{
         					 tmpData = IO_LOW;
         				}
               	break;
               	
         			case SET_ALARM_VOICE:         /*设置告警音*/
         				at91_set_gpio_value(AT91_PIN_PB12,arg);
         				break;
         				
         			case SET_BATTERY_ON:          /*设置后备电池通断*/
         				at91_set_gpio_value(AT91_PIN_PB19,arg);
         				break;
         		  
         		  case LOAD_CTRL_LINE_1:       /*负荷控制第一路*/
         				at91_set_gpio_value(AT91_PIN_PB29,arg);		  	
         		  	break;
         		  	
         		  case LOAD_CTRL_LINE_2:       /*负荷控制第二路*/
         				at91_set_gpio_value(AT91_PIN_PA5,arg);
         		  	break;
         
         		  case LOAD_CTRL_LINE_3:       /*负荷控制第3路*/
         				at91_set_gpio_value(AT91_PIN_PA25,arg);
         		  	break;
         
         		  case LOAD_CTRL_LINE_4:       /*负荷控制第4路*/
         				at91_set_gpio_value(AT91_PIN_PA11,arg);
                                                          //PA11
         		  	break;
         		  	
         		  case SET_BATTERY_CHARGE:    /*充电管理*/
         		  	at91_set_gpio_value(AT91_PIN_PB30,arg);
         		  	break;		  	
         		  	
         			default:
         				break;
             	break;
            }
            break;
          
          case 4:     //专变III型新终端
          	switch(cmd)
          	{
         			case READ_KEY_VALUE:   /*读取I/O脚的值*/
         				if (!at91_get_gpio_value(AT91_PIN_PB21))
         				{
         					 tmpData = KEY_UP;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PA25))
         				{
         					 tmpData = KEY_DOWN;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PB29))
         				{
         					 tmpData = KEY_LEFT;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PB28))
         				{
         					 tmpData = KEY_RIGHT;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PA9))
                                                              //PA11
         				{
         					 tmpData = KEY_CANCEL;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PA5))
         				{
         					 tmpData = KEY_OK;
         				}
         				break;
         
         			case READ_MODULE_TYPE:    //读取无线Modem类型
         				if (at91_get_gpio_value(AT91_PIN_PA31))  /*state0*/
         				{
         					 tmpData |= 0x1;
         				}
         				if (at91_get_gpio_value(AT91_PIN_PC3))  /*state1*/
         				{
         					 tmpData |= 0x2;
         				}
         				if (at91_get_gpio_value(AT91_PIN_PC1))  /*state2*/
         				{
         					 tmpData |= 0x4;
         				}
         				if (at91_get_gpio_value(AT91_PIN_PA30)) /*state3*/
         				{
         					 tmpData |= 0x8;
         				}
         				break;
         				
         			case WIRELESS_POWER_ON_OFF:   /*打开/关闭无线Modem电源*/
         				at91_set_gpio_value(AT91_PIN_PB30,arg);
         				break;
               
         			case WIRELESS_IGT:            /*无线Modem开关机,press key*/
         				at91_set_gpio_value(AT91_PIN_PA9,arg);
         				break;
         
         			case WIRELESS_RESET:          /*无线Modem复位*/
         				at91_set_gpio_value(AT91_PIN_PA22,arg);
         				break;
         
               case READ_YX_VALUE:           /*读取遥信值*/
         				if (at91_get_gpio_value(AT91_PIN_PA6))  /*YX1*/
         				{
         					 tmpData |= 0x1;
         				}
         				if (at91_get_gpio_value(AT91_PIN_PB31))   /*YX2*/
         				{
         					 tmpData |= 0x2;
         				}      	
         				if (at91_get_gpio_value(AT91_PIN_PB20))   /*YX3*/
         				{
         					 tmpData |= 0x4;
         				}      	
         				if (at91_get_gpio_value(AT91_PIN_PC2))   /*YX4*/
         				{
         					 tmpData |= 0x8;
         				}         				      	
         				if (at91_get_gpio_value(AT91_PIN_PB17))   /*YX5*/
         				{
         					 tmpData |= 0x10;
         				}      	
         				if (at91_get_gpio_value(AT91_PIN_PB18))   /*YX6*/
         				{
         					 tmpData |= 0x20;
         				}
               	break;      	
               	
               case READ_GATEK_VALUE:           /*读取门控值*/
         				if (at91_get_gpio_value(AT91_PIN_PB21))
         				{
         					 tmpData |= 0x1;
         				}
         				break;
               	
               case DETECT_POWER_FAILURE:    /*停电检测*/
         				if (at91_get_gpio_value(AT91_PIN_PC15))
         				{
         					 tmpData = IO_HIGH;
         				}
         				else
         				{
         					 tmpData = IO_LOW;
         				}
               	break;
               	
         			case SET_ALARM_VOICE:         /*设置告警音*/
         				at91_set_gpio_value(AT91_PIN_PB12,arg);
         				break;
         				
         			case SET_BATTERY_ON:          /*设置后备电池通断*/
         				at91_set_gpio_value(AT91_PIN_PB19,arg);
         				break;
         		  
         		  case LOAD_CTRL_LINE_1:       /*负荷控制第一路*/
         				at91_set_gpio_value(AT91_PIN_PC4,arg);		  	
         		  	break;
         		  	
         		  case LOAD_CTRL_LINE_2:       /*负荷控制第二路*/
         				at91_set_gpio_value(AT91_PIN_PC8,arg);
         		  	break;
         
         		  case LOAD_CTRL_LINE_3:       /*负荷控制第3路*/
         				at91_set_gpio_value(AT91_PIN_PC6,arg);
         		  	break;
         
         		  case LOAD_CTRL_LINE_4:       /*负荷控制第4路*/
         				at91_set_gpio_value(AT91_PIN_PC10,arg);
         		  	break;
         		  	
         		  case LOAD_CTRL_CLOCK:        /*负荷控制时钟*/
         				at91_set_gpio_value(AT91_PIN_PA4,arg);
         		  	break;         		  	
         		  	
         			default:
         				break;
             	break;
            }
            break;
            
          case 5:     //专变III型新终端(QFP)
          	switch(cmd)
          	{
         			case READ_KEY_VALUE:   /*读取I/O脚的值*/
         				if (!at91_get_gpio_value(AT91_PIN_PB21))
         				{
         					 tmpData = KEY_UP;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PA25))
         				{
         					 tmpData = KEY_DOWN;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PB29))
         				{
         					 tmpData = KEY_LEFT;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PB28))
         				{
         					 tmpData = KEY_RIGHT;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PA22))
         				{
         					 tmpData = KEY_CANCEL;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PA5))
         				{
         					 tmpData = KEY_OK;
         				}
         				break;
         
         			case READ_MODULE_TYPE:    //读取无线Modem类型
         				if (at91_get_gpio_value(AT91_PIN_PA26))  /*state0*/
         				{
         					 tmpData |= 0x1;
         				}
         				if (at91_get_gpio_value(AT91_PIN_PA10))  /*state1*/
         				{
         					 tmpData |= 0x2;
         				}
         				if (at91_get_gpio_value(AT91_PIN_PC1))  /*state2*/
         				{
         					 tmpData |= 0x4;
         				}
         				if (at91_get_gpio_value(AT91_PIN_PA8)) /*state3*/
         				{
         					 tmpData |= 0x8;
         				}
         				break;
         				
         			case WIRELESS_POWER_ON_OFF:   /*打开/关闭无线Modem电源*/
         				at91_set_gpio_value(AT91_PIN_PB30,arg);
         				break;
               
         			case WIRELESS_IGT:            /*无线Modem开关机,press key*/
         				at91_set_gpio_value(AT91_PIN_PA11,arg);
         				break;
         
         			case WIRELESS_RESET:          /*无线Modem复位*/
         				at91_set_gpio_value(AT91_PIN_PA9,arg);
         				break;
         
               case READ_YX_VALUE:           /*读取遥信值*/
         				if (at91_get_gpio_value(AT91_PIN_PA6))  /*YX1*/
         				{
         					 tmpData |= 0x1;
         				}
         				if (at91_get_gpio_value(AT91_PIN_PB31))   /*YX2*/
         				{
         					 tmpData |= 0x2;
         				}      	
         				if (at91_get_gpio_value(AT91_PIN_PB20))   /*YX3*/
         				{
         					 tmpData |= 0x4;
         				}      	
         				if (at91_get_gpio_value(AT91_PIN_PB22))   /*YX4*/
         				{
         					 tmpData |= 0x8;
         				}         				      	
         				if (at91_get_gpio_value(AT91_PIN_PB17))   /*YX5*/
         				{
         					 tmpData |= 0x10;
         				}      	
         				if (at91_get_gpio_value(AT91_PIN_PB18))   /*YX6*/
         				{
         					 tmpData |= 0x20;
         				}
               	break;      	
               	
               case READ_GATEK_VALUE:           /*读取门控值*/
         				if (at91_get_gpio_value(AT91_PIN_PB21))
         				{
         					 tmpData |= 0x1;
         				}
         				break;
               	
               case DETECT_POWER_FAILURE:    /*停电检测*/
         				if (at91_get_gpio_value(AT91_PIN_PC15))
         				{
         					 tmpData = IO_HIGH;
         				}
         				else
         				{
         					 tmpData = IO_LOW;
         				}
               	break;
               	
         			case SET_ALARM_VOICE:         /*设置告警音*/
         				at91_set_gpio_value(AT91_PIN_PB24,arg);
         				break;
         				
         			case SET_BATTERY_ON:          /*设置后备电池通断*/
         				at91_set_gpio_value(AT91_PIN_PB19,arg);
         				break;
         		  
         		  case LOAD_CTRL_LINE_1:       /*负荷控制第一路*/
         				at91_set_gpio_value(AT91_PIN_PC4,arg);		  	
         		  	break;
         		  	
         		  case LOAD_CTRL_LINE_2:       /*负荷控制第二路*/
         				at91_set_gpio_value(AT91_PIN_PC8,arg);
         		  	break;
         
         		  case LOAD_CTRL_LINE_3:       /*负荷控制第3路*/
         				at91_set_gpio_value(AT91_PIN_PC6,arg);
         		  	break;
         
         		  case LOAD_CTRL_LINE_4:       /*负荷控制第4路*/
         				at91_set_gpio_value(AT91_PIN_PC10,arg);
         		  	break;
         		  	
         		  case LOAD_CTRL_CLOCK:        /*负荷控制时钟*/
         				at91_set_gpio_value(AT91_PIN_PA4,arg);
         		  	break;         		  	
         		  	
         			default:
         				break;
             	break;
            }
            break;
            
			  	case 6:    //集中器(控制单元硬件版本v0.4,CPU9260-QFP)
				    switch(cmd)
				    {
         			case READ_KEY_VALUE:   /*读取I/O脚的值*/
         				if (!at91_get_gpio_value(AT91_PIN_PC3))
                                                               //PA10
         				{
         					 tmpData = KEY_UP;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PA25))
         				{
         					 tmpData = KEY_DOWN;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PA11))
                                                              //PA22
         				{
         					 tmpData = KEY_LEFT;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PC1))
         				{
         					 tmpData = KEY_RIGHT;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PA5))
         				{
         					 tmpData = KEY_CANCEL;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PA30))
                                                              //PA8
         				{
         					 tmpData = KEY_OK;
         				}
         				break;
         
         			case READ_MODULE_TYPE:    //读取无线Modem类型
         				/*在控制单元硬件版本V0.4上,无线模块类型识别在xMega上*/
         				break;
         				
         			case WIRELESS_POWER_ON_OFF:   /*打开/关闭无线Modem电源*/
         				at91_set_gpio_value(AT91_PIN_PB30,arg);
         				break;
               
         			case WIRELESS_IGT:            /*无线Modem开关机,press key*/
         				at91_set_gpio_value(AT91_PIN_PA9,arg);
                                                          //PA11
         				break;
         
         			case WIRELESS_RESET:          /*无线Modem复位*/
         				at91_set_gpio_value(AT91_PIN_PA22,arg);
                                                         //PA9
         				break;
         
               case READ_YX_VALUE:           /*读取遥信值*/
         				if (at91_get_gpio_value(AT91_PIN_PA6))  /*YX1*/
         				{
         					 tmpData |= 0x1;
         				}
         				if (at91_get_gpio_value(AT91_PIN_PB31))   /*YX2*/
         				{
         					 tmpData |= 0x2;
         				}      	
               	break;
               	
               case DETECT_POWER_FAILURE:    /*停电检测*/
         				if (at91_get_gpio_value(AT91_PIN_PC15))
         				{
         					 tmpData = IO_HIGH;
         				}
         				else
         				{
         					 tmpData = IO_LOW;
         				}
               	break;
               	
         			case SET_ALARM_LIGHT:         /*设置告警灯*/
         				at91_set_gpio_value(AT91_PIN_PC2,arg);
                                                         //PB22 
         				break;
               	
         			case SET_ALARM_VOICE:         /*设置告警音*/
         				at91_set_gpio_value(AT91_PIN_PB12,arg);
                                                         //PB24
         				break;
         				
         			case SET_BATTERY_ON:          /*设置后备电池通断*/
         				at91_set_gpio_value(AT91_PIN_PB19,arg);
         				break;
         		  
               case SET_CARRIER_MODULE:     /*设置载波模块/set*/
         				at91_set_gpio_value(AT91_PIN_PA4,arg);
               	break;
               	
         		  case RST_CARRIER_MODULE:     /*复位载波模块*/
         				at91_set_gpio_value(AT91_PIN_PB28,arg);
         		  	break;
         		  	
         		  case SET_WATCH_DOG:          /**/
         		  	break;
         		  
         		  case ESAM_RST:
         		  	resetEsam();
         		  	break;
         		  
         		  case READ_SWITCH_KH:
         				if (at91_get_gpio_value(AT91_PIN_PA31))
                                                             //PA26
         				{
         					 tmpData = 0x1;
         				}
         		  	break;
         		  	
         			default:
         				break;
            }
            break;
            
          case 7:     //专变III型新终端(控制单元硬件版本v1.3,QFP)
          	switch(cmd)
          	{
         			case READ_KEY_VALUE:   /*读取I/O脚的值*/
         				if (!at91_get_gpio_value(AT91_PIN_PB21))
         				{
         					 tmpData = KEY_UP;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PA25))
         				{
         					 tmpData = KEY_DOWN;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PB29))
         				{
         					 tmpData = KEY_LEFT;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PB28))
         				{
         					 tmpData = KEY_RIGHT;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PA22))
         				{
         					 tmpData = KEY_CANCEL;
         				}
         				if (!at91_get_gpio_value(AT91_PIN_PA5))
         				{
         					 tmpData = KEY_OK;
         				}
         				break;
         
         			case READ_MODULE_TYPE:    //读取无线Modem类型
         				/*在控制单元硬件版本V1.3上,无线模块类型识别在xMega上*/
         				break;
         				
         			case WIRELESS_POWER_ON_OFF:   /*打开/关闭无线Modem电源*/
         				at91_set_gpio_value(AT91_PIN_PC1,arg);
         				break;
               
         			case WIRELESS_IGT:            /*无线Modem开关机,press key*/
         				at91_set_gpio_value(AT91_PIN_PA10,arg);
         				break;
         
         			case WIRELESS_RESET:          /*无线Modem复位*/
         				at91_set_gpio_value(AT91_PIN_PA9,arg);
         				break;
         
               case READ_YX_VALUE:           /*读取遥信值*/
         				if (at91_get_gpio_value(AT91_PIN_PA6))  /*YX1*/
         				{
         					 tmpData |= 0x1;
         				}
         				if (at91_get_gpio_value(AT91_PIN_PB31))   /*YX2*/
         				{
         					 tmpData |= 0x2;
         				}      	
         				if (at91_get_gpio_value(AT91_PIN_PA26))   /*YX3*/
         				{
         					 tmpData |= 0x4;
         				}      	
         				if (at91_get_gpio_value(AT91_PIN_PA8))    /*YX4*/
         				{
         					 tmpData |= 0x8;
         				}         				      	
         				if (at91_get_gpio_value(AT91_PIN_PB17))   /*YX5*/
         				{
         					 tmpData |= 0x10;
         				}      	
         				if (at91_get_gpio_value(AT91_PIN_PB18))   /*YX6*/
         				{
         					 tmpData |= 0x20;
         				}
               	break;      	
               	
               case READ_GATEK_VALUE:           /*读取门控值*/
         				if (at91_get_gpio_value(AT91_PIN_PB21))
         				{
         					 tmpData |= 0x1;
         				}
         				break;
               	
               case DETECT_POWER_FAILURE:    /*停电检测*/
         				if (at91_get_gpio_value(AT91_PIN_PC15))
         				{
         					 tmpData = IO_HIGH;
         				}
         				else
         				{
         					 tmpData = IO_LOW;
         				}
               	break;
               	
         			case SET_ALARM_VOICE:         /*设置告警音*/
         				at91_set_gpio_value(AT91_PIN_PB24,arg);
         				break;
         				
         			case SET_BATTERY_ON:          /*设置后备电池通断*/
         				at91_set_gpio_value(AT91_PIN_PB19,arg);
         				break;
         		  
         		  case LOAD_CTRL_LINE_1:       /*负荷控制第一路*/
         				at91_set_gpio_value(AT91_PIN_PA4,arg);
         		  	break;
         		  	
         		  case LOAD_CTRL_LINE_2:       /*负荷控制第二路*/
         				at91_set_gpio_value(AT91_PIN_PC8,arg);
         		  	break;
         
         		  case LOAD_CTRL_LINE_3:       /*负荷控制第3路*/
         				at91_set_gpio_value(AT91_PIN_PB16,arg);
         		  	break;
         
         		  case LOAD_CTRL_LINE_4:       /*负荷控制第4路*/
         				at91_set_gpio_value(AT91_PIN_PC10,arg);
         		  	break;
         		  	
         		  case LOAD_CTRL_CLOCK:        /*负荷控制时钟*/
         				at91_set_gpio_value(AT91_PIN_PC4,arg);
         		  	break;
         		  	
         		  case ESAM_RST:
         		  	resetEsam();
         		  	break;

         			default:
         				break;
             	break;
            }
            break;
        }
        break;
   }
   
	 return tmpData;
}

/**************************************************
函数名称:ioChannel_open
功能描述:打开
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static int ioChannel_open(struct inode *inode, struct file *file)
{
	return 0;
}

/**************************************************
函数名称:ioChannel_release
功能描述:释放
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static int ioChannel_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/**************************************************
函数名称:ioChannel_read
功能描述:读
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static ssize_t ioChannel_read(struct file *filp,char __user *buf,
		size_t size,loff_t *ppos)
{
	 int i;
	 unsigned char tmpBuf[500];

	 /*
	 esamError = 0;
	 if (buf[0]==0xf0)   //读取ESAM序列号
	 {
	 	 memcpy(buf, esamSerial,8);
	 }
	 else
	 {
	   for(i=0;i<size;i++)
	   {
	 	   buf[i] = receiveByte();
	   }
	 }
	 */
	 
	 if (buf[0]==88)   //读取ESAM序列号
	 {
	 	  memcpy(buf, esamSerial,8);
	 }
	 else
	 {
  	 memcpy(tmpBuf, &buf[2], buf[1]+(buf[0]>>7)*256);
     
  	 esamError = 0;
  	 
  	 if (esamDeguInfo)
  	 {
  	   printk("\n\nioChannel发送数据,长度=%d:", buf[1]+(buf[0]>>7)*256);
  	   for(i=0;i<buf[1]+(buf[0]>>7)*256;i++)
  	   {
  	 	    printk("%02x ", tmpBuf[i]);
  	   }
  	   printk("\n");
  	 }
  	 
  	 esamPutGet(tmpBuf, buf[1]+(buf[0]>>7)*256, buf[0]&0x7f, size);
  	 
  	 memcpy(buf, tmpBuf, size+2);
   }
	 
	 return esamError;
}

/**************************************************
函数名称:ioChannel_write
功能描述:写
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static ssize_t ioChannel_write(struct file *filp,const char __user *buf,
		size_t size,loff_t *ppos)
{
	int i;
	for(i=0;i<size;i++)
	{
		 sendByte(buf[i]);
	}
	
	return 0;
}

static struct file_operations ioChannel_fops = {
 .owner   = THIS_MODULE,
 .ioctl   = ioChannel_ioctl,
 .open    = ioChannel_open,
 .release = ioChannel_release,
 .read    = ioChannel_read,
 .write   = ioChannel_write,
};

/**************************************************
函数名称:ioChannel_setup_cdev
功能描述:注册动态设备名
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static void ioChannel_setup_cdev(void)
{
	 int error;
	 int devno, tmpMinor=0;
	 
   devno = MKDEV (ioChannelMajor, tmpMinor);
   
   cdev_init (&ioChannelCdev, &ioChannel_fops);
   
   ioChannelCdev.owner = THIS_MODULE;
   ioChannelCdev.ops = &ioChannel_fops;
   error = cdev_add(&ioChannelCdev, devno,1);

   if(error)
   {
     printk(KERN_NOTICE "Error %d adding char_reg_setup_cdev",error);
   }
}

/**************************************************
函数名称:ioChannel_init
功能描述:IO通道驱动初始化
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static int __init ioChannel_init(void)
{
	dev_t  devNo;     /*设备号*/
  
	/*****************************3.注册***********************************/
  /*自动分配一个设备号*/
	if (alloc_chrdev_region(&devNo,0,1,"ioChannel") < 0)
	{
		 return -2;
	}
	
	ioChannelMajor= MAJOR(devNo);

  ioChannel_setup_cdev();
  
  /* create your own class under /sysfs */
  classIoChannel = class_create(THIS_MODULE, "ioChannel");
  if(IS_ERR(classIoChannel))
  {
    printk("IO Channel Err: failed in creating class.\n");
    return -1;
  }

  device_create(classIoChannel, NULL, MKDEV(ioChannelMajor, 0), "ioChannel");

  /*这一段是timer rc中断例程
  //int    result;
  //unsigned int  dummy;
  //unsigned char lyy;  
  dummy = 0xff;
  result = request_irq(AT91SAM9260_ID_TC1, esam_interrupt, IRQF_SHARED, "esam_soft", &dummy);
  //result = request_irq(AT91SAM9260_ID_TC1, esam_interrupt, IRQF_TIMER, "esam_soft", &dummy);
	
	if (result)
	{
		printk("esam driver: Can't get irq\n");
	}
	else
	{
		 printk("esam driver:return result=%d\n", result);
	}
  
  clk_enable(&tc0_clk);
  
  tc0 = ioremap(AT91SAM9260_BASE_TC1, 0x40);
  
  writel(0x02, tc0+0x00);         //CCR, Disable the clock Counter
  writel(0xffffffff, tc0+0x28);   //IDR,禁止所有中断
  dummy = readl(tc0+0x20);

  writel(0x00 | 0x4000, tc0+0x04);  //CMR
  writel(0x01, tc0+0x00);  //CCR, Enabled the clock Counter
  
  writel(0x10, tc0+0x24);  //IER, validate the rc compare interrupt
  
  at91_sys_write(AT91_AIC_IDCR, 1<<AT91SAM9260_ID_TC1);
  
  at91_sys_write(AT91_AIC_SMR(AT91SAM9260_ID_TC1), 0x04 | 0x00);
  at91_sys_write(AT91_AIC_ICCR, 1<<AT91SAM9260_ID_TC1);
  at91_sys_write(AT91_AIC_IECR, 1<<AT91SAM9260_ID_TC1);
  
  //writel(0xFBC5, tc0+0x1c);  //RC
  writel(4, tc0+0x1c);  //RC
  writel(0x4, tc0+0x00);  //CCR

  */


	/***********************ESAM算法安全芯片接口初始化*****************/
  esamDeguInfo = 0;
	hasEsam = 0;

  //产生时钟
  clk_enable(&tc2_clk);

  tcBase = ioremap(AT91SAM9260_BASE_TC2, 0x40);
 
  writel(0x02, tcBase+AT91_TC_CCR);         //CCR, Disable the clock Counter
   
  //ESAM-CLOCK - PB7
 	if (  at91_set_A_periph(AT91_PIN_PC6, 0)!=0)
 	{
 	  printk(KERN_ALERT "ioChannel:set AT91_PIN_PC6 to A periph failed!\n");
 	}
 
  writel(TC_BSWTRG_SET_OUTPUT  |       /* BSWTRG : software trigger set TIOB */
          TC_BCPC_TOGGLE_OUTPUT |       /* BCPC : Register C compare toggle TIOB */
          TC_BCPB_TOGGLE_OUTPUT |       /* BCPB : Register B compare toggle TIOB */
          TC_ASWTRG_SET_OUTPUT  |       /* ASWTRG : software trigger set TIOA */
          TC_WAVE     |                 /* WAVE : Waveform mode */
          TC_CPCTRG   |                 /* CPCTRG : Register C compare trigger enable */
          TC_EEVT_XC0 |                 /* EEVT : XC0 as external event (TIOB=output) */
          TC_CLKS_MCK2                  /* TCCLKS : MCK / 2 */
          , tcBase+AT91_TC_CMR);        //CMR
   
  //writel(0x0c, tcBase+AT91_TC_RC);     //RC
  //writel(0x06, tcBase+AT91_TC_RB);     //RB
  
  writel(24, tcBase+AT91_TC_RC);       //RC
  writel(12, tcBase+AT91_TC_RB);       //RB
   
  writel(0x01, tcBase+AT91_TC_CCR);    //Enable the Clock counter
  writel(0x04, tcBase+AT91_TC_CCR);    // Trig the timer

  //检测是否是集中器上插入(Plugin)了ESAM芯片
  //ESAM-CLOCK - PB7,这个将来要去掉,因为现在CLOCK未直接引TIOB2,为了少搭线,将PB7与PC6相连,ly,2011-03-19
  if (at91_set_gpio_input(AT91_PIN_PB7, 0)!=0)
  {
    printk(KERN_ALERT "ioChannel:set AT91_PIN_PB9 Input failed!\n");
  }

  esamChipRst = AT91_PIN_PB21;
	esamChipIo  = AT91_PIN_PB6;
  
  if (resetEsam()==0x00)
  {
  	 hasEsam = 1;
  }
  else
  {
	  //不是集中器,则该脚要配置成串口1
	  at91_set_A_periph(AT91_PIN_PB6, 1);		/* TXD1 */
	  at91_set_A_periph(AT91_PIN_PB7, 0);		/* RXD1 */

    /*ESAM-CLOCK - PB9,这个将来要去掉,因为现在CLOCK未直接引TIOB2,为了少搭线,专变终端将PB9与PC6相连,ly,2011-03-19*/
   	if (at91_set_gpio_input(AT91_PIN_PB9, 0)!=0)
   	{
   	  printk(KERN_ALERT "ioChannel:set AT91_PIN_PB9 Input failed!\n");
   	}

    esamChipRst = AT91_PIN_PB20;
	  esamChipIo  = AT91_PIN_PB8;
	  
	  if (resetEsam()==0x00)
	  {
	  	 hasEsam = 1;
	  }
	  else
	  {
	    //也没有检测到是专变终端,则该脚要配置成串口2
	    at91_set_A_periph(AT91_PIN_PB8, 1);		/* TXD2 */
	    at91_set_A_periph(AT91_PIN_PB9, 0);		/* RXD2 */

	  	clk_disable(&tc2_clk);
	     	  	   
	  	printk("ESAM chip no found.\n");
	  }
  }

	/*
	//ESAM-DETECT(集中器) - PB20
	if (at91_set_gpio_input(AT91_PIN_PB20, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB20 Input failed!\n");
	}
	
	//ESAM-DETECT(专变终端) - PB22
	if (at91_set_gpio_input(AT91_PIN_PB22, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB22 Input failed!\n");
	}
	hasEsam = 0;
	
  //检测是否是集中器上插入(Plugin)了ESAM芯片
  if (at91_get_gpio_value(AT91_PIN_PB20))
  {
    //ESAM-CLOCK - PB7,这个将来要去掉,因为现在CLOCK未直接引TIOB2,为了少搭线,将PB7与PC6相连,ly,2011-03-19
   	if (at91_set_gpio_input(AT91_PIN_PB7, 0)!=0)
   	{
   	  printk(KERN_ALERT "ioChannel:set AT91_PIN_PB9 Input failed!\n");
   	}

    esamChipRst = AT91_PIN_PB21;
	  esamChipIo  = AT91_PIN_PB6;
	  hasEsam = 1;
  }
  else
  {
  	//检测是否是专变终端上插入(Plugin)了ESAM芯片
    if (at91_get_gpio_value(AT91_PIN_PB22))
    {
      //ESAM-CLOCK - PB9,这个将来要去掉,因为现在CLOCK未直接引TIOB2,为了少搭线,终端将PB9与PC6相连,ly,2011-03-19
   	  if (at91_set_gpio_input(AT91_PIN_PB9, 0)!=0)
   	  {
   	    printk(KERN_ALERT "ioChannel:set AT91_PIN_PB9 Input failed!\n");
   	  }

      esamChipRst = AT91_PIN_PB20;
	    esamChipIo  = AT91_PIN_PB8;
	    hasEsam = 1;
    }
  }

  if (hasEsam)
  {	  
	  if (at91_set_gpio_output(esamChipIo, 0)!=0)
	  {
	     printk(KERN_ALERT "ioChannel:set esamChipIo Output failed!\n");
	  }

	  if (at91_set_gpio_output(esamChipRst, 1)!=0)
	  {
	     printk(KERN_ALERT "ioChannel:set esamChipRst Output failed!\n");
	  }

	  printk("ESAM Chip PlugIn.");
	  
    clk_enable(&tc2_clk);

    tcBase = ioremap(AT91SAM9260_BASE_TC2, 0x40);
   
    writel(0x02, tcBase+AT91_TC_CCR);         //CCR, Disable the clock Counter
     
    //ESAM-CLOCK - PB7
   	if (  at91_set_A_periph(AT91_PIN_PC6, 0)!=0)
   	{
   	  printk(KERN_ALERT "ioChannel:set AT91_PIN_PC6 to A periph failed!\n");
   	}
   
    writel(TC_BSWTRG_SET_OUTPUT  |       // BSWTRG : software trigger set TIOB 
            TC_BCPC_TOGGLE_OUTPUT |       // BCPC : Register C compare toggle TIOB 
            TC_BCPB_TOGGLE_OUTPUT |       // BCPB : Register B compare toggle TIOB 
            TC_ASWTRG_SET_OUTPUT  |       // ASWTRG : software trigger set TIOA
            TC_WAVE     |                 // WAVE : Waveform mode
            TC_CPCTRG   |                 // CPCTRG : Register C compare trigger enable
            TC_EEVT_XC0 |                 // EEVT : XC0 as external event (TIOB=output)
            TC_CLKS_MCK2                  // TCCLKS : MCK / 2 
            , tcBase+AT91_TC_CMR);        //CMR
     
    //writel(0x0c, tcBase+AT91_TC_RC);     //RC
    //writel(0x06, tcBase+AT91_TC_RB);     //RB
    
    writel(24, tcBase+AT91_TC_RC);       //RC
    writel(12, tcBase+AT91_TC_RB);       //RB
     
    writel(0x01, tcBase+AT91_TC_CCR);    //Enable the Clock counter
    writel(0x04, tcBase+AT91_TC_CCR);    // Trig the timer
    
    resetEsam();
  }
  else
  {
  	 printk("ESAM not Plugin.\n");
  }
  */
	
	at91_set_gpio_output(AT91_PIN_PA4, 1);
	at91_set_gpio_output(AT91_PIN_PC8, 1);
	at91_set_gpio_output(AT91_PIN_PC12, 1);////PB16
	at91_set_gpio_output(AT91_PIN_PC10, 1);
	
	at91_set_gpio_output(AT91_PIN_PC4, 0);   //CLK

  printk (KERN_INFO "IO Channel: Init Done!\n");
  
  return 0;
}
 
/**************************************************
函数名称:ioChannel_exit
功能描述:IO通道驱动退出
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static void __exit ioChannel_exit(void)
{
	//unregister_chrdev(ioChannelMajor,DEVICE_NAME);

  dev_t devno = MKDEV (ioChannelMajor, 0);
  cdev_del (&ioChannelCdev);
      
  //delete device node under /dev
  device_destroy(classIoChannel, MKDEV(ioChannelMajor, 0)); 

  //delete class created by us
  class_destroy(classIoChannel);

  unregister_chrdev_region (devno, 1);
  
  printk(KERN_ALERT "ioChannel Driver removed.\n");
}

module_init(ioChannel_init);
module_exit(ioChannel_exit);

MODULE_LICENSE("Dual BSD/GPL");   /*should always exist or you’ll get a warning*/
MODULE_AUTHOR("Leiyong");        /*optional*/
MODULE_DESCRIPTION("IO_CHANNEL"); /*optional*/
MODULE_VERSION("V1.0");

