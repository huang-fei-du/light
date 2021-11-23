/*************************************************
Copyright,2013
�ļ�����md5.h
���ߣ�ly
�汾��0.9
�������:2013��6��
������md5ͷ�ļ���
�޸���ʷ��
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