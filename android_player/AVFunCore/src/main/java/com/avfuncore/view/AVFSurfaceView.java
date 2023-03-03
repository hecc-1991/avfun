package com.avfuncore.view;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import androidx.annotation.NonNull;

import com.avfuncore.AVFContext;

public class AVFSurfaceView extends SurfaceView implements SurfaceHolder.Callback {

    AVFContext mContext;

    public AVFSurfaceView(Context context) {
        super(context);
    }

    public AVFSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = new AVFContext();
        getHolder().addCallback(this);
    }

    public AVFSurfaceView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public AVFSurfaceView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder surfaceHolder) {
        Log.i("hecc","surfaceCreated");
        mContext.nativeSurfaceCreate(surfaceHolder.getSurface());
    }

    @Override
    public void surfaceChanged(@NonNull SurfaceHolder surfaceHolder, int format, int w, int h) {
        Log.i("hecc","surfaceChanged:format="+format+", WxH="+w+"x"+h);
        mContext.nativeSurfaceChanged(w,h);
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder surfaceHolder) {
        Log.i("hecc","surfaceDestroyed");
        mContext.nativeSurfaceDestroyed();
    }

}
