/***************************************************
Copyright,All	Rights Reserved
文件名：lcdGui.h
作者：xiaolei
版本：0.9
完成日期：2009年12月
描述：LCD用户接口头文件

修改历史：
  01,09-12-18,leiyong created.
  02,10-04-20,leiyong add,128*64 LCD处理
  03,10-07-11,leiyong add,320*240(RA8835控制器)LCD处理.
***************************************************/

#ifndef __LCD_GUI_H__
#define __LCD_GUI_H__

#include "common.h"

#ifdef LCD_USE_128_64   //LCD
  #define DOT                       12    //汉字点阵--12点阵

  #define COL_OF_LCD               128    //LCD列数
  #define ROW_OF_LCD                 8    //LCD行数
  #define SCREEN_WIDTH             128    //屏幕宽度
  #define SCREEN_HEIGHT             64    //屏幕高度  
#endif

#ifdef LCD_USE_320_240_RA8835
  #define DOT                       16    //汉字点阵--16点阵

  #define COL_OF_LCD                40    //LCD列数(40*8=320)
  #define ROW_OF_LCD               240    //LCD行数
  #define SCREEN_WIDTH             320    //屏幕宽度
  #define SCREEN_HEIGHT            240    //屏幕高度
#endif

#ifdef LCD_USE_160_160_UC1698 
  #define DOT                       16    //汉字点阵--16点阵

  #define COL_OF_LCD                54    //LCD列数(每列3个点2个字节)
  #define ROW_OF_LCD               160    //LCD行数

  #define SCREEN_WIDTH             160    //屏幕宽度
  #define SCREEN_HEIGHT            160    //屏幕高度
#endif

//前景/背景色
#define   BK_COLOR                   0
#define   TEXT_COLOR                 1

void lcdInit(void);
void lcdClearScreen(void);
void lcdBackLight(INT8U onOff);
void lcdRefresh(INT8U startLine,INT8U endLine);
void guiLine(INT16U x1, INT16U y1, INT16U x2, INT16U y2, INT16U color);
void guiDisplay(INT16U x, INT16U y, const char *s, INT8U color);
void guiAscii(INT16U x, INT16U y, const char *s,INT8U color);
void guiStart(INT16U x, INT16U y,INT8U color);

#endif   //__LCD_GUI_H__

