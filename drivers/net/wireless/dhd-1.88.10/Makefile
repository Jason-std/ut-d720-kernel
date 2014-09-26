# bcmdhd
DHDCFLAGS = -Wall -Wstrict-prototypes -Dlinux -DBCMDRIVER                     \
	-DDHD_FIRSTREAD=128 -DMAX_HDR_READ=128                                \
	-DBCMDONGLEHOST -DUNRELEASEDCHIP -DBCMDMA32 -DBCMFILEIMAGE  \
	-DDHDTHREAD -DDHD_GPL -DDHD_SCHED -DBDC -DTOE    \
	-DDHD_BCMEVENTS -DSHOW_EVENTS -DDONGLEOVERLAYS -DBCMDBG               \
	-DCUSTOMER_HW2 -DOOB_INTR_ONLY        \
	-DMMC_SDIO_ABORT -DBCMSDIO -DBCMLXSDMMC -DBCMPLATFORM_BUS -DWLP2P     \
	-DNEW_COMPAT_WIRELESS -DWIFI_ACT_FRAME -DARP_OFFLOAD_SUPPORT          \
	-DKEEP_ALIVE -DCSCAN  -DPKT_FILTER_SUPPORT     \
	-DEMBEDDED_PLATFORM -DENABLE_INSMOD_NO_FW_LOAD -DPNO_SUPPORT          \
	-DOEM_ANDROID -DLINUX -DWL_ENABLE_P2P_IF -DWL_CFG80211_STA_EVENT   \
	-DWL_SUPPORT_BACKPORTED_KPATCHES -DWLBTAMP \
	-Idrivers/net/wireless/dhd-1.88.10 -Idrivers/net/wireless/dhd-1.88.10/include

DHDOFILES = aiutils.o bcmsdh_sdmmc_linux.o dhd_linux.o siutils.o bcmutils.o   \
	dhd_linux_sched.o bcmwifi_channels.o dhd_sdio.o bcmevent.o dhd_bta.o hndpmu.o  \
	bcmsdh.o dhd_cdc.o bcmsdh_linux.o dhd_common.o dhd_wlfc.o dhd_ip.o linux_osl.o \
	bcmsdh_sdmmc.o dhd_custom_gpio.o sbutils.o wldev_common.o wl_android.o dhd_cfg80211.o

DHDCFLAGS += -DBCMWAPI_WPI -DBCMWAPI_WAI -DCONFIG_WIFI_CONTROL_FUNC 
##DHDCFLAGS += -DCUSTOM_OOB_GPIO_NUM=160
obj-$(CONFIG_DHD_1_88_10) += bcmdhd.o
bcmdhd-objs += $(DHDOFILES)

##  add by albert start
ifeq ($(CONFIG_WIFI_CONTROL_FUNC),y)
DHDCFLAGS += -DOEM_4_AP6XXX=1
DHDCFLAGS += -DOOB_INTR_ONLY -DHW_OOB -DCUSTOMER_OOB
DHDCFLAGS += -DCUSTOMER_HW2 
##  android 4.4
##DHDCFLAGS += -DWL_COMPAT_WIRELESS
#DHDCFLAGS += -DBAND_AG
##DHDCFLAGS += -DGET_CUSTOM_MAC_ENABLE
endif

ifneq ($(CONFIG_WIRELESS_EXT),)
bcmdhd-objs += wl_iw.o
DHDCFLAGS += -DSOFTAP
endif
ifneq ($(CONFIG_CFG80211),)
bcmdhd-objs += wl_cfg80211.o wl_cfgp2p.o wl_linux_mon.o
DHDCFLAGS += -DCUSTOM_ROAM_TRIGGER_SETTING=-85
DHDCFLAGS += -DWL_CFG80211
endif
EXTRA_CFLAGS = $(DHDCFLAGS)
ifeq ($(CONFIG_DHD_1_88_10),m)
EXTRA_LDFLAGS += --strip-debug
endif

