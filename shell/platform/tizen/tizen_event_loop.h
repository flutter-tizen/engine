// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TIZEN_EVENT_LOOP_H_
#define TIZEN_EVENT_LOOP_H_

#include <Ecore.h>

#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include "flutter/shell/platform/embedder/embedder.h"

class TizenEventLoop {
 public:
  using TaskExpiredCallback = std::function<void(const FlutterTask*)>;
  TizenEventLoop(std::thread::id main_thread_id,
                 TaskExpiredCallback on_task_expired);
  ~TizenEventLoop();
  bool RunsTasksOnCurrentThread() const;

  void ExcuteTaskEvents(
      std::chrono::nanoseconds max_wait = std::chrono::nanoseconds::max());

  // Post a Flutter engine tasks to the event loop for delayed execution.
  void PostTask(FlutterTask flutter_task, uint64_t flutter_target_time_nanos);

 private:
  using TaskTimePoint = std::chrono::steady_clock::time_point;
  struct Task {
    uint64_t order;
    TaskTimePoint fire_time;
    FlutterTask task;

    struct Comparer {
      bool operator()(const Task& a, const Task& b) {
        if (a.fire_time == b.fire_time) {
          return a.order > b.order;
        }
        return a.fire_time > b.fire_time;
      }
    };
  };
  std::thread::id main_thread_id_;
  TaskExpiredCallback on_task_expired_;
  std::mutex task_queue_mutex_;
  std::priority_queue<Task, std::deque<Task>, Task::Comparer> task_queue_;
  std::condition_variable task_queue_cv_;
  Ecore_Pipe* ecore_pipe_;

  TizenEventLoop(const TizenEventLoop&) = delete;

  TizenEventLoop& operator=(const TizenEventLoop&) = delete;

  static void ExcuteTaskEvents(void* data, void* buffer, unsigned int nbyte);
  static Eina_Bool TaskTimerCallback(void* data);

  static TaskTimePoint TimePointFromFlutterTime(
      uint64_t flutter_target_time_nanos);
};

#endif  // TIZEN_EVENT_LOOP_H_
