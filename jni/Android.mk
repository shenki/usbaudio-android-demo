LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE	:= usbaudio

LOCAL_LDLIBS := -llog
LOCAL_CFLAGS :=

LOCAL_SRC_FILES =  \
                   descriptor.c \
                   io.c \
                   core.c \
                   sync.c \
                   os/threads_posix.c \
                   os/linux_usbfs.c \
                   usbaudio_dump.c

include $(BUILD_SHARED_LIBRARY)

