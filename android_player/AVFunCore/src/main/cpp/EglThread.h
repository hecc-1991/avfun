#ifndef AVFUNPLAYER_EGLTHREAD_H
#define AVFUNPLAYER_EGLTHREAD_H

#include <thread>
#include <android/native_window_jni.h>

namespace avf {

    class EglThread {

    public:
        void onSurfaceCreate(ANativeWindow *nativeWindow);
        void onSurfaceDestroy();

    private:
        void render(ANativeWindow *nativeWindow);

    private:
        int th_interrupted{false};
        std::thread th_render;

    };
}

#endif
