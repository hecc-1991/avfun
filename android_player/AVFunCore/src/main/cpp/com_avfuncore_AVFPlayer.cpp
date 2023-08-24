#include <jni.h>
#include <string>
#include <vector>
#include <mutex>

#include "LogUtil.h"
#include "AVPlayer.h"

using avf::AVPlayer;

#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

struct fields_t {
    jfieldID context;
};
static fields_t fields;

static std::mutex mtx;

static void avfuncore_native_init(JNIEnv *env) {
    jclass clazz;
    clazz = env->FindClass("com/avfuncore/AVFPlayer");
    if (clazz == NULL) {
        return;
    }

    fields.context = env->GetFieldID(clazz, "mNativeContext", "J");
    if (fields.context == NULL) {
        LOG_ERROR("GetFieldID mNativeContext failed");
        return;
    }

    env->DeleteLocalRef(clazz);
}

static AVPlayer *getPlayer(JNIEnv *env, jobject thiz) {
    std::unique_lock<std::mutex> lock(mtx);
    AVPlayer *const player = (AVPlayer *) env->GetLongField(thiz, fields.context);
    return player;
}

static void avfuncore_native_setup(JNIEnv *env, jobject thiz) {
    LOG_INFO("init");
    std::unique_lock<std::mutex> lock(mtx);
    AVPlayer* player = new AVPlayer();
    env->SetLongField(thiz, fields.context, (jlong) player);
}

static void avfuncore_native_setDataSource(JNIEnv *env, jobject thiz, jstring path) {

    auto spath = env->GetStringUTFChars(path, NULL);
    LOG_INFO("path: %s", spath);

    auto player = getPlayer(env, thiz);
    player->OpenVideo(spath);

    env->ReleaseStringUTFChars(path, spath);

}

static void avfuncore_native_play(JNIEnv *env, jobject thiz) {
    LOG_INFO("play");
    auto player = getPlayer(env, thiz);
    player->Start();
}

static void avfuncore_native_pause(JNIEnv *env, jobject thiz) {
    LOG_INFO("pause");
}

static void avfuncore_native_seekTo(JNIEnv *env, jobject thiz, jlong timeMs) {
    LOG_INFO("seekTo: %ld", timeMs);

}

static void avfuncore_native_stop(JNIEnv *env, jobject thiz) {
    LOG_INFO("stop");

}

static void avfuncore_release(JNIEnv *env, jobject thiz) {
    LOG_INFO("release");
    std::unique_lock<std::mutex> lock(mtx);
    AVPlayer *player = (AVPlayer *) env->GetLongField(thiz, fields.context);
    delete player;
    env->SetLongField(thiz, fields.context, (jlong) 0);
}

//--------------------------------------------------------------------------------------------------

static const JNINativeMethod jniMethods[] = {
        {"native_init",    "()V",                   (void *) avfuncore_native_init},
        {"native_setup",   "()V",                   (void *) avfuncore_native_setup},
        {"_setDataSource", "(Ljava/lang/String;)V", (void *) avfuncore_native_setDataSource},
        {"_play",    "()V",                   (void *) avfuncore_native_play},
        {"_pause",   "()V",                   (void *) avfuncore_native_pause},
        {"_seekTo",  "(J)V",                  (void *) avfuncore_native_seekTo},
        {"_stop",   "()V",                   (void *) avfuncore_native_stop},
        {"_release", "()V",                   (void *) avfuncore_release},

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

static int registerPlayerMethods(JNIEnv *env) {
    return registerMethods(env, "com/avfuncore/AVFPlayer", jniMethods, NELEM(jniMethods));
}

extern int registerContextMethods(JNIEnv *env);

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void * /*reserved*/) {
    LOG_INFO("+++");
    JNIEnv *env = NULL;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }
    if (!registerPlayerMethods(env)) {
        return -1;
    }

    if (!registerContextMethods(env)) {
        return -1;
    }

    LOG_INFO("---");
    return JNI_VERSION_1_4;
}