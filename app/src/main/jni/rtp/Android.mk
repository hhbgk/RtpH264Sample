LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := rtp
LOCAL_SRC_FILES += rtp_jni.c
LOCAL_SRC_FILES += rtp_h264.c
LOCAL_SRC_FILES += rtp_queue.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)