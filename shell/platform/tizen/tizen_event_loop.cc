// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_event_loop.h"

#include <atomic>
#include <utility>

#ifdef TIZEN_RENDERER_EVAS_GL
#include "flutter/shell/platform/tizen/tizen_renderer_evas_gl.h"
#endif

namespace flutter {

TizenEventLoop::TizenEventLoop(std::thread::id main_thread_id,
                               CurrentTimeProc get_current_time,
                               TaskExpiredCallback on_task_expired)
    : main_thread_id_(main_thread_id),
      get_current_time_(get_current_time),
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
  {
    std::lock_guard<std::mutex> lock1(task_queue_mutex_);
    std::lock_guard<std::mutex> lock2(expired_tasks_mutex_);
    while (!task_queue_.empty()) {
      const auto& top = task_queue_.top();

      if (top.fire_time > now) {
        break;
      }

      expired_tasks_.push_back(task_queue_.top().task);
      task_queue_.pop();
    }
  }
  OnTaskExpired();
}

TizenEventLoop::TaskTimePoint TizenEventLoop::TimePointFromFlutterTime(
    uint64_t flutter_target_time_nanos) {
  const auto now = TaskTimePoint::clock::now();
  const int64_t flutter_duration =
      flutter_target_time_nanos - get_current_time_();
  return now + std::chrono::nanoseconds(flutter_duration);
}

void TizenEventLoop::PostTask(FlutterTask flutter_task,
                              uint64_t flutter_target_time_nanos) {
  Task task;
  task.order = ++task_order_;
  task.fire_time = TimePointFromFlutterTime(flutter_target_time_nanos);
  task.task = flutter_task;
  if (ecore_pipe_) {
    ecore_pipe_write(ecore_pipe_, &task, sizeof(task));
  }
}

Eina_Bool TizenEventLoop::TaskTimerCallback(void* data) {
  auto* self = reinterpret_cast<TizenEventLoop*>(data);
  self->ExcuteTaskEvents();
  return EINA_FALSE;
}

void TizenEventLoop::ExcuteTaskEvents(void* data,
                                      void* buffer,
                                      unsigned int nbyte) {
  auto* self = reinterpret_cast<TizenEventLoop*>(data);
  auto* p_task = reinterpret_cast<Task*>(buffer);

  const double flutter_duration =
      (static_cast<double>(p_task->fire_time.time_since_epoch().count()) -
       self->get_current_time_()) /
      1000000000.0;
  if (flutter_duration > 0) {
    {
      std::lock_guard<std::mutex> lock(self->task_queue_mutex_);
      self->task_queue_.push(*p_task);
    }
    ecore_timer_add(flutter_duration, TaskTimerCallback, self);
  } else {
    {
      std::lock_guard<std::mutex> lock(self->expired_tasks_mutex_);
      self->expired_tasks_.push_back(p_task->task);
    }
    self->OnTaskExpired();
  }
}

TizenPlatformEventLoop::TizenPlatformEventLoop(
    std::thread::id main_thread_id,
    CurrentTimeProc get_current_time,
    TaskExpiredCallback on_task_expired)
    : TizenEventLoop(main_thread_id, get_current_time, on_task_expired) {}

TizenPlatformEventLoop::~TizenPlatformEventLoop() {}

void TizenPlatformEventLoop::OnTaskExpired() {
  for (const auto& task : expired_tasks_) {
    on_task_expired_(&task);
  }
  expired_tasks_.clear();
}

#ifdef TIZEN_RENDERER_EVAS_GL
TizenRenderEventLoop::TizenRenderEventLoop(std::thread::id main_thread_id,
                                           CurrentTimeProc get_current_time,
                                           TaskExpiredCallback on_task_expired,
                                           TizenRenderer* renderer)
    : TizenEventLoop(main_thread_id, get_current_time, on_task_expired),
      renderer_(renderer) {
  evas_object_image_pixels_get_callback_set(
      static_cast<TizenRendererEvasGL*>(renderer_)->GetImageHandle(),
      [](void* data, Evas_Object* o) {  // Render callback
        TizenRenderEventLoop* self = static_cast<TizenRenderEventLoop*>(data);
        {
          std::lock_guard<std::mutex> lock(self->expired_tasks_mutex_);
          for (const auto& task : self->expired_tasks_) {
            self->on_task_expired_(&task);
          }
          self->expired_tasks_.clear();
        }
        self->has_pending_renderer_callback_ = false;
      },
      this);
}

TizenRenderEventLoop::~TizenRenderEventLoop() {}

void TizenRenderEventLoop::OnTaskExpired() {
  size_t expired_tasks_count = 0;
  std::lock_guard<std::mutex> lock(expired_tasks_mutex_);
  expired_tasks_count = expired_tasks_.size();
  if (!has_pending_renderer_callback_ && expired_tasks_count) {
    evas_object_image_pixels_dirty_set(
        static_cast<TizenRendererEvasGL*>(renderer_)->GetImageHandle(),
        EINA_TRUE);
    has_pending_renderer_callback_ = true;
  } else {
    // Do nothing
  }
}
#endif  // TIZEN_RENDERER_EVAS_GL

}  // namespace flutter
