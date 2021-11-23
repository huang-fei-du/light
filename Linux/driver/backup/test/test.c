#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>
#include <linux/init.h>         /* Needed for the module-macros */
#include <linux/fs.h>
#include <asm/arch/gpio.h>

#define DEVICE_NAME "Test"

static int test_MAJOR = 225;

static int test_ioctl(struct inode *inode, struct file *file,unsigned int cmd, unsigned long arg)
{
   switch(cmd)
   {
			case 1:
				printk(KERN_ALERT "cmd 1 set PIN_PA4 to %d.\n",(int)arg);
				at91_set_gpio_value(AT91_PIN_PA4,arg);
				break;
				
			case 2:
				printk(KERN_ALERT "cmd 2 set PIN_PA4 to 0.\n");
				at91_set_gpio_value(AT91_PIN_PA4,0);
				break;
				
			case 3:  printk(KERN_ALERT "cmd 3.\n");  break;
			case 4:  printk(KERN_ALERT "cmd 4.\n");  break;
			default:  printk(KERN_ALERT "cmd default.\n");  break;
   }
   
	 return 0;
}

static int test_open(struct inode *inode, struct file *file)
{
	printk(KERN_ALERT "test open sucess!\n");
	if (at91_set_gpio_output(AT91_PIN_PA4, 0)==0)
	{
	   printk(KERN_ALERT "set AT91_PIN_PA4 sucess!\n");
	}
	else
	{
	   printk(KERN_ALERT "set AT91_PIN_PA4 failed!\n");
	}
	
	return 0;
}

static int test_release(struct inode *inode, struct file *filp)
{
	printk(KERN_ALERT "test release.\n");
	return 0;
}

static ssize_t test_read(struct file *filp,char __user *buf,
		size_t size,loff_t *ppos)
{
	printk(KERN_ALERT "test read.\n");
	return 0;
}

static ssize_t test_write(struct file *filp,const char __user *buf,
		size_t size,loff_t *ppos)
{
	printk(KERN_ALERT "test write.\n");
	return 0;
}

static struct file_operations test_fops = {
 .owner   = THIS_MODULE,
 .ioctl   = test_ioctl,
 .open    = test_open,
 .release = test_release,
 .read    = test_read,
 .write   = test_write,
};

static int __init test_init(void)
{
	int result;
	result=register_chrdev(test_MAJOR,DEVICE_NAME,&test_fops);
  if (result < 0) return result;
  printk(KERN_ALERT "Ly Test Driver.\n");
  return 0;
}
 
static void __exit test_exit(void)
{
	unregister_chrdev(test_MAJOR,DEVICE_NAME);
  printk(KERN_ALERT "Close Ly Test Driver.\n");
}

module_init(test_init);

module_exit(test_exit);

MODULE_LICENSE("Dual BSD/GPL");  //should always exist or you¡¯ll get a warning

MODULE_AUTHOR("LY"); //optional

MODULE_DESCRIPTION("TEST_MODULE"); //optional

MODULE_VERSION("V1.0"); 

