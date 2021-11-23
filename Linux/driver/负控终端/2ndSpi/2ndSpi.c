/***************************************************
Copyright ly,2010,All	Rights Reserved
文件名:2ndSpi.c
作者:Leiyong
版本:0.9
完成日期:2010年04月
描述:电力终端(负控终端/集中器,AT91SAM9260处理器)SPI接口(交流采样及单片机通信)字符驱动文件
函数列表:

修改历史:
  01,10-04-20,Leiyong created.

***************************************************/

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <asm/arch/at91_pio.h>
#include <asm/arch/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define AC_SAMPLE_IS_SOFT_SPI     //交采用软件模拟SPI接口
#define CTRL_PROCESS_IS_ATMEGA8L  //控制板单片机是ATMEGA8L

//for pmc
#define PMC_BASE	0xFFFFFC00
#define PMC_PCER	0x0010
#define PMC_PCSR	0x0018

//for spi
#define SPI_BASE_SPI0	0xFFFC8000  //spi0
#define SPI_BASE	    0xFFFCC000  //spi1
#define SPI_CR		    0x0000
#define SPI_MR		    0x0004
#define SPI_SR		    0x0010
#define SPI_TDR		    0x000C
#define SPI_CSR0	    0x0030
#define SPI_CSR1	    0x0034

MODULE_DESCRIPTION("Interface Spi 2nd driver");
MODULE_AUTHOR("lhh");
MODULE_LICENSE("GPL");

static void __iomem  *pmc_base, *spi_base;

//交采SPI变量
struct spi_device    *acSample_spi;
static int           acSample_major=0;
struct cdev          acSampleCdev;
struct class         *classAcSample;

#ifdef CTRL_PROCESS_IS_ATMEGA8L
 //stc12  SPI变量
 struct spi_device   *atmega8_spi;
 static int          atmega8_major=0;
 struct cdev         atmega8Cdev;
 struct class        *classAtmega8;
#else
 //stc12  SPI变量
 struct spi_device   *stc12_spi;
 static int          stc12_major=0;
 struct cdev         stc12Cdev;
 struct class        *classStc12;
#endif


void delayTime(int times)
{
	int i,j;
	for(i=0;i<times;i++)
  {
  	for(j=0;j<0x80;j++)
  	{
  		 ;
  	}
  }
}

/**************************************************
函数名称:spi_read_my(目前没有用这个函数 09.12.20)
功能描述:
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
int spi_read_my(struct spi_device *spi,const u8 *txbuf,unsigned n_tx,u8 *rxbuf,unsigned n_rx)
{
	ssize_t                 ret;
	struct mutex            lock;
	struct spi_transfer     t[2];
	struct spi_message      m;
	
	spi_message_init(&m);
	memset(t, 0, sizeof t);

	t[0].tx_buf = txbuf;
	t[0].len = n_tx;
	spi_message_add_tail(&t[0], &m);

	t[1].rx_buf = rxbuf;
	t[1].len = n_rx;
	spi_message_add_tail(&t[1], &m);

	mutex_init(&lock);
	mutex_lock(&lock);
	ret = spi_sync(acSample_spi, &m);

	mutex_unlock(&lock);
	
	return ret;
}

/**************************************************
函数名称:acSample_llseek
功能描述:交采文件定位操作
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
loff_t acSample_llseek (struct file *filp, loff_t offset, int whence) 
{
  return filp->f_pos;
}

/**************************************************
函数名称:acSample_read
功能描述:交采文件读操作
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static ssize_t acSample_read(struct file *filp, char *buf, size_t size, loff_t *offp)
{
	 unsigned char readCommand;
   int           n;
   int           tmpData;
   unsigned char i;

 
  #ifndef AC_SAMPLE_IS_SOFT_SPI
	 char          *tmp;
   int           ret;

   readCommand = buf[0];   /*读命令*/

   tmp = kmalloc(size, GFP_KERNEL);
   
   if(!tmp)
   {
      return -1;
   }

   at91_set_gpio_value(AT91_PIN_PC5,0);
   
	 ret = spi_write_then_read(acSample_spi, &readCommand, 1, tmp, size);
	 if(ret)
	 {
		  printk(KERN_ALERT"read error @ spi_write_then_read ret=%d \n", ret);
		  kfree(tmp);
		  return -EINVAL;
	 }

	 ret = copy_to_user(buf, tmp, size);
   kfree(tmp);

   at91_set_gpio_value(AT91_PIN_PC5,1);

   return (size - ret);
   
  #else
   readCommand = buf[0];   /*读命令*/

   at91_set_gpio_value(AT91_PIN_PB3,1);     //CS=1
   at91_set_gpio_value(AT91_PIN_PB2,0);     //SCLK=0
   at91_set_gpio_value(AT91_PIN_PB3,0);     //CS=0
   
   //Send 8-bits Command to SPI
   for(n=7;n>=0;n--)
   {
     at91_set_gpio_value(AT91_PIN_PB2,1);   //SCLK=1
          
     for(i=0;i<20;i++)
     {
       if (readCommand&0x80)
       {
         at91_set_gpio_value(AT91_PIN_PB1,1); //MOSI=1
       }
       else
       {
         at91_set_gpio_value(AT91_PIN_PB1,0); //MOSI=0
       }
     }
     for(i=0;i<70;i++)
     {
       at91_set_gpio_value(AT91_PIN_PB2,1);   //SCLK=1
     }
     
     for(i=0;i<90;i++)
     {
       at91_set_gpio_value(AT91_PIN_PB2,0);   //SCLK=0
     }
     readCommand<<=1;
   }

   delayTime(1);
   
   //Read 24-bits Data From SPI
   for(n=23,tmpData=0;n>=0;n--)
   {     
     for(i=0;i<45;i++)
     {
       at91_set_gpio_value(AT91_PIN_PB2,1);   //SCLK=1
     }
     
     for(i=0;i<20;i++)
     {
       tmpData |= at91_get_gpio_value(AT91_PIN_PB0)<<(n+1);
     }
     
     for(i=0;i<90;i++)
     {
       at91_set_gpio_value(AT91_PIN_PB2,0);   //SCLK=0
     }
   }
   at91_set_gpio_value(AT91_PIN_PB3,1);     //CS=1
   
   //tmpData<<=1;
   buf[0] = tmpData>>16;
   buf[1] = tmpData>>8;
   buf[2] = tmpData&0xff;
   return (size);

  #endif 
}

/**************************************************
函数名称:acSample_write
功能描述:交采文件写操作
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static ssize_t acSample_write(struct file *flip, const char *buf, size_t size, loff_t *offp)
{
	#ifdef AC_SAMPLE_IS_SOFT_SPI
   unsigned char tmpCmd;
   unsigned int  tmpData;
   int           n;
   unsigned char i;
  
   tmpCmd = buf[0];
   tmpData = buf[1]<<16 | buf[2]<<8 | buf[3];
   
   at91_set_gpio_value(AT91_PIN_PB3,1);     //CS=1
   at91_set_gpio_value(AT91_PIN_PB2,0);     //SCLK=0
   at91_set_gpio_value(AT91_PIN_PB3,0);     //CS=0
   
   //Send 8-bits Command to SPI
   for(n=7;n>=0;n--)
   {
     at91_set_gpio_value(AT91_PIN_PB2,1);   //SCLK=1
          
     for(i=0;i<20;i++)
     {
       if (tmpCmd&0x80)
       {
         at91_set_gpio_value(AT91_PIN_PB1,1); //MOSI=1
       }
       else
       {
         at91_set_gpio_value(AT91_PIN_PB1,0); //MOSI=0
       }
     }
     for(i=0;i<70;i++)
     {
       at91_set_gpio_value(AT91_PIN_PB2,1);   //SCLK=1
     }
     
     for(i=0;i<90;i++)
     {
       at91_set_gpio_value(AT91_PIN_PB2,0);   //SCLK=0
     }
     tmpCmd<<=1;
   }

   //Send 24-bits Data to SPI
   for(n=23;n>=0;n--)
   {
     at91_set_gpio_value(AT91_PIN_PB2,1);   //SCLK=1
          
     for(i=0;i<20;i++)
     {
       if (tmpData&0x800000)
       {
         at91_set_gpio_value(AT91_PIN_PB1,1); //MOSI=1
       }
       else
       {
         at91_set_gpio_value(AT91_PIN_PB1,0); //MOSI=0
       }
     }
     for(i=0;i<70;i++)
     {
       at91_set_gpio_value(AT91_PIN_PB2,1);   //SCLK=1
     }
     
     for(i=0;i<90;i++)
     {
       at91_set_gpio_value(AT91_PIN_PB2,0);   //SCLK=0
     }
     tmpData<<=1;
   }
   at91_set_gpio_value(AT91_PIN_PB3,1);     //CS=1
   
   delayTime(5);
   
	#else
	 int ret;
	 char *tmp;
        
	 tmp = kmalloc(size+3, GFP_KERNEL);
  
   if(!tmp)
   {
     return -1;
   }
  
   if(copy_from_user(tmp+3, buf, size))
	 {
	 	 kfree(tmp);
		 return -EINVAL;
	 }
	
	 tmp[0] = buf[0];  /*命令第1字节*/
	 tmp[1] = buf[1];  /*命令第2字节*/
   tmp[2] = buf[2];  /*命令第3字节*/
   tmp[3] = buf[3];  /*命令第4字节*/  
	 ret = spi_write(acSample_spi, tmp, size);
		
	 if(ret)
	 {
		 size = -EINVAL;
   }
        
	 kfree(tmp);
  #endif
  
  return size;
}

/**************************************************
函数名称:acSample_ioctl
功能描述:交采文件I/O控制
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
int acSample_ioctl (struct inode *inode, struct file *filp, unsigned int command, unsigned long arg)
{
	 unsigned char ret=0;
	 int           i;

	 
	 switch(command)
	 {
	 	 case 1:   //READ_SIG_OUT_VALUE	 	   	
			 if (at91_get_gpio_value(AT91_PIN_PC9))  /*SIG_OUT*/
			 {
				 ret = 1;
			 }
	 	   break;

	 	 case 2:   //读取是否有交采模块
	 	   break;
	 	   	
	 	 case 3:   //复位交采芯片
	     at91_set_gpio_value(AT91_PIN_PC7,0);
       //停止100us
       for(i=0;i<0x600000;i++)
       {
     	   ;
       }     
	     at91_set_gpio_value(AT91_PIN_PC7,1);
	 	 	 break;
	 	 	  
	 	 case 4:   //设置交采模块模式
	 	 	 at91_set_gpio_value(AT91_PIN_PA27,arg);
	 	 	 break;
	 }
	 
	 return ret;
}


/**************************************************
函数名称:acSample_open
功能描述:交采文件打开时的处理
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/ 
static int acSample_open(struct inode *inode, struct file *filp)
{
   return 0;
}
 
 
/**************************************************
函数名称:acSample_release
功能描述:释放
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/ 
static int acSample_release(struct inode *inode, struct file *filp)
{
   return 0;
}

/**************************************************
函数名称:stc12_read
功能描述:stc12文件读操作
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static ssize_t ctrl_process_read(struct file *filp, char *buf, size_t size, loff_t *offp)
{
	#ifdef CTRL_PROCESS_IS_ATMEGA8L
	 char  *tmp;
	 char  readCommand;
   int   ret;
 
   tmp = kmalloc(size, GFP_KERNEL);
   
   if(!tmp)
   {
      return -1;
   }
   
   readCommand = buf[0];   /*读命令*/
	 
	 #ifdef CTRL_PROCESS_IS_ATMEGA8L
	  ret = spi_write_then_read(atmega8_spi, &readCommand, 0, tmp, size);
	 #else
	  ret = spi_write_then_read(stc12_spi, &readCommand, 0, tmp, size);
	 #endif
	 
	 if(ret)
	 {
		  printk(KERN_ALERT"read error @ spi_write_then_read ret=%d \n", ret);
		  kfree(tmp);
		  return -EINVAL;
	 }

	 ret = copy_to_user(buf, tmp, size);
   kfree(tmp);
  #endif
  
   return (size - ret);
}

/**************************************************
函数名称:stc12_write
功能描述:stc12文件写操作
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static ssize_t ctrl_process_write(struct file *flip, const char *buf, size_t size, loff_t *offp)
{
	#ifdef CTRL_PROCESS_IS_ATMEGA8L
   unsigned char n,m;
   unsigned int  i;
   unsigned char tmpData;
   
   at91_set_gpio_value(AT91_PIN_PC11,1);     //CS=1
   at91_set_gpio_value(AT91_PIN_PA1,0);      //MOSI=0
   at91_set_gpio_value(AT91_PIN_PA2,0);      //SCLK=0
   at91_set_gpio_value(AT91_PIN_PC11,0);     //CS=0   
   
   for(m=0;m<size;m++)
   {     
     tmpData = *buf++;
     
     //Send 8-bits Command to SPI
     for(n=0;n<8;n++)
     {
       at91_set_gpio_value(AT91_PIN_PA2,0);   //SCLK=0
          
       if (tmpData&0x80)
       {
         at91_set_gpio_value(AT91_PIN_PA1,1); //MOSI=1
       }
       else
       {
         at91_set_gpio_value(AT91_PIN_PA1,0); //MOSI=0
       }
       delayTime(1);
       at91_set_gpio_value(AT91_PIN_PA2,1);   //SCLK=1
       delayTime(1);
       
       tmpData<<=1;
     }
   }
   
   at91_set_gpio_value(AT91_PIN_PA1,0);         //MOSI=0
   at91_set_gpio_value(AT91_PIN_PA2,0);         //SCLK=0

   at91_set_gpio_value(AT91_PIN_PC11, 1);       //CS=1
   
	#else
	 int ret;
	 char *tmp;
        
	 tmp = kmalloc(size, GFP_KERNEL);
  
   if(!tmp)
   {
     return -1;
   }
  
   if(copy_from_user(tmp, buf, size))
	 {
		 kfree(tmp);
		 return -EINVAL;
	 }	
	
	 #ifdef CTRL_PROCESS_IS_ATMEGA8L
	  ret = spi_write(atmega8_spi, tmp, size);
	 #else
	  ret = spi_write(stc12_spi, tmp, size);
	 #endif
	
	 if(ret)
	 {
		 size = -EINVAL;
   }
        
	 kfree(tmp);
  #endif
  
  return size;
}

/**************************************************
函数名称:stc12_ioctl
功能描述:stc12文件I/O控制
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
int ctrl_process_ioctl (struct inode *inode, struct file *filp, unsigned int command, unsigned long arg)
{
	 //at91_set_gpio_value(AT91_PIN_PC11,arg);

	 return 0;
}

/**************************************************
函数名称:stc12_release
功能描述:释放
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/ 
static int ctrl_process_release(struct inode *inode, struct file *filp)
{
   return 0;
}

/*交采SPI接口字符驱动 文件操作结构赋值*/
static struct file_operations acSample_fops = {
        .owner  = THIS_MODULE,
        .llseek = acSample_llseek,
        .read   = acSample_read,
        .write  = acSample_write,
        .ioctl  = acSample_ioctl,
        .open   = acSample_open,
        .release= acSample_release,
};

/*单片机(STC12)通信SPI接口字符驱动 文件操作结构赋值*/
static struct file_operations ctrl_process_fops = {
        .owner  = THIS_MODULE,
        .read   = ctrl_process_read,
        .write  = ctrl_process_write,
        .ioctl  = ctrl_process_ioctl,
        .release= ctrl_process_release,
};


/**************************************************
函数名称:spi_acSample_probe
功能描述:注册SPI交采字符驱动
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static int spi_acSample_probe(struct spi_device *spi)
{
	if (!(acSample_spi = kzalloc(sizeof *acSample_spi, GFP_KERNEL)))
	{
     goto fail;
	}
	
	acSample_spi = spi_dev_get(spi);
  printk(KERN_ALERT"name:%s  CS=%d  mode=%x speed=%d.\n", \
	  acSample_spi->modalias, acSample_spi->chip_select, acSample_spi->mode, acSample_spi->max_speed_hz);
	
	return 0;

fail:
  printk(KERN_ALERT"spi_acSample_probe ERROR \n");
  kfree(acSample_spi);
  
  return -1;
}

/**************************************************
函数名称:
功能描述:
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static int spi_acSample_remove(struct spi_device *spi)
{
	return 0;
}

/**************************************************
函数名称:spi_stc12_probe
功能描述:注册SPI单片机通信字符驱动
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static int spi_ctrl_process_probe(struct spi_device *spi)
{
	#ifdef CTRL_PROCESS_IS_ATMEGA8L
	 if (!(atmega8_spi = kzalloc(sizeof *atmega8_spi, GFP_KERNEL)))
	 {
     goto fail;
	 }
	 
	 atmega8_spi = spi_dev_get(spi);
   printk(KERN_ALERT"name:%s  CS=%d  mode=%x speed=%d.\n", \
	   atmega8_spi->modalias, atmega8_spi->chip_select, atmega8_spi->mode, atmega8_spi->max_speed_hz);
	#else
	 if (!(stc12_spi = kzalloc(sizeof *stc12_spi, GFP_KERNEL)))
	 {
     goto fail;
	 }
	 
	 stc12_spi = spi_dev_get(spi);
   printk(KERN_ALERT"name:%s  CS=%d  mode=%x speed=%d.\n", \
	   stc12_spi->modalias, stc12_spi->chip_select, stc12_spi->mode, stc12_spi->max_speed_hz);
	#endif
	
	return 0;

fail:
  printk(KERN_ALERT"spi_stc12_probe ERROR \n");
  kfree(acSample_spi);
  
  return -1;
}

/**************************************************
函数名称:
功能描述:
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static int spi_ctrl_process_remove(struct spi_device *spi)
{
	return 0;
}

//交流采样SPI字符驱动结构
static struct spi_driver spi_acSample_driver = {
   .driver = 
   {
      .name        = "spi1_acSample",
      .owner       = THIS_MODULE,
   },
   .probe          = spi_acSample_probe,
   .remove         = spi_acSample_remove,
};

//单片机(STC12)SPI字符驱动结构
#ifdef CTRL_PROCESS_IS_ATMEGA8L
 static struct spi_driver spi_atmega8_driver = {
   .driver = 
   {
      .name        = "spi0_atmega8",
      .owner       = THIS_MODULE,
   },
   .probe          = spi_ctrl_process_probe,
   .remove         = spi_ctrl_process_remove,
 };
#else
 static struct spi_driver spi_stc12_driver = {
   .driver = 
   {
      .name        = "spi1_stc12",
      .owner       = THIS_MODULE,
   },
   .probe          = spi_ctrl_process_probe,
   .remove         = spi_ctrl_process_remove,
 };
#endif

static void acSample_setup_cdev(void)
{
	 int error;
	 int devno, tmpMinor=0;
	 
   devno = MKDEV (acSample_major, tmpMinor);
   
   cdev_init (&acSampleCdev, &acSample_fops);
   
   acSampleCdev.owner = THIS_MODULE;
   acSampleCdev.ops = &acSample_fops;
   error = cdev_add(&acSampleCdev, devno,1);

   if(error)
   {
     printk(KERN_NOTICE "Error %d adding acSample_setup_cdev",error);
   }
}

static void ctrl_process_setup_cdev(void)
{
	 int error;
	 int devno, tmpMinor=0;
	 
   #ifdef CTRL_PROCESS_IS_ATMEGA8L
    devno = MKDEV (atmega8_major, tmpMinor);
   
    cdev_init (&atmega8Cdev, &ctrl_process_fops);
   
    atmega8Cdev.owner = THIS_MODULE;
    atmega8Cdev.ops = &ctrl_process_fops;
    error = cdev_add(&atmega8Cdev, devno,1);
   #else
    devno = MKDEV (stc12_major, tmpMinor);
   
    cdev_init (&stc12Cdev, &ctrl_process_fops);
   
    stc12Cdev.owner = THIS_MODULE;
    stc12Cdev.ops = &ctrl_process_fops;
    error = cdev_add(&stc12Cdev, devno,1);
   #endif

   if(error)
   {
     printk(KERN_NOTICE "Error %d adding ctrl_process_setup_cdev",error);
   }
}


/**************************************************
函数名称:secondSpi1_init
功能描述:SPI1字符驱动(交采,单片机(stc12)通信)初始化
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值:
***************************************************/
static int __init secondSpi_init(void)
{
	int    ret;
	dev_t  devNo;     /*设备号*/

#if 1
	#ifndef AC_SAMPLE_IS_SOFT_SPI
	 /*第1步:*/
	 /*使能SPI1时钟(enable spi1 clock),*/
	 /*也可以不做这一步,因为在内核里SPI驱动已经加了使能SPI的命令*/
	 pmc_base = ioremap(PMC_BASE, 512);
	 __raw_writel(1<<13, pmc_base + PMC_PCER);
	 iounmap(pmc_base);

	 /*第2步:*/
	 /*设置I/O脚(setup gpio)*/
	 /*也可以不做这一步,因为在内核里SPI驱动已经配置了SPI的管脚*/	
	 at91_set_A_periph(AT91_PIN_PB0, 0);		/*SPI1_MISO*/
	 at91_set_A_periph(AT91_PIN_PB1, 0);		/*SPI1_MOSI*/
	 at91_set_A_periph(AT91_PIN_PB2, 0);		/*SPI1_SPCK*/
	 at91_set_A_periph(AT91_PIN_PB3, 1);		/*SPI1_NPCS0*/
	 //at91_set_B_periph(AT91_PIN_PC5, 1);		/*SPI1_NPCS1*/	
	 at91_set_gpio_output(AT91_PIN_PC5, 1);	

   /*第3步:*/
   /*这一步可以更改SPI的参数,比如说数据位数、时钟、相位、速率等*/
	 /* setup spi1 */
	 spi_base = ioremap(SPI_BASE, 512);
	
	 /* reset spi1 */
	 __raw_writel(1<<7, spi_base + SPI_CR);
	
	 /* config MR(Mode Register,模式寄存器)*/
	 __raw_writel(1 | 1<<4 | 0xF<<16, spi_base + SPI_MR);
	
	 /* config CSR0(Chip Select Register 0)*/
	 __raw_writel(0x0101ff00, spi_base + SPI_CSR0);

	 /* config CSR1(Chip Select Register 0)*/
	 __raw_writel(0x00001900, spi_base + SPI_CSR1);
	 
	 /*使能SPI1(spi1 enable,Control Register[控制寄存器])*/
	 __raw_writel(1, spi_base + SPI_CR);
  
   iounmap(spi_base);
   
  #else
   //定义SPI1模拟管脚   
   at91_set_gpio_output(AT91_PIN_PB2, 0);   //SPCK输出
   at91_set_gpio_output(AT91_PIN_PB3, 1);   //NCPS0输出
   at91_set_gpio_output(AT91_PIN_PB1, 0);   //MOSI输出
   at91_set_gpio_input(AT91_PIN_PB0,  0);   //MISO输入
  #endif

  //*********************************************************************
	
	#ifdef CTRL_PROCESS_IS_ATMEGA8L
   
   //定义SPI1模拟管脚   
   at91_set_gpio_output(AT91_PIN_PA2,  0);  //SPCK输出
   at91_set_gpio_output(AT91_PIN_PC11, 1);  //NCPS1输出
   at91_set_gpio_output(AT91_PIN_PA1,  0);  //MOSI输出
   at91_set_gpio_input(AT91_PIN_PA0,   0);  //MISO输入
   
	#else
	 at91_set_A_periph(AT91_PIN_PA0,  0);		/*SPI0_MISO*/
	 at91_set_A_periph(AT91_PIN_PA1,  0);		/*SPI0_MOSI*/
	 at91_set_A_periph(AT91_PIN_PA2,  0);		/*SPI0_SPCK*/
	 at91_set_B_periph(AT91_PIN_PC11, 1);		/*SPI0_NPCS1*/

	 //SPI0配置
	 /* setup spi0 */
	 spi_base = ioremap(SPI_BASE_SPI0, 512);
	
	 /* reset spi0 */
	 //__raw_writel(1<<7, spi_base + SPI_CR);
	
	 /* config MR(Mode Register,模式寄存器)*/
	 //__raw_writel(1 | 1<<4 | 0xF<<16, spi_base + SPI_MR);
	 
	 /* config CSR1(Chip Select Register 0)*/
	 //__raw_writel(0x0101ff02, spi_base + SPI_CSR1);

	 /*使能SPI0(spi0 enable,Control Register[控制寄存器])*/
	 //__raw_writel(1, spi_base + SPI_CR);
	 iounmap(spi_base);
  #endif
	
#endif
  
	/*********************交采SPI驱动****开始********************************************************/
	/*复位ATT7022B IO脚定义为输出*/
	if (at91_set_gpio_output(AT91_PIN_PC7, 0)==0)
	{
	   printk(KERN_ALERT"Reset ATT7022B...\n");
     
     //停止100us
     for(ret=0;ret<0x600000;ret++)
     {
     	  ;
     }
     
	   at91_set_gpio_value(AT91_PIN_PC7,1);
	}
	else
	{
	   printk(KERN_ALERT "set AT91_PIN_PC7 failed!\n");
	}

	/*ATT7022B模式设置脚定义为输出*/
	if (at91_set_gpio_output(AT91_PIN_PA27, 0)==0)
	{
		;
	}
	else
	{
	  printk(KERN_ALERT "set AT91_PIN_PA27 failed!\n");
	}
	
	/*ATT7022B的信号脚IO配置为输入*/
	if (at91_set_gpio_input(AT91_PIN_PC9, 0)==0)
	{
	  ;
	}
	else
	{
	  printk(KERN_ALERT "set AT91_PIN_PC9 failed!\n");
	}
	
	/*注册驱动*/
	ret=spi_register_driver(&spi_acSample_driver);
	if(ret)
	{
		spi_unregister_driver(&spi_acSample_driver);
		return -1;
	}
 
  /*自动分配一个设备号*/
	if (alloc_chrdev_region(&devNo,0,1,"acSample") < 0)
	{
		 return -2;
	}
	
	acSample_major= MAJOR(devNo);

  acSample_setup_cdev();
  
  /* create your own class under /sysfs */
  classAcSample = class_create(THIS_MODULE, "acSample");
  if(IS_ERR(classAcSample))
  {
    printk("Power Measure Err: failed in creating class.\n");
    return -1;
  }

  device_create(classAcSample, NULL, MKDEV(acSample_major, 0), "acSample");

	/*********************交采SPI驱动****结束********************************************************/
  
 #ifdef CTRL_PROCESS_IS_ATMEGA8L
	/*********************单片机(ATMEGA8L)通信SPI驱动****开始********************************************************/	
	/*注册驱动*/
	ret=spi_register_driver(&spi_atmega8_driver);
	if(ret)
	{
		spi_unregister_driver(&spi_atmega8_driver);
		return -1;
	}
 
  /*自动分配一个设备号*/
	if (alloc_chrdev_region(&devNo,0,1,"atmega8") < 0)
	{
		 return -2;
	}
	
	atmega8_major= MAJOR(devNo);

  ctrl_process_setup_cdev();
  
  /* create your own class under /sysfs */
  classAtmega8 = class_create(THIS_MODULE, "atmega8");
  if(IS_ERR(classAtmega8))
  {
    printk("Atmega8l Err: failed in creating class.\n");
    return -1;
  }

  device_create(classAtmega8, NULL, MKDEV(atmega8_major, 0), "atmega8");
	/*********************单片机ATMEGA8L SPI驱动****结束********************************************************/

 #else    //控制板单片机是STC12
	
	/*********************单片机(stc12)通信SPI驱动****开始********************************************************/	
	/*注册驱动*/
	ret=spi_register_driver(&spi_stc12_driver);
	if(ret)
	{
		spi_unregister_driver(&spi_stc12_driver);
		return -1;
	}
 
  /*自动分配一个设备号*/
	if (alloc_chrdev_region(&devNo,0,1,"stc12") < 0)
	{
		 return -2;
	}
	
	stc12_major= MAJOR(devNo);

  stc12_setup_cdev();
  
  /* create your own class under /sysfs */
  classStc12 = class_create(THIS_MODULE, "stc12");
  if(IS_ERR(classStc12))
  {
    printk("STC12 Err: failed in creating class.\n");
    return -1;
  }

  device_create(classStc12, NULL, MKDEV(stc12_major, 0), "stc12");
	/*********************单片机SPI驱动****结束********************************************************/

 #endif

	printk(KERN_ALERT"SPI Second char driver init Done!\n");
 	
	return 0;
}

/**************************************************
函数名称:secondSpi1_exit
功能描述:退出SPI1字符驱动(交流采样,与单片机通信(stc12))
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值:void
***************************************************/
static void __exit secondSpi_exit(void)
{
	 dev_t devno;
	 
	 spi_unregister_driver(&spi_acSample_driver);
	 
   devno = MKDEV (acSample_major, 0);
   
   cdev_del (&acSampleCdev);
      
   //delete device node under /dev
   device_destroy(classAcSample, MKDEV(acSample_major, 0)); 

   //delete class created by us
   class_destroy(classAcSample);

   unregister_chrdev_region (devno, 1);	 

	 #ifdef CTRL_PROCESS_IS_ATMEGA8L
	  spi_unregister_driver(&spi_atmega8_driver);
	 
    devno = MKDEV (atmega8_major, 0);
   
    cdev_del (&atmega8Cdev);
      
    //delete device node under /dev
    device_destroy(classAtmega8, MKDEV(atmega8_major, 0)); 

    //delete class created by us
    class_destroy(classAtmega8);

    unregister_chrdev_region (devno, 1);
   #else
	  spi_unregister_driver(&spi_stc12_driver);
	 
    devno = MKDEV (stc12_major, 0);
   
    cdev_del (&stc12Cdev);
      
    //delete device node under /dev
    device_destroy(classStc12, MKDEV(stc12_major, 0)); 

    //delete class created by us
    class_destroy(classStc12);

    unregister_chrdev_region (devno, 1);
   #endif
}

module_init(secondSpi_init);
module_exit(secondSpi_exit);

