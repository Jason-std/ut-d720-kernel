/******************************************************************************
 * ut_cam_common.h - 
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
#ifndef __UT_CAM_COMMON_H__
#define  __UT_CAM_COMMON_H__

typedef struct sensor_reg {
u16 u16RegAddr;
u8 u8Val;
u8 u8Mask;
u32 u32Delay_us;
} st_sensor_reg;

typedef struct front_sensor_reg {
u8 addr;
u8 val;
}st_front_sensor_reg;

#define SENSORS_TABLE_END 0xFFFC
#define ARRY_SIZE(A) (sizeof(A)/sizeof(A[0]))

/*static */int reg_read_16(struct i2c_client *client, u16 reg, u8 *val);
/*static */int reg_read_8(struct i2c_client *client, u16 reg, u8 *val);
/*static */int reg_write_16(struct i2c_client *client, u16 reg, u8 val);
/*static */int reg_write_8(struct i2c_client *client, u16 reg, u16 val16);
/*static */int i2c_write_array_8(struct i2c_client *client, st_sensor_reg  *vals);
/*static */int sensor_i2c_write_array_16(struct i2c_client *client, const st_sensor_reg  *vals);
///*static */int sensor_i2c_write_array_16_sleep(struct i2c_client *client, st_sensor_reg  *vals);
/*static */int i2c_check_array_16(struct i2c_client *client, st_sensor_reg  *vals);
/*static */int sensor_i2c_write(struct v4l2_subdev *sd, unsigned char i2c_data[], unsigned char length);

#endif /*__UT_CAM_COMMON_H__ */
