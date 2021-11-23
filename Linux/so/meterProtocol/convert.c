/***************************************************
Copyright,2009,Huawei WoDian co.,LTD,All	Rights Reserved
文件名：convert.c
作者：Leiyong
版本：0.9
完成日期：2009年12月
描述：转换处理文件
函数列表：
     1.
修改历史：
  01,09-12-22,leiyong created.

***************************************************/

#include "stdio.h"
#include "string.h"

#include "convert.h"

/***************************************************
函数名称:digitalToChar
功能描述:将一位整数转换为相应的字符
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
char*	digitalToChar(INT8U digital)
{
  switch(digital)
	{
	  case	0:
			return	"0";
			break;
		case	1:
			return	"1";
			break;
		case	2:
			return	"2";
			break;
		case	3:
			return	"3";
			break;
		case	4:
			return	"4";
			break;
		case	5:
			return	"5";
			break;
		case	6:
			return	"6";
			break;
		case	7:
			return	"7";
			break;
		case	8:
			return	"8";
			break;
		case	9:
			return	"9";
			break;			
		case 0xa:
			return	"A";
			break;			
		case 0xb:
			return	"B";
			break;			
		case 0xc:
			return	"C";
			break;			
		case 0xd:
			return	"D";
			break;			
		case 0xe:
			return	"E";
			break;			
		case 0xf:
			return	"F";
			break;			
	  default:
			return	"X";
			break;
	}
}

/***************************************************
函数名称:intToString
功能描述:将一整数(0--65535)转换为二进制(/十进制)字符串
         >65535转换成32位二进制
         >255转换成16位二进制，小于等于255转换成8位二进制
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
char * intToString(INT32U in,int type,char *returnStr)
{
  char   tmpStr[33];
	INT8U  i;
	INT8U  bits;
	INT32U divisor=1000000000;
  strcpy(returnStr,"");

  switch(type)
  {
    case 1:
   	  if (in>65535)
   	  {
	      bits=32;
	    }
	    else
	    {
	   	  if (in>255)
	 	      bits=16;
	      else
	 	      bits=8;
	 	  }
      for (i=0;i<bits;i++)
	    {
	 	     strcpy(tmpStr,digitalToChar(in%2));
	 	     strcat(tmpStr,returnStr);
	 	     strcpy(returnStr,tmpStr);
	 	     in=in/2;
	    }
	    returnStr[bits]='\0';
	    break;
	    
	  case 2:
      bits=16;
      for (i = 0;i < bits;i++)
	    {
	 	     strcpy(tmpStr,digitalToChar(in%2));
	 	     strcat(tmpStr,returnStr);
	 	     strcpy(returnStr,tmpStr);
	 	     in=in/2;
	    }
	    returnStr[bits]='\0';
	    break;
	    
	  case 3:
   	  if (in==0)
   	  {
   	    strcpy(returnStr,"0");
   	  }
   	  else
   	  {
   	     bits=0;
   	     for(i=0;i<10;i++)
   	     {
   	  	    if (bits==0)
   	  	    {
   	  	       if ((in/divisor)!=0)
   	  	       {
   	  	 	        strcat(returnStr,digitalToChar(in/divisor));
   	  	 	        in=in%divisor;
   	  	 	        bits++;
   	  	       }
   	  	    }
   	  	    else
   	  	    {
   	  	       if ((in/divisor)!=0)
   	  	       {
   	  	 	        strcat(returnStr,digitalToChar(in/divisor));
   	  	       }
   	  	       else
   	  	       {
   	  	 	        strcat(returnStr,"0");
   	  	 	     }

   	  	 	     in=in%divisor;
   	  	 	     bits++;
   	  	    }
   	  	    divisor=divisor/10; 
   	     }
      }
      break;
  }
  return returnStr;
}

/***************************************************
函数名称:digital2ToString
功能描述:将<100的整数(0--99)转换为十进制字符串
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
char * digital2ToString(INT8U in,char *returnStr)
{
   strcpy(returnStr,"");
 
   if (in==0)
       strcpy(returnStr,"00");
   else
   {
   	  if (in <10)
   	  {
   	  	 strcpy(returnStr,"0");
   	  	 strcat(returnStr,digitalToChar(in));
   	  }
   	  else
   	  {
   	  	 strcpy(returnStr,digitalToChar(in/10));
   	  	 strcat(returnStr,digitalToChar(in%10)); 
   	  }
   }
 
   return returnStr;
}

/***************************************************
函数名称:int8uToHex
功能描述:将<=255的整数(0--255)转换为十六进制字符串
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
char * int8uToHex(INT8U in,char *returnStr)
{
   strcpy(returnStr,"");
 
   if (in==0)
       strcpy(returnStr,"00");
   else
   {
   	  if (in <16)
   	  {
   	  	 strcpy(returnStr,"0");
   	  	 strcat(returnStr,digitalToChar(in));
   	  }
   	  else
   	  {
   	  	 strcpy(returnStr,digitalToChar(in/16));
   	  	 strcat(returnStr,digitalToChar(in%16)); 
   	  }
   }
 
   return returnStr;
}

/***************************************************
函数名称:floatToString
功能描述:将带有小数的数据转换为符串
调用函数:
被调用函数:
输入参数:INT32U integer,整数
         INT32U decimal,小数数值
         INT8U  precision,精度(比如小数点后4位)
         INT8U  decNum,返回的数据小数取几位
         char   *returnStr,返回转换好的字符串         
输出参数:
返回值：void
***************************************************/
char * floatToString(INT32U integer,INT32U decimal,INT8U precision, INT8U decNum,char *returnStr)
{
   char   str[15];
   char   decStr[11];
   char   tmpStr[11];
   INT8U  decLen;
   //INT16U compare;
   INT8U i;
   
   if (decNum>precision)
   {
   	  decNum = precision;
   }
   
   strcpy(returnStr,"");
   
   //整数部分
   strcat(returnStr,intToString(integer,3,str));
   if (precision==0 || decNum==0)
   {
   	 ;
   }
   else
   {
     strcat(returnStr,".");
   }
 
   //小数部分   
   if (decimal == 0)
   {
      for(i=0;i<decNum;i++)
        strcat(returnStr,"0");
   }
   else
   {
   	  strcpy(decStr,"0000000000");
   	  strcpy(tmpStr,intToString(decimal,3,str));
   	  
   	  decLen = strlen(tmpStr);
   	  for(i=0;i<decLen;i++)
   	  {
   	  	 decStr[precision-decLen+i] = tmpStr[i];
   	  }
   	  
   	  decStr[decNum] = '\0';

   	  strcat(returnStr,decStr);
   	  
   	  /*
   	  compare = 1;
   	  for(i=0;i<decNum-1;i++)
   	  {
   	     compare *= 10;
   	  }
   	  
   	  for(i=0;i<decNum;i++)
   	  {
   	  	 if (decimal<compare)
   	  	 {
   	  	    strcat(returnStr,"0");
   	  	    compare /=10;
   	  	 }
   	  	 else
   	  	 {
   	  	   strcat(returnStr,intToString(decimal,3,str));
   	  	   break;
   	  	 }
   	  }
   	  */
   }
 
   return returnStr;
}

/***************************************************
函数名称:intToIpadd
功能描述:将INT32型32位IP地址转换为字符串型IP地址
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
char *intToIpadd(INT32U ip,char * returnStr)
{
	 INT8U tmpData,i;
	 char tmpStr[5];
	 strcpy(returnStr,"");

   for(i=0;i<4;i++)
   {
     tmpData = ip>>(24-i*8) & 0xff;
     intToString(tmpData,3,tmpStr);
     if (strlen(tmpStr)==1)
     {
     	 strcat(returnStr,"00");
     }
     if (strlen(tmpStr)==2)
     {
     	 strcat(returnStr,"0");
     }
     strcat(returnStr,tmpStr);
     if (i<3)
     {
       strcat(returnStr,".");
     }
   }
   
	 return returnStr;
}

/*******************************************************
函数名称: hexToBcd
功能描述: 
调用函数:     
被调用函数:
输入参数:   
输出参数:  
返回值： 
*******************************************************/
INT32U hexToBcd(INT32U hex)
{
  INT32U bcd;
  INT32U div;
  INT8U  i;
  bcd = 0;
  div = 10;
  for(i=0;i<8;i++)
  {
     bcd = bcd | (((hex % div) / (div / 10)) << (i * 4));

     hex = hex - hex%div;
     div = div * 10;
  }
  return bcd;
}

/*******************************************************
函数名称: bcdToHex
功能描述: 
调用函数:     
被调用函数:
输入参数:   
输出参数:  
返回值： 
*******************************************************/
INT32U bcdToHex(INT32U bcd)
{
  INT32U hex;
  
  hex = (bcd&0xf) + (bcd>>4&0xf)*10 + (bcd>>8&0xf)*100 + (bcd>>12&0xf)*1000
            + (bcd>>16&0xf)*10000 + (bcd>>20&0xf)*100000 + (bcd>>24&0xf)*1000000
               + (bcd>>28&0xf)*10000000;
  return hex;
}

/*******************************************************
函数名称:timeBcdToHex
功能描述:将BCD格式的时间转换十六进制格式的时间
调用函数:     
被调用函数:
输入参数:   
输出参数:  
返回值:16进制格式的时间
*******************************************************/
DATE_TIME timeBcdToHex(DATE_TIME bcdTime)
{
  DATE_TIME hexTime;
  
	hexTime.second = (bcdTime.second&0xF)+(bcdTime.second>>4&0xF)*10;
	hexTime.minute = (bcdTime.minute&0xF)+(bcdTime.minute>>4&0xF)*10;
	hexTime.hour   = (bcdTime.hour  &0xF)+(bcdTime.hour  >>4&0xF)*10;
	hexTime.day    = (bcdTime.day   &0xF)+(bcdTime.day   >>4&0xF)*10;
	hexTime.month  = (bcdTime.month &0xF)+(bcdTime.month >>4&0xF)*10;
	hexTime.year   = (bcdTime.year  &0xF)+(bcdTime.year  >>4&0xF)*10;

  return hexTime;
}

/*******************************************************
函数名称:timeHexToBcd
功能描述:将十六进制格式的时间转换BCD格式的时间
调用函数:
被调用函数:
输入参数:   
输出参数:  
返回值:BCD格式的时间
*******************************************************/
DATE_TIME timeHexToBcd(DATE_TIME hexTime)
{
  DATE_TIME bcdTime;
  
  bcdTime.second = (hexTime.second/10<<4) | (hexTime.second%10);
  bcdTime.minute = (hexTime.minute/10<<4) | (hexTime.minute%10);
  bcdTime.hour   = (hexTime.hour  /10<<4) | (hexTime.hour  %10);
  bcdTime.day    = (hexTime.day   /10<<4) | (hexTime.day   %10);
  bcdTime.month  = (hexTime.month /10<<4) | (hexTime.month %10);
  bcdTime.year   = (hexTime.year  /10<<4) | (hexTime.year  %10);

  return bcdTime;
}

/*******************************************************
函数名称:bmToYm
功能描述:将补码转成原码
调用函数:
被调用函数:
输入参数:   
输出参数:  
返回值:数据的原码
*******************************************************/
INT32U bmToYm(INT8U *inData, INT8U dataLen)
{
  INT32U checkData = 0;
  INT32U outData = 0;

  if (dataLen>4)
  {
    return 0;
  }
  if (1==dataLen)
  {
    outData = inData[0];
	if (outData&0x80)
	{
	  outData = ~(outData&0x7f);
	}
  }
  else if (2==dataLen)
  {
	outData = inData[0]<<8 | inData[1];
	if (outData&0x8000)
	{
	  outData = (~(outData&0x7fff))&0x7fff;
	}
  }
  else if (3==dataLen)
  {
	outData = inData[0]<<16 | inData[1]<<8 | inData[2];
	if (outData&0x800000)
	{
	  outData = (~(outData&0x7fffff))&0x7fffff;
	}
  }
  else if (4==dataLen)
  {
	outData = inData[0]<<24 | inData[1]<<16 | inData[2]<<8 | inData[3];
	if (outData&0x80000000)
	{
	  outData = (~(outData&0x7fffffff))&0x7fffffff;
	}
  }

  return outData;
}


