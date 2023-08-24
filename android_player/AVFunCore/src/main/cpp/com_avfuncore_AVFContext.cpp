#include <jni.h>
#include <string>
#include <mutex>
#include <android/native_window_jni.h>

#include "LogUtil.h"

#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

#include "EglThread.h"
#include "GLContext.h"

using namespace avf;

struct fields_t {
    jfieldID context;
};
static fields_t fields;

static std::mutex mtx;

static EglThread *eglThread;

static void avfuncore_native_init(JNIEnv *env) {
    jclass clazz;
    clazz = env->FindClass("com/avfuncore/AVFContext");
    if (clazz == NULL) {
        return;
    }

    env->DeleteLocalRef(clazz);
}


static void avfuncore_nativeSurfaceCreate(JNIEnv *env, jobject thiz, jobject surface) {
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    LOG_INFO("avfuncore_nativeSurfaceCreate");
//    if(!eglThread){
//        eglThread = new EglThread();
//        eglThread->onSurfaceCreate(nativeWindow);
//    }
    GLContext::GetInstance()->Init(nativeWindow);
}

static void avfuncore_nativeSurfaceChanged(JNIEnv *env, jobject thiz, jint width, jint height) {

}

static void avfuncore_nativeSurfaceDestroyed(JNIEnv *env, jobject thiz) {

}

//--------------------------------------------------------------------------------------------------

static const JNINativeMethod jniMethods[] = {
        {"native_init", "()V", (void *) avfuncore_native_init},
        {"nativeSurfaceCreate", "(Landroid/view/Surface;)V", (void *) avfuncore_nativeSurfaceCreate},
        {"nativeSurfaceChanged", "(II)V", (void *) avfuncore_nativeSurfaceChanged},
        {"nativeSurfaceDestroyed", "()V", (void *) avfuncore_nativeSurfaceDestroyed},
};

static int registerMethods(JNIEnv *env, const char *className, const JNINativeMethod *gMethods,
                           int numMethods) {
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

int registerContextMethods(JNIEnv *env) {
    return registerMethods(env, "com/avfuncore/AVFContext", jniMethods, NELEM(jniMethods));
}
