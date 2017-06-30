/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This file contains platform-specific typedefs and defines.

#ifndef WEBRTC_TYPEDEFS_H_
#define WEBRTC_TYPEDEFS_H_

// Reserved words definitions
// TODO(andrew): Look at removing these.
#define WEBRTC_EXTERN extern
#define G_CONST const
#define WEBRTC_INLINE extern __inline

// Derived from Chromium's build/build_config.h
// Processor architecture detection.  For more info on what's defined, see:
//   http://msdn.microsoft.com/en-us/library/b0084kay.aspx
//   http://www.agner.org/optimize/calling_conventions.pdf
//   or with gcc, run: "echo | gcc -E -dM -"
// TODO(andrew): replace WEBRTC_LITTLE_ENDIAN with WEBRTC_ARCH_LITTLE_ENDIAN.
#if defined(_M_X64) || defined(__x86_64__)
#define WEBRTC_ARCH_X86_FAMILY
#define WEBRTC_ARCH_X86_64
#define WEBRTC_ARCH_64_BITS
#define WEBRTC_ARCH_LITTLE_ENDIAN
#define WEBRTC_LITTLE_ENDIAN
#elif defined(_M_IX86) || defined(__i386__)
#define WEBRTC_ARCH_X86_FAMILY
#define WEBRTC_ARCH_X86
#define WEBRTC_ARCH_32_BITS
#define WEBRTC_ARCH_LITTLE_ENDIAN
#define WEBRTC_LITTLE_ENDIAN
#elif defined(__ARMEL__)
// TODO(andrew): We'd prefer to control platform defines here, but this is
// currently provided by the Android makefiles. Commented to avoid duplicate
// definition warnings.
//#define WEBRTC_ARCH_ARM
// TODO(andrew): Chromium uses the following two defines. Should we switch?
//#define WEBRTC_ARCH_ARM_FAMILY
//#define WEBRTC_ARCH_ARMEL
#define WEBRTC_ARCH_32_BITS
#define WEBRTC_ARCH_LITTLE_ENDIAN
#define WEBRTC_LITTLE_ENDIAN
#elif defined(__arm64__)
#define WEBRTC_ARCH_64_BITS
#define WEBRTC_ARCH_LITTLE_ENDIAN
#define WEBRTC_LITTLE_ENDIAN
#else
#error Please add support for your architecture in typedefs.h
#endif

#if (defined(WEBRTC_ARCH_X86_FAMILY) && !defined(__SSE2__)) ||  \
	(defined(WEBRTC_ARCH_ARM_V7) && !defined(WEBRTC_ARCH_ARM_NEON))
#define WEBRTC_CPU_DETECTION
#endif

#if defined(__SSE2__) || defined(_MSC_VER)
#define WEBRTC_USE_SSE2
#endif

#if !(defined(_MSC_VER) && (_MSC_VER < 1600))
#include <stdint.h>
#else
// Define C99 equivalent types, since MSVC doesn't provide stdint.h.
#ifndef SRTP_H
typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef __int64             int64_t;
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned __int64    uint64_t;
#endif
#endif

// TODO(andrew): remove WebRtc_ types:
// http://code.google.com/p/webrtc/issues/detail?id=314
typedef int8_t              WebRtc_Word8;
typedef int16_t             WebRtc_Word16;
typedef int32_t             WebRtc_Word32;
typedef int64_t             WebRtc_Word64;
typedef uint8_t             WebRtc_UWord8;
typedef uint16_t            WebRtc_UWord16;
typedef uint32_t            WebRtc_UWord32;
typedef uint64_t            WebRtc_UWord64;

// Borrowed from Chromium's base/compiler_specific.h.
// Annotate a virtual method indicating it must be overriding a virtual
// method in the parent class.
// Use like:
//   virtual void foo() OVERRIDE;
#if defined(_MSC_VER)
#define OVERRIDE override
#elif defined(__clang__)
// Clang defaults to C++03 and warns about using override. Squelch that.
// Intentionally no push/pop here so all users of OVERRIDE ignore the warning
// too. This is like passing -Wno-c++11-extensions, except that GCC won't die
// (because it won't see this pragma).
#pragma clang diagnostic ignored "-Wc++11-extensions"
#define OVERRIDE override
#elif defined(__GNUC__) && __cplusplus >= 201103 && \
	(__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= 40700
// GCC 4.7 supports explicit virtual overrides when C++11 support is enabled.
#define OVERRIDE override
#else
#define OVERRIDE
#endif

// Annotate a function indicating the caller must examine the return value.
// Use like:
//   int foo() WARN_UNUSED_RESULT;
// TODO(ajm): Hack to avoid multiple definitions until the base/ of webrtc and
// libjingle are merged.
#if !defined(WARN_UNUSED_RESULT)
#if defined(__GNUC__)
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define WARN_UNUSED_RESULT
#endif
#endif  // WARN_UNUSED_RESULT

// Macro to be used for switch-case fallthrough (required for enabling
// -Wimplicit-fallthrough warning on Clang).
#ifndef FALLTHROUGH
#if defined(__clang__)
#define FALLTHROUGH() [[clang::fallthrough]]
#else
#define FALLTHROUGH() do { } while (0)
#endif
#endif

// Annotate a function that will not return control flow to the caller.
#if defined(_MSC_VER)
#define NO_RETURN __declspec(noreturn)
#elif defined(__GNUC__)
#define NO_RETURN __attribute__((noreturn))
#else
#define NO_RETURN
#endif

// Compiler detection.
#if defined(__GNUC__)
#define COMPILER_GCC 1
#elif defined(_MSC_VER)
#define COMPILER_MSVC 1
#else
#error Please add support for your compiler in build/build_config.h
#endif

#endif  // WEBRTC_TYPEDEFS_H_
