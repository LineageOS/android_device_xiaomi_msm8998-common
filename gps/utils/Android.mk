LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

## Libs
LOCAL_SHARED_LIBRARIES := \
    libutils \
    libcutils \
    liblog \
    libloc_pla

LOCAL_SRC_FILES += \
    loc_log.cpp \
    loc_cfg.cpp \
    msg_q.c \
    linked_list.c \
    loc_target.cpp \
    platform_lib_abstractions/elapsed_millis_since_boot.cpp \
    LocHeap.cpp \
    LocTimer.cpp \
    LocThread.cpp \
    MsgTask.cpp \
    loc_misc_utils.cpp \
    loc_nmea.cpp

# Flag -std=c++11 is not accepted by compiler when LOCAL_CLANG is set to true
LOCAL_CFLAGS += \
     -fno-short-enums \
     -D_ANDROID_

ifeq ($(TARGET_BUILD_VARIANT),user)
   LOCAL_CFLAGS += -DTARGET_BUILD_VARIANT_USER
endif

LOCAL_LDFLAGS += -Wl,--export-dynamic

## Includes
LOCAL_HEADER_LIBRARIES := \
    libloc_pla_headers \
    liblocation_api_headers

LOCAL_MODULE := libgps.utils
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_OWNER := qti
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS += $(GNSS_CFLAGS)

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libgps.utils_headers
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_HEADER_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
