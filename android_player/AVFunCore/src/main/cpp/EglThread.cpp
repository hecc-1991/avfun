#include <unistd.h>
#include "EglThread.h"
#include "GLContext.h"

namespace avf {

    void EglThread::render(ANativeWindow *nativeWindow) {

        GLContext::GetInstance()->Init(nativeWindow);

        while (true) {
            if(th_interrupted){
                break;
            }
            glClearColor(0.09, 0.55, 0.02f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            GLContext::GetInstance()->Swap();
            usleep(30 * 100);
        }
    }

    void EglThread::onSurfaceCreate(ANativeWindow *nativeWindow) {

        th_render = std::thread(&EglThread::render, this, nativeWindow);
    }

    void EglThread::onSurfaceDestroy() {
        th_interrupted = true;
        th_render.join();
    }

}