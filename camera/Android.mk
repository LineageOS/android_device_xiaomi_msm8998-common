LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libcamshim
LOCAL_MULTILIB := 32
LOCAL_VENDOR_MODULE := true
LOCAL_SRC_FILES := camera.c
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
