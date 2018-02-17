ifeq ($(WIFI_MODE),)
RT28xx_MODE = APSTA
else
RT28xx_MODE = $(WIFI_MODE)
endif

MODULE = mt7612u

# Support Wpa_Supplicant
# i.e. wpa_supplicant -Dralink
HAS_WPA_SUPPLICANT=y

# Support Native WpaSupplicant for Network Maganger
# i.e. wpa_supplicant -Dwext
HAS_NATIVE_WPA_SUPPLICANT_SUPPORT=y

HAS_KTHREAD_SUPPORT=y

#Support statistics count
HAS_STATS_COUNT=y

# Support HOSTAPD function
HAS_HOSTAPD_SUPPORT=n

#Support cfg80211 function with Linux Only.
#Please make sure insmod the cfg80211.ko before our driver,
#our driver references to its symbol.
HAS_CFG80211_SUPPORT=y
#smooth the scan signal for cfg80211 based driver
HAS_CFG80211_SCAN_SIGNAL_AVG_SUPPORT=y

#Support RFKILL hardware block/unblock LINUX-only function
#broken, need ieee80211_hw structure
HAS_RFKILL_HW_SUPPORT=n


HAS_NEW_RATE_ADAPT_SUPPORT=y

#################################################

WFLAGS := -DAGGREGATION_SUPPORT -DPIGGYBACK_SUPPORT -DWMM_SUPPORT  -DLINUX -Wall -Wstrict-prototypes -Wno-trigraphs
WFLAGS += -DSYSTEM_LOG_SUPPORT -DRT28xx_MODE=$(RT28xx_MODE) -DDBG_DIAGNOSE -DDBG_RX_MCS -DDBG_TX_MCS

ifeq ($(HAS_KTHREAD_SUPPORT),y)
WFLAGS += -DKTHREAD_SUPPORT
endif

###############################################################################
#
# config for AP mode
#
###############################################################################

ifeq ($(RT28xx_MODE),AP)
WFLAGS += -DCONFIG_AP_SUPPORT -DMBSS_SUPPORT -DDBG -DDOT1X_SUPPORT -DAP_SCAN_SUPPORT -DSCAN_SUPPORT

ifeq ($(HAS_HOSTAPD_SUPPORT),y)
WFLAGS += -DHOSTAPD_SUPPORT
endif

endif #// endif of RT2860_MODE == AP //

########################################################
#
# config for STA mode
#
########################################################

ifeq ($(RT28xx_MODE),STA)
WFLAGS += -DCONFIG_STA_SUPPORT -DSCAN_SUPPORT -DDBG

ifeq ($(HAS_WPA_SUPPLICANT),y)
WFLAGS += -DWPA_SUPPLICANT_SUPPORT
ifeq ($(HAS_NATIVE_WPA_SUPPLICANT_SUPPORT),y)
WFLAGS += -DNATIVE_WPA_SUPPLICANT_SUPPORT
endif
endif

endif
# endif of ifeq ($(RT28xx_MODE),STA)

###########################################################
#
# config for APSTA
#
###########################################################

ifeq ($(RT28xx_MODE),APSTA)
WFLAGS += -DCONFIG_AP_SUPPORT -DCONFIG_STA_SUPPORT -DCONFIG_APSTA_MIXED_SUPPORT -DMBSS_SUPPORT -DDOT1X_SUPPORT -DAP_SCAN_SUPPORT -DSCAN_SUPPORT -DDBG

ifeq ($(HAS_WPA_SUPPLICANT),y)
WFLAGS += -DWPA_SUPPLICANT_SUPPORT
ifeq ($(HAS_NATIVE_WPA_SUPPLICANT_SUPPORT),y)
WFLAGS += -DNATIVE_WPA_SUPPLICANT_SUPPORT
endif
endif

endif
# endif of ifeq ($(RT28xx_MODE),APSTA)

# Common options 

ifeq ($(HAS_NEW_RATE_ADAPT_SUPPORT),y)
WFLAGS += -DNEW_RATE_ADAPT_SUPPORT
endif

ifeq ($(HAS_STATS_COUNT),y)
WFLAGS += -DSTATS_COUNT_SUPPORT
endif

ifeq ($(HAS_CFG80211_SUPPORT),y)
WFLAGS += -DRT_CFG80211_SUPPORT -DWPA_SUPPLICANT_SUPPORT
ifeq ($(HAS_RFKILL_HW_SUPPORT),y)
WFLAGS += -DRFKILL_HW_SUPPORT
endif
ifeq ($(HAS_CFG80211_SCAN_SIGNAL_AVG_SUPPORT),y)
WFLAGS += -DCFG80211_SCAN_SIGNAL_AVG
endif
endif

##########################################################
#
# Common compiler flag
#
##########################################################

#################################################
# ChipSet specific definitions.
#
#################################################

WFLAGS += -DMT76x2 -DMT_RF -DRTMP_TIMER_TASK_SUPPORT -DRTMP_EFUSE_SUPPORT -DRTMP_RF_RW_SUPPORT
WFLAGS += -DFIFO_EXT_SUPPORT

#################################################
# Platform Related definitions
#
#################################################

EXTRA_CFLAGS := $(WFLAGS)

#RT28xx_DIR = home directory of RT28xx source code
RT28xx_DIR = $(shell pwd)

#APSOC

#RELEASE Package
RELEASE = DPOA

obj_ap :=
obj_sta :=
obj_wsc :=
obj_vht :=
obj_cmm := \
	common/crypt_md5.o\
	common/crypt_sha2.o\
	common/crypt_hmac.o\
	common/crypt_aes.o\
	common/crypt_arc4.o\
	common/mlme.o\
	common/cmm_wep.o\
	common/action.o\
	common/cmm_data.o\
	common/rtmp_init.o\
	common/rtmp_init_inf.o\
	common/cmm_tkip.o\
	common/cmm_aes.o\
	common/cmm_sync.o\
	common/cmm_sanity.o\
	common/cmm_info.o\
	common/cmm_cfg.o\
	common/cmm_wpa.o\
	common/cmm_radar.o\
	common/spectrum.o\
	common/rtmp_timer.o\
	common/rt_channel.o\
	common/cmm_profile.o\
	common/cmm_asic.o\
	common/scan.o\
	common/cmm_cmd.o\
	common/ps.o\
	common/sys_log.o\
	common/txpower.o\
	rate_ctrl/ra_ctrl.o\
	rate_ctrl/alg_legacy.o\
	chips/rtmp_chip.o\
	mgmt/mgmt_entrytb.o\
	tx_rx/wdev_tx.o \
	os/linux/rt_profile.o


obj_phy := phy/phy.o

obj_mac := mac/rtmp_mac.o

obj_phy += phy/rlt_phy.o

obj_mac += mac/ral_nmac.o

obj_cmm += $(obj_phy) $(obj_mac)

ifeq ($(HAS_NEW_RATE_ADAPT_SUPPORT),y)
obj_cmm += rate_ctrl/alg_grp.o
endif

#ifdef DOT11_N_SUPPORT
obj_cmm += \
        common/ba_action.o\
        mgmt/mgmt_ht.o

obj_cmm += 	common/cmm_txbf.o

obj_vht += mgmt/mgmt_vht.o\
	common/vht.o








###############################################################################
#
# config for AP mode
#
###############################################################################

obj_ap += \
	ap/ap_mbss.o\
	ap/ap.o\
	ap/ap_assoc.o\
	ap/ap_auth.o\
	ap/ap_connect.o\
	ap/ap_mlme.o\
	ap/ap_sanity.o\
	ap/ap_sync.o\
	ap/ap_wpa.o\
	ap/ap_data.o\
	ap/ap_autoChSel.o\
	ap/ap_cfg.o

obj_ap += \
	ap/ap_mbss_inf.o\
	os/linux/ap_ioctl.o

MOD_NAME = $(MODULE)

###############################################################################
#
# config for STA mode
#
###############################################################################

obj_sta += \
	sta/assoc.o\
	sta/auth.o\
	sta/auth_rsp.o\
	sta/sync.o\
	sta/sanity.o\
	sta/rtmp_data.o\
	sta/connect.o\
	sta/wpa.o\
	sta/sta_cfg.o\
	sta/sta.o


obj_sta += os/linux/sta_ioctl.o

MOD_NAME = $(MODULE)


###############################################################################
#
# config for AP/STA mixed mode
#
###############################################################################

MOD_NAME = $(MODULE)

###############################################################################
#
# Module Base
#
###############################################################################

#ifdef CONFIG_AP_SUPPORT
ifeq ($(RT28xx_MODE),AP)

$(MOD_NAME)-objs := \
	$(obj_ap)\
	$(obj_vht)\
	$(obj_cmm)\
	$(obj_wsc)\
	$(obj_phy)

$(MOD_NAME)-objs += \
	common/rt_os_util.o\
	os/linux/rt_linux.o\
	os/linux/rt_main_dev.o

#endif // CRDA_SUPPORT //

endif
#endif // CONFIG_AP_SUPPORT //

#ifdef CONFIG_STA_SUPPORT
ifeq ($(RT28xx_MODE), STA)

$(MOD_NAME)-objs := \
	$(obj_sta)\
	$(obj_vht)\
	$(obj_cmm)\
	$(obj_wsc)\
	$(obj_phy)

$(MOD_NAME)-objs += \
	common/rt_os_util.o\
	os/linux/sta_ioctl.o\
	os/linux/rt_linux.o\
	os/linux/rt_main_dev.o

#ifdef ETH_CONVERT
ifeq ($(HAS_ETH_CONVERT_SUPPORT), y)
$(MOD_NAME)-objs += \
	common/cmm_mat.o \
	common/cmm_mat_iparp.o \
	common/cmm_mat_pppoe.o \
	common/cmm_mat_ipv6.o
endif
#endif // ETH_CONVERT //




endif
#endif // CONFIG_STA_SUPPORT //


#ifdef CRDA_SUPPORT
ifeq ($(HAS_CFG80211_SUPPORT),y)
$(MOD_NAME)-objs += \
	os/linux/cfg80211/cfg80211.o\
	os/linux/cfg80211/cfg80211_util.o\
	os/linux/cfg80211/cfg80211_scan.o\
	os/linux/cfg80211/cfg80211_rx.o\
	os/linux/cfg80211/cfg80211_tx.o\
	os/linux/cfg80211/cfg80211_inf.o\
	os/linux/cfg80211/cfg80211_ap.o\
	os/linux/cfg80211/cfg80211drv.o
endif

#ifdef CONFIG_APSTA_SUPPORT
ifeq ($(RT28xx_MODE), APSTA)
$(MOD_NAME)-objs := \
	$(obj_ap)\
	$(obj_sta)\
	$(obj_vht)\
	$(obj_cmm)\
	$(obj_wsc)\
	$(obj_phy)

$(MOD_NAME)-objs += \
	common/rt_os_util.o\
	os/linux/sta_ioctl.o\
	os/linux/rt_linux.o\
	os/linux/rt_main_dev.o

#ifdef ETH_CONVERT
ifeq ($(HAS_ETH_CONVERT_SUPPORT), y)
$(MOD_NAME)-objs += \
	common/cmm_mat.o \
	common/cmm_mat_iparp.o \
	common/cmm_mat_pppoe.o \
	common/cmm_mat_ipv6.o
endif
#endif // ETH_CONVERT //




#ifdef CRDA_SUPPORT
ifeq ($(HAS_CFG80211_SUPPORT),y)
$(MOD_NAME)-objs += \
	os/linux/cfg80211/cfg80211.o\
	os/linux/cfg80211/cfg80211_util.o\
	os/linux/cfg80211/cfg80211_scan.o\
	os/linux/cfg80211/cfg80211_rx.o\
	os/linux/cfg80211/cfg80211_tx.o\
	os/linux/cfg80211/cfg80211_inf.o\
	os/linux/cfg80211/cfg80211_ap.o\
	os/linux/cfg80211/cfg80211drv.o
endif

endif
#endif // CONFIG_APSTA_SUPPORT //


#chip releated

$(MOD_NAME)-objs += \
	common/cmm_mac_usb.o\
	common/cmm_data_usb.o\
	common/rtusb_io.o\
	common/rtusb_bulk.o\
	os/linux/rt_usb.o\
	chips/rt65xx.o\
	chips/mt76x2.o\
	mac/ral_nmac.o\
	mcu/mcu_and.o\
	phy/mt_rf.o

$(MOD_NAME)-objs += \
	os/linux/rt_usb.o\
	os/linux/usb_main_dev.o\
	common/rtusb_dev_id.o

ifneq ($(findstring $(RT28xx_MODE),STA APSTA),)
$(MOD_NAME)-objs += \
        common/frq_cal.o
endif

obj-$(CONFIG_MT7612U) := $(MOD_NAME).o

ccflags-y += -I$(srctree)/$(src)/include
ccflags-y += $(EXTRA_CFLAGS)
