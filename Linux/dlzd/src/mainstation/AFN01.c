/***************************************************
Copyright,2006,LongTong co.,LTD,All	Rights Reserved
文件名：AFN01.c
作者：leiyong
版本：0.9
完成日期：2006年5月
描述：主站AFN01(复位)处理文件。
函数列表：
     1.
修改历史：
  01,06-05-26,Leiyong created.
  02,07-03-07,Leiyong修改,在23:55:00~00:01:00之间不允许终端复位,发否认，其余时间允许复位，发确认
            收到复位命令后，间隔2秒后动作，以便发送确认信息. 
  03,08-06-27,Leiyong修改,参数及数据区初始化及数据区初始化时不清除事件记录区,
              增加参数及数据区初始化事件F1 
***************************************************/
#include "teRunPara.h"
#include "msSetPara.h"
#include "workWithMS.h"
#include "msInput.h"

#include "AFN01.h"

/*******************************************************
函数名称:AFN01
功能描述:接收主站"复位命令"(AFN01)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void AFN01(INT8U *pDataHead, INT8U *pDataEnd ,INT8U dataFrom)
{
   INT8U  fn;
   void (*AFN01Fun[4])(INT8U dataFrom);

   AFN01Fun[0] = AFN01001;
   AFN01Fun[1] = AFN01002;
   AFN01Fun[2] = AFN01003;
   AFN01Fun[3] = AFN01004;

   //根据数据单元标识的值,查找FN，Pn值，确定操作函数号
   fn = findFnPn(*(pDataHead+2), *(pDataHead+3),FIND_FN);

   if (fn>=1 && fn<=4)
   {
     AFN01Fun[fn-1](dataFrom);
   }
   else
   {
     if (fn==88)
     {
   	   AFN01088(dataFrom);
   	 }
   	 else
   	 {
       ackOrNack(FALSE,dataFrom);	  //否认
   	 }
   }
}

/*******************************************************
函数名称:AFN01001(硬件初始化操作)
功能描述:
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/

void AFN01001(INT8U dataFrom)
{   
   ackOrNack(TRUE,dataFrom);	  //确认
   
   //硬件复位动作
   cmdReset = 1;                //等待2秒后复位
}


/*******************************************************
函数名称:AFN01002
功能描述:数据区初始化操作
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
void AFN01002(INT8U dataFrom)
{
   ackOrNack(TRUE, dataFrom);	//确认
   
   flagOfClearData = 0x55;
   flagOfClearPulse = 0x55;
}

/*******************************************************
函数名称:AFN01003
功能描述:参数及全体数据区初始化(即恢复至出厂配置)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
void AFN01003(INT8U dataFrom)
{  
   INT8U     i;
   
   ackOrNack(TRUE, dataFrom);	  //确认

   flagOfClearData = 0xaa;
   flagOfClearPulse = 0xaa;
}

/*******************************************************
函数名称:AFN01004
功能描述:参数(除与系统主站通信有关的)及全体数据初始化
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
void AFN01004(INT8U dataFrom)
{
   ackOrNack(TRUE,dataFrom);	         //确认
   
   flagOfClearData = 0xaa;
   flagOfClearPulse = 0xaa;
}

/*******************************************************
函数名称:AFN01088
功能描述:交采示值初始化
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
void AFN01088(INT8U dataFrom)
{
  ackOrNack(deleteAcVision(),dataFrom);	         //确认
}
