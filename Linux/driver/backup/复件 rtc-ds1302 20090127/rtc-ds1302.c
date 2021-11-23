/*
 * An driver for the Dallas ds1302 RTC
 * Copyright 2009 Huawei Wodian Technologies
 *
 * Author: leiyong
 *
 * based on the other drivers in this same directory.
 *
 * http://www.hw-wd.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>
#include <linux/bcd.h>
#include <linux/rtc.h>
#include <asm/arch/gpio.h>

#define DRV_VERSION "0.9"

static unsigned short normal_i2c[] = {0x59, I2C_CLIENT_END };

/* Module parameters */
I2C_CLIENT_INSMOD;

#define DS1302_RST  AT91_PIN_PA27   /*DS1302 RST*/
#define DS1302_SCLK AT91_PIN_PA28   /*DS1302 SCLK*/
#define DS1302_DATA AT91_PIN_PA26   /*DS1302 I/O*/

/*寄存器*/
#define DS1302_REG_SECOND		0x80    /*秒*/
#define DS1302_REG_MINUTE		0x82    /*分*/
#define DS1302_REG_HOUR  		0x84    /*时*/
#define DS1302_REG_DAY		  0x86    /*日*/
#define DS1302_REG_MONTH		0x88    /*月*/
#define DS1302_REG_DAY_WEEK 0x8a    /*星期*/
#define DS1302_REG_YEAR		  0x8C    /*年*/
#define DS1302_REG_CONTROL	0x8E    /*控制*/


static int ds1302_probe(struct i2c_adapter *adapter, int address, int kind);
static int ds1302_detach(struct i2c_client *client);

/**************************************************
函数名称:ds1302WriteByte
功能描述:写DS1302寄存器
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
***************************************************/
void ds1302WriteByte(unsigned char addr,unsigned char data)
{
   unsigned char i,tmpData;
   
	 at91_set_gpio_output(DS1302_DATA, 0); /*数据线置为输出*/
   at91_set_gpio_value(DS1302_RST,0);    /*RST引脚为低,数据传送中止*/
   at91_set_gpio_value(DS1302_SCLK,0);   /*清零时钟总线*/
   at91_set_gpio_value(DS1302_RST,1);    /*CE引脚为高,逻辑控制有效*/
   
   /*发送地址*/
   tmpData = addr;
   for (i=8;i>0;i--) /*循环8次移位*/
   {
     /*每次传输低字节*/
     if (tmpData&0x01)
     {
       at91_set_gpio_value(DS1302_DATA,1);
     }
     else
     {
       at91_set_gpio_value(DS1302_DATA,0);
     }
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     
     tmpData >>= 1;     /*右移一位*/
     
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
   }
       
   //发送数据
   tmpData = data;
   for ( i=8; i>0; i-- ) 
   {
     if (tmpData&0x1)
     {
       at91_set_gpio_value(DS1302_DATA,1);
     }
     else
     {
       at91_set_gpio_value(DS1302_DATA,0);
     }

     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);

     tmpData >>= 1;
      
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);  
   }

   at91_set_gpio_value(DS1302_SCLK,0);
	 at91_set_gpio_output(DS1302_DATA, 0); /*数据线置为输出*/   
   at91_set_gpio_value(DS1302_RST,0);    /*RST引脚为低,数据传送中止*/
}

/**************************************************
函数名称:ds1302ReadByte
功能描述:读取DS1302寄存器
调用函数:
被调用函数:
输入参数:
输出参数:
返回值：
***************************************************/
unsigned char ds1302ReadByte(unsigned char addr)
{
   unsigned char i,tmpData,data1,data2;

	 at91_set_gpio_output(DS1302_DATA, 0); /*数据线置为输出*/	    
   at91_set_gpio_value(DS1302_RST,0);    /*RST引脚为低,数据传送中止*/
   at91_set_gpio_value(DS1302_SCLK,0);   /*清零时钟总线*/
   at91_set_gpio_value(DS1302_RST,1);    /*CE引脚为高,逻辑控制有效*/
   
   /*发送地址*/
   tmpData = addr;
   for ( i=8; i>0; i-- )                 /*循环8次移位*/
   {
      /*每次传输低字节*/
      if (tmpData&0x01)
      {
        at91_set_gpio_value(DS1302_DATA,1);
      }
      else
      {
        at91_set_gpio_value(DS1302_DATA,0);
      }

      at91_set_gpio_value(DS1302_SCLK,1);
      at91_set_gpio_value(DS1302_SCLK,1);
      at91_set_gpio_value(DS1302_SCLK,1);
      at91_set_gpio_value(DS1302_SCLK,1);
      at91_set_gpio_value(DS1302_SCLK,1);
      at91_set_gpio_value(DS1302_SCLK,1);
      at91_set_gpio_value(DS1302_SCLK,1);
      at91_set_gpio_value(DS1302_SCLK,1);
      at91_set_gpio_value(DS1302_SCLK,1);
      at91_set_gpio_value(DS1302_SCLK,1);
      at91_set_gpio_value(DS1302_SCLK,1);
      at91_set_gpio_value(DS1302_SCLK,1);
      
      tmpData >>= 1;                     /*右移一位*/

      at91_set_gpio_value(DS1302_SCLK,0);
      at91_set_gpio_value(DS1302_SCLK,0);
      at91_set_gpio_value(DS1302_SCLK,0);
      at91_set_gpio_value(DS1302_SCLK,0);
      at91_set_gpio_value(DS1302_SCLK,0);
      at91_set_gpio_value(DS1302_SCLK,0);
      at91_set_gpio_value(DS1302_SCLK,0);
      at91_set_gpio_value(DS1302_SCLK,0);
      at91_set_gpio_value(DS1302_SCLK,0);
      at91_set_gpio_value(DS1302_SCLK,0);
      at91_set_gpio_value(DS1302_SCLK,0);
      at91_set_gpio_value(DS1302_SCLK,0);      
   }
   
   /*读取数据*/
   at91_set_gpio_input(DS1302_DATA, 0);  /*数据线置为输入*/   
   data1=0;
   for (i=8;i>0;i--) 
   {
     data1>>=1;

     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);
     at91_set_gpio_value(DS1302_SCLK,1);

     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);
     at91_set_gpio_value(DS1302_SCLK,0);

     if (at91_get_gpio_value(DS1302_DATA))
     {
      	data1 |= 0x80;
     }
     else
     {
      	data1 |= 0x00;
     }     
   }
   
   at91_set_gpio_value(DS1302_SCLK,0);
	 at91_set_gpio_output(DS1302_DATA, 0); /*数据线置为输出*/
   at91_set_gpio_value(DS1302_RST,0);    /*RST引脚为低,数据传送中止*/

   //printk("%02x\n",data1);
   
   //data2 = data1/16;                     /*数据进制转换*/
   //data1 = data1%16;                     /*十六进制转十进制*/
   //data1 = data1+data2*10;
   
   return (data1);
}

/*
 * In the routines that deal directly with the pcf8563 hardware, we use
 * rtc_time -- month 0-11, hour 0-23, yr = calendar year-epoch.
 */
static int ds1302_get_datetime(struct i2c_client *client, struct rtc_time *tm)
{
	//unsigned char buf[13] = { PCF8563_REG_ST1 };

	//struct i2c_msg msgs[] =
	//{
	//	{ client->addr, 0, 1, buf },	/* setup read ptr */
	//	{ client->addr, I2C_M_RD, 13, buf },	/* read status + date */
	//};

	/* read registers */
	//if ((i2c_transfer(client->adapter, msgs, 2)) != 2)
	//{
	//	dev_err(&client->dev, "%s: read error\n", __FUNCTION__);
	//	return -EIO;
	//}

	//if (buf[PCF8563_REG_SC] & PCF8563_SC_LV)
	//{
	//	dev_info(&client->dev,
	//		"low voltage detected, date/time is not reliable.\n");
  //}
  
	//dev_dbg(&client->dev,
	//	"%s: raw data is st1=%02x, st2=%02x, sec=%02x, min=%02x, hr=%02x, "
	//	"mday=%02x, wday=%02x, mon=%02x, year=%02x\n",
	//	__FUNCTION__,
	//	buf[0], buf[1], buf[2], buf[3],
	//	buf[4], buf[5], buf[6], buf[7],
	//	buf[8]);

	//tm->tm_sec = BCD2BIN(buf[PCF8563_REG_SC] & 0x7F);
	//tm->tm_min = BCD2BIN(buf[PCF8563_REG_MN] & 0x7F);
	//tm->tm_hour = BCD2BIN(buf[PCF8563_REG_HR] & 0x3F); /* rtc hr 0-23 */
	//tm->tm_mday = BCD2BIN(buf[PCF8563_REG_DM] & 0x3F);
	//tm->tm_wday = buf[PCF8563_REG_DW] & 0x07;
	//tm->tm_mon = BCD2BIN(buf[PCF8563_REG_MO] & 0x1F) - 1; /* rtc mn 1-12 */
	//tm->tm_year = BCD2BIN(buf[PCF8563_REG_YR])
	//	+ (buf[PCF8563_REG_MO] & PCF8563_MO_C ? 0 : 100);
  
  tm->tm_sec  = ds1302ReadByte(DS1302_REG_SECOND | 0x1);
  tm->tm_min  = ds1302ReadByte(DS1302_REG_MINUTE | 0x1);
  tm->tm_hour = ds1302ReadByte(DS1302_REG_HOUR | 0x1);
  tm->tm_mday = ds1302ReadByte(DS1302_REG_DAY | 0x1);
  tm->tm_mon  = ds1302ReadByte(DS1302_REG_MONTH | 0x1);
  tm->tm_wday = ds1302ReadByte(DS1302_REG_DAY_WEEK | 0x1);
  tm->tm_year = ds1302ReadByte(DS1302_REG_YEAR | 0x1);
  
  tm->tm_sec  = BCD2BIN(tm->tm_sec&0x7f);
  tm->tm_min  = BCD2BIN(tm->tm_min&0x7f);
  tm->tm_hour = BCD2BIN(tm->tm_hour&0x3f);
  tm->tm_mday = BCD2BIN(tm->tm_mday&0x3f);
  tm->tm_mon  = BCD2BIN(tm->tm_mon&0x1f);
  tm->tm_wday = tm->tm_wday&0x7;
  tm->tm_year = 100+BCD2BIN(tm->tm_year);

	dev_dbg(&client->dev, "%s: tm is secs=%d, mins=%d, hours=%d, "
		"mday=%d, mon=%d, year=%d, wday=%d\n",
		__FUNCTION__,
		tm->tm_sec, tm->tm_min, tm->tm_hour,
		tm->tm_mday, tm->tm_mon, tm->tm_year, tm->tm_wday);

	/* the clock can give out invalid datetime, but we cannot return
	 * -EINVAL otherwise hwclock will refuse to set the time on bootup.
	 */
	if (rtc_valid_tm(tm) < 0)
	{
		dev_err(&client->dev, "retrieved date/time is not valid.\n");
  }

	return 0;
}

static int ds1302_set_datetime(struct i2c_client *client, struct rtc_time *tm)
{
	//int i, err;
	//unsigned char buf[9];
	//unsigned char data[2];
	dev_dbg(&client->dev, "%s: secs=%d, mins=%d, hours=%d, "
		"mday=%d, mon=%d, year=%d, wday=%d\n",
		__FUNCTION__,
		tm->tm_sec, tm->tm_min, tm->tm_hour,
		tm->tm_mday, tm->tm_mon, tm->tm_year, tm->tm_wday);

	ds1302WriteByte(DS1302_REG_CONTROL,0x00);
	ds1302WriteByte(DS1302_REG_SECOND,BIN2BCD(tm->tm_sec));
	ds1302WriteByte(DS1302_REG_MINUTE,BIN2BCD(tm->tm_min));
	ds1302WriteByte(DS1302_REG_HOUR,BIN2BCD(tm->tm_hour));
	ds1302WriteByte(DS1302_REG_DAY,BIN2BCD(tm->tm_mday));
	ds1302WriteByte(DS1302_REG_MONTH,BIN2BCD(tm->tm_mon));
	ds1302WriteByte(DS1302_REG_DAY_WEEK,tm->tm_wday&0x7);
	ds1302WriteByte(DS1302_REG_YEAR,BIN2BCD(tm->tm_year % 100));
	ds1302WriteByte(DS1302_REG_CONTROL,0x80);
	
	/* hours, minutes and seconds */
	//buf[PCF8563_REG_SC] = BIN2BCD(tm->tm_sec);
	//buf[PCF8563_REG_MN] = BIN2BCD(tm->tm_min);
	//buf[PCF8563_REG_HR] = BIN2BCD(tm->tm_hour);

	//buf[PCF8563_REG_DM] = BIN2BCD(tm->tm_mday);

	/* month, 1 - 12 */
	//buf[PCF8563_REG_MO] = BIN2BCD(tm->tm_mon + 1);

	/* year and century */
	//buf[PCF8563_REG_YR] = BIN2BCD(tm->tm_year % 100);
	//if (tm->tm_year < 100)
	//	buf[PCF8563_REG_MO] |= PCF8563_MO_C;

	//buf[PCF8563_REG_DW] = tm->tm_wday & 0x07;

	/* write register's data */
	/*
	for (i = 0; i < 7; i++)
	{
		data[0] =  PCF8563_REG_SC + i;
		data[1] =  buf[PCF8563_REG_SC + i] ;

		err = i2c_master_send(client, data, sizeof(data));
		
		if (err != sizeof(data))
		{
			dev_err(&client->dev,
				"%s: err=%d addr=%02x, data=%02x\n",
				__FUNCTION__, err, data[0], data[1]);
			return -EIO;
		}
	};

	data[0] = 0;
	data[1] = 0;
	err = i2c_master_send(client, data, sizeof(data));
  if (err != sizeof(data))
  {
     dev_err(&client->dev,
              "%s: err=%d addr=%02x, data=%02x\n",
               __FUNCTION__, err, data[0], data[1]);
     return -EIO;
  } 

	data[0] = 0x0D;
  data[1] = 0;
  err = i2c_master_send(client, data, sizeof(data));
  if (err != sizeof(data))
  {
     dev_err(&client->dev,
              "%s: err=%d addr=%02x, data=%02x\n",
               __FUNCTION__, err, data[0], data[1]);
     return -EIO;
  }   
  
  data[0] = 0x0E;
  data[1] = 0;
  err = i2c_master_send(client, data, sizeof(data));
  if (err != sizeof(data))
  {
     dev_err(&client->dev,
              "%s: err=%d addr=%02x, data=%02x\n",
               __FUNCTION__, err, data[0], data[1]);
     return -EIO;
  }
  */
     
	return 0;
}

struct pcf8563_limit
{
	unsigned char reg;
	unsigned char mask;
	unsigned char min;
	unsigned char max;
};

static int ds1302_validate_client(struct i2c_client *client)
{
	//int i;

	/* register, mask, min, max */
	/*
	static const struct pcf8563_limit pattern[] = 
	{
		{ PCF8563_REG_SC,	0x7F,	0,	59	},
		{ PCF8563_REG_MN,	0x7F,	0,	59	},
		{ PCF8563_REG_HR,	0x3F,	0,	23	},
		{ PCF8563_REG_DM,	0x3F,	0,	31	},
		{ PCF8563_REG_MO,	0x1F,	0,	12	},
	};
	*/

	/* check limits (only registers with bcd values) */
	/*
	for (i = 0; i < ARRAY_SIZE(pattern); i++)
	{
		int xfer;
		unsigned char value;
		unsigned char buf = pattern[i].reg;

		struct i2c_msg msgs[] = 
		{
			{ client->addr, 0, 1, &buf },
			{ client->addr, I2C_M_RD, 1, &buf },
		};

		xfer = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));

		if (xfer != ARRAY_SIZE(msgs))
		{
			dev_err(&client->dev,
				"%s: could not read register 0x%02X\n",
				__FUNCTION__, pattern[i].reg);

			return -EIO;
		}

		value = BCD2BIN(buf & pattern[i].mask);

		if (value > pattern[i].max ||
			value < pattern[i].min)
		{
			dev_dbg(&client->dev,
				"%s: pattern=%d, reg=%x, mask=0x%02x, min=%d, "
				"max=%d, value=%d, raw=0x%02X\n",
				__FUNCTION__, i, pattern[i].reg, pattern[i].mask,
				pattern[i].min, pattern[i].max,
				value, buf);

			return -ENODEV;
		}
	}
	*/

	return 0;
}

static int ds1302_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	return ds1302_get_datetime(to_i2c_client(dev), tm);
}

static int ds1302_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	return ds1302_set_datetime(to_i2c_client(dev), tm);
}

static const struct rtc_class_ops ds1302_rtc_ops = 
{
	.read_time	= ds1302_rtc_read_time,
	.set_time	= ds1302_rtc_set_time,
};

static int ds1302_attach(struct i2c_adapter *adapter)
{
	return i2c_probe(adapter, &addr_data, ds1302_probe);
}

static struct i2c_driver ds1302_driver = 
{
	.driver		= {
		.name	= "ds1302",
	},
	.id		= I2C_DRIVERID_DS1302,
	.attach_adapter = &ds1302_attach,
	.detach_client	= &ds1302_detach,
};

static int ds1302_probe(struct i2c_adapter *adapter, int address, int kind)
{
	struct i2c_client *client;
	struct rtc_device *rtc;

	int err = 0;

	dev_dbg(adapter->class_dev.dev, "%s\n", __FUNCTION__);
	
	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
	{
		err = -ENODEV;
		printk("exit here1\n");
		goto exit;
	}

	if (!(client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL)))
	{
		err = -ENOMEM;
		goto exit;
	}

	client->addr = address;
	client->driver = &ds1302_driver;
	client->adapter	= adapter;

	strlcpy(client->name, ds1302_driver.driver.name, I2C_NAME_SIZE);

	/* Verify the chip is really an PCF8563 */
	if (kind < 0)
	{
		if (ds1302_validate_client(client) < 0)
		{
			err = -ENODEV;
		  printk("exit here3\n");
			goto exit_kfree;
		}
	}

	/* Inform the i2c layer */
	if ((err = i2c_attach_client(client)))
		goto exit_kfree;

	dev_info(&client->dev, "chip found, driver version " DRV_VERSION "\n");

	rtc = rtc_device_register(ds1302_driver.driver.name, &client->dev,
				&ds1302_rtc_ops, THIS_MODULE);

	if (IS_ERR(rtc))
	{
		err = PTR_ERR(rtc);
		printk("exit here4\n");
		goto exit_detach;
	}

	i2c_set_clientdata(client, rtc);

	return 0;

exit_detach:
	i2c_detach_client(client);

exit_kfree:
	kfree(client);

exit:
	return err;
}

static int ds1302_detach(struct i2c_client *client)
{
	int err;
	struct rtc_device *rtc = i2c_get_clientdata(client);

	if (rtc)
		rtc_device_unregister(rtc);

	if ((err = i2c_detach_client(client)))
		return err;

	kfree(client);

	return 0;
}

static int __init ds1302_init(void)
{
	/*DS1302 的片选线*/
	if (at91_set_gpio_output(DS1302_RST, 0)!=0)
	{
	   printk(KERN_ALERT "RTC-DS1302:set DS1302_RST Output failed!\n");
	}

	/*DS1302 的时钟线*/
	if (at91_set_gpio_output(DS1302_SCLK, 0)!=0)
	{
	   printk(KERN_ALERT "RTC-DS1302:set DS1302_SCLK Output failed!\n");
	}

	/*DS1302 的数据线*/
	if (at91_set_gpio_output(DS1302_DATA, 0)!=0)
	{
	   printk(KERN_ALERT "RTC-DS1302:set DS1302_DATA Output failed!\n");
	}

	//ds1302WriteByte(DS1302_REG_CONTROL,0x0);  /*禁止写保护*/
	//ds1302WriteByte(DS1302_REG_SECOND,0x0);   /*打开晶振*/
	
	printk(KERN_ALERT "RTC-DS1302 driver init!\n");
  
	return i2c_add_driver(&ds1302_driver);
}

static void __exit ds1302_exit(void)
{
	i2c_del_driver(&ds1302_driver);
}

MODULE_AUTHOR("Leiyong");
MODULE_DESCRIPTION("Dallas DS1302 RTC driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

module_init(ds1302_init);
module_exit(ds1302_exit);
