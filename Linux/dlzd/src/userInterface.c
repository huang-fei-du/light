/***************************************************
Copyright,2009,Huawei WoDian co.,LTD,All	Rights Reserved
文件名：userInterface.c
作者：leiyong
版本：0.9
完成日期：2009年12月
描述：电力终端人机接口文件
函数列表：
     1.
修改历史：
  01,09-12-23,Leiyong created.
  02,10-11-23,Leiyong,重庆电力公司测试发现,485总表未纳入抄表工况统计,将485表抄表成功与否纳入统计
  03,10-11-23,Leiyong,重庆电力公司测试发现,流量统计是以B为单位显示,将其改为以KB为单位统计


***************************************************/
#include <pthread.h>
#include <sys/reboot.h>

#include <stdlib.h>
#include <sys/mount.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/if_arp.h>

#include "string.h"
#include "common.h"
#include "ioChannel.h"
#include "meterProtocol.h"

#include "teRunPara.h"
#include "msSetPara.h"
#include "lcdGui.h"
#include "wlModem.h"
#include "convert.h"
#include "workWithMeter.h"
#include "dataBase.h"
#include "copyMeter.h"
#include "hardwareConfig.h"
#include "att7022b.h"
#include "AFN05.h"
#include "AFN0C.h"
#include "AFN0E.h"

#ifdef LOAD_CTRL_MODULE
 #include "loadCtrl.h"
#endif

#ifdef PLUG_IN_CARRIER_MODULE
 #include "gdw376-2.h"
#endif

#include "userInterface.h"


//各型终端公共显示变量---------------------------------------------------------
INT8U          wlRssi;                                           //无线Modem信号
DATE_TIME      lcdLightDelay;                                    //LCD背光延时
INT8U          lcdLightOn = LCD_LIGHT_ON;                        //LCD背光打开?
INT16U         keyPressCount;                                    //按键计数,防抖
#ifdef PLUG_IN_CARRIER_MODULE
 INT16U        keyCountMax = 10;                                 //按键计数最大值
#else
 INT16U        keyCountMax = 12;                                 //按键计数最大值
#endif

INT8U          keyValue;                                         //键值
INT8U          displayMode=0;                                    //显示模式(默认菜单-1,轮显-2,按键-3)
INT8S          menuInLayer=0;                                    //进菜单层数
INT8U          layer1MenuLight=0;	  	                           //1层菜单的第几个选项

DATE_TIME searchStart, searchEnd;                                //搜索开始时间

char           character[70]=". +-@*#0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";//选择用字符
INT8U          inputStatus=STATUS_NONE;                          //输入状态
INT8U          inputIndex=0;                                     //输入索引
INT8U          selectIndex=0;                                    //选择输入索引

INT8U          alarmCount=0;                                     //告警计数
INT8U          pageWait = 0;                                     //页面显示时间,超过页面显示时间回到常显画面
char           tmpVpnUserName[33]="";                            //临时Vpn用户名
char           tmpVpnPw[33]="";                                  //临时Vpn密码

ABERRANT_ALARM     aberrantAlarm;                                    //异常告警
struct cpAddrLink  *queryMpLink,*tmpMpLink,*tmpPrevMpLink;           //查询测量点数据链表
DATE_TIME          menuQueryTime;                                    //查询时间
INT8U              rowOfLight;                                       //上下键(翻页)控制(专变III型终端通信参数设置等地方用)
INT8U              keyLeftRight;                                     //左右键(翻页)控制(重点用户及通信参数设置等地方用)
char               queryTimeStr[9];                                  //查询时间字符串
char               originPassword[7]="000000";                       //原始密码
char               passWord[7] = "000000";                           //密码输入框中的密码字符
INT8U              pwLight=0;                                        //密码字符高亮序号
char               tmpTeAddr[10]="";                                 //临时终端地址
char               commParaItem[5][30];                              //通信参数设置时的临时字符串项

char               chrEthPara[4][20];                                       //临时以太网参数
char               tmpEthMac[30];

//376.1集中器公共显示变量--------------------------------------------------------
#ifdef PLUG_IN_CARRIER_MODULE
 struct            carrierMeterInfo *foundMeterHead,*tmpFound,*prevFound,*noFoundMeterHead;//发现电表指针
 struct            carrierMeterInfo *existMeterHead,*prevExistFound;                       //发现电表与配置相同指针
 struct            carrierMeterInfo *tmpNoFoundx;

 INT16U            multiCpUpDown,multiCpMax;                         //多测量点抄表上下翻页,及最多页数
 char              singleCopyMp[5]="0001";                           //指定测量点抄表测量点字符串
 char              singleCopyTime[9]="-----";                        //指定测量点抄表抄表时间
 char              singleCopyEnergy[10]="-----";                     //指定测量点抄表示值
 struct            carrierMeterInfo *mpCopyHead, *tmpMpCopy, *prevMpCopy;//所有测量点抄表指针
 char              realCopyMeterItem[2][18] = {"1、指定测量点抄表","2、全部测量点抄表"};
 
 char              chrMp[5][13];                                     //临时测量点信息

 #ifdef MENU_FOR_CQ_CANON  //重庆规约集中器菜单变量-----------------------------

  METER_DEVICE_CONFIG menuMeterConfig;                               //菜单显示用测量点的配置信息
  char      weekName[7][7] = {"星期日","星期一","星期二","星期三","星期四","星期五","星期六"};
  char      layer1MenuItem[MAX_LAYER1_MENU][12]={"1、抄表查询","2、参数查询","3、重点用户","4、统计查询","5、参数设置","6、现场调试"};
  char      paraSetItem[3][17] = {"5-1 通信参数设置", "5-2 修改密码    ", "5-3VPN用户名密码"};
  char      debugItem[7][19]   = {"     实时抄表     ","全部测量点抄表结果","     表号搜索     ","  新增电能表地址  ","新增电能表抄表状态"," 未搜到电能表地址 ","     主动上报     "};
 
  INT8U     layer2MenuLight[MAX_LAYER1_MENU]={0,0,0,0,0,0};          //2层菜单的第几个选项
  INT8U     layer2MenuNum[MAX_LAYER1_MENU]  ={6,5,6,5,2,7};          //2层菜单的各菜单最大值
  INT8U     layer3MenuNum[MAX_LAYER1_MENU][MAX_LAYER2_MENU]= {{8,0,0,0,0},{0,0,0,0,0},{8,0,0,0,0},{8,0,0,0,0},{5,3,0,0,0},{2,0,0,0,0}};
  INT8U     layer3MenuLight[MAX_LAYER1_MENU][MAX_LAYER2_MENU]= {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};

 #else  //MENU_FOR_CQ_CANON,国家电网规约----------------------------------------

  #ifdef LIGHTING
   char     layer1MenuItem[MAX_LAYER1_MENU][15]={"单灯控制点状态","参数设置与查询","   管理维护   ","线路控制点状态","报警控制点状态","    光照度    "};
  #else
   char     layer1MenuItem[MAX_LAYER1_MENU][15]={"测量点数据显示","参数设置与查看","终端管理与维护"};
  #endif

  char      layer2MenuItem[MAX_LAYER1_MENU][MAX_LAYER2_MENU][19]={
	                    {},
	                   #ifdef LIGHTING
	                    {"   通信通道设置   ","        -         ","  控制点参数设置  ","  集中器时间设置  ","   界面密码设置   ","    集中器编号    ","  以太网参数设置  ","虚拟专网用户名密码"},
	                    {"查询单灯控制点状态", "     本机信息     ", "  调节液晶对比度  ", "     复位集中器    ", "  集中器软件升级  ","    红外遥控器    "},
	                   #else
	                    {"   通信通道设置   "," 台区电表参数设置 "," 集抄电表参数设置 ","   终端时间设置   ","   界面密码设置   ","     终端编号     ","  以太网参数设置  ","虚拟专网用户名密码","   级联参数设置   ","   锐拔模块参数   "},
	                    {"     实时抄表     ","全部测量点抄表结果","     表号搜索     ","  新增电能表地址  ","新增电能表抄表状态","     本机信息     ","  调节液晶对比度  "," 本地模块抄读方式 ","     复位终端     ","   终端程序升级   "," 路由模块软件升级 ","居民用户表数据类型","     轮显设置     ","  维护口模式设置  ", "  第2路485口设置  ", " 本地通信模块协议 "},
	                   #endif
	                    };
 #ifdef LIGHTING
  char      layer2xMenuItem[2][3][17]= 
  	                                   {{"    参数查询    ", "    参数设置    "},
  	                                    {"   控制点参数   ", "  通信通道参数  ", "    控制时段    "}
  	                                   };
 #else
  char      layer2xMenuItem[2][2][17]= 
  	                                   {{"    参数查看    ", "    参数设置    "},
  	                                    {" 查询测量点参数 ", "查询通信通道参数"}
  	                                   };
 #endif

  INT8U     layer2MenuLight[MAX_LAYER1_MENU]={0, 0, 0};              //2层菜单的第几个选项

 #ifdef LIGHTING
  INT8U     layer2MenuNum[MAX_LAYER1_MENU]  ={21, 8, 6};             //2层菜单的各菜单最大值

  struct ctrlTimes *tmpCTimesNode;
  
  extern INT8U downLux[6];                                           //服务器下传的当前流明值

	//红外遥控菜单
	char   irMenuItem[8][11] = {
															"  制冷开  ",
															"  制热开  ",
															"  除湿开  ",
															"  关空调  ",
															"学习制冷开",
															"学习制热开",
															"学习除湿开",
															"学习关空调",
														 };
 #else
  INT8U     layer2MenuNum[MAX_LAYER1_MENU]  ={21, 9, 16};            //2层菜单的各菜单最大值
 #endif

  INT8U     layer3MenuNum[MAX_LAYER1_MENU][MAX_LAYER2_MENU]= {{8,0,0,0,0,0},{5,6,6,0,3,0,5,5,6},{2,0,0,0,0,4}};
  INT8U     layer3MenuLight[MAX_LAYER1_MENU][MAX_LAYER2_MENU]= {{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},};
  INT8U     layer2xMenuLight;
  INT8U     mpQueryType, mpQueryPage, mpQueryLight;                  //查询测量点参数临时变量

  char      dateTimeItem[15];                                        //设置日期时间项
  char      chrRlPara[4][3];                                         //临时锐拔模块参数
  char      chrCopyRate[8][6]={"默认","600","1200","2400","4800","7200","9600","19200"};
  char      chrCopyPort[3][8]={"交采","RS485-1","RS485-2"};
  char      chrCopyProtocol[3][13]={"DL/T645-1997","DL/T645-2007","交流采样"};
  
  DATE_TIME cycleDelay;                                             //轮显延时
  struct cpAddrLink *cycleMpLink, *tmpCycleLink;                    //轮显测量点数据链表
 
 #endif  //MENU_FOR_CQ_CANON
 
#else    //no PLUG_IN_CARRIER_MODULE,终端
 //结构 - 抄表地址链表
 struct eventShowLink
 {
	 INT16U               eventNo;                //事件编号
	 
	 struct eventShowLink *next;                  //下一节点指针
 };

 char       layer1MenuItem[MAX_LAYER1_MENU][15]={"1.实时数据",
 	                                               "2.参数定值",
 	                                               "3.控制状态",
 	                                               "4.电能表示数",
 	                                               "5.中文信息",
 	                                               "6.购电信息",
 	                                               "7.终端信息"};
 char       layer2MenuItem[MAX_LAYER1_MENU][MAX_LAYER2_MENU][19]={
	                    {"1.当前功率",
	                     "2.当前电量",
	                     "3.负荷曲线",
	                     "4.开关状态",
	                     "5.功控记录",
	                     "6.电控记录",
	                     "7.遥控记录",
	                     "8.失电记录"
	                    },
	                    {"1.时段控参数",
	                     "2.厂休控参数",
	                     "3.下浮控参数",
	                     "4.KvKiKp    ",
	                     "5.电能表参数",
	                     "6.配置参数  ",
	                     "7.专网用户名",
	                     "8.表计配置  ",
	                     "9.立即抄表  ",
	                     "10.终端软件升级",
	                     "11.级联参数 ",
	                     "12.LCD对比度",
                       "13.维护口模式",
                       "14.第2路485设置",
	                     "15.以太网参数"
	                    },
	                    {},
	                    {},
	                    {}
	                    };
 INT8U      layer2MenuLight[MAX_LAYER1_MENU]={0,0,0,0,0,0,0};       //2层菜单的第几个选项
 INT8U      layer2MenuNum[MAX_LAYER1_MENU]  ={8,8,0,0,0,0,3};       //2层菜单的各菜单最大值
 INT8U      layer3MenuNum[MAX_LAYER1_MENU][MAX_LAYER2_MENU]= {{8,0,0,0,0,0},{5,6,6,0,3,0,0,8,0,0,6},{2,0,0,0,0,3}};
 INT8U      layer3MenuLight[MAX_LAYER1_MENU][MAX_LAYER2_MENU]= {{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},};
 INT8U      numOfPage;                                              //中文信息或总加组页数
 INT8U      mpPowerCurveLight;                                      //测量点功率曲线高亮项
 struct eventShowLink *eventLinkHead,*tmpEventShow;                 //事件读取链表
 char       chrMp[7][13];                                           //临时测量点信息
 char       chrCopyRate[8][6]={"默认","600","1200","2400","4800","7200","9600","19200"};
 char       chrCopyPort[3][8]={"交采","RS485-1","RS485-2"};
 char       chrCopyProtocol[8][13]={"DL/T645-1997","DL/T645-2007","交流采样","ABB方表","兰吉尔ZD表","红相EDMI表","明武多功能表","明武UI表"};
 
#endif   //PLUG_IN_CARRIER_MODULE




//各种机型公共函数***************************************************

/***************************************************
函数名称:getFileName
功能描述:取得文件路径中的文件名
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
int getFileName(char *file, char *filename)
{
	unsigned int i;
	
	char *token = NULL;
	char seps[] = "/";
	token = strtok(file, seps);
	while(token != NULL)
	{
		strcpy(filename, token);
		token = strtok(NULL, seps);
	}
	
	return 0;
}

/***************************************************
函数名称:tipSound
功能描述:提示音
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
INT8U tipSound(INT8U type)
{
   switch(type)
   {
     case 1:
       ioctl(fdOfIoChannel, SET_ALARM_VOICE, 1);
       sleep(1);
       ioctl(fdOfIoChannel, SET_ALARM_VOICE, 0);
       sleep(1);
       ioctl(fdOfIoChannel, SET_ALARM_VOICE, 1);
       sleep(1);
       ioctl(fdOfIoChannel, SET_ALARM_VOICE, 0);
       break;
     
     case 2:
       ioctl(fdOfIoChannel, SET_ALARM_VOICE, 1);
       usleep(100000);
       ioctl(fdOfIoChannel, SET_ALARM_VOICE, 0);
       usleep(300000);
       ioctl(fdOfIoChannel, SET_ALARM_VOICE, 1);
       usleep(100000);
       ioctl(fdOfIoChannel, SET_ALARM_VOICE, 0);
       break;
   }
}
    

/***************************************************
函数名称:uDiskUpgrade
功能描述:U盘升级
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
INT8U uDiskUpgrade(void)
{
  INT8U upOk=1;
  int   len, i;
  char  filenames[20][256];
  FILE  *file;
  char  fileName[256];
  char  tmpProcess[256];
  char  devStr[20];
  INT8U ifFound;
 
  guiLine(1,55,160,105,0);
  guiLine(1,55,1,105,1);
  guiLine(160,55,160,105,1);
  guiLine(1,55,160,55,1);
  guiLine(1,105,160,105,1);
  
  //1.mount
  ifFound = 0;
  for(i=1; i<16; i++)
  {
    sprintf(devStr, "/dev/sda%d", i);
    if (mount(devStr, "/mnt", "vfat", 0, NULL)==0)
    {
      guiDisplay(8, 70 ,"U盘已插入,检测文件", 1);
      lcdRefresh(50, 105);
  
      printf("mount %s /mnt 成功\n", devStr);
      
      ifFound = 1;
      break;
    }
  }

  if (ifFound==0)
  {
    guiDisplay(44, 70, "请插入U盘", 1);
    lcdRefresh(50, 105);
  
    printf("执行mount失败,错误信息:%s\n", strerror(errno));
  
    tipSound(2);
  
    return 0;
  }

  //2.取配置文件
  if((file=fopen("/bin/config.ini","r")) != NULL)
  {
    i = 0;
    while(fgets(filenames[i], 256, file) != NULL)
    {
    	len = strlen(filenames[i]);
    	filenames[i][len - 2] = '\0';
    	i++;
    }
    
    len = i;
  }
  else
  {
    guiLine(3,57,158,103,0);
    guiDisplay(10, 70, "配置文件读取失败", 1);
    lcdRefresh(50, 105);
    
  	printf("config.ini is not exist.");

    tipSound(2);

  	goto umountBreak;
  }
  
  //3.查找是否存在升级文件
  for(i=0;i<len;i++)
  {
    strcpy(tmpProcess,filenames[i]);
    getFileName(tmpProcess, fileName);
    sprintf(tmpProcess,"mnt/upfile/%s", fileName);

    if(access(tmpProcess, F_OK) != 0)
    {
      guiLine(3,57,158,103,0);
      guiDisplay(8, 70, "升级文件未发现/不全", 1);
      lcdRefresh(50, 105);
      upOk = 0;
      printf("升级文件%s不存在\n", tmpProcess);
      
      tipSound(2);

      goto umountBreak;
    }
  }
    
  guiLine(3, 57, 158, 103, 0);
  guiDisplay(3, 60, "升级中,复制:", 1);
  for(i=0; i<len; i++)
  {
    getFileName(filenames[i], fileName);
    guiLine(3, 77, 158, 103, 0);
    guiDisplay(10, 80, fileName, 1);
    lcdRefresh(50, 105);

    sprintf(tmpProcess, "cp /mnt/upfile/%s %s", fileName, filenames[i]);
    if (system(tmpProcess)==0)
    {
    	printf("命令%s成功\n", tmpProcess);
    }
    else
    {
      guiDisplay(127, 80, "失败", 1);
      lcdRefresh(50, 105);
    	 
    	printf("复制文件%s失败\n",fileName);
    	upOk = 0;
      
      tipSound(2);

    	break;
    }
  }
  
umountBreak:
  if (umount("/mnt")==0)
  {
    if (upOk==1)
    {
      guiLine(3, 57, 158, 103, 0);
      guiDisplay(3,70,"升级完成,系统重启..",1);
      lcdRefresh(50,105);

      printf("umount成功,系统重启\n");
    }
  }
  else
  {

    guiDisplay(10,70,"弹出U盘失败.", 1);
    lcdRefresh(50,105);

    printf("执行umount失败,错误信息:%s\n", strerror(errno));

    tipSound(2);
    
    return 0;
  }
  
  if (upOk==1)
  {
    tipSound(1);
    
    sleep(3);
      
    reboot(RB_AUTOBOOT);
  }
}

/*******************************************************
函数名称:showInfo
功能描述:状态栏显示信息(376.1国家电网集中器规约)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void showInfo(char *info)
{
	#ifdef PLUG_IN_CARRIER_MODULE
	 #ifndef CQDL_CSM
	  guiLine(1,LCD_LINE_INFO,160,160,0);
	  guiDisplay(1,LCD_LINE_INFO,info,1);
	 
	  lcdRefresh(LCD_LINE_INFO,160);
	 #endif
	#endif
}

/***************************************************
函数名称:trim
功能描述:去除字符串后面的空格
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
char * trim(char * s)
{
   INT8U i,j,len,numOfPrevSpace;
   
   //去除字符串前面的空格
   len = strlen(s);
   
   for(i=0;i<len;i++)
   {
  	  if (s[i]!=0x20)
  	  {
  	  	 break;
  	  }
   }

   if (i>0)
   {
     numOfPrevSpace = i+1;
     for(j=0;j<len;j++)
     {
     	  if (j+numOfPrevSpace<=len)
     	  {
     	     s[j] = s[i++];
     	  }
     	  else
     	  {
     	  	 s[j] = 0x0;
     	  }
     }
   }

   //去除字符串后面的空格
   len = strlen(s);
   for(i=len;i>0;i--)
   {
  	  if (s[i-1]!=0x20)
  	  {
  	  	 break;
  	  }
  	  if (s[i-1]==0x20)
  	  {
  	  	 s[i-1] = 0x0;
  	  }
   }

   len = strlen(s);
   
   return s;
}

/**************************************************
函数名称:startDisplay
功能描述:开机显示
调用函数:
被调用函数:
输入参数:void *arg
输出参数:
返回值：状态
***************************************************/
void startDisplay(void)
{
  char     str[6];
 
  lcdClearScreen();

  //清屏
  guiLine(1, 1, 160, 160, 0);
 
  layer1Menu(0);
 
  switch(moduleType)
  {
    case GPRS_SIM300C:
    case GPRS_GR64:
    case GPRS_MC52I:
    case GPRS_M72D:
      guiAscii(22,1,"G",1);
      break;

    case ETHERNET:
      guiAscii(15,1,"E",1);
      break;
      
    case CDMA_DTGS800:
    case CDMA_CM180:
      guiAscii(22,1,"C",1);
      break;

    case CASCADE_TE:
      guiAscii(14,1,"JL",1);
      break;
		
    case LTE_AIR720H:
      guiAscii(22,1,"4",1);
      break;

    default:
      guiAscii(22,1,"N",1);
      break;
  }
 
 #ifdef PLUG_IN_CARRIER_MODULE
  #ifdef LIGHTING
   guiDisplay(24,30,"智能路灯集中器",1);
  #else
   guiDisplay(24,30,"低压集抄集中器",1);
  #endif
 
  guiLine(124,1,160,16,0);
  guiAscii(124, 1, digital2ToString(sysTime.hour,str),1);
  guiAscii(139, 1, ":", 1);
  guiAscii(145, 1, digital2ToString(sysTime.minute,str),1);
 
  defaultMenu();


  displayMode = DEFAULT_DISPLAY_MODE;    //默认菜单
 #else
  guiAscii(103, 1, digital2ToString(sysTime.hour,str),1);
  guiAscii(118, 1, ":", 1);
  guiAscii(124, 1, digital2ToString(sysTime.minute,str),1);
  guiAscii(139, 1, ":", 1);
  guiAscii(145, 1, digital2ToString(sysTime.second,str),1);

  guiLine(5,16,155,16,1);
 #endif
 
  lcdRefresh(1,160);
}

/**************************************************
函数名称:signal
功能描述:画信号指示
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：状态
***************************************************/
void modemSignal(int type)
{
  int i;

  if (type == 8 || type==9)
  {
 	 #ifdef LOAD_CTRL_MODULE
 	  guiLine(1,1,40,15,0);
 	 #else
 	  guiLine(1,1,40,16,0);
 	 #endif
 	  if (type==8)
 	  {
 	   guiAscii(1,1,"NoSIM",1);
 	  }
 	  if (type==9)
 	  {
 	    guiAscii(1,1,"NoUIM",1);
 	  }
  }
  else
  {
	 #ifdef LOAD_CTRL_MODULE
	  guiLine(1,1,40,15,0);
	 #else
	  guiLine(1,1,40,16,0);
	 #endif
	 
   #ifndef PLUG_IN_CARRIER_MODULE
    //画天线叉
    guiLine(4,1,4,7,1);
    guiLine(1,2,1,2,1);
    guiLine(2,3,2,3,1);
    guiLine(3,4,3,4,1); 
    guiLine(7,2,7,2,1);
    guiLine(6,3,6,3,1);
    guiLine(5,4,5,4,1);
   #endif
   
    switch(moduleType)
    {
      case GPRS_SIM300C:
      case GPRS_GR64:
      case GPRS_MC52I:
      case GPRS_M590E:
      case GPRS_M72D:
        guiAscii(22,1,"G",1);
        break;
  
      case ETHERNET:
        guiAscii(22,1,"E",1);
        break;
          
      case CDMA_DTGS800:
      case CDMA_CM180:
        guiAscii(22,1,"C",1);
        break;
        
      case CASCADE_TE:
        guiAscii(14,1,"JL",1);
        break;
			
      case LTE_AIR720H:
        guiAscii(22,1,"4",1);
        break;
     
      default:
        guiAscii(22,1,"N",1);
        break;
    }
    
    for(i=0;i<type;i++)
    {
      if (type==7)
      {
       	break;
      }
       
     #ifndef PLUG_IN_CARRIER_MODULE
      guiLine(2+5*i,9-2*i,2+5*i,14,1);
      guiLine(3+5*i,9-2*i,3+5*i,14,1);
     #else
      guiLine(4+5*i,9-2*i,4+5*i,14,1);
      guiLine(5+5*i,9-2*i,5+5*i,14,1);
     #endif
    }
  }
  
  lcdRefresh(1,16);
}

/**************************************************
函数名称:signal
功能描述:画信号指示
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：状态
***************************************************/
void messageTip(int type)
{
  guiLine(42,  3, 55, 14, 0);
	
	if (type==1)
	{
	   guiLine(45,  6, 45,  6, 1);
     guiLine(46,  7, 46,  7, 1);
     guiLine(47,  8, 47,  8, 1);
     guiLine(48,  9, 48,  9, 1);
     guiLine(49, 10, 49, 10, 1);
     guiLine(50, 11, 50, 11, 1);
     guiLine(51, 12, 51, 12, 1);
     guiLine(52, 13, 52, 13, 1);

     guiLine(52,  6, 52,  6, 1);
     guiLine(51,  7, 51,  7, 1);
     guiLine(50,  8, 50,  8, 1);
     guiLine(49,  9, 49,  9, 1);
     guiLine(48, 10, 48, 10, 1);
     guiLine(47, 11, 47, 11, 1);
     guiLine(46, 12, 46, 12, 1);
     guiLine(45, 13, 45, 13, 1);

     guiLine(46,  4, 46,  4, 1);
     guiLine(47,  3, 47,  3, 1);
     guiLine(48,  3, 48,  3, 1);
     guiLine(49,  3, 49,  3, 1);

     guiLine(51,  4, 51,  4, 1);
     guiLine(50,  3, 50,  3, 1);

     guiLine(42,  5, 42, 14, 1);
     guiLine(55,  5, 55, 14, 1);
     guiLine(42,  5, 55,  5, 1);
     guiLine(42, 14, 55, 14, 1);
  }
}

/***************************************************
函数名称:singleReport
功能描述:信号强度最大最小值记录
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：void
***************************************************/
void signalReport(INT8U type,INT8U rssi)
{  
  TERMINAL_STATIS_RECORD terminalStatisRecord;  //终端统计记录
  DATE_TIME              tmpTime;
  BOOL                   ifNeedRecord;
  char                   say[20],str[5];

  //显示信号
  if (type>0)
  {
  	 modemSignal(type);
  	 
  	 return;
  }
  else
  {
  	 wlRssi = rssi;
  	 if (rssi==0)
  	 {
  	   modemSignal(0);
  	 }
  	 else
  	 {
  	   modemSignal(rssi/8+1);
  	 }
  }
  
  if (rssi>0 && rssi<=31)
  {
   #ifdef PLUG_IN_CARRIER_MODULE
    #ifndef MENU_FOR_CQ_CANON
     strcpy(say,"信号强度->");
     strcat(say,intToString(rssi,3,str));
     showInfo(say);
    #endif
   #endif
    
    ifNeedRecord = FALSE;

    tmpTime = timeHexToBcd(sysTime);
	  if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == FALSE)
    {
   	  initTeStatisRecord(&terminalStatisRecord);
    }
    
    //记录日最大信号强度及最大信号发生时间
    if (terminalStatisRecord.maxSignal<rssi)
    {
    	 terminalStatisRecord.maxSignal = rssi;
    	 terminalStatisRecord.maxSignalTime[0] = sysTime.minute;
    	 terminalStatisRecord.maxSignalTime[1] = sysTime.hour;
    	 terminalStatisRecord.maxSignalTime[2] = sysTime.day;
       ifNeedRecord = TRUE;
    }
     
    //记录日最小信号强度及最小信号发生时间
    if (terminalStatisRecord.minSignal>=rssi)
    {
    	 terminalStatisRecord.minSignal = rssi;
    	 terminalStatisRecord.minSignalTime[0]  = sysTime.minute;
    	 terminalStatisRecord.minSignalTime[1] = sysTime.hour;
    	 terminalStatisRecord.minSignalTime[2] = sysTime.day;
       ifNeedRecord = TRUE;
    }
    
    if (ifNeedRecord == TRUE)
    {
      saveMeterData(0, 0, tmpTime, (INT8U *)&terminalStatisRecord, STATIS_DATA, 0x0,sizeof(TERMINAL_STATIS_RECORD));
    }
  }
}

/*******************************************************
函数名称:setLcdDegree
功能描述:设置LCD对比度(376.1国家电网集中器规约)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void setLcdDegree(INT8U lightNum)
{
	INT8U i, tmpX;
	char  str[2];
	
	menuInLayer = 3;
  
 #ifdef PLUG_IN_CARRIER_MODULE
	guiLine(1,17,160,144,0);
 #else
	guiLine(1,17,160,160,0);
 #endif
	
	guiLine(1,50,1,110,1);
	guiLine(160,50,160,110,1);
	guiLine(1,110,160,110,1);
	guiLine(1,50,160,50,1);
	guiLine(1,70,160,70,1);

	guiDisplay(24,52,"液晶对比度设置",1);

	guiLine( 10, 80, 10,100,1);

	guiLine(10,80,10+lightNum*7,100,1);
	lcdAdjustDegree(lightNum);
	guiLine(150, 80,150,100,1);
	guiLine( 10, 80,150, 80,1);
	guiLine( 10,100,150,100,1);	
  saveParameter(88, 7,&lcdDegree,1);         //LCD对比度

 #ifdef PLUG_IN_CARRIER_MODULE
	lcdRefresh(17,144);
 #else
	lcdRefresh(17,160);
 #endif
}



/*******************************************************
函数名称:alarmProcess
功能描述:告警处理
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void alarmProcess(void)
{
	 INT8U i,j;
	  
	 if (ifPowerOff==TRUE)    //停电(LED灯一长两短闪烁)
	 {
       alarmCount++;
       if (alarmCount<20)
       {
         alarmLedCtrl(ALARM_LED_OFF);
       }
       else
       {
       	  if (alarmCount<100)
       	  {
       	  	 alarmLedCtrl(ALARM_LED_ON);
       	  }
       	  else
       	  {
       	  	 if (alarmCount<120)
       	  	 {
       	  	 	  alarmLedCtrl(ALARM_LED_OFF);
       	  	 }
       	  	 else
       	  	 {
       	  	    if (alarmCount<140)
       	  	    {
       	  	 	     alarmLedCtrl(ALARM_LED_ON);
       	  	    }
       	  	    else
       	  	    {
       	  	       alarmCount = 0;
       	  	    }
       	  	 }
       	  }
       }
       
  	 	 if (setParaWaitTime==0)      //设置参数
       {
  	      setBeeper(BEEPER_OFF);
  	      setParaWaitTime = 0xfe;
       }
	  	 if (ctrlCmdWaitTime==0)      //控制命令
       {
  	      setBeeper(BEEPER_OFF);
 	        ctrlCmdWaitTime = 0xfe;
       }
	 }
	 else
	 {
  	 	 if (setParaWaitTime==0)      //设置参数
       {
  	      alarmLedCtrl(ALARM_LED_OFF);
  	      setBeeper(BEEPER_OFF);
  	      setParaWaitTime = 0xfe;

          //如果所有路都处于合闸状态且控制状态变量(ctrlStatus)不为NONE_CTRL,
          //则将ctrlStatus置为NONE_CTRL,使屏幕恢复常显画面
          for(i=0;i<ctrlStatus.numOfAlarm;i++)
          {
             if (ctrlStatus.allPermitClose[i]==1)
             {
          	    for(j=i;j<ctrlStatus.numOfAlarm;j++)
          	    {
          	      ctrlStatus.aQueue[j] = ctrlStatus.aQueue[j+1];
          	      ctrlStatus.allPermitClose[j]=ctrlStatus.allPermitClose[j+1];
          	    }
          	    ctrlStatus.numOfAlarm--;
          	    if (ctrlStatus.numOfAlarm==0)
          	    {
          	    	menuInLayer=1;
          	    }
             }
          }
       }
       else
       {
	  	 	 #ifdef LOAD_CTRL_MODULE 
	  	 	  if (ctrlCmdWaitTime==0)   //控制命令
          {
    	       if (gateCloseWaitTime==0 || gateCloseWaitTime==0xfe)
    	       {
    	         alarmLedCtrl(ALARM_LED_OFF);
    	         setBeeper(BEEPER_OFF);
    	         ctrlCmdWaitTime = 0xfe;
    	       }
          }
          else
          {
          	 if (gateCloseWaitTime==0)  //允许合闸提示音到期
          	 {
    	         alarmLedCtrl(ALARM_LED_OFF);
    	         setBeeper(BEEPER_OFF);
    	         gateCloseWaitTime = 0xfe;
          	 }
          }
         #endif
       }
	 }
}

/*******************************************************
函数名称:setVpn
功能描述:设置虚拟专网用户名、密码
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void setVpn(INT8U lightNum)
{
	INT8U i, row, col;
	char  str[2];
	
	menuInLayer = 3;

	#ifdef PLUG_IN_CARRIER_MODULE
	 #ifndef CQDL_CSM
	  guiLine(1,17,160,144,0);
	 #else
	  guiLine(1,17,160,160,0);
	 #endif
	#else
	 guiLine(1,17,160,160,0);
	#endif
	
	guiLine(1,34,1,128,1);
	guiLine(160,34,160,128,1);
	guiLine(1,128,160,128,1);
	guiLine(1,34,160,34,1);
	guiLine(1,54,160,54,1);

	guiDisplay( 9,36,"虚拟专网用户名密码",1);
	guiDisplay( 4,56,"用户名",1);
	guiDisplay(19,92,"密码",1);

  str[1] = '\0';
  row = 56;
  col = 53;
  for(i=0;i<32;i++)
  {
    str[0] = tmpVpnUserName[i];
 
    if (i==13)
    {
    	 row = 72;
    	 col = 5;
    }
    if (i==lightNum)
    {
      guiDisplay(col,row,str,1);
    }
    else
    {
      guiDisplay(col,row,str,0);
    }
    col+=8;
  }

  row = 92;
  col = 53;
  for(i=32; i<64; i++)
  {
    str[0] = tmpVpnPw[i-32];
 
    if (i==45)
    {
    	row = 108;
    	col = 5;
    }
    if (i==lightNum)
    {
       guiDisplay(col,row,str,1);
    }
    else
    {
       guiDisplay(col,row,str,0);
    }
    col+=8;
  }
  
	lcdRefresh(17,160);
}

/*******************************************************
函数名称:setEthPara
功能描述:设置以太网参数(376.1国家电网集中器规约)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void setEthPara(INT8U layer2Light,INT8U layer3Light)
{
	INT8U i, tmpX,tmpY;
	char  str[2];
	
	menuInLayer = 3;

	#ifdef PLUG_IN_CARRIER_MODULE
	 guiLine(1,17,160,144,0);
  #else
	 guiLine(1,17,160,160,0);
  #endif
	
	guiLine(1,17,1,140,1);
	guiLine(160,17,160,140,1);
	guiLine(29,34,29,50,1);	
	guiLine(52,51,52,102,1);	
	guiLine(118,103,118,119,1);	
	guiLine(1,17,160,17,1);
	guiLine(1,34,160,34,1);	
	guiLine(1,51,160,51,1);	
	guiLine(1,68,160,68,1);	
	guiLine(1,85,160,85,1);
	guiLine(1,102,160,102,1);
	guiLine(80,119,80,140,1);
	guiLine(1,119,160,119,1);
	guiLine(1,140,160,140,1);

	guiDisplay(24,18,"以太网参数设置",1);
	guiDisplay(4, 35,"MAC",1);
	guiDisplay(45, 35,tmpEthMac,1);
	guiDisplay(4, 52,"IP地址",1);
	guiDisplay(4, 69,"掩  码",1);
	guiDisplay(4, 86,"网  关",1);
	guiDisplay(4,103,"以太网登录主站",1);

  tmpY += 51;
  
  tmpX = 54;
  for(i=0;i<15;i++)
  {
    str[0] = chrEthPara[0][i];
    str[1] = '\0';
    if (layer2Light==0 && layer3Light==i)
    {
      guiDisplay(tmpX,tmpY,str,0);
      tmpX += 8;
    }
    else
    {
      guiDisplay(tmpX,tmpY,str,1);
      tmpX += 7;
    }
  }
  tmpY += 17;
  
  tmpX = 54;
  for(i=0;i<15;i++)
  {
    str[0] = chrEthPara[1][i];
    str[1] = '\0';
    if (layer2Light==1 && layer3Light==i)
    {
      guiDisplay(tmpX,tmpY,str,0);
      tmpX += 8;
    }
    else
    {
      guiDisplay(tmpX,tmpY,str,1);
      tmpX += 7;
    }
  }
  tmpY += 17;

  tmpX = 54;
  for(i=0;i<15;i++)
  {
    str[0] = chrEthPara[2][i];
    str[1] = '\0';
    if (layer2Light==2 && layer3Light==i)
    {
      guiDisplay(tmpX,tmpY,str,0);
      tmpX += 8;
    }
    else
    {
      guiDisplay(tmpX,tmpY,str,1);
      tmpX += 7;
    }
  }
  tmpY += 17;
  
 #ifdef PLUG_IN_CARRIER_MODULE
  if (chrEthPara[3][0]==0x55)
  {
  	if (layer2Light==3)
  	{
  		guiDisplay(130,tmpY, "是", 0);
  	}
  	else
  	{
  		guiDisplay(130,tmpY, "是", 1);
  	}
  }
  else
  {
  	if (layer2Light==3)
  	{
  		guiDisplay(130, tmpY, "否", 0);
  	}
  	else
  	{
  		guiDisplay(130, tmpY, "否", 1);
  	}
  }
	if (layer2Light==4)
	{
	  guiDisplay(24, 122, "确定", 0);
	}
	else
	{
	  guiDisplay(24, 122, "确定", 1);
	}
	guiDisplay(104,122,"取消",1);
	lcdRefresh(17,144);

 #else
  
  guiDisplay(130,tmpY, "是", 1);  

	if (layer2Light==3)
	{
	  guiDisplay(24, 122, "确定", 0);
	}
	else
	{
	  guiDisplay(24, 122, "确定", 1);
	}
	guiDisplay(104,122,"取消",1);
	
	lcdRefresh(17,160);
 #endif
}

/*******************************************************
函数名称:adjustEthParaLight
功能描述:调整ETH参数高亮项
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void adjustEthParaLight(INT8U leftRight)
{
 	 INT8U tmpData;
 	 
 	 switch (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
 	 {
 	 	  case 0:
 	 	  case 1:
 	 	  case 2:
 	 	  	tmpData = 15;
 	 	  	break;
 	 	  
 	 	  default:
 	 	  	tmpData = 1;
 	 	  	break;
 	 }
 	 if (leftRight==1)  //右键
 	 {
 	   if (keyLeftRight>=tmpData-1)
 	   {
 	 	   keyLeftRight = 0;
 	   }
 	   else
 	   {
 	 	   keyLeftRight++;
 	   }
 	 }
 	 else
 	 {
 	   if (keyLeftRight==0)
 	   {
 	 	   keyLeftRight = tmpData-1;
 	   }
 	   else
 	   {
 	 	   keyLeftRight--;
 	   }
 	 }
	 
	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<3)
	 {
	 	 if ((keyLeftRight+1)%4==0 && keyLeftRight<16)
	 	 {
	 	   if (leftRight==1)
	 	   {
	 	     keyLeftRight++;
	 	   }
	 	   else
	 	   {
	 	   	 keyLeftRight--;
	 	   }
	 	 }
	 }
}

/*******************************************************
函数名称:adjustSetMeterParaLight
功能描述:调整设置电表参数高亮项
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void adjustSetMeterParaLight(INT8U leftRight,INT8U type)
{
 	 INT8U tmpData;
 	 
 	 if (type==1)
 	 {
 	   switch (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
 	   {
 	 	   case 0:
 	 	  	 tmpData = 4;
 	 	  	 break;
 	 	  	
 	 	   case 1:
 	 	   case 2:
 	 	   case 3:
 	 	     tmpData = 1;
 	 	     break;
 	 	    
 	 	   case 4:
 	 	  	 tmpData = 12;
 	 	  	 break;
 	 	  #ifndef PLUG_IN_CARRIER_MODULE
 	 	   case 5:
 	 	   case 6:
 	 	  	 tmpData = 2;
 	 	   	 break;
 	 	  #endif
 	 	 }
 	 }
 	 else
 	 {
 	   if (type==6)
 	   {
 	   	 tmpData = 2;
 	   }
 	   else
 	   {
   	   switch (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
   	   {
   	 	   case 0:
   	 	  	 tmpData = 4;
   	 	  	 break;
   	 	  	
   	 	   case 1:
   	 	   case 3:
   	 	   	 tmpData = 12;
   	 	   	 break;
   	 	   	 
   	 	   case 2:
   	 	   case 4:
   	 	     tmpData = 1;
   	 	     break;
  
   	 	   case 6:
   	 	   	 tmpData = 2;
   	 	   	 break;
   	 	 }
   	 }
 	 }
 	 if (leftRight==1)  //右键
 	 {
 	   if (keyLeftRight>=tmpData-1)
 	   {
 	 	   keyLeftRight = 0;
 	   }
 	   else
 	   {
 	 	   keyLeftRight++;
 	   }
 	 }
 	 else
 	 {
 	   if (keyLeftRight==0)
 	   {
 	 	   keyLeftRight = tmpData-1;
 	   }
 	   else
 	   {
 	 	   keyLeftRight--;
 	   }
 	 }	 
}

/*******************************************************
函数名称:adjustCasParaLight
功能描述:调整级联参数高亮项
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void adjustCasParaLight(INT8U leftRight)
{
 	 INT8U tmpData;
 	 
 	 switch (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
 	 {
 	 	  case 0:
 	 	  case 1:
 	 	  	tmpData = 1;
 	 	  	break;
 	 	  
 	 	  default:
 	 	  	tmpData = 9;
 	 	  	break;
 	 }
 	 if (leftRight==1)  //右键
 	 {
 	   if (keyLeftRight>=tmpData-1)
 	   {
 	 	   keyLeftRight = 0;
 	   }
 	   else
 	   {
 	 	   keyLeftRight++;
 	   }
 	 }
 	 else
 	 {
 	   if (keyLeftRight==0)
 	   {
 	 	   keyLeftRight = tmpData-1;
 	   }
 	   else
 	   {
 	 	   keyLeftRight--;
 	   }
 	 }
}

/*******************************************************
函数名称:setCascadePara
功能描述:设置级联参数(376.1国家电网规约(I型集中器/专变终端))
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void setCascadePara(INT8U layer2Light,INT8U layer3Light)
{
	INT8U i, tmpX;
	char  str[2];
	
	menuInLayer = 3;
  
  #ifdef PLUG_IN_CARRIER_MODULE
	 guiLine(1,17,160,144,0);
	#else
	 guiLine(1,17,160,160,0);
	#endif
	
	guiLine(1,17,1,140,1);
	guiLine(160,17,160,140,1);
	guiLine(69,34,69,67,1);	
	guiLine(45,69,45,119,1);	
	
	guiLine(1,17,160,17,1);
	guiLine(1,34,160,34,1);	
	guiLine(1,51,160,51,1);	
	guiLine(1,68,160,68,1);	
	guiLine(1,85,160,85,1);	
	guiLine(1,102,160,102,1);
	guiLine(80,119,80,140,1);
	guiLine(1,119,160,119,1);
	guiLine(1,140,160,140,1);

	guiDisplay(33,18,"级联参数设置",1);
	guiDisplay(4, 35,"级联端口",1);
	guiDisplay(4, 52,"级联标志",1);
	guiDisplay(4, 69,"终端1",1);
	guiDisplay(4, 86,"终端2",1);
	guiDisplay(4,103,"终端3",1);

  tmpX = 75;
  str[1] = '\0';
  for(i=0;i<1;i++)
  {
    str[0] = chrMp[0][i];
    if (layer2Light==0 && i==layer3Light)
    {
      guiDisplay(tmpX, 35, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 35, str, 1);
    }
    tmpX += 8;
  }
  
  if (layer2Light==1)
  {
    switch (chrMp[1][0])
    {
      case '1':
        guiDisplay(75, 52, "级联方", 0);
        break;
        
      case '2':
        guiDisplay(75, 52, "被级联方", 0);
        break;
        
      default:
        guiDisplay(75, 52, "不级联", 0);
        break;
    }
  }
  else
  {
    switch (chrMp[1][0])
    {
      case '1':
        guiDisplay(75, 52, "级联方", 1);
        break;
        
      case '2':
        guiDisplay(75, 52, "被级联方", 1);
        break;
        
      default:
        guiDisplay(75, 52, "不级联", 1);
        break;
    }
  }

  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<9;i++)
  {
    str[0] = chrMp[2][i];
    if (layer2Light==2 && i==layer3Light)
    {
      guiDisplay(tmpX, 69, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 69, str, 1);
    }
    tmpX += 8;
  }

  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<9;i++)
  {
    str[0] = chrMp[3][i];
    if (layer2Light==3 && i==layer3Light)
    {
      guiDisplay(tmpX, 86, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 86, str, 1);
    }
    tmpX += 8;
  }


  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<9;i++)
  {
    str[0] = chrMp[4][i];
    if (layer2Light==4 && i==layer3Light)
    {
      guiDisplay(tmpX, 103, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 103, str, 1);
    }
    tmpX += 8;
  }
  
	if (layer2Light==5)
	{
	  guiDisplay(24,122,"确定",0);
	}
	else
	{
	  guiDisplay(24,122,"确定",1);
	}
	guiDisplay(104,122,"取消",1);

  #ifdef PLUG_IN_CARRIER_MODULE
	 lcdRefresh(17,144);
	#else
	 lcdRefresh(17,160);
	#endif
}


/*******************************************************
函数名称:setMainTain
功能描述:维护口模式设置(376.1国家电网集中器规约)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void setMainTain(INT8U lightNum)
{
	INT8U i, row, col;
	char  str[2];
	
	menuInLayer = 3;


 #ifdef PLUG_IN_CARRIER_MODULE
	guiLine(1,17,160,144,0);
 #else
	guiLine(1,17,160,160,0);  
 #endif

	guiLine(1,50,1,130,1);
	guiLine(160,50,160,130,1);
	guiLine(1,130,160,130,1);
	guiLine(1,50,160,50,1);
	guiLine(1,70,160,70,1);

	guiDisplay(25,52,"维护口模式设置",1);
	switch (lightNum)
	{
	  case 0:
	    guiDisplay(24, 82, "   维护模式   ", 0);
	    guiDisplay(24,102, "   监视模式   ", 1);
	    break;
	  
	  case 1:
	    guiDisplay(24, 82, "   维护模式   ", 1);
	    guiDisplay(24,102, "   监视模式   ", 0);
	    break;
	}

 #ifdef PLUG_IN_CARRIER_MODULE
	lcdRefresh(17, 145);
 #else
	lcdRefresh(17, 160);
 #endif
}

/*******************************************************
函数名称:setRs485Port2
功能描述:第2路485设置(376.1国家电网集中器规约)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void setRs485Port2(INT8U lightNum)
{
	INT8U i, row, col;
	char  str[2];
	
	menuInLayer = 3;

 #ifdef PLUG_IN_CARRIER_MODULE
	guiLine(1,17,160,144,0);
 #else
	guiLine(1,17,160,160,0);
 #endif
	guiLine(1,50,1,130,1);
	guiLine(160,50,160,130,1);
	guiLine(1,130,160,130,1);
	guiLine(1,50,160,50,1);
	guiLine(1,70,160,70,1);

	guiDisplay(9,52,"第2路485口功能设置",1);
	switch (lightNum)
	{
	  case 0:
	    guiDisplay(24, 82, "   抄表接口   ", 0);
	    guiDisplay(24,102, "   维护接口   ", 1);
	    break;
	  
	  case 1:
	    guiDisplay(24, 82, "   抄表接口   ", 1);
	    guiDisplay(24,102, "   维护接口   ", 0);
	    break;
	}
 
 #ifdef PLUG_IN_CARRIER_MODULE
	lcdRefresh(17, 145);
 #else
	lcdRefresh(17, 160);
 #endif
}

/*******************************************************
函数名称:setLmProtocol
功能描述:本地通信模块协议设置(376.1国家电网集中器规约)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void setLmProtocol(INT8U lightNum)
{
	INT8U i, row, col;
	char  str[2];
	
	menuInLayer = 3;

	guiLine(1,17,160,144,0);

	guiLine(1,50,1,130,1);
	guiLine(160,50,160,130,1);
	guiLine(1,130,160,130,1);
	guiLine(1,50,160,50,1);
	guiLine(1,70,160,70,1);

	guiDisplay(1,52,"本地通信模块协议设置",1);
	switch (lightNum)
	{
	  case 0:
	    guiDisplay(24, 82, "Q/GDW376.2-2009", 0);
	    guiDisplay(24,102, "   透传协议    ", 1);
	    break;
	  
	  case 1:
	    guiDisplay(24, 82, "Q/GDW376.2-2009", 1);
	    guiDisplay(24,102, "   透传协议    ", 0);
	    break;
	}
 
	lcdRefresh(17, 145);
}


#ifdef LOAD_CTRL_MODULE
/*******************************************************
函数名称:ctrlAlarmDisplay
功能描述:控制过程显示
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void ctrlAlarmDisplay(void)
{
   INT8U        i,j,tmpLine,tmpClose,prevAlarm,tmpAlarm;
   INT8U        pn;
   INT32U       tmpSecond;
   INT32U       tmpKw;
   INT16U       tmpWatt;
   char         say[20],str[12];
   static INT8U k;
   DATE_TIME    tmpTime,readTime;
   INT8U        tmpSetLine,tmpPeriod;
   INT8U        dataBuff[512];
   INT8U        tmpX;
   
   //有键按下时,等待5秒后继续告警画面
   if (pageWait>5)
   {
   	  return;
   }
   
   //有控制投入命令时,为显示"控制投入命令",等待2秒后继续告警画面
   if (ctrlCmdWaitTime>3 && ctrlCmdWaitTime!=0xfe)
   {
   	  return;
   }
   
	 if (ctrlStatus.numOfAlarm>0)
	 {
	   tmpClose = 0;
	   tmpSetLine = 0;
	   
	   guiLine(1,17,160,160,0);  //清除显示区域
	   
	   menuInLayer = 99;         //处于告警画面

  	 switch (ctrlStatus.aQueue[ctrlStatus.nowAlarm])
  	 {
       case REMOTE_CTRL:       //遥控
       	 guiDisplay(48,LCD_LINE_1,"遥控跳闸",0);
       	 guiDisplay(1,LCD_LINE_3,"　①",1);
       	 guiDisplay(1,LCD_LINE_4,"轮②",1);
       	 guiDisplay(1,LCD_LINE_5,"次③",1);
       	 guiDisplay(1,LCD_LINE_6,"　④",1);
       	 tmpX = 36;
       	 tmpLine = LCD_LINE_3;
       	 
       	 for(i=0;i<CONTROL_OUTPUT;i++)
       	 {
       	 	  switch(remoteCtrlConfig[i].status)
       	 	  {
       	 	  	 case 0:
       	 	  	 	 guiDisplay(tmpX,tmpLine,"未控制",1);
       	 	  	 	 tmpClose++;
       	 	  	 	 break;
       	 	  	 	 
       	 	  	 case CTRL_ALARM:
       	 	  	 	 tmpSecond = delayedSpike(sysTime,remoteCtrlConfig[i].remoteStart);
       	 	  	 	 strcpy(say,"告警 ");
       	 	  	 	 strcat(say,digital2ToString(tmpSecond/60,str));
       	 	  	 	 strcat(say,":");
       	 	  	 	 strcat(say,digital2ToString(tmpSecond%60,str));
       	 	  	 	 guiDisplay(tmpX,tmpLine,say,1);
       	 	  	 	 break;

       	 	  	 case CTRL_JUMPED:
       	 	  	 	 tmpSecond = delayedSpike(sysTime,remoteCtrlConfig[i].remoteStart);
       	 	  	 	 strcpy(say,"已跳闸 ");
       	 	  	 	 strcat(say,digital2ToString(tmpSecond/3600,str));
       	 	  	 	 strcat(say,":");
       	 	  	 	 strcat(say,digital2ToString(tmpSecond%3600/60,str));
       	 	  	 	 strcat(say,":");
       	 	  	 	 strcat(say,digital2ToString(tmpSecond%3600%60,str));
       	 	  	 	 guiDisplay(tmpX,tmpLine,say,1);
       	 	  	 	 break;

       	 	  	 case CTRL_CLOSE_GATE:
       	 	  	 	 tmpClose++;
       	 	  	 	 guiDisplay(tmpX,tmpLine,"允许合闸",1);
       	 	  	 	 break;
       	 	  }
       	 	  
       	 	  tmpLine += 16;
       	 }
 	       
 	       if (tmpClose==CONTROL_OUTPUT)
   	     {
   	  	    ctrlStatus.allPermitClose[ctrlStatus.nowAlarm]=1;   //所有路处于合闸状态
   	     }
   	     else
   	     {
   	  	    ctrlStatus.allPermitClose[ctrlStatus.nowAlarm]=0;   	 	  
   	     }
       	 break;
       	 
       case POWER_CTRL:       //当前功率下浮控
       case OBS_CTRL:         //营业报停控
       case WEEKEND_CTRL:     //厂休控
       case TIME_CTRL:        //时段功控
  	 	   pn = ctrlStatus.pn[ctrlStatus.nowAlarm];
  	 	   
	       switch (ctrlStatus.aQueue[ctrlStatus.nowAlarm])
	       {
	         case POWER_CTRL:
	           strcpy(say,"功率下浮控 总加组");
	           tmpAlarm = powerDownCtrl[pn-1].pwrDownAlarm;
	           tmpTime  = powerDownCtrl[pn-1].alarmEndTime;
	           tmpKw = powerDownCtrl[pn-1].powerDownLimit;
	           tmpWatt = powerDownCtrl[pn-1].powerLimitWatt;
	           break;

	         case OBS_CTRL:
	           strcpy(say,"营业报停控 总加组");
	           tmpAlarm = obsCtrlConfig[pn-1].obsAlarm;
	           tmpTime  = obsCtrlConfig[pn-1].alarmEndTime;
	           tmpKw = obsCtrlConfig[pn-1].obsLimit;
	           break;
	           
	         case WEEKEND_CTRL:
	           strcpy(say,"厂休功控　 总加组");
	           tmpAlarm = wkdCtrlConfig[pn-1].wkdAlarm;
	           tmpTime  = wkdCtrlConfig[pn-1].alarmEndTime;
	           tmpKw = wkdCtrlConfig[pn-1].wkdLimit;
	           break;

	         case TIME_CTRL:
	           strcpy(say,"时段功控　 总加组");
	           tmpAlarm = periodCtrlConfig[pn-1].ctrlPara.prdAlarm;
	           tmpTime  = periodCtrlConfig[pn-1].ctrlPara.alarmEndTime;
	           
             tmpKw = 0;
             
             //获取当前时间段的时段索引
             if ((tmpPeriod = getPowerPeriod(sysTime)) != 0)
             {
               if (getPowerLimit(pn, periodCtrlConfig[pn-1].ctrlPara.ctrlPeriod, tmpPeriod, (INT8U *)&tmpKw)==FALSE)
               {
               	  tmpKw = 0;
               }
             }
	           break;
	       }
	 	  	 strcat(say,digital2ToString(pn,str));
	       guiDisplay(5,LCD_LINE_1,say,0);
	       
         if (ctrlStatus.aQueue[ctrlStatus.nowAlarm]!=POWER_CTRL)
         {
           tmpKw = countAlarmLimit((INT8U *)&tmpKw, 0x2, 0x0,&tmpWatt);
         }
 	       strcpy(say,"定值");
 	       if (tmpKw>9999999 && tmpWatt==0)
 	       {
 	         tmpKw/=1000;
 	         strcat(say,intToString(tmpKw,3,str));
           strcat(say,"Mw");
 	         guiDisplay(1,LCD_LINE_2,say,1);
 	       }
 	       else
 	       {
 	         strcat(say,intToString(tmpKw,3,str));
 	         if (tmpWatt>0)
 	         {
 	           strcat(say,".");
 	           strcat(say,intToString(tmpWatt,3,str));
 	         }
           strcat(say,"Kw");
 	         guiDisplay(33,LCD_LINE_2,say,1);
 	       }
   	     
   	     tmpKw = 0x0;
  	     if (calcRealPower(dataBuff, pn))
         {
    	     if (dataBuff[GP_WORK_POWER+1] != 0xFF && dataBuff[GP_WORK_POWER+1] != 0xee)
    	     {
             tmpKw  = dataBuff[GP_WORK_POWER+1];
             tmpKw |= (dataBuff[GP_WORK_POWER+2]&0xF)<<8;
             tmpKw |= (dataBuff[GP_WORK_POWER]&0x10)<<8;
             tmpKw |= (dataBuff[GP_WORK_POWER]&0x07)<<13;
             tmpKw  = countAlarmLimit((INT8U*)&tmpKw, 0x2, 0x00,&tmpWatt);
           }
         }
	      
	       strcpy(say,"当前");
	       strcat(say,intToString(tmpKw,3,str));
	       if (tmpWatt>0)
	       {
	         strcat(say,".");
	         tmpWatt /=10;
	         if (tmpWatt<10)
	         {
	         	  strcat(say,"0");
	         }
	         strcat(say,intToString(tmpWatt,3,str));
	       }
         strcat(say,"Kw");
         
	       guiDisplay(33,LCD_LINE_3,say,1);
	       
	       guiDisplay(33,LCD_LINE_5,"  ①",1);
	       guiDisplay(33,LCD_LINE_6,"轮②",1);
	       guiDisplay(33,LCD_LINE_7,"次③",1);
	       guiDisplay(33,LCD_LINE_8,"  ④",1);
	       
	       prevAlarm = 0;
	       for(j=0;j<CONTROL_OUTPUT;j++)
	       {
     	     strcpy(say,"");
  
     	     if (((powerCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x00)
     	     {
     	     	  strcpy(say,"未设定");
     	     }
     	     else
     	     {
    	       tmpSetLine++;
    	       if (((powerCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x01)
    	       {
    	       	  strcpy(say,"已跳闸");
    	       }
    	       else
    	       {
    	       	  switch(tmpAlarm)
    	       	  {
    	       	  	 case CTRL_ALARM:
  	 	  	 	         if (prevAlarm==0)
  	 	  	 	         {
  	 	  	 	           tmpSecond = delayedSpike(sysTime,tmpTime);
  	 	  	 	           strcpy(say,"告警");
  	 	  	 	           strcat(say,digital2ToString(tmpSecond/60,str));
  	 	  	 	           strcat(say,":");
  	 	  	 	           strcat(say,digital2ToString(tmpSecond%60,str));
  	 	  	 	           prevAlarm++;
  	 	  	 	         }
  	 	  	 	         else
  	 	  	 	         {
  	 	  	 	      	    strcpy(say,"等待");
  	 	  	 	         }
  	 	  	 	         break;
  	 	  	 	       
  	 	  	 	       case CTRL_JUMPED:
    	       	  	   strcpy(say,"等待");
    	       	  	   break;
    	       	  	 
    	       	  	 case CTRL_CLOSE_GATE:
    	       	  	   strcpy(say,"允许合闸");
    	       	  	   tmpClose++;
                    break;
                    
                  case CTRL_ALARM_CANCEL:
    	       	  	   strcpy(say,"告警取消");
    	       	  	   tmpClose++;
                  	 break;

                  case CTRL_ALARM_AUTO_CLOSE:
    	       	  	   strcpy(say,"自动合闸");
    	       	  	   tmpClose++;
                  	 break;
                  
                  default:
    	       	  	   strcpy(say,"合闸");
    	       	  	   tmpClose++;
                  	 break;
               }
    	       }
    	     }
    	     switch(j)
  	 	   	 {
  	 	   	 	  case 0:
  	 	   	 	  	guiDisplay(65,LCD_LINE_5,say,1);
  	 	   	 	  	break;
  	 	   	 	  case 1:
  	 	   	 	  	guiDisplay(65,LCD_LINE_6,say,1);
  	 	   	 	  	break;
  	 	   	 	  case 2:
  	 	   	 	  	guiDisplay(65,LCD_LINE_7,say,1);
  	 	   	 	  	break;
  	 	   	 	  case 3:
  	 	   	 	  	guiDisplay(65,LCD_LINE_8,say,1);
  	 	   	 	  	break;
  	 	   	 }
	 	   	 }
	 	   	 if (tmpClose == tmpSetLine)
	 	   	 {
	 	   	 	  ctrlStatus.allPermitClose[ctrlStatus.nowAlarm]=1;   //所有设定控制路处于合闸状态
	 	   	 }
	 	   	 else
	 	   	 {
            ctrlStatus.allPermitClose[ctrlStatus.nowAlarm]=0;
	 	   	 }
       	 break;

       case MONTH_CTRL:       //月电控
       case CHARGE_CTRL:      //购电控
	 	     pn = ctrlStatus.pn[ctrlStatus.nowAlarm];
   	 	   
 	       switch (ctrlStatus.aQueue[ctrlStatus.nowAlarm])
 	       {
 	         case MONTH_CTRL:
 	           strcpy(say,"月电控　　 总加组");
 	           tmpAlarm = monthCtrlConfig[pn-1].monthAlarm;
 	           tmpTime  = monthCtrlConfig[pn-1].mthAlarmTimeOut;
 	           tmpKw = monthCtrlConfig[pn-1].monthCtrl;
 	           break;
 	         
 	         case CHARGE_CTRL:
 	           strcpy(say,"购电控　　 总加组");
 	           tmpAlarm = chargeCtrlConfig[pn-1].chargeAlarm;
 	           tmpTime  = chargeCtrlConfig[pn-1].alarmTimeOut;
 	           tmpKw = chargeCtrlConfig[pn-1].cutDownLimit;
 	           break;
 	       }
 	 	  	 strcat(say,digital2ToString(pn,str));
 	       guiDisplay(5,LCD_LINE_1,say,0);
 	       
 	       if (ctrlStatus.aQueue[ctrlStatus.nowAlarm]==MONTH_CTRL)
 	       {
 	       	  strcpy(say,"限值");
 	       }
 	       else
 	       {
    	      strcpy(say,"跳闸");
    	      
            if (tmpKw&0x10000000)
    	      {
    	 	      strcat(say, "-");
    	      }
    	   }
    	   tmpKw = countAlarmLimit((INT8U *)&tmpKw, 0x3, 0x0,&tmpWatt);
 	       if (tmpKw>999999)
 	       {
 	          tmpKw/=1000;
 	          strcat(say,intToString(tmpKw,3,str));
            strcat(say,"Mwh");
 	       }
 	       else
 	       {
 	          strcat(say,intToString(tmpKw,3,str));
            strcat(say,"Kwh");
 	       }
 	       guiDisplay(33,LCD_LINE_2,say,1);

 	       if (ctrlStatus.aQueue[ctrlStatus.nowAlarm]==MONTH_CTRL)
 	       {
       	   strcpy(say,"当前");
       	   readTime = timeHexToBcd(sysTime);
           if (readMeterData(dataBuff, pn, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &readTime, 0) == TRUE)	
           {
               if (dataBuff[GP_MONTH_WORK+3] != 0xFF)
               {
                 tmpKw = dataBuff[GP_MONTH_WORK+3]
                          | dataBuff[GP_MONTH_WORK+4] << 8 
                          | dataBuff[GP_MONTH_WORK+5] << 16 
                          | (((dataBuff[GP_MONTH_WORK]&0x01)<<6)|(dataBuff[GP_MONTH_WORK]&0x10)|(dataBuff[GP_MONTH_WORK+6]&0x0f))<<24;
               }
           }
 	       }
 	       else
 	       {
            strcpy(say,"剩余");
       	    readTime = timeHexToBcd(sysTime);
            if (readMeterData(dataBuff, pn, LEFT_POWER, 0x0, &readTime, 0)==TRUE)
            {
              if (dataBuff[0] != 0xFF || dataBuff[0] != 0xEE)
              {
                //当前剩余电量
                tmpKw = dataBuff[0] 
                      | dataBuff[1] << 8 
                      | dataBuff[2] << 16 
                      | (dataBuff[3]&0xf) << 24;
              }
            }
 	       } 	       
    	   tmpKw = countAlarmLimit((INT8U *)&tmpKw, 0x3, 0x0,&tmpWatt);
    	   if (dataBuff[3]&0x10)
    	   {
    	   	 strcat(say, "-");
    	   }
 	       strcat(say,intToString(tmpKw,3,str));
         strcat(say,"Kwh");
         
 	       guiDisplay(33,LCD_LINE_3,say,1);
 	       
 	       guiDisplay(33,LCD_LINE_5,"  ①",1);
 	       guiDisplay(33,LCD_LINE_6,"轮②",1);
 	       guiDisplay(33,LCD_LINE_7,"次③",1);
 	       guiDisplay(33,LCD_LINE_8,"  ④",1);
 	       
 	       prevAlarm = 0;
 	       for(j=0;j<CONTROL_OUTPUT;j++)
 	       {
    	     strcpy(say,"");

    	     if (((electCtrlRoundFlag[pn-1].flag>>j) & 0x01)==0x00)
    	     {
    	     	  strcpy(say,"未设定");
    	     }
    	     else
    	     {
     	       tmpSetLine++;
     	       if (((electCtrlRoundFlag[pn-1].ifJumped>>j) & 0x01)==0x01)
     	       {
     	       	  strcpy(say,"已跳闸");
     	       }
     	       else
     	       {
     	       	  switch(tmpAlarm)
     	       	  {
     	       	  	 case CTRL_ALARM:
   	 	  	 	         if (prevAlarm==0)
   	 	  	 	         {
   	 	  	 	           tmpSecond = delayedSpike(tmpTime,sysTime);
   	 	  	 	           strcpy(say,"告警");
   	 	  	 	           strcat(say,digital2ToString(tmpSecond/60,str));
   	 	  	 	           strcat(say,":");
   	 	  	 	           strcat(say,digital2ToString(tmpSecond%60,str));
   	 	  	 	           prevAlarm++;
   	 	  	 	         }
   	 	  	 	         else
   	 	  	 	         {
   	 	  	 	      	    strcpy(say,"等待");
   	 	  	 	         }
   	 	  	 	         break;
   	 	  	 	       
   	 	  	 	       case CTRL_JUMPED:
     	       	  	   strcpy(say,"等待");
     	       	  	   break;
     	       	  	 
     	       	  	 case CTRL_CLOSE_GATE:
     	       	  	   strcpy(say,"允许合闸");
     	       	  	   tmpClose++;
                     break;
                     
                   case CTRL_ALARM_CANCEL:
     	       	  	   strcpy(say,"告警取消");
     	       	  	   tmpClose++;
                   	 break;

                   case CTRL_ALARM_AUTO_CLOSE:
     	       	  	   strcpy(say,"自动合闸");
     	       	  	   tmpClose++;
                   	 break;

                  default:
    	       	  	   strcpy(say,"合闸");
    	       	  	   tmpClose++;
                  	 break;
                }
     	       }
     	     }
     	     
     	     switch(j)
   	 	   	 {
   	 	   	 	 case 0:
   	 	   	 	   guiDisplay(65,LCD_LINE_5,say,1);
   	 	   	 	   break;
   	 	   	 	 
   	 	   	 	 case 1:
   	 	   	 	   guiDisplay(65,LCD_LINE_6,say,1);
   	 	   	 	   break;
   	 	   	 	 
   	 	   	 	 case 2:
   	 	   	 	   guiDisplay(65,LCD_LINE_7,say,1);
   	 	   	 	   break;
   	 	   	 	 
   	 	   	 	 case 3:
   	 	   	 	   guiDisplay(65,LCD_LINE_8,say,1);
   	 	   	 	   break;         	 	   	 	  	
   	 	   	 }
 	 	   	 }
 	 	   	 if (tmpClose == tmpSetLine)
 	 	   	 {
 	 	   	 	   ctrlStatus.allPermitClose[ctrlStatus.nowAlarm]=1;   //所有设定控制路处于合闸状态
 	 	   	 }
 	 	   	 else
 	 	   	 {
            ctrlStatus.allPermitClose[ctrlStatus.nowAlarm]=0;
 	 	   	 }
       	 break;
       	 
       case REMINDER_FEE:
       	 guiDisplay(15,LCD_LINE_2,"催费告警",1);
       	 break;
       
       case PAUL_ELECTRICITY:
       	 guiDisplay(40,LCD_LINE_2,"终端保电",1);
       	 
       	 tmpSecond = delayedSpike(staySupportStatus.endStaySupport,sysTime);
       	 strcpy(say,"");
       	 strcat(say,digital2ToString(tmpSecond/3600,str));
       	 strcat(say,":");
       	 strcat(say,digital2ToString(tmpSecond%3600/60,str));
       	 strcat(say,":");
       	 strcat(say,digital2ToString(tmpSecond%3600%60,str));
       	 guiDisplay(44,LCD_LINE_3,say,1);
       	 break;
 	   }
 	   
     k++;
     if (k>2)
     {
       k=0;
       ctrlStatus.nowAlarm++;
  	   if (ctrlStatus.nowAlarm>=ctrlStatus.numOfAlarm)
  	   {
  	 	    ctrlStatus.nowAlarm = 0;
  	   }
  	 }
  	 
  	 lcdRefresh(17,160);
   }
}
#endif //LOAD_CTRL_MODULE


//各种机型公共函数************************end***************************


/*******************************************************
函数名称:refreshTitleTime
功能描述:刷新抬头显示时间(376.1集中器通用)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void refreshTitleTime(void)
{
  char     str[6];

  #ifdef LOAD_CTRL_MODULE
    guiLine(103,1,160,15,0);
    guiAscii(103, 1, digital2ToString(sysTime.hour,str),1);
    guiAscii(118, 1, ":", 1);
    guiAscii(124, 1, digital2ToString(sysTime.minute,str),1);
    guiAscii(139, 1, ":", 1);
    guiAscii(145, 1, digital2ToString(sysTime.second,str),1);
  #else
    guiLine(124,1,160,16,0);
    guiAscii(124, 1, digital2ToString(sysTime.hour,str),1);
    guiAscii(139, 1, ":", 1);
    guiAscii(145, 1, digital2ToString(sysTime.minute,str),1);
  #endif
  
  lcdRefresh(1,16);
}

/*******************************************************
函数名称:layer1Menu
功能描述:一层菜单(376.1集中器通用)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void layer1Menu(int	num)
{
	 int i=0;
	 
	 INT8U startX,startY;
   
   #ifdef PLUG_IN_CARRIER_MODULE
	  #ifdef MENU_FOR_CQ_CANON
	   startX = 37;
	   startY = 28;
	   guiLine(1,17,160,160,0);
	  #else
	   startX = 24;
	   startY = LCD_LINE_2;
	   guiLine(1,17,160,144,0);
	  #endif
	 #else
	  startX = 40;
	  startY = 38;
	  guiLine(1,17,160,160,0);
	  
	  guiDisplay(56,19,"主菜单",1);
	 #endif
	 
	 menuInLayer=1;
   
   for(i=0;i<MAX_LAYER1_MENU;i++)
   {
     if (i == num)
     {
        guiDisplay(startX,startY+i*17,layer1MenuItem[i],0);
     }
     else
     {
      	guiDisplay(startX,startY+i*17,layer1MenuItem[i],1);
     }
   }

  #ifdef PLUG_IN_CARRIER_MODULE
   #ifndef MENU_FOR_CQ_CANON
    lcdRefresh(17,145);
   #else
    lcdRefresh(17,160);
   #endif
  #else
   lcdRefresh(17,160);
  #endif
}

/*******************************************************
函数名称:freeQueryMpLink
功能描述:释放查询参数的测量点链表
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void freeQueryMpLink(void)
{
	 struct cpAddrLink    *tmpNode;

   while (queryMpLink!=NULL)
   {
     tmpNode = queryMpLink;
     queryMpLink = queryMpLink->next;
     
     free(tmpNode);
   }
}

/*******************************************************
函数名称:terminalInfo
功能描述:集中器本机信息
调用函数:
被调用函数:
输入参数:num
输出参数:
返回值:void
*******************************************************/
void terminalInfo(INT8U num)
{
   INT8U  i;
   INT8U  tmpY;
   char   str[35],say[50];
	 char   sayStr[10];
	 INT32U integer,decimal;
	 INT32U disData;
	 INT8U  sign;
	 INT8U  dataBuff[LENGTH_OF_PARA_RECORD];
   INT8U  visionBuff[LENGTH_OF_ENERGY_RECORD];
   INT8U  reqBuff[LENGTH_OF_REQ_RECORD];
   INT16U offset;

	 #ifdef LOAD_CTRL_MODULE
	  menuInLayer = 2;
	  guiLine(1,17,160,160,0);
	 #else
	  menuInLayer = 3;
	  guiLine(1,17,160,144,0);
	 #endif

	 switch(num)
	 {
	   case 0:      //显示版本信息
       strcpy(say, "终端地址:");
       #ifdef TE_ADDR_USE_BCD_CODE
        strcat(say,int8uToHex(addrField.a2[1],str));
        strcat(say,int8uToHex(addrField.a2[0],str));
       #else
        strcat(say,intToString(addrField.a2[1]<<8 | addrField.a2[0],3,str));
        strcat(say,"[");
        strcat(say,int8uToHex(addrField.a2[1],str));
        strcat(say,int8uToHex(addrField.a2[0],str));
        strcat(say,"]");
       #endif
	  	 #ifdef PLUG_IN_CARRIER_MODULE
        guiDisplay(1,LCD_LINE_1+8,say,1);
	  	 #else
        guiDisplay(1,LCD_LINE_1+4,say,1);
       #endif
       
       strcpy(say,"行政区划码:");
       strcat(say,digitalToChar(addrField.a1[1]>>4));
       strcat(say,digitalToChar(addrField.a1[1]&0xf));
       strcat(say,digitalToChar(addrField.a1[0]>>4));
       strcat(say,digitalToChar(addrField.a1[0]&0xf));
	  	 #ifdef PLUG_IN_CARRIER_MODULE
        guiDisplay(1,LCD_LINE_2+8,say,1);
       #else
        guiDisplay(1,LCD_LINE_2+4,say,1);
       #endif
       
	  	 	 
	  	 strcpy(say,"软件版本:");
	  	 strcat(say,vers);
	  	 
	  	 #ifdef PLUG_IN_CARRIER_MODULE
	  	  guiDisplay(1,LCD_LINE_4,say,1);
	  	 #else
	  	  guiDisplay(1,LCD_LINE_4-8,say,1);
	  	 #endif
	  	 
	  	 strcpy(say,"发布日期:");
	  	 strcat(say,dispenseDate);
	  	 #ifdef PLUG_IN_CARRIER_MODULE
	  	  guiDisplay(1,LCD_LINE_5,say,1);
	  	 #else
	  	  guiDisplay(1,LCD_LINE_5-8,say,1);
	  	 #endif
	  	 
	  	 strcpy(say,"硬件版本:");
	  	 strcat(say,hardwareVers);
	  	 #ifdef PLUG_IN_CARRIER_MODULE
	  	  guiDisplay(1,LCD_LINE_6,say,1);
	  	 #else
	  	  guiDisplay(1,LCD_LINE_6-8,say,1);
	  	 #endif
	  	 
	  	 strcpy(say,"发布日期:");
	  	 strcat(say,hardwareDate);
	  	 #ifdef PLUG_IN_CARRIER_MODULE
	  	  guiDisplay(1,LCD_LINE_7,say,1);
	  	 #else
	  	  guiDisplay(1,LCD_LINE_7-8,say,1);
	  	 #endif
	  	 
	  	 #ifndef PLUG_IN_CARRIER_MODULE
	  	  guiDisplay(1,LCD_LINE_8,"当前时间:",1);
	  	  sprintf(say,"20%02d-%02d-%02d %02d:%02d:%02d",sysTime.year,sysTime.month,sysTime.day,sysTime.hour,sysTime.minute,sysTime.second);
	  	  guiDisplay(1,LCD_LINE_9,say,1);
	  	 #endif
	  	 break;
	  	 	 
	   case 1:      //网络指示
     	 strcpy(say,"主IP:");
     	 strcat(say,intToIpadd(ipAndPort.ipAddr[0]<<24 | ipAndPort.ipAddr[1]<<16 | ipAndPort.ipAddr[2]<<8 | ipAndPort.ipAddr[3],str));
     	 guiDisplay(1,LCD_LINE_1,say,1);
     	 strcpy(say,"备IP:");
     	 strcat(say,intToIpadd(ipAndPort.ipAddrBak[0]<<24 | ipAndPort.ipAddrBak[1]<<16 | ipAndPort.ipAddrBak[2]<<8 | ipAndPort.ipAddrBak[3],str));
     	 guiDisplay(1,LCD_LINE_2,say,1);
     	 strcpy(say,"端口:主");
     	 strcat(say,intToString(ipAndPort.port[1]<<8 | ipAndPort.port[0],3,str));
     	 //guiDisplay(1,LCD_LINE_3,say,1);
     	 strcat(say," 备");
     	 strcat(say,intToString(ipAndPort.portBak[1]<<8 | ipAndPort.portBak[0],3,str));
     	 guiDisplay(1,LCD_LINE_3,say,1);
       switch (moduleType)
       {
         case GPRS_SIM300C:
         case GPRS_GR64:
         case CDMA_DTGS800:
         case GPRS_M590E:
         case GPRS_M72D:
				 case LTE_AIR720H:
           strcpy(say,"APN:");
           strcat(say,(char *)ipAndPort.apn);
           guiAscii(1,LCD_LINE_4,say,1);
           break;
       }
	  	 
	  	 strcpy(say,"");
	  	 switch(moduleType)
	  	 {
	  	 	 case GPRS_SIM300C:
	  	 	 case GPRS_GR64:
	  	 	 case GPRS_M590E:
	  	 	 case GPRS_M72D:
      	   strcpy(say,"GPRS:");
      	   break;
				 
	  	 	 case LTE_AIR720H:
      	   strcpy(say,"LTE:");
      	   break;
	  	 	 	
	  	 	 case CDMA_DTGS800:
	  	 	 case CDMA_CM180:
      	   strcpy(say,"CDMA:");
      	   break;
	  	 }
	  	 if(wlModemFlag.loginSuccess == TRUE)
	  	 {
	  	 	 strcat(say,"已登录成功");
	  	 }
	  	 else
	  	 {
	  	 	 strcat(say,"登录未成功");
	  	 }
	  	 guiDisplay(1,LCD_LINE_5,say,1);

	  	 	 
	  	 strcpy(say,"上次心跳:");
	  	 if (lastHeartBeat.month == 0 && lastHeartBeat.day == 0 && lastHeartBeat.hour == 0 && lastHeartBeat.minute == 0 && lastHeartBeat.second == 0)
	  	 {
	  	 	 strcat(say,"未发送心跳");
	  	 }
	  	 else
	  	 {
	  	 	 strcat(say,digital2ToString(lastHeartBeat.month,str));
	  	 	 strcat(say,"-");
	  	 	 strcat(say,digital2ToString(lastHeartBeat.day,str));
	  	 	 strcat(say," ");
	  	 	 strcat(say,digital2ToString(lastHeartBeat.hour,str));
	  	 	 strcat(say,":");
	  	 	 strcat(say,digital2ToString(lastHeartBeat.minute,str));
	  	 }
	  	 guiDisplay(1,LCD_LINE_6,say,1);

	 	   #ifdef PLUG_IN_CARRIER_MODULE
	 	    guiDisplay(1, LCD_LINE_7, "集中器IP:", 1);
	 	   #else
	 	    guiDisplay(1, LCD_LINE_7, "终端IP:", 1);
	 	   #endif
	 	   
	 	   guiDisplay(20, LCD_LINE_8, intToIpadd(wlLocalIpAddr,str), 1);
	  	 break;
	  	 	 
   	 case 2:
   	   tmpY = LCD_LINE_1+8;
   	  #ifdef LOAD_CTRL_MODULE
   	   for(i=1;i<3;i++)
   	  #else
   	   for(i=1;i<4;i++)
   	  #endif
   	   {
   	     strcpy(say,"端口");
   	     if (i<3)
   	     {
   	       strcat(say,intToString(i+1,3,str));
   	     }
   	     else
   	     {
   	     	 strcat(say,"31");
   	     }
   	     strcat(say,"抄表间隔:");
   	     strcat(say,intToString(teCopyRunPara.para[i].copyInterval,3,str));
   	     guiDisplay(1,tmpY,say,1);
   	     tmpY += 16;
   	     
 	       strcpy(say,"下次:");
 	       strcat(say,digital2ToString(copyCtrl[i].nextCopyTime.month,str));
 	       strcat(say,"-");
 	       strcat(say,digital2ToString(copyCtrl[i].nextCopyTime.day,str));
	       strcat(say," ");
 	       strcat(say,digital2ToString(copyCtrl[i].nextCopyTime.hour,str));
 	       strcat(say,":");
 	       strcat(say,digital2ToString(copyCtrl[i].nextCopyTime.minute,str));
	       strcat(say,":");
 	       strcat(say,digital2ToString(copyCtrl[i].nextCopyTime.second,str));
 	       guiDisplay(1,tmpY,say,1);
 	       tmpY += 24;
 	     }
	  	 break;
	  
	  #ifdef PLUG_IN_CARRIER_MODULE
	   case 3:
	   	 guiDisplay(17, 17, "本地通信模块信息",0);
	   	 if (carrierModuleType==NO_CARRIER_MODULE)
	   	 {
	   	  #ifdef LM_SUPPORT_UT
	   	   if (0x55==lmProtocol)
	   	   {
	   	     guiDisplay(1, 65, "透传协议本地通信模块", 1);
	   	   }
	   	   else
	   	   {
	   	  #endif
	   	  
	   	     guiDisplay(1, 65, "未检测到本地通信模块", 1);
	   	  
	   	  #ifdef LM_SUPPORT_UT
	   	   }
	   	  #endif
	   	 }
	   	 else
	   	 {
	   	   strcpy(say,"厂商代码:");
	   	   say[9] = carrierFlagSet.productInfo[0];
	   	   say[10] = carrierFlagSet.productInfo[1];
	   	   say[11] = '\0';
	   	  
	   	  #ifndef DKY_SUBMISSION 
	   	   switch(carrierModuleType)
	   	   {
	   	   	 case EAST_SOFT_CARRIER:
	   	   	 	 strcat(say,"东软");
	   	   	 	 break;
	   	   	 case CEPRI_CARRIER:
	   	   	 	 strcat(say,"电科院");
	   	   	 	 break;
	   	   	 case SR_WIRELESS:
	   	   	 	 strcat(say,"桑锐");
	   	   	 	 break;
	   	   	 case RL_WIRELESS:
	   	   	 	 strcat(say,"锐拔");
	   	   	 	 break;
	   	   	 case MIA_CARRIER:
	   	   	 	 strcat(say,"弥亚微");
	   	   	 	 break;
	   	   	 case TC_CARRIER:
	   	   	 	 strcat(say,"鼎信");
	   	   	 	 break;
	   	   	 case LME_CARRIER:
	   	   	 	 strcat(say,"力合微");
	   	   	 	 break;
	   	   	 case FC_WIRELESS:
	   	   	 	 strcat(say,"友迅达");
	   	   	 	 break;
	   	   	 case SC_WIRELESS:
	   	   	 	 strcat(say,"赛康");
	   	   	 	 break;
	   	   }
	   	  #endif
	   	   guiDisplay(1, 33, say, 1);

	   	   strcpy(say,"芯片代码:");
	   	   say[9] = carrierFlagSet.productInfo[2];
	   	   say[10] = carrierFlagSet.productInfo[3];
	   	   say[11] = '\0';
	   	   guiDisplay(1, 49, say, 1);
	   	 
	   	   sprintf(say,"版本:%02x%02x", carrierFlagSet.productInfo[8], carrierFlagSet.productInfo[7]);
	   	   guiDisplay(1, 65, say, 1);
	   	   sprintf(say,"版本日期:%02x-%02x-%02x", carrierFlagSet.productInfo[6], carrierFlagSet.productInfo[5], carrierFlagSet.productInfo[4]);
	   	   guiDisplay(1, 81, say, 1);
	   	   guiDisplay(1, 96, "当前抄读方式:", 1);
	   	   if (localCopyForm==0xaa)
	   	   {
	   	     strcpy(say,"路由主导抄读");
	   	   }
	   	   else
	   	   {
	   	     strcpy(say,"集中器主导抄读");
	   	   }
	   	   guiDisplay(33, 112, say, 1);
	   	   
	   	   if (carrierModuleType==SR_WIRELESS || carrierModuleType==FC_WIRELESS)
	   	   {
	   	     if (carrierFlagSet.wlNetOk==3)
	   	     {
	   	     	 guiDisplay(17, 128, "模块组网完成", 1);
	   	     }
	   	     else
	   	     {
	   	     	 guiDisplay(17, 128, "模块组网未完成", 1);
	   	     }
	   	   }
	   	 }
	   	 break;
	   	 
	   case 4:
       memset(dataBuff,0xee,LENGTH_OF_PARA_RECORD);
       covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
	   	 //guiDisplay( 1,17,"  本机交采实时数据  ",0);

	     tmpY = 17;
	     for(i=0;i<8;i++)
	     {
	    	 sign = 0;
    	   switch(i)
    	   {
    	     case 0:
	    	     disData = dataBuff[POWER_INSTANT_WORK] | dataBuff[POWER_INSTANT_WORK+1]<<8 | dataBuff[POWER_INSTANT_WORK+2]<<16;
    	       strcpy(say,"有功功率总=");
    	       break;
    	       
    	     case 1:
	    	     disData = dataBuff[POWER_PHASE_A_WORK] | dataBuff[POWER_PHASE_A_WORK+1]<<8 | dataBuff[POWER_PHASE_A_WORK+2]<<16;
    	       strcpy(say,"A=");
    	       break;
    	       
    	     case 2:
	    	     disData = dataBuff[POWER_PHASE_B_WORK] | dataBuff[POWER_PHASE_B_WORK+1]<<8 | dataBuff[POWER_PHASE_B_WORK+2]<<16;
    	       strcpy(say,"B=");
    	       break;
    	       
    	     case 3:
	    	     disData = dataBuff[POWER_PHASE_C_WORK] | dataBuff[POWER_PHASE_C_WORK+1]<<8 | dataBuff[POWER_PHASE_C_WORK+2]<<16;
    	       strcpy(say,"C=");
    	       break;

    	     case 4:
	    	     disData = dataBuff[POWER_INSTANT_NO_WORK] | dataBuff[POWER_INSTANT_NO_WORK+1]<<8 | dataBuff[POWER_INSTANT_NO_WORK+2]<<16;
    	       strcpy(say,"无功功率总=");
    	       break;
    	       
    	     case 5:
	    	     disData = dataBuff[POWER_PHASE_A_NO_WORK] | dataBuff[POWER_PHASE_A_NO_WORK+1]<<8 | dataBuff[POWER_PHASE_A_NO_WORK+2]<<16;
    	       strcpy(say,"A=");
    	       break;
    	       
    	     case 6:
	    	     disData = dataBuff[POWER_PHASE_B_NO_WORK] | dataBuff[POWER_PHASE_B_NO_WORK+1]<<8 | dataBuff[POWER_PHASE_B_NO_WORK+2]<<16;
    	       strcpy(say,"B=");
    	       break;
    	       
    	     case 7:
	    	     disData = dataBuff[POWER_PHASE_C_NO_WORK] | dataBuff[POWER_PHASE_C_NO_WORK+1]<<8 | dataBuff[POWER_PHASE_C_NO_WORK+2]<<16;
    	       strcpy(say,"C=");
    	       break;
    	   }
	    	 
	    	 if (disData&0x800000)
	    	 {
	    	 	 disData &= 0x7fffff;
	    	 	 sign = 1;
	    	 }

         decimal = (disData>>12 & 0xf)*1000
                   +(disData>>8 & 0xf)*100
                   +(disData>>4 & 0xf)*10
                   +(disData & 0xf);
         integer = (disData>>20 & 0xf)*10+(disData>>16 & 0xf);
         if (sign==1)
         {
         	 strcat(say,"-");
         }
         strcat(say,floatToString(integer,decimal,4,4,str));

    	   if (i==0 || i==4)
    	   {
    	      guiDisplay(1, tmpY,say,1);
    	   }
    	   else
    	   {
    	      guiDisplay(10, tmpY,say,1);    	   	 
    	   }
    	   
    	   tmpY += 16;
	     }
	   	 break;

	   case 5:
       memset(dataBuff,0xee,LENGTH_OF_PARA_RECORD);
       covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
	   	 //guiDisplay( 1,17,"  本机交采实时数据  ",0);
	     tmpY = 17;
	     for(i=0;i<8;i++)
	     {
	    	 sign = 0;
    	   switch(i)
    	   {
    	     case 0:
	    	     disData = dataBuff[POWER_INSTANT_APPARENT] | dataBuff[POWER_INSTANT_APPARENT+1]<<8 | dataBuff[POWER_INSTANT_APPARENT+2]<<16;
    	       strcpy(say,"视在功率总=");
    	       break;
    	       
    	     case 1:
	    	     disData = dataBuff[POWER_PHASE_A_APPARENT] | dataBuff[POWER_PHASE_A_APPARENT+1]<<8 | dataBuff[POWER_PHASE_A_APPARENT+2]<<16;
    	       strcpy(say,"A=");
    	       break;
    	       
    	     case 2:
	    	     disData = dataBuff[POWER_PHASE_B_APPARENT] | dataBuff[POWER_PHASE_B_APPARENT+1]<<8 | dataBuff[POWER_PHASE_B_APPARENT+2]<<16;
    	       strcpy(say,"B=");
    	       break;
    	       
    	     case 3:
	    	     disData = dataBuff[POWER_PHASE_C_APPARENT] | dataBuff[POWER_PHASE_C_APPARENT+1]<<8 | dataBuff[POWER_PHASE_C_APPARENT+2]<<16;
    	       strcpy(say,"C=");
    	       break;

    	     case 4:
    	       strcpy(say,"功率因数总=");
	    	     disData = dataBuff[TOTAL_POWER_FACTOR] | dataBuff[TOTAL_POWER_FACTOR+1]<<8;
    	       break;
    	       
    	     case 5:
	    	     disData = dataBuff[FACTOR_PHASE_A] | dataBuff[FACTOR_PHASE_A+1]<<8;
    	       strcpy(say,"A=");
    	       break;
    	       
    	     case 6:
	    	     disData = dataBuff[FACTOR_PHASE_B] | dataBuff[FACTOR_PHASE_B+1]<<8;
    	       strcpy(say,"B=");
    	       break;
    	       
    	     case 7:
	    	     disData = dataBuff[FACTOR_PHASE_C] | dataBuff[FACTOR_PHASE_C+1]<<8;
    	       strcpy(say,"C=");
    	       break;
    	   }
	    	 
	    	 if (i<4)
	    	 {
	    	   if (disData&0x800000)
	    	   {
	    	 	   disData &= 0x7fffff;
	    	 	   sign = 1;
	    	   }

           decimal = (disData>>12 & 0xf)*1000
                   +(disData>>8 & 0xf)*100
                   +(disData>>4 & 0xf)*10
                   +(disData & 0xf);
           integer = (disData>>20 & 0xf)*10+(disData>>16 & 0xf);
           if (sign==1)
           {
         	   strcat(say,"-");
           }
           strcat(say,floatToString(integer,decimal,4,4,str));
         }
         else
         {
           if (disData&0x8000)
           {
          	  disData &= 0x7fff;
          	  sign = 1;
           }
           integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10+(disData>>4 & 0xf);
           decimal = (disData & 0xf);
           if (sign==1)
           {
             strcat(say,"-");
           }
           strcat(say,floatToString(integer,decimal,1,1,str));
         }

    	   if (i==0 || i==4)
    	   {
    	      guiDisplay(1, tmpY,say,1);
    	   }
    	   else
    	   {
    	      guiDisplay(10, tmpY,say,1);    	   	 
    	   }
    	   
    	   tmpY += 16;
	     }
	   	 
	   	 break;

	   case 6:
       memset(dataBuff,0xee,LENGTH_OF_PARA_RECORD);
	   	 covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
	   	 //guiDisplay( 1,17,"  本机交采实时数据  ",0);
	   	 guiDisplay( 1, 17,"三相电压",1);	   	 
	   	 guiDisplay( 1, 81,"三相电流",1);
	   	 
	   	 tmpY = 33;
       for(i=0;i<6;i++)
       {
         switch(i)
         {
           case 0:
             strcpy(say,"A=");
    	    	 disData = dataBuff[VOLTAGE_PHASE_A] | dataBuff[VOLTAGE_PHASE_A+1]<<8;
             break;
             
           case 1:
             strcpy(say,"B=");
   	    	   disData = dataBuff[VOLTAGE_PHASE_B] | dataBuff[VOLTAGE_PHASE_B+1]<<8;
             break;
             
           case 2:
             strcpy(say,"C=");
  	    	   disData = dataBuff[VOLTAGE_PHASE_C] | dataBuff[VOLTAGE_PHASE_C+1]<<8;
             break;

           case 3:
             strcpy(say,"A=");
    	    	 disData = dataBuff[CURRENT_PHASE_A] | dataBuff[CURRENT_PHASE_A+1]<<8 | dataBuff[CURRENT_PHASE_A+2]<<16;
             break;
             
           case 4:
             strcpy(say,"B=");
 	    	     disData = dataBuff[CURRENT_PHASE_B] | dataBuff[CURRENT_PHASE_B+1]<<8 | dataBuff[CURRENT_PHASE_B+2]<<16;
             break;
             
           case 5:
             strcpy(say,"C=");
  	    	   disData = dataBuff[CURRENT_PHASE_C] | dataBuff[CURRENT_PHASE_C+1]<<8 | dataBuff[CURRENT_PHASE_C+2]<<16;
             break;
         }
         
         if (i<=2)
         {                  
            decimal = disData& 0xf;
            integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10 + (disData>>4 & 0xf);
            strcat(say,floatToString(integer,decimal,1,1,str));
         }
         else
         {
            decimal = bcdToHex(disData&0xffff);
            integer = bcdToHex(disData>>12& 0xff);
            strcat(say,floatToString(integer,decimal,3,2,str));
         }
         
         guiAscii(10,tmpY,say,1);
         
         if (i==2)
         {
         	 tmpY += 32;
         }
         else
         {
         	 tmpY += 16;
         }
       }
	   	 break;
	   	 
	  #else
	  
	   case 3:
       memset(dataBuff,0xee,LENGTH_OF_PARA_RECORD);
       covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
	   	 guiDisplay( 1,17,"  本机交采实时数据  ",0);

	     tmpY = 33;
	     for(i=0;i<8;i++)
	     {
	    	 sign = 0;
    	   switch(i)
    	   {
    	     case 0:
	    	     disData = dataBuff[POWER_INSTANT_WORK] | dataBuff[POWER_INSTANT_WORK+1]<<8 | dataBuff[POWER_INSTANT_WORK+2]<<16;
    	       strcpy(say,"有功功率总=");
    	       break;
    	       
    	     case 1:
	    	     disData = dataBuff[POWER_PHASE_A_WORK] | dataBuff[POWER_PHASE_A_WORK+1]<<8 | dataBuff[POWER_PHASE_A_WORK+2]<<16;
    	       strcpy(say,"A=");
    	       break;
    	       
    	     case 2:
	    	     disData = dataBuff[POWER_PHASE_B_WORK] | dataBuff[POWER_PHASE_B_WORK+1]<<8 | dataBuff[POWER_PHASE_B_WORK+2]<<16;
    	       strcpy(say,"B=");
    	       break;
    	       
    	     case 3:
	    	     disData = dataBuff[POWER_PHASE_C_WORK] | dataBuff[POWER_PHASE_C_WORK+1]<<8 | dataBuff[POWER_PHASE_C_WORK+2]<<16;
    	       strcpy(say,"C=");
    	       break;

    	     case 4:
	    	     disData = dataBuff[POWER_INSTANT_NO_WORK] | dataBuff[POWER_INSTANT_NO_WORK+1]<<8 | dataBuff[POWER_INSTANT_NO_WORK+2]<<16;
    	       strcpy(say,"无功功率总=");
    	       break;
    	       
    	     case 5:
	    	     disData = dataBuff[POWER_PHASE_A_NO_WORK] | dataBuff[POWER_PHASE_A_NO_WORK+1]<<8 | dataBuff[POWER_PHASE_A_NO_WORK+2]<<16;
    	       strcpy(say,"A=");
    	       break;
    	       
    	     case 6:
	    	     disData = dataBuff[POWER_PHASE_B_NO_WORK] | dataBuff[POWER_PHASE_B_NO_WORK+1]<<8 | dataBuff[POWER_PHASE_B_NO_WORK+2]<<16;
    	       strcpy(say,"B=");
    	       break;
    	       
    	     case 7:
	    	     disData = dataBuff[POWER_PHASE_C_NO_WORK] | dataBuff[POWER_PHASE_C_NO_WORK+1]<<8 | dataBuff[POWER_PHASE_C_NO_WORK+2]<<16;
    	       strcpy(say,"C=");
    	       break;
    	   }
	    	 
	    	 if (disData&0x800000)
	    	 {
	    	 	 disData &= 0x7fffff;
	    	 	 sign = 1;
	    	 }

         decimal = (disData>>12 & 0xf)*1000
                   +(disData>>8 & 0xf)*100
                   +(disData>>4 & 0xf)*10
                   +(disData & 0xf);
         integer = (disData>>20 & 0xf)*10+(disData>>16 & 0xf);
         if (sign==1)
         {
         	 strcat(say,"-");
         }
         strcat(say,floatToString(integer,decimal,4,4,str));

    	   if (i==0 || i==4)
    	   {
    	      guiDisplay(1, tmpY,say,1);
    	   }
    	   else
    	   {
    	      guiDisplay(10, tmpY,say,1);    	   	 
    	   }
    	   
    	   tmpY += 16;
	     }
	   	 break;

	   case 4:
       memset(dataBuff,0xee,LENGTH_OF_PARA_RECORD);
       covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
	   	 guiDisplay( 1,17,"  本机交采实时数据  ",0);
	     tmpY = 33;
	     for(i=0;i<8;i++)
	     {
	    	 sign = 0;
    	   switch(i)
    	   {
    	     case 0:
	    	     disData = dataBuff[POWER_INSTANT_APPARENT] | dataBuff[POWER_INSTANT_APPARENT+1]<<8 | dataBuff[POWER_INSTANT_APPARENT+2]<<16;
    	       strcpy(say,"视在功率总=");
    	       break;
    	       
    	     case 1:
	    	     disData = dataBuff[POWER_PHASE_A_APPARENT] | dataBuff[POWER_PHASE_A_APPARENT+1]<<8 | dataBuff[POWER_PHASE_A_APPARENT+2]<<16;
    	       strcpy(say,"A=");
    	       break;
    	       
    	     case 2:
	    	     disData = dataBuff[POWER_PHASE_B_APPARENT] | dataBuff[POWER_PHASE_B_APPARENT+1]<<8 | dataBuff[POWER_PHASE_B_APPARENT+2]<<16;
    	       strcpy(say,"B=");
    	       break;
    	       
    	     case 3:
	    	     disData = dataBuff[POWER_PHASE_C_APPARENT] | dataBuff[POWER_PHASE_C_APPARENT+1]<<8 | dataBuff[POWER_PHASE_C_APPARENT+2]<<16;
    	       strcpy(say,"C=");
    	       break;

    	     case 4:
    	       strcpy(say,"功率因数总=");
	    	     disData = dataBuff[TOTAL_POWER_FACTOR] | dataBuff[TOTAL_POWER_FACTOR+1]<<8;
    	       break;
    	       
    	     case 5:
	    	     disData = dataBuff[FACTOR_PHASE_A] | dataBuff[FACTOR_PHASE_A+1]<<8;
    	       strcpy(say,"A=");
    	       break;
    	       
    	     case 6:
	    	     disData = dataBuff[FACTOR_PHASE_B] | dataBuff[FACTOR_PHASE_B+1]<<8;
    	       strcpy(say,"B=");
    	       break;
    	       
    	     case 7:
	    	     disData = dataBuff[FACTOR_PHASE_C] | dataBuff[FACTOR_PHASE_C+1]<<8;
    	       strcpy(say,"C=");
    	       break;
    	   }
	    	 
	    	 if (i<4)
	    	 {
	    	   if (disData&0x800000)
	    	   {
	    	 	   disData &= 0x7fffff;
	    	 	   sign = 1;
	    	   }

           decimal = (disData>>12 & 0xf)*1000
                   +(disData>>8 & 0xf)*100
                   +(disData>>4 & 0xf)*10
                   +(disData & 0xf);
           integer = (disData>>20 & 0xf)*10+(disData>>16 & 0xf);
           if (sign==1)
           {
         	   strcat(say,"-");
           }
           strcat(say,floatToString(integer,decimal,4,4,str));
         }
         else
         {
           if (disData&0x8000)
           {
          	  disData &= 0x7fff;
          	  sign = 1;
           }
           integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10+(disData>>4 & 0xf);
           decimal = (disData & 0xf);
           if (sign==1)
           {
             strcat(say,"-");
           }
           strcat(say,floatToString(integer,decimal,1,1,str));
         }

    	   if (i==0 || i==4)
    	   {
    	      guiDisplay(1, tmpY,say,1);
    	   }
    	   else
    	   {
    	      guiDisplay(10, tmpY,say,1);    	   	 
    	   }
    	   
    	   tmpY += 16;
	     }
	   	 
	   	 break;

	   case 5:
       memset(dataBuff,0xee,LENGTH_OF_PARA_RECORD);
	   	 covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
	   	 guiDisplay( 1,17,"  本机交采实时数据  ",0);
	   	 guiDisplay( 1, 33,"三相电压",1);	   	 
	   	 guiDisplay( 1, 97,"三相电流",1);
	   	 
	   	 tmpY = 49;
       for(i=0;i<6;i++)
       {
         switch(i)
         {
           case 0:
             strcpy(say,"A=");
    	    	 disData = dataBuff[VOLTAGE_PHASE_A] | dataBuff[VOLTAGE_PHASE_A+1]<<8;
             break;
             
           case 1:
             strcpy(say,"B=");
   	    	   disData = dataBuff[VOLTAGE_PHASE_B] | dataBuff[VOLTAGE_PHASE_B+1]<<8;
             break;
             
           case 2:
             strcpy(say,"C=");
  	    	   disData = dataBuff[VOLTAGE_PHASE_C] | dataBuff[VOLTAGE_PHASE_C+1]<<8;
             break;

           case 3:
             strcpy(say,"A=");
    	    	 disData = dataBuff[CURRENT_PHASE_A] | dataBuff[CURRENT_PHASE_A+1]<<8 | dataBuff[CURRENT_PHASE_A+2]<<16;
             break;
             
           case 4:
             strcpy(say,"B=");
 	    	     disData = dataBuff[CURRENT_PHASE_B] | dataBuff[CURRENT_PHASE_B+1]<<8 | dataBuff[CURRENT_PHASE_B+2]<<16;
             break;
             
           case 5:
             strcpy(say,"C=");
  	    	   disData = dataBuff[CURRENT_PHASE_C] | dataBuff[CURRENT_PHASE_C+1]<<8 | dataBuff[CURRENT_PHASE_C+2]<<16;
             break;
         }
         
         if (i<=2)
         {                  
            decimal = disData& 0xf;
            integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10 + (disData>>4 & 0xf);
            strcat(say,floatToString(integer,decimal,1,1,str));
         }
         else
         {
            decimal = bcdToHex(disData&0xffff);
            integer = bcdToHex(disData>>12& 0xff);
            strcat(say,floatToString(integer,decimal,3,2,str));
         }
         
         guiAscii(10,tmpY,say,1);
         
         if (i==2)
         {
         	 tmpY += 32;
         }
         else
         {
         	 tmpY += 16;
         }
       }
	   	 break;

	   case 6:
       memset(dataBuff,0xee,LENGTH_OF_PARA_RECORD);
       
	   	 covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
	   	 guiDisplay( 1,17,"  本机交采实时数据  ",0);
	   	 guiDisplay( 1, 33,"电压相位角",1);
	   	 guiDisplay( 1, 97,"电流相位角",1);
	   	 tmpY = 49;
       for(i=0;i<6;i++)
       {
         switch(i)
         {
           case 0:
             strcpy(say,"A=");
    	    	 disData = dataBuff[PHASE_ANGLE_V_A] | dataBuff[PHASE_ANGLE_V_A+1]<<8;
             break;
             
           case 1:
             strcpy(say,"B=");
   	    	   disData = dataBuff[PHASE_ANGLE_V_B] | dataBuff[PHASE_ANGLE_V_B+1]<<8;
             break;
             
           case 2:
             strcpy(say,"C=");
  	    	   disData = dataBuff[PHASE_ANGLE_V_C] | dataBuff[PHASE_ANGLE_V_C+1]<<8;
             break;

           case 3:
             strcpy(say,"A=");
    	    	 disData = dataBuff[PHASE_ANGLE_C_A] | dataBuff[PHASE_ANGLE_C_A+1]<<8;
             break;
             
           case 4:
             strcpy(say,"B=");
   	    	   disData = dataBuff[PHASE_ANGLE_C_B] | dataBuff[PHASE_ANGLE_C_B+1]<<8;
             break;
             
           case 5:
             strcpy(say,"C=");
  	    	   disData = dataBuff[PHASE_ANGLE_C_C] | dataBuff[PHASE_ANGLE_C_C+1]<<8;
             break;
         }
         
         if (disData&0x8000)
         {
          	disData &= 0x7fff;
          	sign = 1;
         }
         integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10+(disData>>4 & 0xf);
         decimal = (disData & 0xf);
         if (sign==1)
         {
           strcat(say,"-");
         }
         strcat(say,floatToString(integer,decimal,1,1,str));
         
         guiAscii(10,tmpY,say,1);
         
         if (i==2)
         {
         	 tmpY += 32;
         }
         else
         {
         	 tmpY += 16;
         }
       }
	   	 break;
	  #endif    //no PLUG_IN_CARRIER_MODULE
	   
	   case 7:
	   case 8:
	   case 9:
	   case 10:
	   case 11:
	   case 12:
	   case 13:
	   case 14:
       memset(visionBuff, 0xee, LENGTH_OF_ENERGY_RECORD);       
	   	 covertAcSample(NULL, visionBuff, NULL, 1, sysTime);
	   	 
	   	 switch(num)
	   	 {
	   	   case 7:
	   	     guiDisplay( 1, 17, "      正向有功      ",0);
	   	     guiDisplay( 1, 33, "有功电能量(kWh)",1);
	   	     guiDisplay( 1, 49, "  当前正向",1);
	   	     offset = POSITIVE_WORK_OFFSET;
	   	     break;
	   	     
	   	   case 8:
	   	     guiDisplay( 1, 17, "      反向有功      ",0);
	   	     guiDisplay( 1, 33, "有功电能量(kWh)",1);
	   	     guiDisplay( 1, 49, "  当前反向",1);
	   	     offset = NEGTIVE_WORK_OFFSET;
	   	   	 break;

	   	   case 9:
	   	     guiDisplay( 1, 17, "      正向无功      ",0);
	   	     guiDisplay( 1, 33, "无功电能量(kVarh)",1);
	   	     guiDisplay( 1, 49, "  当前正向",1);
	   	     offset = POSITIVE_NO_WORK_OFFSET;
	   	   	 break;

	   	   case 10:
	   	     guiDisplay( 1, 17, "      反向无功      ",0);
	   	     guiDisplay( 1, 33, "无功电能量(kVarh)",1);
	   	     guiDisplay( 1, 49, "  当前反向",1);
	   	     offset = NEGTIVE_NO_WORK_OFFSET;
	   	   	 break;

	   	   case 11:
	   	     guiDisplay( 1, 17, "       Q1无功       ",0);
	   	     guiDisplay( 1, 33, "无功电能量(kVarh)",1);
	   	     guiDisplay( 1, 49, "  当前一象限",1);
	   	     offset = QUA1_NO_WORK_OFFSET;
	   	   	 break;
	   	   	 
	   	   case 12:
	   	     guiDisplay( 1, 17, "       Q4无功       ",0);
	   	     guiDisplay( 1, 33, "无功电能量(kVarh)",1);
	   	     guiDisplay( 1, 49, "  当前四象限",1);
	   	     offset = QUA4_NO_WORK_OFFSET;
	   	   	 break;
	   	   	 
	   	   case 13:
	   	     guiDisplay( 1, 17, "       Q2无功       ",0);
	   	     guiDisplay( 1, 33, "无功电能量(kVarh)",1);
	   	     guiDisplay( 1, 49, "  当前二象限",1);
	   	     offset = QUA2_NO_WORK_OFFSET;
	   	   	 break;
	   	   	 
	   	   case 14:
	   	     guiDisplay( 1, 17, "       Q3无功       ",0);
	   	     guiDisplay( 1, 33, "无功电能量(kVarh)",1);
	   	     guiDisplay( 1, 49, "  当前三象限",1);
	   	     offset = QUA3_NO_WORK_OFFSET;
	   	   	 break;
	   	 }
	   	 
	   	 tmpY = 65;
       for(i=0;i<5;i++)
       {
       	  switch(i)
       	  {
       	  	case 0:
       	      strcpy(sayStr,"总=");
       	      break;
       	      
       	  	case 1:
       	      strcpy(sayStr,"尖=");
       	      break;

       	  	case 2:
       	      strcpy(sayStr,"峰=");
       	      break;
       	      
       	  	case 3:
       	      strcpy(sayStr,"平=");
       	      break;
       	      
       	  	case 4:
       	      strcpy(sayStr,"谷=");
       	      break;
       	  }
       	  
       	  if (dataBuff[POSITIVE_WORK_OFFSET+i*4]!=0xee)
       	  {
             decimal = bcdToHex(visionBuff[offset+i*4]);
             integer = bcdToHex(visionBuff[offset+i*4+1] |visionBuff[offset+i*4+2]<<8
                     | visionBuff[offset+i*4+3]<<16);
             strcpy(str, floatToString(integer,decimal,2,2,str));
             strcat(sayStr,str);
       	  }
       	  
       	  guiDisplay(16,tmpY,sayStr,1);
       	  
       	  if (i==0)
       	  {
       	  	 #ifdef PLUG_IN_CARRIER_MODULE
       	  	  tmpY += 16;
       	  	 #else
       	  	  tmpY += 32;
       	  	 #endif
       	  }
       	  else
       	  {
       	  	 tmpY += 16;
       	  }
       }
	   	 break;

	   case 15:
	   case 16:
       memset(reqBuff, 0xee, LENGTH_OF_REQ_RECORD);
	   	 covertAcSample(NULL, NULL, reqBuff, 1, sysTime);
	   	 
	   	 switch(num)
	   	 {
	   	   case 15:
	   	   	 #ifdef PLUG_IN_CARRIER_MODULE
	    	    guiDisplay( 1, 17, "当前正向有功需量:",1);
	   	   	 #else
	   	      guiDisplay( 1, 17, "      有功需量      ",0); 
	    	    guiDisplay( 1, 33, "当前正向有功:",1);
	    	   #endif
       	   strcpy(sayStr, "");
       	   if (reqBuff[REQ_POSITIVE_WORK_OFFSET]!=0xee)
       	   {
             decimal = bcdToHex(reqBuff[REQ_POSITIVE_WORK_OFFSET]|reqBuff[REQ_POSITIVE_WORK_OFFSET+1]<<8);
             integer = bcdToHex(reqBuff[REQ_POSITIVE_WORK_OFFSET+2]);
             strcpy(str, floatToString(integer,decimal,4,4,str));
             strcat(sayStr,str);
             strcat(sayStr,"kWar");
       	   }
       	   #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay(49, 33, sayStr,1);
       	   #else
	   	      guiDisplay(49, 49, sayStr,1);
	   	     #endif
	      
	         strcpy(sayStr,"～发生时间:");
	         #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay( 1, 49, sayStr,1);
	   	     #else
	   	      guiDisplay( 1, 65, sayStr,1);
	   	     #endif
	   	     
	   	     strcpy(sayStr, "");
	         if (reqBuff[REQ_TIME_P_WORK_OFFSET]!=0xee)
	         {
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_P_WORK_OFFSET+3]),str));
	           strcat(sayStr,"-");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_P_WORK_OFFSET+2]),str));
	           strcat(sayStr," ");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_P_WORK_OFFSET+1]),str));
	           strcat(sayStr,":");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_P_WORK_OFFSET]),str));
	         }

	         #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay(49, 65, sayStr,1);
	    	    guiDisplay( 1, 81, "当前反向有功需量:",1);
	         #else
	   	      guiDisplay(49, 81, sayStr,1);
	    	    guiDisplay( 1, 97, "当前反向有功:",1);
	   	     #endif
	   	     
       	   strcpy(sayStr, "");
       	   if (reqBuff[REQ_NEGTIVE_WORK_OFFSET]!=0xee)
       	   {
             decimal = bcdToHex(reqBuff[REQ_NEGTIVE_WORK_OFFSET]|reqBuff[REQ_NEGTIVE_WORK_OFFSET+1]<<8);
             integer = bcdToHex(reqBuff[REQ_NEGTIVE_WORK_OFFSET+2]);
             strcpy(str, floatToString(integer,decimal,4,4,str));
             strcat(sayStr,str);
             strcat(sayStr,"kWar");
       	   }
       	   #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay(49, 97, sayStr,1);
       	   #else
	   	      guiDisplay(49,113, sayStr,1);
	   	     #endif
	      
	         strcpy(sayStr,"～发生时间:");
	         #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay( 1,113, sayStr,1);
	         #else
	   	      guiDisplay( 1,129, sayStr,1);
	   	     #endif
	   	     strcpy(sayStr, "");
	         if (reqBuff[REQ_TIME_N_WORK_OFFSET]!=0xee)
	         {
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_N_WORK_OFFSET+3]),str));
	           strcat(sayStr,"-");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_N_WORK_OFFSET+2]),str));
	           strcat(sayStr," ");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_N_WORK_OFFSET+1]),str));
	           strcat(sayStr,":");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_N_WORK_OFFSET]),str));
	         }
	         #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay(49, 129, sayStr,1);
	         #else
	   	      guiDisplay(49, 145, sayStr,1);
	   	     #endif
	   	     break;
	   	     
	   	   case 16:
	   	     #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay( 1, 17, "当前正向无功需量:",1);
	   	     #else
	   	      guiDisplay( 1, 17, "      无功需量      ",0);
	   	      guiDisplay( 1, 33, "当前正向无功:",1);
	   	     #endif
       	   strcpy(sayStr, "");
       	   if (reqBuff[REQ_POSITIVE_NO_WORK_OFFSET]!=0xee)
       	   {
             decimal = bcdToHex(reqBuff[REQ_POSITIVE_NO_WORK_OFFSET]|reqBuff[REQ_POSITIVE_NO_WORK_OFFSET+1]<<8);
             integer = bcdToHex(reqBuff[REQ_POSITIVE_NO_WORK_OFFSET+2]);
             strcpy(str, floatToString(integer,decimal,4,4,str));
             strcat(sayStr,str);
             strcat(sayStr,"kVar");
       	   }
       	   #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay(49, 33, sayStr,1);
       	   #else
	   	      guiDisplay(49, 49, sayStr,1);
	   	     #endif
	      
	         strcpy(sayStr,"～发生时间:");
	         #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay( 1, 49, sayStr,1);
	         #else
	   	      guiDisplay( 1, 65, sayStr,1);
	   	     #endif
	   	     strcpy(sayStr, "");
	         if (reqBuff[REQ_TIME_P_NO_WORK_OFFSET]!=0xee)
	         {
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_P_NO_WORK_OFFSET+3]),str));
	           strcat(sayStr,"-");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_P_NO_WORK_OFFSET+2]),str));
	           strcat(sayStr," ");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_P_NO_WORK_OFFSET+1]),str));
	           strcat(sayStr,":");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_P_NO_WORK_OFFSET]),str));
	         }
	         #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay(49, 65, sayStr,1);
	   	     
	   	      guiDisplay( 1, 81, "当前反向无功需量:",1);
	         #else
	   	      guiDisplay(49, 81, sayStr,1);
	   	     
	   	      guiDisplay( 1, 97, "当前反向无功:",1);
	   	     #endif
       	   strcpy(sayStr, "");
       	   if (reqBuff[REQ_NEGTIVE_NO_WORK_OFFSET]!=0xee)
       	   {
             decimal = bcdToHex(reqBuff[REQ_NEGTIVE_NO_WORK_OFFSET]|reqBuff[REQ_NEGTIVE_NO_WORK_OFFSET+1]<<8);
             integer = bcdToHex(reqBuff[REQ_NEGTIVE_NO_WORK_OFFSET+2]);
             strcpy(str, floatToString(integer,decimal,4,4,str));
             strcat(sayStr,str);
             strcat(sayStr,"kVar");
       	   }
       	   #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay(49, 97, sayStr,1);
       	   #else
	   	      guiDisplay(49,113, sayStr,1);
	   	     #endif
	      
	         strcpy(sayStr,"～发生时间:");
	         #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay( 1,113, sayStr,1);
	   	     #else
	   	      guiDisplay( 1,129, sayStr,1);
	   	     #endif
	   	     strcpy(sayStr, "");
	         if (reqBuff[REQ_TIME_N_NO_WORK_OFFSET]!=0xee)
	         {
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_N_NO_WORK_OFFSET+3]),str));
	           strcat(sayStr,"-");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_N_NO_WORK_OFFSET+2]),str));
	           strcat(sayStr," ");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_N_NO_WORK_OFFSET+1]),str));
	           strcat(sayStr,":");
	           strcat(sayStr,digital2ToString(bcdToHex(reqBuff[REQ_TIME_N_NO_WORK_OFFSET]),str));
	         }
	         #ifdef PLUG_IN_CARRIER_MODULE
	   	      guiDisplay( 49,129, sayStr,1);
	         #else
	   	      guiDisplay( 49,145, sayStr,1);
	   	     #endif
	   	     break;
	   	 }
	   	 break;
	   	 
	   case 17:
       memset(dataBuff,0xee,LENGTH_OF_PARA_RECORD);
       
	   	 covertAcSample(dataBuff, NULL, NULL, 1, sysTime);
	   	 //guiDisplay( 1,17,"  本机交采实时数据  ",0);
	   	 guiDisplay( 1, 17,"电压相位角",1);
	   	 guiDisplay( 1, 81,"电流相位角",1);
	   	 tmpY = 33;
       for(i=0;i<6;i++)
       {
         switch(i)
         {
           case 0:
             strcpy(say,"A=");
    	    	 disData = dataBuff[PHASE_ANGLE_V_A] | dataBuff[PHASE_ANGLE_V_A+1]<<8;
             break;
             
           case 1:
             strcpy(say,"B=");
   	    	   disData = dataBuff[PHASE_ANGLE_V_B] | dataBuff[PHASE_ANGLE_V_B+1]<<8;
             break;
             
           case 2:
             strcpy(say,"C=");
  	    	   disData = dataBuff[PHASE_ANGLE_V_C] | dataBuff[PHASE_ANGLE_V_C+1]<<8;
             break;

           case 3:
             strcpy(say,"A=");
    	    	 disData = dataBuff[PHASE_ANGLE_C_A] | dataBuff[PHASE_ANGLE_C_A+1]<<8;
             break;
             
           case 4:
             strcpy(say,"B=");
   	    	   disData = dataBuff[PHASE_ANGLE_C_B] | dataBuff[PHASE_ANGLE_C_B+1]<<8;
             break;
             
           case 5:
             strcpy(say,"C=");
  	    	   disData = dataBuff[PHASE_ANGLE_C_C] | dataBuff[PHASE_ANGLE_C_C+1]<<8;
             break;
         }
         
         if (disData&0x8000)
         {
          	disData &= 0x7fff;
          	sign = 1;
         }
         integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10+(disData>>4 & 0xf);
         decimal = (disData & 0xf);
         if (sign==1)
         {
           strcat(say,"-");
         }
         strcat(say,floatToString(integer,decimal,1,1,str));
         
         guiAscii(10,tmpY,say,1);
         
         if (i==2)
         {
         	 tmpY += 32;
         }
         else
         {
         	 tmpY += 16;
         }
       }
	   	 break;
	  }
	  
	  lcdRefresh(17,160);
}

/*******************************************************
函数名称:stringUpDown
功能描述:当按下上或下键时字符串的变动
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void stringUpDown(char *processStr, INT8U timeChar, INT8U upDown)
{ 
   if (upDown==1)    //向下
   {
    	if (processStr[timeChar]<=0x30)
    	{
    		processStr[timeChar]=0x39;
    	}
    	else
    	{
        processStr[timeChar]--;
    	}
   }
   else              //向上
   {
  	 	if (processStr[timeChar]>=0x39)
  	 	{
  	 		processStr[timeChar]=0x30;
  	 	}
  	 	else
  	 	{
        processStr[timeChar]++;
  	 	}
   }
}

/*******************************************************
函数名称:showInputTime
功能描述:显示输入日期框
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void showInputTime(INT8U layer3Light)
{
   INT8U tmpX, i;
   char  str[2];
   
   guiDisplay(1,60,"请选择查询日期:",1);
   tmpX = 40;
   for(i=0;i<8;i++)
   {
     str[0] = queryTimeStr[i];
     str[1] = '\0';
     if (i==layer3Light)
     {
       guiDisplay(tmpX,80,str,0);
     }
     else
     {
     	 guiDisplay(tmpX,80,str,1);
     }
     tmpX += 8;
     if (i==3 || i==5)
     {
        guiDisplay(tmpX,80,"-",1);
     	  tmpX += 8;
     }
   }
}

/*******************************************************
函数名称:checkInputTime
功能描述:检验输入的日期是否合格
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
BOOL checkInputTime(void)
{
  INT16U tmpData;
  
  menuQueryTime = sysTime;
  
  //判断年
  tmpData = (queryTimeStr[0]-0x30)*1000+(queryTimeStr[1]-0x30)*100+(queryTimeStr[2]-0x30)*10+(queryTimeStr[3]-0x30);
  if (tmpData<1970 || tmpData>2099)
  {
  	 guiDisplay(20, 110, "年份输入不正确!", 1);
  	 lcdRefresh(110,130);
  	 return FALSE;
  }
  else
  {
  	 menuQueryTime.year = tmpData-2000; 
  }
  
  //判断月
  tmpData = (queryTimeStr[4]-0x30)*10+(queryTimeStr[5]-0x30);
  if (tmpData<1 || tmpData>12)
  {
  	 guiDisplay(20, 110, "月份输入不正确!", 1);
  	 lcdRefresh(110,130);
  	 return FALSE;
  }
  else
  {
  	 menuQueryTime.month = tmpData;
  }
  
  //判断日      	    		 	 	 	  
  tmpData = (queryTimeStr[6]-0x30)*10+(queryTimeStr[7]-0x30);
  if (tmpData<1 || tmpData>31)
  {
  	 guiDisplay(20, 110, "日期输入不正确!",1);
  	 lcdRefresh(110,130);
  	 return FALSE;
  }
  else
  {
  	 menuQueryTime.day = tmpData;
  }
  
  return TRUE;
}

/*******************************************************
函数名称:fillTimeStr
功能描述:用当前日期填充查询日期字符串
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void fillTimeStr(void)
{
 	 char str[4];
 	 
 	 strcpy(queryTimeStr,"20");
   strcat(queryTimeStr, digital2ToString(sysTime.year,str));
   strcat(queryTimeStr, digital2ToString(sysTime.month,str));
   strcat(queryTimeStr, digital2ToString(sysTime.day,str));
}

/*******************************************************
函数名称:inputPassWord
功能描述:输入密码菜单
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void inputPassWord(INT8U lightNum)
{
   INT8U tmpX, i;
   char  str[2];
   
	#ifdef PLUG_IN_CARRIER_MODULE
	 #ifdef MENU_FOR_CQ_CANON
	  guiLine(1,17,160,160,0); //清屏
	 #else
	  guiLine(1,17,160,144,0); //清屏
	 #endif
	#else
	  guiLine(1,17,160,160,0); //清屏	
	#endif
	 
	 menuInLayer = 20;        //菜单进入第20层(本层只有输入密码用)
   
   guiDisplay(40,60,"请输入密码",1);
   
   //画框
   guiLine(50,80,50,100,1);
   guiLine(110,80,110,100,1);
   guiLine(50,80,110,80,1);
   guiLine(50,100,110,100,1);
   
   tmpX = 58;
   for(i=0;i<6;i++)
   {
     str[0] = passWord[i];
     str[1] = '\0';
     if (i==lightNum)
     {
       guiDisplay(tmpX,82,str,0);
     }
     else
     {
     	 guiDisplay(tmpX,82,str,1);
     }
     tmpX += 8;
   }
   
  #ifdef PLUG_IN_CARRIER_MODULE
   #ifdef MENU_FOR_CQ_CANON
    lcdRefresh(17,160);
   #else
    lcdRefresh(17,145);
   #endif
  #else
    lcdRefresh(17,160);
  #endif
}

/*******************************************************
函数名称:inputApn
功能描述:输入APN
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void inputApn(INT8U leftRight)
{
	 char  str[20];
	 INT8U i, tmpX, tmpY;

 #ifdef PLUG_IN_CARRIER_MODULE 
  #ifdef MENU_FOR_CQ_CANON
   tmpY = 33;
   guiLine(1,tmpY,160,160,0); //清屏
  #else
   tmpY = 17;
   guiLine(1,tmpY,160,144,0); //清屏
  #endif
   menuInLayer = 4;         //菜单进入第4层
 #else
   tmpY = 17;
   guiLine(1,tmpY,160,160,0); //清屏

   menuInLayer = 5;         //菜单进入第5层
 #endif
   
   //画框
   guiLine(1,tmpY+1,1,tmpY+41,1);
   guiLine(160,tmpY+1,160,tmpY+41,1);
   guiLine(1,tmpY+1,160,tmpY+1,1);
   guiLine(1,tmpY+41,160,tmpY+41,1);
   guiLine(1,tmpY+21,160,tmpY+21,1);
   guiDisplay(55,tmpY+3,"输入APN",1);

   tmpX = 15;
   tmpY += 23;
   for(i=0;i<16;i++)
   {
     #ifdef PLUG_IN_CARRIER_MODULE
      str[0] = commParaItem[1][i];
     #else
      str[0] = commParaItem[2][i];
     #endif
     str[1] = '\0';
     if (leftRight==i)
     {
       guiDisplay(tmpX,tmpY,str,0);
     }
     else
     {
       guiDisplay(tmpX,tmpY,str,1);
     }
     tmpX += 8;
   }
   
 #ifdef PLUG_IN_CARRIER_MODULE
  #ifdef MENU_FOR_CQ_CANON 
   lcdRefresh(17,160);
  #else
   lcdRefresh(17,145);
  #endif
 #else
  lcdRefresh(17,160);
 #endif
}


/*******************************************************
函数名称:selectChar
功能描述:选择字符
调用函数:
被调用函数:
输入参数:num
输出参数:
返回值:void
*******************************************************/
void selectChar(INT8U num)
{
    INT8U i,row,col;
    char  say[2];

    strcpy(character,". +-@*#0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

    inputStatus = STATUS_SELECT_CHAR;
	  
	 #ifdef MENU_FOR_CQ_CANON
	  row = 74;
	  guiLine(4,row,156,160,0);
	 #else
	  row = 57;
	  guiLine(4,row,156,144,0);
	 #endif
	  
	  guiLine(  4,row   ,  4,row+86,1);
	  guiLine(156,row   ,156,row+86,1);
	  guiLine(  4,row   ,156,row   ,1);
	  guiLine(  4,row+86,156,row+86,1);
	  guiLine( 24,row   , 24,row+86,1);
	  
	  guiDisplay(7,row+ 3,"选",1);
	  guiDisplay(7,row+24,"择",1);
	  guiDisplay(7,row+45,"字",1);
	  guiDisplay(7,row+66,"符",1);
	  
	  col = 26;
	  row += 1;
	  say[1] = '\0';
	  for(i=0;i<69;i++)
	  {
	     if (i%14==0 && i!=0)
	     {
	     	  row +=17;
	     	  col = 26;
	     }
	     
	     say[0] = character[i];
	     if (i==num)
	     {
	        guiAscii(col+1,row,say,0);
	     }
	     else
	     {
	        guiAscii(col,row,say,1);
	     }
	     col+=9;
	  }
	 
	 #ifdef MENU_FOR_CQ_CANON 
	  lcdRefresh(73,160);
	 #else
	  lcdRefresh(57,145);
	 #endif
}

//Q/GDW376.1-2009集中器菜单
#ifdef PLUG_IN_CARRIER_MODULE

#ifdef LIGHTING
/*******************************************************
函数名称:ccbStatus
功能描述:单灯控制点状态
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void ccbStatus(void)
{
 	char tmpStr[20];
 	
 	guiLine(1,17,160,144,0); //清屏
 	
	menuInLayer = 2;         //菜单进入第2层
	
	
	
	if (queryMpLink!=NULL)
	{
	  sprintf(tmpStr,"[%02x%02x%02x%02x%02x%02x]状态: ", queryMpLink->addr[5], queryMpLink->addr[4], queryMpLink->addr[3], queryMpLink->addr[2], queryMpLink->addr[1], queryMpLink->addr[0]);
	  
	  guiDisplay(1, 17, tmpStr, 0);

	  sprintf(tmpStr, "控制点号:%03d", queryMpLink->mp);
	  guiDisplay(1,33,tmpStr,1);

	  if (queryMpLink->status>100)
	  {
	  	strcpy(tmpStr, "当前亮度:未知");
	  }
	  else
	  {
	    sprintf(tmpStr, "当前亮度:%d%%", queryMpLink->status);
	  }
	  guiDisplay(1,49,tmpStr,1);
	  
	  guiDisplay(1, 65, "获取状态时间:",1);
	  sprintf(tmpStr, "%02d-%02d-%02d %02d:%02d:%02d", queryMpLink->statusTime.year, queryMpLink->statusTime.month, queryMpLink->statusTime.day, queryMpLink->statusTime.hour, queryMpLink->statusTime.minute, queryMpLink->statusTime.second);
	  guiDisplay(16, 81, tmpStr, 1);
	  
	  if (queryMpLink->msCtrlCmd<=100)
	  {
	    sprintf(tmpStr, "主站控制亮度:%d%%", queryMpLink->msCtrlCmd);
	  }
	  else
	  {
	    strcpy(tmpStr, "主站控制命令:无");
	  }
	  guiDisplay(1, 97, tmpStr, 1);
	  guiDisplay(1,113, "主站命令截止时间:",1);
	  sprintf(tmpStr, "%02d-%02d-%02d %02d:%02d:%02d", queryMpLink->msCtrlTime.year, queryMpLink->msCtrlTime.month, queryMpLink->msCtrlTime.day, queryMpLink->msCtrlTime.hour, queryMpLink->msCtrlTime.minute, queryMpLink->msCtrlTime.second);
	  guiDisplay(16,129, tmpStr, 1);
	}
	else
	{
	  guiDisplay(1,17,"控制点当前状态:",1);
	}
	
	lcdRefresh(17, 145);
}

/*******************************************************
函数名称:xlcStatus
功能描述:线路控制器状态
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void xlcStatus(void)
{
 	char tmpStr[20];
 	
 	guiLine(1,17,160,144,0); //清屏
 	
	menuInLayer = 2;         //菜单进入第2层
	
	if (queryMpLink!=NULL)
	{
	  sprintf(tmpStr,"[%02x%02x%02x%02x%02x%02x]%03d#: ", queryMpLink->addr[5], queryMpLink->addr[4], queryMpLink->addr[3], queryMpLink->addr[2], queryMpLink->addr[1], queryMpLink->addr[0],queryMpLink->mp);
	  guiDisplay(1, 17, tmpStr, 0);

	  switch(queryMpLink->bigAndLittleType)
	  {
	  	case 1:
	  		strcpy(tmpStr, "模式:时段控");
	  		break;
	  		
	  	case 2:
	  		strcpy(tmpStr, "模式:光控");
	  		break;
	  		
	  	case 3:
	  		strcpy(tmpStr, "模式:时段控结合光控");
	  		break;
	  		
	  	case 4:
	  		strcpy(tmpStr, "模式:经纬度控");
	  		break;

			case 5:
	  		strcpy(tmpStr, "模式:经纬度结合光控");
	  		break;
	  		
	  	default:
	  		strcpy(tmpStr, "模式:未知");
	  		break;
	  }
	  guiDisplay(1,33,tmpStr,1);

	  if (queryMpLink->collectorAddr[0]!=0xff)
	  {
	    sprintf(tmpStr, "经度:%d.%04d", queryMpLink->collectorAddr[0], queryMpLink->collectorAddr[1] | queryMpLink->collectorAddr[2]<<8);
	    guiDisplay(1,49,tmpStr,1);
	  }
	  
	  if (
	  	  queryMpLink->duty[0]!=0x00 
	  	  && 
				 (
				  CTRL_MODE_LA_LO==queryMpLink->bigAndLittleType
					 || CTRL_MODE_LA_LO_LIGHT==queryMpLink->bigAndLittleType
				 )
	  	 )
	  {
	  	sprintf(tmpStr, "分%02d:%02d", queryMpLink->duty[1], queryMpLink->duty[0]);
	    guiDisplay(105, 49, tmpStr,1);
	  }

	  if (queryMpLink->collectorAddr[3]!=0xff)
	  {
	    sprintf(tmpStr, "纬度:%d.%04d", queryMpLink->collectorAddr[3], queryMpLink->collectorAddr[4] | queryMpLink->collectorAddr[5]<<8);
	    guiDisplay(1,65,tmpStr,1);
	  }
	  
	  if (
	  	  queryMpLink->duty[2]!=0x00 
	  	  && 
				 (
				  CTRL_MODE_LA_LO==queryMpLink->bigAndLittleType
					 || CTRL_MODE_LA_LO_LIGHT==queryMpLink->bigAndLittleType
				 )
	  	 )
	  {
	  	sprintf(tmpStr, "合%02d:%02d", queryMpLink->duty[3], queryMpLink->duty[2]);
	    guiDisplay(105, 65, tmpStr,1);
	  }

	  if (queryMpLink->status>1)
	  {
	  	strcpy(tmpStr, "当前:未知");
	  }
	  else
	  {
	    if (1==queryMpLink->status)
	    {
	      sprintf(tmpStr, "当前:合闸,获取时间:");
	    }
	    else
	    {
	      sprintf(tmpStr, "当前:分闸,获取时间:");
	    }
	  }
	  guiDisplay(1,81,tmpStr,1);

	  sprintf(tmpStr, "%02d-%02d-%02d %02d:%02d:%02d", queryMpLink->statusTime.year, queryMpLink->statusTime.month, queryMpLink->statusTime.day, queryMpLink->statusTime.hour, queryMpLink->statusTime.minute, queryMpLink->statusTime.second);
	  guiDisplay(17, 97, tmpStr, 1);
	  
	  switch (queryMpLink->msCtrlCmd)
	  {
	    case 0:
	      strcpy(tmpStr, "主站命令:分闸");
	      break;

	    case 1:
	      strcpy(tmpStr, "主站命令:合闸");
	      break;
	      
	    default:
	      strcpy(tmpStr, "主站命令:无");
	      break;
	  }
	  strcat(tmpStr, ",截止:");
	  guiDisplay(1, 113, tmpStr, 1);
	  sprintf(tmpStr, "%02d-%02d-%02d %02d:%02d:%02d", queryMpLink->msCtrlTime.year, queryMpLink->msCtrlTime.month, queryMpLink->msCtrlTime.day, queryMpLink->msCtrlTime.hour, queryMpLink->msCtrlTime.minute, queryMpLink->msCtrlTime.second);
	  guiDisplay(17,129, tmpStr, 1);
	}
	else
	{
	  guiDisplay(1,17,"线路控制点当前状态:",1);
	}
	
	lcdRefresh(17, 145);
}

/*******************************************************
函数名称:xlOpenClose
功能描述:线路控制器手动分合闸菜单(照明集中器)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void xlOpenClose(INT8U lightNum)
{
	INT8U i, row, col;
	char  str[2];
	
	menuInLayer = 3;

	guiLine( 60, 38,149,140,0);

	guiLine( 60, 38, 60,138, 1);
	guiLine(149, 38,149,138, 1);
	guiLine( 60, 38,149, 38, 1);
	guiLine( 60,138,149,138, 1);

	switch (lightNum)
	{
	  case 0:
	    guiDisplay(65, 40, "分闸十分钟", 0);
	    guiDisplay(65, 60, "分闸一小时", 1);
	    guiDisplay(65, 80, "合闸十分钟", 1);
	    guiDisplay(65,100, "合闸一小时", 1);
	    guiDisplay(65,120, " 自动控制 ", 1);
	    break;
	  
	  case 1:
	    guiDisplay(65, 40, "分闸十分钟", 1);
	    guiDisplay(65, 60, "分闸一小时", 0);
	    guiDisplay(65, 80, "合闸十分钟", 1);
	    guiDisplay(65,100, "合闸一小时", 1);
	    guiDisplay(65,120, " 自动控制 ", 1);
	    break;

		case 2:
	    guiDisplay(65, 40, "分闸十分钟", 1);
	    guiDisplay(65, 60, "分闸一小时", 1);
	    guiDisplay(65, 80, "合闸十分钟", 0);
	    guiDisplay(65,100, "合闸一小时", 1);
	    guiDisplay(65,120, " 自动控制 ", 1);
	    break;
	  
		case 3:
	    guiDisplay(65, 40, "分闸十分钟", 1);
	    guiDisplay(65, 60, "分闸一小时", 1);
	    guiDisplay(65, 80, "合闸十分钟", 1);
	    guiDisplay(65,100, "合闸一小时", 0);
	    guiDisplay(65,120, " 自动控制 ", 1);
	    break;

		case 4:
	    guiDisplay(65, 40, "分闸十分钟", 1);
	    guiDisplay(65, 60, "分闸一小时", 1);
	    guiDisplay(65, 80, "合闸十分钟", 1);
	    guiDisplay(65,100, "合闸一小时", 1);
	    guiDisplay(65,120, " 自动控制 ", 0);
	    break;
	}
 
	lcdRefresh(38, 140);
}

/*******************************************************
函数名称:xlOpenCloseReply
功能描述:线路控制器手动分合闸菜单(照明集中器)回复
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void xlOpenCloseReply(char *say)
{
	guiLine(12,57,148,103,0);
	guiDisplay(44,70,say,1);
	lcdRefresh(55, 105);
}

/*******************************************************
函数名称:irStudyReply
功能描述:红外学习器学习(照明集中器)回复
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void irStudyReply(char *say)
{
	guiLine(12,57,148,103,0);
	guiDisplay(44,75,say,1);
	lcdRefresh(55, 105);

	guiLine(10,55,150,105,0);
}


/*******************************************************
函数名称:ldgmStatus
功能描述:报警控制点状态
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void ldgmStatus(void)
{
 	char tmpStr[20];
 	
 	guiLine(1,17,160,144,0); //清屏
 	
	menuInLayer = 2;         //菜单进入第2层
	
	if (queryMpLink!=NULL)
	{
	  sprintf(tmpStr,"[%02x%02x%02x%02x%02x%02x]状态: ", queryMpLink->addr[5], queryMpLink->addr[4], queryMpLink->addr[3], queryMpLink->addr[2], queryMpLink->addr[1], queryMpLink->addr[0]);
	  
	  guiDisplay(1, 17, tmpStr, 0);

	  sprintf(tmpStr, "控制点号:%03d", queryMpLink->mp);
	  guiDisplay(1,33,tmpStr,1);

	  if (queryMpLink->status&0x4)
	  {
	  	  strcpy(tmpStr, "当前状态:装置异常");
	  }
	  else
	  {
	    if (queryMpLink->status&0x2)
	    {
	  	  strcpy(tmpStr, "当前状态:线路异常");
	    }
	    else
	    {
	      sprintf(tmpStr, "当前状态:线路正常");
	    }
	  }
	  guiDisplay(1,49,tmpStr,1);
	  
	  guiDisplay(1, 65, "获取状态时间:",1);
	  sprintf(tmpStr, "%02d-%02d-%02d %02d:%02d:%02d", queryMpLink->statusTime.year, queryMpLink->statusTime.month, queryMpLink->statusTime.day, queryMpLink->statusTime.hour, queryMpLink->statusTime.minute, queryMpLink->statusTime.second);
	  guiDisplay(16, 81, tmpStr, 1);
	  
	  if (queryMpLink->status&0x1)
	  {
	  	strcpy(tmpStr, "线路供电:交流");
	  }
	  else
	  {
	    sprintf(tmpStr, "线路供电:直流");
	  }
	  guiDisplay(1,97,tmpStr,1);
	  
	  sprintf(tmpStr, "当前电流值:%4.3fA", (float)(queryMpLink->duty[0] | queryMpLink->duty[1]<<8 | queryMpLink->duty[2]<<16)/1000);
	  guiDisplay(1,113,tmpStr,1);

    sprintf(tmpStr, "当前电压值:%3.1fV", (float)(queryMpLink->duty[3] | queryMpLink->duty[4]<<8)/10);
	  guiDisplay(1,129,tmpStr,1);
	}
	else
	{
	  guiDisplay(1,17,"报警控制点当前状态:",1);
	}
	
	lcdRefresh(17, 145);
}

/*******************************************************
函数名称:lsStatus
功能描述:照度传感器状态
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void lsStatus(void)
{
 	char tmpStr[20];
 	
 	guiLine(1,17,160,144,0); //清屏
 	
	menuInLayer = 2;         //菜单进入第2层
	
	if (queryMpLink!=NULL)
	{
	  sprintf(tmpStr,"[%02x%02x%02x%02x%02x%02x]照度: ", queryMpLink->addr[5], queryMpLink->addr[4], queryMpLink->addr[3], queryMpLink->addr[2], queryMpLink->addr[1], queryMpLink->addr[0]);
	  guiDisplay(1, 17, tmpStr, 0);

	  sprintf(tmpStr, "控制点号:%03d", queryMpLink->mp);
	  guiDisplay(1,33,tmpStr,1);

	  sprintf(tmpStr, "照度:%ldLux", queryMpLink->duty[0] | queryMpLink->duty[1]<<8 | queryMpLink->duty[2]<<16);
	  guiDisplay(1,49,tmpStr,1);
	  
	  guiDisplay(1, 65, "获取照度时间:",1);
	  sprintf(tmpStr, "%02d-%02d-%02d %02d:%02d:%02d", queryMpLink->statusTime.year, queryMpLink->statusTime.month, queryMpLink->statusTime.day, queryMpLink->statusTime.hour, queryMpLink->statusTime.minute, queryMpLink->statusTime.second);
	  guiDisplay(16, 81, tmpStr, 1);
	}
	else
	{
	  guiDisplay(1,17,"照度检测点当前状态:",1);
	}
	
	lcdRefresh(17, 145);
}

/*******************************************************
函数名称:queryCTimes
功能描述:查询控制时段
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void queryCTimes(struct ctrlTimes *tmpNode)
{
	char  str[30], strx[20];
	INT8U i, tmpY;

	guiLine(1,17,160,144,0);  //清屏
	menuInLayer = 14;         //菜单进入第14层
	
	if(tmpNode!=NULL)
	{
    tmpY = 17;
    switch (tmpNode->deviceType)
    {
    	case 2:
        strcpy(str, "线路控制");
        break;

			case 5:
        strcpy(str, "经纬度控");
        break;

    	case 7:
        strcpy(str, "照度控制");
        break;
      
      default:
     	  strcpy(str, "单灯控制");
     	  break;
    }
    sprintf(strx, "%02d-%02d至%02d-%02d", tmpNode->startMonth, tmpNode->startDay, tmpNode->endMonth, tmpNode->endDay);
		strcat(str, strx);
    guiDisplay(1, tmpY, str,1);
    tmpY += 16;
    if (tmpNode->deviceType==7)
		{
		  sprintf(str, "序号%d:", tmpNode->noOfTime);			
		}
		else
		{
		  sprintf(str, "时段%d:", tmpNode->noOfTime);
		}
		if (tmpNode->workDay&0x01)
		{
			strcat(str, "一");
		}
		if (tmpNode->workDay&0x02)
		{
			strcat(str, "二");
		}
		if (tmpNode->workDay&0x04)
		{
			strcat(str, "三");
		}
		if (tmpNode->workDay&0x08)
		{
			strcat(str, "四");
		}
		if (tmpNode->workDay&0x10)
		{
			strcat(str, "五");
		}
		if (tmpNode->workDay&0x20)
		{
			strcat(str, "六");
		}
		if (tmpNode->workDay&0x40)
		{
			strcat(str, "日");
		}
    guiDisplay(1, tmpY, str,1);
    tmpY += 16;
    
    for(i=0; i<6; i++)
    {
      if (tmpNode->hour[i]==0xff)
      {
      	break;
      }
      
      switch (tmpNode->deviceType)
      {
        case 2:
        case 5:
          if (1==tmpNode->bright[i])
          {
            sprintf(str, "%d)%02x:%02x-接通", i+1, tmpNode->hour[i], tmpNode->min[i]);
          }
          else
          {
            sprintf(str, "%d)%02x:%02x-切断", i+1, tmpNode->hour[i], tmpNode->min[i]);
          }
          break;
          
        case 7:
          sprintf(str, "%d)%dLux-%d%% ", i+1, (tmpNode->hour[i] | tmpNode->min[i]<<8), tmpNode->bright[i]);
        	break;
          
        default:
          sprintf(str, "%d)%02x:%02x-%d%% ", i+1, tmpNode->hour[i], tmpNode->min[i], tmpNode->bright[i]);
          break;
      }
      guiDisplay(25, tmpY, str,1);
      tmpY += 16;
    }
	}
	 
	lcdRefresh(17, 145);
}	

/*******************************************************
函数名称:setLearnIr
功能描述:遥控器学习红外命令(376.1国家电网集中器规约)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void setLearnIr(INT8U lightNum)
{
	INT8U i;
	
	menuInLayer = 3;
  
	guiLine(1,17,160,144,0);

	for(i=0; i<8; i++)
	{
		if (i==lightNum)
		{
			guiDisplay(41, 17+16*i, irMenuItem[i], 0);
		}
		else
		{
			guiDisplay(41, 17+16*i, irMenuItem[i], 1);
		}		
	}
	
	lcdRefresh(17, 144);
}

	 
#endif



/*******************************************************
函数名称:copyQueryMenu
功能描述:抄表查询菜单
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void copyQueryMenu(INT8U layer2Light,INT8U layer3Light)
{
   INT8U                dataBuff[LENGTH_OF_ENERGY_RECORD];     //上一次抄表电能量数据
 	 INT32U               integer,decimal;
   struct cpAddrLink    *tmpLink;
	 char                 str[15];
	 char                 sayStr[10];
	 INT8U                i, tmpX, tmpCount;
	 DATE_TIME            tmpTime;
	 INT16U               offset;
	 BOOL                 buffHasData;
	 INT8U                meterInfo[10];
	 BOOL                 singleMeter=0;
   
   if (layer2Light>0)
   {
     guiLine(16,56,144,88,0);
     guiLine(16,56,16,88,1);
     guiLine(144,56,144,88,1);
     guiLine(16,88,144,88,1);
     guiLine(16,56,144,56,1);
     guiDisplay(30,64,"读取数据中...",1);
     lcdRefresh(56,88);
   }
   
 	#ifdef MENU_FOR_CQ_CANON 
 	 guiLine(1,17,160,160,0); //清屏
 	#else
 	 guiLine(1,17,160,144,0); //清屏
 	#endif
 	
	 menuInLayer = 2;         //菜单进入第2层
	 
	 switch(layer2Light)
	 {
	 	 case 0:
	 	   guiDisplay(1, 18, "1", 1);
	 	   guiDisplay(6, 18, "-", 1);
	 	   guiDisplay(11,18, "1", 1);
	 	   
	 	   #ifdef MENU_FOR_CQ_CANON
	 	    guiDisplay(17,17,"测量点有功抄表查询",1);
	 	   #else
	 	    guiDisplay(17,17,"测量点数据查询",1);
	 	   #endif
	 	   
	 	   showInputTime(layer3Light);
	 	 	 break;
     
    #ifdef MENU_FOR_CQ_CANON
	 	 case 1:
	 	   guiDisplay(1,17,"1-2测量点有功总示值",1);
	 	   offset = POSITIVE_WORK_OFFSET;
	 	   break;

	 	 case 2:
	 	   guiDisplay(1,17,"1-3测量点有功尖示值",1);
	 	   offset = POSITIVE_WORK_OFFSET+4;
	 	 	 break;
	 	 	 
	 	 case 3:
	 	   guiDisplay(1,17,"1-4测量点有功峰示值",1);
	 	   offset = POSITIVE_WORK_OFFSET+8;
	 	 	 break;

	 	 case 4:
	 	   guiDisplay(1,17,"1-5测量点有功平示值",1);
	 	   offset = POSITIVE_WORK_OFFSET+12;
	 	 	 break;

	 	 case 5:
	 	   guiDisplay(1,17,"1-6测量点有功谷示值",1);
	 	   offset = POSITIVE_WORK_OFFSET+16;
	 	 	 break;
	 	 	 
	 	#else
	 	
	 	 case 1:
	 	   guiDisplay(1,17,"1-2正向有功总示值",1);
	 	   offset = POSITIVE_WORK_OFFSET;
	 	   break;

	 	 case 2:
	 	   guiDisplay(1,17,"1-3正向有功尖示值",1);
	 	   offset = POSITIVE_WORK_OFFSET+4;
	 	 	 break;
	 	 	 
	 	 case 3:
	 	   guiDisplay(1,17,"1-4正向有功峰示值",1);
	 	   offset = POSITIVE_WORK_OFFSET+8;
	 	 	 break;

	 	 case 4:
	 	   guiDisplay(1,17,"1-5正向有功平示值",1);
	 	   offset = POSITIVE_WORK_OFFSET+12;
	 	 	 break;

	 	 case 5:
	 	   guiDisplay(1,17,"1-6正向有功谷示值",1);
	 	   offset = POSITIVE_WORK_OFFSET+16;
	 	 	 break;
	 	 	 
	 	 case 6:
	 	   guiDisplay(1,17,"1-7反向有功总示值",1);
	 	   break;

	 	 case 7:
	 	   guiDisplay(1,17,"1-8反向有功尖示值",1);
	 	 	 break;
	 	 	 
	 	 case 8:
	 	   guiDisplay(1,17,"1-9反向有功峰示值",1);
	 	 	 break;

	 	 case 9:
	 	   guiDisplay(1,17,"1-10反向有功平示值",1);
	 	 	 break;

	 	 case 10:
	 	   guiDisplay(1,17,"1-11反向有功谷示值",1);
	 	 	 break;
	 	 	 
	 	 case 11:
	 	   guiDisplay(1,17,"1-12正向无功总示值",1);
	 	   break;

	 	 case 12:
	 	   guiDisplay(1,17,"1-13正向无功尖示值",1);
	 	 	 break;
	 	 	 
	 	 case 13:
	 	   guiDisplay(1,17,"1-14正向无功峰示值",1);
	 	 	 break;

	 	 case 14:
	 	   guiDisplay(1,17,"1-15正向无功平示值",1);
	 	 	 break;

	 	 case 15:
	 	   guiDisplay(1,17,"1-16正向无功谷示值",1);
	 	 	 break;
	 	 	 
	 	 case 16:
	 	   guiDisplay(1,17,"1-17反向无功总示值",1);
	 	   break;

	 	 case 17:
	 	   guiDisplay(1,17,"1-18反向无功尖示值",1);
	 	 	 break;
	 	 	 
	 	 case 18:
	 	   guiDisplay(1,17,"1-19反向无功峰示值",1);
	 	 	 break;

	 	 case 19:
	 	   guiDisplay(1,17,"1-20反向无功平示值",1);
	 	 	 break;

	 	 case 20:
	 	   guiDisplay(1,17,"1-21反向无功谷示值",1);
	 	 	 break;
	 	#endif
	 }

	#ifdef MENU_FOR_CQ_CANON 
	 if (layer2Light>=1 && layer2Light<=5)
	#else
	 if (layer2Light>=1 && layer2Light<=20)
	#endif
	 {
 	   if (layer3Light>0)
 	   {
 	     guiDisplay(144,33,"↑",1);
 	   }
     
     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
     {
 	     guiDisplay(144,144,"↓",1);
 	   }
 	   
 	   tmpX = 33;
 	   tmpLink = queryMpLink;
 	   i = 0;
 	   while(tmpLink!=NULL && i<layer3Light)
 	   {
 	    #ifdef MENU_FOR_CQ_CANON
 	     for(tmpCount=0;tmpCount<NUM_MP_PER_PAGE;tmpCount++)
 	    #else
 	     for(tmpCount=0;tmpCount<NUM_MP_PER_PAGE-1;tmpCount++)
 	    #endif
 	     {
 	   	   tmpLink = tmpLink->next;
 	     }
 	     i++;
 	   }
 	   
 	   tmpCount = 0;
 	  #ifdef MENU_FOR_CQ_CANON
 	   while((tmpLink!=NULL) && (tmpCount<NUM_MP_PER_PAGE))
 	  #else
 	   while((tmpLink!=NULL) && (tmpCount<NUM_MP_PER_PAGE-1))
 	  #endif
 	   {
 	   	  strcpy(sayStr,intToString(tmpLink->mp,3,str));
 	   	  strcpy(str,"");
 	   	  if (strlen(sayStr)==1)
 	   	  {
 	   	  	 strcat(str,"00");
 	   	  }
 	   	  if (strlen(sayStr)==2)
 	   	  {
 	   	  	 strcat(str,"0");
 	   	  }	 	   	  
 	   	  strcat(str,sayStr);
 	   	  guiDisplay(30,tmpX,str,1);
 	   	  
 	   	  strcpy(sayStr,"");
 	   	  tmpTime = timeHexToBcd(menuQueryTime);
        
        singleMeter=0;
        queryMeterStoreInfo(tmpLink->mp,meterInfo);
        if (tmpLink->port==PORT_POWER_CARRIER)
        {
          if (layer2Light>=11)
          {
          	guiDisplay(60,tmpX,"    -----",1);

          	goto forgotThis;
          }
          
          singleMeter=1;
          
          #ifdef CQDL_CSM
           if (meterInfo[0]==8)
           {
             buffHasData = readMeterData(dataBuff, tmpLink->mp, meterInfo[2], DAY_FREEZE_COPY_DATA, &tmpTime, 0);
           }
           else
           {
             buffHasData = readMeterData(dataBuff, tmpLink->mp, meterInfo[2], ENERGY_DATA, &tmpTime, 0);
           }
          #else
           if (tmpLink->bigAndLittleType==0 && tmpLink->protocol==DLT_645_2007)
           {
             if (meterInfo[0]==8)
             {
               buffHasData = readMeterData(dataBuff, tmpLink->mp, meterInfo[2], DAY_FREEZE_COPY_DATA, &tmpTime, 0);
             }
             else
             {
               buffHasData = readMeterData(dataBuff, tmpLink->mp, meterInfo[2], ENERGY_DATA, &tmpTime, 0);
               if (buffHasData==FALSE)
               {
                 buffHasData = readMeterData(dataBuff, tmpLink->mp, meterInfo[1], ENERGY_DATA, &tmpTime, 0);
               }
             }
           }
           else
           {
             buffHasData = readMeterData(dataBuff, tmpLink->mp, meterInfo[1], ENERGY_DATA, &tmpTime, 0);
           }
          #endif
        }
        else
        {
          if (meterInfo[0]==1)
          {
            if (layer2Light>=11)
            {
          	  guiDisplay(60,tmpX,"    -----",1);

          	  goto forgotThis;
            }

            singleMeter=1;
            
            buffHasData = readMeterData(dataBuff, tmpLink->mp, meterInfo[1], ENERGY_DATA, &tmpTime, 0);
          }
          else
          {
            buffHasData = readMeterData(dataBuff, tmpLink->mp, LAST_TODAY, ENERGY_DATA, &tmpTime, 0);
          }
        }
       	  
       	
     	  switch(layer2Light)
     	  {
       	 	 case 6:
       	 	   if (singleMeter==1)
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET_S;
       	 	   }
       	 	   else
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET;
       	 	   }
       	 	   break;
       
       	 	 case 7:
       	 	   if (singleMeter==1)
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET_S+4;
       	 	   }
       	 	   else
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET+4;
       	 	   }
       	 	 	 break;
       	 	 	 
       	 	 case 8:
       	 	   if (singleMeter==1)
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET_S+8;
       	 	   }
       	 	   else
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET+8;
       	 	   }
       	 	 	 break;
       
       	 	 case 9:
       	 	   if (singleMeter==1)
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET_S+12;
       	 	   }
       	 	   else
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET+12;
       	 	   }
       	 	 	 break;
       
       	 	 case 10:
       	 	   if (singleMeter==1)
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET_S+16;
       	 	   }
       	 	   else
       	 	   {
       	 	     offset = NEGTIVE_WORK_OFFSET+16;
       	 	   }
       	 	 	 break;
       	 	 	 
       	 	 case 11:
       	 	   offset = POSITIVE_NO_WORK_OFFSET;
       	 	   break;
       
       	 	 case 12:
       	 	   offset = POSITIVE_NO_WORK_OFFSET+4;
       	 	 	 break;
       	 	 	 
       	 	 case 13:
       	 	   offset = POSITIVE_NO_WORK_OFFSET+8;
       	 	 	 break;
       
       	 	 case 14:
       	 	   offset = POSITIVE_NO_WORK_OFFSET+12;
       	 	 	 break;
       
       	 	 case 15:
       	 	   offset = POSITIVE_NO_WORK_OFFSET+16;
       	 	 	 break;
       	 	 	 
       	 	 case 16:
       	 	   offset = NEGTIVE_NO_WORK_OFFSET;
       	 	   break;
       
       	 	 case 17:
       	 	   offset = NEGTIVE_NO_WORK_OFFSET+4;
       	 	 	 break;
       	 	 	 
       	 	 case 18:
       	 	   offset = NEGTIVE_NO_WORK_OFFSET+8;
       	 	 	 break;
       
       	 	 case 19:
       	 	   offset = NEGTIVE_NO_WORK_OFFSET+12;
       	 	 	 break;
       
       	 	 case 20:
       	 	   offset = NEGTIVE_NO_WORK_OFFSET+16;
       	 	 	 break;
       	}      
        
        if (buffHasData==TRUE)
        {
       	   if (dataBuff[offset]!=0xee)
       	   {
              decimal = (dataBuff[offset]>>4 & 0xf) * 10 + (dataBuff[offset] & 0xf);
              integer = (dataBuff[offset+3]>>4 & 0xf)*100000
                          +(dataBuff[offset+3] & 0xf)*10000
                          +(dataBuff[offset+2]>>4 & 0xf)*1000
                          +(dataBuff[offset+2] & 0xf)*100
                          +(dataBuff[offset+1]>>4 & 0xf)*10
                          +(dataBuff[offset+1]& 0xf);
              strcpy(str, floatToString(integer,decimal,2,2,str));
              for(i=0;i<9-strlen(str);i++)
              {
              	strcat(sayStr," ");
              }
              strcat(sayStr,str);
       	   }
       	   else
       	   {
       	   	  strcpy(sayStr,"    -----");
           }
 	   	     guiDisplay(60,tmpX,sayStr,1);
        }
        else
        {
 	   	    guiDisplay(60,tmpX,"    -----",1);
        }

forgotThis:
 	   	  tmpX += 16;
 	   	  tmpLink = tmpLink->next;
 	   	  
 	   	  tmpCount++;
 	   }
	 }
	 
	#ifdef MENU_FOR_CQ_CANON 
	 lcdRefresh(17, 160);
	#else
	 lcdRefresh(17, 145);
	#endif
}

/*******************************************************
函数名称:commParaSetMenu
功能描述:通信参数设置菜单
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void commParaSetMenu(INT8U layer2Light,INT8U layer3Light)
{
	 char  str[20];
	 INT8U i, tmpX, tmpY;
   
  #ifdef MENU_FOR_CQ_CANON 
   guiLine(1,17,160,160,0); //清屏
  #else
   guiLine(1,17,160,144,0); //清屏
	 
	 guiLine(1,17,1,140,1);
	 guiLine(160,17,160,140,1);
	 guiLine(80,119,80,140,1);	  
	 guiLine(1,17,160,17,1);
	 guiLine(1,119,160,119,1);
	 guiLine(1,140,160,140,1);
  #endif
  
   menuInLayer = 3;         //菜单进入第3层
   #ifdef MENU_FOR_CQ_CANON
    tmpY = 17;
   #else   
    tmpY = 19;
   #endif
   
   if (layer3Light!=0xff)
   {
    #ifdef MENU_FOR_CQ_CANON
     guiDisplay(1,tmpY,"5-1 通信参数设置",1);
     tmpY += 16;
    #endif
     
     //行政区划
     guiDisplay(1,tmpY,"行政区划",1);
     tmpX = 68;
     for(i=0;i<4;i++)
     {
       str[0] = commParaItem[0][i];
       str[1] = '\0';
       if (layer2Light==0 && i==layer3Light)
       {
         guiDisplay(tmpX,tmpY,str,0);
       }
       else
       {
         guiDisplay(tmpX,tmpY,str,1);
       }
       tmpX += 8;
     }
     tmpY += 16;
     
     //上行信道
    #ifdef MENU_FOR_CQ_CANON 
     guiDisplay(1,tmpY,"上行信道",1);
     switch(moduleType)
     {
     	  case GPRS_SIM300C:
     	  case GPRS_GR64:
     	  case GPRS_MC52I:
     	  case GPRS_M590E:
     	  case GPRS_M72D:
     	  	strcpy(str, "GPRS");
     	  	break;
  
     	  case CDMA_DTGS800:
     	  case CDMA_CM180:
     	  	strcpy(str, "CDMA");
     	  	break;
  
				case LTE_AIR720H:
					strcpy(str, "LTE");
					break;
				
     	  case ETHERNET:
     	  	strcpy(str, "以太网");
     	  	break;
     	  
     	  default:
     	  	strcpy(str,"无模块");
     	  	break;
     }
     guiDisplay(68,tmpY,str,1);
     tmpY += 16;
    #endif
     
     //APN域名
     guiDisplay(1,  tmpY, "APN",1);
     if (layer2Light==1)
     {
       if (layer3Light<5)
       {       
         strcpy(commParaItem[1],(char *)teApn[layer3Light]);
       }
       guiDisplay(32, tmpY, commParaItem[1], 0);
     }
     else
     {
       guiDisplay(32, tmpY, commParaItem[1], 1);
     }
     tmpY += 16;
     
     guiDisplay(1,tmpY,"主站IP及接入端口",1);
     tmpY += 16;
     
     tmpX = 1;
     for(i=0;i<21;i++)
     {
       str[0] = commParaItem[2][i];
       str[1] = '\0';
       if (layer2Light==2 && layer3Light==i)
       {
         guiDisplay(tmpX,tmpY,str,0);
         tmpX += 8;
       }
       else
       {
         guiDisplay(tmpX,tmpY,str,1);
         tmpX += 7;
       }
     }
     tmpY += 16;
  
     guiDisplay(1,tmpY,"备用IP及接入端口",1);
     tmpY += 16;
     
     tmpX = 1;
     for(i=0;i<21;i++)
     {
       str[0] = commParaItem[3][i];
       str[1] = '\0';
       if (layer2Light==3 && layer3Light==i)
       {
         guiDisplay(tmpX,tmpY,str,0);
         tmpX += 8;
       }
       else
       {
         guiDisplay(tmpX,tmpY,str,1);
         tmpX += 7;
       }
     }
     tmpY += 16;
   }
   else
   {
     tmpY = 40;
     guiDisplay(12,tmpY,"再次确定要继续吗?",1);
     tmpY += 20;
   }
   
  #ifdef MENU_FOR_CQ_CANON
   guiLine(26,tmpY,26,tmpY+15,1);
   guiLine(61,tmpY,61,tmpY+15,1);
   guiLine(26,tmpY,61,tmpY,1);
   guiLine(26,tmpY+15,61,tmpY+15,1);

   guiLine(98,tmpY,98,tmpY+15,1);
   guiLine(134,tmpY,134,tmpY+15,1);
   guiLine(98,tmpY,134,tmpY,1);
   guiLine(98,tmpY+15,134,tmpY+15,1);

  #else
   tmpY = 121;
  #endif 
  
   if (layer2Light==4)
   {
     guiDisplay(28,tmpY,"确定",0);     
   }
   else
   {
     guiDisplay(28,tmpY,"确定",1);
   }
   
   guiDisplay(100,tmpY,"取消",1);
   
  #ifdef MENU_FOR_CQ_CANON
   lcdRefresh(17,160);
  #else
   lcdRefresh(17,145);
  #endif
}

/*******************************************************
函数名称:adjustCommParaLight
功能描述:调整通信参数高亮项
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void adjustCommParaLight(INT8U leftRight)
{
 	 INT8U tmpData;
 	 
 	 switch (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
 	 {
 	 	  case 0:
 	 	  	tmpData = 4;
 	 	  	break;
 	 	  	
 	 	  case 1:
 	 	    tmpData = 4;
 	 	    break;
 	 	    
 	 	  case 2:
 	 	  case 3:
 	 	  	tmpData = 21;
 	 	  	break;
 	 }
 	 if (leftRight==1)  //右键
 	 {
 	   if (keyLeftRight>=tmpData-1)
 	   {
 	 	   keyLeftRight = 0;
 	   }
 	   else
 	   {
 	 	   keyLeftRight++;
 	   }
 	 }
 	 else
 	 {
 	   if (keyLeftRight==0)
 	   {
 	 	   keyLeftRight = tmpData-1;
 	   }
 	   else
 	   {
 	 	   keyLeftRight--;
 	   }
 	 }
	 
	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2 
	 	 	   || layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
	 {
	 	 if ((keyLeftRight+1)%4==0 && keyLeftRight<16)
	 	 {
	 	   if (leftRight==1)
	 	   {
	 	     keyLeftRight++;
	 	   }
	 	   else
	 	   {
	 	   	 keyLeftRight--;
	 	   }
	 	 }
	 }
}

/*******************************************************
函数名称:commParaSetMenu
功能描述:通信参数设置菜单
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void modifyPasswordMenu(INT8U layer2Light,INT8U layer3Light)
{
	 char  str[20];
	 INT8U i, tmpX;

	 #ifdef MENU_FOR_CQ_CANON
	  guiLine(1,17,160,160,0); //清屏
	 #else
	  guiLine(1,17,160,144,0); //清屏
	  
	  guiLine(1,17,1,140,1);
	  guiLine(160,17,160,140,1);
	  guiLine(80,119,80,140,1);	  
	  guiLine(1,17,160,17,1);
	  guiLine(1,119,160,119,1);
	  guiLine(1,140,160,140,1);
	 #endif
	 
	 menuInLayer = 3;         //菜单进入第3层	 
	 
   if (layer3Light!=0xff)
   {
     #ifdef MENU_FOR_CQ_CANON
      guiDisplay(1,17,"5-2 修改密码",1);

      guiDisplay(1,40,"输入新密码",1);
      tmpX = 17;
      for(i=0;i<6;i++)
      {
        str[0] = commParaItem[0][i];
        str[1] = '\0';
        if (layer2Light==0 && i==layer3Light)
        {
          guiDisplay(tmpX,58,str,0);
        }
        else
        {
          guiDisplay(tmpX,58,str,1);
        }
        tmpX += 8;
      }
 
      guiDisplay(1,80,"确认新密码",1);
      tmpX = 17;
      for(i=0;i<6;i++)
      {
        str[0] = commParaItem[1][i];
        str[1] = '\0';
        if (layer2Light==1 && i==layer3Light)
        {
          guiDisplay(tmpX,98,str,0);
        }
        else
        {
          guiDisplay(tmpX,98,str,1);
        }
        tmpX += 8;
      }
     #else
      guiDisplay(36,20,"输入新密码",1);
      tmpX = 52;
      for(i=0;i<6;i++)
      {
        str[0] = commParaItem[0][i];
        str[1] = '\0';
        if (layer2Light==0 && i==layer3Light)
        {
          guiDisplay(tmpX,38,str,0);
        }
        else
        {
          guiDisplay(tmpX,38,str,1);
        }
        tmpX += 8;
      }
 
      guiDisplay(36,60,"确认新密码",1);
      tmpX = 52;
      for(i=0;i<6;i++)
      {
        str[0] = commParaItem[1][i];
        str[1] = '\0';
        if (layer2Light==1 && i==layer3Light)
        {
          guiDisplay(tmpX,78,str,0);
        }
        else
        {
          guiDisplay(tmpX,78,str,1);
        }
        tmpX += 8;
      }
     #endif
   }
   else
   {
     #ifdef MENU_FOR_CQ_CANON
      guiDisplay(1,17,"5-3",1);
     #endif
     
     guiDisplay(12,40,"再次确定要继续吗?",1);
   }   

  #ifdef MENU_FOR_CQ_CANON 
   guiLine(26,129,26,144,1);
   guiLine(61,129,61,144,1);
   guiLine(26,129,61,129,1);
   guiLine(26,144,61,144,1);

   guiLine(98,129,98,144,1);
   guiLine(134,129,134,144,1);
   guiLine(98,129,134,129,1);
   guiLine(98,144,134,144,1);
  #endif

   if (layer2Light==2)
   {
     #ifdef MENU_FOR_CQ_CANON
      guiDisplay(28,129,"确定",0);
     #else
      guiDisplay(28,121,"确定",0);
     #endif
   }
   else
   {
     #ifdef MENU_FOR_CQ_CANON
      guiDisplay(28,129,"确定",1);
     #else
      guiDisplay(28,121,"确定",1);
     #endif
   }
   
  #ifdef MENU_FOR_CQ_CANON 
   guiDisplay(100,129,"取消",1);
  #else
   guiDisplay(100,121,"取消",1);
  #endif
   
   #ifdef MENU_FOR_CQ_CANON
    lcdRefresh(17,160);
   #else
    lcdRefresh(17,145);
   #endif
}

/*******************************************************
函数名称:debugMenu
功能描述:现场调试菜单
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void realCopyMeterMenu(int lightNum)
{
	 INT8U i;

	#ifdef MENU_FOR_CQ_CANON 
	 guiLine(1,17,160,160,0); //清屏
	 guiDisplay(1,17,"6-1 实时抄表",1);
	#else
	 guiLine(1,17,160,144,0); //清屏
	#endif
	 menuInLayer = 3;         //菜单进入第3层
	 
	 for(i=0;i<2;i++)
	 {
		 if (i==lightNum)
		 {
        guiDisplay(12,50+i*18,realCopyMeterItem[i],0);
		 }
		 else
		 {
        guiDisplay(12,50+i*18,realCopyMeterItem[i],1);
		 }
	 }
	
	#ifdef MENU_FOR_CQ_CANON 
	 lcdRefresh(17, 160);
	#else
	 lcdRefresh(17, 145);
	#endif
}

/*******************************************************
函数名称:searchMeter
功能描述:搜索表号
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void searchMeter(INT8U lightNum)
{
	 INT8U tmpX, i;
   char  str[2];
   
  #ifdef MENU_FOR_CQ_CANON 
   if (menuInLayer==3 && layer1MenuLight==5 && layer2MenuLight[layer1MenuLight]==2)
  #else
   if (menuInLayer==3 && layer1MenuLight==2 && layer2MenuLight[layer1MenuLight]==2)
  #endif
   {
   	 #ifdef MENU_FOR_CQ_CANON
   	  guiLine(1,17,160,160,0); //清屏
      guiDisplay(1,17,"6-5表号搜索",1);
   	 #else
   	  guiLine(1,17,160,144,0); //清屏
      guiDisplay(1,17,"   表号搜索",1);
   	 #endif
   	 
   	  menuInLayer = 3;         //菜单进入第3层
      
   
      if (lightNum!=0xff)
      {
        guiLine(45,60,45,78,1);
        guiLine(115,60,115,78,1);
        guiLine(45,60,115,60,1);
        guiLine(45,78,115,78,1);
   
        if (lightNum==0)
        {
          guiDisplay(48,61,"启动搜索",0);
        }
        else
        {
          guiDisplay(48,61,"启动搜索",1);
        }
      }
      else
      {
      	 if (carrierFlagSet.searchMeter==0)
      	 {
      	   guiDisplay(32,120,"表号搜索完毕",1);
      	 }
      	 else
      	 {
      	   guiDisplay(28,120,"表号搜索中...",1);   	 	 
      	 }
      }
      
     #ifdef MENU_FOR_CQ_CANON
      lcdRefresh(17,160);
     #else
      lcdRefresh(17,145);
     #endif
   }
}

/*******************************************************
函数名称:searchMeterReport
功能描述:搜索电表结果报告
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void searchMeterReport(void)
{
   char   str[15];
   
   if (menuInLayer==3 && layer1MenuLight==5 && layer2MenuLight[layer1MenuLight]==3)
   {
   	  newAddMeter(keyLeftRight);
   }
}

/*******************************************************
函数名称:newAddMeter
功能描述:新增电能表地址
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void newAddMeter(INT8U lightNum)
{
	 INT8U  tmpY, i;
	 INT16U tmpNum;
   char   str[10],sayStr[15];
   
	#ifdef MENU_FOR_CQ_CANON 
	 guiLine(1,17,160,160,0); //清屏
   if (carrierModuleType==SR_WIRELESS || carrierModuleType==HWWD_WIRELESS || carrierModuleType==RL_WIRELESS)
   {
     guiDisplay(1,17,"6-6入网节点地址",1);
   }
   else
   {
     guiDisplay(1,17,"6-6新增电能表地址",1);
   }
	#else
	 guiLine(1,17,160,144,0); //清屏
   if (carrierModuleType==SR_WIRELESS || carrierModuleType==RL_WIRELESS || carrierModuleType==HWWD_WIRELESS)
   {
     guiDisplay(1,17,"  未入网节点地址",1);
   }
   else
   {
     guiDisplay(1,17,"   新增电能表地址",1);
   }
	#endif
	
	 menuInLayer = 3;         //菜单进入第3层

   if (lightNum!=0xff)
   {
		 //计算最大页数
		 multiCpMax = 0;
		 #ifdef MENU_FOR_CQ_CANON
		  tmpFound = foundMeterHead;
		 #else
		   if (carrierModuleType==RL_WIRELESS || carrierModuleType==SR_WIRELESS)
		   {
		     tmpFound = noFoundMeterHead;
		   }
		   else
		   {
		     tmpFound = foundMeterHead;		   	 
		   }
		 #endif

		 while(tmpFound!=NULL)
		 {
			 multiCpMax++;
			 tmpFound = tmpFound->next;
		 }
		 
		#ifdef MENU_FOR_CQ_CANON
		 if ((multiCpMax%(NUM_MP_PER_PAGE-1))!=0)
		 {
		    multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-1)+1;
		 }
		 else
		 {
		   multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-1);
		 }
		#else
		 if ((multiCpMax%(NUM_MP_PER_PAGE-2))!=0)
		 {
		    multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-2)+1;
		 }
		 else
		 {
		   multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-2);
		 }
		#endif

     //滚动到需要显示的页
		 #ifdef MENU_FOR_CQ_CANON
		  tmpFound = foundMeterHead;
		 #else
		   if (carrierModuleType==RL_WIRELESS || carrierModuleType==SR_WIRELESS)
		   {
		     tmpFound = noFoundMeterHead;
		   }
		   else
		   {
		     tmpFound = foundMeterHead;
		   }
		 #endif

     if (tmpFound!=NULL)
     {
       #ifdef MENU_FOR_CQ_CANON
        for(i=0;i<multiCpUpDown*(NUM_MP_PER_PAGE-1);i++)
       #else
        for(i=0;i<multiCpUpDown*(NUM_MP_PER_PAGE-2);i++)
       #endif
        {
      	  tmpFound = tmpFound->next;      	  
      	  
      	  if (tmpFound==NULL)
      	  {
      	  	 break;
      	  }
        }
     }

     tmpY     = 33;
     tmpNum   = 0;
     while(tmpFound!=NULL)
     {
     	  strcpy(sayStr,"");
     	  for(i=6;i>0;i--)
     	  {
     	    strcat(sayStr,digital2ToString((tmpFound->addr[i-1]/0x10)*10 + tmpFound->addr[i-1]%0x10,str));
     	  }
     	  
     	  guiDisplay(32,tmpY, sayStr, 1);
     	  
     	  tmpY += 16;
     	  tmpNum++;
     	 #ifdef MENU_FOR_CQ_CANON
     	  if (tmpNum>=(NUM_MP_PER_PAGE-1))
     	 #else
     	  if (tmpNum>=(NUM_MP_PER_PAGE-2))
     	 #endif
     	  {
     	  	 break;
     	  }
     	  
     	  tmpFound = tmpFound->next;
     }
     
     if (carrierModuleType==SR_WIRELESS || carrierModuleType==HWWD_WIRELESS || carrierModuleType==RL_WIRELESS)
     {
     	 ;
     }
     else
     {
      #ifdef MENU_FOR_CQ_CANON 
       guiLine(45,144,45,160,1);
       guiLine(115,144,115,160,1);
       guiLine(45,144,115,144,1);
       guiLine(45,160,115,160,1);
  
       if (lightNum==0)
       {
         guiDisplay(48,144,"启动抄表",0);
       }
       else
       {
         guiDisplay(48,144,"启动抄表",1);
       }
      #else
       guiLine(45,129,45,144,1);
       guiLine(115,129,115,144,1);
       guiLine(45,129,115,129,1);
       guiLine(45,144,115,144,1);
  
       if (lightNum==0)
       {
         guiDisplay(48,129,"启动抄表",0);
       }
       else
       {
         guiDisplay(48,129,"启动抄表",1);
       }
      #endif
     }
   }
   
  #ifdef MENU_FOR_CQ_CANON 
   lcdRefresh(17,160);
  #else
   lcdRefresh(17,145);
  #endif
}

/*******************************************************
函数名称:singleMeterCopy
功能描述:指定测量点抄表(单个测量点)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void singleMeterCopy(char *mp,char *time,INT8U *energyData,INT8U lightNum)
{
	 INT8U tmpX, i;
   char  str[2];
   
	 #ifdef MENU_FOR_CQ_CANON
	  guiLine(1,17,160,160,0); //清屏
	 #else
	  guiLine(1,17,160,144,0); //清屏
	 #endif
	 
	 menuInLayer = 4;         //菜单进入第4层
   
  #ifdef MENU_FOR_CQ_CANON
   guiDisplay(1,17,"6-2指定测量点抄表",1);
  #else
   guiDisplay(1,17,"   指定测量点抄表",1);
  #endif
   guiDisplay(1,40,"输入测量点号",1);
   
   tmpX = 100;
   for(i=0;i<6;i++)
   {
     str[0] = mp[i];
     str[1] = '\0';
     if (i==lightNum)
     {
       guiDisplay(tmpX,40,str,0);
     }
     else
     {
     	 guiDisplay(tmpX,40,str,1);
     }
     tmpX += 8;
   }   
   guiDisplay(1,60,"电能表时间",1);
   guiDisplay(84,60,time,1);
   guiDisplay(1,80,"有功总示值",1);
   guiDisplay(84,80,energyData,1);
   
   if (pDotCopy!=NULL)
   {
   	 if (pDotCopy->dotRecvItem<pDotCopy->dotTotalItem)
   	 {
        guiDisplay(44,120,"抄表中...",1);
   	 }
   }
   
   #ifdef MENU_FOR_CQ_CANON
    lcdRefresh(17,160);
   #else
    lcdRefresh(17,145);
   #endif
}

#ifdef LIGHTING

/*******************************************************
函数名称:singleMeterCopy
功能描述:指定控制点点抄
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void singleCcbCopy(char *mp, char *time, INT8U *energyData, INT8U lightNum)
{
	INT8U tmpX, i;
  char  str[2];
   
	guiLine(1,17,160,144,0); //清屏
	 
	menuInLayer = 4;         //菜单进入第4层
   
  guiDisplay(1,17," 查询指定控制点状态 ",0);
  guiDisplay(1,40,"输入控制点号",1);
   
  tmpX = 100;
  for(i=0;i<6;i++)
  {
    str[0] = mp[i];
    str[1] = '\0';
    if (i==lightNum)
    {
      guiDisplay(tmpX, 40, str, 0);
    }
    else
    {
     	guiDisplay(tmpX, 40, str, 1);
    }
    tmpX += 8;
  }
  guiDisplay(1,60,"当前亮度:",1);
  guiDisplay(84,60,time,1);
  guiDisplay(1,80,"主站命令:",1);
  guiDisplay(84,80,energyData,1);
   
  if (pDotCopy!=NULL)
  {
   	if (pDotCopy->dotRecvItem<pDotCopy->dotTotalItem)
   	{
      guiDisplay(44,120,"查询中...",1);
   	}
  }
   
  lcdRefresh(17,145);
}

#endif

/*******************************************************
函数名称:singleMeterCopyReport
功能描述:指定测量点抄表(单个测量点)抄表结果报告
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void singleMeterCopyReport(INT8U *data)
{
  #ifdef LIGHTING
   
   if (menuInLayer==4 && layer1MenuLight==2 && layer2MenuLight[layer1MenuLight]==0 && layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
   {
     if (data!=NULL)
     {
       if (data[0]<=100)
       {
       	 sprintf(singleCopyEnergy, "%d%%", data[0]);
       }
       else
       {
       	 strcpy(singleCopyEnergy, "无");
       }

       if (data[7]<100)
       {
       	 sprintf(singleCopyTime, "%d%%", data[7]);
       }
       else
       {
       	 strcpy(singleCopyTime, "未知");
       }
       
     }
     else
     {
     	 strcpy(singleCopyTime, "-----");
     	 strcpy(singleCopyEnergy, "-----");
     }
     
     singleCcbCopy(singleCopyMp, singleCopyTime, singleCopyEnergy, keyLeftRight);
   }
   
  #else
  
   char   str[15];
 	 INT32U integer,decimal;
   
  #ifdef MENU_FOR_CQ_CANON
   if (menuInLayer==4 && layer1MenuLight==5 && layer2MenuLight[layer1MenuLight]==0 && layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
  #else
   if (menuInLayer==4 && layer1MenuLight==2 && layer2MenuLight[layer1MenuLight]==0 && layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
  #endif
   {
     if (data!=NULL)
     {
       if (*(data+20)!=0xee)
       {
         strcpy(singleCopyTime, digital2ToString(bcdToHex(*(data+22)),str));
         strcat(singleCopyTime, ":");
         strcat(singleCopyTime,digital2ToString(bcdToHex(*(data+21)),str));
         strcat(singleCopyTime, ":");
         strcat(singleCopyTime,digital2ToString(bcdToHex(*(data+20)),str));
       }
       else
       {
     	   strcpy(singleCopyTime,"-----");
       }
       
       if (*data!=0xee)
       {
         decimal = (*data>>4 & 0xf) * 10 + (*data & 0xf);
         integer = (*(data+3)>>4 & 0xf)*100000
             +(*(data+3) & 0xf)*10000
             +(*(data+2)>>4 & 0xf)*1000
             +(*(data+2) & 0xf)*100
             +(*(data+1)>>4 & 0xf)*10
             +(*(data+1)& 0xf);
         strcpy(singleCopyEnergy, floatToString(integer,decimal,2,2,str));
       }
       else
       {
     	   strcpy(singleCopyEnergy,"-----");
       }
     }
     else
     {
     	 strcpy(singleCopyTime,"-----");
     	 strcpy(singleCopyEnergy,"-----");
     }
     
     singleMeterCopy(singleCopyMp, singleCopyTime, singleCopyEnergy, keyLeftRight);
   }
  #endif
}

/*******************************************************
函数名称:allMpCopy
功能描述:全部测量点抄表
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void allMpCopy(INT8U lightNum)
{
	 INT16U tmpY, i, j;
   char   str[15],sayStr[15];
 	 INT32U integer,decimal;

   #ifdef MENU_FOR_CQ_CANON
    if ((menuInLayer==2 || menuInLayer==3 || menuInLayer==4) && layer1MenuLight==5 && (layer2MenuLight[layer1MenuLight]==1 || layer2MenuLight[layer1MenuLight]==0))
   #else
    if ((menuInLayer==2 || menuInLayer==3 || menuInLayer==4) && layer1MenuLight==2 && (layer2MenuLight[layer1MenuLight]==1 || layer2MenuLight[layer1MenuLight]==0))
   #endif
    {
      ;
    }
    else
    {
    	return;
    }
   
	 #ifdef MENU_FOR_CQ_CANON
	  guiLine(1,17,160,160,0); //清屏
	 #else
	  guiLine(1,17,160,144,0); //清屏	 
	 #endif
	 
	 menuInLayer = 4;          //菜单进入第4层
   
   if (lightNum<2)
   {
     #ifdef MENU_FOR_CQ_CANON
      guiDisplay(1,17,"6-3全部测量点抄表",1);
     #else
      guiDisplay(1,17,"   全部测量点抄表",1);
     #endif
      guiDisplay(4,60,"全部抄表将耗时很长!",1);
   
      guiLine(26,100,26,118,1);
      guiLine(61,100,61,118,1);
      guiLine(26,100,61,100,1);
      guiLine(26,118,61,118,1);
   
      guiLine(98,100,98,118,1);
      guiLine(134,100,134,118,1);
      guiLine(98,100,134,100,1);
      guiLine(98,118,134,118,1);
   
      if (lightNum==0)
      {
        guiDisplay(28,101,"启动",0);
      }
      else
      {
        guiDisplay(28,101,"启动",1);
      }
      
      if (lightNum==1)
      {
        guiDisplay(100,101,"取消",0);
      }
      else
      {
        guiDisplay(100,101,"取消",1);
      }
   }
   else   //全部测量点抄表结果
   {
      #ifdef MENU_FOR_CQ_CANON
       guiDisplay(1,17,"6-4全部测量点抄表",1);
      #else
       guiDisplay(1,17,"   全部测量点抄表",1);
      #endif
      
      guiDisplay(1,33,"测量点",1);
      guiDisplay(52,33,"时间  有功总",1);
      
      tmpY = 49;
      tmpMpCopy = mpCopyHead;
      if (tmpMpCopy!=NULL)
      {
       #ifdef MENU_FOR_CQ_CANON 
        for(i=0;i<multiCpUpDown*(NUM_MP_PER_PAGE-1);i++)
       #else
        for(i=0;i<multiCpUpDown*(NUM_MP_PER_PAGE-2);i++)
       #endif
        {
      	  tmpMpCopy = tmpMpCopy->next;
      	  
      	  if (tmpMpCopy==NULL)
      	  {
      	  	 break;
      	  }
        }
      }
      
      j=0;
      while(tmpMpCopy!=NULL)
      {
 	   	   strcpy(sayStr,intToString(tmpMpCopy->mp,3,str));
 	   	   strcpy(str,"");
 	   	   if (strlen(sayStr)==1)
 	   	   {
 	   	  	 strcat(str,"00");
 	   	   }
 	   	   if (strlen(sayStr)==2)
 	   	   {
 	   	  	  strcat(str,"0");
 	   	   }	 	   	  
 	   	   strcat(str,sayStr);
 	   	   guiDisplay(12,tmpY,str,1);

         
         if (tmpMpCopy->copyTime[0]==0xee)
         {
         	 strcpy(sayStr,"-----");
         }
         else
         {
           strcpy(sayStr, digital2ToString(tmpMpCopy->copyTime[1],str));
           strcat(sayStr, ":");
           strcat(sayStr, digital2ToString(tmpMpCopy->copyTime[0],str));
         }
         guiDisplay(48, tmpY, sayStr, 1);
                  
         if (tmpMpCopy->copyEnergy[0]==0xee)
         {
         	 strcpy(sayStr,"  -----");
         }
         else
         {
           decimal = (tmpMpCopy->copyEnergy[0]>>4 & 0xf) * 10 + (tmpMpCopy->copyEnergy[0] & 0xf);
           integer = (tmpMpCopy->copyEnergy[3]>>4 & 0xf)*100000
                   +(tmpMpCopy->copyEnergy[3] & 0xf)*10000
                   +(tmpMpCopy->copyEnergy[2]>>4 & 0xf)*1000
                   +(tmpMpCopy->copyEnergy[2] & 0xf)*100
                   +(tmpMpCopy->copyEnergy[1]>>4 & 0xf)*10
                   +(tmpMpCopy->copyEnergy[1]& 0xf);
           strcpy(str, floatToString(integer,decimal,2,2,str));

           strcpy(sayStr,"");
           for(i=0;i<9-strlen(str);i++)
           {
              strcat(sayStr," ");
           }         
           strcat(sayStr,str);
         }
         guiDisplay(88,tmpY,sayStr,1);
      	 
      	 tmpMpCopy = tmpMpCopy->next;      	 
      	 tmpY += 16;
      	 
      	 j++;
      	#ifdef MENU_FOR_CQ_CANON 
      	 if (j>=(NUM_MP_PER_PAGE-1))
      	#else
      	 if (j>=(NUM_MP_PER_PAGE-2))
      	#endif
      	 {
      	 	  break;
      	 }
      }
   }
  
  #ifdef MENU_FOR_CQ_CANON 
   lcdRefresh(17,160);
  #else
   lcdRefresh(17,145);
  #endif
}

/*******************************************************
函数名称:newAddMeter
功能描述:新增电能表地址
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void newMeterCpStatus(INT8U lightNum)
{
	 INT8U  tmpY, i, j;
	 INT16U tmpNum;
   char   str[10], sayStr[15];
 	 INT32U integer, decimal;
 	 
  #ifdef MENU_FOR_CQ_CANON 
   if (menuInLayer==3 && layer1MenuLight==5 && layer2MenuLight[layer1MenuLight]==4)
  #else
   if (menuInLayer==3 && layer1MenuLight==2 && layer2MenuLight[layer1MenuLight]==4)
  #endif
   {
  	 #ifdef MENU_FOR_CQ_CANON
  	  guiLine(1,17,160,160,0); //清屏
      guiDisplay(1,17,"6-7新增电表抄表状态",1);
  	 #else
  	  guiLine(1,17,160,144,0); //清屏
      guiDisplay(1,17,"   新增电表抄表状态",1);
  	 #endif
  	 menuInLayer = 3;         //菜单进入第3层
  
     guiDisplay(4,33,"电能表地址 抄表示值",1);

		 //计算最大页数
		 multiCpMax = 0;
		 tmpFound = foundMeterHead;
		 while(tmpFound!=NULL)
		 {
			 multiCpMax++;
			 tmpFound = tmpFound->next;
		 }
		 
		#ifdef MENU_FOR_CQ_CANON
		 if ((multiCpMax%(NUM_MP_PER_PAGE-1))!=0)
		 {
		    multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-1)+1;
		 }
		 else
		 {
		   multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-1);
		 }
	  #else
		 if ((multiCpMax%(NUM_MP_PER_PAGE-2))!=0)
		 {
		    multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-2)+1;
		 }
		 else
		 {
		   multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-2);
		 }
	  #endif
		 
     //滚动到需要显示的页
     tmpFound = foundMeterHead;
     if (tmpFound!=NULL)
     {
       #ifdef MENU_FOR_CQ_CANON
        for(i=0;i<multiCpUpDown*(NUM_MP_PER_PAGE-1);i++)
       #else
        for(i=0;i<multiCpUpDown*(NUM_MP_PER_PAGE-2);i++)
       #endif
        {
      	  tmpFound = tmpFound->next;      	  
      	  
      	  if (tmpFound==NULL)
      	  {
      	  	 break;
      	  }
        }
     }
     
     j=0;
     tmpY = 49;
     while(tmpFound!=NULL)
     {
       	strcpy(sayStr,"");
       	for(i=6;i>0;i--)
       	{
       	  strcat(sayStr,digital2ToString((tmpFound->addr[i-1]/0x10)*10 + tmpFound->addr[i-1]%0x10,str));
       	}   	  
     	  guiDisplay(1,tmpY,sayStr,1);
        
        if (tmpFound->copyEnergy[0]==0xee)
        {
        	 strcpy(sayStr,"-----");
        }
        else
        {
          decimal = (tmpFound->copyEnergy[0]>>4 & 0xf) * 10 + (tmpFound->copyEnergy[0] & 0xf);
          integer = (tmpFound->copyEnergy[3]>>4 & 0xf)*100000
                  +(tmpFound->copyEnergy[3] & 0xf)*10000
                  +(tmpFound->copyEnergy[2]>>4 & 0xf)*1000
                  +(tmpFound->copyEnergy[2] & 0xf)*100
                  +(tmpFound->copyEnergy[1]>>4 & 0xf)*10
                  +(tmpFound->copyEnergy[1]& 0xf);
          strcpy(sayStr, floatToString(integer,decimal,2,2,str));
        }
        guiDisplay(105,tmpY,sayStr,1);
     	 
     	  tmpFound = tmpFound->next;      	 
     	  tmpY += 16;
     	 
     	  j++;
     	 #ifdef MENU_FOR_CQ_CANON 
     	  if (j>=(NUM_MP_PER_PAGE-1))
     	 #else
     	  if (j>=(NUM_MP_PER_PAGE-2))
     	 #endif
     	  {
     	 	  break;
     	  }
     }   
    
    #ifdef MENU_FOR_CQ_CANON 
     lcdRefresh(17,160);
    #else
     lcdRefresh(17,145);
    #endif
   }
}

#ifdef MENU_FOR_CQ_CANON  //重庆规约集中器菜单
/**************************************************
函数名称:userInterface
功能描述:人机接口处理(376.1重庆规约)
调用函数:
被调用函数:
输入参数:void *arg
输出参数:
返回值：状态
***************************************************/
void userInterface(BOOL secondChanged)
{
   char   str[30],strX[30];
   INT16U tmpData;
   INT8U  i;
   struct cpAddrLink * tmpNode;
   
   if (aberrantAlarm.aberrantFlag==1)
   {
     if (secondChanged==TRUE)
     {
       guiLine(44,1,62,16,0);
   
       if (aberrantAlarm.blinkCount==1)
       {
         aberrantAlarm.blinkCount = 2;
         guiDisplay(46, 1, "！", 1);
         guiLine(46,1,62,4,0);
         guiDisplay(46, 2, "○", 1);
       }
       else
       {
   	     guiDisplay(46, 1, digital2ToString(aberrantAlarm.eventNum,str), 1);
   	     aberrantAlarm.blinkCount = 1;
   	   }
   	 }
     
     if (compareTwoTime(aberrantAlarm.timeOut, sysTime))
     {
       guiLine(30,1,62,16,0);
       aberrantAlarm.aberrantFlag = 0;
       lcdRefresh(1,16);
     }
   }
   
   //title时间
   if (sysTime.second==0)
   {
      refreshTitleTime();
   }
   else
   {
   	 if (aberrantAlarm.aberrantFlag==1 && secondChanged==TRUE)
   	 {
   	 	 lcdRefresh(1,16);
   	 } 
   }
   
   if (secondChanged==TRUE)
   {
     if (lcdLightOn==LCD_LIGHT_ON)
     {
       if (compareTwoTime(lcdLightDelay,sysTime) && (!(layer1MenuLight>3 && menuInLayer>1)))
       {
     	   printf("%02d-%02d-%02d %02d:%02d:%02d关显示背光\n",lcdLightDelay.year,lcdLightDelay.month,
     	         lcdLightDelay.day,lcdLightDelay.hour,lcdLightDelay.minute,lcdLightDelay.second
     	         );
     	   lcdLightOn = LCD_LIGHT_OFF;
     	   lcdBackLight(LCD_LIGHT_OFF);
     	   
     	   printf("关显示背光\n");
     	  
     	   defaultMenu();
       }
     }
     if (setParaWaitTime!=0 && setParaWaitTime!=0xfe)
     {
    		setParaWaitTime--;
    		if (setParaWaitTime<1)
    		{
         	setBeeper(BEEPER_OFF);         //蜂鸣器
         	alarmLedCtrl(ALARM_LED_OFF);   //指示灯灭
     	    
     	    //lcdBackLight(LCD_LIGHT_OFF);
     	       
     	    //if (menuInLayer==0)
     	    //{
     	    //   defaultMenu();
     	    //}
    		}
     }
   }
   
   if ((keyValue = ioctl(fdOfIoChannel,READ_KEY_VALUE,0))!=0)
   {
      keyPressCount++;
      
      if (keyPressCount>keyCountMax)
      {
        lcdBackLight(LCD_LIGHT_ON);
        lcdLightOn = LCD_LIGHT_ON;
        
        lcdLightDelay = nextTime(sysTime, 0, 30);
        
        keyPressCount = 0;
        
        if (displayMode!=KEYPRESS_DISPLAY_MODE)
        {
           displayMode = KEYPRESS_DISPLAY_MODE;
           layer1Menu(layer1MenuLight);
           return;
        }
        
        switch(keyValue)
        {
      	   case KEY_OK:     //确定
      	   	 switch(menuInLayer)
      	     {
      	    	 case 0:
      	    		 break;
      	    		 	 
      	    	 case 1:     //菜单第1层
      	    		 switch(layer1MenuLight)
      	    		 {
      	    		 	 case 0: //抄表查询
      	    		 	 	 layer2MenuLight[0] = 0;
      	    		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 fillTimeStr();   //用当前日期填充查询日期字符串
      	    		 	 	 queryMpLink = initPortMeterLink(0xff);
      	    		 	 	 copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 1: //参数查询
      	    		 	 	 layer2MenuLight[1] = 0;
      	    		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 
      	    		 	 	 queryMpLink = initPortMeterLink(0xff);
      	    		 	 	 
                     if (layer2MenuLight[layer1MenuLight]==0)
                     {
                       layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = meterDeviceNum/(NUM_MP_PER_PAGE-1);
                       if (meterDeviceNum%(NUM_MP_PER_PAGE-1)!=0)
                       {
                     	   layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
                       }
                       layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                     }
      	    		 	 	 
      	    		 	 	 paraQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 2: //重点用户
      	    		 	 	 layer2MenuLight[2] = 0;
      	    		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 fillTimeStr();   //用当前日期填充查询日期字符串
   	    		 	 	 	   if (keyHouseHold.numOfHousehold>0)
   	    		 	 	 	   {
                        //根据重点用户序号建立重点用户测量点链表
                       tmpPrevMpLink = queryMpLink;                      
   	    		 	 	 	  	 for(i=0;i<keyHouseHold.numOfHousehold;i++)
   	    		 	 	 	  	 {
   	    		 	 	 	  	 	  tmpData = keyHouseHold.household[i*2] | keyHouseHold.household[i*2+1];
                           
                          if (selectF10Data(0, 0, tmpData, (INT8U *)&menuMeterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
                          {
                             tmpMpLink = (struct cpAddrLink *)malloc(sizeof(struct cpAddrLink));
                             tmpMpLink->mpNo = menuMeterConfig.number;       //测量点序号
                             tmpMpLink->mp = menuMeterConfig.measurePoint;   //测量点号
                             tmpMpLink->protocol = menuMeterConfig.protocol; //协议
                             memcpy(tmpMpLink->addr,menuMeterConfig.addr,6); //通信地址
                             tmpMpLink->next = NULL;
                             
                       		   //printf("userInterface测量点%d\n",tmpMpLink->mp);
                       		        
                             if (queryMpLink==NULL)
                             {
                             	 queryMpLink = tmpMpLink;
                             }
                             else
                             {
                             	 tmpPrevMpLink->next = tmpMpLink;
                             }
                             tmpPrevMpLink = tmpMpLink;
                          }
   	    		 	 	 	  	 }
   	    		 	 	 	   }
      	    		 	 	 keyHouseholdMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 3://统计查询
      	    		 	 	 layer2MenuLight[3] = 0;
      	    		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;      	    		 	 	 
      	    		 	 	 fillTimeStr();   //用当前日期填充查询日期字符串
      	    		 	 	 statisQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 4://参数设置
      	    		 	 case 5://现场调试
      	    		 	 	 if (teInRunning==0x01)
      	    		 	 	 {
      	    		 	 	 	  guiLine(1,17,160,160,0); //清屏
      	    		 	 	 	  guiDisplay(32,50,"集中器已投运",1);
      	    		 	 	 	  if (layer1MenuLight==4)
      	    		 	 	 	  {
      	    		 	 	 	    guiDisplay(32,70,"禁止设置参数",1);
      	    		 	 	 	  }
      	    		 	 	 	  else
      	    		 	 	 	  {
      	    		 	 	 	    guiDisplay(32,70,"禁止现场调试",1);
      	    		 	 	 	  }
      	    		 	 	 	  lcdRefresh(17,160);
      	    		 	 	 	  
      	    		 	 	 	  sleep(2);
      	    		 	 	 	  
      	    		 	 	 	  layer1Menu(layer1MenuLight);
      	    		 	 	 }
      	    		 	 	 else
      	    		 	 	 {
      	    		 	 	   pwLight = 0;
      	    		 	 	   strcpy(passWord,"000000");
      	    		 	 	   inputPassWord(pwLight);
      	    		 	 	 }
      	    		 	 	 break;
      	    		 }
      	    		 break;
      	    		 
      	    	 case 2:     //菜单第2层
      	    		 switch(layer1MenuLight)
      	    		 {
      	    		 	 case 0:  //抄表查询
      	    		 	 	 //如果是1-1(选择查询日期)的确认则要判断日期是否正确
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]==0)
      	    		 	 	 {
      	    		 	 	 	  //判断输入的日期是否正确
      	    		 	 	 	  if (checkInputTime()==FALSE)
      	    		 	 	 	  {
      	    		 	 	 	  	 return;
      	    		 	 	 	  }
      	    		 	 	 }
      	    		 	 	 
      	    		 	 	 //按重庆显示规范要求,按确定键可实现不同分类的屏显间的切换
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]>=layer2MenuNum[layer1MenuLight]-1)
      	    		 	 	 {
      	    		 	 	  	layer2MenuLight[layer1MenuLight]=0;
      	    		 	 	 }
      	    		 	 	 else
      	    		 	 	 {
      	    		 	 	  	layer2MenuLight[layer1MenuLight]++;
      	    		 	 	 }
                     
                     if (layer2MenuLight[layer1MenuLight]!=0)
                     {
                       layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = meterDeviceNum/NUM_MP_PER_PAGE;
                       if (meterDeviceNum%NUM_MP_PER_PAGE!=0)
                       {
                     	   layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
                       }
                       layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                     }

      	    		 	 	 copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 1:  //参数查询
      	    		 	 	 //按重庆显示规范要求,按确定键可实现不同分类的屏显间的切换
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]>=layer2MenuNum[layer1MenuLight]-1)
      	    		 	 	 {
      	    		 	 	  	layer2MenuLight[layer1MenuLight]=0;
      	    		 	 	 }
      	    		 	 	 else
      	    		 	 	 {
      	    		 	 	  	layer2MenuLight[layer1MenuLight]++;
      	    		 	 	 }

      	    		 	 	 paraQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);      	    		 	 	 
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 2:  //重点用户抄表查询
      	    		 	 	 //如果是3-1(选择查询日期)的确认则要判断日期是否正确
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]==0)
      	    		 	 	 {
      	    		 	 	 	  //判断输入的日期是否正确
      	    		 	 	 	  if (checkInputTime()==FALSE)
      	    		 	 	 	  {
      	    		 	 	 	  	 return;
      	    		 	 	 	  }
      	    		 	 	 	  
      	    		 	 	 	  if (keyHouseHold.numOfHousehold==0)
      	    		 	 	 	  {
      	    		 	 	 	  	guiDisplay(20, 110, "未配置重点用户!", 1);
      	    		 	 	 	  	lcdRefresh(17, 130);
      	    		 	 	 	  	return;
      	    		 	 	 	  }
      	    		 	 	 }
      	    		 	 	 
      	    		 	 	 //按重庆显示规范要求,按确定键可实现不同分类的屏显间的切换
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]>=layer2MenuNum[layer1MenuLight]-1)
      	    		 	 	 {
      	    		 	 	  	layer2MenuLight[layer1MenuLight]=0;
      	    		 	 	 }
      	    		 	 	 else
      	    		 	 	 {
      	    		 	 	  	layer2MenuLight[layer1MenuLight]++;
      	    		 	 	 }
                     
                     if (layer2MenuLight[layer1MenuLight]!=0)
                     {
                       keyLeftRight = 0;
                       layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = keyHouseHold.numOfHousehold;
                       layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                     }

      	    		 	 	 keyHouseholdMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 3:  //统计查询
      	    		 	 	 //如果是4-1(选择查询日期)的确认则要判断日期是否正确
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]==0)
      	    		 	 	 {
      	    		 	 	 	  //判断输入的日期是否正确
      	    		 	 	 	  if (checkInputTime()==FALSE)
      	    		 	 	 	  {
      	    		 	 	 	  	 return;
      	    		 	 	 	  }
      	    		 	 	 }
      	    		 	 	 
      	    		 	 	 //按重庆显示规范要求,按确定键可实现不同分类的屏显间的切换
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]>=layer2MenuNum[layer1MenuLight]-1)
      	    		 	 	 {
      	    		 	 	  	layer2MenuLight[layer1MenuLight]=0;
      	    		 	 	 }
      	    		 	 	 else
      	    		 	 	 {
      	    		 	 	  	layer2MenuLight[layer1MenuLight]++;
      	    		 	 	 }

      	    		 	 	 statisQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 4:  //参数设置
      	    		 	 	 switch(layer2MenuLight[layer1MenuLight])
      	    		 	 	 {
      	    		 	 	 	  case 0:  //通信参数设置
                          strcpy(commParaItem[0],digitalToChar(addrField.a1[1]>>4));
                          strcat(commParaItem[0],digitalToChar(addrField.a1[1]&0xf));
                          strcat(commParaItem[0],digitalToChar(addrField.a1[0]>>4));
                          strcat(commParaItem[0],digitalToChar(addrField.a1[0]&0xf));
                       
                          for(i=0;i<4;i++)
                          {
                          	 if (strlen((char *)teApn[i])==0)
                          	 {
                          	   //strcpy((char *)teApn[i],(char *)ipAndPort.apn);
                          	 }
                          }
                          strcpy(commParaItem[1],(char *)ipAndPort.apn);
                          
                          intToString(ipAndPort.port[1]<<8 | ipAndPort.port[0],3,strX);
                       	  strcpy(commParaItem[2],intToIpadd(ipAndPort.ipAddr[0]<<24 | ipAndPort.ipAddr[1]<<16 | ipAndPort.ipAddr[2]<<8 | ipAndPort.ipAddr[3],str));
                          strcat(commParaItem[2],":");
                          
                          for(i=0;i<5-strlen(strX);i++)
                          {
                          	  strcat(commParaItem[2],"0");
                          }
                          strcat(commParaItem[2],strX);
                          intToString(ipAndPort.portBak[1]<<8 | ipAndPort.portBak[0],3,strX);
                       	  strcpy(commParaItem[3],intToIpadd(ipAndPort.ipAddrBak[0]<<24 | ipAndPort.ipAddrBak[1]<<16 | ipAndPort.ipAddrBak[2]<<8 | ipAndPort.ipAddrBak[3],str));
                          strcat(commParaItem[3],":");
                          
                          for(i=0;i<5-strlen(strX);i++)
                          {
                          	strcat(commParaItem[3],"0");
                          }
                          strcat(commParaItem[3],strX);
                          
                          keyLeftRight = 0;
                          layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  	commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	 	  	break;
      	    		 	 	 	  	
      	    		 	 	 	  case 1:  //修改密码
      	    		 	 	 	  	keyLeftRight = 0;
                          layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                          strcpy(commParaItem[0],"000000");
                          strcpy(commParaItem[1],"000000");
      	    		 	 	 	  	modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	 	  	break;
      	    		 	 	 	  	
             	 	 	  		case 2:  //VPN用户名密码
                          strcpy(tmpVpnUserName, (char *)vpn.vpnName);                        
                          tmpVpnUserName[32] = '\0';
                          trim(tmpVpnUserName);                                                
                          
       	 	 	  		 	 	    for(i=strlen(tmpVpnUserName); i<32; i++)
       	 	 	  		 	 	    {
       	 	 	  		 	 	    	tmpVpnUserName[i] = ' ';
       	 	 	  		 	 	    }
       	 	 	  		 	 	    tmpVpnUserName[32] = '\0';
  
                          strcpy(tmpVpnPw, (char *)vpn.vpnPassword);
                          tmpVpnPw[32] = '\0';
                          trim(tmpVpnPw);
       	 	 	  		 	 	    for(i=strlen(tmpVpnPw); i<32; i++)
       	 	 	  		 	 	    {
       	 	 	  		 	 	    	tmpVpnPw[i] = ' ';
       	 	 	  		 	 	    }
       	 	 	  		 	 	    tmpVpnPw[32] = '\0';
       	 	 	  		 	 	    keyLeftRight = 0;
                          inputStatus = STATUS_NONE;
                          setVpn(keyLeftRight);
                 	 	 	  	break;
      	    		 	 	 }
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 5:   //现场调试
      	    		 	 	 switch(layer2MenuLight[layer1MenuLight])   //2层菜单高亮
      	    		 	 	 {
      	    		 	 	 	  case 0:  //实时抄表
      	    		 	 	 	  	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  	realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 	  	break;
      	    		 	 	 	  	
      	    		 	 	 	  case 1: //全部测量点抄表结果
      	    		 	 	 	  	allMpCopy(0xff);
      	    		 	 	 	  	break;
      	    		 	 	 	  	
      	    		 	 	 	  case 2:  //表号搜索
      	    		 	 	 	  	if (carrierModuleType==SR_WIRELESS || carrierModuleType==HWWD_WIRELESS || carrierModuleType==RL_WIRELESS || carrierModuleType==NO_CARRIER_MODULE)
      	    		 	 	 	  	{
                             guiLine(10,55,150,105,0);
                             guiLine(10,55,10,105,1);
                             guiLine(150,55,150,105,1);
                             guiLine(10,55,150,55,1);
                             guiLine(10,105,150,105,1);
                             if (carrierModuleType==NO_CARRIER_MODULE)
                             {
                               guiDisplay(15,60,"未识本地通信模块!",1);
                               guiDisplay(15,80,"     无法搜表!   ",1);
                             }
                             else
                             {
                               guiDisplay(35,60,"无线自组网!",1);
                               guiDisplay(35,80," 无需搜表! ",1);
                             }
                             lcdRefresh(10,120);
      	    		 	 	 	  	}
      	    		 	 	 	  	else
      	    		 	 	 	  	{
      	    		 	 	 	  	  menuInLayer = 3;
      	    		 	 	 	  	  if (carrierFlagSet.searchMeter==0)
      	    		 	 	 	  	  {
      	    		 	 	 	  	    keyLeftRight = 1;
      	    		 	 	 	  	    searchMeter(keyLeftRight);
      	    		 	 	 	  	  }
      	    		 	 	 	  	  else
      	    		 	 	 	  	  {
      	    		 	 	 	  	    keyLeftRight = 0xff;
      	    		 	 	 	  	    searchMeter(keyLeftRight);
      	    		 	 	 	  	  }
      	    		 	 	 	  	}
      	    		 	 	 	  	break;
      	    		 	 	 	  	
      	    		 	 	 	  case 3:  //新增电能表地址
      	    	 	 	  		 	keyLeftRight = 1;
      	    	 	 	  		 	multiCpUpDown = 0;
      	    	 	 	  		 	newAddMeter(keyLeftRight);
      	    	 	 	  		 	break;
      	    	 	 	  		 	
      	    	 	 	  		case 4:  //新增电能表抄表状态
      	    	 	 	  			menuInLayer++;
      	    	 	 	  			      	    	 	 	  			
      	    	 	 	  			multiCpUpDown = 0;
      	    	 	 	  			newMeterCpStatus(0);
      	    	 	 	  			break;
      	    	 	 	  		 	
      	    	 	 	  		case 5:  //未搜到电能表地址
      	    	 	 	  		 	keyLeftRight = 1;
      	    	 	 	  		 	multiCpUpDown = 0;
	                      
	                       #ifdef MENU_FOR_CQ_CANON 
                          noFoundMeterHead = NULL;
                          if (carrierFlagSet.searchMeter==0 && carrierFlagSet.ifSearched == 1)
                          {
                            tmpNode = initPortMeterLink(30);
                       	    
                       	    prevFound = noFoundMeterHead;
                       	   
                       	    //按配置测量点查询
                       	    while(tmpNode!=NULL)
                       	    {
                           	  tmpNoFoundx = foundMeterHead;
                           	  tmpData = 0;
                           	  while(tmpNoFoundx!=NULL)
                           	  {     	 
                           	    if (tmpNoFoundx->addr[0]==tmpNode->addr[0] && tmpNoFoundx->addr[1]==tmpNode->addr[1]
                           	       && tmpNoFoundx->addr[2]==tmpNode->addr[2] && tmpNoFoundx->addr[3]==tmpNode->addr[3]
                           	        && tmpNoFoundx->addr[4]==tmpNode->addr[4] && tmpNoFoundx->addr[5]==tmpNode->addr[5]
                           	       )
                           	    {     	  	 
                                  tmpData = 1;
                                  break;
                                }
                               
                                tmpNoFoundx = tmpNoFoundx->next;
                              }
                    
                           	  if (tmpData==0)
                           	  {
                           	    tmpNoFoundx = existMeterHead;
                           	    while(tmpNoFoundx!=NULL)
                           	    {
                           	      if (tmpNoFoundx->addr[0]==tmpNode->addr[0] && tmpNoFoundx->addr[1]==tmpNode->addr[1]
                           	         && tmpNoFoundx->addr[2]==tmpNode->addr[2] && tmpNoFoundx->addr[3]==tmpNode->addr[3]
                           	           && tmpNoFoundx->addr[4]==tmpNode->addr[4] && tmpNoFoundx->addr[5]==tmpNode->addr[5]
                           	         )
                           	      {     	  	 
                                    tmpData = 1;
                                    break;
                                  }
                               
                                  tmpNoFoundx = tmpNoFoundx->next;
                                }
                              }
                             
                              if (tmpData==0)
                              {
                                tmpFound = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
                                memcpy(tmpFound->addr,tmpNode->addr,6);  //从节点地址
                                tmpFound->copyTime[0]   = 0xee;
                                tmpFound->copyTime[1]   = 0xee;
                                tmpFound->copyEnergy[0] = 0xee;
                                tmpFound->next = NULL;
                             
                                if (noFoundMeterHead==NULL)
                                {
                         	        noFoundMeterHead = tmpFound;
                                }
                                else
                                {
                         	        prevFound->next = tmpFound;
                                }
                               
                                prevFound = tmpFound;
                              }
                             
                              tmpNode = tmpNode->next;
                            }
                          }
                         #endif
                          
      	    	 	 	  			noFoundMeter(0);
      	    	 	 	  			break;
      	    	 	 	  			
      	    	 	 	  		case 6:  //主动上报
      	    	 	 	  			activeReportMenu(1);
      	    	 	 	  			break;
      	    		 	 	 }
      	    		 	 	 break;
      	    		 }
      	    		 break;
      	    		 
      	    	 case 3:   //菜单第3层
      	    	 	 switch(layer1MenuLight)
      	    	 	 {
      	    	 	 	  case 4:  //参数设置
      	    	 	 	  	switch(layer2MenuLight[layer1MenuLight])
      	    	 	 	  	{
      	    	 	 	  		 case 0:  //通信参数设置
      	    		 	 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==4)
      	    		 	 	       {
      	    		 	 	          //再次确认
      	    		 	 	          keyLeftRight = 0xff;
      	    		 	 	          commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	          layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0xff;
      	    		 	 	          
      	    		 	 	          return;
      	    		 	 	       }
      	    		 	 	       
      	    		 	 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] == 0xff)
      	    		 	 	       {
        	  	 	 	      	  	//主IP地址
        	  	 	 	      	  	ipAndPort.ipAddr[0] =(commParaItem[2][0]-0x30)*100+(commParaItem[2][1]-0x30)*10+(commParaItem[2][2]-0x30);
        	  	 	 	      	  	ipAndPort.ipAddr[1] =(commParaItem[2][4]-0x30)*100+(commParaItem[2][5]-0x30)*10+(commParaItem[2][6]-0x30);
        	  	 	 	      	  	ipAndPort.ipAddr[2] =(commParaItem[2][8]-0x30)*100+(commParaItem[2][9]-0x30)*10+(commParaItem[2][10]-0x30);
        	  	 	 	      	  	ipAndPort.ipAddr[3] =(commParaItem[2][12]-0x30)*100+(commParaItem[2][13]-0x30)*10+(commParaItem[2][14]-0x30);
        	  	 	 	      	  	
        	  	 	 	      	  	//主端口
        	  	 	 	      	  	tmpData = (commParaItem[2][16]-0x30)*10000+(commParaItem[2][17]-0x30)*1000
        	  	 	 	      	  	         +(commParaItem[2][18]-0x30)*100+(commParaItem[2][19]-0x30)*10
        	  	 	 	      	  	         +(commParaItem[2][20]-0x30);
        	  	 	 	      	  	ipAndPort.port[1] = tmpData>>8;
        	  	 	 	      	  	ipAndPort.port[0] = tmpData&0xff;

        	  	 	 	      	  	//备用地址
        	  	 	 	      	  	ipAndPort.ipAddrBak[0] =(commParaItem[3][0]-0x30)*100+(commParaItem[3][1]-0x30)*10+(commParaItem[3][2]-0x30);
        	  	 	 	      	  	ipAndPort.ipAddrBak[1] =(commParaItem[3][4]-0x30)*100+(commParaItem[3][5]-0x30)*10+(commParaItem[3][6]-0x30);
        	  	 	 	      	  	ipAndPort.ipAddrBak[2] =(commParaItem[3][8]-0x30)*100+(commParaItem[3][9]-0x30)*10+(commParaItem[3][10]-0x30);
        	  	 	 	      	  	ipAndPort.ipAddrBak[3] =(commParaItem[3][12]-0x30)*100+(commParaItem[3][13]-0x30)*10+(commParaItem[3][14]-0x30);
        	  	 	 	      	  	
        	  	 	 	      	  	//备用端口
        	  	 	 	      	  	tmpData = (commParaItem[3][16]-0x30)*10000+(commParaItem[3][17]-0x30)*1000
        	  	 	 	      	  	         +(commParaItem[3][18]-0x30)*100+(commParaItem[3][19]-0x30)*10
        	  	 	 	      	  	         +(commParaItem[3][20]-0x30);
        	  	 	 	      	  	ipAndPort.portBak[1] = tmpData>>8;
        	  	 	 	      	  	ipAndPort.portBak[0] = tmpData&0xff;
        	  	 	 	      	  	        	  	 	 	      	  	
        	  	 	 	      	  	printf("输入后的apn长度=%d\n",strlen(commParaItem[1]));
        	  	 	 	      	  	
        	  	 	 	      	  	strcpy((char *)ipAndPort.apn, commParaItem[1]);

        	  	 	 	      	  	//保存IP地址
        	  	 	 	      	  	saveParameter(0x04, 3,(INT8U *)&ipAndPort,sizeof(IP_AND_PORT));
                        	    
                        	    saveBakKeyPara(3);    //2012-8-9,add

                 	  	 	 	    addrField.a1[1] = (commParaItem[0][0]-0x30)<<4 | (commParaItem[0][1]-0x30);
                 	  	 	 	    addrField.a1[0] = (commParaItem[0][2]-0x30)<<4 | (commParaItem[0][3]-0x30);
                              
                              //保存行政区划码
                              saveParameter(0x04, 121,(INT8U *)&addrField,4);
                        	    
                        	    saveBakKeyPara(121);    //2012-8-9,add
                              
                              guiLine(10,55,150,105,0);
                              guiLine(10,55,10,105,1);
                              guiLine(150,55,150,105,1);
                              guiLine(10,55,150,55,1);
                              guiLine(10,105,150,105,1);
                              guiDisplay(12,70,"修改通信参数成功!",1);
                              lcdRefresh(10,120);
                              
                              menuInLayer--;
                              layer2MenuLight[layer1MenuLight]=0xff;

                              return;
      	    		 	 	       }
      	    		 	 	       
      	    		 	 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
      	    		 	 	       {
      	    		 	 	       	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	       }
      	    		 	 	       else
      	    		 	 	       {
      	    		 	 	       	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	       }
      	    		 	 	       
      	    		 	 	       keyLeftRight = 0;
      	    		 	 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
      	    		 	 	       {
      	    		 	 	       	  keyLeftRight = 0x88;
      	    		 	 	       	  for(i=0;i<4;i++)
      	    		 	 	       	  {
      	    		 	 	       	  	 if (strcmp((char *)teApn[i],commParaItem[1])==0)
      	    		 	 	       	  	 {
      	    		 	 	       	  	 	  keyLeftRight = i;
      	    		 	 	       	  	 	  break;
      	    		 	 	       	  	 }
      	    		 	 	       	  }
      	    		 	 	       }
      	    		 	 	       commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		 	 break;
      	    	 	 	  		 	 
      	    	 	 	  		 case 1: //修改密码
      	    		 	 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
      	    		 	 	       {
      	    		 	 	       	  for(i=0;i<6;i++)
      	    		 	 	       	  {
      	    		 	 	       	  	 if (commParaItem[0][i]!=commParaItem[1][i])
      	    		 	 	       	  	 {
      	    		 	 	       	  	 	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
      	    		 	 	       	  	 	  keyLeftRight = 0;
      	    		 	 	       	  	 	  guiDisplay(20,113,"输入密码不一致!",1);
      	    		 	 	       	  	 	  lcdRefresh(113,128);
      	    		 	 	       	  	 	  return;
      	    		 	 	       	  	 }
      	    		 	 	       	  }
      	    		 	 	           
      	    		 	 	          //确认新密码
      	    		 	 	          keyLeftRight = 0xff;
      	    		 	 	          modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	          layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0xff;
      	    		 	 	       	  return;
      	    		 	 	       }
      	    		 	 	       
      	    		 	 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0xff)
      	    		 	 	       {
      	    		 	 	       	  for(i=0;i<6;i++)
      	    		 	 	       	  {
      	    		 	 	       	  	 originPassword[i] = commParaItem[0][i];
      	    		 	 	       	  }
      	    		 	 	       	  originPassword[6] = '\0';
                              
                              saveParameter(88, 8, originPassword, 7);     //保存界面密码
                              
                              guiLine(10,55,150,105,0);
                              guiLine(10,55,10,105,1);
                              guiLine(150,55,150,105,1);
                              guiLine(10,55,150,55,1);
                              guiLine(10,105,150,105,1);
                              guiDisplay(44,70,"修改密码成功!",1);
                              lcdRefresh(10,120);
                              
                              menuInLayer--;
                              layer2MenuLight[layer1MenuLight]=0xff;
      	    		 	 	       	  return;
      	    		 	 	       }
      	    		 	 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
      	    		 	 	       {
      	    		 	 	       	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	       }
      	    		 	 	       else
      	    		 	 	       {
      	    		 	 	       	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	       }
      	    		 	 	       keyLeftRight = 0;
      	    		 	 	       modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		 	 break;
      	    	 	 	  		 	 
         	    	 	 	     case 2:   //VPN用户名密码
                    	 	 	 if (inputStatus == STATUS_SELECT_CHAR)
                    	 	 	 {
                    	 	 	  	inputStatus = STATUS_NONE;
                    	 	 	  	if (keyLeftRight<32)
                    	 	 	  	{
                    	 	 	  	  tmpVpnUserName[keyLeftRight] = character[selectIndex];
                    	 	 	  	}
                    	 	 	  	else
                    	 	 	  	{
                    	 	 	  	  tmpVpnPw[keyLeftRight-32] = character[selectIndex];
                    	 	 	  	}
                    	 	 	  	setVpn(keyLeftRight);
                    	 	 	 }
                    	 	 	 else
                    	 	 	 {
                             tmpVpnUserName[32] = '\0';
                             trim(tmpVpnUserName);
                             tmpVpnPw[32] = '\0';
                             trim(tmpVpnPw);
                             //strcpy((char *)vpn.vpnName, tmpVpnUserName);
                             memcpy(vpn.vpnName, tmpVpnUserName, 16);
                             //strcpy((char *)vpn.vpnPassword, tmpVpnPw);
                             memcpy(vpn.vpnPassword, tmpVpnPw, 16);
   
           	  	 	 	      	 //保存vpn用户名密码
           	  	 	 	      	 saveParameter(0x04, 16,(INT8U *)&vpn, sizeof(VPN));
   
                             guiLine(10,55,150,105,0);
                             guiLine(10,55,10,105,1);
                             guiLine(150,55,150,105,1);
                             guiLine(10,55,150,55,1);
                             guiLine(10,105,150,105,1);
                             guiDisplay(20, 70, "专网参数已修改!",1);
                             lcdRefresh(10,120);
                                 
                             menuInLayer--;
                    	 	 	 }
                    	 	 	 break;
      	    	 	 	  	}
      	    	 	 	  	break;
      	    	 	 	  	
      	    	 	 	  case 5:  //现场调试
      	    	 	 	  	switch(layer2MenuLight[layer1MenuLight])
      	    	 	 	  	{
      	    	 	 	  		 case 0:  //实时抄表
      	    	 	 	  		 	 if ((carrierFlagSet.searchMeter!=0 && carrierModuleType==CEPRI_CARRIER) 
      	    	 	 	  		 	 	  || (carrierFlagSet.setupNetwork!=3 && carrierModuleType==RL_WIRELESS))
      	    	 	 	  		 	 {
      	    	 	 	  		 	 	  guiLine(30, 30, 120, 120, 0);
      	    	 	 	  		 	 	  guiLine( 30, 30,  30, 120, 1);
      	    	 	 	  		 	 	  guiLine(120, 30, 120, 120, 1);
      	    	 	 	  		 	 	  guiLine( 30, 30, 120,  30, 1);
      	    	 	 	  		 	 	  guiLine( 30,120, 120, 120, 1);      	    	 	 	  		 	 	  
      	    	 	 	  		 	 	  guiDisplay(48, 55,"正在组网",1);
      	    	 	 	  		 	 	  guiDisplay(44, 75,"请稍后...",1);
      	    	 	 	  		 	 	  lcdRefresh(30, 120);
      	    	 	 	  		 	 }
      	    	 	 	  		 	 else
      	    	 	 	  		 	 {
        	    	 	 	  		 	 switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
        	    	 	 	  		 	 {
        	    	 	 	  		 	 	  case 0: //指定测量点抄表
        	    	 	 	  		 	 	  	keyLeftRight = 0;
        	    	 	 	  		 	 	  	strcpy(singleCopyTime,"-----");
               	 	                strcpy(singleCopyEnergy,"-----");
        	    	 	 	  		 	 	  	singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
        	    	 	 	  		 	 	  	break;
        	    	 	 	  		 	 	  	
        	    	 	 	  		 	 	  case 1: //全部测量点抄表
        	    	 	 	  		 	 	  	keyLeftRight = 1;
        	    	 	 	  		 	 	  	allMpCopy(keyLeftRight);
        	    	 	 	  		 	 	  	break;
        	    	 	 	  		 	 }
        	    	 	 	  		 }
      	    	 	 	  		 	 break;
      	    	 	 	  		 	 
      	    	 	 	  		 case 2:  //表号搜索
      	    	 	 	  		 	 switch(keyLeftRight)
      	    	 	 	  		 	 {
      	    	 	 	  		 	 	 case 0:  //启动搜表
      	    	 	 	  		 	 	   if (carrierFlagSet.searchMeter==0)
      	    	 	 	  		 	 	   {
      	    	 	 	  		 	 	     //清除以前搜索到与配置测量点相同的表地址
      	    	 	 	  		 	 	     while(existMeterHead!=NULL)
      	    	 	 	  		 	 	     {
      	    	 	 	  		 	 	   	    tmpFound = existMeterHead;
      	    	 	 	  		 	 	   	    existMeterHead = existMeterHead->next;
      	    	 	 	  		 	 	   	    free(tmpFound);
      	    	 	 	  		 	 	     }
      	    	 	 	  		 	 	     existMeterHead = NULL;
      	    	 	 	  		 	 	     prevExistFound = NULL;

      	    	 	 	  		 	 	     //清除以前搜索到的表
      	    	 	 	  		 	 	     while(foundMeterHead!=NULL)
      	    	 	 	  		 	 	     {
      	    	 	 	  		 	 	   	    tmpFound = foundMeterHead;
      	    	 	 	  		 	 	   	    foundMeterHead = foundMeterHead->next;
      	    	 	 	  		 	 	   	    free(tmpFound);
      	    	 	 	  		 	 	     }
      	    	 	 	  		 	 	     foundMeterHead = NULL;
      	    	 	 	  		 	 	     prevFound = foundMeterHead;
      	    	 	 	  		 	 	     
      	    	 	 	  		 	 	     carrierFlagSet.foundStudyTime = nextTime(sysTime, assignCopyTime[0]|assignCopyTime[1]<<8, 0); //搜表时间
      	    	 	 	  		 	 	   
      	    	 	 	  		 	 	     carrierFlagSet.searchMeter = 1;         //开始搜表标志置1
      	    	 	 	  		 	 	   }
      	    	 	 	  		 	 	 	 keyLeftRight = 0xff;
      	    	 	 	  		 	 	   searchMeter(keyLeftRight);

      	    	 	 	  		 	 	 	 break;
      	    	 	 	  		 	 	 	  
      	    	 	 	  		 	 	 case 1:
      	    	 	 	  		 	 	 	 keyLeftRight = 0;
      	    	 	 	  		 	 	 	 searchMeter(keyLeftRight);
      	    	 	 	  		 	 	 	 break;
      	    	 	 	  		 	 	 	 
      	    	 	 	  		 	 	 case 0xff:
      	    	 	 	  		 	 	 	 layer2MenuLight[layer1MenuLight]++;
      	    	 	 	  		 	 	 	 keyLeftRight = 1;
      	    	 	 	  		 	 	 	 newAddMeter(keyLeftRight);
      	    	 	 	  		 	 	 	 break;
      	    	 	 	  		 	 }
      	    	 	 	  		 	 break;
      	    	 	 	  		 	 
      	    	 	 	  		 case 3:  //新增电能表地址
      	    	 	 	  		 	 switch(keyLeftRight)
      	    	 	 	  		 	 {
      	    	 	 	  		 	 	  case 0: //启动抄表(抄新增电能表的示值)
             	 	 	  			      keyLeftRight = 1;
             	 	 	  			      
             	 	 	  			      tmpFound = foundMeterHead;
             	 	 	  			      while(tmpFound!=NULL)
             	 	 	  			      {
             	 	 	  			      	 tmpFound->copyTime[0]   = 0xee;
             	 	 	  			      	 tmpFound->copyTime[1]   = 0xee;
             	 	 	  			      	 tmpFound->copyEnergy[0] = 0xee;
             	 	 	  			      	 
             	 	 	  			      	 tmpFound = tmpFound->next;
             	 	 	  			      }
             	 	 	  			      layer2MenuLight[layer1MenuLight]++;
             	 	 	  			      multiCpUpDown = 0;
             	 	 	  			      newMeterCpStatus(0);
             	 	 	  			      
             	 	 	  			      if (foundMeterHead!=NULL)
             	 	 	  			      {
                            	     pDotCopy = (DOT_COPY *)malloc(sizeof(DOT_COPY));
                            	     prevFound = foundMeterHead;
                            	     pDotCopy->dotCopyMp = prevFound->mp;                            	     
             	 	 	  			       	 memcpy(pDotCopy->addr, prevFound->addr, 6);
                            	     pDotCopy->dotCopying = FALSE;
                            	     pDotCopy->port       = PORT_POWER_CARRIER;
                            	     pDotCopy->dataFrom   = DOT_CP_NEW_METER;
                            	     pDotCopy->outTime    = nextTime(sysTime,0,22);
                            	     pDotCopy->dotResult  = RESULT_NONE;
             	 	 	  			      }
      	    	 	 	  		 	 	  	break;
      	    	 	 	  		 	 	  	
      	    	 	 	  		 	 	  case 1:
      	    	 	 	  		 	 	  	//noFoundMeter(0);
      	    	 	 	  		 	 	  	break;
      	    	 	 	  		 	 	  	
      	    	 	 	  		 	 	  case 0xff:   //正在抄表转到新增电能表抄表状态页
      	    	 	 	  		 	 	  	break;
      	    	 	 	  		 	 }
      	    	 	 	  		 	 break;
      	    	 	 	  		 	 
      	    	 	 	  		 case 6:
                           guiLine(30, 118, 160, 140, 0);
                           guiDisplay(40,120,"上报中...",1);
                           lcdRefresh(17,160);
   
                           AFN02008();

                           sleep(2);
                           guiLine(30, 118, 160, 140, 0);
                           guiDisplay(40,120,"上报结束",1);
                           lcdRefresh(17,160);
      	    	 	 	  		 	 break;
      	    	 	 	  	}
      	    	 	 	  	break;
      	    	 	 }
      	    	 	 break;
      	    	 	 
            	 case 4:    //菜单第4层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 4: //参数设置
             	 	 	  	if (inputStatus == STATUS_NONE)
             	 	 	  	{
                         keyLeftRight = 0;
                         layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	 	   commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  	}
             	 	 	  	if (inputStatus == STATUS_SELECT_CHAR)
             	 	 	  	{
             	 	 	  		 inputStatus = STATUS_NONE;
             	 	 	  		 commParaItem[1][inputIndex] = character[selectIndex];
             	 	 	  		 inputApn(inputIndex);
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 5: //现场调试
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //实时抄表
             	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
             	 	 	  			{
             	 	 	  				 case 0:
      		                     if (pDotCopy==NULL)
      		                     {
                            	   tmpData = (singleCopyMp[0]-0x30)*1000+(singleCopyMp[1]-0x30)*100
                            	           + (singleCopyMp[2]-0x30)*10 +(singleCopyMp[3]-0x30);
                            	   if (tmpData>2040)
                            	   {
             	 	 	  			         guiDisplay(20,120,"测量点输入错误!",1);
             	 	                   lcdRefresh(120,140);
                            	   }
                            	   else
                            	   {             	 	 	  			       
             	 	                   strcpy(singleCopyTime,"-----");
             	 	                   strcpy(singleCopyEnergy,"-----");
             	 	                   singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  			         guiDisplay(44,120,"抄表中...",1);
             	 	                   lcdRefresh(120,140);
             	 	                 
                            	     pDotCopy = (DOT_COPY *)malloc(sizeof(DOT_COPY));
                            	     pDotCopy->dotCopyMp    = tmpData;
                            	     pDotCopy->dotCopying   = FALSE;
                            	     pDotCopy->port         = PORT_POWER_CARRIER;
                            	     pDotCopy->dataFrom     = DOT_CP_SINGLE_MP;
                            	     pDotCopy->outTime      = nextTime(sysTime,0,22);
                            	     pDotCopy->dotResult    = RESULT_NONE;
                            	     pDotCopy->dotTotalItem = 2;
                            	     pDotCopy->dotRecvItem  = 0;
                            	   }
                            	 }
             	 	               break;
             	 	               
             	 	             case 1:
             	 	             	 if (keyLeftRight==0)
             	 	             	 {
                                 while(mpCopyHead!=NULL)
                                 {
                                 	  prevMpCopy = mpCopyHead;
                                 	  mpCopyHead = mpCopyHead->next;
                                 	  free(mpCopyHead);
                                 }
                                 
                                 queryMpLink = initPortMeterLink(30);
             	 	 	  			       tmpMpCopy = mpCopyHead;
             	 	 	  			       multiCpMax = 0;
             	 	 	  			       while (queryMpLink!=NULL)
             	 	 	  			       {
             	 	 	  			       	  tmpMpCopy = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
             	 	 	  			       	  tmpMpCopy->mp = queryMpLink->mp;
             	 	 	  			       	  tmpMpCopy->copyTime[0]   = 0xee;
             	 	 	  			       	  tmpMpCopy->copyTime[1]   = 0xee;
             	 	 	  			       	  tmpMpCopy->copyEnergy[0] = 0xee;
             	 	 	  			       	  tmpMpCopy->next = NULL;
             	 	 	  			       	  
             	 	 	  			       	  if (mpCopyHead==NULL)
                                    {
                                    	mpCopyHead = tmpMpCopy;
                                    }
                                    else
                                    {
                                    	prevMpCopy->next = tmpMpCopy;
                                    }
                                    prevMpCopy = tmpMpCopy;
                                    
             	 	 	  			          tmpPrevMpLink = queryMpLink;
             	 	 	  			          queryMpLink = queryMpLink->next;
                                    
             	 	 	  			          multiCpMax++;
                                    free(tmpPrevMpLink);
             	 	 	  			       }
             	 	 	  			       
             	 	 	  			       if ((multiCpMax%(NUM_MP_PER_PAGE-1))!=0)
             	 	 	  			       {
             	 	 	  			       	  multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-1)+1;
             	 	 	  			       }
             	 	 	  			       else
             	 	 	  			       {
             	 	 	  			       	  multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-1);
             	 	 	  			       }
             	 	 	  			       
             	 	 	  			       if (mpCopyHead!=NULL)
             	 	 	  			       {
                            	      multiCpUpDown = 0;
                            	      
                            	      pDotCopy = (DOT_COPY *)malloc(sizeof(DOT_COPY));
                            	      prevMpCopy = mpCopyHead;
                            	      pDotCopy->dotCopyMp  = prevMpCopy->mp;
                            	      pDotCopy->dotCopying = FALSE;
                            	      pDotCopy->port       = PORT_POWER_CARRIER;
                            	      pDotCopy->dataFrom   = DOT_CP_ALL_MP;
                            	      pDotCopy->outTime    = nextTime(sysTime,0,22);
                            	      pDotCopy->dotResult  = RESULT_NONE;
                            	      pDotCopy->dotTotalItem = 2;
                            	      pDotCopy->dotRecvItem  = 0;
             	 	 	  			       }
             	 	 	  			       
             	 	 	  			       keyLeftRight = 0xff;
             	 	 	  			       allMpCopy(keyLeftRight);
             	 	             	 }
             	 	             	 else
             	 	             	 {
             	 	             	 	  if (keyLeftRight!=0xff)
             	 	             	 	  {
             	 	             	 	    realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	             	 	  }
             	 	             	 }
             	 	             	 break;
             	 	          }
             	 	 	  		 	break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;
      	    		 
      	    	 case 20:  //菜单第20层(输入密码)的确认
      	    		 for(i=0;i<6;i++)
      	    		 {
      	    		 	  if (originPassword[i]!=passWord[i])
      	    		 	  {
      	    		 	  	 guiDisplay(30,120,"密码输入错误!",1);
      	    		 	  	 lcdRefresh(120,137);
      	    		 	  	 return;
      	    		 	  }
      	    		 }
      	    		 
      	    		 //根据输入密码前的高亮菜单项,执行相应的动作
      	    		 switch(layer1MenuLight)
      	    		 {
      	    		 	  case 4:   //参数设置
      	    		 	 	  layer2MenuLight[layer1MenuLight] = 0;
      	    		 	 	  if (moduleType==CDMA_CM180 || moduleType==CDMA_DTGS800)
      	    		 	 	  {
      	    		 	 	  	layer2MenuNum[layer1MenuLight] = 3;
      	    		 	 	  }
      	    		 	 	  paraSetMenu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	    		 	    break;
      	    		 	  
      	    		 	  case 5:  //现场调试
      	    		 	 	  layer2MenuLight[layer1MenuLight] = 0;
      	    		 	 	  debugMenu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	    		 	  	break;
      	    		 }
      	    		 break;
      	     }
      	     break;

      	   case KEY_CANCEL: //取消
      	     switch(menuInLayer)
      	     {
      	    	 case 1:     //菜单第1层
      	    		 defaultMenu();
      	    		 break;
      	    		 	 
      	    	 case 2:     //菜单第2层
      	    		 freeQueryMpLink();
      	    		 layer1Menu(layer1MenuLight);
      	    		 break;
      	    		 	 
      	    	 case 3:     //菜单第3层
      	    		 switch(layer1MenuLight)
      	    		 {
      	    		 	 case 4://参数设置
      	    		 	 	 paraSetMenu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	    		 	 	 break;
      	    		 	 	  	
      	    		 	 case 5://现场调试
      	    		 	 	 debugMenu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	    		 	   break;
      	    		 }
      	    		 break;

            	 case 4:    //菜单第4层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 4: //通信参数设置
      	    		 	 	 	if (inputStatus==STATUS_SELECT_CHAR)
      	    		 	 	 	{
      	    		 	 	 		inputStatus = STATUS_NONE;
      	    		 	 	 		inputApn(inputIndex);
      	    		 	 	 	}
      	    		 	 	 	else
      	    		 	 	 	{
      	    		 	 	 	  commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	 	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 5: //现场调试
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //实时抄表
             	 	 	  			realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	break;
             	 	 	  		 	
             	 	 	  		case 1: //全体测量点抄表结果
      	    		 	 	      debugMenu(layer2MenuLight[layer1MenuLight],layer1MenuLight);             	 	 	  			
             	 	 	  			break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;      	    		 	 
      	    		 	 
      	    	 case 20:    //菜单第20层(输入密码)
      	    		 menuInLayer = 1;
     	    		 	 layer1Menu(layer1MenuLight);
      	    		 break;
      	     }
      	     break;
      	    	
           case KEY_DOWN:    //向下
           	 switch(menuInLayer)
             {
             	 case 1:    //菜单第1层
             	 	 if (layer1MenuLight>=MAX_LAYER1_MENU-1)
             	 	 {
             	 	 	  layer1MenuLight = 0;
             	 	 }
             	 	 else
             	 	 {
             	 	 	  layer1MenuLight++;
             	 	 }
             	 	 layer1Menu(layer1MenuLight);
             	   break;

             	 case 2:    //菜单第2层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 0:    //抄表查询
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //1-1测量点有功抄表查询
             	 	 	  		 	stringUpDown(queryTimeStr, layer3MenuLight[0][0], 1);  //高亮的字符减小一个数字
             	 	 	  		 	copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		  break;
             	 	 	  		  
             	 	 	  		default:
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>=layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
                            layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
             	 	 	  		 	}
             	 	 	  		 	copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	break;
             	 	 	  	}
             	 	 	  	break;
             	 	 	  
             	 	 	  case 1:  //参数查询
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //2-1测量点参数查询
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>=layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
                            layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
             	 	 	  		 	}
             	 	 	  		 	paraQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	break;             	 	 	  		 	
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2:    //重点用户抄表查询
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //3-1重点用户抄表查询
             	 	 	  		 	stringUpDown(queryTimeStr, layer3MenuLight[2][0],1);  //高亮的字符减小一个数字
             	 	 	  		 	keyHouseholdMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		  break;

             	 	 	  		default:
             	 	 	  		 	keyLeftRight = 0;
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>=layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
                            layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
             	 	 	  		 	}
             	 	 	  		 	keyHouseholdMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  			break;

             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 3:    //统计信息查询
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //4-1运行统计信息查询
             	 	 	  		 	stringUpDown(queryTimeStr, layer3MenuLight[3][0],1);  //高亮的字符减小一个数字
             	 	 	  		 	statisQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		  break;
             	 	 	    }
             	 	 	    break;

      	    		 	  case 4://参数设置
      	    		 	 	  if (layer2MenuLight[layer1MenuLight]>=layer2MenuNum[layer1MenuLight]-1)
      	    		 	 	  {
      	    		 	 	 	  layer2MenuLight[layer1MenuLight] = 0;
      	    		 	 	  }
      	    		 	 	  else
      	    		 	 	  {
      	    		 	 	 	  layer2MenuLight[layer1MenuLight]++;
      	    		 	 	  }
      	    		 	 	  paraSetMenu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	    		 	 	  break;
      	    		 	 	  
      	    		 	  case 5://现场调试
      	    		 	 	  if (layer2MenuLight[layer1MenuLight]>=layer2MenuNum[layer1MenuLight]-1)
      	    		 	 	  {
      	    		 	 	 	  layer2MenuLight[layer1MenuLight] = 0;
      	    		 	 	  }
      	    		 	 	  else
      	    		 	 	  {
      	    		 	 	 	  layer2MenuLight[layer1MenuLight]++;
      	    		 	 	  }
      	    		 	 	  debugMenu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	    		 	 	  break;
             	 	 }
             	 	 break;
             	 	 
             	 case 3:    //菜单第3层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 4:  //参数设置
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		 case 0: //通信参数设置
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
             	 	 	  		 	   if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
             	 	 	  		 	   {
             	 	 	  		 	 	    //APN的输入处理
             	 	 	  		 	 	    for(i=strlen(commParaItem[1]);i<16;i++)
             	 	 	  		 	 	    {
             	 	 	  		 	 	    	 commParaItem[1][i] = ' ';
             	 	 	  		 	 	    }
             	 	 	  		 	 	    commParaItem[1][16] = '\0';
             	 	 	  		 	 	    inputStatus=STATUS_NONE;
             	 	 	  		 	 	    inputIndex = 0;
             	 	 	  		 	 	    inputApn(inputIndex);
             	 	 	  		 	   }
             	 	 	  		 	   else
             	 	 	  		 	   {
      	    		                stringUpDown(commParaItem[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
      	    		                commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		             }
      	    		           }
             	 	 	  		 	 break;
             	 	 	  		 
             	 	 	  		 case 1: //修改密码
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
      	    		             stringUpDown(commParaItem[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
      	    		             modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 }
             	 	 	  		 	 break;

             	 	 	  		 case 2:  //VPN用户名密码
                 	 	 	  	 if (inputStatus==STATUS_SELECT_CHAR)
                 	 	 	  	 {
                 	 	 	  		 selectIndex += 14;
                 	 	 	  		 if (selectIndex>69)
                 	 	 	  		 {
                 	 	 	  			  selectIndex = 0;
                 	 	 	  		 }
                 	 	 	  		 selectChar(selectIndex);
                 	 	 	  	 }
                 	 	 	  	 if (inputStatus==STATUS_NONE)
                 	 	 	  	 {
                 	 	 	  	   selectIndex = 0;
                             selectChar(selectIndex);
                           }
                 	 	 	  	 break;

             	 	 	  	}
             	 	 	  	break;
             	 	 	  
             	 	 	  case 5:  //现场调试
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		 case 0: //实时抄表
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>=layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
             	 	 	  		 	 {
             	 	 	  		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
             	 	 	  		 	 }
             	 	 	  		 	 else
             	 	 	  		 	 {
             	 	 	  		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
             	 	 	  		 	 }
             	 	 	  		 	 realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	 break;
             	 	 	  		 
             	 	 	  		 case 3:  //新增电能表地址
           	 	 	  				 if (multiCpUpDown>=(multiCpMax-1))
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown=0;
           	 	 	  				 }
           	 	 	  				 else
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown++;
           	 	 	  				 }
           	 	 	  				 
           	 	 	  				 newAddMeter(keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 4:   //新增电能表抄表状态
           	 	 	  				 if (multiCpUpDown>=(multiCpMax-1))
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown=0;
           	 	 	  				 }
           	 	 	  				 else
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown++;
           	 	 	  				 }
           	 	 	  				 
           	 	 	  				 newMeterCpStatus(keyLeftRight);
             	 	 	  		 	 break;

             	 	 	  		 case 5:  //未增到电能表地址
           	 	 	  				 if (multiCpUpDown>=(multiCpMax-1))
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown=0;
           	 	 	  				 }
           	 	 	  				 else
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown++;
           	 	 	  				 }           	 	 	  				 
           	 	 	  				 noFoundMeter(keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;
             	 	 
            	 case 4:    //菜单第4层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 4:
             	 	 	  	if (inputStatus==STATUS_SELECT_CHAR)
             	 	 	  	{
             	 	 	  		selectIndex += 14;
             	 	 	  		if (selectIndex>69)
             	 	 	  		{
             	 	 	  			 selectIndex = 0;
             	 	 	  		}
             	 	 	  		selectChar(selectIndex);
             	 	 	  	}
             	 	 	  	if (inputStatus==STATUS_NONE)
             	 	 	  	{
             	 	 	  	  selectIndex = 0;
                        selectChar(selectIndex);
                      }
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 5: //现场调试
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //实时抄表
             	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
             	 	 	  		 	{
             	 	 	  		 	 	case 0:  //指定测量点抄表
             	 	 	  		 	 		if (pDotCopy==NULL)
             	 	 	  		 	 		{
             	 	 	  		 	 		  stringUpDown(singleCopyMp, keyLeftRight, 1);
             	 	 	  		 	 	    singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	  }
             	 	 	  		 	 	  break;
             	 	 	  		 	 	  
             	 	 	  		 	 	case 1:
                 	 	 	  			if (keyLeftRight==0xff)
                 	 	 	  			{
                 	 	 	  				 if (multiCpUpDown>=(multiCpMax-1))
                 	 	 	  				 {
                 	 	 	  				 	  multiCpUpDown=0;
                 	 	 	  				 }
                 	 	 	  				 else
                 	 	 	  				 {
                 	 	 	  				 	  multiCpUpDown++;
                 	 	 	  				 }
                 	 	 	  				 
                 	 	 	  				 allMpCopy(keyLeftRight);
                 	 	 	  			}
                 	 	 	  			break;
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  		 	
             	 	 	  		case 1:
            	 	 	  			if (keyLeftRight==0xff)
            	 	 	  			{
            	 	 	  				 if (multiCpUpDown>=(multiCpMax-1))
            	 	 	  				 {
            	 	 	  				 	  multiCpUpDown=0;
            	 	 	  				 }
            	 	 	  				 else
            	 	 	  				 {
            	 	 	  				 	  multiCpUpDown++;
            	 	 	  				 }
            	 	 	  				 
            	 	 	  				 allMpCopy(keyLeftRight);
            	 	 	  			}
             	 	 	  			break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;
             	 	 
             	 case 20:   //菜单第20层
             	 	 stringUpDown(passWord, pwLight,1);
             	 	 inputPassWord(pwLight);
             	 	 break;
             }
           	 break;

           case KEY_RIGHT:   //向右
           	 switch(menuInLayer)
             {
             	 case 1:    //菜单第1层
             	 	 if (layer1MenuLight>=(MAX_LAYER1_MENU-1))
             	 	 {
             	 	 	  layer1MenuLight = 0;
             	 	 }
             	 	 else
             	 	 {
             	 	 	  layer1MenuLight++;
             	 	 }
             	 	 layer1Menu(layer1MenuLight);
             	   break;
             	   
             	 case 2:    //菜单第2层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 0:    //抄表查询
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //1-1测量点有功抄表查询
             	 	 	  		 	if (layer3MenuLight[0][0]>=(layer3MenuNum[0][0]-1))
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[0][0] = 0;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[0][0]++;             	 	 	  		 		
             	 	 	  		 	}
             	 	 	  		 	copyQueryMenu(layer2MenuLight[0],layer3MenuLight[0][0]);
             	 	 	  		  break;
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2:     //重点用户查询
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //3-1重点用户抄表查询
             	 	 	  		 	if (layer3MenuLight[2][0]>=(layer3MenuNum[2][0]-1))
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[2][0] = 0;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[2][0]++;             	 	 	  		 		
             	 	 	  		 	}
             	 	 	  		 	keyHouseholdMenu(layer2MenuLight[2],layer3MenuLight[2][0]);
             	 	 	  		  break;
             	 	 	  		  
             	 	 	  		default:
             	 	 	  			if (keyLeftRight>3)
             	 	 	  			{
             	 	 	  				 keyLeftRight = 0;
             	 	 	  			}
             	 	 	  			else
             	 	 	  			{
             	 	 	  				 keyLeftRight++;
             	 	 	  			}
             	 	 	  		 	keyHouseholdMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	break;             	 	 	  			
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 3:     //统计查询
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //4-1运行统计信息查询
             	 	 	  		 	if (layer3MenuLight[3][0]>=(layer3MenuNum[3][0]-1))
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[3][0] = 0;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[3][0]++;             	 	 	  		 		
             	 	 	  		 	}
             	 	 	  		 	statisQueryMenu(layer2MenuLight[3],layer3MenuLight[3][0]);
             	 	 	  		  break;
             	 	 	    }
             	 	 	    break;
             	 	 }
             	 	 break;
             	 	 
             	 case 3:    //菜单第3层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 4:       //参数设置
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		 case 0:  //通道参数设置
      	    		         	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
      	    		         	 {                           
                             adjustCommParaLight(1);
      	    		             commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
      	    		           break;
      	    		           
      	    		         case 1:
      	    		         	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
      	    		         	 {
                             if (keyLeftRight>=5)
                       	     {
                       	 	     keyLeftRight = 0;
                       	     }
                       	     else
                       	     {
                       	 	     keyLeftRight++;
                       	     }
      	    		             modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
      	    		           break;
      	    		           
      	    		         case 2:  //虚拟专网用户名密码
                 	 	 	  	 if (inputStatus==STATUS_NONE)
                 	 	 	  	 {
                 	 	 	  	   if (keyLeftRight<63)
                 	 	 	  	   {
                 	 	 	  		   keyLeftRight++;
                 	 	 	  	   }
                 	 	 	  	   else
                 	 	 	  	   {
                 	 	 	  		   keyLeftRight = 0;
                 	 	 	  	   }
                 	 	 	  	   setVpn(keyLeftRight);
                 	 	 	  	 }
                 	 	 	  	 else
                 	 	 	  	 {
                 	 	 	  	   if (selectIndex<69)
                 	 	 	  	   {
                 	 	 	  		   selectIndex++;
                 	 	 	  	   }
                 	 	 	  	   else
                 	 	 	  	   {
                 	 	 	  		   selectIndex = 0;
                 	 	 	  	   }
                 	 	 	  	   selectChar(selectIndex);
                 	 	 	  	 }
      	    		           break;

      	    		      }
      	    		      break;
      	    		      
      	    		    case 5:       //现场调试
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 2:  //搜索表号
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  searchMeter(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	break;

             	 	 	  		case 3:  //新增电能表地址
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  newAddMeter(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	break;

             	 	 	  		case 5:  //未搜到电能表地址
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  noFoundMeter(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  		 	
             	 	 	  		case 6:  //主动上报
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  activeReportMenu(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	break;
      	    		      }
      	    		    	break;
      	    		 }
             	 	 break;
             	 	 
             	 case 4:    //菜单第4层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 4: //参数设置
             	 	 	  	if (inputStatus==STATUS_NONE)
             	 	 	  	{
             	 	 	  	  if (inputIndex<15)
             	 	 	  	  {
             	 	 	  		   inputIndex++;
             	 	 	  	  }
             	 	 	  	  else
             	 	 	  	  {
             	 	 	  		   inputIndex = 0;
             	 	 	  	  }
             	 	 	  	  inputApn(inputIndex);
             	 	 	  	}
             	 	 	  	else
             	 	 	  	{
             	 	 	  	  if (selectIndex<69)
             	 	 	  	  {
             	 	 	  		   selectIndex++;
             	 	 	  	  }
             	 	 	  	  else
             	 	 	  	  {
             	 	 	  		   selectIndex = 0;
             	 	 	  	  }
             	 	 	  	  selectChar(selectIndex);
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 5: //现场调试
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //实时抄表
             	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
             	 	 	  		 	{
             	 	 	  		 	 	case 0:  //指定测量点抄表
             	 	 	  		 	 		if (pDotCopy==NULL)
             	 	 	  		 	 		{
             	 	 	  		 	 		  if (keyLeftRight>=3)
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight = 0;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    else
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight++;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	  }
             	 	 	  		 	 	  break;
             	 	 	  		 	 	
             	 	 	  		 	 	case 1:  //全部测量点抄表
             	 	 	  		 	 		if (keyLeftRight!=0xff)
             	 	 	  		 	 		{
             	 	 	  		 	 		  if (keyLeftRight>=1)
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight = 0;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    else
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight++;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    allMpCopy(keyLeftRight);
             	 	 	  		 	 	  }
             	 	 	  		 	 		break;
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;
             	 	 
             	 case 20:   //菜单第20层(输入密码)
      	    		 if (pwLight>=5)
      	    		 {
      	    		   pwLight = 0;
      	    		 }
      	    		 else
      	    		 {
      	    		 	 pwLight++;
      	    		 }      	    		 
      	    		 inputPassWord(pwLight);
             	 	 break;
             }
           	 break;

           case KEY_UP:    //向上
           	 switch(menuInLayer)
             {
             	 case 1:
             	 	 if (layer1MenuLight==0)
             	 	 {
             	 	 	  layer1MenuLight = MAX_LAYER1_MENU-1;
             	 	 }
             	 	 else
             	 	 {
             	 	 	  layer1MenuLight--;
             	 	 }
             	 	 layer1Menu(layer1MenuLight);
             	   break;
             	   
             	 case 2:    //菜单第2层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 0:    //抄表查询
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //1-1测量点有功抄表查询
             	 	 	  		 	stringUpDown(queryTimeStr, layer3MenuLight[0][0], 0);  //高亮的字符增大一个字符
             	 	 	  		 	copyQueryMenu(layer2MenuLight[0], layer3MenuLight[0][0]);
             	 	 	  		  break;

             	 	 	  		default:
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
                            layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]--;
             	 	 	  		 	}
             	 	 	  		 	copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	break;
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 1:    //参数查询
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //2-1测量点参数查询
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
                            layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]--;
             	 	 	  		 	}
             	 	 	  		 	paraQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	break;
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2:    //重点用户抄表查询
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //3-1重点用户抄表查询
             	 	 	  		 	stringUpDown(queryTimeStr, layer3MenuLight[2][0], 0);  //高亮的字符增大一个字符
             	 	 	  		 	keyHouseholdMenu(layer2MenuLight[2], layer3MenuLight[2][0]);
             	 	 	  		  break;
             	 	 	  		  
             	 	 	  		default:
             	 	 	  		 	keyLeftRight = 0;
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
                            layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]--;
             	 	 	  		 	}
             	 	 	  		 	keyHouseholdMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	break;
             	 	 	    }
             	 	 	    break;
             	 	 	    
             	 	 	  case 3:    //统计查询
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //4-1运行统计信息查询
             	 	 	  		 	stringUpDown(queryTimeStr, layer3MenuLight[3][0], 0);  //高亮的字符增大一个字符
             	 	 	  		 	statisQueryMenu(layer2MenuLight[3], layer3MenuLight[3][0]);
             	 	 	  		  break;
             	 	 	    }
             	 	 	    break;
             	 	 	    
      	    		 	 case 4:     //参数设置
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]==0)
      	    		 	 	 {      	    		 	 	 	  
      	    		 	 	 	  layer2MenuLight[layer1MenuLight] = layer2MenuNum[layer1MenuLight]-1;
      	    		 	 	 }
      	    		 	 	 else
      	    		 	 	 {
      	    		 	 	 	  layer2MenuLight[layer1MenuLight]--;
      	    		 	 	 }
      	    		 	 	 paraSetMenu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	    		 	 	 break;
      	    		 	 	 
      	    		 	 case 5:     //现场调试
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]==0)
      	    		 	 	 {      	    		 	 	 	  
      	    		 	 	 	  layer2MenuLight[layer1MenuLight] = layer2MenuNum[layer1MenuLight]-1;
      	    		 	 	 }
      	    		 	 	 else
      	    		 	 	 {
      	    		 	 	 	  layer2MenuLight[layer1MenuLight]--;
      	    		 	 	 }
      	    		 	 	 debugMenu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	    		 	 	 break;
             	 	 }
             	 	 break;
             	 	 
             	 case 3:    //菜单第3层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 4:  //参数设置
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		 case 0: //通信参数设置
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
             	 	 	  		 	   if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
             	 	 	  		 	   {
             	 	 	  		 	 	    for(i=strlen(commParaItem[1]);i<16;i++)
             	 	 	  		 	 	    {
             	 	 	  		 	 	    	 commParaItem[1][i] = ' ';
             	 	 	  		 	 	    }
             	 	 	  		 	 	    commParaItem[1][16] = '\0';
             	 	 	  		 	 	    inputStatus=STATUS_NONE;
             	 	 	  		 	 	    inputIndex = 0;
             	 	 	  		 	 	    inputApn(inputIndex); //APN的输入处理
             	 	 	  		 	   }
             	 	 	  		 	   else
             	 	 	  		 	   {
      	    		               stringUpDown(commParaItem[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
      	    		               commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		             }
      	    		           }
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 1: //修改密码
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {                                 	    		           
      	    		             stringUpDown(commParaItem[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
      	    		             modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 2: //VPN用户名密码
                   	 	 	   if (inputStatus==STATUS_SELECT_CHAR)
                   	 	 	   {
                   	 	 	  	 if (selectIndex>=14)
                   	 	 	  	 {
                   	 	 	  		 selectIndex -= 14;
                   	 	 	  	 }
                   	 	 	  	 else
                   	 	 	  	 {
                   	 	 	  		 selectIndex = 68;
                   	 	 	  	 }
                   	 	 	  	 selectChar(selectIndex);
                   	 	 	   }
                   	 	 	   if (inputStatus==STATUS_NONE)
                   	 	 	   {
                   	 	 	  	 selectIndex = 0;
                             selectChar(selectIndex);
                           }
                   	 	 	   break;
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 5:  //现场调试
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		 case 0:  //实时抄表
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
             	 	 	  		 	 {
                             layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1;
                           }
             	 	 	  		 	 else
             	 	 	  		 	 {
            	 	 	  		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]--;
             	 	 	  		 	 }
             	 	 	  		 	 realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 3:  //新增电能表地址
           	 	 	  				 if (multiCpUpDown==0)
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown =  multiCpMax-1;
           	 	 	  				 }
           	 	 	  				 else
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown--;
           	 	 	  				 }
           	 	 	  				 
           	 	 	  				 newAddMeter(keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 4:   //新增电能表抄表状态
           	 	 	  				 if (multiCpUpDown==0)
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown = multiCpMax-1;
           	 	 	  				 }
           	 	 	  				 else
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown--;
           	 	 	  				 }
           	 	 	  				 
           	 	 	  				 newMeterCpStatus(keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 5:  //未搜到电能表地址
           	 	 	  				 if (multiCpUpDown==0)
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown =  multiCpMax-1;
           	 	 	  				 }
           	 	 	  				 else
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown--;
           	 	 	  				 }
           	 	 	  				 
           	 	 	  				 noFoundMeter(keyLeftRight);
           	 	 	  				 break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;

            	 case 4:    //菜单第4层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 4: //参数设置
             	 	 	  	if (inputStatus==STATUS_SELECT_CHAR)
             	 	 	  	{
             	 	 	  		if (selectIndex>=14)
             	 	 	  		{
             	 	 	  			 selectIndex -= 14;
             	 	 	  		}
             	 	 	  		else
             	 	 	  		{
             	 	 	  			 selectIndex = 68;
             	 	 	  	  }
             	 	 	  		selectChar(selectIndex);
             	 	 	  	}
             	 	 	  	if (inputStatus==STATUS_NONE)
             	 	 	  	{
             	 	 	  	  selectIndex = 0;
                        selectChar(selectIndex);
                      }
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 5: //现场调试
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //实时抄表
             	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
             	 	 	  		 	{
             	 	 	  		 	 	case 0:  //指定测量点抄表
             	 	 	  		 	 		if (pDotCopy==NULL)
             	 	 	  		 	 		{
             	 	 	  		 	 		  stringUpDown(singleCopyMp, keyLeftRight, 0);
             	 	 	  		 	 	    singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	  }
             	 	 	  		 	 	  break;
             	 	 	  		 	 	  
             	 	 	  		 	 	case 1:
                 	 	 	  			if (keyLeftRight==0xff)
                 	 	 	  			{
                 	 	 	  				 if (multiCpUpDown==0)
                 	 	 	  				 {
                 	 	 	  				 	  multiCpUpDown=multiCpMax-1;
                 	 	 	  				 }
                 	 	 	  				 else
                 	 	 	  				 {
                 	 	 	  				 	  multiCpUpDown--;
                 	 	 	  				 }
                 	 	 	  				 
                 	 	 	  				 allMpCopy(keyLeftRight);
                 	 	 	  			}
                 	 	 	  			break;             	 	 	  		 	 		
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  		 	
             	 	 	  		case 1:
             	 	 	  			if (keyLeftRight==0xff)
             	 	 	  			{
             	 	 	  				 if (multiCpUpDown==0)
             	 	 	  				 {
             	 	 	  				 	  multiCpUpDown=multiCpMax-1;
             	 	 	  				 }
             	 	 	  				 else
             	 	 	  				 {
             	 	 	  				 	  multiCpUpDown--;
             	 	 	  				 }
             	 	 	  				 
             	 	 	  				 allMpCopy(keyLeftRight);
             	 	 	  			}
             	 	 	  			break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;
             	 	              	 	 
             	 case 20:   //菜单第20层
             	 	 stringUpDown(passWord, pwLight,0);
             	 	 inputPassWord(pwLight);
             	 	 break;
             }
           	 break;
           	 
           case KEY_LEFT:  //向左
           	 switch(menuInLayer)
             {
             	 case 1:
             	 	 if (layer1MenuLight==0)
             	 	 {
             	 	 	  layer1MenuLight = MAX_LAYER1_MENU-1;
             	 	 }
             	 	 else
             	 	 {
             	 	 	  layer1MenuLight--;
             	 	 }
             	 	 layer1Menu(layer1MenuLight);
             	   break;
             	   
             	 case 2:    //菜单第2层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 0:    //抄表查询
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //1-1测量点有功抄表查询
             	 	 	  		 	if (layer3MenuLight[0][0]==0)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[0][0] = layer3MenuNum[0][0]-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[0][0]--;
             	 	 	  		 	}
             	 	 	  		 	copyQueryMenu(layer2MenuLight[0],layer3MenuLight[0][0]);
             	 	 	  		  break;
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2:    //重点用户查询
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //3-1重点用户抄表查询
             	 	 	  		 	if (layer3MenuLight[2][0]==0)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[2][0] = layer3MenuNum[2][0]-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[2][0]--;
             	 	 	  		 	}
             	 	 	  		 	keyHouseholdMenu(layer2MenuLight[2],layer3MenuLight[2][0]);
             	 	 	  		  break;
             	 	 	  		  
             	 	 	  		default:
             	 	 	  			if (keyLeftRight==0)
             	 	 	  			{
             	 	 	  				 keyLeftRight = 3;
             	 	 	  			}
             	 	 	  			else
             	 	 	  			{
             	 	 	  				 keyLeftRight--;
             	 	 	  			}
             	 	 	  		 	keyHouseholdMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  			break;
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 3:    //统计查询
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //4-1运行统计信息查询
             	 	 	  		 	if (layer3MenuLight[3][0]==0)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[3][0] = layer3MenuNum[3][0]-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[3][0]--;
             	 	 	  		 	}
             	 	 	  		 	statisQueryMenu(layer2MenuLight[3],layer3MenuLight[3][0]);
             	 	 	  		  break;
             	 	 	  	}
             	 	 	  	break;
             	   }
             	   break;
             	 
             	 case 3:    //菜单第3层
                 switch(layer1MenuLight)
                 {
                 	  case 4:  //参数设置
                 	  	switch(layer2MenuLight[layer1MenuLight])
                 	  	{
                 	  		 case 0: //通信参数设置
                           if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
                             adjustCommParaLight(0);
      	    		             commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
      	    		           break;
      	    		        
      	    		         case 1: //修改密码
                           if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
                             if (keyLeftRight==0)
                       	     {
                       	 	     keyLeftRight = 5;
                       	     }
                       	     else
                       	     {
                       	 	     keyLeftRight--;
                       	     }
      	    		             modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
      	    		           break;
      	    		           
      	    		         case 2:  //虚拟专网用户名密码
                	 	 	  	 if (inputStatus==STATUS_NONE)
                	 	 	  	 {
                             if (keyLeftRight==0)
                         	   {
                         	 	   keyLeftRight = 63;
                         	   }
                         	   else
                         	   {
                         	 	   keyLeftRight--;
                         	   }
        	    		           setVpn(keyLeftRight);
                	 	 	  	 }
                	 	 	  	 else
                	 	 	  	 {
                	 	 	  	   if (selectIndex==0)
                	 	 	  	   {
                	 	 	  		    selectIndex=68;
                	 	 	  	   }
                	 	 	  	   else
                	 	 	  	   {
                	 	 	  		    selectIndex--;
                	 	 	  	   }
                	 	 	  	   selectChar(selectIndex);
                	 	 	  	 }
      	    		           break;
      	    		      }
      	    		      break;
      	    		      
      	    		    case 5:       //现场调试
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 2:  //搜索表号
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	   	keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  searchMeter(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  		 	
             	 	 	  		case 3:  //新增电能表地址
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	   	keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  newAddMeter(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  		 	
             	 	 	  		case 6:  //主动上报
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	   	keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  activeReportMenu(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	break;
      	    		      }
      	    		    	break;
      	    		 }
             	 	 break;
             	 	 
             	 case 4:    //菜单第4层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 4: //参数设置             	 	 	  	
             	 	 	  	if (inputStatus==STATUS_NONE)
             	 	 	  	{
             	 	 	  	  if (inputIndex==0)
             	 	 	  	  {
             	 	 	  		  inputIndex = 15;
             	 	 	  	  }
             	 	 	  	  else
             	 	 	  	  {
             	 	 	  		  inputIndex--;
             	 	 	  	  }
             	 	 	  	  inputApn(inputIndex);
             	 	 	  	}
             	 	 	  	else
             	 	 	  	{
             	 	 	  	  if (selectIndex==0)
             	 	 	  	  {
             	 	 	  		   selectIndex=68;
             	 	 	  	  }
             	 	 	  	  else
             	 	 	  	  {
             	 	 	  		   selectIndex--;
             	 	 	  	  }
             	 	 	  	  selectChar(selectIndex);
             	 	 	  	}
             	 	 	  	break;
             	 	 	  
             	 	 	  case 5: //现场调试
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //实时抄表
             	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
             	 	 	  		 	{
             	 	 	  		 	 	case 0:  //指定测量点抄表
             	 	 	  		 	 		if (pDotCopy==NULL)
             	 	 	  		 	 		{
             	 	 	  		 	 		  if (keyLeftRight==0)
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight = 3;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    else
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight--;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	  }
             	 	 	  		 	 	  break;
             	 	 	  		 	 	  
             	 	 	  		 	 	case 1:  //全部测量点抄表
             	 	 	  		 	 		if (keyLeftRight!=0xff)
             	 	 	  		 	 		{
             	 	 	  		 	 		  if (keyLeftRight==0)
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight = 1;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    else
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight--;
             	 	 	  		 	 	    }
             	 	 	  		 	 		  allMpCopy(keyLeftRight);
             	 	 	  		 	 		}
             	 	 	  		 	 		break;
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;
             	 
             	 case 20:   //菜单第20层(输入密码)
      	    		 if (pwLight==0)
      	    		 {
      	    		   pwLight = 5;
      	    		 }
      	    		 else
      	    		 {
      	    		 	 pwLight--;
      	    		 }
      	    		 inputPassWord(pwLight);
             	 	 break;
             }
           	 break;
        }
      }      
   }
   else
   {
      keyPressCount = 0;
   }
}

/*******************************************************
函数名称:defaultMenu
功能描述:默认菜单(主界面,重庆规约376.1集中器菜单)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void defaultMenu(void)
{
   char     str[25];
   
   menuInLayer = 0;
   displayMode = DEFAULT_DISPLAY_MODE;

	 guiLine(1,17,160,160,0);
	 
	 if (strlen((char *)teName)>10)
	 {
	   guiDisplay((160-strlen((char *)teName)*8)/2,30,(char *)teName,1);
	   guiDisplay(40,48,"台区集中器",1);
	 }
	 else
	 {
	 	 strcpy(str,(char *)teName);
	 	 strcat(str,"台区集中器");
	 	 
	   guiDisplay((160-strlen(str)*8)/2,40,str,1);
	 }
   
   guiLine(40,80,120,122,0);
   guiAscii(40, 80, "20",1);
   guiAscii(56, 80, digital2ToString(sysTime.year,str),1);
   guiAscii(72, 80, "-", 1);
   guiAscii(80, 80, digital2ToString(sysTime.month,str),1);
   guiAscii(96, 80, "-", 1);
   guiAscii(104, 80, digital2ToString(sysTime.day,str),1);
   
   guiDisplay(56,98, weekName[dayWeek(2000+sysTime.year, sysTime.month, sysTime.day)],1);
   
   guiLine(4,140,4,160,1);
   guiLine(42,140,42,160,1);
   guiLine(80,140,80,160,1);
   guiLine(118,140,118,160,1);
   guiLine(157,140,157,160,1);   

   guiLine(4,140,157,140,1);
   guiLine(4,160,157,160,1);

   guiDisplay(8,143,"正常",1);
   
   if (teInRunning==1)
   {
     guiDisplay(46,143,"投运",1);
   }
   else
   {
     guiDisplay(46,143,"未投",1);
   }
   
   //任务执行状态
   if (copyCtrl[4].meterCopying==TRUE)
   {
   	 guiDisplay(84,143,"抄表",1);
   }
   else
   {
   	 if (carrierFlagSet.searchMeter!=0)
   	 {
   	   guiDisplay(84,143,"搜表",1);
   	 }
   	 else
   	 {
       guiDisplay(84,143,"完毕",1);
     }
   }
   
   //主站通信状态
   if (wlModemFlag.permitSendData==0)
   {
     guiDisplay(123,143,"掉线",1);
   }
   else
   {
     guiDisplay(123,143,"空闲",1);   	 
   }
   
   lcdRefresh(17,160);
}

/*******************************************************
函数名称:paraQueryMenu
功能描述:参数查询菜单
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void paraQueryMenu(INT8U layer2Light,INT8U layer3Light)
{
   struct cpAddrLink *tmpLink;
	 char              str[30];
	 char              sayStr[30];
	 INT8U             i, tmpX, tmpY, tmpCount;
	 //DATE_TIME         tmpTime;

	 guiLine(1,17,160,160,0); //清屏
	 menuInLayer = 2;         //菜单进入第2层
	 
	 switch(layer2Light)
	 {
	 	 case 0:
	 	   guiDisplay(1,17,"2-1测量点参数查询",1);
	 	   guiDisplay(1,33,"测量点号  电表地址",1);
	 	   
   	   if (layer3Light>0)
   	   {
   	     guiDisplay(144,33,"↑",1);
   	   }
       
       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
       {
   	     guiDisplay(144,144,"↓",1);
   	   }
   	   
   	   tmpY = 49;
   	   tmpLink = queryMpLink;
   	   i = 0;
   	   while(tmpLink!=NULL && i<layer3Light)
   	   {
   	     for(tmpCount=0;tmpCount<NUM_MP_PER_PAGE-1;tmpCount++)
   	     {
   	   	   tmpLink = tmpLink->next;
   	     }
   	     i++;
   	   }
   	   tmpCount = 0;
   	   while((tmpLink!=NULL) && (tmpCount<(NUM_MP_PER_PAGE-1)))
   	   {
   	   	  //
   	   	  strcpy(sayStr,intToString(tmpLink->mpNo,3,str));
   	   	  strcpy(str,"");
   	   	  if (strlen(sayStr)==1)
   	   	  {
   	   	  	 strcat(str,"00");
   	   	  }
   	   	  if (strlen(sayStr)==2)
   	   	  {
   	   	  	 strcat(str,"0");
   	   	  }	 	   	  
   	   	  strcat(str,sayStr);
   	   	  guiDisplay(22,tmpY,str,1);

       	  strcpy(str,"");
       	  for(i=6; i>0; i--)
       	  {
       	  	 if(tmpLink->addr[i-1]==0x00)
       	  	 {
       	  	 	  strcat(str, "00");
       	  	 }
       	  	 else
       	  	 {
       	  	 	  strcat(str,digital2ToString((tmpLink->addr[i-1]/0x10)*10
       	  	          +tmpLink->addr[i-1]%0x10,sayStr));
       	  	 }
       	  }
 	   	    guiDisplay(65,tmpY,str,1);

   	   	  tmpY += 16;
   	   	  tmpLink = tmpLink->next;
   	   	  
   	   	  tmpCount++;
   	   }	 	   
	 	   break;
	 	  	
	 	 case 1:
	 	   guiDisplay(1,17,"2-2通信参数查询",1);
	 	   //行政区划
	 	   guiDisplay(1,33,"行政区划",1);
       strcpy(sayStr,digitalToChar(addrField.a1[1]>>4));
       strcat(sayStr,digitalToChar(addrField.a1[1]&0xf));
       strcat(sayStr,digitalToChar(addrField.a1[0]>>4));
       strcat(sayStr,digitalToChar(addrField.a1[0]&0xf));
	 	   guiDisplay(68,33,sayStr,1);
	 	   
	 	   //上行信道
	 	   guiDisplay(1,49,"上行信道",1);
	 	   switch(moduleType)
	 	   {
	 	   	  case GPRS_SIM300C:
	 	   	  case GPRS_GR64:
	 	   	  case GPRS_M590E:
	 	   	  case GPRS_M72D:
	 	   	  	strcpy(sayStr, "GPRS");
	 	   	  	break;

	 	   	  case CDMA_DTGS800:
	 	   	  case CDMA_CM180:
	 	   	  	strcpy(sayStr, "CDMA");
	 	   	  	break;
					
	 	   	  case LTE_AIR720H:
	 	   	  	strcpy(sayStr, "LTE");
	 	   	  	break;

	 	   	  case ETHERNET:
	 	   	  	strcpy(sayStr, "以太网");
	 	   	  	break;
	 	   	  
	 	   	  default:
	 	   	  	strcpy(sayStr,"无模块");
	 	   	  	break;
	 	   }
	 	   guiDisplay(68,49,sayStr,1);
	 	   
	 	   //APN域名
	 	   guiDisplay(1,  65, "APN域名",1);
	 	   guiDisplay(68, 65, (char *)ipAndPort.apn, 1);
	 	   
	 	   guiDisplay(1,81,"主站IP及接入端口",1);
	  	 strcpy(sayStr,intToIpadd(ipAndPort.ipAddr[0]<<24 | ipAndPort.ipAddr[1]<<16 | ipAndPort.ipAddr[2]<<8 | ipAndPort.ipAddr[3],str));
       strcat(sayStr,":");
	 	   guiDisplay(1,97,sayStr,1);
	 	   tmpX = 1+8*strlen(sayStr);
 	     strcpy(sayStr,intToString(ipAndPort.port[1]<<8 | ipAndPort.port[0],3,str));
	 	   guiDisplay(tmpX,97,sayStr,1);
	 	   
	 	   guiDisplay(1,113,"集中器IP",1);
	 	   guiDisplay(1,129,intToIpadd(wlLocalIpAddr,str),1);
	 	   guiDisplay(1,145,"程序版本",1);
	 	   guiDisplay(68,145,vers,1);
	 	   break;
	 	   
	 	 case 2:
	 	   guiDisplay(1,17,"2-3抄表参数查询",1);
	 	   guiDisplay(1,33,"抄表开始时间  00:01",1);

	 	   strcpy(sayStr,"重点户抄表间隔");
	 	   strcat(sayStr,intToString(teCopyRunPara.para[0].copyInterval,3,str));
	 	   strcat(sayStr,"分钟");
	 	   guiDisplay(1,49,sayStr,1);
	 	   
	 	   guiDisplay(1,65,"重点户采集量U",1);
	 	   guiDisplay(105,65,"/",1);
	 	   guiDisplay(112,65,"I",1);
	 	   guiDisplay(119,65,"/",1);
	 	   guiDisplay(126,65,"P",1);
	 	   guiDisplay(133,65,"/",1);
	 	   guiDisplay(140,65,"Q",1);
	 	   guiDisplay(147,65,"/",1);
	 	   guiDisplay(153,65,"R",1);
	 	   break;
	 	   
	 	 case 3:
	 	   guiDisplay(1,17,"2-4中继路由信息查询",1);
	 	   guiDisplay(1,33,"1级中继电表信息:",1);
	 	   guiDisplay(1,49,"测量点 上级中继表号",1);
	 	   break;
	 	   
	 	 case 4:
	 	   guiDisplay(1,17,"2-5中断路由信息查询",1);
	 	   guiDisplay(1,33,"2级中继电表信息",1);
	 	   guiDisplay(1,49,"测量点 上级中继表号",1);
	 	   break;
	 }
	 
	 lcdRefresh(17, 160);
}

/*******************************************************
函数名称:keyHouseholdMenu
功能描述:重点用户信息查询菜单
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void keyHouseholdMenu(INT8U layer2Light,INT8U layer3Light)
{
	 INT8U             dataBuff[LENGTH_OF_ENERGY_RECORD];
 	 INT32U            integer,decimal,disData;
	 char              str[30];
	 char              sayStr[30];
	 INT8U             i, tmpY, tmpCount, tmpX=0;
	 INT8U             dataType;
	 INT8U             tmpMinute;
	 DATE_TIME         tmpTime, readTime;
   struct cpAddrLink *tmpLink;
   INT16U            offset;
   INT8U             sign;
   INT8U             meterInfo[10];
   BOOL              buffHasData;
   INT8U             xx;

	 guiLine(1,17,160,160,0); //清屏
	 menuInLayer = 2;         //菜单进入第2层
	 
	 switch(layer2Light)
	 {
	 	 case 0:
	 	   guiDisplay(1,17,"3-1重点用户抄表查询",1);
	 	   showInputTime(layer3Light);
	 	   break;
	 	   
	 	 case 1:
	 	   guiDisplay(1,17,"3-2重点用户电能示值",1);
	 	   guiDisplay(78,33,"电能示值",1);
	 	   dataType = ENERGY_DATA;
	 	   offset   = POSITIVE_WORK_OFFSET;
	 	 	 break;
	 	 	 
	 	 case 2:
	 	   guiDisplay(1,17,"3-3重点用户有功曲线",1);
	 	   guiDisplay(78,33,"有功功率",1);
	 	   dataType = PARA_VARIABLE_DATA;
	 	   offset   = POWER_INSTANT_WORK;
	 	 	 break;

	 	 case 3:
	 	   guiDisplay(1,17,"3-4重点用户无功曲线",1);
	 	   guiDisplay(78,33,"无功功率",1);
	 	   dataType = PARA_VARIABLE_DATA;
	 	   offset   = POWER_INSTANT_NO_WORK;
	 	 	 break;

	 	 case 4:
	 	   guiDisplay(1,17,"3-5重点用户电流曲线",1);
	 	   guiDisplay(70,33,"A相 B相 C相",1);
	 	   dataType = PARA_VARIABLE_DATA;
	 	   offset = CURRENT_PHASE_A;
	 	 	 break;

	 	 case 5:
	 	   guiDisplay(1,17,"3-6重点用户电压曲线",1);
	 	   guiDisplay(70,33,"A相 B相 C相",1);
	 	   dataType = PARA_VARIABLE_DATA;
	 	   offset = VOLTAGE_PHASE_A;
	 	 	 break;
	 }

	 lcdRefresh(17, 160);
	 
	 if (layer2Light>=1 && layer2Light<=5)
	 {
 	   if (layer3Light>0)
 	   {
 	     //guiDisplay(144,33,"↑",1);
 	   }
     
     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
     {
 	     //guiDisplay(144,144,"↓",1);
 	   }
 	   
 	   tmpLink = queryMpLink;
 	   i = 0;
 	   while(tmpLink!=NULL && i<layer3Light)
 	   {
 	   	  tmpLink = tmpLink->next;
 	      i++;
 	   }
 	   if (tmpLink==NULL)
 	   {
 	   	  tmpLink = queryMpLink;
 	   }
 	   
 	   strcpy(sayStr,"测量点");
 	   if (tmpLink->mp<10)
 	   {
 	   	 strcat(sayStr,"0");
 	   }
 	   strcat(sayStr,intToString(tmpLink->mp,3,str));
 	   guiDisplay(1,33,sayStr,1);

		 tmpTime = menuQueryTime;
		 tmpTime.hour   = keyLeftRight*6;
		 tmpTime.minute = 0x0;
		 tmpTime.second = 0x0;
		 tmpTime = backTime(tmpTime, 0, 0, 0, 15, 0);
	   
	   lcdRefresh(33, 48);
		 
		 tmpY = 52;
		 for(i = 0; i < 6; i++)
		 {
				strcpy(sayStr,"");
				if (tmpTime.hour==23)
				{
					strcpy(sayStr,"00");
				}
				else
				{
				  if (tmpTime.hour<9)
				  {
					   strcat(sayStr,"0");
				  }
				  strcat(sayStr,intToString(tmpTime.hour+1,3,str));
				}
				guiDisplay(17, tmpY, sayStr, 1);
				
				buffHasData = FALSE;
				
				queryMeterStoreInfo(tmpLink->mp,meterInfo);
				readTime  = timeHexToBcd(tmpTime);
				if (meterInfo[0]==8)
				{
          buffHasData = readMeterData(dataBuff, tmpLink->mp, CURVE_KEY_HOUSEHOLD, dataType, &readTime, 0x60);
				}
				else
				{
          buffHasData = readMeterData(dataBuff, tmpLink->mp, CURVE_DATA_PRESENT, dataType, &readTime, 0x60);
				}
				strcpy(sayStr,"");
				if(buffHasData== TRUE)
				{
				   if (dataBuff[offset]!=0xee)
       	   {
  				   switch(layer2Light)
  				   {
  				     case 1:  //电能示值
                 decimal = (dataBuff[offset]>>4 & 0xf) * 10 + (dataBuff[offset] & 0xf);
                 integer = (dataBuff[offset+3]>>4 & 0xf)*100000
                           +(dataBuff[offset+3] & 0xf)*10000
                           +(dataBuff[offset+2]>>4 & 0xf)*1000
                           +(dataBuff[offset+2] & 0xf)*100
                           +(dataBuff[offset+1]>>4 & 0xf)*10
                           +(dataBuff[offset+1]& 0xf);
                 strcpy(str, floatToString(integer,decimal,2,2,str));
                 for(tmpCount=0;tmpCount<10-strlen(str);tmpCount++)
                 {
                	 strcat(sayStr," ");
                 }
                 strcat(sayStr,str);
                 tmpX = 0;
                 break;
                 
               case 2: //有功功率
               case 3: //无功功率
     	    	     sign = 0;
               	 disData = dataBuff[offset] | dataBuff[offset+1]<<8 | dataBuff[offset+2]<<16;
     	    	     if (dataBuff[offset+2]&0x80)
     	    	     {
     	    	        sign = 1;
     	    	        disData &=0x7fff;
     	    	     }
                 decimal = (disData>>12 & 0xf)*1000
                          +(disData>>8 & 0xf)*100
                          +(disData>>4 & 0xf)*10
                          +(disData & 0xf);
                 integer = (disData>>20 & 0xf)*10+(disData>>16 & 0xf);
                 if (sign==1)
                 {
                 	 strcat(sayStr,"-");
                 }
                 strcat(sayStr,floatToString(integer,decimal,4,2,str));
                 if (strlen(sayStr)<=5)
                 {
                   strcpy(str,"");
                   for(tmpCount=0;tmpCount<5-strlen(sayStr);tmpCount++)
                   {
                	   strcat(str," ");
                   }
                   strcat(str, sayStr);
                   strcpy(sayStr,str);
                 }
                 tmpX = 24;
               	 break;
               	 
               case 4:  //电流
               	 for(tmpCount=0; tmpCount<3; tmpCount++)
               	 {
               	   disData = dataBuff[offset+tmpCount*3] | dataBuff[offset+tmpCount*3+1]<<8 | dataBuff[offset+tmpCount*3+2]<<16;
               	   decimal = bcdToHex(disData& 0xfff);
                   integer = bcdToHex(disData>>12& 0xfff);
                   floatToString(integer,decimal,3,1,str);
                   if (strlen(str)==1)
                   {
                   	 strcat(sayStr,"  ");
                   }
                   if (strlen(str)==2)
                   {
                   	 strcat(sayStr," ");
                   }
                   strcat(sayStr,str);
                   if (tmpCount!=2)
                   {
                     strcat(sayStr," ");
                   }
                 }
                 tmpX = 0;
               	 break;

               case 5:  //电压
               	 for(tmpCount=0; tmpCount<3; tmpCount++)
               	 {
               	   disData = dataBuff[offset+tmpCount*2] | dataBuff[offset+tmpCount*2+1]<<8;
               	   decimal = disData& 0xf;
                   integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10 + (disData>>4 & 0xf);
                   floatToString(integer,decimal,0,0,str);
                   if (strlen(str)==1)
                   {
                   	 strcat(sayStr,"  ");
                   }
                   if (strlen(str)==2)
                   {
                   	 strcat(sayStr," ");
                   }
                   strcat(sayStr,str);
                   if (tmpCount!=2)
                   {
                     strcat(sayStr," ");                   
                   }
                 }
                 tmpX = 0;
               	 break;
             }
       	   }
       	   else
       	   {
       	      strcpy(sayStr,"-----");
       	      if (tmpX==0)
       	      {
       	      	 tmpX = 24;
       	      }
           }
				}
				else
				{
				   strcpy(sayStr,"-----");
       	   if (tmpX==0)
       	   {
       	   	 tmpX = 24;
       	   }
       	}
				
				guiDisplay(70+tmpX,tmpY,sayStr,1);

				lcdRefresh(tmpY, tmpY+16);
				
				tmpY += 18;
				tmpTime = nextTime(tmpTime, 60, 0);				
		 }
	 }
	 
	 lcdRefresh(17, 160);
}

/*******************************************************
函数名称:statisQueryMenu
功能描述:统计信息查询菜单
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void statisQueryMenu(INT8U layer2Light,INT8U layer3Light)
{
	 char                   str[30];
	 char                   sayStr[30];
	 INT8U                  i, tmpY, tmpCount, tmpX=0;
	 INT16U                 tmpSuccess,tmpFailure;
	 struct cpAddrLink      *tmpNode;
   TERMINAL_STATIS_RECORD terminalStatisRecord;  //终端统计记录
   DATE_TIME              tmpTime;
   INT32U                 tmpData;
   INT8U                  buf[20];
   INT8U                  k;

   if (layer2Light>0)
   {
     guiLine(16,56,144,88,0);
     guiLine(16,56,16,88,1);
     guiLine(144,56,144,88,1);
     guiLine(16,88,144,88,1);
     guiLine(16,56,144,56,1);
     guiDisplay(30,64,"读取数据中...",1);
     lcdRefresh(56,88);
   }

	 guiLine(1,17,160,160,0); //清屏
	 menuInLayer = 2;         //菜单进入第2层

	 switch(layer2Light)
	 {
	 	 case 0:
	 	   guiDisplay(1,17,"4-1运行统计信息查询",1);
	 	   showInputTime(layer3Light);
	 	   break;
	 	   
	 	 case 1:
	 	   guiDisplay(1,17,"4-2抄表统计",1);
	 	   guiDisplay(1,33,"电能表总数",1);
	 	   guiDisplay(90,33, intToString(meterDeviceNum,3,str),1);
	 	   guiDisplay(1,49,"重点用户数",1);
	 	   guiDisplay(90,49, intToString(keyHouseHold.numOfHousehold, 3, str), 1);
	 	   tmpSuccess = 0;
	 	   tmpFailure = 0;
       
       tmpTime = timeHexToBcd(menuQueryTime);
       tmpNode = initPortMeterLink(0xff);

       lcdLightDelay = nextTime(sysTime, 20, 0);

       k = 0;
       while(tmpNode!=NULL)
       {
         if (checkLastDayData(tmpNode->mp, 1, tmpTime, 0)==TRUE)
         {
         	 tmpSuccess++;
         }

       	 tmpNode = tmpNode->next;
      	 
      	 k++;
      	 if (k>=15)
      	 {
      	   k = 0;
      	   
      	   //查询期间发送心跳给xMega,避免xMega给CPU发复位信号
      	   sendXmegaInTimeFrame(CPU_HEART_BEAT, buf, 0);
      	 }

         //释放CPU时间片给其它线程执行
         usleep(100);
       }

	 	   tmpFailure = meterDeviceNum-tmpSuccess;
	 	   
	 	   guiDisplay(1,65,"抄表成功数",1);
       guiDisplay(90,65, intToString(tmpSuccess, 3, str),1);
	 	   guiDisplay(1,81,"抄表失败数",1);
       guiDisplay(90,81, intToString(tmpFailure, 3, str),1);
	 	 	 break;
	 	 	 
	 	 case 2:
	 	   guiDisplay(1,17,"4-3抄表失败清单",1);
	 	   guiDisplay(1,33,"测量点号",1);

	 	   tmpNode = copyCtrl[4].cpLinkHead;
	 	   tmpX    = 1;
	 	   tmpY    = 49;
	 	   tmpFailure = 0;
	 	   while(tmpNode!=NULL)
	 	   {
	 	   	 if (tmpNode->copySuccess==FALSE)
	 	   	 {
 	   	     tmpFailure ++;
 	   	     strcpy(sayStr,intToString(tmpNode->mp,3,str));
 	   	     strcpy(str,"");
 	   	     if (strlen(sayStr)==1)
 	   	     {
 	   	  	   strcat(str," 00");
 	   	     }
 	   	     if (strlen(sayStr)==2)
 	   	     {
 	   	  	   strcat(str," 0");
 	   	     }
 	   	     strcat(str,sayStr);
 	   	     guiDisplay(tmpX,tmpY,str,1);
 	   	     tmpX += 40;
 	   	     if ((tmpFailure%4)==0)
 	   	     {
 	   	     	  tmpY += 16;
 	   	     }
 	   	     
 	   	     //需处理下一屏
 	   	     if (tmpY>=160)
 	   	     {
 	   	     	  break;
 	   	     }
	 	   	 }
	 	   	  
	 	   	 tmpNode = tmpNode->next;	 	   	  
	 	   }
	 	 	 break;
	 	 	 
	 	 case 3:
	 	   guiDisplay(1,17,"4-4数据通信统计",1);
	 	   guiDisplay(1,33,"当日通信流量",1);
	 	   guiDisplay(1,49,"当月通信流量",1);
	 	   guiDisplay(1,65,"信号强度Max",1);
	 	   guiDisplay(1,81,"发生时间",1);
	 	   guiDisplay(1,97,"信号强度Min",1);
	 	   guiDisplay(1,113,"发生时间",1);	 	   
	     
 	   	 tmpTime = timeHexToBcd(menuQueryTime);
	     if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
       {
       	  if ((terminalStatisRecord.sendBytes+terminalStatisRecord.receiveBytes)<1024)
       	  {
       	    strcpy(sayStr, intToString(terminalStatisRecord.sendBytes+terminalStatisRecord.receiveBytes,3,str));
       	    strcat(sayStr, "B");
       	  }
       	  else
       	  {
       	    strcpy(sayStr, intToString((terminalStatisRecord.sendBytes+terminalStatisRecord.receiveBytes)/1024,3,str));
       	    strcat(sayStr, "KB");
       	  }
       	  guiDisplay(104, 33, sayStr, 1);
       	  guiDisplay(104,65,intToString(terminalStatisRecord.maxSignal,3,str),1);
       	  strcpy(sayStr,"");
       	  strcat(sayStr,digital2ToString(terminalStatisRecord.maxSignalTime[1],str));
       	  strcat(sayStr,":");
       	  strcat(sayStr,digital2ToString(terminalStatisRecord.maxSignalTime[0],str));
       	  guiDisplay(104,81,sayStr,1);
       	  
       	  guiDisplay(104,97,intToString(terminalStatisRecord.minSignal,3,str),1);
       	  strcpy(sayStr,"");
       	  strcat(sayStr,digital2ToString(terminalStatisRecord.minSignalTime[1],str));
       	  strcat(sayStr,":");
       	  strcat(sayStr,digital2ToString(terminalStatisRecord.minSignalTime[0],str));
       	  guiDisplay(104,113,sayStr,1);
       	         	  
       	  tmpCount = monthDays(sysTime.year+2000,sysTime.month);
       	  tmpData = 0;
       	  for(i=1;i<=tmpCount && i<=sysTime.day;i++)
       	  {
       	  	 tmpTime = sysTime;
       	  	 tmpTime.day = i;
       	  	 tmpTime = timeHexToBcd(tmpTime);
	           if(readMeterData((INT8U *)&terminalStatisRecord, 0, STATIS_DATA, 0, &tmpTime, 0) == TRUE)
             {
             	  tmpData += terminalStatisRecord.sendBytes+terminalStatisRecord.receiveBytes;
             }
       	  }
       	  if (tmpData<1024)
       	  {
       	    strcpy(sayStr,intToString(tmpData,3,str));
       	    strcat(sayStr, "B");
       	  }
       	  else
       	  {
       	    strcpy(sayStr,intToString(tmpData/1024,3,str));
       	    strcat(sayStr, "KB");
       	  }
       	  guiDisplay(104, 49, sayStr, 1);       	  
       }
	 	 	 break;
	 	 	 
	 	 case 4:
	 	   guiDisplay(1,17,"4-5中继路由信息统计",1);
	 	   guiDisplay(1,33,"0级中继电能表数",1);
	 	   guiDisplay(1,49,"1级中继电能表数",1);
	 	   guiDisplay(1,65,"2级中继电能表数",1);
	 	 	 break;
	 }
	 
	 lcdRefresh(17,160);
}

/*******************************************************
函数名称:paraSetMenu
功能描述:参数设置菜单
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void paraSetMenu(int lightNum,int layer1Num)
{
	 INT8U i;

	 guiLine(1,17,160,160,0); //清屏
	 menuInLayer = 2;         //菜单进入第2层
   
	 for(i=0;i<layer2MenuNum[layer1Num];i++)
	 {
		 if (i==lightNum)
		 {
        guiDisplay(16,40+i*17,paraSetItem[i],0);
		 }
		 else
		 {
        guiDisplay(16,40+i*17,paraSetItem[i],1);
		 }
	 }
	 
	 lcdRefresh(17, 160);
}

/*******************************************************
函数名称:debugMenu
功能描述:现场调试菜单
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void debugMenu(int lightNum,int layer1Num)
{
	 INT8U i;

	 guiLine(1,17,160,160,0); //清屏
	 menuInLayer = 2;         //菜单进入第2层
   
   if (carrierModuleType==SR_WIRELESS || carrierModuleType==HWWD_WIRELESS || carrierModuleType==RL_WIRELESS)
   {
   	 strcpy(debugItem[3], "   入网节点地址   ");
   	 strcpy(debugItem[5], "  未入网节点地址  ");
   }
   
	 for(i=0;i<layer2MenuNum[layer1Num];i++)
	 {
		 if (i==lightNum)
		 {
        guiDisplay(8,32+i*17,debugItem[i],0);
		 }
		 else
		 {
        guiDisplay(8,32+i*17,debugItem[i],1);
		 }
	 }
	 
	 lcdRefresh(17, 160);
}

/*******************************************************
函数名称:noFoundMeter
功能描述:未搜到电能表地址
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void noFoundMeter(INT8U lightNum)
{
	 INT8U  tmpY, i;
	 INT16U tmpNum;
   char   str[10],sayStr[15];
   
	#ifdef MENU_FOR_CQ_CANON 
	 guiLine(1,17,160,160,0); //清屏
   if (carrierModuleType==SR_WIRELESS || carrierModuleType==HWWD_WIRELESS || carrierModuleType==RL_WIRELESS)
   {
     guiDisplay(1,17,"6-8未入网节点地址",1);
   }
   else
   {
     guiDisplay(1,17,"6-8未搜到电能表地址",1);
     
     if (carrierFlagSet.searchMeter==0)
     {
       if (carrierFlagSet.ifSearched == 0)
       {
         guiDisplay(12,49,"未启动搜表,无统计",1);
       }
     }
     else
     {
       guiDisplay(17,49,"搜表中,请等待...",1);
     }
   }
	#else
	 guiLine(1,17,160,144,0); //清屏
   if (carrierModuleType==SR_WIRELESS || carrierModuleType==HWWD_WIRELESS || carrierModuleType==RL_WIRELESS)
   {
     guiDisplay(1,17,"   未入网节点地址",1);
   }
   else
   {
     guiDisplay(1,17,"  未搜到电能表地址",1);
   }
	#endif
	
	 menuInLayer = 3;         //菜单进入第3层

   if (lightNum!=0xff)
   {
		 //计算最大页数
		 multiCpMax = 0;
		 tmpFound = noFoundMeterHead;
		 while(tmpFound!=NULL)
		 {
			 multiCpMax++;
			 tmpFound = tmpFound->next;
		 }
		 
		#ifdef MENU_FOR_CQ_CANON
		 if ((multiCpMax%(NUM_MP_PER_PAGE-1))!=0)
		 {
		    multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-1)+1;
		 }
		 else
		 {
		   multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-1);
		 }
		#else
		 if ((multiCpMax%(NUM_MP_PER_PAGE-2))!=0)
		 {
		    multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-2)+1;
		 }
		 else
		 {
		   multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-2);
		 }
		#endif

     //滚动到需要显示的页
     tmpFound = noFoundMeterHead;
     if (tmpFound!=NULL)
     {
       #ifdef MENU_FOR_CQ_CANON
        for(i=0;i<multiCpUpDown*(NUM_MP_PER_PAGE-1);i++)
       #else
        for(i=0;i<multiCpUpDown*(NUM_MP_PER_PAGE-2);i++)
       #endif
        {
      	  tmpFound = tmpFound->next;      	  
      	  
      	  if (tmpFound==NULL)
      	  {
      	  	 break;
      	  }
        }
     }

     tmpY     = 33;
     tmpNum   = 0;
     while(tmpFound!=NULL)
     {
     	  strcpy(sayStr,"");
     	  for(i=6;i>0;i--)
     	  {
     	    strcat(sayStr,digital2ToString((tmpFound->addr[i-1]/0x10)*10 + tmpFound->addr[i-1]%0x10,str));
     	  }
     	  
     	  guiDisplay(32,tmpY, sayStr, 1);
     	  
     	  tmpY += 16;
     	  tmpNum++;
     	 #ifdef MENU_FOR_CQ_CANON
     	  if (tmpNum>=(NUM_MP_PER_PAGE-1))
     	 #else
     	  if (tmpNum>=(NUM_MP_PER_PAGE-2))
     	 #endif
     	  {
     	  	 break;
     	  }
     	  
     	  tmpFound = tmpFound->next;
     }
   }
   
  #ifdef MENU_FOR_CQ_CANON 
   lcdRefresh(17,160);
  #else
   lcdRefresh(17,145);
  #endif
}

/*******************************************************
函数名称:activeReportMenu
功能描述:主动上报菜单
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void activeReportMenu(INT8U lightNum)
{
	 INT8U tmpX, i;
   char  str[2];
   
	 guiLine(1,17,160,160,0); //清屏
	 menuInLayer = 3;         //菜单进入第3层

   guiDisplay(1,17,"6-9主动上报",1);
   
   guiLine(45,60,45,78,1);
   guiLine(115,60,115,78,1);
   guiLine(45,60,115,60,1);
   guiLine(45,78,115,78,1);

   if (lightNum==0)
   {
     guiDisplay(48,61,"呼叫主站",0);
   }
   else
   {
     guiDisplay(48,61,"呼叫主站",1);
   }

   lcdRefresh(17,160);
}


//以上从约81行开始都是重庆集中器菜单

#else  //MENU_FOR_CQ_CANON , 从本行开始是国家电网集中器要求菜单

/**************************************************
函数名称:userInterface
功能描述:人机接口处理(376.1国家电网集中器规约)
调用函数:
被调用函数:
输入参数:void *arg
输出参数:
返回值：状态
***************************************************/
void userInterface(BOOL secondChanged)
{
	 METER_DEVICE_CONFIG meterConfig;
   char      str[30],strX[30];
   INT16U    tmpData;
   INT8U     i;
   DATE_TIME tmpTime;
   INT16U    tmpAddr;                    //临时终端地址
   char      *tmpChar;

   //ly,2011-4-13,addd
   register int fd, interface, retn = 0;
   struct ifreq buf[MAXINTERFACES];
   struct ifconf ifc;

   if (secondChanged==TRUE)
   {
     if (aberrantAlarm.aberrantFlag==1)
     {
       guiLine(44,1,62,16,0);
   
       if (aberrantAlarm.blinkCount==1)
       {
         aberrantAlarm.blinkCount = 2;
         guiDisplay(46, 1, "！", 1);
         guiLine(46,1,62,4,0);
         guiDisplay(46, 2, "○", 1);
       }
       else
       {
   	     guiDisplay(46, 1, digital2ToString(aberrantAlarm.eventNum, str), 1);
   	     aberrantAlarm.blinkCount = 1;
   	   }
       
       if (compareTwoTime(aberrantAlarm.timeOut, sysTime))
       {
         guiLine(30,1,62,16,0);
         aberrantAlarm.aberrantFlag = 0;
       }
       
       lcdRefresh(1,16);
     }
     
     //title时间
     if (sysTime.second==0)
     {
       refreshTitleTime();
     }

     if (lcdLightOn==LCD_LIGHT_ON)
     {
       if (compareTwoTime(lcdLightDelay, sysTime))
       {
     	  #ifndef LIGHTING
     	   if (displayMode==CYCLE_DISPLAY_MODE)
     	   {
     	  #else
     	   defaultMenu();
     	  #endif
     	     
     	     lcdLightOn = LCD_LIGHT_OFF;
     	     lcdBackLight(LCD_LIGHT_OFF);
     	     
     	  #ifndef LIGHTING
     	   }
     	   else
     	   {
     	     //进入轮显菜单模式
     	     menuInLayer = 0;
     	     defaultMenu();
     	     
           while(cycleMpLink!=NULL)
           {
           	  tmpCycleLink = cycleMpLink;
           	  cycleMpLink  = cycleMpLink->next;
           	  free(tmpCycleLink);
           }
           
           //2012-10-23,修改为轮显内容可设置
           if (cycleDataType==0x55)
           {
             cycleMpLink = initPortMeterLink(0x01);
           }
           else
           {
             cycleMpLink = initPortMeterLink(0xff);
           }
           tmpCycleLink = cycleMpLink;
           if (cycleMpLink==NULL)
           {
     	       lcdLightOn = LCD_LIGHT_OFF;
     	       lcdBackLight(LCD_LIGHT_OFF);
           }
           else
           {           
     	       lcdLightDelay = nextTime(sysTime,1,0);
     	       displayMode = CYCLE_DISPLAY_MODE;
     	       cycleMenu();
     	     }
     	   }
     	  #endif
       }
     }
     
    #ifndef LIGHTING
     if (displayMode==CYCLE_DISPLAY_MODE)
     {
     	 if (compareTwoTime(cycleDelay,sysTime))
     	 {
     	 	 cycleMenu();
     	 }
     }
    #endif
     
     if (setParaWaitTime!=0 && setParaWaitTime!=0xfe)
     {
    		setParaWaitTime--;
    		if (setParaWaitTime<1)
    		{
         	setBeeper(BEEPER_OFF);         //蜂鸣器
         	alarmLedCtrl(ALARM_LED_OFF);   //指示灯灭
     	    lcdBackLight(LCD_LIGHT_OFF);
    		}
     }
     
     //默认菜单时显示时间
     if (displayMode==DEFAULT_DISPLAY_MODE && menuInLayer==0)
     {
       guiLine(40,90,120,122,0);
       guiAscii(40, 90, "20",1);
       guiAscii(56, 90, digital2ToString(sysTime.year,str),1);
       guiAscii(72, 90, "-", 1);
       guiAscii(80, 90, digital2ToString(sysTime.month,str),1);
       guiAscii(96, 90, "-", 1);
       guiAscii(104, 90, digital2ToString(sysTime.day,str),1);
     
       guiAscii(48, 106, digital2ToString(sysTime.hour,str),1);
       guiAscii(64, 106, ":", 1);
       guiAscii(72, 106, digital2ToString(sysTime.minute,str),1);
       guiAscii(90, 106, ":", 1);
       guiAscii(98, 106, digital2ToString(sysTime.second,str),1);
       lcdRefresh(90,122);
     }
   }

   if ((keyValue = ioctl(fdOfIoChannel,READ_KEY_VALUE,0))!=0)
   {
      keyPressCount++;
      
      if (keyPressCount>keyCountMax)
      {
        lcdBackLight(LCD_LIGHT_ON);

        lcdLightOn = LCD_LIGHT_ON;
        
        lcdLightDelay = nextTime(sysTime, 1, 0);

        keyPressCount = 0;
        
        if (displayMode!=KEYPRESS_DISPLAY_MODE)
        {
          displayMode = KEYPRESS_DISPLAY_MODE;
          guiLine(85, 1,117,16,0);
          lcdRefresh(1,16);
          layer1Menu(layer1MenuLight);
          return;
        }
        
        switch(keyValue)
        {
      	  case KEY_OK:   //确定
      	    switch(menuInLayer)
      	    {
      	    	case 1:      //菜单第1层
      	    		switch(layer1MenuLight)
      	    		{
	    		 	      case 0://测量点数据查询
	    		 	 	     #ifdef LIGHTING
	    		 	 	      
	    		 	 	      queryMpLink = copyCtrl[4].cpLinkHead;
	    		 	 	      if (queryMpLink==NULL)
	    		 	 	      {
                      guiLine(10,55,150,105,0);
                      guiLine(10,55,10,105,1);
                      guiLine(150,55,150,105,1);
                      guiLine(10,55,150,55,1);
                      guiLine(10,105,150,105,1);
                      guiDisplay(12,70,"未配置控制点参数!",1);
                      lcdRefresh(10,120);
	    		 	 	      }
	    		 	 	      else
	    		 	 	      {
	    		 	 	        ccbStatus();
	    		 	 	      }
	    		 	 	      
	    		 	 	     #else
	    		 	 	     
	    		 	 	      layer2MenuLight[0] = 0;
	    		 	 	      layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;      	    		 	 	 
	    		 	 	      fillTimeStr();   //用当前日期填充查询日期字符串
	    		 	 	      queryMpLink = initPortMeterLink(0xff);
	    		 	 	      copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
	    		 	 	      
	    		 	 	     #endif
	    		 	 	      break;
      	    		 	 
      	    		 	case 1:
      	    		 		layer2xMenuLight = 0;
      	    		 		layer2xMenu(0, layer2xMenuLight);
      	    		 		
      	    		 		//pwLight = 0;
      	    		 	 	//strcpy(passWord,"000000");
      	    		 	 	//inputPassWord(pwLight);
      	    		 	 	break;
      	    		 	 	 	 
      	    		 	case 2:
      	    		 	  layer2Menu(layer2MenuLight[layer1MenuLight], layer1MenuLight);
      	    		 	  break;
      	    		 	
      	    		 #ifdef LIGHTING
      	    		 	case 3:
	    		 	 	      queryMpLink = xlcLink;
	    		 	 	      if (queryMpLink==NULL)
	    		 	 	      {
                      guiLine(10,55,150,105,0);
                      guiLine(10,55,10,105,1);
                      guiLine(150,55,150,105,1);
                      guiLine(10,55,150,55,1);
                      guiLine(10,105,150,105,1);
                      guiDisplay(12,70,"未配置控制点参数!",1);
                      lcdRefresh(10,120);
	    		 	 	      }
	    		 	 	      else
	    		 	 	      {
	    		 	 	        xlcStatus();
	    		 	 	      }
      	    		 		
      	    		 		break;
      	    		 		
      	    		 	case 4:
	    		 	 	      queryMpLink = ldgmLink;
	    		 	 	      if (queryMpLink==NULL)
	    		 	 	      {
                      guiLine(10,55,150,105,0);
                      guiLine(10,55,10,105,1);
                      guiLine(150,55,150,105,1);
                      guiLine(10,55,150,55,1);
                      guiLine(10,105,150,105,1);
                      guiDisplay(12,70,"未配置控制点参数!",1);
                      lcdRefresh(10,120);
	    		 	 	      }
	    		 	 	      else
	    		 	 	      {
	    		 	 	        ldgmStatus();
	    		 	 	      }      	    		 		
      	    		 		break;
      	    		 		
      	    		 	case 5:
	    		 	 	      queryMpLink = lsLink;
	    		 	 	      if (queryMpLink==NULL)
	    		 	 	      {
                      guiLine(10,55,150,105,0);
                      guiLine(10,55,10,105,1);
                      guiLine(150,55,150,105,1);
                      guiLine(10,55,150,55,1);
                      guiLine(10,105,150,105,1);
                      
	                    guiDisplay(40, 60, "关联照度值", 1);
                      sprintf(str, "%ldLux", downLux[0] | downLux[1]<<8 | downLux[2]<<16);
	                    guiDisplay(50, 80, str, 1);
                      //guiDisplay(12,70,"未配置控制点参数!",1);
                      lcdRefresh(10,120);
	    		 	 	      }
	    		 	 	      else
	    		 	 	      {
	    		 	 	        lsStatus();
	    		 	 	      }      	    		 		
      	    		 		break;
      	    		 #endif
      	    		}
      	    		break;
      	    		 	 
      	    	case 2:      //菜单第2层
      	    		switch(layer1MenuLight)
      	    		{
      	    		 	 case 0:  //抄表查询
      	    		 	 	#ifdef LIGHTING
      	    		 	 	
      	    		 	 	#else 
      	    		 	 	 
      	    		 	 	 //如果是1-1(选择查询日期)的确认则要判断日期是否正确
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]==0)
      	    		 	 	 {
      	    		 	 	 	 //判断输入的日期是否正确
      	    		 	 	 	 if (checkInputTime()==FALSE)
      	    		 	 	 	 {
      	    		 	 	 	   return;
      	    		 	 	 	 }
      	    		 	 	 }
      	    		 	 	 
      	    		 	 	 //按重庆显示规范要求,按确定键可实现不同分类的屏显间的切换
      	    		 	 	 if (layer2MenuLight[layer1MenuLight]>=layer2MenuNum[layer1MenuLight]-1)
      	    		 	 	 {
      	    		 	 	   layer2MenuLight[layer1MenuLight]=0;
      	    		 	 	 }
      	    		 	 	 else
      	    		 	 	 {
      	    		 	 	   layer2MenuLight[layer1MenuLight]++;
      	    		 	 	 }
                     
                     if (layer2MenuLight[layer1MenuLight]!=0)
                     {
                       layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = meterDeviceNum/(NUM_MP_PER_PAGE-1);
                       if ((meterDeviceNum%(NUM_MP_PER_PAGE-1))!=0)
                       {
                     	   layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
                       }
                       layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                     }

      	    		 	 	 copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 
      	    		 	 	#endif
      	    		 	 	 break;
      	    		 	 	  	
      	    		 	case 1:    //参数设置与查看
      	    		 	 	switch(layer2MenuLight[1])
      	    		 	 	{
      	    		 	 	  case 0:  //通讯通道设置
                        strcpy(commParaItem[0],digitalToChar(addrField.a1[1]>>4));
                        strcat(commParaItem[0],digitalToChar(addrField.a1[1]&0xf));
                        strcat(commParaItem[0],digitalToChar(addrField.a1[0]>>4));
                        strcat(commParaItem[0],digitalToChar(addrField.a1[0]&0xf));
                       
                        for(i=0;i<4;i++)
                        {
                          if (strlen((char *)teApn[i])==0)
                          {
                            strcpy((char *)teApn[i],(char *)ipAndPort.apn);
                          }
                        }
                        strcpy(commParaItem[1],(char *)ipAndPort.apn);
                          
                        intToString(ipAndPort.port[1]<<8 | ipAndPort.port[0],3,strX);
                       	strcpy(commParaItem[2],intToIpadd(ipAndPort.ipAddr[0]<<24 | ipAndPort.ipAddr[1]<<16 | ipAndPort.ipAddr[2]<<8 | ipAndPort.ipAddr[3],str));
                        strcat(commParaItem[2],":");
                          
                        for(i=0;i<5-strlen(strX);i++)
                        {
                          strcat(commParaItem[2],"0");
                        }
                        strcat(commParaItem[2],strX);
                        intToString(ipAndPort.portBak[1]<<8 | ipAndPort.portBak[0],3,strX);
                       	strcpy(commParaItem[3],intToIpadd(ipAndPort.ipAddrBak[0]<<24 | ipAndPort.ipAddrBak[1]<<16 | ipAndPort.ipAddrBak[2]<<8 | ipAndPort.ipAddrBak[3],str));
                        strcat(commParaItem[3],":");
                          
                        for(i=0;i<5-strlen(strX);i++)
                        {
                          strcat(commParaItem[3],"0");
                        }
                        strcat(commParaItem[3],strX);
                          
                        keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	  	break;
      	    		 	 	  	
      	    		 	 	  case 1:  //台区电表参数设置
                       #ifndef LIGHTING 
                        strcpy(chrMp[0],"0001");
                        strcpy(chrMp[1],"0");
                        strcpy(chrMp[2],"1");
                        strcpy(chrMp[3],"0");
                        strcpy(chrMp[4],"000000000001");
                        keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  set485Meter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	 	 #endif
      	    		 	 	  	break;
      	    		 	 	  	
      	    		 	 	  case 2:  //集抄电表参数设置
                        strcpy(chrMp[0],"0001");
                        strcpy(chrMp[1],"000000000001");
                        strcpy(chrMp[2],"1");
                        strcpy(chrMp[3],"000000000000");
                        strcpy(chrMp[4],"0");
                        keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  setCarrierMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);      	    		 	 	  	
      	    		 	 	  	break;
      	    		 	 	  	
      	    		 	 	 	case 3:  //终端时间设置
      	    		 	 	 		keyLeftRight = 0;
      	    		 	 	 		strcpy(dateTimeItem,"20");
                        digital2ToString(sysTime.year,str);
                        dateTimeItem[2] = str[0];
                        dateTimeItem[3] = str[1];
                        digital2ToString(sysTime.month,str);
                        dateTimeItem[4] = str[0];
                        dateTimeItem[5] = str[1];
                        digital2ToString(sysTime.day,str);
                        dateTimeItem[6] = str[0];
                        dateTimeItem[7] = str[1];
                        digital2ToString(sysTime.hour,str);
                        dateTimeItem[8] = str[0];
                        dateTimeItem[9] = str[1];
                        digital2ToString(sysTime.minute,str);
                        dateTimeItem[10] = str[0];
                        dateTimeItem[11] = str[1];
                        digital2ToString(sysTime.second,str);
                        dateTimeItem[12] = str[0];
                        dateTimeItem[13] = str[1];
                        dateTimeItem[14] = '\0';
      	    		 	 	 		setTeDateTime(keyLeftRight);
      	    		 	 	 		break;
      	    		 	 	 		
      	    		 	 	 	case 4:  //修改界面密码
      	    		 	 	 	  keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                        strcpy(commParaItem[0],"000000");
                        strcpy(commParaItem[1],"000000");
      	    		 	 	 	  modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	 	  break;
      	    		 	 	  	     
      	    	 	 	  	case 5:  //终端编号
      	    	 	 	  		keyLeftRight = 0;
    	  	 	 	  	 	 	 	#ifdef TE_ADDR_USE_BCD_CODE
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a2[1]>>4);
                         tmpTeAddr[0] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a2[1]&0xf);
                         tmpTeAddr[1] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a2[0]>>4);
                         tmpTeAddr[2] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a2[0]&0xf);
                         tmpTeAddr[3] = *tmpChar;          	  	 	 	  	 	 	 	 
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a1[1]>>4);
                         tmpTeAddr[4] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a1[1]&0xf);
                         tmpTeAddr[5] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a1[0]>>4);
                         tmpTeAddr[6] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a1[0]&0xf);
                         tmpTeAddr[7] = *tmpChar;
                         tmpTeAddr[8] = 0x0;
    	  	 	 	  	 	 	 	 modifyTeAddr(0);
    	  	 	 	  	 	 	 	#else
    	  	 	 	  	 	 	 	 tmpAddr = addrField.a2[1]<<8 | addrField.a2[0];
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(tmpAddr/10000);
    	  	 	 	  	 	 	 	 tmpTeAddr[0] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpAddr %= 10000;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(tmpAddr/1000);
    	  	 	 	  	 	 	 	 tmpTeAddr[1] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpAddr %= 1000;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(tmpAddr/100);
    	  	 	 	  	 	 	 	 tmpTeAddr[2] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpAddr %= 100;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(tmpAddr/10);
    	  	 	 	  	 	 	 	 tmpTeAddr[3] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpAddr %= 10;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(tmpAddr);
    	  	 	 	  	 	 	 	 tmpTeAddr[4] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a1[1]>>4);
                         tmpTeAddr[5] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a1[1]&0xf);
                         tmpTeAddr[6] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a1[0]>>4);
                         tmpTeAddr[7] = *tmpChar;
    	  	 	 	  	 	 	 	 tmpChar = digitalToChar(addrField.a1[0]&0xf);
                         tmpTeAddr[8] = *tmpChar;
                         tmpTeAddr[9] = 0x0;
    	  	 	 	  	 	 	 	#endif
    	  	 	 	  	 	 	 	
      	    	 	 	  		setTeAddr(keyLeftRight);
      	    	 	 	  		break;
      	    	 	 	  		
      	    	 	 	  	case 6:  //以太网参数设置
                        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
                        {
                          ifc.ifc_len = sizeof buf;
                          ifc.ifc_buf = (caddr_t) buf;
                          if (!ioctl(fd, SIOCGIFCONF, (char *) &ifc))
                          {
                            interface = ifc.ifc_len / sizeof(struct ifreq);
                            while (interface-- > 0)
                            {
                              if (strstr(buf[interface].ifr_name,"eth0"))
                              {
                               /*Get HW ADDRESS of the net card */
                                if (!(ioctl(fd, SIOCGIFHWADDR, (char *) &buf[interface]))) 
                                {
                                    sprintf(tmpEthMac, "%02x%02x%02x%02x%02x%02x",
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[0],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[1],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[2],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[3],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[4],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[5]); // 利用sprintf转换成char *
                                }
                              }
                            }//end of while
                          }
                          else
                          {
                            perror("cpm: ioctl");
                          }
                        }
                        else
                        {
                          perror("cpm: socket");
                        }
                     
                        close(fd);
                       	
                       	strcpy(chrEthPara[0],intToIpadd(teIpAndPort.teIpAddr[0]<<24 | teIpAndPort.teIpAddr[1]<<16 | teIpAndPort.teIpAddr[2]<<8 | teIpAndPort.teIpAddr[3],str));
                       	strcpy(chrEthPara[1],intToIpadd(teIpAndPort.mask[0]<<24 | teIpAndPort.mask[1]<<16 | teIpAndPort.mask[2]<<8 | teIpAndPort.mask[3],str));
                       	strcpy(chrEthPara[2],intToIpadd(teIpAndPort.gateWay[0]<<24 | teIpAndPort.gateWay[1]<<16 | teIpAndPort.gateWay[2]<<8 | teIpAndPort.gateWay[3],str));
                        chrEthPara[3][0] = teIpAndPort.ethIfLoginMs;
                        keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    	 	 	  		setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		break;
      	    	 	 	  		
                      case 7:  //虚拟专网用户名密码
                        strcpy(tmpVpnUserName, (char *)vpn.vpnName);                        
                        tmpVpnUserName[32] = '\0';
                        trim(tmpVpnUserName);
                        
     	 	 	  		 	 	    for(i=strlen(tmpVpnUserName); i<32; i++)
     	 	 	  		 	 	    {
     	 	 	  		 	 	    	tmpVpnUserName[i] = ' ';
     	 	 	  		 	 	    }
     	 	 	  		 	 	    tmpVpnUserName[32] = '\0';

                        strcpy(tmpVpnPw, (char *)vpn.vpnPassword);
                        tmpVpnPw[32] = '\0';
                        trim(tmpVpnPw);
     	 	 	  		 	 	    for(i=strlen(tmpVpnPw); i<32; i++)
     	 	 	  		 	 	    {
     	 	 	  		 	 	    	tmpVpnPw[i] = ' ';
     	 	 	  		 	 	    }
     	 	 	  		 	 	    tmpVpnPw[32] = '\0';
     	 	 	  		 	 	    keyLeftRight = 0;
                        inputStatus = STATUS_NONE;
                        setVpn(keyLeftRight);
                        break;
                        
                      case 8:    //终端级联参数设置
                        sprintf(chrMp[0], "%01d", cascadeCommPara.commPort);    //级联端口
                        if (cascadeCommPara.flagAndTeNumber&0x80)
                        {
                          strcpy(chrMp[1],"2");    //被级联
                        }
                        else
                        {
                          strcpy(chrMp[1],"1");    //级联方
                        }
                        switch (cascadeCommPara.flagAndTeNumber&0xf)
                        {
                        	case 1:
                           sprintf(chrMp[2],"%02x%02x%05d",cascadeCommPara.divisionCode[1],cascadeCommPara.divisionCode[0],cascadeCommPara.cascadeTeAddr[0]|cascadeCommPara.cascadeTeAddr[1]<<8);      //终端1
                           strcpy(chrMp[3],"000000000");
                           strcpy(chrMp[4],"000000000");
                           break;
                        	case 2:
                           sprintf(chrMp[2],"%02x%02x%05d",cascadeCommPara.divisionCode[1],cascadeCommPara.divisionCode[0],cascadeCommPara.cascadeTeAddr[0]|cascadeCommPara.cascadeTeAddr[1]<<8);      //终端1
                           sprintf(chrMp[3],"%02x%02x%05d",cascadeCommPara.divisionCode[3],cascadeCommPara.divisionCode[2],cascadeCommPara.cascadeTeAddr[2]|cascadeCommPara.cascadeTeAddr[3]<<8);      //终端1
                           strcpy(chrMp[4],"000000000");
                           break;
                        	case 3:
                           sprintf(chrMp[2],"%02x%02x%05d",cascadeCommPara.divisionCode[1],cascadeCommPara.divisionCode[0],cascadeCommPara.cascadeTeAddr[0]|cascadeCommPara.cascadeTeAddr[1]<<8);      //终端1
                           sprintf(chrMp[3],"%02x%02x%05d",cascadeCommPara.divisionCode[3],cascadeCommPara.divisionCode[2],cascadeCommPara.cascadeTeAddr[2]|cascadeCommPara.cascadeTeAddr[3]<<8);      //终端1
                           sprintf(chrMp[4],"%02x%02x%05d",cascadeCommPara.divisionCode[5],cascadeCommPara.divisionCode[4],cascadeCommPara.cascadeTeAddr[4]|cascadeCommPara.cascadeTeAddr[5]<<8);      //终端1
                           break;
                           
                        	default:
                           strcpy(chrMp[2],"000000000");
                           strcpy(chrMp[3],"000000000");
                           strcpy(chrMp[4],"000000000");
                           
                           strcpy(chrMp[1],"0");      //不级联
                           break;
                        }
                        
                        keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
                      	break;

      	    		 	 	  case 9:    //锐拔模块参数设置
                        sprintf(chrRlPara[0],"%02d",rlPara[0]);
                        sprintf(chrRlPara[1],"%02d",rlPara[1]);
                        sprintf(chrRlPara[2],"%02d",rlPara[2]);
                        sprintf(chrRlPara[3],"%02d",rlPara[3]);
                        keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  setRlPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	  	break;
      	    		 	 	}
      	    		 	 	break;

      	    		 	case 2:    //终端管理与维护
      	    		 	 	switch(layer2MenuLight[layer1MenuLight])   //2层菜单高亮
      	    		 	 	{
      	    		 	 	 	  case 0:  //实时抄表
      	    		 	 	 	   #ifdef LIGHTING    //路灯集中器是指定测量点抄表
    	    	 	 	  		 	  if (carrierModuleType==NO_CARRIER_MODULE || ((carrierModuleType==RL_WIRELESS || carrierModuleType==SR_WIRELESS)  && carrierFlagSet.wlNetOk<3))
      	    		 	 	 	    {
                            guiLine(10,55,150,105,0);
                            guiLine(10,55,10,105,1);
                            guiLine(150,55,150,105,1);
                            guiLine(10,55,150,55,1);
                            guiLine(10,105,150,105,1);
                            if (carrierModuleType==NO_CARRIER_MODULE)
                            {
                              guiDisplay(15,60,"  等待识别模块!",1);
                            }
                            else
                            {
                              guiDisplay(15,60,"等待模块组网完成!",1);
                            }
                            guiDisplay(15,80,"     请稍候   ",1);
                            lcdRefresh(10,120);
      	    		 	 	 	    }
      	    		 	 	 	    else
      	    		 	 	 	    {
      	    	 	 	  		 	  if (0<carrierFlagSet.broadCast)
      	    	 	 	  		 	  {
                              guiLine(10,55,150,105,0);
                              guiLine(10,55,10,105,1);
                              guiLine(150,55,150,105,1);
                              guiLine(10,55,150,55,1);
                              guiLine(10,105,150,105,1);
                              guiDisplay(15,60,"广播命令发送中!",1);
                              guiDisplay(15,80,"     请稍候   ",1);
                              lcdRefresh(10,120);
      	    	 	 	  		 	  }
      	    	 	 	  		 	  else
      	    	 	 	  		 	  {
      	    	 	 	  		 	    keyLeftRight = 0;
      	    	 	 	  		 	    strcpy(singleCopyTime,"-----");
             	 	              strcpy(singleCopyEnergy,"-----");
             	 	            
      	    	 	 	  		 	    singleCcbCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
      	    	 	 	  		 	  }
      	    	 	 	  		 	}
      	    	 	 	  		 	
      	    		 	 	 	   #else
      	    		 	 	 	   	
      	    		 	 	 	   	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  	realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	 	   
      	    		 	 	 	   #endif
      	    		 	 	 	  	break;
      	    		 	 	 	  	
      	    		 	 	 	  case 1: //全部测量点抄表结果
      	    		 	 	 	   #ifdef LIGHTING    //路灯集中器本项是本机信息
      	    		 	 	 	  	
      	    		 	 	 	  	layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 2;
      	    		 	 	 	  	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    	 	 	  			terminalInfo(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    	 	 	  			
      	    		 	 	 	   #else
      	    		 	 	 	  	
      	    		 	 	 	  	allMpCopy(0xff);
      	    		 	 	 	   
      	    		 	 	 	   #endif
      	    		 	 	 	  	break;
      	    		 	 	 	  	
      	    		 	 	 	  case 2:  //表号搜索
      	    		 	 	 	   #ifdef LIGHTING    //路灯集中器本选项是调节液晶比对度
      	    		 	 	 	    
      	    		 	 	 	    setLcdDegree(lcdDegree);
      	    		 	 	 	     
      	    		 	 	 	   #else
      	    		 	 	 	   
      	    		 	 	 	  	if (carrierModuleType==SR_WIRELESS || carrierModuleType==HWWD_WIRELESS || carrierModuleType==RL_WIRELESS || carrierModuleType==NO_CARRIER_MODULE)
      	    		 	 	 	  	{
                             guiLine(10,55,150,105,0);
                             guiLine(10,55,10,105,1);
                             guiLine(150,55,150,105,1);
                             guiLine(10,55,150,55,1);
                             guiLine(10,105,150,105,1);
                             if (carrierModuleType==NO_CARRIER_MODULE)
                             {
                               guiDisplay(15,60,"未识本地通信模块!",1);
                               guiDisplay(15,80,"     无法搜表!   ",1);
                             }
                             else
                             {
                               guiDisplay(35,60,"无线自组网!",1);
                               guiDisplay(35,80," 无需搜表! ",1);
                             }
                             lcdRefresh(10,120);
      	    		 	 	 	  	}
      	    		 	 	 	  	else
      	    		 	 	 	  	{
        	    		 	 	 	  	menuInLayer = 3;
        	    		 	 	 	  	if (carrierFlagSet.searchMeter==0)
        	    		 	 	 	  	{
        	    		 	 	 	  	  keyLeftRight = 1;
        	    		 	 	 	  	  searchMeter(keyLeftRight);
        	    		 	 	 	  	}
        	    		 	 	 	  	else
        	    		 	 	 	  	{
        	    		 	 	 	  	  keyLeftRight = 0xff;
        	    		 	 	 	  	  searchMeter(keyLeftRight);
        	    		 	 	 	  	}
        	    		 	 	 	  }
        	    		 	 	 	  
        	    		 	 	 	 #endif
      	    		 	 	 	  	break;
      	    		 	 	 	  	
      	    		 	 	 	  case 3:  //新增电能表地址
      	    	 	 	  		 #ifdef LIGHTING    //路灯集中器该项是"复位集中器"
      	    	 	 	  		  
      	    	 	 	  		  reboot(RB_AUTOBOOT);

      	    	 	 	  		 #else
      	    	 	 	  		 	keyLeftRight = 1;
      	    	 	 	  		 	multiCpUpDown = 0;
      	    	 	 	  		 	newAddMeter(keyLeftRight);
      	    	 	 	  		 #endif
      	    	 	 	  		 	break;
      	    	 	 	  		 	
      	    	 	 	  		case 4:  //新增电能表抄表状态
      	    	 	 	  		 #ifdef LIGHTING    //路灯集中器该项是“软件升级”
      	    	 	 	  		  
      	    	 	 	  		  uDiskUpgrade();

      	    	 	 	  		 #else
      	    	 	 	  		 
      	    	 	 	  		 	menuInLayer++;

      	    	 	 	  			multiCpUpDown = 0;
      	    	 	 	  			newMeterCpStatus(0);
      	    	 	 	  			
      	    	 	 	  		 #endif
      	    	 	 	  			break;
      	    	 	 	  		 	
      	    	 	 	  		case 5:  //本机信息
        								 #ifdef LIGHTING
												 	keyLeftRight = 0;
													setLearnIr(keyLeftRight);
												 #else
													if (ifHasAcModule==TRUE)
                          {
      	    		 	 	 	  	  layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 18;
      	    		 	 	 	  	}
      	    		 	 	 	  	else
      	    		 	 	 	  	{
      	    		 	 	 	  	  layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 4;
      	    		 	 	 	  	}
      	    		 	 	 	  	
      	    		 	 	 	  	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    	 	 	  			terminalInfo(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
												 #endif
      	    	 	 	  			break;
      	    	 	 	  			
      	    	 	 	  	  case 6:  //调节LCD对比度
      	    	 	 	  	  	setLcdDegree(lcdDegree);
      	    	 	 	  	  	break;
      	    	 	 	  	  
      	    	 	 	  	  case 7:  //本地通信模块抄读方式
                      	  if (carrierModuleType==EAST_SOFT_CARRIER
                      	   	   || carrierModuleType==TC_CARRIER
                      	   	     || carrierModuleType==MIA_CARRIER
                      	   	 )
                      	  {
        	    	 	 	  	  	if (localCopyForm==0xaa)
        	    	 	 	  	  	{
        	    	 	 	  	  	  keyLeftRight = 1;
        	    	 	 	  	  	}
        	    	 	 	  	  	else
        	    	 	 	  	  	{
        	    	 	 	  	  		keyLeftRight = 0;
        	    	 	 	  	  	}
        	    	 	 	  	  	setCopyForm(keyLeftRight);
        	    	 	 	  	  }
        	    	 	 	  	  else
        	    	 	 	  	  {
                            guiLine(1,55,150,105,0);
                            guiLine(1,55,1,105,1);
                            guiLine(160,55,160,105,1);
                            guiLine(1,55,160,55,1);
                            guiLine(1,105,160,105,1);
                            
                            if (carrierModuleType==NO_CARRIER_MODULE)
	   	                      {
	   	                      	guiDisplay(1, 70, "未检测到本地通信模块", 1);
	   	                      }
	   	                      else
	   	                      {
                              guiDisplay(17, 60, "本类型本地通信模块",1);
                              guiDisplay(1, 80, "无需设置抄读方式!",1);
                            }
                            
                            lcdRefresh(55,110);
                          }
      	    	 	 	  	  	break;
      	    	 	 	  			
      	    	 	 	  		case 8:     //复位终端
      	    	 	 	  			reboot(RB_AUTOBOOT);
      	    	 	 	  			break;
      	    	 	 	  	  
      	    	 	 	  	  case 9:     //终端程序升级
      	    	 	 	  	  	uDiskUpgrade();
      	    	 	 	  	  	break;
      	    	 	 	  	  
      	    	 	 	  	  case 10:    //升级路由程序
      	    	 	 	  	  	upRtFlag = 1;
      	    	 	 	  	  	break;
      	    	 	 	  	  
      	    	 	 	  	  case 11:    //居民用户表数据类型
        	    	 	 	  	  switch (denizenDataType)
        	    	 	 	  	  {
        	    	 	 	  	  	case 0x55:
        	    	 	 	  	  		keyLeftRight = 1;
        	    	 	 	  	  	  break;
        	    	 	 	  	  	  
        	    	 	 	  	  	case 0xaa:
        	    	 	 	  	  	  keyLeftRight = 2;
        	    	 	 	  	  	  break;
        	    	 	 	  	  
        	    	 	 	  	  	default:
        	    	 	 	  	  		keyLeftRight = 0;
        	    	 	 	  	  		break;
        	    	 	 	  	  }
                          setDenizenDataType(keyLeftRight);
      	    	 	 	  	  	break;
      	    	 	 	  	  
      	    	 	 	  	  case 12:    //轮显设置
      	    	 	 	  	  	if (cycleDataType==0x55)
      	    	 	 	  	  	{
      	    	 	 	  	  	  keyLeftRight = 1;
      	    	 	 	  	  	}
      	    	 	 	  	  	else
      	    	 	 	  	  	{
      	    	 	 	  	  	  keyLeftRight = 0;
      	    	 	 	  	  	}
      	    	 	 	  	  	setCycleType(keyLeftRight);
      	    	 	 	  	  	break;
      	    	 	 	  	  
      	    	 	 	  	  case 13:    //维护接口模式设置
      	    	 	 	  	  	if (mainTainPortMode==0x55)
      	    	 	 	  	  	{
      	    	 	 	  	  	  keyLeftRight = 1;
      	    	 	 	  	  	}
      	    	 	 	  	  	else
      	    	 	 	  	  	{
      	    	 	 	  	  	  keyLeftRight = 0;
      	    	 	 	  	  	}
      	    	 	 	  	  	setMainTain(keyLeftRight);
      	    	 	 	  	  	break;

      	    	 	 	  	  case 14:    //第2路485口功能设置
      	    	 	 	  	  	if (rs485Port2Fun==0x55)
      	    	 	 	  	  	{
      	    	 	 	  	  	  keyLeftRight = 1;
      	    	 	 	  	  	}
      	    	 	 	  	  	else
      	    	 	 	  	  	{
      	    	 	 	  	  	  keyLeftRight = 0;
      	    	 	 	  	  	}
      	    	 	 	  	  	setRs485Port2(keyLeftRight);
      	    	 	 	  	  	break;
      	    	 	 	  	  
      	    	 	 	  	  case 15:    //本地通信模块协议设置
      	    	 	 	  	  	if (lmProtocol==0x55)
      	    	 	 	  	  	{
      	    	 	 	  	  	  keyLeftRight = 1;
      	    	 	 	  	  	}
      	    	 	 	  	  	else
      	    	 	 	  	  	{
      	    	 	 	  	  	  keyLeftRight = 0;
      	    	 	 	  	  	}
      	    	 	 	  	  	setLmProtocol(keyLeftRight);
      	    	 	 	  	  	break;
      	    		 	 	}
      	    		 	 	break;
										
									case 3:    //线路控制点状态
									  keyLeftRight=0;
										xlOpenClose(keyLeftRight);
									  break;
      	    		}
      	    		break;
      	    		
      	    	case 3:   //菜单第3层
      	    	 	switch(layer1MenuLight)
      	    	 	{
      	    	 	 	case 1:    //参数设置与查看
      	    	 	 	  switch(layer2MenuLight[layer1MenuLight])
      	    	 	 	  {
      	    	 	 	  	case 0:  //通信通道设置
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==4)
      	    		 	 	    {
      	    		 	 	       //确认修改参数
      	    		 	 	       keyLeftRight = 0xff;
      	    		 	 	       commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	       layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0xff;
      	    		 	 	          
      	    		 	 	       return;
      	    		 	 	    }

      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] == 0xff)
      	    		 	 	    {
        	  	 	 	      	//主IP地址
        	  	 	 	      	ipAndPort.ipAddr[0] =(commParaItem[2][0]-0x30)*100+(commParaItem[2][1]-0x30)*10+(commParaItem[2][2]-0x30);
        	  	 	 	      	ipAndPort.ipAddr[1] =(commParaItem[2][4]-0x30)*100+(commParaItem[2][5]-0x30)*10+(commParaItem[2][6]-0x30);
        	  	 	 	      	ipAndPort.ipAddr[2] =(commParaItem[2][8]-0x30)*100+(commParaItem[2][9]-0x30)*10+(commParaItem[2][10]-0x30);
        	  	 	 	      	ipAndPort.ipAddr[3] =(commParaItem[2][12]-0x30)*100+(commParaItem[2][13]-0x30)*10+(commParaItem[2][14]-0x30);
        	  	 	 	      	  	
        	  	 	 	      	//主端口
        	  	 	 	      	tmpData = (commParaItem[2][16]-0x30)*10000+(commParaItem[2][17]-0x30)*1000
        	  	 	 	      	         +(commParaItem[2][18]-0x30)*100+(commParaItem[2][19]-0x30)*10
        	  	 	 	      	         +(commParaItem[2][20]-0x30);
        	  	 	 	      	ipAndPort.port[1] = tmpData>>8;
        	  	 	 	      	ipAndPort.port[0] = tmpData&0xff;

        	  	 	 	      	//备用地址
        	  	 	 	      	ipAndPort.ipAddrBak[0] =(commParaItem[3][0]-0x30)*100+(commParaItem[3][1]-0x30)*10+(commParaItem[3][2]-0x30);
        	  	 	 	      	ipAndPort.ipAddrBak[1] =(commParaItem[3][4]-0x30)*100+(commParaItem[3][5]-0x30)*10+(commParaItem[3][6]-0x30);
        	  	 	 	      	ipAndPort.ipAddrBak[2] =(commParaItem[3][8]-0x30)*100+(commParaItem[3][9]-0x30)*10+(commParaItem[3][10]-0x30);
        	  	 	 	      	ipAndPort.ipAddrBak[3] =(commParaItem[3][12]-0x30)*100+(commParaItem[3][13]-0x30)*10+(commParaItem[3][14]-0x30);
        	  	 	 	      	  	
        	  	 	 	      	//备用端口
        	  	 	 	      	tmpData = (commParaItem[3][16]-0x30)*10000+(commParaItem[3][17]-0x30)*1000
        	  	 	 	      	         +(commParaItem[3][18]-0x30)*100+(commParaItem[3][19]-0x30)*10
        	  	 	 	      	         +(commParaItem[3][20]-0x30);
        	  	 	 	      	ipAndPort.portBak[1] = tmpData>>8;
        	  	 	 	      	ipAndPort.portBak[0] = tmpData&0xff;
        	  	 	 	      	  	        	  	 	 	      	  	
        	  	 	 	      	strcpy((char *)ipAndPort.apn, commParaItem[1]);

        	  	 	 	      	//保存IP地址
        	  	 	 	      	saveParameter(0x04, 3,(INT8U *)&ipAndPort,sizeof(IP_AND_PORT));
                        	
                        	saveBakKeyPara(3);    //2012-8-9,add

                 	  	 	 	addrField.a1[1] = (commParaItem[0][0]-0x30)<<4 | (commParaItem[0][1]-0x30);
                 	  	 	 	addrField.a1[0] = (commParaItem[0][2]-0x30)<<4 | (commParaItem[0][3]-0x30);
                              
                          //保存行政区划码
                          saveParameter(0x04, 121,(INT8U *)&addrField,4);
                        	saveBakKeyPara(121);    //2012-8-9,add
                              
                          guiLine(10,55,150,105,0);
                          guiLine(10,55,10,105,1);
                          guiLine(150,55,150,105,1);
                          guiLine(10,55,150,55,1);
                          guiLine(10,105,150,105,1);
                          guiDisplay(12,70,"修改通信参数成功!",1);
                          lcdRefresh(10,120);
                              
                          menuInLayer--;
                          layer2MenuLight[layer1MenuLight]=0xff;
                          
                          //2012-08-02,add,修改IP后立即用新IP登录
                          if (bakModuleType==MODEM_PPP)
                          {
                          	resetPppWlStatus();
                          }
                          else
                          {
                            wlModemPowerOnOff(0);
                          }

                          return;
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
      	    		 	 	    {
      	    		 	 	    	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	    }
      	    		 	 	    else
      	    		 	 	    {
      	    		 	 	      layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	    }

      	    		 	 	    keyLeftRight = 0;
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
      	    		 	 	    {
      	    		 	 	      keyLeftRight = 0x88;
      	    		 	 	      for(i=0;i<4;i++)
      	    		 	 	      {
      	    		 	 	       	if (strcmp((char *)teApn[i],commParaItem[1])==0)
      	    		 	 	       	{
      	    		 	 	       	  keyLeftRight = i;
      	    		 	 	       	  break;
      	    		 	 	       	}
      	    		 	 	      }
      	    		 	 	    }
      	    		 	 	    commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		break;
      	    	 	 	  		
      	    	 	 	  	case 1:  //台区电表参数设置
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==5)
      	    		 	 	    {
        	  	 	 	      	//测量点号
        	  	 	 	      	meterConfig.measurePoint = (chrMp[0][0]-0x30)*1000+(chrMp[0][1]-0x30)*100+(chrMp[0][2]-0x30)*10+(chrMp[0][3]-0x30);
        	  	 	 	      	
        	  	 	 	      	//序号
        	  	 	 	      	meterConfig.number = meterConfig.measurePoint;

        	  	 	 	      	//端口和速率
        	  	 	 	      	meterConfig.rateAndPort = (chrMp[1][0]-0x30)<<5;
        	  	 	 	      	switch(chrMp[2][0])
        	  	 	 	      	{
        	  	 	 	      		case 0x30:   //交采端口为1
        	  	 	 	      		 	meterConfig.rateAndPort |= 0x1;
        	  	 	 	      		 	break;

        	  	 	 	      		case 0x31:   //RS485-1端口为2
        	  	 	 	      		 	meterConfig.rateAndPort |= 0x2;
        	  	 	 	      		 	break;

        	  	 	 	      		case 0x32:   //RS485-2端口为3
        	  	 	 	      		 	meterConfig.rateAndPort |= 0x3;
        	  	 	 	      		 	break;
        	  	 	 	      	}
        	  	 	 	      	
        	  	 	 	      	//协议
        	  	 	 	      	switch(chrMp[3][0])
        	  	 	 	        {
        	  	 	 	        	 case 0x32:
        	  	 	 	        	 	 meterConfig.protocol = 02;
        	  	 	 	        	 	 break;

        	  	 	 	        	 case 0x31:
        	  	 	 	        	 	 meterConfig.protocol = 30;
        	  	 	 	        	 	 break;
        	  	 	 	        	 	 
        	  	 	 	        	 default:
        	  	 	 	        	 	 meterConfig.protocol = 1;
        	  	 	 	        	 	 break;
        	  	 	 	        }
        	  	 	 	        
     	  	 	 	      	  	//电表地址
     	  	 	 	      	  	meterConfig.addr[5] = (chrMp[4][0]-0x30)<<4 | (chrMp[4][1]-0x30);
     	  	 	 	      	  	meterConfig.addr[4] = (chrMp[4][2]-0x30)<<4 | (chrMp[4][3]-0x30);
     	  	 	 	      	  	meterConfig.addr[3] = (chrMp[4][4]-0x30)<<4 | (chrMp[4][5]-0x30);
     	  	 	 	      	  	meterConfig.addr[2] = (chrMp[4][6]-0x30)<<4 | (chrMp[4][7]-0x30);
     	  	 	 	      	  	meterConfig.addr[1] = (chrMp[4][8]-0x30)<<4 | (chrMp[4][9]-0x30);
     	  	 	 	      	  	meterConfig.addr[0] = (chrMp[4][10]-0x30)<<4 | (chrMp[4][11]-0x30);
     	  	 	 	      	  	
     	  	 	 	      	  	//采集器地址
     	  	 	 	      	  	for(i=0;i<6;i++)
     	  	 	 	      	    {
     	  	 	 	      	  	  meterConfig.collectorAddr[i] = 0x0;
     	  	 	 	      	  	}
     	  	 	 	      	  	
     	  	 	 	      	  	//整数位及小数位个数
     	  	 	 	      	  	meterConfig.mixed = 0x05;
     	  	 	 	      	  	
     	  	 	 	      	  	//费率个数
     	  	 	 	      	  	meterConfig.numOfTariff = 4;
     	  	 	 	      	  	
     	  	 	 	      	  	//大类号及小类号
     	  	 	 	      	  	meterConfig.bigAndLittleType = 0x0;
                              
                          //保存
  		                    saveDataF10(meterConfig.measurePoint, meterConfig.rateAndPort&0x1f, meterConfig.addr, meterConfig.number, (INT8U *)&meterConfig, 27);
                              
                          guiLine(10,55,150,105,0);
                          guiLine(10,55,10,105,1);
                          guiLine(150,55,150,105,1);
                          guiLine(10,55,150,55,1);
                          guiLine(10,105,150,105,1);
                          guiDisplay(12,70,"台区电表设置成功!",1);
                          lcdRefresh(10,120);
                              
                          menuInLayer--;
                          layer2MenuLight[layer1MenuLight]=0xff;

                          return;
      	    		 	 	    }
      	    		 	 	       
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
      	    		 	 	    {
      	    		 	 	    	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	    }
      	    		 	 	    else
      	    		 	 	    {
      	    		 	 	      layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    //切换测量点时读出已有的测量点信息
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
      	    		 	 	    {
      	    		 	 	    	 tmpData = (chrMp[0][0]-0x30)*1000 + (chrMp[0][1]-0x30)*100 + (chrMp[0][2]-0x30)*10 + (chrMp[0][3]-0x30);
      	    		 	 	    	 if (tmpData>64 || tmpData<1)
      	    		 	 	    	 {
                              guiLine(10,55,150,105,0);
                              guiLine(10,55,10,105,1);
                              guiLine(150,55,150,105,1);
                              guiLine(10,55,150,55,1);
                              guiLine(10,105,150,105,1);
                              guiDisplay(20,70,"测量点输入错误!",1);
                              lcdRefresh(10,120);
      	    		 	 	    	 	  
      	    		 	 	    	 	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
      	    		 	 	    	 	  return;
      	    		 	 	    	 }
                           
                           if (selectF10Data(tmpData, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
                           {
                           	  //速率
                           	  chrMp[1][0] = 0x30+(meterConfig.rateAndPort>>5);
                           	  
                           	  //端口
                           	  switch(meterConfig.rateAndPort&0x1f)
                           	  {
                           	  	 case 1:
                           	  	 	 chrMp[2][0] = 0x30;
                           	  	 	 break;
                           	  	 	 
                           	  	 case 2:
                           	  	 	 chrMp[2][0] = 0x31;
                           	  	 	 break;

                           	  	 case 3:
                           	  	 	 chrMp[2][0] = 0x32;
                           	  	 	 break;
                           	  }
                           	  
                           	  switch(meterConfig.protocol)
                           	  {
                           	  	 case 2:
                           	  	 	 chrMp[3][0] = 0x32;
                           	  	 	 break;
                           	  	 	 
                           	  	 case 30:
                           	  	 	 chrMp[3][0] = 0x31;
                           	  	 	 break;
                           	  	 	 
                           	  	 default:
                           	  	 	 chrMp[3][0] = 0x30;
                           	  	 	 break;
                           	  }
                           	  
                           	  //表地址
                  	 	 	   	  chrMp[4][0]  = (meterConfig.addr[5]>>4)+0x30;
                  	 	 	   	  chrMp[4][1]  = (meterConfig.addr[5]&0xf)+0x30;
                  	 	 	   	  chrMp[4][2]  = (meterConfig.addr[4]>>4)+0x30;
                  	 	 	   	  chrMp[4][3]  = (meterConfig.addr[4]&0xf)+0x30;
                  	 	 	   	  chrMp[4][4]  = (meterConfig.addr[3]>>4)+0x30;
                  	 	 	   	  chrMp[4][5]  = (meterConfig.addr[3]&0xf)+0x30;
                  	 	 	   	  chrMp[4][6]  = (meterConfig.addr[2]>>4)+0x30;
                  	 	 	   	  chrMp[4][7]  = (meterConfig.addr[2]&0xf)+0x30;
                  	 	 	   	  chrMp[4][8]  = (meterConfig.addr[1]>>4)+0x30;
                  	 	 	   	  chrMp[4][9]  = (meterConfig.addr[1]&0xf)+0x30;
                  	 	 	   	  chrMp[4][10] = (meterConfig.addr[0]>>4)+0x30;
                  	 	 	   	  chrMp[4][11] = (meterConfig.addr[0]&0xf)+0x30;
                  	 	 	   	  chrMp[4][12] = '\0';
                           }
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    keyLeftRight = 0;
      	    		 	 	    set485Meter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		break;
      	    	 	 	  		
      	    	 	 	  	case 2:  //集抄电表参数设置
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==5)
      	    		 	 	    {
        	  	 	 	      	//测量点号
        	  	 	 	      	meterConfig.measurePoint = (chrMp[0][0]-0x30)*1000+(chrMp[0][1]-0x30)*100+(chrMp[0][2]-0x30)*10+(chrMp[0][3]-0x30);
        	  	 	 	      	
        	  	 	 	      	//序号
        	  	 	 	      	meterConfig.number = meterConfig.measurePoint;

        	  	 	 	      	
        	  	 	 	      	//协议
        	  	 	 	       #ifdef LIGHTING
        	  	 	 	      	//端口和速率
        	  	 	 	      	switch(chrMp[3][0])
        	  	 	 	        {
        	  	 	 	        	case 0x31:
        	  	 	 	      	    meterConfig.rateAndPort = 0x02;
        	  	 	 	      	    break;

        	  	 	 	        	case 0x32:
        	  	 	 	      	    meterConfig.rateAndPort = 0x03;
        	  	 	 	      	    break;
        	  	 	 	      	    
        	  	 	 	        	default:
        	  	 	 	      	    meterConfig.rateAndPort = 31;
        	  	 	 	      	    break;
        	  	 	 	        }
        	  	 	 	       
        	  	 	 	        switch(chrMp[2][0])
        	  	 	 	        {
        	  	 	 	          default:
        	  	 	 	            meterConfig.protocol = 30;
        	  	 	 	            break;

        	  	 	 	          case 0x32:
        	  	 	 	            meterConfig.protocol = 0x2;
        	  	 	 	            break;
        	  	 	 	            
        	  	 	 	          case 0x33:
        	  	 	 	            meterConfig.protocol = 130;
        	  	 	 	            break;
        	  	 	 	        }
        	  	 	 	       
        	  	 	 	       #else	
        	  	 	 	      	//端口和速率
        	  	 	 	      	meterConfig.rateAndPort = 0x1f;

        	  	 	 	      	switch(chrMp[2][0])
        	  	 	 	        {
        	  	 	 	        	case 0x31:
        	  	 	 	        	  meterConfig.protocol = 30;
        	  	 	 	        	  break;
        	  	 	 	        	 	 
        	  	 	 	        	default:
        	  	 	 	        	  meterConfig.protocol = 1;
        	  	 	 	        	  break;
        	  	 	 	        }
        	  	 	 	       #endif
        	  	 	 	        
     	  	 	 	      	  	//电表地址
     	  	 	 	      	  	meterConfig.addr[5] = (chrMp[1][0]-0x30)<<4 | (chrMp[1][1]-0x30);
     	  	 	 	      	  	meterConfig.addr[4] = (chrMp[1][2]-0x30)<<4 | (chrMp[1][3]-0x30);
     	  	 	 	      	  	meterConfig.addr[3] = (chrMp[1][4]-0x30)<<4 | (chrMp[1][5]-0x30);
     	  	 	 	      	  	meterConfig.addr[2] = (chrMp[1][6]-0x30)<<4 | (chrMp[1][7]-0x30);
     	  	 	 	      	  	meterConfig.addr[1] = (chrMp[1][8]-0x30)<<4 | (chrMp[1][9]-0x30);
     	  	 	 	      	  	meterConfig.addr[0] = (chrMp[1][10]-0x30)<<4 | (chrMp[1][11]-0x30);
     	  	 	 	      	  	
     	  	 	 	      	   
     	  	 	 	      	   #ifdef LIGHTING
     	  	 	 	      	    
     	  	 	 	      	    //采集器地址
     	  	 	 	      	    meterConfig.collectorAddr[5] = 0x0;
     	  	 	 	      	    meterConfig.collectorAddr[4] = 0x0;
     	  	 	 	      	    meterConfig.collectorAddr[3] = 0x0;
     	  	 	 	      	    meterConfig.collectorAddr[2] = 0x0;
     	  	 	 	      	    meterConfig.collectorAddr[1] = 0x0;
     	  	 	 	      	    meterConfig.collectorAddr[0] = 0x0;

     	  	 	 	      	   #else

     	  	 	 	      	  	//采集器地址
     	  	 	 	      	  	meterConfig.collectorAddr[5] = (chrMp[3][0]-0x30)<<4 | (chrMp[3][1]-0x30);
     	  	 	 	      	  	meterConfig.collectorAddr[4] = (chrMp[3][2]-0x30)<<4 | (chrMp[3][3]-0x30);
     	  	 	 	      	  	meterConfig.collectorAddr[3] = (chrMp[3][4]-0x30)<<4 | (chrMp[3][5]-0x30);
     	  	 	 	      	  	meterConfig.collectorAddr[2] = (chrMp[3][6]-0x30)<<4 | (chrMp[3][7]-0x30);
     	  	 	 	      	  	meterConfig.collectorAddr[1] = (chrMp[3][8]-0x30)<<4 | (chrMp[3][9]-0x30);
     	  	 	 	      	  	meterConfig.collectorAddr[0] = (chrMp[3][10]-0x30)<<4 | (chrMp[3][11]-0x30);

     	  	 	 	      	  	
     	  	 	 	      	   #endif

     	  	 	 	      	  	//费率个数
     	  	 	 	      	  	meterConfig.numOfTariff = chrMp[4][0]-0x30;

     	  	 	 	      	  	//整数位及小数位个数
     	  	 	 	      	  	meterConfig.mixed = 0x05;
     	  	 	 	      	  	
     	  	 	 	      	  	//大类号及小类号
     	  	 	 	      	  	meterConfig.bigAndLittleType = 0x0;
                              
                          //保存
  		                    saveDataF10(meterConfig.measurePoint, meterConfig.rateAndPort&0x1f, meterConfig.addr, meterConfig.number, (INT8U *)&meterConfig, 27);
                          carrierFlagSet.synSlaveNode = 1;    //申请同步载波模块从节点信息

                          guiLine(10,55,150,105,0);
                          guiLine(10,55,10,105,1);
                          guiLine(150,55,150,105,1);
                          guiLine(10,55,150,55,1);
                          guiLine(10,105,150,105,1);
                         #ifdef LIGHTING
                          guiDisplay(12,70," 控制点设置成功!",1);
                         #else 
                          guiDisplay(12,70,"集抄电表设置成功!",1);
                         #endif
                          lcdRefresh(10,120);
                              
                          menuInLayer--;
                          layer2MenuLight[layer1MenuLight]=0xff;

                          return;
      	    		 	 	    }
      	    		 	 	       
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
      	    		 	 	    {
      	    		 	 	    	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	    }
      	    		 	 	    else
      	    		 	 	    {
      	    		 	 	      layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    //切换测量点时读出已有的测量点信息
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
      	    		 	 	    {
      	    		 	 	    	 tmpData = (chrMp[0][0]-0x30)*1000 + (chrMp[0][1]-0x30)*100 + (chrMp[0][2]-0x30)*10 + (chrMp[0][3]-0x30);
      	    		 	 	    	 if (tmpData>2040 || tmpData<1)
      	    		 	 	    	 {
                              guiLine(10,55,150,105,0);
                              guiLine(10,55,10,105,1);
                              guiLine(150,55,150,105,1);
                              guiLine(10,55,150,55,1);
                              guiLine(10,105,150,105,1);
                              guiDisplay(20,70,"测量点输入错误!",1);
                              lcdRefresh(10,120);
      	    		 	 	    	 	  
      	    		 	 	    	 	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
      	    		 	 	    	 	  return;
      	    		 	 	    	 }
                           
                           if (selectF10Data(tmpData, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
                           {  
                           	  switch(meterConfig.protocol)
                           	  {
                           	  	 case 2:
                           	  	 	 chrMp[2][0] = 0x32;
                           	  	 	 break;
                           	  	 	 
                           	  	 case 30:
                           	  	 	 chrMp[2][0] = 0x31;
                           	  	 	 break;

                           	  	#ifdef LIGHTING
                           	  	 case 130:
                           	  	 	 chrMp[2][0] = 0x33;    //线路控制器
                           	  	 	 break;
                           	  	#endif
                           	  	 	 
                           	  	 default:
                           	  	 	 chrMp[2][0] = 0x30;
                           	  	 	 break;
                           	  }
                           	  
                           	  //表地址
                  	 	 	   	  chrMp[1][0]  = (meterConfig.addr[5]>>4)+0x30;
                  	 	 	   	  chrMp[1][1]  = (meterConfig.addr[5]&0xf)+0x30;
                  	 	 	   	  chrMp[1][2]  = (meterConfig.addr[4]>>4)+0x30;
                  	 	 	   	  chrMp[1][3]  = (meterConfig.addr[4]&0xf)+0x30;
                  	 	 	   	  chrMp[1][4]  = (meterConfig.addr[3]>>4)+0x30;
                  	 	 	   	  chrMp[1][5]  = (meterConfig.addr[3]&0xf)+0x30;
                  	 	 	   	  chrMp[1][6]  = (meterConfig.addr[2]>>4)+0x30;
                  	 	 	   	  chrMp[1][7]  = (meterConfig.addr[2]&0xf)+0x30;
                  	 	 	   	  chrMp[1][8]  = (meterConfig.addr[1]>>4)+0x30;
                  	 	 	   	  chrMp[1][9]  = (meterConfig.addr[1]&0xf)+0x30;
                  	 	 	   	  chrMp[1][10] = (meterConfig.addr[0]>>4)+0x30;
                  	 	 	   	  chrMp[1][11] = (meterConfig.addr[0]&0xf)+0x30;
                  	 	 	   	  chrMp[1][12] = '\0';
                  	 	 	   	  
                  	 	 	   	  //采集器地址
                  	 	 	   	  chrMp[3][0]  = (meterConfig.collectorAddr[5]>>4)+0x30;
                  	 	 	   	  chrMp[3][1]  = (meterConfig.collectorAddr[5]&0xf)+0x30;
                  	 	 	   	  chrMp[3][2]  = (meterConfig.collectorAddr[4]>>4)+0x30;
                  	 	 	   	  chrMp[3][3]  = (meterConfig.collectorAddr[4]&0xf)+0x30;
                  	 	 	   	  chrMp[3][4]  = (meterConfig.collectorAddr[3]>>4)+0x30;
                  	 	 	   	  chrMp[3][5]  = (meterConfig.collectorAddr[3]&0xf)+0x30;
                  	 	 	   	  chrMp[3][6]  = (meterConfig.collectorAddr[2]>>4)+0x30;
                  	 	 	   	  chrMp[3][7]  = (meterConfig.collectorAddr[2]&0xf)+0x30;
                  	 	 	   	  chrMp[3][8]  = (meterConfig.collectorAddr[1]>>4)+0x30;
                  	 	 	   	  chrMp[3][9]  = (meterConfig.collectorAddr[1]&0xf)+0x30;
                  	 	 	   	  chrMp[3][10] = (meterConfig.collectorAddr[0]>>4)+0x30;
                  	 	 	   	  chrMp[3][11] = (meterConfig.collectorAddr[0]&0xf)+0x30;
                  	 	 	   	  chrMp[3][12] = '\0';
                  	 	 	   	  
                  	 	 	   	 #ifdef LIGHTING 
                           	  //端口
                           	  switch(meterConfig.rateAndPort&0x1f)
                           	  {
                           	  	 case 31:
                           	  	 	 chrMp[3][0] = 0x30;
                           	  	 	 break;
                           	  	 	 
                           	  	 case 2:
                           	  	 	 chrMp[3][0] = 0x31;
                           	  	 	 break;

                           	  	 case 3:
                           	  	 	 chrMp[3][0] = 0x32;
                           	  	 	 break;
                           	  }
                           	 #endif

                  	 	 	   	  //费率个数
                  	 	 	   	  chrMp[4][0] = meterConfig.numOfTariff+0x30;
                           }
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    keyLeftRight = 0;
      	    		 	 	    setCarrierMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		break;
      	    	 	 	  		
      	    	 	 	  	case 3: //终端时间设置
    	    		 	 	      tmpTime.year   = (dateTimeItem[2]-0x30)*10+(dateTimeItem[3]-0x30);
    	    		 	 	      tmpTime.month  = (dateTimeItem[4]-0x30)*10+(dateTimeItem[5]-0x30);
    	    		 	 	      tmpTime.day    = (dateTimeItem[6]-0x30)*10+(dateTimeItem[7]-0x30);
    	    		 	 	      tmpTime.hour   = (dateTimeItem[8]-0x30)*10+(dateTimeItem[9]-0x30);
    	    		 	 	      tmpTime.minute = (dateTimeItem[10]-0x30)*10+(dateTimeItem[11]-0x30);
    	    		 	 	      tmpTime.second = (dateTimeItem[12]-0x30)*10+(dateTimeItem[13]-0x30);    	    		 	 	      
    	    		 	 	      tmpData        = (dateTimeItem[0]-0x30)*1000+(dateTimeItem[1]-0x30)*100 + (dateTimeItem[2]-0x30)*10+(dateTimeItem[3]-0x30);
    	    		 	 	      
    	    		 	 	      //判断年份
    	    		 	 	      if (tmpData>2099 || tmpData<1999)
    	    		 	 	      {
    	    		 	 	       	 keyLeftRight = 0;
    	    		 	 	       	 guiDisplay(28,110,"年份输入错误!",1);
    	    		 	 	       	 lcdRefresh(110,126);
    	    		 	 	       	 return;
    	    		 	 	      }
    	    		 	 	      
    	    		 	 	      //判断月份
    	    		 	 	      if (tmpTime.month>12 || tmpTime.month<1)
    	    		 	 	      {
    	    		 	 	       	 keyLeftRight = 4;
    	    		 	 	       	 guiDisplay(28,110,"月份输入错误!",1);
    	    		 	 	       	 lcdRefresh(110,126);
    	    		 	 	       	 return;
    	    		 	 	      }
    	    		 	 	      
    	    		 	 	      //判断日期
    	    		 	 	      tmpData = monthDays(tmpData,tmpTime.month);
    	    		 	 	      if (tmpTime.day>tmpData || tmpTime.day<1)
    	    		 	 	      {
    	    		 	 	       	 keyLeftRight = 6;
    	    		 	 	       	 guiDisplay(28,110,"日期输入错误!",1);
    	    		 	 	       	 lcdRefresh(110,126);
    	    		 	 	       	 return;    	    		 	 	      	 
    	    		 	 	      }

    	    		 	 	      //判断小时
    	    		 	 	      if (tmpTime.hour>24)
    	    		 	 	      {
    	    		 	 	       	 keyLeftRight = 8;
    	    		 	 	       	 guiDisplay(28,110,"小时输入错误!",1);
    	    		 	 	       	 lcdRefresh(110,126);
    	    		 	 	       	 return;    	    		 	 	      	 
    	    		 	 	      }
    	    		 	 	      
    	    		 	 	      //判断分钟
    	    		 	 	      if (tmpTime.minute>59)
    	    		 	 	      {
    	    		 	 	       	 keyLeftRight = 10;
    	    		 	 	       	 guiDisplay(28,110,"分钟输入错误!",1);
    	    		 	 	       	 lcdRefresh(110,126);
    	    		 	 	       	 return;    	    		 	 	      	 
    	    		 	 	      }

    	    		 	 	      //判断秒数
    	    		 	 	      if (tmpTime.second>59)
    	    		 	 	      {
    	    		 	 	       	 keyLeftRight = 12;
    	    		 	 	       	 guiDisplay(28,110,"秒数输入错误!",1);
    	    		 	 	       	 lcdRefresh(110,126);
    	    		 	 	       	 return;    	    		 	 	      	 
    	    		 	 	      }
                        
                        setSystemDateTime(tmpTime);
   
                        //刷新title时间显示
                        refreshTitleTime();

                        //重新设置抄表时间
                        reSetCopyTime();

                        guiLine(10,55,150,105,0);
                        guiLine(10,55,10,105,1);
                        guiLine(150,55,150,105,1);
                        guiLine(10,55,150,55,1);
                        guiLine(10,105,150,105,1);
                        guiDisplay(28,70,"修改时间成功!",1);
                        lcdRefresh(10,120);
                            
                        menuInLayer--;
    	    		 	 	       	  
    	    	 	 	  		 	break;
      	    	 	 	  		
      	    	 	 	  	case 4: //修改界面密码
    	    		 	 	      if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
    	    		 	 	      {
    	    		 	 	       	for(i=0;i<6;i++)
    	    		 	 	       	{
    	    		 	 	       	  if (commParaItem[0][i]!=commParaItem[1][i])
    	    		 	 	       	  {
    	    		 	 	       	  	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
    	    		 	 	       	  	 keyLeftRight = 0;
    	    		 	 	       	  	 guiDisplay(20,100,"输入密码不一致!",1);
    	    		 	 	       	  	 lcdRefresh(100,120);
    	    		 	 	       	  	 return;
    	    		 	 	       	  }
    	    		 	 	       	}
    	    		 	 	           
    	    		 	 	        //确认新密码
    	    		 	 	        keyLeftRight = 0xff;
    	    		 	 	        modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
    	    		 	 	        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0xff;
    	    		 	 	       	return;
    	    		 	 	      }
    	    		 	 	       
    	    		 	 	      if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0xff)
    	    		 	 	      {
    	    		 	 	       	  for(i=0;i<6;i++)
    	    		 	 	       	  {
    	    		 	 	       	  	 originPassword[i] = commParaItem[0][i];
    	    		 	 	       	  }
    	    		 	 	       	  originPassword[6] = '\0';
                            guiLine(10,55,150,105,0);
                            guiLine(10,55,10,105,1);
                            guiLine(150,55,150,105,1);
                            guiLine(10,55,150,55,1);
                            guiLine(10,105,150,105,1);
                            guiDisplay(28,70,"修改密码成功!",1);
                            lcdRefresh(10,120);
                            
                            menuInLayer--;
                            layer2MenuLight[layer1MenuLight]=0xff;
    	    		 	 	       	  return;
    	    		 	 	       }
    	    		 	 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
    	    		 	 	       {
    	    		 	 	       	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
    	    		 	 	       }
    	    		 	 	       else
    	    		 	 	       {
    	    		 	 	       	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
    	    		 	 	       }
    	    		 	 	       keyLeftRight = 0;
    	    		 	 	       modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
    	    	 	 	  		 	 break;
    	    	 	 	  		 	 
      	    	 	 	  	case 5: //终端编号
   	  	 	 	      	    #ifdef TE_ADDR_USE_BCD_CODE
   	  	 	 	      	  	 addrField.a2[1] = (tmpTeAddr[0]-0x30)<<4 | (tmpTeAddr[1]-0x30);
   	  	 	 	      	  	 addrField.a2[0] = (tmpTeAddr[2]-0x30)<<4 | (tmpTeAddr[3]-0x30);
   	  	 	 	      	  	 addrField.a1[1] = (tmpTeAddr[4]-0x30)<<4 | (tmpTeAddr[5]-0x30);
   	  	 	 	      	  	 addrField.a1[0] = (tmpTeAddr[6]-0x30)<<4 | (tmpTeAddr[7]-0x30);
   	  	 	 	      	    #else
   	  	 	 	      	  	 tmpAddr = (tmpTeAddr[0]-0x30)*10000+(tmpTeAddr[1]-0x30)*1000
   	  	 	 	      	  	         +(tmpTeAddr[2]-0x30)*100+(tmpTeAddr[3]-0x30)*10
   	  	 	 	      	  	         +(tmpTeAddr[4]-0x30);
   	  	 	 	      	  	 addrField.a2[1] = tmpAddr>>8;
   	  	 	 	      	  	 addrField.a2[0] = tmpAddr&0xff;
   	  	 	 	      	  	 addrField.a1[1] = (tmpTeAddr[5]-0x30)<<4 | (tmpTeAddr[6]-0x30);
   	  	 	 	      	  	 addrField.a1[0] = (tmpTeAddr[7]-0x30)<<4 | (tmpTeAddr[8]-0x30);
   	  	 	 	      	    #endif   	  	 	 	      	  	

                        //保存行政区划码
                        saveParameter(0x04, 121,(INT8U *)&addrField,4);
                        
                        saveBakKeyPara(121);    //2012-8-9,add
                        
                        guiLine(10,55,150,105,0);
                        guiLine(10,55,10,105,1);
                        guiLine(150,55,150,105,1);
                        guiLine(10,55,150,55,1);
                        guiLine(10,105,150,105,1);
                        guiDisplay(16,70,"修改终端编号成功!",1);
                        lcdRefresh(10,120);
                        
                        menuInLayer--;
      	    	 	 	  		break;
      	    	 	 	    
      	    	 	 	    case 6:  //以太网参数设置
    	    		 	 	      if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==4)
    	    		 	 	      {
        	  	 	 	      	//以太网IP地址
        	  	 	 	      	teIpAndPort.teIpAddr[0] =(chrEthPara[0][0]-0x30)*100+(chrEthPara[0][1]-0x30)*10+(chrEthPara[0][2]-0x30);
        	  	 	 	      	teIpAndPort.teIpAddr[1] =(chrEthPara[0][4]-0x30)*100+(chrEthPara[0][5]-0x30)*10+(chrEthPara[0][6]-0x30);
        	  	 	 	      	teIpAndPort.teIpAddr[2] =(chrEthPara[0][8]-0x30)*100+(chrEthPara[0][9]-0x30)*10+(chrEthPara[0][10]-0x30);
        	  	 	 	      	teIpAndPort.teIpAddr[3] =(chrEthPara[0][12]-0x30)*100+(chrEthPara[0][13]-0x30)*10+(chrEthPara[0][14]-0x30);

        	  	 	 	      	//以太网IP掩码
        	  	 	 	      	teIpAndPort.mask[0] =(chrEthPara[1][0]-0x30)*100+(chrEthPara[1][1]-0x30)*10+(chrEthPara[1][2]-0x30);
        	  	 	 	      	teIpAndPort.mask[1] =(chrEthPara[1][4]-0x30)*100+(chrEthPara[1][5]-0x30)*10+(chrEthPara[1][6]-0x30);
        	  	 	 	      	teIpAndPort.mask[2] =(chrEthPara[1][8]-0x30)*100+(chrEthPara[1][9]-0x30)*10+(chrEthPara[1][10]-0x30);
        	  	 	 	      	teIpAndPort.mask[3] =(chrEthPara[1][12]-0x30)*100+(chrEthPara[1][13]-0x30)*10+(chrEthPara[1][14]-0x30);

        	  	 	 	      	//以太网网关
        	  	 	 	      	teIpAndPort.gateWay[0] =(chrEthPara[2][0]-0x30)*100+(chrEthPara[2][1]-0x30)*10+(chrEthPara[2][2]-0x30);
        	  	 	 	      	teIpAndPort.gateWay[1] =(chrEthPara[2][4]-0x30)*100+(chrEthPara[2][5]-0x30)*10+(chrEthPara[2][6]-0x30);
        	  	 	 	      	teIpAndPort.gateWay[2] =(chrEthPara[2][8]-0x30)*100+(chrEthPara[2][9]-0x30)*10+(chrEthPara[2][10]-0x30);
        	  	 	 	      	teIpAndPort.gateWay[3] =(chrEthPara[2][12]-0x30)*100+(chrEthPara[2][13]-0x30)*10+(chrEthPara[2][14]-0x30);
        	  	 	 	      	
        	  	 	 	      	//是否使用以太网登录
        	  	 	 	      	teIpAndPort.ethIfLoginMs = chrEthPara[3][0];

        	  	 	 	      	//保存
	                        saveIpMaskGateway(teIpAndPort.teIpAddr,teIpAndPort.mask,teIpAndPort.gateWay);  //保存到rcS中,ly,2011-04-12
	 
	                        saveParameter(0x04, 7, (INT8U *)&teIpAndPort, sizeof(TE_IP_AND_PORT));

													//保存备份007参数文件,2020-11-18,Add
													saveBakKeyPara(7);
                              
                          guiLine(6,55,154,105,0);
                          guiLine(6,55,6,105,1);
                          guiLine(154,55,154,105,1);
                          guiLine(6,55,154,55,1);
                          guiLine(6,105,154,105,1);
                          guiDisplay(8,60,"修改以太网参数成功",1);
                          guiDisplay(16,80,"设置生效需要重启",1);
                          lcdRefresh(10,120);
                              
                          menuInLayer--;
                          layer2MenuLight[layer1MenuLight]=0xff;
    	    		 	 	      }
    	    		 	 	      else
    	    		 	 	      {
    	    		 	 	        if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
    	    		 	 	        {
    	    		 	 	      	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
    	    		 	 	        }
    	    		 	 	        else
    	    		 	 	        {
    	    		 	 	       	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
    	    		 	 	        }
    	    		 	 	        keyLeftRight = 0;
    	    		 	 	        setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
    	    		 	 	      }
      	    	 	 	    	break;
      	    	 	 	    	
      	    	 	 	    case 7:   //VPN用户名密码
                 	 	 	  if (inputStatus == STATUS_SELECT_CHAR)
                 	 	 	  {
                 	 	 	  	inputStatus = STATUS_NONE;
                 	 	 	  	if (keyLeftRight<32)
                 	 	 	  	{
                 	 	 	  	  tmpVpnUserName[keyLeftRight] = character[selectIndex];
                 	 	 	  	}
                 	 	 	  	else
                 	 	 	  	{
                 	 	 	  	  tmpVpnPw[keyLeftRight-32] = character[selectIndex];
                 	 	 	  	}
                 	 	 	  	setVpn(keyLeftRight);
                 	 	 	  }
                 	 	 	  else
                 	 	 	  {
                          tmpVpnUserName[32] = '\0';
                          trim(tmpVpnUserName);
                          tmpVpnPw[32] = '\0';
                          trim(tmpVpnPw);
                          //strcpy((char *)vpn.vpnName, tmpVpnUserName);
                          //memcpy(vpn.vpnName, tmpVpnUserName, 16);
                          //ly,2012-03-12,更改在兰州发现错误
                          memcpy(vpn.vpnName, tmpVpnUserName, 32);
                          //strcpy((char *)vpn.vpnPassword, tmpVpnPw);
                          //memcpy(vpn.vpnPassword, tmpVpnPw, 16);
                          memcpy(vpn.vpnPassword, tmpVpnPw, 32);

        	  	 	 	      	//保存vpn用户名密码
        	  	 	 	      	saveParameter(0x04, 16,(INT8U *)&vpn, sizeof(VPN));
                        	
                        	saveBakKeyPara(16);    //2012-8-9,add

                          guiLine(10,55,150,105,0);
                          guiLine(10,55,10,105,1);
                          guiLine(150,55,150,105,1);
                          guiLine(10,55,150,55,1);
                          guiLine(10,105,150,105,1);
                          guiDisplay(20, 70, "专网参数已修改!",1);
                          lcdRefresh(10,120);
                              
                          menuInLayer--;
                 	 	 	  }
                 	 	 	  break;
                 	 	 	  
      	    	 	 	  	case 8:   //设置终端级联
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==5)
      	    		 	 	    {
        	  	 	 	      	//级联端口
        	  	 	 	      	if (chrMp[0][0]!=0x33)
        	  	 	 	      	{
                             guiLine(10,55,150,105,0);
                             guiLine(10,55,10,105,1);
                             guiLine(150,55,150,105,1);
                             guiLine(10,55,150,55,1);
                             guiLine(10,105,150,105,1);
                             guiDisplay(20,60,"输入错误!级联端口只",1);
                             guiDisplay(20,80,"能为3",1);
                             lcdRefresh(10,120);
      	    		 	 	    	 	 
      	    		 	 	    	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
                             return;
        	  	 	 	      	}
        	  	 	 	      	
        	  	 	 	      	cascadeCommPara.commPort = 0x03; //端口3
        	  	 	 	      	cascadeCommPara.ctrlWord = 0x8B;             //默认4800,8-e-1,目前最高只能通这个速率
        	  	 	 	      	cascadeCommPara.receiveMsgTimeout = 0x05;
        	  	 	 	      	cascadeCommPara.receiveByteTimeout = 0x05;
        	  	 	 	      	cascadeCommPara.cascadeMretryTime = 0x01;
        	  	 	 	      	cascadeCommPara.groundSurveyPeriod = 0x05;   //巡测周期
        	  	 	 	      	
        	  	 	 	      	cascadeCommPara.flagAndTeNumber = 0x00;
        	  	 	 	      	if (chrMp[1][0]==0x30)
        	  	 	 	      	{
        	  	 	 	      	  cascadeCommPara.divisionCode[0] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[1] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[2] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[3] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[4] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[5] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[0] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[1] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[2] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[3] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[4] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[5] = 0x00;
        	  	 	 	      	}
        	  	 	 	      	else
        	  	 	 	      	{
   	  	 	 	      	  	    //终端地址1
   	  	 	 	      	  	    tmpAddr = (chrMp[2][4]-0x30)*10000+(chrMp[2][5]-0x30)*1000
   	  	 	 	      	  	            +(chrMp[2][6]-0x30)*100+(chrMp[2][7]-0x30)*10
   	  	 	 	      	  	            +(chrMp[2][8]-0x30);
        	  	 	 	      	  tmpData = (chrMp[2][0]-0x30)<<12 | (chrMp[2][1]-0x30)<<8 | (chrMp[2][2]-0x30)<<4 | (chrMp[2][3]-0x30);
        	  	 	 	      	  if (tmpData!=0x0 && tmpAddr!=0x00)
        	  	 	 	      	  {
        	  	 	 	      	  	cascadeCommPara.flagAndTeNumber++;

   	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[1] = tmpAddr>>8;
   	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[0] = tmpAddr&0xff;
   	  	 	 	      	  	      cascadeCommPara.divisionCode[1] = (chrMp[2][0]-0x30)<<4 | (chrMp[2][1]-0x30);
   	  	 	 	      	  	      cascadeCommPara.divisionCode[0] = (chrMp[2][2]-0x30)<<4 | (chrMp[2][3]-0x30);
        	  	 	 	      	  }

        	  	 	 	      		if (chrMp[1][0]==0x32)
        	  	 	 	      		{
        	  	 	 	      			cascadeCommPara.flagAndTeNumber |= 0x80;
        	  	 	 	      		}
        	  	 	 	      		else
        	  	 	 	      		{
     	  	 	 	      	  	    //终端地址2
     	  	 	 	      	  	    tmpAddr = (chrMp[3][4]-0x30)*10000+(chrMp[3][5]-0x30)*1000
     	  	 	 	      	  	            +(chrMp[3][6]-0x30)*100+(chrMp[3][7]-0x30)*10
     	  	 	 	      	  	            +(chrMp[3][8]-0x30);
          	  	 	 	      	  tmpData = (chrMp[3][0]-0x30)<<12 | (chrMp[3][1]-0x30)<<8 | (chrMp[3][2]-0x30)<<4 | (chrMp[3][3]-0x30);
          	  	 	 	      	  if (tmpData!=0x0 && tmpAddr!=0x00)
          	  	 	 	      	  {
          	  	 	 	      	  	cascadeCommPara.flagAndTeNumber++;
  
     	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[3] = tmpAddr>>8;
     	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[2] = tmpAddr&0xff;
     	  	 	 	      	  	      cascadeCommPara.divisionCode[3] = (chrMp[3][0]-0x30)<<4 | (chrMp[3][1]-0x30);
     	  	 	 	      	  	      cascadeCommPara.divisionCode[2] = (chrMp[3][2]-0x30)<<4 | (chrMp[3][3]-0x30);
          	  	 	 	      	  }

     	  	 	 	      	  	    //终端地址3
     	  	 	 	      	  	    tmpAddr = (chrMp[4][4]-0x30)*10000+(chrMp[4][5]-0x30)*1000
     	  	 	 	      	  	            +(chrMp[4][6]-0x30)*100+(chrMp[4][7]-0x30)*10
     	  	 	 	      	  	            +(chrMp[4][8]-0x30);
          	  	 	 	      	  tmpData = (chrMp[4][0]-0x30)<<12 | (chrMp[4][1]-0x30)<<8 | (chrMp[4][2]-0x30)<<4 | (chrMp[4][3]-0x30);
          	  	 	 	      	  if (tmpData!=0x0 && tmpAddr!=0x00)
          	  	 	 	      	  {
          	  	 	 	      	  	cascadeCommPara.flagAndTeNumber++;
  
     	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[5] = tmpAddr>>8;
     	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[4] = tmpAddr&0xff;
     	  	 	 	      	  	      cascadeCommPara.divisionCode[5] = (chrMp[4][0]-0x30)<<4 | (chrMp[4][1]-0x30);
     	  	 	 	      	  	      cascadeCommPara.divisionCode[4] = (chrMp[4][2]-0x30)<<4 | (chrMp[4][3]-0x30);
          	  	 	 	      	  }
        	  	 	 	      		}
        	  	 	 	      	}
        	  	 	 	      	
                          saveParameter(0x04, 37, (INT8U *)&cascadeCommPara, sizeof(CASCADE_COMM_PARA));

                          guiLine(10,55,150,105,0);
                          guiLine(10,55,10,105,1);
                          guiLine(150,55,150,105,1);
                          guiLine(10,55,150,55,1);
                          guiLine(10,105,150,105,1);
                          guiDisplay(12,70,"级联参数设置成功!",1);
                          lcdRefresh(10,120);
                              
                          menuInLayer--;
                          return;        	  	 	 	      	
        	  	 	 	      }

      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
      	    		 	 	    {
      	    		 	 	    	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	    }
      	    		 	 	    else
      	    		 	 	    {
      	    		 	 	      layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    keyLeftRight = 0;
      	    		 	 	    setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		break;
      	    	 	 	  		      	    	 	 	  		
      	    	 	 	  	case 9:   //锐拔模块参数设置
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==4)
      	    		 	 	    {
        	  	 	 	      	//基本功率
        	  	 	 	      	if (((chrRlPara[0][0]-0x30)*10+(chrRlPara[0][1]-0x30))>63
        	  	 	 	      		  || ((chrRlPara[1][0]-0x30)*10+(chrRlPara[1][1]-0x30))>63
        	  	 	 	      		  || ((chrRlPara[2][0]-0x30)*10+(chrRlPara[2][1]-0x30))>63
        	  	 	 	      		  || ((chrRlPara[3][0]-0x30)*10+(chrRlPara[3][1]-0x30))>6
        	  	 	 	      		 )
        	  	 	 	      	{
                             guiLine(10,55,150,105,0);
                             guiLine(10,55,10,105,1);
                             guiLine(150,55,150,105,1);
                             guiLine(10,55,150,55,1);
                             guiLine(10,105,150,105,1);
                             guiDisplay(20,60,"输入错误!功率范",1);
                             guiDisplay(20,80,"围1-63,信道1-6",1);
                             lcdRefresh(10,120);
      	    		 	 	    	 	 
      	    		 	 	    	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
                             return;
        	  	 	 	      	}
        	  	 	 	      	
        	  	 	 	      	rlPara[0] = (chrRlPara[0][0]-0x30)*10+(chrRlPara[0][1]-0x30);
        	  	 	 	      	rlPara[1] = (chrRlPara[1][0]-0x30)*10+(chrRlPara[1][1]-0x30);
        	  	 	 	      	rlPara[2] = (chrRlPara[2][0]-0x30)*10+(chrRlPara[2][1]-0x30);
        	  	 	 	      	rlPara[3] = (chrRlPara[3][0]-0x30)*10+(chrRlPara[3][1]-0x30);
        	  	 	 	      	
        	  	 	 	      	saveParameter(0x04, 135, (INT8U *)&rlPara, 4);
                          
                          guiLine(10,55,150,105,0);
                          guiLine(10,55,10,105,1);
                          guiLine(150,55,150,105,1);
                          guiLine(10,55,150,55,1);
                          guiLine(10,105,150,105,1);
                          guiDisplay(12,70,"锐拔参数设置成功!",1);
                          lcdRefresh(10,120);
                              
                          menuInLayer--;
                          return;        	  	 	 	      	
        	  	 	 	      }

      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
      	    		 	 	    {
      	    		 	 	    	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	    }
      	    		 	 	    else
      	    		 	 	    {
      	    		 	 	      layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    keyLeftRight = 0;
      	    		 	 	    setRlPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		break;
      	    	 	 	  }
      	    	 	 	  break;
      	    	 	 	
      	    	 	 	case 2:    //终端管理与维护
      	    	 	 	  switch(layer2MenuLight[layer1MenuLight])
      	    	 	 	  {
    	    	 	 	  		 case 0:  //实时抄表
    	    	 	 	  		 	 if (carrierModuleType==NO_CARRIER_MODULE || ((carrierModuleType==RL_WIRELESS || carrierModuleType==SR_WIRELESS)  && carrierFlagSet.wlNetOk<3))
      	    		 	 	 	   {
                           guiLine(10,55,150,105,0);
                           guiLine(10,55,10,105,1);
                           guiLine(150,55,150,105,1);
                           guiLine(10,55,150,55,1);
                           guiLine(10,105,150,105,1);
                           if (carrierModuleType==NO_CARRIER_MODULE)
                           {
                             guiDisplay(15,60,"  等待识别模块!",1);
                           }
                           else
                           {
                             guiDisplay(15,60,"等待模块组网完成!",1);
                           }
                           guiDisplay(15,80,"     请稍候   ",1);
                           lcdRefresh(10,120);
      	    		 	 	 	   }
      	    		 	 	 	   else
      	    		 	 	 	   {
      	    	 	 	  		 	 switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
      	    	 	 	  		 	 {
      	    	 	 	  		 	 	  case 0: //指定测量点抄表
      	    	 	 	  		 	 	  	keyLeftRight = 0;
      	    	 	 	  		 	 	  	strcpy(singleCopyTime,"-----");
             	 	                strcpy(singleCopyEnergy,"-----");
      	    	 	 	  		 	 	  	singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
      	    	 	 	  		 	 	  	break;
      	    	 	 	  		 	 	  	
      	    	 	 	  		 	 	  case 1: //全部测量点抄表
      	    	 	 	  		 	 	  	if (pDotCopy==NULL)
      	    	 	 	  		 	 	  	{
      	    	 	 	  		 	 	  	  keyLeftRight = 1;
      	    	 	 	  		 	 	  	  allMpCopy(keyLeftRight);
      	    	 	 	  		 	 	  	}
      	    	 	 	  		 	 	  	else
      	    	 	 	  		 	 	  	{
                                  guiLine(10,55,150,105,0);
                                  guiLine(10,55,10,105,1);
                                  guiLine(150,55,150,105,1);
                                  guiLine(10,55,150,55,1);
                                  guiLine(10,105,150,105,1);
                                  guiDisplay(12,70,"正在进行点抄,稍候",1);
                                  lcdRefresh(10,120);
      	    	 	 	  		 	 	  	}
      	    	 	 	  		 	 	  	break;
      	    	 	 	  		 	 }
      	    	 	 	  		 }
    	    	 	 	  		 	 break;
    	    	 	 	  		 
    	    	 	 	  		 case 1:
    	    	 	 	  		 	#ifdef LIGHTING    //照明集中器是本机信息

      	    		 	 	 	   if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-2)
      	    		 	 	 	   {
      	    		 	 	 	   	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
      	    		 	 	 	   }
      	    		 	 	 	   else
      	    		 	 	 	   {
      	    		 	 	 	  	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	 	   }
      	    	 	 	  		 terminalInfo(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    	 	 	  		 
    	    	 	 	  		 	#endif
    	    	 	 	  		 	 break;
    	    	 	 	  		 	 
    	    	 	 	  		 case 2:  //表号搜索
    	    	 	 	  		 	#ifndef LIGHTING
    	    	 	 	  		 	 switch(keyLeftRight)
    	    	 	 	  		 	 {
    	    	 	 	  		 	 	 case 0:  //启动搜表
    	    	 	 	  		 	 	   if (carrierFlagSet.searchMeter==0)
    	    	 	 	  		 	 	   {
    	    	 	 	  		 	 	     //清除以前搜索到的表
    	    	 	 	  		 	 	     while(foundMeterHead!=NULL)
    	    	 	 	  		 	 	     {
    	    	 	 	  		 	 	   	    tmpFound = foundMeterHead;
    	    	 	 	  		 	 	   	    foundMeterHead = foundMeterHead->next;
    	    	 	 	  		 	 	   	    free(tmpFound);
    	    	 	 	  		 	 	     }
    	    	 	 	  		 	 	     foundMeterHead = NULL;
    	    	 	 	  		 	 	     prevFound = foundMeterHead;
    	    	 	 	  		 	 	     carrierFlagSet.foundStudyTime = nextTime(sysTime, assignCopyTime[0]|assignCopyTime[1]<<8, 0); //搜表时间
    	    	 	 	  		 	 	   
    	    	 	 	  		 	 	     carrierFlagSet.searchMeter = 1;         //开始搜表标志置1
    	    	 	 	  		 	 	   }
    	    	 	 	  		 	 	 	 keyLeftRight = 0xff;
    	    	 	 	  		 	 	   searchMeter(keyLeftRight);
 
    	    	 	 	  		 	 	 	 break;
    	    	 	 	  		 	 	 	  
    	    	 	 	  		 	 	 case 1:
    	    	 	 	  		 	 	 	 keyLeftRight = 0;
    	    	 	 	  		 	 	 	 searchMeter(keyLeftRight);
    	    	 	 	  		 	 	 	 break;
    	    	 	 	  		 	 	 	 
    	    	 	 	  		 	 	 case 0xff:
    	    	 	 	  		 	 	 	 layer2MenuLight[layer1MenuLight]++;
    	    	 	 	  		 	 	 	 keyLeftRight = 1;
    	    	 	 	  		 	 	 	 newAddMeter(keyLeftRight);
    	    	 	 	  		 	 	 	 break;
    	    	 	 	  		 	 }
    	    	 	 	  		 	#endif
    	    	 	 	  		 	 break;
    	    	 	 	  		 	 
    	    	 	 	  		 case 3:  //新增电能表地址
    	    	 	 	  		 	 switch(keyLeftRight)
    	    	 	 	  		 	 {
    	    	 	 	  		 	 	  case 0: //启动抄表(抄新增电能表的示值)
           	 	 	  			      keyLeftRight = 1;
           	 	 	  			      
           	 	 	  			      tmpFound = foundMeterHead;
           	 	 	  			      while(tmpFound!=NULL)
           	 	 	  			      {
           	 	 	  			      	 tmpFound->copyTime[0]   = 0xee;
           	 	 	  			      	 tmpFound->copyTime[1]   = 0xee;
           	 	 	  			      	 tmpFound->copyEnergy[0] = 0xee;
           	 	 	  			      	 
           	 	 	  			      	 tmpFound = tmpFound->next;
           	 	 	  			      }
           	 	 	  			      layer2MenuLight[layer1MenuLight]++;
           	 	 	  			      multiCpUpDown = 0;
           	 	 	  			      newMeterCpStatus(0);
           	 	 	  			      
           	 	 	  			      if (foundMeterHead!=NULL)
           	 	 	  			      {
                          	     pDotCopy = (DOT_COPY *)malloc(sizeof(DOT_COPY));
                          	     prevFound = foundMeterHead;
                          	     pDotCopy->dotCopyMp = prevFound->mp;
           	 	 	  			       	 memcpy(pDotCopy->addr, prevFound->addr, 6);
                          	     pDotCopy->dotCopying = FALSE;
                          	     pDotCopy->port       = PORT_POWER_CARRIER;
                          	     pDotCopy->dataFrom   = DOT_CP_NEW_METER;
                          	     pDotCopy->outTime    = nextTime(sysTime,0,22);
                          	     pDotCopy->dotResult  = RESULT_NONE;
           	 	 	  			      }
    	    	 	 	  		 	 	  	break;
    	    	 	 	  		 	 	  	
    	    	 	 	  		 	 	  case 1:
    	    	 	 	  		 	 	  	//noFoundMeter(0);
    	    	 	 	  		 	 	  	break;
    	    	 	 	  		 	 	  	
    	    	 	 	  		 	 	  case 0xff:   //正在抄表转到新增电能表抄表状态页
    	    	 	 	  		 	 	  	break;
    	    	 	 	  		 	 }
    	    	 	 	  		 	 break;
    	    	 	 	  		 	 
    	    	 	 	  		 case 5:
											 	#ifdef LIGHTING
												 //红外遥控学习器在RS485口上

										     //如果该端口当前未处理转发
										     if (copyCtrl[1].pForwardData==NULL)
										     {
													 guiLine(10,55,150,105,0);
													 guiLine(10,55,10,105,1);
													 guiLine(150,55,150,105,1);
													 guiLine(10,55,150,55,1);
													 guiLine(10,105,150,105,1);
													 
													 tmpData = 0;
													 if (keyLeftRight<4)
										       {
													   if (selectParameter(5, 160+keyLeftRight, (INT8U *)str, 2)==FALSE)
													   {
														   tmpData = 1;
															 
															 guiDisplay(20,75,"无红外学习数据.",1);
													   }
										       }

													 if (0==tmpData)
													 {
									       	   copyCtrl[1].pForwardData = (FORWARD_DATA *)malloc(sizeof(FORWARD_DATA));
									           copyCtrl[1].pForwardData->fn            = 1;
									           copyCtrl[1].pForwardData->dataFrom      = DOT_CP_IR;
									       	   copyCtrl[1].pForwardData->ifSend        = FALSE;
									       	   copyCtrl[1].pForwardData->receivedBytes = 0;
									       	   copyCtrl[1].pForwardData->forwardResult = RESULT_NONE;
									       
									       	   //透明转发通信控制字,2400-8-n-1
									       	   copyCtrl[1].pForwardData->ctrlWord = 0x63;    //2400-8-n-1
									       	  
									       	   //透明转发接收等待字节超时时间
									       	   copyCtrl[1].pForwardData->byteTimeOut = 1;
									       	  
														 copyCtrl[1].pForwardData->data[768] = keyLeftRight;		//本字节保存学习或命令类型
														 
														 //学习
														 if (keyLeftRight>3)
														 {
										       	   //透明转发接收等待报文超时时间
															 //单位为s
										       	   copyCtrl[1].pForwardData->frameTimeOut = 5;
															 
															 copyCtrl[1].pForwardData->length = 4;
										       	  
										       	   //透明转发内容
										       	   copyCtrl[1].pForwardData->data[0] = 0xfa;
										       	   copyCtrl[1].pForwardData->data[1] = 0xfd;
										       	   copyCtrl[1].pForwardData->data[2] = 0x01;
										       	   copyCtrl[1].pForwardData->data[3] = 0x01;

															 guiDisplay(40,75,"学习中...",1);
														 }
														 else    //发命令
													 	 {
														   //透明转发接收等待报文超时时间
														   //单位为s
														   copyCtrl[1].pForwardData->frameTimeOut = 1;

															 if (selectParameter(5, 160+keyLeftRight, copyCtrl[1].pForwardData->data, 2)==TRUE)
															 {
																 copyCtrl[1].pForwardData->length = copyCtrl[1].pForwardData->data[0] | copyCtrl[1].pForwardData->data[1]<<8;
																 printf("读出长度=%d\n", copyCtrl[1].pForwardData->length);
																 
																 selectParameter(5, 160+keyLeftRight, copyCtrl[1].pForwardData->data, copyCtrl[1].pForwardData->length+2);
																 for(tmpData=0; tmpData<copyCtrl[1].pForwardData->length; tmpData++)
																 {
																 	 copyCtrl[1].pForwardData->data[tmpData] = copyCtrl[1].pForwardData->data[tmpData+2];
																 }
																 
																 guiDisplay(40,75,"发送中...",1);
															 }
													 	 }
													 }
													 lcdRefresh(10,120);
										     }
												 
												#else

												 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-2)
      	    		 	 	 	   {
      	    		 	 	 	   	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
      	    		 	 	 	   }
      	    		 	 	 	   else
      	    		 	 	 	   {
      	    		 	 	 	  	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	 	   }
      	    	 	 	  		 terminalInfo(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
												#endif
      	    	 	 	  		 break;
      	    	 	 	  	
      	    	 	 	  	 case 7:  //本地通信模块抄读方式设置
                         //保存行政区划码
                         if (keyLeftRight==1)
                         {
                         	 localCopyForm = 0xaa;
                         }
                        else
                         {
                         	 localCopyForm = 0x55;
                         }
                         saveParameter(88, 55, &localCopyForm, 1);

                         guiLine(10,55,150,105,0);
                         guiLine(10,55,10,105,1);
                         guiLine(150,55,150,105,1);
                         guiLine(10,55,150,55,1);
                         guiLine(10,105,150,105,1);
                         guiDisplay(12,70,"设置抄读方式成功!",1);
                         lcdRefresh(10,120);
                             
                         menuInLayer--;
                         return;
      	    	 	 	  	 	 break;
      	    	 	 	  	 
      	    	 	 	  	 case 11:    //居民用户表数据类型
        	    	 	 	  	 switch (keyLeftRight)
        	    	 	 	  	 {
        	    	 	 	  	  	case 0x01:
        	    	 	 	  	  		denizenDataType = 0x55;
        	    	 	 	  	  	  break;
        	    	 	 	  	  	  
        	    	 	 	  	  	case 0x02:
        	    	 	 	  	  	  denizenDataType = 0xaa;
        	    	 	 	  	  	  break;
        	    	 	 	  	  
        	    	 	 	  	  	default:
        	    	 	 	  	  		denizenDataType = 0;
        	    	 	 	  	  		break;
        	    	 	 	  	 }
        	    	 	 	  	 saveParameter(0x04, 138, (INT8U *)&denizenDataType, 1);
        	    	 	 	  	  
                         guiLine(10,55,150,105,0);
                         guiLine(10,55,10,105,1);
                         guiLine(150,55,150,105,1);
                         guiLine(10,55,150,55,1);
                         guiLine(10,105,150,105,1);
                         guiDisplay(12,70,"居民表型设置成功!",1);
                         lcdRefresh(10,120);
                             
                         menuInLayer--;
                         return;
                         break;
                         
      	    	 	 	  	 case 12:    //轮显设置
        	    	 	 	  	 switch (keyLeftRight)
        	    	 	 	  	 {
        	    	 	 	  	   case 0x01:
        	    	 	 	  	  	 cycleDataType = 0x55;
        	    	 	 	  	  	 break;
        	    	 	 	  	  
        	    	 	 	  	   default:
        	    	 	 	  	  	 cycleDataType = 0;
        	    	 	 	  	  	 break;
        	    	 	 	  	 }
        	    	 	 	  	 saveParameter(0x04, 199, (INT8U *)&cycleDataType, 1);
        	    	 	 	  	  
                         guiLine(10,55,150,105,0);
                         guiLine(10,55,10,105,1);
                         guiLine(150,55,150,105,1);
                         guiLine(10,55,150,55,1);
                         guiLine(10,105,150,105,1);
                         guiDisplay(12,70,"轮显内容设置成功!",1);
                         lcdRefresh(10,120);
                             
                         menuInLayer--;
                         return;
                         break;

      	    	 	 	  	 case 13:    //维护接口模式设置
        	    	 	 	  	 switch (keyLeftRight)
        	    	 	 	  	 {
        	    	 	 	  	   case 0x01:
        	    	 	 	  	  	 mainTainPortMode = 0x55;
        	    	 	 	  	  	 break;
        	    	 	 	  	  
        	    	 	 	  	   default:
        	    	 	 	  	  	 mainTainPortMode = 0;
        	    	 	 	  	  	 break;
        	    	 	 	  	 }
        	    	 	 	  	  
                         guiLine(10,55,150,105,0);
                         guiLine(10,55,10,105,1);
                         guiLine(150,55,150,105,1);
                         guiLine(10,55,150,55,1);
                         guiLine(10,105,150,105,1);
                         guiDisplay(9,70,"维护口模式设置成功",1);
                         lcdRefresh(10,120);
                             
                         menuInLayer--;
                         return;
      	    	 	 	  	   break;

      	    	 	 	  	 case 14:    //第2路485口功能设置
        	    	 	 	  	 switch (keyLeftRight)
        	    	 	 	  	 {
        	    	 	 	  	   case 0x01:
        	    	 	 	  	  	 rs485Port2Fun = 0x55;
        	    	 	 	  	  	 
        	    	 	 	  	  	 //设置第2路485的速率为9600-8-e-1
                            #ifdef WDOG_USE_X_MEGA
                             str[0] = 0x02;    //xMega端口2
                             str[1] = 0xcb;    //端口速率,9600-8-e-1
                             sendXmegaFrame(COPY_PORT_RATE_SET,(INT8U *)str, 2);
                              
                             printf("设置第2路速率为9600-8-e-1\n");
                            #endif

        	    	 	 	  	  	 break;
        	    	 	 	  	  
        	    	 	 	  	   default:
        	    	 	 	  	  	 rs485Port2Fun = 0;
        	    	 	 	  	  	 break;
        	    	 	 	  	 }
        	    	 	 	  	  
                         guiLine(10,55,150,105,0);
                         guiLine(10,55,10,105,1);
                         guiLine(150,55,150,105,1);
                         guiLine(10,55,150,55,1);
                         guiLine(10,105,150,105,1);
                         guiDisplay(9,70,"485口2功能设置成功",1);
                         lcdRefresh(10,120);
                             
                         menuInLayer--;
                         return;
      	    	 	 	  	   break;
      	    	 	 	  	   
      	    	 	 	  	 case 15:    //本地通信模块协议设置
        	    	 	 	  	 switch (keyLeftRight)
        	    	 	 	  	 {
        	    	 	 	  	   case 0x01:
        	    	 	 	  	  	 lmProtocol = 0x55;
        	    	 	 	  	  	 break;
        	    	 	 	  	  
        	    	 	 	  	   default:
        	    	 	 	  	  	 lmProtocol = 0;
        	    	 	 	  	  	 break;
        	    	 	 	  	 }
                         saveParameter(88, 56, &lmProtocol, 1);

                         guiLine(10,55,150,105,0);
                         guiLine(10,55,10,105,1);
                         guiLine(150,55,150,105,1);
                         guiLine(10,55,150,55,1);
                         guiLine(10,105,150,105,1);
                         guiDisplay(15,70,"模块协议设置成功",1);
                         lcdRefresh(10,120);
                             
                         menuInLayer--;
                         return;
      	    	 	 	  	   break;
      	    	 	 	  }
      	    	 	 	  break;
									
									case 3:    //线路控制点状态
										if (queryMpLink!=NULL)
										{
										  if (xlcForwardFrame(queryMpLink->port-1, queryMpLink->addr, keyLeftRight)==TRUE)
											{
												guiLine(10,55,150,105,0);
												guiLine(10,55,10,105,1);
												guiLine(150,55,150,105,1);
												guiLine(10,55,150,55,1);
												guiLine(10,105,150,105,1);
												if (keyLeftRight<2)
												{
												  guiDisplay(52,70,"分闸...",1);
												}
												else
												{
												  if (keyLeftRight<4)
													{
													  guiDisplay(52,70,"合闸...",1);
													}
													else
													{
													  guiDisplay(52,70,"自动控制...",1);
													}
												}
												lcdRefresh(50,110);
											}
										}
										break;
      	    	 	}
      	    	 	break;
      	    	 	
            	case 4:    //菜单第4层
             	 	switch(layer1MenuLight)
             	 	{
             	 	 	case 1: //参数设置与查看
             	 	 	  if (inputStatus == STATUS_NONE)
             	 	 	  {
                      keyLeftRight = 0;
                      layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	 	commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  }
             	 	 	  if (inputStatus == STATUS_SELECT_CHAR)
             	 	 	  {
             	 	 	  	inputStatus = STATUS_NONE;
             	 	 	  	commParaItem[1][inputIndex] = character[selectIndex];
             	 	 	  	inputApn(inputIndex);
             	 	 	  }
             	 	 	  break;
             	 	 	  
           	 	 	  case 2: //终端管理与维护
           	 	 	  	switch(layer2MenuLight[layer1MenuLight])
           	 	 	  	{
           	 	 	  		case 0:  //实时抄表
           	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
           	 	 	  			{
           	 	 	  				 case 0:
                          	 tmpData = (singleCopyMp[0]-0x30)*1000+(singleCopyMp[1]-0x30)*100
                          	         + (singleCopyMp[2]-0x30)*10 +(singleCopyMp[3]-0x30);
                          	 if (tmpData>2040)
                          	 {
           	 	 	  			      #ifdef LIGHTING
           	 	 	  			       guiDisplay(20,120,"控制点输入错误!",1);
           	 	 	  			      #else
           	 	 	  			       guiDisplay(20,120,"测量点输入错误!",1);
           	 	 	  			      #endif
           	 	                 lcdRefresh(120,140);
                          	 }
                          	 else
                          	 {             	 	 	  			       
           	 	                 strcpy(singleCopyTime,"-----");
           	 	                 strcpy(singleCopyEnergy,"-----");
           	 	                #ifdef LIGHTING
           	 	                 singleCcbCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
           	 	 	  			       guiDisplay(44,120,"查询中...",1);
           	 	                #else 
           	 	                 singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
           	 	 	  			       guiDisplay(44,120,"抄表中...",1);
           	 	 	  			      #endif
           	 	                 lcdRefresh(120,140);
           	 	                 
                          	   pDotCopy = (DOT_COPY *)malloc(sizeof(DOT_COPY));
                          	   pDotCopy->dotCopyMp  = tmpData;
                          	   pDotCopy->dotCopying = FALSE;
                          	   pDotCopy->port       = PORT_POWER_CARRIER;
                          	   pDotCopy->dataFrom   = DOT_CP_SINGLE_MP;
                          	   pDotCopy->outTime    = nextTime(sysTime,0,22);
                          	   pDotCopy->dotResult  = RESULT_NONE;
                          	 }
           	 	               break;
           	 	               
           	 	             case 1:
           	 	             	 if (keyLeftRight==0)
           	 	             	 {
                               while(mpCopyHead!=NULL)
                               {
                               	  prevMpCopy = mpCopyHead;
                               	  mpCopyHead = mpCopyHead->next;
                               	  free(mpCopyHead);
                               }
                               
                               queryMpLink = initPortMeterLink(30);
           	 	 	  			       tmpMpCopy = mpCopyHead;
           	 	 	  			       multiCpMax = 0;
           	 	 	  			       while (queryMpLink!=NULL)
           	 	 	  			       {
           	 	 	  			       	  tmpMpCopy = (struct carrierMeterInfo *)malloc(sizeof(struct carrierMeterInfo));
           	 	 	  			       	  tmpMpCopy->mp = queryMpLink->mp;
           	 	 	  			       	  tmpMpCopy->copyTime[0] = 0xee;
           	 	 	  			       	  tmpMpCopy->copyTime[1] = 0xee;
           	 	 	  			       	  tmpMpCopy->copyEnergy[0] = 0xee;
           	 	 	  			       	  tmpMpCopy->next = NULL;
           	 	 	  			       	  
           	 	 	  			       	  if (mpCopyHead==NULL)
                                  {
                                  	mpCopyHead = tmpMpCopy;
                                  }
                                  else
                                  {
                                  	prevMpCopy->next = tmpMpCopy;
                                  }
                                  prevMpCopy = tmpMpCopy;
                                  
           	 	 	  			          tmpPrevMpLink = queryMpLink;
           	 	 	  			          queryMpLink = queryMpLink->next;
                                  
           	 	 	  			          multiCpMax++;
                                  free(tmpPrevMpLink);
           	 	 	  			       }
           	 	 	  			       
           	 	 	  			       if ((multiCpMax%(NUM_MP_PER_PAGE-2))!=0)
           	 	 	  			       {
           	 	 	  			       	  multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-2)+1;
           	 	 	  			       }
           	 	 	  			       else
           	 	 	  			       {
           	 	 	  			       	  multiCpMax = multiCpMax/(NUM_MP_PER_PAGE-2);
           	 	 	  			       }
           	 	 	  			       
           	 	 	  			       if (mpCopyHead!=NULL)
           	 	 	  			       {
                          	      multiCpUpDown = 0;
                          	      
                          	      pDotCopy = (DOT_COPY *)malloc(sizeof(DOT_COPY));
                          	      prevMpCopy = mpCopyHead;
                          	      pDotCopy->dotCopyMp  = prevMpCopy->mp;
                          	      pDotCopy->dotCopying = FALSE;
                          	      pDotCopy->port       = PORT_POWER_CARRIER;
                          	      pDotCopy->dataFrom   = DOT_CP_ALL_MP;
                          	      pDotCopy->outTime    = nextTime(sysTime,0,22);
                          	      pDotCopy->dotResult  = RESULT_NONE;
           	 	 	  			       }
           	 	 	  			       
           	 	 	  			       keyLeftRight = 0xff;
           	 	 	  			       allMpCopy(keyLeftRight);
           	 	             	 }
           	 	             	 else
           	 	             	 {
           	 	             	 	  if (keyLeftRight!=0xff)
           	 	             	 	  {
           	 	             	 	    realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
           	 	             	 	  }
           	 	             	 }
           	 	             	 break;
           	 	          }
           	 	 	  		 	break;
           	 	 	  	}
           	 	 	  	break;
             	 	}
             	 	break;
      	    		 	 
      	    	case 12:  //菜单第12层(仅二层菜单第2项(参数设置与查看)扩展有效)
      	    		switch (layer2xMenuLight)
      	    	  {
      	    	  	case 0:  //参数查看
      	    	  		layer2xMenuLight = 0;
      	    	  		layer2xMenu(1, layer2xMenuLight);
      	    	  		break;
      	    	  		
      	    	  	case 1:  //参数设置
      	    		 		pwLight = 0;
      	    		 	 	strcpy(passWord, "000000");
      	    		 	 	inputPassWord(pwLight);
      	    		 	 	break;
      	    		}
      	    		break;

      	    	case 13:  //菜单第13层(仅二层菜单第2项(参数设置与查看)扩展有效)
      	    		switch (layer2xMenuLight)
      	    	  {
      	    	  	case 0:    //查询测量点参数
      	    	  		queryMpLink = initPortMeterLink(0xff);
      	    	  		mpQueryType = 0;
      	    	  		mpQueryLight = 0;
      	    	  		
                    mpQueryPage = meterDeviceNum/(NUM_MP_PER_PAGE-1);
                    if (meterDeviceNum%(NUM_MP_PER_PAGE-1)!=0)
                    {
                   	  mpQueryPage++;
                    }

      	    	  		mpQueryMenu(mpQueryType, mpQueryLight);
      	    	  		break;
      	    	  		
      	    	  	case 1:    //查询通信通道参数
      	    		 		commParaQueryMenu();
      	    		 	 	break;
      	    		 	
      	    		 #ifdef LIGHTING
      	    		 	case 2:    //查询控制时段
	                  tmpCTimesNode = cTimesHead;
      	    		 		queryCTimes(tmpCTimesNode);
	                  break;
      	    		 #endif
      	    		}
      	    		break;
      	    		
      	    	case 20:  //菜单第20层(输入密码)的确认
      	    		for(i=0;i<6;i++)
      	    		{
      	    		 	 if (originPassword[i]!=passWord[i])
      	    		 	 {
      	    		 	  	guiDisplay(30,120,"密码输入错误!",1);
      	    		 	  	lcdRefresh(120,137);
      	    		 	  	return;
      	    		 	 }
      	    		}
      	    		 
      	    		//根据输入密码前的高亮菜单项,执行相应的动作
      	    		if (layer1MenuLight==1 || layer1MenuLight==2)
      	    		{
      	    			if (layer2MenuLight[layer1MenuLight]==0xff)
      	    			{
      	    				 layer2MenuLight[layer1MenuLight] = 0;
      	    			}
      	    			
      	    			if (layer1MenuLight==1 && carrierModuleType==RL_WIRELESS)
      	    			{
      	    				layer2MenuNum[1] = 10;
      	    			}
      	    			else
      	    			{
      	    			 #ifndef LIGHTING
      	    				layer2MenuNum[1] = 9;
      	    			 #endif
      	    			}
      	    			layer2Menu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	    		}
      	    }
      	    break;

      	  case KEY_CANCEL:   //取消
      	    switch(menuInLayer)
      	    {
      	    	case 1:
      	    		defaultMenu();
      	    		break;
      	    		 	 
      	    	case 2:     //菜单第2层
      	    	case 12:    //菜单第12层(仅二层菜单第2项(参数设置与查看)扩展有效)
      	    	case 13:    //菜单第13层(仅二层菜单第2项(参数设置与查看)扩展有效)
      	    	case 20:    //菜单第20层,输入密码层
      	    		layer1Menu(layer1MenuLight);
      	    		break;
      	    		 	 
      	    	case 3:     //菜单第3层
								if (3==layer1MenuLight)
								{
								  xlcStatus();
								}
								else
								{
								  layer2Menu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
								}
      	    		break;
            	
            	case 4:    //菜单第4层
             	 	switch(layer1MenuLight)
             	 	{
             	 	 	case 1: //通信参数设置
      	    		 	 	if (inputStatus==STATUS_SELECT_CHAR)
      	    		 	 	{
      	    		 	 	 	inputStatus = STATUS_NONE;
      	    		 	 	 	inputApn(inputIndex);
      	    		 	 	}
      	    		 	 	else
      	    		 	 	{
      	    		 	 	 	commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		 	 	}
             	 	 	  break;
             	 	 	
             	 	 	case 2: //终端管理与维护
             	 	 	  switch(layer2MenuLight[layer1MenuLight])
             	 	 	  {
             	 	 	  	case 0:    //实时抄表
             	 	 	  	 #ifdef LIGHTING
             	 	 	  	  layer2Menu(layer2MenuLight[layer1MenuLight], layer1MenuLight);
             	 	 	  	 #else
             	 	 	  		realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  	 #endif
             	 	 	  		break;
             	 	 	  		 	
             	 	 	  	case 1:    //全体测量点抄表结果
      	    		 	 	    layer2Menu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
             	 	 	  		break;
             	 	 	  }
             	 	 	  break;
             	 	}
             	 	break;
             	 
             	case 14:
             	  layer2xMenu(1, layer2xMenuLight);
             	  break;    
      	    }
      	    break;
      	    	
          case KEY_UP:    //向上
           	 switch(menuInLayer)
             {
             	 case 1:
             	 	 if (layer1MenuLight==0)
             	 	 {
             	 	 	  layer1MenuLight = MAX_LAYER1_MENU-1;
             	 	 }
             	 	 else
             	 	 {
             	 	 	  layer1MenuLight--;
             	 	 }
             	 	 layer1Menu(layer1MenuLight);
             	   break;
             	   
             	 case 2:    //菜单第2层
             	 	 switch (layer1MenuLight)
             	 	 {
             	 	 	 case 0:    //抄表查询
             	 	 	   switch(layer2MenuLight[layer1MenuLight])
             	 	 	   {
             	 	 	  		case 0: //1-1测量点有功抄表查询
             	 	 	  		 #ifdef LIGHTING
             	 	 	  		  ;    //单链表的向上翻不好处理,还没做
             	 	 	  		 #else
             	 	 	  		 
             	 	 	  		 	stringUpDown(queryTimeStr, layer3MenuLight[0][0], 0);  //高亮的字符增大一个字符
             	 	 	  		 	copyQueryMenu(layer2MenuLight[0], layer3MenuLight[0][0]);
             	 	 	  		 
             	 	 	  		 #endif
             	 	 	  		  break;

             	 	 	  		default:
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
                            layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]--;
             	 	 	  		 	}
             	 	 	  		 	copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	break;
             	 	 	   }
             	 	 	   break;

             	 	 	 case 1:
             	 	 	 case 2:
       	  	 	 	     if (layer2MenuLight[layer1MenuLight] == 0 || layer2MenuLight[layer1MenuLight] == 0xff)
       	  	 	 	     {
       	  	 	 	       layer2MenuLight[layer1MenuLight] = layer2MenuNum[layer1MenuLight]-1;
       	  	 	 	     }
       	  	 	 	     else
       	  	 	 	     {
       	  	 	 	       layer2MenuLight[layer1MenuLight]--;
       	  	 	 	     }
       	  	 	 	     layer2Menu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
       	  	 	 	     break;
             	 	 }
             	 	 break;
             	 	 
             	 case 3:    //菜单第3层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 1:  //参数设置与查看
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		 case 0: //通信参数设置
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
             	 	 	  		 	   if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
             	 	 	  		 	   {
             	 	 	  		 	 	    for(i=strlen(commParaItem[1]);i<16;i++)
             	 	 	  		 	 	    {
             	 	 	  		 	 	    	 commParaItem[1][i] = ' ';
             	 	 	  		 	 	    }
             	 	 	  		 	 	    commParaItem[1][16] = '\0';
             	 	 	  		 	 	    inputStatus=STATUS_NONE;
             	 	 	  		 	 	    inputIndex = 0;
             	 	 	  		 	 	    inputApn(inputIndex); //APN的输入处理
             	 	 	  		 	   }
             	 	 	  		 	   else
             	 	 	  		 	   {
      	    		               stringUpDown(commParaItem[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
      	    		               commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		             }
      	    		           }
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 1: //台区电表参数设置
  	    		               stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
         	 	 	  		 	     {
         	 	 	  		 	     	 if (chrMp[1][0]>0x37)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[1][0] = 0x30;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
         	 	 	  		 	     {
         	 	 	  		 	     	 if (chrMp[2][0]>0x32)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[2][0] = 0x30;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
         	 	 	  		 	     {
         	 	 	  		 	     	 if (chrMp[3][0]>0x32)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[3][0] = 0x30;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
  	    		               set485Meter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 2: //集抄电表参数设置
         	 	 	  		 	    #ifdef LIGHTING
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
         	 	 	  		 	     {
  	    		                 stringUpDown(chrMp[4], keyLeftRight, 0);
         	 	 	  		 	     	 if (chrMp[4][0]>0x34)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[4][0] = 0x31;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
         	 	 	  		 	     else
         	 	 	  		 	     {
           	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
           	 	 	  		 	     {
    	    		                 stringUpDown(chrMp[3], keyLeftRight, 0);
           	 	 	  		 	     	 if (chrMp[3][0]>0x32)
           	 	 	  		 	     	 {
           	 	 	  		 	     	 	 chrMp[3][0] = 0x30;
           	 	 	  		 	     	 }
           	 	 	  		 	     }
           	 	 	  		 	     else
           	 	 	  		 	     {
             	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==4)
             	 	 	  		 	     {
      	    		                 stringUpDown(chrMp[2], keyLeftRight, 0);
             	 	 	  		 	     	 if (chrMp[2][0]>0x33)
             	 	 	  		 	     	 {
             	 	 	  		 	     	 	 chrMp[2][0] = 0x31;
             	 	 	  		 	     	 }
             	 	 	  		 	     }
             	 	 	  		 	     else
             	 	 	  		 	     {
  	    		                     stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
  	    		                   }
  	    		                 }
         	 	 	  		 	     }
         	 	 	  		 	    #else
  	    		               stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
         	 	 	  		 	     {
         	 	 	  		 	     	 if (chrMp[2][0]>0x31)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[2][0] = 0x30;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
         	 	 	  		 	    #endif
         	 	 	  		 	    
  	    		               setCarrierMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 3: //终端时间设置
      	    		           stringUpDown(dateTimeItem,keyLeftRight,0);
      	    		           setTeDateTime(keyLeftRight);
             	 	 	  		 	 break;

             	 	 	  		 case 4: //修改界面密码
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {                                 	    		           
      	    		             stringUpDown(commParaItem[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
      	    		             modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
             	 	 	  		 	 break;

             	 	 	  		 case 5: //终端编号设置
      	    		           stringUpDown(tmpTeAddr,keyLeftRight,0);
      	    		           setTeAddr(keyLeftRight);
             	 	 	  		 	 break;

             	 	 	  		 case 6: //以太网参数设置
      	    		           if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
      	    		           {
      	    		           	 if (chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]][0]==0x55)
      	    		           	 {
      	    		           	 	 chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]][0] = 0x0;
      	    		           	 }
      	    		           	 else
      	    		           	 {
      	    		           	   chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]][0] = 0x55;
      	    		           	 }
      	    		           }
      	    		           else
      	    		           {
      	    		             stringUpDown(chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
      	    		           }
      	    		           setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 
             	 	 	  		 case 7: //VPN用户名密码
                   	 	 	   if (inputStatus==STATUS_SELECT_CHAR)
                   	 	 	   {
                   	 	 	  	 if (selectIndex>=14)
                   	 	 	  	 {
                   	 	 	  		 selectIndex -= 14;
                   	 	 	  	 }
                   	 	 	  	 else
                   	 	 	  	 {
                   	 	 	  		 selectIndex = 68;
                   	 	 	  	 }
                   	 	 	  	 selectChar(selectIndex);
                   	 	 	   }
                   	 	 	   if (inputStatus==STATUS_NONE)
                   	 	 	   {
                   	 	 	  	 selectIndex = 0;
                             selectChar(selectIndex);
                           }
                   	 	 	   break;

             	 	 	  		 case 8: //级联参数设置
  	    		               if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>1 && layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<5)
  	    		               {
  	    		               	 stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
  	    		               }
  	    		               else
  	    		               {
  	    		               	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
  	    		               	 {
  	    		               	 	  if (chrMp[0][0]!='3')
  	    		               	 	  {
  	    		               	 	  	 chrMp[0][0]='3';
  	    		               	 	  }
  	    		               	 }
  	    		               	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
  	    		               	 {
  	    		               	 	 if (chrMp[1][0]>0x31)
  	    		               	 	 {
  	    		               	 	  	chrMp[1][0]='0';
  	    		               	 	 }
  	    		               	 	 else
  	    		               	 	 {
  	    		               	 	  	chrMp[1][0]++;
  	    		               	 	 }
  	    		               	 }
  	    		               }
  	    		               setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 
             	 	 	  		 case 9: //锐拔模块参数设置
  	    		               stringUpDown(chrRlPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
  	    		               setRlPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2:  //终端管理与维护
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //实时抄表
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
             	 	 	  		 	{
													  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1;
												  }
												  else
												  {
													  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]--;
												  }
												  realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
												  break;             	 	 	  		 	 
             	 	 	  		 	 
             	 	 	  		case 3:  //新增电能表地址
           	 	 	  				if (multiCpUpDown==0)
           	 	 	  				{
           	 	 	  				 	multiCpUpDown =  multiCpMax-1;
           	 	 	  				}
           	 	 	  				else
           	 	 	  				{
           	 	 	  				 	multiCpUpDown--;
           	 	 	  				}
           	 	 	  				 
           	 	 	  				newAddMeter(keyLeftRight);
             	 	 	  		 	break;
             	 	 	  		 	 
             	 	 	  		case 4:   //新增电能表抄表状态
           	 	 	  				if (multiCpUpDown==0)
           	 	 	  				{
           	 	 	  				 	multiCpUpDown = multiCpMax-1;
           	 	 	  				}
           	 	 	  				else
           	 	 	  				{
           	 	 	  				 	multiCpUpDown--;
           	 	 	  				}
           	 	 	  				 
           	 	 	  				newMeterCpStatus(keyLeftRight);
             	 	 	  		 	break;
													
												case 5:
												 #ifdef LIGHTING
												 	if (0==keyLeftRight)
												 	{
												 	  keyLeftRight = 7;
												 	}
													else
												 	{
												 	 	keyLeftRight--;
												 	}
													setLearnIr(keyLeftRight);
												 #endif
													 break;
        	    	 	 	  	 
        	    	 	 	  	case 7:  //本地通信模块抄读方式
        	    	 	 	  	  if (keyLeftRight==0)
        	    	 	 	  	  {
        	    	 	 	  	    keyLeftRight = 1;
        	    	 	 	  	  }
        	    	 	 	  	  else
        	    	 	 	  	  {
        	    	 	 	  	  	keyLeftRight = 0;
        	    	 	 	  	  }
        	    	 	 	  	  setCopyForm(keyLeftRight);
        	    	 	 	  	  break;

        	    	 	 	  	 case 11:  //居民用户表数据类型
        	    	 	 	  	   if (keyLeftRight==0)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 2;
        	    	 	 	  	   }
        	    	 	 	  	   else
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight--;
        	    	 	 	  	   }
        	    	 	 	  	   setDenizenDataType(keyLeftRight);
        	    	 	 	  	   break;
        	    	 	 	  	 
        	    	 	 	  	 case 12:  //轮显设置
        	    	 	 	  	   if (keyLeftRight==0)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 1;
        	    	 	 	  	   }
        	    	 	 	  	   else
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight--;
        	    	 	 	  	   }
        	    	 	 	  	   setCycleType(keyLeftRight);
        	    	 	 	  	   break;

      	    	 	 	  	   case 13:    //维护接口模式设置
        	    	 	 	  	   if (keyLeftRight==0)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 1;
        	    	 	 	  	   }
        	    	 	 	  	   else
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight--;
        	    	 	 	  	   }
      	    	 	 	  	  	 setMainTain(keyLeftRight);      	    	 	 	  	  	
      	    	 	 	  	  	 break;

      	    	 	 	  	   case 14:    //第2路485口功能设置
        	    	 	 	  	   if (keyLeftRight==0)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 1;
        	    	 	 	  	   }
        	    	 	 	  	   else
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight--;
        	    	 	 	  	   }
      	    	 	 	  	  	 setRs485Port2(keyLeftRight);      	    	 	 	  	  	
      	    	 	 	  	  	 break;
      	    	 	 	  	  	 
      	    	 	 	  	   case 15:    //本地通信模块协议设置
        	    	 	 	  	   if (keyLeftRight==0)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 1;
        	    	 	 	  	   }
        	    	 	 	  	   else
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight--;
        	    	 	 	  	   }
      	    	 	 	  	  	 setLmProtocol(keyLeftRight);      	    	 	 	  	  	
      	    	 	 	  	  	 break;
             	 	 	  	}
             	 	 	  	break;
									  
										case 3:    //线路控制点状态
									    if (0==keyLeftRight)
											{
												keyLeftRight = 4;	
											}
											else
											{
											  keyLeftRight--;	
											}
										  xlOpenClose(keyLeftRight);
									    break;
             	 	 }
             	 	 break;
             	 	 
            	 case 4:    //菜单第4层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 1: //参数设置与查看
             	 	 	  	if (inputStatus==STATUS_SELECT_CHAR)
             	 	 	  	{
             	 	 	  		if (selectIndex>=14)
             	 	 	  		{
             	 	 	  			 selectIndex -= 14;
             	 	 	  		}
             	 	 	  		else
             	 	 	  		{
             	 	 	  			 selectIndex = 68;
             	 	 	  	  }
             	 	 	  		selectChar(selectIndex);
             	 	 	  	}
             	 	 	  	if (inputStatus==STATUS_NONE)
             	 	 	  	{
             	 	 	  	  selectIndex = 0;
                        selectChar(selectIndex);
                      }
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2: //终端管理与维护
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //实时抄表
             	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
             	 	 	  		 	{
             	 	 	  		 	 	case 0:  //指定测量点抄表
             	 	 	  		 	 		stringUpDown(singleCopyMp, keyLeftRight, 0);
             	 	 	  		 	 		
             	 	 	  		 	 	 #ifdef LIGHTING	
             	 	 	  		 	 	  singleCcbCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	 #else
             	 	 	  		 	 	  singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	 #endif
             	 	 	  		 	 	  break;
             	 	 	  		 	 	  
             	 	 	  		 	 	case 1:
                 	 	 	  			if (keyLeftRight==0xff)
                 	 	 	  			{
                 	 	 	  				 if (multiCpUpDown==0)
                 	 	 	  				 {
                 	 	 	  				 	  multiCpUpDown=multiCpMax-1;
                 	 	 	  				 }
                 	 	 	  				 else
                 	 	 	  				 {
                 	 	 	  				 	  multiCpUpDown--;
                 	 	 	  				 }
                 	 	 	  				 
                 	 	 	  				 allMpCopy(keyLeftRight);
                 	 	 	  			}
                 	 	 	  			break;             	 	 	  		 	 		
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  		 	
             	 	 	  		case 1:  //全部测量点抄表结果
             	 	 	  		 	if (multiCpUpDown<1)
             	 	 	  		 	{
             	 	 	  		 	 	 multiCpUpDown=multiCpMax-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
             	 	 	  		 	 	 multiCpUpDown--;
             	 	 	  		 	}
             	 	 	  		 	allMpCopy(0xff);
             	 	 	  		 	break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;

      	    	 case 12:  //菜单第12层(仅二层菜单第2项(参数设置与查看)扩展有效)
      	    	 case 13:  //菜单第13层(仅二层菜单第2项(参数设置与查看)扩展有效)
      	    		 switch(layer1MenuLight)
      	    	   {
             	 	   case 1: //参数设置与查看
             	 	 		#ifdef LIGHTING
             	 	 		 if (13==menuInLayer)
             	 	 		 {
             	 	 		 	 if (layer2xMenuLight==0)
             	 	 		 	 {
             	 	 		 	 	 layer2xMenuLight = 2;
             	 	 		 	 }
             	 	 		 	 else
             	 	 		 	 {
             	 	 		 	   layer2xMenuLight--;
             	 	 		 	 }
             	 	 		 }
             	 	 		 else
             	 	 		 {
             	 	 		#endif
             	 	 		
             	 	 		   if (layer2xMenuLight==0)
             	 	 		   {
             	 	 		     layer2xMenuLight = 1;
             	 	 		   }
             	 	 		   else
             	 	 		   {
             	 	 			   layer2xMenuLight = 0;
             	 	 		   }
             	 	 		   
             	 	 		#ifdef LIGHTING
             	 	 		 }
             	 	 		#endif
             	 	 		
             	 	 		 if (menuInLayer==12)
             	 	 		 {
             	 	 		   layer2xMenu(0, layer2xMenuLight);
             	 	 		 }
             	 	 		 else
             	 	 		 {
             	 	 		   layer2xMenu(1, layer2xMenuLight);
             	 	 		 }
             	 	 		 break;
      	    	   }
      	    		 break;

      	    	 case 14:  //菜单第14层(仅二层菜单第2项(参数设置与查看)扩展有效)
      	    		 switch (layer2xMenuLight)
      	    	   {
      	    	  	 case 0:  //查询测量点参数
      	    	  		 if (mpQueryLight==0)
      	    	  		 {
      	    	  		    mpQueryLight = mpQueryPage-1;
      	    	  		 }
      	    	  		 else
      	    	  		 {
      	    	  		 	  mpQueryLight--;
      	    	  		 }
      	    	  		 mpQueryMenu(mpQueryType, mpQueryLight);
      	    	  		 break;
      	    	  		 
      	    		  #ifdef LIGHTING
      	    		 	 case 2:    //查询控制时段
	                   if (tmpCTimesNode->next==NULL)
	                   {
	                     tmpCTimesNode = cTimesHead;
	                   }
	                   else
	                   {
	                     tmpCTimesNode = tmpCTimesNode->next;
	                   }
      	    		 		 queryCTimes(tmpCTimesNode);
	                   break;
      	    		  #endif
      	    	   }
      	    	   break;

             	 case 20:   //菜单第20层
             	 	 stringUpDown(passWord, pwLight,0);
             	 	 inputPassWord(pwLight);
             	 	 break;
             }
           	 break;
          
          case KEY_DOWN:    //向下
           	switch(menuInLayer)
            {
             	 case 1:    //菜单第1层
             	 	 if (layer1MenuLight>=MAX_LAYER1_MENU-1)
             	 	 {
             	 	 	  layer1MenuLight = 0;
             	 	 }
             	 	 else
             	 	 {
             	 	 	  layer1MenuLight++;
             	 	 }
             	 	 layer1Menu(layer1MenuLight);
             	   break;

             	 case 2:    //菜单第2层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	 case 0:    //抄表查询
             	 	 	   switch(layer2MenuLight[layer1MenuLight])
             	 	 	   {
             	 	 	  	 case 0: //1-1测量点有功抄表查询
             	 	 	  	 	#ifdef LIGHTING
             	 	 	  	 	 
             	 	 	  	 	 if (queryMpLink->next!=NULL)
             	 	 	  	 	 {
             	 	 	  	 	 	 queryMpLink = queryMpLink->next;
             	 	 	  	 	 }
             	 	 	  	 	 else
             	 	 	  	 	 {
             	 	 	  	 	 	 queryMpLink = copyCtrl[4].cpLinkHead;
             	 	 	  	 	 }
             	 	 	  	 	 ccbStatus();
             	 	 	  	 	 
             	 	 	  	 	#else
             	 	 	  	 	
             	 	 	  		 stringUpDown(queryTimeStr, layer3MenuLight[0][0], 1);  //高亮的字符减小一个数字
             	 	 	  		 copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		
             	 	 	  		#endif
             	 	 	  		 break;
             	 	 	  	
             	 	 	  	 default:
             	 	 	  		 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>=layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
             	 	 	  		 {
             	 	 	  		 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
             	 	 	  		 }
             	 	 	  		 else
             	 	 	  		 {
                           layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
             	 	 	  		 }
             	 	 	  		 copyQueryMenu(layer2MenuLight[layer1MenuLight],layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 break;
             	 	 	   }
             	 	 	   break;

             	 	 	 case 1:
             	 	 	 case 2:
       	  	 	 	     if (layer2MenuLight[layer1MenuLight]+1>layer2MenuNum[layer1MenuLight]-1)
       	  	 	 	     {
       	  	 	 	       layer2MenuLight[layer1MenuLight] = 0;
       	  	 	 	     }
       	  	 	 	     else
       	  	 	 	     {
       	  	 	 	       layer2MenuLight[layer1MenuLight]++;
       	  	 	 	     }
       	  	 	 	     layer2Menu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
       	  	 	 	     break;
       	  	 	 	     
             	 	 	#ifdef LIGHTING
             	 	 	 case 3:
             	 	 	   if (queryMpLink->next!=NULL)
             	 	 	   {
             	 	 	  	 queryMpLink = queryMpLink->next;
             	 	 	   }
             	 	 	   else
             	 	 	   {
             	 	 	   	 queryMpLink = xlcLink;
             	 	 	   }
             	 	 	   xlcStatus();             	 	 	  	 	 
             	 	 	   break;

             	 	 	 case 4:
             	 	 	   if (queryMpLink->next!=NULL)
             	 	 	   {
             	 	 	  	 queryMpLink = queryMpLink->next;
             	 	 	   }
             	 	 	   else
             	 	 	   {
             	 	 	   	 queryMpLink = ldgmLink;
             	 	 	   }
             	 	 	   ldgmStatus();             	 	 	  	 	 
             	 	 	   break;
             	 	 	   
             	 	 	 case 5:
             	 	 	   if (queryMpLink->next!=NULL)
             	 	 	   {
             	 	 	  	 queryMpLink = queryMpLink->next;
             	 	 	   }
             	 	 	   else
             	 	 	   {
             	 	 	   	 queryMpLink = lsLink;
             	 	 	   }
             	 	 	   lsStatus();
             	 	 	   break;
             	 	 	#endif
             	 	 	  		  
             	 	 }
             	 	 break;
             	 	 
             	 case 3:    //菜单第3层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 1:  //参数设置与查看
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		 case 0: //通信参数设置
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
             	 	 	  		 	   if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
             	 	 	  		 	   {
             	 	 	  		 	 	    //APN的输入处理
             	 	 	  		 	 	    for(i=strlen(commParaItem[1]);i<16;i++)
             	 	 	  		 	 	    {
             	 	 	  		 	 	    	 commParaItem[1][i] = ' ';
             	 	 	  		 	 	    }
             	 	 	  		 	 	    commParaItem[1][16] = '\0';
             	 	 	  		 	 	    inputStatus=STATUS_NONE;
             	 	 	  		 	 	    inputIndex = 0;
             	 	 	  		 	 	    inputApn(inputIndex);
             	 	 	  		 	   }
             	 	 	  		 	   else
             	 	 	  		 	   {
      	    		                stringUpDown(commParaItem[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
      	    		                commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		             }
      	    		           }
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 1: //台区电表参数设置
  	    		               stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
         	 	 	  		 	     {
         	 	 	  		 	     	 if (chrMp[1][0]>0x37)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[1][0] = 0x37;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
         	 	 	  		 	     {
         	 	 	  		 	     	 if (chrMp[2][0]>0x32)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[2][0] = 0x32;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
         	 	 	  		 	     {
         	 	 	  		 	     	 if (chrMp[3][0]>0x32)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[3][0] = 0x32;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
  	    		               set485Meter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 2: //集抄电表参数设置
         	 	 	  		 	    #ifdef LIGHTING
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
         	 	 	  		 	     {
  	    		                 stringUpDown(chrMp[4], keyLeftRight, 1);
         	 	 	  		 	     	 if (chrMp[4][0]<=0x31)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[4][0] = 0x34;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
         	 	 	  		 	     else
         	 	 	  		 	     {
           	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
           	 	 	  		 	     {
    	    		                 stringUpDown(chrMp[3], keyLeftRight, 1);
           	 	 	  		 	     	 if (chrMp[3][0]<0x30 || chrMp[3][0]>0x32)
           	 	 	  		 	     	 {
           	 	 	  		 	     	 	 chrMp[3][0] = 0x32;
           	 	 	  		 	     	 }
           	 	 	  		 	     }
           	 	 	  		 	     else
           	 	 	  		 	     {
             	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==4)
             	 	 	  		 	     {
      	    		                 stringUpDown(chrMp[2], keyLeftRight, 1);
             	 	 	  		 	     	 if (chrMp[2][0]<0x31)
             	 	 	  		 	     	 {
             	 	 	  		 	     	 	 chrMp[2][0] = 0x33;
             	 	 	  		 	     	 }
             	 	 	  		 	     }
             	 	 	  		 	     else
             	 	 	  		 	     {
             	 	 	  		 	     	 stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
             	 	 	  		 	     }
             	 	 	  		 	   }
         	 	 	  		 	     }
         	 	 	  		 	    #else 
  	    		               stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
         	 	 	  		 	     if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
         	 	 	  		 	     {
         	 	 	  		 	     	 if (chrMp[2][0]>0x31)
         	 	 	  		 	     	 {
         	 	 	  		 	     	 	 chrMp[2][0] = 0x31;
         	 	 	  		 	     	 }
         	 	 	  		 	     }
         	 	 	  		 	    #endif
  	    		               setCarrierMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 3: //终端时间设置
      	    		           stringUpDown(dateTimeItem,keyLeftRight,1);
      	    		           setTeDateTime(keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 4: //修改界面密码
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
      	    		             stringUpDown(commParaItem[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
      	    		             modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 }
             	 	 	  		 	 break;

             	 	 	  		 case 5: //终端编号设置
      	    		           stringUpDown(tmpTeAddr,keyLeftRight,1);
      	    		           setTeAddr(keyLeftRight);
             	 	 	  		 	 break;

             	 	 	  		 case 6: //以太网参数设置
      	    		           if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
      	    		           {
      	    		           	  if (chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]][0]==0x55)
      	    		           	  {
      	    		           	  	chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]][0] = 0x0;
      	    		           	  }
      	    		           	  else
      	    		           	  {
      	    		           	  	chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]][0] = 0x55;
      	    		           	  }
      	    		           }
      	    		           else
      	    		           {
      	    		             stringUpDown(chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
      	    		           }
      	    		           setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 
             	 	 	  		 case 7:  //VPN用户名密码
                 	 	 	  	 if (inputStatus==STATUS_SELECT_CHAR)
                 	 	 	  	 {
                 	 	 	  		 selectIndex += 14;
                 	 	 	  		 if (selectIndex>69)
                 	 	 	  		 {
                 	 	 	  			  selectIndex = 0;
                 	 	 	  		 }
                 	 	 	  		 selectChar(selectIndex);
                 	 	 	  	 }
                 	 	 	  	 if (inputStatus==STATUS_NONE)
                 	 	 	  	 {
                 	 	 	  	   selectIndex = 0;
                             selectChar(selectIndex);
                           }
                 	 	 	  	 break;
                 	 	 	  	 
             	 	 	  		 case 8: //级联参数设置
  	    		               if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>1 && layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<5)
  	    		               {
  	    		               	 stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
  	    		               }
  	    		               else
  	    		               {
  	    		               	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
  	    		               	 {
  	    		               	 	  if (chrMp[0][0]!='3')
  	    		               	 	  {
  	    		               	 	  	 chrMp[0][0]='3';
  	    		               	 	  }
  	    		               	 }
  	    		               	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
  	    		               	 {
  	    		               	 	 if (chrMp[1][0]=='0')
  	    		               	 	 {
  	    		               	 	  	chrMp[1][0]='2';
  	    		               	 	 }
  	    		               	 	 else
  	    		               	 	 {
  	    		               	 	  	chrMp[1][0]--;
  	    		               	 	 }
  	    		               	 }
  	    		               }
  	    		               setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 9: //锐拔模块参数设置
  	    		               stringUpDown(chrRlPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
  	    		               setRlPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2:  //终端管理与维护
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		 case 0: //实时抄表
             	 	 	  		 	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>=layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
             	 	 	  		 	 {
             	 	 	  		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
             	 	 	  		 	 }
             	 	 	  		 	 else
             	 	 	  		 	 {
             	 	 	  		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
             	 	 	  		 	 }
             	 	 	  		 	 realCopyMeterMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 3:  //新增电能表地址
           	 	 	  				 if (multiCpUpDown>=(multiCpMax-1))
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown=0;
           	 	 	  				 }
           	 	 	  				 else
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown++;
           	 	 	  				 }
           	 	 	  				 
           	 	 	  				 newAddMeter(keyLeftRight);
             	 	 	  		 	 break;
             	 	 	  		 	 
             	 	 	  		 case 4:   //新增电能表抄表状态
           	 	 	  				 if (multiCpUpDown>=(multiCpMax-1))
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown=0;
           	 	 	  				 }
           	 	 	  				 else
           	 	 	  				 {
           	 	 	  				 	  multiCpUpDown++;
           	 	 	  				 }
           	 	 	  				 
           	 	 	  				 newMeterCpStatus(keyLeftRight);
             	 	 	  		 	 break;
													 
												 case 5:
												 	#ifdef LIGHTING
												 	 if (keyLeftRight>=7)
												 	 {
												 	 	 keyLeftRight = 0;
												 	 }
													 else
												 	 {
												 	 	 keyLeftRight++;
												 	 }
													 setLearnIr(keyLeftRight);
												  #endif
													 break;

												 
        	    	 	 	  	 case 7:  //本地通信模块抄读方式
        	    	 	 	  	   if (keyLeftRight==0)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 1;
        	    	 	 	  	   }
        	    	 	 	  	   else
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 0;
        	    	 	 	  	   }
        	    	 	 	  	   setCopyForm(keyLeftRight);
        	    	 	 	  	   break;

        	    	 	 	  	 case 11:  //居民用户表数据类型
        	    	 	 	  	   keyLeftRight++;
        	    	 	 	  	   if (keyLeftRight>2)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 0;
        	    	 	 	  	   }
        	    	 	 	  	   setDenizenDataType(keyLeftRight);
        	    	 	 	  	   break;

        	    	 	 	  	 case 12:  //轮显设置
        	    	 	 	  	   keyLeftRight++;
        	    	 	 	  	   if (keyLeftRight>1)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 0;
        	    	 	 	  	   }
        	    	 	 	  	   setCycleType(keyLeftRight);
        	    	 	 	  	   break;

      	    	 	 	  	   case 13:    //维护接口模式设置
        	    	 	 	  	   keyLeftRight++;
        	    	 	 	  	   if (keyLeftRight>1)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 0;
        	    	 	 	  	   }
      	    	 	 	  	  	 setMainTain(keyLeftRight);      	    	 	 	  	  	
      	    	 	 	  	  	 break;

      	    	 	 	  	   case 14:    //第2路485口功能设置
        	    	 	 	  	   keyLeftRight++;
        	    	 	 	  	   if (keyLeftRight>1)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 0;
        	    	 	 	  	   }
      	    	 	 	  	  	 setRs485Port2(keyLeftRight);      	    	 	 	  	  	
      	    	 	 	  	  	 break;
      	    	 	 	  	  	 
      	    	 	 	  	   case 15:    //本地通信模块协议设置
        	    	 	 	  	   keyLeftRight++;
        	    	 	 	  	   if (keyLeftRight>1)
        	    	 	 	  	   {
        	    	 	 	  	  	 keyLeftRight = 0;
        	    	 	 	  	   }
      	    	 	 	  	  	 setLmProtocol(keyLeftRight);
      	    	 	 	  	  	 break;
             	 	 	  	}
             	 	 	  	break;
											
										case 3:    //线路控制点状态
									    if (keyLeftRight>3)
											{
												keyLeftRight=0;	
											}
											else
											{
											  keyLeftRight++;	
											}
										  xlOpenClose(keyLeftRight);
									    break;
             	 	 }
             	 	 break;
             	 	 
            	 case 4:    //菜单第4层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 1:   //参数设置与查看
             	 	 	  	if (inputStatus==STATUS_SELECT_CHAR)
             	 	 	  	{
             	 	 	  		selectIndex += 14;
             	 	 	  		if (selectIndex>69)
             	 	 	  		{
             	 	 	  			 selectIndex = 0;
             	 	 	  		}
             	 	 	  		selectChar(selectIndex);
             	 	 	  	}
             	 	 	  	if (inputStatus==STATUS_NONE)
             	 	 	  	{
             	 	 	  	  selectIndex = 0;
                        selectChar(selectIndex);
                      }
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2: //终端管理与维护
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //实时抄表
             	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
             	 	 	  		 	{
             	 	 	  		 	 	case 0:  //指定测量点抄表
             	 	 	  		 	 		stringUpDown(singleCopyMp, keyLeftRight, 1);
             	 	 	  		 	 	 #ifdef LIGHTING
             	 	 	  		 	 	  singleCcbCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	 #else
             	 	 	  		 	 	  singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	 #endif
             	 	 	  		 	 	  break;
             	 	 	  		 	 	  
             	 	 	  		 	 	case 1:
                 	 	 	  			if (keyLeftRight==0xff)
                 	 	 	  			{
                 	 	 	  				 if (multiCpUpDown>=(multiCpMax-1))
                 	 	 	  				 {
                 	 	 	  				 	  multiCpUpDown=0;
                 	 	 	  				 }
                 	 	 	  				 else
                 	 	 	  				 {
                 	 	 	  				 	  multiCpUpDown++;
                 	 	 	  				 }
                 	 	 	  				 
                 	 	 	  				 allMpCopy(keyLeftRight);
                 	 	 	  			}
                 	 	 	  			break;
             	 	 	  		 	}
             	 	 	  		 	break;

             	 	 	  		case 1:  //全部测量点抄表结果
             	 	 	  		 	if (multiCpUpDown>=multiCpMax-1)
             	 	 	  		 	{
             	 	 	  		 	 	 multiCpUpDown=0;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
             	 	 	  		 	 	 multiCpUpDown++;
             	 	 	  		 	}
             	 	 	  		 	allMpCopy(0xff);
             	 	 	  		 	break;

             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;
             	 	 
      	    	 case 12:  //菜单第12层(仅二层菜单第2项(参数设置与查看)扩展有效)
      	    	 case 13:  //菜单第13层(仅二层菜单第2项(参数设置与查看)扩展有效)
      	    	 	 switch(layer1MenuLight)
      	    	   {
             	 	   case 1: //参数设置与查看
             	 	 		#ifdef LIGHTING
             	 	 		 if (13==menuInLayer)
             	 	 		 {
             	 	 		 	 layer2xMenuLight++;
             	 	 		 	 if (layer2xMenuLight>2)
             	 	 		 	 {
             	 	 		 	 	 layer2xMenuLight = 0;
             	 	 		 	 }
             	 	 		 }
             	 	 		 else
             	 	 		 {
             	 	 		#endif

             	 	 	 	   if (layer2xMenuLight==0)
             	 	 		   {
             	 	 		 	   layer2xMenuLight = 1;
             	 	 		   }
             	 	 		   else
             	 	 		   {
             	 	 			   layer2xMenuLight = 0;
             	 	 		   }
             	 	 		   
             	 	 		#ifdef LIGHTING
             	 	 		 }
             	 	 		#endif
             	 	 		 
             	 	 		 if (menuInLayer==12)
             	 	 		 {
             	 	 		   layer2xMenu(0, layer2xMenuLight);
             	 	 		 }
             	 	 		 else
             	 	 		 {
             	 	 		   layer2xMenu(1, layer2xMenuLight);
             	 	 		 }
             	 	 	 	 break;
      	    	   }
      	    		 break;

      	    	 case 14:  //菜单第14层(仅二层菜单第2项(参数设置与查看)扩展有效)
      	    		 switch (layer2xMenuLight)
      	    	   {
      	    	  	 case 0:  //查询测量点参数
      	    	  		 if (mpQueryLight>=(mpQueryPage-1))
      	    	  		 {
      	    	  		   mpQueryLight = 0;
      	    	  		 }
      	    	  		 else
      	    	  		 {
      	    	  		 	 mpQueryLight++;
      	    	  		 }
      	    	  		 mpQueryMenu(mpQueryType, mpQueryLight);
      	    	  		 break;
      	    	  		 
      	    		  #ifdef LIGHTING
      	    		 	 case 2:    //查询控制时段
	                   if (tmpCTimesNode->next!=NULL)
	                   {
	                     tmpCTimesNode = tmpCTimesNode->next;
	                   }
	                   else
	                   {
	                     tmpCTimesNode = cTimesHead;
	                   }
      	    		 		 queryCTimes(tmpCTimesNode);
	                   break;
      	    		  #endif

      	    	   }
      	    	   break;

             	 case 20:   //菜单第20层
             	 	 stringUpDown(passWord, pwLight,1);
             	 	 inputPassWord(pwLight);
             	 	 break;
            }
            break;
             
          case KEY_RIGHT:   //向右
           	switch(menuInLayer)
            {
             	case 1:    //菜单第1层
             		if (layer1MenuLight>=MAX_LAYER1_MENU-1)
             	 	{
             	 	 	layer1MenuLight = 0;
             	 	}
             	 	else
             	 	{
             	 	 	layer1MenuLight++;
             	 	}
             	 	layer1Menu(layer1MenuLight);
             	  break;

             	case 2:    //菜单第2层
             	 	switch(layer1MenuLight)
             	 	{
             	 	 	case 0:    //抄表查询
             	 	 	  switch(layer2MenuLight[layer1MenuLight])
             	 	 	  {
             	 	 	  	case 0: //1-1测量点有功抄表查询
             	 	 	  	 #ifdef LIGHTING
             	 	 	  	  ;
             	 	 	  	 #else	
             	 	 	  		if (layer3MenuLight[0][0]>=(layer3MenuNum[0][0]-1))
             	 	 	  		{
             	 	 	  		 	layer3MenuLight[0][0] = 0;
             	 	 	  		}
             	 	 	  		else
             	 	 	  		{
             	 	 	  		 	layer3MenuLight[0][0]++;             	 	 	  		 		
             	 	 	  		}
             	 	 	  		copyQueryMenu(layer2MenuLight[0],layer3MenuLight[0][0]);
             	 	 	  	 #endif
             	 	 	  		break;
             	 	 	  }
             	 	 	  break;
             	 	}
             	  break;             	   

             	case 3:    //菜单第3层
             	 	switch(layer1MenuLight)
             	 	{
             	 	 	  case 1:       //参数设置与查看
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		 case 0:  //通道参数设置
      	    		         	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
      	    		         	 {                           
                             adjustCommParaLight(1);
      	    		             commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
      	    		           break;

             	 	 	  		 case 1:  //台区电表参数设置
                           adjustSetMeterParaLight(1,1);
      	    		           set485Meter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           break;

             	 	 	  		 case 2:  //集抄电表参数设置
                           adjustSetMeterParaLight(1, 2);
      	    		           setCarrierMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           break;
      	    		           
      	    		         case 3:  //终端时间设置
                           if (keyLeftRight>=13)
                       	   {
                       	 	   keyLeftRight = 0;
                       	   }
                       	   else
                       	   {
                       	 	   keyLeftRight++;
                       	   }
      	    		           setTeDateTime(keyLeftRight);
      	    		           break;
      	    		           
      	    		         case 4:  //界面密码修改
      	    		         	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
      	    		         	 {
                             if (keyLeftRight>=5)
                       	     {
                       	 	     keyLeftRight = 0;
                       	     }
                       	     else
                       	     {
                       	 	     keyLeftRight++;
                       	     }
      	    		             modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
      	    		           break;

      	    		         case 5:  //终端编号设置
                           #ifdef TE_ADDR_USE_BCD_CODE
                            if (keyLeftRight>=7)
                           #else
                            if (keyLeftRight>=8)
                           #endif
                       	   {
                       	 	   keyLeftRight = 0;
                       	   }
                       	   else
                       	   {
                       	 	   keyLeftRight++;
                       	   }
      	    		           setTeAddr(keyLeftRight);
      	    		           break;
      	    		           
      	    		         case 6:  //以太网参数设置
      	    		         	 adjustEthParaLight(1);
      	    		         	 setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);      	    		         	 
      	    		         	 break;
      	    		         
      	    		         case 7:  //虚拟专网用户名密码
                 	 	 	  	 if (inputStatus==STATUS_NONE)
                 	 	 	  	 {
                 	 	 	  	   if (keyLeftRight<63)
                 	 	 	  	   {
                 	 	 	  		   keyLeftRight++;
                 	 	 	  	   }
                 	 	 	  	   else
                 	 	 	  	   {
                 	 	 	  		   keyLeftRight = 0;
                 	 	 	  	   }
                 	 	 	  	   setVpn(keyLeftRight);
                 	 	 	  	 }
                 	 	 	  	 else
                 	 	 	  	 {
                 	 	 	  	   if (selectIndex<69)
                 	 	 	  	   {
                 	 	 	  		   selectIndex++;
                 	 	 	  	   }
                 	 	 	  	   else
                 	 	 	  	   {
                 	 	 	  		   selectIndex = 0;
                 	 	 	  	   }
                 	 	 	  	   selectChar(selectIndex);
                 	 	 	  	 }
      	    		           break;

      	    		         case 8:  //终端级联参数设置
      	    		         	 adjustCasParaLight(1);
      	    		           setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		         	 break;

             	 	 	  		 case 9:  //锐拔参数参数设置
                           adjustSetMeterParaLight(1, 6);
      	    		           setRlPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           break;
      	    		      }      	    		      
      	    		      break;
      	    		      
      	    		    case 2:       //终端管理与维护
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 2:  //搜索表号
             	 	 	  		 #ifdef LIGHTING    //路灯集中器该项是“调节液晶对比度”

      	    		 	 	 	  	if (lcdDegree<20)
      	    		 	 	 	  	{
      	    		 	 	 	  		 lcdDegree++;
      	    		 	 	 	  	}
      	    	 	 	  	  	setLcdDegree(lcdDegree);

             	 	 	  		 #else
             	 	 	  		 	
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  searchMeter(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	
             	 	 	  		 #endif
             	 	 	  		 	break;

             	 	 	  		case 3:  //新增电能表地址
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  newAddMeter(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  		 	
             	 	 	  		case 5: 
       	    		 	 	 	   if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-2)
      	    		 	 	 	   {
      	    		 	 	 	   	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
      	    		 	 	 	   }
      	    		 	 	 	   else
      	    		 	 	 	   {
      	    		 	 	 	  	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	 	   }
      	    	 	 	  		 terminalInfo(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    	 	 	  		 break;
             	 	 	  		 	
      	    	 	 	  	  case 6:  //调节LCD对比度
      	    		 	 	 	  	if (lcdDegree<20)
      	    		 	 	 	  	{
      	    		 	 	 	  		 lcdDegree++;
      	    		 	 	 	  	}
      	    	 	 	  	  	setLcdDegree(lcdDegree);
      	    	 	 	  	  	break;
      	    		      }
      	    		    	break;
      	    		}
             	 	break;
             	 	 
             	case 4:    //菜单第4层
             	  switch(layer1MenuLight)
             	  {
             	 	 	  case 1: //参数设置与查看
             	 	 	  	if (inputStatus==STATUS_NONE)
             	 	 	  	{
             	 	 	  	  if (inputIndex<15)
             	 	 	  	  {
             	 	 	  		   inputIndex++;
             	 	 	  	  }
             	 	 	  	  else
             	 	 	  	  {
             	 	 	  		   inputIndex = 0;
             	 	 	  	  }
             	 	 	  	  inputApn(inputIndex);
             	 	 	  	}
             	 	 	  	else
             	 	 	  	{
             	 	 	  	  if (selectIndex<69)
             	 	 	  	  {
             	 	 	  		   selectIndex++;
             	 	 	  	  }
             	 	 	  	  else
             	 	 	  	  {
             	 	 	  		   selectIndex = 0;
             	 	 	  	  }
             	 	 	  	  selectChar(selectIndex);
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2: //终端管理与维护
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //实时抄表
             	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
             	 	 	  		 	{
             	 	 	  		 	 	case 0:  //指定测量点抄表
             	 	 	  		 	 		if (keyLeftRight>=3)
             	 	 	  		 	 	  {
             	 	 	  		 	 	  	keyLeftRight = 0;
             	 	 	  		 	 	  }
             	 	 	  		 	 	  else
             	 	 	  		 	 	  {
             	 	 	  		 	 	  	keyLeftRight++;
             	 	 	  		 	 	  }
             	 	 	  		 	 	  
             	 	 	  		 	 	 #ifdef LIGHTING
             	 	 	  		 	 	  singleCcbCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	 #else
             	 	 	  		 	 	  singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	 #endif
             	 	 	  		 	 	  break;
             	 	 	  		 	 	
             	 	 	  		 	 	case 1:  //全部测量点抄表
             	 	 	  		 	 		if (keyLeftRight!=0xff)
             	 	 	  		 	 		{
             	 	 	  		 	 		  if (keyLeftRight>=1)
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight = 0;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    else
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight++;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    allMpCopy(keyLeftRight);
             	 	 	  		 	 	  }
             	 	 	  		 	 		break;
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  	}
             	 	 	  	break;
             	 	}
             	 	break;
             	 	 
      	    	case 14:  //菜单第14层(仅二层菜单第2项(参数设置与查看)扩展有效)
      	    		switch (layer2xMenuLight)
      	    	  {
      	    	  	 case 0:  //查询测量点参数
      	    	  		#ifdef LIGHTING
      	    	  		 if (mpQueryType==0)
      	    	  		 {
      	    	  		   mpQueryType = 2;
      	    	  		 }
      	    	  		 else
      	    	  		 {
      	    	  		 	 mpQueryType = 0;
      	    	  		 }
      	    	  		#else
      	    	  		 if (mpQueryType>1)
      	    	  		 {
      	    	  		   mpQueryType = 0;
      	    	  		 }
      	    	  		 else
      	    	  		 {
      	    	  		 	 mpQueryType++;
      	    	  		 }
      	    	  		#endif
      	    	  		 mpQueryMenu(mpQueryType, mpQueryLight);
      	    	  		 break;
      	    	  }
      	    	  break;
      	    	   
             	case 20:   //菜单第20层(输入密码)
      	    		if (pwLight>=5)
      	    		{
      	    		   pwLight = 0;
      	    		}
      	    		else
      	    		{
      	    		 	 pwLight++;
      	    		}      	    		 
      	    		inputPassWord(pwLight);
             	 	break;
            }
           	break;
           	 
          case KEY_LEFT:  //向左
           	switch(menuInLayer)
            {
             	 case 1:
             	 	 if (layer1MenuLight==0)
             	 	 {
             	 	 	  layer1MenuLight = MAX_LAYER1_MENU-1;
             	 	 }
             	 	 else
             	 	 {
             	 	 	  layer1MenuLight--;
             	 	 }
             	 	 layer1Menu(layer1MenuLight);
             	   break;
             	   
             	 case 2:    //菜单第2层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 0:    //抄表查询
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0: //1-1测量点有功抄表查询
             	 	 	  		 #ifdef LIGHTING
             	 	 	  		  ;
             	 	 	  		 #else
             	 	 	  		 	if (layer3MenuLight[0][0]==0)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[0][0] = layer3MenuNum[0][0]-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[0][0]--;
             	 	 	  		 	}
             	 	 	  		 	copyQueryMenu(layer2MenuLight[0],layer3MenuLight[0][0]);
             	 	 	  		 #endif
             	 	 	  		  break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;
             	   
             	 case 3:    //菜单第3层
                 switch(layer1MenuLight)
                 {
                 	  case 1:  //参数设置
                 	  	switch(layer2MenuLight[layer1MenuLight])
                 	  	{
                 	  		 case 0: //通信参数设置
                           if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
                             adjustCommParaLight(0);
      	    		             commParaSetMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
      	    		           break;

                 	  		 case 1: //台区电表参数设置
                           adjustSetMeterParaLight(0,1);
      	    		           set485Meter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           break;

                 	  		 case 2: //集抄电表参数设置
                           adjustSetMeterParaLight(0,2);
      	    		           setCarrierMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           break;
      	    		           
      	    		         case 3: //终端时间设置
                           if (keyLeftRight==0)
                       	   {
                       	 	   keyLeftRight = 13;
                       	   }
                       	   else
                       	   {
                       	 	   keyLeftRight--;
                       	   }
      	    		           setTeDateTime(keyLeftRight);
      	    		           break;
      	    		           
      	    		         case 4: //修改界面密码
                           if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]!=0xff)
                           {
                             if (keyLeftRight==0)
                       	     {
                       	 	     keyLeftRight = 5;
                       	     }
                       	     else
                       	     {
                       	 	     keyLeftRight--;
                       	     }
      	    		             modifyPasswordMenu(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           }
      	    		           break;

      	    		         case 5: //终端编号设置
                           if (keyLeftRight==0)
                       	   {
                       	 	   #ifdef TE_ADDR_USE_BCD_CODE
                       	 	    keyLeftRight = 7;
                       	 	   #else
                       	 	    keyLeftRight = 8;
                       	 	   #endif
                       	   }
                       	   else
                       	   {
                       	 	   keyLeftRight--;
                       	   }
      	    		           setTeAddr(keyLeftRight);
      	    		           break;
      	    		         
      	    		         case 6:  //以太网参数设置
      	    		         	 adjustEthParaLight(0);
      	    		         	 setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);      	    		         	 
      	    		         	 break;
      	    		         	 
      	    		         case 7: //虚拟专网用户名密码
                	 	 	  	 if (inputStatus==STATUS_NONE)
                	 	 	  	 {
                             if (keyLeftRight==0)
                         	   {
                         	 	   keyLeftRight = 63;
                         	   }
                         	   else
                         	   {
                         	 	   keyLeftRight--;
                         	   }
        	    		           setVpn(keyLeftRight);
                	 	 	  	 }
                	 	 	  	 else
                	 	 	  	 {
                	 	 	  	   if (selectIndex==0)
                	 	 	  	   {
                	 	 	  		    selectIndex=68;
                	 	 	  	   }
                	 	 	  	   else
                	 	 	  	   {
                	 	 	  		    selectIndex--;
                	 	 	  	   }
                	 	 	  	   selectChar(selectIndex);
                	 	 	  	 }
      	    		           break;
      	    		         
      	    		         case 8:  //终端级联参数设置
      	    		         	 adjustCasParaLight(0);
      	    		           setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		         	 break;
      	    		         	 
             	 	 	  		 case 9:  //锐拔参数参数设置
                           adjustSetMeterParaLight(0, 6);
      	    		           setRlPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		           break;
      	    		      }
      	    		      break;
      	    		      
      	    		    case 2:       //终端管理与维护
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 2:  //搜索表号
             	 	 	  		 #ifdef LIGHTING    //路灯集中器该项是“调节液晶对比度”

      	    		 	 	 	  	if (lcdDegree>0)
      	    		 	 	 	  	{
      	    		 	 	 	  		lcdDegree--;
      	    		 	 	 	  	}
      	    	 	 	  	  	setLcdDegree(lcdDegree);

             	 	 	  		 #else
             	 	 	  		 	
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	   	keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  searchMeter(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	
             	 	 	  		 #endif
             	 	 	  		 	break;
             	 	 	  		 	
             	 	 	  		case 3:  //新增电能表地址
             	 	 	  		 	if (keyLeftRight!=0xff)
             	 	 	  		 	{
             	 	 	  		 	  if (keyLeftRight>0)
             	 	 	  		 	  {
             	 	 	  		 	 	  keyLeftRight = 0;
             	 	 	  		 	  }
             	 	 	  		 	  else
             	 	 	  		 	  {
             	 	 	  		 	   	keyLeftRight = 1;
             	 	 	  		 	  }
             	 	 	  		 	  newAddMeter(keyLeftRight);
             	 	 	  		 	}
             	 	 	  		 	break;
    	    	 	 	  		 
    	    	 	 	  		 case 5:
      	    		 	 	 	   if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>1 )
      	    		 	 	 	   {
      	    		 	 	 	   	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]--;
      	    		 	 	 	   }
      	    		 	 	 	   else
      	    		 	 	 	   {
      	    		 	 	 	  	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1;
      	    		 	 	 	   }
      	    	 	 	  		 terminalInfo(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    	 	 	  		 break;
      	    	 	 	  		 
      	    	 	 	  	  case 6:  //调节LCD对比度
      	    		 	 	 	  	if (lcdDegree>0)
      	    		 	 	 	  	{
      	    		 	 	 	  		 lcdDegree--;
      	    		 	 	 	  	}
      	    	 	 	  	  	setLcdDegree(lcdDegree);
      	    	 	 	  	  	break;
      	    		      }
      	    		    	break;
      	    		 }
             	 	 break;
             	 	 
             	 case 4:    //菜单第4层
             	 	 switch(layer1MenuLight)
             	 	 {
             	 	 	  case 1: //参数设置与查看
             	 	 	  	if (inputStatus==STATUS_NONE)
             	 	 	  	{
             	 	 	  	  if (inputIndex==0)
             	 	 	  	  {
             	 	 	  		  inputIndex = 15;
             	 	 	  	  }
             	 	 	  	  else
             	 	 	  	  {
             	 	 	  		  inputIndex--;
             	 	 	  	  }
             	 	 	  	  inputApn(inputIndex);
             	 	 	  	}
             	 	 	  	else
             	 	 	  	{
             	 	 	  	  if (selectIndex==0)
             	 	 	  	  {
             	 	 	  		   selectIndex=68;
             	 	 	  	  }
             	 	 	  	  else
             	 	 	  	  {
             	 	 	  		   selectIndex--;
             	 	 	  	  }
             	 	 	  	  selectChar(selectIndex);
             	 	 	  	}
             	 	 	  	break;
             	 	 	  	
             	 	 	  case 2: //终端管理与维护
             	 	 	  	switch(layer2MenuLight[layer1MenuLight])
             	 	 	  	{
             	 	 	  		case 0:  //实时抄表
             	 	 	  			switch(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]])
             	 	 	  		 	{
             	 	 	  		 	 	case 0:  //指定测量点抄表
             	 	 	  		 	 		if (keyLeftRight==0)
             	 	 	  		 	 	  {
             	 	 	  		 	 	  	keyLeftRight = 3;
             	 	 	  		 	 	  }
             	 	 	  		 	 	  else
             	 	 	  		 	 	  {
             	 	 	  		 	 	  	keyLeftRight--;
             	 	 	  		 	 	  }
             	 	 	  		 	 	 #ifdef LIGHTING
             	 	 	  		 	 	  singleCcbCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	 #else
             	 	 	  		 	 	  singleMeterCopy(singleCopyMp,singleCopyTime,singleCopyEnergy,keyLeftRight);
             	 	 	  		 	 	 #endif
             	 	 	  		 	 	  break;
             	 	 	  		 	 	  
             	 	 	  		 	 	case 1:  //全部测量点抄表
             	 	 	  		 	 		if (keyLeftRight!=0xff)
             	 	 	  		 	 		{
             	 	 	  		 	 		  if (keyLeftRight==0)
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight = 1;
             	 	 	  		 	 	    }
             	 	 	  		 	 	    else
             	 	 	  		 	 	    {
             	 	 	  		 	 	  	  keyLeftRight--;
             	 	 	  		 	 	    }
             	 	 	  		 	 		  allMpCopy(keyLeftRight);
             	 	 	  		 	 		}
             	 	 	  		 	 		break;
             	 	 	  		 	}
             	 	 	  		 	break;
             	 	 	  	}
             	 	 	  	break;
             	 	 }
             	 	 break;
             	 	 
      	    	 case 14:  //菜单第14层(仅二层菜单第2项(参数设置与查看)扩展有效)
      	    		 switch (layer2xMenuLight)
      	    	   {
      	    	  	 case 0:  //查询测量点参数
      	    	  		#ifdef LIGHTING
      	    	  		 if (mpQueryType==0)
      	    	  		 {
      	    	  		   mpQueryType = 2;
      	    	  		 }
      	    	  		 else
      	    	  		 {
      	    	  		 	 mpQueryType = 0;
      	    	  		 }
      	    	  		#else
      	    	  		 if (mpQueryType==0)
      	    	  		 {
      	    	  		   mpQueryType = 2;
      	    	  		 }
      	    	  		 else
      	    	  		 {
      	    	  		 	 mpQueryType--;
      	    	  		 }
      	    	  		#endif
      	    	  		 mpQueryMenu(mpQueryType, mpQueryLight);
      	    	  		 break;
      	    	   }
      	    	   break;

             	 case 20:   //菜单第20层(输入密码)
      	    		 if (pwLight==0)
      	    		 {
      	    		   pwLight = 5;
      	    		 }
      	    		 else
      	    		 {
      	    		 	 pwLight--;
      	    		 }
      	    		 inputPassWord(pwLight);
             	 	 break;
            }
           	break;              	
        }
      }
   }
   else
   {
      keyPressCount = 0;
   }
}

/*******************************************************
函数名称:defaultMenu
功能描述:默认菜单(主界面,国网376.1集中器规约菜单)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void defaultMenu(void)
{
   if (displayMode==CYCLE_DISPLAY_MODE)
   {
   	  return;
   }
   
   menuInLayer = 0;
   guiLine(1,17,160,144,0);

  #ifdef LIGHTING
   guiDisplay(40,30,"照明集中器",1);
  #else
   guiDisplay(24,30,"低压集抄集中器",1);
  #endif
   
   displayMode = DEFAULT_DISPLAY_MODE;
   lcdRefresh(17,145);
}

/*******************************************************
函数名称:cycleMenu
功能描述:轮显菜单(国网376.1集中器规约菜单)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void cycleMenu(void)
{
   char      str[20];
   char      say[20];
   DATE_TIME tmpTime;   
   INT8U     dataBuff[LENGTH_OF_ENERGY_RECORD];
   BOOL      buffHasData=FALSE;
 	 INT32U    integer,decimal;
 	 INT16U    offset;
 	 INT32U    disData;
 	 INT8U     sign;

   if (tmpCycleLink==NULL)
   {
   	 return;
   }

   //测量点号显示
   guiLine(85, 1,117,16,0);
   if (tmpCycleLink->mp<100)
   {
     guiLine(100, 1,100,16,1);
     guiLine(117, 1,117,16,1);
     guiLine(100, 1,117, 1,1);
     guiLine(100,16,117,16,1);
     
     strcpy(say,"");
     if (tmpCycleLink->mp<10)
     {
     	 strcat(say,"0");
     }
     strcat(say,intToString(tmpCycleLink->mp,3,str));
     guiDisplay(102,1,say,1);
   }
   else
   {
     if (tmpCycleLink->mp<1000)
     {
       guiLine( 93, 1, 93,16,1);
       guiLine(117, 1,117,16,1);
       guiLine( 93, 1,117, 1,1);
       guiLine( 93,16,117,16,1);
       guiDisplay(94,1,intToString(tmpCycleLink->mp,3,str),1); 
     }
     else
     {
       guiLine( 85, 1, 85,16,1);
       guiLine(117, 1,117,16,1);
       guiLine( 85, 1,117, 1,1);
       guiLine( 85,16,117,16,1);
       guiDisplay(86,1,intToString(tmpCycleLink->mp,3,str),1);
     }
   }

   guiLine(1,17,160,144,0);
   guiLine(1,18,1,140,1);
   guiLine(160,18,160,140,1);
   guiLine(1,18,160,18,1);
   guiLine(1,140,160,140,1);
   
   if (tmpCycleLink->port==31)
   {
     guiDisplay( 2,LCD_LINE_1+6,"有功总=",1);
     guiDisplay( 2,LCD_LINE_2+6,"电压A    B    C   ",1);
     guiDisplay( 2,LCD_LINE_3+6,"电流A    B    C   ",1);
     guiDisplay( 2,LCD_LINE_4+6,"功率因数总=",1);
   }
   else
   {
  	 tmpTime = queryCopyTime(tmpCycleLink->mp);

     //2014-04-17,改成读取当天的最后一条数据
     //buffHasData = readMeterData(dataBuff, tmpCycleLink->mp, PRESENT_DATA, PARA_VARIABLE_DATA, &tmpTime, 0);   	 
     buffHasData = readMeterData(dataBuff, tmpCycleLink->mp, LAST_TODAY, PARA_VARIABLE_DATA, &tmpTime, 0);   	 

     if (buffHasData==TRUE)
     {	
       strcpy(say,"有功总=");
       if (dataBuff[POWER_INSTANT_WORK]!=0xee)
       {
         disData = dataBuff[POWER_INSTANT_WORK] | dataBuff[POWER_INSTANT_WORK+1]<<8 | dataBuff[POWER_INSTANT_WORK+2]<<16;
         sign = 0;
         if (dataBuff[POWER_INSTANT_WORK+2]&0x80)
         {
          	sign = 1;
          	disData &=0x7fffff;
         }
         
         decimal = (disData>>12 & 0xf)*1000
                 +(disData>>8 & 0xf)*100
                 +(disData>>4 & 0xf)*10
                 +(disData & 0xf);
         integer = (disData>>20 & 0xf)*10+(disData>>16 & 0xf);
         if (sign==1)
         {
         	 strcat(say,"-");
         }
         strcat(say,floatToString(integer,decimal,4,4,str));
       }       
       guiDisplay( 2,LCD_LINE_1+6,say,1);
       
       strcpy(say,"电压A");
       if (dataBuff[VOLTAGE_PHASE_A]!=0xee && dataBuff[VOLTAGE_PHASE_A]!=0xff)
       {
	       disData = dataBuff[VOLTAGE_PHASE_A] | dataBuff[VOLTAGE_PHASE_A+1]<<8;
         decimal = disData& 0xf;
         integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10 + (disData>>4 & 0xf);
         strcat(say,floatToString(integer,0,1,0,str));
       }
       guiDisplay( 2,LCD_LINE_2+6,say,1);

       strcpy(say,"B");
       if (dataBuff[VOLTAGE_PHASE_B]!=0xee && dataBuff[VOLTAGE_PHASE_B]!=0xff)
       {
	       disData = dataBuff[VOLTAGE_PHASE_B] | dataBuff[VOLTAGE_PHASE_B+1]<<8;
         decimal = disData& 0xf;
         integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10 + (disData>>4 & 0xf);
         strcat(say,floatToString(integer,0,1,0,str));
       }
       guiDisplay(76,LCD_LINE_2+6,say,1);

       strcpy(say,"C");
       if (dataBuff[VOLTAGE_PHASE_C]!=0xee && dataBuff[VOLTAGE_PHASE_C]!=0xff)
       {
	       disData = dataBuff[VOLTAGE_PHASE_C] | dataBuff[VOLTAGE_PHASE_C+1]<<8;
         decimal = disData& 0xf;
         integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10 + (disData>>4 & 0xf);
         strcat(say,floatToString(integer,0,1,0,str));
       }
       guiDisplay(120,LCD_LINE_2+6,say,1);
       
       strcpy(say,"电流A");
       if (dataBuff[CURRENT_PHASE_A]!=0xee && dataBuff[CURRENT_PHASE_A]!=0xff)
       {
	       disData = dataBuff[CURRENT_PHASE_A] | dataBuff[CURRENT_PHASE_A+1]<<8 | (dataBuff[CURRENT_PHASE_A+2]&0x7f)<<16;
         decimal = (disData>>8 & 0xf)*100+(disData>>4 & 0xf)*10 + (disData& 0xf);
         integer = (disData>>20 & 0xf)*100+(disData>>16 & 0xf)*10 + (disData>>12 & 0xf);
         strcat(say,floatToString(integer,decimal,3,2,str));
       }
       guiDisplay( 2,LCD_LINE_3+6,say,1);

       strcpy(say,"B");
       if (dataBuff[CURRENT_PHASE_B]!=0xee && dataBuff[CURRENT_PHASE_B]!=0xff)
       {
	       disData = dataBuff[CURRENT_PHASE_B] | dataBuff[CURRENT_PHASE_B+1]<<8 | (dataBuff[CURRENT_PHASE_B+2]&0x7f)<<16;
         decimal = (disData>>8 & 0xf)*100+(disData>>4 & 0xf)*10 + (disData& 0xf);
         integer = (disData>>20 & 0xf)*100+(disData>>16 & 0xf)*10 + (disData>>12 & 0xf);
         strcat(say,floatToString(integer,decimal,3,2,str));
       }
       guiDisplay(76,LCD_LINE_3+6,say,1);

       strcpy(say,"C");
       if (dataBuff[CURRENT_PHASE_C]!=0xee && dataBuff[CURRENT_PHASE_C]!=0xff)
       {
	       disData = dataBuff[CURRENT_PHASE_C] | dataBuff[CURRENT_PHASE_C+1]<<8 | (dataBuff[CURRENT_PHASE_C+2]&0x7f)<<16;
         decimal = (disData>>8 & 0xf)*100+(disData>>4 & 0xf)*10 + (disData& 0xf);
         integer = (disData>>20 & 0xf)*100+(disData>>16 & 0xf)*10 + (disData>>12 & 0xf);
         strcat(say,floatToString(integer,decimal,3,2,str));
       }
       guiDisplay(120,LCD_LINE_3+6,say,1);
       
     	 strcpy(say,"功率因数总=");
     	 if (dataBuff[TOTAL_POWER_FACTOR]!=0xee)
     	 {
     	   disData = dataBuff[TOTAL_POWER_FACTOR] | dataBuff[TOTAL_POWER_FACTOR+1]<<8;
         sign = 0;
         if (disData&0x8000)
         {
           disData &= 0x7fff;
           sign = 1; 
         }
         integer = (disData>>12 & 0xf)*100 + (disData>>8 & 0xf)*10+(disData>>4 & 0xf);
         decimal = (disData & 0xf);
         if (sign==1)
         {
           strcat(say,"-");
         }
         strcat(say,floatToString(integer,decimal,1,1,str));
     	 }
       guiDisplay(2,LCD_LINE_4+6,say,1);
     }
     else
     {
       guiDisplay( 2,LCD_LINE_1+6,"有功总=",1);
       guiDisplay( 2,LCD_LINE_2+6,"电压A    B    C   ",1);
       guiDisplay( 2,LCD_LINE_3+6,"电流A    B    C   ",1);
       guiDisplay( 2,LCD_LINE_4+6,"功率因数总=",1);
     }     
   }
   
   if (tmpCycleLink->port==31)
   {
  	 tmpTime = timeHexToBcd(sysTime);
     
     if (tmpCycleLink->bigAndLittleType==0x00 && tmpCycleLink->protocol==DLT_645_2007) 
     {
       buffHasData = readMeterData(dataBuff, tmpCycleLink->mp, SINGLE_PHASE_DAY, ENERGY_DATA, &tmpTime, 0);
       if (buffHasData==FALSE)
       {
       	 buffHasData = readMeterData(dataBuff, tmpCycleLink->mp, SINGLE_PHASE_PRESENT, ENERGY_DATA, &tmpTime, 0);
       }
     }
     else
     {
       buffHasData = readMeterData(dataBuff, tmpCycleLink->mp, SINGLE_PHASE_PRESENT, ENERGY_DATA, &tmpTime, 0);
     }
   }
   else
   {
  	 tmpTime = queryCopyTime(tmpCycleLink->mp);
     
     //2014-04-17,改成读取当天的最后一条数据
     //buffHasData = readMeterData(dataBuff, tmpCycleLink->mp, PRESENT_DATA, ENERGY_DATA, &tmpTime, 0);
     buffHasData = readMeterData(dataBuff, tmpCycleLink->mp, LAST_TODAY, ENERGY_DATA, &tmpTime, 0);     
   }
   offset = POSITIVE_WORK_OFFSET;
   
   if (buffHasData==TRUE)
   {
     strcpy(say,"有功总=");
   	 
   	 if (dataBuff[offset]!=0xee)
   	 {
       decimal = (dataBuff[offset]>>4 & 0xf) * 10 + (dataBuff[offset] & 0xf);
       integer = (dataBuff[offset+3]>>4 & 0xf)*100000
               +(dataBuff[offset+3] & 0xf)*10000
               +(dataBuff[offset+2]>>4 & 0xf)*1000
               +(dataBuff[offset+2] & 0xf)*100
               +(dataBuff[offset+1]>>4 & 0xf)*10
               +(dataBuff[offset+1]& 0xf);
       strcat(say,floatToString(integer,decimal,2,2,str));
     }
     guiDisplay( 2,LCD_LINE_5+6,say,1);
     
     offset+=4;
     strcpy(say,"尖=");  	        
   	 if (dataBuff[offset]!=0xee)
   	 {
       decimal = (dataBuff[offset]>>4 & 0xf) * 10 + (dataBuff[offset] & 0xf);
       integer = (dataBuff[offset+3]>>4 & 0xf)*100000
               +(dataBuff[offset+3] & 0xf)*10000
               +(dataBuff[offset+2]>>4 & 0xf)*1000
               +(dataBuff[offset+2] & 0xf)*100
               +(dataBuff[offset+1]>>4 & 0xf)*10
               +(dataBuff[offset+1]& 0xf);
       strcat(say,floatToString(integer,decimal,2,2,str));
       say[8]='\0';
     }
     guiDisplay( 2,LCD_LINE_6+6,say,1);
     
     offset+=4;
     strcpy(say,"峰=");  	        
   	 if (dataBuff[offset]!=0xee)
   	 {
       decimal = (dataBuff[offset]>>4 & 0xf) * 10 + (dataBuff[offset] & 0xf);
       integer = (dataBuff[offset+3]>>4 & 0xf)*100000
               +(dataBuff[offset+3] & 0xf)*10000
               +(dataBuff[offset+2]>>4 & 0xf)*1000
               +(dataBuff[offset+2] & 0xf)*100
               +(dataBuff[offset+1]>>4 & 0xf)*10
               +(dataBuff[offset+1]& 0xf);
       strcat(say,floatToString(integer,decimal,2,2,str));
       say[8]='\0';
     }
     guiDisplay(81,LCD_LINE_6+6,say,1);

     offset+=4;
     strcpy(say,"平=");  	        
   	 if (dataBuff[offset]!=0xee)
   	 {     
       decimal = (dataBuff[offset]>>4 & 0xf) * 10 + (dataBuff[offset] & 0xf);
       integer = (dataBuff[offset+3]>>4 & 0xf)*100000
               +(dataBuff[offset+3] & 0xf)*10000
               +(dataBuff[offset+2]>>4 & 0xf)*1000
               +(dataBuff[offset+2] & 0xf)*100
               +(dataBuff[offset+1]>>4 & 0xf)*10
               +(dataBuff[offset+1]& 0xf);
       strcat(say,floatToString(integer,decimal,2,2,str));
       say[8]='\0';
     }
     guiDisplay( 2,LCD_LINE_7+6,say,1);
     
     offset+=4;
     strcpy(say,"谷=");
   	 if (dataBuff[offset]!=0xee)
   	 {
       decimal = (dataBuff[offset]>>4 & 0xf) * 10 + (dataBuff[offset] & 0xf);
       integer = (dataBuff[offset+3]>>4 & 0xf)*100000
               +(dataBuff[offset+3] & 0xf)*10000
               +(dataBuff[offset+2]>>4 & 0xf)*1000
               +(dataBuff[offset+2] & 0xf)*100
               +(dataBuff[offset+1]>>4 & 0xf)*10
               +(dataBuff[offset+1]& 0xf);
       strcat(say,floatToString(integer,decimal,2,2,str));
       say[8]='\0';
     }
     guiDisplay(81,LCD_LINE_7+6,say,1);
   }
   else
   {
     guiDisplay( 2,LCD_LINE_5+6,"有功总=",1);
     guiDisplay( 2,LCD_LINE_6+6,"尖=",1);   
     guiDisplay(80,LCD_LINE_6+6,"峰=",1);   
     guiDisplay( 2,LCD_LINE_7+6,"平=",1);
     guiDisplay(80,LCD_LINE_7+6,"谷=",1);   	 
   }

   lcdRefresh(1,144);

   cycleDelay = nextTime(sysTime, 0, 8);
   tmpCycleLink = tmpCycleLink->next;
   
   if (tmpCycleLink==NULL)
   {
   	 tmpCycleLink = cycleMpLink;
   }
}

/*******************************************************
函数名称:layer2Menu
功能描述:二层菜单(376.1国家电网集中器规约)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void layer2Menu(int lightNum,int layer1Num)
{
	INT8U i, j;
	INT8U startItem=0, endItem=0;

	guiLine(1,17,160,144,0);

  menuInLayer = 2;

 #ifndef LIGHTING
	if (layer1Num==2)
  {
   	if (carrierModuleType==RL_WIRELESS || carrierModuleType==SR_WIRELESS)
   	{
   	  strcpy(layer2MenuItem[2][3],"  未入网节点地址  ");
   	}
  }
 #endif
   
  if (layer2MenuNum[layer1Num]>8)
  {
   	if (lightNum>7)
   	{
   	 	startItem = lightNum-7;
   	 	endItem = layer2MenuNum[layer1Num]-startItem;
   	}
   	else
   	{
   	  startItem = 0;
   	  endItem = 8;
   	}
  }
  else
  {
   	startItem = 0;
   	endItem = layer2MenuNum[layer1Num];
  }

	for(i=0,j=startItem; i<endItem && i<8; i++,j++)
	{
		if (j==lightNum)
		{
      guiDisplay(8,LCD_LINE_1+i*16,layer2MenuItem[layer1Num][j],0);
		}
		else
		{
      guiDisplay(8,LCD_LINE_1+i*16,layer2MenuItem[layer1Num][j],1);
		}
	}
	 
  lcdRefresh(17,145);    //刷新LCD
}

/*******************************************************
函数名称:layer2xMenu
功能描述:二层菜单第二项(参数设置与查看)附加菜单(376.1国家电网集中器规约)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void layer2xMenu(INT8U type, INT8U lightNum)
{
	 INT8U i;

	 guiLine(1,17,160,144,0);

   if (type==1)
   {
     menuInLayer = 13;
   }
   else
   {
     menuInLayer = 12;
   }
  #ifdef LIGHTING
   if (type==1)
   {
	   for(i=0;i<3;i++)
	   {
		   if (i==lightNum)
		   {
         guiDisplay(17,LCD_LINE_3+i*17,layer2xMenuItem[type][i],0);
		   }
		   else
		   {
         guiDisplay(17,LCD_LINE_3+i*17,layer2xMenuItem[type][i],1);
		   }
		 }
   }
   else
   {
  #endif 
	   
	   for(i=0;i<2;i++)
	   {
		   if (i==lightNum)
		   {
         guiDisplay(17,LCD_LINE_3+i*17,layer2xMenuItem[type][i],0);
		   }
		   else
		   {
         guiDisplay(17,LCD_LINE_3+i*17,layer2xMenuItem[type][i],1);
		   }
		 }
		 
	#ifdef LIGHTING
	 }
	#endif
	 
   lcdRefresh(17,145);               //刷新LCD
}

/*******************************************************
函数名称:commParaQueryMenu
功能描述:通信参数查询菜单(国网集中器菜单)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void commParaQueryMenu(void)
{
	 char              str[30];
	 char              sayStr[30];
	 INT8U             tmpX;

	 guiLine(1,17,160,144,0); //清屏
	 menuInLayer = 14;        //菜单进入第14层
	 
   guiDisplay(33,17,"通信参数查询",1);
   
   //行政区划
   guiDisplay(1,33,"行政区划",1);
   strcpy(sayStr,digitalToChar(addrField.a1[1]>>4));
   strcat(sayStr,digitalToChar(addrField.a1[1]&0xf));
   strcat(sayStr,digitalToChar(addrField.a1[0]>>4));
   strcat(sayStr,digitalToChar(addrField.a1[0]&0xf));
   guiDisplay(68,33,sayStr,1);
   
   //上行信道
   guiDisplay(1,49,"上行信道",1);
   switch(moduleType)
   {
   	  case GPRS_SIM300C:
   	  case GPRS_GR64:
   	  case GPRS_M590E:
   	  case GPRS_M72D:
   	  	strcpy(sayStr, "GPRS");
   	  	break;

   	  case CDMA_DTGS800:
   	  case CDMA_CM180:
   	  	strcpy(sayStr, "CDMA");
   	  	break;
			
   	  case LTE_AIR720H:
   	  	strcpy(sayStr, "LTE");
   	  	break;

   	  case ETHERNET:
   	  	strcpy(sayStr, "以太网");
   	  	break;
   	  
   	  default:
   	  	strcpy(sayStr,"无模块");
   	  	break;
   }
   guiDisplay(68,49,sayStr,1);
   
   //APN域名
   guiDisplay(1,  65, "APN域名",1);
   guiDisplay(68, 65, (char *)ipAndPort.apn, 1);
   
   guiDisplay(1,81,"主站IP及接入端口",1);
	 strcpy(sayStr,intToIpadd(ipAndPort.ipAddr[0]<<24 | ipAndPort.ipAddr[1]<<16 | ipAndPort.ipAddr[2]<<8 | ipAndPort.ipAddr[3],str));
   strcat(sayStr,":");
   guiDisplay(1,97,sayStr,1);
   tmpX = 1+8*strlen(sayStr);
    strcpy(sayStr,intToString(ipAndPort.port[1]<<8 | ipAndPort.port[0],3,str));
   guiDisplay(tmpX,97,sayStr,1);
   
   guiDisplay(1,113,"集中器IP", 1);
   guiDisplay(1,129,intToIpadd(wlLocalIpAddr,str),1);
	 
	 lcdRefresh(17, 145);
}

/*******************************************************
函数名称:mpQueryMenu
功能描述:参数查询菜单
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void mpQueryMenu(INT8U type, INT8U layer3Light)
{
   struct cpAddrLink *tmpLink;
	 char              str[30];
	 char              sayStr[30];
	 INT8U             i, tmpX, tmpY, tmpCount;

	 guiLine(1,17,160,144,0);  //清屏
	 menuInLayer = 14;         //菜单进入第2层
	 
   switch(type)
   {
   	 case 0:  //表地址
   	 	#ifdef LIGHTING
       guiDisplay(1,17,"控制点 通信地址 时段",1);
   	 	#else
       guiDisplay(1,17,"测量点号  电表地址",1);
      #endif
       break;

   	 case 1:  //采集地址
       guiDisplay(1,17,"测量点号 采集器地址",1);
       break;

   	 case 2:  //端口 协议
      #ifdef LIGHTING 
       guiDisplay(1,17,"控制点   端口 协议",1);
      #else
       guiDisplay(1,17,"测量点号 端口 协议",1);
      #endif
       break;
   }
   
   tmpY = 33;
   tmpLink = queryMpLink;
   i = 0;
   while(tmpLink!=NULL && i<layer3Light)
   {
     for(tmpCount=0;tmpCount<NUM_MP_PER_PAGE-1;tmpCount++)
     {
   	   tmpLink = tmpLink->next;
     }
     i++;
   }
   tmpCount = 0;
   while((tmpLink!=NULL) && (tmpCount<(NUM_MP_PER_PAGE-1)))
   {
   	  strcpy(sayStr,intToString(tmpLink->mp, 3, str));
   	  
   	  strcpy(str,"");
   	  if (strlen(sayStr)==1)
   	  {
   	  	 strcat(str,"00");
   	  }
   	  if (strlen(sayStr)==2)
   	  {
   	  	 strcat(str,"0");
   	  }	 	   	  
   	  strcat(str,sayStr);
   	 #ifdef LIGHTING
   	  guiDisplay(12,tmpY,str,1);   	  
   	 #else 
   	  guiDisplay(22,tmpY,str,1);
   	 #endif

   	  if (type==2)
   	  {
   	  	 sprintf(str,"%02d",tmpLink->port);
   	  	 guiDisplay(80,tmpY,str,1);
   	  	 
   	  	 strcpy(str,"");
   	  	 switch(tmpLink->protocol)
   	  	 {
   	  	 	 case 1:
   	  	     strcat(str,"97");
   	  	     break;

   	  	 	 case 2:
   	  	     strcat(str,"交采");
   	  	     break;

   	  	 	 case 30:
   	  	    #ifdef LIGHTING
   	  	     if (tmpLink->port==31)
   	  	     {
   	  	     	 strcat(str,"单灯");
   	  	     }
   	  	     else
   	  	     {
   	  	     	 strcat(str,"07表");
   	  	     }
   	  	    #else 
   	  	     strcat(str,"07");
   	  	    #endif
   	  	     break;
   	  	  
   	  	  #ifdef LIGHTING
   	  	 	 case 130:
   	  	     strcat(str,"线路");
   	  	     break;

   	  	 	 case 131:
   	  	     strcat(str,"报警");
   	  	     break;

   	  	 	 case 132:
   	  	     strcat(str,"照度");
   	  	     break;
   	  	  #endif
   	  	   
   	  	   default:
   	  	     sprintf(str,"%d",tmpLink->protocol);
   	  	   	 break;
   	  	 }
   	  	 guiDisplay(114, tmpY, str, 1);
   	  }
   	  else
   	  {
     	  strcpy(str,"");
     	  for(i=6; i>0; i--)
     	  {
        	switch(type)
        	{
        	  	case 0:
          	  	if(tmpLink->addr[i-1]==0x00)
          	  	{
          	  	 	strcat(str, "00");
          	  	}
          	  	else
          	  	{
          	  	 	strcat(str,digital2ToString((tmpLink->addr[i-1]/0x10)*10
          	  	        +tmpLink->addr[i-1]%0x10,sayStr));
          	  	}
        	      break;
        	      
        	    case 1:
          	  	if(tmpLink->collectorAddr[i-1]==0x00)
          	  	{
          	  	 	strcat(str, "00");
          	  	}
          	  	else
          	  	{
          	  	 	strcat(str,digital2ToString((tmpLink->collectorAddr[i-1]/0x10)*10
          	  	        +tmpLink->collectorAddr[i-1]%0x10,sayStr));
          	  	}
        	    	break;
        	}
     	  }
     	  
     	 #ifdef LIGHTING
     	  if (type==0)
     	  {
     	  	//2015-12-07,单灯控制点的ctrlTime的低4位表示控制时段了
     	  	sprintf(sayStr,  " %d", tmpLink->ctrlTime&0x0f);
     	  	strcat(str, sayStr);
     	  }
    	  guiDisplay(48, tmpY, str, 1);
     	 #else
    	  guiDisplay(65, tmpY, str, 1);
     	 #endif
    	}

   	  tmpY += 16;
   	  tmpLink = tmpLink->next;
   	  
   	  tmpCount++;
   }
	 
	 lcdRefresh(17, 145);
}

/*******************************************************
函数名称:setTeDateTime
功能描述:设置终端日期/时间(376.1国家电网集中器规约)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void setTeDateTime(INT8U lightNum)
{
	INT8U i, tmpX;
	char  str[2];
	
	menuInLayer = 3;

	guiLine(1,17,160,144,0);	
	guiLine(1,60,1,100,1);
	guiLine(160,60,160,100,1);
	guiLine(1,100,160,100,1);
	guiLine(1,60,160,60,1);
	guiLine(1,80,160,80,1);

 #ifdef LIGHTING
	guiDisplay(24, 62, "集中器时间设置", 1);
 #else	
	guiDisplay(32, 62, "终端时间设置", 1);
 #endif
  
  tmpX = 5;
  for(i=0;i<14;i++)
  {
    str[0] = dateTimeItem[i];
    str[1] = '\0';
    if (lightNum==i)
    {
      guiDisplay(tmpX,82,str,0);
    }
    else
    {
      guiDisplay(tmpX,82,str,1);
    }
    tmpX += 8;
    
    if (i==3 || i==5)
    {
      guiDisplay(tmpX,82,"-",1);
      tmpX += 8;
    }
    if (i==7)
    {
    	tmpX += 8;
    }
    if (i==9 || i==11)
    {
      guiDisplay(tmpX,82,":",1);
      tmpX += 8;
    }
  }
	 
	lcdRefresh(17,145);
}

/*******************************************************
函数名称:setTeAddr
功能描述:设置终端编号(376.1国家电网集中器规约)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void setTeAddr(INT8U lightNum)
{
	INT8U i, row, col;
	char  str[2];
	
	menuInLayer = 3;

	guiLine(1,17,160,144,0);	
	guiLine(1,50,1,110,1);
	guiLine(160,50,160,110,1);
	guiLine(1,110,160,110,1);
	guiLine(1,50,160,50,1);
	guiLine(1,70,160,70,1);

 #ifdef LIGHTING
 	guiDisplay(24,52,"集中器编号设置",1);
	guiDisplay(15,72,"集中器编号",1);
 #else
 	guiDisplay(32,52,"终端编号设置",1);
	guiDisplay(15,72,"  终端编号",1);
 #endif
	guiDisplay(15,92,"行政区划码",1);
  
  str[1] = '\0';
  row = 72;
  col = 100;
  #ifdef TE_ADDR_USE_BCD_CODE
   for(i=0;i<8;i++)
  #else
   for(i=0;i<9;i++)
  #endif
  {  
    str[0] = tmpTeAddr[i];
 
    #ifdef TE_ADDR_USE_BCD_CODE
     if (i==4)
    #else
     if (i==5)
    #endif
    {
    	 row = 92;
    	 col = 100;
    }
    if (i==lightNum)
    {
       guiDisplay(col,row,str,0);
    }
    else
    {
       guiDisplay(col,row,str,1);
    }
    col+=8;
  }

	lcdRefresh(17,145);
}

/*******************************************************
函数名称:set485Meter
功能描述:台区电表参数设置(485表)(376.1国家电网集中器规约)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void set485Meter(INT8U layer2Light,INT8U layer3Light)
{
	INT8U i, tmpX;
	char  str[2];
	
	menuInLayer = 3;

	guiLine(1,17,160,144,0);
	
	guiLine(1,17,1,140,1);
	guiLine(160,17,160,140,1);
	guiLine(52,34,52,119,1);	
	guiLine(1,17,160,17,1);
	guiLine(1,34,160,34,1);	
	guiLine(1,51,160,51,1);	
	guiLine(1,68,160,68,1);	
	guiLine(1,85,160,85,1);
	guiLine(1,102,160,102,1);
	guiLine(80,119,80,140,1);
	guiLine(1,119,160,119,1);
	guiLine(1,140,160,140,1);

	guiDisplay(16,18,"台区电表参数设置",1);
	guiDisplay(4, 35,"测量点",1);
	guiDisplay(4, 52,"速  率",1);
	guiDisplay(4, 69,"端  口",1);
	guiDisplay(4, 86,"协  议",1);
	guiDisplay(4,103,"表地址",1);

  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<4;i++)
  {
    str[0] = chrMp[0][i];
    if (layer2Light==0 && i==layer3Light)
    {
      guiDisplay(tmpX, 35, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 35, str, 1);
    }
    tmpX += 8;
  }
  
  if (layer2Light==1)
  {
    if (chrMp[1][0]>0x37)
    {
      guiDisplay(60, 52, "未知", 0);
    }
    else
    {
      guiDisplay(60, 52, chrCopyRate[chrMp[1][0]-0x30], 0);
    }
  }
  else
  {
    if (chrMp[1][0]>0x37)
    {
      guiDisplay(60, 52, "未知", 1);
    }
    else
    {
      guiDisplay(60, 52, chrCopyRate[chrMp[1][0]-0x30], 1);
    }
  }

  if (layer2Light==2)
  {
    if (chrMp[2][0]>0x32)
    {
    	guiDisplay(60, 69, "未知", 0);
    }
    else
    {
      guiDisplay(60, 69, chrCopyPort[chrMp[2][0]-0x30], 0);
    }
  }
  else
  {
    if (chrMp[2][0]>0x32)
    {
    	guiDisplay(60, 69, "未知", 1);
    }
    else
    {
      guiDisplay(60, 69, chrCopyPort[chrMp[2][0]-0x30], 1);
    }
  }

  if (layer2Light==3)
  {
    if (chrMp[3][0]>0x32)
    {
    	guiDisplay(60, 86, "未知", 0);
    }
    else
    {
      guiDisplay(60, 86, chrCopyProtocol[chrMp[3][0]-0x30], 0);
    }
  }
  else
  {
    if (chrMp[3][0]>0x32)
    {
    	guiDisplay(60, 86, "未知", 1);
    }
    else
    {
      guiDisplay(60, 86, chrCopyProtocol[chrMp[3][0]-0x30], 1);
    }
  }
  
  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<12;i++)
  {
    str[0] = chrMp[4][i];
    if (layer2Light==4 && i==layer3Light)
    {
      guiDisplay(tmpX, 103, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 103, str, 1);
    }
    tmpX += 8;
  }

	if (layer2Light==5)
	{
	  guiDisplay(24,122,"确定",0);
	}
	else
	{
	  guiDisplay(24,122,"确定",1);
	}
	guiDisplay(104,122,"取消",1);

	lcdRefresh(17,144);
}

/*******************************************************
函数名称:setCarrierMeter
功能描述:集抄电表参数设置(载波表)(376.1国家电网集中器规约)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void setCarrierMeter(INT8U layer2Light,INT8U layer3Light)
{
	INT8U i, tmpX;
 #ifdef LIGHTING
	char  str[20];
 #else
	char  str[2];
 #endif
	
	menuInLayer = 3;

	guiLine(1,17,160,144,0);

	guiLine(1,17,1,140,1);
	guiLine(160,17,160,140,1);
	guiLine(52,34,52,119,1);	
	guiLine(1,17,160,17,1);
	guiLine(1,34,160,34,1);	
	guiLine(1,51,160,51,1);	
	guiLine(1,68,160,68,1);	
	guiLine(1,85,160,85,1);
	guiLine(1,102,160,102,1);
	guiLine(80,119,80,140,1);
	guiLine(1,119,160,119,1);
	guiLine(1,140,160,140,1);

 #ifdef LIGHTING
 
	guiDisplay(24,18,"控制点参数设置",1);
	guiDisplay(4, 35,"控制点",1);
	guiDisplay(4, 52," 地址",1);
	guiDisplay(4, 69," 时段",1);
	guiDisplay(4, 86," 端口",1);
	guiDisplay(4,103," 协议",1);

  //测量点号
  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<4;i++)
  {
    str[0] = chrMp[0][i];
    if (layer2Light==0 && i==layer3Light)
    {
      guiDisplay(tmpX, 35, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 35, str, 1);
    }
    tmpX += 8;
  }
  
  //表地址
  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<12;i++)
  {
    str[0] = chrMp[1][i];
    if (layer2Light==1 && i==layer3Light)
    {
      guiDisplay(tmpX, 52, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 52, str, 1);
    }
    tmpX += 8;
  }

  //本控制点控制时段号(用费率数代替)
  str[0] = chrMp[4][0];
  if (layer2Light==2)
  {
    guiDisplay(60, 69, str, 0);
  }
  else
  {
    guiDisplay(60, 69, str, 1);
  }
  
  strcpy(str, "");
  switch(chrMp[3][0])
  {
  	case 0x31:
  		strcpy(str, "RS485-1");
  		break;
  		
  	case 0x32:
  		strcpy(str, "RS485-2");
  		break;
  		
  	case 0x30:
  		strcpy(str, "载波端口");
  		break;
  }
  if (layer2Light==3)
  {
    guiDisplay(60, 86, str, 0);
  }
  else
  {
    guiDisplay(60, 86, str, 1);
  }

  strcpy(str, "");
  switch(chrMp[2][0])
  {
  	case 0x31:
  		if (0x30==chrMp[3][0])
  		{
  		  strcpy(str, "单灯控制器");
  		}
  		else
  		{
  		  strcpy(str, "07表");
  		}
  		break;
  		
  	case 0x32:
  		strcpy(str, "97表");
  		break;
  		
  	case 0x33:
  		strcpy(str, "线路控制");
  		break;
  }
  if (layer2Light==4)
  {
    guiDisplay(60,103, str, 0);
  }
  else
  {
    guiDisplay(60,103, str, 1);
  }
  
  
	if (layer2Light==5)
	{
	  guiDisplay(24, 122, "确定", 0);
	}
	else
	{
	  guiDisplay(24, 122, "确定", 1);
	}
	guiDisplay(104, 122, "取消", 1);
	
	
 #else
 

	guiDisplay(16,18,"集抄电表参数设置",1);
	guiDisplay(4, 35,"测量点",1);
	guiDisplay(4, 52,"表地址",1);
	guiDisplay(4, 69,"协  议",1);
	guiDisplay(4, 86,"采集器",1);
	guiDisplay(4,103,"费率数",1);

  //测量点号
  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<4;i++)
  {
    str[0] = chrMp[0][i];
    if (layer2Light==0 && i==layer3Light)
    {
      guiDisplay(tmpX, 35, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 35, str, 1);
    }
    tmpX += 8;
  }
  
  //表地址
  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<12;i++)
  {
    str[0] = chrMp[1][i];
    if (layer2Light==1 && i==layer3Light)
    {
      guiDisplay(tmpX, 52, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 52, str, 1);
    }
    tmpX += 8;
  }
  
  //协议  
  if (layer2Light==2)
  {
    if (chrMp[2][0]>0x31)
    {
    	guiDisplay(60, 69, "未知", 0);
    }
    else
    {
      guiDisplay(60, 69, chrCopyProtocol[chrMp[2][0]-0x30], 0);
    }
  }
  else
  {
    if (chrMp[2][0]>0x32)
    {
    	guiDisplay(60, 69, "未知", 1);
    }
    else
    {
      guiDisplay(60, 69, chrCopyProtocol[chrMp[2][0]-0x30], 1);
    }
  }
  
  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<12;i++)
  {
    str[0] = chrMp[3][i];
    if (layer2Light==3 && i==layer3Light)
    {
      guiDisplay(tmpX, 86, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 86, str, 1);
    }
    tmpX += 8;
  }
  
  //费率数
  str[0] = chrMp[4][0];
  if (layer2Light==4)
  {
    guiDisplay(60, 103, str, 0);
  }
  else
  {
    guiDisplay(60, 103, str, 1);
  }  

	if (layer2Light==5)
	{
	  guiDisplay(24,122,"确定",0);
	}
	else
	{
	  guiDisplay(24,122,"确定",1);
	}
	guiDisplay(104,122,"取消",1);
	
 #endif

	lcdRefresh(17,144);
}

/*******************************************************
函数名称:setRlPara
功能描述:设置锐拔无线模块参数(376.1国家电网集中器规约)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void setRlPara(INT8U layer2Light,INT8U layer3Light)
{
	INT8U i, j, tmpX, tmpY;
	char  str[2];
	
	menuInLayer = 3;

	guiLine(1,17,160,144,0);
	
	guiLine(1,17,1,122,1);
	guiLine(160,17,160,122,1);
	guiLine(68,34,68,102,1);	
	guiLine(1,17,160,17,1);
	guiLine(1,34,160,34,1);	
	guiLine(1,51,160,51,1);	
	guiLine(1,68,160,68,1);	
	guiLine(1,85,160,85,1);
	guiLine(1,102,160,102,1);
	guiLine(1,122,160,122,1);

	guiDisplay(16,18,"锐拔模块参数设置",1);
	guiDisplay(4, 35,"基本功率",1);
	guiDisplay(4, 52,"最大功率",1);
	guiDisplay(4, 69,"信号强度",1);
	guiDisplay(4, 86,"信    道",1);

  //基本功率
  tmpY = 35;
  for(j=0;j<4;j++)
  {
    tmpX = 85;
    str[1] = '\0';
    for(i=0;i<2;i++)
    {
      str[0] = chrRlPara[j][i];
      if (layer2Light==j && i==layer3Light)
      {
        guiDisplay(tmpX, tmpY, str, 0);
      }
      else
      {
        guiDisplay(tmpX, tmpY, str, 1);
      }
      tmpX += 8;
    }
    
    tmpY += 17;
  }
  
	if (layer2Light==4)
	{
	  guiDisplay(64,105,"确定",0);
	}
	else
	{
	  guiDisplay(64,105,"确定",1);
	}

	lcdRefresh(17,144);
}

/*******************************************************
函数名称:setCopyForm
功能描述:设置本地通信模块抄读方式(376.1国家电网集中器规约)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void setCopyForm(INT8U lightNum)
{
	INT8U i, row, col;
	char  str[2];
	
	menuInLayer = 3;

	guiLine(1,17,160,144,0);	
	guiLine(1,50,1,110,1);
	guiLine(160,50,160,110,1);
	guiLine(1,110,160,110,1);
	guiLine(1,50,160,50,1);
	guiLine(1,70,160,70,1);

	guiDisplay(33,52,"抄读方式设置",1);
	if (lightNum==0)
	{
	  guiDisplay(24, 72, "集中器主导抄读", 0);
	  guiDisplay(24, 92, " 路由主导抄读 ", 1);
	}
	else
	{
	  guiDisplay(24, 72, "集中器主导抄读", 1);
	  guiDisplay(24, 92, " 路由主导抄读 ", 0);
	}

	lcdRefresh(17, 145);
}

/*******************************************************
函数名称:setDenizenDataType
功能描述:设置居民用户表数据类型(376.1国家电网集中器规约)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void setDenizenDataType(INT8U lightNum)
{
	INT8U i, row, col;
	char  str[2];
	
	menuInLayer = 3;

	guiLine(1,17,160,144,0);
	guiLine(1,50,1,130,1);
	guiLine(160,50,160,130,1);
	guiLine(1,130,160,130,1);
	guiLine(1,50,160,50,1);
	guiLine(1,70,160,70,1);

	guiDisplay(17,52,"户表数据类型设置",1);
	switch (lightNum)
	{
	  case 0:
	    guiDisplay(24, 72, "实时+冻结数据", 0);
	    guiDisplay(24, 92, "仅实时数据   ", 1);
	    guiDisplay(24,112, "仅实时总示值 ", 1);
	    break;
	  
	  case 1:
	    guiDisplay(24, 72, "实时+冻结数据", 1);
	    guiDisplay(24, 92, "仅实时数据   ", 0);
	    guiDisplay(24,112, "仅实时总示值 ", 1);
	    break;

	  case 2:
	    guiDisplay(24, 72, "实时+冻结数据", 1);
	    guiDisplay(24, 92, "仅实时数据   ", 1);
	    guiDisplay(24,112, "仅实时总示值 ", 0);
	    break;
	}
 
	lcdRefresh(17, 145);
}

/*******************************************************
函数名称:setCycleType
功能描述:设置轮显数据类型(376.1国家电网集中器规约)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void setCycleType(INT8U lightNum)
{
	INT8U i, row, col;
	char  str[2];
	
	menuInLayer = 3;

	guiLine(1,17,160,144,0);
	guiLine(1,50,1,130,1);
	guiLine(160,50,160,130,1);
	guiLine(1,130,160,130,1);
	guiLine(1,50,160,50,1);
	guiLine(1,70,160,70,1);

	guiDisplay(33,52,"轮显数据设置",1);
	switch (lightNum)
	{
	  case 0:
	    guiDisplay(24, 72, "所有表计", 0);
	    guiDisplay(24, 92, "仅台区表", 1);
	    break;
	  
	  case 1:
	    guiDisplay(24, 72, "所有表计", 1);
	    guiDisplay(24, 92, "仅台区表", 0);
	    break;
	}

	lcdRefresh(17, 145);
}


#endif //MENU_FOR_CQ_CANON

#else     //PLUG_IN_CARRIER_MODULE专变III型菜单

/*******************************************************
函数名称:adjustCommParaLightIII
功能描述:调整通信参数高亮项(专变III型)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void adjustCommParaLightIII(INT8U leftRight)
{
 	 INT8U tmpData;
 	 
 	 switch (rowOfLight)
 	 {
 	 	  case 0:
 	 	  	tmpData = 4;
 	 	  	break;
 	 	  	
 	 	  case 1:
 	 	   #ifdef TE_ADDR_USE_BCD_CODE
 	 	    tmpData = 4;
 	 	   #else
 	 	    tmpData = 5;
 	 	   #endif
 	 	    break;

 	 	  case 2:
 	 	    tmpData = 1;
 	 	    break;
 	 	    
 	 	  case 3:
 	 	  case 4:
 	 	  	tmpData = 21;
 	 	  	break;
 	 }
 	 
 	 if (leftRight==1)  //右键
 	 {
 	   if (keyLeftRight>=tmpData-1)
 	   {
 	 	   keyLeftRight = 0;
 	   }
 	   else
 	   {
 	 	   keyLeftRight++;
 	   }
 	 }
 	 else
 	 {
 	   if (keyLeftRight==0)
 	   {
 	 	   keyLeftRight = tmpData-1;
 	   }
 	   else
 	   {
 	 	   keyLeftRight--;
 	   }
 	 }
	 
	 if (rowOfLight==3 
	 	 	   || rowOfLight==4)
	 {
	 	 if ((keyLeftRight+1)%4==0 && keyLeftRight<16)
	 	 {
	 	   if (leftRight==1)
	 	   {
	 	     keyLeftRight++;
	 	   }
	 	   else
	 	   {
	 	   	 keyLeftRight--;
	 	   }
	 	 }
	 }
}


void setZbMeter(INT8U layer2Light,INT8U layer3Light)
{
	INT8U i, tmpX;
	char  str[2];
	
	menuInLayer = 3;

	guiLine(1,17,160,160,0);
	
	guiLine(1,17,1,156,1);
	guiLine(160,17,160,156,1);
	guiLine(52,34,52,136,1);	
	guiLine(1,17,160,17,1);
	guiLine(1,34,160,34,1);	
	guiLine(1,51,160,51,1);	
	guiLine(1,68,160,68,1);	
	guiLine(1,85,160,85,1);
	guiLine(1,102,160,102,1);	
	guiLine(80,119,80,156,1);
	guiLine(132,119,132,136,1);
	guiLine(1,119,160,119,1);
	guiLine(1,136,160,136,1);	
	guiLine(1,156,160,156,1);
	

	guiDisplay(49,18,"表计设置",1);
	guiDisplay(4, 35,"测量点",1);
	guiDisplay(4, 52,"速  率",1);
	guiDisplay(4, 69,"端  口",1);
	guiDisplay(4, 86,"协  议",1);
	guiDisplay(4,103,"表地址",1);
	guiDisplay(4,120,"大类号",1);
	guiDisplay(82,120,"小类号",1);

  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<4;i++)
  {
    str[0] = chrMp[0][i];
    if (layer2Light==0 && i==layer3Light)
    {
      guiDisplay(tmpX, 35, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 35, str, 1);
    }
    tmpX += 8;
  }
  
  if (layer2Light==1)
  {
    if (chrMp[1][0]>0x37)
    {
      guiDisplay(60, 52, "未知", 0);
    }
    else
    {
      guiDisplay(60, 52, chrCopyRate[chrMp[1][0]-0x30], 0);
    }
  }
  else
  {
    if (chrMp[1][0]>0x37)
    {
      guiDisplay(60, 52, "未知", 1);
    }
    else
    {
      guiDisplay(60, 52, chrCopyRate[chrMp[1][0]-0x30], 1);
    }
  }

  if (layer2Light==2)
  {
    if (chrMp[2][0]>0x32)
    {
    	guiDisplay(60, 69, "未知", 0);
    }
    else
    {
      guiDisplay(60, 69, chrCopyPort[chrMp[2][0]-0x30], 0);
    }
  }
  else
  {
    if (chrMp[2][0]>0x32)
    {
    	guiDisplay(60, 69, "未知", 1);
    }
    else
    {
      guiDisplay(60, 69, chrCopyPort[chrMp[2][0]-0x30], 1);
    }
  }

  if (layer2Light==3)
  {
    if (chrMp[3][0]>0x37)
    {
    	guiDisplay(60, 86, "未知", 0);
    }
    else
    {
      guiDisplay(60, 86, chrCopyProtocol[chrMp[3][0]-0x30], 0);
    }
  }
  else
  {
    if (chrMp[3][0]>0x37)
    {
    	guiDisplay(60, 86, "未知", 1);
    }
    else
    {
      guiDisplay(60, 86, chrCopyProtocol[chrMp[3][0]-0x30], 1);
    }
  }
  
  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<12;i++)
  {
    str[0] = chrMp[4][i];
    if (layer2Light==4 && i==layer3Light)
    {
      guiDisplay(tmpX, 103, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 103, str, 1);
    }
    tmpX += 8;
  }

  tmpX = 60;
  str[1] = '\0';
  for(i=0;i<2;i++)
  {
    str[0] = chrMp[5][i];
    if (layer2Light==5 && i==layer3Light)
    {
      guiDisplay(tmpX, 120, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 120, str, 1);
    }
    tmpX += 8;
  }

  tmpX = 140;
  str[1] = '\0';
  for(i=0;i<2;i++)
  {
    str[0] = chrMp[6][i];
    if (layer2Light==6 && i==layer3Light)
    {
      guiDisplay(tmpX, 120, str, 0);
    }
    else
    {
      guiDisplay(tmpX, 120, str, 1);
    }
    tmpX += 8;
  }
  
	if (layer2Light==7)
	{
	  guiDisplay(24,138,"确定",0);
	}
	else
	{
	  guiDisplay(24,138,"确定",1);
	}
	guiDisplay(104,138,"取消",1);

	lcdRefresh(17,160);
}


/**************************************************
函数名称:userInterface
功能描述:人机接口处理(376.1国家电网专变III型菜单规约)
调用函数:
被调用函数:
输入参数:void *arg
输出参数:
返回值：状态
***************************************************/
void userInterface(BOOL secondChanged)
{
	 METER_DEVICE_CONFIG meterConfig;
   char      str[30],strX[30];
   INT8U     i,nextInfo;
   INT16U    j;
   INT16U    tmpAddr;                    //临时终端地址
   char      *tmpChar;
   INT16U    tmpData;
   
   //ly,2011-8-26,add
   register int fd, interface, retn = 0;
   struct ifreq buf[MAXINTERFACES];
   struct ifconf ifc;

   if (secondChanged==TRUE)
   {
     #ifdef LOAD_CTRL_MODULE
       ctrlAlarmDisplay();   //控制过程显示
     #endif
        
     if (aberrantAlarm.aberrantFlag==1)
     {
       guiLine(60,1,76,15,0);
   
       if (aberrantAlarm.blinkCount==1)
       {
         aberrantAlarm.blinkCount = 2;
         guiDisplay(60, 1, "！", 1);
         guiLine(60,1,62,4,0);
         guiDisplay(60, 2, "○", 1);
       }
       else
       {
   	     guiDisplay(60, 1, digital2ToString(aberrantAlarm.eventNum,str), 1);
   	     aberrantAlarm.blinkCount = 1;
   	   }
       
       if (compareTwoTime(aberrantAlarm.timeOut, sysTime))
       {
         guiLine(60,1,76,15,0);
         aberrantAlarm.aberrantFlag = 0;
       }
       
       lcdRefresh(1, 16);
     }
     
     //title时间
     refreshTitleTime();
     
     if (lcdLightOn==LCD_LIGHT_ON)
     {
       if (compareTwoTime(lcdLightDelay, sysTime))
       {
     	   lcdLightOn = LCD_LIGHT_OFF;
     	   lcdBackLight(LCD_LIGHT_OFF);
     	   
     	   defaultMenu();
       }
     }
     
     if (setParaWaitTime!=0 && setParaWaitTime!=0xfe)
     {
    		setParaWaitTime--;
     }
  	 
  	 if (ctrlCmdWaitTime!=0 && ctrlCmdWaitTime!=0xfe)
  	 {
  			ctrlCmdWaitTime--;
  			if (ctrlCmdWaitTime<1)
  			{
  			 	 //resumeLcd = 1;                 //恢复LCD菜单显示标志
  			}
  	 }

  	 if (pageWait!=0)
  	 {
  			pageWait--;
  			if (pageWait<1)
  			{
  			 	//resumeLcd = 1;                  //恢复常显画面
     	    defaultMenu();
  			}
  	 }
  	
  	 //允许合闸提示音时长
  	 #ifdef LOAD_CTRL_MODULE
  	  if (gateCloseWaitTime!=0 && gateCloseWaitTime!=0xfe)
  	  {
  		  gateCloseWaitTime--;
  	  }
  	 #endif
   }
   
   //4.告警处理
   alarmProcess();

   if ((keyValue = ioctl(fdOfIoChannel, READ_KEY_VALUE, 0))!=0)
   {
      keyPressCount++;
      
      if (keyPressCount>keyCountMax)
      {
        if (menuInLayer!=99)
        {
           pageWait = 10;
        }
        else
        {
           if (keyValue==KEY_OK)
           {
          	  pageWait = 10;
          	  menuInLayer = 1;
           }
        }

        lcdBackLight(LCD_LIGHT_ON);
        lcdLightOn = LCD_LIGHT_ON;
        
        //如果所有路都处于合闸状态且控制状态变量(ctrlStatus)不为NONE_CTRL,
        //则将ctrlStatus置为NONE_CTRL,使屏幕恢复常显画面
        for(i=0;i<ctrlStatus.numOfAlarm;i++)
        {
           if (ctrlStatus.allPermitClose[i]==1)
           {
        	    for(j=i;j<ctrlStatus.numOfAlarm;j++)
        	    {
        	       ctrlStatus.aQueue[j] = ctrlStatus.aQueue[j+1];
        	       ctrlStatus.allPermitClose[j]=ctrlStatus.allPermitClose[j+1];
        	    }
        	    ctrlStatus.numOfAlarm--;
        	    if (ctrlStatus.numOfAlarm==0)
        	    {
        	       menuInLayer=1;
        	    }
        	 }
        }
        lcdLightDelay = nextTime(sysTime, 1, 0);

        keyPressCount = 0;
        
        switch(keyValue)
        {
      	  case KEY_OK:       //确定
      	  	switch(menuInLayer)
      	  	{
      	  		case 1:  //菜单第1层
      	    		switch (layer1MenuLight)
      	    		{
      	    		  case 0:  //实时数据
      	    		  case 1:  //参数定值
      	    		    if (layer1MenuLight==1)
      	    		    {
      	    		    	if (moduleType==ETHERNET)
      	    		    	{      	    		    		
      	    		    	  layer2MenuNum[1] = 15;
      	    		    	}
      	    		    	else
      	    		    	{
      	    		    	  layer2MenuNum[1] = 14;
      	    		    	}
      	    		    }
      	    		    layer2Menu(layer2MenuLight[layer1MenuLight], layer1MenuLight);
      	    		    break;
      	    		    
      	    		  case 2:  //控制状态
      	    		 	 	layer2MenuLight[2] = 0;
      	    		  	controlStatus(layer2MenuLight[2]);
      	    		  	break;
      	    		  	
      	    		  case 3:  //电能表示数
                    countParameter (0x04, 10, &meterDeviceNum);
                    layer2MenuNum[layer1MenuLight] = meterDeviceNum;
      	    		 	 	layer2MenuLight[layer1MenuLight] = 0;
      	    		 	 	queryMpLink = initPortMeterLink(0xff);
      	    		  	meterVisionValue(layer2MenuLight[layer1MenuLight]);
      	    		  	break;
      	    		  
      	    		  case 4:  //中文信息
      	    		  	layer2MenuNum[4] = chnMessage.numOfMessage;
          	  	 	 	layer2MenuLight[4] = 0;
          	  	 	 	numOfPage = 0;
          	  	 	 	chinese(layer2MenuLight[4],numOfPage,0);
      	    		  	break;
      	    		  	
      	    		  case 5:  //购电信息
      	    		  	layer2MenuLight[5] = 0;
      	    		  	chargeInfo(0);
      	    		  	break;
      	    		  	
      	    		  case 6:  //终端信息
      	    		 	 	layer2MenuLight[6] = 0;
      	    		 	 	if (ifHasAcModule==TRUE)
      	    		 	 	{
      	    		 	 	  layer2MenuNum[6] = 17;
      	    		 	 	}
      	    		 	 	else
      	    		 	 	{
      	    		 	 	  layer2MenuNum[6] = 3;
      	    		 	 	}
      	    	 	 	  terminalInfo(layer2MenuLight[6]);
      	    		  	break;
      	    		}
      	  		 	break;
      	  		
      	  		case 2:   //菜单第2层
      	  			switch(layer1MenuLight)
      	  		  {
      	  		  	case 0:  //实时数据
      	  		  	 	switch(layer2MenuLight[layer1MenuLight])
      	  		  	 	{
      	  		  	 		 case 0:  //当前功率
      	  		  	 		 	 layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = totalAddGroup.numberOfzjz+pulseConfig.numOfPulse;
      	  		  	 		 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	  		  	 		 	 currentPower(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	  		  	 		 	 break;
      	  		  	 		 	 
      	  		  	 		 case 1:  //当前电量
                         layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = meterDeviceNum;
      	    		 	 	     layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	     queryMpLink = initPortMeterLink(0xff);
      	    		 	 	     currentEnergy(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	  		  	 		 	 break;
      	  		  	 		 	 
      	  		  	 		 case 2:  //功率曲线
          	    		 	 	 mpPowerCurveLight = 0;
          	    		 	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
          	    		 	 	 fillTimeStr();   //用当前日期填充查询日期字符串
          	    		 	 	 queryMpLink = initPortMeterLink(0xff);
          	    		 	 	 powerCurve(mpPowerCurveLight,layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
          	    		 	 	 break;
      	  		  	 		 	 
      	  		  	 		 case 3:  //开关状态
      	  		  	 		 	 statusOfSwitch();
      	  		  	 		 	 break;
      	  		  	 		 	 
      	  		  	 		 case 4:  //功控记录
                         searchCtrlEvent(ERC(6));
                         tmpEventShow = eventLinkHead;
                         eventRecordShow(ERC(6));
      	  		  	 		 	 break;

      	  		  	 		 case 5:  //电控记录
                         searchCtrlEvent(ERC(7));
                         tmpEventShow = eventLinkHead;
                         eventRecordShow(ERC(7));
      	  		  	 		 	 break;

      	  		  	 		 case 6:  //遥控记录
                         searchCtrlEvent(ERC(5));
                         tmpEventShow = eventLinkHead;
                         eventRecordShow(ERC(5));
      	  		  	 		 	 break;
      	  		  	 		 	 
      	  		  	 		 case 7:  //失电记录
                         searchCtrlEvent(ERC(14));
                         tmpEventShow = eventLinkHead;
                         eventRecordShow(ERC(14));
      	  		  	 		 	 break;
      	  		  	 	}
      	  		  	 	break;
      	  		  	
      	  		  	case 1:  //参数定值
      	  		  		switch(layer2MenuLight[layer1MenuLight])
      	  		  		{
      	  		  			 case 0:  //时段控参数
                         layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                         periodPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	  		  			 	 break;
      	  		  			 
      	  		  			 case 1:  //厂休控参数
                         layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	  		  			 	 wkdPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	  		  			 	 break;
      	  		  			 	 
      	  		  			 case 2:  //下浮控参数
                         layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	  		  			 	 powerDownPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	  		  			 	 break;      	  		  			 	 

      	  		  			 case 3:  //KvKiKp
      	  		  			 	 queryMpLink = initPortMeterLink(0xff);
      	  		  			 	 tmpMpLink = queryMpLink;
      	  		  			 	 kvkikp(tmpMpLink);
      	  		  			 	 break;

      	  		  			 case 4:  //电能表参数
      	  		  			 	 queryMpLink = initPortMeterLink(0xff);
      	  		  			 	 tmpMpLink = queryMpLink;
      	  		  			 	 meterPara(tmpMpLink);
      	  		  			 	 break;
      	  		  			 	 
      	  		  			 case 5:  //配置参数
      	  		  			 	 configPara(88,88);
      	  		  			 	 break;
      	  		  			 	 
                       case 6:  //虚拟专网用户名密码
                         strcpy(tmpVpnUserName, (char *)vpn.vpnName);
                         tmpVpnUserName[32] = '\0';
                         trim(tmpVpnUserName);
                        
     	 	 	  		 	 	     for(i=strlen(tmpVpnUserName); i<32; i++)
     	 	 	  		 	 	     {
     	 	 	  		 	 	    	 tmpVpnUserName[i] = ' ';
     	 	 	  		 	 	     }
     	 	 	  		 	 	     tmpVpnUserName[32] = '\0';

                         strcpy(tmpVpnPw, (char *)vpn.vpnPassword);
                         tmpVpnPw[32] = '\0';
                         trim(tmpVpnPw);
     	 	 	  		 	 	     for(i=strlen(tmpVpnPw); i<32; i++)
     	 	 	  		 	 	     {
     	 	 	  		 	 	    	 tmpVpnPw[i] = ' ';
     	 	 	  		 	 	     }
     	 	 	  		 	 	     tmpVpnPw[32] = '\0';
     	 	 	  		 	 	     keyLeftRight = 0;
                         inputStatus  = STATUS_NONE;
                         setVpn(keyLeftRight);
                         break;
                         
                      case 7:    //表计配置
                        strcpy(chrMp[0],"0001");
                        strcpy(chrMp[1],"0");
                        strcpy(chrMp[2],"1");
                        strcpy(chrMp[3],"0");
                        strcpy(chrMp[4],"000000000001");
                        strcpy(chrMp[5],"00");
                        strcpy(chrMp[6],"00");
                        keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  setZbMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
                      	break;
                      
                      case 8:    //立即抄表
                      	copyCtrl[0].nextCopyTime = nextTime(sysTime, 0, 5);
                      	copyCtrl[1].nextCopyTime = nextTime(sysTime, 0, 5);
                      	copyCtrl[2].nextCopyTime = nextTime(sysTime, 0, 5);
                        guiLine(10,55,150,105,0);
                        guiLine(10,55,10,105,1);
                        guiLine(150,55,150,105,1);
                        guiLine(10,55,150,55,1);
                        guiLine(10,105,150,105,1);
                        guiDisplay(16,70,"5秒钟后开始抄表!",1);
                        lcdRefresh(10,120);
                        menuInLayer--;                        
                      	break;

      	    	 	 	  	case 9:    //终端程序升级
      	    	 	 	  	  uDiskUpgrade();
      	    	 	 	  	  break;

                      case 10:   //终端级联参数设置
                        sprintf(chrMp[0], "%01d", cascadeCommPara.commPort);   //级联端口
                        if (cascadeCommPara.flagAndTeNumber&0x80)
                        {
                          strcpy(chrMp[1],"2");      //被级联
                        }
                        else
                        {
                          strcpy(chrMp[1],"1");      //级联方
                        }
                        switch (cascadeCommPara.flagAndTeNumber&0xf)
                        {
                        	case 1:
                           sprintf(chrMp[2],"%02x%02x%05d",cascadeCommPara.divisionCode[1],cascadeCommPara.divisionCode[0],cascadeCommPara.cascadeTeAddr[0]|cascadeCommPara.cascadeTeAddr[1]<<8);      //终端1
                           strcpy(chrMp[3],"000000000");
                           strcpy(chrMp[4],"000000000");
                           break;
                        	case 2:
                           sprintf(chrMp[2],"%02x%02x%05d",cascadeCommPara.divisionCode[1],cascadeCommPara.divisionCode[0],cascadeCommPara.cascadeTeAddr[0]|cascadeCommPara.cascadeTeAddr[1]<<8);      //终端1
                           sprintf(chrMp[3],"%02x%02x%05d",cascadeCommPara.divisionCode[3],cascadeCommPara.divisionCode[2],cascadeCommPara.cascadeTeAddr[2]|cascadeCommPara.cascadeTeAddr[3]<<8);      //终端1
                           strcpy(chrMp[4],"000000000");
                           break;
                        	case 3:
                           sprintf(chrMp[2],"%02x%02x%05d",cascadeCommPara.divisionCode[1],cascadeCommPara.divisionCode[0],cascadeCommPara.cascadeTeAddr[0]|cascadeCommPara.cascadeTeAddr[1]<<8);      //终端1
                           sprintf(chrMp[3],"%02x%02x%05d",cascadeCommPara.divisionCode[3],cascadeCommPara.divisionCode[2],cascadeCommPara.cascadeTeAddr[2]|cascadeCommPara.cascadeTeAddr[3]<<8);      //终端1
                           sprintf(chrMp[4],"%02x%02x%05d",cascadeCommPara.divisionCode[5],cascadeCommPara.divisionCode[4],cascadeCommPara.cascadeTeAddr[4]|cascadeCommPara.cascadeTeAddr[5]<<8);      //终端1
                           break;
                           
                        	default:
                           strcpy(chrMp[2],"000000000");
                           strcpy(chrMp[3],"000000000");
                           strcpy(chrMp[4],"000000000");
                           
                           strcpy(chrMp[1],"0");      //不级联
                           break;
                        }
                        
                        keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	 	  setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
                      	break;
                      	
                      case 11:    //调节LCD对比度
      	    	 	 	  	  setLcdDegree(lcdDegree);
      	    	 	 	  	  break;
      	    	 	 	  	
    	    	 	 	  	  case 12:    //维护接口模式设置
    	    	 	 	  	  	if (mainTainPortMode==0x55)
    	    	 	 	  	  	{
    	    	 	 	  	  	  keyLeftRight = 1;
    	    	 	 	  	  	}
    	    	 	 	  	  	else
    	    	 	 	  	  	{
    	    	 	 	  	  	  keyLeftRight = 0;
    	    	 	 	  	  	}
    	    	 	 	  	  	setMainTain(keyLeftRight);
    	    	 	 	  	  	break;

    	    	 	 	  	  case 13:    //第2路485口功能设置
    	    	 	 	  	  	if (rs485Port2Fun==0x55)
    	    	 	 	  	  	{
    	    	 	 	  	  	  keyLeftRight = 1;
    	    	 	 	  	  	}
    	    	 	 	  	  	else
    	    	 	 	  	  	{
    	    	 	 	  	  	  keyLeftRight = 0;
    	    	 	 	  	  	}
    	    	 	 	  	  	setRs485Port2(keyLeftRight);
    	    	 	 	  	  	break;

      	    	 	 	  	case 14:    //以太网参数设置
                        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
                        {
                          ifc.ifc_len = sizeof buf;
                          ifc.ifc_buf = (caddr_t) buf;
                          if (!ioctl(fd, SIOCGIFCONF, (char *) &ifc))
                          {
                            interface = ifc.ifc_len / sizeof(struct ifreq);
                            while (interface-- > 0)
                            {
                              if (strstr(buf[interface].ifr_name,"eth0"))
                              {
                               /*Get HW ADDRESS of the net card */
                                if (!(ioctl(fd, SIOCGIFHWADDR, (char *) &buf[interface]))) 
                                {
                                    sprintf(tmpEthMac, "%02x%02x%02x%02x%02x%02x",
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[0],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[1],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[2],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[3],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[4],
                                            (unsigned char) buf[interface].ifr_hwaddr.sa_data[5]); // 利用sprintf转换成char *
                                }
                              }
                            }//end of while
                          }
                          else
                          {
                            perror("cpm: ioctl");
                          }
                        }
                        else
                        {
                          perror("cpm: socket");
                        }
                     
                        close(fd);
                       	
                       	strcpy(chrEthPara[0],intToIpadd(teIpAndPort.teIpAddr[0]<<24 | teIpAndPort.teIpAddr[1]<<16 | teIpAndPort.teIpAddr[2]<<8 | teIpAndPort.teIpAddr[3],str));
                       	strcpy(chrEthPara[1],intToIpadd(teIpAndPort.mask[0]<<24 | teIpAndPort.mask[1]<<16 | teIpAndPort.mask[2]<<8 | teIpAndPort.mask[3],str));
                       	strcpy(chrEthPara[2],intToIpadd(teIpAndPort.gateWay[0]<<24 | teIpAndPort.gateWay[1]<<16 | teIpAndPort.gateWay[2]<<8 | teIpAndPort.gateWay[3],str));
                        chrEthPara[3][0] = teIpAndPort.ethIfLoginMs;
                        keyLeftRight = 0;
                        layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                        layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 4;
      	    	 	 	  		setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		break;
      	  		  		}
      	  		  		break;

                  case 2:  //控制状态
      	    		 	 	if (layer2MenuLight[layer1MenuLight]+2>totalAddGroup.numberOfzjz)
      	    		 	 	{
      	    		 	 		layer2MenuLight[layer1MenuLight]=0;
      	    		 	 	}
      	    		 	 	else
      	    		 	 	{
      	    		 	 		layer2MenuLight[layer1MenuLight]++;
      	    		 	 	}
                  	controlStatus(layer2MenuLight[layer1MenuLight]);
                  	break;

      	  		  	case 3:  //电能表示数
 	  		  	 		 	  if (layer2MenuNum[layer1MenuLight]<2)
 	  		  	 		 	  {
 	  		  	 		 	 	  layer2MenuLight[layer1MenuLight] = 0;
 	  		  	 		 	  }
 	  		  	 		 	  else
 	  		  	 		 	  {
 	  		  	 		 	    if(layer2MenuLight[layer1MenuLight]>=layer2MenuNum[layer1MenuLight]-1)
 	  		  	 		 	    {
 	  		  	 		 	      layer2MenuLight[layer1MenuLight] = 0;
 	  		  	 		 	    }
 	  		  	 		 	    else
 	  		  	 		 	    {
 	  		  	 		 	   	  layer2MenuLight[layer1MenuLight]++;
 	  		  	 		 	    }
 	  		  	 		 	  }
      	    		  	meterVisionValue(layer2MenuLight[layer1MenuLight]);
      	    		  	break;

      	  		  	case 4:  //中文信息
                    nextInfo = 0;
	  	 	 	  	      if (chnMessage.message[layer3MenuLight[2][1]].len<=160)
	  	 	 	  	      {
	  	 	 	  	         nextInfo = 1;
	  	 	 	  	      }
	  	 	 	  	      else
	  	 	 	  	      {
                       if (chnMessage.message[layer3MenuLight[2][1]].len%160!=0)
                       {
	  	 	 	  	      	    if (numOfPage<chnMessage.message[layer3MenuLight[2][1]].len/160)
	  	 	 	  	            {
	  	 	 	  	      	       numOfPage++;
	  	 	 	  	            }
	  	 	 	  	            else
	  	 	 	  	            	nextInfo = 1;
	  	 	 	  	         }
	  	 	 	  	         else
	  	 	 	  	         {
	  	 	 	  	      	    if (numOfPage<chnMessage.message[layer3MenuLight[2][1]].len/160-1)
	  	 	 	  	            {
	  	 	 	  	      	       numOfPage++;
	  	 	 	  	            }
	  	 	 	  	            else
	  	 	 	  	            	nextInfo = 1;
	  	 	 	  	         }
	  	 	 	  	      }

	  	 	 	  	      if (nextInfo==1)
	  	 	 	  	      {
	  	 	 	  	         numOfPage = 0;
	  	 	 	  	         if (layer3MenuLight[2][1]+1>chnMessage.numOfMessage-1)
	  	 	 	  	         {
	  	 	 	  		          layer3MenuLight[2][1] = 0;
	  	 	 	             }
	  	 	 	             else
	  	 	 	             {
	  	 	 	    	          layer3MenuLight[2][1]++;
	  	 	 	    	       }
	  	 	 	    	    }
	  	 	 	    	  	chinese(layer3MenuLight[2][1],numOfPage,0);
      	  		  		break;

      	  		  	case 5:
      	  		  		if(layer2MenuLight[5]+2>totalAddGroup.numberOfzjz)
      	  		  		{
      	  		  			 layer2MenuLight[5] = 0;
      	  		  		}
      	  		  		else
      	  		  		{
      	  		  			 layer2MenuLight[5]++;
      	  		  		}      	  		  		
      	  		  		chargeInfo(layer2MenuLight[5]);
      	  		  		break;
      	  		  	
      	  		    case 6:
      	    		 	 	if (layer2MenuLight[6]+2>layer2MenuNum[6])
      	    		 	 	{
      	    		 	 		layer2MenuLight[6]=0;
      	    		 	 	}
      	    		 	 	else
      	    		 	 	{
      	    		 	 		layer2MenuLight[6]++;
      	    		 	 	}
      	    	 	 	  terminalInfo(layer2MenuLight[6]);
      	  		    	break;
      	  		  }
      	  		  break;
      	  		  
      	  		case 3:  //菜单第3层
      	  			switch(layer1MenuLight)
      	  			{
      	  				 case 0:  //实时数据
      	  				 	 switch(layer2MenuLight[layer1MenuLight])
      	  				 	 {
      	  		  	 		 case 0:  //当前功率
      	  		  	 		 	 if (layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<2)
      	  		  	 		 	 {
      	  		  	 		 	 	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	  		  	 		 	 }
      	  		  	 		 	 else
      	  		  	 		 	 {
      	  		  	 		 	   if(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-2)
      	  		  	 		 	   {
      	  		  	 		 	   	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	  		  	 		 	   }
      	  		  	 		 	   else
      	  		  	 		 	   {
      	  		  	 		 	   	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;      	  		  	 		 	   	  
      	  		  	 		 	   }
      	  		  	 		 	 }
      	  		  	 		 	 currentPower(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	  		  	 		 	 break;

      	  		  	 		 case 1:  //当前电量
      	  		  	 		 	 if (layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<2)
      	  		  	 		 	 {
      	  		  	 		 	 	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	  		  	 		 	 }
      	  		  	 		 	 else
      	  		  	 		 	 {
      	  		  	 		 	   if(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-2)
      	  		  	 		 	   {
      	  		  	 		 	   	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	  		  	 		 	   }
      	  		  	 		 	   else
      	  		  	 		 	   {
      	  		  	 		 	   	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;      	  		  	 		 	   	  
      	  		  	 		 	   }
      	  		  	 		 	 }
      	  		  	 		 	 currentEnergy(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	  		  	 		 	 break;
      	  		  	 		 	 
      	  		  	 		 case 2:  //功率曲线      	  		  	 		 	 
      	    		 	 	     //如果是3-1(选择查询日期)的确认则要判断日期是否正确
      	    		 	 	     if (mpPowerCurveLight==0)
      	    		 	 	     {
      	    		 	 	 	     //判断输入的日期是否正确
      	    		 	 	 	     if (checkInputTime()==FALSE)
      	    		 	 	 	     {
      	    		 	 	 	  	   return;
      	    		 	 	 	     }
      	    		 	 	 	  
      	    		 	 	 	     if (meterDeviceNum==0)
      	    		 	 	 	     {
      	    		 	 	 	  	   guiDisplay(12, 110, "未配置测量点信息!", 1);
      	    		 	 	 	  	   lcdRefresh(17, 130);
      	    		 	 	 	  	   return;
      	    		 	 	 	     }
      	    		 	 	     }
      	    		 	 	 
      	    		 	 	     //按确定键可实现不同分类的屏显间的切换
      	    		 	 	     if (mpPowerCurveLight>=2)
      	    		 	 	     {
      	    		 	 	  	   mpPowerCurveLight=0;
      	    		 	 	     }
      	    		 	 	     else
      	    		 	 	     {
      	    		 	 	  	   mpPowerCurveLight++;
      	    		 	 	     }
                     
                         if (layer2MenuLight[layer1MenuLight]!=0)
                         {
                           keyLeftRight = 0;
                           layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = meterDeviceNum;
                           layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                         }
      	    		 	 	     powerCurve(mpPowerCurveLight,layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
      	    		 	 	     break;
      	    		 	 	     
      	    		 	 	   case 4:  //功控记录
      	    		 	 	   case 5:  //电控记录
      	    		 	 	   case 6:  //遥控记录
      	    		 	 	   case 7:  //失电记录
      	    		 	 	   	 if (eventLinkHead==NULL)
      	    		 	 	   	 {
      	    		 	 	   	 	 tmpEventShow = eventLinkHead;
      	    		 	 	   	 }
      	    		 	 	   	 else
      	    		 	 	   	 {
      	    		 	 	   	   if (tmpEventShow->next!=NULL)
      	    		 	 	   	   {
      	    		 	 	   	 	    tmpEventShow = tmpEventShow->next;
      	    		 	 	   	   }
      	    		 	 	   	   else
      	    		 	 	   	   {
      	    		 	 	   	 	    tmpEventShow = eventLinkHead;
      	    		 	 	   	 	 }
      	    		 	 	   	 }

      	    		 	 	   	 switch(layer2MenuLight[layer1MenuLight])
      	    		 	 	   	 {
      	    		 	 	   	   case 4:
      	    		 	 	   	     eventRecordShow(ERC(6));
      	    		 	 	   	     break;

      	    		 	 	   	   case 5:
      	    		 	 	   	     eventRecordShow(ERC(7));
      	    		 	 	   	     break;

      	    		 	 	   	   case 6:
      	    		 	 	   	     eventRecordShow(ERC(5));
      	    		 	 	   	     break;
      	    		 	 	   	     
      	    		 	 	   	   case 7:
      	    		 	 	   	     eventRecordShow(ERC(14));
      	    		 	 	   	     break;
      	    		 	 	   	 }      	    		 	 	   	 
      	    		 	 	   	 break;
      	  		  	 	 }
      	  		  	 	 break;

      	  				 case 1:  //参数定值
      	  				 	 switch(layer2MenuLight[layer1MenuLight])
      	  				 	 {
                       case 0:   //时段控参数
                         if ((layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]+2)>totalAddGroup.numberOfzjz)
                         {
                         	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                         }
                         else
                         {
                         	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
                         }
                         periodPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
                         break;

                       case 1:   //厂休控参数
                         if ((layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]+2)>totalAddGroup.numberOfzjz)
                         {
                         	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                         }
                         else
                         {
                         	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
                         }
                         wkdPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
                         break;
                         
                       case 2:  //下浮控参数
                         if ((layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]+2)>totalAddGroup.numberOfzjz)
                         {
                         	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                         }
                         else
                         {
                         	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
                         }
                         powerDownPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
                       	 break;

      	  				 	 	 case 3:  //KvKiKp
      	  				 	 	 	 if (tmpMpLink!=NULL)
      	  				 	 	 	 {
      	  				 	 	 	   if (tmpMpLink->next!=NULL)
      	  				 	 	 	   {
      	  				 	 	 	 	    tmpMpLink = tmpMpLink->next;
      	  				 	 	 	   }
      	  				 	 	 	   else      	  				 	 	 	 
      	  				 	 	 	   {
      	  				 	 	 	 	    tmpMpLink = queryMpLink;
      	  				 	 	 	   }
      	  				 	 	 	   kvkikp(tmpMpLink);
      	  				 	 	 	 }
      	  				 	 	 	 break;

      	  				 	 	 case 4:  //电能表参数
      	  				 	 	 	 if (tmpMpLink!=NULL)
      	  				 	 	 	 {
      	  				 	 	 	   if (tmpMpLink->next!=NULL)
      	  				 	 	 	   {
      	  				 	 	 	 	   tmpMpLink = tmpMpLink->next;
      	  				 	 	 	   }
      	  				 	 	 	   else      	  				 	 	 	 
      	  				 	 	 	   {
      	  				 	 	 	 	    tmpMpLink = queryMpLink;
      	  				 	 	 	   }
      	  				 	 	 	   meterPara(tmpMpLink);
      	  				 	 	 	 }
      	  				 	 	 	 break;
      	  				 	 	 	 
      	  				 	 	 case 5: //配置参数
      	    		 	 	     //ly,2011-03-31,取消密码输入后进入修改配置参数
      	    		 	 	     //pwLight = 0;
      	    		 	 	     //strcpy(passWord,"000000");
      	    		 	 	     //inputPassWord(pwLight);
                   
                         strcpy(commParaItem[0],digitalToChar(addrField.a1[1]>>4));
                         strcat(commParaItem[0],digitalToChar(addrField.a1[1]&0xf));
                         strcat(commParaItem[0],digitalToChar(addrField.a1[0]>>4));
                         strcat(commParaItem[0],digitalToChar(addrField.a1[0]&0xf));
                         
         	 	 	  	 	 	 	 #ifdef TE_ADDR_USE_BCD_CODE
         	 	 	  	 	 	 	  tmpChar = digitalToChar(addrField.a2[1]>>4);
                          tmpTeAddr[0] = *tmpChar;
         	 	 	  	 	 	 	  tmpChar = digitalToChar(addrField.a2[1]&0xf);
                          tmpTeAddr[1] = *tmpChar;
         	 	 	  	 	 	 	  tmpChar = digitalToChar(addrField.a2[0]>>4);
                          tmpTeAddr[2] = *tmpChar;
         	 	 	  	 	 	 	  tmpChar = digitalToChar(addrField.a2[0]&0xf);
                          tmpTeAddr[3] = *tmpChar;          	  	 	 	  	 	 	 	 
                          tmpTeAddr[4] = 0x0;
         	 	 	  	 	 	 	 #else
         	 	 	  	 	 	 	  tmpAddr = addrField.a2[1]<<8 | addrField.a2[0];
         	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr/10000);
         	 	 	  	 	 	 	  tmpTeAddr[0] = *tmpChar;
         	 	 	  	 	 	 	  tmpAddr %= 10000;
         	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr/1000);
         	 	 	  	 	 	 	  tmpTeAddr[1] = *tmpChar;
         	 	 	  	 	 	 	  tmpAddr %= 1000;
         	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr/100);
         	 	 	  	 	 	 	  tmpTeAddr[2] = *tmpChar;
         	 	 	  	 	 	 	  tmpAddr %= 100;
         	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr/10);
         	 	 	  	 	 	 	  tmpTeAddr[3] = *tmpChar;
         	 	 	  	 	 	 	  tmpAddr %= 10;
         	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr);
         	 	 	  	 	 	 	  tmpTeAddr[4] = *tmpChar;
         	 	 	  	 	 	 	  tmpTeAddr[5] = 0x0;
         	 	 	  	 	 	 	 #endif   	 	 	  	 	 	 	 
                         strcpy(commParaItem[1], tmpTeAddr);
          	  	 	 	  	 
                         for(i=0;i<4;i++)
                         {
                         	 if (strlen((char *)teApn[i])==0)
                         	 {
                         	   strcpy((char *)teApn[i],(char *)ipAndPort.apn);
                         	 }
                         }
                         strcpy(commParaItem[2],(char *)ipAndPort.apn);
                         
                         intToString(ipAndPort.port[1]<<8 | ipAndPort.port[0],3,strX);
                      	 strcpy(commParaItem[3],intToIpadd(ipAndPort.ipAddr[0]<<24 | ipAndPort.ipAddr[1]<<16 | ipAndPort.ipAddr[2]<<8 | ipAndPort.ipAddr[3],str));
                         strcat(commParaItem[3],":");
                         
                         for(i=0;i<5-strlen(strX);i++)
                         {
                         	  strcat(commParaItem[3],"0");
                         }
                         strcat(commParaItem[3],strX);
                         intToString(ipAndPort.portBak[1]<<8 | ipAndPort.portBak[0],3,strX);
                      	  strcpy(commParaItem[4],intToIpadd(ipAndPort.ipAddrBak[0]<<24 | ipAndPort.ipAddrBak[1]<<16 | ipAndPort.ipAddrBak[2]<<8 | ipAndPort.ipAddrBak[3],str));
                         strcat(commParaItem[4],":");
                         
                         for(i=0;i<5-strlen(strX);i++)
                         {
                         	 strcat(commParaItem[4],"0");
                         }
                         strcat(commParaItem[4],strX);
                         
                         keyLeftRight = 0;
                         rowOfLight = 0;
            	    			 configPara(rowOfLight, keyLeftRight);
      	    			 
      	  				 	 	 	 break;
      	  				 	 	 	 
      	    	 	 	    case 6:   //VPN用户名密码
                 	 	 	  if (inputStatus == STATUS_SELECT_CHAR)
                 	 	 	  {
                 	 	 	  	inputStatus = STATUS_NONE;
                 	 	 	  	if (keyLeftRight<32)
                 	 	 	  	{
                 	 	 	  	  tmpVpnUserName[keyLeftRight] = character[selectIndex];
                 	 	 	  	}
                 	 	 	  	else
                 	 	 	  	{
                 	 	 	  	  tmpVpnPw[keyLeftRight-32] = character[selectIndex];
                 	 	 	  	}
                 	 	 	  	setVpn(keyLeftRight);
                 	 	 	  }
                 	 	 	  else
                 	 	 	  {
                          tmpVpnUserName[32] = '\0';
                          trim(tmpVpnUserName);
                          tmpVpnPw[32] = '\0';
                          trim(tmpVpnPw);
                          //strcpy((char *)vpn.vpnName, tmpVpnUserName);
                          //memcpy(vpn.vpnName, tmpVpnUserName, 16);
                          //ly,2012-03-12,更改在兰州发现错误
                          memcpy(vpn.vpnName, tmpVpnUserName, 32);
                          //strcpy((char *)vpn.vpnPassword, tmpVpnPw);
                          //memcpy(vpn.vpnPassword, tmpVpnPw, 16);
                          memcpy(vpn.vpnPassword, tmpVpnPw, 32);

        	  	 	 	      	//保存vpn用户名密码
        	  	 	 	      	saveParameter(0x04, 16,(INT8U *)&vpn, sizeof(VPN));

                        	saveBakKeyPara(16);    //2012-8-9,add

                          guiLine(10,55,150,105,0);
                          guiLine(10,55,10,105,1);
                          guiLine(150,55,150,105,1);
                          guiLine(10,55,150,55,1);
                          guiLine(10,105,150,105,1);
                          guiDisplay(20, 70, "专网参数已修改!",1);
                          lcdRefresh(10,120);

                          menuInLayer--;
                 	 	 	  }
                 	 	 	  break;
                 	 	 	
                 	 	 	case 7:  //表计配置
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==7)
      	    		 	 	    {
        	  	 	 	      	//测量点号
        	  	 	 	      	meterConfig.measurePoint = (chrMp[0][0]-0x30)*1000+(chrMp[0][1]-0x30)*100+(chrMp[0][2]-0x30)*10+(chrMp[0][3]-0x30);
        	  	 	 	      	
        	  	 	 	      	//序号
        	  	 	 	      	meterConfig.number = meterConfig.measurePoint;

        	  	 	 	      	//端口和速率
        	  	 	 	      	meterConfig.rateAndPort = (chrMp[1][0]-0x30)<<5;
        	  	 	 	      	switch(chrMp[2][0])
        	  	 	 	      	{
        	  	 	 	      		case 0x30:   //交采端口为1
        	  	 	 	      		 	meterConfig.rateAndPort |= 0x1;
        	  	 	 	      		 	break;

        	  	 	 	      		case 0x31:   //RS485-1端口为2
        	  	 	 	      		 	meterConfig.rateAndPort |= 0x2;
        	  	 	 	      		 	break;

        	  	 	 	      		case 0x32:   //RS485-2端口为3
        	  	 	 	      		 	meterConfig.rateAndPort |= 0x3;
        	  	 	 	      		 	break;
        	  	 	 	      	}
        	  	 	 	      	
        	  	 	 	      	//协议
        	  	 	 	      	switch(chrMp[3][0])
        	  	 	 	        {
        	  	 	 	        	 case 0x32:
        	  	 	 	        	 	 meterConfig.protocol = 02;
        	  	 	 	        	 	 break;

        	  	 	 	        	 case 0x31:
        	  	 	 	        	 	 meterConfig.protocol = 30;
        	  	 	 	        	 	 break;
        	  	 	 	        	 	 
                             case 0x33:
                             	 meterConfig.protocol = ABB_METER;
                             	 break;

                             case 0x34:
                             	 meterConfig.protocol = SIMENS_ZD_METER;
                             	 break;

                             case 0x35:
                             	 meterConfig.protocol = EDMI_METER;
                             	 break;
														 
                             case 0x36:
                             	 meterConfig.protocol = MODBUS_MW_F;
                             	 break;
														 
                             case 0x37:
                             	 meterConfig.protocol = MODBUS_MW_UI;
                             	 break;
                             	 
        	  	 	 	        	 default:
        	  	 	 	        	 	 meterConfig.protocol = 1;
        	  	 	 	        	 	 break;
        	  	 	 	        }
        	  	 	 	        
     	  	 	 	      	  	//电表地址
     	  	 	 	      	  	meterConfig.addr[5] = (chrMp[4][0]-0x30)<<4 | (chrMp[4][1]-0x30);
     	  	 	 	      	  	meterConfig.addr[4] = (chrMp[4][2]-0x30)<<4 | (chrMp[4][3]-0x30);
     	  	 	 	      	  	meterConfig.addr[3] = (chrMp[4][4]-0x30)<<4 | (chrMp[4][5]-0x30);
     	  	 	 	      	  	meterConfig.addr[2] = (chrMp[4][6]-0x30)<<4 | (chrMp[4][7]-0x30);
     	  	 	 	      	  	meterConfig.addr[1] = (chrMp[4][8]-0x30)<<4 | (chrMp[4][9]-0x30);
     	  	 	 	      	  	meterConfig.addr[0] = (chrMp[4][10]-0x30)<<4 | (chrMp[4][11]-0x30);
     	  	 	 	      	  	
     	  	 	 	      	  	//采集器地址
     	  	 	 	      	  	for(i=0;i<6;i++)
     	  	 	 	      	    {
     	  	 	 	      	  	  meterConfig.collectorAddr[i] = 0x0;
     	  	 	 	      	  	}
     	  	 	 	      	  	
     	  	 	 	      	  	//整数位及小数位个数
     	  	 	 	      	  	meterConfig.mixed = 0x05;
     	  	 	 	      	  	
     	  	 	 	      	  	//费率个数
     	  	 	 	      	  	meterConfig.numOfTariff = 4;
     	  	 	 	      	  	
     	  	 	 	      	  	//大类号及小类号
     	  	 	 	      	  	meterConfig.bigAndLittleType = 0x0;
     	  	 	 	      	  	tmpData = (chrMp[5][0]-0x30)*10+chrMp[5][1]-0x30;
     	  	 	 	      	  	if (tmpData<=15)
     	  	 	 	      	  	{
     	  	 	 	      	  		 meterConfig.bigAndLittleType |= tmpData<<4;
     	  	 	 	      	  	}
     	  	 	 	      	  	tmpData = (chrMp[6][0]-0x30)*10+chrMp[6][1]-0x30;
     	  	 	 	      	  	if (tmpData<=15)
     	  	 	 	      	  	{
     	  	 	 	      	  		 meterConfig.bigAndLittleType |= tmpData;
     	  	 	 	      	  	}
                              
                          //保存
  		                    saveDataF10(meterConfig.measurePoint, meterConfig.rateAndPort&0x1f, meterConfig.addr, meterConfig.number, (INT8U *)&meterConfig, 27);
                              
                          guiLine(10,55,150,105,0);
                          guiLine(10,55,10,105,1);
                          guiLine(150,55,150,105,1);
                          guiLine(10,55,150,55,1);
                          guiLine(10,105,150,105,1);
                          guiDisplay(27,70,"表计设置成功!",1);
                          lcdRefresh(10,120);
                              
                          menuInLayer--;

                          return;
      	    		 	 	    }
      	    		 	 	       
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
      	    		 	 	    {
      	    		 	 	    	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	    }
      	    		 	 	    else
      	    		 	 	    {
      	    		 	 	      layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    //切换测量点时读出已有的测量点信息
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
      	    		 	 	    {
      	    		 	 	    	 tmpData = (chrMp[0][0]-0x30)*1000 + (chrMp[0][1]-0x30)*100 + (chrMp[0][2]-0x30)*10 + (chrMp[0][3]-0x30);
      	    		 	 	    	 if (tmpData>2040 || tmpData<1)
      	    		 	 	    	 {
                              guiLine(10,55,150,105,0);
                              guiLine(10,55,10,105,1);
                              guiLine(150,55,150,105,1);
                              guiLine(10,55,150,55,1);
                              guiLine(10,105,150,105,1);
                              guiDisplay(20,70,"测量点输入错误!",1);
                              lcdRefresh(10,120);
      	    		 	 	    	 	  
      	    		 	 	    	 	  layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
      	    		 	 	    	 	  return;
      	    		 	 	    	 }
                           
                           if (selectF10Data(tmpData, 0, 0, (INT8U *)&meterConfig, sizeof(METER_DEVICE_CONFIG))==TRUE)
                           {
                           	  //速率
                           	  chrMp[1][0] = 0x30+(meterConfig.rateAndPort>>5);
                           	  
                           	  //端口
                           	  switch(meterConfig.rateAndPort&0x1f)
                           	  {
                           	  	 case 1:
                           	  	 	 chrMp[2][0] = 0x30;
                           	  	 	 break;
                           	  	 	 
                           	  	 case 2:
                           	  	 	 chrMp[2][0] = 0x31;
                           	  	 	 break;

                           	  	 case 3:
                           	  	 	 chrMp[2][0] = 0x32;
                           	  	 	 break;
                           	  }

                              //char       chrCopyProtocol[3][13]={"DL/T645-1997","DL/T645-2007","交流采样","ABB方表","兰吉尔ZD表","红相EDMI表"};
                           	  switch(meterConfig.protocol)
                           	  {
                           	  	 case 2:
                           	  	 	 chrMp[3][0] = 0x32;
                           	  	 	 break;
                                 	 
                           	  	 case 30:
                           	  	 	 chrMp[3][0] = 0x31;
                           	  	 	 break;

                                 case ABB_METER:
                           	  	 	 chrMp[3][0] = 0x33;
                                 	 break;

                                 case SIMENS_ZD_METER:
                           	  	 	 chrMp[3][0] = 0x34;
                                 	 break;

                                 case EDMI_METER:
                           	  	 	 chrMp[3][0] = 0x35;
                                 	 break;
																 
                                 case MODBUS_MW_F:
                           	  	 	 chrMp[3][0] = 0x36;
                                 	 break;
                           	  	 	 
																 case MODBUS_MW_UI:
																	 chrMp[3][0] = 0x37;
																	 break;
																 
                           	  	 default:
                           	  	 	 chrMp[3][0] = 0x30;
                           	  	 	 break;
                           	  }
                           	  
                           	  //表地址
                  	 	 	   	  chrMp[4][0]  = (meterConfig.addr[5]>>4)+0x30;
                  	 	 	   	  chrMp[4][1]  = (meterConfig.addr[5]&0xf)+0x30;
                  	 	 	   	  chrMp[4][2]  = (meterConfig.addr[4]>>4)+0x30;
                  	 	 	   	  chrMp[4][3]  = (meterConfig.addr[4]&0xf)+0x30;
                  	 	 	   	  chrMp[4][4]  = (meterConfig.addr[3]>>4)+0x30;
                  	 	 	   	  chrMp[4][5]  = (meterConfig.addr[3]&0xf)+0x30;
                  	 	 	   	  chrMp[4][6]  = (meterConfig.addr[2]>>4)+0x30;
                  	 	 	   	  chrMp[4][7]  = (meterConfig.addr[2]&0xf)+0x30;
                  	 	 	   	  chrMp[4][8]  = (meterConfig.addr[1]>>4)+0x30;
                  	 	 	   	  chrMp[4][9]  = (meterConfig.addr[1]&0xf)+0x30;
                  	 	 	   	  chrMp[4][10] = (meterConfig.addr[0]>>4)+0x30;
                  	 	 	   	  chrMp[4][11] = (meterConfig.addr[0]&0xf)+0x30;
                  	 	 	   	  chrMp[4][12] = '\0';
                  	 	 	   	  
                  	 	 	   	  //大类号
                  	 	 	   	  chrMp[5][0] = 0x30+((meterConfig.bigAndLittleType>>4)/10);
                  	 	 	   	  chrMp[5][1] = 0x30+((meterConfig.bigAndLittleType>>4)%10);

                  	 	 	   	  //小类号
                  	 	 	   	  chrMp[6][0] = 0x30+((meterConfig.bigAndLittleType&0xf)/10);
                  	 	 	   	  chrMp[6][1] = 0x30+((meterConfig.bigAndLittleType&0xf)%10);                  	 	 	   	  
                           }
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    keyLeftRight = 0;
      	    		 	 	    setZbMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		break;

      	    	 	 	  	case 10:   //设置终端级联
      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==5)
      	    		 	 	    {
        	  	 	 	      	//级联端口
        	  	 	 	      	if (chrMp[0][0]!=0x33)
        	  	 	 	      	{
                             guiLine(10,55,150,105,0);
                             guiLine(10,55,10,105,1);
                             guiLine(150,55,150,105,1);
                             guiLine(10,55,150,55,1);
                             guiLine(10,105,150,105,1);
                             guiDisplay(20,60,"输入错误!级联端口只",1);
                             guiDisplay(20,80,"能为3",1);
                             lcdRefresh(10,120);
      	    		 	 	    	 	 
      	    		 	 	    	 	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]=0;
                             return;
        	  	 	 	      	}
        	  	 	 	      	
        	  	 	 	      	cascadeCommPara.commPort = 0x03; //端口3
        	  	 	 	      	cascadeCommPara.ctrlWord = 0x8B;             //默认4800,8-e-1,目前最高只能通这个速率
        	  	 	 	      	cascadeCommPara.receiveMsgTimeout = 0x05;
        	  	 	 	      	cascadeCommPara.receiveByteTimeout = 0x05;
        	  	 	 	      	cascadeCommPara.cascadeMretryTime = 0x01;
        	  	 	 	      	cascadeCommPara.groundSurveyPeriod = 0x05;   //巡测周期
        	  	 	 	      	
        	  	 	 	      	cascadeCommPara.flagAndTeNumber = 0x00;
        	  	 	 	      	if (chrMp[1][0]==0x30)
        	  	 	 	      	{
        	  	 	 	      	  cascadeCommPara.divisionCode[0] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[1] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[2] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[3] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[4] = 0x00;
        	  	 	 	      	  cascadeCommPara.divisionCode[5] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[0] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[1] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[2] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[3] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[4] = 0x00;
        	  	 	 	      	  cascadeCommPara.cascadeTeAddr[5] = 0x00;
        	  	 	 	      	}
        	  	 	 	      	else
        	  	 	 	      	{
   	  	 	 	      	  	    //终端地址1
   	  	 	 	      	  	    tmpAddr = (chrMp[2][4]-0x30)*10000+(chrMp[2][5]-0x30)*1000
   	  	 	 	      	  	            +(chrMp[2][6]-0x30)*100+(chrMp[2][7]-0x30)*10
   	  	 	 	      	  	            +(chrMp[2][8]-0x30);
        	  	 	 	      	  tmpData = (chrMp[2][0]-0x30)<<12 | (chrMp[2][1]-0x30)<<8 | (chrMp[2][2]-0x30)<<4 | (chrMp[2][3]-0x30);
        	  	 	 	      	  if (tmpData!=0x0 && tmpAddr!=0x00)
        	  	 	 	      	  {
        	  	 	 	      	  	cascadeCommPara.flagAndTeNumber++;

   	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[1] = tmpAddr>>8;
   	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[0] = tmpAddr&0xff;
   	  	 	 	      	  	      cascadeCommPara.divisionCode[1] = (chrMp[2][0]-0x30)<<4 | (chrMp[2][1]-0x30);
   	  	 	 	      	  	      cascadeCommPara.divisionCode[0] = (chrMp[2][2]-0x30)<<4 | (chrMp[2][3]-0x30);
        	  	 	 	      	  }

        	  	 	 	      		if (chrMp[1][0]==0x32)
        	  	 	 	      		{
        	  	 	 	      			cascadeCommPara.flagAndTeNumber |= 0x80;
        	  	 	 	      		}
        	  	 	 	      		else
        	  	 	 	      		{
     	  	 	 	      	  	    //终端地址2
     	  	 	 	      	  	    tmpAddr = (chrMp[3][4]-0x30)*10000+(chrMp[3][5]-0x30)*1000
     	  	 	 	      	  	            +(chrMp[3][6]-0x30)*100+(chrMp[3][7]-0x30)*10
     	  	 	 	      	  	            +(chrMp[3][8]-0x30);
          	  	 	 	      	  tmpData = (chrMp[3][0]-0x30)<<12 | (chrMp[3][1]-0x30)<<8 | (chrMp[3][2]-0x30)<<4 | (chrMp[3][3]-0x30);
          	  	 	 	      	  if (tmpData!=0x0 && tmpAddr!=0x00)
          	  	 	 	      	  {
          	  	 	 	      	  	cascadeCommPara.flagAndTeNumber++;
  
     	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[3] = tmpAddr>>8;
     	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[2] = tmpAddr&0xff;
     	  	 	 	      	  	      cascadeCommPara.divisionCode[3] = (chrMp[3][0]-0x30)<<4 | (chrMp[3][1]-0x30);
     	  	 	 	      	  	      cascadeCommPara.divisionCode[2] = (chrMp[3][2]-0x30)<<4 | (chrMp[3][3]-0x30);
          	  	 	 	      	  }

     	  	 	 	      	  	    //终端地址3
     	  	 	 	      	  	    tmpAddr = (chrMp[4][4]-0x30)*10000+(chrMp[4][5]-0x30)*1000
     	  	 	 	      	  	            +(chrMp[4][6]-0x30)*100+(chrMp[4][7]-0x30)*10
     	  	 	 	      	  	            +(chrMp[4][8]-0x30);
          	  	 	 	      	  tmpData = (chrMp[4][0]-0x30)<<12 | (chrMp[4][1]-0x30)<<8 | (chrMp[4][2]-0x30)<<4 | (chrMp[4][3]-0x30);
          	  	 	 	      	  if (tmpData!=0x0 && tmpAddr!=0x00)
          	  	 	 	      	  {
          	  	 	 	      	  	cascadeCommPara.flagAndTeNumber++;
  
     	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[5] = tmpAddr>>8;
     	  	 	 	      	  	      cascadeCommPara.cascadeTeAddr[4] = tmpAddr&0xff;
     	  	 	 	      	  	      cascadeCommPara.divisionCode[5] = (chrMp[4][0]-0x30)<<4 | (chrMp[4][1]-0x30);
     	  	 	 	      	  	      cascadeCommPara.divisionCode[4] = (chrMp[4][2]-0x30)<<4 | (chrMp[4][3]-0x30);
          	  	 	 	      	  }
        	  	 	 	      		}
        	  	 	 	      	}
        	  	 	 	      	
                          saveParameter(0x04, 37, (INT8U *)&cascadeCommPara, sizeof(CASCADE_COMM_PARA));

                          guiLine(10,55,150,105,0);
                          guiLine(10,55,10,105,1);
                          guiLine(150,55,150,105,1);
                          guiLine(10,55,150,55,1);
                          guiLine(10,105,150,105,1);
                          guiDisplay(12,70,"级联参数设置成功!",1);
                          lcdRefresh(10,120);
                              
                          menuInLayer--;
                          return;        	  	 	 	      	
        	  	 	 	      }

      	    		 	 	    if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
      	    		 	 	    {
      	    		 	 	    	layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
      	    		 	 	    }
      	    		 	 	    else
      	    		 	 	    {
      	    		 	 	      layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
      	    		 	 	    }
      	    		 	 	    
      	    		 	 	    keyLeftRight = 0;
      	    		 	 	    setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    	 	 	  		break;

     	    	 	 	  	  case 12:    //维护接口模式设置
       	    	 	 	  	  switch (keyLeftRight)
       	    	 	 	  	  {
       	    	 	 	  	    case 0x01:
       	    	 	 	  	  	  mainTainPortMode = 0x55;
       	    	 	 	  	  	  break;
       	    	 	 	  	  
       	    	 	 	  	    default:
       	    	 	 	  	  	  mainTainPortMode = 0;
       	    	 	 	  	  	  break;
       	    	 	 	  	  }
       	    	 	 	  	  
                        guiLine(10,55,150,105,0);
                        guiLine(10,55,10,105,1);
                        guiLine(150,55,150,105,1);
                        guiLine(10,55,150,55,1);
                        guiLine(10,105,150,105,1);
                        guiDisplay(9,70,"维护口模式设置成功",1);
                        lcdRefresh(10,120);
                            
                        menuInLayer--;
                        return;
     	    	 	 	  	    break;

     	    	 	 	  	  case 13:    //第2路485口功能设置
       	    	 	 	  	  switch (keyLeftRight)
       	    	 	 	  	  {
       	    	 	 	  	    case 0x01:
       	    	 	 	  	  	  rs485Port2Fun = 0x55;
       	    	 	 	  	  	 
       	    	 	 	  	  	  //设置第2路485的速率为9600-8-e-1
                            #ifdef WDOG_USE_X_MEGA
                             str[0] = 0x02;    //xMega端口2
                             str[1] = 0xcb;    //端口速率,9600-8-e-1
                             sendXmegaFrame(COPY_PORT_RATE_SET,(INT8U *)str, 2);
                             
                             printf("设置第2路速率为9600-8-e-1\n");
                            #endif

       	    	 	 	  	  	  break;
       	    	 	 	  	  
       	    	 	 	  	    default:
       	    	 	 	  	  	  rs485Port2Fun = 0;
       	    	 	 	  	  	  break;
       	    	 	 	  	  }
       	    	 	 	  	  
                        guiLine(10,55,150,105,0);
                        guiLine(10,55,10,105,1);
                        guiLine(150,55,150,105,1);
                        guiLine(10,55,150,55,1);
                        guiLine(10,105,150,105,1);
                        guiDisplay(9,70,"485口2功能设置成功",1);
                        lcdRefresh(10,120);
                            
                        menuInLayer--;
                        return;
     	    	 	 	  	    break;
      	    	 	 	  		
      	    	 	 	    case 14:  //以太网参数设置
    	    		 	 	      if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
    	    		 	 	      {
        	  	 	 	      	//以太网IP地址
        	  	 	 	      	teIpAndPort.teIpAddr[0] =(chrEthPara[0][0]-0x30)*100+(chrEthPara[0][1]-0x30)*10+(chrEthPara[0][2]-0x30);
        	  	 	 	      	teIpAndPort.teIpAddr[1] =(chrEthPara[0][4]-0x30)*100+(chrEthPara[0][5]-0x30)*10+(chrEthPara[0][6]-0x30);
        	  	 	 	      	teIpAndPort.teIpAddr[2] =(chrEthPara[0][8]-0x30)*100+(chrEthPara[0][9]-0x30)*10+(chrEthPara[0][10]-0x30);
        	  	 	 	      	teIpAndPort.teIpAddr[3] =(chrEthPara[0][12]-0x30)*100+(chrEthPara[0][13]-0x30)*10+(chrEthPara[0][14]-0x30);
   
        	  	 	 	      	//以太网IP掩码
        	  	 	 	      	teIpAndPort.mask[0] =(chrEthPara[1][0]-0x30)*100+(chrEthPara[1][1]-0x30)*10+(chrEthPara[1][2]-0x30);
        	  	 	 	      	teIpAndPort.mask[1] =(chrEthPara[1][4]-0x30)*100+(chrEthPara[1][5]-0x30)*10+(chrEthPara[1][6]-0x30);
        	  	 	 	      	teIpAndPort.mask[2] =(chrEthPara[1][8]-0x30)*100+(chrEthPara[1][9]-0x30)*10+(chrEthPara[1][10]-0x30);
        	  	 	 	      	teIpAndPort.mask[3] =(chrEthPara[1][12]-0x30)*100+(chrEthPara[1][13]-0x30)*10+(chrEthPara[1][14]-0x30);
   
        	  	 	 	      	//以太网网关
        	  	 	 	      	teIpAndPort.gateWay[0] =(chrEthPara[2][0]-0x30)*100+(chrEthPara[2][1]-0x30)*10+(chrEthPara[2][2]-0x30);
        	  	 	 	      	teIpAndPort.gateWay[1] =(chrEthPara[2][4]-0x30)*100+(chrEthPara[2][5]-0x30)*10+(chrEthPara[2][6]-0x30);
        	  	 	 	      	teIpAndPort.gateWay[2] =(chrEthPara[2][8]-0x30)*100+(chrEthPara[2][9]-0x30)*10+(chrEthPara[2][10]-0x30);
        	  	 	 	      	teIpAndPort.gateWay[3] =(chrEthPara[2][12]-0x30)*100+(chrEthPara[2][13]-0x30)*10+(chrEthPara[2][14]-0x30);
        	  	 	 	      	
   
        	  	 	 	      	//保存
                          saveIpMaskGateway(teIpAndPort.teIpAddr,teIpAndPort.mask,teIpAndPort.gateWay);  //保存到rcS中,ly,2011-04-12
    
                          saveParameter(0x04, 7, (INT8U *)&teIpAndPort, sizeof(TE_IP_AND_PORT));
                              
                          guiLine(6,55,154,105,0);
                          guiLine(6,55,6,105,1);
                          guiLine(154,55,154,105,1);
                          guiLine(6,55,154,55,1);
                          guiLine(6,105,154,105,1);
                          guiDisplay(8,60,"修改以太网参数成功",1);
                          guiDisplay(16,80,"设置生效需要重启",1);
                          lcdRefresh(10,120);
                              
                          menuInLayer--;
    	    		 	 	      }
    	    		 	 	      else
    	    		 	 	      {
    	    		 	 	         if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] >= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
    	    		 	 	         {
    	    		 	 	      	   layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
    	    		 	 	         }
    	    		 	 	         else
    	    		 	 	         {
    	    		 	 	       	   layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
    	    		 	 	         }
    	    		 	 	         keyLeftRight = 0;
    	    		 	 	         setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
    	    		 	 	      }
      	    	 	 	    	break;
      	  				 	 }
      	  				 	 break;
      	  			}
      	  			break;
      	  	  
      	  	  case 4:  //菜单第4层
    		 	 	    if (rowOfLight == 5)
    		 	 	    {
 	  	 	 	      	//主IP地址
 	  	 	 	      	ipAndPort.ipAddr[0] =(commParaItem[3][0]-0x30)*100+(commParaItem[3][1]-0x30)*10+(commParaItem[3][2]-0x30);
 	  	 	 	      	ipAndPort.ipAddr[1] =(commParaItem[3][4]-0x30)*100+(commParaItem[3][5]-0x30)*10+(commParaItem[3][6]-0x30);
 	  	 	 	      	ipAndPort.ipAddr[2] =(commParaItem[3][8]-0x30)*100+(commParaItem[3][9]-0x30)*10+(commParaItem[3][10]-0x30);
 	  	 	 	      	ipAndPort.ipAddr[3] =(commParaItem[3][12]-0x30)*100+(commParaItem[3][13]-0x30)*10+(commParaItem[3][14]-0x30);
 	  	 	 	      	  	
 	  	 	 	      	//主端口
 	  	 	 	      	tmpData = (commParaItem[3][16]-0x30)*10000+(commParaItem[3][17]-0x30)*1000
 	  	 	 	      	         +(commParaItem[3][18]-0x30)*100+(commParaItem[3][19]-0x30)*10
 	  	 	 	      	         +(commParaItem[3][20]-0x30);
 	  	 	 	      	ipAndPort.port[1] = tmpData>>8;
 	  	 	 	      	ipAndPort.port[0] = tmpData&0xff;

 	  	 	 	      	//备用地址
 	  	 	 	      	ipAndPort.ipAddrBak[0] =(commParaItem[4][0]-0x30)*100+(commParaItem[4][1]-0x30)*10+(commParaItem[4][2]-0x30);
 	  	 	 	      	ipAndPort.ipAddrBak[1] =(commParaItem[4][4]-0x30)*100+(commParaItem[4][5]-0x30)*10+(commParaItem[4][6]-0x30);
 	  	 	 	      	ipAndPort.ipAddrBak[2] =(commParaItem[4][8]-0x30)*100+(commParaItem[4][9]-0x30)*10+(commParaItem[4][10]-0x30);
 	  	 	 	      	ipAndPort.ipAddrBak[3] =(commParaItem[4][12]-0x30)*100+(commParaItem[4][13]-0x30)*10+(commParaItem[4][14]-0x30);
 	  	 	 	      	  	
 	  	 	 	      	//备用端口
 	  	 	 	      	tmpData = (commParaItem[4][16]-0x30)*10000+(commParaItem[4][17]-0x30)*1000
 	  	 	 	      	         +(commParaItem[4][18]-0x30)*100+(commParaItem[4][19]-0x30)*10
 	  	 	 	      	         +(commParaItem[4][20]-0x30);
 	  	 	 	      	ipAndPort.portBak[1] = tmpData>>8;
 	  	 	 	      	ipAndPort.portBak[0] = tmpData&0xff;
 	  	 	 	      	  	        	  	 	 	      	  	
 	  	 	 	      	strcpy((char *)ipAndPort.apn, commParaItem[2]);

 	  	 	 	      	//保存IP地址
 	  	 	 	      	saveParameter(0x04, 3,(INT8U *)&ipAndPort,sizeof(IP_AND_PORT));
                  saveBakKeyPara(3);    //2012-8-9,add

          	  	 	addrField.a1[1] = (commParaItem[0][0]-0x30)<<4 | (commParaItem[0][1]-0x30);
          	  	 	addrField.a1[0] = (commParaItem[0][2]-0x30)<<4 | (commParaItem[0][3]-0x30);
 	 	 	      	    
 	 	 	      	    #ifdef TE_ADDR_USE_BCD_CODE
 	 	 	      	  	 addrField.a2[1] = (commParaItem[1][0]-0x30)<<4 | (commParaItem[1][1]-0x30);
 	 	 	      	  	 addrField.a2[0] = (commParaItem[1][2]-0x30)<<4 | (commParaItem[1][3]-0x30);
 	 	 	      	    #else
 	 	 	      	  	 tmpAddr = (commParaItem[1][0]-0x30)*10000+(commParaItem[1][1]-0x30)*1000
 	 	 	      	  	         +(commParaItem[1][2]-0x30)*100+(commParaItem[1][3]-0x30)*10
 	 	 	      	  	         +(commParaItem[1][4]-0x30);
 	 	 	      	  	 addrField.a2[1] = tmpAddr>>8;
 	 	 	      	  	 addrField.a2[0] = tmpAddr&0xff;
 	 	 	      	    #endif

                  //保存行政区划码
                  saveParameter(0x04, 121,(INT8U *)&addrField,4);
                  
                  saveBakKeyPara(121);    //2012-8-9,add

                  guiLine(10,55,150,105,0);
                  guiLine(10,55,10,105,1);
                  guiLine(150,55,150,105,1);
                  guiLine(10,55,150,55,1);
                  guiLine(10,105,150,105,1);
                  guiDisplay(12,70,"修改通信参数成功!",1);
                  lcdRefresh(10,120);
                       
                  menuInLayer--;
                  
                  //2012-10-22,add,修改IP后立即用新IP登录
                  if (bakModuleType==MODEM_PPP)
                  {
                    resetPppWlStatus();
                  }
                  else
                  {
                  	wlModemPowerOnOff(0);
                  }
                  return;
    		 	 	    }
      	    		 	 	    
 		 	 	        if (rowOfLight >= 5)
 		 	 	        {
 		 	 	       	  rowOfLight = 0;
 		 	 	        }
 		 	 	        else
 		 	 	        {
 		 	 	       	  rowOfLight++;
 		 	 	        }
 		 	 	        keyLeftRight = 0;
 		 	 	        configPara(rowOfLight, keyLeftRight);
      	    		break;
      	    		
             case 5:    //菜单第5层
             	 if (layer1MenuLight==1 && layer2MenuLight[1]==5)
             	 {
           	 	 	  if (inputStatus == STATUS_NONE)
           	 	 	  {
                    keyLeftRight = 0;
             	 	 	  
             	 	 	  for(i=15;i>0;i--)
             	 	 	  {
             	 	 	  	if (commParaItem[2][i]!=' ' && i<15)
             	 	 	  	{
             	 	 	  		 commParaItem[2][i+1] = '\0';
             	 	 	  		 break;
             	 	 	  	}
             	 	 	  }

                    rowOfLight++;
    	    		 	 	 	configPara(rowOfLight, keyLeftRight);
           	 	 	  }
           	 	 	  if (inputStatus == STATUS_SELECT_CHAR)
           	 	 	  {
           	 	 	  	inputStatus = STATUS_NONE;
           	 	 	  	commParaItem[2][inputIndex] = character[selectIndex];
           	 	 	  	inputApn(inputIndex);
           	 	 	  }
             	  }
             	 	break;
      	  			
      	    	case 20:  //菜单第20层(输入密码)的确认
      	    		//ly,2011-03-31,取消密码输入后进入修改配置参数
      	    		/*
      	    		for(i=0;i<6;i++)
      	    		{
      	    		 	 if (originPassword[i]!=passWord[i])
      	    		 	 {
      	    		 	  	guiDisplay(30,120,"密码输入错误!",1);
      	    		 	  	lcdRefresh(120,137);
      	    		 	  	return;
      	    		 	 }
      	    		}
      	    		 
      	    		//根据输入密码前的高亮菜单项,执行相应的动作
      	    		if (layer1MenuLight==1 || layer2MenuLight[1]==5)
      	    		{
                   strcpy(commParaItem[0],digitalToChar(addrField.a1[1]>>4));
                   strcat(commParaItem[0],digitalToChar(addrField.a1[1]&0xf));
                   strcat(commParaItem[0],digitalToChar(addrField.a1[0]>>4));
                   strcat(commParaItem[0],digitalToChar(addrField.a1[0]&0xf));
                   
   	 	 	  	 	 	 	 #ifdef TE_ADDR_USE_BCD_CODE
   	 	 	  	 	 	 	  tmpChar = digitalToChar(addrField.a2[1]>>4);
                    tmpTeAddr[0] = *tmpChar;
   	 	 	  	 	 	 	  tmpChar = digitalToChar(addrField.a2[1]&0xf);
                    tmpTeAddr[1] = *tmpChar;
   	 	 	  	 	 	 	  tmpChar = digitalToChar(addrField.a2[0]>>4);
                    tmpTeAddr[2] = *tmpChar;
   	 	 	  	 	 	 	  tmpChar = digitalToChar(addrField.a2[0]&0xf);
                    tmpTeAddr[3] = *tmpChar;          	  	 	 	  	 	 	 	 
                    tmpTeAddr[4] = 0x0;
   	 	 	  	 	 	 	 #else
   	 	 	  	 	 	 	  tmpAddr = addrField.a2[1]<<8 | addrField.a2[0];
   	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr/10000);
   	 	 	  	 	 	 	  tmpTeAddr[0] = *tmpChar;
   	 	 	  	 	 	 	  tmpAddr %= 10000;
   	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr/1000);
   	 	 	  	 	 	 	  tmpTeAddr[1] = *tmpChar;
   	 	 	  	 	 	 	  tmpAddr %= 1000;
   	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr/100);
   	 	 	  	 	 	 	  tmpTeAddr[2] = *tmpChar;
   	 	 	  	 	 	 	  tmpAddr %= 100;
   	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr/10);
   	 	 	  	 	 	 	  tmpTeAddr[3] = *tmpChar;
   	 	 	  	 	 	 	  tmpAddr %= 10;
   	 	 	  	 	 	 	  tmpChar = digitalToChar(tmpAddr);
   	 	 	  	 	 	 	  tmpTeAddr[4] = *tmpChar;
   	 	 	  	 	 	 	  tmpTeAddr[5] = 0x0;
   	 	 	  	 	 	 	 #endif   	 	 	  	 	 	 	 
                   strcpy(commParaItem[1], tmpTeAddr);
    	  	 	 	  	 
                   for(i=0;i<4;i++)
                   {
                   	 if (strlen((char *)teApn[i])==0)
                   	 {
                   	   strcpy((char *)teApn[i],(char *)ipAndPort.apn);
                   	 }
                   }
                   strcpy(commParaItem[2],(char *)ipAndPort.apn);
                   
                   intToString(ipAndPort.port[1]<<8 | ipAndPort.port[0],3,strX);
                	 strcpy(commParaItem[3],intToIpadd(ipAndPort.ipAddr[0]<<24 | ipAndPort.ipAddr[1]<<16 | ipAndPort.ipAddr[2]<<8 | ipAndPort.ipAddr[3],str));
                   strcat(commParaItem[3],":");
                   
                   for(i=0;i<5-strlen(strX);i++)
                   {
                   	  strcat(commParaItem[3],"0");
                   }
                   strcat(commParaItem[3],strX);
                   intToString(ipAndPort.portBak[1]<<8 | ipAndPort.portBak[0],3,strX);
                	  strcpy(commParaItem[4],intToIpadd(ipAndPort.ipAddrBak[0]<<24 | ipAndPort.ipAddrBak[1]<<16 | ipAndPort.ipAddrBak[2]<<8 | ipAndPort.ipAddrBak[3],str));
                   strcat(commParaItem[4],":");
                   
                   for(i=0;i<5-strlen(strX);i++)
                   {
                   	 strcat(commParaItem[4],"0");
                   }
                   strcat(commParaItem[4],strX);
                   
                   keyLeftRight = 0;
                   rowOfLight = 0;
      	    			 configPara(rowOfLight, keyLeftRight);
      	    		}
      	    		*/
      	    		break;
      	  	}
      	  	break;
      	  	
      	  case KEY_CANCEL:   //取消
      	  	switch(menuInLayer)
      	  	{
      	  		case 2:  //菜单第2层
      	  			layer1Menu(layer1MenuLight);
      	  		 	break;
      	  		 	
      	  		case 3:  //菜单第3层
             		if ((layer2MenuLight[layer1MenuLight]==4 || layer2MenuLight[layer1MenuLight]==5)&& layer1MenuLight==1)
             		{
             			 freeQueryMpLink();
             		}
             		
             		if ((layer2MenuLight[layer1MenuLight]==4 || layer2MenuLight[layer1MenuLight]==5 || layer2MenuLight[layer1MenuLight]==6 || layer2MenuLight[layer1MenuLight]==7)&& layer1MenuLight==0)
             		{
             			 while (eventLinkHead!=NULL)
             			 {
             			   tmpEventShow = eventLinkHead;
             			   eventLinkHead = eventLinkHead->next;
             			   
             			   free(tmpEventShow);
             			 }             			 
             		}
             		layer2Menu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	  			break;
      	  			
      	  		case 4:   //菜单第4层
      	  			if (layer1MenuLight==1 && layer2MenuLight[1]==5)
      	  			{
             		  layer2Menu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
      	  			}
      	  		  break;

      	  		case 5:   //菜单第5层
      	  			if (layer1MenuLight==1 && layer2MenuLight[1]==5)
      	  			{
             	 	 	 for(i=15;i>0;i--)
             	 	 	 {
             	 	 	  	if (commParaItem[2][i]!=' ' && i<15)
             	 	 	  	{
             	 	 	  		 commParaItem[2][i+1] = 0x0;
             	 	 	  		 break;
             	 	 	  	}
             	 	 	 }
             	 	 	 
      	  				 configPara(rowOfLight, keyLeftRight);
      	  			}
      	  		  break;      	  		
      	  	}
      	  	break;
      	  
      	  case KEY_UP:       //上键
           	switch(menuInLayer)
            {
             	case 1:    //菜单第1层
             	 	if (layer1MenuLight==0)
             	 	{
             	 	 	 layer1MenuLight = MAX_LAYER1_MENU-1;
             	 	}
             	 	else
             	 	{
             	 	 	 layer1MenuLight--;
             	 	}
             	 	layer1Menu(layer1MenuLight);
             	  break;
             	  
             	case 2:    //菜单第2层
             		switch(layer1MenuLight)
             		{
             			case 0:
             			case 1:
             				if (layer2MenuLight[layer1MenuLight]==0)
             				{
             					layer2MenuLight[layer1MenuLight] = layer2MenuNum[layer1MenuLight]-1;
             				}
             				else
             				{
             					layer2MenuLight[layer1MenuLight]--;
             				}
             				layer2Menu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
             				break; 
             		}
             		break;
             		
              case 3:    //菜单第3层
              	switch(layer1MenuLight)
                {
                	case 0:  //实时数据
                	 	switch(layer2MenuLight[layer1MenuLight])
                	 	{
                	 		case 2:  //功率曲线
                	 			if (mpPowerCurveLight==0)
             	 	 	  	  {
             	 	 	  		 	stringUpDown(queryTimeStr, layer3MenuLight[0][2], 0);  //高亮的字符增大一个字符
             	 	 	  		 	powerCurve(mpPowerCurveLight, layer3MenuLight[0][2]);
             	 	 	  		}
             	 	 	  		else
             	 	 	  		{
             	 	 	  		 	keyLeftRight = 0;
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]= layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
                            layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]--;
             	 	 	  		 	}
             	 	 	  		 	powerCurve(mpPowerCurveLight,layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		}
             	 	 	  		break;
             	 	 	  }
             	 	 	  break;
             	 	 	  
             	 	 	case 1:  //参数定值
                 	  switch(layer2MenuLight[layer1MenuLight])
                 	  {
      	    		      case 6: //虚拟专网用户名密码
                	 	 	  if (inputStatus==STATUS_SELECT_CHAR)
                	 	 	  {
                	 	 	  	if (selectIndex>=14)
                	 	 	  	{
                	 	 	  		 selectIndex -= 14;
                	 	 	  	}
                	 	 	  	else
                	 	 	  	{
                	 	 	  	  selectIndex = 68;
                	 	 	  	}
                	 	 	  	selectChar(selectIndex);
                	 	 	  }
                	 	 	  if (inputStatus==STATUS_NONE)
                	 	 	  {
                	 	 	  	selectIndex = 0;
                          selectChar(selectIndex);
                        }
    	    		          break;
    	    		          
        	 	 	  		  case 7: //表计配置
  		                  stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
    	 	 	  		 	      if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
    	 	 	  		 	      {
    	 	 	  		 	     	  if (chrMp[1][0]>0x37)
    	 	 	  		 	     	  {
    	 	 	  		 	     	  	chrMp[1][0] = 0x30;
    	 	 	  		 	     	  }
    	 	 	  		 	      }
    	 	 	  		 	      if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
    	 	 	  		 	      {
    	 	 	  		 	     	  if (chrMp[2][0]>0x32)
    	 	 	  		 	     	  {
    	 	 	  		 	     	 	  chrMp[2][0] = 0x30;
    	 	 	  		 	     	  }
    	 	 	  		 	      }
    	 	 	  		 	      if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
    	 	 	  		 	      {
    	 	 	  		 	     	  if (chrMp[3][0]>0x37)
    	 	 	  		 	     	  {
    	 	 	  		 	     	 	  chrMp[3][0] = 0x30;
    	 	 	  		 	     	  }
    	 	 	  		 	      }
  		                  setZbMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
        	 	 	  		 	  break;
         	 	 	  		  
         	 	 	  		  case 10: //级联参数设置
   		                  if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>1 && layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<5)
   		                  {
   		               	    stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
   		                  }
      		              else
      		              {
      		               	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
      		               	 {
      		               	 	  if (chrMp[0][0]!='3')
      		               	 	  {
      		               	 	  	 chrMp[0][0]='3';
      		               	 	  }
      		               	 }
      		               	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
      		               	 {
      		               	 	 if (chrMp[1][0]>0x31)
      		               	 	 {
      		               	 	  	chrMp[1][0]='0';
      		               	 	 }
      		               	 	 else
      		               	 	 {
      		               	 	  	chrMp[1][0]++;
      		               	 	 }
      		               	 }
      		              }
      		              setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
         	 	 	  		 	  break;

   	    	 	 	  	   case 12:    //维护接口模式设置
     	    	 	 	  	   if (keyLeftRight==0)
     	    	 	 	  	   {
     	    	 	 	  	  	 keyLeftRight = 1;
     	    	 	 	  	   }
     	    	 	 	  	   else
     	    	 	 	  	   {
     	    	 	 	  	  	 keyLeftRight--;
     	    	 	 	  	   }
   	    	 	 	  	  	 setMainTain(keyLeftRight);      	    	 	 	  	  	
   	    	 	 	  	  	 break;

   	    	 	 	  	   case 13:    //第2路485口功能设置
     	    	 	 	  	   if (keyLeftRight==0)
     	    	 	 	  	   {
     	    	 	 	  	  	 keyLeftRight = 1;
     	    	 	 	  	   }
     	    	 	 	  	   else
     	    	 	 	  	   {
     	    	 	 	  	  	 keyLeftRight--;
     	    	 	 	  	   }
   	    	 	 	  	  	 setRs485Port2(keyLeftRight);      	    	 	 	  	  	
   	    	 	 	  	  	 break;

         	 	 	  		  case 14: //以太网参数设置
  	    		            stringUpDown(chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,0);
  	    		            setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
         	 	 	  		 	  break;
    	    		      }
             	 	 		break;
             	 	}
              	break;
              	
              case 4:    //菜单第4层
              	if (layer1MenuLight==1 && layer2MenuLight[1]==5)
              	{
    	 	 	  		 	 if (rowOfLight==2)
    	 	 	  		 	 {
    	 	 	  		 	 	  for(i=strlen(commParaItem[2]);i<16;i++)
    	 	 	  		 	 	  {
    	 	 	  		 	 	    commParaItem[2][i] = ' ';
    	 	 	  		 	 	  }
    	 	 	  		 	 	  commParaItem[2][16] = '\0';
    	 	 	  		 	 	  inputStatus=STATUS_NONE;
    	 	 	  		 	 	  inputIndex = 0;
    	 	 	  		 	 	  inputApn(inputIndex); //APN的输入处理
    	 	 	  		 	 }
    	 	 	  		 	 else
    	 	 	  		 	 {
   		               stringUpDown(commParaItem[rowOfLight],keyLeftRight,0);
   		               configPara(rowOfLight,keyLeftRight);
   		             }
      	    		}
      	    		break;
      	    		
            	case 5:    //菜单第5层
             	 	if (layer1MenuLight==1 && layer2MenuLight[1]==5)
             	 	{
          	 	 	   if (inputStatus==STATUS_SELECT_CHAR)
          	 	 	   {
          	 	 	  		if (selectIndex>=14)
          	 	 	  		{
          	 	 	  			 selectIndex -= 14;
          	 	 	  		}
          	 	 	  		else
          	 	 	  		{
          	 	 	  			 selectIndex = 68;
          	 	 	  	  }
          	 	 	  		selectChar(selectIndex);
          	 	 	   }
          	 	 	   
          	 	 	   if (inputStatus==STATUS_NONE)
          	 	 	   {
          	 	 	  	  selectIndex = 0;
                      selectChar(selectIndex);
                   }
                }
             	 	break;
             
              case 20:   //菜单第20层
             	  stringUpDown(passWord, pwLight,0);
             	  inputPassWord(pwLight);
             	  break;
            }
      	  	break;

      	  case KEY_DOWN:     //下键
           	switch(menuInLayer)
            {
             	case 1:
             	 	if (layer1MenuLight>MAX_LAYER1_MENU-2)
             	 	{
             	 	 	 layer1MenuLight = 0;
             	 	}
             	 	else
             	 	{
             	 	 	 layer1MenuLight++;
             	 	}
             	 	layer1Menu(layer1MenuLight);
             	  break;

             	case 2:    //菜单第2层
             		switch(layer1MenuLight)
             		{
             			case 0:
             			case 1:
             				if (layer2MenuLight[layer1MenuLight]>layer2MenuNum[layer1MenuLight]-2)
             				{
             					layer2MenuLight[layer1MenuLight] = 0;
             				}
             				else
             				{
             					layer2MenuLight[layer1MenuLight]++;
             				}
             				layer2Menu(layer2MenuLight[layer1MenuLight],layer1MenuLight);
             				break; 
             		}
             		break;
             		
              case 3:    //菜单第3层
              	switch(layer1MenuLight)
                {
                	case 0:  //实时数据
                	 	switch(layer2MenuLight[layer1MenuLight])
                	 	{
                	 		case 2:  //功率曲线
                	 			if (mpPowerCurveLight==0)
             	 	 	  	  {
             	 	 	  		 	stringUpDown(queryTimeStr, layer3MenuLight[0][2], 1);  //高亮的字符减小一个字符
             	 	 	  		 	powerCurve(mpPowerCurveLight, layer3MenuLight[0][2]);
             	 	 	  		}
             	 	 	  		else
             	 	 	  		{
             	 	 	  		 	keyLeftRight = 0;
             	 	 	  		 	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>=layer3MenuNum[layer1MenuLight][layer2MenuLight[layer1MenuLight]]-1)
             	 	 	  		 	{
             	 	 	  		 		layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]= 0;
             	 	 	  		 	}
             	 	 	  		 	else
             	 	 	  		 	{
                            layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
             	 	 	  		 	}
             	 	 	  		 	powerCurve(mpPowerCurveLight,layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
             	 	 	  		}
             	 	 	  		break;
             	 	 	  }
             	 	 	  break;

                	case 1:  //参数定值
                	 	switch(layer2MenuLight[layer1MenuLight])
                	 	{
    	    		         case 6: //虚拟专网用户名密码
                 	 	 	   if (inputStatus==STATUS_SELECT_CHAR)
                 	 	 	   {
                 	 	 	  		selectIndex += 14;
                 	 	 	  		if (selectIndex>69)
                 	 	 	  		{
                 	 	 	  			 selectIndex = 0;
                 	 	 	  		}
                 	 	 	  		selectChar(selectIndex);
                 	 	 	   }
                 	 	 	   if (inputStatus==STATUS_NONE)
                 	 	 	   {
                 	 	 	  	  selectIndex = 0;
                            selectChar(selectIndex);
                         }
    	    		           break;
    	    		           
        	 	 	  		   case 7: //表计配置
   		                   stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
    	 	 	  		 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
    	 	 	  		 	       {
    	 	 	  		 	     	   if (chrMp[1][0]>0x37)
    	 	 	  		 	     	   {
    	 	 	  		 	     	 	   chrMp[1][0] = 0x37;
    	 	 	  		 	     	   }
    	 	 	  		 	       }
    	 	 	  		 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==2)
    	 	 	  		 	       {
    	 	 	  		 	     	   if (chrMp[2][0]>0x32)
    	 	 	  		 	     	   {
    	 	 	  		 	     	 	   chrMp[2][0] = 0x32;
    	 	 	  		 	     	   }
    	 	 	  		 	       }
    	 	 	  		 	       if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==3)
    	 	 	  		 	       {
    	 	 	  		 	     	   if (chrMp[3][0]>0x37)
    	 	 	  		 	     	   {
    	 	 	  		 	     	 	   chrMp[3][0] = 0x37;
    	 	 	  		 	     	   }
    	 	 	  		 	       }
   		                   setZbMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
        	 	 	  		 	   break;
        	 	 	  		 	   
          	 	 	  		 case 10: //级联参数设置
       		               if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>1 && layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]<5)
       		               {
       		               	 stringUpDown(chrMp[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
       		               }
       		               else
       		               {
       		               	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
       		               	 {
       		               	 	  if (chrMp[0][0]!='3')
       		               	 	  {
       		               	 	  	 chrMp[0][0]='3';
       		               	 	  }
       		               	 }
       		               	 if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==1)
       		               	 {
       		               	 	 if (chrMp[1][0]=='0')
       		               	 	 {
       		               	 	  	chrMp[1][0]='2';
       		               	 	 }
       		               	 	 else
       		               	 	 {
       		               	 	  	chrMp[1][0]--;
       		               	 	 }
       		               	 }
       		               }
       		               setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
          	 	 	  		 	 break;
          	 	 	  		 	 
    	    	 	 	  	   case 12:    //维护接口模式设置
      	    	 	 	  	   keyLeftRight++;
      	    	 	 	  	   if (keyLeftRight>1)
      	    	 	 	  	   {
      	    	 	 	  	  	 keyLeftRight = 0;
      	    	 	 	  	   }
    	    	 	 	  	  	 setMainTain(keyLeftRight);      	    	 	 	  	  	
    	    	 	 	  	  	 break;

    	    	 	 	  	   case 13:    //第2路485口功能设置
      	    	 	 	  	   keyLeftRight++;
      	    	 	 	  	   if (keyLeftRight>1)
      	    	 	 	  	   {
      	    	 	 	  	  	 keyLeftRight = 0;
      	    	 	 	  	   }
    	    	 	 	  	  	 setRs485Port2(keyLeftRight);      	    	 	 	  	  	
    	    	 	 	  	  	 break;
    	    	 	 	  	  	 
             	 	 	  	 case 14: //以太网参数设置
      	    		         stringUpDown(chrEthPara[layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]],keyLeftRight,1);
      	    		         setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
             	 	 	  		 break;
    	    		      }
    	    		      break;
             	 	}
              	break;
              	
              case 4:    //菜单第4层
              	if (layer1MenuLight==1 && layer2MenuLight[1]==5)
              	{
    	 	 	  		 	 if (rowOfLight==2)
    	 	 	  		 	 {
    	 	 	  		 	 	  for(i=strlen(commParaItem[2]);i<16;i++)
    	 	 	  		 	 	  {
    	 	 	  		 	 	    commParaItem[2][i] = ' ';
    	 	 	  		 	 	  }
    	 	 	  		 	 	  commParaItem[2][16] = '\0';
    	 	 	  		 	 	  inputStatus=STATUS_NONE;
    	 	 	  		 	 	  inputIndex = 0;
    	 	 	  		 	 	  inputApn(inputIndex); //APN的输入处理
    	 	 	  		 	 }
    	 	 	  		 	 else
    	 	 	  		 	 {
   		               stringUpDown(commParaItem[rowOfLight],keyLeftRight,1);
   		               configPara(rowOfLight,keyLeftRight);
   		             }
      	    		}
      	    		break;
      	    		
            	case 5:    //菜单第5层
             	 	if (layer1MenuLight==1 && layer2MenuLight[1]==5)
             	 	{
          	 	 	   if (inputStatus==STATUS_SELECT_CHAR)
          	 	 	   {
          	 	 	  		selectIndex += 14;
          	 	 	  		if (selectIndex>69)
          	 	 	  		{
          	 	 	  			 selectIndex = 0;
          	 	 	  		}
          	 	 	  		selectChar(selectIndex);
          	 	 	   }
          	 	 	   if (inputStatus==STATUS_NONE)
          	 	 	   {
          	 	 	  	  selectIndex = 0;
                     selectChar(selectIndex);
                   }
                }
             	 	break;
      	    		
              case 20:   //菜单第20层
             	  stringUpDown(passWord, pwLight,1);
             	  inputPassWord(pwLight);
             	  break;

            }
      	  	break;

      	  case KEY_LEFT:     //左键
      	  	switch(menuInLayer)
      	  	{
      	  		case 2:
      	  			switch(layer1MenuLight)
      	  		  {
      	    		  case 6:  //终端信息
      	    		 	 	if (layer2MenuLight[6]>0)
      	    		 	 	{
      	    		 	 	  layer2MenuLight[6]--;
      	    		 	 	}
      	    		 	 	else
      	    		 	 	{
      	    		 	 	  layer2MenuLight[6] = layer2MenuNum[6]-1;
      	    		 	 	}
      	    	 	 	  terminalInfo(layer2MenuLight[6]);
      	    		  	break;
      	    		}
      	    		break;
      	    		
      	  		case 3:
           	  	switch(layer1MenuLight)
           	    {
           	    	case 0:  //实时数据
           	    		switch(layer2MenuLight[layer1MenuLight])
           	    		{
           	    			case 2:
                	 	 	  if (mpPowerCurveLight==0)
                	 	 	  {
                	 	 	  	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]==0)
                	 	 	  	{
                	 	 	  		 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 7;
                	 	 	  	}
                	 	 	  	else
                	 	 	  	{
                	 	 	  		 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]--;
                	 	 	  	}
                	 	 	  	powerCurve(mpPowerCurveLight,layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
                	 	 	  }
                	 	 	  else
                	 	 	  {
                	 	 	  	if (keyLeftRight==0)
                	 	 	  	{
                	 	 	  		 keyLeftRight = 3;
                	 	 	  	}
                	 	 	  	else
                	 	 	  	{
                	 	 	  		 keyLeftRight--;
                	 	 	  	}
                	 	 	  	powerCurve(mpPowerCurveLight,layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
                	 	 	  	break;
                	 	 	 }
                  	 	 break;
                  	}
                  	break;
                  
             	 	 	case 1:  //参数定值
                 	  switch(layer2MenuLight[layer1MenuLight])
                 	  {
      	    		      case 6: //虚拟专网用户名密码        	 	 	  	
              	 	 	  	if (inputStatus==STATUS_NONE)
              	 	 	  	{
                          if (keyLeftRight==0)
                       	  {
                       	 	  keyLeftRight = 63;
                       	  }
                       	  else
                       	  {
                       	 	  keyLeftRight--;
                       	  }
      	    		          setVpn(keyLeftRight);
              	 	 	  	}
              	 	 	  	else
              	 	 	  	{
              	 	 	  	  if (selectIndex==0)
              	 	 	  	  {
              	 	 	  		  selectIndex=68;
              	 	 	  	  }
              	 	 	  	  else
              	 	 	  	  {
              	 	 	  		  selectIndex--;
              	 	 	  	  }
              	 	 	  	  selectChar(selectIndex);
              	 	 	  	}
    	    		          break;
    	    		          
    	    		        case 7:  //表计配置
                        adjustSetMeterParaLight(0,1);
      	    		        setZbMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
    	    		        	break;
      	    		      
      	    		      case 10:  //终端级联参数设置
      	    		        adjustCasParaLight(0);
      	    		        setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		        break;
      	    	 	 	  	
      	    	 	 	  	case 11:  //调节LCD对比度
      	    		 	 	 	  if (lcdDegree>0)
      	    		 	 	 	  {
      	    		 	 	 	  	lcdDegree--;
      	    		 	 	 	  }
      	    	 	 	  	  setLcdDegree(lcdDegree);
      	    	 	 	  	  break;

      	    		      case 14:  //以太网参数设置
      	    		        adjustEthParaLight(0);
      	    		        setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);      	    		         	 
      	    		        break;
    	    		      }
             	 	 		break;
                }
                break;
              
              case 4:    //菜单第4层
              	if (layer1MenuLight==1 && layer2MenuLight[1]==5)
              	{
              	   adjustCommParaLightIII(0);
      	    		   configPara(rowOfLight, keyLeftRight);
      	    		}
              	break;
              	
             	case 5:    //菜单第5层
             	 	if (layer1MenuLight==1 && layer2MenuLight[1]==5)
             	 	{
           	 	 	  if (inputStatus==STATUS_NONE)
           	 	 	  {
           	 	 	  	  if (inputIndex==0)
           	 	 	  	  {
           	 	 	  		  inputIndex = 15;
           	 	 	  	  }
           	 	 	  	  else
           	 	 	  	  {
           	 	 	  		  inputIndex--;
           	 	 	  	  }
           	 	 	  	  inputApn(inputIndex);
           	 	 	  }
           	 	 	  else
           	 	 	  {
           	 	 	    if (selectIndex==0)
           	 	 	  	{
           	 	 	  		selectIndex=68;
           	 	 	  	}
           	 	 	  	else
           	 	 	  	{
           	 	 	  		selectIndex--;
           	 	 	  	}
           	 	 	  	selectChar(selectIndex);
           	 	 	  }
             	 	}             	 	 	  
             	 	break;
               
              case 20:   //菜单第20层(输入密码)
       	    		if (pwLight==0)
       	    		{
       	    		  pwLight = 5;
       	    		}
       	    		else
       	    		{
       	    		 	pwLight--;
       	    		}
       	    		inputPassWord(pwLight);
              	break;
            }
      	  	break;

      	  case KEY_RIGHT:    //右键
      	  	switch(menuInLayer)
      	    {
      	  		case 2:
      	  			switch(layer1MenuLight)
      	  		  {
      	    		  case 6:  //终端信息
      	    		 	 	if (layer2MenuLight[6]>layer2MenuNum[6]-2)
      	    		 	 	{
      	    		 	 	  layer2MenuLight[6] = 0;;
      	    		 	 	}
      	    		 	 	else
      	    		 	 	{
      	    		 	 	  layer2MenuLight[6]++;
      	    		 	 	}
      	    	 	 	  terminalInfo(layer2MenuLight[6]);
      	    		  	break;
      	    		}
      	    		break;

          	  case 3:
          	   	switch(layer1MenuLight)
          	    {
          	    	case 0:  //实时数据
          	    		switch(layer2MenuLight[layer1MenuLight])
          	    		{
          	    			case 2:
                 	 	 	  if (mpPowerCurveLight==0)
                 	 	 	  {
                 	 	 	  	if (layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]>=7)
                 	 	 	  	{
                 	 	 	  		 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
                 	 	 	  	}
                 	 	 	  	else
                 	 	 	  	{
                 	 	 	  		 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
                 	 	 	  	}
                 	 	 	  	powerCurve(mpPowerCurveLight,layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
                 	 	 	  }
                 	 	 	  else
                 	 	 	  {
                 	 	 	  	if (keyLeftRight>=3)
                 	 	 	  	{
                 	 	 	  		 keyLeftRight = 0;
                 	 	 	  	}
                 	 	 	  	else
                 	 	 	  	{
                 	 	 	  		 keyLeftRight++;
                 	 	 	  	}
                 	 	 	  	powerCurve(mpPowerCurveLight,layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]);
                 	 	 	  	break;
                 	 	 	 }
                 	 	 	 break;
                 	  }
                 	  break;
                 	  
          	    	case 1:  //参数定值
          	    		switch(layer2MenuLight[layer1MenuLight])
          	    		{
    	    		        case 6:  //虚拟专网用户名密码
               	 	 	  	if (inputStatus==STATUS_NONE)
               	 	 	  	{
               	 	 	  	   if (keyLeftRight<63)
               	 	 	  	   {
               	 	 	  		   keyLeftRight++;
               	 	 	  	   }
               	 	 	  	   else
               	 	 	  	   {
               	 	 	  		   keyLeftRight = 0;
               	 	 	  	   }
               	 	 	  	   setVpn(keyLeftRight);
               	 	 	  	}
               	 	 	  	else
               	 	 	  	{
               	 	 	  	   if (selectIndex<69)
               	 	 	  	   {
               	 	 	  		   selectIndex++;
               	 	 	  	   }
               	 	 	  	   else
               	 	 	  	   {
               	 	 	  		   selectIndex = 0;
               	 	 	  	   }
               	 	 	  	   selectChar(selectIndex);
               	 	 	  	}
    	    		          break;

      	    		      case 7:  //表计配置
                        adjustSetMeterParaLight(1,1);
      	    		        setZbMeter(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		        break;

      	    		      case 10:  //终端级联参数设置
      	    		        adjustCasParaLight(1);
      	    		        setCascadePara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		        break;
      	    	 	 	  	
      	    	 	 	  	case 11:  //调节LCD对比度
      	    		 	 	 	  if (lcdDegree<20)
      	    		 	 	 	  {
      	    		 	 	 	  	lcdDegree++;
      	    		 	 	 	  }
      	    	 	 	  	  setLcdDegree(lcdDegree);
      	    	 	 	  	  break;
      	    	 	 	  	  
      	    		      case 14:  //以太网参数设置
      	    		        adjustEthParaLight(1);
      	    		        setEthPara(layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]],keyLeftRight);
      	    		        break;
    	    		      }
                }
                break;

              case 4:    //菜单第4层
              	if (layer1MenuLight==1 && layer2MenuLight[1]==5)
              	{
              	   adjustCommParaLightIII(1);
      	    		   configPara(rowOfLight, keyLeftRight);
      	    		}
              	break;
              	
             	case 5:    //菜单第5层
             	 	if (layer1MenuLight==1 && layer2MenuLight[1]==5)
             	 	{
        	 	 	  	if (inputStatus==STATUS_NONE)
        	 	 	  	{
        	 	 	  	  if (inputIndex<15)
        	 	 	  	  {
        	 	 	  		   inputIndex++;
        	 	 	  	  }
        	 	 	  	  else
        	 	 	  	  {
        	 	 	  		   inputIndex = 0;
        	 	 	  	  }
        	 	 	  	  inputApn(inputIndex);
        	 	 	  	}
        	 	 	  	else
        	 	 	  	{
        	 	 	  	  if (selectIndex<69)
        	 	 	  	  {
        	 	 	  		   selectIndex++;
        	 	 	  	  }
        	 	 	  	  else
        	 	 	  	  {
        	 	 	  		   selectIndex = 0;
        	 	 	  	  }
        	 	 	  	  selectChar(selectIndex);
        	 	 	  	}
             	 	}             	 	 	  
             	 	break;


             	case 20:   //菜单第20层(输入密码)
      	    		if (pwLight<5)
      	    		{
      	    		  pwLight++;
      	    		}
      	    		else
      	    		{
      	    		 	pwLight = 0;
      	    		}
      	    		inputPassWord(pwLight);
             	 	break;
            }
      	  	break;
      	}
      }
   }
}

/*******************************************************
函数名称:searchCtrlEvent
功能描述:搜索控制事件
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void searchCtrlEvent(INT8U erc)
{
  INT16U    j;
  struct    eventShowLink *tmpEventNode,*tmpPrevEvent;
  INT8U     tmpReadBuff[200];
  DATE_TIME tmpTime;
	
	tmpPrevEvent = eventLinkHead;

  for(j=iEventCounter;j>0;j--)
  {
     tmpTime = sysTime;
     if (readMeterData(tmpReadBuff, 1 , EVENT_RECORD, j, &tmpTime, 0)==TRUE)
     {
       if (tmpReadBuff[0]==erc)  //ERC
       {
         tmpEventNode = (struct eventShowLink *)malloc(sizeof(struct eventShowLink));
         tmpEventNode->eventNo = j;     //事件序号
         tmpEventNode->next = NULL;
		        
         if (eventLinkHead==NULL)
         {
      	   eventLinkHead = tmpEventNode;
         }
         else
         {
      	   tmpPrevEvent->next = tmpEventNode;
         }
         tmpPrevEvent = tmpEventNode;
       }
     }
  }
}

/*******************************************************
函数名称:power2Str
功能描述:数据格式2功率转换成字符串
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
char * power2Str(INT8U *pData)
{
	 char      say[35],str[25];
   INT16U    multiply,decimal,integer;

   if (*pData==0xee)
   {
   	  strcpy(say,"");
   	  return say;
   }

   //符号(+/-)
   if (*(pData+1) & 0x10)
   {
   	  strcpy(say,"-");
   }
   else
   {
   	  strcpy(say,"");
   }

   if (((*(pData+1)>>5) & 0x7) <5)
   {
   	  switch((*(pData+1)>>5) & 0x7)
   	  {
 	  	  case 0:
 	  	  	multiply = 10000;
 	  	  	break;
 	  	  case 1:
 	  	  	multiply = 1000;
 	  	  	break;
 	  	  case 2:
 	  	  	multiply = 100;
 	  	  	break;
 	  	  case 3:
 	  	  	multiply = 10;
 	  	    break;
 	  	  case 4:
 	  	  	multiply = 1;
 	  	  	break;
   	  }
   	  strcat(say,floatToString(((*(pData+1)& 0xf)*100+(*pData>>4 & 0xf)*10 + (*pData & 0xf))*multiply,0,3,3,str));
    }
   	else
    {
      switch((*(pData+1)>>5) & 0x7)
      {
    	  case 5:
    	  	integer = (*(pData+1) & 0xf)*10+(*pData>>4 & 0xf);
    	  	decimal = *pData & 0xf;
    	  	break;
    	  case 6:
    	  	integer = *(pData+1) & 0xf;
    	  	decimal = (*pData>>4 & 0xf)*10 + (*pData & 0xf);
    	  	if (decimal<100)
    	  	{
    	  		 decimal *= 10;
    	  	}
    	  	break;
    	  case 7:
    	  	integer = 0;
    	  	decimal = (*(pData+1) & 0xf)*100 + (*pData>>4 & 0xf)*10 + (*pData & 0xf);
    	  	break;
      }
      
      strcat(say,floatToString(integer,decimal,3,3,str));
    }
    strcat(say,"kW");
    
    return say;
}

/*******************************************************
函数名称:eventRecordShow
功能描述:事件记录显示
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void eventRecordShow(INT8U erc)
{
   INT8U     tmpReadBuff[200];
   DATE_TIME tmpTime;
   INT8U     *pData;
   INT8U     row;
   INT32U    tmpData;
   INT8U     type;
 	 
 	 char      say[50],str[25];
	 
	 guiLine(1,17,160,160,0);

   menuInLayer = 3;
   
   switch(erc)
   {
  	 case 5:   //遥控记录
  	 	 strcpy(str,"遥控记录");
  	 	 break;

  	 case 6:   //功控记录
  	 	 strcpy(str,"功控记录");
  	 	 break;

  	 case 7:   //电控记录
  	 	 strcpy(str,"电控记录");
  	 	 break;
  	 	 
  	 case 14:
  	 	 strcpy(str,"失电记录");
  	 	 break;
   }
   
   if (tmpEventShow==NULL)
   {
   	  strcpy(say,"无");
   	  strcat(say,str);
   	  strcat(say,"!");
   	  
   	  guiDisplay(10,LCD_LINE_3,say,1);
      lcdRefresh(17, 160);
      
      return;
   }
   
   guiDisplay(49, LCD_LINE_1, str, 0);
   
   tmpTime = sysTime;
   
   printf("eventNo=%d\n",tmpEventShow->eventNo);
   
   if (readMeterData(tmpReadBuff, 1 , EVENT_RECORD, tmpEventShow->eventNo, &tmpTime, 0)==TRUE)
   {
   	  sprintf(say,"%02x-%02x-%02x %02x:%02x:%02x",tmpReadBuff[7],tmpReadBuff[6],tmpReadBuff[5],tmpReadBuff[4],tmpReadBuff[3],tmpReadBuff[2]);
   	  guiDisplay(13, LCD_LINE_3, say, 1);
   	  
   	  pData = &tmpReadBuff[8];
   	  
   	  if (erc==14)
   	  {   	    
   	    guiDisplay(1, LCD_LINE_5, "上电时间:", 1);   	  	 
   	    sprintf(say,"%02x-%02x-%02x %02x:%02x:%02x",*(pData+5),*(pData+4),*(pData+3),*(pData+2),*(pData+1),*pData);
   	    guiDisplay(13, LCD_LINE_6, say, 1);   	  	 
   	  }
   	  else
   	  {
     	  if (erc!=5)
     	  {
     	    sprintf(say,"总加组:%d",*pData);
     	    guiDisplay(17, LCD_LINE_2, say, 1);
     	    pData++;
     	  }
     	  
     	  strcpy(say,"跳闸轮次:");
     	  if (*pData&0x1)
     	  {
     	  	 strcat(say,"①");
     	  }
     	  if (*pData&0x2)
     	  {
     	  	 strcat(say,"②");
     	  }
     	  if (*pData&0x4)
     	  {
     	  	 strcat(say,"③");
     	  }
     	  if (*pData&0x8)
     	  {
     	  	 strcat(say,"④");
     	  }
     	  guiDisplay(1, LCD_LINE_4, say, 1);
     	  pData++;
     	  row = LCD_LINE_5;
     	  
     	  switch(erc)
     	  {
     	  	 case 5:    //遥控记录
     	  	 	 strcpy(say,"跳闸前功率:");
     	  	 	 strcat(say,power2Str(pData));
     	  	 	 guiDisplay(1,row,say,1);
     	  	 	 pData += 2;
     	  	 	 row += 16;
  
     	  	 	 strcpy(say,"跳闸后功率:");
     	  	 	 strcat(say,power2Str(pData));
     	  	 	 guiDisplay(1,row,say,1);
     	  	 	 break;
     	  	 	 
     	  	 case 6:    //功控记录
     	  	 	 strcpy(say,"功控类别:");
     	  	 	 if (*pData&0x1)
     	  	 	 {
     	  	 	 	 strcat(say,"时段控");
     	  	 	 }
     	  	 	 if (*pData&0x2)
     	  	 	 {
     	  	 	 	 strcat(say,"厂休控");
     	  	 	 }
     	  	 	 if (*pData&0x4)
     	  	 	 {
     	  	 	 	 strcat(say,"营业报停控");
     	  	 	 }
     	  	 	 if (*pData&0x8)
     	  	 	 {
     	  	 	 	 strcat(say,"下浮控");
     	  	 	 }
     	       guiDisplay(1, row, say, 1);
     	       row += 16;
     	       pData++;
     	  	 	 
     	  	 	 strcpy(say,"跳闸前功率:");
     	  	 	 strcat(say,power2Str(pData));
     	  	 	 guiDisplay(1,row,say,1);
     	  	 	 pData += 2;
     	  	 	 row += 16;
  
     	  	 	 strcpy(say,"跳闸后功率:");
     	  	 	 strcat(say,power2Str(pData));
     	  	 	 guiDisplay(1,row,say,1);
     	  	 	 pData += 2;
     	  	 	 row += 16;
  
     	  	 	 strcpy(say,"跳闸功率定值:");
     	  	 	 strcat(say,power2Str(pData));
     	  	 	 guiDisplay(1,row,say,1);
     	  	 	 pData += 2;
     	  	 	 row += 16;   	  	 	 
     	  	 	 break;
     	  	 	 
     	  	 case 7:    //电控记录
     	  	 	 strcpy(say,"电控类别:");
     	  	 	 type = 0;
     	  	 	 if (*pData&0x1)
     	  	 	 {
     	  	 	 	 strcat(say,"月电控");
     	  	 	 	 type = 1;
     	  	 	 }
     	  	 	 if (*pData&0x2)
     	  	 	 {
     	  	 	 	 strcat(say,"购电控");
     	  	 	 	 type = 2;
     	  	 	 }
     	       guiDisplay(1, row, say, 1);
     	       row += 16;
     	       pData++;
     	  	 	 
     	  	 	 if (type==2)
     	  	 	 {
     	  	 	   strcpy(say,"跳闸时剩余:");
     	  	 	 }
     	  	 	 else
     	  	 	 {
     	  	 	   strcpy(say,"月电能量:");
     	  	 	 }
     	  	 	 tmpData = (*(pData+3)&0xf)<<24 | *(pData+2)<<16 | *(pData+1)<<8 | *pData;
     	  	 	 if (*(pData+3)&0x10)
     	  	 	 {
     	  	 	 	 strcat(say, "-");
     	  	 	 }
     	  	 	 tmpData = bcdToHex(tmpData);
     	  	 	 strcat(say,intToString(tmpData,3,str));
     	  	 	 if (*(pData+3)&0x40)
     	  	 	 {
     	  	 	 	 strcat(say, "MWh");
     	  	 	 }
     	  	 	 else
     	  	 	 {
     	  	 	 	 strcat(say, "kWh");
     	  	 	 }
     	  	 	 guiDisplay(1,row,say,1);
     	  	 	 pData += 4;
     	  	 	 row += 16;
  
     	  	 	 if (type==2)
     	  	 	 {
     	  	 	   strcpy(say,"跳闸门限:");
     	  	 	 }
     	  	 	 else
     	  	 	 {
     	  	 	   strcpy(say,"月电控定值:");
     	  	 	 }
     	  	 	 tmpData = (*(pData+3)&0xf)<<24 | *(pData+2)<<16 | *(pData+1)<<8 | *pData;
     	  	 	 tmpData = bcdToHex(tmpData);
     	  	 	 if (*(pData+3)&0x10)
     	  	 	 {
     	  	 	 	 strcat(say, "-");
     	  	 	 }
     	  	 	 strcat(say,intToString(tmpData,3,str));
     	  	 	 if (*(pData+3)&0x40)
     	  	 	 {
     	  	 	 	 strcat(say, "MWh");
     	  	 	 }
     	  	 	 else
     	  	 	 {
     	  	 	 	 strcat(say, "kWh");
     	  	 	 }
     	  	 	 guiDisplay(1,row,say,1);
     	  	 	 break;
     	  }
     	}
   }
   else
   {
     guiDisplay(49, LCD_LINE_3, "读取事件记录失败", 1);   	 
   }   
   
   lcdRefresh(17, 160);
}

/*******************************************************
函数名称:defaultMenu
功能描述:默认菜单(主界面,国网376.1专变III型终端规约菜单)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void defaultMenu(void)
{
	 while (eventLinkHead!=NULL)
	 {
	   tmpEventShow = eventLinkHead;
	   eventLinkHead = eventLinkHead->next;
	   
	   free(tmpEventShow);
	 }

	 layer1Menu(layer1MenuLight);
}

/*******************************************************
函数名称:layer2Menu
功能描述:二层菜单(376.1国家电网终端规约)
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void layer2Menu(int lightNum,int layer1Num)
{
	 INT8U startX, startY;
	 INT8U i, j;
	 INT8U startItem=0, endItem=0;

   if (layer1Num==0)
	 {
	 	 startX = 40;
	 }
	 else
	 {
	 	 startX = 35;
	 }
	 startY = 20;

	 guiLine(1, 17, 160, 160, 0);

   menuInLayer = 2;
   
   if (layer2MenuNum[layer1Num]>8)
   {
   	 if (lightNum>7)
   	 {
   	 	 startItem = lightNum-7;
   	 	 endItem = layer2MenuNum[layer1Num]-startItem;
   	 }
   	 else
   	 {
   	   startItem = 0;
   	   endItem = 8;
   	 }
   }
   else
   {
   	 startItem = 0;
   	 endItem = layer2MenuNum[layer1Num];
   }
   
	 for(i=0,j=startItem; i<endItem; i++,j++)
	 {
		 if (j==lightNum)
		 {
       guiDisplay(startX,startY+i*17,layer2MenuItem[layer1Num][j],0);
		 }
		 else
		 {
       guiDisplay(startX,startY+i*17,layer2MenuItem[layer1Num][j],1);
		 }
	 }

   lcdRefresh(17,160);               //刷新LCD
}

/*******************************************************
函数名称:currentPower
功能描述:当前功率
调用函数:
被调用函数:
输入参数:item
输出参数:
返回值:void
*******************************************************/
void currentPower(INT8U item)
{
 	  char      say[30],str[25],units[6],sayx[10];
 	  INT16U    multiply;
 	  INT8U     i;
 	  INT8U     tmpPort;
 	  INT32U    integer;
 	  INT32U    decimal;
 	  INT8U     dataBuff[LEN_OF_ZJZ_BALANCE_RECORD];
 	  DATE_TIME tmpTime;
 	  
 	  menuInLayer = 3;
	  guiLine(1,17,160,160,0);
	  
	  if (totalAddGroup.numberOfzjz==0 && pulseConfig.numOfPulse==0)
	  {
  	  guiDisplay(4,LCD_LINE_3,"无总加组及脉冲配置!",1);
  	  lcdRefresh(17,160);
  	  
  	  return;
	  }
	  
	  if (item<totalAddGroup.numberOfzjz)
	  {
  	  strcpy(say,"总加组");
  	  strcat(say,digital2ToString(totalAddGroup.perZjz[item].zjzNo,str));
  	  guiDisplay(48,LCD_LINE_1,say,0);

      tmpTime = timeHexToBcd(sysTime);
      if (readMeterData(dataBuff, totalAddGroup.perZjz[item].zjzNo, LAST_REAL_BALANCE, GROUP_REAL_BALANCE, &tmpTime, 0) == TRUE)
	  	{
 	      strcpy(say,"有功功率=");
 	      if (dataBuff[GP_WORK_POWER+1]!=0xEE)
 	      {
   	      //符号(+/-)
   	      if (dataBuff[GP_WORK_POWER] & 0x10)
   	      {
   	   	    strcat(say,"-");
   	      }
   	      else
   	      {
   	   	    strcat(say,"");
   	   	  }

   	   	  if ((dataBuff[GP_WORK_POWER] & 0x7) <5)
   	   	  {
   	   	    switch(dataBuff[GP_WORK_POWER] & 0x7)
   	   	    {
   	   	  	  case 0:
   	   	  	  	multiply = 10000;
   	   	  	  	break;
   	   	  	  case 1:
   	   	  	  	multiply = 1000;
   	   	  	  	break;
   	   	  	  case 2:
   	   	  	  	multiply = 100;
   	   	  	  	break;
   	   	  	  case 3:
   	   	  	  	multiply = 10;
   	   	  	    break;
   	   	  	  case 4:
   	   	  	  	multiply = 1;
   	   	  	  	break;
   	   	    }
   	   	    strcat(say,floatToString(((dataBuff[GP_WORK_POWER+2]& 0xf)*100+(dataBuff[GP_WORK_POWER+1]>>4 & 0xf)*10 + (dataBuff[GP_WORK_POWER+1] & 0xf))*multiply,0,3,2,str));
   	   	  }
   	   	  else
   	   	  {
   	   	    switch(dataBuff[GP_WORK_POWER] & 0x7)
   	   	    {
   	   	  	  case 5:
   	   	  	  	integer = (dataBuff[GP_WORK_POWER+2] & 0xf)*10+(dataBuff[GP_WORK_POWER+1]>>4 & 0xf);
   	   	  	  	decimal = (dataBuff[GP_WORK_POWER+1] & 0xf)*100;
   	   	  	  	break;
   	   	  	  	
   	   	  	  case 6:
   	   	  	  	integer = dataBuff[GP_WORK_POWER+2] & 0xf;
   	   	  	  	decimal = (dataBuff[GP_WORK_POWER+1]>>4 & 0xf)*10 + (dataBuff[GP_WORK_POWER+1] & 0xf);
   	   	  	  	if (decimal<100)
   	   	  	  	{
   	   	  	  		 decimal *= 10;
   	   	  	  	}
   	   	  	  	break;
   	   	  	  	
   	   	  	  case 7:
   	   	  	  	integer = 0;
   	   	  	  	decimal = (dataBuff[GP_WORK_POWER+2] & 0xf)*100 + (dataBuff[GP_WORK_POWER+1]>>4 & 0xf)*10 + (dataBuff[GP_WORK_POWER+1] & 0xf);
   	   	  	  	break;
   	   	    }
   	   	    
   	   	    strcat(say,floatToString(integer,decimal,3,2,str));
   	   	  }
   	      strcat(say,"kW");
   	    }   	    
   	    guiDisplay(1,LCD_LINE_3,say,1);
   	    
 	      strcpy(say,"无功功率=");
 	      if (dataBuff[GP_NO_WORK_POWER+1]!=0xEE)
 	      {
   	      //符号(+/-)
   	      if (dataBuff[GP_NO_WORK_POWER] & 0x10)
   	      {
   	   	    strcat(say,"-");
   	      }
   	      else
   	      {
   	   	    strcat(say,"");
   	   	  }

   	   	  if ((dataBuff[GP_NO_WORK_POWER] & 0x7) <5)
   	   	  {
   	   	    switch(dataBuff[GP_NO_WORK_POWER] & 0x7)
   	   	    {
   	   	  	  case 0:
   	   	  	  	multiply = 10000;
   	   	  	  	break;
   	   	  	  case 1:
   	   	  	  	multiply = 1000;
   	   	  	  	break;
   	   	  	  case 2:
   	   	  	  	multiply = 100;
   	   	  	  	break;
   	   	  	  case 3:
   	   	  	  	multiply = 10;
   	   	  	    break;
   	   	  	  case 4:
   	   	  	  	multiply = 1;
   	   	  	  	break;
   	   	    }
   	   	    strcat(say,floatToString(((dataBuff[GP_NO_WORK_POWER+2]& 0xf)*100+(dataBuff[GP_NO_WORK_POWER+1]>>4 & 0xf)*10 + (dataBuff[GP_NO_WORK_POWER+1] & 0xf))*multiply,0,3,2,str));
   	   	  }
   	   	  else
   	   	  {
   	   	    switch(dataBuff[GP_NO_WORK_POWER] & 0x7)
   	   	    {
   	   	  	  case 5:
   	   	  	  	integer = (dataBuff[GP_NO_WORK_POWER+2] & 0xf)*10+(dataBuff[GP_NO_WORK_POWER+1]>>4 & 0xf);
   	   	  	  	decimal = (dataBuff[GP_NO_WORK_POWER+1] & 0xf)*100;
   	   	  	  	break;
   	   	  	  	
   	   	  	  case 6:
   	   	  	  	integer = dataBuff[GP_NO_WORK_POWER+2] & 0xf;
   	   	  	  	decimal = (dataBuff[GP_NO_WORK_POWER+1]>>4 & 0xf)*10 + (dataBuff[GP_NO_WORK_POWER+1] & 0xf);
   	   	  	  	if (decimal<100)
   	   	  	  	{
   	   	  	  		 decimal *= 10;
   	   	  	  	}
   	   	  	  	break;
   	   	  	  	
   	   	  	  case 7:
   	   	  	  	integer = 0;
   	   	  	  	decimal = (dataBuff[GP_NO_WORK_POWER+2] & 0xf)*100 + (dataBuff[GP_NO_WORK_POWER+1]>>4 & 0xf)*10 + (dataBuff[GP_NO_WORK_POWER+1] & 0xf);
   	   	  	  	break;
   	   	    }
   	   	    
   	   	    strcat(say,floatToString(integer,decimal,3,2,str));
   	   	  }
   	      strcat(say,"kvar");
   	    }   	    
   	    guiDisplay(1,LCD_LINE_4,say,1);
      }
      else
      {
   	    guiDisplay(1,LCD_LINE_3,"有功功率=",1);
   	    guiDisplay(1,LCD_LINE_4,"无功功率=",1);
      }
  	}
  	else
  	{	  
  	  item -= totalAddGroup.numberOfzjz;
  	  if (item<pulseConfig.numOfPulse)
  	  {
    	  if (pulseConfig.numOfPulse<1)
    	  {
    	  	 guiDisplay(1,LCD_LINE_1,"无脉冲配置参数!",1);
    	  	 lcdRefresh(2,8);
    	  	 return;
    	  }
    
    	  strcpy(say,"脉冲量端口");
    	  strcat(say,digital2ToString(pulseConfig.perPulseConfig[item].ifNo,str));
    	  guiDisplay(32,LCD_LINE_1,say,0);
    	  
    	  tmpPort = pulseConfig.perPulseConfig[item].ifNo-1;
    	  strcpy(say,"测量点=");
    	  strcat(say,digital2ToString(pulseConfig.perPulseConfig[item].pn,str));
    	  strcat(say," 常数=");
    	  strcat(say,intToString(pulseConfig.perPulseConfig[item].meterConstant[0] | pulseConfig.perPulseConfig[item].meterConstant[1]<<8,3,str));
    	  guiDisplay(1,LCD_LINE_2,say,1);
    
    	  switch(pulseConfig.perPulseConfig[item].character&0x3)
    	  {
    	  	 case 0:
    	  	 	 strcpy(sayx,"正向有功=");
    	  	 	 strcpy(units,"kW");
    	  	 	 break;
    
    	  	 case 1:
    	  	 	 strcpy(sayx,"正向无功=");
    	  	 	 strcpy(units,"kvar");
    	  	 	 break;
    	  	 	 
    	  	 case 2:
    	  	 	 strcpy(sayx,"反向有功=");
    	  	 	 strcpy(units,"kW");
    	  	 	 break;
    	  	 	 
    	  	 case 3:
    	  	 	 strcpy(sayx,"反向无功=");
    	  	 	 strcpy(units,"kvar");
    	  	 	 break;
    	  }
    	  
    	  strcpy(say,"功率=");
    	  integer = pulse[tmpPort].prevMinutePulse*pulse[tmpPort].voltageTimes*pulse[tmpPort].currentTimes*60
    	          /pulse[tmpPort].meterConstant;
        decimal = pulse[tmpPort].prevMinutePulse*pulse[tmpPort].voltageTimes*pulse[tmpPort].currentTimes*60
                %pulse[tmpPort].meterConstant*100/pulse[tmpPort].meterConstant;
    	  strcat(say,floatToString(integer,decimal,2,2,str));
        strcat(say,units);
    	  guiDisplay(1,LCD_LINE_4,say,1);
    	  
    	  //示值整数
    	  strcpy(say,sayx);
    	  integer = pulseDataBuff[53*tmpPort] | pulseDataBuff[53*tmpPort+1]<<8 
    	            | pulseDataBuff[53*tmpPort+2]<<16;
    
    	  //示值*电压变比*电流变比=整数部分的能量
    	  integer *= (pulse[tmpPort].voltageTimes*pulse[tmpPort].currentTimes);
    
    	  //(能量+尾数部分乘倍率/常数)= 能量整数部分
    	  integer += (pulse[tmpPort].pulseCount*pulse[tmpPort].voltageTimes*pulse[tmpPort].currentTimes
    	             /pulse[tmpPort].meterConstant);
    	  //小数
    	  decimal = (pulse[tmpPort].pulseCount*pulse[tmpPort].voltageTimes*pulse[tmpPort].currentTimes
    	            %pulse[tmpPort].meterConstant*100/pulse[tmpPort].meterConstant);
        strcat(say,floatToString(integer,decimal,2,2,str));
        strcat(say,units);
        strcat(say,"h");
    	  guiDisplay(1,LCD_LINE_6,say,1);
    	  
    
    	  //需量整数
    	  integer = (pulseDataBuff[53*tmpPort+5] | pulseDataBuff[53*tmpPort+6]<<8)
    	            *pulse[tmpPort].voltageTimes*pulse[tmpPort].currentTimes*60/pulse[tmpPort].meterConstant;
    	  decimal = (pulseDataBuff[53*tmpPort+5] | pulseDataBuff[53*tmpPort+6]<<8)
    	            *pulse[tmpPort].voltageTimes*pulse[tmpPort].currentTimes*60%pulse[tmpPort].meterConstant*100
    	            /pulse[tmpPort].meterConstant;
    	  strcpy(say,"需量=");
    	  strcat(say,floatToString(integer,decimal,2,2,str));
        strcat(say,units);    
    	  guiDisplay(1,LCD_LINE_7,say,1);
        
    	  strcpy(say,"需量时间");
    	  strcat(say,digital2ToString(pulseDataBuff[53*tmpPort+10],str));
    	  strcat(say,"-");
    	  strcat(say,digital2ToString(pulseDataBuff[53*tmpPort+9],str));
    	  strcat(say," ");
    	  strcat(say,digital2ToString(pulseDataBuff[53*tmpPort+8],str));
    	  strcat(say,":");
    	  strcat(say,digital2ToString(pulseDataBuff[53*tmpPort+7],str));
    	  guiDisplay(1,LCD_LINE_8,say,1);
    	}
  	}
    
	  lcdRefresh(17,160);
}

/*******************************************************
函数名称:currentEnergy
功能描述:当前电量
调用函数:
被调用函数:
输入参数:item
输出参数:
返回值:void
*******************************************************/
void currentEnergy(INT16U item)
{
	 INT8U                dataBuff[LEN_OF_ENERGY_BALANCE_RECORD];
 	 INT32U               integer,decimal;
   struct cpAddrLink    *tmpLink;
	 char                 str[15];
	 char                 sayStr[10];
	 DATE_TIME            time;
	 INT16U               i;
	 INT8U                tmpY;

 	 menuInLayer = 3;
	 guiLine(1,17,160,160,0);

 	 tmpLink = queryMpLink;
 	 i = 0;
 	 while(tmpLink!=NULL && i<item)
 	 {
 	   tmpLink = tmpLink->next;
 	   i++;
 	 }
 	 
 	 if (tmpLink!=NULL)
 	 {
   	 if (tmpLink->protocol==AC_SAMPLE)
   	 {
   	 	  tmpLink = tmpLink->next;
   	 	  if (tmpLink==NULL)
   	 	  {
   	 	  	 tmpLink = queryMpLink;
   	 	  	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
   	 	  }
   	 	  else
   	 	  {
   	 	  	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
   	 	  }
   	 }
  	 strcpy(sayStr,"测量点");
  	 strcat(sayStr,digital2ToString(tmpLink->mp,str));
  	 guiDisplay(48,LCD_LINE_1,sayStr,0);
     
     //查询本测量点的上次抄表时间
     time = queryCopyTime(tmpLink->mp);
     
     if (readMeterData(dataBuff, tmpLink->mp, LAST_REAL_BALANCE, REAL_BALANCE_POWER_DATA, &time, 0)==TRUE)
     {
        guiLine(1,LCD_LINE_2-1,160,LCD_LINE_2-1,1);
       	
       	tmpY=LCD_LINE_2;
       	for(i=0;i<5;i++)
       	{
       	  switch(i)
       	  {
       	  	case 0:
       	      strcpy(sayStr,"当日正向有功总=");
       	      break;
       	      
       	  	case 1:
       	      strcpy(sayStr,"尖=");
       	      break;

       	  	case 2:
       	      strcpy(sayStr,"峰=");
       	      break;
       	      
       	  	case 3:
       	      strcpy(sayStr,"平=");
       	      break;
       	      
       	  	case 4:
       	      strcpy(sayStr,"谷=");
       	      break;
       	  }
       	  
       	  if (dataBuff[DAY_P_WORK_OFFSET+i*7]!=0xee)
       	  {
             decimal = bcdToHex(dataBuff[DAY_P_WORK_OFFSET+i*7+1] |dataBuff[DAY_P_WORK_OFFSET+i*7+2]<<8);
             integer = bcdToHex(dataBuff[DAY_P_WORK_OFFSET+i*7+3] |dataBuff[DAY_P_WORK_OFFSET+i*7+4]<<8
                     |dataBuff[DAY_P_WORK_OFFSET+i*7+5]<<16 |dataBuff[DAY_P_WORK_OFFSET+i*7+6]<<24);
             strcpy(str, floatToString(integer,decimal,4,2,str));
             strcat(sayStr,str);
       	  }
       	  
       	  if ((i!=0) && (i%2==0))
       	  {
       	    guiDisplay(81,tmpY,sayStr,1);
       	  }
       	  else
       	  {
       	    guiDisplay(1,tmpY,sayStr,1);
       	  }
       	  
       	  if (i==0 || i==2)
       	  {
       	  	 tmpY += 16;
       	  }
       	}
       	
       	//当日正向无功总
       	strcpy(sayStr,"正向无功总=");
       	if (dataBuff[DAY_P_NO_WORK_OFFSET]!=0xee)
       	{
             decimal = bcdToHex(dataBuff[DAY_P_NO_WORK_OFFSET+1] |dataBuff[DAY_P_NO_WORK_OFFSET+2]<<8);
             integer = bcdToHex(dataBuff[DAY_P_NO_WORK_OFFSET+3] |dataBuff[DAY_P_NO_WORK_OFFSET+4]<<8
                     |dataBuff[DAY_P_NO_WORK_OFFSET+5]<<16 |dataBuff[DAY_P_NO_WORK_OFFSET+6]<<24);
             strcpy(str, floatToString(integer,decimal,4,2,str));
             strcat(sayStr,str);
       	}
       	guiDisplay(1,LCD_LINE_5,sayStr,1);
       	
       	guiLine(1,LCD_LINE_6-1,160,LCD_LINE_6-1,1);
       	
       	tmpY=LCD_LINE_6;
       	for(i=0;i<5;i++)
       	{
       	  switch(i)
       	  {
       	  	case 0:
       	      strcpy(sayStr,"当月正向有功总=");
       	      break;
       	      
       	  	case 1:
       	      strcpy(sayStr,"尖=");
       	      break;

       	  	case 2:
       	      strcpy(sayStr,"峰=");
       	      break;
       	      
       	  	case 3:
       	      strcpy(sayStr,"平=");
       	      break;
       	      
       	  	case 4:
       	      strcpy(sayStr,"谷=");
       	      break;
       	  }
       	  
       	  if (dataBuff[MONTH_P_WORK_OFFSET+i*7]!=0xee)
       	  {
             decimal = bcdToHex(dataBuff[MONTH_P_WORK_OFFSET+i*7+1] |dataBuff[MONTH_P_WORK_OFFSET+i*7+2]<<8);
             integer = bcdToHex(dataBuff[DAY_P_WORK_OFFSET+i*7+3] |dataBuff[DAY_P_WORK_OFFSET+i*7+4]<<8
                     |dataBuff[DAY_P_WORK_OFFSET+i*7+5]<<16 |dataBuff[DAY_P_WORK_OFFSET+i*7+6]<<24);
             strcpy(str, floatToString(integer,decimal,4,2,str));
             strcat(sayStr,str);
       	  }
       	  
       	  if ((i!=0) && (i%2==0))
       	  {
       	    guiDisplay(81,tmpY,sayStr,1);
       	  }
       	  else
       	  {
       	    guiDisplay(1,tmpY,sayStr,1);
       	  }
       	  
       	  if (i==0 || i==2)
       	  {
       	  	 tmpY += 16;
       	  }
       	}
       	//当日正向无功总
       	strcpy(sayStr,"正向无功总=");
       	if (dataBuff[MONTH_P_NO_WORK_OFFSET]!=0xee)
       	{
             decimal = bcdToHex(dataBuff[MONTH_P_NO_WORK_OFFSET+1] |dataBuff[MONTH_P_NO_WORK_OFFSET+2]<<8);
             integer = bcdToHex(dataBuff[MONTH_P_NO_WORK_OFFSET+3] |dataBuff[MONTH_P_NO_WORK_OFFSET+4]<<8
                     |dataBuff[MONTH_P_NO_WORK_OFFSET+5]<<16 |dataBuff[MONTH_P_NO_WORK_OFFSET+6]<<24);
             strcpy(str, floatToString(integer,decimal,4,2,str));
             strcat(sayStr,str);
       	}
       	guiDisplay(1,LCD_LINE_8+16,sayStr,1);
     }
     else
     {
     	  guiDisplay(1, LCD_LINE_2,"当日正向有功总=",1);
     	  guiDisplay(1, LCD_LINE_3,"尖=",1);
     	  guiDisplay(81,LCD_LINE_3,"峰=",1);
     	  guiDisplay(1, LCD_LINE_4,"平=",1);
     	  guiDisplay(81,LCD_LINE_4,"谷=",1);
     	  guiDisplay(81,LCD_LINE_5,"无功总=",1);
     	  
     	  guiDisplay(1, LCD_LINE_6,"当月正向有功总=",1);
     	  guiDisplay(1, LCD_LINE_7,"尖=",1);
     	  guiDisplay(81,LCD_LINE_7,"峰=",1);
     	  guiDisplay(1, LCD_LINE_8,"平=",1);
     	  guiDisplay(81,LCD_LINE_8,"谷=",1);
     	  guiDisplay(81,LCD_LINE_8+16,"无功总=",1);
     }     
   }
   else
   {
     guiDisplay(12, LCD_LINE_3,"无测量点配置参数!",1);   	 
   }
   
   lcdRefresh(17,160);
}

/*******************************************************
函数名称:powerCurve
功能描述:功率曲线
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void powerCurve(INT8U layer2Light,INT8U layer3Light)
{
	 INT8U             dataBuff[LENGTH_OF_ENERGY_RECORD];
 	 INT32U            integer,decimal,disData;
	 char              str[30];
	 char              sayStr[30];
	 INT8U             i, tmpY, tmpCount, tmpX=0;
	 INT8U             dataType;
	 INT8U             tmpMinute;
	 DATE_TIME         tmpTime, readTime;
   struct cpAddrLink *tmpLink;
   INT16U            offset;
   INT8U             sign;

	 guiLine(1,17,160,160,0); //清屏
	 menuInLayer = 3;         //菜单进入第2层
	 
	 switch(layer2Light)
	 {
	 	 case 0:
	 	   guiDisplay(48,17,"功率曲线",0);
	 	   showInputTime(layer3Light);
	 	   break;
	 	   
	 	 case 1:
       strcpy(sayStr, digital2ToString(sysTime.year,str));
       strcat(sayStr,"-");
       strcat(sayStr, digital2ToString(sysTime.month,str));
       strcat(sayStr,"-");
       strcat(sayStr, digital2ToString(sysTime.day,str));
       strcat(sayStr,"功率曲线");
	 	   guiDisplay(16,17,sayStr,0);
	 	   guiDisplay(78,33,"有功功率",1);
	 	   dataType = PARA_VARIABLE_DATA;
	 	   offset   = POWER_INSTANT_WORK;
	 	 	 break;

	 	 case 2:
       strcpy(sayStr, digital2ToString(sysTime.year,str));
       strcat(sayStr,"-");
       strcat(sayStr, digital2ToString(sysTime.month,str));
       strcat(sayStr,"-");
       strcat(sayStr, digital2ToString(sysTime.day,str));
       strcat(sayStr,"功率曲线");
	 	   guiDisplay(16,17,sayStr,0);
	 	   guiDisplay(78,33,"无功功率",1);
	 	   dataType = PARA_VARIABLE_DATA;
	 	   offset   = POWER_INSTANT_NO_WORK;
	 	 	 break;
	 }

	 lcdRefresh(17, 160);
	 
	 if (layer2Light>=1 && layer2Light<=2)
	 {
 	   tmpLink = queryMpLink;
 	   i = 0;
 	   while(tmpLink!=NULL && i<layer3Light)
 	   {
 	   	  tmpLink = tmpLink->next;
 	      i++;
 	   }
 	   if (tmpLink==NULL)
 	   {
 	   	  tmpLink = queryMpLink;
 	   }
 	   
 	   strcpy(sayStr,"测量点");
 	   if (tmpLink->mp<10)
 	   {
 	   	 strcat(sayStr,"0");
 	   }
 	   strcat(sayStr,intToString(tmpLink->mp,3,str));
 	   guiDisplay(1,33,sayStr,1);

		 tmpTime = menuQueryTime;
		 tmpTime.hour   = keyLeftRight*6;
		 tmpTime.minute = 0x0;
		 tmpTime.second = 0x0;
		 tmpTime = backTime(tmpTime, 0, 0, 0, 15, 0);
	   
	   lcdRefresh(33, 48);
		 
		 tmpY = 52;
		 for(i = 0; i < 6; i++)
		 {
				strcpy(sayStr,"");
				if (tmpTime.hour==23)
				{
					strcpy(sayStr,"00");
				}
				else
				{
				  if (tmpTime.hour<9)
				  {
					   strcat(sayStr,"0");
				  }
				  strcat(sayStr,intToString(tmpTime.hour+1,3,str));
				}
				guiDisplay(17, tmpY, sayStr, 1);
				
				readTime  = timeHexToBcd(tmpTime);
				strcpy(sayStr,"");
				if(readMeterData(dataBuff, tmpLink->mp, CURVE_DATA_PRESENT, dataType, &readTime, 0x60) == TRUE)
				{
				   if (dataBuff[offset]!=0xee)
       	   {
  				   switch(layer2Light)
  				   {
               case 1:
               case 2:
     	    	     sign = 0;
               	 disData = dataBuff[offset] | dataBuff[offset+1]<<8 | dataBuff[offset+2]<<16;
     	    	     if (dataBuff[offset+2]&0x80)
     	    	     {
     	    	        sign = 1;
     	    	        disData &=0x7fff;
     	    	     }
                 decimal = (disData>>12 & 0xf)*1000
                          +(disData>>8 & 0xf)*100
                          +(disData>>4 & 0xf)*10
                          +(disData & 0xf);
                 integer = (disData>>20 & 0xf)*10+(disData>>16 & 0xf);
                 if (sign==1)
                 {
                 	 strcat(sayStr,"-");
                 }
                 strcat(sayStr,floatToString(integer,decimal,4,2,str));
                 if (strlen(sayStr)<=5)
                 {
                   strcpy(str,"");
                   for(tmpCount=0;tmpCount<5-strlen(sayStr);tmpCount++)
                   {
                	   strcat(str," ");
                   }
                   strcat(str, sayStr);
                   strcpy(sayStr,str);
                 }
                 tmpX = 24;
               	 break;
             }
       	   }
       	   else
       	   {
       	      strcpy(sayStr,"-----");
       	      if (tmpX==0)
       	      {
       	      	 tmpX = 24;
       	      }
           }
				}
				else
				{
				   strcpy(sayStr,"-----");
       	   if (tmpX==0)
       	   {
       	   	 tmpX = 24;
       	   }
       	}
				
				guiDisplay(70+tmpX,tmpY,sayStr,1);

				lcdRefresh(tmpY, tmpY+16);
				
				tmpY += 18;
				tmpTime = nextTime(tmpTime, 60, 0);				
		 }
	 }
	 
	 lcdRefresh(17, 160);
}

/*******************************************************
函数名称:meterVisionValue
功能描述:电能表示数
调用函数:
被调用函数:
输入参数:
输出参数:
返回值:void
*******************************************************/
void meterVisionValue(INT8U item)
{
	 INT8U                dataBuff[LENGTH_OF_ENERGY_RECORD];
 	 INT32U               integer,decimal;
   struct cpAddrLink    *tmpLink;
	 char                 str[15];
	 char                 sayStr[10];
	 DATE_TIME            time;
	 INT16U               i;
	 INT8U                tmpY;

 	 menuInLayer = 2;
	 guiLine(1,17,160,160,0);

 	 tmpLink = queryMpLink;
 	 i = 0;
 	 while(tmpLink!=NULL && i<item)
 	 {
 	   tmpLink = tmpLink->next;
 	   i++;
 	 }
 	 
 	 if (tmpLink!=NULL)
 	 {
   	 if (tmpLink->protocol==AC_SAMPLE)
   	 {
   	 	  tmpLink = tmpLink->next;
   	 	  if (tmpLink==NULL)
   	 	  {
   	 	  	 tmpLink = queryMpLink;
   	 	  	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]] = 0;
   	 	  }
   	 	  else
   	 	  {
   	 	  	 layer3MenuLight[layer1MenuLight][layer2MenuLight[layer1MenuLight]]++;
   	 	  }
   	 }
   	 
  	 strcpy(sayStr,"测量点");
  	 strcat(sayStr,digital2ToString(tmpLink->mp,str));
  	 strcat(sayStr,"电能表示数");
  	 guiDisplay(8,LCD_LINE_1,sayStr,0);
     
     //查询本测量点的上次抄表时间
     time = queryCopyTime(tmpLink->mp);
     if (readMeterData(dataBuff, tmpLink->mp, PRESENT_DATA, ENERGY_DATA, &time, 0)==TRUE)
     {
       	tmpY=LCD_LINE_2+8;
       	for(i=0;i<5;i++)
       	{
       	  switch(i)
       	  {
       	  	case 0:
       	      strcpy(sayStr,"正向有功总=");
       	      break;
       	      
       	  	case 1:
       	      strcpy(sayStr,"尖=");
       	      break;

       	  	case 2:
       	      strcpy(sayStr,"峰=");
       	      break;
       	      
       	  	case 3:
       	      strcpy(sayStr,"平=");
       	      break;
       	      
       	  	case 4:
       	      strcpy(sayStr,"谷=");
       	      break;
       	  }
       	  
       	  if (dataBuff[POSITIVE_WORK_OFFSET+i*4]!=0xee)
       	  {
             decimal = bcdToHex(dataBuff[POSITIVE_WORK_OFFSET+i*4]);
             integer = bcdToHex(dataBuff[POSITIVE_WORK_OFFSET+i*4+1] |dataBuff[POSITIVE_WORK_OFFSET+i*4+2]<<8
                     | dataBuff[POSITIVE_WORK_OFFSET+i*4+3]<<16);
             strcpy(str, floatToString(integer,decimal,2,2,str));
             strcat(sayStr,str);
       	  }
       	  
       	  if ((i!=0) && (i%2==0))
       	  {
       	    guiDisplay(81,tmpY,sayStr,1);
       	  }
       	  else
       	  {
       	    guiDisplay(1,tmpY,sayStr,1);
       	  }
       	  
       	  if (i==0 || i==2)
       	  {
       	  	 tmpY += 16;
       	  }
       	}
        
        tmpY+=24;
        strcpy(sayStr,"正向无功总=");       	  
       	if (dataBuff[POSITIVE_NO_WORK_OFFSET]!=0xee)
       	{
           decimal = bcdToHex(dataBuff[POSITIVE_NO_WORK_OFFSET]);
           integer = bcdToHex(dataBuff[POSITIVE_NO_WORK_OFFSET+1] |dataBuff[POSITIVE_NO_WORK_OFFSET+2]<<8
                     | dataBuff[POSITIVE_NO_WORK_OFFSET+3]<<16);
           strcpy(str, floatToString(integer,decimal,2,2,str));
           strcat(sayStr,str);
       	}
       	guiDisplay(1,tmpY,sayStr,1);

        tmpY+=16;
        strcpy(sayStr,"反向无功总=");
       	if (dataBuff[NEGTIVE_NO_WORK_OFFSET]!=0xee)
       	{
           decimal = bcdToHex(dataBuff[NEGTIVE_NO_WORK_OFFSET]);
           integer = bcdToHex(dataBuff[NEGTIVE_NO_WORK_OFFSET+1] |dataBuff[NEGTIVE_NO_WORK_OFFSET+2]<<8
                     | dataBuff[NEGTIVE_NO_WORK_OFFSET+3]<<16);
           strcpy(str, floatToString(integer,decimal,2,2,str));
           strcat(sayStr,str);
       	}
       	guiDisplay(1,tmpY,sayStr,1);
       	
       	
     }
     else
     {
     	  guiDisplay(1, LCD_LINE_2+8,"正向有功总=",1);
     	  guiDisplay(1, LCD_LINE_3+8,"尖=",1);
     	  guiDisplay(81,LCD_LINE_3+8,"峰=",1);
     	  guiDisplay(1, LCD_LINE_4+8,"平=",1);
     	  guiDisplay(81,LCD_LINE_4+8,"谷=",1);
     	  guiDisplay(1, LCD_LINE_6,"正向无功总=",1);
     	  guiDisplay(1, LCD_LINE_7,"反向无功总=",1);
     }
     
     time = queryCopyTime(tmpLink->mp);
     if (readMeterData(dataBuff, tmpLink->mp, PRESENT_DATA, REQ_REQTIME_DATA, &time, 0)==TRUE)
     {
       	strcpy(sayStr,"正向有功需量=");
       	if (dataBuff[REQ_POSITIVE_WORK_OFFSET]!=0xee)
       	{
           decimal = bcdToHex(dataBuff[REQ_POSITIVE_WORK_OFFSET]|dataBuff[REQ_POSITIVE_WORK_OFFSET+1]<<8);
           integer = bcdToHex(dataBuff[REQ_POSITIVE_WORK_OFFSET+2]);
           strcpy(str, floatToString(integer,decimal,4,4,str));
           strcat(sayStr,str);
       	}
       	guiDisplay(1,LCD_LINE_8,sayStr,1);
	      
	      strcpy(sayStr,"需量时间=");
	      if (dataBuff[REQ_TIME_P_WORK_OFFSET]!=0xee)
	      {
	        strcat(sayStr,digital2ToString(bcdToHex(dataBuff[REQ_TIME_P_WORK_OFFSET+3]),str));
	        strcat(sayStr,"-");
	        strcat(sayStr,digital2ToString(bcdToHex(dataBuff[REQ_TIME_P_WORK_OFFSET+2]),str));
	        strcat(sayStr," ");
	        strcat(sayStr,digital2ToString(bcdToHex(dataBuff[REQ_TIME_P_WORK_OFFSET+1]),str));
	        strcat(sayStr,":");
	        strcat(sayStr,digital2ToString(bcdToHex(dataBuff[REQ_TIME_P_WORK_OFFSET]),str));
	      }
	      guiDisplay(1,LCD_LINE_8+16,sayStr,1);
     }
     else
     {
     	  guiDisplay(1, LCD_LINE_8,"正向有功需量=",1);
     	  guiDisplay(1, LCD_LINE_8+16,"需量时间=",1);
     }     
   }
   else
   {
     guiDisplay(12, LCD_LINE_3,"无测量点配置参数!",1);
   }
   
   lcdRefresh(17,160);
}

/*******************************************************
函数名称:statusOfSwitch
功能描述:当前开关量状态
调用函数:
被调用函数:
输入参数:num
输出参数:
返回值:void
*******************************************************/
void statusOfSwitch(void)
{
	 menuInLayer = 3;
	  
	 guiLine(1,17,160,160,0);

 	 if (statusInput[0]&0x1)
 	 {
 	   if (stOfSwitch&0x1)
 	   {
 	     guiDisplay(30,LCD_LINE_2,"开关量1:合",1);
 	   }
 	   else
 	   {
 	     guiDisplay(30,LCD_LINE_2,"开关量1:分",1);
 	   }
 	 }
 	 else
 	 {
 	 	  guiDisplay(30,LCD_LINE_2,"开关量1:未接入",1);
 	 }
 	 
 	 if (statusInput[0]&0x2)
 	 {
 	 	 if (stOfSwitch&0x2)
 	 	 {
 	 	   guiDisplay(30,LCD_LINE_3,"开关量2:合",1);
 	 	 }
 	 	 else
 	 	 {
 	 	   guiDisplay(30,LCD_LINE_3,"开关量2:分",1);
 	 	 }
 	 }
 	 else
 	 {
 	 	  guiDisplay(30,LCD_LINE_3,"开关量2:未接入",1);
 	 }
 	 
 	 if (statusInput[0]&0x4)
 	 {
 	 	 if (stOfSwitch&0x4)
 	     guiDisplay(30,LCD_LINE_4,"开关量3:合",1);
 	   else
 	     guiDisplay(30,LCD_LINE_4,"开关量3:分",1);
 	 }
 	 else
 	 {
 	 	  guiDisplay(30,LCD_LINE_4,"开关量3:未接入",1);
 	 }
 	 
 	 if (statusInput[0]&0x8)
 	 {
 	   if (stOfSwitch&0x8)
 	   {
 	   	  guiDisplay(30,LCD_LINE_5,"开关量4:合",1);
 	   }
 	   else
 	   {
 	     guiDisplay(30,LCD_LINE_5,"开关量4:分",1);
 	   }
 	 }
 	 else
 	 {
 	 	  guiDisplay(30,LCD_LINE_5,"开关量4:未接入",1);
 	 }
 	 
   if (statusInput[0]&0x10)
	 {
	 	 if (stOfSwitch&0x10)
	 	 {
	     guiDisplay(30,LCD_LINE_6,"开关量5:合",1);
	   }
	   else
	   {
	     guiDisplay(30,LCD_LINE_6,"开关量5:分",1);
	   }
	 }
	 else
	 {
	   guiDisplay(30,LCD_LINE_6,"开关量5:未接入",1);
	 }
	   
	 if (statusInput[0]&0x20)
	 {
 	   if (stOfSwitch&0x20)
 	   {
 	     guiDisplay(30,LCD_LINE_7,"开关量6:合",1);
 	   }
 	   else
 	   {
 	     guiDisplay(30,LCD_LINE_7,"开关量6:分",1);
 	   }
 	 }
	 else
	 {
	   guiDisplay(30,LCD_LINE_7,"开关量6:未接入",1);
	 }
 
	 if (getGateKValue()==1)   //控制型的门控节点
	 {
	   guiDisplay(30,LCD_LINE_8,"门控:合",1);
	 }
	 else
	 {
	   guiDisplay(30,LCD_LINE_8,"门控:分",1);
	 }
	  
	 lcdRefresh(17,160);
}

/*******************************************************
函数名称:controlStatus
功能描述:控制状态(二层菜单3)
调用函数:
被调用函数:
输入参数:num
输出参数:
返回值:void
*******************************************************/
void controlStatus(INT8U num)
{
 	  INT8U i,tmpLine,pn;
 	  char  str[42],str1[15];
	  INT8U tmpUse;

 	  menuInLayer = 2;
	  guiLine(1,17,160,160,0);

	  if(totalAddGroup.numberOfzjz==0)
	  {
	  	 guiDisplay(12,30,"无总加组配置参数!",1);
	  	 lcdRefresh(17,160);
	  	 return;
	  }
	  
	  pn = totalAddGroup.perZjz[num].zjzNo;
	  strcpy(str,"总加组");
   	strcat(str,digital2ToString(pn,str1));
   	guiDisplay(48,LCD_LINE_1,str,0);
   	
   	for(i=0;i<6;i++)
    {
    	 switch(i)
    	 {
    	 	  case 0:
    	 	  	tmpUse = ctrlRunStatus[pn-1].ifUsePrdCtrl;
    	 	  	strcpy(str,"时段控:");
    	 	  	tmpLine = LCD_LINE_2;
    	 	  	break;
    	 	  	
    	 	  case 1:
    	 	  	tmpUse = ctrlRunStatus[pn-1].ifUseWkdCtrl;
    	 	  	strcpy(str,"厂休控:");
    	 	  	tmpLine = LCD_LINE_3;
    	 	  	break;
    	 	  	
    	 	  case 2:
    	 	  	tmpUse = ctrlRunStatus[pn-1].ifUseObsCtrl;
    	 	  	strcpy(str,"报停控:");
    	 	  	tmpLine = LCD_LINE_4;
    	 	  	break;
    	 	  	
    	 	  case 3:
    	 	  	tmpUse = ctrlRunStatus[pn-1].ifUsePwrCtrl;
    	 	  	strcpy(str,"下浮控:");
    	 	  	tmpLine = LCD_LINE_5;
    	 	  	break;
    	 	  	
    	 	  case 4:
    	 	  	tmpUse = ctrlRunStatus[pn-1].ifUseMthCtrl;
    	 	  	strcpy(str,"月电控:");
    	 	  	tmpLine = LCD_LINE_6;
    	 	  	break;
    	 	  	
    	 	  case 5:
    	 	  	tmpUse = ctrlRunStatus[pn-1].ifUseChgCtrl;
    	 	  	strcpy(str,"购电控:");
    	 	  	tmpLine = LCD_LINE_7;
    	 	  	break;    	 	  	
    	 }
     	 
     	 if (tmpUse==CTRL_JUMP_IN)
     	 {
     		  strcat(str,"投入");
     	 }
     	 else
     	 {
     		 if (tmpUse==CTRL_RELEASE || tmpUse==CTRL_RELEASEED)
     		 {
     		 	  strcat(str,"解除");
     		 }
     		 else
     		 {
     		 	  strcat(str,"未控");
     		 }
     	 }
     	 guiDisplay(32,tmpLine,str,1);
    }
    
    guiDisplay(32,LCD_LINE_8,"保电:",1);
   	
   	lcdRefresh(17,160);
}

/*******************************************************
函数名称:periodPara
功能描述:时段功控参数
调用函数:
被调用函数:
输入参数:num(总加组序号)
输出参数:
返回值:void
*******************************************************/
void periodPara(INT8U num)
{
  INT8U   pn;
  char    say[42],str1[15];
  INT8U   periodNum;
	INT16U  periodCtrlLimit;
  INT32U  tmpKw;
  INT16U  tmpWatt;

  menuInLayer = 3;
  guiLine(1,17,160,160,0);

  if(totalAddGroup.numberOfzjz==0)
  {
  	 guiDisplay(12,30,"无总加组配置参数!",1);
  	 lcdRefresh(17,160);
  	 return;
  }
  
  pn = totalAddGroup.perZjz[num].zjzNo;
  strcpy(say,"总加组");
 	strcat(say,digital2ToString(pn,str1));
 	guiDisplay(48,LCD_LINE_1,say,0);
 	
  //if (ctrlRunStatus[pn-1].ifUsePrdCtrl == CTRL_JUMP_IN)
  //{
 	   //投入轮次
 	   strcpy(say,"功控轮次:");
 	   if (powerCtrlRoundFlag[pn-1].flag&0x1)
 	   {
 	   	  strcat(say,"①");
 	   }
 	   if (powerCtrlRoundFlag[pn-1].flag>>1&0x1)
 	   {
 	   	  strcat(say,"②");
 	   }
 	   if (powerCtrlRoundFlag[pn-1].flag>>2&0x1)
 	   {
 	   	  strcat(say,"③");
 	   }
 	   if (powerCtrlRoundFlag[pn-1].flag>>3&0x1)
 	   {
 	   	  strcat(say,"④");
 	   }
 	   guiDisplay(1,LCD_LINE_2,say,1);
 	   
 	   strcpy(say,"投入方案:第");
 	   strcat(say,intToString(periodCtrlConfig[pn-1].ctrlPara.ctrlPeriod+1, 3, str1));
 	   strcat(say,"套方案");
 	   guiDisplay(1,LCD_LINE_3,say,1);
 	   
 	   strcpy(say,"投入时段:");
 	   if (periodCtrlConfig[pn-1].ctrlPara.limitPeriod&0x1)
 	   {
 	   	  strcat(say,"1");
 	   }
 	   if (periodCtrlConfig[pn-1].ctrlPara.limitPeriod>>1&0x1)
 	   {
 	   	  strcat(say,"2");
 	   }
 	   if (periodCtrlConfig[pn-1].ctrlPara.limitPeriod>>2&0x1)
 	   {
 	   	  strcat(say,"3");
 	   }
 	   if (periodCtrlConfig[pn-1].ctrlPara.limitPeriod>>3&0x1)
 	   {
 	   	  strcat(say,"4");
 	   }
 	   if (periodCtrlConfig[pn-1].ctrlPara.limitPeriod>>4&0x1)
 	   {
 	   	  strcat(say,"5");
 	   }
 	   if (periodCtrlConfig[pn-1].ctrlPara.limitPeriod>>5&0x1)
 	   {
 	   	  strcat(say,"6");
 	   }
 	   if (periodCtrlConfig[pn-1].ctrlPara.limitPeriod>>6&0x1)
 	   {
 	   	  strcat(say,"7");
 	   }
 	   if (periodCtrlConfig[pn-1].ctrlPara.limitPeriod>>7&0x1)
 	   {
 	   	  strcat(say,"8");
 	   }
 	   guiDisplay(1,LCD_LINE_4,say,1);
 	   
 	   strcpy(say,"当前时段:");
 	   if ((periodNum=getPowerPeriod(sysTime)) == 0)
 	   {
 	     strcat(say, "获取失败");
 	   }
 	   else
 	   {
 	   	 sprintf(str1, "%d", periodNum);
 	   	 strcat(say, str1);
 	   }
 	   guiDisplay(1,LCD_LINE_5,say,1);

 	   strcpy(say,"当前定值:");
 	   if (getPowerLimit(pn, periodCtrlConfig[pn-1].ctrlPara.ctrlPeriod, periodNum, (INT8U *)&periodCtrlLimit)==FALSE)
 	   {
 	     strcat(say, "获取失败");
 	   }
 	   else
 	   {
       tmpKw = periodCtrlLimit;
       tmpKw = countAlarmLimit((INT8U *)&tmpKw, 0x2, 0x0,&tmpWatt);
       if (tmpKw>9999999 && tmpWatt==0)
       {
          tmpKw/=1000;
          strcat(say,intToString(tmpKw,3,str1));
          strcat(say,"Mw");
       }
       else
       {
          strcat(say,intToString(tmpKw,3,str1));
          if (tmpWatt>0)
          {
             strcat(say,".");
             strcat(say,intToString(tmpWatt,3,str1));
          }
          strcat(say,"Kw");
       }
 	   }

 	   guiDisplay(1,LCD_LINE_6,say,1);

  //}
  //else
  //{
  //	 guiDisplay(1,LCD_LINE_3,"时段功控未投入!",1);
  //}
 	
 	lcdRefresh(17,160);
}

/*******************************************************
函数名称:wkdPara
功能描述:厂休控参数
调用函数:
被调用函数:
输入参数:num(总加组序号)
输出参数:
返回值:void
*******************************************************/
void wkdPara(INT8U num)
{
 	  INT8U   pn;
 	  char    say[42],str1[15];
    INT32U  tmpKw;
    INT16U  tmpWatt;

 	  menuInLayer = 3;
	  guiLine(1,17,160,160,0);

	  if(totalAddGroup.numberOfzjz==0)
	  {
	  	 guiDisplay(12,30,"无总加组配置参数!",1);
	  	 lcdRefresh(17,160);
	  	 return;
	  }
	  
	  pn = totalAddGroup.perZjz[num].zjzNo;
	  strcpy(say,"总加组");
   	strcat(say,digital2ToString(pn,str1));
   	guiDisplay(48,LCD_LINE_1,say,0);
    
	  if (pn > 0 && pn <= 8 && wkdCtrlConfig[pn-1].wkdStartHour != 0xFF)
	  {
   	   //投入轮次
   	   strcpy(say,"轮次");
   	   if (powerCtrlRoundFlag[pn-1].flag&0x1)
   	   {
   	   	  strcat(say,"①");
   	   }
   	   if (powerCtrlRoundFlag[pn-1].flag>>1&0x1)
   	   {
   	   	  strcat(say,"②");
   	   }
   	   if (powerCtrlRoundFlag[pn-1].flag>>2&0x1)
   	   {
   	   	  strcat(say,"③");
   	   }
   	   if (powerCtrlRoundFlag[pn-1].flag>>3&0x1)
   	   {
   	   	  strcat(say,"④");
   	   }
   	   guiDisplay(1,LCD_LINE_2,say,1);
   	   
   	   //定值
   	   strcpy(say,"厂休控定值");
	     tmpKw = wkdCtrlConfig[pn-1].wkdLimit;
	     tmpKw = countAlarmLimit((INT8U *)&tmpKw, 0x2, 0x0,&tmpWatt);
	     if (tmpKw>9999999 && tmpWatt==0)
	     {
	        tmpKw/=1000;
	        strcat(say,intToString(tmpKw,3,str1));
          strcat(say,"Mw");
	     }
	     else
	     {
	        strcat(say,intToString(tmpKw,3,str1));
	        if (tmpWatt>0)
	        {
	           strcat(say,".");
	           strcat(say,intToString(tmpWatt,3,str1));
	        }
          strcat(say,"Kw");
	     }
	     guiDisplay(1,LCD_LINE_3,say,1);
   	   
   	   strcpy(say,"起始:");
   	   strcat(say,digital2ToString((wkdCtrlConfig[pn-1].wkdStartHour&0xf)+(wkdCtrlConfig[pn-1].wkdStartHour>>4&0xf)*10,str1));
   	   strcat(say,":");
   	   strcat(say,digital2ToString((wkdCtrlConfig[pn-1].wkdStartMin&0xf)+(wkdCtrlConfig[pn-1].wkdStartMin>>4&0xf)*10,str1));
	     guiDisplay(1,LCD_LINE_4,say,1);
   	   
   	   strcpy(say,"延续:");
   	   strcat(say,intToString(wkdCtrlConfig[pn-1].wkdTime/2,3,str1));
   	   if (wkdCtrlConfig[pn-1].wkdTime%2)
   	   {
   	     strcat(say,".5");
   	   }
   	   strcat(say,"小时");
	     guiDisplay(1,LCD_LINE_5,say,1);
	     
	     strcpy(say,"厂休日:");
	     guiDisplay(1,LCD_LINE_6,say,1);
	     
	     strcpy(say,"");
	     if (wkdCtrlConfig[pn-1].wkdDate>>1&0x1)
	     {
	     	  strcat(say,"一");
	     }
	     if (wkdCtrlConfig[pn-1].wkdDate>>2&0x1)
	     {
	     	  strcat(say,"二");
	     }
	     if (wkdCtrlConfig[pn-1].wkdDate>>3&0x1)
	     {
	     	  strcat(say,"三");
	     }
	     if (wkdCtrlConfig[pn-1].wkdDate>>4&0x1)
	     {
	     	  strcat(say,"四");
	     }
	     if (wkdCtrlConfig[pn-1].wkdDate>>5&0x1)
	     {
	     	  strcat(say,"五");
	     }
	     if (wkdCtrlConfig[pn-1].wkdDate>>6&0x1)
	     {
	     	  strcat(say,"六");
	     }
	     if (wkdCtrlConfig[pn-1].wkdDate>>7&0x1)
	     {
	     	  strcat(say,"日");
	     }
	     guiDisplay(49,LCD_LINE_7,say,1);
   	}
   	else
   	{
   		 guiDisplay(33,LCD_LINE_3,"未配置厂休控参数",1);
   	}
   	
   	lcdRefresh(17,160);
}

/*******************************************************
函数名称:powerDownPara
功能描述:当前功率下浮控参数
调用函数:
被调用函数:
输入参数:num(总加组序号)
输出参数:
返回值:void
*******************************************************/
void powerDownPara(INT8U num)
{
 	  INT8U   pn;
 	  char    say[42],str1[15];
    INT32U  tmpKw;
    INT16U  tmpWatt;

 	  menuInLayer = 3;
	  guiLine(1,17,160,160,0);

	  if(totalAddGroup.numberOfzjz==0)
	  {
	  	 guiDisplay(12,30,"无总加组配置参数!",1);
	  	 lcdRefresh(17,160);
	  	 return;
	  }
	  
	  pn = totalAddGroup.perZjz[num].zjzNo;
	  strcpy(say,"总加组");
   	strcat(say,digital2ToString(pn,str1));
   	guiDisplay(48,LCD_LINE_1,say,0);
   	
    //if (ctrlRunStatus[pn-1].ifUsePwrCtrl == CTRL_JUMP_IN)
    //{
   	   //投入轮次
   	   strcpy(say,"轮次");
   	   if (powerCtrlRoundFlag[pn-1].flag&0x1)
   	   {
   	   	  strcat(say,"①");
   	   }
   	   if (powerCtrlRoundFlag[pn-1].flag>>1&0x1)
   	   {
   	   	  strcat(say,"②");
   	   }
   	   if (powerCtrlRoundFlag[pn-1].flag>>2&0x1)
   	   {
   	   	  strcat(say,"③");
   	   }
   	   if (powerCtrlRoundFlag[pn-1].flag>>3&0x1)
   	   {
   	   	  strcat(say,"④");
   	   }
   	   guiDisplay(1,LCD_LINE_2,say,1);
   	   
   	   strcpy(say,"定值:");
   	   if (powerDownCtrl[pn-1].freezeTime.year!=0xff)
   	   {
   	   	  strcat(say,"等待计算!");
   	   }
   	   else
   	   {
   	   	  tmpKw = powerDownCtrl[pn-1].powerDownLimit;
   	   	  tmpWatt = powerDownCtrl[pn-1].powerLimitWatt;
    	    if (tmpKw>9999999 && tmpWatt==0)
    	    {
    	        tmpKw/=1000;
    	        strcat(say,intToString(tmpKw,3,str1));
              strcat(say,"Mw");
    	    }
    	    else
    	    {
    	        strcat(say,intToString(tmpKw,3,str1));
    	        if (tmpWatt>0)
    	        {
    	           strcat(say,".");
    	           strcat(say,intToString(tmpWatt,3,str1));
    	        }
              strcat(say,"Kw");
    	    }
   	   }
   	   guiDisplay(1,LCD_LINE_3,say,1);
   	   
   	   strcpy(say,"下浮系数:");
 	     if (powerDownCtrl[pn-1].floatFactor&0x80)
 	     {
 	     	  strcat(say,"下浮");
 	     }
 	     else
 	     {
 	     	  strcat(say,"上浮");
 	     }
 	     strcat(say,intToString((powerDownCtrl[pn-1].floatFactor&0xF) + (powerDownCtrl[pn-1].floatFactor>>4&0x7)*10,3,str1));
 	     strcat(say,"%");
   	   guiDisplay(1,LCD_LINE_4,say,1);

   	   strcpy(say,"冻结时延:");
   	   strcat(say,intToString(powerDownCtrl[pn-1].freezeDelay,3,str1));
   	   strcat(say,"分");
   	   guiDisplay(1,LCD_LINE_5,say,1);

   	   strcpy(say,"控制时间:");
   	   strcat(say,intToString(powerDownCtrl[pn-1].downCtrlTime,3,str1));
   	   strcat(say,"*0.5小时");
   	   guiDisplay(1,LCD_LINE_6,say,1);
   	   
   	   strcpy(say,"告警时间:");
   	   guiDisplay(1,LCD_LINE_7,say,1);
   	   strcpy(say,"第1轮:");
   	   strcat(say,intToString(powerDownCtrl[pn-1].roundAlarmTime[0],3,str1));
   	   strcat(say,"分");
   	   guiDisplay(1,LCD_LINE_8,say,1);
   	   strcpy(say,"第2轮:");
   	   strcat(say,intToString(powerDownCtrl[pn-1].roundAlarmTime[1],3,str1));
   	   strcat(say,"分");
   	   guiDisplay(81,LCD_LINE_8,say,1);
   	   strcpy(say,"第3轮:");
   	   strcat(say,intToString(powerDownCtrl[pn-1].roundAlarmTime[2],3,str1));
   	   strcat(say,"分");
   	   guiDisplay(1,LCD_LINE_9,say,1);
   	   strcpy(say,"第4轮:");
   	   strcat(say,intToString(powerDownCtrl[pn-1].roundAlarmTime[3],3,str1));
   	   strcat(say,"分");
   	   guiDisplay(81,LCD_LINE_9,say,1);   	   
    //}
    //else
    //{
    //	 guiDisplay(1,LCD_LINE_3,"功率下浮控未投入!",1);
    //}
   	
   	lcdRefresh(17,160);
}

/*******************************************************
函数名称:meterPara
功能描述:电能表参数信息(三层菜单2.5)
调用函数:
被调用函数:
输入参数:num
输出参数:
返回值:void
*******************************************************/
void meterPara(struct cpAddrLink *mpLink)
{
 	  INT8U j,ifFound,noneZero;
 	  char  str[42],str1[5];

 	  menuInLayer = 3;
	  guiLine(1,17,160,160,0);
	  
	  if(mpLink==NULL)
	  {
	  	 guiDisplay(12,30,"无测量点配置参数!",1);
	  	 lcdRefresh(17,160);
	  	 return;
	  }
	  
	  strcpy(str,"测量点");
	  strcat(str,intToString(mpLink->mp,3,str1));
	  guiDisplay(48,LCD_LINE_1,str,0);
	  	  
	  strcpy(str,"序号:");
	  strcat(str,intToString(mpLink->mpNo,3,str1));
	  guiDisplay(1,LCD_LINE_3,str,1);

	  strcpy(str,"端口:");
	  strcat(str,intToString(mpLink->port,3,str1));
	  guiDisplay(1,LCD_LINE_4,str,1);
	  	  
	  strcpy(str,"规约:");
	  switch(mpLink->protocol)
	  {
	     case 1:   //DL/T645-1997
	     	 strcat(str,"DL/T645-1997");
	     	 break;

	     case 2:   //交采
	     	 strcat(str,"交流采样装置");
	     	 break;
	     
	     case SIMENS_ZD_METER:
	     case SIMENS_ZB_METER:
	     	 strcat(str,"兰尔尔ZD/ZB表");
	     	 break;

	     case ABB_METER:
	     	 strcat(str,"ABB方表");
	     	 break;

	     case EDMI_METER:
	     	 strcat(str,"红相EDMI表");
	     	 break;
	     	 
	     case 30:   //DL/T645-207
	     	 strcat(str,"DL/T645-2007");
	     	 break;
			 
	     case 57:   //DL/T645-207
	     	 strcat(str,"明武多功能表");
	     	 break;
			 
	     case 58:   //DL/T645-207
	     	 strcat(str,"明武UI表");
	     	 break;
	  }
	  guiDisplay(1,LCD_LINE_5,str,1);
	  	  
	  strcpy(str,"表地址:");
	  noneZero = 0;
	  for(j=6;j>0;j--)
	  {
	  	 if(mpLink->addr[j-1]!=0x00 || noneZero !=0)
	  	 {
	  	 	  noneZero++;
	  	 	  strcat(str,digital2ToString((mpLink->addr[j-1]/0x10)*10+mpLink->addr[j-1]%0x10,str1));
	  	 }
	  }
	  guiDisplay(1,LCD_LINE_6,str,1);
	  
	  lcdRefresh(17,160);
}

/*******************************************************
函数名称:kvkikp
功能描述:KvKiKp(三层菜单2.4)
调用函数:
被调用函数:
输入参数:num
输出参数:
返回值:void
*******************************************************/
void kvkikp(struct cpAddrLink *mpLink)
{
 	  char  str[42],str1[5];
	  MEASURE_POINT_PARA pointPara;

 	  menuInLayer = 3;
	  guiLine(1,17,160,160,0);
	  
	  if(mpLink==NULL)
	  {
	  	 guiDisplay(12,30,"无测量点配置参数!",1);
	  	 lcdRefresh(17,160);
	  	 return;
	  }
	  
	  strcpy(str,"测量点");
	  strcat(str,intToString(mpLink->mp,3,str1));
	  guiDisplay(48,LCD_LINE_1,str,0);
	  
	  if(selectViceParameter(0x04, 25, mpLink->mp, (INT8U *)&pointPara, sizeof(MEASURE_POINT_PARA)) == TRUE)
	  {
	  	strcpy(str,"Kv=");
	  	strcat(str,intToString(pointPara.voltageTimes,3,str1));
	  	guiDisplay(40,LCD_LINE_3,str,1);

	  	strcpy(str,"Ki=");
	  	strcat(str,intToString(pointPara.currentTimes,3,str1));
	  	guiDisplay(40,LCD_LINE_4,str,1);
	  	
	  	strcpy(str,"Kp=");
	  	strcat(str,intToString(pointPara.voltageTimes*pointPara.currentTimes,3,str1));
	  	guiDisplay(40,LCD_LINE_5,str,1);
	  }
	  else
	  {
	  	guiDisplay(40,LCD_LINE_3,"Kv未配置",1);
	  	guiDisplay(40,LCD_LINE_4,"Ki未配置",1);
	  	guiDisplay(40,LCD_LINE_5,"Kp未配置",1);
	  }
	  
	  lcdRefresh(17,160);
}

/*******************************************************
函数名称:configPara
功能描述:配置参数(三层菜单2.6)
调用函数:
被调用函数:
输入参数:num
输出参数:
返回值:void
*******************************************************/
void configPara(INT8U rowLight,INT8U colLight)
{	 	   
 	  char  str[40],str1[20];
 	  INT8U tmpX;
 	  INT8U i;

	  guiLine(1,17,160,160,0);

	  if (rowLight==88 && colLight==88)
	  { 
 	    menuInLayer = 3;
  	  
  	  //行政区划
  	  guiDisplay(1,LCD_LINE_1,"行政区划:",1);
      strcpy(str,digitalToChar(addrField.a1[1]>>4));
      strcat(str,digitalToChar(addrField.a1[1]&0xf));
      strcat(str,digitalToChar(addrField.a1[0]>>4));
      strcat(str,digitalToChar(addrField.a1[0]&0xf));
  	  guiDisplay(73,LCD_LINE_1,str,1);
  	  
  	  guiDisplay(128,LCD_LINE_1,"修改",0);	  
  
      strcpy(str, "终端地址:");
      #ifdef TE_ADDR_USE_BCD_CODE
       strcat(str,int8uToHex(addrField.a2[1],str1));
       strcat(str,int8uToHex(addrField.a2[0],str1));
      #else
       strcat(str,intToString(addrField.a2[1]<<8 | addrField.a2[0],3,str1));
       strcat(str,"[");
       strcat(str,int8uToHex(addrField.a2[1],str1));
       strcat(str,int8uToHex(addrField.a2[0],str1));
       strcat(str,"]");
      #endif
      guiDisplay(1,LCD_LINE_2,str,1);
  	   
  	  //APN域名
  	  guiDisplay(1,  LCD_LINE_3, "APN域名:",1);
  	  guiDisplay(68, LCD_LINE_3, (char *)ipAndPort.apn, 1);
  	   
  	  guiDisplay(1,LCD_LINE_4,"主站IP及接入端口:",1);
   	  strcpy(str,intToIpadd(ipAndPort.ipAddr[0]<<24 | ipAndPort.ipAddr[1]<<16 | ipAndPort.ipAddr[2]<<8 | ipAndPort.ipAddr[3],str1));
      strcat(str,":");
  	  guiDisplay(1,LCD_LINE_5,str,1);
  	  tmpX = 1+8*strlen(str);
      strcpy(str,intToString(ipAndPort.port[1]<<8 | ipAndPort.port[0],3,str1));
  	  guiDisplay(tmpX,LCD_LINE_5,str,1);
  
  	  guiDisplay(1,LCD_LINE_6,"备IP及接入端口:",1);
   	  strcpy(str,intToIpadd(ipAndPort.ipAddrBak[0]<<24 | ipAndPort.ipAddrBak[1]<<16 | ipAndPort.ipAddrBak[2]<<8 | ipAndPort.ipAddrBak[3],str1));
      strcat(str,":");
  	  guiDisplay(1,LCD_LINE_7,str,1);
  	  tmpX = 1+8*strlen(str);
      strcpy(str,intToString(ipAndPort.portBak[1]<<8 | ipAndPort.portBak[0],3,str1));
  	  guiDisplay(tmpX,LCD_LINE_7,str,1);
  	   
  	  guiDisplay(1,LCD_LINE_8,"终端IP:",1);
  	  guiDisplay(1,LCD_LINE_8+16,intToIpadd(wlLocalIpAddr,str1),1);
	  }
	  else   //修改配置参数
	  {
 	    menuInLayer = 4;
  	  guiDisplay(1,LCD_LINE_1,"行政区划:",1);
      tmpX = 73;
      for(i=0;i<4;i++)
      {
        str[0] = commParaItem[0][i];
        str[1] = '\0';
        if (rowLight==0 && i==colLight)
        {
          guiDisplay(tmpX,17,str,0);
        }
        else
        {
          guiDisplay(tmpX,17,str,1);
        }
        tmpX += 8;
      }
      
      guiDisplay(1, LCD_LINE_2, "终端地址:", 1);
      tmpX = 73;      
      #ifdef TE_ADDR_USE_BCD_CODE
       for(i=0;i<4;i++)
      #else
       for(i=0;i<5;i++)
      #endif
       {
         str[0] = commParaItem[1][i];
         str[1] = '\0';
         if (rowLight==1 && i==colLight)
         {
           guiDisplay(tmpX,33,str,0);
         }
         else
         {
           guiDisplay(tmpX,33,str,1);
         }
         tmpX += 8;
       }
       
       //APN域名
       guiDisplay(1,  49, "APN",1);
       if (rowLight==2)
       {
         guiDisplay(32, 49, commParaItem[2], 0);
       }
       else
       {
         guiDisplay(32, 49, commParaItem[2], 1);
       }
     
       guiDisplay(1,65,"主站IP及接入端口",1);     
       tmpX = 1;
       for(i=0;i<21;i++)
       {
         str[0] = commParaItem[3][i];
         str[1] = '\0';
         
         if (rowLight==3 && colLight==i)
         {
           guiDisplay(tmpX,81,str,0);
           tmpX += 8;
         }
         else
         {
           guiDisplay(tmpX,81,str,1);
           tmpX += 7;
         }
       }
  
       guiDisplay(1,97,"备用IP及接入端口",1);
       tmpX = 1;
       for(i=0;i<21;i++)
       {
         str[0] = commParaItem[4][i];
         str[1] = '\0';
         if (rowLight==4 && colLight==i)
         {
           guiDisplay(tmpX,113,str,0);
           tmpX += 8;
         }
         else
         {
           guiDisplay(tmpX,113,str,1);
           tmpX += 7;
         }
       }
       
       guiLine( 60, 133,  60, 152, 1);
       guiLine(100, 133, 100, 152, 1);
       guiLine( 60, 133, 100, 133, 1);
       guiLine( 60, 152, 100, 152, 1);       
       if (rowLight==5)
       {
         guiDisplay(64,135,"确定",0);     
       }
       else
       {
         guiDisplay(64,135,"确定",1);
       }
    }
	  lcdRefresh(17,160);
}

/*******************************************************
函数名称:chinese
功能描述:中文信息(三层菜单3.2)
调用函数:
被调用函数:
输入参数:num
输出参数:
返回值:void
*******************************************************/
void chinese(INT8U num, INT8U numOfPage)
{
   char  display[35],str[5];
   INT8U ifTail,i,j;
   
   layer1MenuLight = 4;
   menuInLayer = 2;
   
   guiLine(1,17,160,160,0);

   messageTip(0);
   
   if (chnMessage.numOfMessage==0)
   {
    	guiDisplay(32,30,"无中文信息!",1);
  	  lcdRefresh(17,160);
    	return;
   }
   
   if ((chnMessage.message[num].typeAndNum>>4)==1)
   {
   	 strcpy(display,"重要");
   }
   else
   {
   	 strcpy(display,"普通");
   }
   strcat(display,"信息 ");
	 strcat(display,"页");
	 strcat(display,intToString(numOfPage+1,3,str));
	 strcat(display," ");	 
	 strcat(display,intToString(num+1,3,str));
	 strcat(display,"/");
	 strcat(display,intToString(chnMessage.numOfMessage,3,str));
	 guiDisplay(10,LCD_LINE_1,display,1);
   guiLine(1,32,160,32,1);
   ifTail = 1;
   for(i=0;i<8;i++)
   {
      for(j=0;j<20;j++)
      {
      	 if (chnMessage.message[num].chn[numOfPage*160+j+i*20]=='\0')
      	 {
      	 	  ifTail=2;
      	 	  break;
      	 }
      	 else
      	 {
      	    display[j] = chnMessage.message[num].chn[numOfPage*160+j+i*20];
      	 }
      }
      if (j>0)
      {
         display[j] = '\0';
         guiDisplay(1,33+i*16,display,1);
      }
      
      if (ifTail==2)
      {
      	break;
      }
   }
	 lcdRefresh(17,160); 
}

/*******************************************************
函数名称:chargeInfo
功能描述:购电信息(二层菜单2.6)
调用函数:
被调用函数:
输入参数:num(总加组序号)
输出参数:
返回值:void
*******************************************************/
void chargeInfo(INT8U num)
{
 	  INT8U     pn;
 	  char      say[42],str1[15];
    INT32U    tmpKw;
    INT16U    tmpWatt;
    INT8U     dataBuff[512];
    DATE_TIME readTime;

 	  menuInLayer = 2;
	  guiLine(1,17,160,160,0);

	  if(totalAddGroup.numberOfzjz==0)
	  {
	  	 guiDisplay(12,LCD_LINE_3,"无总加组配置参数!",1);
	  	 lcdRefresh(17,160);
	  	 return;
	  }
	  
	  pn = totalAddGroup.perZjz[num].zjzNo;
	  strcpy(say,"总加组");
   	strcat(say,digital2ToString(pn,str1));
   	strcat(say,"购电信息");
   	guiDisplay(16,LCD_LINE_1,say,0);
   	
	  if (pn > 0 && pn <= 8 && (chargeCtrlConfig[pn-1].flag == 0x55 || chargeCtrlConfig[pn-1].flag == 0xAA))
	  {
   	   //购电单号
   	   strcpy(say,"购电单号:");
   	   strcat(say,intToString(chargeCtrlConfig[pn-1].numOfBill,3,str1));
   	   guiDisplay(1,LCD_LINE_3,say,1);
   	   
   	   //报警门限
   	   strcpy(say,"报警门限:");
	     tmpKw = chargeCtrlConfig[pn-1].alarmLimit;
    	 tmpKw = countAlarmLimit((INT8U *)&tmpKw, 0x3, 0x0,&tmpWatt);
    	 
    	 if (chargeCtrlConfig[pn-1].alarmLimit&0x10000000)
    	 {
    	 	 strcat(say, "-");
    	 }
    	 
 	     if (tmpKw>999999)
 	     {
 	        tmpKw/=1000;
 	        strcat(say,intToString(tmpKw,3,str1));
          strcat(say,"Mwh");
 	     }
 	     else
 	     {
 	        strcat(say,intToString(tmpKw,3,str1));
          strcat(say,"Kwh");
 	     }
   	   guiDisplay(1,LCD_LINE_4,say,1);
 	     
 	     
   	   //跳闸门限
   	   strcpy(say,"跳闸门限:");
	     tmpKw = chargeCtrlConfig[pn-1].cutDownLimit;
    	 tmpKw = countAlarmLimit((INT8U *)&tmpKw, 0x3, 0x0,&tmpWatt);
    	 if (chargeCtrlConfig[pn-1].cutDownLimit&0x10000000)
    	 {
    	 	 strcat(say, "-");
    	 }
 	     if (tmpKw>999999)
 	     {
 	        tmpKw/=1000;
 	        strcat(say,intToString(tmpKw,3,str1));
          strcat(say,"Mwh");
 	     }
 	     else
 	     {
 	        strcat(say,intToString(tmpKw,3,str1));
          strcat(say,"Kwh");
 	     }
   	   guiDisplay(1,LCD_LINE_5,say,1);
   	   
       strcpy(say,"剩余电量:");
       readTime = timeHexToBcd(sysTime);
       if (readMeterData(dataBuff, pn, LEFT_POWER, 0x0, &readTime, 0)==TRUE)
       {
          if (dataBuff[0] != 0xFF || dataBuff[0] != 0xEE)
          {
                //当前剩余电量
                tmpKw = dataBuff[0] 
                      | dataBuff[1] << 8 
                      | dataBuff[2] << 16 
                      | dataBuff[3] << 24;
          }
    	    tmpKw = countAlarmLimit((INT8U *)&tmpKw, 0x3, 0x0,&tmpWatt);     	    
     	    if (dataBuff[3]&0x10)
    	    {
    	   	  strcat(say, "-");
    	    }
 	        strcat(say,intToString(tmpKw,3,str1));
          strcat(say,"Kwh");
 	     }
   	   guiDisplay(1,LCD_LINE_6,say,1);
   	}
   	else
   	{
   		 guiDisplay(1,LCD_LINE_3,"未配置购电控参数!",1);
   	}
   	
   	lcdRefresh(17,160);
}

#endif    //PLUG_IN_CARRIER_MODULE

