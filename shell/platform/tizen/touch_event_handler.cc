// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "touch_event_handler.h"

#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/tizen_embedder_engine.h"

static const int DIRECTION_VERTICAL = 0;
static const int DIRECTION_HORIZONTAL = 1;

TouchEventHandler::TouchEventHandler(TizenEmbedderEngine *engine)
    : engine_(engine) {
  touch_event_handlers_.push_back(
      ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, OnTouch, this));
  touch_event_handlers_.push_back(
      ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP, OnTouch, this));
  touch_event_handlers_.push_back(
      ecore_event_handler_add(ECORE_EVENT_MOUSE_WHEEL, OnTouch, this));
  touch_event_handlers_.push_back(
      ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE, OnTouch, this));
  touch_event_handlers_.push_back(ecore_event_handler_add(
      ECORE_WL2_EVENT_WINDOW_VISIBILITY_CHANGE, OnTouch, this));
}

TouchEventHandler::~TouchEventHandler() {
  for (auto handler : touch_event_handlers_) {
    ecore_event_handler_del(handler);
  }
  touch_event_handlers_.clear();
}

void TouchEventHandler::SendFlutterPointerEvent(FlutterPointerPhase phase,
                                                double x, double y,
                                                double scroll_delta_x,
                                                double scroll_delta_y,
                                                size_t timestamp) {
  if (!engine_->flutter_engine) {
    return;
  }

  // Correct errors caused by window rotation.
  auto window_geometry = engine_->tizen_native_window->GetGeometry();
  double width = window_geometry.w;
  double height = window_geometry.h;
  double new_x = x, new_y = y;
  if (rotation == 90) {
    new_x = height - y;
    new_y = x;
  } else if (rotation == 180) {
    new_x = width - x;
    new_y = height - y;
  } else if (rotation == 270) {
    new_x = y;
    new_y = width - x;
  }

  FlutterPointerEvent event = {};
  event.struct_size = sizeof(event);
  event.phase = phase;
  event.x = new_x;
  event.y = new_y;
  if (scroll_delta_x != 0 || scroll_delta_y != 0) {
    event.signal_kind = kFlutterPointerSignalKindScroll;
  }
  event.scroll_delta_x = scroll_delta_x;
  event.scroll_delta_y = scroll_delta_y;
  event.timestamp = timestamp / 1000;
  FlutterEngineSendPointerEvent(engine_->flutter_engine, &event, 1);
}

Eina_Bool TouchEventHandler::OnTouch(void *data, int type, void *event) {
  auto *self = reinterpret_cast<TouchEventHandler *>(data);

  if (type == ECORE_EVENT_MOUSE_BUTTON_DOWN) {
    self->pointer_state_ = true;
    auto *button_event = reinterpret_cast<Ecore_Event_Mouse_Button *>(event);
    self->SendFlutterPointerEvent(kDown, button_event->x, button_event->y, 0, 0,
                                  button_event->timestamp);
  } else if (type == ECORE_EVENT_MOUSE_BUTTON_UP) {
    self->pointer_state_ = false;
    auto *button_event = reinterpret_cast<Ecore_Event_Mouse_Button *>(event);
    self->SendFlutterPointerEvent(kUp, button_event->x, button_event->y, 0, 0,
                                  button_event->timestamp);
  } else if (type == ECORE_EVENT_MOUSE_MOVE) {
    if (self->pointer_state_) {
      auto *move_event = reinterpret_cast<Ecore_Event_Mouse_Move *>(event);
      self->SendFlutterPointerEvent(kMove, move_event->x, move_event->y, 0, 0,
                                    move_event->timestamp);
    }
  } else if (type == ECORE_WL2_EVENT_WINDOW_VISIBILITY_CHANGE) {
    auto *focus_event =
        reinterpret_cast<Ecore_Wl2_Event_Window_Visibility_Change *>(event);
    LoggerD("Visibility changed: %u, %d", focus_event->win,
            focus_event->fully_obscured);
  } else if (type == ECORE_EVENT_MOUSE_WHEEL) {
    auto *wheel_event = reinterpret_cast<Ecore_Event_Mouse_Wheel *>(event);
    double scroll_delta_x = 0.0, scroll_delta_y = 0.0;
    if (wheel_event->direction == DIRECTION_VERTICAL) {
      scroll_delta_y += wheel_event->z;
    } else if (wheel_event->direction == DIRECTION_HORIZONTAL) {
      scroll_delta_x += wheel_event->z;
    }
    const int kScrollOffsetMultiplier = 20;
    scroll_delta_x *= kScrollOffsetMultiplier;
    scroll_delta_y *= kScrollOffsetMultiplier;
    self->SendFlutterPointerEvent(
        self->pointer_state_ ? kMove : kHover, wheel_event->x, wheel_event->y,
        scroll_delta_x, scroll_delta_y, wheel_event->timestamp);
  }
  return ECORE_CALLBACK_PASS_ON;
}
