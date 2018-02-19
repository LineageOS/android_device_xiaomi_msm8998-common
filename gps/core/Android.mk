LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libloc_core
LOCAL_MODULE_OWNER := qcom

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libcutils \
    libgps.utils \
    libdl \
    libloc_pla

LOCAL_SRC_FILES += \
    LocApiBase.cpp \
    LocAdapterBase.cpp \
    ContextBase.cpp \
    LocDualContext.cpp \
    loc_core_log.cpp \
    SystemStatus.cpp

LOCAL_CFLAGS += \
     -fno-short-enums \
     -D_ANDROID_

LOCAL_C_INCLUDES:= \
    $(TARGET_OUT_HEADERS)/gps.utils \
    $(TARGET_OUT_HEADERS)/libflp \
    $(TARGET_OUT_HEADERS)/libloc_pla

LOCAL_COPY_HEADERS_TO:= libloc_core/
LOCAL_COPY_HEADERS:= \
    LocApiBase.h \
    LocAdapterBase.h \
    ContextBase.h \
    LocDualContext.h \
    LBSProxyBase.h \
    UlpProxyBase.h \
    loc_core_log.h \
    LocAdapterProxyBase.h \
    SystemStatus.h

include $(BUILD_SHARED_LIBRARY)
