GNSS_CFLAGS := \
    -Werror \
    -Wno-undefined-bool-conversion

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libloc_pla_headers
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/android
include $(BUILD_HEADER_LIBRARY)
