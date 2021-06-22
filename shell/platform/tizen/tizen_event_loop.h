// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_EVENT_LOOP_H_
#define EMBEDDER_TIZEN_EVENT_LOOP_H_

#include <Ecore.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include "flutter/shell/platform/embedder/embedder.h"

namespace flutter {

class TizenRenderer;

class TizenEventLoop {
 public:
  using TaskExpiredCallback = std::function<void(const FlutterTask*)>;
  TizenEventLoop(std::thread::id main_thread_id,
                 TaskExpiredCallback on_task_expired);
  virtual ~TizenEventLoop();
  bool RunsTasksOnCurrentThread() const;

  void ExcuteTaskEvents(
      std::chrono::nanoseconds max_wait = std::chrono::nanoseconds::max());

  // Post a Flutter engine tasks to the event loop for delayed execution.
  void PostTask(FlutterTask flutter_task, uint64_t flutter_target_time_nanos);

  virtual void OnTaskExpired() = 0;

 protected:
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
  std::vector<FlutterTask> expired_tasks_;
  std::mutex expired_tasks_mutex_;
  std::atomic<std::uint64_t> task_order_{0};

 private:
  Ecore_Pipe* ecore_pipe_;

  TizenEventLoop(const TizenEventLoop&) = delete;

  TizenEventLoop& operator=(const TizenEventLoop&) = delete;

  static void ExcuteTaskEvents(void* data, void* buffer, unsigned int nbyte);

  static Eina_Bool TaskTimerCallback(void* data);

  static TaskTimePoint TimePointFromFlutterTime(
      uint64_t flutter_target_time_nanos);
};

class TizenPlatformEventLoop : public TizenEventLoop {
 public:
  TizenPlatformEventLoop(std::thread::id main_thread_id,
                         TaskExpiredCallback on_task_expired);
  virtual ~TizenPlatformEventLoop();
  virtual void OnTaskExpired() override;
};

#ifdef TIZEN_RENDERER_EVAS_GL
class TizenRenderEventLoop : public TizenEventLoop {
 public:
  TizenRenderEventLoop(std::thread::id main_thread_id,
                       TaskExpiredCallback on_task_expired,
                       TizenRenderer* renderer);
  virtual ~TizenRenderEventLoop();
  virtual void OnTaskExpired() override;

 private:
  TizenRenderer* renderer_{nullptr};
  std::atomic_bool has_pending_renderer_callback_{false};
};
#endif

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_EVENT_LOOP_H_
