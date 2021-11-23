/***************************************************
Copyright Ly,2010,All	Rights Reserved
�ļ���:ioChannel.c
����:lhh
�汾:0.9
�������:2010��4��
����:�����ն�(�����ն�,AT91SAM9260������)������������ļ�
     �������ͨ������:GPRS�����źš����̡�YX��ADC

�����б�:

�޸���ʷ:
  01,10-02-18,lhh created.
***************************************************/

#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>
#include <linux/init.h>         /* Needed for the module-macros */
#include <linux/fs.h>
#include <asm/arch/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include "ioChannel.h"


#define DEVICE_NAME "ioChannel"

static int   ioChannelMajor;
struct cdev  ioChannelCdev;
struct class *classIoChannel;

static int ioChannel_ioctl(struct inode *inode, struct file *file,unsigned int cmd, unsigned long arg)
{   
   u8 tmpData = 0;
   
   switch(cmd)
   {
			case READ_KEY_VALUE:   /*��ȡI/O�ŵ�ֵ*/
				if (!at91_get_gpio_value(AT91_PIN_PA7))
				{
					 tmpData = KEY_OK;
				}
				if (!at91_get_gpio_value(AT91_PIN_PA6))
				{
					 tmpData = KEY_CANCEL;
				}
				break;

			case READ_MODULE_TYPE:    //��ȡ����Modem����
				if (at91_get_gpio_value(AT91_PIN_PC4))  /*state0*/
				{
					 tmpData |= 0x1;
				}
				if (at91_get_gpio_value(AT91_PIN_PC8))  /*state1*/
				{
					 tmpData |= 0x2;
				}
				if (at91_get_gpio_value(AT91_PIN_PC6))  /*state2*/
				{
					 tmpData |= 0x4;
				}
				if (at91_get_gpio_value(AT91_PIN_PC10)) /*state3*/
				{
					 tmpData |= 0x8;
				}
				break;
				
			case WIRELESS_POWER_ON_OFF:   /*��/�ر�����Modem��Դ*/
				at91_set_gpio_value(AT91_PIN_PC0,arg);
				break;
      
			case WIRELESS_IGT:            /*����Modem���ػ�,press key*/
				at91_set_gpio_value(AT91_PIN_PA22,arg);
				break;

			case WIRELESS_RESET:          /*����Modem��λ*/
				at91_set_gpio_value(AT91_PIN_PA22,arg);
				break;

      case READ_YX_VALUE:           /*��ȡң��ֵ*/
				if (at91_get_gpio_value(AT91_PIN_PC2))  /*YX1*/
				{
					 tmpData |= 0x1;
				}
				if (at91_get_gpio_value(AT91_PIN_PB20))   /*YX2*/
				{
					 tmpData |= 0x2;
				}      	
				if (at91_get_gpio_value(AT91_PIN_PB17))   /*YX3*/
				{
					 tmpData |= 0x4;
				}      	
				if (at91_get_gpio_value(AT91_PIN_PB18))   /*YX4*/
				{
					 tmpData |= 0x8;
				}      	
				if (at91_get_gpio_value(AT91_PIN_PB10))   /*YX5*/
				{
					 tmpData |= 0x10;
				}      	
				if (at91_get_gpio_value(AT91_PIN_PB11))   /*YX6*/
				{
					 tmpData |= 0x20;
				}      	
				if (at91_get_gpio_value(AT91_PIN_PA31))   /*YX7*/
				{
					 tmpData |= 0x40;
				}      	
				if (at91_get_gpio_value(AT91_PIN_PA30))   /*YX8*/
				{
					 tmpData |= 0x80;
				}
      	break;      	
      	
      case READ_GATEK_VALUE:           /*��ȡ�ſ�ֵ*/
				if (at91_get_gpio_value(AT91_PIN_PB21))
				{
					 tmpData |= 0x1;
				}
				break;
      	
      case DETECT_POWER_FAILURE:    /*ͣ����*/
				if (at91_get_gpio_value(AT91_PIN_PC15))
				{
					 tmpData = IO_HIGH;
				}
				else
				{
					 tmpData = IO_LOW;
				}
      	break;
      	
			case SET_ALARM_VOICE:         /*���ø澯��*/
				at91_set_gpio_value(AT91_PIN_PB12,arg);
				break;
				
			case SET_BATTERY_ON:          /*���ú󱸵��ͨ��*/
				at91_set_gpio_value(AT91_PIN_PB19,arg);
				break;
		  
		  case LOAD_CTRL_LINE_1:       /*���ɿ��Ƶ�һ·*/
				at91_set_gpio_value(AT91_PIN_PB29,arg);		  	
		  	break;
		  	
		  case LOAD_CTRL_LINE_2:       /*���ɿ��Ƶڶ�·*/
				at91_set_gpio_value(AT91_PIN_PA5,arg);
		  	break;

		  case LOAD_CTRL_LINE_3:       /*���ɿ��Ƶ�3·*/
				at91_set_gpio_value(AT91_PIN_PA25,arg);
		  	break;

		  case LOAD_CTRL_LINE_4:       /*���ɿ��Ƶ�4·*/
				at91_set_gpio_value(AT91_PIN_PA11,arg);
		  	break;
		  	
		  case SET_BATTERY_CHARGE:    /*������*/
		  	at91_set_gpio_value(AT91_PIN_PB30,arg);
		  	break;		  	
		  	
			default:
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

static void ioChannel_setup_cdev(void)
{
	 int error;
	 int devno, tmpMinor=0;
	 
   devno = MKDEV (ioChannelMajor, tmpMinor);
   
   cdev_init (&ioChannelCdev, &ioChannel_fops);
   
   ioChannelCdev.owner = THIS_MODULE;
   ioChannelCdev.ops = &ioChannel_fops;
   error = cdev_add(&ioChannelCdev, devno,1);

   if(error)
   {
     printk(KERN_NOTICE "Error %d adding char_reg_setup_cdev",error);
   }
}

static int __init ioChannel_init(void)
{
	dev_t  devNo;     /*�豸��*/
  //int    result;

	/***************************1.����Modem*********************************/
	/*1.2����Modem���ص�Դ�� - PC0*/
	if (at91_set_gpio_output(AT91_PIN_PC0, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC0 Output failed!\n");
	}
	/*1.3����Modem��λ��(������IGT) - PA22*/
	if (at91_set_gpio_output(AT91_PIN_PA22, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA22 Output failed!\n");
	}
	/*1.4����Modemģ���ͺ�ʶ��state0 - PC4*/
	if (at91_set_gpio_input(AT91_PIN_PC4, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC4 Output failed!\n");
	}
	/*1.5����Modemģ���ͺ�ʶ��state1 - PC8*/
	if (at91_set_gpio_input(AT91_PIN_PC8, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC8 Output failed!\n");
	}
	/*1.6����Modemģ���ͺ�ʶ��state2 - PC6*/
	if (at91_set_gpio_input(AT91_PIN_PC6, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC6 Output failed!\n");
	}
	/*1.7����Modemģ���ͺ�ʶ��state3 - PC10*/
	if (at91_set_gpio_input(AT91_PIN_PC10, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC10 Output failed!\n");
	}

	/***************************2.����**************************************/
	/*2.1ȷ�ϼ� - PA7*/
	if (at91_set_gpio_input(AT91_PIN_PA7, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB17 Input  failed!\n");
	}

	/*2.2ѡ��� - PA6*/
	if (at91_set_gpio_input(AT91_PIN_PA6, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB16 Input  failed!\n");
	}

	/***************************3.ң��**************************************/
	/*3.1 ң��1(YX1) - PC2*/
	if (at91_set_gpio_input(AT91_PIN_PC2, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC2 Input  failed!\n");
	}
	/*3.2 ң��2(YX2) - PB20*/
	if (at91_set_gpio_input(AT91_PIN_PB20, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB20 Input  failed!\n");
	}
	/*3.3 ң��3(YX3) - PB17*/
	if (at91_set_gpio_input(AT91_PIN_PB17, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB17 Input  failed!\n");
	}
	/*3.4 ң��4(YX4) - PB18*/
	if (at91_set_gpio_input(AT91_PIN_PB18, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB18 Input  failed!\n");
	}
	/*3.5 ң��5(YX5) - PB10*/
	if (at91_set_gpio_input(AT91_PIN_PB10, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB10 Input  failed!\n");
	}
	/*3.6 ң��6(YX6) - PB11*/
	if (at91_set_gpio_input(AT91_PIN_PB11, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB11 Input  failed!\n");
	}
	/*3.7 ң��7(YX7) - PA31*/
	if (at91_set_gpio_input(AT91_PIN_PA31, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA31 Input  failed!\n");
	}
	/*3.8 ң��8(YX8) - PA30*/
	if (at91_set_gpio_input(AT91_PIN_PA30, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA30 Input  failed!\n");
	}
	
	/*3.9�ſ����� - PB21*/
	if (at91_set_gpio_input(AT91_PIN_PB21, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB21 Input  failed!\n");
	}
	
	/***************************4.ͣ����**********************************/
	/*ͣ�������� - PC15*/
	if (at91_set_gpio_input(AT91_PIN_PC15, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC15 Input  failed!\n");
	}
	
	/***************************5.�澯ָʾ�ƺ͸澯��************************/
	/*5.1�澯��(������) - PB12*/
	if (at91_set_gpio_output(AT91_PIN_PB12, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB12 Output failed!\n");
	}

	/***************************6.�󱳵�ع���*********************************/
	/*6.1 5V�̵������� - PB19*/
	if (at91_set_gpio_output(AT91_PIN_PB19, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB19 Output failed!\n");
	}
	/*6.2 ������ - PB30*/
	if (at91_set_gpio_output(AT91_PIN_PB30, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB30 Output failed!\n");
	}

	/***************************7.���Ƽ̵�������***********************************/
	/*7.1�̵�������1 - PB29*/
	if (at91_set_gpio_output(AT91_PIN_PB29, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC4 Output failed!\n");
	}
	/*7.2�̵�������2 - PA5*/
	if (at91_set_gpio_output(AT91_PIN_PA5, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC6 Output failed!\n");
	}
	/*7.3�̵�������3 - PA25*/
	if (at91_set_gpio_output(AT91_PIN_PA25, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC7 Output failed!\n");
	}
	/*7.4�̵�������4 - PA11*/
	if (at91_set_gpio_output(AT91_PIN_PA11, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA26 Output failed!\n");
	}

	/*****************************3.ע��***********************************/
	/*
	ioChannelMajor=register_chrdev(0,DEVICE_NAME,&ioChannel_fops);
	
  if (ioChannelMajor < 0)
  {
     printk(KERN_INFO "ioChannel:can't get major number\n\r");

  	 return ioChannelMajor;
  }
  
  printk(KERN_ALERT "ioChannel Driver Done.\n");
  */
  
  //devNo=MKDEV(255,0);

  /*�Զ�����һ���豸��*/
	if (alloc_chrdev_region(&devNo,0,1,"ioChannel") < 0)
	{
		 return -2;
	}
	
	ioChannelMajor= MAJOR(devNo);

  /*
  result=register_chrdev_region(ioChannelMajor,1,"ioChannel");
  if(result<0)
  {
    printk(KERN_WARNING "ioChannel: can't get major number %d\n", ioChannelMajor);
    return result;
  }
  */

  ioChannel_setup_cdev();
  
  /* create your own class under /sysfs */
  classIoChannel = class_create(THIS_MODULE, "ioChannel");
  if(IS_ERR(classIoChannel))
  {
    printk("IO Channel Err: failed in creating class.\n");
    return -1;
  }

  device_create(classIoChannel, NULL, MKDEV(ioChannelMajor, 0), "ioChannel");

  printk (KERN_INFO "ioChannel driver init Done!\n");
 
  return 0;
}
 
static void __exit ioChannel_exit(void)
{
	//unregister_chrdev(ioChannelMajor,DEVICE_NAME);

  dev_t devno = MKDEV (ioChannelMajor, 0);
  cdev_del (&ioChannelCdev);
      
  //delete device node under /dev
  device_destroy(classIoChannel, MKDEV(ioChannelMajor, 0)); 

  //delete class created by us
  class_destroy(classIoChannel);

  unregister_chrdev_region (devno, 1);
  
  printk(KERN_ALERT "ioChannel Driver removed.\n");
}

module_init(ioChannel_init);
module_exit(ioChannel_exit);

MODULE_LICENSE("Dual BSD/GPL");   /*should always exist or you��ll get a warning*/
MODULE_AUTHOR("ly");              /*optional*/
MODULE_DESCRIPTION("IO_CHANNEL"); /*optional*/
MODULE_VERSION("V1.0");

