/*************************************************
Copyright,2010,Huawei WoDian co.,LTD
文件名：AFN0A.c
作者：wan guihua
版本：0.9
完成日期：2010年1月
描述：主站AFN0A(查询参数)处理文件。
函数列表：
  01,10-1-13,wan guihua created.
**************************************************/

#ifndef __INCAfn0AH
#define __INCAfn0AH

#include "common.h"

//函数声明
void AFN0A(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom);

//组1
INT16U AFN0A001(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A002(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A003(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A004(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A005(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A006(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A007(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A008(INT16U frameTail,INT8U *pHandle);

//组2
INT16U AFN0A009(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A010(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A011(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A012(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A013(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A014(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A015(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A016(INT16U frameTail,INT8U *pHandle);

//组3
INT16U AFN0A017(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A018(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A019(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A020(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A021(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A022(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A023(INT16U frameTail,INT8U *pHandle);

//组4
INT16U AFN0A025(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A026(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A027(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A028(INT16U frameTail,INT8U *pHandle);

INT16U AFN0A029(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A030(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A031(INT16U frameTail,INT8U *pHandle);

//组5
INT16U AFN0A033(INT16U frameTail,INT8U *pHandle);

INT16U AFN0A034(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A035(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A036(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A037(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A038(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A039(INT16U frameTail,INT8U *pHandle);

//组6
INT16U AFN0A041(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A042(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A043(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A044(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A045(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A046(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A047(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A048(INT16U frameTail,INT8U *pHandle);

//组7
INT16U AFN0A049(INT16U frameTail,INT8U *pHandle);

#ifdef LIGHTING
 INT16U AFN0A050(INT16U frameTail,INT8U *pHandle);
 INT16U AFN0A051(INT16U frameTail,INT8U *pHandle);
 INT16U AFN0A052(INT16U frameTail,INT8U *pHandle);
#endif

//组8
INT16U AFN0A057(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A058(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A059(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A060(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A061(INT16U frameTail,INT8U *pHandle);

//组9
INT16U AFN0A065(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A066(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A067(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A068(INT16U frameTail,INT8U *pHandle);

//组10
INT16U AFN0A073(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A074(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A075(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A076(INT16U frameTail,INT8U *pHandle);

//组11
INT16U AFN0A081(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A082(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A083(INT16U frameTail,INT8U *pHandle);

//规约扩展
#ifdef SDDL_CSM
 INT16U AFN0A088(INT16U frameTail,INT8U *pHandle);
 INT16U AFN0A224(INT16U frameTail,INT8U *pHandle); 
#endif
INT16U AFN0A097(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A098(INT16U frameTail,INT8U *pHandle);

INT16U AFN0A099(INT16U frameTail,INT8U *pHandle);
INT16U AFN0A100(INT16U frameTail,INT8U *pHandle);

INT16U AFN0A121(INT16U frameTail,INT8U *pHandle);  //终端地址及行政区划码
INT16U AFN0A133(INT16U frameTail,INT8U *pHandle);  //载波/无线主节点地址
INT16U AFN0A134(INT16U frameTail,INT8U *pHandle);  //设备编号
INT16U AFN0A135(INT16U frameTail,INT8U *pHandle);  //锐拔模块参数
INT16U AFN0A136(INT16U frameTail,INT8U *pHandle);  //厂商信息
INT16U AFN0A138(INT16U frameTail,INT8U *pHandle);  //厂商信息

#endif/*__INCAfn0AH*/
