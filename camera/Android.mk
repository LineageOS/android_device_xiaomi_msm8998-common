LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libcamshim
LOCAL_MULTILIB := 32
LOCAL_SRC_FILES := camera.c
LOCAL_MODULE_TAGS := optional
LOCAL_WHOLE_STATIC_LIBRARIES := \
    libminikin
LOCAL_SHARED_LIBRARIES := \
    libharfbuzz_ng \
    libft2 \
    libz \
    libutils \
    liblog \
    libicuuc
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libcamshim.vendor
LOCAL_REQUIRED_MODULES := libcamshim
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_PREBUILT_MODULE_FILE := $(2ND_TARGET_OUT_INTERMEDIATE_LIBRARIES)/libcamshim.so
LOCAL_STRIP_MODULE := false
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional
LOCAL_INSTALLED_MODULE_STEM := libcamshim.so
LOCAL_MODULE_SUFFIX := .so
LOCAL_VENDOR_MODULE := true
include $(BUILD_PREBUILT)
