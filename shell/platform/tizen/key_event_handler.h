// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_KEY_EVENT_HANDLER_H_
#define EMBEDDER_KEY_EVENT_HANDLER_H_

#include <Ecore_Input.h>

#include <vector>

namespace flutter {

class FlutterTizenEngine;

class KeyEventHandler {
 public:
  explicit KeyEventHandler(FlutterTizenEngine* engine);
  virtual ~KeyEventHandler();

 private:
  FlutterTizenEngine* engine_;
  std::vector<Ecore_Event_Handler*> key_event_handlers_;

  static Eina_Bool OnKey(void* data, int type, void* event);
};

}  // namespace flutter

#endif  // EMBEDDER_KEY_EVENT_HANDLER_H_
