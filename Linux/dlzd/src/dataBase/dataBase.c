/***************************************************
Copyright,2009,Huawei WoDian co.,LTD,All	Rights Reserved
文件名:dataBase.c
作者:leiyong
版本:0.9
完成日期:2009年 月
描述:电力终端(负控终端/集中器,AT91SAM9260处理器)数据库处理文件
函数列表:
  1.
修改历史:
  01,09-10-28,Leiyong created.
  02,14-09-26,Leiyong,增加,每天晚上0点7分清理一次过期数据

***************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "sqlite3.h"

#include "common.h"
#include "msSetPara.h"
#include "teRunPara.h"
#include "convert.h"
#include "att7022b.h"

#include "workWithMeter.h"
#include "copyMeter.h"
#include "userInterface.h"

#include "wlModem.h"
#include "hardwareConfig.h"

#include "dataBalance.h"
#include "dataBase.h"

//变量定义
sqlite3 *sqlite3Db;                //全局SQLite3数据库句柄(SQLite db handle)

INT32U  iEventStartPtr;            //重要事件第1条记录指针
INT32U  nEventStartPtr;            //重要事件第1条记录指针
extern  char originPassword[7];    //原始密码

INT8U   dbLocker=0;                //写数据库锁
INT8U   dbMonitor=0;               //监视到数据库异常次数

/*******************************************************
函数名称:sqlite3Aberrant
功能描述:Sqlite3异常处理
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:
*******************************************************/
BOOL sqlite3Aberrant(INT16U aberrantCode)
{
	char tmpStr[100];
	
	if (aberrantCode!=SQLITE_DONE && aberrantCode!=SQLITE_ROW)
	{
	  sprintf(tmpStr,"sqlite3Aberrant:(%s) code=%d", sqlite3_errmsg(sqlite3Db), aberrantCode);

	  if (debugInfo&PRINT_DATA_BASE)
	  {
	    printf("%s\n",tmpStr);
	  }
	  
	  logRun(tmpStr);
	  
	  if (aberrantCode!=SQLITE_ERROR)
	  {
  	  //2012-09-04,监视数据库异常次数
  	  dbMonitor++;
  	  if (dbMonitor>50)
  	  {
  	  	sqlite3_close(sqlite3Db);
  	  	
  	  	sleep(1);
  	  	
  	  	logRun("sqlite3Aberrant:monitered aberrant times is over 50,exit process");
  	  	
  	  	printf("sqlite3Aberrant:monitered aberrant times is over 50,exit process\n");
  	  	exit(0);
  	  }
  
  	  if (debugInfo&PRINT_DATA_BASE)
  	  {
  	    printf("sqlite3Aberrant:监视数据库异常次数为%d\n", dbMonitor);
  	  }
  	}
	}
	
	switch(aberrantCode)
  {
  	case SQLITE_PERM:    //拒绝访问
  	case SQLITE_BUSY:    //数据库忙
  	case SQLITE_LOCKED:  //数据库锁定
  	case SQLITE_MISUSE:  //Library used incorrectly
      //关闭数据库
      sqlite3_close(sqlite3Db);

      //打开数据库
      if (sqlite3_open("powerData.db", &sqlite3Db))
      {
        printf("sqlite3Aberrant:Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
        sqlite3_close(sqlite3Db);
        
        logRun("sqlite3Aberrant:detected database locked or access permission denied, and reopen failure, process over\n");
        
        exit(0);
      }
      else
      {
        sprintf(tmpStr,"sqlite3Aberrant:database aberrant,aberrant code=%d,reopen database success", aberrantCode);
        logRun(tmpStr);
        printf("%s\n", tmpStr);
      }
  	 	break;
  }

  return TRUE;
}

/*******************************************************
函数名称:sqlite3Watch
功能描述:Sqlite3监视
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:

历史:
   1)ly,2012-04-25,添加此函数.旨在监视数据库状态
   2)ly,2012-07-28,测试发现,一旦访问数据库失败,重新打开还是访问数据库失败,修改为访问数据库失败,直接退出进程,由守护重新开始进程执行
*******************************************************/
void sqlite3Watch(void)
{
  char    *errMessage;               //错误消息(Error msg written her)

  if (sqlite3_exec(sqlite3Db, "select acc_pn from f10_info limit 1;", NULL, NULL, &errMessage)==SQLITE_OK)   //查询成功
  {
  	;
  }
  else
  {
    //关闭数据库
    sqlite3_close(sqlite3Db);

    //打开数据库
    if (sqlite3_open("powerData.db", &sqlite3Db))
    {
      printf("sqlite3Watch:Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
      sqlite3_close(sqlite3Db);
    }
    
    logRun("sqlite3Watch:数据库异常.结束进程\n");
    
    printf("sqlite3Watch:数据库异常,结束进程\n");
    
    exit(0);
  }
  
  return TRUE;
}

/*******************************************************
函数名称:saveBakKeyPara
功能描述:保存备份关键登录参数
调用函数:
被调用函数:
输入参数:type 1-终端地址及行政区划码
              2-VPN用户名密码
              3-主站IP地址及端口
输出参数:
返回值:

历史:
*******************************************************/
void saveBakKeyPara(INT8U type)
{
	FILE *fp;

 	switch(type)
  {
  	case 3:    //主站IP地址和端口(28Bytes)
      if((fp=fopen("/keyPara003","wb"))==NULL)
      {
  	    logRun("open file keyPara003 (for save bak key para) failure.");
  	    return;
      }

  		fwrite(&ipAndPort, sizeof(IP_AND_PORT), 1, fp);
  		break;
			
		case 7: 	 //终端IP地址和端口(存1Bytes),2020-11-18,Add
			if((fp=fopen("/keyPara007","wb"))==NULL)
			{
				logRun("open file keyPara007 (for save bak key para) failure.");
				return;
			}
		
			fwrite(&teIpAndPort.ethIfLoginMs, 1, 1, fp);
			break;

  	case 16:    //VPN用户名密码(64Bytes)
      if((fp=fopen("/keyPara016","wb"))==NULL)
      {
  	    logRun("open file keyPara016 (for save bak key para) failure.");
  	    return;
      }
      
  		fwrite(&vpn, 64, 1, fp);
  		break;

  	case 121:    //地址(5Bytes)
      if((fp=fopen("/keyPara121","wb"))==NULL)
      {
  	    logRun("open file keyPara121 (for save bak key para) failure.");
  	    return;
      }

  		fwrite(&addrField, 5, 1, fp);
  		break;

  	case 129:    //交采校表参数(80+xBytes)
      if((fp=fopen("/keyPara129","wb"))==NULL)
      {
  	    logRun("open file keyPara129 (for save bak key para) failure.");
  	    return;
      }

  		fwrite(&acSamplePara, sizeof(AC_SAMPLE_PARA), 1, fp);
  		break;  
  }
  
  printf("saveBakKeyPara:type=%d ok\n", type);

  fclose(fp);
}

/*******************************************************
函数名称:readBakKeyPara
功能描述:读取备份关键登录参数
调用函数:
被调用函数:
输入参数:type 1-终端地址及行政区划码
              2-VPN用户名密码
              3-主站IP地址及端口
输出参数:
返回值:

历史:
*******************************************************/
void readBakKeyPara(INT8U type,INT8U *buf)
{
	FILE *fp;
  
 	switch(type)
  {
  	case 3:    //主站IP地址和端口(28Bytes)
      if((fp=fopen("/keyPara003","rb"))==NULL)
      {
      	logRun("open file keyPara003 (for read bak key para) failure.");
      	return;
      }

  		fread(buf, sizeof(IP_AND_PORT), 1, fp);
  		break;
			
		case 7: 	 //终端IP地址和端口(存1Bytes),2020-11-18,Add
			if((fp=fopen("/keyPara007","rb"))==NULL)
			{
				logRun("open file keyPara007 (for read bak key para) failure.");
				return;
			}
		
			fread(&teIpAndPort.ethIfLoginMs, 1, 1, fp);
			break;
  	case 16:    //VPN用户名密码(64Bytes)
      if((fp=fopen("/keyPara016","rb"))==NULL)
      {
      	logRun("open file keyPara016 (for read bak key para) failure.");
      	return;
      }
      
  		fread(buf, 64, 1, fp);
  		break;

  	case 121:    //地址(5Bytes)
      if((fp=fopen("/keyPara121","rb"))==NULL)
      {
      	logRun("open file keyPara121 (for read bak key para) failure.");
      	return;
      }
  
  		fread(buf, 5, 1, fp);
  		break;

  	case 129:    //交采校表参数(80+xBytes)
      if((fp=fopen("/keyPara129","rb"))==NULL)
      {
      	logRun("open file keyPara129 (for read bak key para) failure.");
      	return;
      }
  
  		fread(buf, sizeof(AC_SAMPLE_PARA), 1, fp);
  		break;
  }

  fclose(fp);
}

/*******************************************************
函数名称:initDataBase
功能描述:初始化数据库
         检查数据库(/表)是否存在,如果不存在则建立数据库或数据库表         
调用函数:
被调用函数:
输入参数:
输出参数:
输出参数:
返回值:void
*******************************************************/
void initDataBase(void)
{  
  char    *pSqlStr;                  //SQL语句字符串指针
  char    *errMessage;               //错误消息(Error msg written her)
  int     result;
	sqlite3_stmt  *stat;

  //查看系统是否支持ppp协议,如果支持,则使用ppp拨号
  //12-11-06
  if(access("/dev/ppp", F_OK) == 0)
  {
    operateModem  = 0;           //暂停操作modem
    bakModuleType = MODEM_PPP;
  }
  else
  {
  	operateModem = 1;         //可以操作modem
  }

  //主数据库 start-----------------------------------------------------------------

  //0.打开数据库
  if (sqlite3_open("powerData.db", &sqlite3Db))
  {
     printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
     sqlite3_close(sqlite3Db);
     return;
  }
  
  //1.参数表
  //1.1 f10_info
  if (sqlite3_exec(sqlite3Db, "select acc_pn from f10_info limit 1;", NULL, NULL, &errMessage)==SQLITE_OK)   //查询成功
  {
  	 if (debugInfo&PRINT_DATA_BASE)
  	 {
  	   printf("f10_info参数表已经建立\n");
  	 }
  }
  else
  {
	   pSqlStr = "create table f10_info(acc_pn INTEGER, acc_port INTEGER, acc_meter_addr blob, acc_num INTEGER, acc_data blob)";

     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("创建参数表f10_info成功\n");
     }
     else
     {
     	  printf("创建参数表f10_info失败\n");
     }
	}
	
	//1.2 base_info
  if (sqlite3_exec(sqlite3Db, "select acc_afn from base_info limit 1;", NULL, NULL, &errMessage)==SQLITE_OK)   //查询成功
  {
  	 if (debugInfo&PRINT_DATA_BASE)
  	 {
  	   printf("base_info参数表已经建立\n");
  	 }
  }
  else
  {
	   pSqlStr = "create table base_info(acc_afn blob, acc_fn blob, acc_data blob)";	
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("创建参数表base_info成功\n");
     }
     else
     {
     	  printf("创建参数表base_info失败\n");
     }
	}
	
	//1.3 base_vice_info
  if (sqlite3_exec(sqlite3Db, "select acc_afn from base_vice_info limit 1;", NULL, NULL, &errMessage)==SQLITE_OK)   //查询成功
  {
  	 if (debugInfo&PRINT_DATA_BASE)
  	 {
  	   printf("base_vice_info参数表已经建立\n");
  	 }
  }
  else
  {
	   pSqlStr = "create table base_vice_info(acc_afn blob, acc_fn blob, acc_pn blob, acc_data blob)";	
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("创建参数表base_vice_info成功\n");
     }
     else
     {
     	  printf("创建参数表base_vice_info失败\n");
     }
	}

  //2.三相表实时数据表
  checkSpRealTable(1);
  
  //3.上月数据表
  if (sqlite3_exec(sqlite3Db, "select pn from lastMonthData limit 1;", NULL, NULL, &errMessage)==SQLITE_OK)   //查询成功
  {
  	 if (debugInfo&PRINT_DATA_BASE)
  	 {
  	   printf("上月数据表已经建立\n");
  	 }
  }
  else
  {
     pSqlStr = "create table lastMonthData(pn int,queryType int,dataType int,time int,data blob);";
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("创建表lastMonthData成功\n");
     }
     else
     {
     	  printf("创建表lastMonthData失败\n");
     }
  }
  
  //4.日结算数据表
  if (sqlite3_exec(sqlite3Db, "select pn from dayBalanceData limit 1;", NULL, NULL, &errMessage)==SQLITE_OK)   //查询成功
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("日结算数据表已经建立\n");
  	}
  }
  else
  {
     pSqlStr = "create table dayBalanceData(pn int,queryType int,dataType int,freezeTime int, copyTime int,data blob);";
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("创建表dayBalanceData成功\n");
     }
     else
     {
     	  printf("创建表dayBalanceData失败\n");
     }
  }
  
  //5.月结算数据表
  if (sqlite3_exec(sqlite3Db, "select pn from monthBalanceData limit 1;", NULL, NULL, &errMessage)==SQLITE_OK)   //查询成功
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("月结算数据表已经建立\n");
  	}
  }
  else
  {
     pSqlStr = "create table monthBalanceData(pn int,queryType int,dataType int,freezeTime int, copyTime int,data blob);";
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("创建表monthBalanceData成功\n");
     }
     else
     {
     	  printf("创建表monthBalanceData失败\n");
     }
  }

  //6.事件记录表
  if (sqlite3_exec(sqlite3Db, "select storeNo from eventRecord limit 1;", NULL, NULL, &errMessage)==SQLITE_OK)   //查询成功
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("事件记录表已经建立\n");
  	}
  }
  else
  {
     pSqlStr = "create table eventRecord(storeNo int, eventType int, len int, data blob);";
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("创建表eventRecord成功\n");
     }
     else
     {
     	  printf("创建表eventRecord失败\n");
     }
  }
  
  //7.终端/电表统计数据表
  if (sqlite3_exec(sqlite3Db, "select pn from statisData limit 1;", NULL, NULL, &errMessage)==SQLITE_OK)   //查询成功
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("事件记录表已经建立\n");
  	}
  }
  else
  {
     pSqlStr = "create table statisData(pn int, day int, month int, year int, data blob);";
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("创建表statisData 成功\n");
     }
     else
     {
     	  printf("创建表statisData失败\n");
     }
  }
  
  //8.实时数据表
  checkSpRealTable(0);    //单相表

  //9.单相表日冻结数据
  pSqlStr = "select pn from singlePhaseDay limit 1;";
  if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)   //查询成功
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("单相表日冻结数据表已经建立\n");
  	}
  }
  else
  {
     pSqlStr = "create table singlePhaseDay(pn int,freezeTime int, copyTime int,data blob);";
     
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("创建表singlePhaseDay成功\n");
     }
     else
     {
     	  printf("创建表singlePhaseDay失败\n");
     }
  }
  
  //10.单相表月冻结数据
  pSqlStr = "select pn from singlePhaseMonth limit 1;";
  if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)   //查询成功
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("单相表月冻结数据表已经建立\n");
  	}
  }
  else
  {
     pSqlStr = "create table singlePhaseMonth(pn int, freezeTime int, copyTime int,data blob);";
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("创建表singlePhaseMonth成功\n");
     }
     else
     {
     	  printf("创建表singlePhaseMonth失败\n");
     }
  }
  
 #ifdef LOAD_CTRL_MODULE
  //11.剩余电量表
  pSqlStr = "select pn from leftPower limit 1;";
  if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)   //查询成功
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("剩余电量数据表已经建立\n");
  	}
  }
  else
  {
     pSqlStr = "create table leftPower(pn int,data blob);";
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("创建表leftPower成功\n");
     }
     else
     {
     	  printf("创建表leftPower失败\n");
     }
  }
 #endif
 
  //12.整点冻结数据表
  checkSpRealTable(2);
  
  //13.交采示值数据表
  pSqlStr = "select data from acVision;";
  if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)   //查询成功
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("交采示值数据表已经建立\n");
  	}
  }
  else
  {
     pSqlStr = "create table acVision(dataType int, time int, data blob);";
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	 printf("创建表acVision成功\n");
     }
     else
     {
     	 printf("创建表acVision失败\n");
     }
  }
  
  //14.直流模拟量数据
 #ifdef PLUG_IN_CARRIER_MODULE
  pSqlStr = "select pn from dcAnalog limit 1;";
  if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)   //查询成功
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("直流模拟量数据表已经建立\n");
  	}
  }
  else
  {
     pSqlStr = "create table dcAnalog(pn int, copyTime int, data blob);";
     
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
     	  printf("创建表dcAnalog成功\n");
     }
     else
     {
     	  printf("创建表dcAnalog失败\n");
     }
  }
 #endif
}

/*******************************************************
函数名称:deleteData
功能描述:删除库中数据         
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void deleteData(INT8U type)
{
   sqlite3      *sqlite3Dbx;
   INT16U       i;
   INT8U        tmpFn;
   char         pSqlStr[512];          //SQL语句字符串指针
	 INT16U       result;
	 sqlite3_stmt *stat;
   DATE_TIME    tmpTime;
   char         tableName[30], tblNamex[30];
   char         *errMessage;           //错误消息(Error msg written her)
	 INT8U        leftPower[12];
	 struct clearQueue *pClrHead=NULL;
	 struct clearQueue *tmpNode, *prevNode;
	 
	#ifdef LIGHTING
	 INT8U        irData160[250];
	 INT8U 			  irData161[250];
	 INT8U 			  irData162[250];
	 INT8U 			  irData163[250];
	#endif

	 char   dataTable[][20]={"lastMonthData", "dayBalanceData","monthBalanceData", "eventRecord", 
	 	                     "statisData", "singlePhaseDay","singlePhaseMonth","dcAnalog"};
   char   paraTable[][20]={"base_info", "base_vice_info", "f10_info", ""};
   
   //1.为了drop表必须将原有sqlite指针关掉,再开一个才能drop
   if (debugInfo&PRINT_DATA_BASE)
   {
   	 printf("Database-deleteData:为drop table而关闭sqlite3全局指针sqlite3Db,用sqlite3Dbx打开数据库\n");
   }
   sqlite3_close(sqlite3Db);
   if (sqlite3_open("powerData.db", &sqlite3Dbx))
   {
     printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Dbx));
     sqlite3_close(sqlite3Dbx);
     
     //ly,2011-10-12,add,打不开新的全局指针,还原为原全局指针
     usleep(2000);
     if (sqlite3_open("powerData.db", &sqlite3Db))
     {
       printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
       sqlite3_close(sqlite3Db);
       
       usleep(2000);
       if (sqlite3_open("powerData.db", &sqlite3Db))
       {
         printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
         sqlite3_close(sqlite3Db);
       }
     }
     
     return;
   }
   
	 //2.删除数据区数据
	 for(i=0; i<8; i++)
	 {
	   //pSqlStr = sqlite3_mprintf("delete from %s;",dataTable[i]);
	   sprintf(pSqlStr, "delete from %s;", dataTable[i]);
	   result = sqlite3_exec(sqlite3Dbx, pSqlStr, 0, 0, NULL);
     if (result==SQLITE_OK)
     {
   	   if (debugInfo&PRINT_DATA_BASE)
   	   {
   	      printf("删除表%s数据成功\n",dataTable[i]);
   	   }
     }
     else
     {
   	   if (debugInfo&PRINT_DATA_BASE)
   	   {
   	      printf("删除表%s数据失败\n",dataTable[i]);
   	   }
     }
     //sqlite3_free(pSqlStr);
   }
   
   if (system("rm /data/*")==0)
   {
   	 printf("删除备份数据文件成功\n");
   }
   else
   {
   	 printf("删除备份数据文件失败\n");
   }
   
   //3.删除三相表和载波单相表实时数据 drop table
   //3.1查找数据库中的mpxxxxxx,spxxxxxx,hfxxxx
   sprintf(pSqlStr,"select * from sqlite_master;");
	 sqlite3_prepare(sqlite3Db, pSqlStr, -1, &stat, 0);
	 result = sqlite3_step(stat);
	 prevNode = pClrHead;
   while(result==SQLITE_ROW)
   {
		 strcpy(tableName, sqlite3_column_text(stat, 2));
		 
		 //printf("表名=%s\n", tableName);
		 
		 if ((tableName[0]=='m' && tableName[1]=='p')
		 	   || (tableName[0]=='s' && tableName[1]=='p')
		 	    || (tableName[0]=='h' && tableName[1]=='f')
		 	  )
		 {
       tmpNode = (struct clearQueue *)malloc(sizeof(struct clearQueue));

       strcpy(tmpNode->tableName, tableName);
 
       tmpNode->next = NULL;
      
       if (pClrHead==NULL)
       {
      	 pClrHead = tmpNode;
       }
       else
       {
      	 prevNode->next = tmpNode;
       }
       prevNode = tmpNode;
     }
     
     result = sqlite3_step(stat);
   }
   sqlite3_finalize(stat);
   
   tmpNode = pClrHead;
   while(tmpNode!=NULL)
   {
	   sprintf(pSqlStr, "drop table %s;", tmpNode->tableName);
	   if (debugInfo&PRINT_DATA_BASE)
	   {
	     printf(pSqlStr);
	     printf("\n");
	   }
	    
	   result = sqlite3_exec(sqlite3Dbx, pSqlStr, NULL, NULL, &errMessage);
     if (result==SQLITE_OK)
     {
   	   if (debugInfo&PRINT_DATA_BASE)
   	   {
   	     printf("删除表%s成功\n",tmpNode->tableName);
   	   }
     }
     else
     {
   	   if (debugInfo&PRINT_DATA_BASE)
   	   {
   	      printf("删除表%s失败,Error:%s\n",tmpNode->tableName,sqlite3_errmsg(sqlite3Dbx));
   	   }
     }
     prevNode = tmpNode;
   	 tmpNode = tmpNode->next;
   	 free(prevNode);
   }

   /*
   tmpTime = sysTime;
   for(i=0;i<32;i++)
   {
	    //单相表
	    strcpy(tableName, bringTableName(tmpTime,0));
	    //pSqlStr = sqlite3_mprintf("drop table %s;",tableName);
	    sprintf(pSqlStr, "drop table %s;", tableName);
	    if (debugInfo&PRINT_DATA_BASE)
	    {
	      printf(pSqlStr);
	      printf("\n");
	    }
	    
	    result = sqlite3_exec(sqlite3Dbx, pSqlStr, NULL, NULL, &errMessage);
      if (result==SQLITE_OK)
      {
   	    if (debugInfo&PRINT_DATA_BASE)
   	    {
   	      printf("删除表%s成功\n",tableName);
   	    }
      }
      else
      {
   	    if (debugInfo&PRINT_DATA_BASE)
   	    {
   	      printf("删除表%s失败,Error:%s\n",tableName,sqlite3_errmsg(sqlite3Dbx));
   	    }
      }
      //sqlite3_free(pSqlStr);
      
      //三相表
	    strcpy(tableName, bringTableName(tmpTime,1));
	    //pSqlStr = sqlite3_mprintf("drop table %s;",tableName);
	    sprintf(pSqlStr, "drop table %s;", tableName);
	    
	    if (debugInfo&PRINT_DATA_BASE)
	    {
	      printf(pSqlStr);
	      printf("\n");
	    }
	    
	    result = sqlite3_exec(sqlite3Dbx, pSqlStr, NULL, NULL, &errMessage);
      if (result==SQLITE_OK)
      {
   	    if (debugInfo&PRINT_DATA_BASE)
   	    {
   	      printf("删除表%s成功\n",tableName);
   	    }
      }
      else
      {
   	    if (debugInfo&PRINT_DATA_BASE)
   	    {
   	      printf("删除表%s失败,Error:%s\n",tableName,sqlite3_errmsg(sqlite3Dbx));
   	    }
      }
      //sqlite3_free(pSqlStr);
      
      //整点冻结表
	    strcpy(tableName, bringTableName(tmpTime,2));
	    //pSqlStr = sqlite3_mprintf("drop table %s;",tableName);
	    
	    sprintf(pSqlStr, "drop table %s;", tableName);
	    
	    if (debugInfo&PRINT_DATA_BASE)
	    {
	      printf(pSqlStr);
	      printf("\n");
	    }
	    
	    result = sqlite3_exec(sqlite3Dbx, pSqlStr, NULL, NULL, &errMessage);
      if (result==SQLITE_OK)
      {
   	    if (debugInfo&PRINT_DATA_BASE)
   	    {
   	      printf("删除表%s成功\n",tableName);
   	    }
      }
      else
      {
   	    if (debugInfo&PRINT_DATA_BASE)
   	    {
   	      printf("删除表%s失败,Error:%s\n",tableName,sqlite3_errmsg(sqlite3Dbx));
   	    }
      }
      //sqlite3_free(pSqlStr);
      
   	  tmpTime = backTime(tmpTime, 0, 1, 0, 0, 0);
   }
   */
   
   //4.还原sqlite指针为sqlite3Db
   if (debugInfo&PRINT_DATA_BASE)
   {
   	 printf("Database-deleteData:还原sqlite3全局指针\n");
   }
   sqlite3_close(sqlite3Dbx);
   if (sqlite3_open("powerData.db", &sqlite3Db))
   {
     printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
     sqlite3_close(sqlite3Db);
     
     usleep(2000);
     if (sqlite3_open("powerData.db", &sqlite3Db))
     {
       printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
       sqlite3_close(sqlite3Db);
       
       usleep(2000);
       if (sqlite3_open("powerData.db", &sqlite3Db))
       {
         printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
         sqlite3_close(sqlite3Db);
       }
     }
     
     return;
   }

   //5.删除脉冲量中间值,这两个是数据但存储在参数中的
   deleteParameter(88, 3);
   deleteParameter(88, 13);
   if (debugInfo&PRINT_PULSE_DEBUG)
   {
  	 printf("deleteData:删除脉冲量中间值\n");
   }
   
   //6.删除参数数据
   if (type==1)
   {
   	 //专变终端删除剩余电量表
   	#ifdef LOAD_CTRL_MODULE
	     //pSqlStr = sqlite3_mprintf("delete from leftPower;");
	     
	     strcpy(pSqlStr, "delete from leftPower;");
	     result = sqlite3_exec(sqlite3Db, pSqlStr, 0, 0, NULL);
       if (result==SQLITE_OK)
       {
   	     if (debugInfo&PRINT_DATA_BASE)
   	     {
   	        printf("删除表leftPower数据成功\n");
   	     }
       }
       else
       {
   	     if (debugInfo&PRINT_DATA_BASE)
   	     {
   	        printf("删除表leftPower数据失败\n");
   	     }
       }
       //sqlite3_free(pSqlStr);
    #endif
		
		#ifdef LIGHTING
		 //读出红外学习数据
     memset(irData160, 0x0, 5);
		 memset(irData161, 0x0, 5);
		 memset(irData162, 0x0, 5);
		 memset(irData163, 0x0, 5);
		 if (selectParameter(5, 160, irData160, 2)==TRUE)
		 {
		 	 printf("读出的红外制冷开数据长度=%d\n", irData160[0] | irData160[1]<<8);
		 	 selectParameter(5, 160, irData160, (irData160[0] | irData160[1]<<8)+2);
		 }
		 else
		 {
		 	 printf("读出红外数据160失败\n");
		 }
		 
		 if (selectParameter(5, 161, irData161, 2)==TRUE)
		 {
		 	 printf("读出的红外制热开数据长度=%d\n", irData161[0] | irData161[1]<<8);
		 	 selectParameter(5, 161, irData161, (irData161[0] | irData161[1]<<8)+2);
		 }
		 else
		 {
		 	 printf("读出红外数据161失败\n");
		 }
		 
		 if (selectParameter(5, 162, irData162, 2)==TRUE)
		 {
		 	 printf("读出的红外除湿开数据长度=%d\n", irData162[0] | irData162[1]<<8);
		 	 selectParameter(5, 162, irData162, (irData162[0] | irData162[1]<<8)+2);
		 }
		 else
		 {
		 	 printf("读出红外数据162失败\n");
		 }
		 
		 if (selectParameter(5, 163, irData163, 2)==TRUE)
		 {
		 	 printf("读出的红外关数据长度=%d\n", irData163[0] | irData163[1]<<8);
		 	 selectParameter(5, 163, irData163, (irData163[0] | irData163[1]<<8)+2);
		 }
		 else
		 {
		 	 printf("读出红外数据163失败\n");
		 }
		#endif
   	 
   	 for(i=0;i<3;i++)
   	 {
   	   //pSqlStr = sqlite3_mprintf("delete from %s;",paraTable[i]);
   	   sprintf(pSqlStr, "delete from %s;",paraTable[i]);
   	   result  = sqlite3_exec(sqlite3Db, pSqlStr, 0, 0, NULL);
       if (result==SQLITE_OK)
       {
      	  if (debugInfo&PRINT_DATA_BASE)
      	  {
      	    printf("删除参数表%s数据成功\n",paraTable[i]);
      	  }
       }
       else
       {
      	  if (debugInfo&PRINT_DATA_BASE)
      	  {
      	  	printf("删除参数表%s数据失败\n",paraTable[i]);
      	  }
       }
       //sqlite3_free(pSqlStr);
     }
   }
   
   //8.保存重要参数
   if (type==1)
   {
     if (debugInfo&PRINT_DATA_BASE)
     {
       printf("Database-deleteData::保存重要参数\n");
     }

     //保存重要参数
     saveParameter(0x04,   3, (INT8U *)&ipAndPort,sizeof(IP_AND_PORT));             //IP和端口
     saveParameter(0x04,   7, (INT8U *)&teIpAndPort, sizeof(TE_IP_AND_PORT));       //终端IP地址和端口等
     saveParameter(0x04,  16, (INT8U *)&vpn, sizeof(VPN));                          //VPN
     saveParameter(0x04,  37, (INT8U *)&cascadeCommPara, sizeof(CASCADE_COMM_PARA));//级联参数
     saveParameter(0x04, 121, (INT8U *)&addrField, sizeof(ADDR_FIELD));             //地址和行政区划码
     saveParameter(0x04, 129, (INT8U *)&acSamplePara,sizeof(AC_SAMPLE_PARA));       //交采校表参数
     saveParameter(0x04, 133, mainNodeAddr, 6);
     saveParameter(0x04, 134, (INT8U *)&deviceNumber, 6);
     saveParameter(0x04, 136, (INT8U *)&csNameId, 12);

    #ifdef LIGHTING
     callAndReport = 0x01;    //照明集中器默认允许终端主动上报,2015-09-08
		 
		 //读出红外学习数据
		 if ((irData160[0] | irData160[1]<<8)>0)
		 {
		 	 saveParameter(5, 160, irData160, (irData160[0] | irData160[1]<<8)+2);
			 printf("保存红外160数据:");
			 for(i=0; i<(irData160[0] | irData160[1]<<8)+2; i++)
			 {
			 	 printf("%02X ", irData160[i]);
			 }
			 printf("\n");
		 }
		 if ((irData161[0] | irData161[1]<<8)>0)
		 {
		 	 saveParameter(5, 161, irData161, (irData161[0] | irData161[1]<<8)+2);			 
			 printf("保存红外161数据:");
			 for(i=0; i<(irData161[0] | irData161[1]<<8)+2; i++)
			 {
			 	 printf("%02X ", irData161[i]);
			 }
			 printf("\n");
		 }
		 if ((irData162[0] | irData162[1]<<8)>0)
		 {
		 	 saveParameter(5, 162, irData162, (irData162[0] | irData162[1]<<8)+2);			 
			 
			 printf("保存红外162数据:");
			 for(i=0; i<(irData162[0] | irData162[1]<<8)+2; i++)
			 {
			 	 printf("%02X ", irData162[i]);
			 }
			 printf("\n");
		 }
		 if ((irData163[0] | irData163[1]<<8)>0)
		 {
		 	 saveParameter(5, 163, irData163, (irData163[0] | irData163[1]<<8)+2);
			 printf("保存红外163数据:");
			 for(i=0; i<(irData163[0] | irData163[1]<<8)+2; i++)
			 {
			 	 printf("%02X ", irData163[i]);
			 }
			 printf("\n");
		 }
		 
    #else
     callAndReport = 0x00;
    #endif
     saveParameter(0x05, 29, &callAndReport, 1);									                 //控制F29
     
     saveParameter(88, 7,&lcdDegree,1);                                            //LCD对比度值
   }

   eventReadedPointer[0]=0x0;                                                //终端重要事件已读指针
   eventReadedPointer[1]=0x0;                                                //终端重要事件已读指针   	  
   saveParameter(88, 2, eventReadedPointer, 2);                              //终端重要事件已读指针
   iEventStartPtr = 0;
   iEventCounter  = 0;   
   
   
   //9.专变终端更新参考剩余电量
   #ifdef LOAD_CTRL_MODULE
	  if (type==0)
	  {
     	for(i=1; i<=32; i++)
      {
     	  tmpTime = sysTime;
        if (readMeterData(leftPower, i, LEFT_POWER, 0x0, &tmpTime, 0)==TRUE)
     	  {
     	  	//参考剩余电量B=A
     	  	leftPower[4] = leftPower[0];
     	  	leftPower[5] = leftPower[1];
     	  	leftPower[6] = leftPower[2];
     	  	leftPower[7] = leftPower[3];

     	  	//计算用参考总加电能量置0
     	  	leftPower[8] = 0;
     	  	leftPower[9] = 0;
     	  	leftPower[10] = 0;
     	  	leftPower[11] = 0;
     	  	
     	  	saveMeterData(i, 0, sysTime, leftPower, LEFT_POWER, 0x0, 12);
   	      
   	      if (debugInfo&PRINT_DATA_BASE)
   	      {
   	        printf("更新总加组%d参考剩余电量及参考总加电能量\n", i);
   	      }
     	  }
     	}
	  }
	 #endif
	 
	 //2012-08-08,改到最后做回收工作
	 //7.回收存储空间
   //2012-10-22,取消执行回收存储空间工作,因为查资料表明我们的系统不适合执行该命令
   // SQLite FQA里面说，在Linux的环境下，大约0.5秒/M。并且要使用两倍于数据库文件的空间。
   // 我们的系统中数据库文件可能有100M左右,但整个磁盘空间为256M,OS和文件占去15到20M,因此做不了这个工作
   // 测试中...
   //strcpy(pSqlStr, "vacuum;");
   //if (sqlite3_exec(sqlite3Db, pSqlStr, 0, 0, NULL)==SQLITE_OK)
   //{
   //  if (debugInfo&PRINT_DATA_BASE)
   //  {
   //     printf("回收存储空间成功\n");
   //  }
   //}
   //else
   //{
   //   if (debugInfo&PRINT_DATA_BASE)
   //   {
   //     printf("回收存储空间失败\n");
   //   }
   //}
}

/*******************************************************
函数名称:clearOutData
功能描述:清除过期数据
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void clearOutData(char * tableName, INT8U queryType, INT8U days, INT8U hasNo)
{
	 sqlite3_stmt *stat;
	 INT16U       i, result;
   char         *pSqlStr;        //SQL语句字符串指针
   char         *errMessage;     //错误消息(Error msg written her)
	 sqlite3_stmt *stmt;
	 const char   *tail;
	 int          execResult;
   DATE_TIME    tmpTime;
   struct timeval tv;           //Linux timeval

   if (hasNo==1)
   {
     if (queryType==MONTH_BALANCE)
     {
       tmpTime = backTime(sysTime, days, 0, 0, 0, 0);
     }
     else
     {
       tmpTime = backTime(sysTime, 0, days, 0, 0, 0);
     }
     
     getLinuxFormatDateTime(&tmpTime,&tv,1);
     
     pSqlStr = sqlite3_mprintf("delete from %s where time<%d;", tableName, tv.tv_sec);
     if (sqlite3_exec(sqlite3Db, pSqlStr, 0, 0, NULL)==SQLITE_OK)
     {
	      if (debugInfo&PRINT_DATA_BASE)
	      {
	        printf("clearOutData:删除表%s过期数据成功\n", tableName);
	      }
     }
   }
   else
   {
     if (queryType==2)
     {
       tmpTime = timeHexToBcd(backTime(sysTime, 0, days, 0, 0, 0));
			#ifdef LIGHTING
       pSqlStr = sqlite3_mprintf("delete from %s where (day<>88 and day<>89 and day<>99 and (year<%d or (year=%d and month<%d) or (year=%d and month=%d and day<%d)))"\
	                   , tableName, tmpTime.year, tmpTime.year, tmpTime.month, tmpTime.year, tmpTime.month, tmpTime.day);
			#else
       pSqlStr = sqlite3_mprintf("delete from %s where (day<>88 and day<>99 and (year<%d or (year=%d and month<%d) or (year=%d and month=%d and day<%d)))"\
	                   , tableName, tmpTime.year, tmpTime.year, tmpTime.month, tmpTime.year, tmpTime.month, tmpTime.day);
			#endif
     }
     else
     {
       if (queryType==1)
       {
         tmpTime = backTime(sysTime, days, 0, 0, 0, 0);
       }
       else
       {
         tmpTime = backTime(sysTime, 0, days, 0, 0, 0);
       }
       getLinuxFormatDateTime(&tmpTime,&tv,1);

       pSqlStr = sqlite3_mprintf("delete from %s where copyTime<%d;",tableName, tv.tv_sec);
     }
     
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
   	   if (debugInfo&PRINT_DATA_BASE)
   	   {
   	     printf("clearOutData:删除表%s过期数据成功\n",tableName);
   	   }
     }
     else
     {
   	   if (debugInfo&PRINT_DATA_BASE)
   	   {
   	     printf("clearOutData:删除表%s过期数据失败\n",tableName);
   	   }
     }
   }
   
   sqlite3_free(pSqlStr);
}

/*******************************************************
函数名称:saveBakDayFile
功能描述:保存日冻结数据到文件中
调用函数:
被调用函数:
输入参数:type =0x01,单相表数据
              =0x0b,三相表示值
              =0x0c,三相表需量
输出参数:
返回值:void
*******************************************************/
BOOL deleteBakDayFile(INT16U pn, DATE_TIME freezeTime, INT8U type)
{
	FILE           *fp;
	char           fileName[20];
  struct timeval tv;                  //Linux timeval
  DATE_TIME      tmpTime;
  SP_F_DAY       tmpSpfDay;
  MP_F_DAY       tmpMpfDay;
  INT16U         tmpCount;
	
	//冻结数据文件名
	if (type==1)
	{
	  sprintf(fileName, "/data/spd%02d%02d%02d", freezeTime.year, freezeTime.month, freezeTime.day);
	}
	else
	{
	  sprintf(fileName, "/data/mpd%02d%02d%02d", freezeTime.year, freezeTime.month, freezeTime.day);
	}

  //查看目录/文件是否存在
  if(access(fileName, F_OK) != 0)
  {
    return FALSE;    //不存在,无需做删除动作
  }

  if((fp=fopen(fileName, "rb+"))==NULL)
  {
    if (debugInfo&PRINT_DATA_BASE)
    {
  	  printf("deleteBakDayFile:打开文件%s失败\n", fileName);
  	}
  	
  	return FALSE;
  }

  tmpCount = 0;
  while (!feof(fp))
  {
  	if (type==1)
  	{
    	if (fread(&tmpSpfDay, sizeof(SP_F_DAY), 1, fp)!=1)
    	{
    	  fseek(fp, 0, 2);    //到文件尾
    	  
    	  return FALSE;
    	}
      
      if (tmpSpfDay.pn==pn)
      {
      	if (debugInfo&PRINT_DATA_BASE)
      	{
      	  printf("delteBakDayFile:测试点%d数据存在\n", pn);
      	}
      	
      	//定位到本记录
      	rewind(fp);
      	fseek(fp, tmpCount*sizeof(SP_F_DAY), 0);
      	break;
      }
    }
    else
    {
    	if (fread(&tmpMpfDay, sizeof(MP_F_DAY), 1, fp)!=1)
    	{
    	  fseek(fp, 0 ,2);    //到文件尾
    	  
    	  return FALSE;
    	}
      
      if (tmpMpfDay.pn==pn)
      {
      	if (debugInfo&PRINT_DATA_BASE)
      	{
      	  printf("delteBakDayFile:测试点%d数据存在\n", pn);
      	}
      	
      	//定位到本记录
      	rewind(fp);
      	fseek(fp, tmpCount*sizeof(MP_F_DAY), 0);
      	break;
      }
    }

  	tmpCount++;
  }
  
  if (type==1)
  {
    tmpSpfDay.pn = 0xeeee;
    if (fwrite(&tmpSpfDay, sizeof(SP_F_DAY), 1, fp)!=1)
    {
    	if (debugInfo&PRINT_DATA_BASE)
    	{
    	  printf("delteBakDayFile:文件%s删除数据失败\n", fileName);
    	}
    	
    	fclose(fp);
    	return FALSE;
    }
  }
  else
  {
    tmpMpfDay.pn = 0xeeee;
    if (fwrite(&tmpMpfDay, sizeof(MP_F_DAY), 1, fp)!=1)
    {
    	if (debugInfo&PRINT_DATA_BASE)
    	{
    	  printf("delteBakDayFile:文件%s删除数据失败\n", fileName);
    	}
    	
    	fclose(fp);
    	return FALSE;
    }
  }
  
  if (debugInfo&PRINT_DATA_BASE)
  {
    printf("delteBakDayFile:文件%s删除数据完成\n", fileName);
  }
  
  fclose(fp);
  return TRUE;
}

/*******************************************************
函数名称:deleteFreezeData
功能描述:删除测量点冻结数据
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void
*******************************************************/
void deleteFreezeData(INT16U pn)
{
	sqlite3_stmt *stat;
	INT8S        *sql;
  DATE_TIME    deleteTime;
  INT8U        i;
  
  sql = sqlite3_mprintf("delete from singlePhaseDay where pn=%d;", pn);
  if (sqlite3_exec(sqlite3Db, sql, 0, 0, NULL)==SQLITE_OK)
  {
    if (debugInfo&PRINT_DATA_BASE)
    {
      printf("删除singlePhaseDay表测量点%d数据成功\n", pn);
    }
  }
  sqlite3_free(sql);

  sql = sqlite3_mprintf("delete from singlePhaseMonth where pn=%d;", pn);
  if (sqlite3_exec(sqlite3Db, sql, 0, 0, NULL)==SQLITE_OK)
  {
    if (debugInfo&PRINT_DATA_BASE)
    {
      printf("删除singlePhaseMonth表测量点%d数据成功\n", pn);
    }
  }
  sqlite3_free(sql);

  sql = sqlite3_mprintf("delete from dayBalanceData where pn=%d;", pn);
  if (sqlite3_exec(sqlite3Db, sql, 0, 0, NULL)==SQLITE_OK)
  {
    if (debugInfo&PRINT_DATA_BASE)
    {
      printf("删除dayBalanceData表测量点%d数据成功\n", pn);
    }
  }
  sqlite3_free(sql);

  sql = sqlite3_mprintf("delete from monthBalanceData where pn=%d;", pn);
  if (sqlite3_exec(sqlite3Db, sql, 0, 0, NULL)==SQLITE_OK)
  {
    if (debugInfo&PRINT_DATA_BASE)
    {
      printf("删除monthBalanceData表测量点%d数据成功\n", pn);
    }
  }
  sqlite3_free(sql);
  
  deleteTime = sysTime;
  for(i=0; i<15; i++)
  {
    deleteTime = backTime(sysTime, 0, 1, 0, 0, 0);
    
    deleteBakDayFile(pn, sysTime, 0x1);
    deleteBakDayFile(pn, sysTime, 0xb);
    deleteBakDayFile(pn, sysTime, 0xc);
  }
}

#ifdef LIGHTING 

/***************************************************
函数名称:initCtrlTimesLink
功能描述:初始化控制时段链表
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void

历史：
    1.
***************************************************/
void initCtrlTimesLink(void)
{
	 struct ctrlTimes *tmpNode, *prevNode;
	 sqlite3_stmt     *stmt;
	 char             *sql;
	 INT16U           result;
	 INT8U            tmpAfn1, tmpAfn2, tmpAfn5, tmpAfn7;
	 INT8U            i;
	 
	 //释放链表
	 while(cTimesHead!=NULL)
	 {
	 	 tmpNode = cTimesHead->next;
	 	 
	 	 free(cTimesHead);
	 	 
	 	 cTimesHead = tmpNode;
	 }
	 
	 cTimesHead  = NULL;
	 prevNode = cTimesHead;
	 
	 //查询
	 sql = "select * from base_vice_info where (acc_afn=? or acc_afn=? or acc_afn=? or acc_afn=?) order by acc_fn, acc_pn, acc_afn";
	 sqlite3_prepare(sqlite3Db, sql, -1, &stmt, 0);
	 tmpAfn1 = 1;
	 sqlite3_bind_blob(stmt, 1, &tmpAfn1, 1, NULL);
	 tmpAfn2 = 2;
	 sqlite3_bind_blob(stmt, 2, &tmpAfn2, 1, NULL);
	 tmpAfn5 = 5;
	 sqlite3_bind_blob(stmt, 3, &tmpAfn5, 1, NULL);
	 tmpAfn7 = 7;
	 sqlite3_bind_blob(stmt, 4, &tmpAfn7, 1, NULL);

	 result = sqlite3_step(stmt);
   while(result==SQLITE_ROW)
   {
     tmpNode = (struct ctrlTimes *)malloc(sizeof(struct ctrlTimes));

		 memcpy(tmpNode, sqlite3_column_blob(stmt, 3), sizeof(struct ctrlTimes));

     tmpNode->next = NULL;
         
     if (cTimesHead==NULL)
     {
       cTimesHead = tmpNode;
     }
     else
     {
       prevNode->next = tmpNode;
     }
     prevNode = tmpNode;
            
     result = sqlite3_step(stmt);
   }
   sqlite3_finalize(stmt);
   
   if (debugInfo&PRINT_DATA_BASE)
   {
     prevNode = cTimesHead;
     while(prevNode!=NULL)
     {
     	 switch (prevNode->deviceType)
     	 {
     	 	 case 2:
     	 	   printf("线路控制器,");
     	 	   break;

     	 	 case 5:
     	 	   printf("经纬度控制,");
     	 	   break;

     	 	 case 7:
     	 	   printf("  照度控制,");
     	 	   break;
     	 	   
     	 	 default:
     	 	   printf("单灯控制器,");
     	 	   break;
     	 }
     	 printf("%02d-%02d至%02d-%02d,", prevNode->startMonth, prevNode->startDay, prevNode->endMonth, prevNode->endDay);
     	 printf("时段%d,启用日=%02x,", prevNode->noOfTime, prevNode->workDay);
     	 for(i=0; i<6; i++)
     	 {
     	   printf("%d)%02x:%02x-%d%% ", i+1,prevNode->hour[i], prevNode->min[i], prevNode->bright[i]);
     	 }
     	 printf("\n");
     	 
     	 prevNode = prevNode->next;
     }
   }
}

/*******************************************************
函数名称:deleteCtimes
功能描述:用于删除控制时段数据(基本信息副表)
调用函数:
被调用函数:
输入参数:afn  fn
输出参数:
返回值：void
*******************************************************/
void deleteCtimes(void)
{
	sqlite3_stmt *stat;
	INT8S        *sql;
	INT8U        tmpAfn1, tmpAfn2, tmpAfn5, tmpAfn7;
	
	//删除相应数据
	sql = "delete from base_vice_info where (acc_afn = ? or acc_afn = ? or acc_afn = ? or acc_afn = ?);";	
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	tmpAfn1 = 1;
	sqlite3_bind_blob(stat, 1, &tmpAfn1, 1, NULL);
	tmpAfn2 = 2;
	sqlite3_bind_blob(stat, 2, &tmpAfn2, 1, NULL);
	tmpAfn5 = 5;    //2016-10-28,Add
	sqlite3_bind_blob(stat, 3, &tmpAfn5, 1, NULL);
	tmpAfn7 = 7;
	sqlite3_bind_blob(stat, 4, &tmpAfn7, 1, NULL);
	sqlite3_step(stat);
	sqlite3_finalize(stat);
}

#endif

/*******************************************************
函数名称:loadParameter
功能描述:开机启动时初始化数据
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：void

历史:
    1)2012-08-09,add,关键登录参数(终端地址、主站IP地址和端口、VPN用户名密码)备份在独立文件,这些独立文件
                 只在设置参数时修改,其余时间不动,一旦数据库中的参数被误修改时,使用备份独立文件中的参数
    2)2012-09-26,add,校表参数也作关键参数备份处理
*******************************************************/
void loadParameter(void)
{
	 sqlite3_stmt   *stat;
	 INT16U         i, result, j;
   char           *pSqlStr;              //SQL语句字符串指针
   char           *errMessage;           //错误消息(Error msg written her)
   INT32U         tmpAddr;
   ADC_PARA       adcPara;               //直流模拟量配置参数(AFN04-FN81,FN82,FN83)
   
   ADDR_FIELD     bakAddrField;          //bak地址域
   VPN            bakVpn;                //bak虚拟专网用户名、密码(AFN04-FN16)
   IP_AND_PORT    bakIpAndPort;          //bak主站IP地址和端口(AFN04-FN03)
   AC_SAMPLE_PARA bakAcPara;             //bak交采校表值
   INT8U          *pAcPara, *pAcParax;   //比较交采校表值指针
   INT8U          foundAcPara=0;
   

   //0.参数状态变量
   memset(paraStatus, 0x0, 31);

	 //1.读出所有参数,关键参数没有的话赋初值****************************************************
	 //F121行政区划码和终端地址
	 if (selectParameter(0x04, 121, (INT8U *)&addrField, sizeof(ADDR_FIELD))==FALSE)	   			  //F1
	 {
	   //如果备份终端地址文件存在
	   if(access("/keyPara121", F_OK) == 0)
	   {
	   	 readBakKeyPara(121, &addrField);
	   	 
	   	 printf("数据库中无终端地址,备份终端地址存在,使用备份终端地址\n");
	   }
	   else
	   {
	     //设置默认值
	     addrField.a1[0] = 0x00;
	     addrField.a1[1] = 0x50;
	     addrField.a2[0] = 0x01;  //0001
	     addrField.a2[1] = 0x00;
	     
	     saveBakKeyPara(121);
	   }
	   
	   //保存F121
	   saveParameter(0x04, 121, (INT8U *)&addrField, sizeof(ADDR_FIELD));
	   
	   printf("设置行政区划码和终端地址初始值\n");
	 }
	 else
	 {
	   //如果备份终端地址文件不存在
	   if(access("/keyPara121", F_OK) != 0)
	   {
	     saveBakKeyPara(121);
     }
   }
   
   readBakKeyPara(121, &bakAddrField);
   printf("备份终端地址:行政区划=%02x%02x,终端地址=%d\n", bakAddrField.a1[1],bakAddrField.a1[0], bakAddrField.a2[1]<<8 | bakAddrField.a2[0]);
   
   if (addrField.a1[0]!=bakAddrField.a1[0] || addrField.a1[1]!=bakAddrField.a1[1]
   	   || addrField.a2[0]!=bakAddrField.a2[0] || addrField.a2[1]!=bakAddrField.a2[1]
   	  )
   {
	   readBakKeyPara(121, &addrField);
	   	 
	   printf("终端地址与备份终端地址不相同,使用备份终端地址\n");
   }

   //F1终端通信参数
	 if (selectParameter(0x04, 1, (INT8U *)&commPara, sizeof(COMM_PARA))==FALSE)    //F1
	 {
    #ifdef SDDL_CSM    //2013-11-20,根据山东电力公司营销部通知的默认值更改
     commPara.rts = 0;
     commPara.delay = 0;
   
     commPara.timeOutReSendTimes[0] = 0x00;
     commPara.timeOutReSendTimes[1] = 0x00;
   
     commPara.flagOfCon = 0x07;
     commPara.heartBeat = 10;
    #else
     commPara.rts = 22;
     commPara.delay = 20;
   
     commPara.timeOutReSendTimes[0] = 0x0d;
     commPara.timeOutReSendTimes[1] = 0x30;
   
     commPara.flagOfCon = 0;
     
     #ifdef LIGHTING
      commPara.heartBeat = 30;    //2016-07-04,照明集中器默认心跳值改成30秒
     #else
      #ifdef CQDL_CSM
       commPara.heartBeat = 10;
      #else
       #ifdef DKY_SUBMISSION
        commPara.heartBeat = 1;  //ly,2011-08-25,将默认心跳改成1分钟
       #else
        commPara.heartBeat = 5;
       #endif
      #endif
     #endif
	  #endif

	   saveParameter(0x04, 1, (INT8U *)&commPara, sizeof(COMM_PARA));
	   
	   printf("设置终端通信参数初始值\n");
	 }
	 else    //2012-08-02,add "else process"
	 {
	 	 if (commPara.heartBeat<1)
	 	 {
      #ifdef LIGHTING
       commPara.heartBeat = 30;    //2016-07-04,照明集中器默认心跳值改成30秒
      #else
     	 commPara.heartBeat = 1;
     	#endif
	 	 }
	 }
	 
	 //F2
	 if (selectParameter(0x04, 2, (INT8U *)&relayConfig, sizeof(RELAY_CONFIG))==TRUE)
	 {
  	 ;
	 }
	 
	 //F3.主站IP和端口
	 if (selectParameter(0x04, 3, (INT8U *)&ipAndPort, sizeof(IP_AND_PORT))==FALSE)
	 {
		 //如果备份主站IP地址文件存在
	   if(access("/keyPara003", F_OK) == 0)
	   {
	   	 readBakKeyPara(3, &ipAndPort);
	   	 
	   	 printf("数据库中无主站IP地址,备份主站IP地址存在,使用备份主站IP地址\n");
	   }
	   else
  	 {
  		 //默认值
  		 ipAndPort.ipAddr[0] = 222;
  		 ipAndPort.ipAddr[1] = 178;
  		 ipAndPort.ipAddr[2] =  86;	
  		 ipAndPort.ipAddr[3] =  65;
  		 ipAndPort.port[0] = 0x29;
  		 ipAndPort.port[1] = 0x23;
  		 //备用IP和端口
  		 ipAndPort.ipAddrBak[0] = 222;
  		 ipAndPort.ipAddrBak[1] = 178;
  		 ipAndPort.ipAddrBak[2] = 86;
  		 ipAndPort.ipAddrBak[3] = 65;
  		 ipAndPort.portBak[0] = 0x29;
  		 ipAndPort.portBak[1] = 0x23;
  		
  		 strcpy((char *)ipAndPort.apn,"CMNET");
  		 
  		 saveBakKeyPara(3);
  	 }
		
		 //插入F3
		 saveParameter(0x04, 3, (INT8U *)&ipAndPort, sizeof(IP_AND_PORT));
		 
		 printf("设置主站IP地址和端口初始值\n");
	 }
	 else
	 {
	   if(access("/keyPara003", F_OK) != 0)  //不存在
	   {
	   	 saveBakKeyPara(3);
	   }
	 }

   readBakKeyPara(3, &bakIpAndPort);
   printf("备份主IP=%03d.%03d.%03d.%03d,主端口=%d,备IP=%03d.%03d.%03d.%03d,备端口=%d,APN=%s\n", 
            bakIpAndPort.ipAddr[0], bakIpAndPort.ipAddr[1], bakIpAndPort.ipAddr[2], bakIpAndPort.ipAddr[3],
            bakIpAndPort.port[1]<<8 | bakIpAndPort.port[0],
            bakIpAndPort.ipAddrBak[0], bakIpAndPort.ipAddrBak[1], bakIpAndPort.ipAddrBak[2], bakIpAndPort.ipAddrBak[3],
            bakIpAndPort.portBak[1]<<8 | bakIpAndPort.portBak[0],
            (char *)bakIpAndPort.apn
            );
   
   if (ipAndPort.ipAddr[0]!=bakIpAndPort.ipAddr[0] || ipAndPort.ipAddr[1]!=bakIpAndPort.ipAddr[1] || ipAndPort.ipAddr[2]!=bakIpAndPort.ipAddr[2] || ipAndPort.ipAddr[3]!=bakIpAndPort.ipAddr[3]
       || ipAndPort.ipAddrBak[0]!=bakIpAndPort.ipAddrBak[0] || ipAndPort.ipAddrBak[1]!=bakIpAndPort.ipAddrBak[1] || ipAndPort.ipAddrBak[2]!=bakIpAndPort.ipAddrBak[2] || ipAndPort.ipAddrBak[3]!=bakIpAndPort.ipAddrBak[3]
   	    || ipAndPort.port[0] != bakIpAndPort.port[0] || ipAndPort.port[1] != bakIpAndPort.port[1] 
   	     || ipAndPort.portBak[0] != bakIpAndPort.portBak[0] || ipAndPort.portBak[1] != bakIpAndPort.portBak[1] 
   	      || strcmp((char *)&ipAndPort.apn,(char *)&bakIpAndPort.apn)!=0
   	   )
   {
   	 readBakKeyPara(3, &ipAndPort);
	   	 
	   printf("主站IP地址与备份主站IP地址不相同,使用备份主站IP地址\n");
   }
   
	 //F4
	 if (selectParameter(0x04, 4, (INT8U *)&phoneAndSmsNumber, sizeof(PHONE_AND_SMS))==TRUE)
	 {
  	 ;
	 }
	 
	 //F5
	 if (selectParameter(0x04, 5, (INT8U *)messageAuth, 3)==TRUE)
	 {
  	 ;
	 }
	 
	 //F6
	 if (selectParameter(0x04, 6, (INT8U *)groupAddr, sizeof(groupAddr))==TRUE)
	 {
  	 ;
	 }
	
	 //F7.终端IP地址及端口
	 if (FALSE==selectParameter(0x04, 7, (INT8U *)&teIpAndPort, sizeof(TE_IP_AND_PORT)))
	 {
	   //如果备份终端IP地址及端口文件存在,2020-11-18,Add
	   if(access("/keyPara007", F_OK) == 0)
	   {
	   	 readBakKeyPara(7, &teIpAndPort.ethIfLoginMs);
	   	 
	   	 printf("数据库中无终端IP地址及端口,备份终端IP地址及端口文件存在,使用备份终端IP地址和端口\n");
	   }
	   else
	   {
	   	#ifdef PLUG_IN_CARRIER_MODULE
	     //设置默认值,2020-11-18,Add
	     teIpAndPort.ethIfLoginMs = 0x55;
			 printf("设置默认值:以太网登录主站\n");
			#endif
	   }
	 }
	 else
	 {
	   //如果备份终端IP地址及端口文件不存在,2020-11-18,Add
	   if(access("/keyPara007", F_OK) != 0)
	   {
	     saveBakKeyPara(7);
     }
   }
	 if (teIpAndPort.teIpAddr[0]==0x0)
	 {
		 //终端IP地址、子网掩码地址、网关地址读rcS文件中的值
	   readIpMaskGateway(teIpAndPort.teIpAddr, teIpAndPort.mask, teIpAndPort.gateWay);

		 //代理类型
		 teIpAndPort.proxyType = 0;   //不使用代理
		 //代理服务器地址
		 teIpAndPort.proxyServer[0] = 0;
		 teIpAndPort.proxyServer[1] = 0;
		 teIpAndPort.proxyServer[2] = 0;
		 teIpAndPort.proxyServer[3] = 0;
		 //代理服务器端口
		 teIpAndPort.proxyPort[0] = 0;
		 teIpAndPort.proxyPort[1] = 0;
		 
		 //代理服务器连接方式
		 teIpAndPort.proxyLinkType = 0;		 
     //用户名长度
     teIpAndPort.userNameLen = 0;   //用户名长度m     
     //用户名
     memset(teIpAndPort.userName,0x0,20);    
     //密码长度n
     teIpAndPort.passwordLen = 0;   //用户名长度m
     //密码
     memset(teIpAndPort.password,0x0,20);     
     //终端侦听端口6412
     teIpAndPort.listenPort[0] = 0x0c;
     teIpAndPort.listenPort[1] = 0x19;

		 //2020-11-18,修改
		 if (0x55!=teIpAndPort.ethIfLoginMs)
		 {
     	 teIpAndPort.ethIfLoginMs = 0x00;
		 }
   
		 saveParameter(0x04, 7, (INT8U *)&teIpAndPort, sizeof(TE_IP_AND_PORT));
		 
		 printf("设置终端IP地址和端口初始值\n");
	 }
	 readIpMaskGateway(teIpAndPort.teIpAddr, teIpAndPort.mask, teIpAndPort.gateWay);

	 //F8.终端上行通信工作方式
	 if (selectParameter(0x04, 8, (INT8U *)&tePrivateNetMethod, sizeof(PRIVATE_NET_METHOD))==TRUE)
	 {
  	 ;
	 }
	 else
	 {
	 	 tePrivateNetMethod.workMethod = 0x11;
	 	 tePrivateNetMethod.redialInterval[0] = 20;
	 	 tePrivateNetMethod.redialInterval[1] = 0;
	 }
	
	 //F9.终端事件记录配置设置
	 if (selectParameter(0x04, 9, (INT8U *)&eventRecordConfig, sizeof(EVENT_RECORD_CONFIG))==FALSE)
	 {
    	for(i=0;i<8;i++)
    	{
    	  eventRecordConfig.nEvent[i] = 0x0;
    	  eventRecordConfig.iEvent[i] = 0x0;
    	}

     #ifdef SDDL_CSM
      #ifdef PLUG_IN_CARRIER_MODULE    //集中器
    	 eventRecordConfig.nEvent[0] = 0xc7;
       eventRecordConfig.nEvent[1] = 0x7f;
       eventRecordConfig.nEvent[2] = 0xfd;
       eventRecordConfig.nEvent[3] = 0xff;
       eventRecordConfig.nEvent[4] = 0xff;
       eventRecordConfig.nEvent[5] = 0x01;
       
       eventRecordConfig.iEvent[0] = 0xc2;
       eventRecordConfig.iEvent[1] = 0x38;
       eventRecordConfig.iEvent[2] = 0x10;
       eventRecordConfig.iEvent[3] = 0x7c;
       eventRecordConfig.iEvent[4] = 0x90;
       eventRecordConfig.iEvent[5] = 0x01;
      #else
    	 eventRecordConfig.nEvent[0] = 0xff;
       eventRecordConfig.nEvent[1] = 0x7f;
       eventRecordConfig.nEvent[2] = 0xfd;
       eventRecordConfig.nEvent[3] = 0xff;
       eventRecordConfig.nEvent[4] = 0xff;
       eventRecordConfig.nEvent[5] = 0x01;
       
       eventRecordConfig.iEvent[0] = 0xc2;
       eventRecordConfig.iEvent[1] = 0x3f;
       eventRecordConfig.iEvent[2] = 0x10;
       eventRecordConfig.iEvent[3] = 0x7c;
       eventRecordConfig.iEvent[4] = 0x90;
       eventRecordConfig.iEvent[5] = 0x01;
      #endif      
     #else
    	eventRecordConfig.nEvent[0] = 0x00;
      eventRecordConfig.nEvent[1] = 0x10;
      eventRecordConfig.nEvent[2] = 0x80;
      eventRecordConfig.nEvent[3] = 0x01;
      eventRecordConfig.nEvent[4] = 0x00;
      //eventRecordConfig.iEvent[0] = 0xf5; //11110101
      //ly,2011-11-21,改成的以下这句,因为在山东发现,如果下任务参数时,回了事件数据的话,地玮系统就不能得到确认
      //由于专变的v1.6急着生产所以还没有改这个地方
      eventRecordConfig.iEvent[0] = 0xf1; //11110001
      eventRecordConfig.iEvent[1] = 0x3a; //00111010
      eventRecordConfig.iEvent[2] = 0x9d; //10011101
      eventRecordConfig.iEvent[3] = 0xbf; //10111111
      eventRecordConfig.iEvent[4] = 0x08; //00001000
     #endif
      
 	   saveParameter(0x04, 9, (INT8U *)&eventRecordConfig, 16);
 	    
 	   printf("设置终端事件记录配置初始值\n");
	 }
	 
	 if (selectParameter(0x04, 10, (INT8U *)&meterDeviceNum, 2)==TRUE)
	 {
  	 ;
	 } 		 	
   countParameter (0x04, 10, &meterDeviceNum);
 	 
 	 //F12.终端状态量输入参数
 	 if (selectParameter(0x04, 12, (INT8U *)statusInput, 2)==TRUE)
 	 {
  	 ;
 	 }
   
   //F11.终端脉冲配置参数
	 if (selectParameter(0x04, 11, (INT8U *)&pulseConfig, sizeof(PULSE_CONFIG))==FALSE)
	 {
  	 pulseConfig.numOfPulse = 0;
	 }
	 
	 //F13终端电压/电流模拟量配置参数
	 if (selectParameter(0x04, 13, (INT8U *)&simuIUConfig, sizeof(IU_SIMULATE_CONFIG))==TRUE)
	 {
	 	 ;
	 }
	 
	 //F14.终端总加组配置参数
	 if (selectParameter(0x04, 14, (INT8U *)&totalAddGroup, sizeof(TOTAL_ADD_GROUP))==FALSE)
	 {
	 	 totalAddGroup.numberOfzjz = 0;
	 }
	 
	 selectParameter(0x04, 15, (INT8U *)&differenceConfig, sizeof(ENERGY_DIFFERENCE_CONFIG));//F15
	 
	 //F16
	 if (selectParameter(0x04, 16, (INT8U *)&vpn, sizeof(VPN))==FALSE)
	 {
		 //如果备份VPN文件存在
	   if(access("/keyPara016", F_OK) == 0)
	   {
	   	 readBakKeyPara(16, &vpn);
	   	 
	   	 printf("数据库中无VPN参数,备份VPN参数存在,使用备份VPN参数\n");
	   }
	   else
  	 {
  	 	 strcpy((char *)&vpn.vpnName, "card");
  	 	 strcpy((char *)&vpn.vpnPassword, "card");
  	 	 
  	 	 saveBakKeyPara(16);
  	 }
  	 	 
		 //插入F16
		 saveParameter(0x04, 16, (INT8U *)&vpn, sizeof(VPN));
		   
		 printf("设置VPN初始值\n");
	 }
	 else
	 {
	   if(access("/keyPara016", F_OK) != 0)    //不存在文件
	   {
	   	 saveBakKeyPara(16);
	   }
	 }
	 
	 readBakKeyPara(16, &bakVpn);
	 printf("备份VPN:username=%s,password=%s\n", (char *)bakVpn.vpnName,(char *)bakVpn.vpnPassword);
	 
	 if (strcmp((char *)&vpn.vpnName,(char *)&bakVpn.vpnName)!=0
	 	   || strcmp((char *)&vpn.vpnPassword,(char *)&bakVpn.vpnPassword)!=0
	 	  )
	 {
	 	 readBakKeyPara(16, &vpn);
	 	 
	 	 printf("VPN参数与备份VPN参数不相同,使用备份VPN参数\n");
	 }

   printf("*****************************\n");
   printf("当前关键登录参数:\n");
   printf("    1)终端地址:行政区划=%02x%02x,终端地址=%d\n", addrField.a1[1],addrField.a1[0], addrField.a2[1]<<8 | addrField.a2[0]);
   printf("    2)主IP=%03d.%03d.%03d.%03d,主端口=%d,备IP=%03d.%03d.%03d.%03d,备端口=%d,APN=%s\n", 
            ipAndPort.ipAddr[0], ipAndPort.ipAddr[1], ipAndPort.ipAddr[2], ipAndPort.ipAddr[3],
            ipAndPort.port[1]<<8 | ipAndPort.port[0],
            ipAndPort.ipAddrBak[0], ipAndPort.ipAddrBak[1], ipAndPort.ipAddrBak[2], ipAndPort.ipAddrBak[3],
            ipAndPort.portBak[1]<<8 | ipAndPort.portBak[0],
            (char *)ipAndPort.apn
            );
	 printf("    3)VPN:用户名=%s,密码=%s\n", (char *)bakVpn.vpnName,(char *)bakVpn.vpnPassword);
	 printf("    4)使用以太网登录主站:%X\n", teIpAndPort.ethIfLoginMs);
   printf("*****************************\n");


	 //组3
	 selectParameter(0x04, 17, (INT8U *)protectLimit, 2);																	   //F17
   selectParameter(0x04, 18, (INT8U *)&ctrlPara, sizeof(CONTRL_PARA));                     //F18,F19,F20

	 selectParameter(0x04, 23, (INT8U *)chargeAlarm, 3);																	   //F23

	 //F21.终端电能量费率时段和费率数
	 if (selectParameter(0x04, 21, (INT8U *)periodTimeOfCharge, 49)==FALSE)
	 {
	 	 //F21终端电能量费率时段和费率数置初值
	 	 periodTimeOfCharge[48] = 0x4;
	 	 for(i=0;i<48;i++)
	 	 {
	 	 	 periodTimeOfCharge[i] = i/12;
	 	 }
	 }
	 
	 //F22.终端电能量费率
	 selectParameter(0x04, 22, (INT8U *)&chargeRateNum, sizeof(CHARGE_RATE_NUM));
	 
	 //F33.终端抄表运行参数设置
	 if (selectParameter(0x04, 33, (INT8U *)&teCopyRunPara, sizeof(TE_COPY_RUN_PARA))==FALSE)
	 {
      //默认值
      teCopyRunPara.numOfPara = NUM_OF_COPY_METER;
      
     #ifdef SDDL_CSM
      for(i=0;i<teCopyRunPara.numOfPara;i++)
      {
      	//终端通信端口号
      	if (i==4)
      	{
      	  teCopyRunPara.para[i].commucationPort = 31;
      	}
      	else
      	{
      	  teCopyRunPara.para[i].commucationPort = i+1;
      	}
  			
  			//台区集中抄表运行控制字(抄读电表状态字,不搜寻新增电表,要求对电表校时,不广播冻结校时,抄所有表,按时段抄表)
      	teCopyRunPara.para[i].copyRunControl[0] = 0x28;
      	teCopyRunPara.para[i].copyRunControl[1] = 0x00;	
      	
      	//抄表日-日期(1日)
      	teCopyRunPara.para[i].copyDay[0] = 0x01;
      	teCopyRunPara.para[i].copyDay[1] = 0x00;
      	teCopyRunPara.para[i].copyDay[2] = 0x00;
      	teCopyRunPara.para[i].copyDay[3] = 0x00;
      	
      	//抄表日-时间(0时0分)
      	teCopyRunPara.para[i].copyTime[0] = 0x0;
      	teCopyRunPara.para[i].copyTime[1] = 0x0;
      	
  	    //抄表间隔时间默认值
  	    if (i==4)  //载波抄表间隔为60
  	    {
  	      teCopyRunPara.para[i].copyInterval = 60;
  	    }
  	    else       //485接口为5分钟
  	    {
  	     #ifdef PLUG_IN_CARRIER_MODULE
  	      teCopyRunPara.para[i].copyInterval = 60;
  	     #else
  	      teCopyRunPara.para[i].copyInterval = 30;
  	     #endif
  	    }
      	
      	//广播校时定时时间
      	teCopyRunPara.para[i].broadcastCheckTime[0] = 0x40;
      	teCopyRunPara.para[i].broadcastCheckTime[1] = 0x16;
      	teCopyRunPara.para[i].broadcastCheckTime[2] = 0x00;    //每日校时
      	
      	teCopyRunPara.para[i].hourPeriodNum = 1;							//允许抄表时段数
      	
      	teCopyRunPara.para[i].hourPeriod[0][0] = 0x00;
      	teCopyRunPara.para[i].hourPeriod[0][1] = 0x00;
      	teCopyRunPara.para[i].hourPeriod[1][0] = 0x59;
      	teCopyRunPara.para[i].hourPeriod[1][1] = 0x23;
      }
     #else
      for(i=0;i<teCopyRunPara.numOfPara;i++)
      {
      	//终端通信端口号
      	if (i==4)  //2012-3-27,从3改成4
      	{
      	  teCopyRunPara.para[i].commucationPort = 31;
      	}
      	else
      	{
      	  teCopyRunPara.para[i].commucationPort = i+1;
      	}
  			
  			//台区集中抄表运行控制字(抄读电表状态字,搜寻新增电表,不对电表校时,不广播冻结校时,抄所有表,按时段抄表)
      	teCopyRunPara.para[i].copyRunControl[0] = 0x30;
      	teCopyRunPara.para[i].copyRunControl[1] = 0x00;	
      	
      	//抄表日-日期(1日)
      	teCopyRunPara.para[i].copyDay[0] = 0x01;
      	teCopyRunPara.para[i].copyDay[1] = 0x00;
      	teCopyRunPara.para[i].copyDay[2] = 0x00;
      	teCopyRunPara.para[i].copyDay[3] = 0x00;
      	
      	//抄表日-时间(0时0分)
      	teCopyRunPara.para[i].copyTime[0] = 0x0;
      	teCopyRunPara.para[i].copyTime[1] = 0x0;
      	
  	    //抄表间隔时间默认值
  	    if (i==4)  //载波抄表间隔为20  ,2012-3-27,从3改成4
  	    {
  	      teCopyRunPara.para[i].copyInterval = 20;
  	    }
  	    else       //485接口为5分钟
  	    {
  	     #ifdef LIGHTING    //照明集中器485口默认为20秒
  	      //teCopyRunPara.para[i].copyInterval = 1;
  	      teCopyRunPara.para[i].copyInterval = 2;   //2016-06-02,修改为单位为*10秒，2即为20秒
  	     #else
  	      teCopyRunPara.para[i].copyInterval = 5;
  	     #endif
  	    }
      	
      	//广播校时定时时间
      	teCopyRunPara.para[i].broadcastCheckTime[0] = 0x0;
      	teCopyRunPara.para[i].broadcastCheckTime[1] = 0x0;
      	teCopyRunPara.para[i].broadcastCheckTime[2] = 0x0;
      	
      	teCopyRunPara.para[i].hourPeriodNum = 1;							//允许抄表时段数
      	
				//2016-09-08,载波端口的每日起始抄表时间从00:01-23:59改到00:10-23:55
				if (4==i)
				{
					teCopyRunPara.para[i].hourPeriod[0][0] = 0x10;
				}
				else
				{
				  teCopyRunPara.para[i].hourPeriod[0][0] = 0x01;
				}
      	teCopyRunPara.para[i].hourPeriod[0][1] = 0x00;
				if (4==i)
				{
      	  teCopyRunPara.para[i].hourPeriod[1][0] = 0x55;
			  }
				else
				{
      	  teCopyRunPara.para[i].hourPeriod[1][0] = 0x59;
				}
      	teCopyRunPara.para[i].hourPeriod[1][1] = 0x23;
      }
     #endif
     
  		saveParameter(0x04, 33, (INT8U *)&teCopyRunPara, sizeof(TE_COPY_RUN_PARA));
      printf("设置终端抄表运行参数初始值\n");
	 }
	
	 selectParameter(0x04, 34, (INT8U *)&downRiverModulePara, sizeof(DOWN_RIVER_MODULE_PARA));//F34
	 selectParameter(0x04, 35, (INT8U *)&keyHouseHold, sizeof(KEY_HOUSEHOLD));							  //F35
	 
	 if (selectParameter(0x04, 36, (INT8U *)upTranslateLimit, 4)==FALSE)   								    //F36
	 {
	 	 upTranslateLimit[0] = 0x00;
	 	 upTranslateLimit[1] = 0x00;
	 	 upTranslateLimit[2] = 0x00;
	 	 upTranslateLimit[3] = 0x00;
	 }
	 
	 selectParameter(0x04, 37, (INT8U *)&cascadeCommPara, sizeof(CASCADE_COMM_PARA));			    //F37
	 selectParameter(0x04, 38, (INT8U *)&typeDataConfig1, sizeof(TYPE_1_2_DATA_CONFIG));		  //F38
	 selectParameter(0x04, 39, (INT8U *)&typeDataConfig2, sizeof(TYPE_1_2_DATA_CONFIG));		  //F39
  	
	 //组8
	 //F57终端声音告警允许设置
	 if (selectParameter(0x04, 57, (INT8U *)voiceAlarm, 3)==FALSE)														//F57
	 {
	 	 voiceAlarm[0] = 0x0;
	 	 voiceAlarm[1] = 0x0;
	 	 voiceAlarm[2] = 0x0;
	 	 
     saveParameter(0x04, 57, (INT8U *)voiceAlarm, 3);
     
     printf("设置终端声音告警允许/禁止初始值\n");
	 }
	
	 selectParameter(0x04, 58, (INT8U *)&noCommunicationTime, 1);													    //F58
	
   //F59电能表异常判别阈值
	 selectParameter(0x04, 59, (INT8U *)&meterGate, sizeof(METER_GATE));										  //F59
   if (meterGate.powerOverGate == 0x00 && meterGate.meterFlyGate == 0x00 && meterGate.meterStopGate == 0x00 && meterGate.meterCheckTimeGate == 0x00)
   {
  	#ifdef LIGHTING    //2014-06-25,增加路灯集中器的控制点阈值
  	 meterGate.powerOverGate = 0x01;
  	 meterGate.meterFlyGate  = 0x01;         //CCB发现故障重试次数默认1次
  	 meterGate.meterStopGate = 0x01;         //默认1分钟广播命令等待阈值
  	 meterGate.meterCheckTimeGate = 0x01;    //默认1分钟超差校时
  	#else
  	 #ifdef SDDL_CSM
  	  meterGate.powerOverGate = 0x20;
  	  meterGate.meterFlyGate = 0x40;
  	  meterGate.meterStopGate = 0x18;        //默认6小时记录停走
  	  meterGate.meterCheckTimeGate = 0x05;   //默认5分钟超差校时
  	 #else
  	  meterGate.powerOverGate = 0x99;
  	  meterGate.meterFlyGate = 0x99;
  	  meterGate.meterStopGate = 0xC0;        //默认48小时记录停走
  	  meterGate.meterCheckTimeGate = 0x10;   //默认16分钟超差校时
     #endif
    #endif

     saveParameter(0x04, 59, (INT8U *)&meterGate, 4);
     
     printf("设置电能表异常判别阈值初始值\n");
   }
  	     		
	 selectParameter(0x04, 60, (INT8U *)&waveLimit, sizeof(WAVE_LIMIT));										  //F60
	 selectParameter(0x04, 61, (INT8U *)&adcInFlag, 1);																		    //F61

	 if (selectParameter(0x04, 65, (INT8U *)&reportTask1, sizeof(REPORT_TASK_PARA))==FALSE)		//F65
	 {
	 	 memset((INT8U *)&reportTask1, 0x0, sizeof(REPORT_TASK_PARA));
	 }
	 if (selectParameter(0x04, 66, (INT8U *)&reportTask2, sizeof(REPORT_TASK_PARA))==FALSE)	  //F66
	 {
	 	 memset((INT8U *)&reportTask2, 0x0, sizeof(REPORT_TASK_PARA));
	 }

   //第1路直流模拟量参数
   if (selectViceParameter(0x04, 81, 1, (INT8U *)&adcPara, sizeof(ADC_PARA))==FALSE)
   {
	   adcPara.adcStartValue[0] = 0x40;   //量程起始值 - 默认值4mA
	   adcPara.adcStartValue[1] = 0xa0;
	   adcPara.adcEndValue[0]   = 0x00;   //量程终止值 - 默认值20mA
	   adcPara.adcEndValue[1]   = 0xa2;
	   adcPara.adcUpLimit[0]    = 0x50;   //上限 - 默认值25mA
	   adcPara.adcUpLimit[1]    = 0xa2;
	   adcPara.adcLowLimit[0]   = 0x10;   //下限 - 默认值1mA
	   adcPara.adcLowLimit[1]   = 0xa0;
	   adcPara.adcFreezeDensity = 0x01;   //冻结密度 - 默认为15分钟
	   saveViceParameter(0x04, 81, 1, (INT8U *)&adcPara, sizeof(ADC_PARA));
     
     printf("设置第1路直流模拟量参数初始值\n");
	 }

	 //扩展
	 selectParameter(0x04, 97, (INT8U *)teName, 20);						  													  //F97
	 
	 selectParameter(0x04, 98, (INT8U *)sysRunId, 20);					  													  //F98
	 
	 if (selectParameter(0x04, 99, (INT8U *)assignCopyTime, 6)==FALSE)  		 							    //F99
	 {
	 	  //搜索持续时间
	 	  assignCopyTime[0] = 0x14;
	 	  assignCopyTime[1] = 0x00;
	 	  
	 	  //抄表持续时间
	 	  assignCopyTime[2] = 0x14;
	 	  assignCopyTime[3] = 0x00;

	 	  //学习路由持续时间
	 	  assignCopyTime[4] = 0x14;
	 	  assignCopyTime[5] = 0x00; 		 	  
	 }
	 
	 selectParameter(0x04, 100, (INT8U *)teApn, 64);						  													   //F100
	
	 //selectParameter(0x04, 129, (INT8U *)&acSamplePara, sizeof(AC_SAMPLE_PARA));            //F129
	 //2012-09-26,为了防止数据库崩溃而无交采校表参数而增加备份参数文件
	 foundAcPara = 1;
	 if (selectParameter(0x04, 129, (INT8U *)&acSamplePara, sizeof(AC_SAMPLE_PARA))==FALSE)	  //F129
	 {
	   //如果备份终端地址文件存在
	   if(access("/keyPara129", F_OK) == 0)
	   {
	   	 readBakKeyPara(129, &acSamplePara);
	   	 
	   	 printf("数据库中无校表参数,备份校表参数文件存在,使用备份校表参数\n");
	   }
	   else
	   {
	   	 printf("数据库中无校表参数,备份校表参数文件也不存在,确认为无校表参数\n");

	   	 foundAcPara = 0;
	   }
	 }
	 else
	 {
	   //如果备份终端地址文件不存在
	   if(access("/keyPara129", F_OK) != 0)
	   {
	     saveBakKeyPara(129);
	     
	     printf("数据库中有校表参数,备份校表参数文件不存在,保存备份校表参数\n");
     }
   }
   
   if (foundAcPara==1)
   {
     readBakKeyPara(129, &bakAcPara);
  
     pAcPara = (INT8U *)&acSamplePara;
     pAcParax = (INT8U *)&bakAcPara;
     for(i=0; i<sizeof(AC_SAMPLE_PARA); i++)
     {
     	 if (*pAcPara!=*pAcParax)
     	 {
     	 	 readBakKeyPara(129, &acSamplePara);
	       
	       printf("数据库中的校表参数与备份校表参数不一致,使用备份文件校表值\n");

     	 	 break;
     	 }
     	 
     	 pAcPara++;
     	 pAcParax++;
     }
   }
   
   if (selectParameter(0x04, 133, mainNodeAddr, 6)==FALSE)
   {
      tmpAddr = hexToBcd(addrField.a2[0] | addrField.a2[1]<<8);
      mainNodeAddr[0] = tmpAddr&0xff;
      mainNodeAddr[1] = tmpAddr>>8&0xff;
      mainNodeAddr[2] = tmpAddr>>16&0xf;
      mainNodeAddr[3] = 0x0;
      mainNodeAddr[4] = 0x0;
      mainNodeAddr[5] = 0x0;
   }
   else
   {
     if (mainNodeAddr[0]==0x00 && mainNodeAddr[1]==0x00 && mainNodeAddr[2]==0x00
     	   && mainNodeAddr[3]==0x00 && mainNodeAddr[4]==0x00 && mainNodeAddr[5]==0x00
     	  )
     {
        tmpAddr = hexToBcd(addrField.a2[0] | addrField.a2[1]<<8);
        mainNodeAddr[0] = tmpAddr&0xff;
        mainNodeAddr[1] = tmpAddr>>8&0xff;
        mainNodeAddr[2] = tmpAddr>>16&0xf;
        mainNodeAddr[3] = 0x0;
        mainNodeAddr[4] = 0x0;
        mainNodeAddr[5] = 0x0;
     }
   }

   //设置编号
   selectParameter(0x04, 134, (INT8U *)&deviceNumber, 2);     
   if (deviceNumber==0x0)
   {
   	 deviceNumber = 0x1;
   }
   
   //锐拔模块参数
   selectParameter(0x04, 135, (INT8U *)&rlPara, 4);
   //基本功率
   if (rlPara[0]==0x0)
   {
   	 rlPara[0] = 50;
   }
   
   //最大功率
   if (rlPara[1]==0x0)
   {
   	 rlPara[1] = 63;
   }
   
   //信号强度
   if (rlPara[2]==0x0)
   {
   	 rlPara[2] = 40;
   }
   
   //信道
   if (rlPara[3]==0x0)
   {
   	 rlPara[3] = 1;
   }
   
   selectParameter(0x04, 136, (INT8U *)&csNameId, 12);

  #ifdef PLUG_IN_CARRIER_MODULE
   //民民用户表数据类型(对07表有意义)
   if (selectParameter(0x04, 138, (INT8U *)&denizenDataType, 1)==FALSE)
   {
   	 denizenDataType = 0x0;    //默认为：实时+冻结数据
   }
   
   switch(denizenDataType)
   {
   	 case 0x55:
   	 	 printf("------居民用户07表:仅抄读实时数据(总及各费率)----------------\n");
   	 	 break;

   	 case 0xaa:
   	 	 printf("------居民用户07表:仅抄读实时(总示值)------------------------\n");
   	 	 break;

   	 default:
   	 	 printf("------居民用户07表:抄读实时+冻结数据-------------------------\n");
   	 	 break;
   }
   
   //轮显内容设置
   if (selectParameter(0x04, 199, (INT8U *)&cycleDataType, 1)==FALSE)
   {
   	 cycleDataType = 0x0;    //默认为:所有表计
   }

  #endif

	 //组6(总加组相关信息)
	 for(i=0; i<8; i++)
	 {
		 selectViceParameter(0x04, 41, (i+1), (INT8U *)&periodCtrlConfig[i], sizeof(PERIOD_CTRL_CONFIG));		//F41
		 selectViceParameter(0x04, 42, (i+1), (INT8U *)&wkdCtrlConfig[i], sizeof(WKD_CTRL_CONFIG));					//F42
		 selectViceParameter(0x04, 43, (i+1), (INT8U *)&powerCtrlCountTime[i], sizeof(POWERCTRL_COUNT_TIME));//F43
		 selectViceParameter(0x04, 44, (i+1), (INT8U *)&obsCtrlConfig[i], sizeof(OBS_CTRL_CONFIG));					//F44
		 selectViceParameter(0x04, 45, (i+1), (INT8U *)&powerCtrlRoundFlag[i], sizeof(POWERCTRL_ROUND_FLAG));//F45
		 selectViceParameter(0x04, 46, (i+1), (INT8U *)&monthCtrlConfig[i], sizeof(MONTH_CTRL_CONFIG));			//F46
		 selectViceParameter(0x04, 47, (i+1), (INT8U *)&chargeCtrlConfig[i], sizeof(CHARGE_CTRL_CONFIG));		//F47
		 selectViceParameter(0x04, 48, (i+1), (INT8U *)&electCtrlRoundFlag[i], sizeof(ELECTCTRL_ROUND_FLAG));//F48
		 selectViceParameter(0x04, 49, (i+1), (INT8U *)&powerCtrlAlarmTime[i], sizeof(POWERCTRL_ALARM_TIME));//F49
	 }
	
	 //控制参数 AFN05
   selectParameter(0x05, 1, (INT8U *)&remoteCtrlConfig,sizeof(REMOTE_CTRL_CONFIG)*CONTROL_OUTPUT);//遥控设置参数及过程参数
   selectParameter(0x05, 3, (INT8U *)&remoteEventInfor,sizeof(REMOTE_EVENT_INFOR)*8);             //遥控现场参数用FN03代替
   selectParameter(0x05, 4, (INT8U *)&ctrlRunStatus, sizeof(CTRL_RUN_STATUS)*8);                  //控制投运状态用FN04代替
   selectParameter(0x05, 5, (INT8U *)&powerCtrlEventInfor, sizeof(POWER_CTRL_EVENT_INFOR));       //功控现场记录状态用FN05代替
   selectParameter(0x05,12, (INT8U *)&powerDownCtrl, sizeof(POWER_DOWN_CONFIG)*8);                //当前功率下浮功控F12
   selectParameter(0x05,25, (INT8U *)&staySupportStatus, sizeof(STAY_SUPPORT_STATUS));            //终端保电投入F25/解除F33
   selectParameter(0x05,26, &reminderFee, 1);                                                     //催费告警投入F26/解除F34
   selectParameter(0x05,28, &toEliminate, 1);                                                     //剔除投入F28/解除F36	   

   selectParameter(0x05,29, &callAndReport, 1);                    //允许/禁止终端主动上报
   selectParameter(0x05,30, &teInRunning, 1);                      //终端运行投入/退出运行
   selectParameter(0x05,32, &chnMessage, sizeof(CHN_MESSAGE));     //中文信息
   
   //终端参数 AFN88
   //终端参数列表:
   //    FN=01,终端停上电记录
   //    FN=02,终端重要事件已读事件
   //    FN=03,脉冲量数据缓存
   //    FN=07,LCD对比度值
   //    FN=08,界面密码
   //    FN=13,脉冲量计数缓存
   //    FN=33,远程升级标志
   //    FN=34,13版规约升级标志
   //    FN=55,本地通信模块抄读方式
   selectParameter(88, 1, (INT8U *)&powerOnOffRecord,sizeof(POWER_ON_OFF));   //终端停上电记录
   selectParameter(88, 2, eventReadedPointer,2);                              //终端重要事件已读指针
   
   #ifdef PULSE_GATHER
     //脉冲量数据缓存
     if (selectParameter(88,  3, pulseDataBuff, NUM_OF_SWITCH_PULSE*53)==FALSE)
     {
     	  printf("DataBase-loadParameter:无脉冲量数据缓存,将缓存清零\n");
     	  
     	  memset(pulseDataBuff, 0x0, NUM_OF_SWITCH_PULSE*53);
     }
     
     //脉冲量计数缓存
     if (selectParameter(88, 13, pulse, sizeof(ONE_PULSE)*NUM_OF_SWITCH_PULSE)==FALSE)
     {
     	 printf("DataBase-loadParameter:无脉冲量计数缓存,将脉冲计数清零\n");
     	 
     	 for (i = 0; i < NUM_OF_SWITCH_PULSE; i++)
     	 {
     	 	 pulse[i].pulseCount = 0x0;
     	 	 
     	 	 for(j=0;j<14;j++)
     	 	 {
     	 	 	 pulse[i].pulseCountTariff[j] = 0x0;
     	 	 }
     	 }
     }
   #endif

   //FN=7,LCD对比度 
   selectParameter(88, 7,&lcdDegree,1);                                      //LCD对比度值
   if (lcdDegree==0)
   {
   	 lcdDegree = 10;
   }
   
   //FN=8,界面密码
   selectParameter(88, 8, originPassword, 7);

   //FN=55,本地通信模块抄读方式 ly,2012-01-13,add
   if (selectParameter(88, 55, &localCopyForm, 1)==FALSE)
   {
   	 localCopyForm = 0xaa;
   }
 	 if (localCopyForm!=0x55 && localCopyForm!=0xaa)    //ly,2012-01-13,add
 	 {
 	 	 localCopyForm = 0xaa;        //默认为路由主导抄读,0xaa为集中器主导抄读
 	 }

   //FN=56,本地通信模块协议 ly,2015-09-16,add
   if (selectParameter(88, 56, &lmProtocol, 1)==FALSE)
   {
   	 lmProtocol = 0xaa;
   }
 	 if (lmProtocol!=0x55 && lmProtocol!=0xaa)
 	 {
 	 	 lmProtocol = 0xaa;        //默认为Q/GDW376.2-2009
 	 }
 	 
	 //2.读出事件记录指针
	 iEventStartPtr = queryEventStoreNo(1);
	 nEventStartPtr = queryEventStoreNo(2);
  
   //2-1重要事件指针
	 if (iEventStartPtr>255)
	 {
		 iEventStartPtr -= 255;
     
	   pSqlStr = sqlite3_mprintf("delete from eventRecord where eventType=1 and storeNo<%d",iEventStartPtr);
     if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
     {
   	    if (debugInfo&PRINT_EVENT_DEBUG)
   	    {
   	      printf("删除多余的重要事件记录成功!\n");
   	    }
     }
     sqlite3_free(pSqlStr);
     
		 iEventCounter = 255;
	 }
	 else
	 {
		 iEventCounter  = iEventStartPtr;
		 iEventStartPtr = 0;
	 }

   //2-2.一般事件指针	
	 if (nEventStartPtr>255)
	 {
		  nEventStartPtr -= 255;

	    pSqlStr = sqlite3_mprintf("delete from eventRecord where eventType=2 and storeNo<%d",nEventStartPtr);
      if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
      {
   	    if (debugInfo&PRINT_EVENT_DEBUG)
   	    {
   	      printf("删除多余的一般事件记录成功!\n");
   	    }
      }
      sqlite3_free(pSqlStr);
     
		  nEventCounter  = 255;
	 }
	 else
	 {
		  nEventCounter  = nEventStartPtr;
		  nEventStartPtr = 0;
	 }
	
	 if (debugInfo&PRINT_EVENT_DEBUG)
	 {
	  printf("iEventStartPtr=%d,iEventCounter=%d,nEventStartPtr=%d,nEventCounter=%d\n",iEventStartPtr,iEventCounter,nEventStartPtr,nEventCounter);
   }
   
   //交采需量缓存
  #ifdef AC_SAMPLE_DEVICE
   readAcVision(acReqTimeBuf, sysTime, REQ_REQTIME_DATA);
  #endif
  
 #ifdef LIGHTING
  initCtrlTimesLink();
  
  //F51 控制点阈值
	selectParameter(0x04, 51, (INT8U *)&pnGate, sizeof(PN_GATE));    //F51
	
	//F52 控制模式,2015-6-9,add
	selectParameter(0x04, 52, &ctrlMode, 1);
	printf("控制模式:%d\n", ctrlMode);

	//F53 光控提前-延迟时长,2015-06-25,add
	selectParameter(0x04, 53, beforeOnOff, 4);
	
	//2015-11-16,Add默认值
	if (beforeOnOff[0]==0)
	{
		beforeOnOff[0] = 30;
	}
	if (beforeOnOff[1]==0)
	{
		beforeOnOff[1] = 30;
	}
	if (beforeOnOff[2]==0)
	{
		beforeOnOff[2] = 30;
	}
	if (beforeOnOff[3]==0)
	{
		beforeOnOff[3] = 30;
	}
	printf("光控开灯提前%d分钟有效\n", beforeOnOff[0]);
	printf("光控开灯延迟%d分钟有效\n", beforeOnOff[1]);
	printf("光控关灯提前%d分钟有效\n", beforeOnOff[2]);
	printf("光控关灯延迟%d分钟有效\n", beforeOnOff[3]);
	
  if (pnGate.failureRetry == 0x00 && pnGate.boardcastWaitGate == 0x00 && pnGate.checkTimeGate == 0x00)
  {
  	pnGate.failureRetry  = 0x01;        //CCB发现故障重试次数默认1次
  	pnGate.boardcastWaitGate = 0x01;    //默认1分钟广播命令等待阈值
  	pnGate.checkTimeGate = 0x01;        //默认1分钟超差校时
  	pnGate.lddOffGate = 0x05;           //默认5小时离线域值,2016-11-28,离线域值单位从“分”改成“小时”
  	pnGate.lddtRetry = 0x05;            //默认5分钟搜索末端重试次数
  	pnGate.offLineRetry = 0x02;         //默认2分钟离线重试次数
  	pnGate.lcWave = 0x00;               //默认0Lux光控照度震荡值
  	pnGate.leakCurrent = 10;            //默认10mA漏电流阈值

    saveParameter(0x04, 51, (INT8U *)&pnGate, sizeof(PN_GATE));
     
    printf("设置控制点阈值初始值\n");
  }

 #endif
}

/*******************************************************
函数名称:saveParameter
功能描述:用于存储数据
调用函数:
被调用函数:
输入参数:para:待存储的数据  len: 数据长度（字节）
输出参数:
返回值：void
*******************************************************/
void saveParameter(INT8U afn, INT8U fn, INT8U *para, INT32U len)
{
	sqlite3_stmt *stat;
	
	INT8S *sql;
	
	INT16U result;
  
	sql = "select * from base_info where acc_afn = ? and acc_fn = ?";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_blob(stat, 1, &afn, 1, NULL);
	sqlite3_bind_blob(stat, 2, &fn, 1, NULL);
	result = sqlite3_step(stat);
	sqlite3_finalize(stat);
	
	if(result == SQLITE_ROW)
	{
		sql = "update base_info set acc_data = ? where acc_afn = ? and acc_fn = ?";
		sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
		sqlite3_bind_blob(stat, 1, para, len, NULL);
		sqlite3_bind_blob(stat, 2, &afn, 1, NULL);
		sqlite3_bind_blob(stat, 3, &fn, 1, NULL);
		sqlite3_step(stat);
		sqlite3_finalize(stat);
	}
	else
	{
		sql = "insert into base_info(acc_afn, acc_fn, acc_data) values(?, ?, ?)";
		sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
		sqlite3_bind_blob(stat, 1, &afn, 1, NULL);
		sqlite3_bind_blob(stat, 2, &fn, 1, NULL);
		sqlite3_bind_blob(stat, 3, para, len, NULL);
		sqlite3_step(stat);
		sqlite3_finalize(stat);
	}
}

/*******************************************************
函数名称:saveViceParameter
功能描述:用于存储数据(基本信息副表), 进行更新，插入操作
调用函数:
被调用函数:
输入参数:fn  pn  para:待存储的数据  len: 数据长度（字节）
输出参数:
返回值：void
*******************************************************/
void saveViceParameter(INT8U afn, INT8U fn, INT16U pn, INT8U *para, INT16U len)
{
	sqlite3_stmt *stat;
	
	INT8S *sql;
	
	INT16U result;
	
	sql = "select * from base_vice_info where acc_afn = ? and acc_fn = ? and acc_pn = ?;";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_blob(stat, 1, &afn, 1, NULL);
	sqlite3_bind_blob(stat, 2, &fn, 1, NULL);
	sqlite3_bind_blob(stat, 3, &pn, 2, NULL);
	result = sqlite3_step(stat);
	sqlite3_finalize(stat);
	if(result == SQLITE_ROW)
	{
		sql = "update base_vice_info set acc_data = ? where acc_afn = ? and acc_fn = ? and acc_pn = ?;";
		sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
		sqlite3_bind_blob(stat, 1, para, len, NULL);
		sqlite3_bind_blob(stat, 2, &afn, 1, NULL);
		sqlite3_bind_blob(stat, 3, &fn, 1, NULL);
		sqlite3_bind_blob(stat, 4, &pn, 2, NULL);
		sqlite3_step(stat);
		sqlite3_finalize(stat);
	}
	else
	{
		sql = "insert into base_vice_info(acc_afn, acc_fn, acc_pn, acc_data) values(?, ?, ?, ?);";
		sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
		sqlite3_bind_blob(stat, 1, &afn, 1, NULL);
		sqlite3_bind_blob(stat, 2, &fn, 1, NULL);
		sqlite3_bind_blob(stat, 3, &pn, 2, NULL);
		sqlite3_bind_blob(stat, 4, para, len, NULL);
		sqlite3_step(stat);
		sqlite3_finalize(stat);
	}
}

/*******************************************************
函数名称:deleteParameter
功能描述:用于删除数据(基本信息副表)
调用函数:
被调用函数:
输入参数:afn  fn
输出参数:
返回值：void
*******************************************************/
void deleteParameter(INT8U afn, INT8U fn)
{
	sqlite3_stmt *stat;
	
	INT8S *sql;

	//删除相应数据
	sql = "delete from base_info where acc_afn = ? and acc_fn = ?";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_blob(stat, 1, &afn, 1, NULL);
	sqlite3_bind_blob(stat, 2, &fn, 1, NULL);
	sqlite3_step(stat);
	sqlite3_finalize(stat);
}

/*******************************************************
函数名称:insertParameter
功能描述:用于存储数据(基本信息副表)
调用函数:
被调用函数:
输入参数:fn  pn  para:待存储的数据  len: 数据长度（字节）
输出参数:
返回值：void
*******************************************************/
void insertParameter(INT8U afn, INT8U fn, INT16U pn, INT8U *para, INT16U len)
{
	sqlite3_stmt *stat;
	
	INT8S *sql;

	//插入新数据
	sql = "insert into base_vice_info(acc_afn, acc_fn, acc_pn, acc_data) values(?, ?, ?, ?)";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_blob(stat, 1, &afn, 1, NULL);
	sqlite3_bind_blob(stat, 2, &fn, 1, NULL);
	sqlite3_bind_blob(stat, 3, &pn, 1, NULL);
	sqlite3_bind_blob(stat, 4, para, len, NULL);
	sqlite3_step(stat);
	sqlite3_finalize(stat);
}

/*******************************************************
函数名称:countParameter
功能描述:用于统计数据(基本信息副表)
调用函数:
被调用函数:
输入参数:fn    	pn count:存在数据数量
输出参数:
返回值：void
*******************************************************/
void countParameter(INT8U afn, INT8U fn, INT16U *num)
{
	sqlite3_stmt *stat;
	
	INT8S *sql;
	
	INT16U result;
	
	*num = 0;
	
	//计算数据数量
	sql = "select * from f10_info";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	result = sqlite3_step(stat);
	while(result == SQLITE_ROW)
	{
		(*num)++;
		result = sqlite3_step(stat);
	}
	sqlite3_finalize(stat);
}

/*******************************************************
函数名称:selectParameter
功能描述:用于查询基本表中的数据
调用函数:
被调用函数:
输入参数:afn  fn  *para:保存查询出的数据   len:数据长度(字节为单位)
输出参数:
返回值：void
*******************************************************/
BOOL selectParameter(INT8U afn, INT8U fn, INT8U *para, INT32U len)
{
	sqlite3_stmt *stat;
	
	INT8S *sql;
	
	INT16U result;
	
	//查询数据
	sql = "select * from base_info where acc_afn = ? and acc_fn = ?";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_blob(stat, 1, &afn, 1, NULL);
	sqlite3_bind_blob(stat, 2, &fn, 1, NULL);
	result = sqlite3_step(stat);
	if(result == SQLITE_ROW)
	{
		memcpy(para, sqlite3_column_blob(stat, 2), len);
		sqlite3_finalize(stat);
    
    if (afn==4)
    {
    	if (fn>0 && fn<84)
    	{
    	  paraStatus[(fn-1)/8] |= 1<<((fn-1)%8);               //置"终端参数状态"位
    	}
    }
    
		return TRUE;
	}
	else
	{
		if (result!=SQLITE_DONE)
		{
		  logRun("selectParameter happen aberrant.");
		}
		
		sqlite3Aberrant(result);
	}

	sqlite3_finalize(stat);

	return FALSE;
}

/*******************************************************
函数名称:selectViceParameter
功能描述:用于查询基本副表中的数据
调用函数:
被调用函数:
输入参数:afn  fn  pn:测量点号/总加组号/任务号/直流模拟量端口号  
         *para:保存查询出的数据   len:数据长度(字节为单位)
输出参数:
返回值：void
*******************************************************/
BOOL selectViceParameter(INT8U afn, INT8U fn, INT16U pn, INT8U *para, INT16U len)
{
	sqlite3_stmt *stat;
	
	INT8S *sql;	
	INT16U result;
	
	//查询数据
	sql = "select * from base_vice_info where acc_afn = ? and acc_fn = ? and acc_pn = ?";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_blob(stat, 1, &afn, 1, NULL);
	sqlite3_bind_blob(stat, 2, &fn, 1, NULL);
	sqlite3_bind_blob(stat, 3, &pn, 2, NULL);
	result = sqlite3_step(stat);
	if(result == SQLITE_ROW)
	{
		memcpy(para, sqlite3_column_blob(stat, 3), len);
		sqlite3_finalize(stat);
		
		return TRUE;
	}
	sqlite3_finalize(stat);

	return FALSE;
}

/*******************************************************
函数名称:saveDataF10
功能描述:用于存储F10数据, 进行更新，插入操作
调用函数:
被调用函数:
输入参数:pn  port:端口号  num:序号  
				 para:待存储的数据  len: 数据长度（字节）
输出参数:
返回值：void
*******************************************************/
void saveDataF10(INT16U pn, INT8U port, INT8U *meterAddr, INT16U num, INT8U *para, INT16U len)
{
	METER_DEVICE_CONFIG mdc;
	sqlite3_stmt        *stat;
	INT8S               *sql;
	INT16U              result;
	
	//2013-05-07,经东软测试提示才发现,不是序号为0的删除,是测量点为0的删除
	if (0==pn)
	{
    //查询该序号对应的原测量点,如有则删除原测量点的冻结数据
    sql = "select * from f10_info where acc_num = ?";
    sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
    sqlite3_bind_int(stat, 1, num);
    result = sqlite3_step(stat);
    if(result == SQLITE_ROW)
    {
    	memcpy((INT8U *)&mdc, sqlite3_column_blob(stat, 4), sizeof(METER_DEVICE_CONFIG));
      sqlite3_finalize(stat);
      
      if (debugInfo&PRINT_DATA_BASE)
      {
        printf("saveDataF10:删除测量点%d的冻结数据\n", mdc.measurePoint);
      }
      	
      deleteFreezeData(mdc.measurePoint);
    }
    else
    {
      sqlite3_finalize(stat);
    }
	
	  //删除测量点号为0的序号为num的测量点
	  sql = "delete from f10_info where acc_num = ?";
	  sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	  sqlite3_bind_int(stat, 1, num);
	  result = sqlite3_step(stat);
	  sqlite3_finalize(stat);
		
		if (debugInfo&PRINT_DATA_BASE)
		{
		  printf("saveDataF10:删除测量点号为0的序号为%d的表地址\n", num);
		}
		
		return;
	}

	//2012-09-06,在长寿发现地玮主站是一条一条的删除表地址,用的是规约上说的序号为0的就删除该测量点
	//           增加该处理以适应地玮主站
	if (0==num)
	{
	  //删除序号为0的测量点
	  sql = "delete from f10_info where acc_pn = ?";
	  sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	  sqlite3_bind_int(stat, 1, pn);
	  result = sqlite3_step(stat);
	  sqlite3_finalize(stat);
		
		if (debugInfo&PRINT_DATA_BASE)
		{
		  printf("saveDataF10:删除序号为0的测量点%d参数\n", pn);
		}
		
		return;
	}
	
	//查询原来是否有对应的测量点,如有,本次会删除原测量点,同时删除原测量点的冻结数据
	sql = "select * from f10_info where acc_pn = ? or acc_num = ? or acc_meter_addr = ?";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_int(stat, 1, pn);
	sqlite3_bind_int(stat, 2, num);
	sqlite3_bind_blob(stat, 3, meterAddr, 6, NULL);
	result = sqlite3_step(stat);	
	if(result == SQLITE_ROW)
	{
		memcpy((INT8U *)&mdc, sqlite3_column_blob(stat, 4), sizeof(METER_DEVICE_CONFIG));
	  sqlite3_finalize(stat);
    
    if ((mdc.measurePoint!=pn) || (compareTwoAddr(mdc.addr, meterAddr, 0)==FALSE))
    {
    	if (debugInfo&PRINT_DATA_BASE)
    	{
    	  printf("saveDataF10:测量点号不相同或表地址不相同,需要删除数据\n");
    	}
    	
      deleteFreezeData(mdc.measurePoint);
    }
	}
	else
	{
	  sqlite3_finalize(stat);
	}
	
	//删除已存在的数据
	sql = "delete from f10_info where acc_pn = ? or acc_num = ? or acc_meter_addr = ?";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_int(stat, 1, pn);
	sqlite3_bind_int(stat, 2, num);
	sqlite3_bind_blob(stat, 3, meterAddr, 6, NULL);
	result = sqlite3_step(stat);
	sqlite3_finalize(stat);
	
	//插入新的数据
	sql = "insert into f10_info(acc_pn, acc_port, acc_meter_addr, acc_num, acc_data) values(?, ?, ?, ?, ?)";
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_int(stat, 1, pn);
	sqlite3_bind_int(stat, 2, port);
	sqlite3_bind_blob(stat, 3, meterAddr, 6, NULL);
	sqlite3_bind_int(stat, 4, num);
	sqlite3_bind_blob(stat, 5, para, len, NULL);
	sqlite3_step(stat);
	sqlite3_finalize(stat);
}

/*******************************************************
函数名称:selectF10Data
功能描述:用于查询F10数据, 进行更新，插入操作
调用函数:
被调用函数:
输入参数:pn  port:端口号  num:序号  
				 para:待存储的数据  len: 数据长度（字节）
输出参数:
返回值：void
*******************************************************/
BOOL selectF10Data(INT16U pn, INT8U port, INT16U num, INT8U *para, INT16U len)
{
	sqlite3_stmt *stat;
	INT8S        *sql;	
	INT8U        flg;
	INT16U       result;
	
	sql = "select * from f10_info where (acc_pn = ? or acc_num = ?)";
	if(port != 0)
	{
		sql = "select * from f10_info where (acc_pn = ? or acc_num = ?) and acc_port = ?";
	}
	else
	{
		sql = "select * from f10_info where acc_pn = ? or acc_num = ?";
	}
	
	sqlite3_prepare(sqlite3Db, sql, -1, &stat, 0);
	sqlite3_bind_int(stat, 1, pn);
	sqlite3_bind_int(stat, 2, num);
	if(port != 0)
	{
		sqlite3_bind_int(stat, 3, port);
	}
	result = sqlite3_step(stat);
	
	if(result == SQLITE_ROW)
	{
		 memcpy(para, sqlite3_column_blob(stat, 4), len);
	   sqlite3_finalize(stat);
		 return TRUE;
	}
	sqlite3_finalize(stat);
	
	return FALSE;
}

/*******************************************************
函数名称:queryData
功能描述:根据传入的SQL语句查询库中有几条记录
调用函数:
被调用函数:
输入参数:char  *pSql-SQL语句,
         INT8U *data-查询成功返回的数据
         INT8U type -查询类型(1-参数,2-查询抄表时间,3..待定义)
输出参数:
返回值:查询到的记录条数
*******************************************************/
INT16U queryData(char *pSql,INT8U *data,INT8U type)
{
	 sqlite3_stmt *stmt;
	 const char   *tail;
	 int          execResult;
	 INT16U       numOfRecord;
	 INT8U        i,ncols;
	 
   if (sqlite3_prepare(sqlite3Db, pSql, strlen(pSql), &stmt, &tail)!= SQLITE_OK)
   {
     fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(sqlite3Db));
   }   
   execResult = sqlite3_step(stmt);
   
   if (type)
   {
      ncols = sqlite3_column_count(stmt);
   }
   
   numOfRecord=0;
   while(execResult==SQLITE_ROW)
   {
     if (type)
     {
       for(i=0; i < ncols; i++)
       {
         switch(type)
         {
           case 1:    //查询参数
             if (i>1)
             {
               *data = atoi(sqlite3_column_text(stmt, i));
               data++;
             }
             break;

           case 2:    //查询抄表时间
             *data = atoi(sqlite3_column_text(stmt, i));
             data++;
             break;
         }
       }
     }
     
     numOfRecord++;
     
     if (type==2)
     {
      	break;
     }

     execResult = sqlite3_step(stmt);
   }
   
   sqlite3_finalize(stmt);
   
   return numOfRecord;
}

/*******************************************************
函数名称:qureyEventStoreNo
功能描述:查询事件存储序号
调用函数:
被调用函数:
输入参数:INT8U  eventType,事件类型
输出参数:
返回值:查询到存储序号
*******************************************************/
INT32U queryEventStoreNo(INT8U eventType)
{
	 sqlite3_stmt *stmt;
	 const char   *tail;
   char         *pSqlStr;                  //SQL语句字符串指针
	 int          execResult;
	 INT32U       recordNo;
	 
	 pSqlStr = sqlite3_mprintf("select storeNo from eventRecord where eventType=%d order by storeNo desc limit 1",eventType);
	 
   if (sqlite3_prepare(sqlite3Db, pSqlStr, strlen(pSqlStr), &stmt, &tail)!= SQLITE_OK)
   {
     fprintf(stderr, "Query Store No SQL error: %s\n", sqlite3_errmsg(sqlite3Db));
   }   
   execResult = sqlite3_step(stmt);
   
   recordNo=0;
   
   while(execResult==SQLITE_ROW)
   {
      recordNo = sqlite3_column_int(stmt,0);
            
      execResult = sqlite3_step(stmt);
   }
   sqlite3_finalize(stmt);
   
   return recordNo;
}

/*******************************************************
函数名称:insertData
功能描述:向数据库中插入数据
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
BOOL insertData(char *pSql)
{
   char    *errMessage;               //错误消息(Error msg written her)
   
   if (sqlite3_exec(sqlite3Db, pSql, NULL, NULL, &errMessage)==SQLITE_OK)
   {
   	  return TRUE;
   }
   
   return FALSE;
}

/*******************************************************
函数名称:saveBakDayFile
功能描述:保存日冻结数据到文件中
调用函数:
被调用函数:
输入参数:type =0x01,单相表数据
              =0x0b,三相表示值
              =0x0c,三相表需量
输出参数:
返回值:void
*******************************************************/
BOOL saveBakDayFile(INT16U pn, int freezeTime, int copyTime, INT8U *buf, INT8U type)
{
	FILE           *fp;
	char           fileName[20];
  struct timeval tv;                  //Linux timeval
  DATE_TIME      tmpTime;
  SP_F_DAY       tmpSpfDay;
  MP_F_DAY       tmpMpfDay;
  INT16U         tmpCount;
	
  tv.tv_sec = freezeTime;
  getLinuxFormatDateTime(&tmpTime, &tv, 2);
	
	//冻结数据文件名
	if (type==1)
	{
	  sprintf(fileName, "/data/spd%02d%02d%02d", tmpTime.year, tmpTime.month, tmpTime.day);
	}
	else
	{
	  sprintf(fileName, "/data/mpd%02d%02d%02d", tmpTime.year, tmpTime.month, tmpTime.day);
	}

  //查看目录/文件是否存在
  if(access(fileName, F_OK) != 0)
  {
    if (mkdir("/data", S_IRUSR | S_IWUSR | S_IXUSR)==0)
    {
    	if (debugInfo&PRINT_DATA_BASE)
    	{
    	  printf("saveBakDayFile:创建目录/data成功\n");
    	}
    }
    else
    {
    	if (debugInfo&PRINT_DATA_BASE)
    	{
    	  printf("saveBakDayFile:创建目录/data失败\n");
    	}
    }
    
    //创建文件
    if((fp=fopen(fileName,"wb+"))==NULL)
    {
      if (debugInfo&PRINT_DATA_BASE)
      {
        printf("saveBakDayFile:创建文件%s失败.\n", fileName);
      }
      	
      return FALSE;
    }
      
    fclose(fp);
  }

  if((fp=fopen(fileName, "rb+"))==NULL)
  {
    if (debugInfo&PRINT_DATA_BASE)
    {
  	  printf("saveBakDayFile:打开文件%s失败\n", fileName);
  	}
  	
  	return FALSE;
  }

  tmpCount = 0;
  while (!feof(fp))
  {
  	if (type==1)
  	{
    	if (fread(&tmpSpfDay, sizeof(SP_F_DAY), 1, fp)!=1)
    	{
    	  fseek(fp, 0 ,2);    //到文件尾
    	  
    	  break;
    	}
      
      if (tmpSpfDay.pn==pn)
      {
      	if (debugInfo&PRINT_DATA_BASE)
      	{
      	  printf("saveBakDayFile:测试点%d数据已存在\n", pn);
      	}
      	
      	//定位到本记录
      	rewind(fp);
      	fseek(fp, tmpCount*sizeof(SP_F_DAY), 0);
      	break;
      }
    }
    else
    {
    	if (fread(&tmpMpfDay, sizeof(MP_F_DAY), 1, fp)!=1)
    	{
    	  fseek(fp, 0 ,2);    //到文件尾
    	  
    	  break;
    	}
      
      if (tmpMpfDay.pn==pn)
      {
      	if (debugInfo&PRINT_DATA_BASE)
      	{
      	  printf("saveBakDayFile:测试点%d数据已存在\n", pn);
      	}
      	
      	//定位到本记录
      	rewind(fp);
      	fseek(fp, tmpCount*sizeof(MP_F_DAY), 0);
      	break;
      }
    }

  	tmpCount++;
  }
  
  if (type==1)
  {
    tmpSpfDay.pn = pn;
    tmpSpfDay.copyTime = copyTime;
    memcpy(tmpSpfDay.data, buf, 40);
    if (fwrite(&tmpSpfDay, sizeof(SP_F_DAY), 1, fp)!=1)
    {
    	if (debugInfo&PRINT_DATA_BASE)
    	{
    	  printf("saveBakDayFile:写入数据到文件%s失败\n", fileName);
    	}
    	
    	fclose(fp);
    	return FALSE;
    }
  }
  else
  {
    tmpMpfDay.pn   = pn;
    tmpMpfDay.type = type;
    tmpMpfDay.copyTime = copyTime;
    memcpy(tmpMpfDay.data, buf, 288);
    if (fwrite(&tmpMpfDay, sizeof(MP_F_DAY), 1, fp)!=1)
    {
    	if (debugInfo&PRINT_DATA_BASE)
    	{
    	  printf("saveBakDayFile:写入数据到文件%s失败\n", fileName);
    	}
    	
    	fclose(fp);
    	return FALSE;
    }
  }
  
  if (debugInfo&PRINT_DATA_BASE)
  {
    printf("saveBakDayFile:写入数据到文件%s成功\n", fileName);
  }
  
  fclose(fp);
  return TRUE;
}

/*******************************************************
函数名称:saveCopyData
功能描述:保存抄表数据
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
BOOL saveMeterData(INT16U pn, INT8U port, DATE_TIME saveTime, INT8U *buff, INT8U queryType, INT8U dataType,INT16U len)
{
	 sqlite3_stmt   *stmt;
   char           *pSqlStr;                  //SQL语句字符串指针
	 INT8U          tmpBuff[768];
   char           *errMessage;               //错误消息(Error msg written her)
   DATE_TIME      tmpTime, tmpTimex, tmpBakTimex;
   char           tableName[30];
   INT16U         i;
   struct timeval tv, tvxx;                  //Linux timeval
   INT8U          backDataType;
   INT16U         execResult;
   SP_F_DAY       spFileDay;                 //单相表日冻结数据文件记录变量,2012-09-30
   
   pSqlStr=NULL;

   if (debugInfo&PRINT_DATA_BASE)
   {
     printf("saveMeterData(%d-%d-%d %d:%d:%d):ready,测量点:%d,查询类型=%02x,数据类型=%02x\n", sysTime.year, sysTime.month, sysTime.day, sysTime.hour, sysTime.minute, sysTime.second, pn, queryType, dataType);
   }

   while(dbLocker)
   {
   	 usleep(100);
   	 
   	 printf("**************数据库写入等待,pn=%d,queryType=%02x,dataType=%02x****************\n", pn, queryType, dataType);
   }
   
   dbLocker = 1;
   
   //清除数据期,禁止写入数据
   if (flagOfClearData==0x55 || flagOfClearData==0xaa)
   {
     //因为在清数据时要更新剩余电量,所以剩余电量除外
     if (queryType==LEFT_POWER && dataType==0)
     {
     	 ;
     }
     else
     {
       //if (debugInfo&PRINT_DATA_DEBUG)
       //{
         printf("saveMeterData:清除数据期,禁止写入数据,queryType=0x%02x,dataType=0x%02x\n",queryType,dataType);
       //}
   	   
   	   dbLocker = 0;
   	   
   	   return FALSE;
   	 }
   }

   //查找原来是否存在该测量点的统计数据,如果存在则更新
   if(queryType==STATIS_DATA)
   {
     tmpTime = saveTime;
     if (readMeterData(tmpBuff, pn, queryType, dataType, &tmpTime, 0)==TRUE)
     {
     	 switch (dataType)
     	 {
     	 	 case 88:   //三相表与时间无关量
   	       pSqlStr = sqlite3_mprintf("update statisData set data=? where pn=%d and day=88;", pn);
   	       break;

     	 	#ifdef LIGHTING
     	 	 case 89:   //线路测量点与时间无关量
   	       pSqlStr = sqlite3_mprintf("update statisData set data=? where pn=%d and day=89;", pn);
   	       break;
   	    #endif

     	 	 case 99:   //单相表与时间无关量
   	       pSqlStr = sqlite3_mprintf("update statisData set data=? where pn=%d and day=99;", pn);
   	       break;
   	       
   	     default:
   	       pSqlStr = sqlite3_mprintf("update statisData set data=? where pn=%d and day=%d and month=%d and year=%d;",pn, saveTime.day, saveTime.month,saveTime.year);
   	       break;
   	   }
   	   
   	   if (debugInfo&PRINT_DATA_BASE)
   	   {
   	     printf("更新测量点统计数据SQL:");
   	     printf(pSqlStr);
   	     printf("\n");
       }
       
       sqlite3_prepare(sqlite3Db, pSqlStr, -1, &stmt, 0);
       sqlite3_bind_blob(stmt, 1, buff, len, NULL);
       if (sqlite3_step(stmt)==SQLITE_DONE)
       {
       	 if (debugInfo&PRINT_DATA_BASE)
       	 {
       	   printf("saveMeterData:更新测量点%d统计数据成功\n",pn);
       	 }
       	 
       	 dbLocker = 0;
       	 
       	 sqlite3_finalize(stmt);

       	 return TRUE;
       }
       else
       {
       	 if (debugInfo&PRINT_DATA_BASE)
       	 {
       	   printf("saveMeterData:更新测量点%d统计数据失败\n",pn);
       	 }
         
         dbLocker = 0;
       	 
       	 sqlite3_finalize(stmt);
       	 
       	 return FALSE;
       }
     }
   }

   //查找原来是否存在该总加组的剩余电量数据,如果存在则更新
   if(queryType==LEFT_POWER)
   {
     tmpTime = saveTime;
     if (readMeterData(tmpBuff, pn, queryType, dataType, &tmpTime, 0)==TRUE)
     {
   	   pSqlStr = sqlite3_mprintf("update leftPower set data=? where pn=%d;",pn);
       sqlite3_prepare(sqlite3Db, pSqlStr, -1, &stmt, 0);
       sqlite3_bind_blob(stmt, 1, buff, len, NULL);
       if (sqlite3_step(stmt)==SQLITE_DONE)
       {
       	  if (debugInfo&PRINT_DATA_BASE)
       	  {
       	    printf("saveMeterData:更新总加组%d剩余电量数据成功\n",pn);
       	  }
       	  
       	  dbLocker = 0;
       	  
       	  sqlite3_finalize(stmt);

       	  return TRUE;
       }
       else
       {
       	  if (debugInfo&PRINT_DATA_BASE)
       	  {
       	    printf("saveMeterData:更新总加组%d剩余电量数据失败\n",pn);
       	  }
          
          dbLocker = 0;

       	  sqlite3_finalize(stmt);

       	  return FALSE;
       }
     }
   }

   tmpTime = timeBcdToHex(saveTime);
   getLinuxFormatDateTime(&tmpTime,&tv,1);
   if (debugInfo&PRINT_DATA_BASE)
   {
     printf("saveMeterData:存储时间:%02x-%02x-%02x %02x:%02x:%02x,存储秒数:%d 测量点:%d\n",saveTime.year,saveTime.month,saveTime.day,saveTime.hour,saveTime.minute,saveTime.second,tv.tv_sec,pn);
   }

   switch (queryType)
   {
     case PRESENT_DATA:              //实时抄表数据
     case REAL_BALANCE:              //实时结算数据
     case LAST_REAL_BALANCE:         //最近一条实时结算数据
     	 strcpy(tableName,bringTableName(timeBcdToHex(saveTime),1));
     	 tmpTime = saveTime;
     	 if (readMeterData(tmpBuff, pn, queryType, dataType, &tmpTime, 0)==TRUE)
     	 {
     	 	 printf("saveMeterData:存在相同信息的数据(可能净数据不同),更新数据, 时间=%02x-%02x-%02x %02x:%02x:%02x pn=%d,queryType=%d,dataType=%d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second,pn, queryType, dataType);
	       
	       pSqlStr = sqlite3_mprintf("update %s set data=? where pn=%d and queryType=%d and dataType=%d and time=%d;",tableName, pn, queryType, dataType, tv.tv_sec);
     	 }
     	 else
     	 {
	       pSqlStr = sqlite3_mprintf("insert into %s values (%d,%d,%d,%d,?);",tableName, pn, queryType, dataType, tv.tv_sec);
	     }
       break;

     case THREE_PRESENT:             //三相智能表实时数据
     case THREE_LOCAL_CTRL_PRESENT:  //三相本地费控表实时数据
     case THREE_REMOTE_CTRL_PRESENT: //三相远程费控表实时数据
     case KEY_HOUSEHOLD_PRESENT:     //重点用户实时数据
     	 strcpy(tableName,bringTableName(timeBcdToHex(saveTime),1));
     	 tmpTime = saveTime;
	     
	     pSqlStr = sqlite3_mprintf("insert into %s values (%d,%d,%d,%d,?);",tableName, pn, queryType, dataType, tv.tv_sec);
       break;
        
     case LAST_MONTH_DATA:           //上月数据
	     pSqlStr = sqlite3_mprintf("insert into lastMonthData values (%d,%d,%d,%d,?);", pn, queryType, dataType, tv.tv_sec);
       break;

     case DAY_BALANCE:               //日结算数据
     case MONTH_BALANCE:             //月结算数据
     case THREE_DAY:
     case THREE_MONTH:
     case THREE_LOCAL_CTRL_DAY:
     case THREE_LOCAL_CTRL_MONTH:
     case THREE_REMOTE_CTRL_DAY:
     case THREE_REMOTE_CTRL_MONTH:
     case KEY_HOUSEHOLD_DAY:
     case KEY_HOUSEHOLD_MONTH:
     	 //如果是直接从电表采集到的上一次日冻结数据
       //冻结日期
     	 if (dataType==DAY_FREEZE_COPY_DATA_M || dataType==DAY_FREEZE_COPY_REQ_M || dataType==MONTH_FREEZE_COPY_DATA_M || dataType==MONTH_FREEZE_COPY_REQ_M)
     	 {
     	 	 //07表为电表冻结时间
     	 	 if (dataType==DAY_FREEZE_COPY_DATA_M || dataType==MONTH_FREEZE_COPY_DATA_M)
     	 	 {
     	 	   if (buff[DAY_FREEZE_TIME_FLAG_T]==0xee)
     	 	   {
     	 	 	   if (debugInfo&PRINT_DATA_BASE)
     	 	 	   {
     	 	 	     printf("saveMeterData:三相表日冻结时标未抄到\n");
     	 	 	   }
     	 	 	   
     	 	 	   dbLocker = 0;
     	 	 	   
     	 	 	   return FALSE;
     	 	   }
     	 	   else
     	 	   {
     	 	     tmpTime.second = 0x0;
     	 	     tmpTime.minute = buff[DAY_FREEZE_TIME_FLAG_T+0];
     	 	     tmpTime.hour   = buff[DAY_FREEZE_TIME_FLAG_T+1];
     	 	     tmpTime.day    = buff[DAY_FREEZE_TIME_FLAG_T+2];
     	 	     tmpTime.month  = buff[DAY_FREEZE_TIME_FLAG_T+3];
     	 	     tmpTime.year   = buff[DAY_FREEZE_TIME_FLAG_T+4];
     	 	   }
     	 	 }
     	 	 else   //需量时间需要另算
     	 	 {
     	     if (port==31)
     	     {
       	 	   if (copyCtrl[4].dataBuff[DAY_FREEZE_TIME_FLAG_T]==0xee)
       	 	   {
       	 	 	   if (debugInfo&PRINT_DATA_BASE)
       	 	 	   {
       	 	 	     printf("saveMeterData:三相表日冻结时标未抄到\n");
       	 	 	   }
       	 	 	   
       	 	 	   dbLocker = 0;
       	 	 	   
       	 	 	   return FALSE;
       	 	   }
       	 	   else
       	 	   {
       	 	     tmpTime.second = 0x0;
       	 	     tmpTime.minute = copyCtrl[4].dataBuff[DAY_FREEZE_TIME_FLAG_T+0];
       	 	     tmpTime.hour   = copyCtrl[4].dataBuff[DAY_FREEZE_TIME_FLAG_T+1];
       	 	     tmpTime.day    = copyCtrl[4].dataBuff[DAY_FREEZE_TIME_FLAG_T+2];
       	 	     tmpTime.month  = copyCtrl[4].dataBuff[DAY_FREEZE_TIME_FLAG_T+3];
       	 	     tmpTime.year   = copyCtrl[4].dataBuff[DAY_FREEZE_TIME_FLAG_T+4];
       	 	   }
     	     }
     	     else
     	     {
       	 	   if (copyCtrl[port-1].dataBuff[DAY_FREEZE_TIME_FLAG_T]==0xee)
       	 	   {
       	 	 	   if (debugInfo&PRINT_DATA_BASE)
       	 	 	   {
       	 	 	     printf("saveMeterData:三相表日冻结时标未抄到\n");
       	 	 	   }
       	 	 	   
       	 	 	   dbLocker = 0;
       	 	 	   
       	 	 	   return FALSE;
       	 	   }
       	 	   else
       	 	   {
       	 	     tmpTime.second = 0x0;
       	 	     tmpTime.minute = copyCtrl[port-1].dataBuff[DAY_FREEZE_TIME_FLAG_T+0];
       	 	     tmpTime.hour   = copyCtrl[port-1].dataBuff[DAY_FREEZE_TIME_FLAG_T+1];
       	 	     tmpTime.day    = copyCtrl[port-1].dataBuff[DAY_FREEZE_TIME_FLAG_T+2];
       	 	     tmpTime.month  = copyCtrl[port-1].dataBuff[DAY_FREEZE_TIME_FLAG_T+3];
       	 	     tmpTime.year   = copyCtrl[port-1].dataBuff[DAY_FREEZE_TIME_FLAG_T+4];
       	 	   }
     	     }
     	 	 }

   	 	   if (dataType==MONTH_FREEZE_COPY_DATA_M || dataType==MONTH_FREEZE_COPY_REQ_M)
   	 	   {
   	 	 	   tmpTime = timeHexToBcd(backTime(timeBcdToHex(tmpTime), 0, 1, 0, 0, 0));
   	 	   }
     	 }
     	 else
     	 {
     	 	 tmpTime = saveTime;
     	 }
     	 
     	 tmpTimex = tmpTime;
       tmpBakTimex = tmpTimex;

       if (debugInfo&PRINT_DATA_BASE)
       {
         printf("saveMeterData-三相表冻结时间:%02x-%02x-%02x %02x:%02x:%02x\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
       }

       //冻结日期
       tmpTime = timeBcdToHex(tmpTime);
       tmpTime.hour   = 0;
       tmpTime.minute = 0;
       tmpTime.second = 0;
       getLinuxFormatDateTime(&tmpTime,&tv,1);
       
       backDataType = dataType;
       
       //抄表时间
       tmpTime = timeBcdToHex(saveTime);
       getLinuxFormatDateTime(&tmpTime,&tvxx,1);
       
       if (dataType==DAY_FREEZE_COPY_DATA_M)
       {
       	 dataType = DAY_FREEZE_COPY_DATA;
       }
       
       if (dataType==DAY_FREEZE_COPY_REQ_M)
       {
       	 dataType = DAY_FREEZE_COPY_REQ;
       }

       if (dataType==MONTH_FREEZE_COPY_DATA_M)
       {
       	 dataType = MONTH_FREEZE_COPY_DATA;
       }
       
       if (dataType==MONTH_FREEZE_COPY_REQ_M)
       {
       	 dataType = MONTH_FREEZE_COPY_REQ;
       }
       if (readMeterData(tmpBuff, pn, queryType, dataType , &tmpTimex, 0) == TRUE)
       {
	        //如果原来日冻结数据的正向有功有数,而本次没有抄到数,不要更新,保持原来的数据
	        if (tmpBuff[POSITIVE_WORK_OFFSET]!=0xee && buff[POSITIVE_WORK_OFFSET]==0xee)
	        {
	        	 if (debugInfo&PRINT_DATA_BASE)
	        	 {
	        	   printf("saveMeterData:多功能电表原来日/月冻结数据的正向有功有数,而本次没有抄到数,不要更新,保持原来的数据\n");
	        	 }
	        	 
	        	 dbLocker = 0;
	        	 
	        	 return FALSE;
	        }
	        
  	      //将电表没有冻结的数据复制到buff中,造成的局面是,电表冻结的数据是直接读电表的,电表没有的数则是备份日冻结的
     	    if (backDataType==DAY_FREEZE_COPY_DATA_M || backDataType==MONTH_FREEZE_COPY_DATA_M
     	    	|| backDataType==DAY_FREEZE_COPY_REQ_M || backDataType==MONTH_FREEZE_COPY_REQ_M)  //先有备份日冻结数据,再有采集的电表日冻结数据
     	    {
  	        if (queryType==DAY_BALANCE || queryType==THREE_DAY || queryType==THREE_LOCAL_CTRL_DAY || queryType==THREE_REMOTE_CTRL_DAY || queryType==KEY_HOUSEHOLD_DAY)
  	        {
  		        if (queryType==THREE_LOCAL_CTRL_DAY && dataType==DAY_FREEZE_COPY_DATA)
  		        {
  		          tmpTime = tmpBakTimex;
  		          if (readMeterData(tmpBuff, pn, queryType, dataType, &tmpTime, 0)==TRUE)
  		          {
  	              memcpy(&buff[METER_STATUS_WORD_T], &tmpBuff[METER_STATUS_WORD_T], LENGTH_OF_THREE_LOCAL_RECORD-METER_STATUS_WORD_T);
  	            }		        	 
  		        }
  
  		        if (queryType==THREE_REMOTE_CTRL_DAY && dataType==DAY_FREEZE_COPY_DATA)
  		        {
  		          tmpTime = tmpBakTimex;
  		          if (readMeterData(tmpBuff, pn, queryType, dataType, &tmpTime, 0)==TRUE)
  		          {
  	              memcpy(&buff[METER_STATUS_WORD_T], &tmpBuff[METER_STATUS_WORD_T], LENGTH_OF_THREE_REMOTE_RECORD-METER_STATUS_WORD_T);
  	            }
  		        }
  
  		        //因07规约上面没有规定电表冻结的正向无功及反向无功最大需量的读取数据标识,所以要抄读以前先将备份
  		        //前一日冻结数据读出来,正向有功及反向有功的用抄读电表冻结的数据作补充
  		        if (dataType==DAY_FREEZE_COPY_REQ)
  		        {
  		          tmpTime = tmpBakTimex;
  		          if (readMeterData(tmpBuff, pn, queryType, dataType, &tmpTime, 0)==TRUE)
  		          {
  	              memcpy(&buff[72], &tmpBuff[72], 72);
  	              memcpy(&buff[216], &tmpBuff[216], 72);
  	            }
  	          }
  	          
  	          pSqlStr = sqlite3_mprintf("update dayBalanceData set copyTime=%d,data=? where pn=%d and freezeTime=%d and queryType=%d and dataType=%d;",
  	                tvxx.tv_sec, pn, tv.tv_sec, queryType, dataType);
  	        }
  	        else
  	        {
  		        if (queryType==THREE_LOCAL_CTRL_MONTH && dataType==MONTH_FREEZE_COPY_DATA)
  		        {
  		          tmpTime = tmpBakTimex;
  		          if (readMeterData(tmpBuff, pn, queryType, dataType, &tmpTime, 0)==TRUE)
  		          {
  	              memcpy(&buff[METER_STATUS_WORD_T], &tmpBuff[METER_STATUS_WORD_T], LENGTH_OF_THREE_LOCAL_RECORD-METER_STATUS_WORD_T);
  	            }		        	 
  		        }
  
  		        if (queryType==THREE_REMOTE_CTRL_MONTH && dataType==MONTH_FREEZE_COPY_DATA)
  		        {
  		          tmpTime = tmpBakTimex;
  		          if (readMeterData(tmpBuff, pn, queryType, dataType, &tmpTime, 0)==TRUE)
  		          {
  	              memcpy(&buff[METER_STATUS_WORD_T], &tmpBuff[METER_STATUS_WORD_T], LENGTH_OF_THREE_REMOTE_RECORD-METER_STATUS_WORD_T);
  	            }		        	 
  		        }
  		        
  		        if (dataType==MONTH_FREEZE_COPY_REQ)
  		        {
  		          tmpTime = tmpBakTimex;
  		          if (readMeterData(tmpBuff, pn, queryType, dataType, &tmpTime, 0)==TRUE)
  		          {
  	              memcpy(&buff[72], &tmpBuff[72], 72);
  	              memcpy(&buff[216], &tmpBuff[216], 72);
  	            }
  	          }
  
  	          pSqlStr = sqlite3_mprintf("update monthBalanceData set copyTime=%d,data=? where pn=%d and freezeTime=%d and queryType=%d and dataType=%d;",
  	                tvxx.tv_sec, pn, tv.tv_sec, queryType, dataType);
  	        }
  	      }
  	      else
  	      {
            tmpTime = timeBcdToHex(tmpTimex);
            
            if (debugInfo&PRINT_DATA_BASE)
            {
              printf("saveMeterData-三相表冻结时间保持为采集的电表日冻结数据时间:%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
            }
            
            getLinuxFormatDateTime(&tmpTime,&tvxx,1);

  	        //将采集的电表日冻结数据(正反有功电能示值等)复制到备份日冻结数据缓存中
  	        if (backDataType==DAY_FREEZE_COPY_DATA || backDataType==MONTH_FREEZE_COPY_DATA)
  	        {
  	          memcpy(buff, tmpBuff, 288);
            }
            else
            {
  	          memcpy(buff, tmpBuff, 72);
  	          memcpy(&buff[144], &tmpBuff[144], 72);
            }

	          if (backDataType==MONTH_FREEZE_COPY_DATA)
	          {
	            pSqlStr = sqlite3_mprintf("update monthBalanceData set copyTime=%d,data=? where pn=%d and freezeTime=%d and queryType=%d and dataType=%d;",
	                  tvxx.tv_sec, pn, tv.tv_sec, queryType, dataType);
	          }
	          else
	          {
	            pSqlStr = sqlite3_mprintf("update dayBalanceData set copyTime=%d,data=? where pn=%d and freezeTime=%d and queryType=%d and dataType=%d;",
	                  tvxx.tv_sec, pn, tv.tv_sec, queryType, dataType);
	          }
  	      }
	       
	       if (debugInfo&PRINT_DATA_BASE)
	       {
	         if (queryType==DAY_BALANCE || queryType==THREE_DAY || queryType==THREE_LOCAL_CTRL_DAY || queryType==THREE_REMOTE_CTRL_DAY || queryType==KEY_HOUSEHOLD_DAY)
	         {
	           printf("saveMeterData:多功能电表更新日冻结数据\n");
	         }
	         else
	         {
	           printf("saveMeterData:多功能电表更新月冻结数据\n");
	         }
	       }
       }
       else
       {
	        if (queryType==DAY_BALANCE || queryType==THREE_DAY || queryType==THREE_LOCAL_CTRL_DAY || queryType==THREE_REMOTE_CTRL_DAY || queryType==KEY_HOUSEHOLD_DAY)
	        {
	          pSqlStr = sqlite3_mprintf("insert into dayBalanceData values (%d,%d,%d,%d,%d,?);", pn, queryType, dataType, tv.tv_sec,tvxx.tv_sec);
	          
	        }
	        else
	        {
	          pSqlStr = sqlite3_mprintf("insert into monthBalanceData values (%d,%d,%d,%d,%d,?);", pn, queryType, dataType, tv.tv_sec,tvxx.tv_sec);
	        }
	        
	        if (debugInfo&PRINT_DATA_BASE)
	        {
	          if (queryType==DAY_BALANCE || queryType==THREE_DAY || queryType==THREE_LOCAL_CTRL_DAY || queryType==THREE_REMOTE_CTRL_DAY || queryType==KEY_HOUSEHOLD_DAY)
	          {
	            printf("saveMeterData:多功能电表插入日冻结数据\n");
	          }
	          else
	          {
	            printf("saveMeterData:多功能电表插入月冻结数据\n");
	          }
	        }
	     }
	     
	     if (queryType==DAY_BALANCE || queryType==THREE_DAY || queryType==THREE_LOCAL_CTRL_DAY || queryType==THREE_REMOTE_CTRL_DAY || queryType==KEY_HOUSEHOLD_DAY)
	     {
	       //保存日冻结数据到文件(备份数据),2012-10-10,add
	       saveBakDayFile(pn, tv.tv_sec, tvxx.tv_sec, buff, dataType);
	     }

       break;
       
     case STATIS_DATA:          //电表统计数据
	     switch(dataType)
	     {
	     	 case 88:  //三相表与时间无关量
	         pSqlStr = sqlite3_mprintf("insert into statisData values (%d,88,0,0,?);", pn);
	         break;

	     	#ifdef LIGHTING
	     	 case 89:  //线路测量点与时间无关量
	         pSqlStr = sqlite3_mprintf("insert into statisData values (%d,89,0,0,?);", pn);
	         break;
	      #endif

	     	 case 99:  //单相表与时间无关量
	         pSqlStr = sqlite3_mprintf("insert into statisData values (%d,99,0,0,?);", pn);
	         break;
	       
	       default:  //三相表与时间有关量
	         pSqlStr = sqlite3_mprintf("insert into statisData values (%d,%d,%d,%d,?);", pn, saveTime.day, saveTime.month, saveTime.year);
	         break;
	     }
     	 break;
     	 
     case SINGLE_PHASE_PRESENT:       //单相表当前数据
     case SINGLE_LOCAL_CTRL_PRESENT:  //单相本地费控表当前数据
     case SINGLE_REMOTE_CTRL_PRESENT: //单相远程费控表当前数据
     	 strcpy(tableName,bringTableName(timeBcdToHex(saveTime),0));
	     pSqlStr = sqlite3_mprintf("insert into %s values (%d,%d,?);", tableName,pn, tv.tv_sec);
       break;

     case SINGLE_PHASE_DAY:        //单相表日冻结数据
     case SINGLE_PHASE_MONTH:      //单相表月冻结数据
     case SINGLE_LOCAL_CTRL_DAY:   //单相本地费控表日冻结数据
     case SINGLE_LOCAL_CTRL_MONTH: //单相本地费控表月冻结数据
     case SINGLE_REMOTE_CTRL_DAY:  //单相远程费控表日冻结数据
     case SINGLE_REMOTE_CTRL_MONTH://单相远程控表月冻结数据
       //冻结日期
     	 if (dataType==DAY_FREEZE_COPY_DATA || dataType==MONTH_FREEZE_COPY_DATA)
     	 {
     	 	 //printf("dataType=%d\n", dataType);
     	 	 
     	 	 //07表为电表冻结时间     	 	 
   	 	   if (buff[DAY_FREEZE_TIME_FLAG_S]==0xee)
   	 	   {
   	 	 	   if (debugInfo&PRINT_DATA_BASE)
   	 	 	   {
   	 	 	     printf("saveMeterData:单相表日冻结时标未抄到\n");
   	 	 	   }
   	 	 	   
   	 	 	   dbLocker = 0;
   	 	 	   
   	 	 	   return FALSE;
   	 	   }
   	 	   else
   	 	   {
   	 	     tmpTime.second = 0x0;
   	 	     tmpTime.minute = buff[DAY_FREEZE_TIME_FLAG_S+0];
   	 	     tmpTime.hour   = buff[DAY_FREEZE_TIME_FLAG_S+1];
   	 	     tmpTime.day    = buff[DAY_FREEZE_TIME_FLAG_S+2];
   	 	     tmpTime.month  = buff[DAY_FREEZE_TIME_FLAG_S+3];
   	 	     tmpTime.year   = buff[DAY_FREEZE_TIME_FLAG_S+4];
   	 	   }
     	 	 
     	 	 if (dataType==MONTH_FREEZE_COPY_DATA)
     	 	 {
     	 	 	 tmpTime = timeHexToBcd(backTime(timeBcdToHex(tmpTime), 0, 1, 0, 0, 0));
     	 	 }
     	 }
     	 else
     	 {
     	 	 tmpTime = saveTime;
     	 }
     	 tmpTimex = tmpTime;
       tmpTime = timeBcdToHex(tmpTime);
       
       if (debugInfo&PRINT_DATA_BASE)
       {
         printf("saveMeterData-单相表冻结时间:%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
       }
       
       if (queryType==SINGLE_PHASE_MONTH || queryType==SINGLE_LOCAL_CTRL_MONTH || queryType==SINGLE_REMOTE_CTRL_MONTH)
       {
       	 tmpTime.day = 1;
       }
       tmpTime.hour   = 0;
       tmpTime.minute = 0;
       tmpTime.second = 0;
       getLinuxFormatDateTime(&tmpTime,&tv,1);
       
       //抄表时间
       tmpTime = timeBcdToHex(saveTime);
       if (debugInfo&PRINT_DATA_BASE)
       {
         printf("saveMeterData-单相表抄表时间:%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
       }
       getLinuxFormatDateTime(&tmpTime,&tvxx,1);
       
       if (readMeterData(tmpBuff, pn, queryType, ENERGY_DATA, &tmpTimex, 0) == TRUE)     //先有备份日冻结,再采集冻结数据
       {
	        //如果原来日冻结数据的正向有功有数,而本次没有抄到数,不要更新,保持原来的数据
	        if (tmpBuff[POSITIVE_WORK_OFFSET]!=0xee && buff[POSITIVE_WORK_OFFSET]==0xee)
	        {
	        	 if (debugInfo&PRINT_DATA_BASE)
	        	 {
	        	   printf("原来日/月冻结数据的正向有功有数,而本次没有抄到数,不要更新,保持原来的数据\n");
	        	 }
	        	 
	        	 dbLocker = 0;
	        	 
	        	 return FALSE;
	        }
     	    
  	      //将电表没有冻结的数据复制到buff中,造成的局面是,电表冻结的数据是直接读电表的,电表没有的数则是备份日冻结的
     	    if (dataType==DAY_FREEZE_COPY_DATA || dataType==MONTH_FREEZE_COPY_DATA)  //先有备份日冻结数据,再有采集的电表日冻结数据
     	    {
  	        switch(queryType)
  	        {
  	        	 case SINGLE_PHASE_DAY:
  	        	 case SINGLE_PHASE_MONTH:
  	        	 	 memcpy(&buff[METER_STATUS_WORD_S],&tmpBuff[METER_STATUS_WORD_S], 12);
  	        	 	 break;
  	        	 
  	        	 case SINGLE_LOCAL_CTRL_DAY:
  	        	 case SINGLE_LOCAL_CTRL_MONTH:
  	        	 	 memcpy(&buff[METER_STATUS_WORD_S],&tmpBuff[METER_STATUS_WORD_S], 50);
  	        	 	 if (debugInfo&PRINT_DATA_BASE)
  	        	 	 {
  	        	 	   printf("复制本地费控表备份日/月冻结数据\n");
  	        	 	 }
  	        	 	 break;
  
  	        	 case SINGLE_REMOTE_CTRL_DAY:
  	        	 case SINGLE_REMOTE_CTRL_MONTH:
  	        	 	 memcpy(&buff[METER_STATUS_WORD_S],&tmpBuff[METER_STATUS_WORD_S], 26);
  	        	 	 if (debugInfo&PRINT_DATA_BASE)
  	        	 	 {
  	        	 	   printf("复制远程费控表备份日/月冻结数据\n");
  	        	 	 }
  	        	 	 break;
  	        }
     	    }
     	    else           //先有采集的电表日冻结数据,再有备份日冻结
     	    {
  	        //将采集的电表日冻结数据(正反有功电能示值)复制到备份日冻结数据缓存中
  	        memcpy(buff,tmpBuff, 40);
  	        
            tmpTime = timeBcdToHex(tmpTimex);
            
            if (debugInfo&PRINT_DATA_BASE)
            {
              printf("saveMeterData-单相表冻结时间保持为采集的电表日冻结数据时间:%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
            }
                        
            getLinuxFormatDateTime(&tmpTime,&tvxx,1);
  	      }
	        
	        if (queryType==SINGLE_PHASE_DAY || queryType==SINGLE_LOCAL_CTRL_DAY || queryType==SINGLE_REMOTE_CTRL_DAY)
	        {
	          pSqlStr = sqlite3_mprintf("update singlePhaseDay set copyTime=%d,data=? where pn=%d and freezeTime=%d;",
	               tvxx.tv_sec, pn, tv.tv_sec);
	          if (debugInfo&PRINT_DATA_BASE)
	          {
	            printf("单相表:更新日冻结数据\n");
	          }
	        }
	        else
	        {
	          pSqlStr = sqlite3_mprintf("update singlePhaseMonth set copyTime=%d,data=? where pn=%d and freezeTime=%d;",
	               tvxx.tv_sec, pn, tv.tv_sec);
	          if (debugInfo&PRINT_DATA_BASE)
	          {
	            printf("单相表:更新月冻结数据\n");
	          }
	        }
       }
       else
       {
	        //表结构pSqlStr = "create table singlePhaseDay(pn int,freezeTime int, copyTime int,data blob);";
	        if (queryType==SINGLE_PHASE_DAY || queryType==SINGLE_LOCAL_CTRL_DAY || queryType==SINGLE_REMOTE_CTRL_DAY)
	        {
	          pSqlStr = sqlite3_mprintf("insert into singlePhaseDay values (%d,%d,%d,?);", pn,tv.tv_sec,tvxx.tv_sec);
	          
	          if (debugInfo&PRINT_DATA_BASE)
	          {
	            printf("插入日冻结数据\n");
	          }
	        }
	        else
	        {
	          pSqlStr = sqlite3_mprintf("insert into singlePhaseMonth values (%d,%d,%d,?);", pn,tv.tv_sec,tvxx.tv_sec);

	          if (debugInfo&PRINT_DATA_BASE)
	          {
	            printf("插入月冻结数据\n");
	          }
	        }
	     }
	      
	     if (queryType==SINGLE_PHASE_DAY || queryType==SINGLE_LOCAL_CTRL_DAY || queryType==SINGLE_REMOTE_CTRL_DAY)
	     {
	       //保存日冻结数据到文件(备份数据)
	       saveBakDayFile(pn, tv.tv_sec, tvxx.tv_sec, buff, 1);
	     }
       break;

     case LEFT_POWER:          //剩余电量
	     pSqlStr = sqlite3_mprintf("insert into leftPower values (%d,?);", pn);
     	 break;

     case DC_ANALOG:           //直流模拟量数据
	     pSqlStr = sqlite3_mprintf("insert into dcAnalog values (%d,%d,?);", pn, tv.tv_sec);
     	 break;

     case HOUR_FREEZE:         //实时抄表数据
       tmpTime.second = 0x0;
       tmpTime.minute = buff[128];
       tmpTime.hour   = buff[129];
       tmpTime.day    = buff[130];
       tmpTime.month  = buff[131];
       tmpTime.year   = buff[132];
       
       tmpTimex = tmpTime;
       if (readMeterData(tmpBuff, pn, HOUR_FREEZE, 0x0, &tmpTimex, 0) == FALSE)
       {
         if (debugInfo&PRINT_DATA_BASE)
         {
           printf("saveMeterData: 测量点%d整点冻结时间:%02x-%02x-%02x %02x:%02x:%02x\n", pn, tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute, tmpTime.second);
         }
         
         tmpTime = timeBcdToHex(tmpTime);
         getLinuxFormatDateTime(&tmpTime, &tv, 1);
       	 strcpy(tableName,bringTableName(timeBcdToHex(saveTime), 2));
  	     pSqlStr = sqlite3_mprintf("insert into %s values (%d,%d,?);", tableName, pn, tv.tv_sec);
  	   }
  	   else
  	   {
	       if (debugInfo&PRINT_DATA_BASE)
	       {
	         printf("已存在该测量点该整点的冻结数据\n");
	       }
	       
	       dbLocker = 0;
	       
	       return;
  	   }
       break;
     
     case HOUR_FREEZE_SLC:    //单灯控制器的小时冻结,存储时用的是十进制日期
       tmpTimex = saveTime;
       if (readMeterData(tmpBuff, pn, HOUR_FREEZE_SLC, 0x0, &tmpTimex, 0) == FALSE)
       {
         if (debugInfo&PRINT_DATA_BASE)
         {
           printf("saveMeterData: 控制点%d整点冻结时间:%02d-%02d-%02d %02d:%02d:%02d\n", pn, tmpTime.year, tmpTime.month, tmpTime.day, tmpTime.hour, tmpTime.minute, tmpTime.second);
         }
         
         tmpTime = saveTime;
         getLinuxFormatDateTime(&tmpTime, &tv, 1);
       	 strcpy(tableName,bringTableName(saveTime, 2));
  	     pSqlStr = sqlite3_mprintf("insert into %s values (%d,%d,?);", tableName, pn, tv.tv_sec);
  	   }
  	   else
  	   {
	       if (debugInfo&PRINT_DATA_BASE)
	       {
	         printf("saveMeterData:控制点%d该整点的冻结数据已存在\n", pn);
	       }
	       
	       dbLocker = 0;
	       
	       return;
  	   }
     	 break;
       
     default:
     	 printf("未指定存储表,queryType=0x%02x,dataType=0x%02x\n", queryType, dataType);
     	 
     	 dbLocker = 0;
     	 
     	 return FALSE;
   }

	 if (debugInfo&PRINT_DATA_BASE)
	 {
	   printf("saveMeterData:SQL=");
	   printf(pSqlStr);
	   printf("\n");
	 }
	         
   //只要缓存非空，就把其中的数据全部写入数据库
   if (buff!=NULL)
   {
	   if (pSqlStr==NULL)
	   {
	   	 printf("saveMeterData:pSqlStr指针为空,不能存储\n");
	   }
	   else
	   {
 	     sqlite3_prepare(sqlite3Db, pSqlStr, -1, &stmt, 0);
 	     sqlite3_bind_blob(stmt, 1, buff, len, NULL);
       if ((execResult=sqlite3_step(stmt))==SQLITE_DONE)
       {
         if (debugInfo&PRINT_DATA_BASE)
         {
        	 printf("saveMeterData:插入数据成功!queryType=0x%02x,dataType=0x%02x\n",queryType,dataType);
         }
        	
         //监视数据库异常次数清零,2012-09-04
         dbMonitor = 0;
       }
       else
       {
        	//处理异常
          printf("saveMeterData happen aberrant.\n");
          logRun("saveMeterData happen aberrant.");
          sqlite3Aberrant(execResult);

        	if (queryType==SINGLE_PHASE_PRESENT)
        	{
        		 //检查单相数据存储表是否存在
        		 checkSpRealTable(0);
        	}
        	
          if (queryType==PRESENT_DATA || queryType==REAL_BALANCE || queryType==LAST_REAL_BALANCE || queryType==THREE_LOCAL_CTRL_PRESENT || queryType==THREE_REMOTE_CTRL_PRESENT)
        	{
        		 //检查三相数据存储表是否存在
        		 checkSpRealTable(1);
        	}
        	
        	if (
        		  queryType==HOUR_FREEZE
        		  || queryType== HOUR_FREEZE_SLC    //照明系统的控制点整点冻结数据,2015-02-03,add this line
        		 )
        	{
        		 //检查整点冻结数据存储表是否存在
        		 checkSpRealTable(2);
        	}
        	
        	if (debugInfo&PRINT_DATA_BASE)
        	{
            fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(sqlite3Db));
        	  
        	  printf("saveMeterData:插入数据失败!queryType=0x%02x,dataType=0x%02x\n",queryType,dataType);
        	}
       }
       
       sqlite3_finalize(stmt);
     }
   }
	 
	 if (pSqlStr!=NULL)
	 {
     sqlite3_free(pSqlStr);
   }
   
   dbLocker = 0;
}

/*******************************************************
函数名称:readBakDayFile
功能描述:从文件中读取日冻结数据
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
BOOL readBakDayFile(INT16U pn, DATE_TIME *time, INT8U *buf, INT8U type)
{
	FILE           *fp;
	char           fileName[20];
  struct timeval tv;                  //Linux timeval
  DATE_TIME      tmpTime;
  SP_F_DAY       tmpSpfDay;
  MP_F_DAY       tmpMpfDay;

	//冻结数据文件名
	if (type==1)
	{
	  sprintf(fileName, "/data/spd%02x%02x%02x", time->year, time->month, time->day);
	}
	else
	{
	  sprintf(fileName, "/data/mpd%02x%02x%02x", time->year, time->month, time->day);
	}
	
  if((fp=fopen(fileName,"rb"))==NULL)
  {
  	if(debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("readBakDayFile:打开文件%s失败\n", fileName);
  	}
  	
  	return FALSE;
  }

  if (feof(fp))
  {
  	fclose(fp);

  	if(debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("readBakDayFile:文件%s空,无记录\n", fileName);
  	}

  	return FALSE;
  }
  else
  {
    if (type==1)
    {
      while (!feof(fp))
      {
      	if (fread(&tmpSpfDay, sizeof(SP_F_DAY), 1, fp)!=1)
      	{
      	  fclose(fp);
      	  return FALSE;
      	}
        
        if (tmpSpfDay.pn==pn)
        {
        	break;
        }
      }
      
      tv.tv_sec = tmpSpfDay.copyTime;
      getLinuxFormatDateTime(time, &tv, 2);    //抄表时间
      *time = timeHexToBcd(*time);
      memcpy(buf, tmpSpfDay.data, 40);         //数据
    }
    else
    {
      while (!feof(fp))
      {
      	if (fread(&tmpMpfDay, sizeof(MP_F_DAY), 1, fp)!=1)
      	{
      	  fclose(fp);
      	  return FALSE;
      	}
        
        if (tmpMpfDay.pn==pn && tmpMpfDay.type==type)
        {
        	break;
        }
      }
      
      tv.tv_sec = tmpMpfDay.copyTime;
      getLinuxFormatDateTime(time, &tv, 2);    //抄表时间
      *time = timeHexToBcd(*time);
      memcpy(buf, tmpMpfDay.data, 288);         //数据
    }
    
    if (debugInfo&PRINT_DATA_BASE)
    {
      printf("readBakDayFile:读取文件%s测量点%d日数据成功\n", fileName, pn);
    }
  }
  
  fclose(fp);
  return TRUE;
}

/*******************************************************
函数名称:readMeterData
功能描述:读取数据库中的电表数据(当前数据,上月(上一结算日)数据,结算数据)
调用函数:
被调用函数:
输入参数:*tmpData-数据缓存指针
         pn-测量点或总加组号,
         queryType--查询类型
         dataType-数据类型,
         time-查询时间
输出参数:
返回值:TRUE(查询成功) or FALSE(查询失败)
*******************************************************/
//2018-10-10,Add,上一次成功查找曲线数据缓存
INT8U     curveDataBuff[LENGTH_OF_PARA_RECORD];    //上一次成功查找到曲线数据的缓存
INT16U    curvePn=0;                               //上一次成功查找到曲线数据的Pn
INT8U     curveQueryType=0;                        //上一次成功查找到曲线数据的查询类型
INT8U     curveDataType=0;                         //上一次成功查找到曲线数据的数据类型
DATE_TIME curveTime;                               //上一次成功查找到曲线数据的查询时间
INT8U     lastCurveSuccess=0;                      //上一次是否成功查找到曲线?
BOOL readMeterData(INT8U *tmpData, INT16U pn, INT8U queryType, INT16U dataType, DATE_TIME *time, INT8U mix)
{
	  INT32U                   storeNox;
	  char                     strTableName[20];
	  sqlite3_stmt             *stmt;
	  const char               *tail;
	  char                     *pSqlStr;                //SQL语句字符串指针
	  int                      execResult;
	  INT8U                    ifFind = 0;
	  INT16U                   lenOfRecord;
	  METER_STATIS_EXTRAN_TIME *pMeterStatisExtranTime; //与时间无关的电表统计记录
	  METER_STATIS_BEARON_TIME *pMeterStatisBearonTime; //与时间有关的电表统计记录
    struct timeval           tv,tv1;                  //Linux timeval
    DATE_TIME                tmpTime,tmpTime2;
    
    pSqlStr = NULL;
		
		
    if (queryType==EVENT_RECORD)
    {
    	storeNox = dataType;
    	dataType = 0x01;
    }
    
    if (debugInfo&PRINT_DATA_BASE)
    {
			printf("readMeterData(%d-%d-%d %d:%d:%d):查询时间:%02x-%02x-%02x %02x:%02x:%02x 测量点:%d,查询类型=%02x,数据类型=%02x\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, time->year,time->month,time->day,time->hour,time->minute,time->second,pn,queryType,dataType);
    }
    
    //2012-09-05
    if (time->year==0x0 && time->month==0x00 && time->day==0x0)
    {
      printf("readMeterData(%d-%d-%d %d:%d:%d):查询时间为全0,异常调用,不查询直接退出\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
    	
    	return FALSE;
    }

		//2018-10-10,曲线缓存
		if (queryType==CURVE_DATA_PRESENT)
		{
			if (1==lastCurveSuccess)
			{
				if (
					 pn==curvePn 
					 	&& queryType==curveQueryType 
					 	 && dataType==curveDataType 
					 	  && time->year==curveTime.year
							 && time->month==curveTime.month
								&& time->day==curveTime.day
								 && time->hour==curveTime.hour
									&& time->minute==curveTime.minute
									 && time->second==curveTime.second
					 )
				{
					memcpy(tmpData, curveDataBuff, LENGTH_OF_PARA_RECORD);

					if (debugInfo&PRINT_DATA_BASE)
					{
						printf("%s(%d-%d-%d %d:%d:%d):查询时间:%02x-%02x-%02x %02x:%02x:%02x 测量点:%d,查询类型=%02x,数据类型=%02x,数据在曲线缓存中,拷贝该数据\n",__func__,sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, time->year,time->month,time->day,time->hour,time->minute,time->second,pn,queryType,dataType);
					}

					return TRUE;
				}
					 
			 	//上一次曲线数据是否查找成功置为未成功,2018-10-10
			 	lastCurveSuccess = 0;
			}
		}
		
    switch(dataType)
    {
    	 case ENERGY_DATA:
    	 case POWER_PARA_LASTMONTH:
    	 	 switch(queryType)
    	 	 {
    	 	 	 case SINGLE_PHASE_PRESENT:
    	 	 	 case CURVE_SINGLE_PHASE:
    	 	 	 case SINGLE_PHASE_DAY:
    	 	 	 case SINGLE_PHASE_MONTH:
    	 	 	   lenOfRecord = LENGTH_OF_SINGLE_ENERGY_RECORD;
    	 	 	   break;
    	 	 	   
    	 	 	 case SINGLE_LOCAL_CTRL_PRESENT:
    	 	 	 case SINGLE_LOCAL_CTRL_DAY:
    	 	 	 case SINGLE_LOCAL_CTRL_MONTH:
    	 	 	 	 lenOfRecord = LENGTH_OF_SINGLE_LOCAL_RECORD;
    	 	 	 	 break;

    	 	 	 case SINGLE_REMOTE_CTRL_PRESENT:
    	 	 	 case SINGLE_REMOTE_CTRL_DAY:
    	 	 	 case SINGLE_REMOTE_CTRL_MONTH:
    	 	 	 	 lenOfRecord = LENGTH_OF_SINGLE_REMOTE_RECORD;
    	 	 	 	 break;
    	 	 	 
    	 	 	 case THREE_PRESENT:
    	 	 	 	 lenOfRecord = LENGTH_OF_THREE_ENERGY_RECORD;
    	 	 	 	 break;

    	 	 	 case THREE_REMOTE_CTRL_PRESENT:
    	 	 	 	 lenOfRecord = LENGTH_OF_THREE_REMOTE_RECORD;
    	 	 	 	 break;

    	 	 	 case THREE_LOCAL_CTRL_PRESENT:
    	 	 	 	 lenOfRecord = LENGTH_OF_THREE_LOCAL_RECORD;
    	 	 	 	 break;
    	 	 	 	 
    	 	 	 case KEY_HOUSEHOLD_PRESENT:
    	 	 	 	 lenOfRecord = LENGTH_OF_KEY_ENERGY_RECORD;
    	 	 	 	 break;
    	 	 	 	 
    	 	 	 case CURVE_KEY_HOUSEHOLD:
    	 	 	 	 lenOfRecord = LENGTH_OF_KEY_ENERGY_RECORD;
    	 	 	 	 break;
    	 	 	   
    	 	 	 default:
    	 	    lenOfRecord = LENGTH_OF_ENERGY_RECORD;
    	 	    break;
    	 	 }
    	 	 break;
    	 	 
    	 case REQ_REQTIME_DATA:
    	 case REQ_REQTIME_LASTMONTH:
    	 	 lenOfRecord = LENGTH_OF_REQ_RECORD;
    	 	 break;
    	 	 
    	 case PARA_VARIABLE_DATA:
    	 	 if (queryType==KEY_HOUSEHOLD_PRESENT)
    	 	 {
    	 	   lenOfRecord = LENGTH_OF_KEY_PARA_RECORD;
    	 	 }
    	 	 else
    	 	 {
    	 	   lenOfRecord = LENGTH_OF_PARA_RECORD;
    	 	 }
    	 	 break;
    	 	 
    	 case SHI_DUAN_DATA:
    	 	 lenOfRecord = LENGTH_OF_SHIDUAN_RECORD;
    	 	 break;
    	 	 
    	 case REAL_BALANCE_POWER_DATA:
    	 case DAY_BALANCE_POWER_DATA:
    	 case MONTH_BALANCE_POWER_DATA:
    	 	 lenOfRecord = LEN_OF_ENERGY_BALANCE_RECORD;
    	 	 break;
    	 	 
    	 case REAL_BALANCE_PARA_DATA:
    	 case DAY_BALANCE_PARA_DATA:
    	 case MONTH_BALANCE_PARA_DATA:
    	 	 lenOfRecord = LEN_OF_PARA_BALANCE_RECORD;
    	 	 break;
    	 	 
    	 case DAY_FREEZE_COPY_DATA:
    	 case DAY_FREEZE_COPY_DATA_M:
    	 case COPY_FREEZE_COPY_DATA:
    	 case MONTH_FREEZE_COPY_DATA:
    	 case MONTH_FREEZE_COPY_DATA_M:
    	 	 switch (queryType)
    	 	 {
    	 	 	 case THREE_DAY:
    	 	 	 case THREE_MONTH:
    	 	     lenOfRecord = LENGTH_OF_THREE_ENERGY_RECORD;
    	 	 	 	 break;

    	 	 	 case THREE_LOCAL_CTRL_DAY:
    	 	 	 case THREE_LOCAL_CTRL_MONTH:
    	 	     lenOfRecord = LENGTH_OF_THREE_LOCAL_RECORD;    	 	 	 	 
    	 	 	 	 break;

    	 	 	 case THREE_REMOTE_CTRL_DAY:
    	 	 	 case THREE_REMOTE_CTRL_MONTH:
    	 	     lenOfRecord = LENGTH_OF_THREE_REMOTE_RECORD;    	 	 	 	 
    	 	 	 	 break;
    	 	 	 
    	 	 	 case KEY_HOUSEHOLD_DAY:
    	 	 	 case KEY_HOUSEHOLD_MONTH:
    	 	     lenOfRecord = LENGTH_OF_KEY_ENERGY_RECORD;
    	 	 	   break;
    	 	 	 
    	 	 	 default:
    	 	     lenOfRecord = LENGTH_OF_ENERGY_RECORD;
    	 	 }
    	 	 break;
    	 	 
    	 case DAY_FREEZE_COPY_REQ:
    	 case DAY_FREEZE_COPY_REQ_M:
    	 case COPY_FREEZE_COPY_REQ:
    	 case MONTH_FREEZE_COPY_REQ:
    	 case MONTH_FREEZE_COPY_REQ_M:
    	 	 lenOfRecord = LENGTH_OF_REQ_RECORD;
    	 	 break;
    	 	 
    	 case GROUP_REAL_BALANCE:
    	 case GROUP_DAY_BALANCE:
    	 case GROUP_MONTH_BALANCE:
    	 	 lenOfRecord = LEN_OF_ZJZ_BALANCE_RECORD;
    	 	 break;
    	 
    	 case DC_ANALOG_CURVE_DATA:
    	 	 lenOfRecord = 2;
    	 	 break;
    	 
    	 default:
    	 	 if (queryType==STATIS_DATA)
    	 	 {
					 if (pn==0)
    	 	   {
    	 	     lenOfRecord = sizeof(TERMINAL_STATIS_RECORD);
    	 	   }
    	 	   else
    	 	   {
    	 	     switch (dataType)
    	 	     {
    	 	     	 case 88:   //三相表与时间无关量
    	 	         lenOfRecord = sizeof(METER_STATIS_EXTRAN_TIME);
    	 	         break;
    	 	      
    	 	      #ifdef LIGHTING
    	 	     	 case 89:   //线路测量点与时间无关量
							   lenOfRecord = sizeof(KZQ_STATIS_EXTRAN_TIME);
    	 	         break;
    	 	      #endif

    	 	     	 case 99:   //单相表与时间无关量
    	 	         lenOfRecord = sizeof(METER_STATIS_EXTRAN_TIME_S);
    	 	         break;
    	 	         
    	 	       default:   //三相表与时间有关量
    	 	         lenOfRecord = sizeof(METER_STATIS_BEARON_TIME);
    	 	         break;
    	 	     }
    	 	   }
    	 	 }
    	 	 else
    	 	 {
      	   if (queryType==LEFT_POWER)
      	   {
      	   	 lenOfRecord = 12;
      	   }
      	   else
      	   {
      	     if (queryType==HOUR_FREEZE)
      	     {
      	   	   lenOfRecord = LENGTH_OF_HOUR_FREEZE_RECORD;
      	     }
      	     else
      	     {
      	       if (HOUR_FREEZE_SLC==queryType)    //2014-09-15,add
      	       {
      	       	 lenOfRecord = LEN_OF_LIGHTING_FREEZE;
      	       }
      	       else
      	       {
      	         if (debugInfo&PRINT_DATA_BASE)
      	         {
    	 	           printf("未知记录长度\n");
    	 	         }
    	 	         return FALSE;
    	 	       }
    	 	     }
    	 	   }
    	 	 }
    	 	 break;    	 	 
    }

    if (flagOfClearData==0x55 || flagOfClearData==0xaa)
    {
      //因为在清数据时要更新剩余电量,所以剩余电量除外
      if (queryType==LEFT_POWER && dataType==0)
      {
     	  ;
      }
      else
      {
        //if (debugInfo&PRINT_DATA_DEBUG)
        //{
          printf("readMeterData:清除数据期,禁止读取数据,queryType=0x%02x,dataType=0x%02x\n",queryType,dataType);
        //}
	    
	      memset(tmpData, 0xee, lenOfRecord);

   	    return FALSE;
   	  }
    }

    //在数据库查找
	  switch (queryType)
	  {
   	  case PRESENT_DATA:
   	  case REAL_BALANCE:
        strcpy(strTableName,bringTableName(timeBcdToHex(*time),1));
        tmpTime = timeBcdToHex(*time);
        getLinuxFormatDateTime(&tmpTime,&tv,1);
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d and time=%d",strTableName,
	              pn, queryType, dataType, tv.tv_sec);
   	  	break;

   	  case THREE_PRESENT:
   	  case THREE_LOCAL_CTRL_PRESENT:
   	  case THREE_REMOTE_CTRL_PRESENT:
   	  case KEY_HOUSEHOLD_PRESENT:
        strcpy(strTableName,bringTableName(timeBcdToHex(*time),1));
        tmpTime = timeBcdToHex(*time);
        getLinuxFormatDateTime(&tmpTime,&tv,1);
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d order by time desc limit 1",strTableName,
	              pn, queryType, dataType);
   	  	break;

      case FIRST_TODAY:
        strcpy(strTableName,bringTableName(timeBcdToHex(*time),1));
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d order by time limit 1",strTableName,
	              pn, PRESENT_DATA, dataType);
      	break;
      	
   	  case LAST_REAL_BALANCE:    //读取上一次实时结算的结果
        strcpy(strTableName,bringTableName(timeBcdToHex(*time),1));
        
        //ly,2011-10-18,add this 2 lines
        tmpTime = timeBcdToHex(*time);
        getLinuxFormatDateTime(&tmpTime,&tv,1);
	      
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d and time<=%d order by time desc limit 1",strTableName,
	              pn, REAL_BALANCE, dataType,tv.tv_sec);
      	break;
      	
      case LAST_TODAY:
        strcpy(strTableName,bringTableName(timeBcdToHex(*time),1));
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d order by time desc limit 1",strTableName,
	              pn, PRESENT_DATA, dataType);
      	break;

      case LAST_LAST_REAL_DATA:  //上上次实时数据
        tmpTime = timeBcdToHex(*time);
        //printf("tmpTime=%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
        tmpTime = backTime(tmpTime, 0, 0, 0, mix, 0);
        //printf("后退后的tmpTime=%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
        
        getLinuxFormatDateTime(&tmpTime, &tv, 1);
        strcpy(strTableName, bringTableName(tmpTime, 1));
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d and time<=%d order by time desc limit 1",strTableName,
	              pn, PRESENT_DATA, dataType, tv.tv_sec);
      	break;
      	
      case FIRST_MONTH:          //当月的第一个数据
	    	tmpTime  = timeBcdToHex(*time);
	    	tmpTime2 = tmpTime;

	    	tmpTime.day    = 1;
	    	tmpTime.hour   = 0;
	    	tmpTime.minute = 0;
	    	tmpTime.second = 0;
	    	while(tmpTime.day<=tmpTime2.day)
	    	{
           strcpy(strTableName,bringTableName(tmpTime,1));
	         pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d order by time limit 1",strTableName,
	              pn, PRESENT_DATA, dataType);
	         
	         if (debugInfo&PRINT_DATA_BASE)
	         {
	           printf("readMeterData:查找当月第一个数SQL");
	           printf(pSqlStr);
             printf("\n");
           }
           
           if (sqlite3_prepare(sqlite3Db, pSqlStr, strlen(pSqlStr), &stmt, &tail)!= SQLITE_OK)
           {
             if (debugInfo&PRINT_DATA_BASE)
             {
               fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(sqlite3Db));
             }
           }
           else
           {
             execResult = sqlite3_step(stmt);
             if(execResult==SQLITE_ROW)
             {
             	 break;
             }
           }
           
           tmpTime.day++;
	    	}
      	break;

    	case CURVE_DATA_PRESENT:
    	case CURVE_KEY_HOUSEHOLD:
   	  case CURVE_DATA_BALANCE:
			  strcpy(strTableName,bringTableName(timeBcdToHex(*time),1));
	    	tmpTime = timeBcdToHex(*time);
        getLinuxFormatDateTime(&tmpTime,&tv,1);
	    	if (debugInfo&PRINT_DATA_BASE)
	    	{
	    	  printf("三相表曲线数据查找起始时刻:%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
	    	}

        tmpTime = nextTime(timeBcdToHex(*time), bcdToHex(mix), 0);
        getLinuxFormatDateTime(&tmpTime, &tv1, 1);
	    	if (debugInfo&PRINT_DATA_BASE)
	    	{
	    	  printf("三相表曲线数据查找结束时刻:%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
	    	}
        
	      if (queryType==CURVE_DATA_PRESENT)
	      {
	       #ifdef LIGHTING
	        pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d and time>%d and time<=%d order by time desc limit 1",strTableName,
	                    pn, PRESENT_DATA, dataType, tv.tv_sec, tv1.tv_sec);
	       #else
	        pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d and time>=%d and time<%d order by time desc limit 1",strTableName,
	                    pn, PRESENT_DATA, dataType, tv.tv_sec, tv1.tv_sec);
	       #endif
	      }
	      else
	      {
	        if (queryType==CURVE_KEY_HOUSEHOLD)
	        {
	          pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d and time>=%d and time<%d order by time desc limit 1",strTableName,
	                      pn, KEY_HOUSEHOLD_PRESENT, dataType, tv.tv_sec, tv1.tv_sec);
	        }
	        else
	        {
	          pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d and time>=%d and time<%d order by time desc",strTableName,
	                      pn, REAL_BALANCE, dataType, tv.tv_sec, tv1.tv_sec);
	        }
	      }

				
    		break;

      case DC_ANALOG:    //直流模拟量曲线数据
        strcpy(strTableName,bringTableName(timeBcdToHex(*time),1));
	    	tmpTime = timeBcdToHex(*time);
        getLinuxFormatDateTime(&tmpTime,&tv,1);
        tmpTime = nextTime(timeBcdToHex(*time),bcdToHex(mix),0);
        getLinuxFormatDateTime(&tmpTime,&tv1,1);
	      pSqlStr = sqlite3_mprintf("select * from dcAnalog where pn=%d and copyTime>=%d and copyTime<%d order by copyTime desc limit 1", pn, tv.tv_sec, tv1.tv_sec);
      	break;

      case DAY_BALANCE:
      case MONTH_BALANCE:
      case THREE_DAY:
      case THREE_MONTH:
      case THREE_LOCAL_CTRL_DAY:
      case THREE_LOCAL_CTRL_MONTH:
      case THREE_REMOTE_CTRL_DAY:
      case THREE_REMOTE_CTRL_MONTH:
      case KEY_HOUSEHOLD_DAY:
      case KEY_HOUSEHOLD_MONTH:
	    	tmpTime = timeBcdToHex(*time);
	    	tmpTime.hour   = 0;
	    	tmpTime.minute = 0;
	    	tmpTime.second = 0;
        getLinuxFormatDateTime(&tmpTime, &tv, 1);
        if (queryType==DAY_BALANCE || queryType==THREE_DAY || queryType==THREE_LOCAL_CTRL_DAY || queryType==THREE_REMOTE_CTRL_DAY || queryType==KEY_HOUSEHOLD_DAY)
        {
	        if (queryType==DAY_BALANCE && dataType==MONTH_BALANCE_PARA_DATA)  //当月参变量统计数据,取当月最近一个数据
	        {
	    	    tmpTime = timeBcdToHex(*time);
	    	    tmpTime.hour   = 0;
	    	    tmpTime.minute = 0;
	    	    tmpTime.second = 0;
	    	    tmpTime.day    = 1;
            getLinuxFormatDateTime(&tmpTime, &tv, 1);
	          pSqlStr = sqlite3_mprintf("select * from dayBalanceData where pn=%d and queryType=%d and dataType=%d and freezeTime>%d order by freezeTime desc limit 1",
	                      pn, queryType, dataType, tv.tv_sec);
	        }
	        else
	        {
	           pSqlStr = sqlite3_mprintf("select * from dayBalanceData where pn=%d and queryType=%d and dataType=%d and freezeTime=%d limit 1",
	                      pn, queryType, dataType, tv.tv_sec);
	        }
      	}
      	else
      	{
	        pSqlStr = sqlite3_mprintf("select * from monthBalanceData where pn=%d and queryType=%d and dataType=%d and freezeTime=%d limit 1",
	                    pn, queryType, dataType, tv.tv_sec);
      	}
      	break;

      case LAST_MONTH_DATA:
	    	tmpTime = timeBcdToHex(*time);
	    	tmpTime.day    = 1;
	    	tmpTime.hour   = 0;
	    	tmpTime.minute = 0;
	    	tmpTime.second = 0;
        getLinuxFormatDateTime(&tmpTime,&tv,1);        
        tmpTime.day = monthDays(2000+tmpTime.year,tmpTime.month);
	    	tmpTime.hour   = 23;
	    	tmpTime.minute = 59;
	    	tmpTime.second = 59;
        getLinuxFormatDateTime(&tmpTime,&tv1,1);
        if (queryType==LAST_MONTH_DATA)
        {
        	 strcpy(strTableName,"lastMonthData");
        }
        else
        {
        	 strcpy(strTableName,"monthBalanceData");
        }
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and queryType=%d and dataType=%d and time>=%d and time<=%d order by time desc limit 1",strTableName,
	                    pn, queryType, dataType, tv.tv_sec, tv1.tv_sec);
      	break;
   	   	
	  	case EVENT_RECORD:
	      if (pn==1)
	      {
	        if (debugInfo&PRINT_EVENT_DEBUG)
	        {
	          printf("读取重要事件存储序号=%d\n",storeNox+iEventStartPtr);
	        }
	        
	        pSqlStr = sqlite3_mprintf("select * from eventRecord where storeNo=%d and eventType=%d;", storeNox+iEventStartPtr, pn);
	      }
	      else
	      {
	        if (debugInfo&PRINT_EVENT_DEBUG)
	        {
	          printf("读取一般事件存储序号=%d\n",storeNox+nEventStartPtr);
	        }
	        pSqlStr = sqlite3_mprintf("select * from eventRecord where storeNo=%d and eventType=%d;", storeNox+nEventStartPtr, pn);
	      }
	      break;
	      
	    case STATIS_DATA:
	      switch (dataType)
	      {
	      	case 88:    //三相表与时间无关的记录
	          pSqlStr = sqlite3_mprintf("select * from statisData where pn=%d and day=88;", pn);
	          break;

	       #ifdef LIGHTING
	      	case 89:    //线路测量点与时间无关的记录
	          pSqlStr = sqlite3_mprintf("select * from statisData where pn=%d and day=89;", pn);
	          break;
	       #endif

	      	case 99:    //单相表与时间无关的记录
	          pSqlStr = sqlite3_mprintf("select * from statisData where pn=%d and day=99;", pn);
	          break;
	        
	        default:    //单相表与时间无关的记录
	          pSqlStr = sqlite3_mprintf("select * from statisData where pn=%d and day=%d and month=%d and year=%d;", pn, time->day, time->month, time->year);
	          break;
	      }
	      break;
	      
	    case SINGLE_PHASE_PRESENT:
	    case SINGLE_LOCAL_CTRL_PRESENT:
	    case SINGLE_REMOTE_CTRL_PRESENT:
	      strcpy(strTableName,bringTableName(timeBcdToHex(*time),0));
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d order by time desc",strTableName,
	              pn, tv.tv_sec,tv1.tv_sec);
	    	break;

	    case CURVE_SINGLE_PHASE:
	      strcpy(strTableName,bringTableName(timeBcdToHex(*time),0));
	    	tmpTime = timeBcdToHex(*time);

	    	if (debugInfo&PRINT_DATA_BASE)
	    	{
	    	  printf("单相表曲线数据查找起始时刻:%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
	    	}
	    	
        getLinuxFormatDateTime(&tmpTime,&tv,1);
        tmpTime = nextTime(timeBcdToHex(*time),bcdToHex(mix),0);
        getLinuxFormatDateTime(&tmpTime,&tv1,1);

	    	if (debugInfo&PRINT_DATA_BASE)
	    	{
	    	  printf("单相表曲线数据查找结束时刻:%02d-%02d-%02d %02d:%02d:%02d\n",tmpTime.year,tmpTime.month,tmpTime.day,tmpTime.hour,tmpTime.minute,tmpTime.second);
	    	}
        
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and time>=%d and time<%d order by time desc",strTableName,
	              pn, tv.tv_sec, tv1.tv_sec);
	      break;

	    case SINGLE_PHASE_DAY:
	    case SINGLE_PHASE_MONTH:
	    case SINGLE_LOCAL_CTRL_DAY:
	    case SINGLE_LOCAL_CTRL_MONTH:
	    case SINGLE_REMOTE_CTRL_DAY:
	    case SINGLE_REMOTE_CTRL_MONTH:
	    	tmpTime = timeBcdToHex(*time);
	    	if (queryType==SINGLE_PHASE_MONTH || queryType==SINGLE_LOCAL_CTRL_MONTH || queryType==SINGLE_REMOTE_CTRL_MONTH)
	    	{
	    	  tmpTime.day = 1;
	    	}
	    	tmpTime.hour   = 0;
	    	tmpTime.minute = 0;
	    	tmpTime.second = 0;
        getLinuxFormatDateTime(&tmpTime,&tv,1);
        
        if (debugInfo&PRINT_DATA_BASE)
        {
          printf("readMeterData:tv.sec=%d\n", tv.tv_sec);
        }

	      if (queryType==SINGLE_PHASE_DAY || queryType==SINGLE_LOCAL_CTRL_DAY || queryType==SINGLE_REMOTE_CTRL_DAY)
	      {
	         pSqlStr = sqlite3_mprintf("select * from singlePhaseDay where pn=%d and freezeTime=%d", pn, tv.tv_sec);
	      }
	      else
	      {
           pSqlStr = sqlite3_mprintf("select * from singlePhaseMonth where pn=%d and freezeTime=%d", pn, tv.tv_sec);
	      }
	      break;
	      
	    case LEFT_POWER:       //剩余电量
        pSqlStr = sqlite3_mprintf("select * from leftPower where pn=%d", pn);
	    	break;
	    	
	    case HOUR_FREEZE:
	      strcpy(strTableName, bringTableName(timeBcdToHex(*time), 2));
	    	tmpTime = timeBcdToHex(*time);
	    	tmpTime.second = 0;
        getLinuxFormatDateTime(&tmpTime,&tv,1);
        
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and freezeTime=%d", strTableName, pn, tv.tv_sec);
	      break;

	    case HOUR_FREEZE_SLC:    //单灯控制器的小时冻结,存储时用的是十进制日期
	      strcpy(strTableName,bringTableName(*time, 2));
	    	tmpTime = *time;
	    	tmpTime.second = 0;
        getLinuxFormatDateTime(&tmpTime, &tv, 1);
        
	      pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and freezeTime=%d", strTableName, pn, tv.tv_sec);
	      break;
	      
	    default:
	      if (debugInfo&PRINT_DATA_BASE)
	      {
	        printf("读取数据 - 查询类型为:0x%02x,数据类型为0x%02x\n",queryType,dataType);
	      }
	      break;
    }
    
	  if (debugInfo&PRINT_DATA_BASE)
	  {
	    printf(pSqlStr);
	    printf("\n");
	  }
	  
	  if (pSqlStr==NULL)
	  {
      if (debugInfo&PRINT_DATA_BASE)
      {
        printf("readMeterData:pSqlStr为空指针,返回FALSE\n");
      }
      
	    memset(tmpData, 0xee, lenOfRecord);
	     
	    return FALSE;
	  }

		//gettimeofday(&tv,NULL);
		//printf("%s==>4,%ldms\n", __func__, tv.tv_sec*1000 + tv.tv_usec/1000);
		
    if ((execResult=sqlite3_prepare(sqlite3Db, pSqlStr, strlen(pSqlStr), &stmt, &tail))!= SQLITE_OK)
    {
      if (debugInfo&PRINT_DATA_BASE)
      {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(sqlite3Db));
        
        //处理异常      
        if (execResult!=101)
        {
          printf("readMeterData(1)->");
        }
      }
      
      logRun("readMeterData happen aberrant.");
      sqlite3Aberrant(execResult);
      
      if (execResult==SQLITE_ERROR)
      {
      	checkSpRealTable(0);    //单相表实时数据表
      	
      	checkSpRealTable(1);    //三相表实时数据表
      }
    }
    else
    {
      execResult = sqlite3_step(stmt);
      
      if(execResult==SQLITE_ROW)
      {
        switch(queryType)
        {
        	case EVENT_RECORD:
            lenOfRecord = sqlite3_column_int(stmt, 2);
            bzero(tmpData,lenOfRecord);
            memcpy(tmpData,sqlite3_column_blob(stmt, 3),lenOfRecord);
            break;
            
          case STATIS_DATA:
            bzero(tmpData,lenOfRecord);
            memcpy(tmpData,sqlite3_column_blob(stmt, 4),lenOfRecord);
            break;

          case LEFT_POWER:    //剩余电量
            bzero(tmpData,lenOfRecord);
            memcpy(tmpData,sqlite3_column_blob(stmt, 1),lenOfRecord);
            break;

          case DC_ANALOG:     //直流模拟量
            bzero(tmpData,lenOfRecord);
            memcpy(tmpData,sqlite3_column_blob(stmt, 2),lenOfRecord);
            break;
          
          case SINGLE_PHASE_PRESENT:
          case SINGLE_LOCAL_CTRL_PRESENT:
          case SINGLE_REMOTE_CTRL_PRESENT:
          case CURVE_SINGLE_PHASE:
            ifFind=0xee;
            while(execResult==SQLITE_ROW)
            {
              tv.tv_sec = sqlite3_column_int(stmt,1);
              getLinuxFormatDateTime(time,&tv,2);
              *time = timeHexToBcd(*time);
              bzero(tmpData,lenOfRecord);
              memcpy(tmpData,sqlite3_column_blob(stmt, 2),lenOfRecord);
              
              //正向有功电能示值总有数据
              if (tmpData[0]!=0xee)
              {
                ifFind = 2;
                break;
              }
              
              execResult = sqlite3_step(stmt);
            }
            break;
  
          case SINGLE_PHASE_DAY:
          case SINGLE_PHASE_MONTH:
          case SINGLE_LOCAL_CTRL_DAY:
          case SINGLE_LOCAL_CTRL_MONTH:
          case SINGLE_REMOTE_CTRL_DAY:
          case SINGLE_REMOTE_CTRL_MONTH:
            tv.tv_sec = sqlite3_column_int(stmt,2);
            getLinuxFormatDateTime(time,&tv,2);
            *time = timeHexToBcd(*time);
          	
            bzero(tmpData,lenOfRecord);
            memcpy(tmpData,sqlite3_column_blob(stmt, 3),lenOfRecord);
            break;
            
          //case LAST_LAST_REAL_DATA:  //上上次实时数据
            // tv.tv_sec = sqlite3_column_int(stmt,3);
            // getLinuxFormatDateTime(time,&tv,2);
            // *time = timeHexToBcd(*time);
            // bzero(tmpData,lenOfRecord);
            // memcpy(tmpData,sqlite3_column_blob(stmt, 4),lenOfRecord);
            
            //#else
            // tmpTime = timeBcdToHex(*time);
            // getLinuxFormatDateTime(&tmpTime,&tv,1);
            // ifFind = 0xee;
            // while(execResult==SQLITE_ROW)
            // {
            //   tv1.tv_sec = sqlite3_column_int(stmt,3);
            //  
            //   if (tv1.tv_sec<tv.tv_sec)
            //   {
            //     getLinuxFormatDateTime(time,&tv1,2);
            //     *time = timeHexToBcd(*time);
            //     bzero(tmpData,lenOfRecord);
            //     memcpy(tmpData,sqlite3_column_blob(stmt, 4),lenOfRecord);
            //     ifFind = 1;
            //     break;
            //   }
            //   execResult = sqlite3_step(stmt);
            // }
            //#endif
          	break;
          	
   	      case DAY_BALANCE:
   	      case MONTH_BALANCE:
   	      case THREE_DAY:
   	      case THREE_MONTH:
   	      case THREE_LOCAL_CTRL_DAY:
   	      case THREE_LOCAL_CTRL_MONTH:
   	      case THREE_REMOTE_CTRL_DAY:
   	      case THREE_REMOTE_CTRL_MONTH:
   	      case KEY_HOUSEHOLD_DAY:
   	      case KEY_HOUSEHOLD_MONTH:
            //pSqlStr = "create table dayBalanceData(pn int,queryType int,dataType int,freezeTime int, copyTime int,data blob);";
            tv.tv_sec = sqlite3_column_int(stmt,4);
            getLinuxFormatDateTime(time,&tv,2);
            *time = timeHexToBcd(*time);
            
            bzero(tmpData,lenOfRecord);
            memcpy(tmpData,sqlite3_column_blob(stmt, 5),lenOfRecord);
   	      	break;
   	      	
    	    case CURVE_DATA_PRESENT:
   	      case PRESENT_DATA:
          case LAST_TODAY:
          case FIRST_TODAY:
          case FIRST_MONTH:          //当月的第一个数据
   	      case REAL_BALANCE:
   	      case CURVE_DATA_BALANCE:
   	      case LAST_REAL_BALANCE:    //读取上一次实时结算的结果
          case LAST_LAST_REAL_DATA:  //上上次实时数据
   	      case LAST_MONTH_DATA:
   	      case THREE_PRESENT:
   	      case THREE_LOCAL_CTRL_PRESENT:
   	      case THREE_REMOTE_CTRL_PRESENT:
   	      case KEY_HOUSEHOLD_PRESENT:
   	      case CURVE_KEY_HOUSEHOLD:
						
            bzero(tmpData,lenOfRecord);
            memcpy(tmpData,sqlite3_column_blob(stmt, 4),lenOfRecord);

						if (CURVE_DATA_PRESENT==queryType && PARA_VARIABLE_DATA==dataType)
						{
							memcpy(curveDataBuff, tmpData, lenOfRecord);
							curvePn = pn;
							curveQueryType = queryType;
							curveDataType = dataType;
							curveTime = *time;
							lastCurveSuccess = 1;
						}
						
						tv.tv_sec = sqlite3_column_int(stmt,3);
						getLinuxFormatDateTime(time,&tv,2);
						*time = timeHexToBcd(*time);
   	      	break;

   	      case HOUR_FREEZE:
            tv.tv_sec = sqlite3_column_int(stmt,1);
            getLinuxFormatDateTime(time,&tv,2);
            *time = timeHexToBcd(*time);
            bzero(tmpData, lenOfRecord);
            memcpy(tmpData, sqlite3_column_blob(stmt, 2), lenOfRecord);
   	      	break;

   	      case HOUR_FREEZE_SLC:
            tv.tv_sec = sqlite3_column_int(stmt,1);
            getLinuxFormatDateTime(time,&tv,2);
            bzero(tmpData, lenOfRecord);
            memcpy(tmpData, sqlite3_column_blob(stmt, 2), lenOfRecord);
   	      	break;
            
          default:
            bzero(tmpData,lenOfRecord);
            memcpy(tmpData,sqlite3_column_blob(stmt, 2),lenOfRecord);
            break;
        }
        
        if (ifFind==0xee)
        {
        	ifFind = 0;
        }
        else
        {
          ifFind = 1;
        }
      }
      else
      {
        //处理异常
        if (execResult!=SQLITE_DONE)
        {
          logRun("readMeterData(2) happen aberrant.");
          
          if (debugInfo&PRINT_DATA_BASE)
          {
            printf("readMeterData(2) happen aberrant.\n");
          }
        }
        
        sqlite3Aberrant(execResult);
      }
  	  
  	  sqlite3_finalize(stmt);
  	}

	  sqlite3_free(pSqlStr);

    //根据查询结果处理返回数据
    if (ifFind == 0)
    {
	     if (debugInfo&PRINT_DATA_BASE)
	     {
	       printf("readMeterData(%d-%d-%d %d:%d:%d):未找到数据:pn=%d,查询类型为:0x%02x,数据类型为0x%02x,长度=%d\n\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, pn,queryType,dataType,lenOfRecord);
	     }
	     
	     memset(tmpData, 0xee, lenOfRecord);
	     
	     return FALSE;
    }
    else
    { 
      if (debugInfo&PRINT_DATA_BASE)
      {
        printf("readMeterData(%d-%d-%d %d:%d:%d):找到数据:pn=%d,查询类型为:0x%0x,数据类型为0x%02x,数据时间:%02x-%02x-%02x %02x:%02x:%02x,长度=%d\n\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,pn,queryType,dataType,time->year,time->month,time->day,time->hour,time->minute,time->second,lenOfRecord);
      }
    }
		
		//gettimeofday(&tv,NULL);
		//printf("%s==>5,%ldms\n", __func__, tv.tv_sec*1000 + tv.tv_usec/1000);

    return TRUE;
}

/*******************************************************
函数名称:writeEvent
功能描述:写事件记录入FLASH
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:TRUE or FALSE
*******************************************************/
BOOL writeEvent(INT8U *data,INT8U length,INT8U type, INT8U dataFromType)
{
   INT8U                  tmpErc;
	 sqlite3_stmt           *stmt;
	 const char             *tail;
   char                   *pSqlStr;              //SQL语句字符串指针   
	 int                    execResult;
	 INT32U                 recordNo;
   TERMINAL_STATIS_RECORD terminalStatisRecord;  //终端统计记录
   DATE_TIME              tmpTime;
   char                   *errMessage;           //错误消息(Error msg written her)

   if (flagOfClearData==0x55 || flagOfClearData==0xaa)
   {
      //if (debugInfo&PRINT_DATA_DEBUG)
      //{
        printf("writeEvent:清除数据期,禁止写入数据\n");
      //}
     
   	  return FALSE;
   }


 	 recordNo = queryEventStoreNo(type);
     	
   tmpErc = *data;

   recordNo++;
   if (debugInfo&PRINT_EVENT_DEBUG)
   {
     printf("writeEvent:事件记录序号=%d,类型=%d,ERC=%d\n",recordNo,type,tmpErc);
   }
	 
	 pSqlStr = sqlite3_mprintf("insert into eventRecord values (%d,%d,%d,?);", recordNo, type, length);
	 sqlite3_prepare(sqlite3Db, pSqlStr, -1, &stmt, 0);
	 sqlite3_bind_blob(stmt, 1, data, length, NULL);
   if (sqlite3_step(stmt)==SQLITE_DONE)
   {
     if (debugInfo&PRINT_EVENT_DEBUG)
     {
       printf("writeEvent:插入数据成功!\n");
     }
   }
   else
   {
     if (debugInfo&PRINT_EVENT_DEBUG)
     {
       printf("writeEvent:插入数据失败!\n");
     }
   }
   sqlite3_finalize(stmt);

   //调整事件计数器值
   if (type == 1)
   {
  	 if (recordNo>255)
  	 {
	     iEventStartPtr++;
	     pSqlStr = sqlite3_mprintf("delete from eventRecord where eventType=1 and storeNo<%d",iEventStartPtr);
       if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
       {
   	      if (debugInfo&PRINT_EVENT_DEBUG)
   	      {
   	        printf("writeEvent:删除多余的重要事件记录成功!\n");
   	      }
       }
       sqlite3_free(pSqlStr);
         	   
  	   iEventCounter = 255;
  	   eventReadedPointer[0] = 255;
  	   if (eventReadedPointer[1]>0)
  	   {
  	     eventReadedPointer[1]--;
  	   }
  	   saveParameter(88, 2,&eventReadedPointer, 2);
  	 }
  	 else
  	 {
  	   iEventCounter = recordNo;
  	 }
  	 
  	 if (debugInfo&PRINT_EVENT_DEBUG)
  	 {
  	   printf("writeEvent:重要事件计数器值=%d,iEventStartPtr=%d\n",iEventCounter,iEventStartPtr);
  	 }
   }
   else
   {
  	 if (recordNo>255)
  	 {
	     nEventStartPtr++;
	     pSqlStr = sqlite3_mprintf("delete from eventRecord where eventType=2 and storeNo<%d",nEventStartPtr);
       if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
       {
   	      if (debugInfo&PRINT_EVENT_DEBUG)
   	      {
   	        printf("writeEvent:删除多余的一般事件记录成功!\n");
   	      }
       }
       sqlite3_free(pSqlStr);
       
  	   nEventCounter = 255;
  	 }
  	 else
  	 {
  	   nEventCounter = recordNo;  	 	  
  	 }

  	 if (debugInfo&PRINT_EVENT_DEBUG)
  	 {
  	   printf("writeEvent:一般事件计数器值=%d\n", nEventCounter);
  	 }
   }

   //LCD闪烁显示有事件发生
   aberrantAlarm.eventNum     = tmpErc;
   aberrantAlarm.timeOut      = nextTime(sysTime, 0, 20);
   aberrantAlarm.aberrantFlag = 1;
   
   return TRUE;
}

/*******************************************************
函数名称:bringTableName
功能描述:产生表实时数据表名
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
char * bringTableName(DATE_TIME time,INT8U type)
{
  char strTableName[3][3]={"sp","mp","hf"};
  char tableName[30];
  char str[10];
    
  if (type>2)
  {
  	type = 0;
  }
  
  strcpy(tableName,strTableName[type]);
  strcat(tableName,digital2ToString(time.year,str));
  strcat(tableName,digital2ToString(time.month,str));
  strcat(tableName,digital2ToString(time.day,str));
  
  return tableName;
}

/*******************************************************
函数名称:checkSpRealTable
功能描述:检查单相表实时数据表是否存在,如不存在则建立
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void checkSpRealTable(INT8U type)
{ 
	char    *pSqlStr;                  //SQL语句字符串指针
  char    *errMessage;               //错误消息(Error msg written her)
  char    tableName[20];             //临时表名
  char    str[10];
  
	strcpy(tableName, bringTableName(sysTime,type));
	pSqlStr = sqlite3_mprintf("select * from  %s  limit 1;",tableName);
	
  if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)   //查询成功
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("数据表%s已经建立\n",tableName);
  	}
  }
  else
  {
	  switch (type)
	  {
	    case 1:    //三相表实时数据表
	      pSqlStr = sqlite3_mprintf("create table %s(pn int,queryType int,dataType int,time int,data blob);",tableName);
	      break;

	    case 2:    //整点冻结数据表
	      pSqlStr = sqlite3_mprintf("create table %s(pn int,freezeTime int,data blob);",tableName);
	      break;
	     	
	    default:   //单相表实时数据表
	      pSqlStr = sqlite3_mprintf("create table %s(pn int,time int,data blob);",tableName);
	      break;
	  }

    if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
    {
     	if (debugInfo&PRINT_DATA_BASE)
     	{
     	  printf("创建表%s成功\n",tableName);
     	}
    }
    else
    {
     	if (debugInfo&PRINT_DATA_BASE)
     	{
     	  printf("创建表%s失败\n",tableName);
     	}
     	 
     	sqlite3_free(errMessage);
    }
  }
}

/*******************************************************
函数名称:threadOfClearData
功能描述:清理数据线程(过期数据或主站下发清除数据)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void threadOfClearData(void *arg)
{ 
   sqlite3      *sqlite3Dbx;
   DATE_TIME    tmpTime;
   char         tableName[30];
   INT16U       i;
   char         *pSqlStr;              //SQL语句字符串指针
   char         *errMessage;           //错误消息(Error msg written her)
   INT8U        ifDel;
   INT8U        tmpNumOfZjz;
   INT16U       pn;
   INT8U        eventData[18];

   DIR           *dp;          //2012-10-10,add
   struct dirent *filename;    //2012-10-10,add
   char          tmpFileName[30];

   while(1)
   {
     //1.清除过期数据(复位后做一次)
     if (flagOfClearData==0x99)
     {
       if (debugInfo&PRINT_DATA_BASE)
       {
         printf("清理过期数据开始\n");
       }
     
       sleep(62);    //2014-09-26,从32秒改成62秒

       flagOfClearData = 0x0;
       
       sqlite3_close(sqlite3Db);
    
       if (sqlite3_open("powerData.db", &sqlite3Dbx))
       {
         printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Dbx));
         sqlite3_close(sqlite3Dbx);
         
         //ly,2011-10-14,add,打不开新的全局指针,还原为原全局指针
         usleep(2000);
         if (sqlite3_open("powerData.db", &sqlite3Db))
         {
           printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
           sqlite3_close(sqlite3Db);
       
           usleep(2000);
           if (sqlite3_open("powerData.db", &sqlite3Db))
           {
             printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
             sqlite3_close(sqlite3Db);
           }
         }

         continue;
       }
    
       //1.三相表/单相表实时抄表数据
      #ifdef SUPPORT_ETH_COPY
       //2014-09-26,松藻以太网口抄表终端由于抄表速度快, 数据库急增到240M以上了,导致系统崩溃了
       //    修改为只存3天的实时和结算数据,且每天凌晨的时候做一次清理
       tmpTime = backTime(sysTime, 0, 3, 0, 0, 0);
      #else
       //tmpTime = backTime(sysTime, 0, 14, 0, 0, 0);
       //2020-11-19,从14天修改为5天
       tmpTime = backTime(sysTime, 0, 5, 0, 0, 0);
      #endif
       for(i=0;i<10;i++)
       {
       	  tmpTime = backTime(tmpTime, 0, 1, 0, 0, 0);
          
          //三相表
    	    strcpy(tableName, bringTableName(tmpTime,1));
    	    pSqlStr = sqlite3_mprintf("drop table %s",tableName);
          if (sqlite3_exec(sqlite3Dbx, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
          {
       	    if (debugInfo&PRINT_DATA_BASE)
       	    {
       	      printf("删除表%s成功\n",tableName);
       	    }
          }
          else
          {
       	    if (debugInfo&PRINT_DATA_BASE)
       	    {
       	      printf("删除表%s失败.Error:%s\n",tableName,sqlite3_errmsg(sqlite3Dbx));
       	    }
          }
          sqlite3_free(pSqlStr);
          
          //载波/单相表
    	    strcpy(tableName, bringTableName(tmpTime,0));
    	    pSqlStr = sqlite3_mprintf("drop table %s",tableName);
    	    
          if (sqlite3_exec(sqlite3Dbx, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
          {
       	    if (debugInfo&PRINT_DATA_BASE)
       	    {
       	      printf("删除表%s成功\n",tableName);
       	    }
          }
          else
          {
       	    if (debugInfo&PRINT_DATA_BASE)
       	    {
       	      printf("删除表%s失败.Error:%s\n",tableName,sqlite3_errmsg(sqlite3Dbx));
       	    }
          }
          sqlite3_free(pSqlStr);
          
          //整点冻结表
    	    strcpy(tableName, bringTableName(tmpTime,2));
    	    pSqlStr = sqlite3_mprintf("drop table %s",tableName);
    	    
          if (sqlite3_exec(sqlite3Dbx, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
          {
       	    if (debugInfo&PRINT_DATA_BASE)
       	    {
       	      printf("删除表%s成功\n",tableName);
       	    }
          }
          else
          {
       	    if (debugInfo&PRINT_DATA_BASE)
       	    {
       	      printf("删除表%s失败.Error:%s\n",tableName,sqlite3_errmsg(sqlite3Dbx));
       	    }
          }
          sqlite3_free(pSqlStr);
          
          //照明日数据表
    	   #ifdef LIGHTING
    	    sprintf(tableName, "slday%02d%02d%02d", tmpTime.year, tmpTime.month, tmpTime.day);
    	    pSqlStr = sqlite3_mprintf("drop table %s",tableName);
    	    
          if (sqlite3_exec(sqlite3Dbx, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
          {
       	    if (debugInfo&PRINT_DATA_BASE)
       	    {
       	      printf("删除表%s成功\n",tableName);
       	    }
          }
          else
          {
       	    if (debugInfo&PRINT_DATA_BASE)
       	    {
       	      printf("删除表%s失败.Error:%s\n",tableName,sqlite3_errmsg(sqlite3Dbx));
       	    }
          }
          sqlite3_free(pSqlStr);
         #endif
       }
       sqlite3_close(sqlite3Dbx);
    
       if (sqlite3_open("powerData.db", &sqlite3Db))
       {
         printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
         sqlite3_close(sqlite3Db);
         //ly,2011-10-14,add,打不开新的全局指针,还原为原全局指针
         usleep(2000);
         if (sqlite3_open("powerData.db", &sqlite3Db))
         {
           printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
           sqlite3_close(sqlite3Db);
       
           usleep(2000);
           if (sqlite3_open("powerData.db", &sqlite3Db))
           {
             printf("Can't open database: %s\n", sqlite3_errmsg(sqlite3Db));
             sqlite3_close(sqlite3Db);
           }
         }
         
         continue;
       }
    
       //2.三相表上月/上一结算日数据 lastMonthData(保存近1月(31天)的数据)
       clearOutData("lastMonthData", LAST_MONTH_DATA, 31, 1);
    
       //3.三相表日结算数据 dayBalanceData (保存近31天的数据)
       clearOutData("dayBalanceData", DAY_BALANCE, 31, 1);
    
       //4.三相表月结算数据 monthBalanceData (保存近12个月130天的数据)
       clearOutData("monthBalanceData", MONTH_BALANCE, 12, 1);
    
       //5.统计统计数据(保存近31天的数据)
       clearOutData("statisData", 2, 31, 0);
       
       //6.单相表日冻结数据singlePhaseDay(保存近31天的数据)
       clearOutData("singlePhaseDay", 0, 31, 0);
          
       //7.单相表月冻结数据singlePhaseMonth(保存近12个月的数据)
       clearOutData("singlePhaseMonth", 1, 12, 0);
    
       //8.直流模拟量数据dcAnalog(保存近14天的数据)
       clearOutData("dcAnalog", 0, 14, 0);

       //2012-10-10,add,删除过期日冻结文件
       dp = opendir("/data/");
       if (!dp)
       {
         fprintf(stderr,"open directory error\n");
       }
       else
       {
         while (filename=readdir(dp))
         {
           if (strstr(filename->d_name,"mpd"))
           {
             tmpTime = backTime(sysTime, 0, 31, 0, 0, 0);
             sprintf(tmpFileName, "mpd%02d%02d%02d", tmpTime.year, tmpTime.month, tmpTime.day);
             
             if (strcmp(filename->d_name, tmpFileName)<0)
             {
               sprintf(tmpFileName,"rm /data/%s",filename->d_name);
               if (system(tmpFileName)==0)
               {
             	   printf("文件%s删除成功\n", filename->d_name);
               }
               else
               {
             	   printf("文件%s删除失败\n", filename->d_name);
             	 }

             }
             else
             {
             	 printf("%s不应该删除\n", filename->d_name);             	 
             }
           }

           if (strstr(filename->d_name,"spd"))
           {
             tmpTime = backTime(sysTime, 0, 31, 0, 0, 0);
             sprintf(tmpFileName, "spd%02d%02d%02d", tmpTime.year, tmpTime.month, tmpTime.day);
             
             if (strcmp(filename->d_name, tmpFileName)<0)
             {
               sprintf(tmpFileName,"rm /data/%s",filename->d_name);
               if (system(tmpFileName)==0)
               {
             	   printf("文件%s删除成功\n", filename->d_name);
               }
               else
               {
             	   printf("文件%s删除失败\n", filename->d_name);
             	 }
             }
             else
             {
             	 printf("%s不应该删除\n", filename->d_name);             	 
             }
           }
         }
         closedir(dp);
       }
  
       if (debugInfo&PRINT_DATA_BASE)
       {
         printf("清理过期数据完成\n");
       }
     }
   
     //2.清除数据/参数(按需)
     if (flagOfClearData==0x55 || flagOfClearData==0xaa)
     {
       if (flagOfClearData==0xaa)
       {
         ifDel = 1;
       }
       else
       {
         ifDel = 0;
       }
     
       if (ifDel==1)
       {
         //专变终端,解除控制
         #ifndef PLUG_IN_CARRIER_MODULE
          //1.保电解除
          if (staySupportStatus.ifStaySupport==CTRL_JUMP_IN)
          {
          	staySupportStatus.ifStaySupport = CTRL_RELEASE;  //保电已解除
          }
          
          //2.剔除解除
          if (toEliminate==CTRL_JUMP_IN)
          {
          	   toEliminate = CTRL_RELEASE;    //剔除解除
          }
          
          //3.催费告警解除 
          if (reminderFee==CTRL_JUMP_IN)
          {
          	reminderFee = CTRL_RELEASE;
          }
    
          //4.遥控解除
          for (i=0; i<CONTROL_OUTPUT; i++)
          {
          	if (remoteCtrlConfig[i].ifUseRemoteCtrl==CTRL_JUMP_IN)
          	{
          		 remoteCtrlConfig[i].remoteEnd = sysTime;   //遥控解除
          	}
          }
          
          //5.功控和电控解除
       	  for (tmpNumOfZjz = 0; tmpNumOfZjz<totalAddGroup.numberOfzjz; tmpNumOfZjz++)
       	  {
       	    pn = totalAddGroup.perZjz[tmpNumOfZjz].zjzNo;
       	    
       	    //4.1 解除月电量控
           	if (ctrlRunStatus[pn-1].ifUseMthCtrl == CTRL_JUMP_IN)
       	    {
       	    	ctrlRunStatus[pn-1].ifUseMthCtrl = CTRL_RELEASE;
       	    }
       	    
       	    //4.2 解除购电控
       	    if (ctrlRunStatus[pn-1].ifUseChgCtrl == CTRL_JUMP_IN)
       	    {
       	    	ctrlRunStatus[pn-1].ifUseChgCtrl = CTRL_RELEASE;
       	    }
       	    
       	    //4.3 解除下浮控
           	if (ctrlRunStatus[pn-1].ifUsePwrCtrl == CTRL_JUMP_IN)
           	{
           		ctrlRunStatus[pn-1].ifUsePwrCtrl = CTRL_RELEASE;
           	}
           	
       	    //4.4 解除报停控
           	if (ctrlRunStatus[pn-1].ifUseObsCtrl == CTRL_JUMP_IN)
           	{
           		ctrlRunStatus[pn-1].ifUseObsCtrl = CTRL_RELEASE;
           	}
    
       	    //4.5 解除厂休控
           	if (ctrlRunStatus[pn-1].ifUseWkdCtrl == CTRL_JUMP_IN)
           	{
           		ctrlRunStatus[pn-1].ifUseWkdCtrl = CTRL_RELEASE;
           	}
    
       	    //4.6 解除时段控
           	if (ctrlRunStatus[pn-1].ifUsePrdCtrl == CTRL_JUMP_IN)
           	{
           		ctrlRunStatus[pn-1].ifUsePrdCtrl = CTRL_RELEASE;
           	}
       	  }
          
          sleep(2);
         #endif
         
         #ifdef PRINT_DATA_BASE
          printf("删除数据线程:数据及参数\n");
         #endif
         
         //删除数据及参数
    	   deleteData(1);

         //准备事件数据
         if ((eventRecordConfig.iEvent[0] & 0x01) || (eventRecordConfig.nEvent[0] & 0x01))
         {
         	  eventData[0] = 0x1;
         	  eventData[1] = 0x12;
         	  eventData[2] = sysTime.second/10<<4 | sysTime.second%10;
         	  eventData[3] = sysTime.minute/10<<4 | sysTime.minute%10;
         	  eventData[4] = sysTime.hour/10<<4 | sysTime.hour%10;
         	  eventData[5] = sysTime.day/10<<4 | sysTime.day%10;
         	  eventData[6] = sysTime.month/10<<4 | sysTime.month%10;
         	  eventData[7] = sysTime.year/10<<4 | sysTime.year%10;
         	  eventData[8] = 0x1;
         	  eventData[9] = 0x0;
         	  eventData[10] = 0x0;
         	  eventData[11] = 0x0;
         	  eventData[12] = 0x0;
         	  eventData[13] = 0x0;
         	  eventData[14] = 0x0;
         	  eventData[15] = 0x0;
         	  eventData[16] = 0x0;
         	  eventData[17] = 0x0;
         }
    	 }
    	 else
    	 {
         #ifdef PRINT_DATA_BASE
          printf("删除数据线程:清除数据区\n");
         #endif
         
         //清除数据区 
         deleteData(0);
    	 }
       
       //硬件复位动作
       #ifdef PLUG_IN_CARRIER_MODULE
        flagOfClearData = 0x0;

        if (ifDel==1)
        {
          if (eventRecordConfig.iEvent[0] & 0x01)
          {
         	   writeEvent(eventData, 18, 1, DATA_FROM_GPRS);  //记入重要事件队列
         	}
          if (eventRecordConfig.nEvent[0] & 0x01)
          {
         	  writeEvent(eventData, 18, 2, DATA_FROM_GPRS);  //记入一般事件队列
         	}

         	eventStatus[0] = eventStatus[0] | 0x01;
        }

        cmdReset = 1;               //等待2秒后复位
       #else
        printf("删除数据线程:专变终端不复位重新装参数\n");
        
        //专变终端清参数后不复位,需要将参数重新装入参数
        loadParameter();
        fillPulseVar(1);
    
        //专变终端脉冲禁止标志复位
        if (flagOfClearPulse==0x55 || flagOfClearPulse==0xaa)
        {
       	  flagOfClearPulse = 0x0;
        }

        if (ifDel==1)
        {
          if (eventRecordConfig.iEvent[0] & 0x01)
          {
         	   writeEvent(eventData, 18, 1, DATA_FROM_GPRS);  //记入重要事件队列
         	}
          if (eventRecordConfig.nEvent[0] & 0x01)
          {
         	  writeEvent(eventData, 18, 2, DATA_FROM_GPRS);  //记入一般事件队列
         	}
         	  
         	eventStatus[0] = eventStatus[0] | 0x01;
        }
       #endif
       
       flagOfClearData = 0x0;
     }
     
     usleep(500000);
   }
}

/************************************************
函数名称:logRun
描述:
调用函数:
被调用:
输入参数:
输出参数:
返回值:
**************************************************/
void logRun(char *str)
{
	FILE *fp,*fpout;
  char timeStr[20];
  
  if((fp=fopen("/runLog","a+"))==NULL)
  {
  	return;
  }	
 	sprintf(timeStr,"%02d-%02d-%02d %02d:%02d:%02d,",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
 	fputs(timeStr,fp);
 	fputs(str,fp);
 	fputs("\n",fp);

  fclose(fp);
}

/************************************************
函数名称:readIpMaskGateway
描述:读取rcS文件中的IP地址、掩码及默认地址
调用函数:
被调用:
输入参数:
输出参数:
返回值:
**************************************************/
void readIpMaskGateway(unsigned char ip[4],unsigned char mask[4],unsigned char gw[4])
{
  FILE *fpOfRcs;
  int  i;
  unsigned char ipPtr, j;
  char say[100];  

  if ((fpOfRcs=fopen("/etc/init.d/rcS","r+"))==NULL)
  {
  	printf("Can't open rcS\n");  	
  }
  
  //查找IP地址及掩码
  i=0;
  while(i<50)
  {
    i++;
    fgets(say, 100, fpOfRcs);
    //printf("%s\n", say);
    
    if (strstr(say,"netmask"))
    {
    	//IP地址
      ipPtr = 14;
  	  for(j=0;j<4;j++)
  	  {
    	  if (j<3)
    	  {
      	  if (say[ipPtr+1]==0x2e)    //只有一位数
      	  {
      	     ip[j] = say[ipPtr]-0x30;
      	     ipPtr+=2;
      	  }
      	  else
      	  {
         	   if (say[ipPtr+2]==0x2e)    //只有二位数
        	   {
      	        ip[j] = (say[ipPtr]-0x30)*10 + say[ipPtr+1]-0x30;
      	        ipPtr+=3;
      	     }
      	     else
      	     {
      	        ip[j] = (say[ipPtr]-0x30)*100 + (say[ipPtr+1]-0x30)*10 + say[ipPtr+2]-0x30;
      	        ipPtr+=4;
      	     }
      	  }
      	}
      	else
      	{
      	  if (say[ipPtr+1]==0x20)    //只有一位数
      	  {
      	     ip[j] = say[ipPtr]-0x30;
      	     ipPtr+=2;
      	  }
      	  else
      	  {
         	   if (say[ipPtr+2]==0x20)    //只有二位数
        	   {
      	        ip[j] = (say[ipPtr]-0x30)*10 + say[ipPtr+1]-0x30;
      	        ipPtr+=3;
      	     }
      	     else
      	     {
      	        ip[j] = (say[ipPtr]-0x30)*100 + (say[ipPtr+1]-0x30)*10 + say[ipPtr+2]-0x30;
      	        ipPtr+=4;
      	     }
      	  }
      	}
    	}
    	
    	ipPtr+=8;
    	//IP地址掩码
  	  for(j=0;j<4;j++)
  	  {
    	  if (j<3)
    	  {
      	  if (say[ipPtr+1]==0x2e)    //只有一位数
      	  {
      	     mask[j] = say[ipPtr]-0x30;
      	     ipPtr+=2;
      	  }
      	  else
      	  {
         	   if (say[ipPtr+2]==0x2e)    //只有二位数
        	   {
      	        mask[j] = (say[ipPtr]-0x30)*10 + say[ipPtr+1]-0x30;
      	        ipPtr+=3;
      	     }
      	     else
      	     {
      	        mask[j] = (say[ipPtr]-0x30)*100 + (say[ipPtr+1]-0x30)*10 + say[ipPtr+2]-0x30;
      	        ipPtr+=4;
      	     }
      	  }
      	}
      	else
      	{
      	  if (say[ipPtr+1]==0x20)    //只有一位数
      	  {
      	     mask[j] = say[ipPtr]-0x30;
      	     ipPtr+=2;
      	  }
      	  else
      	  {
         	   if (say[ipPtr+2]==0x20)    //只有二位数
        	   {
      	        mask[j] = (say[ipPtr]-0x30)*10 + say[ipPtr+1]-0x30;
      	        ipPtr+=3;
      	     }
      	     else
      	     {
      	        mask[j] = (say[ipPtr]-0x30)*100 + (say[ipPtr+1]-0x30)*10 + say[ipPtr+2]-0x30;
      	        ipPtr+=4;
      	     }
      	  }
      	}
    	}
    }
    
    if (strstr(say,"route add default gw"))
    {
    	//IP地址
      ipPtr = 21;
  	  for(j=0;j<4;j++)
  	  {
    	  if (j<3)
    	  {
      	  if (say[ipPtr+1]==0x2e)    //只有一位数
      	  {
      	     gw[j] = say[ipPtr]-0x30;
      	     ipPtr+=2;
      	  }
      	  else
      	  {
         	   if (say[ipPtr+2]==0x2e)    //只有二位数
        	   {
      	        gw[j] = (say[ipPtr]-0x30)*10 + say[ipPtr+1]-0x30;
      	        ipPtr+=3;
      	     }
      	     else
      	     {
      	        gw[j] = (say[ipPtr]-0x30)*100 + (say[ipPtr+1]-0x30)*10 + say[ipPtr+2]-0x30;
      	        ipPtr+=4;
      	     }
      	  }
      	}
      	else
      	{
      	  if (say[ipPtr+1]==0x20)    //只有一位数
      	  {
      	     gw[j] = say[ipPtr]-0x30;
      	     ipPtr+=2;
      	  }
      	  else
      	  {
         	   if (say[ipPtr+2]==0x20)    //只有二位数
        	   {
      	        gw[j] = (say[ipPtr]-0x30)*10 + say[ipPtr+1]-0x30;
      	        ipPtr+=3;
      	     }
      	     else
      	     {
      	        gw[j] = (say[ipPtr]-0x30)*100 + (say[ipPtr+1]-0x30)*10 + say[ipPtr+2]-0x30;
      	        ipPtr+=4;
      	     }
      	  }
      	}
    	}
    	
    	break;
    }
  }
  fclose(fpOfRcs);
}

/************************************************
函数名称:saveIpMaskGateway
描述:保存以太网的IP地址、掩码及默认地址到rcS中
调用函数:
被调用:
输入参数:
输出参数:
返回值:
**************************************************/
void saveIpMaskGateway(unsigned char ip[4],unsigned char mask[4],unsigned char gw[4])
{
  FILE *fpOfRcs;
  int  i, j;
  char say[100];  

  if ((fpOfRcs=fopen("/etc/init.d/rcS","r+"))==NULL)
  {
  	printf("Can't open rcS\n");  	
  }
  else
  {
    i=0;
    while(i<50)
    {
      i++;
      fgets(say, 100, fpOfRcs);
      
      if (strstr(say,"netmask"))
      {
        fseek(fpOfRcs, -(strlen(say)), 1);
        
        sprintf(say,"ifconfig eth0 %d.%d.%d.%d netmask %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3],mask[0],mask[1],mask[2],mask[3]);
        
        for(j=strlen(say); j<52; j++)
        {
        	 strcat(say," ");
        }
        
        fputs(say, fpOfRcs);
  
      	printf("eth0 ip and mask saved!\n");
      }
      
      if (strstr(say,"route add default gw"))
      {
        fseek(fpOfRcs, -(strlen(say)), 1);
        
        sprintf(say,"route add default gw %d.%d.%d.%d dev eth0", gw[0], gw[1], gw[2], gw[3]);
        
        for(j=strlen(say); j<43; j++)
        {
        	 strcat(say," ");
        }
        
        fputs(say, fpOfRcs);
  
      	printf("default gateway saved!\n");
      	
      	break;
      }
    }
    fclose(fpOfRcs);
  }
}

/*******************************************************
函数名称:readAcVision
功能描述:读取交采示值
调用函数:
被调用函数:
输入参数:*tmpData-数据缓存指针
输出参数:
返回值:TRUE(查询成功) or FALSE(查询失败)
*******************************************************/
BOOL readAcVision(INT8U *tmpData, DATE_TIME time, INT8U dataType)
{
	sqlite3_stmt *stat;
	char   *pSqlStr;
	INT16U result;
  struct timeval  tv;       //Linux timeval
	
	//查询数据
  time.day    = 1;
  time.hour   = 0;
  time.minute = 0;
  time.second = 0;
  getLinuxFormatDateTime(&time, &tv, 1);
	
	pSqlStr = sqlite3_mprintf("select * from acVision where dataType=%d and time=%d", dataType, tv.tv_sec);	
  sqlite3_prepare(sqlite3Db, pSqlStr, strlen(pSqlStr), &stat, 0);
	result = sqlite3_step(stat);
	if(result == SQLITE_ROW)
	{
		if (dataType==ENERGY_DATA)
		{
		  memcpy(tmpData, sqlite3_column_blob(stat, 2), SIZE_OF_ENERGY_VISION);
		}
		
		if (dataType==REQ_REQTIME_DATA)
		{
		  memcpy(tmpData, sqlite3_column_blob(stat, 2), LENGTH_OF_REQ_RECORD);
		}
		
		sqlite3_finalize(stat);

	  sqlite3_free(pSqlStr);

		return TRUE;
	}

	sqlite3_finalize(stat);
	sqlite3_free(pSqlStr);
  
  if (dataType==ENERGY_DATA)
  {
    memset(tmpData, 0x0, SIZE_OF_ENERGY_VISION);
  }

  if (dataType==REQ_REQTIME_DATA)
  {
    memset(tmpData, 0x0, LENGTH_OF_REQ_RECORD);
  }

	return FALSE;
}

/*******************************************************
函数名称:updateAcVision
功能描述:更新交采示值
调用函数:
被调用函数:
输入参数:*tmpData-数据缓存指针
输出参数:
返回值:TRUE(更新成功) or FALSE(更新失败)
*******************************************************/
BOOL updateAcVision(INT8U *tmpData, DATE_TIME time, INT8U dataType)
{
	sqlite3_stmt *stat;
	char   *pSqlStr;
	INT16U result;
  struct timeval  tv;                  //Linux timeval

  time.day    = 1;
  time.hour   = 0;
  time.minute = 0;
  time.second = 0;
  getLinuxFormatDateTime(&time,&tv,1);

	//查询数据
	pSqlStr = sqlite3_mprintf("select * from acVision where dataType=%d and time=%d", dataType, tv.tv_sec);	
  sqlite3_prepare(sqlite3Db, pSqlStr, strlen(pSqlStr), &stat, 0);
	result = sqlite3_step(stat);
	sqlite3_finalize(stat);
	sqlite3_free(pSqlStr);

	if(result == SQLITE_ROW)
	{
   	pSqlStr = sqlite3_mprintf("update acVision set data=? where dataType=%d and time=%d;",dataType,tv.tv_sec);
		sqlite3_prepare(sqlite3Db, pSqlStr, -1, &stat, 0);
		if (dataType==ENERGY_DATA)
		{
		  sqlite3_bind_blob(stat, 1, tmpData, SIZE_OF_ENERGY_VISION, NULL);
		}
		else
		{
		  sqlite3_bind_blob(stat, 1, tmpData, LENGTH_OF_REQ_RECORD, NULL);
		}
		sqlite3_step(stat);
		sqlite3_finalize(stat);
		
		if (debugInfo&PRINT_AC_VISION)
		{
		  printf("更新交采示值\n");
		}
	}
	else
	{
   	pSqlStr = sqlite3_mprintf("insert into acVision values(%d, %d, ?);",dataType,tv.tv_sec);
		sqlite3_prepare(sqlite3Db, pSqlStr, -1, &stat, 0);
		if (dataType==ENERGY_DATA)
		{
		  sqlite3_bind_blob(stat, 1, tmpData, SIZE_OF_ENERGY_VISION, NULL);
		}
		else
		{
		  sqlite3_bind_blob(stat, 1, tmpData, LENGTH_OF_REQ_RECORD, NULL);
		}
		sqlite3_step(stat);
		sqlite3_finalize(stat);
		
		if (debugInfo&PRINT_AC_VISION)
		{
		  printf("插入交采示值\n");
		}
	}
	
	sqlite3_free(pSqlStr);

	return TRUE;
}

/*******************************************************
函数名称:deleteAcVision
功能描述:清除交采示值
调用函数:
被调用函数:
输入参数:void
输出参数:
返回值:TRUE(查询成功) or FALSE(查询失败)
*******************************************************/
BOOL deleteAcVision(void)
{
 #ifdef AC_SAMPLE_DEVICE	
	
	sqlite3_stmt *stat;
	char   *sql;
	INT16U result;

	//查询数据
	sql = "delete from acVision";
	result = sqlite3_exec(sqlite3Db, sql, 0, 0, NULL);
	if(result == SQLITE_OK)
	{
    //交采需量缓存
    readAcVision(acReqTimeBuf, sysTime, REQ_REQTIME_DATA);

		return TRUE;
	}
  
  //交采需量缓存
  readAcVision(acReqTimeBuf, sysTime, REQ_REQTIME_DATA);
  
 #endif

	return FALSE;
}


/*******************************************************
函数名称:deletePresentData
功能描述:删除测量点pn的当前数据         
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void deletePresentData(INT16U pn)
{
	 INT16U       result;
   char         tableName[30];
   char         pSqlStr[512];          //SQL语句字符串指针

   strcpy(tableName, bringTableName(sysTime,1));

   sprintf(pSqlStr, "delete from %s where pn=%d;", tableName, pn);
   result = sqlite3_exec(sqlite3Db, pSqlStr, 0, 0, NULL);
   if (result==SQLITE_OK)
   {
 	   if ((debugInfo&PRINT_PULSE_DEBUG) || (debugInfo&PRINT_DATA_BASE))
 	   {
 	      printf("删除表%s中测量点%d实时数据成功\n", tableName, pn);
 	   }
   }
   else
   {
 	   if ((debugInfo&PRINT_PULSE_DEBUG) || (debugInfo&PRINT_DATA_BASE))
 	   {
 	      printf("删除表%s中测量点%d实时数据失败\n", tableName, pn);
 	   }
   }
}


#ifdef LIGHTING

/*******************************************************
函数名称:readSlDayData
功能描述:读取路灯日数据
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
BOOL readSlDayData(INT16U pn, INT8U type, DATE_TIME readTime, INT8U *dataBuf)
{ 
	sqlite3_stmt *stmt;
  char         tableName[20];     //临时表名
	char         *pSqlStr;          //SQL语句字符串指针
  INT16U       execResult;
	const char   *tail;
  
  //0.sl日数据表名,每日一个表
  sprintf(tableName, "slday%02x%02x%02x", readTime.year, readTime.month, readTime.day);
  
	pSqlStr = sqlite3_mprintf("select * from %s where pn=%d and type=%d", tableName, pn, type);
	
  if ((execResult=sqlite3_prepare(sqlite3Db, pSqlStr, strlen(pSqlStr), &stmt, &tail))!= SQLITE_OK)
  {
    if (debugInfo&PRINT_DATA_BASE)
    {
      fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(sqlite3Db));
      
      //处理异常      
      if (execResult!=101)
      {
        printf("readSlDayData(1)->");
      }
    }
    
    logRun("readSlDayData happen aberrant.");
    sqlite3Aberrant(execResult);
    
    if (debugInfo&PRINT_DATA_BASE)
    {
      printf("readSlDayData(%02d-%02d-%02d %02d:%02d:%02d):未找到数据:pn=%d,类型为:%02d\n\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, pn, type);
    }
     
    memset(dataBuf, 0xee, 6);
    
    return FALSE;
  }
  else
  {
    execResult = sqlite3_step(stmt);
    if(execResult==SQLITE_ROW)
    {
      memcpy(dataBuf, sqlite3_column_blob(stmt, 2), 6);
      
      if (debugInfo&PRINT_DATA_BASE)
      {
        printf("readSlDayData(%02d-%02d-%02d %02d:%02d:%02d):找到数据:pn=%d,类型为:%02d\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second,pn,type);
      }
    }
    else
    {
      //处理异常
      if (execResult!=SQLITE_DONE)
      {
        logRun("readSlDayData(2) happen aberrant.");
          
        if (debugInfo&PRINT_DATA_BASE)
        {
          printf("readSlDayData(2) happen aberrant.\n");
        }
      }
      
      sqlite3Aberrant(execResult);
      sqlite3_finalize(stmt);
      
      memset(dataBuf, 0xee, 6);
      
      if (debugInfo&PRINT_DATA_BASE)
      {
        printf("readSlDayData(%02d-%02d-%02d %02d:%02d:%02d):无数据:pn=%d,类型为:%02d\n",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second, pn, type);
      }

      return FALSE;
    }
    
	  sqlite3_finalize(stmt);
	}

  sqlite3_free(pSqlStr);

  return TRUE;
}

/*******************************************************
函数名称:saveSlDayData
功能描述:存储路灯日数据
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
BOOL saveSlDayData(INT16U pn, INT8U type, DATE_TIME saveTime, INT8U *dataBuf, INT8U len)
{ 
	sqlite3_stmt *stmt;
	char         *pSqlStr;          //SQL语句字符串指针
  char         *errMessage;       //错误消息(Error msg written her)
  INT16U       execResult;
  char         tableName[20];     //临时表名
  INT8U        tmpReadBuf[20];
  INT8U        i;
  
  //0.sl日数据表名,每日一个表
  sprintf(tableName, "slday%02x%02x%02x", saveTime.year, saveTime.month, saveTime.day);
  
	//1.查询是否存在数据表,如不存在建立
	pSqlStr = sqlite3_mprintf("select * from  %s  limit 1;",tableName);
  if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)   //查询成功
  {
  	if (debugInfo&PRINT_DATA_BASE)
  	{
  	  printf("数据表%s已经建立\n",tableName);
  	}
  }
  else
  {
    sqlite3_free(pSqlStr);
	  
	  pSqlStr = sqlite3_mprintf("create table %s(pn int, type int, data blob);",tableName);
    if (sqlite3_exec(sqlite3Db, pSqlStr, NULL, NULL, &errMessage)==SQLITE_OK)
    {
     	if (debugInfo&PRINT_DATA_BASE)
     	{
     	  printf("创建表%s成功\n",tableName);
     	}
    }
    else
    {
     	if (debugInfo&PRINT_DATA_BASE)
     	{
     	  printf("创建表%s失败\n",tableName);
     	}
     	 
     	sqlite3_free(errMessage);
      sqlite3_free(pSqlStr);
      
      return FALSE;
    }
  }
  
  sqlite3_free(pSqlStr);
  
  
  //查询是否存在需要存储的数据,如果没有,则插入,如果有则比较数据是否一致,如果不致,则更新
  if (readSlDayData(pn, type, saveTime, tmpReadBuf)==FALSE)
  {
  	pSqlStr = sqlite3_mprintf("insert into %s values (%d,%d,?);", tableName, pn, type);
    
    if (debugInfo&PRINT_DATA_BASE)
    {
     	printf("saveSlDayData:插入数据");
    }
  }
  else
  {
  	for(i=0; i<len; i++)
    {
    	if (tmpReadBuf[i]!=dataBuf[i])
    	{
    		break;
    	}
    }
    
    if (i>=len)
    {
      if (debugInfo&PRINT_DATA_BASE)
      {
   	    printf("saveSlDayData:数据已存在!pn=%02d,type=%02d\n", pn, type);
      }

    	return FALSE;
    }
    
   	pSqlStr = sqlite3_mprintf("update %s set data=? where pn=%d and type=%d;", tableName, pn, type);

    if (debugInfo&PRINT_DATA_BASE)
    {
     	printf("saveSlDayData:更新数据");
    }
  }
  
  sqlite3_prepare(sqlite3Db, pSqlStr, -1, &stmt, 0);
  sqlite3_bind_blob(stmt, 1, dataBuf, len, NULL);

  if ((execResult=sqlite3_step(stmt))==SQLITE_DONE)
  {
    if (debugInfo&PRINT_DATA_BASE)
    {
   	  printf("成功!pn=%02d,type=%02d\n", pn, type);
    }
   	
    //监视数据库异常次数清零
    dbMonitor = 0;
  }
  else
  {
   	//处理异常
    printf("saveSlDayData happen aberrant.\n");
    logRun("saveSlDayData happen aberrant.");
    sqlite3Aberrant(execResult);
         	
   	if (debugInfo&PRINT_DATA_BASE)
   	{
      fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(sqlite3Db));
   	  
   	  printf("失败!pn=%02d,type=%02d\n", pn, type);
   	}
  }
  
  sqlite3_finalize(stmt);
}

#endif

