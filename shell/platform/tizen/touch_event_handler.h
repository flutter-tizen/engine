// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TOUCH_EVENT_HANDLER_H_
#define EMBEDDER_TOUCH_EVENT_HANDLER_H_

#define EFL_BETA_API_SUPPORT
#include <Ecore_Input.h>
#include <Ecore_Wl2.h>

#include <vector>

#include "flutter/shell/platform/embedder/embedder.h"

class TizenEmbedderEngine;

class TouchEventHandler {
 public:
  explicit TouchEventHandler(TizenEmbedderEngine* engine);
  virtual ~TouchEventHandler();

  int32_t rotation = 0;

 private:
  TizenEmbedderEngine* engine_;
  std::vector<Ecore_Event_Handler*> touch_event_handlers_;
  bool pointer_state_ = false;

  void SendFlutterPointerEvent(FlutterPointerPhase phase, double x, double y,
                               size_t timestamp);

  static Eina_Bool OnTouch(void* data, int type, void* event);
};

#endif  // EMBEDDER_TOUCH_EVENT_HANDLER_H_
