/*************************************************
Copyright,2006,LongTong co.,LTD
文件名：AFN05.h
作者：leiyong
版本：0.9
完成日期:2006年6月 日
描述：主站“控制命令(AFN05)”头文件
修改历史：
  01,06-6-26,leiyong created.
**************************************************/
#ifndef __AFN05H
#define __AFN05H

#include "common.h"

extern INT8U  ctrlCmdWaitTime;              //收到控制命令显示停留时间


void AFN05(INT8U *pDataHead, INT8U *pDataEnd, INT8U dataFrom);

BOOL AFN05001(INT8U *pHandle, INT8U fn);
BOOL AFN05002(INT8U *pHandle, INT8U fn);

BOOL AFN05009(INT8U *pHandle, INT8U fn);
BOOL AFN05010(INT8U *pHandle, INT8U fn);
BOOL AFN05011(INT8U *pHandle, INT8U fn);
BOOL AFN05012(INT8U *pHandle, INT8U fn);
BOOL AFN05015(INT8U *pHandle, INT8U fn);
BOOL AFN05016(INT8U *pHandle, INT8U fn);

BOOL AFN05017(INT8U *pHandle, INT8U fn);
BOOL AFN05018(INT8U *pHandle, INT8U fn);
BOOL AFN05019(INT8U *pHandle, INT8U fn);
BOOL AFN05020(INT8U *pHandle, INT8U fn);
BOOL AFN05023(INT8U *pHandle, INT8U fn);
BOOL AFN05024(INT8U *pHandle, INT8U fn);
BOOL AFN05025(INT8U *pHandle, INT8U fn);
BOOL AFN05026(INT8U *pHandle, INT8U fn);
BOOL AFN05027(INT8U *pHandle, INT8U fn);
BOOL AFN05028(INT8U *pHandle, INT8U fn);
BOOL AFN05029(INT8U *pHandle, INT8U fn);
BOOL AFN05030(INT8U *pHandle, INT8U fn);    //终端投运(重庆规约)

BOOL AFN05031(INT8U *pHandle, INT8U fn);
BOOL AFN05032(INT8U *pHandle, INT8U fn);

BOOL AFN05033(INT8U *pHandle, INT8U fn);
BOOL AFN05034(INT8U *pHandle, INT8U fn);
BOOL AFN05035(INT8U *pHandle, INT8U fn);
BOOL AFN05036(INT8U *pHandle, INT8U fn);
BOOL AFN05037(INT8U *pHandle, INT8U fn);

BOOL AFN05040(INT8U *pHandle, INT8U fn);    //终端退出运行(重庆规约)

BOOL AFN05049(INT8U *pHandle, INT8U fn);
BOOL AFN05050(INT8U *pHandle, INT8U fn);
BOOL AFN05051(INT8U *pHandle, INT8U fn);
BOOL AFN05052(INT8U *pHandle, INT8U fn);
BOOL AFN05053(INT8U *pHandle, INT8U fn);
BOOL AFN05054(INT8U *pHandle, INT8U fn);    //命令启动搜表

BOOL AFN05066(INT8U *pHandle, INT8U fn);

#endif //__AFN05H