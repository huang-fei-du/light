/***************************************************
Copyright,2009-2010,All	Rights Reserved
文件名:parallelBus.c
作者:leiyong
版本:0.9
完成日期:2009年12月
描述:并行总线(LCD,uc1698/lcd12864,ra8835驱动芯片)驱动文件
函数列表:

修改历史:
  01,09-11-20,Leiyong created.
  02,09-12-17,Leiyong规范化文件格式.
  03,10-05-14,Leiyong合并uc1698及lcd12864驱动.
  04,10-07-11,leiyong增加320*240(RA8835控制器)驱动.
***************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <asm/arch/gpio.h>
#include <asm/arch/at91sam9260.h>
#include <asm/arch/at91sam926x_mc.h>
#include <asm/arch/at91sam9260_matrix.h>

/*调试信息*/
//#define DEBUG_LCD_INFO

/***************************KS1008 Start********************************************/
//128*64屏KS1008控制器定义
#define SET_X_ADDRESS        0xB8
#define SET_Y_ADDRESS        0x40
#define SET_Z_ADDRESS        0xC0
/***************************KS1008 End ********************************************/


/***************************UC1698 Start********************************************/
//#define LCD_DEGREE_DEFAULT   0xc2
#define LCD_DEGREE_DEFAULT   0xe0      //UC1698默认对比度
/***************************UC1698 end ********************************************/


/***************************RA8835 Start********************************************/
//320*240屏RA8835控制器定义

//1.RA8835的命令集
#define SYSTEM_SET           0x40
#define SCROLL               0x44
#define SLEEP_IN             0x53
#define DISP_ON              0x59
#define DISP_OFF             0x58
#define OVLAY                0x5b
#define HDOT_SCR             0x5a
#define CSRFORM              0x5d
#define CGRAM_ADR            0x5c
#define CSR_RIGHT            0x4c
#define CSR_LEFT             0x4d
#define CSR_UP               0x4e
#define CSR_DOWN             0x4f
#define CSRW                 0x46
#define CSRR                 0x47
#define MWRITE               0x42
#define MREAD                0x43

#define AP                     40

//2.屏幕大小定义
#define SCREEN_WIDTH         320       //320 pixels
#define SCREEN_HIGHT         240       //240 pixels


//3.初始化所用到的参数
#define GRAPHICS_BASE_ADDR  0x1000  // 该地址位于SED1335所使用的32kB SRAM中

static const unsigned char SYSTAB[8] = {
    0x30,       // 0011 0000    B: 1, W/S: 0, M2: 0, M1: 0, M0: 0
    0x87,       // 1000 0111    WF: 1, FX: 7
    0x0f,       // 0000 1111    FY: 15
    0x27,       // 0010 0111    C/R: 39
    0x42,       // 0100 0010    TC/R: 66
    0xf0,       // 1111 0000    L/F: 240
    0x28,       // 0010 1000    APL: 40
    0x00        // 0000 0000    APH: 0
};

static const unsigned char SCRTAB[10] = {
    0x00,       // 0000 0000    SAD1L:  0x00
    0x00,       // 0000 0000    SAD1H:  0x00
    0xf0,       // 1111 0000    SL1:    240
    (GRAPHICS_BASE_ADDR & 0xff),  // 0000 0000    SAD2L:  0x00
    (GRAPHICS_BASE_ADDR >> 8),    // 0001 0000    SAD2H:  0x10
    0xf0,       // 1111 0000    SL2:    240
    0x00,       // 0000 0000    SAD3L:  0x00
    0x04,       // 1000 0000    SAD3H:  0x40
    0x00,       // 0000 0000    SAD4L:  0x00
    0x50        // 0101 0000    SAD4H:  0x50
};
/***************************RA8835 End ********************************************/

/*LCD控制(操作)基地址*/
#define LCD_CTRL_BASE_ADDR AT91_CHIPSELECT_0

/*结构 - 总线配置*/
typedef struct
{
	 u8	busWidth16;        //总线宽度是否为16位
	 u8 cs;                //片选信号
}BUS_LCD_CONFIG;

/*结构 - LCD设备*/
typedef struct
{
   struct cdev lcdCdev;
   char data;
}DEVICE_LCD;

/*类 - LCD类*/
struct class *classLcd;

/*变量 - 操作UC1698的命令地址及数据地址*/
static void __iomem *uc1698CmdAddr,*uc1698DataAddr;

/*变量 - 操作lcd12864的命令地址及数据地址*/
static void __iomem *lcd12864CmdWrAddr1,*lcd12864DataWrAddr1,*lcd12864CmdReadAddr1,*lcd12864DataReadAddr1;
static void __iomem *lcd12864CmdWrAddr2,*lcd12864DataWrAddr2,*lcd12864CmdReadAddr2,*lcd12864DataReadAddr2;

/*变量 - 操作RA8835的命令地址及数据地址*/
static void __iomem *ra8835CmdAddr,*ra8835DataAddr;

/*静态变量 - lcd的主设备号*/
static int parallelBusMajor;

/*静态变量 - 总线(操作)配置*/
static BUS_LCD_CONFIG busLcdConfig = 
{
	.busWidth16	 = 0,
	.cs          = 0,
};


/*操作LCD的资源指定*/
static struct resource parallelLcdResources[] = 
{
	{
		.start = (LCD_CTRL_BASE_ADDR),
		.end	 = (LCD_CTRL_BASE_ADDR+0x1f),
		.flags = IORESOURCE_MEM,
	}
};

/*平台设备 - LCD*/
static struct platform_device at91sam9260_lcd_device = 
{
	.name	= "parallelBus",
	.id		= -1,
	.dev	=
	      {
				  .platform_data	= &busLcdConfig,
	      },
	.resource	= parallelLcdResources,
	.num_resources	= ARRAY_SIZE(parallelLcdResources),
};

/*结构变量 - LCD设备*/
static DEVICE_LCD *parallelLcdDev;

unsigned char lcdType=0;                   //LCD显示器类型

/****************************************uc1698私有函数******************************************/
/**************************************************
函数名称:initUc1698
功能描述:
调用函数:
被调用函数:
输入参数:初始化uc1698
输出参数:
返回值：
***************************************************/
void initUc1698(void)
{
	unsigned long mode;  //csa, mode;
	int           i, j;
	
	 //这几句是为了适应9260-912模块而调试的 ly,2010-06-25
	 /*设置总线接口特性(set the bus interface characteristics)*/	 
	 //at91_sys_write(AT91_SMC_SETUP(busLcdConfig.cs), AT91_SMC_NWESETUP_(0) | AT91_SMC_NCS_WRSETUP_(0) | AT91_SMC_NRDSETUP_(0) | AT91_SMC_NCS_RDSETUP_(0));
	 //ly,2010-08-11,集中器v0.4控制板,改成以下这句,其它没有动
	 at91_sys_write(AT91_SMC_SETUP(busLcdConfig.cs), AT91_SMC_NWESETUP_(2) | AT91_SMC_NCS_WRSETUP_(2) | AT91_SMC_NRDSETUP_(2) | AT91_SMC_NCS_RDSETUP_(2));

	 at91_sys_write(AT91_SMC_PULSE(busLcdConfig.cs), AT91_SMC_NWEPULSE_(4) | AT91_SMC_NCS_WRPULSE_(4) | AT91_SMC_NRDPULSE_(4) | AT91_SMC_NCS_RDPULSE_(4));
	 
	 at91_sys_write(AT91_SMC_CYCLE(busLcdConfig.cs), AT91_SMC_NWECYCLE_(5) | AT91_SMC_NRDCYCLE_(5));

	 if (busLcdConfig.busWidth16)
	 {
	 	  mode = AT91_SMC_DBW_16;
	 }
	 else
	 {
	 	  mode = AT91_SMC_DBW_8;
	 }
	 at91_sys_write(AT91_SMC_MODE(busLcdConfig.cs), mode | AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_TDF_(2));
	
	 //这几句是为了适应迈冲模块而调试的 ly,2010-06-25
	/*设置总线接口特性(set the bus interface characteristics)*/
	//at91_sys_write(AT91_SMC_SETUP(busLcdConfig.cs), AT91_SMC_NWESETUP_(0) | AT91_SMC_NCS_WRSETUP_(0) | AT91_SMC_NRDSETUP_(0) | AT91_SMC_NCS_RDSETUP_(0));

	//at91_sys_write(AT91_SMC_PULSE(busLcdConfig.cs), AT91_SMC_NWEPULSE_(3) | AT91_SMC_NCS_WRPULSE_(3) | AT91_SMC_NRDPULSE_(3) | AT91_SMC_NCS_RDPULSE_(3));

	//at91_sys_write(AT91_SMC_CYCLE(busLcdConfig.cs), AT91_SMC_NWECYCLE_(5) | AT91_SMC_NRDCYCLE_(5));

	//if (busLcdConfig.busWidth16)
	//{
	//	 mode = AT91_SMC_DBW_16;
	//}
	//else
	//{
	//  mode = AT91_SMC_DBW_8;
	//}
	//at91_sys_write(AT91_SMC_MODE(busLcdConfig.cs), mode | AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_TDF_(2));

	writeb(0xE2, uc1698CmdAddr);     /*系统软件复位(System Reset)*/
	
	//复位延时处理
	for (i=0;i<500;i++)
	{
		for (j=0;j<1000;j++)
		{
			;
		}
	}	
	
	writeb(0xAE, uc1698CmdAddr);     /*设置显示显示控制(Set Display Enable)[显示关闭]*/

	writeb(0xE9, uc1698CmdAddr);     /*Set LCD Bias ratio:1/11*/
	writeb(0xA3, uc1698CmdAddr);     /*Set Line Rate 3-15.2Kips,默认值为2*/

	writeb(0x81, uc1698CmdAddr);     /*Set gain and potentiometer Mode*/
	writeb(LCD_DEGREE_DEFAULT, uc1698CmdAddr);     /*PM值设置为0xc2,调节对比度就调节这个值*/
	
	//writeb(0x20, uc1698CmdAddr);     /*Set Temperature Compensation (00-0.25%)*/
	//writeb(0x2A, uc1698CmdAddr);     /*Set Power Control (Interal VLCD;Panel loading definition<13nF)*/
	
	writeb(0x89, uc1698CmdAddr);     /*Set RAM Address Control*/
	writeb(0xC4, uc1698CmdAddr);     /*Set LCD Maping Control (MY=1, MX=0)*/
	
	writeb(0x84, uc1698CmdAddr);     /*Set Partial Display Off*/
	
	writeb(0xD6, uc1698CmdAddr);     /*Set Color Mode (64K)*/
	
	writeb(0xD1, uc1698CmdAddr);     /*Set Color Pattern (RGB)*/
	writeb(0xDE, uc1698CmdAddr);     /*Set COM Scan Function*/

	writeb(0xF4, uc1698CmdAddr);
	writeb(0x25, uc1698CmdAddr);
	
	writeb(0xF5, uc1698CmdAddr);
	writeb(0x00, uc1698CmdAddr);
	
	writeb(0xF6, uc1698CmdAddr);
	writeb(0x5A, uc1698CmdAddr);
	
	writeb(0xF7, uc1698CmdAddr);
	writeb(0xA3, uc1698CmdAddr);

	writeb(0xF8, uc1698CmdAddr);
	writeb(0xD1, uc1698CmdAddr);
	
	writeb(0xAF, uc1698CmdAddr);     /*显示开(Set Display Enable)*/
}

/**************************************************
函数名称:uc1698ClearScreen
功能描述:
调用函数:
被调用函数:
输入参数:uc1698清屏
输出参数:
返回值：
***************************************************/
void uc1698ClearScreen(void)
{
	int i,j;
	u8 uc_RowAddrH,uc_RowAddrL;
	
	for (i=0;i<160;i++)
	{
		//置行地址
		uc_RowAddrH = i /16;
    uc_RowAddrL = i %16;
		writeb(0x70 | uc_RowAddrH, uc1698CmdAddr);
		writeb(0x60 | uc_RowAddrL, uc1698CmdAddr);
		
		//置列地址
		writeb(0x12, uc1698CmdAddr);
		writeb(0x05, uc1698CmdAddr);
		
		for (j=0;j<54;j++)
		{
			writeb(0x00, uc1698DataAddr);
			writeb(0x00, uc1698DataAddr);
		}
	}
}


/****************************************lcd12864私有函数******************************************/
void lcdCheckBusy(void __iomem *addr)
{
	int i;
	
  for(i = 0; i < 0x20; i++)
  {
  	 ;
  }
}

void lcdGoto(int line, int col)
{
	 lcdCheckBusy(lcd12864CmdReadAddr1);
   writeb(SET_X_ADDRESS | line,lcd12864CmdWrAddr1);
	 lcdCheckBusy(lcd12864CmdReadAddr1);
   writeb(SET_Y_ADDRESS | col,lcd12864CmdWrAddr1);

	 lcdCheckBusy(lcd12864CmdReadAddr2);
   writeb(SET_X_ADDRESS | line,lcd12864CmdWrAddr2);
	 lcdCheckBusy(lcd12864CmdReadAddr2);
   writeb(SET_Y_ADDRESS | col,lcd12864CmdWrAddr2);
}

void lcdPutc(char c)
{
	 lcdCheckBusy(lcd12864CmdReadAddr1);
	 writeb(c,lcd12864DataWrAddr1);
	 lcdCheckBusy(lcd12864CmdReadAddr2);
	 writeb(c,lcd12864DataWrAddr2);
}

void lcdClear(void)
{
   unsigned char l, c;
    
   for (l = 0; l < 8; l++)
   {
      lcdGoto(l, 0);
       
      for (c = 0; c < 64; c++)
      {
         lcdPutc(0x00);
      }
   }
}

/**************************************************
函数名称:initlcd12864
功能描述:
调用函数:
被调用函数:
输入参数:初始化lcd12864
输出参数:
返回值：
***************************************************/
void initlcd12864(void)
{	
	 unsigned char i;
	 unsigned char lcdInitCmd[4]= {0x3F, 0xC0, 0x40, 0xB8};
	 unsigned long mode;
	 
	 /***************************LCD背光***********************************/
	 if (at91_set_gpio_output(AT91_PIN_PA28, 0)!=0)
	 {
	   printk(KERN_ALERT "LCD12864:set AT91_PIN_PA28 Output failed!\n");
	 }	
	 
	 /*设置总线接口特性(set the bus interface characteristics)*/	 
	 at91_sys_write(AT91_SMC_SETUP(busLcdConfig.cs), AT91_SMC_NWESETUP_(0x03) | AT91_SMC_NCS_WRSETUP_(0x03) | AT91_SMC_NRDSETUP_(0x03) | AT91_SMC_NCS_RDSETUP_(0x03));

	 at91_sys_write(AT91_SMC_PULSE(busLcdConfig.cs), AT91_SMC_NWEPULSE_(0x3f) | AT91_SMC_NCS_WRPULSE_(0x40) | AT91_SMC_NRDPULSE_(0x3f) | AT91_SMC_NCS_RDPULSE_(0x40));

	 //at91_sys_write(AT91_SMC_CYCLE(busConfig->cs), AT91_SMC_NWECYCLE_(0x80) | AT91_SMC_NRDCYCLE_(0x80));

	 if (busLcdConfig.busWidth16)
	 {
	 	  mode = AT91_SMC_DBW_16;
	 }
	 else
	 {
	 	  mode = AT91_SMC_DBW_8;
	 }
	 at91_sys_write(AT91_SMC_MODE(busLcdConfig.cs), mode | AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_TDF_(2));

	 for(i = 0; i < 4; i++)
	 {
		  //lcdCheckBusy(lcd12864CmdReadAddr1);
		  writeb(lcdInitCmd[i], lcd12864CmdWrAddr1);
		  //lcdCheckBusy(lcd12864CmdReadAddr2);
		  writeb(lcdInitCmd[i], lcd12864CmdWrAddr2);
	 }
	
	 lcdClear();
}

/**************************************************
函数名称:lcd12864ClearScreen
功能描述:
调用函数:
被调用函数:
输入参数:lcd12864清屏
输出参数:
返回值：
***************************************************/
void lcd12864ClearScreen(void)
{
	int i,j;
	u8 uc_RowAddrH,uc_RowAddrL;
	
	for (i=0;i<160;i++)
	{
		//置行地址
		uc_RowAddrH = i /16;
    uc_RowAddrL = i %16;
		//writeb(0x70 | uc_RowAddrH, lcd12864CmdAddr);
		//writeb(0x60 | uc_RowAddrL, lcd12864CmdAddr);
		
		//置列地址
		//writeb(0x12, lcd12864CmdAddr);
		//writeb(0x05, lcd12864CmdAddr);
		
		for (j=0;j<54;j++)
		{
			//writeb(0x00, lcd12864DataAddr);
			//writeb(0x00, lcd12864DataAddr);
		}
	}
}

/****************************************RA8835私有函数******************************************/

/**************************************************
函数名称:ra8835ClearMemory
功能描述:RA8835显示存储器清零
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
***************************************************/
static void ra8835ClearMemory(void)
{
   int i;

   // 光标方向向右，指针指向零地址
   writeb(CSR_RIGHT, ra8835CmdAddr);
   writeb(CSRW, ra8835CmdAddr);
   writeb(0, ra8835DataAddr);
   writeb(0, ra8835DataAddr);

   // 显存中数据全部清零
   writeb(MWRITE, ra8835CmdAddr);
   for (i = 0; i < 32 * 1024; i++)
   {
      writeb(0, ra8835DataAddr);
   }
}

/**************************************************
函数名称:ra8835SetCursor
功能描述:RA8835设置光标
调用函数:
被调用函数:
输入参数: x,横坐标 y,纵坐标
         注意:由于只能字节寻址，横坐标必须被8相除
输出参数:
返回值：
***************************************************/
void ra8835SetCursor(int x, int y)
{
   unsigned char  low, high;
   int actualAddr;

   x = x / 8;

   actualAddr = y * AP + x + GRAPHICS_BASE_ADDR;

   low = actualAddr&0xff;
   high = actualAddr>>8&0xff;

   writeb(CSRW, ra8835CmdAddr);  
   writeb(low, ra8835DataAddr);  
   writeb(high, ra8835DataAddr);  
}

/**************************************************
函数名称:ra8835ClearScreen
功能描述:清除显示屏幕
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
***************************************************/
void ra8835ClearScreen(void)
{
   int i;

   // 光标向右，在图形显示区将显存全部清零
   writeb(CSR_RIGHT, ra8835CmdAddr);

   ra8835SetCursor(0, 0);

   writeb(MWRITE, ra8835CmdAddr);

   for (i = 0; i < SCREEN_WIDTH * SCREEN_HIGHT / 8; i++)
   {
     writeb(0x0, ra8835DataAddr);
   }
}

/**************************************************
函数名称:initRa8835
功能描述:
调用函数:
被调用函数:
输入参数:初始化ra8835
输出参数:
返回值：
***************************************************/
void initRa8835(void)
{
	unsigned long mode;  //csa, mode;
	int           i;
	
	/*设置总线接口特性(set the bus interface characteristics)*/
	at91_sys_write(AT91_SMC_SETUP(busLcdConfig.cs), AT91_SMC_NWESETUP_(1) | AT91_SMC_NCS_WRSETUP_(1) | AT91_SMC_NRDSETUP_(1) | AT91_SMC_NCS_RDSETUP_(1));

	at91_sys_write(AT91_SMC_PULSE(busLcdConfig.cs), AT91_SMC_NWEPULSE_(8) | AT91_SMC_NCS_WRPULSE_(8) | AT91_SMC_NRDPULSE_(8) | AT91_SMC_NCS_RDPULSE_(8));

	if (busLcdConfig.busWidth16)
	{
		 mode = AT91_SMC_DBW_16;
	}
	else
	{
	  mode = AT91_SMC_DBW_8;
	}
	at91_sys_write(AT91_SMC_MODE(busLcdConfig.cs), mode | AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_TDF_(2));

  writeb(SYSTEM_SET, ra8835CmdAddr);// System config
  for (i = 0; i < 8; i++)
  {
     writeb(SYSTAB[i], ra8835DataAddr);
  }
	
  writeb(SCROLL, ra8835CmdAddr);   // Scroll settings
  for (i = 0; i < 10; i++)
  {  
     writeb(SCRTAB[i], ra8835DataAddr);
  }

  writeb(HDOT_SCR, ra8835CmdAddr);  
  writeb(0x00, ra8835DataAddr);   // 0000 0000    

  writeb(OVLAY, ra8835CmdAddr);
  writeb(0x01, ra8835DataAddr);   // 0000 0110    OV: 0, DM1: 1, DM2: 0 MX1: 1, MX2: 0

  writeb(DISP_OFF, ra8835CmdAddr);
  writeb(0x00, ra8835DataAddr);

  ra8835ClearMemory();

  writeb(CSRW, ra8835CmdAddr);
  writeb(0x00, ra8835DataAddr);   // 0000 0000    CSRL: 0x00
  writeb(0x00, ra8835DataAddr);   // 0000 0000    CSRH: 0x00

  writeb(CSRFORM, ra8835CmdAddr);
  writeb(0x00, ra8835DataAddr);   // 0000 0001    CRX: 1
  writeb(0x8f, ra8835DataAddr);   // 1000 1111    CM: 0, CRY: 15

  writeb(DISP_ON, ra8835CmdAddr);
  writeb(0x56, ra8835DataAddr);   // 0001 0110

  writeb(CSR_RIGHT, ra8835CmdAddr);

  ra8835SetCursor(0, 0);
}












/*------------------ LCD(parallel) char device driver(file operater) ----------------------------------------------*/

/**************************************************
函数名称:openParallelLcd
功能描述:
调用函数:
被调用函数:
输入参数:LCD设备文件操作 - 打开
输出参数:
返回值：
***************************************************/
static int openParallelLcd(struct inode *inodep, struct file *filp)
{
	 filp->private_data = parallelLcdDev;
	
	 #ifdef DEBUG_LCD_INFO
	   printk(KERN_ALERT "parallel LCD open sucess.\n");
	 #endif
	
	 return 0;
}

/**************************************************
函数名称:ioctlParallelLcd
功能描述:
调用函数:
被调用函数:
输入参数:并口LCD设备文件操作 - I/O控制
输出参数:
返回值：
***************************************************/
static int ioctlParallelLcd(struct inode *inode, struct file *filp, unsigned int command, unsigned long arg)
{
	unsigned int tmpData;
	
	switch (command)
	{
		 case 0x01:  //清屏
	     switch(lcdType)
	     {
	     	 case 2:  //12864
		 	     lcd12864ClearScreen();
	     	 	 break;
	     	 	 
	     	 case 4: //RA8835
	     	 	 ra8835ClearScreen();
	     	 	 break;
	     	 	 
	     	 default: //uc1698
	     	 	 uc1698ClearScreen();
	     	 	 break;
	     }
		 	 break;
		 	 
		 case 0x02:  //设置行
	     switch(lcdType)
	     {
	     	 case 2:  //12864
		       lcdGoto(arg,0);
	     	 	 break;
	     	 	 
	     	 case 4:  //320*240
           writeb(CSR_RIGHT, ra8835CmdAddr);
	     	 	 ra8835SetCursor(0,arg);
	     	 	 break;
	     	 	 
	     	 default: //uc1698
		       writeb(0x70 | (arg>>4&0xf), uc1698CmdAddr);         /*Set Row Address MSB*/
		       writeb(0x60 | (arg&0xf), uc1698CmdAddr);            /*Set Row Address LSB*/
		       break;
		   }
		   break;
		   
		 case 0x03:  //设置列
	     switch(lcdType)
	     {
	     	 case 2:  //12864
	     	 	 break;
	     	 
	     	 case 4:  //320*240
	     	 	 break;
	     	 	 
	     	 default: //uc1698
		       writeb(0x10 | ((arg+0x25)>>4&0x7), uc1698CmdAddr);  /*Set Column Address MSB*/
		       writeb(0x00 | ((arg+0x25)&0xf), uc1698CmdAddr);     /*Set Column Address LSB*/
		       break;
		   }
		   break;

		 case 0x04:  //LCD背光亮灭
	     switch(lcdType)
	     {
	     	 case 2:  //沃电老终端,12864
	     	 case 3:  //沃电专变III型终端,uc1698
		 	     at91_set_gpio_value(AT91_PIN_PA28, arg);
	     	 	 break;
	     	 	 
	     	 case 5:
		 	     at91_set_gpio_value(AT91_PIN_PC7, arg);	     	 	 
	     	 	 break;
	     	 	 
	     	 default: //沃电集中器,uc1698
		 	     at91_set_gpio_value(AT91_PIN_PC12,arg);
		 	     break;
		 	 }
		 	 break;

		 case 0x05:  //调节LCD对比度
	     switch(lcdType)
	     {
	     	 default: //沃电集中器,uc1698
		 	     if (arg<=20)
		 	     {
	           if (arg>10)
	           {
	             if (LCD_DEGREE_DEFAULT+6*(arg-10)>255)
	             {
	             	 tmpData = 254;
	             }
	             else
	             {
	               tmpData = LCD_DEGREE_DEFAULT+6*(arg-10);
	             }
	           }
	           else
	           {
	           	 tmpData = LCD_DEGREE_DEFAULT-6*(10-arg);
	           }
	         
	           writeb(0x81, uc1698CmdAddr);     /*Set gain and potentiometer Mode*/
	           writeb(tmpData, uc1698CmdAddr);  /*调节对比度就调节这个值*/
	         }
		 	     break;
		 	 }
		 	 break;

		 case 88:    //初始化显示器
	     lcdType = arg;
	     
	     switch(lcdType)
	     {
	     	 case 2:  //LCD12864
	         initlcd12864();
	         lcd12864ClearScreen();
		
	         printk(KERN_ALERT "Initializing lcd12864.... Cleared Screen.\n");
	     	 	 break;
	     	 	 
	     	 case 3:  //专变III型终端,uc1698
	         initUc1698();
	         uc1698ClearScreen();
	         
	         /***************************LCD背光***********************************/
	         if (at91_set_gpio_output(AT91_PIN_PA28, 0)!=0)
	         {
	           printk(KERN_ALERT "LCD12864:set AT91_PIN_PA28 Output failed!\n");
	         }

	         printk(KERN_ALERT "专变III型:Initializing Parallel Lcd uc1698u....Cleared Screen.\n");	     	 	 
	     	 	 break;
	     	 	 
	     	 case 4:  //RA8835
	         initRa8835();
	         ra8835ClearScreen();
	         
	         /***************************LCD背光***********************************/
           if (at91_set_gpio_output(AT91_PIN_PA26, 0)!=0)
	         {
	           printk(KERN_ALERT "LCD12864:set AT91_PIN_PA26 Output failed!\n");
	         }

	         printk(KERN_ALERT "Initializing Parallel Lcd RA8835....Cleared Screen.\n");	     	 	 
	     	 	 break;
	     	 	 
	     	 case 5:  //专变III型终端,uc1698(CPU-QFP)
	         initUc1698();
	         uc1698ClearScreen();
	         
	         /***************************LCD背光***********************************/
	         if (at91_set_gpio_output(AT91_PIN_PC7, 0)!=0)
	         {
	           printk(KERN_ALERT "LCD12864:set AT91_PIN_PC7 Output failed!\n");
	         }

	         printk(KERN_ALERT "专变III型(QFP):Initializing Parallel Lcd uc1698u....Cleared Screen.\n");	     	 	 
	     	 	 break;
	     	 	 
	     	 default: //集中器,uc1698
	         initUc1698();
	         uc1698ClearScreen();
	         
	         /***************************LCD背光***********************************/
	         if (at91_set_gpio_output(AT91_PIN_PC12, 1)!=0)
	         {
	           printk(KERN_ALERT "ioChannel:set AT91_PIN_PC12 Output failed!\n");
	         }

	         printk(KERN_ALERT "Initializing Parallel Lcd uc1698u....Cleared Screen.\n");
	         break;
	     }
		 	 break;
	}

	return 0;
}
/**************************************************
函数名称:writeParallelLcd
功能描述:向LCD(Parallel)写数据
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static ssize_t writeParallelLcd(struct file *flip, const char *buf, size_t size, loff_t *offp)
{
	 int i;
	 
	 switch(lcdType)
	 {
	   case 2:  //12864
    	 for(i=0;i<64;i++)
    	 {
    	   lcdCheckBusy(lcd12864CmdReadAddr1);
    	   writeb(*buf++,lcd12864DataWrAddr1);
    	 }
    	 
    	 for(i=64;i<128;i++)
    	 {
    	   lcdCheckBusy(lcd12864CmdReadAddr2);
    	   writeb(*buf++,lcd12864DataWrAddr2);
    	 }
	     break;
	     
	   case 4:  //320*240(RA8835)
	   	 writeb(MWRITE, ra8835CmdAddr);
	     for(i=0;i<size;i++)
	     {
	       writeb(*buf++, ra8835DataAddr);
	     }
	     break;
	     	 	 
	   default: //uc1698
	     for(i=0;i<size;i++)
	     {
	       writeb(*buf++, uc1698DataAddr);
	     }
	     break;
	 }
	 
	 return 0;
}

/**************************************************
函数名称:releaseParallelLcd
功能描述:
调用函数:
被调用函数:
输入参数:LCD设备文件操作 - 释放(关闭)
输出参数:
返回值：
***************************************************/
static int releaseParallelLcd(struct inode *inodep, struct file *filp)
{
	filp->private_data = NULL;
	
	#ifdef DEBUG_LCD_INFO
	 printk(KERN_ALERT "Parallel LCD released.\n");
	#endif
	
	return 0;
}


/*结构变量 - 文件操作(LCD驱动)*/
static struct file_operations fopsParallelLcd = 
{
  .owner	 = THIS_MODULE,
 	.open	   = openParallelLcd,
  .release = releaseParallelLcd,
  .ioctl   = ioctlParallelLcd,
  .write   = writeParallelLcd,
};

/**************************************************
函数名称:lcd_setup_cdev
功能描述:
调用函数:
被调用函数:
输入参数:添加LCD字符设备
输出参数:
返回值：
***************************************************/
static void lcd_setup_cdev(DEVICE_LCD *dev, int index)
{
	 int err;
	 int devNo;    /*设备号*/
	 
	 //向linux申请个设备号
	 devNo = MKDEV(parallelBusMajor,index);
	 
	 //向/dev/添加一个文件
	 cdev_init(&dev->lcdCdev,&fopsParallelLcd);
	 
	 dev->lcdCdev.owner = fopsParallelLcd.owner;
	 
	 //添加字符设备
	 err=cdev_add(&dev->lcdCdev,devNo,1);
	 
	 if (err)
	 {
		  printk(KERN_NOTICE"Error %d adding Parallel LCD %d\n",err,index);
	 }
}


/*********************platform device register ****************************************************************************************************/
/**************************************************
函数名称:at91_add_device_parallel_bus
功能描述:
调用函数:
被调用函数:
输入参数:添加设备(LCD-Parallel)
输出参数:
返回值：
***************************************************/
void at91_add_device_parallel_bus(void)
{
   //向平台设备注册LCD设备
	 platform_device_register(&at91sam9260_lcd_device);
}

/**************************************************
函数名称:at91_parallel_bus_probe
功能描述:
调用函数:
被调用函数:
输入参数:AT91驱动LCD加载
输出参数:
返回值：
***************************************************/
static int __devinit at91_parallel_bus_probe(struct platform_device *pdev)
{
	 return 0;
}

/**************************************************
函数名称:at91_parallel_bus_remove
功能描述:
调用函数:
被调用函数:
输入参数:AT91驱动LCD卸载
输出参数:
返回值：
***************************************************/
static int __devexit at91_parallel_bus_remove(struct platform_device *pdev)
{
	#ifdef DEBUG_LCD_INFO
	  printk(KERN_ALERT "Platform driver(parallel Bus) removed.\n");
	#endif
	
	return 0;
}

/*静态变量 - 平台设备(LCD)*/
static struct platform_driver at91_parallel_bus_driver = 
{
	.probe		= at91_parallel_bus_probe,
	.remove		= at91_parallel_bus_remove,
	.driver		= 
	  {
		 .name	= "parallelBus",
		 .owner = THIS_MODULE,
	  },
};

/**************************************************
函数名称:at91_parallel_bus_init
功能描述:
调用函数:
被调用函数:
输入参数:AT91驱动LCD初始化
输出参数:
返回值：
***************************************************/
static int __init at91_parallel_bus_init(void)
{
	int             result;
	
	dev_t           devNo;     /*设备号*/
	struct resource *res;      /*资源*/
	
	//向AT91SAM平台添加设备 - parallel LCD
	at91_add_device_parallel_bus();
  
  /*自动分配一个设备号*/
	if (alloc_chrdev_region(&devNo,0,1,"parallelBus") < 0)
	{
		 return -2;
	}
	
	parallelBusMajor= MAJOR(devNo);
	at91sam9260_lcd_device.dev.devt = devNo;
	
	parallelLcdDev = kmalloc(sizeof(DEVICE_LCD),GFP_KERNEL);
	
	if (!parallelLcdDev)
	{
	   result = -1;
	   goto fail_malloc;
	}
	
	memset(parallelLcdDev ,0,sizeof(DEVICE_LCD));
	
	lcd_setup_cdev(parallelLcdDev,0);
	
	res=platform_get_resource(&at91sam9260_lcd_device,IORESOURCE_MEM,0);	
	
	//uc1698地址映射
	uc1698CmdAddr = ioremap(res->start, 4);
	uc1698DataAddr = ioremap(res->end, 4);
	
	//lcd12864地址映射
	lcd12864CmdWrAddr1    = ioremap(res->start+0x04, 4);
	lcd12864DataWrAddr1   = ioremap(res->start+0x05, 4);
	lcd12864CmdReadAddr1  = ioremap(res->start+0x06, 4);
	lcd12864DataReadAddr1 = ioremap(res->start+0x07, 4);
	lcd12864CmdWrAddr2    = ioremap(res->start+0x08, 4);
	lcd12864DataWrAddr2   = ioremap(res->start+0x09, 4);
	lcd12864CmdReadAddr2  = ioremap(res->start+0x0a, 4);
	lcd12864DataReadAddr2 = ioremap(res->start+0x0b, 4);

	//RA8835地址映射
	ra8835CmdAddr = ioremap(res->start+1, 4);
	ra8835DataAddr = ioremap(res->start+2, 4);
	
	/* create your own class under /sysfs */
  classLcd = class_create(THIS_MODULE, "classLcd");
  if(IS_ERR(classLcd)) 
  {
     printk("Err: failed in creating class.\n");
     cdev_del(&parallelLcdDev->lcdCdev);
	   kfree(parallelLcdDev);
  	 goto fail_malloc; 
  } 

  /* register your own device in sysfs, and this will cause udev to create corresponding device node */
	device_create(classLcd, NULL, MKDEV(parallelBusMajor, 0), "parallelBus");
	
	result = platform_driver_register(&at91_parallel_bus_driver);
  if (result) 
  { 
 		 device_destroy(classLcd, MKDEV(parallelBusMajor, 0));
		 class_destroy(classLcd);
  	 cdev_del(&parallelLcdDev->lcdCdev);
	   kfree(parallelLcdDev);
  	 goto fail_malloc; 
  }
  
	printk(KERN_ALERT "Parallel Bus Driver Done.\n");
  return result;
  
fail_malloc:
	unregister_chrdev_region(devNo,1);
  
  //uc1698
	iounmap(uc1698CmdAddr);
	iounmap(uc1698DataAddr);
	
	//lcd12864
	iounmap(lcd12864CmdWrAddr1);
	iounmap(lcd12864DataWrAddr1);
	iounmap(lcd12864CmdReadAddr1);
	iounmap(lcd12864DataReadAddr1);
	iounmap(lcd12864CmdWrAddr2);
	iounmap(lcd12864DataWrAddr2);
	iounmap(lcd12864CmdReadAddr2);
	iounmap(lcd12864DataReadAddr2);

	//RA8835
	iounmap(ra8835CmdAddr);
	iounmap(ra8835DataAddr);

	printk(KERN_ALERT "Parallel LCD Driver Load Error.\n");
	return -1;
}

static void __exit at91_parallel_bus_exit(void)
{
	 platform_driver_unregister(&at91_parallel_bus_driver);
	 device_destroy(classLcd, MKDEV(parallelBusMajor, 0));  //delete device node under /dev
   class_destroy(classLcd);                           //delete class created by us
	 cdev_del(&parallelLcdDev->lcdCdev);
	 kfree(parallelLcdDev);
	 unregister_chrdev_region(MKDEV(parallelBusMajor,0),1);
	 
	 //uc1698
	 iounmap(uc1698CmdAddr);
	 iounmap(uc1698DataAddr);
	 
	 //lcd12864
	 iounmap(lcd12864CmdWrAddr1);
	 iounmap(lcd12864DataWrAddr1);
	 iounmap(lcd12864CmdReadAddr1);
	 iounmap(lcd12864DataReadAddr1);
   iounmap(lcd12864CmdWrAddr2);
	 iounmap(lcd12864DataWrAddr2);
	 iounmap(lcd12864CmdReadAddr2);
	 iounmap(lcd12864DataReadAddr2);
	 
	 //RA8835
	 iounmap(ra8835CmdAddr);
	 iounmap(ra8835DataAddr);
	 
   printk(KERN_ALERT "Parallel Bus Driver Exited.\n");
}


module_init(at91_parallel_bus_init);
module_exit(at91_parallel_bus_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ly");
MODULE_DESCRIPTION("Parallel Bus driver for AT91SAM9260");
