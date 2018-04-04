/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

package com.yuntongxun.ecsdk.core.voip;

import android.graphics.ImageFormat;
import android.graphics.PixelFormat;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;

import com.hisun.phone.core.voice.util.Log4Util;
import com.yuntongxun.ecsdk.core.voip.VideoCaptureDeviceInfoAndroid.AndroidVideoCaptureDevice;

import java.util.List;
import java.util.concurrent.locks.ReentrantLock;

public class VideoCaptureAndroid implements PreviewCallback, Callback, ViEFilterRenderView.ViERendererCallback {

    private final static String TAG = "console";

    private Camera camera;
    private AndroidVideoCaptureDevice currentDevice = null;
    public ReentrantLock previewBufferLock = new ReentrantLock();
    // This lock takes sync with StartCapture and SurfaceChanged
    private ReentrantLock captureLock = new ReentrantLock();
    private int PIXEL_FORMAT = ImageFormat.NV21;
    PixelFormat pixelFormat = new PixelFormat();
    // True when the C++ layer has ordered the camera to be started.
    private boolean isCaptureStarted = false;
    private boolean isCaptureRunning = false;
    private boolean isSurfaceReady = false;
    private SurfaceHolder surfaceHolder = null;

    private final int numCaptureBuffers = 3;
    private int expectedFrameSize = 0;
    private int orientation = 0;
    private int id = 0;
    // C++ callback context variable.
    private long context = 0;
    private SurfaceHolder localPreview = null;
    // True if this class owns the preview video buffers.
    private boolean ownsBuffers = false;

    private int mCaptureWidth = -1;
    private int mCaptureHeight = -1;
    private int mCaptureFPS = -1;

    //
    boolean useFiterRenderView = false;
    private ViEFilterRenderView vieFilterRenderView = null;
    private SurfaceTexture vieFilterTexture = null;

    public static
    void DeleteVideoCaptureAndroid(VideoCaptureAndroid captureAndroid) {
    	if(ViESurfaceRenderer.DEBUG){
    		Log4Util.d(TAG, "DeleteVideoCaptureAndroid");
    	}

        captureAndroid.StopCapture();
        captureAndroid.camera.release();
        captureAndroid.camera = null;
        captureAndroid.context = 0;
    }

    public VideoCaptureAndroid(int in_id, long in_context, Camera in_camera,
            AndroidVideoCaptureDevice in_device) {
        id = in_id;
        context = in_context;
        camera = in_camera;
        currentDevice = in_device;
    }

    private int tryStartCapture(int width, int height, int frameRate) {
        if (camera == null) {
        	if(ViESurfaceRenderer.DEBUG){
        		Log4Util.e(TAG, "Camera not initialized %d" + id);
        	}
            return -1;
        }
        if(ViESurfaceRenderer.DEBUG){
        	
        	Log4Util.d(TAG, "tryStartCapture width: " + width +
        			",height:" + height +",frame rate:" + frameRate +
        			",isCaptureRunning:" + isCaptureRunning +
        			",isSurfaceReady:" + isSurfaceReady +
        			",isCaptureStarted:" + isCaptureStarted);
        }

        if (isCaptureRunning || !isSurfaceReady || !isCaptureStarted) {
            return 0;
        }

        try {
            if(useFiterRenderView) {
                camera.setPreviewTexture(vieFilterTexture);
            } else {
                camera.setPreviewDisplay(surfaceHolder);
            }

            CaptureCapabilityAndroid currentCapability =
                    new CaptureCapabilityAndroid();
            currentCapability.width = width;
            currentCapability.height = height;
            currentCapability.maxFPS = frameRate;
            PixelFormat.getPixelFormatInfo(PIXEL_FORMAT, pixelFormat);

            Camera.Parameters parameters = camera.getParameters();
            parameters.setPreviewSize(currentCapability.width,
                    currentCapability.height);
            parameters.setPreviewFormat(PIXEL_FORMAT);
            parameters.setPreviewFrameRate(currentCapability.maxFPS);
            parameters.setWhiteBalance(Camera.Parameters.WHITE_BALANCE_AUTO);
            parameters.setSceneMode(Camera.Parameters.SCENE_MODE_AUTO);
            if(ViESurfaceRenderer.DEBUG){
            	Log4Util.i(TAG, ""+"ha");

            	Log4Util.i(TAG, "set rotation:" + currentDevice.orientation);
            }
//            parameters.setRotation(currentDevice.orientation);
//            parameters.set("orientation", "portrait");
//            parameters.set("rotation", currentDevice.orientation);

            List<String> list= parameters.getSupportedFocusModes();
            if(list!=null&&list.contains(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO)){
                if(android.os.Build.VERSION.SDK_INT>=14 && currentDevice.frontCameraType ==
                        VideoCaptureDeviceInfoAndroid.FrontFacingCameraType.None) {
                    parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);//1连续对焦
                    camera.cancelAutoFocus();// 2如果要实现连续的自动对焦，这一句必须加上
                }
            }

            camera.setParameters(parameters);
            
            //camera.setDisplayOrientation(currentDevice.orientation);

            int bufSize = width * height * pixelFormat.bitsPerPixel / 8;
            byte[] buffer = null;
            for (int i = 0; i < numCaptureBuffers; i++) {
                buffer = new byte[bufSize];
                camera.addCallbackBuffer(buffer);
            }
            previewBufferLock.lock();
            expectedFrameSize = bufSize;
            if(ViESurfaceRenderer.DEBUG){
            	
            	Log4Util.i(TAG, "expectedFrameSize="+bufSize);
            }
            isCaptureRunning = true;
            previewBufferLock.unlock();
            if(!useFiterRenderView) {
                camera.setPreviewCallbackWithBuffer(this);
                ownsBuffers = true;
            }

            camera.startPreview();

        }
        catch (Exception ex) {
        	if(ViESurfaceRenderer.DEBUG){
        		
        		Log4Util.e(TAG, "Failed to start camera");
        	}
            return -1;
        }

        isCaptureRunning = true;
        return 0;
    }

    public int StartCapture(int width, int height, int frameRate) {
    	if(ViESurfaceRenderer.DEBUG){
    		
    		Log4Util.d(TAG, "StartCapture width " + width +
    				" height " + height +" frame rate " + frameRate);
    	}
        // Get the local preview SurfaceHolder from the static render class
        if(!useFiterRenderView) {
            localPreview = ViERenderer.GetLocalRenderer();
            if(ViESurfaceRenderer.DEBUG){

                Log4Util.d(TAG, "start capture :" + localPreview);
            }
            if (localPreview != null) {
                if(ViERenderer.isHolderReady()){
                    surfaceHolder = ViERenderer.getLocalHolder();
                    isSurfaceReady = true;
                }else
                    localPreview.addCallback(this);
            }
        } else {
    	    vieFilterRenderView = ViEFilterRenderView.getFilterRendererView();
            vieFilterRenderView.setImageFrameSize(width, height);
            vieFilterRenderView.setCallback(this);
            if(vieFilterRenderView != null) {
                if(vieFilterRenderView.isRenderReady()) {
                    vieFilterTexture = vieFilterRenderView.getRenderTexture();
                    isSurfaceReady = true;
                }
            }
        }

        captureLock.lock();
        isCaptureStarted = true;
        mCaptureWidth = width;
        mCaptureHeight = height;
        mCaptureFPS = frameRate;

        int res = tryStartCapture(mCaptureWidth, mCaptureHeight, mCaptureFPS);

        captureLock.unlock();
        return res;
    }

    public int StopCapture() {
    	if(ViESurfaceRenderer.DEBUG){
    		
    		Log4Util.d(TAG, "StopCapture");
    	}
        try {
            previewBufferLock.lock();
            isCaptureRunning = false;
            previewBufferLock.unlock();
            camera.stopPreview();
            camera.setPreviewCallbackWithBuffer(null);
        }
        catch (Exception ex) {
        	if(ViESurfaceRenderer.DEBUG){
        		
        		Log4Util.e(TAG, "Failed to stop camera");
        	}
            return -1;
        }

        isCaptureStarted = false;
        if(useFiterRenderView) {
            // stop之后需要重新创建vieFilterRender.
            // vieFilterRender.stopRender();
        }
        return 0;
    }

    native void ProvideCameraFrame(byte[] data, int length, long captureObject);

    public void onPreviewFrame(byte[] data, Camera camera) {
        if(useFiterRenderView) {
            return;
        }
        previewBufferLock.lock();
        if(ViESurfaceRenderer.DEBUG){
        	
        	Log4Util.e(TAG, "onPreviewFram : len="+data.length+",expect:"+expectedFrameSize);
        }
        // The following line is for debug only
        // Log.v(TAG, "preview frame length " + data.length +
        //            " context" + context);
        if (isCaptureRunning) {
            // If StartCapture has been called but not StopCapture
            // Call the C++ layer with the captured frame
        	
            if (data.length == expectedFrameSize) {
                ProvideCameraFrame(data, expectedFrameSize, context);
                if (ownsBuffers) {
                    // Give the video buffer to the camera service again.
                    camera.addCallbackBuffer(data);
                }
            }
        }
        previewBufferLock.unlock();
    }

    // Sets the rotation of the preview render window.
    // Does not affect the captured video image.
    public void SetPreviewRotation(int rotation) {
    	if(true) { //ViESurfaceRenderer.DEBUG){
    		
    		Log4Util.v(TAG, "SetPreviewRotation:" + rotation);
    	}

        if (camera != null) {
            previewBufferLock.lock();
            int width = 0;
            int height = 0;
            int framerate = 0;
            boolean isRestart = false;

            if (isCaptureRunning) {
                width = mCaptureWidth;
                height = mCaptureHeight;
                framerate = mCaptureFPS;
                StopCapture();
                isRestart = true;
            }

            int resultRotation = 0;
            if (currentDevice.frontCameraType ==
                    VideoCaptureDeviceInfoAndroid.FrontFacingCameraType.Android23) {
                // this is a 2.3 or later front facing camera.
                // SetDisplayOrientation will flip the image horizontally
                // before doing the rotation.
                resultRotation=(360-rotation) % 360; // compensate the mirror
                Log4Util.d(TAG, "VideoCaptureAndroid::SetPreviewRotation front rotation:"+rotation+" reRotation:%d"+resultRotation);
            }
            else {
                // Back facing or 2.2 or previous front camera
                resultRotation=rotation;
                Log4Util.d(TAG, "VideoCaptureAndroid::SetPreviewRotation back rotation:"+rotation+" reRotation:%d"+resultRotation);
            }
            camera.setDisplayOrientation(resultRotation);

            if (isRestart) {
                StartCapture(width, height, framerate);
            }
            previewBufferLock.unlock();
        }
    }

    public void surfaceChanged(SurfaceHolder holder,
                               int format, int width, int height) {
    	//if(ViESurfaceRenderer.DEBUG){
    		
    		Log4Util.d(TAG, "VideoCaptureAndroid::surfaceChanged");
    	//}

        captureLock.lock();
        isSurfaceReady = true;
        surfaceHolder = holder;

        tryStartCapture(mCaptureWidth, mCaptureHeight, mCaptureFPS);
        captureLock.unlock();
        return;
    }

    public void surfaceCreated(SurfaceHolder holder) {
    	//if(ViESurfaceRenderer.DEBUG){
    		
    		Log4Util.d(TAG, "VideoCaptureAndroid::surfaceCreated");
    	//}
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
    	//if(ViESurfaceRenderer.DEBUG){
    		
    		Log4Util.d(TAG, "VideoCaptureAndroid::surfaceDestroyed");
    	//}
        isSurfaceReady = false;
    }

    /**
     * ViEFilterRenderView.ViERendererCallback
     * Must Open Camera after filterRender ready
     */
    @Override
    public void onFilterRenderReady() {
        if(isSurfaceReady) {
            return;
        }
        Log4Util.d(TAG, "VideoCaptureAndroid::surfaceChanged");

        captureLock.lock();
        vieFilterTexture = vieFilterRenderView.getRenderTexture();
        isSurfaceReady = true;

        tryStartCapture(mCaptureWidth, mCaptureHeight, mCaptureFPS);
        captureLock.unlock();
    }

    /**
     * ViEFilterRenderView.ViERendererCallback, callback rgba image data after video filter
     * @param data      image data, format: rgba
     * @param length    image data length
     * @param width     image width
     * @param height    image height
     */
    @Override
    public void onIncomingRgbaFrame(byte[] data, int length, int width, int height) {
        if(!isCaptureStarted){
            return;
        }
        previewBufferLock.lock();
        ProvideCameraFrame(data, length, context);
        previewBufferLock.unlock();
    }
}
