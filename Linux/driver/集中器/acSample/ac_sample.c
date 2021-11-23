/***************************************************
Copyright,2009,Huawei WoDian co.,LTD,All	Rights Reserved
文件名:spi_acSample.c
作者:leiyong
版本:0.9
完成日期:2009年12月
描述:电力终端(负控终端/集中器,AT91SAM9260处理器)SPI接口交流采样字符驱动文件
函数列表:

修改历史:
  01,09-12-14,Leiyong created.

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

//for pmc
#define PMC_BASE	0xFFFFFC00
#define PMC_PCER	0x0010
#define PMC_PCSR	0x0018

//for spi
//#define SPI_BASE	0xFFFC8000 //spi0
#define SPI_BASE	0xFFFCC000 //spi1
#define SPI_CR		0x0000
#define SPI_MR		0x0004
#define SPI_SR		0x0010
#define SPI_TDR		0x000C
#define SPI_CSR0	0x0030

MODULE_DESCRIPTION("ac sample device driver @spi");
MODULE_AUTHOR("leiyong");
MODULE_LICENSE("GPL");

struct spi_device    *acSample_spi;     //交采SPI变量
static void __iomem  *pmc_base, *spi_base;
static int           acSample_major=0;

struct cdev          acSampleCdev;
struct class         *classAcSample;

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
	 char  *tmp;
	 char  readCommand;
   int   ret;
 
   tmp = kmalloc(size, GFP_KERNEL);
   
   if(!tmp)
   {
      return -1;
   }
   
   readCommand = buf[0];   /*读命令*/
	 ret = spi_write_then_read(acSample_spi, &readCommand, 1, tmp, size);
	 if(ret)
	 {
		  printk(KERN_ALERT"read error @ spi_write_then_read ret=%d \n", ret);
		  kfree(tmp);
		  return -EINVAL;
	 }

	 ret = copy_to_user(buf, tmp, size);
   kfree(tmp);
 
   return (size - ret);
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
			 if (at91_get_gpio_value(AT91_PIN_PA0))  /*SIG_OUT*/
			 {
				 ret = 1;
			 }
	 	   break;

	 	 case 2:   //读取是否有交采模块
			 if (at91_get_gpio_value(AT91_PIN_PA1))  /*SIG_OUT*/
			 {
				 ret = 1;
			 }
	 	   break;
	 	   	
	 	 case 3:   //复位交采芯片
	     at91_set_gpio_value(AT91_PIN_PA2,0);
       //停止100us
       for(i=0;i<0x600000;i++)
       {
     	   ;
       }     
	     at91_set_gpio_value(AT91_PIN_PA2,1);
	 	 	 break;
	 	 	  
	 	 case 4:   //设置交采模块模式
	 	 	 at91_set_gpio_value(AT91_PIN_PA3,arg);
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

//交流采样SPI字符驱动结构
static struct spi_driver spi_acSample_driver = {
   .driver = 
   {
      .name        = "spi_acSample",
      .owner       = THIS_MODULE,
   },
   .probe          = spi_acSample_probe,
   .remove         = spi_acSample_remove,
};

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

/**************************************************
函数名称:spi_acSample_init
功能描述:交流采样SPI字符驱动初始化
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值:
***************************************************/
static int __init spi_acSample_init(void)
{
	int    ret;
	dev_t  devNo;     /*设备号*/
  int    result;

	/*复位ATT7022B IO脚定义为输出*/
	if (at91_set_gpio_output(AT91_PIN_PA2, 0)==0)
	{
	   printk(KERN_ALERT"Reset ATT7022B...\n");
     
     //停止100us
     for(ret=0;ret<0x600000;ret++)
     {
     	  ;
     }
     
	   at91_set_gpio_value(AT91_PIN_PA2,1);
	}
	else
	{
	   printk(KERN_ALERT "set AT91_PIN_PA2 failed!\n");
	}
  
  /*模式选择IO脚定义为输出(高-三相四线)*/
	if (at91_set_gpio_output(AT91_PIN_PA3, 1)==0)
	{
		 ;
	}
	else
	{
	   printk(KERN_ALERT "set AT91_PIN_PA3 failed!\n");
	}
	
	/*ATT7022B的信号脚IO配置为输入*/
	if (at91_set_gpio_input(AT91_PIN_PA0, 0)==0)
	{
	   ;
	}
	else
	{
	   printk(KERN_ALERT "set AT91_PIN_PA0 failed!\n");
	}

	/*有/无交采模块检测脚IO配置为输入*/
	if (at91_set_gpio_input(AT91_PIN_PA1, 0)==0)
	{
	   ;
	}
	else
	{
	   printk(KERN_ALERT "set AT91_PIN_PA1 failed!\n");
	}

	/*第1步:*/
	/*使能SPI0时钟(enable spi0 clock),*/
	/*也可以不做这一步,因为在内核里SPI驱动已经加了使能SPI的命令 ly,2009-12-14*/
	pmc_base = ioremap(PMC_BASE, 512);
	__raw_writel(1<<12, pmc_base + PMC_PCER);
	iounmap(pmc_base);

	/*第2步:*/
	/*设置I/O脚(setup gpio)*/
	/*也可以不做这一步,因为在内核里SPI驱动已经配置了SPI的管脚 ly,2009-12-14*/
	
	//at91_set_A_periph(AT91_PIN_PA0, 1);		/*SPI0_MISO*/
	//at91_set_A_periph(AT91_PIN_PA1, 1);		/*SPI0_MOSI*/
	//at91_set_A_periph(AT91_PIN_PA2, 1);		/*SPI0_SPCK*/
	//at91_set_A_periph(AT91_PIN_PA3, 1);		/*SPI0_NPCS0*/

  /*第3步:*/
  /*这一步可以更改SPI的参数,比如说数据位数、时钟、相位、速率等*/
#if 1
	/* setup spi0 */
	spi_base = ioremap(SPI_BASE, 512);
	
	/* reset spi0 */
	__raw_writel(1<<7, spi_base + SPI_CR);
	
	/* config MR(Mode Register,模式寄存器)*/
	__raw_writel(1 | 1<<4 | 0xF<<16, spi_base + SPI_MR);
	
	/* config CSR0(Chip Select Register 0)*/
	__raw_writel(0x00001900, spi_base + SPI_CSR0);
	
	/*使能SPI0(spi0 enable,Control Register[控制寄存器])*/
	__raw_writel(1, spi_base + SPI_CR);

	iounmap(spi_base);
#endif
  
  /*第4步:注册驱动*/
	ret=spi_register_driver(&spi_acSample_driver);
	if(ret)
	{
		spi_unregister_driver(&spi_acSample_driver);
		return -1;
	}

	/*
	acSample_major = register_chrdev(0,"acSample", &acSample_fops);
  if(acSample_major < 0)
  {
     printk(KERN_INFO "acSample:can't get major number\n\r");
     
     return acSample_major;
  }
  */
  
  /*自动分配一个设备号*/
	if (alloc_chrdev_region(&devNo,0,1,"acSample") < 0)
	{
		 return -2;
	}
	
	acSample_major= MAJOR(devNo);

  //result=register_chrdev_region(acSample_major,1,"acSample");
  //if(result<0)
  //{
  //  printk(KERN_WARNING "acSample: can't get major number %d\n", acSample_major);
  //  return result;
  //}

  acSample_setup_cdev();
  
  /* create your own class under /sysfs */
  classAcSample = class_create(THIS_MODULE, "acSample");
  if(IS_ERR(classAcSample))
  {
    printk("AC Sample Err: failed in creating class.\n");
    return -1;
  }

  device_create(classAcSample, NULL, MKDEV(acSample_major, 0), "acSample");

	printk(KERN_ALERT"ac sample char driver init Done!\n");
	
	return 0;
}

/**************************************************
函数名称:spi_acSample_exit
功能描述:退出交流采样SPI字符驱动
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值:void
***************************************************/
static void __exit spi_acSample_exit(void)
{
	 dev_t devno;
	 
	 spi_unregister_driver(&spi_acSample_driver);
	 
	 //unregister_chrdev(acSample_major, "acSample");
   
   devno = MKDEV (acSample_major, 0);
   
   cdev_del (&acSampleCdev);
      
   //delete device node under /dev
   device_destroy(classAcSample, MKDEV(acSample_major, 0)); 

   //delete class created by us
   class_destroy(classAcSample);

   unregister_chrdev_region (devno, 1);	 
}

module_init(spi_acSample_init);
module_exit(spi_acSample_exit);

