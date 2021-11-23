/***************************************************
Copyright,All	Rights Reserved
文件名：lcdGui.c
作者：xiaolei
版本：0.9
完成日期：2009年12月
描述：LCD用户接口处理文件
函数列表：
     1.
修改历史：
  01,09-12-18,leiyong created.
  02,10-04-20,leiyong add,128*64 LCD处理.
  03,10-07-11,leiyong add,320*240(RA8835控制器)LCD处理.
***************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "teRunPara.h"
#include "lattice.h"

#include "lcdGui.h"

//显存
#ifdef LCD_USE_128_64
  static INT8U guiScreen[ROW_OF_LCD][COL_OF_LCD];
#endif

#ifdef LCD_USE_320_240_RA8835
  static INT8U guiScreen[ROW_OF_LCD][COL_OF_LCD];
#endif

#ifdef LCD_USE_160_160_UC1698
  static INT8U guiScreen[ROW_OF_LCD][COL_OF_LCD*2];
#endif

int          fdOfLcd;                              //LCD文件操作符

/*******************************************************
函数名称:lcdInit
功能描述:初始化显示器
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
void lcdInit(void)
{
    INT8U line, col;
    INT8U buf[5];
    
    //清零缓存
   #ifdef LCD_USE_320_240_RA8835
    for (line=0; line<ROW_OF_LCD; line++)
    {
       for (col=0; col<COL_OF_LCD; col++)
       {
          guiScreen[line][col] = 0x00;
       }
    }
   #else
    for (line=0; line<ROW_OF_LCD; line++)
    {
      #ifdef LCD_USE_128_64
       for (col=0; col<COL_OF_LCD; col++)
       {
      #else
       for (col=0; col<COL_OF_LCD*2; col++)
       {
      #endif
          guiScreen[line][col] = 0x00;
       }
    }
   #endif
    
	  #ifdef YONGCHUANG_OLD_JZQ
	   fdOfLcd = open("/dev/lcdUc1698",O_RDWR);
	  #else	  
	   fdOfLcd = open("/dev/parallelBus",O_RDWR);
	  #endif
	  
	  if (fdOfLcd == -1)
	  {
		   printf("Unable to open parallelBus.\n\r");
		   return;
	  }

    //打开操作LCD文件
   #ifdef LCD_USE_128_64
	  ioctl(fdOfLcd,88,2);     //显示器为12864
	 #else
	  #ifdef LCD_USE_320_240_RA8835
	   ioctl(fdOfLcd,88,4);    //显示器为320*240
	  #else
	   #ifdef PLUG_IN_CARRIER_MODULE
	    #ifdef CPU_912_9206_QFP
	     ioctl(fdOfLcd,88,5);   //专变III型终端,显示器为默认uc1698u 160160(CPU-QFP)
	    #else
	     ioctl(fdOfLcd,88,1);   //集中器,显示器为默认uc1698u 160160	  
	    #endif
	   #else
	    #ifdef CPU_912_9206_QFP
	     ioctl(fdOfLcd,88,5);   //专变III型终端,显示器为默认uc1698u 160160(CPU-QFP)
	    #else
	     ioctl(fdOfLcd,88,3);   //专变III型终端,显示器为默认uc1698u 160160
	    #endif
	   #endif
	   lcdAdjustDegree(lcdDegree);  //对比度
	  #endif
	 #endif
	 
	  ioctl(fdOfLcd,1,0);          //清屏
	  
	  if (debugInfox&WATCH_RESTART)
	  {
	    lcdBackLight(LCD_LIGHT_OFF);  //LCD背光灭
	  }
	  else
	  {
	    lcdBackLight(LCD_LIGHT_ON);  //LCD背光亮
	  }
}

/*******************************************************
函数名称:lcdClearScreen
功能描述:清屏
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
void lcdClearScreen(void)
{
	 ioctl(fdOfLcd,1,0);
}

/*******************************************************
函数名称:lcdBackLight
功能描述:LCD背光控制
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
void lcdBackLight(INT8U onOff)
{
	 if (onOff==LCD_LIGHT_OFF)
	 {
	   ioctl(fdOfLcd, 4, 0);
	 }
	 else
	 {
	   ioctl(fdOfLcd, 4, 1);	 	 
	 }
}

/*******************************************************
函数名称:lcdRefresh
功能描述:LCD刷新
         整个显示器(160*160)共160行,因此刷新时可部分刷新
调用函数:
被调用函数:
输入参数:startLine起始行(1--160),endLine结束行(1--160)
输出参数:无
返回值：void
*******************************************************/
void lcdRefresh(INT8U startLine,INT8U endLine)
{
   INT8U  line, col, i;
    
   if (startLine>ROW_OF_LCD || endLine>ROW_OF_LCD)
   {
    	return;
   }
    
   for (line=startLine-1; line<endLine; line++)
   {
      ioctl(fdOfLcd,2,line);     //设置行
      ioctl(fdOfLcd,3,0);        //设置列
       
      #ifdef LCD_USE_128_64
       write(fdOfLcd,&guiScreen[line],COL_OF_LCD);
      #endif
      
      #ifdef LCD_USE_160_160_UC1698
       write(fdOfLcd,&guiScreen[line],COL_OF_LCD*2);
      #endif
      
      #ifdef LCD_USE_320_240_RA8835
       write(fdOfLcd, &guiScreen[line], 40);
      #endif
   }
}

/*******************************************************
函数名称:guiLine
功能描述:画线
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
void guiLine(INT16U x1, INT16U y1, INT16U x2, INT16U y2, INT16U color)
{
    INT16U row, col, row1, row2, col1, col2;
    INT8U  i,tmpData;
   
   #ifdef LCD_USE_128_64
    row1 = y1;
    row2 = y2;
    col1 = x1;
    col2 = x2;
    if (row1<1 || row1>SCREEN_HEIGHT)
  	   return;
    if (row2<1 || row2>SCREEN_HEIGHT)
  	   return;
    if (col1<1 || col1>SCREEN_WIDTH)
  	   return;
    if (col2<1 || col2>SCREEN_WIDTH)
  	   return;
    if (row1>row2)
    {
  	   row1=x2;
  	   row2=x1;
    }
    if (col1>col2)
    {
    	 col1=y2;
    	 col2=y1;
    }
    row1--;
    row2--;
    col1--;
    col2--;
    for (row=row1; row<=row2; row++)
    {
       for (col=col1; col<=col2; col++)
       {         
         if (color==1)
         {
           guiScreen[row/8][col] |= color<<(row%8);
         }
         else
         {
           guiScreen[row/8][col] = color<<(row%8);
         }
       }
    }
   #endif
   
   #ifdef LCD_USE_160_160_UC1698
    row1 = y1;
    row2 = y2;
    col1 = x1;
    col2 = x2;
    if (row1<1 || row1>SCREEN_HEIGHT)
  	   return;
    if (row2<1 || row2>SCREEN_HEIGHT)
  	   return;
    if (col1<1 || col1>SCREEN_WIDTH)
  	   return;
    if (col2<1 || col2>SCREEN_WIDTH)
  	   return;
    if (row1>row2)
    {
  	   row1=x2;
  	   row2=x1;
    }
    if (col1>col2)
    {
    	 col1=y2;
    	 col2=y1;
    }
    row1--;
    row2--;
    for (row=row1; row<=row2; row++)
    {
       for (col=col1; col<=col2; col++)
       {
         if (color==1)
         {
           if ((col%3)==1)
           {
              guiScreen[row][col/3*2] = 0xf8;
           }
           if ((col%3)==2)
           {
              guiScreen[row][col/3*2] |= 0x07;
              guiScreen[row][col/3*2+1] = 0xe0;
           }
           if ((col%3)==0)
           {
              guiScreen[row][col/3*2-1] |= 0x1f;
           }
         }
         else
         {
           if ((col%3)==1)
           {
              guiScreen[row][col/3*2] &= 0x07;
           }
           if ((col%3)==2)
           {
              guiScreen[row][col/3*2] &= 0xf8;
              guiScreen[row][col/3*2+1] &= 0x1f;
           }
           if ((col%3)==0)
           {
              guiScreen[row][col/3*2-1] &= 0xe0;
           }           
         }
       }
    }
   #endif
   
   #ifdef LCD_USE_320_240_RA8835
    row1 = y1;
    row2 = y2;
    col1 = x1;
    col2 = x2;
    if (row1<1 || row1>SCREEN_HEIGHT)
  	   return;
    if (row2<1 || row2>SCREEN_HEIGHT)
  	   return;
    if (col1<1 || col1>SCREEN_WIDTH)
  	   return;
    if (col2<1 || col2>SCREEN_WIDTH)
  	   return;
  	   
    if (row1>row2)
    {
  	   row1=x2;
  	   row2=x1;
    }
    if (col1>col2)
    {
    	 col1=y2;
    	 col2=y1;
    }
    row1--;
    row2--;
    col1--;
    col2--;
    for (row=row1; row<=row2; row++)
    {
       for (col=col1; col<=col2; col++)
       {
         if (color==1)
         {
           guiScreen[row][col/8] |= color<<(7-col%8);
         }
         else
         {
           guiScreen[row][col/8] &= ~(1<<(7-col%8));
           guiScreen[row][col/8] &= ~(color<<(7-col%8));
         }
       }
    }
   #endif
}

/*******************************************************
函数名称:guiDisplay
功能描述:显示16点阵汉字或西文字符
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
void guiDisplay(INT16U x, INT16U y, const char *s, INT8U color)
{
    FILE          *fp;    

    INT16U        shift;          //显示移位比较因子
    INT16U        row, col;       //LCD显示器的行、列
    unsigned long offset;         //汉字在字库中的字库中的偏移
    INT8U         *offsetAscii;   //西字在字库中的偏移
    INT16U        tmpChar;
    INT8U         i,j;
    INT8U         wordbuf[32];
    
   #ifdef LCD_USE_128_64
    INT16U        tmpData[13];
    INT8U         tmpCol;

    //超出LCD显示范围,不显示
    if (y<1 || y>SCREEN_HEIGHT  || x<1 || x>SCREEN_WIDTH)
    {
       return;
    }    
    if (y+DOT > SCREEN_HEIGHT+1)
    {
      return;
    }

    if((fp=fopen("HZK12","rb"))==NULL)
    {
       printf("Can't open hzk12!\n");
       return;
    }
    
    row = y-1;   //行
    col = x-1;   //列
    
    while (*s)
    {
      for (j=0; j<13; j++)
      {
        tmpData[j] = 0;
      }
      tmpCol = 0;
      if((*(s)&0x80)!=0)                 //汉字
      {
         offset = (94*(*s-0xa1)+(*(s+1)-0xa1))*24L; //计算该汉字在字库中偏移量
         s+=2;

         //读出字模数据
         fseek(fp,offset,SEEK_SET);
         fread(wordbuf,32,1,fp);
         
         if (col+DOT > SCREEN_WIDTH+1)
         {
            break;
         }
         
         for (i=0; i<DOT; i++)
         {
            shift = 0x80;
            tmpChar = wordbuf[i*2];
            for (j=0; j<8; j++)
            {
               if ((tmpChar & shift)==0)
               {
                 tmpData[j] |= !color<<i;
               }
               else
               {
                 tmpData[j] |= color<<i;
               }
               shift = shift>>1;
            }

            tmpChar = wordbuf[i*2+1];
            shift = 0x80;
            for (j=8; j<12; j++)
            {
               if ((tmpChar & shift)==0)
               {
                 tmpData[j] |= !color<<i;
               }
               else
               {
                 tmpData[j] |= color<<i;
               }
               shift = shift>>1;
            }
            
            if (color==0)
            {
              tmpData[12] |= !color<<i;   //空一列
            }
         }         
         
         //空一行
         for (i=0; i<13; i++)
         {
           tmpData[i] |= !color<<12;
         }

         if (color==1)
         {
           tmpCol = DOT;
         }
         else
         {
           tmpCol = DOT+1;
         }         
      }
      else   //显示西文字符
      {
         if (col+5 > SCREEN_WIDTH+1)
         {
            break;
         }
         
         offsetAscii =ASCII + (*s-0x20)*8;
         
         s++;
       
         for(j=0;j<5;j++)
         {
         	 tmpData[j] = !color | !color<<1 | !color<<2 | !color<<11 | !color<<12;
         }
         for (i=3; i<11; i++)
         {
           shift = 0x80;
           tmpChar = *offsetAscii++;
           for (j=0; j<5; j++)
           {
              if ((tmpChar & shift)==0)
              {
                tmpData[j] |= !color<<i;
              }
              else
              {
                tmpData[j] |= color<<i;
              }

              shift = shift>>1;
           }
         }

         tmpCol = 5;
      }

      if (row%8==0)
      {
   	     for(j=0;j<tmpCol;j++)
   	     {
   	  	   guiScreen[row/8][col+j] |= tmpData[j]&0xff;
   	  	   guiScreen[row/8+1][col+j] |= tmpData[j]>>8&0x1f;
   	     }
      }
      else
      {
      	 for(j=0;j<tmpCol;j++)
      	 {
      	   guiScreen[row/8][col+j] |= (tmpData[j]<<(row%8))&0xff;
      	   guiScreen[row/8+1][col+j] |= (tmpData[j]>>(8-row%8))&0xff;
      	   if (row%8>4)
      	   {
      	     guiScreen[row/8+2][col+j] |= tmpData[j]>>(16-(row%8));
      	   }
      	 }
      }
      
      col += tmpCol;
    }
   #endif
   
   #ifdef LCD_USE_160_160_UC1698 
    //超出LCD显示范围,不显示
    if (y<1 || y>SCREEN_HEIGHT  || x<1 || x>SCREEN_WIDTH)
    {
       return;
    }    
    if (y+DOT > SCREEN_HEIGHT+1)
    {
      return;
    }
    
    if((fp=fopen("HZK16","rb"))==NULL)
    {
       printf("Can't open hzk16!\n");
       return;
    }

    row = y-1;   //行
    col = x;   //列
    
    while (*s)
    {
      if((*(s)&0x80)!=0)                 //汉字
      {
         offset = (94*(*s-0xa1)+(*(s+1)-0xa1))*32L; //计算该汉字在字库中偏移量
         s+=2;
         
         //读出字模数据
         fseek(fp,offset,SEEK_SET);
         fread(wordbuf,32,1,fp);

         if (col+DOT > SCREEN_WIDTH+1)
         {
            break;
         }
         
         col += DOT;
         for (i=0; i<DOT; i++)
         {        
            col -= DOT;
            
            shift = 0x8000;
            tmpChar = (wordbuf[i*2]<<8) | wordbuf[i*2+1];
            for (j=0; j<16; j++)
            {
               if (((tmpChar & shift)==0 && !color)
               	   || ((tmpChar & shift)!=0 && color))
               {
                 if ((col%3)==1)
                 {
                   guiScreen[row+i][col/3*2] = 0xf8;
                 }
                 if ((col%3)==2)
                 {
                   guiScreen[row+i][col/3*2] |= 0x07;
                   guiScreen[row+i][col/3*2+1] = 0xe0;
                 }
                 if ((col%3)==0)
                 {
                   guiScreen[row+i][col/3*2-1] |= 0x1f;
                 }
               }
               shift = shift>>1;
               
               col++;
            }
         }
      }
      else   //显示西文字符
      {
         if (col+8 > SCREEN_WIDTH+1)
         {            
            break;
         }
         
         offsetAscii = (unsigned char *)(ASCII + (*s-0x20)*16);
         s++;
         
         col+=8;
         for (i=0; i<16; i++)
         {
           col -= 8;
           shift = 0x80;
           tmpChar = *offsetAscii++;
           for (j=0; j<8; j++)
           {
              if (((tmpChar & shift)==0 && !color)
                 || ((tmpChar & shift)!=0 && color))
              {
                 if ((col%3)==1)
                 {
                   guiScreen[row+i][col/3*2] = 0xf8;
                 }
                 if ((col%3)==2)
                 {
                   guiScreen[row+i][col/3*2] |= 0x07;
                   guiScreen[row+i][col/3*2+1] = 0xe0;
                 }
                 if ((col%3)==0)
                 {
                   guiScreen[row+i][col/3*2-1] |= 0x1f;
                 }
              }

              shift = shift>>1;
              
              col++;
           }
         }
      }
    }
   #endif
   
   #ifdef LCD_USE_320_240_RA8835 
    //超出LCD显示范围,不显示
    if (y<1 || y>SCREEN_HEIGHT  || x<1 || x>SCREEN_WIDTH)
    {
       return;
    }    
    if (y+DOT > SCREEN_HEIGHT+1)
    {
      return;
    }
    
    if((fp=fopen("HZK16","rb"))==NULL)
    {
       printf("Can't open hzk16!\n");
       return;
    }

    row = y-1;   //行
    col = x;     //列
    
    while (*s)
    {
      if((*(s)&0x80)!=0)                 //汉字
      {
         offset = (94*(*s-0xa1)+(*(s+1)-0xa1))*32L; //计算该汉字在字库中偏移量
         s+=2;
         
         //读出字模数据
         fseek(fp,offset,SEEK_SET);
         fread(wordbuf,32,1,fp);

         if (col+DOT > SCREEN_WIDTH+1)
         {
            break;
         }
         
         col += DOT;
         for (i=0; i<DOT; i++)
         {
            col -= DOT;
            
            shift = 0x8000;
            tmpChar = (wordbuf[i*2]<<8) | wordbuf[i*2+1];
            for (j=0; j<16; j++)
            {
               if (((tmpChar & shift)==0 && !color)
               	   || ((tmpChar & shift)!=0 && color))
               {
                 guiScreen[row+i][col/8] |= 1<<(7-col%8);
               }
               else
               {
                 guiScreen[row+i][col/8] &= ~(1<<(7-col%8));
               }
               
               shift = shift>>1;
               col++;
            }
         }
      }
      else   //显示西文字符
      {
         if (col+8 > SCREEN_WIDTH+1)
         {            
            break;
         }
         
         offsetAscii = (unsigned char *)(ASCII + (*s-0x20)*16);
         s++;
         
         col+=8;
         for (i=0; i<16; i++)
         {
           col -= 8;
           shift = 0x80;
           tmpChar = *offsetAscii++;
           for (j=0; j<8; j++)
           {
              if (((tmpChar & shift)==0 && !color)
                 || ((tmpChar & shift)!=0 && color))
              {
                 guiScreen[row+i][col/8] |= 1<<(7-col%8);
              }
              else
              {
                 guiScreen[row+i][col/8] &= ~(1<<(7-col%8));
              }

              shift = shift>>1;
              
              col++;
           }
         }
      }
    }
   #endif
    fclose(fp);
}

/*******************************************************
函数名称:guiAscii
功能描述:显示5x8点阵西文字符
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
void guiAscii(INT16U x, INT16U y, const char *s,INT8U color)
{
    unsigned char shift;
    INT16U row, col, i, j;
    const unsigned char *offset;
    unsigned char tmpChar;
    
   #ifdef LCD_USE_128_64
    
    unsigned char tmpData[5];
   
    //超出LCD显示范围,不显示
    if (y<1 || y>SCREEN_HEIGHT || x<1 || x>SCREEN_WIDTH)
    {
       return;
    }
    row = y-1;
    col = x-1;

    while(*s)
    {
      if ((row+8 > SCREEN_HEIGHT+1) || (col+5 > SCREEN_WIDTH+1))
      {
         break;
      }
                     
      offset = (unsigned char *)(ASCII + (*s-0x20)* 8);
      s++;
      for(j=0;j<5;j++)
      {
        tmpData[j] = 0;
      }
      for (i=0; i<8; i++)
      {
         shift = 0x80;
         tmpChar = *offset++;
         for (j=0; j<5; j++)
         {
            if ((tmpChar & shift)==0)
            {
              tmpData[j] |= !color<<i;
            }
            else
            {
              tmpData[j] |= color<<i;
            }
            
            shift = shift>>1;
         }
      }
      
      if (row%8==0)
      {
      	 for(j=0;j<5;j++)
      	 {
      	  	guiScreen[row/8][col+j] = tmpData[j];
      	 }
      }
      else
      {
      	 for(j=0;j<5;j++)
      	 {
      	   guiScreen[row/8][col+j] |= tmpData[j]<<(row%8);
      	   guiScreen[row/8+1][col+j] |= tmpData[j]>>(8-(row%8));
      	 }
      }
      
      col += 6;
    }
   #endif
   
   #ifdef LCD_USE_160_160_UC1698
    //超出LCD显示范围,不显示
    if (y<1 || y>SCREEN_HEIGHT || x<1 || x>SCREEN_WIDTH)
    {
       return;
    }
    row = y-1;
    col = x;

    while(*s)
    {
      if ((row+16 > SCREEN_HEIGHT+1) || (col+8 > SCREEN_WIDTH+1))
      {
         break;
      }
                     
      offset = (unsigned char *)(ASCII + (*s-0x20)* 16);
      s++;
      
      col+=8;
      for (i=0; i<16; i++)
      {
        col -= 8;
        shift = 0x80;
        tmpChar = *offset++;
        for (j=0; j<8; j++)
        {
           if (((tmpChar & shift)==0 && !color)
              || ((tmpChar & shift)!=0 && color))
           {
              if ((col%3)==1)
              {
                guiScreen[row+i][col/3*2] = 0xf8;
              }
              if ((col%3)==2)
              {
                guiScreen[row+i][col/3*2] |= 0x07;
                guiScreen[row+i][col/3*2+1] = 0xe0;
              }
              if ((col%3)==0)
              {
                guiScreen[row+i][col/3*2-1] |= 0x1f;
              }
           }

           shift = shift>>1;
           
           col++;
        }
      }
    }
   #endif
   
   #ifdef LCD_USE_320_240_RA8835
    //超出LCD显示范围,不显示
    if (y<1 || y>SCREEN_HEIGHT || x<1 || x>SCREEN_WIDTH)
    {
       return;
    }
    row = y-1;
    col = x;

    while(*s)
    {
      if ((row+16 > SCREEN_HEIGHT+1) || (col+8 > SCREEN_WIDTH+1))
      {
         break;
      }
                     
      offset = (unsigned char *)(ASCII + (*s-0x20)* 16);
      s++;
      
      col+=8;
      for (i=0; i<16; i++)
      {
        col -= 8;
        shift = 0x80;
        tmpChar = *offset++;
        for (j=0; j<8; j++)
        {
           if (((tmpChar & shift)==0 && !color)
              || ((tmpChar & shift)!=0 && color))
           {
              guiScreen[row+i][col/8] |= 1<<(7-col%8);
           }
           else
           {
              guiScreen[row+i][col/8] &= ~(1<<(7-col%8));
           }

           shift = shift>>1;
           
           col++;
        }
      }
    }
   #endif
}

/*******************************************************
函数名称:guiStart
功能描述:显示开机大字(24*24)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
*******************************************************/
void guiStart(INT16U x, INT16U y,INT8U color)
{
    char                say[10];
    INT32U              shift;
    INT8U               row, col, i, j, k;
    const unsigned char *offset;
    INT32U              tmpChar;
    INT32U              tmpData[24];
    
    //超出LCD显示范围,不显示
    if (y<1 || y>160 || x<1 || x>160)
    {
       return;
    }
    row = y;
    col = x;
    
    j=0;
    
   #ifdef LCD_USE_128_64
    while(j<2)
    {
        if ((row+8 > 64+1) || (col+5 > 128+1))
        {
           break;
        }
        
        for(i=0;i<24;i++)
        {
        	 tmpData[i] = 0;
        }

        offset = (unsigned char *)(START + j*72);
        
        for (i=0; i<24; i++)
        {
           shift = 0x80;
           tmpChar = *offset++;
           
           for (k=1; k<9; k++)
           {
             if ((tmpChar & shift)==0)
             {
               tmpData[k] |= !color<<i;
             }
             else
             {
               tmpData[k] |= color<<i;
             }
               
             shift = shift>>1;
           }

           tmpChar = *offset;
           offset++;
           shift = 0x80;                
           for (k=9; k<17; k++)
           {
              if ((tmpChar & shift)==0)
              {
                tmpData[k] |= !color<<i;
              }
              else
              {
                tmpData[k] |= color<<i;
              }
              shift = shift>>1;
           }
           
           tmpChar = *offset;
           offset++;
           shift = 0x80;
           for (k=17; k<25; k++)
           {
               if ((tmpChar & shift)==0)
               {
                  tmpData[k] |= !color<<i;
               }
               else
               {
                  tmpData[k] |= color<<i;
               }
               
               shift = shift>>1;
           }
        }
        
        if (row%8==0)
        {
     	    for(i=0;i<24;i++)
     	    {
     	  	  guiScreen[row/8][col+i] |= tmpData[i]&0xff;
     	  	  guiScreen[row/8+1][col+i] |= tmpData[i]>>8&0xff;
     	  	  guiScreen[row/8+2][col+i] |= tmpData[i]>>16&0xff;
     	    }
        }
        else
        {
        	 for(i=0;i<24;i++)
        	 {
        	   guiScreen[row/8][col+i] |= (tmpData[i]<<(row%8))&0xff;
        	   guiScreen[row/8+1][col+i] |= (tmpData[i]>>(8-row%8))&0xff;
        	   guiScreen[row/8+2][col+i] |= (tmpData[i]>>(16-(row%8)))&0xff;
        	   guiScreen[row/8+3][col+i] |= tmpData[i]>>(24-(row%8));
        	 }
        }
        
        col += 35;
        j++;
    }

    strcpy(say,vers);
    guiDisplay(93, 32, say,1);
    guiDisplay(16, 2, "电力负荷管理系统",1);
    guiDisplay(28, 50, "华伟沃电科技",1); //公司标志    
 	  
 	  lcdRefresh(1, 8);
   #else
    #ifdef LCD_USE_320_240_RA8835
    #else
      #ifdef PLUG_IN_CARRIER_MODULE
       while(j<3)
      #else
       j=3;
       while(j<5)
      #endif
      {
          if ((row+8 > 160+1) || (col+5 > 160+1))
          {
              break;
          }
  
          offset = (unsigned char *)(START + j*288);
          
          col += 48;
          for (i=0; i<48; i++)
          {
             col -= 48;
             shift = 0x800000;
             tmpChar = *offset<<16 | (*(offset+1)<<8) | (*(offset+2));
             offset += 3;
             for (k=1; k<25; k++)
             {
               if ((tmpChar & shift)==0)
               {
                  //tmpData[k] |= !color<<i;
               }
               else
               {
                  if ((col%3)==1)
                  {
                     guiScreen[row+i][col/3*2] = 0xf8;
                  }
                  if ((col%3)==2)
                  {
                     guiScreen[row+i][col/3*2] |= 0x07;
                     guiScreen[row+i][col/3*2+1] = 0xe0;
                  }
                  if ((col%3)==0)
                  {
                     guiScreen[row+i][col/3*2-1] |= 0x1f;
                  }
               }
  
               shift = shift>>1;
               
               col++;
             }
  
             shift = 0x800000;
             tmpChar = *offset<<16 | (*(offset+1)<<8) | (*(offset+2));
             offset += 3;
             for (k=1; k<25; k++)
             {
               if ((tmpChar & shift)==0)
               {
                  //tmpData[k] |= !color<<i;
               }
               else
               {
                  if ((col%3)==1)
                  {
                     guiScreen[row+i][col/3*2] = 0xf8;
                  }
                  if ((col%3)==2)
                  {
                     guiScreen[row+i][col/3*2] |= 0x07;
                     guiScreen[row+i][col/3*2+1] = 0xe0;
                  }
                  if ((col%3)==0)
                  {
                     guiScreen[row+i][col/3*2-1] |= 0x1f;
                  }
               }
  
               shift = shift>>1;
               
               col++;
             }
          }
          
          j++;
      }
      strcpy(say,vers);
      
     #ifdef CQDL_CSM
      strcat(say, ".1");
      guiDisplay(111, 100, say,1);              //软件版本
     #else
      guiDisplay(125, 100, say,1);              //软件版本
     #endif      
     
     #ifdef LIGHTING
      guiDisplay(1, 20, "智能照明节能管理系统",1);
     #else
      guiDisplay(17, 20, "用电信息采集系统",1);
     #endif
      
     #ifdef CQDL_CSM
      guiDisplay(49, 130, "重庆华伟",1);      //公司标志
     #else
      memcpy(say, csNameId, 8);
      say[8] = '\0';
      guiDisplay(49, 130, say, 1);      //公司标志
      #ifdef SDDL_CSM
       guiDisplay(120, 130, "山东", 1);      //标识为:山东版
      #endif
     #endif
      
   	  lcdRefresh(1, 160);

      sleep(1);
   	#endif
 	 #endif
}

/*******************************************************
函数名称:lcdAdjustDegree
功能描述:调节LCD对比度
调用函数:
被调用函数:
输入参数:
输出参数:无
返回值：void
*******************************************************/
void lcdAdjustDegree(INT8U num)
{
	 ioctl(fdOfLcd, 5, num);
}