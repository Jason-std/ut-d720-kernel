#ifndef __LINUX_CHIPONE_I2C_TS_H__
#define __LINUX_CHIPONE_I2C_TS_H__

// gpio base address
#define PIO_BASE_ADDRESS             (0x01c20800)
#define PIO_RANGE_SIZE               (0x400)
#define CONFIG_FT5X0X_MULTITOUCH     (1)

#define IRQ_EINT21                   (21) 
#define IRQ_EINT29                   (29) 

#define PIO_INT_STAT_OFFSET          (0x214)
#define PIO_INT_CTRL_OFFSET          (0x210)
#define PIO_INT_CFG2_OFFSET          (0x208)
#define PIO_INT_CFG3_OFFSET          (0x20c)

                                 
#undef  AW_GPIO_INT_API_ENABLE
//#define AW_FPGA_SIM
#ifdef AW_FPGA_SIM
#endif

//#define NT1100X_NAME		"novatek-ts"		//"synaptics_i2c_rmi"//"synaptics-rmi-ts"// 
#define NT1100X_NAME		"novatek_ts"		//"synaptics_i2c_rmi"//"synaptics-rmi-ts"// 
#define AW_GPIO_API_ENABLE
//#undef CONFIG_HAS_EARLYSUSPEND
#ifndef CONFIG_HAS_EARLYSUSPEND
//#define CONFIG_HAS_EARLYSUSPEND
#endif
struct ft5x_ts_platform_data{
	u16	intr;		/* irq number	*/
};

#if 0  //masked andydeng 7.13
enum ft5x_ts_regs {
	FT5X0X_REG_PMODE	= 0xA5,	/* Power Consume Mode		*/	
};
#endif 

#define PRESS_MAX       255

//FT5X0X_REG_PMODE
#define PMODE_ACTIVE        0x00
#define PMODE_MONITOR       0x01
#define PMODE_STANDBY       0x02
#define PMODE_HIBERNATE     0x03


#ifndef ABS_MT_TOUCH_MAJOR
    #define ABS_MT_TOUCH_MAJOR	0x30	/* touching ellipse */
    #define ABS_MT_TOUCH_MINOR	0x31	/* (omit if circular) */
    #define ABS_MT_WIDTH_MAJOR	0x32	/* approaching ellipse */
    #define ABS_MT_WIDTH_MINOR	0x33	/* (omit if circular) */
    #define ABS_MT_ORIENTATION	0x34	/* Ellipse orientation */
    #define ABS_MT_POSITION_X	0x35	/* Center X ellipse position */
    #define ABS_MT_POSITION_Y	0x36	/* Center Y ellipse position */
    #define ABS_MT_TOOL_TYPE	0x37	/* Type of touching device */
    #define ABS_MT_BLOB_ID		0x38	/* Group set of pkts as blob */
#endif /* ABS_MT_TOUCH_MAJOR */


//Rocky@20111207+

#define TOUCHSCREEN_MINX 0
#define TOUCHSCREEN_MAXX 600
#define TOUCHSCREEN_MINY 0
#define TOUCHSCREEN_MAXY 1024

/*! @brief Values for positions in the touch key map array */
#define CHIPONE_FINGERID_TOUCHING_NUM      0
#define CHIPONE_TOUCHING_X1_H              1
#define CHIPONE_TOUCHING_X1_L              2
#define CHIPONE_TOUCHING_Y1_H              3
#define CHIPONE_TOUCHING_Y1_L              4
#define CHIPONE_TOUCHING_X2_H              5
#define CHIPONE_TOUCHING_X2_L              6
#define CHIPONE_TOUCHING_Y2_H              7
#define CHIPONE_TOUCHING_Y2_L              8

#define CHIPONE_TOUCH_KEY_NUMBER           11


#define CHIPONE_READ_DATA_SIZE             12

#define CHIPONE_KEY_RELEASE                     0
#define CHIPONE_KEY_PRESS                       1

#define CHIPONE_FINGERID_FIRST                  0x10


#define IOMUX_NAME_SIZE 48

#ifndef FALSE
#define	FALSE	0
#define	TRUE	(!FALSE)


/* touch keys */
#define TOUCH_KEY_NUMBER_MENU    1
#define TOUCH_KEY_NUMBER_HOME    2
#define TOUCH_KEY_NUMBER_BACK    4
#define TOUCH_KEY_NUMBER_SEARCH  8

#define TOUCH_KEYCODE_MENU      139
#define TOUCH_KEYCODE_HOME      102
#define TOUCH_KEYCODE_BACK      158
#define TOUCH_KEYCODE_SEARCH    217

//Rocky@20111207-


#define READ_COOR_ADDR					0x00
#define DRIVER_SEND_CFG
#define NT11002   0
#define NT11003   1
#define IC_DEFINE   NT11003
#if IC_DEFINE == NT11002
#define IIC_BYTENUM    4
#elif IC_DEFINE == NT11003
#define IIC_BYTENUM    6
#endif


//define default resolution of the touchscreen
//#define TOUCH_MAX_HEIGHT 	960
//#define TOUCH_MAX_WIDTH		1600
//#define TOUCH_MAX_HEIGHT 	896
//#define TOUCH_MAX_WIDTH		1536

enum finger_state {
#define FLAG_MASK 0x01
	FLAG_UP = 0,
	FLAG_DOWN = 1,
	FLAG_INVALID = 2,
};


struct point_node
{
	uint8_t id;
	//uint8_t retry;
	enum finger_state state;
	uint8_t pressure;
	unsigned int x;
	unsigned int y;
};

//Set Bootloader function
#define _NOVATEK_CAPACITANCEPANEL_BOOTLOADER_FUNCTION_
#ifdef _NOVATEK_CAPACITANCEPANEL_BOOTLOADER_FUNCTION_
enum
{
  RS_OK         = 0,
  RS_INIT_ER    = 8,
  RS_ERAS_ER    = 9,
  RS_FLCS_ER    = 10,
  RS_WD_ER      = 11,
} ;
#endif

#define    SW_RST   1
#define    HW_RST   2

#define NOVATEK_MULTI_TOUCH
#ifdef NOVATEK_MULTI_TOUCH
	#define MAX_FINGER_NUM	5
#else
	#define MAX_FINGER_NUM	1	
#endif


#endif
#endif

