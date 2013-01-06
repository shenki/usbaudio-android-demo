LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libusb-1.0

LOCAL_SRC_FILES := lib/$(TARGET_ARCH_ABI)/libusb-1.0.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include/libusb-1.0
LOCAL_EXPORT_LDLIBS := -llog

include $(PREBUILT_SHARED_LIBRARY)
