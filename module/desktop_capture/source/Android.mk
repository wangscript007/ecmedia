# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../../android-webrtc.mk

LOCAL_ARM_MODE := arm
LOCAL_MODULE := libwebrtc_deskop_capture
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
		window_capturer_android.cc \
		desktop_region.cc \
		shared_memory.cc \
		desktop_frame.cc

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
    

LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)/. \
		$(LOCAL_PATH)/../.. \
		$(LOCAL_PATH)/../interface \
		$(LOCAL_PATH)/../../../system_wrappers/interface \
		$(LOCAL_PATH)/../../../base
		
    
LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libstlport

ifndef NDK_ROOT
include external/stlport/libstlport.mk
endif
include $(BUILD_STATIC_LIBRARY)

