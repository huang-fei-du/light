/*************************************************
Copyright,2010,Huawei Wodian co.,LTD
文件名：AFN09.h
作者：leiyong
版本：0.9
完成日期:2010年3月 日
描述：主站"请求终端配置及信息(AFN09)"头文件
修改历史：
  01,10-03-23,leiyong created.
**************************************************/
#ifndef __AFN09H
#define __AFN09H

#include "common.h"

//函数声明

//组1
INT16U AFN09001(INT16U frameTail,INT8U *pHandle, INT8U fn);
INT16U AFN09002(INT16U frameTail,INT8U *pHandle, INT8U fn);
INT16U AFN09003(INT16U frameTail,INT8U *pHandle, INT8U fn);
INT16U AFN09004(INT16U frameTail,INT8U *pHandle, INT8U fn);
INT16U AFN09005(INT16U frameTail,INT8U *pHandle, INT8U fn);
INT16U AFN09006(INT16U frameTail,INT8U *pHandle, INT8U fn);
INT16U AFN09007(INT16U frameTail,INT8U *pHandle, INT8U fn);
INT16U AFN09008(INT16U frameTail,INT8U *pHandle, INT8U fn);

#ifdef SDDL_CSM
 INT16U AFN09011(INT16U frameTail,INT8U *pHandle, INT8U fn);
#endif

#endif  /*__AFN09H*/
