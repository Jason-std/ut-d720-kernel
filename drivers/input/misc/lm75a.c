/*
 * lm75a.c - Part of lm_sensors, Linux kernel modules for hardware
 *	 monitoring
 * Copyright (c) 1998, 1999  Frodo Looijaard <frodol@dds.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include "lm75a.h"
#include <linux/input.h>
#include <asm/uaccess.h>



/*
 * This driver handles the lm75a and compatible digital temperature sensors.
 */

enum lm75a_type {		/* keep sorted in alphabetical order */
	adt75,
	ds1775,
	ds75,
	lm75,
	lm75a,
	max6625,
	max6626,
	mcp980x,
	stds75,
	tcn75,
	tmp100,
	tmp101,
	tmp105,
	tmp175,
	tmp275,
	tmp75,
};

/* Addresses scanned */
static const unsigned short normal_i2c[] = { 0x48, 0x49, 0x4a, 0x4b, 0x4c,
					0x4d, 0x4e, 0x4f, I2C_CLIENT_END };


/* The lm75a registers */
#define LM75A_REG_CONF		0x01
static const u8 LM75A_REG_TEMP[3] = {
	0x00,		/* input */
	0x03,		/* max */
	0x02,		/* hyst */
};

/* Each client has this additional data */
struct lm75a_data {
	struct mutex		update_lock;
	struct delayed_work	work;	/* for temp polling */
	struct i2c_client *lm75a_client;
	u8			orig_conf;
	char			valid;		/* !=0 if registers are valid */
	unsigned long		last_updated;	/* In jiffies */
	u16			temp[3];	/* Register values,
						   0 = input
						   1 = max
						   2 = hyst */
};

static struct input_dev* lm75a_input=NULL;
static struct class *lm75a_class  = NULL;
struct device* lm75a_dev = NULL;
static int lm75a_major = 0;
#define  TEMP_PROC_NAME "lm75a"
static int lm75a_poll_delay = 1000;	//delay 1s
static struct i2c_client *this_client;
static int temp_value;
static int lm75a_read_value(struct i2c_client *client, u8 reg);
static int lm75a_write_value(struct i2c_client *client, u8 reg, u16 value);
static struct lm75a_data *lm75a_update_device(struct device *dev);


/*-----------------------------------------------------------------------*/

/* sysfs attributes for hwmon */
#if	0
static ssize_t show_temp(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct lm75a_data *data = lm75a_update_device(dev);

	if (IS_ERR(data))
		return PTR_ERR(data);

	return sprintf(buf, "%d\n",
		       lm75a_TEMP_FROM_REG(data->temp[attr->index]));
}

static ssize_t set_temp(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct lm75a_data *data = i2c_get_clientdata(client);
	int nr = attr->index;
	long temp;
	int error;

	error = kstrtol(buf, 10, &temp);
	if (error)
		return error;

	mutex_lock(&data->update_lock);
	data->temp[nr] = lm75a_TEMP_TO_REG(temp);
	lm75a_write_value(client, LM75A_REG_TEMP[nr], data->temp[nr]);
	mutex_unlock(&data->update_lock);
	return count;
}

static SENSOR_DEVICE_ATTR(temp1_max, S_IWUSR | S_IRUGO,
			show_temp, set_temp, 1);
static SENSOR_DEVICE_ATTR(temp1_max_hyst, S_IWUSR | S_IRUGO,
			show_temp, set_temp, 2);
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_temp, NULL, 0);

static struct attribute *lm75a_attributes[] = {
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp1_max.dev_attr.attr,
	&sensor_dev_attr_temp1_max_hyst.dev_attr.attr,

	NULL
};

static const struct attribute_group lm75a_group = {
	.attrs = lm75a_attributes,
};
#endif
/*-----------------------------------------------------------------------*/

/* device probe and removal */


#if	1
static ssize_t lm75a_enable_show(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct lm75a_data *data = lm75a_update_device(dev);

	//if (IS_ERR(data))
	//	return PTR_ERR(data);

	return sprintf(buf, "%d\n", 0);
}

static ssize_t lm75a_enable_store(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct lm75a_data *data = i2c_get_clientdata(client);
	long temp;
	int error,status,enable;

	error = kstrtol(buf, 10, &temp);
	if (error)
		return error;

	status = lm75a_read_value(client, LM75A_REG_CONF);
	if (status < 0) {
		dev_dbg(&client->dev, "Can't read config? %d\n", status);
		return status;
	}


	enable = (status&0x01)?0:1;

	if(enable != temp)
	{

		if(temp)	 //enable
		{
			status = status & ~LM75A_SHUTDOWN;
			lm75a_write_value(client, LM75A_REG_CONF, status);
			schedule_delayed_work(&data->work, msecs_to_jiffies(lm75a_poll_delay));
		}
		else
		{
			status = status | LM75A_SHUTDOWN;
			lm75a_write_value(client, LM75A_REG_CONF, status);
			cancel_delayed_work_sync(&data->work);
		}
	}

	return count;
}

static ssize_t lm75a_temperature_show(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct lm75a_data *data = lm75a_update_device(dev);

	if (IS_ERR(data))
		return PTR_ERR(data);

	return sprintf(buf, "%d\n", 0);
}

static ssize_t lm75a_set_delay(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct lm75a_data *data = i2c_get_clientdata(client);
	long temp;
	int error;

	error = kstrtol(buf, 10, &temp);
	if (error)
		return error;

	lm75a_poll_delay = temp;	// unit :ms

	return count;
}

static DEVICE_ATTR(enable, 0662,
		NULL, lm75a_enable_store);
static DEVICE_ATTR(temperature, 0664,
		lm75a_temperature_show, NULL);
static DEVICE_ATTR(delay, 0662,
		NULL, lm75a_set_delay);

static struct attribute *lm75a_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_temperature.attr,
	&dev_attr_delay.attr,
	NULL
};

static const struct attribute_group lm75a_group = {
	.attrs = lm75a_attributes,
};
#endif

static void lm75a_work_func(struct work_struct *work)
{
	struct lm75a_data *data = container_of((struct delayed_work *)work,
			struct lm75a_data, work);
	unsigned long delay = msecs_to_jiffies(lm75a_poll_delay);
	int status,temp;
	//printk("===lm75a_work_func===\n");
	status = lm75a_read_value(data->lm75a_client, LM75A_REG_TEMP[0]);	//read temperature reg
	if(status < 0)
	{
		printk("can not read temperature reg. err code is %d",status);
		schedule_delayed_work(&data->work, delay);
		return;
	}
//	printk("==status=%x,status=%d\n",status,status);
	temp = status >> 7;
        temp_value=temp;
//	printk("==temp=%x,temp=%d\n",temp,temp);
	input_report_abs(lm75a_input, ABS_TEMP,temp);
	input_sync(lm75a_input);

	schedule_delayed_work(&data->work, delay);

}
static int test_i2c_txdata(char *txdata, int length)
{
	int ret;

	struct i2c_msg msg[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= length,
			.buf	= txdata,
		},
	};

   	//msleep(1);
	ret = i2c_transfer(this_client->adapter, msg, 1);
	if (ret < 0)
		pr_err("%s i2c write error: %d\n", __func__, ret);

	return ret;
}

#if 1
static int lm75a_open(struct inode *inode, struct file *file)
{
	printk("---%s---\n",__func__);
	return 0;
}

ssize_t lm75a_read(struct file *file, char __user * buf, size_t count, loff_t * ppos)
{
//printk("%s,lux_value[0]:%d lux_value[1]:%d\n",__func__,lux_value[0],lux_value[1]);
	if(count > 2)
		count = 2 ;
	if(copy_to_user(buf, temp_value, count))
		return -EFAULT;
	return sizeof(temp_value);

}

struct file_operations lm75a_fops = {
	.owner = THIS_MODULE,
	.open = lm75a_open,
	.read = lm75a_read,
	};
#endif

static int
lm75a_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct lm75a_data *data;
	int status,retry,err ;
	u8 set_mask, clr_mask;
	int new,ret;
	printk("%s ===start!!!\n",__func__);
	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA))
		return -EIO;

	data = kzalloc(sizeof(struct lm75a_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;


	i2c_set_clientdata(client, data);

	data->lm75a_client = client;
	this_client=client;

#if 1 //FIXME: disable it for testing
	for(retry=0; retry < 4; retry++) {
		err =test_i2c_txdata( NULL, 0);
		if (err > 0)
			break;
	}

	if(err <= 0) {
		printk(">>>>>>>>>>>> %s---> i2c test err!!!\n",__func__);
		dev_err(&client->dev, "Warnning: I2C connection might be something wrong!\n");
		goto err_i2c_failed;
	}
#endif

	mutex_init(&data->update_lock);

	/* Set to lm75a resolution (9 bits, 1/2 degree C) and range.
	 * Then tweak to be more precise when appropriate.
	 */
	set_mask = 0;
	clr_mask = (1 << 0)			/* continuous conversions */
		| (1 << 6) | (1 << 5);		/* 9-bit mode */

	/* configure as specified */

	status = lm75a_read_value(client, LM75A_REG_CONF);
	if (status < 0) {
		dev_dbg(&client->dev, "Can't read config? %d\n", status);
		goto exit_free;
	}


	data->orig_conf = status;
	new = status & ~clr_mask;
	new |= set_mask;
	if (status != new)
		lm75a_write_value(client, LM75A_REG_CONF, new);
	dev_dbg(&client->dev, "Config %02x\n", new);

	/* Register sysfs hooks */
	status = sysfs_create_group(&client->dev.kobj, &lm75a_group);
	if (status)
		goto exit_free;

	lm75a_input = input_allocate_device();
	if(!lm75a_input)
	 	printk("lm75a_input alloc Error");

	set_bit(EV_ABS, lm75a_input->evbit);
	input_set_abs_params(lm75a_input, ABS_TEMP, 0, 30000, 0, 0);

	lm75a_input->name = "TI temperature sensor";

 	 if(input_register_device(lm75a_input) != 0){
			printk("lm75a input register device fail!!\n");
			input_free_device(lm75a_input);
	}

#if 1
		lm75a_major = register_chrdev(0, TEMP_PROC_NAME, &lm75a_fops);
		if(lm75a_major < 0){
			ret = lm75a_major;
			return -1;
		}

		lm75a_class = class_create(THIS_MODULE, TEMP_PROC_NAME);
		if (IS_ERR(lm75a_class)) {
			ret = -EIO;
			return -1;
		}
		/* Register char device */
		lm75a_dev = device_create(lm75a_class, NULL, MKDEV(lm75a_major, 0), NULL, TEMP_PROC_NAME);
		if (IS_ERR(lm75a_dev)) {
			ret = -EIO;
			return -1;
		}
#endif

	INIT_DELAYED_WORK(&data->work, lm75a_work_func);
	schedule_delayed_work(&data->work,msecs_to_jiffies(lm75a_poll_delay));
	printk("%s ===end!!!\n",__func__);
	return 0;

//exit_remove:
//	sysfs_remove_group(&client->dev.kobj, &lm75a_group);
err_i2c_failed:
exit_free:
	kfree(data);
	printk(">>>eking %s(%d)\n",__func__,__LINE__);
	return status;
}

static int lm75a_remove(struct i2c_client *client)
{
	struct lm75a_data *data = i2c_get_clientdata(client);

	//sysfs_remove_group(&client->dev.kobj, &lm75a_group);
	lm75a_write_value(client, LM75A_REG_CONF, data->orig_conf);
	kfree(data);
	return 0;
}

static const struct i2c_device_id lm75a_ids[] = {
	{ "lm75a", 0, },
};
MODULE_DEVICE_TABLE(i2c, lm75a_ids);

#define LM75A_ID 0xA1

/* Return 0 if detection is successful, -ENODEV otherwise */
static int lm75a_detect(struct i2c_client *new_client,
		       struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = new_client->adapter;
	int i;
	int conf, hyst, os;
	bool is_lm75a = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA |
				     I2C_FUNC_SMBUS_WORD_DATA))
		return -ENODEV;

	/*
	 * Now, we do the remaining detection. There is no identification-
	 * dedicated register so we have to rely on several tricks:
	 * unused bits, registers cycling over 8-address boundaries,
	 * addresses 0x04-0x07 returning the last read value.
	 * The cycling+unused addresses combination is not tested,
	 * since it would significantly slow the detection down and would
	 * hardly add any value.
	 *
	 * The National Semiconductor LM75A is different than earlier
	 * LM75s.  It has an ID byte of 0xaX (where X is the chip
	 * revision, with 1 being the only revision in existence) in
	 * register 7, and unused registers return 0xff rather than the
	 * last read value.
	 *
	 * Note that this function only detects the original National
	 * Semiconductor LM75 and the LM75A. Clones from other vendors
	 * aren't detected, on purpose, because they are typically never
	 * found on PC hardware. They are found on embedded designs where
	 * they can be instantiated explicitly so detection is not needed.
	 * The absence of identification registers on all these clones
	 * would make their exhaustive detection very difficult and weak,
	 * and odds are that the driver would bind to unsupported devices.
	 */

	/* Unused bits */
	conf = i2c_smbus_read_byte_data(new_client, 1);
	if (conf & 0xe0)
		return -ENODEV;

	/* First check for LM75A */
	if (i2c_smbus_read_byte_data(new_client, 7) == LM75A_ID) {
		/* LM75A returns 0xff on unused registers so
		   just to be sure we check for that too. */
		if (i2c_smbus_read_byte_data(new_client, 4) != 0xff
		 || i2c_smbus_read_byte_data(new_client, 5) != 0xff
		 || i2c_smbus_read_byte_data(new_client, 6) != 0xff)
			return -ENODEV;
		is_lm75a = 1;
		hyst = i2c_smbus_read_byte_data(new_client, 2);
		os = i2c_smbus_read_byte_data(new_client, 3);
	} else { /* Traditional style LM75 detection */
		/* Unused addresses */
		hyst = i2c_smbus_read_byte_data(new_client, 2);
		if (i2c_smbus_read_byte_data(new_client, 4) != hyst
		 || i2c_smbus_read_byte_data(new_client, 5) != hyst
		 || i2c_smbus_read_byte_data(new_client, 6) != hyst
		 || i2c_smbus_read_byte_data(new_client, 7) != hyst)
			return -ENODEV;
		os = i2c_smbus_read_byte_data(new_client, 3);
		if (i2c_smbus_read_byte_data(new_client, 4) != os
		 || i2c_smbus_read_byte_data(new_client, 5) != os
		 || i2c_smbus_read_byte_data(new_client, 6) != os
		 || i2c_smbus_read_byte_data(new_client, 7) != os)
			return -ENODEV;
	}

	/* Addresses cycling */
	for (i = 8; i <= 248; i += 40) {
		if (i2c_smbus_read_byte_data(new_client, i + 1) != conf
		 || i2c_smbus_read_byte_data(new_client, i + 2) != hyst
		 || i2c_smbus_read_byte_data(new_client, i + 3) != os)
			return -ENODEV;
		if (is_lm75a && i2c_smbus_read_byte_data(new_client, i + 7)
				!= LM75A_ID)
			return -ENODEV;
	}

	strlcpy(info->type, is_lm75a ? "lm75a" : "lm75", I2C_NAME_SIZE);

	return 0;
}

#ifdef CONFIG_PM
static int lm75a_suspend(struct device *dev)
{
	int status;
	struct i2c_client *client = to_i2c_client(dev);
	status = lm75a_read_value(client, LM75A_REG_CONF);
	if (status < 0) {
		dev_dbg(&client->dev, "Can't read config? %d\n", status);
		return status;
	}
	status = status | LM75A_SHUTDOWN;
	lm75a_write_value(client, LM75A_REG_CONF, status);
	return 0;
}

static int lm75a_resume(struct device *dev)
{
	int status;
	struct i2c_client *client = to_i2c_client(dev);
	status = lm75a_read_value(client, LM75A_REG_CONF);
	if (status < 0) {
		dev_dbg(&client->dev, "Can't read config? %d\n", status);
		return status;
	}
	status = status & ~LM75A_SHUTDOWN;
	lm75a_write_value(client, LM75A_REG_CONF, status);
	return 0;
}

static const struct dev_pm_ops lm75a_dev_pm_ops = {
	.suspend	= lm75a_suspend,
	.resume		= lm75a_resume,
};
#define lm75a_DEV_PM_OPS (&lm75a_dev_pm_ops)
#else
#define lm75a_DEV_PM_OPS NULL
#endif /* CONFIG_PM */

static struct i2c_driver lm75a_driver = {
	.driver = {
		.name	= "lm75a",
		.owner = THIS_MODULE,
		.pm	= lm75a_DEV_PM_OPS,
	},
	.probe		= lm75a_probe,
	.remove		= lm75a_remove,
	.id_table	= lm75a_ids,
	.detect		= lm75a_detect,
};

/*-----------------------------------------------------------------------*/

/* register access */

/*
 * All registers are word-sized, except for the configuration register.
 * lm75a uses a high-byte first convention, which is exactly opposite to
 * the SMBus standard.
 */

static inline s32
i2c_smbus_write_word_swapped(const struct i2c_client *client,
			     u8 command, u16 value)
{
	return i2c_smbus_write_word_data(client, command, swab16(value));
}

static inline s32
i2c_smbus_read_word_swapped(const struct i2c_client *client, u8 command)
{
	s32 value = i2c_smbus_read_word_data(client, command);

	return (value < 0) ? value : swab16(value);
}

static int lm75a_read_value(struct i2c_client *client, u8 reg)
{
	if (reg == LM75A_REG_CONF)
		return i2c_smbus_read_byte_data(client, reg);
	else
		return i2c_smbus_read_word_swapped(client, reg);
}

static int lm75a_write_value(struct i2c_client *client, u8 reg, u16 value)
{
	if (reg == LM75A_REG_CONF)
		return i2c_smbus_write_byte_data(client, reg, value);
	else
		return i2c_smbus_write_word_swapped(client, reg, value);
}

static struct lm75a_data *lm75a_update_device(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct lm75a_data *data = i2c_get_clientdata(client);
	struct lm75a_data *ret = data;

	mutex_lock(&data->update_lock);

	if (time_after(jiffies, data->last_updated + HZ + HZ / 2)
	    || !data->valid) {
		int i;
		dev_dbg(&client->dev, "Starting lm75a update\n");

		for (i = 0; i < ARRAY_SIZE(data->temp); i++) {
			int status;

			status = lm75a_read_value(client, LM75A_REG_TEMP[i]);
			if (unlikely(status < 0)) {
				dev_dbg(dev,
					"lm75a: Failed to read value: reg %d, error %d\n",
					LM75A_REG_TEMP[i], status);
				ret = ERR_PTR(status);
				data->valid = 0;
				goto abort;
			}
			data->temp[i] = status;
		}
		data->last_updated = jiffies;
		data->valid = 1;
	}

abort:
	mutex_unlock(&data->update_lock);
	return ret;
}

static int __init lm75a_init(void)
{

	int ret;
	ret = i2c_add_driver(&lm75a_driver);
	printk("ret = %d.\n",ret);
	return ret;

}

static void __exit lm75a_exit(void)
{
	printk(KERN_INFO"=exit=\n");
	i2c_del_driver(&lm75a_driver);
}


//module_i2c_driver(lm75a_driver);
late_initcall(lm75a_init);
module_exit(lm75a_exit);


MODULE_AUTHOR("Frodo Looijaard <frodol@dds.nl>");
MODULE_DESCRIPTION("lm75a driver");
MODULE_LICENSE("GPL");

