LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libusb-1.0
LOCAL_SRC_FILES := libusb-1.0/lib/$(TARGET_ARCH_ABI)/libusb-1.0.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/libusb-1.0/include/libusb-1.0
LOCAL_EXPORT_LDLIBS := -llog
include $(PREBUILT_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE	:= usbaudio
LOCAL_SHARED_LIBRARIES := libusb-1.0
LOCAL_LDLIBS := -llog
#LOCAL_CFLAGS :=
LOCAL_SRC_FILES := usbaudio_dump.c
include $(BUILD_SHARED_LIBRARY)

