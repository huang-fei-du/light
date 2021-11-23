/*************************************************
Copyright,2013
文件名：md5.h
作者：ly
版本：0.9
完成日期:2013年6月
描述：md5头文件。
修改历史：
  01,13-06-06,leiyong created.
**************************************************/

#ifndef __INCMD5H
#define __INCMD5H

typedef struct 
{
  unsigned long int state[4];      /* state (ABCD) */
  unsigned long int count[2];      /* number of bits, modulo 2^64 (lsb first) */
  unsigned char     buffer[64];    /* input buffer */
}MD5_CTX;

char * MDString(char *);
char * MDFile(char *);
char * hmac_md5(char* text, char* key);

#endif   //__INCMD5H