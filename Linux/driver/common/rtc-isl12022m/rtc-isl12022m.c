/*
 * An I2C driver for the Intersil ISL12022M RTC
 * Copyright 2010 LY Technologies
 *
 * Author: LY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/i2c.h>
#include <linux/bcd.h>
#include <linux/rtc.h>

#define DRV_VERSION "0.9"

/* Addresses to scan: none
 * This chip cannot be reliably autodetected. An empty eeprom
 * located at 0x6f will pass the validation routine due to
 * the way the registers are implemented.
 */
static unsigned short normal_i2c[] = {0x6f, I2C_CLIENT_END };

/* Module parameters */
I2C_CLIENT_INSMOD;

/*isl12022m寄存器*/
/*日历寄存器*/
#define ISL12022M_REG_SC		   0x00  /*秒*/
#define ISL12022M_REG_MN		   0x01  /*分*/
#define ISL12022M_REG_HR		   0x02  /*时*/
#define ISL12022M_REG_HR_MIL   0x80  /* 24h/12h mode */
#define ISL12022M_REG_HR_PM    0x20  /* PM/AM bit in 12h mode */
#define ISL12022M_REG_DM		   0x03  /*日*/
#define ISL12022M_REG_MO		   0x04  /*月*/
#define ISL12022M_REG_YR		   0x05  /*年*/
#define ISL12022M_REG_DW		   0x06  /*星期几*/

/*控制及状态寄存器*/
#define ISL12022M_REG_STATUS   0x07  /*状态寄存器*/
#define ISL12022M_REG_INT      0x08  /*中断寄存器*/

/*状态寄存器位标识*/
#define ISL12022M_STATUS_RTCF  0x01  /*状态寄存器-RTC失败*/
#define ISL12022M_STATUS_BAT75 0x02  /*状态寄存器-低电压指示75%*/
#define ISL12022M_STATUS_BAT85 0x04  /*状态寄存器-低电压指示85%*/
#define ISL12022M_STATUS_LVDD  0x08  /*状态寄存器-VDD低指示*/
#define ISL12022M_STATUS_ALM   0x20  /*状态寄存器-ALARM bit*/
#define ISL12022M_STATUS_OSCF  0x40  /*状态寄存器-晶体停振*/
#define ISL12022M_STATUS_BUSY  0x80  /*状态寄存器-忙指示*/

/*中断控制寄存器标识(Interrupt Control Register)*/
#define ISL12022M_INT_ARST     0x80  /*AUTOMATIC RESET BIT(ARST)*/
#define ISL12022M_INT_WTRC     0x40  /*WRITE RTC ENABLE BIT(WRTC)*/
#define ISL12022M_INT_IM       0x20  /*INTERRUPT/ALARM MODE BIT (IM)*/
#define ISL12022M_INT_FOBATB   0x10  /*FREQUENCY OUTPUT AND INTERRUPT BIT*/


static int isl12022m_probe(struct i2c_adapter *adapter, int address, int kind);
static int isl12022m_detach(struct i2c_client *client);

/*
 * In the routines that deal directly with the isl12022m hardware, we use
 * rtc_time -- month 0-11, hour 0-23, yr = calendar year-epoch.
 */
static int isl12022m_get_datetime(struct i2c_client *client, struct rtc_time *tm)
{
	unsigned rtcStatus = 0;
	unsigned char buf[7] = { ISL12022M_REG_SC };
	unsigned char _hr;
  
	struct i2c_msg msgs[] = 
	{
		{ client->addr, 0, 1, buf },	      /*设置读取寄存器起始地址(setup read ptr)*/
		{ client->addr, I2C_M_RD, 7, buf },	/*读取日期和状态(read status + date)*/
	};

	/*读寄存器(read registers)*/
	if ((i2c_transfer(client->adapter, msgs, 2)) != 2)
	{
		dev_err(&client->dev, "%s: read error\n", __FUNCTION__);
		
		return -EIO;
	}
  
  /*状态寄存器指示所有电压均有失压,需重新写时间寄存器才能走时*/
	if (buf[ISL12022M_REG_STATUS] & ISL12022M_STATUS_RTCF)
	{
		rtcStatus |= ISL12022M_STATUS_RTCF;
		dev_info(&client->dev,"Total power failure(Vdd and Vbat), date/time is stopped.write date register to set DATE_TIME\n");
  }
  /*低电压75%指示*/
	if (buf[ISL12022M_REG_STATUS] & ISL12022M_STATUS_BAT75)
	{
		rtcStatus |= ISL12022M_STATUS_BAT75;
		dev_info(&client->dev,"Low Battery 75 percent indicator.\n");
  }
  /*低电压85%指示*/
	if (buf[ISL12022M_REG_STATUS] & ISL12022M_STATUS_BAT85)
	{
		rtcStatus |= ISL12022M_STATUS_BAT85;
		dev_info(&client->dev,"Low Battery 85 percent indicator.\n");
  }
  /*低电压VDD指示*/
	if (buf[ISL12022M_REG_STATUS] & ISL12022M_STATUS_LVDD)
	{
		rtcStatus |= ISL12022M_STATUS_LVDD;
		dev_info(&client->dev,"Low VDD indicator.\n");
  }
  /*忙指示*/
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
	
	/* HR field has a more complex interpretation */
	_hr = buf[ISL12022M_REG_HR];
	if (_hr & ISL12022M_REG_HR_MIL) /* 24h format */
	{
			tm->tm_hour = BCD2BIN(_hr & 0x3f);
	}
	else                          /* 12h format*/
	{
		tm->tm_hour = BCD2BIN(_hr & 0x1f);
		if (_hr & ISL12022M_REG_HR_PM) /* PM flag set */
		{
			tm->tm_hour += 12;
		}
	}

	tm->tm_mday = BCD2BIN(buf[ISL12022M_REG_DM] & 0x3F);
	tm->tm_wday = buf[ISL12022M_REG_DW] & 0x07;
	tm->tm_mon = BCD2BIN(buf[ISL12022M_REG_MO] & 0x1F) - 1; /* rtc mn 1-12 */
	tm->tm_year = BCD2BIN(buf[ISL12022M_REG_YR])+100;

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

	return 0;
}

static int isl12022m_set_datetime(struct i2c_client *client, struct rtc_time *tm)
{
	int i, err;
	unsigned char buf[9];
	unsigned char data[2];

	dev_dbg(&client->dev, "%s: secs=%d, mins=%d, hours=%d, "
		"mday=%d, mon=%d, year=%d, wday=%d\n",
		__FUNCTION__,
		tm->tm_sec, tm->tm_min, tm->tm_hour,
		tm->tm_mday, tm->tm_mon, tm->tm_year, tm->tm_wday);

	buf[ISL12022M_REG_SC] = BIN2BCD(tm->tm_sec);                           /*秒*/
	buf[ISL12022M_REG_MN] = BIN2BCD(tm->tm_min);                           /*分*/
	buf[ISL12022M_REG_HR] = BIN2BCD(tm->tm_hour) | ISL12022M_REG_HR_MIL;   /*小时*/
	buf[ISL12022M_REG_DM] = BIN2BCD(tm->tm_mday);	                         /*日*/
	buf[ISL12022M_REG_MO] = BIN2BCD(tm->tm_mon+1);                         /*月,1-12*/	
	buf[ISL12022M_REG_YR] = BIN2BCD(tm->tm_year % 100);                    /*年*/
	buf[ISL12022M_REG_DW] = tm->tm_wday & 0x07;                            /*星期几*/

	
	/*写使能*/
	data[0] = ISL12022M_REG_INT;
	data[1] = 0xda;	
	err = i2c_master_send(client, data, 2);
	
	/* write register's data */
	for (i = 0; i < 7; i++)
	{
		data[0] = ISL12022M_REG_SC + i;
		data[1] = buf[ISL12022M_REG_SC + i];

		err = i2c_master_send(client, data, 2);
		
		if (err != sizeof(data))
		{
			dev_err(&client->dev,
				"%s: err=%d addr=%02x, data=%02x\n",
				__FUNCTION__, err, data[0], data[1]);
			return -EIO;
		}
	};

	/*写禁止*/
	data[0] = ISL12022M_REG_INT;
	data[1] = 0x9a;
	err = i2c_master_send(client, data, 2);
  
  printk(KERN_ALERT "ISL12022M set datetime success.\n");

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
		//{ ISL12022M_REG_HR,	0x3F,	0,	23	},
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

static int isl12022m_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	isl12022m_get_datetime(to_i2c_client(dev), tm);
	return isl12022m_get_datetime(to_i2c_client(dev), tm);
}

static int isl12022m_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	return isl12022m_set_datetime(to_i2c_client(dev), tm);
}

static const struct rtc_class_ops isl12022m_rtc_ops = {
	.read_time	= isl12022m_rtc_read_time,
	.set_time	= isl12022m_rtc_set_time,
};

static int isl12022m_attach(struct i2c_adapter *adapter)
{
	return i2c_probe(adapter, &addr_data, isl12022m_probe);
}

static struct i2c_driver isl12022m_driver = {
	.driver		= {
		.name	= "isl12022m",
	},
	.id		= I2C_DRIVERID_ISL12022M,
	.attach_adapter = &isl12022m_attach,
	.detach_client	= &isl12022m_detach,
};

static int isl12022m_probe(struct i2c_adapter *adapter, int address, int kind)
{
	struct i2c_client *client;
	struct rtc_device *rtc;

	int err = 0;
	
	dev_dbg(adapter->class_dev.dev, "%s\n", __FUNCTION__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
	{
		err = -ENODEV;

		goto exit;
	}

	if (!(client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL))) 
	{
		err = -ENOMEM;
		
		goto exit;
	}

	client->addr = address;
	client->driver = &isl12022m_driver;
	client->adapter	= adapter;

	strlcpy(client->name, isl12022m_driver.driver.name, I2C_NAME_SIZE);

	/*校验芯片是否真的为ISL12022M(Verify the chip is really an isl12022m)*/
	if (kind < 0)
	{
		if (isl12022m_validate_client(client) < 0)
		{
			err = -ENODEV;

			goto exit_kfree;
		}
	}

	/*通知I2C层(Inform the i2c layer)*/
	if ((err = i2c_attach_client(client)))
	{
  	printk("ISL12022M :In probe 4\n");
		
		goto exit_kfree;
	}

	dev_info(&client->dev, "chip found, driver version " DRV_VERSION "\n");
  
  //注册RTC
	rtc = rtc_device_register(isl12022m_driver.driver.name, &client->dev,
				&isl12022m_rtc_ops, THIS_MODULE);

	if (IS_ERR(rtc))
	{
		err = PTR_ERR(rtc);

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

static int isl12022m_detach(struct i2c_client *client)
{
	int err;
	struct rtc_device *rtc = i2c_get_clientdata(client);

	if (rtc)
	{
		rtc_device_unregister(rtc);
  }

	if ((err = i2c_detach_client(client)))
	{
		return err;
  }

	kfree(client);

	return 0;
}

static int __init isl12022m_init(void)
{
	return i2c_add_driver(&isl12022m_driver);
}

static void __exit isl12022m_exit(void)
{
	i2c_del_driver(&isl12022m_driver);
}

MODULE_AUTHOR("LY");
MODULE_DESCRIPTION("Intersil isl12022m RTC driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

module_init(isl12022m_init);
module_exit(isl12022m_exit);
