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

//���� SPI0_CS0����
struct spi_device    *spi1_cs0_spi;
static int           spi1_cs0_major=0;
struct cdev          spi1_cs0Cdev;
struct class         *classACSSpi1Cs0;

//���� SPI1_CS1����
struct spi_device    *spi1_cs1_spi;
static int           spi1_cs1_major=0;
struct cdev          spi1_cs1Cdev;
struct class         *classACSSpi1Cs1;

//���Ƶ�Ƭ�� SPI����
struct spi_device    *spi0_cs1_spi;
static int           spi0_cs1_major=0;
struct cdev          spi0cs1Cdev;
struct class         *classSpi0Cs1;

unsigned char machineTypex;     //����

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
	ret = spi_sync(spi1_cs1_spi, &m);

	mutex_unlock(&lock);
	
	return ret;
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
   
   readCommand = buf[0];   /*������*/
	 
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
��������:stc12_write
��������:stc12�ļ�д����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/
static ssize_t spi0_cs1_write(struct file *flip, const char *buf, size_t size, loff_t *offp)
{
  unsigned char n,m;
  unsigned char tmpData;
	
	switch(machineTypex)
	{
		 case 2:    //�ֵ����ն�,ATMEGA8L,���ģ��SPI
		 case 4:    //�ֵ�GDW376.1ר��III���ն�,ATMEGA8L,���ģ��SPI
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
��������:spi0_cs1_ioctl
��������:SPIO-CS1�ļ�I/O����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/
int spi0_cs1_ioctl (struct inode *inode, struct file *filp, unsigned int command, unsigned long arg)
{
	 switch(command)
	 {
	 	 case 88:     //��Ƭ������֪ͨ
 	 	   machineTypex = arg;
	 	 
	 	   switch(machineTypex)
	 	   {
	 	 	   case 2:   //�ֵ����ն�
	 	 	   case 4:   //�ֵ�GDW376.1ר��III���ն�
           //����SPI1ģ��ܽ�
           at91_set_gpio_output(AT91_PIN_PA2,  0);  //SPCK���
           at91_set_gpio_output(AT91_PIN_PC11, 1);  //NCPS1���
           at91_set_gpio_output(AT91_PIN_PA1,  0);  //MOSI���
           at91_set_gpio_input(AT91_PIN_PA0,   0);  //MISO����
	 	 	  	 break;
	 	 	  	 
	 	 	   default:    //
	 	 	   	 break;
	 	   }
	 	   break;
	 }

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
static int spi0_cs1_release(struct inode *inode, struct file *filp)
{
   return 0;
}

/**************************************************
��������:spi1_cs0_llseek
��������:�ļ���λ����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/
loff_t spi1_cs0_llseek (struct file *filp, loff_t offset, int whence) 
{
  return filp->f_pos;
}

/**************************************************
��������:spi1_cs0_read
��������:spi1_cs0�ļ�������
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
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
   	  case 1:  //�ֵ�GDW376.1������
   	  case 4:  //�ֵ�GDW376.1ר��III���ն�
   	  case 5:  //�ֵ�GDW376.1ר��III���ն�(CPU-QFP)
         readCommand = buf[0];   /*������*/
      
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
         
      case 2:  //�ֵ����ն�,���SPI,����Ϊ6.8K
         readCommand = buf[0];   /*������*/
      
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
��������:spi1_cs0_write
��������:�ļ�д����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
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
	  case 1:    //�ֵ�GDW376.1������
	  case 4:    //�ֵ�GDW376.1ר��III���ն�
	  case 5:    //�ֵ�GDW376.1ר��III���ն�(CPU-QFP)
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
    	
    	tmp[0] = buf[0];  /*�����1�ֽ�*/
    	tmp[1] = buf[1];  /*�����2�ֽ�*/
      tmp[2] = buf[2];  /*�����3�ֽ�*/
      tmp[3] = buf[3];  /*�����4�ֽ�*/
    	ret = spi_write(spi1_cs1_spi, tmp, size);
    	
      at91_set_gpio_value(AT91_PIN_PB3,1);

    	if(ret)
    	{
    		 size = -EINVAL;
      }
            
    	kfree(tmp);
    	break;
    	
    case 2:    //�ֵ����ն�,���SPI�ӿ�6.8K
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
��������:spi1_cs0_ioctl
��������:�ļ�I/O����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
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
	     	 case 2:    //�ֵ����ն�
	     	 case 4:    //�ֵ�GDW376.1ר��III���ն�
	     	 case 5:    //�ֵ�GDW376.1ר��III���ն�(CPU-QFP)
			     if (at91_get_gpio_value(AT91_PIN_PC9))  /*SIG_OUT*/
			     {
				     ret = 1;
			     }
	 	       break;
	 	   }
	 	   break;

	 	 case 2:   //��ȡ�Ƿ��н���ģ��
	 	 	 switch(machineTypex)
	 	 	 {
	 	 	 	  case 4:   //�ֵ�GDW376.1ר��III���ն�
	 	 	 	  case 5:   //�ֵ�GDW376.1ר��III���ն�(CPU-QFP)
			      if (at91_get_gpio_value(AT91_PIN_PA27))
			      {
				      ret = 1;
			      }
	 	 	 	  	break;
	 	 	 }
	 	   break;
	 	   	
	 	 case 3:   //��λ����оƬ
	     switch(machineTypex)
	     {
	     	 case 2:    //�ֵ����ն�
	     	 case 4:    //�ֵ�GDW376.1ר��III���ն�
	         at91_set_gpio_value(AT91_PIN_PC7,0);
           //ֹͣ100us
           for(i=0;i<0x600000;i++)
           {
     	       ;
           }     
	         at91_set_gpio_value(AT91_PIN_PC7,1);
	         break;
	         
	       case 5:    //�ֵ�GDW376.1ר��III���ն�(CPU-QFP)
	         at91_set_gpio_value(AT91_PIN_PA28,0);
           //ֹͣ100us
           for(i=0;i<0x600000;i++)
           {
     	       ;
           }
	         at91_set_gpio_value(AT91_PIN_PA28,1);
	       	 break;
	     } 
	 	 	 break;
	 	 	  
	 	 case 4:   //���ý���ģ��ģʽ
	 	 	 switch(machineTypex)
	 	 	 {
	 	 	 	 case 2:    //�ֵ����ն�
	 	 	     at91_set_gpio_value(AT91_PIN_PA27,arg);
	 	 	     break;
	 	 	     
	 	 	   case 4:    //�ֵ�GDW376.1ר��III���ն�
	 	 	   case 5:    //�ֵ�GDW376.1ר��III���ն�(CPU-QFP)
	 	 	     at91_set_gpio_value(AT91_PIN_PC0,arg);
	 	 	   	 break;
	 	 	 }
	 	 	 break;
	 	 	 
	 	 case 88:
       machineTypex = arg;
       printk("���û���Ϊ%d\n",machineTypex);
       
       switch(machineTypex)
       {
       	  default:   //Ĭ��Ϊ�ֵ缯����
       	  	break;
       	  	
       	  case 2:    //�ֵ����ն�
            //����SPI1ģ��ܽ�
            at91_set_gpio_output(AT91_PIN_PB2, 0);   //SPCK���
            at91_set_gpio_output(AT91_PIN_PB3, 1);   //NCPS0���
            at91_set_gpio_output(AT91_PIN_PB1, 0);   //MOSI���
            at91_set_gpio_input(AT91_PIN_PB0,  0);   //MISO����

            /*��λATT7022B IO�Ŷ���Ϊ���*/
            if (at91_set_gpio_output(AT91_PIN_PC7, 0)==0)
            {
           	   printk(KERN_ALERT"Reset ATT7022B...\n");
                
                //ֹͣ100us
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
           	break;
           	
          case 3:
         	  break;
         	  
         	case 4:    //�ֵ�ר��III���ն�
         	case 5:    //�ֵ�ר��III���ն�(CPU-QFP)
            if (machineTypex==5)
            {
              /*��λATT7022B IO�Ŷ���Ϊ���*/
              if (at91_set_gpio_output(AT91_PIN_PA28, 0)==0)
              {
           	     printk(KERN_ALERT"Reset ATT7022B(ר��III���ն�,QFP)...\n");
                
                 //ֹͣ100us
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
              /*��λATT7022B IO�Ŷ���Ϊ���*/
              if (at91_set_gpio_output(AT91_PIN_PC7, 0)==0)
              {
           	     printk(KERN_ALERT"Reset ATT7022B(ר��III���ն�)...\n");
                
                 //ֹͣ100us
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
           
           	/*ATT7022Bģʽ���ýŶ���Ϊ���*/
           	if (at91_set_gpio_output(AT91_PIN_PC0, 0)==0)
           	{
           		;
           	}
           	else
           	{
           	  printk(KERN_ALERT "set AT91_PIN_PC0 failed!\n");
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

           	/*���޽���ģ����źŽ�IO����Ϊ����*/
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
��������:spi1_cs0_open
��������:�����ļ���ʱ�Ĵ���
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/ 
static int spi1_cs0_open(struct inode *inode, struct file *filp)
{
   return 0;
}
 
 
/**************************************************
��������:spi1_cs0_release
��������:�ͷ�
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/ 
static int spi1_cs0_release(struct inode *inode, struct file *filp)
{
   return 0;
}

/**************************************************
��������:spi1_cs1_llseek
��������:�ļ���λ����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/
loff_t spi1_cs1_llseek (struct file *filp, loff_t offset, int whence) 
{
  return filp->f_pos;
}

/**************************************************
��������:spi1_cs1_read
��������:spi1_cs1�ļ�������
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/
static ssize_t spi1_cs1_read(struct file *filp, char *buf, size_t size, loff_t *offp)
{
  return 0;
}

/**************************************************
��������:spi1_cs1_write
��������:�ļ�д����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/
static ssize_t spi1_cs1_write(struct file *flip, const char *buf, size_t size, loff_t *offp)
{
  return 0;
}

/**************************************************
��������:spi1_cs1_ioctl
��������:�ļ�I/O����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/
int spi1_cs1_ioctl (struct inode *inode, struct file *filp, unsigned int command, unsigned long arg)
{
	 return 0;
}


/**************************************************
��������:spi1_cs1_open
��������:�����ļ���ʱ�Ĵ���
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/ 
static int spi1_cs1_open(struct inode *inode, struct file *filp)
{
   return 0;
}
 
 
/**************************************************
��������:spi1_cs1_release
��������:�ͷ�
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/ 
static int spi1_cs1_release(struct inode *inode, struct file *filp)
{
   return 0;
}

/*SPI�ӿ�SPI0 CS1�ַ����� �ļ������ṹ��ֵ*/
static struct file_operations spi0_cs1_fops = {
        .owner  = THIS_MODULE,
        .read   = spi0_cs1_read,
        .write  = spi0_cs1_write,
        .ioctl  = spi0_cs1_ioctl,
        .release= spi0_cs1_release,
};

/*SPI�ӿ�SPI1 CS0�ַ����� �ļ������ṹ��ֵ*/
static struct file_operations spi1_cs0_fops = {
        .owner  = THIS_MODULE,
        .llseek = spi1_cs0_llseek,
        .read   = spi1_cs0_read,
        .write  = spi1_cs0_write,
        .ioctl  = spi1_cs0_ioctl,
        .open   = spi1_cs0_open,
        .release= spi1_cs0_release,
};

/*SPI�ӿ�SPI1 CS1�ַ����� �ļ������ṹ��ֵ*/
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
��������:spi1_cs0_probe
��������:ע��SPI1_CS0�����ַ�����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
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
��������:spi1_cs0_remove
��������:
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/
static int spi1_cs0_remove(struct spi_device *spi)
{
	return 0;
}

/**************************************************
��������:spi1_cs1_probe
��������:ע��SPI1_CS1�����ַ�����
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
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
��������:spi1_cs1_remove
��������:
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/
static int spi1_cs1_remove(struct spi_device *spi)
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
��������:
��������:
���ú���:
�����ú���:
�������:��
�������:
����ֵ��
***************************************************/
static int spi_spi0_cs1_remove(struct spi_device *spi)
{
	return 0;
}

//��������SPI�ַ������ṹ
static struct spi_driver spi_spi1_cs1_driver = {
   .driver = 
   {
      .name        = "spi1_cs1",
      .owner       = THIS_MODULE,
   },
   .probe          = spi1_cs1_probe,
   .remove         = spi1_cs1_remove,
};

//��������SPI�ַ������ṹ
static struct spi_driver spi_spi1_cs0_driver = {
   .driver = 
   {
      .name        = "spi1_cs0",
      .owner       = THIS_MODULE,
   },
   .probe          = spi1_cs0_probe,
   .remove         = spi1_cs0_remove,
};

//���Ƶ�Ƭ��SPI�ַ������ṹ
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
  at91_set_B_periph(AT91_PIN_PC5, 1);		/*SPI1_NPCS1*/
  //at91_set_gpio_output(AT91_PIN_PC5, 1);
  at91_set_gpio_output(AT91_PIN_PB3, 1);/*��������SPI�ӿ���ATT7022Bͨ��ʱ,ATT7022Bÿһ�ζ���ֻҪһ��CS,���Ҫ�ֶ���CS*/

  /*��3��:*/
  /*��һ�����Ը���SPI�Ĳ���,����˵����λ����ʱ�ӡ���λ�����ʵ�*/
  /* setup spi1 */
  spi_base = ioremap(SPI_BASE, 512);

  /* reset spi1 */
  __raw_writel(1<<7, spi_base + SPI_CR);

  /* config MR(Mode Register,ģʽ�Ĵ���)*/
  __raw_writel(1 | 1<<4 | 0xF<<16, spi_base + SPI_MR);

  /* config CSR0(Chip Select Register 0)*/
  //__raw_writel(0x0101ff00, spi_base + SPI_CSR0);   //400K
  __raw_writel(0x01016300, spi_base + SPI_CSR0);   //1M
  //__raw_writel(0x01013000, spi_base + SPI_CSR0);   //2M

  /* config CSR1(Chip Select Register 0)*/
  __raw_writel(0x00001900, spi_base + SPI_CSR1);
 
  /*ʹ��SPI1(spi1 enable,Control Register[���ƼĴ���])*/
  __raw_writel(1, spi_base + SPI_CR);
 
  iounmap(spi_base);

	/*********************SPI1_CS0����****��ʼ********************************************************/
	/*ע������*/
	ret=spi_register_driver(&spi_spi1_cs0_driver);
	if(ret)
	{
		spi_unregister_driver(&spi_spi1_cs0_driver);
		return -1;
	}
 
  /*�Զ�����һ���豸��*/
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

	/*********************SPI1_CS0����****����********************************************************/  
  
	/*********************SPI1_CS1����****��ʼ********************************************************/
	/*ע������*/
	ret=spi_register_driver(&spi_spi1_cs1_driver);
	if(ret)
	{
		spi_unregister_driver(&spi_spi1_cs1_driver);
		return -1;
	}
 
  /*�Զ�����һ���豸��*/
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

	/*********************SPI1_CS1����****����********************************************************/
  
	/*********************SPI0 CS1����****��ʼ********************************************************/	
	/*ע������*/
	ret=spi_register_driver(&spi_spi0_cs1_driver);
	if(ret)
	{
		spi_unregister_driver(&spi_spi0_cs1_driver);
		return -1;
	}
 
  /*�Զ�����һ���豸��*/
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
	/*********************SPI0 CS1����*****����********************************************************/	

	printk(KERN_ALERT"SPI Second char driver init Done!\n");
 	
	return 0;
}

/**************************************************
��������:secondSpi_exit
��������:�˳�SPI�ַ�����(��������,�뵥Ƭ��ͨ��(stc12))
���ú���:
�����ú���:
�������:��
�������:
����ֵ:void
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

