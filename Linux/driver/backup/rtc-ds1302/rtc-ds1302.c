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

static unsigned short normal_i2c[] = {0x6f, I2C_CLIENT_END };

/* Module parameters */
I2C_CLIENT_INSMOD;

#define DS1302_RST  AT91_PIN_PA27   /*DS1302 RST*/
#define DS1302_SCLK AT91_PIN_PA28   /*DS1302 SCLK*/
#define DS1302_DATA AT91_PIN_PA26   /*DS1302 I/O*/

/*?Ĵ???*/
#define DS1302_REG_SECOND		0x80    /*??*/
#define DS1302_REG_MINUTE		0x82    /*??*/
#define DS1302_REG_HOUR  		0x84    /*ʱ*/
#define DS1302_REG_DAY		  0x86    /*??*/
#define DS1302_REG_MONTH		0x88    /*??*/
#define DS1302_REG_DAY_WEEK 0x8a    /*????*/
#define DS1302_REG_YEAR		  0x8C    /*??*/
#define DS1302_REG_CONTROL	0x8E    /*????*/

/*isl12022m?Ĵ???*/
/*?????Ĵ???*/
#define ISL12022M_REG_SC		   0x00  /*??*/
#define ISL12022M_REG_MN		   0x01  /*??*/
#define ISL12022M_REG_HR		   0x02  /*ʱ*/
#define ISL12022M_REG_DM		   0x03  /*??*/
#define ISL12022M_REG_MO		   0x04  /*??*/
#define ISL12022M_REG_YR		   0x05  /*??*/
#define ISL12022M_REG_DW		   0x06  /*???ڼ?*/

/*???Ƽ?״̬?Ĵ???*/
#define ISL12022M_REG_STATUS   0x07  /*״̬?Ĵ???*/
#define ISL12022M_REG_INT      0x08  /*?жϼĴ???*/

/*״̬?Ĵ???λ??ʶ*/
#define ISL12022M_STATUS_RTCF  0x01  /*״̬?Ĵ???-RTCʧ??*/
#define ISL12022M_STATUS_BAT75 0x02  /*״̬?Ĵ???-?͵?ѹָʾ75%*/
#define ISL12022M_STATUS_BAT85 0x04  /*״̬?Ĵ???-?͵?ѹָʾ85%*/
#define ISL12022M_STATUS_LVDD  0x08  /*״̬?Ĵ???-VDD??ָʾ*/
#define ISL12022M_STATUS_ALM   0x20  /*״̬?Ĵ???-ALARM bit*/
#define ISL12022M_STATUS_OSCF  0x40  /*״̬?Ĵ???-????ͣ??*/
#define ISL12022M_STATUS_BUSY  0x80  /*״̬?Ĵ???-æָʾ*/

static int isl12022m_probe(struct i2c_adapter *adapter, int address, int kind);
static int isl12022m_detach(struct i2c_client *client);

static int ds1302_probe(struct i2c_adapter *adapter, int address, int kind);
static int ds1302_detach(struct i2c_client *client);

/*
 * In the routines that deal directly with the isl12022m hardware, we use
 * rtc_time -- month 0-11, hour 0-23, yr = calendar year-epoch.
 */
static int isl12022m_get_datetime(struct i2c_client *client, struct rtc_time *tm)
{
	unsigned rtcStatus = 0;
	unsigned char buf[7] = { ISL12022M_REG_SC };
  
	struct i2c_msg msgs[] = 
	{
		{ client->addr, 0, 1, buf },	      /*???ö?ȡ?Ĵ?????ʼ??ַ(setup read ptr)*/
		{ client->addr, I2C_M_RD, 7, buf },	/*??ȡ???ں?״̬(read status + date)*/
	};

	/*???Ĵ???(read registers)*/
	if ((i2c_transfer(client->adapter, msgs, 2)) != 2)
	{
		dev_err(&client->dev, "%s: read error\n", __FUNCTION__);
		
		//return -EIO;
	}
  
  /*״̬?Ĵ???ָʾ???е?ѹ????ʧѹ,??????дʱ???Ĵ?????????ʱ*/
	if (buf[ISL12022M_REG_STATUS] & ISL12022M_STATUS_RTCF)
	{
		rtcStatus |= ISL12022M_STATUS_RTCF;
		dev_info(&client->dev,"Total power failure(Vdd and Vbat), date/time is stopped.write date register to set DATE_TIME\n");
  }
  /*?͵?ѹ75%ָʾ*/
	if (buf[ISL12022M_REG_STATUS] & ISL12022M_STATUS_BAT75)
	{
		rtcStatus |= ISL12022M_STATUS_BAT75;
		dev_info(&client->dev,"Low Battery 75 percent indicator.\n");
  }
  /*?͵?ѹ85%ָʾ*/
	if (buf[ISL12022M_REG_STATUS] & ISL12022M_STATUS_BAT85)
	{
		rtcStatus |= ISL12022M_STATUS_BAT85;
		dev_info(&client->dev,"Low Battery 85 percent indicator.\n");
  }
  /*?͵?ѹVDDָʾ*/
	if (buf[ISL12022M_REG_STATUS] & ISL12022M_STATUS_LVDD)
	{
		rtcStatus |= ISL12022M_STATUS_LVDD;
		dev_info(&client->dev,"Low VDD indicator.\n");
  }
  /*æָʾ*/
	if (buf[ISL12022M_REG_STATUS] & ISL12022M_STATUS_BUSY)
	{
		rtcStatus |= ISL12022M_STATUS_BUSY;
		dev_info(&client->dev,"ISL12022M busy indicator.\n");
  }

	dev_dbg(&client->dev,
		"%s: raw data is sec=%02x, min=%02x, hr=%02x, "
		"mday=%02x, wday=%02x, mon=%02x, year=%02x, st=%02x\n",
		__FUNCTION__,
		buf[0], buf[1], buf[2], buf[3],
		buf[4], buf[5], buf[6], buf[7]);


	tm->tm_sec = BCD2BIN(buf[ISL12022M_REG_SC] & 0x7F);
	tm->tm_min = BCD2BIN(buf[ISL12022M_REG_MN] & 0x7F);
	tm->tm_hour = BCD2BIN(buf[ISL12022M_REG_HR] & 0x3F);    /* rtc hr 0-23 */
	tm->tm_mday = BCD2BIN(buf[ISL12022M_REG_DM] & 0x3F);
	tm->tm_wday = buf[ISL12022M_REG_DW] & 0x07;
	tm->tm_mon = BCD2BIN(buf[ISL12022M_REG_MO] & 0x1F) - 1; /* rtc mn 1-12 */
	tm->tm_year = BCD2BIN(buf[ISL12022M_REG_YR]);           /*+2000?д???֤???*/

	dev_dbg(&client->dev, "%s: tm is secs=%d, mins=%d, hours=%d, "
		"mday=%d, mon=%d, year=%d, wday=%d\n",
		__FUNCTION__,
		tm->tm_sec, tm->tm_min, tm->tm_hour,
		tm->tm_mday, tm->tm_mon, tm->tm_year, tm->tm_wday);

	/* the clock can give out invalid datetime, but we cannot return
	 * -EINVAL otherwise hwclock will refuse to set the time on bootup.
	 */
	if (rtc_valid_tm(tm) < 0)
		dev_err(&client->dev, "retrieved date/time is not valid.\n");

	return rtcStatus;
}

static int isl12022m_set_datetime(struct i2c_client *client, struct rtc_time *tm)
{
	int i, err;
	unsigned char buf[9];

	dev_dbg(&client->dev, "%s: secs=%d, mins=%d, hours=%d, "
		"mday=%d, mon=%d, year=%d, wday=%d\n",
		__FUNCTION__,
		tm->tm_sec, tm->tm_min, tm->tm_hour,
		tm->tm_mday, tm->tm_mon, tm->tm_year, tm->tm_wday);

	buf[ISL12022M_REG_SC] = BIN2BCD(tm->tm_sec);        /*??*/
	buf[ISL12022M_REG_MN] = BIN2BCD(tm->tm_min);        /*??*/
	buf[ISL12022M_REG_HR] = BIN2BCD(tm->tm_hour);       /*Сʱ*/
	buf[ISL12022M_REG_DM] = BIN2BCD(tm->tm_mday);	      /*??*/
	buf[ISL12022M_REG_MO] = BIN2BCD(tm->tm_mon);        /*??,1-12*/	
	buf[ISL12022M_REG_YR] = BIN2BCD(tm->tm_year % 100); /*??*/
	buf[ISL12022M_REG_DW] = tm->tm_wday & 0x07;         /*???ڼ?*/

	/* write register's data */
	for (i = 0; i < 7; i++)
	{
		unsigned char data[2] = { ISL12022M_REG_SC + i, buf[ISL12022M_REG_SC + i] };

		err = i2c_master_send(client, data, sizeof(data));
		
		if (err != sizeof(data))
		{
			dev_err(&client->dev,
				"%s: err=%d addr=%02x, data=%02x\n",
				__FUNCTION__, err, data[0], data[1]);
			return -EIO;
		}
	};

	return 0;
}

struct isl12022m_limit
{
	unsigned char reg;
	unsigned char mask;
	unsigned char min;
	unsigned char max;
};

static int isl12022m_validate_client(struct i2c_client *client)
{
	int i;

	static const struct isl12022m_limit pattern[] = 
	{
		/* register, mask, min, max */
		{ ISL12022M_REG_SC,	0x7F,	0,	59	},
		{ ISL12022M_REG_MN,	0x7F,	0,	59	},
		{ ISL12022M_REG_HR,	0x3F,	0,	23	},
		{ ISL12022M_REG_DM,	0x3F,	0,	31	},
		{ ISL12022M_REG_MO,	0x1F,	0,	12	},
	};

	/* check limits (only registers with bcd values) */
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

		if (value > pattern[i].max || value < pattern[i].min) 
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

	return 0;
}


/**************************************************
????????:ds1302WriteByte
????????:дDS1302?Ĵ???
???ú???:
?????ú???:
????????:
????????:
????ֵ??
***************************************************/
void ds1302WriteByte(unsigned char addr,unsigned char data)
{
   unsigned char i,tmpData;
   
	 at91_set_gpio_output(DS1302_DATA, 0); /*????????Ϊ????*/
   at91_set_gpio_value(DS1302_RST,0);    /*RST????Ϊ??,???ݴ?????ֹ*/
   at91_set_gpio_value(DS1302_SCLK,0);   /*????ʱ??????*/
   at91_set_gpio_value(DS1302_RST,1);    /*CE????Ϊ??,?߼???????Ч*/
   
   /*???͵?ַ*/
   tmpData = addr;
   for (i=8;i>0;i--) /*ѭ??8????λ*/
   {
     /*ÿ?δ??????ֽ?*/
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
     
     tmpData >>= 1;     /*????һλ*/
     
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
       
   //????????
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
	 at91_set_gpio_output(DS1302_DATA, 0); /*????????Ϊ????*/   
   at91_set_gpio_value(DS1302_RST,0);    /*RST????Ϊ??,???ݴ?????ֹ*/
}

/**************************************************
????????:ds1302ReadByte
????????:??ȡDS1302?Ĵ???
???ú???:
?????ú???:
????????:
????????:
????ֵ??
***************************************************/
unsigned char ds1302ReadByte(unsigned char addr)
{
   unsigned char i,tmpData,data1,data2;

	 at91_set_gpio_output(DS1302_DATA, 0); /*????????Ϊ????*/	    
   at91_set_gpio_value(DS1302_RST,0);    /*RST????Ϊ??,???ݴ?????ֹ*/
   at91_set_gpio_value(DS1302_SCLK,0);   /*????ʱ??????*/
   at91_set_gpio_value(DS1302_RST,1);    /*CE????Ϊ??,?߼???????Ч*/
   
   /*???͵?ַ*/
   tmpData = addr;
   for ( i=8; i>0; i-- )                 /*ѭ??8????λ*/
   {
      /*ÿ?δ??????ֽ?*/
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
      
      tmpData >>= 1;                     /*????һλ*/

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
   
   /*??ȡ????*/
   at91_set_gpio_input(DS1302_DATA, 0);  /*????????Ϊ????*/   
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
	 at91_set_gpio_output(DS1302_DATA, 0); /*????????Ϊ????*/
   at91_set_gpio_value(DS1302_RST,0);    /*RST????Ϊ??,???ݴ?????ֹ*/

   //printk("%02x\n",data1);
   
   //data2 = data1/16;                     /*???ݽ???ת??*/
   //data1 = data1%16;                     /*ʮ??????תʮ????*/
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
	//return ds1302_get_datetime(to_i2c_client(dev), tm);
	return isl12022m_get_datetime(to_i2c_client(dev), tm);
}

static int ds1302_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	//return ds1302_set_datetime(to_i2c_client(dev), tm);
	return isl12022m_set_datetime(to_i2c_client(dev), tm);
}

static const struct rtc_class_ops ds1302_rtc_ops = 
{
	.read_time	= ds1302_rtc_read_time,
	.set_time	= ds1302_rtc_set_time,
};

static int ds1302_attach(struct i2c_adapter *adapter)
{
	//return i2c_probe(adapter, &addr_data, ds1302_probe);
	return i2c_probe(adapter, &addr_data, isl12022m_probe);
}

static struct i2c_driver ds1302_driver = 
{
	.driver		= {
		.name	= "ds1302",
	},
	.id		= I2C_DRIVERID_ISL12022M,
	.attach_adapter = &ds1302_attach,
	.detach_client	= &ds1302_detach,
};

static int isl12022m_probe(struct i2c_adapter *adapter, int address, int kind)
{
	struct i2c_client *client;
	struct rtc_device *rtc;

	int err = 0;
  
  printk("ISL12022M probe: exit here1\n");
  printk("address:%02x\n",address);

	dev_dbg(adapter->class_dev.dev, "%s\n", __FUNCTION__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
	{
		err = -ENODEV;
  printk("ISL12022M probe: exit here2\n");

		goto exit;
	}

	if (!(client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL))) {
		err = -ENOMEM;
    printk("ISL12022M probe: exit here2\n");
		goto exit;
	}

	client->addr = address;
	//client->driver = &isl12022m_driver;
	client->driver = &ds1302_driver;
	client->adapter	= adapter;

	strlcpy(client->name, ds1302_driver.driver.name, I2C_NAME_SIZE);

	/*У??оƬ?Ƿ?????ΪISL12022M(Verify the chip is really an isl12022m)*/
	if (kind < 0)
	{
		if (isl12022m_validate_client(client) < 0)
		{
			err = -ENODEV;
      printk("ISL12022M probe: exit here3\n");

		
			//goto exit_kfree;
		}
	}

	/*֪ͨI2C??(Inform the i2c layer)*/
	if ((err = i2c_attach_client(client)))
	{
      printk("ISL12022M probe: exit here4\n");

		goto exit_kfree;
	}

	dev_info(&client->dev, "chip found, driver version " DRV_VERSION "\n");
  
  //ע??RTC
	//rtc = rtc_device_register(isl12022m_driver.driver.name, &client->dev,
	//			&isl12022m_rtc_ops, THIS_MODULE);
	rtc = rtc_device_register(ds1302_driver.driver.name, &client->dev,
				&ds1302_rtc_ops, THIS_MODULE);

	if (IS_ERR(rtc))
	{
		err = PTR_ERR(rtc);

	  dev_info(&client->dev, "ISL12022M OUT 6\n");

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
	/*DS1302 ??Ƭѡ??*/
	//if (at91_set_gpio_output(DS1302_RST, 0)!=0)
	//{
	//   printk(KERN_ALERT "RTC-DS1302:set DS1302_RST Output failed!\n");
	//}

	/*DS1302 ??ʱ????*/
	//if (at91_set_gpio_output(DS1302_SCLK, 0)!=0)
	//{
	//   printk(KERN_ALERT "RTC-DS1302:set DS1302_SCLK Output failed!\n");
	//}

	/*DS1302 ????????*/
	//if (at91_set_gpio_output(DS1302_DATA, 0)!=0)
	//{
	//   printk(KERN_ALERT "RTC-DS1302:set DS1302_DATA Output failed!\n");
	//}

	//ds1302WriteByte(DS1302_REG_CONTROL,0x0);  /*??ֹд????*/
	//ds1302WriteByte(DS1302_REG_SECOND,0x0);   /*?򿪾???*/
	
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
