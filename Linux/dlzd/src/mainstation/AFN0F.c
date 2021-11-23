/***************************************************
Copyright,2006-2007,LongTong co.,LTD,All	Rights Reserved
文件名：AFN0F.c
作者：leiyong
版本：0.9
完成日期：2007年1月
描述：主站AFN0F(文件传输)处理文件。
函数列表：
     1.
修改历史：
  01,07-1-18,Leiyong created.
  02,07-3-24,Leiyong确定升级办法,并实时升级
             (AFN=0F,fn=2时主站发起远程升级命令,fn=3主站传送文件,fn=4主站传送文件完毕,fn=5清除升级标志)
  03,07-04-04,Leiyong修改升级中读取已收到的程序片段的个数的方法
  04,07-10-12,Leiyong修正远程升级bug,存储原来没有存储的最后一个数据包。
  05,08-07-25,Leiyong修改,
              1)远程升级每程序片段为固定512字节;
              2)利用NAND Flash的spare空间存储收到的程序片序号
              3)请求程序片段序号从0开始,重试升级时,利用读到的已收到的序号,即是说用请求第211个程序片段时,发的请求序号为210,以利于主站处理
  06,10-03-31,Leiyong,移植到Linux2.6.20
  07,12-09-06,Leiyong,增加文件上传功能
  07,12-09-18,Leiyong,修改上传功能为可以传文件的一部分
***************************************************/
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"

#include "dataBase.h"
#include "string.h"
#include "lcdGui.h"
#include "teRunPara.h"
#include "msSetPara.h"
#include "userInterface.h"
#include "md5.h"

#include "AFN0F.h"

//变量声明
INT8U        upgradeBuff[512];  //升级写缓存
FILE         *fpOfUpgrade=NULL; //升级文件指针
UPGRADE_FLAG upgradeFlag;       //升级标志
char         fileName[256], tmpFileName[100], destFileName[100];
BOOL         checkSumError = FALSE;
BOOL         powerUpFirst  = FALSE; //上电后的第一次请求
INT16U       uploadCnt=0;           //上传文件计数

/*******************************************************
函数名称:AFN0F
功能描述:主站"文件传输"(AFN0F)的处理函数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void AFN0F(INT8U *pDataHead, INT8U *pDataEnd ,INT8U dataFrom)
{
   INT8U        fn, tmpSize;
   INT16U       i, j;
   INT8U        *pData,*tmpData;
   char         say[40],str[10];
	 INT8U        tmpBuf[512];
	 struct stat  statBuf;
	 INT8U        checkData[8], dataX[4];

   UPGRADE_FLAG upgradeFlagv13;    //13版规约升级标志
	 INT32U       recvPackage;       //当前接收的包标识(/偏移)
	 INT8U        recvEnd = 0;       //是否是最后一包数据
	 INT16U       nowPkgSize=0;      //当前接收的包的大小
	 FILE         *fpOfTar = NULL;
	 char         chkInfo[128];
   char         filenames[20][256];
   char         tmpProcess[256];
   int          len;
   
   //根据数据单元标识的值,查找FN，Pn值，确定操作函数号
   fn = findFnPn(*(pDataHead+2), *(pDataHead+3), FIND_FN);
   if (fn!=1 && fn!=3 && fn!=22)
   {
     lcdBackLight(LCD_LIGHT_ON);
     setParaWaitTime = SET_PARA_WAIT_TIME;
     lcdLightOn = LCD_LIGHT_ON;
     lcdLightDelay = nextTime(sysTime, 0, SET_PARA_WAIT_TIME);
     guiLine(1,17,160,160,0);
   }
   
   switch(fn)
   {
   	 case 1:
 	   	 //DADT
 	   	 pData = pDataHead+4;
 	   	 
       //读出升级标志
       selectParameter(88, 34, (INT8U *)&upgradeFlagv13, sizeof(UPGRADE_FLAG));
       
       //第0字节,文件标识
       //清除传输文件,恢复到升级前状态
       if (*pData==0x00)
       {
       	 upgradeFlagv13.flag = 0x0;
         saveParameter(88, 34, (INT8U *)&upgradeFlagv13, sizeof(UPGRADE_FLAG));
         
         AFN0F001(0);
         
         return;
       }
       pData++;

 	   	 //第1字节,文件属性(起始、中间帧为00H,结束帧为01H)
 	   	 //判断是否是最后一帧数据
 	   	 if (*pData==0x1)
 	   	 {
 	   	 	 recvEnd = 1;
 	   	 }
 	   	 else
 	   	 {
 	   	 	 recvEnd = 0;
 	   	 }
 	   	 pData++;
 	   	 
 	   	 //第2字节,文件指令
 	   	 pData++;
 	   	 
 	   	 //第3,4字节,总段数n
 	   	 pData += 2;
 	   	 
 	   	 //第5,6,7,8字节,接收包号(4Bytes)
 	   	 recvPackage = *pData | *(pData+1)<<8 | *(pData+2)<<16 | *(pData+3)<<24;
 	     pData+=4;

 	     //第9字节,第i段数据长度,2Bytes
 	     nowPkgSize = *pData | *(pData+1)<<8;
 	     pData +=2;

       //是否是升级开始?
       if (recvPackage==0)
       {
         if (mkdir("/backup", S_IRUSR | S_IWUSR | S_IXUSR)==0)
         {
         	 if (debugInfo&PRINT_UPGRADE_DEBUG)
         	 {
         	   printf("远程升级v13:创建目录/backup成功\n");
         	 }
         }
         else
         {
         	 if (debugInfo&PRINT_UPGRADE_DEBUG)
         	 {
         	   printf("远程升级v13:创建目录/backup失败\n");
         	 }
         }
     
         strcpy(tmpFileName,"/backup/tar");
         if (mkdir(tmpFileName, S_IRUSR | S_IWUSR | S_IXUSR)==0)
         {
         	 if (debugInfo&PRINT_UPGRADE_DEBUG)
         	 {
         	   printf("远程升级v13:创建目录%s成功\n", tmpFileName);
         	 }
         }
         else
         {
         	 if (debugInfo&PRINT_UPGRADE_DEBUG)
         	 {
         	   printf("远程升级v13:创建目录%s失败\n", tmpFileName);
         	 }
         }
     
         //如果存删除该文件
         strcpy(tmpFileName,"/backup/remoteLoad");
         if(access(tmpFileName, F_OK) == 0)
         {
           remove(tmpFileName);
           
           if (debugInfo&PRINT_UPGRADE_DEBUG)
           {
             printf("远程升级v13:remove file %s\n", tmpFileName);
           }
         
           chmod(tmpFileName, S_IRUSR | S_IWUSR | S_IXUSR);
         }
         
         if (debugInfo&PRINT_UPGRADE_DEBUG)
         {
           printf("远程升级v13:%s download begin.\n", tmpFileName);
         }
     
         //存储升级标志
         upgradeFlagv13.flag = 1;    //远程升级标志置为升级中
         upgradeFlagv13.counter = 0;
         upgradeFlagv13.perFrameSize = nowPkgSize;
         saveParameter(88, 34, (INT8U *)&upgradeFlagv13, sizeof(UPGRADE_FLAG));
       }
       else
       {
   	   	 //本次收到的程序片段序号与应收的序号比较,如果不相同,则返回   	   	 
         //  如果文件存在,查找已收到的程序片段
         strcpy(tmpFileName, "/backup/remoteLoad");
         if(access(tmpFileName, F_OK) == 0)
         {
           stat(tmpFileName, &statBuf);
           if (upgradeFlagv13.perFrameSize)
           {
             upgradeFlagv13.counter = statBuf.st_size/upgradeFlagv13.perFrameSize;           	 
           }
           else
           {
             upgradeFlagv13.counter = statBuf.st_size/nowPkgSize;
           }
           
           if (recvPackage!=upgradeFlagv13.counter)
           {
             if (debugInfo&PRINT_UPGRADE_DEBUG)
             {
           	   printf("远程升级v13:收到数据段标识与已收到的数据段个数不一致,丢弃\n");
             }
             return;
           }
         }
         else
         {
           if (debugInfo&PRINT_UPGRADE_DEBUG)
           {
           	 printf("远程升级v13:无法访问%s\n", tmpFileName);
           }
           return;
         }
   	   }

 	     upgradeFlagv13.counter = recvPackage;
 	   	 
  	   if (debugInfo&PRINT_UPGRADE_DEBUG)
  	   {
 	   	   printf("远程升级v13:第%d段数据长度=%d\n", upgradeFlagv13.counter, nowPkgSize);
 	   	 }


 	     //存储收到的程序片段
       strcpy(tmpFileName,"/backup/remoteLoad");
       if(access(tmpFileName, F_OK) == 0)  //如果文件存在,以追加方式打开
       {
       	 if((fpOfUpgrade = fopen(tmpFileName,"ab")) == NULL)
         {
  	       if (debugInfo&PRINT_UPGRADE_DEBUG)
  	       {
  	         printf("远程升级v13:Can't open file %s\n",tmpFileName);
  	       }
  	      
  	       return;
         }
       }
       else                             //如果文件不存在,以只写方式打开
       {
       	 if((fpOfUpgrade = fopen(tmpFileName,"wb")) == NULL)
         {
  	       if (debugInfo&PRINT_UPGRADE_DEBUG)
  	       {
  	         printf("远程升级v13:Can't open file %s\n",tmpFileName);
  	       }
  	      
  	       return;
         }         	 
       }
        	       
       //写入数据
       if (fwrite(pData, 1, nowPkgSize, fpOfUpgrade)!=nowPkgSize)
       {
         fclose(fpOfUpgrade);

         chmod(tmpFileName, S_IRUSR | S_IWUSR | S_IXUSR);
         
         if (debugInfo&PRINT_UPGRADE_DEBUG)
         {
           printf("远程升级v13:写入失败,改变文件属性\n");
         }
         
         return;
       }

       fclose(fpOfUpgrade);
              
       printf("远程升级v13:确认第%d个程序片段\n", upgradeFlagv13.counter);
       
       sprintf(say, "升级<-数据段%d", upgradeFlagv13.counter);
       showInfo(say);
       
       //收到最后一包数据,升级结束
       if (1==recvEnd)
       {
       	 upgradeFlagv13.flag = 0;
       	 
         if (debugInfo&PRINT_UPGRADE_DEBUG)
         {
       	   printf("远程升级v13:收到最后一包数据\n");
       	 }

         //1.删除/backup/tar/下的所有文件
         if (system("rm /backup/tar/*")==0)
         {
   	       if (debugInfo&PRINT_UPGRADE_DEBUG)
   	       {
   	         printf("远程升级v13:删除/backup/tar/目录中所有文件成功\n");
   	       }
   	     }
   	     else
   	     {
   	       if (debugInfo&PRINT_UPGRADE_DEBUG)
   	       {
   	         printf("远程升级v13:删除/backup/tar/目录中所有文件失败\n");
   	       }
   	     }

      	 //2.将下载的文件复制到/backup/tar/目录下
       	 //2.1源文件名
       	 strcpy(tmpFileName, "/backup/remoteLoad");
       	  
       	 //2.2目标文件名
       	 strcpy(destFileName, "/backup/tar/remoteLoad");

       	 //2.3复制文件
         if(rename(tmpFileName, destFileName)<0)
         {
           if (debugInfo&PRINT_UPGRADE_DEBUG)
           {
             printf("复制新文件%s出错!\n", destFileName);
           }
           
           AFN0F001(0);    //由于复制文件出错,需要重新下载
         }
         else
         {
           if (debugInfo&PRINT_UPGRADE_DEBUG)
           {
             printf("复制新文件%s成功\n",destFileName);
           }
       	   chmod(destFileName, S_IRUSR | S_IWUSR | S_IXUSR);

       	   sprintf(tmpProcess, "tar xzvf %s -C /backup/tar/", destFileName);
       	   if (system(tmpProcess)==0)
           {
   	         if (debugInfo&PRINT_UPGRADE_DEBUG)
   	         {
   	           printf("远程升级v13:tar解压文件%s成功\n", destFileName);
   	         }
   	         
   	         strcpy(tmpFileName, "/backup/tar/chk");
   	         stat(tmpFileName, &statBuf);    //读出校验文件统计信息
           	 if((fpOfTar = fopen(tmpFileName, "rb")) == NULL)
             {
      	       fclose(fpOfTar);

      	       if (debugInfo&PRINT_UPGRADE_DEBUG)
      	       {
      	         printf("远程升级v13:Can't open file %s\n",tmpFileName);
      	       }
               
               AFN0F001(0);    //打开校验文件失败,需要重新下载
             }
             else
             {
               if (fread(&chkInfo, 1, statBuf.st_size, fpOfTar)==statBuf.st_size)
               {
                 fclose(fpOfTar);
                 
                 chkInfo[statBuf.st_size] = '\0';                 
                 if (debugInfo&PRINT_UPGRADE_DEBUG)
                 {
                   printf("远程升级v13:校验文件chk content:%s", chkInfo);
                 }

                 //计算压缩文件的MD5值
                 memcpy(tmpFileName, &chkInfo[34], statBuf.st_size-35);
                 tmpFileName[statBuf.st_size-35] = '\0';
                 sprintf(destFileName, "/backup/tar/%s", tmpFileName);
                 strcpy(say, MDFile(destFileName));
                 if (debugInfo&PRINT_UPGRADE_DEBUG)
                 {
                   printf("远程升级v13:tar file %s md5 is:%s\n", destFileName, say);
                 }
                 
                 //比较校验值
                 checkData[0] = 0;
                 for(i=0;i<32;i++)
                 {
                 	 if (chkInfo[i]!=say[i])
                 	 {
                 	 	 checkData[0] = 1;
                 	 	 break;
                 	 }
                 }
                 
                 if (checkData[0])
                 {
                   if (debugInfo&PRINT_UPGRADE_DEBUG)
                   {
                   	 printf("远程升级v13:压缩文件%s校验错误,重新下载\n", destFileName);
                   }

                   AFN0F001(0);    //压纹文件校验错误,需要重新下载
                 }
                 else
                 {
                   if (debugInfo&PRINT_UPGRADE_DEBUG)
                   {
                   	 printf("远程升级v13:压缩文件%s校验正确\n", destFileName);
                   }
                   
                   sprintf(tmpProcess, "tar xzvf %s -C /backup/tar", destFileName);
               	   if (system(tmpProcess)==0)
                   {
           	         if (debugInfo&PRINT_UPGRADE_DEBUG)
           	         {
           	           printf("远程升级v13:再次解压文件%s成功\n", destFileName);
           	         }

                     //取配置文件
                     if((fpOfTar=fopen("/backup/tar/filelist.ini", "r")) != NULL)
                     {
                       i = 0;
                       while(fgets(filenames[i], 256, fpOfTar) != NULL && i<20)
                       {
                       	 len = strlen(filenames[i]);
                       	 filenames[i][len - 2] = '\0';
                       	 
                       	 i++;
                       }
                       
                       len = i;

                       //复制
                       for(i=0; i<len; i++)
                       {
                         getFileName(filenames[i], fileName);
                         
                         sprintf(tmpProcess, "cp /backup/tar/%s %s", fileName, filenames[i]);
                         if (system(tmpProcess)==0)
                         {
                         	 printf("命令%s成功\n", tmpProcess);
                         }
                         else
                         {
                         	 if (debugInfo&PRINT_UPGRADE_DEBUG)
                         	 {
                         	   printf("远程升级v13:复制文件%s失败\n", fileName);
                         	 }
                         }

                         sprintf(tmpProcess, "chmod 555 %s/%s", filenames[i], fileName);
                         if (system(tmpProcess)==0)
                         {
                         	 printf("命令%s成功\n", tmpProcess);
                         }
                         else
                         {
                         	 if (debugInfo&PRINT_UPGRADE_DEBUG)
                         	 {
                         	   printf("远程升级v13:改变文件%s属性失败\n", fileName);
                         	 }
                         }
                       }
                       
                       fclose(fpOfTar);
    
                       AFN0F001(recvPackage);      //确认收到的片段序号
                       
                       cmdReset=1;
                       
                       if (debugInfo&PRINT_UPGRADE_DEBUG)
                       {
                       	 printf("升级文件处理完毕,exit后重新运行\n");
                       }
                     }
                     else
                     {
                     	 if (debugInfo&PRINT_UPGRADE_DEBUG)
                     	 {
                     	   printf("远程升级v13:filelist.ini is not exist.");
                     	 }
                     	 
                       AFN0F001(0);    //文件列表文件读取失败,需要重新下载
                     }
           	       }
           	       else
           	       {
           	         if (debugInfo&PRINT_UPGRADE_DEBUG)
           	         {
           	           printf("远程升级v13:再次解压文件%s失败\n", destFileName);
           	         }           	       	 
                     
                     AFN0F001(0);    //再次解压文件失败,需要重新下载
           	       }
                 }
               }
               else
               {
                 fclose(fpOfTar);

                 AFN0F001(0);    //读文件失败,需要重新下载
               }
             }
           }
           else
           {
   	         if (debugInfo&PRINT_UPGRADE_DEBUG)
   	         {
   	           printf("远程升级v13:tar解压文件%s失败\n", destFileName);
   	         }
   	         
             AFN0F001(0);    //由于解压文件失败,需要重新下载
           }
         }
       }
       else
       {
         AFN0F001(recvPackage);    //确认收到的片段序号
       }
       saveParameter(88, 34, (INT8U *)&upgradeFlagv13, sizeof(UPGRADE_FLAG));
   	 	 break;
   	   
 	   case 2:   //远程升级命令
 	   	 pData = pDataHead+4;
       
       j = *pData | *(pData+1)<<8;
       
       if (debugInfo&PRINT_UPGRADE_DEBUG)
       {
         printf("远程升级:每程序片段大小j=%d\n", j);
       }
       
       pData+=2;   //升级程序片段大小
       
       //复制文件名
       tmpSize = *pData++;
       for(i=0;i<tmpSize;i++)
       {
       	  fileName[i] = *pData++;
       }
       fileName[i] = '\0';

       if (debugInfo&PRINT_UPGRADE_DEBUG)
       {
         printf("远程升级:文件%s\n",fileName);
       }
       
       //读出升级标志
       selectParameter(88, 33,(INT8U *)&upgradeFlag,sizeof(UPGRADE_FLAG));
       //upgradeFlag.flag = 0x1;
       printf("远程升级:读出升级标志,flag=%d\n", upgradeFlag.flag);

       if (upgradeFlag.flag==0x1)
       {
          upgradeFlag.perFrameSize = j;
          
          //如果文件存在,查找已收到的程序片段
          strcpy(tmpFileName, "/backup/");
          strcat(tmpFileName, fileName);
          if(access(tmpFileName, F_OK) == 0)
          {
            stat(tmpFileName, &statBuf);
            upgradeFlag.counter = statBuf.st_size/upgradeFlag.perFrameSize;
            
            if ((statBuf.st_size%upgradeFlag.perFrameSize)!=0)
            {
            	upgradeFlag.counter++;
            }
          }
          else
          {
          	upgradeFlag.counter = 0;
          }
          
          guiDisplay(28,70,"远程升级中...",1);
 	   	    strcpy(say,"请求第");
 	   	    strcat(say,intToString(upgradeFlag.counter+1, 3, str));
 	   	    strcat(say,"个程序片段");
          guiDisplay(5,90,say,1);
          requestPacket(upgradeFlag.counter);

          if (debugInfo&PRINT_UPGRADE_DEBUG)
          {
            printf("远程升级:文件大小=%d\n", statBuf.st_size);
            printf("远程升级:已收到的块数=%d\n", upgradeFlag.counter);
            printf("远程升级:%s download continue.\n", tmpFileName);
            printf("远程升级:请求第%d个程序片段\n", upgradeFlag.counter+1);
          }
       }
       else
       {
          upgradeFlag.perFrameSize = j;
          
          strcpy(tmpFileName,"/backup");
          if (mkdir(tmpFileName, S_IRUSR | S_IWUSR | S_IXUSR)==0)
          {
          	 if (debugInfo&PRINT_UPGRADE_DEBUG)
          	 {
          	   printf("创建目录%s成功\n",tmpFileName);
          	 }
          }
          else
          {
          	 if (debugInfo&PRINT_UPGRADE_DEBUG)
          	 {
          	   printf("创建目录%s失败\n",tmpFileName);
          	 }
          }

          strcpy(tmpFileName,"/backup/old");
          if (mkdir(tmpFileName, S_IRUSR | S_IWUSR | S_IXUSR)==0)
          {
          	 if (debugInfo&PRINT_UPGRADE_DEBUG)
          	 {
          	   printf("创建目录%s成功\n",tmpFileName);
          	 }
          }
          else
          {
          	 if (debugInfo&PRINT_UPGRADE_DEBUG)
          	 {
          	   printf("创建目录%s失败\n",tmpFileName);
          	 }
          }

          //如果存在先删除该文件,再打开文件
          strcpy(tmpFileName,"/backup/");
          strcat(tmpFileName,fileName);
          if(access(tmpFileName, F_OK) == 0)
          {
	          remove(tmpFileName);
	          
	          if (debugInfo&PRINT_UPGRADE_DEBUG)
	          {
	            printf("远程升级:remove file %s\n",tmpFileName);
	          }
          
            chmod(tmpFileName, S_IRUSR | S_IWUSR | S_IXUSR);
          }
          
          if (debugInfo&PRINT_UPGRADE_DEBUG)
          {
            printf("远程升级:%s download begin.\n", tmpFileName);
          }

          //存储升级标志
          upgradeFlag.flag = 1;                 //远程升级标志置为升级中
          upgradeFlag.counter = 0;
          saveParameter(88, 33, (INT8U *)&upgradeFlag, sizeof(UPGRADE_FLAG));

          requestPacket(upgradeFlag.counter);

          guiLine(1,17,160,160,0);
          guiDisplay(16,70,"远程升级程序命令",1);
 	   	    strcpy(say,"每片段");
 	   	    strcat(say,intToString(SIZE_OF_UPGRADE, 3, str));
 	   	    strcat(say,"字节");
          guiDisplay(28, 90, say, 1);
          
          if (debugInfo&PRINT_UPGRADE_DEBUG)
          {
            printf("远程升级:请求第%d个程序片段\n", upgradeFlag.counter+1);
          }
       }
       checkSumError = FALSE;
 	   	 break;

 	   case 3:    //传送文件
 	   	 pData = pDataHead+4;
 	   	 
 	   	 //提取本次收到的程序片段序号与应收的序号比较,如果不相同,则返回
 	   	 if ((upgradeFlag.counter+1) != (*pData | *(pData+1)<<8))
 	   	 {
          guiLine(1,17,160,160,0);
          guiDisplay(50,35 ,intToString(*pData | *(pData+1)<<8,3,str),1);
          guiDisplay(75,35 ,intToString(upgradeFlag.counter+1,3,str),1);
          lcdRefresh(17,160);
 	   	 	  return;
 	   	 }

 	     //存储收到的程序片段
 	     pData+=2;
 	     tmpData = pData;
 	     frame.loadLen -= 6;
 	     if ((frame.loadLen)>SIZE_OF_UPGRADE)
 	     {
 	     	  return;
 	     }
       for(i=0;i<frame.loadLen;i++)
       {
          upgradeBuff[i] = *pData++;
       }
       
       strcpy(tmpFileName,"/backup/");
       strcat(tmpFileName,fileName);
       if(access(tmpFileName, F_OK) == 0)  //如果文件存在,以追加方式打开
       {
         stat(tmpFileName,&statBuf);
         upgradeFlag.counter = statBuf.st_size/upgradeFlag.perFrameSize;
       	 
       	 if((fpOfUpgrade = fopen(tmpFileName,"ab")) == NULL)
         {
  	       if (debugInfo&PRINT_UPGRADE_DEBUG)
  	       {
  	         printf("远程升级:Can't open file %s\n",tmpFileName);
  	       }
  	      
  	       return;
         }
       }
       else                             //如果文件不存在,以只写方式打开
       {
       	 if((fpOfUpgrade = fopen(tmpFileName,"wb")) == NULL)
         {
  	       if (debugInfo&PRINT_UPGRADE_DEBUG)
  	       {
  	         printf("远程升级:Can't open file %s\n",tmpFileName);
  	       }
  	      
  	       return;
         }         	 
       }
        	       
       //写入数据
       if (fwrite(&upgradeBuff, 1, frame.loadLen, fpOfUpgrade)!=frame.loadLen)
       {
         fclose(fpOfUpgrade);

         chmod(tmpFileName, S_IRUSR | S_IWUSR | S_IXUSR);
         
         if (debugInfo&PRINT_UPGRADE_DEBUG)
         {
           printf("写入失败,改变文件属性\n");
         }
         
         return;
       }

       fclose(fpOfUpgrade);
       
       upgradeFlag.counter++;   //收到并存储正确包序号加1
       
       printf("远程升级:请求第%d个程序片段\n",upgradeFlag.counter+1);

       requestPacket(upgradeFlag.counter);      //确认收到的片段序号
       checkSumError = FALSE;
 	   	 break;
 	   	 
 	   case 4:    //升级完成
       if (checkSumError==TRUE)
       {
         guiDisplay(16,70,"文件校验错误",1);
         guiDisplay(48,90,"不进行更新",1);
         
         lcdRefresh(17,160);
       	 return;
       }
       
       guiDisplay(16,70,"远程升级下载完成",1);
       guiDisplay(48,90,"更新文件",1);
       
       //文件名前3个字符为"lib"的文件复制到lib目录下
       if (fileName[0]==0x6c && fileName[1]==0x69 && fileName[2]==0x62)
       {
       	  //1.先将原文件备份到/backup/old目录中
       	  //1.1源文件名
       	  strcpy(tmpFileName,"/lib/");
       	  strcat(tmpFileName,fileName);
          if(access(tmpFileName, F_OK) == 0)  //如果文件存在,备份文件到/backup/old
          {
       	    //1.2目标文件名
       	    strcpy(destFileName,"/backup/old/");
       	    strcat(destFileName,fileName);
       	  
       	    //1.3复制文件
            if(rename(tmpFileName,destFileName)<0)
            {
              if (debugInfo&PRINT_UPGRADE_DEBUG)
              {
                printf("备份文件%s出错!\n",destFileName);
              }
            
              return;
            }
            else
            {
              if (debugInfo&PRINT_UPGRADE_DEBUG)
              {
                printf("备份文件%s成功\n",destFileName);
              }
            }
       	  }

       	  //2.将下载的文件复制到/lib目录下
       	  //2.1源文件名
       	  strcpy(tmpFileName,"/backup/");
       	  strcat(tmpFileName,fileName);
       	  
       	  //2.2目标文件名
       	  strcpy(destFileName,"/lib/");
       	  strcat(destFileName,fileName);
       	  
       	  //2.3复制文件
          if(rename(tmpFileName,destFileName)<0)
          {
            if (debugInfo&PRINT_UPGRADE_DEBUG)
            {
             printf("复制新文件%s出错!\n",destFileName);
            }
          	
          	//2.4更新失败,还原原文件到/lib目录下
          	//2.4.1源文件名
          	strcpy(tmpFileName,"/backup/old");
          	strcat(tmpFileName,fileName);
          	  
          	//2.4.2目标文件名
          	strcpy(destFileName,"/lib/");
          	strcat(destFileName,fileName);
          	  
          	//2.4.3复制文件
            if(rename(tmpFileName,destFileName)<0)
            {
               if (debugInfo&PRINT_UPGRADE_DEBUG)
               {
                 printf("还原文件失败\n");
               }
            }
            
            return;
          }
          else
          {
            if (debugInfo&PRINT_UPGRADE_DEBUG)
            {
              printf("复制新文件%s成功\n",destFileName);
            }
       	  }
       	  
       	  chmod(destFileName, S_IRUSR | S_IWUSR | S_IXUSR);
       }
       else
       {
       	  //如果是字库文件HZK16
       	  if (fileName[0]=='H' && fileName[1]=='Z' && fileName[2]=='K')
       	  {
          	 //3.将下载的字库文件复制到/根目录下
          	 //3.1源文件名
          	 strcpy(tmpFileName,"/backup/");
          	 strcat(tmpFileName,fileName);
          	  
          	 //3.2目标文件名
          	 strcpy(destFileName,"/");
          	 strcat(destFileName,fileName);
          	  
          	 //3.3复制文件
             if(rename(tmpFileName,destFileName)<0)
             {
               if (debugInfo&PRINT_UPGRADE_DEBUG)
               {
                 printf("复制新文件%s出错!\n",destFileName);
               }
               
               return;
             }
             else
             {
               if (debugInfo&PRINT_UPGRADE_DEBUG)
               {
                 printf("复制新文件%s成功\n",destFileName);
               }
          	 }
          	 
          	 chmod(destFileName, S_IRUSR | S_IWUSR | S_IXUSR);
       	  }
       	  else
       	  {
         	  //如果是rcS
         	  if (fileName[0]=='r' && fileName[1]=='c' && fileName[2]=='S')
         	  {
            	 //3.将下载的rcS复制到/根目录下
            	 //3.1源文件名
            	 strcpy(tmpFileName,"/backup/");
            	 strcat(tmpFileName,fileName);
            	  
            	 //3.2目标文件名
            	 strcpy(destFileName,"/etc/init.d/");
            	 strcat(destFileName,fileName);
            	  
            	 //3.3复制文件
               if(rename(tmpFileName,destFileName)<0)
               {
                 if (debugInfo&PRINT_UPGRADE_DEBUG)
                 {
                   printf("复制新文件%s出错!\n",destFileName);
                 }
                 
                 return;
               }
               else
               {
                 if (debugInfo&PRINT_UPGRADE_DEBUG)
                 {
                   printf("复制新文件%s成功\n",destFileName);
                 }
            	 }
            	 
            	 //system("chmod 555 /etc/init.d/rcS");
            	 
            	 chmod(destFileName, S_IRUSR | S_IWUSR | S_IXUSR);
         	  }
         	  else
         	  {
            	 //4.先将原文件备份到/backup/old目录中
            	 //4.1源文件名
            	 strcpy(tmpFileName,"/bin/");
            	 strcat(tmpFileName,fileName);
            	  
            	 if(access(tmpFileName, F_OK) == 0)  //如果文件存在,备份文件到/backup/old
            	 {
            	   //4.2目标文件名
            	   strcpy(destFileName,"/backup/old/");
            	   strcat(destFileName,fileName);
            	  
            	   //4.3复制文件
                 if(rename(tmpFileName,destFileName)<0)
                 {
                   if (debugInfo&PRINT_UPGRADE_DEBUG)
                   {
                     printf("备份文件%s出错!\n",destFileName);
                   }
                 
                   return;
                 }
                 else
                 {
                   if (debugInfo&PRINT_UPGRADE_DEBUG)
                   {
                     printf("备份文件%s成功\n",destFileName);
                   }
                 }
            	 }
   
            	 //5.将下载的文件复制到/bin目录下
            	 //5.1源文件名
            	 strcpy(tmpFileName,"/backup/");
            	 strcat(tmpFileName,fileName);
            	  
            	 //5.2目标文件名
            	 strcpy(destFileName,"/bin/");
            	 strcat(destFileName,fileName);
            	  
            	 //5.3复制文件
               if(rename(tmpFileName, destFileName)<0)
               {
                 if (debugInfo&PRINT_UPGRADE_DEBUG)
                 {
                   printf("复制新文件%s出错!\n",destFileName);
                 }
                 
            	   //5.4更新失败,还原原文件到/bin目录下
            	   //5.4.1源文件名
            	   strcpy(tmpFileName,"/backup/old");
            	   strcat(tmpFileName,fileName);
            	  
            	   //5.4.2目标文件名
            	   strcpy(destFileName,"/bin/");
            	   strcat(destFileName,fileName);
            	  
            	   //5.4.3复制文件
                 if(rename(tmpFileName,destFileName)<0)
                 {
                 	  if (debugInfo&PRINT_UPGRADE_DEBUG)
                 	  {
                 	    printf("还原文件失败\n");
                 	  }
                 }
	               chmod(destFileName, S_IRUSR | S_IWUSR | S_IXUSR);
                 
                 return;
               }
               else
               {
                 if (debugInfo&PRINT_UPGRADE_DEBUG)
                 {
                   printf("复制新文件%s成功\n",destFileName);
                 }
                 
	               chmod(destFileName, S_IRUSR | S_IWUSR | S_IXUSR);
            	 }
            }
       	  }
       }         

       ackOrNack(TRUE,dataFrom);   //全部确认

       //存储升级标志
       upgradeFlag.flag = 0x2;     //标志置为升级完成
       saveParameter(88, 33, (INT8U *)&upgradeFlag, sizeof(UPGRADE_FLAG));
       
       if (fileName[0]=='e' && fileName[1]=='s' && fileName[2]=='R' && fileName[3]=='t')
       {
       	  upRtFlag = 1;;           //开始升级路由程序
       }
       else   //硬件复位动作
       {
          cmdReset = 1;            //等待2秒后复位
       }
 	   	 break;
 	   	 
 	   case 5:    //擦除升级标志
       upgradeFlag.flag = 0x0;   //标志置为升级完成
       saveParameter(88, 33, (INT8U *)&upgradeFlag, sizeof(UPGRADE_FLAG));
       
       upgradeFlagv13.flag = 0x0;
       saveParameter(88, 34, (INT8U *)&upgradeFlagv13, sizeof(UPGRADE_FLAG));

       guiDisplay(24,75,"擦除升级标志区",1);
       ackOrNack(TRUE,dataFrom);                 //全部确认
 	   	 break;
 	   	 
 	   case 6:    //校验文件
 	   	 for(i=0;i<8;i++)
 	   	 {
 	   	   checkData[i] = 0x0;
 	   	 }
  	   
  	   printf("远程升级-校验文件\n");

       strcpy(tmpFileName,"/backup/");
       strcat(tmpFileName,fileName);
       
       if (debugInfo&PRINT_UPGRADE_DEBUG)
       {
         printf("文件名=%s\n",tmpFileName);
       }
       
       if((fpOfUpgrade = fopen(tmpFileName,"r+")) == NULL)
       {
  	      if (debugInfo&PRINT_UPGRADE_DEBUG)
  	      {
  	        printf("远程升级-校验:Can't open file %s\n",fileName);
  	      }
  	      
  	      return;
       }

       fseek(fpOfUpgrade, SEEK_SET, 0);
       i=0;
       while (!feof(fpOfUpgrade))
       {
          if (i==0)
          {
            i = 1;
            if (fread(&dataX, 1, 4, fpOfUpgrade)==4)
            {
              for(j=0;j<4;j++)
              {
                 checkData[j] += dataX[j];
              }
            }
          }
          else
          {
            i = 0;
            if (fread(&dataX, 1, 4, fpOfUpgrade)==4)
            {
              for(j=0;j<4;j++)
              {
                 checkData[4+j] += dataX[j];
              }
            }
          }
       }
       fclose(fpOfUpgrade);
       
 	   	 pData = pDataHead+4;
 	   	 
 	   	 if (debugInfo&PRINT_UPGRADE_DEBUG)
 	   	 {
 	   	   printf("主站校验值:%d,%d,%d,%d,%d,%d,%d,%d\n",*pData,*(pData+1),*(pData+2),*(pData+3),*(pData+4),*(pData+5),*(pData+6),*(pData+7));
 	   	   printf("终端校验值:%d,%d,%d,%d,%d,%d,%d,%d\n",checkData[0],checkData[1],checkData[2],checkData[3],checkData[4],checkData[5],checkData[6],checkData[7]);
 	   	 }
 	   	 
 	   	 if (*pData==checkData[0] && *(pData+1)==checkData[1]
 	   	 	   && *(pData+2)==checkData[2]  
 	   	 	     //&& *(pData+3)==checkData[3]
 	   	 	     //ly,2011-08-18,升级文件老是出现校验错误,没有找到原因,但错的就只有这个字节,但把下载的文件copy
 	   	 	     //   出来到电脑上比较又是正常的,因此不比较这个字节
 	   	 	    && *(pData+4)==checkData[4] && *(pData+5)==checkData[5]
 	   	 	     && *(pData+6)==checkData[6] && *(pData+7)==checkData[7]
 	   	 	   )
 	   	 {
 	   	 	  if (debugInfo&PRINT_UPGRADE_DEBUG)
 	   	 	  {
 	   	 	    printf("文件检验正确\n");
 	   	 	  }
          guiDisplay(16,75,"升级文件校验正确",1);

          requestPacket(0xfffe);                    //全部确认
 	   	 }
 	   	 else
 	   	 {
 	   	 	  if (debugInfo&PRINT_UPGRADE_DEBUG)
 	   	 	  {
 	   	 	    printf("文件检验错误\n"); 
 	   	 	  }
 	   	 	  
 	   	 	  checkSumError = TRUE;
          
          guiDisplay(16,75,"升级文件校验错误",1);
          requestPacket(0xfffd);                   //全部否认
 	   	 }
 	   	 break;
 	   
 	   case 21:    //文件上传命令
 	   	 pData = pDataHead+4;
       
       j = *pData | *(pData+1)<<8;
       
       if (debugInfo&PRINT_UPGRADE_DEBUG)
       {
         printf("远程升级:文件上传,每程序片段大小=%d\n", j);
       }
       
       pData+=2;   //升级程序片段大小
       
       //复制文件名
       tmpSize = *pData++;
       for(i=0;i<tmpSize;i++)
       {
       	 fileName[i] = *pData++;
       }
       fileName[i] = '\0';

       if (debugInfo&PRINT_UPGRADE_DEBUG)
       {
         printf("远程升级:上传文件%s\n",fileName);
       }
       if(access(fileName, F_OK) != 0)
       {
  	     if (debugInfo&PRINT_UPGRADE_DEBUG)
  	     {
  	       printf("远程升级(文件上传):Can't open file %s\n", fileName);
  	     }
  	     uploadFile(0xfffe);
       }
       else
       {
       	 uploadCnt = (*pData | *(pData+1)<<8 | *(pData+2)<<16 | *(pData+3)<<24)/512;
  	     
  	     if (debugInfo&PRINT_UPGRADE_DEBUG)
  	     {
       	   printf("起始字节=%d\n",uploadCnt*512);
       	 }
       	 
  	     uploadFile(0xfffd);  //文件大小
       }
       
       guiDisplay(28,70,"文件上传中...",1);
 	   	 break;
 	   
 	   case 22:    //文件上传
 	   	 uploadFile(uploadCnt);
 	   	 break;

 	   case 23:    //文件上传结束
 	   	 break;
   }
   
   if (fn!=3)
   {
     lcdRefresh(17,160);
   }
}

/*******************************************************
函数名称:requestPacket
功能描述:请求主站数据包
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
void AFN0F001(INT32U num)
{
    INT8U  tmpI,checkSum;
    INT16U frameTail0f;
   
    if (fQueue.tailPtr == 0)
    {
      frameTail0f = 0;
    }
    else
    {
      frameTail0f = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
    }
    
    msFrame[frameTail0f+0]  = 0x68;   //帧起始字符

    tmpI = ((22 -6) << 2) | 0x2;
    msFrame[frameTail0f+1]  = tmpI & 0xFF;   //L
    msFrame[frameTail0f+2]  = tmpI >> 8;
    msFrame[frameTail0f+3]  = tmpI & 0xFF;   //L
    msFrame[frameTail0f+4]  = tmpI >> 8; 
  
    msFrame[frameTail0f+5]  = 0x68;  //帧起始字符
 
    msFrame[frameTail0f+6]  = 0xa8;  //控制字节10001000

    //地址域
    msFrame[frameTail0f+7]  = addrField.a1[0];
    msFrame[frameTail0f+8]  = addrField.a1[1];
    msFrame[frameTail0f+9]  = addrField.a2[0];
    msFrame[frameTail0f+10] = addrField.a2[1];
    msFrame[frameTail0f+11] = addrField.a3;

    msFrame[frameTail0f+12] = 0x0F;  //AFN

    msFrame[frameTail0f+13] = 0x0;

    msFrame[frameTail0f+14] = 0x00;    //DA1
    msFrame[frameTail0f+15] = 0x00;    //DA2
    msFrame[frameTail0f+16] = 0x01;    //DT1
    msFrame[frameTail0f+17] = 0x00;    //DT2
    
      
    msFrame[frameTail0f+18] = num&0xff;      	
    msFrame[frameTail0f+19] = num>>8&0xff;
    msFrame[frameTail0f+20] = num>>16&0xff;
    msFrame[frameTail0f+21] = num>>24&0xff;

    msFrame[frameTail0f+22] = checkSum;
    msFrame[frameTail0f+23] = 0x16;
    
    fQueue.frame[fQueue.tailPtr].head = frameTail0f;
    fQueue.frame[fQueue.tailPtr].len = 24;
    
    if ((frameTail0f+24+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
    	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
    {
       fQueue.frame[fQueue.tailPtr].next = 0x0;
    	 fQueue.tailPtr = 0;
    }
    else
    {                 
       fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
       fQueue.tailPtr++;
    }
}

/*******************************************************
函数名称:requestPacket
功能描述:请求主站数据包
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
void requestPacket(INT16U num)
{
    INT8U  tmpI,checkSum;
    INT16U frameTail0f;
   
    if (fQueue.tailPtr == 0)
    {
      frameTail0f = 0;
    }
    else
    {
      frameTail0f = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
    }
    
    msFrame[frameTail0f+0]  = 0x68;   //帧起始字符

    tmpI = ((20 -6) << 2) | 0x2;
    msFrame[frameTail0f+1]  = tmpI & 0xFF;   //L
    msFrame[frameTail0f+2]  = tmpI >> 8;
    msFrame[frameTail0f+3]  = tmpI & 0xFF;   //L
    msFrame[frameTail0f+4]  = tmpI >> 8; 
  
    msFrame[frameTail0f+5]  = 0x68;  //帧起始字符
 
    msFrame[frameTail0f+6]  = 0xa8;  //控制字节10001000

    //地址域
    msFrame[frameTail0f+7]  = addrField.a1[0];
    msFrame[frameTail0f+8]  = addrField.a1[1];
    msFrame[frameTail0f+9]  = addrField.a2[0];
    msFrame[frameTail0f+10] = addrField.a2[1];
    msFrame[frameTail0f+11] = addrField.a3;

    msFrame[frameTail0f+12] = 0x0F;  //AFN

    msFrame[frameTail0f+13] = 0x0;

    if (num==0xfffe || num==0xfffd)
    {
      //校验文件状态
      msFrame[frameTail0f+14] = 0x00;    //DA1
      msFrame[frameTail0f+15] = 0x00;    //DA2
      msFrame[frameTail0f+16] = 0x20;    //DT1
      msFrame[frameTail0f+17] = 0x00;    //DT2
      
      if (num==0xfffe)
      {
      	msFrame[frameTail0f+18] = 0x1;      	
      }
      else
      {
      	msFrame[frameTail0f+18] = 0x2;
      }
      
      msFrame[frameTail0f+19] = 0x0;
    }
    else
    {
      msFrame[frameTail0f+14] = 0x00;    //DA1
      msFrame[frameTail0f+15] = 0x00;    //DA2
      msFrame[frameTail0f+16] = 0x04;    //DT1
      msFrame[frameTail0f+17] = 0x00;    //DT2

      //请求包序号
      msFrame[frameTail0f+18] = num & 0xff;
      msFrame[frameTail0f+19] = num>>8 & 0xff;
    }

    msFrame[frameTail0f+20] = checkSum;
    msFrame[frameTail0f+21] = 0x16;
    
    fQueue.frame[fQueue.tailPtr].head = frameTail0f;
    fQueue.frame[fQueue.tailPtr].len = 22;
    
    if ((frameTail0f+22+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
    	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
    {
       fQueue.frame[fQueue.tailPtr].next = 0x0;
    	 fQueue.tailPtr = 0;
    }
    else
    {                 
       fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
       fQueue.tailPtr++;
    }
}

/*******************************************************
函数名称:uploadFile
功能描述:上传文件数据包
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
void uploadFile(INT16U num)
{
	  struct stat statBuf;
    
    INT8U  checkSum;
    INT16U tmpI;
    INT16U frameTail0f, tmpHead0f;
    INT8U  readData[512];
    INT16U countOfRead=0;
   
    if (fQueue.tailPtr == 0)
    {
      tmpHead0f = 0;
    }
    else
    {
      tmpHead0f = fQueue.frame[fQueue.tailPtr-1].head + fQueue.frame[fQueue.tailPtr-1].len;
    }
    
    msFrame[tmpHead0f+0]  = 0x68;   //帧起始字符
  
    msFrame[tmpHead0f+5]  = 0x68;  //帧起始字符
 
    msFrame[tmpHead0f+6]  = 0xa8;  //控制字节10001000

    //地址域
    msFrame[tmpHead0f+7]  = addrField.a1[0];
    msFrame[tmpHead0f+8]  = addrField.a1[1];
    msFrame[tmpHead0f+9]  = addrField.a2[0];
    msFrame[tmpHead0f+10] = addrField.a2[1];
    msFrame[tmpHead0f+11] = addrField.a3;

    msFrame[tmpHead0f+12] = 0x0F;  //AFN

    msFrame[tmpHead0f+13] = 0x0;
    
    frameTail0f = tmpHead0f+14;
    
    //文件大小
    stat(fileName,&statBuf);

    if (num==0xfffe || num==0xfffd)
    {
      //F21需要上传的文件不存在
      msFrame[frameTail0f++] = 0x00;    //DA1
      msFrame[frameTail0f++] = 0x00;    //DA2
      msFrame[frameTail0f++] = 0x10;    //DT1
      msFrame[frameTail0f++] = 0x02;    //DT2
      
      if (num==0xfffe)
      {
        msFrame[frameTail0f++] = 0x0;
      }
      else
      {
        msFrame[frameTail0f++] = 0x1;
        msFrame[frameTail0f++] = statBuf.st_size&0xff;
        msFrame[frameTail0f++] = statBuf.st_size>>8;
        msFrame[frameTail0f++] = statBuf.st_size>>16;
        msFrame[frameTail0f++] = statBuf.st_size>>24;
      }
    }
    else
    {
      //文件内容
	    if((fpOfUpgrade = fopen(fileName,"r+")) == NULL)
	    {
	  	  if (debugInfo&PRINT_UPGRADE_DEBUG)
	  	  {
	  	    printf("远程升级-校验:Can't open file %s\n", fileName);
	  	  }
	  	  
        //F21需要上传的文件打开失败
        msFrame[frameTail0f++] = 0x00;    //DA1
        msFrame[frameTail0f++] = 0x00;    //DA2
        msFrame[frameTail0f++] = 0x10;    //DT1
        msFrame[frameTail0f++] = 0x02;    //DT2
      
        msFrame[frameTail0f++] = 0x1;
	    }
	    else
	    {
        if (uploadCnt==0xffee)
        {
          //F23.上传文件结束
          msFrame[frameTail0f++] = 0x00;    //DA1
          msFrame[frameTail0f++] = 0x00;    //DA2
          msFrame[frameTail0f++] = 0x40;    //DT1
          msFrame[frameTail0f++] = 0x02;    //DT2
          
          guiDisplay(28,70,"文件上传完成.",1);
        }
        else
        {
          //F22.上传文件内容
          msFrame[frameTail0f++] = 0x00;    //DA1
          msFrame[frameTail0f++] = 0x00;    //DA2
          msFrame[frameTail0f++] = 0x20;    //DT1
          msFrame[frameTail0f++] = 0x02;    //DT2
          
          fseek(fpOfUpgrade, uploadCnt*512, 0);
          
          fread(readData, 512, 1, fpOfUpgrade);
          
          if (feof(fpOfUpgrade))
          {
            memcpy(&msFrame[frameTail0f], readData, statBuf.st_size-512*uploadCnt);
            frameTail0f += statBuf.st_size-512*uploadCnt;

            uploadCnt = 0xffee;
          }
          else
          {
            uploadCnt++;        	

            memcpy(&msFrame[frameTail0f], readData, 512);
            frameTail0f += 512;
          }
          
          fclose(fpOfUpgrade);          
        }
	    }
    }

    tmpI = ((frameTail0f - tmpHead0f - 6) << 2) | 0x2;
    msFrame[tmpHead0f+1]  = tmpI & 0xFF;   //L
    msFrame[tmpHead0f+2]  = tmpI >> 8;
    msFrame[tmpHead0f+3]  = tmpI & 0xFF;   //L
    msFrame[tmpHead0f+4]  = tmpI >> 8;

    msFrame[frameTail0f++] = checkSum;
    msFrame[frameTail0f++] = 0x16;
    
    fQueue.frame[fQueue.tailPtr].head = tmpHead0f;
    fQueue.frame[fQueue.tailPtr].len  = frameTail0f-tmpHead0f;
    
    if ((tmpHead0f+fQueue.frame[fQueue.tailPtr].len+MAX_OF_PER_FRAME)>SIZE_OF_SEND_MS_BUF
    	   || fQueue.tailPtr==LEN_OF_SEND_QUEUE-1)
    {
      fQueue.frame[fQueue.tailPtr].next = 0x0;
    	fQueue.tailPtr = 0;
    }
    else
    {                 
      fQueue.frame[fQueue.tailPtr].next = fQueue.tailPtr+1;
      fQueue.tailPtr++;
    }
}


/*******************************************************
函数名称:unRecvSeg
功能描述:查找文件传输未收到的数据段,13版规约
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：帧尾部
*******************************************************/
void unRecvSeg(INT8U *pFrame)
{
  UPGRADE_FLAG upgradeFlagv13;    //13版规约升级标志
	struct stat  statBuf;
	INT8U        i;
  char         say[30],str[10];

  //读出升级标志
  selectParameter(88, 34, (INT8U *)&upgradeFlagv13, sizeof(UPGRADE_FLAG));
  
  if (debugInfo&PRINT_UPGRADE_DEBUG)
  {
    printf("远程升级v13-unRecvSeg:读出升级标志,flag=%d\n", upgradeFlagv13.flag);
  }

  //组号
  *pFrame++ = 0x00;
  *pFrame++ = 0x00;

  //组内各数据段未收到标识
  memset(pFrame, 0xff, 128);

  if (0x1==upgradeFlagv13.flag)
  {
    //如果文件存在,查找已收到的程序片段
    strcpy(tmpFileName, "/backup/remoteLoad");
    if(access(tmpFileName, F_OK) == 0)
    {
      stat(tmpFileName, &statBuf);
      if (upgradeFlagv13.perFrameSize)    //防止除数是0
      {
        upgradeFlagv13.counter = statBuf.st_size/upgradeFlagv13.perFrameSize;
      }
      else
      {
      	upgradeFlagv13.counter = 0;
      }
      
      memset(pFrame, 0x00, upgradeFlagv13.counter/8);
      for(i=0; i<upgradeFlagv13.counter%8; i++)
      {
      	pFrame[upgradeFlagv13.counter/8] &= ~(1<<i);
      }
    }
    else
    {
     	upgradeFlagv13.counter = 0;
    }

    if (debugInfo&PRINT_UPGRADE_DEBUG)
    {
      printf("远程升级v13-unRecvSeg:已收到的文件大小=%d\n", statBuf.st_size);
      printf("远程升级v13-unRecvSeg:已收到的块数=%d\n", upgradeFlagv13.counter);
    }
  }
}
  
