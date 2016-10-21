/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 */

#include "h264_video_toolbox_decoder.h"

#if defined(WEBRTC_VIDEO_TOOLBOX_SUPPORTED)

#include <memory>

#if defined(WEBRTC_IOS)
#include "RTCUIApplication.h"
#endif
#include "libyuv/convert.h"
#include "checks.h"
#include "logging.h"
#include "corevideo_frame_buffer.h"
#include "h264_video_toolbox_nalu.h"
#include "video_frame.h"

namespace cloopenwebrtc {
    
    static const int64_t kMsPerSec = 1000;
    
    // Convenience function for creating a dictionary.
    inline CFDictionaryRef CreateCFDictionary(CFTypeRef* keys,
                                              CFTypeRef* values,
                                              size_t size) {
        return CFDictionaryCreate(nullptr, keys, values, size,
                                  &kCFTypeDictionaryKeyCallBacks,
                                  &kCFTypeDictionaryValueCallBacks);
    }
    
    // Struct that we pass to the decoder per frame to decode. We receive it again
    // in the decoder callback.
    struct FrameDecodeParams {
        FrameDecodeParams(DecodedImageCallback* cb, int64_t ts , int64_t ntp_time)
        : callback(cb), timestamp(ts),ntp_time_ms(ntp_time) {}
        DecodedImageCallback* callback;
        int64_t timestamp;
        int64_t ntp_time_ms;
    };
    
    // This is the callback function that VideoToolbox calls when decode is
    // complete.
    void VTDecompressionOutputCallback(void* decoder,
                                       void* params,
                                       OSStatus status,
                                       VTDecodeInfoFlags info_flags,
                                       CVImageBufferRef image_buffer,
                                       CMTime timestamp,
                                       CMTime duration) {
        FrameDecodeParams* decode_params(
                                         reinterpret_cast<FrameDecodeParams*>(params));
        if (status != noErr) {
            LOG(LS_ERROR) << "Failed to decode frame. Status: " << status;
            return;
        }
       
        // TODO(tkchin): Handle CVO properly.
        scoped_refptr<VideoFrameBuffer> buffer2 =
        new RefCountedObject<CoreVideoFrameBuffer>(image_buffer);
        scoped_refptr<VideoFrameBuffer> buffer = buffer2->NativeToI420Buffer();
        I420VideoFrame decoded_frame; 
        
        int size_y = buffer->height()*buffer->StrideY();
        int size_u = ((buffer->height()+1)/2)*buffer->StrideU();
        int size_v = ((buffer->height()+1)/2)*buffer->StrideV();
        decoded_frame.CreateFrame(size_y, buffer->DataY(), size_u, buffer->DataU(), size_v, buffer->DataV(), buffer->width(), buffer->height(), buffer->StrideY(), buffer->StrideU(), buffer->StrideV());
        decoded_frame.set_timestamp(decode_params->timestamp);
        //decoded_frame.set_ntp_time_ms( CMTimeGetSeconds(timestamp) * kMsPerSec);
        decoded_frame.set_ntp_time_ms(decode_params->ntp_time_ms);
        decode_params->callback->Decoded(decoded_frame);
        //printf("decode success timestamp %lld  %lld %lld\n",decode_params->timestamp/90 ,timestamp.value , duration.value);
        delete decode_params;
    }
    
}  // namespace internal

namespace cloopenwebrtc {
    H264VideoToolboxDecoder* H264VideoToolboxDecoder::Create() {
        return new H264VideoToolboxDecoder();
    }
    
    
    H264VideoToolboxDecoder::H264VideoToolboxDecoder()
    : callback_(nullptr),
    video_format_(nullptr),
    decompression_session_(nullptr) {}
    
    H264VideoToolboxDecoder::~H264VideoToolboxDecoder() {
        DestroyDecompressionSession();
        SetVideoFormat(nullptr);
    }
    
    int H264VideoToolboxDecoder::InitDecode(const VideoCodec* video_codec,
                                            int number_of_cores) {
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
    int H264VideoToolboxDecoder::Decode(
                                        const EncodedImage& input_image,
                                        bool missing_frames,
                                        const RTPFragmentationHeader* fragmentation,
                                        const CodecSpecificInfo* codec_specific_info,
                                        int64_t render_time_ms) {
        DCHECK(input_image._buffer);
        
#if defined(WEBRTC_IOS)
        if (!RTCIsUIApplicationActive()) {
            // Ignore all decode requests when app isn't active. In this state, the
            // hardware decoder has been invalidated by the OS.
            // Reset video format so that we won't process frames until the next
            // keyframe.
            SetVideoFormat(nullptr);
            return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
        }
#endif
        CMVideoFormatDescriptionRef input_format = nullptr;
        if (H264AnnexBBufferHasVideoFormatDescription(input_image._buffer,
                                                      input_image._length)) {
            input_format = CreateVideoFormatDescription(input_image._buffer,
                                                        input_image._length);
            if (input_format) {
                // Check if the video format has changed, and reinitialize decoder if
                // needed.
                if (!CMFormatDescriptionEqual(input_format, video_format_)) {
                    SetVideoFormat(input_format);
                    ResetDecompressionSession();
                }
                CFRelease(input_format);
            }
        }
        if (!video_format_) {
            // We received a frame but we don't have format information so we can't
            // decode it.
            // This can happen after backgrounding. We need to wait for the next
            // sps/pps before we can resume so we request a keyframe by returning an
            // error.
            LOG(LS_WARNING) << "Missing video format. Frame with sps/pps required.";
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        CMSampleBufferRef sample_buffer = nullptr;
        if (!H264AnnexBBufferToCMSampleBuffer(input_image._buffer,
                                              input_image._length, video_format_,
                                              &sample_buffer)) {
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        
        DCHECK(sample_buffer);
        VTDecodeFrameFlags decode_flags =
        kVTDecodeFrame_EnableAsynchronousDecompression ;
        FrameDecodeParams* frame_decode_params;
        frame_decode_params = new FrameDecodeParams(callback_, input_image._timeStamp,input_image.ntp_time_ms_);
        OSStatus status = VTDecompressionSessionDecodeFrame(
                                                            decompression_session_, sample_buffer, decode_flags,
                                                            frame_decode_params, nullptr);
#if defined(WEBRTC_IOS)
        // Re-initialize the decoder if we have an invalid session while the app is
        // active and retry the decode request.
        if (status == kVTInvalidSessionErr &&
            ResetDecompressionSession() == WEBRTC_VIDEO_CODEC_OK) {
            frame_decode_params = new FrameDecodeParams(callback_, input_image._timeStamp,input_image.ntp_time_ms_);
            status = VTDecompressionSessionDecodeFrame(
                                                       decompression_session_, sample_buffer, decode_flags,
                                                       frame_decode_params, nullptr);
        }
#endif
        CFRelease(sample_buffer);
        if (status != noErr) {
            LOG(LS_ERROR) << "Failed to decode frame with code: " << status;
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
    int H264VideoToolboxDecoder::RegisterDecodeCompleteCallback(
                                                                DecodedImageCallback* callback) {
        DCHECK(!callback_);
        callback_ = callback;
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
    int H264VideoToolboxDecoder::Release() {
        // Need to invalidate the session so that callbacks no longer occur and it
        // is safe to null out the callback.
        DestroyDecompressionSession();
        SetVideoFormat(nullptr);
        callback_ = nullptr;
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
    int H264VideoToolboxDecoder::ResetDecompressionSession() {
        DestroyDecompressionSession();
        
        // Need to wait for the first SPS to initialize decoder.
        if (!video_format_) {
            return WEBRTC_VIDEO_CODEC_OK;
        }
        
        // Set keys for OpenGL and IOSurface compatibilty, which makes the encoder
        // create pixel buffers with GPU backed memory. The intent here is to pass
        // the pixel buffers directly so we avoid a texture upload later during
        // rendering. This currently is moot because we are converting back to an
        // I420 frame after decode, but eventually we will be able to plumb
        // CVPixelBuffers directly to the renderer.
        // TODO(tkchin): Maybe only set OpenGL/IOSurface keys if we know that that
        // we can pass CVPixelBuffers as native handles in decoder output.
        static size_t const attributes_size = 3;
        CFTypeRef keys[attributes_size] = {
#if defined(WEBRTC_IOS)
            kCVPixelBufferOpenGLESCompatibilityKey,
#elif defined(WEBRTC_MAC)
            kCVPixelBufferOpenGLCompatibilityKey,
#endif
            kCVPixelBufferIOSurfacePropertiesKey,
            kCVPixelBufferPixelFormatTypeKey
        };
        CFDictionaryRef io_surface_value =
        CreateCFDictionary(nullptr, nullptr, 0);
        int64_t nv12type = kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
        CFNumberRef pixel_format = CFNumberCreate(nullptr, kCFNumberLongType, &nv12type);
        CFTypeRef values[attributes_size] = {kCFBooleanTrue, io_surface_value,
            pixel_format};
        CFDictionaryRef attributes = CreateCFDictionary(keys, values, attributes_size);
        if (io_surface_value) {
            CFRelease(io_surface_value);
            io_surface_value = nullptr;
        }
        if (pixel_format) {
            CFRelease(pixel_format);
            pixel_format = nullptr;
        }
        VTDecompressionOutputCallbackRecord record = {
            VTDecompressionOutputCallback, this,
        };
        OSStatus status =
        VTDecompressionSessionCreate(nullptr, video_format_, nullptr, attributes,
                                     &record, &decompression_session_);
        CFRelease(attributes);
        if (status != noErr) {
            DestroyDecompressionSession();
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        ConfigureDecompressionSession();
        
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
    void H264VideoToolboxDecoder::ConfigureDecompressionSession() {
        DCHECK(decompression_session_);
#if defined(WEBRTC_IOS)
        VTSessionSetProperty(decompression_session_,
                             kVTDecompressionPropertyKey_RealTime, kCFBooleanTrue);
#endif
    }
    
    void H264VideoToolboxDecoder::DestroyDecompressionSession() {
        if (decompression_session_) {
            VTDecompressionSessionInvalidate(decompression_session_);
            CFRelease(decompression_session_);
            decompression_session_ = nullptr;
        }
    }
    
    void H264VideoToolboxDecoder::SetVideoFormat(
                                                 CMVideoFormatDescriptionRef video_format) {
        if (video_format_ == video_format) {
            return;
        }
        if (video_format_) {
            CFRelease(video_format_);
        }
        video_format_ = video_format;
        if (video_format_) {
            CFRetain(video_format_);
        }
    }
    
    const char* H264VideoToolboxDecoder::ImplementationName() const {
        return "VideoToolbox";
    }
    
    WebRtc_Word32
    H264VideoToolboxDecoder::Reset()
    {
        return WEBRTC_VIDEO_CODEC_OK;
    }
    
}  // namespace webrtc

#endif  // defined(WEBRTC_VIDEO_TOOLBOX_SUPPORTED)