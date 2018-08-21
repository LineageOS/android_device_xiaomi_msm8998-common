LOCAL_PATH := $(call my-dir)

include $(CLEAN_VARS)
LOCAL_MODULE := android.hardware.camera.common@1.0-helper
LOCAL_MULTILIB := 32
LOCAL_C_INCLUDES := \
    system/media/private/camera/include \
    $(LOCAL_PATH)/include
LOCAL_SRC_FILES := \
    CameraModule.cpp \
    CameraMetadata.cpp \
    CameraParameters.cpp
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := \
    liblog \
    libbase \
    libcutils \
    liblog \
    libhidlbase \
    libhidltransport \
    libhwbinder \
    libutils \
    libhardware \
    libcamera_metadata \
    android.hardware.graphics.mapper@2.0
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)

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
