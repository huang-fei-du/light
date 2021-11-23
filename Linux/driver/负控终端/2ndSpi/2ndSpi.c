/***************************************************
Copyright ly,2010,All	Rights Reserved
�ļ���:2ndSpi.c
����:Leiyong
�汾:0.9
�������:2010��04��
����:�����ն�(�����ն�/������,AT91SAM9260������)SPI�ӿ�(������������Ƭ��ͨ��)�ַ������ļ�
�����б�:

�޸���ʷ:
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

#define AC_SAMPLE_IS_SOFT_SPI     //���������ģ��SPI�ӿ�
#define CTRL_PROCESS_IS_ATMEGA8L  //���ư嵥Ƭ����ATMEGA8L

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

//����SPI����
struct spi_device    *acSample_spi;
static int           acSample_major=0;
struct cdev          acSampleCdev;
struct class         *classAcSample;

#ifdef CTRL_PROCESS_IS_ATMEGA8L
 //stc12  SPI����
 struct spi_device   *atmega8_spi;
 static int          atmega8_major=0;
 struct cdev         atmega8Cdev;
 struct class        *classAtmega8;
#else
 //stc12  SPI����
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
	 unsigned char readCommand;
   int           n;
   int           tmpData;
   unsigned char i;

 
  #ifndef AC_SAMPLE_IS_SOFT_SPI
	 char          *tmp;
   int           ret;

   readCommand = buf[0];   /*������*/

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
   readCommand = buf[0];   /*������*/

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
  #endif
  
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
			 if (at91_get_gpio_value(AT91_PIN_PC9))  /*SIG_OUT*/
			 {
				 ret = 1;
			 }
	 	   break;

	 	 case 2:   //��ȡ�Ƿ��н���ģ��
	 	   break;
	 	   	
	 	 case 3:   //��λ����оƬ
	     at91_set_gpio_value(AT91_PIN_PC7,0);
       //ֹͣ100us
       for(i=0;i<0x600000;i++)
       {
     	   ;
       }     
	     at91_set_gpio_value(AT91_PIN_PC7,1);
	 	 	 break;
	 	 	  
	 	 case 4:   //���ý���ģ��ģʽ
	 	 	 at91_set_gpio_value(AT91_PIN_PA27,arg);
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

/**************************************************
��������:stc12_read
��������:stc12�ļ�������
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
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
   
   readCommand = buf[0];   /*������*/
	 
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
��������:stc12_write
��������:stc12�ļ�д����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
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
��������:stc12_ioctl
��������:stc12�ļ�I/O����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/
int ctrl_process_ioctl (struct inode *inode, struct file *filp, unsigned int command, unsigned long arg)
{
	 //at91_set_gpio_value(AT91_PIN_PC11,arg);

	 return 0;
}

/**************************************************
��������:stc12_release
��������:�ͷ�
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/ 
static int ctrl_process_release(struct inode *inode, struct file *filp)
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

/*��Ƭ��(STC12)ͨ��SPI�ӿ��ַ����� �ļ������ṹ��ֵ*/
static struct file_operations ctrl_process_fops = {
        .owner  = THIS_MODULE,
        .read   = ctrl_process_read,
        .write  = ctrl_process_write,
        .ioctl  = ctrl_process_ioctl,
        .release= ctrl_process_release,
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

/**************************************************
��������:spi_stc12_probe
��������:ע��SPI��Ƭ��ͨ���ַ�����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
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
��������:
��������:
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/
static int spi_ctrl_process_remove(struct spi_device *spi)
{
	return 0;
}

//��������SPI�ַ������ṹ
static struct spi_driver spi_acSample_driver = {
   .driver = 
   {
      .name        = "spi1_acSample",
      .owner       = THIS_MODULE,
   },
   .probe          = spi_acSample_probe,
   .remove         = spi_acSample_remove,
};

//��Ƭ��(STC12)SPI�ַ������ṹ
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
��������:secondSpi1_init
��������:SPI1�ַ�����(����,��Ƭ��(stc12)ͨ��)��ʼ��
���ú���:
�����ú���:
�������:��
�������:
����ֵ:
***************************************************/
static int __init secondSpi_init(void)
{
	int    ret;
	dev_t  devNo;     /*�豸��*/

#if 1
	#ifndef AC_SAMPLE_IS_SOFT_SPI
	 /*��1��:*/
	 /*ʹ��SPI1ʱ��(enable spi1 clock),*/
	 /*Ҳ���Բ�����һ��,��Ϊ���ں���SPI�����Ѿ�����ʹ��SPI������*/
	 pmc_base = ioremap(PMC_BASE, 512);
	 __raw_writel(1<<13, pmc_base + PMC_PCER);
	 iounmap(pmc_base);

	 /*��2��:*/
	 /*����I/O��(setup gpio)*/
	 /*Ҳ���Բ�����һ��,��Ϊ���ں���SPI�����Ѿ�������SPI�Ĺܽ�*/	
	 at91_set_A_periph(AT91_PIN_PB0, 0);		/*SPI1_MISO*/
	 at91_set_A_periph(AT91_PIN_PB1, 0);		/*SPI1_MOSI*/
	 at91_set_A_periph(AT91_PIN_PB2, 0);		/*SPI1_SPCK*/
	 at91_set_A_periph(AT91_PIN_PB3, 1);		/*SPI1_NPCS0*/
	 //at91_set_B_periph(AT91_PIN_PC5, 1);		/*SPI1_NPCS1*/	
	 at91_set_gpio_output(AT91_PIN_PC5, 1);	

   /*��3��:*/
   /*��һ�����Ը���SPI�Ĳ���,����˵����λ����ʱ�ӡ���λ�����ʵ�*/
	 /* setup spi1 */
	 spi_base = ioremap(SPI_BASE, 512);
	
	 /* reset spi1 */
	 __raw_writel(1<<7, spi_base + SPI_CR);
	
	 /* config MR(Mode Register,ģʽ�Ĵ���)*/
	 __raw_writel(1 | 1<<4 | 0xF<<16, spi_base + SPI_MR);
	
	 /* config CSR0(Chip Select Register 0)*/
	 __raw_writel(0x0101ff00, spi_base + SPI_CSR0);

	 /* config CSR1(Chip Select Register 0)*/
	 __raw_writel(0x00001900, spi_base + SPI_CSR1);
	 
	 /*ʹ��SPI1(spi1 enable,Control Register[���ƼĴ���])*/
	 __raw_writel(1, spi_base + SPI_CR);
  
   iounmap(spi_base);
   
  #else
   //����SPI1ģ��ܽ�   
   at91_set_gpio_output(AT91_PIN_PB2, 0);   //SPCK���
   at91_set_gpio_output(AT91_PIN_PB3, 1);   //NCPS0���
   at91_set_gpio_output(AT91_PIN_PB1, 0);   //MOSI���
   at91_set_gpio_input(AT91_PIN_PB0,  0);   //MISO����
  #endif

  //*********************************************************************
	
	#ifdef CTRL_PROCESS_IS_ATMEGA8L
   
   //����SPI1ģ��ܽ�   
   at91_set_gpio_output(AT91_PIN_PA2,  0);  //SPCK���
   at91_set_gpio_output(AT91_PIN_PC11, 1);  //NCPS1���
   at91_set_gpio_output(AT91_PIN_PA1,  0);  //MOSI���
   at91_set_gpio_input(AT91_PIN_PA0,   0);  //MISO����
   
	#else
	 at91_set_A_periph(AT91_PIN_PA0,  0);		/*SPI0_MISO*/
	 at91_set_A_periph(AT91_PIN_PA1,  0);		/*SPI0_MOSI*/
	 at91_set_A_periph(AT91_PIN_PA2,  0);		/*SPI0_SPCK*/
	 at91_set_B_periph(AT91_PIN_PC11, 1);		/*SPI0_NPCS1*/

	 //SPI0����
	 /* setup spi0 */
	 spi_base = ioremap(SPI_BASE_SPI0, 512);
	
	 /* reset spi0 */
	 //__raw_writel(1<<7, spi_base + SPI_CR);
	
	 /* config MR(Mode Register,ģʽ�Ĵ���)*/
	 //__raw_writel(1 | 1<<4 | 0xF<<16, spi_base + SPI_MR);
	 
	 /* config CSR1(Chip Select Register 0)*/
	 //__raw_writel(0x0101ff02, spi_base + SPI_CSR1);

	 /*ʹ��SPI0(spi0 enable,Control Register[���ƼĴ���])*/
	 //__raw_writel(1, spi_base + SPI_CR);
	 iounmap(spi_base);
  #endif
	
#endif
  
	/*********************����SPI����****��ʼ********************************************************/
	/*��λATT7022B IO�Ŷ���Ϊ���*/
	if (at91_set_gpio_output(AT91_PIN_PC7, 0)==0)
	{
	   printk(KERN_ALERT"Reset ATT7022B...\n");
     
     //ֹͣ100us
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

	/*ATT7022Bģʽ���ýŶ���Ϊ���*/
	if (at91_set_gpio_output(AT91_PIN_PA27, 0)==0)
	{
		;
	}
	else
	{
	  printk(KERN_ALERT "set AT91_PIN_PA27 failed!\n");
	}
	
	/*ATT7022B���źŽ�IO����Ϊ����*/
	if (at91_set_gpio_input(AT91_PIN_PC9, 0)==0)
	{
	  ;
	}
	else
	{
	  printk(KERN_ALERT "set AT91_PIN_PC9 failed!\n");
	}
	
	/*ע������*/
	ret=spi_register_driver(&spi_acSample_driver);
	if(ret)
	{
		spi_unregister_driver(&spi_acSample_driver);
		return -1;
	}
 
  /*�Զ�����һ���豸��*/
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

	/*********************����SPI����****����********************************************************/
  
 #ifdef CTRL_PROCESS_IS_ATMEGA8L
	/*********************��Ƭ��(ATMEGA8L)ͨ��SPI����****��ʼ********************************************************/	
	/*ע������*/
	ret=spi_register_driver(&spi_atmega8_driver);
	if(ret)
	{
		spi_unregister_driver(&spi_atmega8_driver);
		return -1;
	}
 
  /*�Զ�����һ���豸��*/
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
	/*********************��Ƭ��ATMEGA8L SPI����****����********************************************************/

 #else    //���ư嵥Ƭ����STC12
	
	/*********************��Ƭ��(stc12)ͨ��SPI����****��ʼ********************************************************/	
	/*ע������*/
	ret=spi_register_driver(&spi_stc12_driver);
	if(ret)
	{
		spi_unregister_driver(&spi_stc12_driver);
		return -1;
	}
 
  /*�Զ�����һ���豸��*/
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
	/*********************��Ƭ��SPI����****����********************************************************/

 #endif

	printk(KERN_ALERT"SPI Second char driver init Done!\n");
 	
	return 0;
}

/**************************************************
��������:secondSpi1_exit
��������:�˳�SPI1�ַ�����(��������,�뵥Ƭ��ͨ��(stc12))
���ú���:
�����ú���:
�������:��
�������:
����ֵ:void
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

