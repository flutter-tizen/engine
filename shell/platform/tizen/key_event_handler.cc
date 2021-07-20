// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "key_event_handler.h"

#ifndef __X64_SHELL__
#include <app.h>
#endif

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

namespace {

constexpr char kBackKey[] = "XF86Back";
constexpr char kExitKey[] = "XF86Exit";

}  // namespace

KeyEventHandler::KeyEventHandler(FlutterTizenEngine* engine) : engine_(engine) {
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

Eina_Bool KeyEventHandler::OnKey(void* data, int type, void* event) {
  auto* self = reinterpret_cast<KeyEventHandler*>(data);
  auto* key = reinterpret_cast<Ecore_Event_Key*>(event);
  auto* engine = self->engine_;
  auto is_down = type == ECORE_EVENT_KEY_DOWN;

  if (is_down) {
    FT_LOG(Info) << "Key pressed: " << key->key << "(" << key->keycode << ")";
  }

  if (engine->text_input_channel) {
    if (engine->text_input_channel->SendKeyEvent(key, is_down)) {
      return ECORE_CALLBACK_PASS_ON;
    }
  }

  if (engine->platform_view_channel) {
    engine->platform_view_channel->SendKeyEvent(key, is_down);
  }

  if (engine->key_event_channel) {
    engine->key_event_channel->SendKeyEvent(
        key, is_down,
        [engine, keyname = std::string(key->keyname), is_down](bool handled) {
          if (handled) {
            return;
          }
          if (keyname == kBackKey && !is_down) {
            if (engine->navigation_channel) {
              engine->navigation_channel->PopRoute();
            }
          } else if (keyname == kExitKey && !is_down) {
#ifndef __X64_SHELL__
            ui_app_exit();
#endif
          }
        });
  }
  return ECORE_CALLBACK_PASS_ON;
}

}  // namespace flutter
