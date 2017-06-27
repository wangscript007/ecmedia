/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "./common_video/video_render_frames.h"

#include <utility>

#include "./base/logging.h"
#include "./base/timeutils.h"
#include "./module/interface/module_common_types.h"
#include "./system_wrappers/include/trace.h"

namespace cloopenwebrtc {
namespace {
// Don't render frames with timestamp older than 500ms from now.
const int kOldRenderTimestampMS = 500;
// Don't render frames with timestamp more than 10s into the future.
const int kFutureRenderTimestampMS = 10000;

const uint32_t kEventMaxWaitTimeMs = 200;
const uint32_t kMinRenderDelayMs = 10;
const uint32_t kMaxRenderDelayMs = 500;
const size_t kMaxIncomingFramesBeforeLogged = 100;

uint32_t EnsureValidRenderDelay(uint32_t render_delay) {
  return (render_delay < kMinRenderDelayMs || render_delay > kMaxRenderDelayMs)
             ? kMinRenderDelayMs
             : render_delay;
}
}  // namespace

VideoRenderFrames::VideoRenderFrames(uint32_t render_delay_ms)
    : render_delay_ms_(EnsureValidRenderDelay(render_delay_ms)) {}

int32_t VideoRenderFrames::AddFrame(VideoFrame&& new_frame) {
  const int64_t time_now = cloopenwebrtc::TimeMillis();

  // Drop old frames only when there are other frames in the queue, otherwise, a
  // really slow system never renders any frames.
  if (!incoming_frames_.empty() &&
      new_frame.render_time_ms() + kOldRenderTimestampMS < time_now) {
    WEBRTC_TRACE(kTraceWarning,
                 kTraceVideoRenderer,
                 -1,
                 "%s: too old frame, timestamp=%u.",
                 __FUNCTION__,
                 new_frame.timestamp());
    return -1;
  }

  if (new_frame.render_time_ms() > time_now + kFutureRenderTimestampMS) {
    WEBRTC_TRACE(kTraceWarning, kTraceVideoRenderer, -1,
                 "%s: frame too long into the future, timestamp=%u.",
                 __FUNCTION__, new_frame.timestamp());
    return -1;
  }

  if (new_frame.render_time_ms() < last_render_time_ms_) {
    WEBRTC_TRACE(kTraceWarning, kTraceVideoRenderer, -1,
                 "%s: frame scheduled out of order, render_time=%u, latest=%u.",
                 __FUNCTION__, new_frame.render_time_ms(),
                 last_render_time_ms_);
    // For more details, see bug:
    // https://bugs.chromium.org/p/webrtc/issues/detail?id=7253
    return -1;
  }

  last_render_time_ms_ = new_frame.render_time_ms();
  incoming_frames_.emplace_back(std::move(new_frame));

  if (incoming_frames_.size() > kMaxIncomingFramesBeforeLogged)
    LOG(LS_WARNING) << "Stored incoming frames: " << incoming_frames_.size();
  return static_cast<int32_t>(incoming_frames_.size());
}

cloopenwebrtc::Optional<VideoFrame> VideoRenderFrames::FrameToRender() {
  cloopenwebrtc::Optional<VideoFrame> render_frame;
  // Get the newest frame that can be released for rendering.
  while (!incoming_frames_.empty() && TimeToNextFrameRelease() <= 0) {
    render_frame =
		cloopenwebrtc::Optional<VideoFrame>(std::move(incoming_frames_.front()));
    incoming_frames_.pop_front();
  }
  return render_frame;
}

uint32_t VideoRenderFrames::TimeToNextFrameRelease() {
  if (incoming_frames_.empty()) {
    return kEventMaxWaitTimeMs;
  }
  const int64_t time_to_release = incoming_frames_.front().render_time_ms() -
                                  render_delay_ms_ -
                                  cloopenwebrtc::TimeMillis();
  return time_to_release < 0 ? 0u : static_cast<uint32_t>(time_to_release);
}

bool VideoRenderFrames::HasPendingFrames() const {
  return !incoming_frames_.empty();
}

}  // namespace cloopenwebrtc
