/***************************************************
Copyright,2009,Huawei WoDian co.,LTD,All	Rights Reserved
文件名:lcd_uc1698.c
作者:leiyong
版本:0.9
完成日期:2009年12月
描述:电力终端(负控终端/集中器,AT91SAM9260处理器)LCD(uc1698驱动芯片)驱动文件
函数列表:

修改历史:
  01,09-11-20,Luoyun created.
  02,09-12-17,Leiyong规范化文件格式.
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

/*LCD控制(操作)基地址*/
#define LCD_CTRL_BASE_ADDR AT91_CHIPSELECT_0

/*结构 - 总线(操作LCD)配置*/
/*struct at91_lcdcmd_data*/
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


/*静态变量 - uc1698的主设备号*/
static int lcdUc1698Major;

/*静态变量 - 总线(操作)配置*/
static BUS_LCD_CONFIG busLcdConfig = 
{
	.busWidth16	 = 0,
	.cs          = 0,
};


/*操作uc1698的资源指定*/
static struct resource lcdUc1698Resources[] = 
{
	{
		.start = (LCD_CTRL_BASE_ADDR),
		.end	 = (LCD_CTRL_BASE_ADDR+0x4),
		.flags = IORESOURCE_MEM,
	}
};

/*平台设备 - LCD*/
static struct platform_device at91sam9260_lcd_device = 
{
	.name	= "lcdUc1698",
	.id		= -1,
	.dev	=
	      {
				  .platform_data	= &busLcdConfig,
	      },
	.resource	= lcdUc1698Resources,
	.num_resources	= ARRAY_SIZE(lcdUc1698Resources),
};

/*结构变量 - LCD设备(uc1698)*/
static DEVICE_LCD *lcdDevUc1698;

void uc1698ClearScreen(void);

/*------------------ LCD(uc1698) char device driver(file operater) ----------------------------------------------*/

/**************************************************
函数名称:openLcdUc1698
功能描述:
调用函数:
被调用函数:
输入参数:uc1698 LCD设备文件操作 - 打开
输出参数:
返回值：
***************************************************/
static int openLcdUc1698(struct inode *inodep, struct file *filp)
{
	 filp->private_data = lcdDevUc1698;
	
	 #ifdef DEBUG_LCD_INFO
	   printk(KERN_ALERT "LCD(uc1698) open sucess.\n");
	 #endif
	
	 return 0;
}

/**************************************************
函数名称:ioctlLcdUc1698
功能描述:
调用函数:
被调用函数:
输入参数:uc1698 LCD设备文件操作 - I/O控制
输出参数:
返回值：
***************************************************/
static int ioctlLcdUc1698(struct inode *inode, struct file *filp, unsigned int command, unsigned long arg)
{
	switch (command)
	{
		 case 0x01:  //清屏
		 	 uc1698ClearScreen();
		 	 break;
		 	 
		 case 0x02:  //设置行
		   writeb(0x70 | (arg>>4&0xf), uc1698CmdAddr);         /*Set Row Address MSB*/
		   writeb(0x60 | (arg&0xf), uc1698CmdAddr);            /*Set Row Address LSB*/
		   break;
		   
		 case 0x03:  //设置列
		   writeb(0x10 | ((arg+0x25)>>4&0x7), uc1698CmdAddr);  /*Set Column Address MSB*/
		   writeb(0x00 | ((arg+0x25)&0xf), uc1698CmdAddr);     /*Set Column Address LSB*/
		   break;

		 case 0x04:  //LCD背光亮灭
		 	 at91_set_gpio_value(AT91_PIN_PC12,arg);
		 	 break;
	}

	return 0;
}
/**************************************************
函数名称:writeLcdUc1698
功能描述:向LCD(uc1698)写数据
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static ssize_t writeLcdUc1698(struct file *flip, const char *buf, size_t size, loff_t *offp)
{
	 int i;
	 
	 for(i=0;i<size;i++)
	 {
	    writeb(*buf++, uc1698DataAddr);
	 }
	 
	 return 0;
}

/**************************************************
函数名称:releaseLcdUc1698
功能描述:
调用函数:
被调用函数:
输入参数:uc1698 LCD设备文件操作 - 释放(关闭)
输出参数:
返回值：
***************************************************/
static int releaseLcdUc1698(struct inode *inodep, struct file *filp)
{
	filp->private_data = NULL;
	
	#ifdef DEBUG_LCD_INFO
	 printk(KERN_ALERT "LCD(uc1698) released.\n");
	#endif
	
	return 0;
}


/*结构变量 - 文件操作(LCD-UC1698驱动)*/
static struct file_operations fopsLcdUc1698 = 
{
  .owner	 = THIS_MODULE,
 	.open	   = openLcdUc1698,
  .release = releaseLcdUc1698,
  .ioctl   = ioctlLcdUc1698,
  .write   = writeLcdUc1698,
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
	 devNo = MKDEV(lcdUc1698Major,index);
	 
	 //向/dev/添加一个文件
	 cdev_init(&dev->lcdCdev,&fopsLcdUc1698);
	 
	 dev->lcdCdev.owner = fopsLcdUc1698.owner;
	 
	 //添加字符设备
	 err=cdev_add(&dev->lcdCdev,devNo,1);
	 
	 if (err)
	 {
		  printk(KERN_NOTICE"Error %d adding LCD(uc1698) %d\n",err,index);
	 }
}


/*********************platform device register ****************************************************************************************************/


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
	writeb(0xE2, uc1698CmdAddr);     /*系统软件复位(System Reset)*/
	writeb(0xAE, uc1698CmdAddr);     /*设置显示显示控制(Set Display Enable)[显示关闭]*/
	writeb(0x27, uc1698CmdAddr);     /*Set Temperature Compensation (00-0.25%)*/
	writeb(0x2A, uc1698CmdAddr);     /*Set Power Control (Interal VLCD;Panel loading definition<13nF)*/
	writeb(0xE9, uc1698CmdAddr);     /*Set LCD Bias ratio:1/11*/
	
	writeb(0x81, uc1698CmdAddr);     /*Set gain and potentiometer Mode*/
	writeb(0xa0, uc1698CmdAddr);     /*PM值设置为0xc2,调节对比度就调节这个值*/
	
	writeb(0x89, uc1698CmdAddr);     /*Set RAM Address Control*/
	writeb(0xC4, uc1698CmdAddr);     /*Set LCD Maping Control (MY=1, MX=0)*/
	
	writeb(0x84, uc1698CmdAddr);     /*Set Partial Display Off*/
	
	writeb(0xD6, uc1698CmdAddr);     /*Set Color Mode (64K)*/
	writeb(0xD1, uc1698CmdAddr);     /*Set Color Pattern (RGB)*/
	writeb(0xDE, uc1698CmdAddr);     /*Set COM Scan Function*/
	
	writeb(0xAD, uc1698CmdAddr);     /*显示开(Set Display Enable)*/
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

/**************************************************
函数名称:at91_add_device_lcd_uc1698
功能描述:
调用函数:
被调用函数:
输入参数:添加设备(LCD-uc1698u)
输出参数:
返回值：
***************************************************/
void at91_add_device_lcd_uc1698(BUS_LCD_CONFIG *busConfig)
{
	 unsigned long mode;  //csa, mode;

	 if (!busConfig)
	 {
		  return;
	 }

	 //csa = at91_sys_read(AT91_MATRIX_EBICSA); csa = csa & 0xffffffdf;
	 //at91_sys_write(AT91_MATRIX_EBICSA, csa | AT91_MATRIX_CS5A_SMC);

	 /*设置总线接口特性(set the bus interface characteristics)*/	 
	 at91_sys_write(AT91_SMC_SETUP(busConfig->cs), AT91_SMC_NWESETUP_(0) | AT91_SMC_NCS_WRSETUP_(0) | AT91_SMC_NRDSETUP_(0) | AT91_SMC_NCS_RDSETUP_(0));

	 at91_sys_write(AT91_SMC_PULSE(busConfig->cs), AT91_SMC_NWEPULSE_(3) | AT91_SMC_NCS_WRPULSE_(3) | AT91_SMC_NRDPULSE_(3) | AT91_SMC_NCS_RDPULSE_(3));

	 at91_sys_write(AT91_SMC_CYCLE(busConfig->cs), AT91_SMC_NWECYCLE_(5) | AT91_SMC_NRDCYCLE_(5));

	 if (busConfig->busWidth16)
	 {
	 	  mode = AT91_SMC_DBW_16;
	 }
	 else
	 {
	 	  mode = AT91_SMC_DBW_8;
	 }
	 at91_sys_write(AT91_SMC_MODE(busConfig->cs), mode | AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_TDF_(2));

   //向平台设备注册LCD设备
	 platform_device_register(&at91sam9260_lcd_device);
}

/**************************************************
函数名称:at91_lcd_probe
功能描述:
调用函数:
被调用函数:
输入参数:AT91驱动LCD加载
输出参数:
返回值：
***************************************************/
static int __devinit at91_lcd_probe(struct platform_device *pdev)
{
	 initUc1698();
	 uc1698ClearScreen();
		
	 printk(KERN_ALERT "Initializing uc1698.... Cleared Screen.\n");

	 return 0;
}

/**************************************************
函数名称:at91_lcd_remove
功能描述:
调用函数:
被调用函数:
输入参数:AT91驱动LCD卸载
输出参数:
返回值：
***************************************************/
static int __devexit at91_lcd_remove(struct platform_device *pdev)
{
	#ifdef DEBUG_LCD_INFO
	  printk(KERN_ALERT "Platform driver(LCD-UC1698) removed.\n");
	#endif
	
	return 0;
}

/*静态变量 - 平台设备(LCD)*/
static struct platform_driver at91_lcd_driver = 
{
	.probe		= at91_lcd_probe,
	.remove		= at91_lcd_remove,
	.driver		= 
	   {
		  .name	= "lcdUc1698",
		  .owner	= THIS_MODULE,
	   },
};

/**************************************************
函数名称:at91_lcd_init
功能描述:
调用函数:
被调用函数:
输入参数:AT91驱动LCD初始化
输出参数:
返回值：
***************************************************/
static int __init at91_lcd_init(void)
{
	int             result;
	
	dev_t           devNo;     /*设备号*/
	struct resource *res;      /*资源*/

	/***************************LCD背光***********************************/
	if (at91_set_gpio_output(AT91_PIN_PC12, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC12 Output failed!\n");
	}
	
	//向AT91SAM平台添加设备 - LCD(uc1698)
	at91_add_device_lcd_uc1698(&busLcdConfig);
  
  /*自动分配一个设备号*/
	if (alloc_chrdev_region(&devNo,0,1,"lcdUc1698") < 0)
	{
		 return -2;
	}
	
	lcdUc1698Major= MAJOR(devNo);
	at91sam9260_lcd_device.dev.devt = devNo;
	
	lcdDevUc1698 = kmalloc(sizeof(DEVICE_LCD),GFP_KERNEL);
	
	if (!lcdDevUc1698)
	{
	   result = -1;
	   goto fail_malloc;
	}
	
	memset(lcdDevUc1698 ,0,sizeof(DEVICE_LCD));
	
	lcd_setup_cdev(lcdDevUc1698,0);
	
	res=platform_get_resource(&at91sam9260_lcd_device,IORESOURCE_MEM,0);
	uc1698CmdAddr = ioremap(res->start, 4);
	uc1698DataAddr = ioremap(res->end, 4);
	
	/* create your own class under /sysfs */
  classLcd = class_create(THIS_MODULE, "classLcd");
  if(IS_ERR(classLcd)) 
  {
     printk("Err: failed in creating class.\n");
     cdev_del(&lcdDevUc1698->lcdCdev);
	   kfree(lcdDevUc1698);
  	 goto fail_malloc; 
  } 

  /* register your own device in sysfs, and this will cause udev to create corresponding device node */
	device_create(classLcd, NULL, MKDEV(lcdUc1698Major, 0), "lcdUc1698");
	
	result = platform_driver_register(&at91_lcd_driver);
  if (result) 
  { 
 		 device_destroy(classLcd, MKDEV(lcdUc1698Major, 0));
		 class_destroy(classLcd);
  	 cdev_del(&lcdDevUc1698->lcdCdev);
	   kfree(lcdDevUc1698);
  	 goto fail_malloc; 
  }
  
	printk(KERN_ALERT "LCD(UC1698) Driver Done.\n");
  return result;
  
fail_malloc:
	unregister_chrdev_region(devNo,1);
	iounmap(uc1698CmdAddr);
	iounmap(uc1698DataAddr);
	printk(KERN_ALERT "LCD(UC1698) Driver Load Error.\n");
	return -1;
}

static void __exit at91_lcd_exit(void)
{
	 platform_driver_unregister(&at91_lcd_driver);
	 device_destroy(classLcd, MKDEV(lcdUc1698Major, 0));  //delete device node under /dev
   class_destroy(classLcd);                           //delete class created by us
	 cdev_del(&lcdDevUc1698->lcdCdev);
	 kfree(lcdDevUc1698);
	 unregister_chrdev_region(MKDEV(lcdUc1698Major,0),1);
	 
	 iounmap(uc1698CmdAddr);
	 iounmap(uc1698DataAddr);
	 
   printk(KERN_ALERT "LCD(UC1698) Driver Exited.\n");
}


module_init(at91_lcd_init);
module_exit(at91_lcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ly");
MODULE_DESCRIPTION("lcdcmd driver for AT91RM9260");
