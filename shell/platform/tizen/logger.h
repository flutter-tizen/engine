// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_LOGGER_H_
#define EMBEDDER_LOGGER_H_
#include <dlog.h>

#include <string>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "ConsoleMessage"

#undef LOG_
#define LOG_(id, prio, tag, fmt, arg...)                               \
  __dlog_print(id, prio, tag, "%s: %s(%d) > " fmt, __FILE__, __func__, \
               __LINE__, ##arg);

#define _LOGGER_LOG(prio, fmt, args...) \
  LOG_(LOG_ID_MAIN, prio, LOG_TAG, fmt, ##args)

#define LoggerD(fmt, args...) _LOGGER_LOG(DLOG_DEBUG, fmt, ##args)
#define LoggerI(fmt, args...) _LOGGER_LOG(DLOG_INFO, fmt, ##args)
#define LoggerW(fmt, args...) _LOGGER_LOG(DLOG_WARN, fmt, ##args)
#define LoggerE(fmt, args...) _LOGGER_LOG(DLOG_ERROR, fmt, ##args)

#endif  // EMBEDDER_LOGGER_H_
