# Makefile of modem_if

EXTRA_CFLAGS += -Idrivers/misc/modem_if

obj-y += sipc5_modem.o sipc5_io_device.o
obj-y += sipc4_modem.o sipc4_io_device.o
obj-y += modem_net_flowcontrol_device.o modem_utils.o modem_debug.o

obj-$(CONFIG_UMTS_MODEM_XMM6260) += modem_modemctl_device_xmm6260.o

obj-$(CONFIG_LINK_DEVICE_USB) += modem_link_device_usb.o modem_link_pm_usb.o
