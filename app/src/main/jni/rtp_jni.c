#include "rtp_jni.h"
#include "rtp/rtp_h264.h"

#define JNI_CLASS_IJKPLAYER     "com/rtp/h264/sample/RtpClient"
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))

#ifdef __GNUC__
    #define UNUSED_PARAM __attribute__ ((unused))
#else /* not a GCC */
    #define UNUSED_PARAM
#endif /* GCC */

static JavaVM* g_jvm = NULL;
static jobject g_obj = NULL;

static void jni_rtp_init(JNIEnv *env, jobject thiz){
    logd("%s", __func__);
    //保存全局JVM以便在子线程中使用
    (*env)->GetJavaVM(env,&g_jvm);
    //不能直接赋值(g_obj = thiz)
    g_obj = (*env)->NewGlobalRef(env, thiz);

    jclass clazz = (*env)->GetObjectClass(env, thiz);
    if(clazz == NULL){
    	(*env)->ThrowNew(env, "java/lang/NullPointerException", "Unable to find exception class");
    }
}

static jboolean jni_rtp_send(JNIEnv *env, jobject thiz, jstring path){
    logd("%s", __func__);
    if(path == NULL) return JNI_FALSE;

    char *file_path = (*env)->GetStringUTFChars(env, path, NULL);
    logi("file path=%s", file_path);
    rtp_setup(file_path);
    (*env)->ReleaseStringUTFChars(env, path, file_path);
    return JNI_TRUE;
}

static jboolean jni_rtp_destroy(JNIEnv *env, jobject thiz){
	logd("%s", __func__);
    rtp_destroy();
	return JNI_TRUE;
}

static JNINativeMethod g_methods[] = {
    { "_init",         "()V",                                               (void *) jni_rtp_init },
    { "_send",         "(Ljava/lang/String;)Z",           (void *) jni_rtp_send },
    { "_destroy",         "()Z",                                               (void *) jni_rtp_destroy},
 };

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv* env = NULL;

    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }
    assert(env != NULL);

    // FindClass returns LocalReference
    jclass klass = (*env)->FindClass (env, JNI_CLASS_IJKPLAYER);
    if (klass == NULL) {
      //LOGE ("Native registration unable to find class '%s'", JNI_CLASS_IJKPLAYER);
      return JNI_ERR;
    }
    (*env)->RegisterNatives(env, klass, g_methods, NELEM(g_methods) );

    return JNI_VERSION_1_4;
}
