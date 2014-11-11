/******************************************************************************
 * ut_cam_common.c - 
 * 
 * Copyright 2010-2018 Urbetter Co.,Ltd.
 * 
 * DESCRIPTION: - 
 * 
 * modification history
 * --------------------
 * v1.0   2014/09/09, Albert_lee create this file
 * 
 ******************************************************************************/
#include <linux/delay.h>
#include <linux/i2c.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-common.h>
#include "ut_cam_common.h"

/*static */int reg_read_16(struct i2c_client *client, u16 reg, u8 *val)
{
	int ret;
	/* We have 16-bit i2c addresses - care for endianess */
	unsigned char data[2] = { reg >> 8, reg & 0xff };

	ret = i2c_master_send(client, data, 2);
	if (ret < 2) {
		dev_err(&client->dev, "%s: i2c read error, reg: %x\n",
			__func__, reg);
		return ret < 0 ? ret : -EIO;
	}

	ret = i2c_master_recv(client, val, 1);
	if (ret < 1) {
		dev_err(&client->dev, "%s: i2c read error, reg: %x\n",
				__func__, reg);
		return ret < 0 ? ret : -EIO;
	}
	return 0;
}

/*static */int reg_read_8(struct i2c_client *client, u16 reg, u8 *val)
{
	int ret;
	/* We have 16-bit i2c addresses - care for endianess */
	unsigned char data[1] = { reg & 0xff };

	ret = i2c_master_send(client, data, 1);
	if (ret < 1) {
		dev_err(&client->dev, "%s: i2c read error, reg: %x\n",
			__func__, reg);
		return ret < 0 ? ret : -EIO;
	}

	ret = i2c_master_recv(client, val, 1);
	if (ret < 1) {
		dev_err(&client->dev, "%s: i2c read error, reg: %x\n",
				__func__, reg);
		return ret < 0 ? ret : -EIO;
	}
	return 0;
}

/*static */int reg_write_16(struct i2c_client *client, u16 reg, u8 val)
{
	int ret;
	unsigned char data[3] = { reg >> 8, reg & 0xff, val };

	ret = i2c_master_send(client, data, 3);
	if (ret < 3) {
		dev_err(&client->dev, "%s: i2c write error, reg: %x\n",
			__func__, reg);
		return ret < 0 ? ret : -EIO;
	}

	return 0;
}

/*static */int reg_write_8(struct i2c_client *client, u16 reg, u16 val16)
{
	int ret;
	unsigned char data[2] = { reg & 0xff, val16 & 0xff };

	ret = i2c_master_send(client, data, 2);
	if (ret < 2) {
		dev_err(&client->dev, "%s: i2c write error, reg: %x\n",
			__func__, reg);
		return ret < 0 ? ret : -EIO;
	}
	return 0;
}

/*static */int i2c_write_array_8(struct i2c_client *client,
				st_sensor_reg  *vals)
{
	while(vals->u16RegAddr != SENSORS_TABLE_END) {

		int ret = reg_write_8(client, vals->u16RegAddr, vals->u8Val);

		if (ret < 0)
			return ret;

		#if 0 // for test write sensor reg
			//mdelay(50);
			uint8_t value = -1;;

			ret = reg_read_8(client, vals->u16RegAddr, &value);
			if(vals->u8Val != (value & 0xff))
				printk("[addr]%x, [R] reg[0x:%x] [W]:value:[0x%x]  [R]value[:0x%x]\n",client->addr, vals->u16RegAddr,vals->u8Val,value);
		#endif
		vals++;
	}
	return 0;
}

/*static */int sensor_i2c_write_array_16(struct i2c_client *client, const st_sensor_reg  *vals)
{
//	uint i = 0;

	while(vals->u16RegAddr != SENSORS_TABLE_END) {

		int ret = reg_write_16(client, vals->u16RegAddr, vals->u8Val);
		if (ret < 0) {
			printk("[addr]%x, reg[0x:%04x] value:[0x%02x]  ret=%d\n",client->addr, vals->u16RegAddr,vals->u8Val, ret);
			return ret;
		}

		#if 0 // for test write sensor reg
			//mdelay(50);
			msleep(10);
			uint8_t value = -1;;

			ret = reg_read_16(client, vals->u16RegAddr, &value);
			if(vals->u8Val != (value & 0xff))
				printk("[addr]%x, [R] reg[0x:%x] [W]:value:[0x%x]  [R]value[:0x%x]\n",client->addr, vals->u16RegAddr,vals->u8Val,value);
		#endif
		if(vals->u32Delay_us > 0)
			usleep_range(vals->u32Delay_us, (vals->u32Delay_us+100));
		vals++;
	}
	dev_dbg(&client->dev, "Register list loaded\n");

	return 0;
}

///*static */int sensor_i2c_write_array_16_sleep(struct i2c_client *client, st_sensor_reg  *vals)
//{
//	uint i = 0;
//
//	while(vals->u16RegAddr != SENSORS_TABLE_END) {
//
//		int ret = reg_write_16(client, vals->u16RegAddr, vals->u8Val);
//
//		if (ret < 0)
//			return ret;
//
//		#if 0 // for test write sensor reg
//			//mdelay(50);
//		//	msleep(10);
//			uint8_t value = -1;;
//
//			ret = reg_read_16(client, vals->u16RegAddr, &value);
//			if(vals->u8Val != (value & 0xff))
//				printk("[addr]%x, [R] reg[0x:%x] [W]:value:[0x%x]  [R]value[:0x%x]\n",client->addr, vals->u16RegAddr,vals->u8Val,value);
//		#endif
//		if(vals->u32Delay_us > 0)
//			usleep_range(vals->u32Delay_us, vals->u32Delay_us+100);
//		vals++;
//	}
//	dev_dbg(&client->dev, "Register list loaded\n");
//
//	return 0;
//}

///*static */int i2c_check_array_af(struct i2c_client *client,
//				unsigned char * pv)
//{
//	uint i = 0,ret;
//	u8 val;
//	u16 start=0x8000,end=0x9572,reg;
//	pv+=2;
//	for(reg=start,i=0;reg<end+1;reg++,i++){
//		reg_read_16(client, reg, &val);
//		if(val!=pv[i]){
//			printk("%s:reg[0x%04x],R[0x%02x],W[0x%02x]\n",__func__,reg,val,pv[i]);
//		}
//	}
//}

/*static */int i2c_check_array_16(struct i2c_client *client,
				st_sensor_reg  *vals)
{
//	uint ret;

	while(vals->u16RegAddr != SENSORS_TABLE_END) {

		//int ret = reg_write_16(client, vals->u16RegAddr, vals->u8Val);

		//if (ret < 0)
		//	return ret;

		#if 0 // for test write sensor reg
			//mdelay(50);
			uint8_t value = -1;;

			ret = reg_read_16(client, vals->u16RegAddr, &value);
			if(vals->u8Val != (value & 0xff))
				printk("[addr]%x, [R] reg[0x:%x] [W]:value:[0x%x]  [R]value[:0x%x]\n",client->addr, vals->u16RegAddr,vals->u8Val,value);
		#endif
		vals++;
	}
	dev_dbg(&client->dev, "Register list loaded\n");

	return 0;
}

/*static */int sensor_i2c_write(struct v4l2_subdev *sd, unsigned char i2c_data[], unsigned char length)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[length], i;
	struct i2c_msg msg = { client->addr, 0, length, buf };

	for (i = 0; i < length; i++)
	{
		buf[i] = i2c_data[i];
	}
	return i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
}
