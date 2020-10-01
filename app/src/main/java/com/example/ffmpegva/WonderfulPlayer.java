package com.example.ffmpegva;

import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class WonderfulPlayer implements SurfaceHolder.Callback {

    static {
        System.loadLibrary("wonderful");
    }

    private SurfaceHolder surfaceHolder;

    public void setSurfaceHolder(SurfaceView surfaceView){
        if (surfaceHolder != null){
            surfaceHolder.removeCallback(this);
        }
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    public void startPlay(String path,String yuvPath,String yuvVideoPath,String yuvPgmPath){
        playVideo(path,yuvPath,yuvVideoPath,yuvPgmPath,surfaceHolder.getSurface());
        Log.d("FfmpegVA", "path: " + path + "surface: " + surfaceHolder.getSurface().toString());
    }

    private native void playVideo(String path,String yuvPath,String yuvVideoPath,String yuvPgmPath, Surface surface);
}
