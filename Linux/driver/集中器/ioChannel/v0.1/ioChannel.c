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
#include <linux/cdev.h>
#include <linux/device.h>
#include "ioChannel.h"


#define DEVICE_NAME "ioChannel"

static int   ioChannelMajor;
struct cdev  ioChannelCdev;
struct class *classIoChannel;

unsigned char watchDogx=0; 

static int ioChannel_ioctl(struct inode *inode, struct file *file,unsigned int cmd, unsigned long arg)
{
   u8 tmpData = 0;
   switch(cmd)
   {
			case READ_KEY_VALUE:   /*��ȡI/O�ŵ�ֵ*/
				if (!at91_get_gpio_value(AT91_PIN_PA28))
				{
					 tmpData = KEY_UP;
				}
				if (!at91_get_gpio_value(AT91_PIN_PA27))
				{
					 tmpData = KEY_DOWN;
				}
				if (!at91_get_gpio_value(AT91_PIN_PA26))
				{
					 tmpData = KEY_LEFT;
				}
				if (!at91_get_gpio_value(AT91_PIN_PA25))
				{
					 tmpData = KEY_RIGHT;
				}
				if (!at91_get_gpio_value(AT91_PIN_PB16))
				{
					 tmpData = KEY_CANCEL;
				}
				if (!at91_get_gpio_value(AT91_PIN_PB17))
				{
					 tmpData = KEY_OK;
				}
				break;

			case READ_MODULE_TYPE:    //��ȡ����Modem����
				if (at91_get_gpio_value(AT91_PIN_PA10))  /*state0*/
				{
					 tmpData |= 0x1;
				}
				if (at91_get_gpio_value(AT91_PIN_PA9))   /*state1*/
				{
					 tmpData |= 0x2;
				}
				if (at91_get_gpio_value(AT91_PIN_PA8))   /*state2*/
				{
					 tmpData |= 0x4;
				}
				if (at91_get_gpio_value(AT91_PIN_PA7))   /*state3*/
				{
					 tmpData |= 0x8;
				}
				if (at91_get_gpio_value(AT91_PIN_PA6))   /*state4*/
				{
					 tmpData |= 0x10;
				}
				break;
				
			case WIRELESS_POWER_ON_OFF:   /*��/�ر�����Modem��Դ*/
				at91_set_gpio_value(AT91_PIN_PB21,arg);
				break;
      
			case WIRELESS_IGT:            /*����Modem���ػ�,press key*/
				at91_set_gpio_value(AT91_PIN_PB20,arg);
				break;

			case WIRELESS_RESET:          /*����Modem��λ*/
				at91_set_gpio_value(AT91_PIN_PB22,arg);
				break;

      case READ_YX_VALUE:           /*��ȡң��ֵ*/
				if (at91_get_gpio_value(AT91_PIN_PC2))  /*YX1*/
				{
					 tmpData |= 0x1;
				}
				if (at91_get_gpio_value(AT91_PIN_PC3))   /*YX2*/
				{
					 tmpData |= 0x2;
				}      	
      	break;
      	
      case DETECT_POWER_FAILURE:    /*ͣ����*/
				if (at91_get_gpio_value(AT91_PIN_PC4))  /*YX1*/
				{
					 tmpData = IO_HIGH;
				}
				else
				{
					 tmpData = IO_LOW;
				}
      	break;
      	
			case SET_ALARM_LIGHT:         /*���ø澯��*/
				at91_set_gpio_value(AT91_PIN_PB30,arg);
				break;
      	
			case SET_ALARM_VOICE:         /*���ø澯��*/
				at91_set_gpio_value(AT91_PIN_PC7,arg);
				break;
				
			case SET_BATTERY_ON:          /*���ú󱸵��ͨ��*/
				at91_set_gpio_value(AT91_PIN_PC4,arg);
				break;
		  
      case SET_CARRIER_MODULE:     /*�����ز�ģ��/set*/
				at91_set_gpio_value(AT91_PIN_PA5,arg);
      	break;
      	
		  case RST_CARRIER_MODULE:     /*��λ�ز�ģ��*/
				at91_set_gpio_value(AT91_PIN_PB18,arg);
		  	break;
		  	
		  case SET_WATCH_DOG:          /**/
		  	if (watchDogx==0)
		  	{
		  		watchDogx = 1;
		  		at91_set_gpio_output(AT91_PIN_PC0, 1);
				}
				at91_set_gpio_value(AT91_PIN_PC0,arg);		  	
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
  int    result;

	/***************************1.����Modem*********************************/
	/*1.1����Modem�����ź��� - PB20*/
	if (at91_set_gpio_output(AT91_PIN_PB20, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB20 Output failed!\n");
	}
	/*1.2����Modem���ص�Դ�� - PB21*/
	if (at91_set_gpio_output(AT91_PIN_PB21, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB21 Output failed!\n");
	}
	/*1.2����Modem���ص�Դ�� - PB31*/
	//if (at91_set_gpio_output(AT91_PIN_PB31, 0)!=0)
	//{
	//   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB31 Output failed!\n");
	//}
	/*1.3����Modem��λ�� - PB22*/
	if (at91_set_gpio_output(AT91_PIN_PB22, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB22 Output failed!\n");
	}
	/*1.4����Modemģ���ͺ�ʶ��state0 - PA10*/
	if (at91_set_gpio_input(AT91_PIN_PA10, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA10 Output failed!\n");
	}
	/*1.5����Modemģ���ͺ�ʶ��state1 - PA9*/
	if (at91_set_gpio_input(AT91_PIN_PA9, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA9 Output failed!\n");
	}
	/*1.6����Modemģ���ͺ�ʶ��state2 - PA8*/
	if (at91_set_gpio_input(AT91_PIN_PA8, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA8 Output failed!\n");
	}
	/*1.7����Modemģ���ͺ�ʶ��state3 - PA7*/
	if (at91_set_gpio_input(AT91_PIN_PA7, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA7 Output failed!\n");
	}
	/*1.8����Modemģ���ͺ�ʶ��state4 - PA6*/
	if (at91_set_gpio_input(AT91_PIN_PA6, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA6 Output failed!\n");
	}
	

	/***************************2.����**************************************/
	/*2.1�ϼ� - PA28*/
	if (at91_set_gpio_input(AT91_PIN_PA28, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA28 Input  failed!\n");
	}

	/*2.2�¼� - PA27*/
	if (at91_set_gpio_input(AT91_PIN_PA27, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA27 Input  failed!\n");
	}

	/*2.3��� - PA26*/
	if (at91_set_gpio_input(AT91_PIN_PA26, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA26 Input  failed!\n");
	}

	/*2.4�Ҽ� - PA25*/
	if (at91_set_gpio_input(AT91_PIN_PA25, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA25 Input failed!\n");
	}	

	/*2.5ȷ�ϼ� - PB17*/
	if (at91_set_gpio_input(AT91_PIN_PB17, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB17 Input failed!\n");
	}	

	/*2.6ȡ���� - PB16*/
	if (at91_set_gpio_input(AT91_PIN_PB16, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB16 Input failed!\n");
	}
	
	/***************************3.ң��**************************************/
	/*3.1 ң��1(YX1) - PC2*/
	if (at91_set_gpio_input(AT91_PIN_PC2, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC2 Input  failed!\n");
	}
	/*3.2 ң��2(YX2) - PC3*/
	if (at91_set_gpio_input(AT91_PIN_PC3, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC3 Input  failed!\n");
	}
	
	/***************************4.ͣ����**********************************/
	/*ͣ�������� - PC4*/
	if (at91_set_gpio_input(AT91_PIN_PC4, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC4 Input  failed!\n");
	}
	
	/***************************5.�ز�ģ���������**************************/
	/*5.1�ز�ģ������(Z/SET) - PA5*/
	if (at91_set_gpio_output(AT91_PIN_PA5, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PA5 Output failed!\n");
	}
	/*5.2�ز�ģ�鸴λ(Z/RST) - PB18*/
	if (at91_set_gpio_output(AT91_PIN_PB18, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB18 Output failed!\n");
	}

	/***************************6.�澯ָʾ�ƺ͸澯��************************/
	/*6.1�澯ָʾ - PA29*/
	if (at91_set_gpio_output(AT91_PIN_PB30, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PB30 Output failed!\n");
	}
	/*6.2�澯�� - PC7*/
	if (at91_set_gpio_output(AT91_PIN_PC7, 0)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC7 Output failed!\n");
	}

	/***************************7.ͣ�����*********************************/
	/*������ - PC6*/
	if (at91_set_gpio_output(AT91_PIN_PC6, 1)!=0)
	{
	   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC6 Output failed!\n");
	}

	/***************************8.���Ź�***********************************/
	//if (at91_set_gpio_output(AT91_PIN_PC0, 1)!=0)
	//{
	//   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC0 Output failed!\n");
	//}

	//if (at91_set_gpio_output(AT91_PIN_PC12, 1)!=0)
	//{
	//   printk(KERN_ALERT "ioChannel:set AT91_PIN_PC12 Output failed!\n");
	//}


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

  printk (KERN_INFO "IO Channel: Init Done!\n");
 
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
MODULE_AUTHOR("Leiyong");        /*optional*/
MODULE_DESCRIPTION("IO_CHANNEL"); /*optional*/
MODULE_VERSION("V1.0");

