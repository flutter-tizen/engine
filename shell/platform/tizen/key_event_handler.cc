// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "key_event_handler.h"

#include <app.h>

#include <iostream>

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

namespace {

constexpr char kBackKey[] = "XF86Back";
constexpr char kExitKey[] = "XF86Exit";

// Keys that should always be handled by the app first but not by the system.
const std::vector<std::string> kBindableSystemKeys = {
    "XF86Menu",           "XF86Back",        "XF86AudioPlay",
    "XF86AudioPause",     "XF86AudioStop",   "XF86AudioNext",
    "XF86AudioPrev",      "XF86AudioRewind", "XF86AudioForward",
    "XF86AudioPlayPause", "XF86AudioRecord", "XF86LowerChannel",
    "XF86RaiseChannel",   "XF86ChannelList", "XF86PreviousChannel",
    "XF86SysMenu",        "XF86SimpleMenu",  "XF86History",
    "XF86Favorites",      "XF86Info",        "XF86Red",
    "XF86Green",          "XF86Yellow",      "XF86Blue",
    "XF86Subtitle",       "XF86PlayBack",    "XF86ChannelGuide",
    "XF86Caption",        "XF86Exit",
};

}  // namespace

KeyEventHandler::KeyEventHandler(FlutterTizenEngine* engine) : engine_(engine) {
  if (!engine->renderer() || !engine->renderer()->IsValid()) {
    return;
  }
  engine->renderer()->BindKeys(kBindableSystemKeys);

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

Eina_Bool KeyEventHandler::OnKey(void* data, int type, void* raw_event) {
  auto* self = reinterpret_cast<KeyEventHandler*>(data);
  auto* event = reinterpret_cast<Ecore_Event_Key*>(raw_event);
  auto* engine = self->engine_;
  bool is_down = type == ECORE_EVENT_KEY_DOWN;

  if (engine->renderer()->GetWindowId() != event->window) {
    return ECORE_CALLBACK_PASS_ON;
  }

  if (is_down) {
    FT_LOG(Info) << "Key symbol: " << event->key << ", code: 0x" << std::setw(8)
                 << std::setfill('0') << std::right << std::hex
                 << event->keycode;
  }

  if (engine->text_input_channel()) {
    if (engine->text_input_channel()->SendKeyEvent(event, is_down)) {
      return ECORE_CALLBACK_DONE;
    }
  }

  if (engine->platform_view_channel()) {
    engine->platform_view_channel()->SendKeyEvent(event, is_down);
  }

  if (engine->key_event_channel()) {
    engine->key_event_channel()->SendKeyEvent(
        event, is_down,
        [engine, symbol = std::string(event->key), is_down](bool handled) {
          if (handled) {
            return;
          }
          if (symbol == kBackKey && !is_down) {
            if (engine->navigation_channel()) {
              engine->navigation_channel()->PopRoute();
            }
          } else if (symbol == kExitKey && !is_down) {
            ui_app_exit();
          }
        });
  }
  return ECORE_CALLBACK_DONE;
}

}  // namespace flutter
