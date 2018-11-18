LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE           := libril-wrapper
LOCAL_MULTILIB         := 64
LOCAL_VENDOR_MODULE    := true
LOCAL_SRC_FILES        := ril-wrapper.c
LOCAL_SHARED_LIBRARIES := android.hardware.radio.config@1.0 libdl liblog libril libcutils
LOCAL_CFLAGS           := -Wall -Werror
include $(BUILD_SHARED_LIBRARY)
