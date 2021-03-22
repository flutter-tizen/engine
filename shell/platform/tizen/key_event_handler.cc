// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "key_event_handler.h"

#include "flutter/shell/platform/tizen/tizen_embedder_engine.h"

static constexpr char kPlatformBackButtonName[] = "XF86Back";

KeyEventHandler::KeyEventHandler(TizenEmbedderEngine *engine)
    : engine_(engine) {
  key_event_handlers_.push_back(
      ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, OnKey, this));
  key_event_handlers_.push_back(
      ecore_event_handler_add(ECORE_EVENT_KEY_UP, OnKey, this));
}

KeyEventHandler::~KeyEventHandler() {
  for (auto handler : key_event_handlers_) {
    ecore_event_handler_del(handler);
  }
  key_event_handlers_.clear();
}

Eina_Bool KeyEventHandler::OnKey(void *data, int type, void *event) {
  auto *self = reinterpret_cast<KeyEventHandler *>(data);
  auto *key = reinterpret_cast<Ecore_Event_Key *>(event);
  auto *engine = self->engine_;
  auto is_down = type == ECORE_EVENT_KEY_DOWN;

  if (strcmp(key->keyname, kPlatformBackButtonName) == 0) {
    // The device back button was pressed.
    if (engine->navigation_channel && !is_down) {
      engine->navigation_channel->PopRoute();
    }
  } else {
    if (engine->text_input_channel) {
      if (is_down) {
        engine->text_input_channel->OnKeyDown(key);
      }
      if (engine->text_input_channel->IsSoftwareKeyboardShowing()) {
        return ECORE_CALLBACK_PASS_ON;
      }
    }
    if (engine->key_event_channel) {
      engine->key_event_channel->SendKeyEvent(key, is_down);
    }
  }
  return ECORE_CALLBACK_PASS_ON;
}
