// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TOUCH_EVENT_HANDLER_H_
#define EMBEDDER_TOUCH_EVENT_HANDLER_H_

#include <Ecore_Input.h>
#ifdef __X64_SHELL__
#include <Efl_Core.h>
#endif

#include <vector>

#include "flutter/shell/platform/embedder/embedder.h"

namespace flutter {

class FlutterTizenEngine;

class TouchEventHandler {
 public:
  explicit TouchEventHandler(FlutterTizenEngine* engine);
  virtual ~TouchEventHandler();

  int32_t rotation = 0;

 private:
  FlutterTizenEngine* engine_;
  std::vector<Ecore_Event_Handler*> touch_event_handlers_;
  bool pointer_state_ = false;
  uintptr_t window_id_ = 0;

  void SendFlutterPointerEvent(FlutterPointerPhase phase,
                               double x,
                               double y,
                               double scroll_delta_x,
                               double scroll_delta_y,
                               size_t timestamp,
                               int device_id);

  static Eina_Bool OnTouch(void* data, int type, void* event);
};

}  // namespace flutter

#endif  // EMBEDDER_TOUCH_EVENT_HANDLER_H_
