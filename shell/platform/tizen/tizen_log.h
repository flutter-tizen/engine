// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_LOG_H_
#define EMBEDDER_TIZEN_LOG_H_
#include <dlog.h>

#include <cassert>
#include <cstdlib>
#include <string>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "ConsoleMessage"

#undef __LOG
#define __LOG(id, prio, tag, fmt, arg...)                              \
  __dlog_print(id, prio, tag, "%s: %s(%d) > " fmt, __FILE__, __func__, \
               __LINE__, ##arg);

#define __FT_LOG(prio, fmt, args...) \
  __LOG(LOG_ID_MAIN, prio, LOG_TAG, fmt, ##args)

#define FT_LOGD(fmt, args...) __FT_LOG(DLOG_DEBUG, fmt, ##args)
#define FT_LOGI(fmt, args...) __FT_LOG(DLOG_INFO, fmt, ##args)
#define FT_LOGW(fmt, args...) __FT_LOG(DLOG_WARN, fmt, ##args)
#define FT_LOGE(fmt, args...) __FT_LOG(DLOG_ERROR, fmt, ##args)

#if defined(NDEBUG)
#define FT_ASSERT(assertion) ((void)0)
#define FT_ASSERT_NOT_REACHED() ((void)0)
#define FT_ASSERT_STATIC(assertion, reason)
#else
#define FT_ASSERT(assertion) assert(assertion);
#define FT_ASSERT_NOT_REACHED() \
  do {                          \
    assert(false);              \
  } while (0)
#define FT_ASSERT_STATIC(assertion, reason) static_assert(assertion, reason)
#endif

#define FT_RELEASE_ASSERT(assertion) \
  do {                               \
    if (!(assertion)) {              \
      FT_LOGE("RELEASE_ASSERT");     \
      abort();                       \
    }                                \
  } while (0);

#define FT_RELEASE_ASSERT_NOT_REACHED()    \
  do {                                     \
    FT_LOGE("RELEASE_ASSERT_NOT_REACHED"); \
    abort();                               \
  } while (0)

/* COMPILE_ASSERT */
#ifndef FT_COMPILE_ASSERT
#define FT_COMPILE_ASSERT(exp, name) static_assert((exp), #name)
#endif

#define FT_LOGD_UNIMPLEMENTED() FT_LOGD("UNIMPLEMENTED")

#endif  // EMBEDDER_TIZEN_LOG_H_
