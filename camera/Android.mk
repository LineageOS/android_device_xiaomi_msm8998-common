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
LOCAL_MODULE := libMiCameraHal
LOCAL_MULTILIB := 32
LOCAL_SRC_FILES := camera.c
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)
