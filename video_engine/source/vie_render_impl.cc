/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "vie_render_impl.h"

#include "engine_configurations.h"
#include "video_render.h"
#include "video_render_defines.h"
#include "../system_wrappers/include/logging.h"
#include "vie_errors.h"
#include "vie_capturer.h"
#include "vie_channel.h"
#include "vie_channel_manager.h"
#include "vie_defines.h"
#include "vie_frame_provider_base.h"
#include "vie_impl.h"
#include "vie_input_manager.h"
#include "vie_render_manager.h"
#include "vie_renderer.h"
#include "vie_shared_data.h"

#include "../system_wrappers/include/trace.h"
#ifdef ENABLE_SCREEN_SHARE
#include "vie_desktop_share_manager.h"
#endif

namespace cloopenwebrtc {

ViERender* ViERender::GetInterface(VideoEngine* video_engine) {
#ifdef WEBRTC_VIDEO_ENGINE_RENDER_API
  if (!video_engine) {
    return NULL;
  }
  VideoEngineImpl* vie_impl = static_cast<VideoEngineImpl*>(video_engine);
  ViERenderImpl* vie_render_impl = vie_impl;
  // Increase ref count.
  (*vie_render_impl)++;
  return vie_render_impl;
#else
  return NULL;
#endif
}

int ViERenderImpl::Release() {
  // Decrease ref count
  (*this)--;
  int32_t ref_count = GetCount();
  if (ref_count < 0) {
    LOG(LS_ERROR) << "ViERender release too many times";
    return -1;
  }
  return ref_count;
}

ViERenderImpl::ViERenderImpl(ViESharedData* shared_data)
    : shared_data_(shared_data) {}

ViERenderImpl::~ViERenderImpl() {}

int ViERenderImpl::RegisterVideoRenderModule(
  VideoRender& render_module) {
  LOG_F(LS_INFO);
  if (shared_data_->render_manager()->RegisterVideoRenderModule(
      &render_module) != 0) {
    shared_data_->SetLastError(kViERenderUnknownError);
    return -1;
  }
  return 0;
}

int ViERenderImpl::DeRegisterVideoRenderModule(
  VideoRender& render_module) {
  LOG_F(LS_INFO);
  if (shared_data_->render_manager()->DeRegisterVideoRenderModule(
      &render_module) != 0) {
    // Error logging is done in ViERenderManager::DeRegisterVideoRenderModule.
    shared_data_->SetLastError(kViERenderUnknownError);
    return -1;
  }
  return 0;
}


int ViERenderImpl::AddRenderer(const int render_id, void* window,
	const unsigned int z_order, const float left,
	const float top, const float right,
	const float bottom,/*seanmodify20130402*/
	ReturnVideoWidthHeight return_video_width_height/*seanmodify*/) {
		WEBRTC_TRACE(kTraceApiCall, kTraceVideo, ViEId(shared_data_->instance_id()),
			"%s (render_id: %d,  window: 0x%p, z_order: %u, left: %f, "
			"top: %f, right: %f, bottom: %f)",
			__FUNCTION__, render_id, window, z_order, left, top, right,
			bottom);
		/*if (!shared_data_->Initialized()) {
			shared_data_->SetLastError(kViENotInitialized);
			WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(shared_data_->instance_id()),
				"%s - ViE instance %d not initialized", __FUNCTION__,
				shared_data_->instance_id());
			return -1;
		}*/
		{
			ViERenderManagerScoped rs(*(shared_data_->render_manager()));
			if (rs.Renderer(render_id)) {
				WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(shared_data_->instance_id()),
					"%s - Renderer already exist %d.", __FUNCTION__,
					render_id);
				shared_data_->SetLastError(kViERenderAlreadyExists);
				return -1;
			}
		}
#ifdef ENABLE_SCREEN_SHARE
		if(render_id >= kViEDesktopIdBase && render_id <= kViEDesktopIdMax){
			// Camera or file.
			ViEDesktopShareScoped is(*(shared_data_->desktop_share_manager()));
			ViEFrameProviderBase* frame_provider = is.FrameProvider(render_id);
			if (!frame_provider) {
				WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(shared_data_->instance_id()),
					"%s: FrameProvider id %d doesn't exist", __FUNCTION__,
					render_id);
				shared_data_->SetLastError(kViERenderInvalidRenderId);
				return -1;
			}
			ViERenderer* renderer = shared_data_->render_manager()->AddRenderStream(
				render_id, window, z_order, left, top, right, bottom);
			if (!renderer) {
				shared_data_->SetLastError(kViERenderUnknownError);
				return -1;
			}
			return frame_provider->RegisterFrameCallback(render_id, renderer);
		}
#endif
		if (render_id >= kViEChannelIdBase && render_id <= kViEChannelIdMax) {
			// This is a channel.
			ViEChannelManagerScoped cm(*(shared_data_->channel_manager()));
			ViEFrameProviderBase* frame_provider = cm.Channel(render_id);
			if (!frame_provider) {
				WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(shared_data_->instance_id()),
					"%s: FrameProvider id %d doesn't exist", __FUNCTION__,
					render_id);
				shared_data_->SetLastError(kViERenderInvalidRenderId);
				return -1;
			}
            
			ViERenderer* renderer = shared_data_->render_manager()->AddRenderStream(
				render_id, window, z_order, left, top, right, bottom);
			if (!renderer) {
				shared_data_->SetLastError(kViERenderUnknownError);
				return -1;
			}

			/*sean 20130402*/
			renderer->SetCallbackForWidthHeight(return_video_width_height);
			/*sean*/
			return frame_provider->RegisterFrameCallback(render_id, renderer);
		} else {
			// Camera or file.
			ViEInputManagerScoped is(*(shared_data_->input_manager()));
			ViEFrameProviderBase* frame_provider = is.FrameProvider(render_id);
			if (!frame_provider) {
				WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(shared_data_->instance_id()),
					"%s: FrameProvider id %d doesn't exist", __FUNCTION__,
					render_id);
				shared_data_->SetLastError(kViERenderInvalidRenderId);
				return -1;
			}
            
			ViERenderer* renderer = shared_data_->render_manager()->AddRenderStream(
				render_id, window, z_order, left, top, right, bottom);
			if (!renderer) {
				shared_data_->SetLastError(kViERenderUnknownError);
				return -1;
			}
			return frame_provider->RegisterFrameCallback(render_id, renderer);
		}
}

int ViERenderImpl::RemoveRenderer(const int render_id) {
  LOG_F(LS_INFO) << "render_id: " << render_id;
  ViERenderer* renderer = NULL;
  {
    ViERenderManagerScoped rs(*(shared_data_->render_manager()));
    renderer = rs.Renderer(render_id);
    if (!renderer) {
      shared_data_->SetLastError(kViERenderInvalidRenderId);
      return -1;
    }
    // Leave the scope lock since we don't want to lock two managers
    // simultanousely.
  }
  if (render_id >= kViEChannelIdBase && render_id <= kViEChannelIdMax) {
    // This is a channel.
    ViEChannelManagerScoped cm(*(shared_data_->channel_manager()));
    ViEChannel* channel = cm.Channel(render_id);
    if (!channel) {
      shared_data_->SetLastError(kViERenderInvalidRenderId);
      return -1;
    }
    channel->DeregisterFrameCallback(renderer);
  }
#ifdef ENABLE_SCREEN_SHARE
  else if (render_id >= kViEDesktopIdBase && render_id <= kViEDesktopIdMax) {
	  // Desktop capture.
	  ViEDesktopShareScoped is(*(shared_data_->desktop_share_manager()));
	  ViEFrameProviderBase* frame_provider = is.FrameProvider(render_id);
	  if (!frame_provider) {
		  WEBRTC_TRACE(kTraceError, kTraceVideo, ViEId(shared_data_->instance_id()),
			  "%s: FrameProvider id %d doesn't exist", __FUNCTION__,
			  render_id);
		  shared_data_->SetLastError(kViERenderInvalidRenderId);
		  return -1;
	  }
	  frame_provider->DeregisterFrameCallback(renderer);
  }
#endif 
 else {
    // Provider owned by inputmanager, i.e. file or capture device.
    ViEInputManagerScoped is(*(shared_data_->input_manager()));
    ViEFrameProviderBase* provider = is.FrameProvider(render_id);
    if (!provider) {
      shared_data_->SetLastError(kViERenderInvalidRenderId);
      return -1;
    }
    provider->DeregisterFrameCallback(renderer);
  }
  if (shared_data_->render_manager()->RemoveRenderStream(render_id) != 0) {
    shared_data_->SetLastError(kViERenderUnknownError);
    return -1;
  }
  return 0;
}
 
int ViERenderImpl::StartRender(const int render_id) {
  LOG_F(LS_INFO) << "render_id: " << render_id;
  ViERenderManagerScoped rs(*(shared_data_->render_manager()));
  ViERenderer* renderer = rs.Renderer(render_id);
  if (!renderer) {
    shared_data_->SetLastError(kViERenderInvalidRenderId);
    return -1;
  }
  if (renderer->StartRender() != 0) {
    shared_data_->SetLastError(kViERenderUnknownError);
    return -1;
  }
  return 0;
}

int ViERenderImpl::StopRender(const int render_id) {
  LOG_F(LS_INFO) << "render_id: " << render_id;
  ViERenderManagerScoped rs(*(shared_data_->render_manager()));
  ViERenderer* renderer = rs.Renderer(render_id);
  if (!renderer) {
    shared_data_->SetLastError(kViERenderInvalidRenderId);
    return -1;
  }
  if (renderer->StopRender() != 0) {
    shared_data_->SetLastError(kViERenderUnknownError);
    return -1;
  }
  return 0;
}
    
int ViERenderImpl::ChangeWindow(int render_id, void *video_window) {
    LOG_F(LS_INFO) << "render_id: " << render_id;
    ViERenderManagerScoped rs(*(shared_data_->render_manager()));
    ViERenderer* renderer = rs.Renderer(render_id);
    if (!renderer) {
        shared_data_->SetLastError(kViERenderInvalidRenderId);
        return -1;
    }
    if (renderer->ChangeWindow(video_window) != 0) {
        shared_data_->SetLastError(kViERenderUnknownError);
        return -1;
    }
    return 0;
}
    
int ViERenderImpl::SetExpectedRenderDelay(int render_id, int render_delay) {
  LOG_F(LS_INFO) << "render_id: " << render_id
                 << " render_delay: " << render_delay;
  ViERenderManagerScoped rs(*(shared_data_->render_manager()));
  ViERenderer* renderer = rs.Renderer(render_id);
  if (!renderer) {
    shared_data_->SetLastError(kViERenderInvalidRenderId);
    return -1;
  }
  if (renderer->SetExpectedRenderDelay(render_delay) != 0) {
    shared_data_->SetLastError(kViERenderUnknownError);
    return -1;
  }
  return 0;
}

int ViERenderImpl::ConfigureRender(int render_id, const unsigned int z_order,
                                   const float left, const float top,
                                   const float right, const float bottom) {
  LOG_F(LS_INFO) << "render_id: " << render_id << " z_order: " << z_order
                 << " left: " << left << " top: " << top << " right: " << right
                 << " bottom: " << bottom;
  ViERenderManagerScoped rs(*(shared_data_->render_manager()));
  ViERenderer* renderer = rs.Renderer(render_id);
  if (!renderer) {
    shared_data_->SetLastError(kViERenderInvalidRenderId);
    return -1;
  }

  if (renderer->ConfigureRenderer(z_order, left, top, right, bottom) != 0) {
    shared_data_->SetLastError(kViERenderUnknownError);
    return -1;
  }
  return 0;
}

int ViERenderImpl::MirrorRenderStream(const int render_id, const bool enable,
                                      const bool mirror_xaxis,
                                      const bool mirror_yaxis) {
  ViERenderManagerScoped rs(*(shared_data_->render_manager()));
  ViERenderer* renderer = rs.Renderer(render_id);
  if (!renderer) {
    shared_data_->SetLastError(kViERenderInvalidRenderId);
    return -1;
  }
  if (renderer->EnableMirroring(render_id, enable, mirror_xaxis, mirror_yaxis)
      != 0) {
    shared_data_->SetLastError(kViERenderUnknownError);
    return -1;
  }
  return 0;
}

int ViERenderImpl::AddRenderer(const int render_id,
                               RawVideoType video_input_format,
                               ExternalRenderer* external_renderer) {
  // Check if the client requested a format that we can convert the frames to.
  if (video_input_format != kVideoI420 &&
      video_input_format != kVideoYV12 &&
      video_input_format != kVideoYUY2 &&
      video_input_format != kVideoUYVY &&
      video_input_format != kVideoARGB &&
      video_input_format != kVideoRGB24 &&
      video_input_format != kVideoRGB565 &&
      video_input_format != kVideoARGB4444 &&
      video_input_format != kVideoARGB1555) {
    LOG(LS_ERROR) << "Unsupported video frame format requested.";
    shared_data_->SetLastError(kViERenderInvalidFrameFormat);
    return -1;
  }
  {
    // Verify the renderer doesn't exist.
    ViERenderManagerScoped rs(*(shared_data_->render_manager()));
    if (rs.Renderer(render_id)) {
      LOG_F(LS_ERROR) << "Renderer already exists for render_id: " << render_id;
      shared_data_->SetLastError(kViERenderAlreadyExists);
      return -1;
    }
  }
  if (render_id >= kViEChannelIdBase && render_id <= kViEChannelIdMax) {
    // This is a channel.
    ViEChannelManagerScoped cm(*(shared_data_->channel_manager()));
    ViEFrameProviderBase* frame_provider = cm.Channel(render_id);
    if (!frame_provider) {
      shared_data_->SetLastError(kViERenderInvalidRenderId);
      return -1;
    }
    ViERenderer* renderer = shared_data_->render_manager()->AddRenderStream(
        render_id, NULL, 0, 0.0f, 0.0f, 1.0f, 1.0f);
    if (!renderer) {
      shared_data_->SetLastError(kViERenderUnknownError);
      return -1;
    }
    if (renderer->SetExternalRenderer(render_id, video_input_format,
                                      external_renderer) == -1) {
      shared_data_->SetLastError(kViERenderUnknownError);
      return -1;
    }

    return frame_provider->RegisterFrameCallback(render_id, renderer);
  } else {
    // Camera or file.
    ViEInputManagerScoped is(*(shared_data_->input_manager()));
    ViEFrameProviderBase* frame_provider = is.FrameProvider(render_id);
    if (!frame_provider) {
      shared_data_->SetLastError(kViERenderInvalidRenderId);
      return -1;
    }
    ViERenderer* renderer = shared_data_->render_manager()->AddRenderStream(
        render_id, NULL, 0, 0.0f, 0.0f, 1.0f, 1.0f);
    if (!renderer) {
      shared_data_->SetLastError(kViERenderUnknownError);
      return -1;
    }
    if (renderer->SetExternalRenderer(render_id, video_input_format,
                                      external_renderer) == -1) {
      shared_data_->SetLastError(kViERenderUnknownError);
      return -1;
    }
    return frame_provider->RegisterFrameCallback(render_id, renderer);
  }
}

int ViERenderImpl::AddRenderCallback(int render_id,
                                     VideoRenderCallback* callback) {
  if (render_id < kViEChannelIdBase || render_id > kViEChannelIdMax)
    return -1;
  // This is a channel.
#if 0
  ViEChannelManagerScoped cm(*(shared_data_->channel_manager()));
  ViEFrameProviderBase* frame_provider = cm.Channel(render_id);
  if (!frame_provider) {
    shared_data_->SetLastError(kViERenderInvalidRenderId);
    return -1;
  }
  ViERenderer* renderer = shared_data_->render_manager()->AddRenderStream(
      render_id, NULL, 0, 0.0f, 0.0f, 1.0f, 1.0f);
  if (!renderer) {
    shared_data_->SetLastError(kViERenderUnknownError);
    return -1;
  }
  if (renderer->SetVideoRenderCallback(render_id, callback) != 0) {
    shared_data_->SetLastError(kViERenderUnknownError);
    return -1;
  }

  return frame_provider->RegisterFrameCallback(render_id, renderer);
#endif

  ViERenderManagerScoped rs(*(shared_data_->render_manager()));
  ViERenderer* renderer = rs.Renderer(render_id);
  if (!renderer) {
	  shared_data_->SetLastError(kViERenderUnknownError);
	  return -1;
  }
  //if (renderer->SetVideoRenderCallback(render_id, callback) != 0) {
	 // shared_data_->SetLastError(kViERenderUnknownError);
	 // return -1;
  //}
  if (renderer->AddVideoRenderCallback(render_id, callback) != 0)
  {
	  shared_data_->SetLastError(kViERenderUnknownError);
	  return -1;
  }
  return 0;
}

}  // namespace webrtc
