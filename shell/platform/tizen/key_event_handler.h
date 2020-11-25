// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_KEY_EVENT_HANDLER_H_
#define EMBEDDER_KEY_EVENT_HANDLER_H_

#include <Ecore_Input.h>

#include <vector>

class TizenEmbedderEngine;

class KeyEventHandler {
 public:
  explicit KeyEventHandler(TizenEmbedderEngine* engine);
  virtual ~KeyEventHandler();

 private:
  TizenEmbedderEngine* engine_;
  std::vector<Ecore_Event_Handler*> key_event_handlers_;

  static Eina_Bool OnKey(void* data, int type, void* event);
};

#endif  // EMBEDDER_KEY_EVENT_HANDLER_H_
