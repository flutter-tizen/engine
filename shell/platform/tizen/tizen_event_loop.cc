// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_event_loop.h"

#include <atomic>
#include <utility>

TizenEventLoop::TizenEventLoop(std::thread::id main_thread_id,
                               TaskExpiredCallback on_task_expired)
    : main_thread_id_(main_thread_id),
      on_task_expired_(std::move(on_task_expired)) {
  ecore_pipe_ = ecore_pipe_add(ExcuteTaskEvents, this);
}

TizenEventLoop::~TizenEventLoop() {
  if (ecore_pipe_) {
    ecore_pipe_del(ecore_pipe_);
  }
}

bool TizenEventLoop::RunsTasksOnCurrentThread() const {
  return std::this_thread::get_id() == main_thread_id_;
}

void TizenEventLoop::ExcuteTaskEvents(std::chrono::nanoseconds max_wait) {
  const auto now = TaskTimePoint::clock::now();
  std::vector<FlutterTask> expired_tasks;
  {
    std::lock_guard<std::mutex> lock(task_queue_mutex_);
    while (!task_queue_.empty()) {
      const auto& top = task_queue_.top();

      if (top.fire_time > now) {
        break;
      }
      expired_tasks.push_back(task_queue_.top().task);
      task_queue_.pop();
    }
  }
  for (const auto& task : expired_tasks) {
    on_task_expired_(&task);
  }
}

TizenEventLoop::TaskTimePoint TizenEventLoop::TimePointFromFlutterTime(
    uint64_t flutter_target_time_nanos) {
  const auto now = TaskTimePoint::clock::now();
  const int64_t flutter_duration =
      flutter_target_time_nanos - FlutterEngineGetCurrentTime();
  return now + std::chrono::nanoseconds(flutter_duration);
}

void TizenEventLoop::PostTask(FlutterTask flutter_task,
                              uint64_t flutter_target_time_nanos) {
  static std::atomic<std::uint64_t> sGlobalTaskOrder(0);
  Task task;
  task.order = ++sGlobalTaskOrder;
  task.fire_time = TimePointFromFlutterTime(flutter_target_time_nanos);
  task.task = flutter_task;
  if (ecore_pipe_) {
    ecore_pipe_write(ecore_pipe_, &task, sizeof(task));
  }
}

Eina_Bool TizenEventLoop::TaskTimerCallback(void* data) {
  TizenEventLoop* tizenEventLoop = reinterpret_cast<TizenEventLoop*>(data);
  tizenEventLoop->ExcuteTaskEvents();
  return EINA_FALSE;
}

void TizenEventLoop::ExcuteTaskEvents(void* data, void* buffer,
                                      unsigned int nbyte) {
  TizenEventLoop* tizenEventLoop = reinterpret_cast<TizenEventLoop*>(data);
  Task* p_task = reinterpret_cast<Task*>(buffer);

  const double flutter_duration =
      ((double)(p_task->fire_time.time_since_epoch().count()) -
       FlutterEngineGetCurrentTime()) /
      1000000000.0;
  if (flutter_duration > 0) {
    {
      std::lock_guard<std::mutex> lock(tizenEventLoop->task_queue_mutex_);
      tizenEventLoop->task_queue_.push(*p_task);
    }
    ecore_timer_add(flutter_duration, TaskTimerCallback, tizenEventLoop);
  } else {
    tizenEventLoop->on_task_expired_(&(p_task->task));
  }
}
