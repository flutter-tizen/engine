// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "logger.h"

#ifndef __X64_SHELL__
#include <dlog.h>
#endif
#include <unistd.h>

#include <cstdlib>
#include <iostream>

namespace flutter {

void* Logger::Redirect(void* arg) {
  int* pipe = static_cast<int*>(arg);
  ssize_t size;
  char buffer[1024];

  while ((size = read(pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
    buffer[size] = 0;
    Print(pipe == stdout_pipe_ ? kLogLevelInfo : kLogLevelError,
          std::string(buffer));
  }

  close(pipe[0]);
  close(pipe[1]);

  return nullptr;
}

void Logger::Start() {
  if (started_) {
    FT_LOG(Info) << "The threads have already started.";
    return;
  }
  if (pipe(stdout_pipe_) < 0 || pipe(stderr_pipe_) < 0) {
    FT_LOG(Error) << "Failed to create pipes.";
    return;
  }
  if (dup2(stdout_pipe_[1], 1) < 0 || dup2(stderr_pipe_[1], 2) < 0) {
    FT_LOG(Error) << "Failed to duplicate file descriptors.";
    return;
  }
  if (pthread_create(&stdout_thread_, 0, Redirect, stdout_pipe_) != 0 ||
      pthread_create(&stderr_thread_, 0, Redirect, stderr_pipe_) != 0) {
    FT_LOG(Error) << "Failed to create threads.";
    return;
  }
  if (pthread_detach(stdout_thread_) != 0 ||
      pthread_detach(stderr_thread_) != 0) {
    FT_LOG(Warn) << "Failed to detach threads.";
  }
  started_ = true;
}

int Logger::GetLoggingLevel() {
  return logging_level_;
}

void Logger::SetLoggingLevel(int level) {
  logging_level_ = level;
}

void Logger::Print(int level, std::string message) {
#ifdef __X64_SHELL__
  std::cerr << message << std::endl;
  std::cerr.flush();
#else
  log_priority priority;
  if (level == kLogLevelDebug) {
    priority = DLOG_DEBUG;
  } else if (level == kLogLevelInfo) {
    priority = DLOG_INFO;
  } else if (level == kLogLevelWarn) {
    priority = DLOG_WARN;
  } else if (level == kLogLevelError) {
    priority = DLOG_ERROR;
  } else if (level == kLogLevelFatal) {
    priority = DLOG_FATAL;
  } else {
    priority = DLOG_INFO;
  }
#ifdef TV_PROFILE
  // LOG_ID_MAIN must be used to display logs properly on TV devices.
  // Note: dlog_print(...) is an alias of __dlog_print(LOG_ID_APPS, ...).
  __dlog_print(LOG_ID_MAIN, priority, "ConsoleMessage", "%s", message.c_str());
#else
  dlog_print(priority, "ConsoleMessage", "%s", message.c_str());
#endif
#endif  // __X64_SHELL__
}

LogMessage::LogMessage(int level,
                       const char* file,
                       const char* function,
                       int line)
    : level_(level), file_(file), function_(function), line_(line) {
  stream_ << file_ << ": " << function_ << "(" << line_ << ") > ";
}

LogMessage::~LogMessage() {
  if (level_ < Logger::GetLoggingLevel()) {
    return;
  }
  Logger::Print(level_, stream_.str());

  if (level_ >= kLogLevelFatal) {
    abort();
  }
}

}  // namespace flutter
