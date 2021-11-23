/***************************************************
Copyright,2009,Huawei WoDian co.,LTD,All	Rights Reserved
�ļ���:spi_acSample.c
����:leiyong
�汾:0.9
�������:2009��12��
����:�����ն�(�����ն�/������,AT91SAM9260������)SPI�ӿڽ��������ַ������ļ�
�����б�:

�޸���ʷ:
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

struct spi_device    *acSample_spi;     //����SPI����
static void __iomem  *pmc_base, *spi_base;
static int           acSample_major=0;

struct cdev          acSampleCdev;
struct class         *classAcSample;

/**************************************************
��������:spi_read_my(Ŀǰû����������� 09.12.20)
��������:
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
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
��������:acSample_llseek
��������:�����ļ���λ����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/
loff_t acSample_llseek (struct file *filp, loff_t offset, int whence) 
{
  return filp->f_pos;
}

/**************************************************
��������:acSample_read
��������:�����ļ�������
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
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
   
   readCommand = buf[0];   /*������*/
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
��������:acSample_write
��������:�����ļ�д����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
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
	
	tmp[0] = buf[0];  /*�����1�ֽ�*/
	tmp[1] = buf[1];  /*�����2�ֽ�*/
  tmp[2] = buf[2];  /*�����3�ֽ�*/
  tmp[3] = buf[3];  /*�����4�ֽ�*/  
	ret = spi_write(acSample_spi, tmp, size);
	
	if(ret)
	{
		 size = -EINVAL;
  }
        
	kfree(tmp);
 
  return size;
}

/**************************************************
��������:acSample_ioctl
��������:�����ļ�I/O����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
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

	 	 case 2:   //��ȡ�Ƿ��н���ģ��
			 if (at91_get_gpio_value(AT91_PIN_PA1))  /*SIG_OUT*/
			 {
				 ret = 1;
			 }
	 	   break;
	 	   	
	 	 case 3:   //��λ����оƬ
	     at91_set_gpio_value(AT91_PIN_PA2,0);
       //ֹͣ100us
       for(i=0;i<0x600000;i++)
       {
     	   ;
       }     
	     at91_set_gpio_value(AT91_PIN_PA2,1);
	 	 	 break;
	 	 	  
	 	 case 4:   //���ý���ģ��ģʽ
	 	 	 at91_set_gpio_value(AT91_PIN_PA3,arg);
	 	 	 break;
	 }
	 
	 return ret;
}


/**************************************************
��������:acSample_open
��������:�����ļ���ʱ�Ĵ���
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/ 
static int acSample_open(struct inode *inode, struct file *filp)
{
   return 0;
}
 
 
/**************************************************
��������:acSample_release
��������:�ͷ�
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/ 
static int acSample_release(struct inode *inode, struct file *filp)
{
   return 0;
}

/*����SPI�ӿ��ַ����� �ļ������ṹ��ֵ*/
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
��������:spi_acSample_probe
��������:ע��SPI�����ַ�����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
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
��������:
��������:
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/
static int spi_acSample_remove(struct spi_device *spi)
{
	return 0;
}

//��������SPI�ַ������ṹ
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
��������:spi_acSample_init
��������:��������SPI�ַ�������ʼ��
���ú���:
�����ú���:
�������:��
�������:
����ֵ:
***************************************************/
static int __init spi_acSample_init(void)
{
	int    ret;
	dev_t  devNo;     /*�豸��*/
  int    result;

	/*��λATT7022B IO�Ŷ���Ϊ���*/
	if (at91_set_gpio_output(AT91_PIN_PA2, 0)==0)
	{
	   printk(KERN_ALERT"Reset ATT7022B...\n");
     
     //ֹͣ100us
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
  
  /*ģʽѡ��IO�Ŷ���Ϊ���(��-��������)*/
	if (at91_set_gpio_output(AT91_PIN_PA3, 1)==0)
	{
		 ;
	}
	else
	{
	   printk(KERN_ALERT "set AT91_PIN_PA3 failed!\n");
	}
	
	/*ATT7022B���źŽ�IO����Ϊ����*/
	if (at91_set_gpio_input(AT91_PIN_PA0, 0)==0)
	{
	   ;
	}
	else
	{
	   printk(KERN_ALERT "set AT91_PIN_PA0 failed!\n");
	}

	/*��/�޽���ģ�����IO����Ϊ����*/
	if (at91_set_gpio_input(AT91_PIN_PA1, 0)==0)
	{
	   ;
	}
	else
	{
	   printk(KERN_ALERT "set AT91_PIN_PA1 failed!\n");
	}

	/*��1��:*/
	/*ʹ��SPI0ʱ��(enable spi0 clock),*/
	/*Ҳ���Բ�����һ��,��Ϊ���ں���SPI�����Ѿ�����ʹ��SPI������ ly,2009-12-14*/
	pmc_base = ioremap(PMC_BASE, 512);
	__raw_writel(1<<12, pmc_base + PMC_PCER);
	iounmap(pmc_base);

	/*��2��:*/
	/*����I/O��(setup gpio)*/
	/*Ҳ���Բ�����һ��,��Ϊ���ں���SPI�����Ѿ�������SPI�Ĺܽ� ly,2009-12-14*/
	
	//at91_set_A_periph(AT91_PIN_PA0, 1);		/*SPI0_MISO*/
	//at91_set_A_periph(AT91_PIN_PA1, 1);		/*SPI0_MOSI*/
	//at91_set_A_periph(AT91_PIN_PA2, 1);		/*SPI0_SPCK*/
	//at91_set_A_periph(AT91_PIN_PA3, 1);		/*SPI0_NPCS0*/

  /*��3��:*/
  /*��һ�����Ը���SPI�Ĳ���,����˵����λ����ʱ�ӡ���λ�����ʵ�*/
#if 1
	/* setup spi0 */
	spi_base = ioremap(SPI_BASE, 512);
	
	/* reset spi0 */
	__raw_writel(1<<7, spi_base + SPI_CR);
	
	/* config MR(Mode Register,ģʽ�Ĵ���)*/
	__raw_writel(1 | 1<<4 | 0xF<<16, spi_base + SPI_MR);
	
	/* config CSR0(Chip Select Register 0)*/
	__raw_writel(0x00001900, spi_base + SPI_CSR0);
	
	/*ʹ��SPI0(spi0 enable,Control Register[���ƼĴ���])*/
	__raw_writel(1, spi_base + SPI_CR);

	iounmap(spi_base);
#endif
  
  /*��4��:ע������*/
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
  
  /*�Զ�����һ���豸��*/
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
��������:spi_acSample_exit
��������:�˳���������SPI�ַ�����
���ú���:
�����ú���:
�������:��
�������:
����ֵ:void
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

