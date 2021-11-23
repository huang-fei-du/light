/***************************************************
Copyright,All	Rights Reserved
�ļ�����lcdGui.h
���ߣ�xiaolei
�汾��0.9
������ڣ�2009��12��
������LCD�û��ӿ�ͷ�ļ�

�޸���ʷ��
  01,09-12-18,leiyong created.
  02,10-04-20,leiyong add,128*64 LCD����
  03,10-07-11,leiyong add,320*240(RA8835������)LCD����.
***************************************************/

#ifndef __LCD_GUI_H__
#define __LCD_GUI_H__

#include "common.h"

#ifdef LCD_USE_128_64   //LCD
  #define DOT                       12    //���ֵ���--12����

  #define COL_OF_LCD               128    //LCD����
  #define ROW_OF_LCD                 8    //LCD����
  #define SCREEN_WIDTH             128    //��Ļ���
  #define SCREEN_HEIGHT             64    //��Ļ�߶�  
#endif

#ifdef LCD_USE_320_240_RA8835
  #define DOT                       16    //���ֵ���--16����

  #define COL_OF_LCD                40    //LCD����(40*8=320)
  #define ROW_OF_LCD               240    //LCD����
  #define SCREEN_WIDTH             320    //��Ļ���
  #define SCREEN_HEIGHT            240    //��Ļ�߶�
#endif

#ifdef LCD_USE_160_160_UC1698 
  #define DOT                       16    //���ֵ���--16����

  #define COL_OF_LCD                54    //LCD����(ÿ��3����2���ֽ�)
  #define ROW_OF_LCD               160    //LCD����

  #define SCREEN_WIDTH             160    //��Ļ���
  #define SCREEN_HEIGHT            160    //��Ļ�߶�
#endif

//ǰ��/����ɫ
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

