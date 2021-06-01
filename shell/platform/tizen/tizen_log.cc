// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_log.h"

#include <pthread.h>
#include <unistd.h>

static int stdout_pipe[2];
static int stderr_pipe[2];
static pthread_t stdout_thread;
static pthread_t stderr_thread;
static bool is_running = false;

static void* LoggingFunction(void* arg) {
  int* pipe = static_cast<int*>(arg);
  auto priority = pipe == stdout_pipe ? DLOG_INFO : DLOG_ERROR;

  ssize_t size;
  char buffer[1024];

  while ((size = read(pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
    buffer[size] = 0;
    __LOG(priority, "%s", buffer);
  }

  close(pipe[0]);
  close(pipe[1]);

  return nullptr;
}

void StartLogging() {
  if (is_running) {
    FT_LOGD("The threads are already running.");
    return;
  }

  if (pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0) {
    FT_LOGE("Failed to create pipes.");
    return;
  }

  if (dup2(stdout_pipe[1], 1) < 0 || dup2(stderr_pipe[1], 2) < 0) {
    FT_LOGE("Failed to duplicate file descriptors.");
    return;
  }

  if (pthread_create(&stdout_thread, 0, LoggingFunction, stdout_pipe) != 0 ||
      pthread_create(&stderr_thread, 0, LoggingFunction, stderr_pipe) != 0) {
    FT_LOGE("Failed to create threads.");
    return;
  }

  if (pthread_detach(stdout_thread) != 0 ||
      pthread_detach(stderr_thread) != 0) {
    FT_LOGW("Failed to detach threads.");
  }
  is_running = true;
}
