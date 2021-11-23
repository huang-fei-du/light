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

#define LCDCMD_BASE	AT91_CHIPSELECT_0

struct at91_lcdcmd_data {
	u8		enable_pin;
	u8		bus_width_16;
};

struct lcdcmd_sb {
struct cdev lcdcmd_cdev;
char data;
};

struct class *lcdcmd_class;

static void __iomem *Lcd_base0,*Lcd_base1;

static struct lcdcmd_sb *lcdcmd;

static int LCDCMD_MAJOR;

static struct at91_lcdcmd_data lcdcmd_data;

static struct at91_lcdcmd_data lcdcmd_drdata = {
	.enable_pin	= AT91_PIN_PC14,
	.bus_width_16	= 0,
};

static struct resource lcdcmd_resources[] = {
	{
		.start	= LCDCMD_BASE,
		.end	= LCDCMD_BASE + 1, //SZ_8M - 1,
		.flags	= IORESOURCE_MEM,
	}
};

static struct platform_device at91sam9260_lcdcmd_device = {
	.name		= "at91_lcdcmd",
	.id		= -1,
	.dev		= {
				.platform_data	= &lcdcmd_data,
	},
	.resource	= lcdcmd_resources,
	.num_resources	= ARRAY_SIZE(lcdcmd_resources),
};

void Lcdcmdint(void)
{
	writeb(0xE2, Lcd_base0);
	writeb(0xAE, Lcd_base0);
	writeb(0x24, Lcd_base0);
	writeb(0x2A, Lcd_base0);
	writeb(0xE9, Lcd_base0);
	writeb(0x81, Lcd_base0);
	writeb(0xC2, Lcd_base0);
	writeb(0x89, Lcd_base0);
	writeb(0xC4, Lcd_base0);
	writeb(0x84, Lcd_base0);
	writeb(0xD6, Lcd_base0);
	writeb(0xD1, Lcd_base0);
	writeb(0xDE, Lcd_base0);
	writeb(0xAD, Lcd_base0);
}

void Lcdcmdcls(void)
{
	int i,j;
	u8 uc_RowAddrH,uc_RowAddrL;
	
	//writeb(0x70, Lcd_base0);	writeb(0x60, Lcd_base0);
	//writeb(0x12, Lcd_base0);	writeb(0x05, Lcd_base0);
	
	for (i=0;i<160;i++)
	{
		uc_RowAddrH =i /16;
    uc_RowAddrL =i %16;
		writeb(0x70 | uc_RowAddrH, Lcd_base0);
		writeb(0x60 | uc_RowAddrL, Lcd_base0);
		writeb(0x12, Lcd_base0);
		writeb(0x05, Lcd_base0);
		for (j=0;j<54;j++)
		{
			writeb(0x00, Lcd_base1);
			writeb(0x00, Lcd_base1);
		}
	}
}

void at91_add_device_lcdcmd(struct at91_lcdcmd_data *data)
{
	unsigned long mode; //csa, mode;

	if (!data)
		return;

	//csa = at91_sys_read(AT91_MATRIX_EBICSA); csa = csa & 0xffffffdf;
	//at91_sys_write(AT91_MATRIX_EBICSA, csa | AT91_MATRIX_CS5A_SMC);

	/* set the bus interface characteristics */
	at91_sys_write(AT91_SMC_SETUP(0), AT91_SMC_NWESETUP_(1) | AT91_SMC_NCS_WRSETUP_(1) | AT91_SMC_NRDSETUP_(1) | AT91_SMC_NCS_RDSETUP_(1));

	at91_sys_write(AT91_SMC_PULSE(0), AT91_SMC_NWEPULSE_(3) | AT91_SMC_NCS_WRPULSE_(3) | AT91_SMC_NRDPULSE_(3) | AT91_SMC_NCS_RDPULSE_(3));

	at91_sys_write(AT91_SMC_CYCLE(0), AT91_SMC_NWECYCLE_(5) | AT91_SMC_NRDCYCLE_(5));

	if (data->bus_width_16)	{	mode = AT91_SMC_DBW_16;	}
	else	{	mode = AT91_SMC_DBW_8;	}
	at91_sys_write(AT91_SMC_MODE(0), mode | AT91_SMC_READMODE | AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_TDF_(2));

	/* enable pin */
	if (data->enable_pin)
		at91_set_gpio_output(data->enable_pin, 0);
		at91_set_gpio_value(data->enable_pin,0);

   lcdcmd_data = *data;
	platform_device_register(&at91sam9260_lcdcmd_device);
}

static int lcdcmd_open(struct inode *inodep, struct file *filp)
{
	filp->private_data = lcdcmd;
	printk(KERN_ALERT "Lcdcmdsb open sucess.\n");
	return 0;
}

static int lcdcmd_ioctl(struct inode *inode, struct file *filp, unsigned int command, unsigned long arg)
{
	if (command == 0x01) 
	{
		//at91_set_gpio_output(AT91_PIN_PC15,0);
		//at91_set_gpio_value(AT91_PIN_PC15,0);
		
		writeb(0x70, Lcd_base0);
		writeb(0x60, Lcd_base0);
		writeb(0x12, Lcd_base0);
		writeb(0x05, Lcd_base0);
		writeb(0xF8, Lcd_base1);
		writeb(0x1F, Lcd_base1);
		printk(KERN_ALERT "Lcd write Suc1.\n");
		return 0;
	//	return writeb(0xAA, Lcd_base);
	}
	if (command == 0x02) 
	{
		//at91_set_gpio_output(AT91_PIN_PC15,0);
		//at91_set_gpio_value(AT91_PIN_PC15,1);
		//writeb(0x00, Lcd_base1); //writeb(0x00, Lcd_base1);
		printk(KERN_ALERT "Lcd write Suc2.\n");
		return 0;
	//	return writeb(0xAA, Lcd_base);
	}

	printk(KERN_ALERT "Lcd write err.\n");
	return 0;
}

static int lcdcmd_release(struct inode *inodep, struct file *filp)
{
	filp->private_data = NULL;
	printk(KERN_ALERT "Lcdcmdsb release.\n");
	return 0;
}

static struct file_operations lcdcmd_fops = {
  .owner	= THIS_MODULE,
 	.open	= lcdcmd_open,
  .release = lcdcmd_release,
  .ioctl   = lcdcmd_ioctl,
};

static void lcdcmd_setup_cdev(struct lcdcmd_sb *dev, int index)
{
	int err,devno = MKDEV(LCDCMD_MAJOR,index);

	cdev_init(&dev->lcdcmd_cdev,&lcdcmd_fops);
	dev->lcdcmd_cdev.owner = lcdcmd_fops.owner;
	err=cdev_add(&dev->lcdcmd_cdev,devno,1);
	if (err) { printk(KERN_NOTICE"Error %d adding Lcdcmdsb %d\n",err,index); }
}

static int __devinit at91_lcdcmd_probe(struct platform_device *pdev)
{
	/*struct resource *res;
	struct resource *ioarea;

	res=platform_get_resource(&at91sam9260_lcdcmd_device,IORESOURCE_MEM,0);
	ioarea=request_mem_region(res->start,(res->end-res->start)+1,at91sam9260_lcdcmd_device.name);*/
		
	Lcdcmdint();
	Lcdcmdcls();
		
	printk(KERN_ALERT "Open platform_driver.\n");

	return 0;
}

static int __devexit at91_lcdcmd_remove(struct platform_device *pdev)
{
	printk(KERN_ALERT "Close platform_driver.\n");
	return 0;
}

static struct platform_driver at91_lcdcmd_driver = {
	.probe		= at91_lcdcmd_probe,
	.remove		= at91_lcdcmd_remove,
	.driver		= {
		.name	= "at91_lcdcmd",
		.owner	= THIS_MODULE,
	},
};

static int __init at91_lcdcmd_init(void)
{
	int result;
	dev_t devno;
	struct resource *res;
	
	at91_add_device_lcdcmd(&lcdcmd_drdata);
	
	devno = at91sam9260_lcdcmd_device.dev.devt;
	LCDCMD_MAJOR=MAJOR(devno);
	if (LCDCMD_MAJOR) { result=register_chrdev_region(devno,1,"Lcdcmdsbyy"); }
	else
	{
		result=alloc_chrdev_region(&devno,0,1,"Lcdcmdsbzd");
		LCDCMD_MAJOR= MAJOR(devno);
	}
	if (result < 0) { return result; }
	lcdcmd=kmalloc(sizeof(struct lcdcmd_sb),GFP_KERNEL);
	if (!lcdcmd)	{	result = -1; goto fail_malloc; }
	memset(lcdcmd ,0,sizeof(struct lcdcmd_sb));
	lcdcmd_setup_cdev(lcdcmd,0);
	
	res=platform_get_resource(&at91sam9260_lcdcmd_device,IORESOURCE_MEM,0);
	Lcd_base0 = ioremap(res->start, 32);
	Lcd_base1 = ioremap(res->end, 32);
	
	/* create your own class under /sysfs */
     lcdcmd_class = class_create(THIS_MODULE, "lcdcmd_class");
     if(IS_ERR(lcdcmd_class)) 
     {
          printk("Err: failed in creating class.\n");
          cdev_del(&lcdcmd->lcdcmd_cdev);
	    		kfree(lcdcmd);
  				goto fail_malloc; 
      } 

  /* register your own device in sysfs, and this will cause udev to create corresponding device node */
	device_create( lcdcmd_class, NULL, MKDEV(LCDCMD_MAJOR, 0), "Lcdcmdsb%d", 0 );
	
	result = platform_driver_register(&at91_lcdcmd_driver);
  if (result) 
  	{ 
 		device_destroy(lcdcmd_class, MKDEV(LCDCMD_MAJOR, 0));
		class_destroy(lcdcmd_class);
  		cdev_del(&lcdcmd->lcdcmd_cdev);
	    kfree(lcdcmd);
  		goto fail_malloc; 
  	}
	printk(KERN_ALERT "Open Lcdcmdsb Driver.\n");
  return result;
fail_malloc:
	unregister_chrdev_region(devno,1);
	iounmap(Lcd_base0);
	iounmap(Lcd_base1);
	printk(KERN_ALERT "Open Lcdcmdsb Driver err.\n");
	return result;
}

static void __exit at91_lcdcmd_exit(void)
{
	platform_driver_unregister(&at91_lcdcmd_driver);
	device_destroy(lcdcmd_class, MKDEV(LCDCMD_MAJOR, 0));  //delete device node under /dev
   class_destroy(lcdcmd_class);               //delete class created by us
	cdev_del(&lcdcmd->lcdcmd_cdev);
	kfree(lcdcmd);
	unregister_chrdev_region(MKDEV(LCDCMD_MAJOR,0),1);
	iounmap(Lcd_base0);
	iounmap(Lcd_base1);
  printk(KERN_ALERT "Close Lcdcmdsb Driver.\n");
}


module_init(at91_lcdcmd_init);
module_exit(at91_lcdcmd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ly");
MODULE_DESCRIPTION("lcdcmd driver for AT91RM9260");
