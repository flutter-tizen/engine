// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include "tizen_log.h"

int dlog_print(log_priority prio, const char* tag, const char* fmt, ...) {
  va_list arglist;
  va_start(arglist, fmt);
  vprintf(fmt, arglist);
  va_end(arglist);
  printf("\n");
  return 0;
}

namespace flutter {

void SetMinLoggingLevel(log_priority p){};

log_priority GetMinLoggingLevel() {
  return DLOG_ERROR;
};

void StartLogging() {}

}  // namespace flutter
