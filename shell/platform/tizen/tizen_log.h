// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_LOG_H_
#define EMBEDDER_TIZEN_LOG_H_

#include <dlog.h>

#include <cassert>
#include <cstdlib>

namespace flutter {

// Starts logging threads which constantly redirect stdout/stderr to dlog.
// The threads can be started only once per process.
void StartLogging();

// Handles filtering of logs.
void SetMinLoggingLevel(log_priority p);
log_priority GetMinLoggingLevel();

#ifdef LOG_TAG
#undef LOG_TAG
#endif
// This is the only valid log tag that TV devices can understand.
#define LOG_TAG "ConsoleMessage"

#undef __LOG
#ifdef TV_PROFILE
// dlog_print() cannot be used because it implicitly passes LOG_ID_APPS as
// a log id, which is ignored by TV devices. Instead, an internal function
// __dlog_print() that takes a log id as a parameter is used.
#define __LOG(prio, fmt, args...) \
  __dlog_print(LOG_ID_MAIN, prio, LOG_TAG, fmt, ##args)
#else
#define __LOG(prio, fmt, args...) dlog_print(prio, LOG_TAG, fmt, ##args)
#endif

#define __FT_LOG(prio, fmt, args...)                                   \
  do {                                                                 \
    if (prio >= flutter::GetMinLoggingLevel()) {                       \
      __LOG(prio, "%s: %s(%d) > " fmt, __MODULE__, __func__, __LINE__, \
            ##args);                                                   \
    }                                                                  \
  } while (0)

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
  } while (0)

#define FT_RELEASE_ASSERT_NOT_REACHED()    \
  do {                                     \
    FT_LOGE("RELEASE_ASSERT_NOT_REACHED"); \
    abort();                               \
  } while (0)

/* COMPILE_ASSERT */
#ifndef FT_COMPILE_ASSERT
#define FT_COMPILE_ASSERT(exp, name) static_assert((exp), #name)
#endif

#define FT_UNIMPLEMENTED() FT_LOGW("UNIMPLEMENTED!")

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_LOG_H_
