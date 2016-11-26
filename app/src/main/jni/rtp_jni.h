//
// Created by bob on 16-9-13.
//

#ifndef RTPH264SIMPLE_RTP_JNI_H
#define RTPH264SIMPLE_RTP_JNI_H

#ifdef __ANDROID__
#include <android/log.h>
#include <jni.h>
#endif

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#ifdef __ANDROID__

#include <android/log.h>
#define TAG "rtp_jni"
#define logd(...)  ((void)__android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__))
#define logi(...)  ((void)__android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__))
#define logw(...)  ((void)__android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__))
#define loge(...)  ((void)__android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__))

#else

#define LOGI(...)
#define LOGW(...)
#define LOGE(...)

#endif//__ANDROID__
#endif //RTPH264SIMPLE_RTP_JNI_H
