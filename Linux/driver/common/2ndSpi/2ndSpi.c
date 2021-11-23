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
MODULE_AUTHOR("ly");
MODULE_LICENSE("GPL");

static void __iomem  *pmc_base, *spi_base;

//交采 SPI0_CS0变量
struct spi_device    *spi1_cs0_spi;
static int           spi1_cs0_major=0;
struct cdev          spi1_cs0Cdev;
struct class         *classACSSpi1Cs0;

//交采 SPI1_CS1变量
struct spi_device    *spi1_cs1_spi;
static int           spi1_cs1_major=0;
struct cdev          spi1_cs1Cdev;
struct class         *classACSSpi1Cs1;

//控制单片机 SPI变量
struct spi_device    *spi0_cs1_spi;
static int           spi0_cs1_major=0;
struct cdev          spi0cs1Cdev;
struct class         *classSpi0Cs1;

unsigned char machineTypex;     //机型

void delayTime(int times)
{
	int i,j;
	for(i=0;i<times;i++)
  {
  	for(j=0;j<0x40;j++)
  	{
  		 ;
  	}
  }
}

void delayTimex(int times)
{
	int i;
	for(i=0;i<times;i++)
  {
  	;
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
	ret = spi_sync(spi1_cs1_spi, &m);

	mutex_unlock(&lock);
	
	return ret;
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
static ssize_t spi0_cs1_read(struct file *filp, char *buf, size_t size, loff_t *offp)
{
   int   ret;
	 char  *tmp;
	 char  readCommand;
 
   tmp = kmalloc(size, GFP_KERNEL);
   
   if(!tmp)
   {
      return -1;
   }
   
   readCommand = buf[0];   /*读命令*/
	 
	 ret = spi_write_then_read(spi0_cs1_spi, &readCommand, 0, tmp, size);
	 
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
函数名称:stc12_write
功能描述:stc12文件写操作
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static ssize_t spi0_cs1_write(struct file *flip, const char *buf, size_t size, loff_t *offp)
{
  unsigned char n,m;
  unsigned char tmpData;
	
	switch(machineTypex)
	{
		 case 2:    //沃电老终端,ATMEGA8L,软件模块SPI
		 case 4:    //沃电GDW376.1专变III型终端,ATMEGA8L,软件模块SPI
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
        break;
        
     default:   //CTRL_PROCESS_IS_STC12
     	 break;
  }
  
  return size;
}

/**************************************************
函数名称:spi0_cs1_ioctl
功能描述:SPIO-CS1文件I/O控制
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
int spi0_cs1_ioctl (struct inode *inode, struct file *filp, unsigned int command, unsigned long arg)
{
	 switch(command)
	 {
	 	 case 88:     //单片机类型通知
 	 	   machineTypex = arg;
	 	 
	 	   switch(machineTypex)
	 	   {
	 	 	   case 2:   //沃电老终端
	 	 	   case 4:   //沃电GDW376.1专变III型终端
           //定义SPI1模拟管脚
           at91_set_gpio_output(AT91_PIN_PA2,  0);  //SPCK输出
           at91_set_gpio_output(AT91_PIN_PC11, 1);  //NCPS1输出
           at91_set_gpio_output(AT91_PIN_PA1,  0);  //MOSI输出
           at91_set_gpio_input(AT91_PIN_PA0,   0);  //MISO输入
	 	 	  	 break;
	 	 	  	 
	 	 	   default:    //
	 	 	   	 break;
	 	   }
	 	   break;
	 }

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
static int spi0_cs1_release(struct inode *inode, struct file *filp)
{
   return 0;
}

/**************************************************
函数名称:spi1_cs0_llseek
功能描述:文件定位操作
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
loff_t spi1_cs0_llseek (struct file *filp, loff_t offset, int whence) 
{
  return filp->f_pos;
}

/**************************************************
函数名称:spi1_cs0_read
功能描述:spi1_cs0文件读操作
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static ssize_t spi1_cs0_read(struct file *filp, char *buf, size_t size, loff_t *offp)
{
	 unsigned char readCommand;
   int           n;
   int           tmpData;
   char          *tmp;
   int           ret;

   switch(machineTypex)
   {
   	  case 1:  //沃电GDW376.1集中器
   	  case 4:  //沃电GDW376.1专变III型终端
   	  case 5:  //沃电GDW376.1专变III型终端(CPU-QFP)
         readCommand = buf[0];   /*读命令*/
      
         tmp = kmalloc(size, GFP_KERNEL);
         
         if(!tmp)
         {
            return -1;
         }
      
         at91_set_gpio_value(AT91_PIN_PB3,0);
         
      	 ret = spi_write_then_read(spi1_cs0_spi, &readCommand, 1, tmp, size);
      	 if(ret)
      	 {
      		  printk(KERN_ALERT"read error @ spi_write_then_read ret=%d \n", ret);
      		  kfree(tmp);
      		  return -EINVAL;
      	 }
      
      	 ret = copy_to_user(buf, tmp, size);
         kfree(tmp);
      
         at91_set_gpio_value(AT91_PIN_PB3,1);
      
         return (size - ret);
         break;
         
      case 2:  //沃电老终端,软件SPI,速率为6.8K
         readCommand = buf[0];   /*读命令*/
      
         at91_set_gpio_value(AT91_PIN_PB3,1);     //CS=1
         at91_set_gpio_value(AT91_PIN_PB2,0);     //SCLK=0
         at91_set_gpio_value(AT91_PIN_PB3,0);     //CS=0
         
         //Send 8-bits Command to SPI
         for(n=7;n>=0;n--)
         {
           at91_set_gpio_value(AT91_PIN_PB2,1);   //SCLK=1
                
           delayTimex(2000);
           
           if (readCommand&0x80)
           {
             at91_set_gpio_value(AT91_PIN_PB1,1); //MOSI=1
           }
           else
           {
             at91_set_gpio_value(AT91_PIN_PB1,0); //MOSI=0
           }

           delayTimex(2000);
           
           at91_set_gpio_value(AT91_PIN_PB2,0);   //SCLK=0
           
           delayTimex(4000);
           
           at91_set_gpio_value(AT91_PIN_PB1,0); //MOSI=0

           readCommand<<=1;
         }
         
         //Read 24-bits Data From SPI
         for(n=23,tmpData=0;n>=0;n--)
         {
           at91_set_gpio_value(AT91_PIN_PB2,1);   //SCLK=1
           
           delayTimex(4000);
           
           tmpData |= at91_get_gpio_value(AT91_PIN_PB0)<<n;
           tmpData |= at91_get_gpio_value(AT91_PIN_PB0)<<n;
           
           at91_set_gpio_value(AT91_PIN_PB2,0);   //SCLK=0

           delayTimex(4000);

           at91_set_gpio_value(AT91_PIN_PB2,0);   //SCLK=0
         }
         
         at91_set_gpio_value(AT91_PIN_PB1,0);       //MOSI=0
         at91_set_gpio_value(AT91_PIN_PB3,1);       //CS=1

         buf[0] = tmpData>>16;
         buf[1] = tmpData>>8;
         buf[2] = tmpData&0xff;
         return (size);
         break;
   }
   
   return 0;
}

/**************************************************
函数名称:spi1_cs0_write
功能描述:文件写操作
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static ssize_t spi1_cs0_write(struct file *flip, const char *buf, size_t size, loff_t *offp)
{
  int ret;
  char *tmp;
  unsigned char tmpCmd;
  unsigned int  tmpData;
  int           n;

  switch(machineTypex)
  {
	  case 1:    //沃电GDW376.1集中器
	  case 4:    //沃电GDW376.1专变III型终端
	  case 5:    //沃电GDW376.1专变III型终端(CPU-QFP)
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
      
      at91_set_gpio_value(AT91_PIN_PB3,0);
    	
    	tmp[0] = buf[0];  /*命令第1字节*/
    	tmp[1] = buf[1];  /*命令第2字节*/
      tmp[2] = buf[2];  /*命令第3字节*/
      tmp[3] = buf[3];  /*命令第4字节*/
    	ret = spi_write(spi1_cs1_spi, tmp, size);
    	
      at91_set_gpio_value(AT91_PIN_PB3,1);

    	if(ret)
    	{
    		 size = -EINVAL;
      }
            
    	kfree(tmp);
    	break;
    	
    case 2:    //沃电老终端,软件SPI接口6.8K
      tmpCmd = buf[0];
      tmpData = buf[1]<<16 | buf[2]<<8 | buf[3];
      
      at91_set_gpio_value(AT91_PIN_PB3,1);     //CS=1
      at91_set_gpio_value(AT91_PIN_PB2,0);     //SCLK=0
      at91_set_gpio_value(AT91_PIN_PB3,0);     //CS=0
      
      //Send 8-bits Command to SPI
      for(n=7;n>=0;n--)
      {
        at91_set_gpio_value(AT91_PIN_PB2,1);   //SCLK=1
        
        delayTimex(2000);

        if (tmpCmd&0x80)
        {
          at91_set_gpio_value(AT91_PIN_PB1,1); //MOSI=1
        }
        else
        {
          at91_set_gpio_value(AT91_PIN_PB1,0); //MOSI=0
        }
        delayTimex(2000);
        
        at91_set_gpio_value(AT91_PIN_PB2,0);   //SCLK=0

        delayTimex(4000);
        
        tmpCmd<<=1;
      }
   
      //Send 24-bits Data to SPI
      for(n=23;n>=0;n--)
      {
        at91_set_gpio_value(AT91_PIN_PB2,1);   //SCLK=1
        delayTimex(2000);           
        
        if (tmpData&0x800000)
        {
          at91_set_gpio_value(AT91_PIN_PB1,1); //MOSI=1
        }
        else
        {
          at91_set_gpio_value(AT91_PIN_PB1,0); //MOSI=0
        }
        delayTimex(2000);
        
        at91_set_gpio_value(AT91_PIN_PB2,0);   //SCLK=0
        
        delayTimex(4000);

        tmpData<<=1;
      }
      at91_set_gpio_value(AT91_PIN_PB3,1);     //CS=1
      
      delayTime(5);
            
      break;
  }
  
  return size;
}

/**************************************************
函数名称:spi1_cs0_ioctl
功能描述:文件I/O控制
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
int spi1_cs0_ioctl(struct inode *inode, struct file *filp, unsigned int command, unsigned long arg)
{
	 unsigned char ret=0;
	 int i;
	 
	 switch(command)
	 {
	 	 case 1:   //READ_SIG_OUT_VALUE
	     switch(machineTypex)
	     {
	     	 case 2:    //沃电老终端
	     	 case 4:    //沃电GDW376.1专变III型终端
	     	 case 5:    //沃电GDW376.1专变III型终端(CPU-QFP)
			     if (at91_get_gpio_value(AT91_PIN_PC9))  /*SIG_OUT*/
			     {
				     ret = 1;
			     }
	 	       break;
	 	   }
	 	   break;

	 	 case 2:   //读取是否有交采模块
	 	 	 switch(machineTypex)
	 	 	 {
	 	 	 	  case 4:   //沃电GDW376.1专变III型终端
	 	 	 	  case 5:   //沃电GDW376.1专变III型终端(CPU-QFP)
			      if (at91_get_gpio_value(AT91_PIN_PA27))
			      {
				      ret = 1;
			      }
	 	 	 	  	break;
	 	 	 }
	 	   break;
	 	   	
	 	 case 3:   //复位交采芯片
	     switch(machineTypex)
	     {
	     	 case 2:    //沃电老终端
	     	 case 4:    //沃电GDW376.1专变III型终端
	         at91_set_gpio_value(AT91_PIN_PC7,0);
           //停止100us
           for(i=0;i<0x600000;i++)
           {
     	       ;
           }     
	         at91_set_gpio_value(AT91_PIN_PC7,1);
	         break;
	         
	       case 5:    //沃电GDW376.1专变III型终端(CPU-QFP)
	         at91_set_gpio_value(AT91_PIN_PA28,0);
           //停止100us
           for(i=0;i<0x600000;i++)
           {
     	       ;
           }
	         at91_set_gpio_value(AT91_PIN_PA28,1);
	       	 break;
	     } 
	 	 	 break;
	 	 	  
	 	 case 4:   //设置交采模块模式
	 	 	 switch(machineTypex)
	 	 	 {
	 	 	 	 case 2:    //沃电老终端
	 	 	     at91_set_gpio_value(AT91_PIN_PA27,arg);
	 	 	     break;
	 	 	     
	 	 	   case 4:    //沃电GDW376.1专变III型终端
	 	 	   case 5:    //沃电GDW376.1专变III型终端(CPU-QFP)
	 	 	     at91_set_gpio_value(AT91_PIN_PC0,arg);
	 	 	   	 break;
	 	 	 }
	 	 	 break;
	 	 	 
	 	 case 88:
       machineTypex = arg;
       printk("设置机型为%d\n",machineTypex);
       
       switch(machineTypex)
       {
       	  default:   //默认为沃电集中器
       	  	break;
       	  	
       	  case 2:    //沃电老终端
            //定义SPI1模拟管脚
            at91_set_gpio_output(AT91_PIN_PB2, 0);   //SPCK输出
            at91_set_gpio_output(AT91_PIN_PB3, 1);   //NCPS0输出
            at91_set_gpio_output(AT91_PIN_PB1, 0);   //MOSI输出
            at91_set_gpio_input(AT91_PIN_PB0,  0);   //MISO输入

            /*复位ATT7022B IO脚定义为输出*/
            if (at91_set_gpio_output(AT91_PIN_PC7, 0)==0)
            {
           	   printk(KERN_ALERT"Reset ATT7022B...\n");
                
                //停止100us
                for(i=0;i<0x600000;i++)
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
           	break;
           	
          case 3:
         	  break;
         	  
         	case 4:    //沃电专变III型终端
         	case 5:    //沃电专变III型终端(CPU-QFP)
            if (machineTypex==5)
            {
              /*复位ATT7022B IO脚定义为输出*/
              if (at91_set_gpio_output(AT91_PIN_PA28, 0)==0)
              {
           	     printk(KERN_ALERT"Reset ATT7022B(专变III型终端,QFP)...\n");
                
                 //停止100us
                 for(i=0;i<0x600000;i++)
                 {
                	  ;
                 }
                
           	    at91_set_gpio_value(AT91_PIN_PA28,1);
              }
              else
              {
           	     printk(KERN_ALERT "set AT91_PIN_PA28 failed!\n");
              }

            }
            else
            {
              /*复位ATT7022B IO脚定义为输出*/
              if (at91_set_gpio_output(AT91_PIN_PC7, 0)==0)
              {
           	     printk(KERN_ALERT"Reset ATT7022B(专变III型终端)...\n");
                
                 //停止100us
                 for(i=0;i<0x600000;i++)
                 {
                	  ;
                 }
                
           	    at91_set_gpio_value(AT91_PIN_PC7,1);
              }
              else
              {
           	     printk(KERN_ALERT "set AT91_PIN_PC7 failed!\n");
              }
            }
           
           	/*ATT7022B模式设置脚定义为输出*/
           	if (at91_set_gpio_output(AT91_PIN_PC0, 0)==0)
           	{
           		;
           	}
           	else
           	{
           	  printk(KERN_ALERT "set AT91_PIN_PC0 failed!\n");
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

           	/*有无交采模块的信号脚IO配置为输入*/
           	if (at91_set_gpio_input(AT91_PIN_PA27, 0)==0)
           	{
           	  ;
           	}
           	else
           	{
           	  printk(KERN_ALERT "set AT91_PIN_PA27 failed!\n");
           	}
         		break;
       } 
	 	 	 break;
	 }
	 
	 return ret;
}


/**************************************************
函数名称:spi1_cs0_open
功能描述:交采文件打开时的处理
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/ 
static int spi1_cs0_open(struct inode *inode, struct file *filp)
{
   return 0;
}
 
 
/**************************************************
函数名称:spi1_cs0_release
功能描述:释放
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/ 
static int spi1_cs0_release(struct inode *inode, struct file *filp)
{
   return 0;
}

/**************************************************
函数名称:spi1_cs1_llseek
功能描述:文件定位操作
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
loff_t spi1_cs1_llseek (struct file *filp, loff_t offset, int whence) 
{
  return filp->f_pos;
}

/**************************************************
函数名称:spi1_cs1_read
功能描述:spi1_cs1文件读操作
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static ssize_t spi1_cs1_read(struct file *filp, char *buf, size_t size, loff_t *offp)
{
  return 0;
}

/**************************************************
函数名称:spi1_cs1_write
功能描述:文件写操作
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static ssize_t spi1_cs1_write(struct file *flip, const char *buf, size_t size, loff_t *offp)
{
  return 0;
}

/**************************************************
函数名称:spi1_cs1_ioctl
功能描述:文件I/O控制
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
int spi1_cs1_ioctl (struct inode *inode, struct file *filp, unsigned int command, unsigned long arg)
{
	 return 0;
}


/**************************************************
函数名称:spi1_cs1_open
功能描述:交采文件打开时的处理
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/ 
static int spi1_cs1_open(struct inode *inode, struct file *filp)
{
   return 0;
}
 
 
/**************************************************
函数名称:spi1_cs1_release
功能描述:释放
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/ 
static int spi1_cs1_release(struct inode *inode, struct file *filp)
{
   return 0;
}

/*SPI接口SPI0 CS1字符驱动 文件操作结构赋值*/
static struct file_operations spi0_cs1_fops = {
        .owner  = THIS_MODULE,
        .read   = spi0_cs1_read,
        .write  = spi0_cs1_write,
        .ioctl  = spi0_cs1_ioctl,
        .release= spi0_cs1_release,
};

/*SPI接口SPI1 CS0字符驱动 文件操作结构赋值*/
static struct file_operations spi1_cs0_fops = {
        .owner  = THIS_MODULE,
        .llseek = spi1_cs0_llseek,
        .read   = spi1_cs0_read,
        .write  = spi1_cs0_write,
        .ioctl  = spi1_cs0_ioctl,
        .open   = spi1_cs0_open,
        .release= spi1_cs0_release,
};

/*SPI接口SPI1 CS1字符驱动 文件操作结构赋值*/
static struct file_operations spi1_cs1_fops = {
        .owner  = THIS_MODULE,
        .llseek = spi1_cs1_llseek,
        .read   = spi1_cs1_read,
        .write  = spi1_cs1_write,
        .ioctl  = spi1_cs1_ioctl,
        .open   = spi1_cs1_open,
        .release= spi1_cs1_release,
};

/**************************************************
函数名称:spi1_cs0_probe
功能描述:注册SPI1_CS0交采字符驱动
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static int spi1_cs0_probe(struct spi_device *spi)
{
	if (!(spi1_cs0_spi = kzalloc(sizeof *spi1_cs0_spi, GFP_KERNEL)))
	{
     goto fail;
	}
	
	spi1_cs0_spi = spi_dev_get(spi);
  printk(KERN_ALERT"name:%s  CS=%d  mode=%x speed=%d.\n", \
	  spi1_cs0_spi->modalias, spi1_cs0_spi->chip_select, spi1_cs0_spi->mode, spi1_cs0_spi->max_speed_hz);
	
	return 0;

fail:
  printk(KERN_ALERT"spi1_cs1_probe ERROR \n");
  kfree(spi1_cs1_spi);
  
  return -1;
}

/**************************************************
函数名称:spi1_cs0_remove
功能描述:
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static int spi1_cs0_remove(struct spi_device *spi)
{
	return 0;
}

/**************************************************
函数名称:spi1_cs1_probe
功能描述:注册SPI1_CS1交采字符驱动
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static int spi1_cs1_probe(struct spi_device *spi)
{
	if (!(spi1_cs1_spi = kzalloc(sizeof *spi1_cs1_spi, GFP_KERNEL)))
	{
     goto fail;
	}
	
	spi1_cs1_spi = spi_dev_get(spi);
  printk(KERN_ALERT"name:%s  CS=%d  mode=%x speed=%d.\n", \
	  spi1_cs1_spi->modalias, spi1_cs1_spi->chip_select, spi1_cs1_spi->mode, spi1_cs1_spi->max_speed_hz);
	
	return 0;

fail:
  printk(KERN_ALERT"spi1_cs1_probe ERROR \n");
  kfree(spi1_cs1_spi);
  
  return -1;
}

/**************************************************
函数名称:spi1_cs1_remove
功能描述:
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值：
***************************************************/
static int spi1_cs1_remove(struct spi_device *spi)
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
static int spi_spi0_cs1_probe(struct spi_device *spi)
{
	if (!(spi0_cs1_spi = kzalloc(sizeof *spi0_cs1_spi, GFP_KERNEL)))
	{
     goto fail;
	}
	 
	spi0_cs1_spi = spi_dev_get(spi);
  printk(KERN_ALERT"name:%s  CS=%d  mode=%x speed=%d.\n", \
	   spi0_cs1_spi->modalias, spi0_cs1_spi->chip_select, spi0_cs1_spi->mode, spi0_cs1_spi->max_speed_hz);
	
	return 0;

fail:
  printk(KERN_ALERT"spi_stc12_probe ERROR \n");
  kfree(spi1_cs1_spi);
  
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
static int spi_spi0_cs1_remove(struct spi_device *spi)
{
	return 0;
}

//交流采样SPI字符驱动结构
static struct spi_driver spi_spi1_cs1_driver = {
   .driver = 
   {
      .name        = "spi1_cs1",
      .owner       = THIS_MODULE,
   },
   .probe          = spi1_cs1_probe,
   .remove         = spi1_cs1_remove,
};

//交流采样SPI字符驱动结构
static struct spi_driver spi_spi1_cs0_driver = {
   .driver = 
   {
      .name        = "spi1_cs0",
      .owner       = THIS_MODULE,
   },
   .probe          = spi1_cs0_probe,
   .remove         = spi1_cs0_remove,
};

//控制单片机SPI字符驱动结构
static struct spi_driver spi_spi0_cs1_driver = {
   .driver = 
   {
      .name        = "spi0_cs1",
      .owner       = THIS_MODULE,
   },
   .probe          = spi_spi0_cs1_probe,
   .remove         = spi_spi0_cs1_remove,
};

static void spi1_cs0_setup_cdev(void)
{
	 int error;
	 int devno, tmpMinor=0;
	 
   devno = MKDEV (spi1_cs0_major, tmpMinor);
   
   cdev_init (&spi1_cs0Cdev, &spi1_cs0_fops);
   
   spi1_cs1Cdev.owner = THIS_MODULE;
   spi1_cs1Cdev.ops = &spi1_cs0_fops;
   error = cdev_add(&spi1_cs0Cdev, devno,1);

   if(error)
   {
     printk(KERN_NOTICE "Error %d adding spi1_cs0_setup_cdev",error);
   }
}

static void spi1_cs1_setup_cdev(void)
{
	 int error;
	 int devno, tmpMinor=0;
	 
   devno = MKDEV (spi1_cs1_major, tmpMinor);
   
   cdev_init (&spi1_cs1Cdev, &spi1_cs1_fops);
   
   spi1_cs1Cdev.owner = THIS_MODULE;
   spi1_cs1Cdev.ops = &spi1_cs1_fops;
   error = cdev_add(&spi1_cs1Cdev, devno,1);

   if(error)
   {
     printk(KERN_NOTICE "Error %d adding spi1_cs1_setup_cdev",error);
   }
}

static void spi0_cs1_setup_cdev(void)
{
	 int error;
	 int devno, tmpMinor=0;	 

   devno = MKDEV (spi0_cs1_major, tmpMinor);
   
   cdev_init (&spi0cs1Cdev, &spi0_cs1_fops);
   
   spi0cs1Cdev.owner = THIS_MODULE;
   spi0cs1Cdev.ops = &spi0_cs1_fops;
   error = cdev_add(&spi0cs1Cdev, devno,1);

   if(error)
   {
     printk(KERN_NOTICE "Error %d adding spi0_cs1_setup_cdev",error);
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
  at91_set_B_periph(AT91_PIN_PC5, 1);		/*SPI1_NPCS1*/
  //at91_set_gpio_output(AT91_PIN_PC5, 1);
  at91_set_gpio_output(AT91_PIN_PB3, 1);/*由于用作SPI接口与ATT7022B通信时,ATT7022B每一次读数只要一个CS,因此要手动置CS*/

  /*第3步:*/
  /*这一步可以更改SPI的参数,比如说数据位数、时钟、相位、速率等*/
  /* setup spi1 */
  spi_base = ioremap(SPI_BASE, 512);

  /* reset spi1 */
  __raw_writel(1<<7, spi_base + SPI_CR);

  /* config MR(Mode Register,模式寄存器)*/
  __raw_writel(1 | 1<<4 | 0xF<<16, spi_base + SPI_MR);

  /* config CSR0(Chip Select Register 0)*/
  //__raw_writel(0x0101ff00, spi_base + SPI_CSR0);   //400K
  __raw_writel(0x01016300, spi_base + SPI_CSR0);   //1M
  //__raw_writel(0x01013000, spi_base + SPI_CSR0);   //2M

  /* config CSR1(Chip Select Register 0)*/
  __raw_writel(0x00001900, spi_base + SPI_CSR1);
 
  /*使能SPI1(spi1 enable,Control Register[控制寄存器])*/
  __raw_writel(1, spi_base + SPI_CR);
 
  iounmap(spi_base);

	/*********************SPI1_CS0驱动****开始********************************************************/
	/*注册驱动*/
	ret=spi_register_driver(&spi_spi1_cs0_driver);
	if(ret)
	{
		spi_unregister_driver(&spi_spi1_cs0_driver);
		return -1;
	}
 
  /*自动分配一个设备号*/
	if (alloc_chrdev_region(&devNo,0,1,"spi1_cs0") < 0)
	{
		 return -2;
	}
	
	spi1_cs0_major= MAJOR(devNo);

  spi1_cs0_setup_cdev();
  
  /* create your own class under /sysfs */
  classACSSpi1Cs0 = class_create(THIS_MODULE, "spi1_cs0");
  if(IS_ERR(classACSSpi1Cs0))
  {
    printk("Power Measure Err: failed in creating class.\n");
    return -1;
  }

  device_create(classACSSpi1Cs0, NULL, MKDEV(spi1_cs0_major, 0), "spi1_cs0");

	/*********************SPI1_CS0驱动****结束********************************************************/  
  
	/*********************SPI1_CS1驱动****开始********************************************************/
	/*注册驱动*/
	ret=spi_register_driver(&spi_spi1_cs1_driver);
	if(ret)
	{
		spi_unregister_driver(&spi_spi1_cs1_driver);
		return -1;
	}
 
  /*自动分配一个设备号*/
	if (alloc_chrdev_region(&devNo,0,1,"spi1_cs1") < 0)
	{
		 return -2;
	}
	
	spi1_cs1_major= MAJOR(devNo);

  spi1_cs1_setup_cdev();
  
  /* create your own class under /sysfs */
  classACSSpi1Cs1 = class_create(THIS_MODULE, "spi1_cs1");
  if(IS_ERR(classACSSpi1Cs1))
  {
    printk("Power Measure Err: failed in creating class.\n");
    return -1;
  }

  device_create(classACSSpi1Cs1, NULL, MKDEV(spi1_cs1_major, 0), "spi1_cs1");

	/*********************SPI1_CS1驱动****结束********************************************************/
  
	/*********************SPI0 CS1驱动****开始********************************************************/	
	/*注册驱动*/
	ret=spi_register_driver(&spi_spi0_cs1_driver);
	if(ret)
	{
		spi_unregister_driver(&spi_spi0_cs1_driver);
		return -1;
	}
 
  /*自动分配一个设备号*/
	if (alloc_chrdev_region(&devNo,0,1,"spi0_cs1") < 0)
	{
		 return -2;
	}
	
	spi0_cs1_major= MAJOR(devNo);

  spi0_cs1_setup_cdev();
  
  /* create your own class under /sysfs */
  classSpi0Cs1 = class_create(THIS_MODULE, "spi0_cs1");
  if(IS_ERR(classSpi0Cs1))
  {
    printk("STC12 Err: failed in creating class.\n");
    return -1;
  }

  device_create(classSpi0Cs1, NULL, MKDEV(spi0_cs1_major, 0), "spi0_cs1");
	/*********************SPI0 CS1驱动*****结束********************************************************/	

	printk(KERN_ALERT"SPI Second char driver init Done!\n");
 	
	return 0;
}

/**************************************************
函数名称:secondSpi_exit
功能描述:退出SPI字符驱动(交流采样,与单片机通信(stc12))
调用函数:
被调用函数:
输入参数:无
输出参数:
返回值:void
***************************************************/
static void __exit secondSpi_exit(void)
{
	 dev_t devno;
	 
	 spi_unregister_driver(&spi_spi1_cs1_driver);
	 
   devno = MKDEV (spi1_cs1_major, 0);
   
   cdev_del (&spi1_cs1Cdev);
      
   //delete device node under /dev
   device_destroy(classACSSpi1Cs1, MKDEV(spi1_cs1_major, 0)); 

   //delete class created by us
   class_destroy(classACSSpi1Cs1);

   unregister_chrdev_region (devno, 1);	 

	 spi_unregister_driver(&spi_spi0_cs1_driver);
	 
   devno = MKDEV (spi0_cs1_major, 0);
   
   cdev_del (&spi0cs1Cdev);
      
   //delete device node under /dev
   device_destroy(classSpi0Cs1, MKDEV(spi0_cs1_major, 0)); 

   //delete class created by us
   class_destroy(classSpi0Cs1);

   unregister_chrdev_region (devno, 1);
}

module_init(secondSpi_init);
module_exit(secondSpi_exit);

