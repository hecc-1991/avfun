#ifndef AVFUNPLAYER_GLCONTEXT_H
#define AVFUNPLAYER_GLCONTEXT_H

#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace avf {

    class GLContext {
    private:
        // EGL configurations
        ANativeWindow* window_;
        EGLDisplay display_;
        EGLSurface surface_;
        EGLContext context_;
        EGLConfig config_;

        // Screen parameters
        int32_t screen_width_;
        int32_t screen_height_;
        int32_t color_size_;
        int32_t depth_size_;

        // Flags
        bool gles_initialized_;
        bool egl_context_initialized_;
        bool es3_supported_;
        float gl_version_;
        bool context_valid_;

        void InitGLES();
        void Terminate();
        bool InitEGLSurface();
        bool InitEGLContext();

        GLContext(GLContext const&);
        void operator=(GLContext const&);
        GLContext();
        virtual ~GLContext();

    public:
        static GLContext* GetInstance() {
            // Singleton
            static GLContext instance;

            return &instance;
        }

        bool Init(ANativeWindow* window);

        bool Make();

        EGLint Swap();

        bool Invalidate();

        void Suspend();
        EGLint Resume(ANativeWindow* window);

        ANativeWindow* GetANativeWindow(void) const { return window_; };
        int32_t GetScreenWidth() const { return screen_width_; }
        int32_t GetScreenHeight() const { return screen_height_; }

        int32_t GetBufferColorSize() const { return color_size_; }
        int32_t GetBufferDepthSize() const { return depth_size_; }
        float GetGLVersion() const { return gl_version_; }
        bool CheckExtension(const char* extension);

        EGLDisplay GetDisplay() const { return display_; }
        EGLSurface GetSurface() const { return surface_; }
    };
}


#endif
