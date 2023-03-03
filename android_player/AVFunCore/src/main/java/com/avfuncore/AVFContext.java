package com.avfuncore;

import android.view.Surface;

public class AVFContext {
    static {
        System.loadLibrary("avfuncore");
        native_init();
    }

    private static native final void native_init();

    public native void nativeSurfaceCreate(Surface surface);

    public native void nativeSurfaceChanged(int width, int height);

    public native void nativeSurfaceDestroyed();
}
