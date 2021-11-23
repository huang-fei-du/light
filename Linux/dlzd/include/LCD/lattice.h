/*************************************************
Copyright,2006,LongTong co.,LTD
文件名：ascii.h
作者：leiyong
版本：0.9
完成日期:2006年8月9日
描述：字库点阵[ASCII和16x16]头文件
修改历史：
  01,06-8-9,leiyong created.
  02,09-12-18,leiyong添加16*8点阵及48*48点阵字库
**************************************************/
#ifndef __latticeH
#define __latticeH

#include "common.h"

//西文字符字模
//5*8字模
#ifdef LCD_USE_128_64

unsigned char ASCII[]= 
{
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,    //space = 0x20
	0x40,0x40,0x40,0x40,0x40,0x40,0x00,0x40,    //!
	0x90,0x90,0x90,0X00,0X00,0X00,0X00,0X00,    //"
	0x50,0x50,0xF8,0x50,0x50,0xF8,0x50,0x50,    //#
	0x20,0x70,0xA8,0x60,0x30,0xA8,0x70,0x20,    //$
	0x00,0x40,0xA8,0x50,0x20,0x50,0xA8,0x10,    //%
	0x40,0xA0,0x40,0x40,0xA8,0x90,0x90,0x68,    //&
	0x00,0x00,0x40,0x40,0x40,0x00,0x00,0x00,    //'
	0x20,0x40,0x40,0x40,0x40,0x40,0x40,0x20,    //(
	0x40,0x20,0x20,0x20,0x20,0x20,0x20,0x40,    //)
	0x00,0x20,0xA8,0x70,0x70,0xA8,0x20,0x00,    //*
	0x00,0x00,0x20,0x20,0xF8,0x20,0x20,0x00,    //+
	0x00,0x00,0x00,0x00,0x00,0x20,0x40,0x00,    //,
	0x00,0x00,0x00,0x00,0xF8,0x00,0x00,0x00,    //-
	0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x60,    //.
	0x10,0x10,0x20,0x20,0x40,0x40,0x80,0x80,    // /
	0x70,0x88,0x88,0x88,0x88,0x88,0x88,0x70,    //0
	0x10,0x70,0x10,0x10,0x10,0x10,0x10,0x10,    //1
	0x70,0x88,0x08,0x08,0x10,0x20,0x40,0xF8,    //2
	0x70,0x88,0x08,0x30,0x08,0x08,0x88,0x70,    //3
	0x10,0x30,0x30,0x50,0x50,0x90,0xF8,0x10,    //4
  0xF8,0x80,0x80,0xF0,0x08,0x08,0x88,0x70,    //5
  0x70,0x88,0x80,0xF0,0x88,0x88,0x88,0x70,    //6
  0xF8,0x08,0x10,0x10,0x20,0x20,0x40,0x40,    //7
  0x70,0x88,0x88,0x70,0x88,0x88,0x88,0x70,    //8
  0x70,0x88,0x88,0x88,0x78,0x08,0x88,0x70,    //9
  0x00,0x30,0x30,0x00,0x00,0x30,0x30,0x00,     //:
  0x00,0x20,0x00,0x00,0x00,0x00,0x20,0x40,    //;
  0x00,0x08,0x10,0x20,0x40,0x20,0x10,0x08,    //<
  0x00,0x00,0x00,0x7C,0x00,0x7C,0x00,0x00,    //=
  0x00,0x40,0x20,0x10,0x08,0x10,0x20,0x40,    //>
  0x70,0x88,0x08,0x10,0x20,0x20,0x00,0x20,    //?
  0x00,0x70,0x98,0xA8,0xB8,0xA8,0x88,0x70,    //@
  0x20,0x50,0x50,0x88,0x88,0xF8,0x88,0x88,    //A
  0xF0,0x88,0x88,0xF0,0x88,0x88,0x88,0xF0,    //B
  0x70,0x88,0x80,0x80,0x80,0x80,0x88,0x70,    //C
  0xE0,0x90,0x88,0x88,0x88,0x88,0x90,0xE0,    //D
  0xF8,0x80,0x80,0xF0,0x80,0x80,0x80,0xF8,    //E
  0xF8,0x80,0x80,0xF0,0x80,0x80,0x80,0x80,    //F
  0x70,0x88,0x80,0xB8,0x88,0x88,0x98,0x68,    //G
  0x88,0x88,0x88,0xF8,0x88,0x88,0x88,0x88,    //H
  0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,    //I
  0x10,0x10,0x10,0x10,0x10,0x90,0x90,0x60,    //J
  0x80,0x90,0xA0,0xC0,0xC0,0xA0,0x90,0x88,    //K
  0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x7C,    //L
  0x88,0x88,0xD8,0xD8,0xA8,0xA8,0x88,0x88,    //M
  0x88,0xC8,0xC8,0xA8,0xA8,0x98,0x98,0x88,    //N
  0x70,0x88,0x88,0x88,0x88,0x88,0x88,0x70,    //O
  0xF0,0x88,0x88,0x88,0xF0,0x80,0x80,0x80,    //P
  0x70,0x88,0x88,0x88,0xA8,0x98,0x70,0x08,    //Q
  0xF0,0x88,0x88,0xF0,0x88,0x88,0x88,0x88,    //R
  0x70,0x88,0x80,0x70,0x08,0x08,0x88,0x70,    //S
  0xF8,0x20,0x20,0x20,0x20,0x20,0x20,0x20,    //T
  0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x70,    //U
  0x88,0x88,0x88,0x88,0x50,0x50,0x20,0x20,    //V
  0x88,0x88,0xA8,0xA8,0xA8,0x50,0x50,0x00,    //W
  0x88,0x88,0x50,0x20,0x50,0x88,0x88,0x88,    //X
  0x88,0x88,0x88,0x50,0x20,0x20,0x20,0x20,    //Y
  0xF8,0x08,0x10,0x20,0x40,0x80,0x80,0xF8,    //Z
  0x60,0x40,0x40,0x40,0x40,0x40,0x40,0x60,    //[
  0x40,0x40,0x20,0x20,0x10,0x10,0x08,0x08,    //"\"
  0x60,0x20,0x20,0x20,0x20,0x20,0x20,0x60,    //]
  0x00,0x20,0x50,0x88,0x00,0x00,0x00,0x00,    //^
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFC,    //_
  0x00,0x00,0x40,0x40,0x40,0x00,0x00,0x00,    //'
  0x00,0x00,0x70,0x08,0x78,0x88,0x88,0x78,    //a
  0x80,0x80,0x80,0xF0,0x88,0x88,0x88,0xF0,    //b
  0x00,0x00,0x70,0x88,0x80,0x80,0x88,0x70,    //c
  0x08,0x08,0x08,0x78,0x88,0x88,0x88,0x78,    //d
  0x00,0x00,0x70,0x88,0xF8,0x80,0x88,0x70,    //e
  0x20,0x40,0x40,0x60,0x40,0x40,0x40,0x40,    //f
  0x78,0x88,0x88,0x88,0x88,0x78,0x08,0xF0,    //g
  0x80,0x80,0x80,0xB0,0xC8,0x88,0x88,0x88,    //h
  0x40,0x00,0x40,0x40,0x40,0x40,0x40,0x40,    //i
  0x40,0x00,0x40,0x40,0x40,0x40,0x40,0x80,    //j
  0x80,0x80,0x90,0xA0,0xC0,0xA0,0x90,0x88,    //k
  0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x40,    //l
  0x00,0x00,0x50,0xA8,0xA8,0xA8,0xA8,0xA8,    //m
  0x00,0x00,0xB0,0xC8,0x88,0x88,0x88,0x88,    //n
  0x00,0x00,0x70,0x88,0x88,0x88,0x88,0x70,    //o
  0x00,0xF0,0x88,0x88,0x88,0xF0,0x80,0x80,    //p
  0x00,0x78,0x88,0x88,0x88,0x78,0x08,0x08,    //q
  0x00,0x00,0x50,0x60,0x40,0x40,0x40,0x40,    //r
  0x00,0x00,0x30,0x48,0x20,0x10,0x48,0x30,    //s
  0x40,0x40,0xE0,0x40,0x40,0x40,0x40,0x20,    //t
  0x00,0x00,0x88,0x88,0x88,0x88,0x98,0x68,    //u
  0x00,0x00,0x88,0x88,0x50,0x50,0x20,0x20,    //v
  0x00,0x00,0x88,0xA8,0xA8,0xD8,0x50,0x50,    //w
  0x00,0x00,0x48,0x48,0x30,0x30,0x48,0x48,    //x
  0x48,0x48,0x48,0x48,0x30,0x20,0x20,0xC0,    //y
  0x00,0x00,0x78,0x08,0x10,0x20,0x40,0x78,    //z
  0x10,0x20,0x20,0x60,0x40,0x20,0x20,0x10,    //{
  0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,    //|
  0x40,0x20,0x20,0x20,0x10,0x20,0x20,0x40,    //}
  0x00,0x00,0x00,0x32,0x4C,0x00,0x00,0x00,    //~  
};

#else

/*8*16字模(顺序与5*8描述的一样)*/
unsigned char ASCII[]= 
{
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x00,0x00,0x18,0x18,0x00,0x00,
  0x36,0x24,0x48,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x24,0x24,0x24,0xFE,0x48,0x48,0x48,0xFE,0x48,0x48,0x48,0x00,0x00,
  0x10,0x38,0x54,0x92,0x92,0x50,0x30,0x18,0x14,0x12,0x92,0x92,0x54,0x38,0x10,0x00,
  0x00,0x62,0x92,0x94,0x94,0x68,0x08,0x10,0x20,0x2C,0x52,0x52,0x92,0x8C,0x00,0x00,
  0x00,0x30,0x48,0x48,0x48,0x48,0x30,0x20,0x54,0x94,0x88,0x88,0x94,0x62,0x00,0x00,
  0x30,0x30,0x10,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x04,0x08,0x10,0x10,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x10,0x10,0x08,0x04,0x00,
  0x40,0x20,0x10,0x10,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x10,0x10,0x20,0x40,0x00,
  0x00,0x00,0x00,0x10,0x92,0x54,0x38,0x10,0x38,0x54,0x92,0x10,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x10,0x10,0x10,0xFE,0x10,0x10,0x10,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x10,0x20,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,
  0x00,0x02,0x02,0x04,0x04,0x08,0x08,0x10,0x20,0x20,0x40,0x40,0x80,0x80,0x00,0x00,
  0x00,0x30,0x48,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x84,0x48,0x30,0x00,0x00,
  0x00,0x10,0x70,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x00,0x00,
  0x00,0x30,0x48,0x84,0x84,0x04,0x08,0x08,0x10,0x20,0x20,0x40,0x80,0xFC,0x00,0x00,
  0x00,0x30,0x48,0x84,0x84,0x04,0x08,0x30,0x08,0x04,0x84,0x84,0x48,0x30,0x00,0x00,
  0x00,0x08,0x08,0x18,0x18,0x28,0x28,0x48,0x48,0x88,0xFC,0x08,0x08,0x08,0x00,0x00,
  0x00,0xFC,0x80,0x80,0x80,0xB0,0xC8,0x84,0x04,0x04,0x04,0x84,0x48,0x30,0x00,0x00,
  0x00,0x30,0x48,0x84,0x84,0x80,0xB0,0xC8,0x84,0x84,0x84,0x84,0x48,0x30,0x00,0x00,
  0x00,0xFC,0x04,0x04,0x08,0x08,0x08,0x10,0x10,0x10,0x20,0x20,0x20,0x20,0x00,0x00,
  0x00,0x30,0x48,0x84,0x84,0x84,0x48,0x30,0x48,0x84,0x84,0x84,0x48,0x30,0x00,0x00,
  0x00,0x30,0x48,0x84,0x84,0x84,0x84,0x4C,0x34,0x04,0x84,0x84,0x48,0x30,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x00,0x00,0x30,0x30,0x10,0x20,0x00,
  0x00,0x00,0x04,0x08,0x10,0x20,0x40,0x80,0x40,0x20,0x10,0x08,0x04,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x80,0x40,0x20,0x10,0x08,0x04,0x08,0x10,0x20,0x40,0x80,0x00,0x00,0x00,
  0x00,0x30,0x48,0x84,0x84,0x04,0x08,0x10,0x20,0x20,0x00,0x00,0x30,0x30,0x00,0x00,
  0x00,0x38,0x44,0x82,0x9A,0xAA,0xAA,0xAA,0xAA,0xAA,0x9C,0x80,0x42,0x3C,0x00,0x00,
  0x00,0x10,0x10,0x28,0x28,0x28,0x28,0x44,0x44,0x44,0x7C,0x82,0x82,0x82,0x00,0x00,
  0x00,0xF8,0x84,0x82,0x82,0x82,0x84,0xF8,0x84,0x82,0x82,0x82,0x84,0xF8,0x00,0x00,
  0x00,0x38,0x44,0x82,0x82,0x80,0x80,0x80,0x80,0x80,0x82,0x82,0x44,0x38,0x00,0x00,
  0x00,0xF8,0x84,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x84,0xF8,0x00,0x00,
  0x00,0xFE,0x80,0x80,0x80,0x80,0x80,0xFC,0x80,0x80,0x80,0x80,0x80,0xFE,0x00,0x00,
  0x00,0xFE,0x80,0x80,0x80,0x80,0x80,0xFC,0x80,0x80,0x80,0x80,0x80,0x80,0x00,0x00,
  0x00,0x38,0x44,0x82,0x82,0x80,0x80,0x80,0x8E,0x82,0x82,0x82,0x46,0x3A,0x00,0x00,
  0x00,0x82,0x82,0x82,0x82,0x82,0x82,0xFE,0x82,0x82,0x82,0x82,0x82,0x82,0x00,0x00,
  0x00,0x38,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x38,0x00,0x00,
  0x00,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x84,0x84,0x48,0x30,0x00,0x00,
  0x00,0x82,0x84,0x84,0x88,0x90,0x90,0xA0,0xD0,0x88,0x88,0x84,0x82,0x82,0x00,0x00,
  0x00,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0xFE,0x00,0x00,
  0x00,0x82,0x82,0xC6,0xC6,0xC6,0xC6,0xAA,0xAA,0xAA,0xAA,0x92,0x92,0x92,0x00,0x00,
  0x00,0x82,0x82,0xC2,0xC2,0xA2,0xA2,0x92,0x92,0x8A,0x8A,0x86,0x86,0x82,0x00,0x00,
  0x00,0x38,0x44,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x44,0x38,0x00,0x00,
  0x00,0xF8,0x84,0x82,0x82,0x82,0x84,0xF8,0x80,0x80,0x80,0x80,0x80,0x80,0x00,0x00,
  0x00,0x38,0x44,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x92,0x8A,0x44,0x3A,0x00,0x00,
  0x00,0xF8,0x84,0x82,0x82,0x82,0x84,0xF8,0x88,0x88,0x84,0x84,0x82,0x82,0x00,0x00,
  0x00,0x38,0x44,0x82,0x82,0x80,0x60,0x18,0x04,0x02,0x82,0x82,0x44,0x38,0x00,0x00,
  0x00,0xFE,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x00,0x00,
  0x00,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x82,0x44,0x38,0x00,0x00,
  0x00,0x82,0x82,0x82,0x44,0x44,0x44,0x44,0x28,0x28,0x28,0x10,0x10,0x10,0x00,0x00,
  0x00,0x92,0x92,0x92,0x92,0xAA,0xAA,0xAA,0xAA,0x44,0x44,0x44,0x44,0x44,0x00,0x00,
  0x00,0x82,0x82,0x44,0x44,0x28,0x28,0x10,0x28,0x28,0x44,0x44,0x82,0x82,0x00,0x00,
  0x00,0x82,0x82,0x44,0x44,0x28,0x28,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x00,0x00,
  0x00,0xFE,0x02,0x04,0x04,0x08,0x08,0x10,0x20,0x20,0x40,0x40,0x80,0xFE,0x00,0x00,
  0x7C,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x7C,0x00,
  0x00,0x82,0x82,0x44,0x44,0x28,0x28,0x7C,0x10,0x10,0x7C,0x10,0x10,0x10,0x00,0x00,
  0x7C,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x7C,0x00,
  0x10,0x28,0x44,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,
  0x20,0x10,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x78,0x84,0x04,0x3C,0x44,0x84,0x8C,0x76,0x00,0x00,
  0x00,0x80,0x80,0x80,0x80,0x80,0xB8,0xC4,0x82,0x82,0x82,0x82,0xC4,0xB8,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x42,0x80,0x80,0x80,0x80,0x42,0x3C,0x00,0x00,
  0x00,0x02,0x02,0x02,0x02,0x02,0x3A,0x46,0x82,0x82,0x82,0x82,0x46,0x3A,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x44,0x82,0xFE,0x80,0x80,0x42,0x3C,0x00,0x00,
  0x00,0x18,0x20,0x20,0x20,0x20,0xF8,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x3A,0x44,0x44,0x38,0x40,0x7C,0x82,0x82,0x7C,0x00,
  0x00,0x80,0x80,0x80,0x80,0x80,0xB8,0xC4,0x82,0x82,0x82,0x82,0x82,0x82,0x00,0x00,
  0x00,0x00,0x10,0x10,0x00,0x00,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x00,0x00,
  0x00,0x00,0x10,0x10,0x00,0x00,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x60,0x00,
  0x00,0x80,0x80,0x80,0x80,0x80,0x84,0x88,0x90,0xA0,0xD0,0x88,0x84,0x82,0x00,0x00,
  0x00,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0xAC,0xD2,0x92,0x92,0x92,0x92,0x92,0x92,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0xB8,0xC4,0x82,0x82,0x82,0x82,0x82,0x82,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x44,0x82,0x82,0x82,0x82,0x44,0x38,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0xB8,0xC4,0x82,0x82,0x82,0xC4,0xB8,0x80,0x80,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x3A,0x46,0x82,0x82,0x82,0x46,0x3A,0x02,0x02,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x2E,0x30,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x7C,0x82,0x80,0x60,0x1C,0x02,0x82,0x7C,0x00,0x00,
  0x00,0x00,0x20,0x20,0x20,0x20,0xF8,0x20,0x20,0x20,0x20,0x20,0x20,0x18,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x82,0x82,0x82,0x82,0x82,0x82,0x46,0x3A,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x82,0x82,0x44,0x44,0x28,0x28,0x10,0x10,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x92,0x92,0x92,0xAA,0xAA,0x44,0x44,0x44,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x82,0x44,0x28,0x10,0x10,0x28,0x44,0x82,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x82,0x82,0x44,0x44,0x28,0x28,0x10,0x20,0xC0,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0xFE,0x04,0x08,0x10,0x20,0x40,0x80,0xFE,0x00,0x00,
  0x1C,0x10,0x10,0x10,0x10,0x10,0x10,0x20,0x10,0x10,0x10,0x10,0x10,0x10,0x1C,0x00,
  0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
  0x70,0x10,0x10,0x10,0x10,0x10,0x10,0x08,0x10,0x10,0x10,0x10,0x10,0x10,0x70,0x00,
  0x64,0x98,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
#endif

//开机画面汉字"负控系统终端"
unsigned char START[]= {
   
  #ifdef LCD_USE_128_64 
   //集抄终端24*24
   /*
   0x01,0x10,0x00,0x01,0xCC,0x00,0x03,0x0E,    //集
   0x00,0x02,0x04,0x18,0x07,0xFF,0xFC,0x06,
   0x0C,0x00,0x0E,0x0C,0x10,0x17,0xFF,0xF8,
   0x26,0x0C,0x00,0x46,0x0C,0x10,0x07,0xFF,
   0xF8,0x06,0x0C,0x00,0x06,0x0C,0x08,0x07,
   0xFF,0xFC,0x04,0x10,0x00,0x00,0x18,0x0C,
   0x7F,0xFF,0xFE,0x00,0x7A,0x00,0x00,0xDB,
   0x00,0x01,0x99,0x80,0x06,0x18,0xF8,0x18,
   0x18,0x3E,0x60,0x18,0x08,0x00,0x10,0x00,
   0x04,0x00,0x00,0x07,0x01,0x00,0x06,0x01,    //抄
   0xC0,0x06,0x01,0x80,0x06,0x01,0x80,0x06,
   0xC9,0x80,0x7F,0xEF,0xB0,0x06,0x0D,0x9C,
   0x06,0x19,0x8E,0x06,0x19,0x86,0x06,0xD1,
   0x82,0x07,0x31,0x80,0x1E,0x21,0x88,0x76,
   0x41,0x8E,0x26,0x41,0x9C,0x06,0x81,0x18,
   0x06,0x00,0x30,0x06,0x00,0x60,0x06,0x00,
   0xC0,0x06,0x01,0x80,0x06,0x06,0x00,0x7E,
   0x18,0x00,0x1C,0x60,0x00,0x09,0x80,0x00,
   */
   0x04,0x04,0x00,0x07,0x07,0x00,0x06,0x06,   //终
   0x00,0x06,0x06,0x18,0x0C,0x0F,0xFC,0x09,
   0x08,0x30,0x19,0xDC,0x30,0x11,0x94,0x60,
   0x23,0x22,0x60,0x7F,0x21,0xC0,0x32,0x41,
   0xC0,0x06,0x03,0x60,0x04,0x06,0x38,0x08,
   0x18,0x1E,0x10,0x22,0x08,0x3F,0x81,0x80,
   0x18,0x00,0xC0,0x00,0x00,0xC0,0x01,0x80,
   0x80,0x0E,0x1E,0x00,0x78,0x03,0x80,0x20,
   0x00,0xE0,0x00,0x00,0x60,0x00,0x00,0x20,
   0x10,0x01,0x00,0x08,0x21,0xC8,0x0C,0x39,  //端
   0x8E,0x0C,0x31,0x8C,0x0C,0x31,0x8C,0x01,
   0xB1,0x8C,0x7F,0xBF,0xFC,0x00,0x20,0x08,
   0x02,0x00,0x04,0x23,0xBF,0xFE,0x13,0x02,
   0x00,0x13,0x03,0x00,0x1A,0x22,0x0C,0x1A,
   0x3F,0xFE,0x1A,0x32,0x4C,0x14,0x32,0x4C,
   0x07,0xB2,0x4C,0x7E,0x32,0x4C,0x38,0x32,
   0x4C,0x20,0x32,0x4C,0x00,0x32,0x0C,0x00,
   0x30,0x3C,0x00,0x20,0x18,0x00,0x00,0x00 
   
  #else 
   
   /*集中器48*48*/
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x71,0xF0,  //集
   0x00,0x00,0x00,0x00,0x78,0xF8,0x00,0x00,0x00,0x00,0x7C,0x38,0x00,0x00,0x00,0x00,
   0x78,0x18,0x00,0x00,0x00,0x00,0xF0,0x00,0xF8,0x00,0x00,0x00,0xF0,0x1F,0xFC,0x00,
   0x00,0x01,0xE3,0xFF,0xC0,0x00,0x00,0x03,0xFF,0xF0,0x00,0x00,0x00,0x03,0x80,0x70,
   0x00,0x00,0x00,0x07,0xC0,0x70,0x00,0x00,0x00,0x0F,0xC0,0x7F,0xE0,0x00,0x00,0x1D,
   0xC3,0xFF,0x00,0x00,0x00,0x39,0xCF,0xF0,0x00,0x00,0x00,0x71,0xC0,0x70,0x00,0x00,
   0x00,0xE1,0xC0,0x73,0xC0,0x00,0x01,0x81,0xC0,0x7F,0xC0,0x00,0x00,0x01,0xCF,0xFC,
   0x00,0x00,0x00,0x01,0xC3,0x78,0x00,0x00,0x00,0x01,0xC0,0x78,0x00,0x00,0x00,0x01,
   0xC0,0x79,0xFC,0x00,0x00,0x01,0xC0,0x7F,0xFC,0x00,0x00,0x03,0xFF,0xFF,0xC0,0x00,
   0x00,0x03,0xFE,0x00,0x00,0x00,0x00,0x03,0x80,0x00,0x00,0x00,0x00,0x01,0x83,0xE0,
   0x00,0x00,0x00,0x01,0x81,0xE0,0x7F,0xC0,0x00,0x00,0x01,0xFF,0xFF,0xF0,0x00,0x00,
   0x1F,0xFF,0xFF,0xE0,0x00,0x3F,0xFF,0xF8,0x00,0x00,0x0F,0xFF,0x0F,0xDC,0x00,0x00,
   0x03,0xC0,0x1F,0xCE,0x00,0x00,0x00,0x00,0x3D,0xCF,0x00,0x00,0x00,0x00,0x79,0xC7,
   0x80,0x00,0x00,0x00,0xF1,0xC3,0xC0,0x00,0x00,0x01,0xE1,0xC1,0xF0,0x00,0x00,0x03,
   0xC1,0xC0,0xF8,0x00,0x00,0x07,0x81,0xC0,0xFF,0x00,0x00,0x1E,0x01,0xC0,0x7F,0xC0,
   0x00,0x3C,0x01,0xC0,0x3F,0xFC,0x00,0xF0,0x01,0xC0,0x1F,0xFC,0x01,0x80,0x01,0xC0,
   0x00,0x00,0x00,0x00,0x01,0xC0,0x00,0x00,0x00,0x00,0x01,0xC0,0x00,0x00,0x00,0x00,
   0x01,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x80,   //中
   0x00,0x00,0x00,0x00,0x07,0xC0,0x00,0x00,0x00,0x00,0x07,0xC0,0x00,0x00,0x00,0x00,
   0x07,0xC0,0x00,0x00,0x00,0x00,0x07,0xC0,0x00,0x00,0x00,0x00,0x07,0xC0,0x00,0x00,
   0x00,0x00,0x07,0xC0,0x00,0x00,0x00,0x00,0x07,0xC0,0x00,0x00,0x00,0x00,0x03,0xC0,
   0x00,0x00,0x00,0x00,0x03,0xC0,0x38,0x00,0x00,0x00,0x03,0xCF,0xFE,0x00,0x00,0x00,
   0x1F,0xFF,0xFF,0x00,0x01,0xFF,0xFF,0xF0,0x3F,0x80,0x00,0xFF,0xE3,0xC0,0x3F,0x00,
   0x00,0x78,0x03,0xC0,0x3E,0x00,0x00,0x78,0x03,0xC0,0x3E,0x00,0x00,0x78,0x03,0xC0,
   0x3C,0x00,0x00,0x38,0x03,0xC0,0x3C,0x00,0x00,0x38,0x03,0xC0,0x78,0x00,0x00,0x38,
   0x03,0xC0,0x78,0x00,0x00,0x3C,0x03,0xC0,0x70,0x00,0x00,0x3C,0x03,0xDF,0xF8,0x00,
   0x00,0x1C,0x3F,0xFF,0xF8,0x00,0x00,0x1F,0xFF,0xFC,0x00,0x00,0x00,0x1F,0xE3,0xC0,
   0x00,0x00,0x00,0x1C,0x03,0xC0,0x00,0x00,0x00,0x0C,0x03,0xC0,0x00,0x00,0x00,0x00,
   0x03,0xC0,0x00,0x00,0x00,0x00,0x03,0xC0,0x00,0x00,0x00,0x00,0x03,0xC0,0x00,0x00,
   0x00,0x00,0x03,0xC0,0x00,0x00,0x00,0x00,0x03,0x80,0x00,0x00,0x00,0x00,0x03,0x80,
   0x00,0x00,0x00,0x00,0x03,0x80,0x00,0x00,0x00,0x00,0x03,0x80,0x00,0x00,0x00,0x00,
   0x03,0x80,0x00,0x00,0x00,0x00,0x03,0x80,0x00,0x00,0x00,0x00,0x03,0x80,0x00,0x00,
   0x00,0x00,0x03,0x80,0x00,0x00,0x00,0x00,0x03,0x80,0x00,0x00,0x00,0x00,0x03,0x80,
   0x00,0x00,0x00,0x00,0x03,0x80,0x00,0x00,0x00,0x00,0x03,0x80,0x00,0x00,0x00,0x00,
   0x03,0x80,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  //器
   0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x00,0x61,0xFC,0x00,0x00,0x00,
   0xF8,0x7F,0xFF,0x00,0x00,0xC7,0xFC,0x78,0x3F,0x00,0x00,0xFE,0x3E,0x78,0x3C,0x00,
   0x00,0x78,0x3C,0x38,0x38,0x00,0x00,0x78,0x38,0x38,0x78,0x00,0x00,0x38,0x78,0x38,
   0x70,0x00,0x00,0x38,0x70,0x3F,0xF8,0x00,0x00,0x39,0xF8,0x3F,0x80,0x00,0x00,0x1F,
   0xEC,0x18,0x00,0x00,0x00,0x18,0x0F,0x80,0x7C,0x00,0x00,0x1C,0x07,0x80,0x1F,0x00,
   0x00,0x00,0x07,0x80,0x0F,0x00,0x00,0x00,0x0F,0x07,0xC3,0x00,0x00,0x00,0x0F,0xFF,
   0xF0,0x00,0x00,0x00,0x3F,0xFF,0xE0,0x00,0x00,0x0F,0xFF,0x80,0x00,0x00,0x00,0x3F,
   0xDF,0xE0,0x00,0x00,0x00,0x00,0x3C,0xF0,0x00,0x00,0x00,0x00,0x7C,0x7C,0x00,0x00,
   0x00,0x00,0x78,0x1F,0x80,0x00,0x00,0x00,0xF0,0x0F,0xE0,0x00,0x00,0x03,0xE0,0x07,
   0xFC,0x00,0x00,0x07,0xC0,0x01,0xFF,0xF0,0x00,0x1F,0x80,0x00,0xFF,0xFC,0x00,0x3E,
   0x00,0x00,0x3F,0x00,0x00,0xF8,0x00,0x00,0x00,0x00,0x07,0xC0,0x00,0x00,0x3C,0x00,
   0x00,0x00,0x78,0x7F,0xFF,0x00,0x00,0xE7,0xFE,0x7C,0x1F,0x80,0x00,0xFF,0x3E,0x78,
   0x1F,0x00,0x00,0x70,0x3C,0x78,0x1E,0x00,0x00,0x70,0x3C,0x38,0x1E,0x00,0x00,0x70,
   0x38,0x38,0x1C,0x00,0x00,0x70,0x38,0x38,0x7E,0x00,0x00,0x70,0x7C,0x3F,0xFF,0x00,
   0x00,0x3F,0xFC,0x3F,0xC0,0x00,0x00,0x3F,0x80,0x18,0x00,0x00,0x00,0x30,0x00,0x18,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   
   
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x38,  //终
   0x00,0x00,0x00,0x00,0x00,0x3C,0x00,0x00,0x00,0x00,0x00,0x3C,0x00,0x00,0x00,0x60,
   0x00,0x38,0x00,0x00,0x00,0x70,0x00,0x78,0x00,0x00,0x00,0x78,0x00,0x70,0x70,0x00,
   0x00,0x70,0x00,0xF7,0xF8,0x00,0x00,0xF0,0x00,0xFF,0xFC,0x00,0x00,0xE0,0x01,0xC0,
   0xFC,0x00,0x01,0xC0,0x01,0xC0,0xF0,0x00,0x01,0xC0,0x03,0x80,0xE0,0x00,0x03,0x86,
   0x03,0x01,0xE0,0x00,0x07,0x07,0x07,0x81,0xC0,0x00,0x06,0x07,0x8C,0xE3,0xC0,0x00,
   0x0C,0x07,0x18,0x77,0x80,0x00,0x18,0x0F,0x10,0x3F,0x00,0x00,0x38,0x1E,0x20,0x0E,
   0x00,0x00,0x7F,0xFC,0x00,0x1F,0x80,0x00,0x7F,0xF8,0x00,0x3B,0xC0,0x00,0x70,0x70,
   0x00,0x71,0xF0,0x00,0x00,0x60,0x01,0xE0,0xFC,0x00,0x00,0xC0,0x03,0xC0,0x7F,0x00,
   0x01,0x80,0x07,0x00,0x1F,0xC0,0x03,0x03,0x1E,0x00,0x0F,0xF8,0x06,0x7C,0x78,0x00,
   0x07,0xFE,0x0F,0xF0,0xE0,0x00,0x03,0xFF,0x1F,0xC1,0x80,0x70,0x01,0xF8,0x1F,0x00,
   0x00,0x7C,0x00,0x00,0x0C,0x00,0x00,0x3E,0x00,0x00,0x00,0x00,0x00,0x1F,0x00,0x00,
   0x00,0x03,0x80,0x0F,0x00,0x00,0x00,0x0E,0x00,0x07,0x00,0x00,0x00,0x7C,0x00,0x02,
   0x00,0x00,0x01,0xF0,0x00,0x00,0x00,0x00,0x0F,0xE0,0x00,0x00,0x00,0x00,0x7F,0x80,
   0x01,0x80,0x00,0x00,0x3F,0x00,0x00,0xF8,0x00,0x00,0x1C,0x00,0x00,0x7C,0x00,0x00,
   0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00,0x00,0x1F,0x80,0x00,0x00,0x00,0x00,0x0F,
   0x80,0x00,0x00,0x00,0x00,0x07,0xC0,0x00,0x00,0x00,0x00,0x03,0xC0,0x00,0x00,0x00,
   0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,   
  
   
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  //端
   0x00,0x00,0x00,0x00,0x00,0x07,0x00,0x00,0x00,0x00,0x00,0x07,0x80,0x00,0x00,0x00,
   0x00,0x03,0x80,0x00,0x00,0x78,0x00,0x03,0x80,0x00,0x00,0x3E,0x00,0x03,0x80,0x00,
   0x00,0x1F,0x00,0x03,0x81,0x80,0x00,0x0F,0x00,0x03,0x01,0xC0,0x00,0x07,0x00,0x03,
   0x01,0xC0,0x00,0x03,0x01,0x83,0x01,0xC0,0x00,0x00,0x01,0xC3,0x01,0x80,0x00,0x00,
   0x01,0xC3,0x03,0x80,0x00,0x00,0x01,0x83,0x1F,0x80,0x00,0x01,0xC3,0x83,0xFF,0x00,
   0x00,0x1F,0xE3,0xFF,0xC3,0x00,0x00,0xFF,0x07,0xF8,0x02,0x00,0x07,0xF8,0x07,0xC0,
   0x00,0x00,0x07,0x80,0x02,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x08,0x00,0x00,0x03,
   0xC0,0x00,0xFE,0x00,0x03,0x03,0xC0,0x1F,0xF8,0x00,0x03,0x83,0x81,0xFF,0x80,0x00,
   0x01,0xC3,0x81,0xF8,0x00,0x00,0x01,0xC3,0x80,0x1C,0x00,0x00,0x01,0xC3,0x00,0x1C,
   0x00,0x00,0x00,0xC7,0x00,0x3C,0x00,0xC0,0x00,0x06,0x00,0x30,0x0F,0xF0,0x00,0x06,
   0x40,0x63,0xFF,0xF8,0x00,0x0F,0xA1,0xFF,0xC0,0x7C,0x00,0x0F,0x3F,0xE0,0xC0,0x38,
   0x00,0x7C,0x38,0xE0,0xC0,0x38,0x01,0xF8,0x18,0x60,0xC0,0x70,0x1F,0xE0,0x18,0x60,
   0xC0,0x70,0x3F,0x80,0x18,0x60,0xC0,0x70,0x3F,0x00,0x18,0x60,0xC0,0x70,0x1C,0x00,
   0x18,0x60,0xC0,0x70,0x00,0x00,0x18,0x60,0xC0,0xF0,0x00,0x00,0x18,0x60,0xC0,0xE0,
   0x00,0x00,0x18,0x00,0x8F,0xE0,0x00,0x00,0x18,0x00,0x87,0xE0,0x00,0x00,0x18,0x00,
   0x03,0xE0,0x00,0x00,0x00,0x00,0x01,0xC0,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  #endif
};

#endif //__latticeH