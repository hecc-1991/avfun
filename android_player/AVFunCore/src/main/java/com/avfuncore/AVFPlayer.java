package com.avfuncore;

public class AVFPlayer {

    static {
        System.loadLibrary("avfuncore");
        native_init();
    }

    long mNativeContext;

    public AVFPlayer() {
        native_init();
    }

    public static AVFPlayer Create(String path) {
        AVFPlayer player = new AVFPlayer();
        player._setDataSource(path);
        return player;
    }

    public void play() {
        _play();
    }

    public void pause() {
        _pause();
    }

    public void seekTo(long timeMs) {
        _seekTo(timeMs);
    }

    public void release() {
        _release();
    }


    //----------------------------------------------------------------------------------------------

    private static native final void native_init();

    private native void native_setup();

    native void _setDataSource(String path);

    native void _play();

    native void _pause();

    native void _seekTo(long timeMs);

    private native void _release();

}