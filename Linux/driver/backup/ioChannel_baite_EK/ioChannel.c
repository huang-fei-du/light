/***************************************************
Copyright,2009,Huawei WoDian co.,LTD,All	Rights Reserved
�ļ���:ioChannel.c
����:leiyong
�汾:0.9
�������:2009��12��
����:�����ն�(�����ն�/������,AT91SAM9260������)I/Oͨ�������ļ�
     I/Oͨ������:GPRS�����źš����̡�YX��ADC

�����б�:

�޸���ʷ:
  01,09-12-20,Leiyong created.
***************************************************/

#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>
#include <linux/init.h>         /* Needed for the module-macros */
#include <linux/fs.h>
#include <asm/arch/gpio.h>
#include "ioChannel.h"

#define DEVICE_NAME "ioChannel"

static int ioChannelMajor;

static int ioChannel_ioctl(struct inode *inode, struct file *file,unsigned int cmd, unsigned long arg)
{
   u8 tmpData = 0;
   switch(cmd)
   {
			case READ_IO_PIN:   /*��ȡI/O�ŵ�ֵ*/
				if (!at91_get_gpio_value(AT91_PIN_PB0))
				{
					 tmpData = KEY_1;
				}
				if (!at91_get_gpio_value(AT91_PIN_PB1))
				{
					 tmpData = KEY_2;
				}
				if (!at91_get_gpio_value(AT91_PIN_PB2))
				{
					 tmpData = KEY_3;
				}
				if (!at91_get_gpio_value(AT91_PIN_PB3))
				{
					 tmpData = KEY_4;
				}
				break;

			case WIRELESS_ON_OFF:   /*����Modem�����ź���*/
				at91_set_gpio_value(AT91_PIN_PC9,arg);
				break;				
      
      case 3:
				at91_set_gpio_value(AT91_PIN_PA7,arg);
      	break;

      case 4:
				at91_set_gpio_value(AT91_PIN_PA8,arg);
      	break;
      					
			default:
				printk(KERN_ALERT "cmd default.\n");
				break;
   }
   
	 return tmpData;
}

static int ioChannel_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int ioChannel_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t ioChannel_read(struct file *filp,char __user *buf,
		size_t size,loff_t *ppos)
{
	 return 0;
}

static ssize_t ioChannel_write(struct file *filp,const char __user *buf,
		size_t size,loff_t *ppos)
{
	return 0;
}

static struct file_operations ioChannel_fops = {
 .owner   = THIS_MODULE,
 .ioctl   = ioChannel_ioctl,
 .open    = ioChannel_open,
 .release = ioChannel_release,
 .read    = ioChannel_read,
 .write   = ioChannel_write,
};

static int __init ioChannel_init(void)
{
	/*PC9-����Modem�����ź���*/
	if (at91_set_gpio_output(AT91_PIN_PC9, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC9 Output failed!\n");
	}

	/*PB0-KEY1*/
	if (at91_set_gpio_input(AT91_PIN_PB0, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB0 Input  failed!\n");
	}

	/*PB1-KEY2*/
	if (at91_set_gpio_input(AT91_PIN_PB1, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB1 Input  failed!\n");
	}

	/*PB2-KEY3*/
	if (at91_set_gpio_input(AT91_PIN_PB2, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB2 Input  failed!\n");
	}

	/*PB3-KEY4*/
	if (at91_set_gpio_input(AT91_PIN_PB3, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB3 Input failed!\n");
	}	

	at91_set_gpio_output(AT91_PIN_PA7, 0);
	at91_set_gpio_output(AT91_PIN_PA8, 0);
	
	
	ioChannelMajor=register_chrdev(0,DEVICE_NAME,&ioChannel_fops);
	
  if (ioChannelMajor < 0)
  {
     printk(KERN_INFO "ioChannel:can't get major number\n\r");

  	 return ioChannelMajor;
  }
  
  printk(KERN_ALERT "ioChannel Driver Done.\n");
  return 0;
}
 
static void __exit ioChannel_exit(void)
{
	unregister_chrdev(ioChannelMajor,DEVICE_NAME);
  printk(KERN_ALERT "ioChannel Driver removed.\n");
}

module_init(ioChannel_init);
module_exit(ioChannel_exit);

MODULE_LICENSE("Dual BSD/GPL");   /*should always exist or you��ll get a warning*/
MODULE_AUTHOR("Leiyong");        /*optional*/
MODULE_DESCRIPTION("IO_CHANNEL"); /*optional*/
MODULE_VERSION("V1.0");

