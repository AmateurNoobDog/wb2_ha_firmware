#
# "main" pseudo-component makefile.
#
include $(BL60X_SDK_PATH)/components/network/ble/ble_common.mk
ifeq ($(CONFIG_ENABLE_PSM_RAM),0)
CPPFLAGS += -DCONF_USER_ENABLE_PSRAM
endif

ifeq ($(CONFIG_ENABLE_VFS_ROMFS),1)
CPPFLAGS += -DCONF_USER_ENABLE_VFS_ROMFS
endif

CFLAGS   += -DBFLB_BLE

ifeq ($(CONFIG_BT_MESH),1)
CFLAGS   += -DCONFIG_BT_MESH
endif

ifeq ($(CONFIG_BLE_TP_SERVER),1)
CFLAGS += -DCONFIG_BLE_TP_SERVER
endif

ifeq ($(CONFIG_BT_STACK_PTS),1)
CFLAGS += -DCONFIG_BT_STACK_PTS
endif

# compiler setting
CFLAGS += -Wno-unused-local-typedefs
