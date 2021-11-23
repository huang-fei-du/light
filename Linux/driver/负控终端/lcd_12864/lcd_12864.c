/***************************************************
Copyright,All	Rights Reserved
�ļ���:lcd_12864.c
����:LCD(12864����оƬ)�����ļ�
�����б�:
�޸���ʷ:
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

/*������Ϣ*/
//#define DEBUG_LCD_INFO


/*LCD����(����)����ַ*/
#define LCD_CTRL_BASE_ADDR AT91_CHIPSELECT_0

/*�ṹ - ����(����LCD)����*/
/*struct at91_lcdcmd_data*/
typedef struct
{
	 u8	busWidth16;        //���߿���Ƿ�Ϊ16λ
	 u8 cs;                //Ƭѡ�ź�
}BUS_LCD_CONFIG;

/*�ṹ - LCD�豸*/
typedef struct
{
   struct cdev lcdCdev;
   char data;
}DEVICE_LCD;

/*�� - LCD��*/
struct class *classLcd;

/*���� - ����lcd12864�������ַ�����ݵ�ַ*/
static void __iomem *lcd12864CmdWrAddr1,*lcd12864DataWrAddr1,*lcd12864CmdReadAddr1,*lcd12864DataReadAddr1;
static void __iomem *lcd12864CmdWrAddr2,*lcd12864DataWrAddr2,*lcd12864CmdReadAddr2,*lcd12864DataReadAddr2;
static void __iomem *lcd12864tmpAddr;

/*��̬���� - lcd12864�����豸��*/
static int lcd12864Major;

/*��̬���� - ����(����)����*/
static BUS_LCD_CONFIG busLcdConfig = 
{
	.busWidth16	 = 0,
	.cs          = 0,
};


/*����lcd12864����Դָ��*/
static struct resource lcd12864Resources[] = 
{
	{
		.start = (LCD_CTRL_BASE_ADDR),
		.end	 = (LCD_CTRL_BASE_ADDR+0x1f),
		.flags = IORESOURCE_MEM,
	}
};

/*ƽ̨�豸 - LCD*/
static struct platform_device at91sam9260_lcd_device = 
{
	.name	= "lcd12864",
	.id		= -1,
	.dev	=
	      {
				  .platform_data	= &busLcdConfig,
	      },
	.resource	= lcd12864Resources,
	.num_resources	= ARRAY_SIZE(lcd12864Resources),
};

/*�ṹ���� - LCD�豸(lcd12864)*/
static DEVICE_LCD *lcdDev12864;

void lcd12864ClearScreen(void);

void lcdCheckBusy(void __iomem *addr)
{
	int i;
	//unsigned char tmpData;
	
  for(i = 0; i < 0x20; i++)
  {
    //tmpData = readb(addr);
    //printk("tmpData=%02x\n",tmpData);
    //if (!(tmpData& 0x80))
    //{
    //   break;
    //}
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
       
      //for (c = 0; c < 32; c++)
      //{
      //   lcdPutc(0xff);
      //}
      
      for (c = 0; c < 64; c++)
      {
         lcdPutc(0x00);
      }
   }
}
void lcdClearx(void)
{
   unsigned char l, c;
    
   for (l = 0; l < 8; l++)
   {
      lcdGoto(l, 0);
       
      for (c = 32; c < 64; c++)
      {
         lcdPutc(0xff);
      }
   }
}

/*------------------ LCD(lcd12864) char device driver(file operater) ----------------------------------------------*/

/**************************************************
��������:openlcd12864
��������:
���ú���:
�����ú���:
�������:lcd12864 LCD�豸�ļ����� - ��
�������:
����ֵ��
***************************************************/
static int openlcd12864(struct inode *inodep, struct file *filp)
{
	 filp->private_data = lcdDev12864;
	
	 #ifdef DEBUG_LCD_INFO
	   printk(KERN_ALERT "LCD(lcd12864) open sucess.\n");
	 #endif
	
	 return 0;
}



/**************************************************
��������:ioctllcd12864
��������:
���ú���:
�����ú���:
�������:lcd12864 LCD�豸�ļ����� - I/O����
�������:
����ֵ��
***************************************************/
static int ioctllcd12864(struct inode *inode, struct file *filp, unsigned int command, unsigned long arg)
{
	switch (command)
	{
		 case 0x01:  //����
		 	 lcd12864ClearScreen();
		 	 break;
		 	 
		 case 0x02:  //������
		   lcdGoto(arg,0);
		   break;
		   
		 case 0x03:  //������
		   break;
		   
		 case 0x04:  //LCD��������
		 	 at91_set_gpio_value(AT91_PIN_PA28, arg);
		 	 break;
	}

	return 0;
}
/**************************************************
��������:writelcd12864
��������:��LCD(lcd12864)д����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/
static ssize_t writelcd12864(struct file *flip, const char *buf, size_t size, loff_t *offp)
{
	 int i;

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
	 
	 return 0;
}

/**************************************************
��������:releaselcd12864
��������:
���ú���:
�����ú���:
�������:lcd12864 LCD�豸�ļ����� - �ͷ�(�ر�)
�������:
����ֵ��
***************************************************/
static int releaselcd12864(struct inode *inodep, struct file *filp)
{
	filp->private_data = NULL;
	
	#ifdef DEBUG_LCD_INFO
	 printk(KERN_ALERT "LCD(lcd12864) released.\n");
	#endif
	
	return 0;
}


/*�ṹ���� - �ļ�����(LCD-lcd12864����)*/
static struct file_operations fopslcd12864 = 
{
  .owner	 = THIS_MODULE,
 	.open	   = openlcd12864,
  .release = releaselcd12864,
  .ioctl   = ioctllcd12864,
  .write   = writelcd12864,
};

/**************************************************
��������:lcd_setup_cdev
��������:
���ú���:
�����ú���:
�������:���LCD�ַ��豸
�������:
����ֵ��
***************************************************/
static void lcd_setup_cdev(DEVICE_LCD *dev, int index)
{
	 int err;
	 int devNo;    /*�豸��*/
	 
	 //��linux������豸��
	 devNo = MKDEV(lcd12864Major,index);
	 
	 //��/dev/���һ���ļ�
	 cdev_init(&dev->lcdCdev,&fopslcd12864);
	 
	 dev->lcdCdev.owner = fopslcd12864.owner;
	 
	 //����ַ��豸
	 err=cdev_add(&dev->lcdCdev,devNo,1);
	 
	 if (err)
	 {
		  printk(KERN_NOTICE"Error %d adding LCD(lcd12864) %d\n",err,index);
	 }
}


/*********************platform device register ****************************************************************************************************/


/**************************************************
��������:initlcd12864
��������:
���ú���:
�����ú���:
�������:��ʼ��lcd12864
�������:
����ֵ��
***************************************************/
void initlcd12864(void)
{	
	 unsigned char i;
	 unsigned char lcdInitCmd[4]= {0x3F, 0xC0, 0x40, 0xB8};
	 for(i = 0; i < 4; i++)
	 {
		  //lcdCheckBusy(lcd12864CmdReadAddr1);
		  writeb(lcdInitCmd[i], lcd12864CmdWrAddr1);
		  //lcdCheckBusy(lcd12864CmdReadAddr2);
		  writeb(lcdInitCmd[i], lcd12864CmdWrAddr2);
	 }
	
	 lcdClear();
	 //lcdClearx();
}

/**************************************************
��������:lcd12864ClearScreen
��������:
���ú���:
�����ú���:
�������:lcd12864����
�������:
����ֵ��
***************************************************/
void lcd12864ClearScreen(void)
{
	int i,j;
	u8 uc_RowAddrH,uc_RowAddrL;
	
	for (i=0;i<160;i++)
	{
		//���е�ַ
		uc_RowAddrH = i /16;
    uc_RowAddrL = i %16;
		//writeb(0x70 | uc_RowAddrH, lcd12864CmdAddr);
		//writeb(0x60 | uc_RowAddrL, lcd12864CmdAddr);
		
		//���е�ַ
		//writeb(0x12, lcd12864CmdAddr);
		//writeb(0x05, lcd12864CmdAddr);
		
		for (j=0;j<54;j++)
		{
			//writeb(0x00, lcd12864DataAddr);
			//writeb(0x00, lcd12864DataAddr);
		}
	}
}

/**************************************************
��������:at91_add_device_lcd_lcd12864
��������:
���ú���:
�����ú���:
�������:����豸(LCD-lcd12864u)
�������:
����ֵ��
***************************************************/
void at91_add_device_lcd_lcd12864(BUS_LCD_CONFIG *busConfig)
{
	 unsigned long mode;  //csa, mode;

	 if (!busConfig)
	 {
		  return;
	 }

	 //csa = at91_sys_read(AT91_MATRIX_EBICSA); csa = csa & 0xffffffdf;
	 //at91_sys_write(AT91_MATRIX_EBICSA, csa | AT91_MATRIX_CS5A_SMC);

	 /*�������߽ӿ�����(set the bus interface characteristics)*/	 
	 at91_sys_write(AT91_SMC_SETUP(busConfig->cs), AT91_SMC_NWESETUP_(0x03) | AT91_SMC_NCS_WRSETUP_(0x03) | AT91_SMC_NRDSETUP_(0x03) | AT91_SMC_NCS_RDSETUP_(0x03));

	 at91_sys_write(AT91_SMC_PULSE(busConfig->cs), AT91_SMC_NWEPULSE_(0x3f) | AT91_SMC_NCS_WRPULSE_(0x40) | AT91_SMC_NRDPULSE_(0x3f) | AT91_SMC_NCS_RDPULSE_(0x40));

	 //at91_sys_write(AT91_SMC_CYCLE(busConfig->cs), AT91_SMC_NWECYCLE_(0x80) | AT91_SMC_NRDCYCLE_(0x80));

	 if (busConfig->busWidth16)
	 {
	 	  mode = AT91_SMC_DBW_16;
	 }
	 else
	 {
	 	  mode = AT91_SMC_DBW_8;
	 }
	 at91_sys_write(AT91_SMC_MODE(busConfig->cs), mode | AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_TDF_(2));

   //��ƽ̨�豸ע��LCD�豸
	 platform_device_register(&at91sam9260_lcd_device);
}

/**************************************************
��������:at91_lcd_probe
��������:
���ú���:
�����ú���:
�������:AT91����LCD����
�������:
����ֵ��
***************************************************/
static int __devinit at91_lcd_probe(struct platform_device *pdev)
{
	 initlcd12864();
	 lcd12864ClearScreen();
		
	 printk(KERN_ALERT "Initializing lcd12864.... Cleared Screen.\n");

	 return 0;
}

/**************************************************
��������:at91_lcd_remove
��������:
���ú���:
�����ú���:
�������:AT91����LCDж��
�������:
����ֵ��
***************************************************/
static int __devexit at91_lcd_remove(struct platform_device *pdev)
{
	#ifdef DEBUG_LCD_INFO
	  printk(KERN_ALERT "Platform driver(LCD-lcd12864) removed.\n");
	#endif
	
	return 0;
}

/*��̬���� - ƽ̨�豸(LCD)*/
static struct platform_driver at91_lcd_driver = 
{
	.probe		= at91_lcd_probe,
	.remove		= at91_lcd_remove,
	.driver		= 
	   {
		  .name	= "lcd12864",
		  .owner	= THIS_MODULE,
	   },
};

/**************************************************
��������:at91_lcd_init
��������:
���ú���:
�����ú���:
�������:AT91����LCD��ʼ��
�������:
����ֵ��
***************************************************/
static int __init at91_lcd_init(void)
{
	int             result;
	
	dev_t           devNo;     /*�豸��*/
	struct resource *res;      /*��Դ*/
	
	/***************************LCD����***********************************/
	if (at91_set_gpio_output(AT91_PIN_PA28, 0)!=0)
	{
	   printk(KERN_ALERT "LCD12864:set AT91_PIN_PA28 Output failed!\n");
	}	
	
	//��AT91SAMƽ̨����豸 - LCD(lcd12864)
	at91_add_device_lcd_lcd12864(&busLcdConfig);
  
  /*�Զ�����һ���豸��*/
	if (alloc_chrdev_region(&devNo,0,1,"lcd12864") < 0)
	{
		 return -2;
	}
	
	lcd12864Major= MAJOR(devNo);
	at91sam9260_lcd_device.dev.devt = devNo;
	
	lcdDev12864 = kmalloc(sizeof(DEVICE_LCD),GFP_KERNEL);
	
	if (!lcdDev12864)
	{
	   result = -1;
	   goto fail_malloc;
	}
	
	memset(lcdDev12864 ,0,sizeof(DEVICE_LCD));
	
	lcd_setup_cdev(lcdDev12864,0);
	
	res=platform_get_resource(&at91sam9260_lcd_device,IORESOURCE_MEM,0);

	lcd12864tmpAddr       = ioremap(res->start, 1);
	lcd12864CmdWrAddr1    = ioremap(res->start+0x04, 4);
	lcd12864DataWrAddr1   = ioremap(res->start+0x05, 4);
	lcd12864CmdReadAddr1  = ioremap(res->start+0x06, 4);
	lcd12864DataReadAddr1 = ioremap(res->start+0x07, 4);
	lcd12864CmdWrAddr2    = ioremap(res->start+0x08, 4);
	lcd12864DataWrAddr2   = ioremap(res->start+0x09, 4);
	lcd12864CmdReadAddr2  = ioremap(res->start+0x0a, 4);
	lcd12864DataReadAddr2 = ioremap(res->start+0x0b, 4);
	
	/* create your own class under /sysfs */
  classLcd = class_create(THIS_MODULE, "classLcd");
  if(IS_ERR(classLcd)) 
  {
     printk("Err: failed in creating class.\n");
     cdev_del(&lcdDev12864->lcdCdev);
	   kfree(lcdDev12864);
  	 goto fail_malloc; 
  } 

  /* register your own device in sysfs, and this will cause udev to create corresponding device node */
	device_create(classLcd, NULL, MKDEV(lcd12864Major, 0), "lcd12864");
	
	result = platform_driver_register(&at91_lcd_driver);
  if (result) 
  { 
 		 device_destroy(classLcd, MKDEV(lcd12864Major, 0));
		 class_destroy(classLcd);
  	 cdev_del(&lcdDev12864->lcdCdev);
	   kfree(lcdDev12864);
  	 goto fail_malloc; 
  }
  
	printk(KERN_ALERT "LCD(lcd12864) Driver Done.\n");
  return result;
  
fail_malloc:
	unregister_chrdev_region(devNo,1);
	iounmap(lcd12864CmdWrAddr1);
	iounmap(lcd12864DataWrAddr1);
	iounmap(lcd12864CmdReadAddr1);
	iounmap(lcd12864DataReadAddr1);
	iounmap(lcd12864CmdWrAddr2);
	iounmap(lcd12864DataWrAddr2);
	iounmap(lcd12864CmdReadAddr2);
	iounmap(lcd12864DataReadAddr2);
	printk(KERN_ALERT "LCD(lcd12864) Driver Load Error.\n");
	return -1;
}

static void __exit at91_lcd_exit(void)
{
	 platform_driver_unregister(&at91_lcd_driver);
	 device_destroy(classLcd, MKDEV(lcd12864Major, 0));  //delete device node under /dev
   class_destroy(classLcd);                           //delete class created by us
	 cdev_del(&lcdDev12864->lcdCdev);
	 kfree(lcdDev12864);
	 unregister_chrdev_region(MKDEV(lcd12864Major,0),1);
	 
	 iounmap(lcd12864CmdWrAddr1);
	 iounmap(lcd12864DataWrAddr1);
	 iounmap(lcd12864CmdReadAddr1);
	 iounmap(lcd12864DataReadAddr1);
	 iounmap(lcd12864CmdWrAddr2);
	 iounmap(lcd12864DataWrAddr2);
	 iounmap(lcd12864CmdReadAddr2);
	 iounmap(lcd12864DataReadAddr2);
	 
   printk(KERN_ALERT "LCD(lcd12864) Driver Exited.\n");
}


module_init(at91_lcd_init);
module_exit(at91_lcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lhh");
MODULE_DESCRIPTION("lcd 12864 driver for AT91RM9260");
