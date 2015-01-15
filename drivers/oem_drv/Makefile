#
# Makefile for the urbetter drivers
#

obj-y     += touchscreen/
obj-y     += mtk_wcn_combo/
obj-y 		+= nfc/
obj-y     += oem_gpio_cfg/
obj-y     += power_gpio/
obj-$(CONFIG_BT_RDA_587X_MODULE_SUPPORT) += ut_bt_dev.o

obj-$(CONFIG_URBETTER_DOCK_KEYBOARD)            += dock_keyboard.o
obj-$(CONFIG_URBETTER_SIM_DETECT)            += sim_detect.o

#obj-$(CONFIG_URBETTER_POWER_GPIO)            += power_gpio.o

##obj-$(CONFIG_URBETTER_BATTERY)		       += battery.o
obj-$(CONFIG_URBETTER_BATTERY)		       += ut_battery.o

obj-$(CONFIG_URBETTER_GSENEOR_MMA7660)   += mma7660.o


obj-$(CONFIG_URBETTER_REGWATCH)               += regwatch.o

obj-$(CONFIG_URBETTER_MID_BUTTON)             += button_ut7gm.o
obj-$(CONFIG_URBETTER_DISPLAY_CONTROLLER)          += display_controller.o
#alpum1-objs += alpum.o art_interface.o alpum1_enc_kernel.o
#obj-y += alpum1.o
obj-$(CONFIG_URBETTER_MOTOR_SHAKE) += motor.o
obj-$(CONFIG_UT_LIGHTSENSOR_BH1721) +=bh1721.o
obj-$(CONFIG_QN8027_FM) +=qn8027.o
obj-$(CONFIG_OEM_BTN) +=oem_btn.o
obj-y   +=e2130s_3g.o
obj-y   +=bq24190_charger.o


