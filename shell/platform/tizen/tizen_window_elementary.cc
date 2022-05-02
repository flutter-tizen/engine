// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_window_elementary.h"

#include "flutter/shell/platform/tizen/flutter_tizen_view.h"
#include "flutter/shell/platform/tizen/logger.h"

#include <efl_extension.h>
#include <ui/efl_util.h>

namespace {

static const int kScrollDirectionVertical = 0;
static const int kScrollDirectionHorizontal = 1;
static const int kScrollOffsetMultiplier = 20;

}  // namespace

namespace flutter {

TizenWindowElementary::TizenWindowElementary(Geometry geometry,
                                             bool transparent,
                                             bool focusable,
                                             bool top_level)
    : TizenWindow(geometry, transparent, focusable, top_level) {
  if (!CreateWindow()) {
    FT_LOG(Error) << "Failed to create a platform window.";
    return;
  }

  SetWindowOptions();
  RegisterEventHandlers();
  Show();
}

TizenWindowElementary::~TizenWindowElementary() {
  UnregisterEventHandlers();
  DestroyWindow();
}

bool TizenWindowElementary::CreateWindow() {
  elm_config_accel_preference_set("hw:opengl");

  elm_win_ = elm_win_add(nullptr, nullptr,
                         top_level_ ? ELM_WIN_NOTIFICATION : ELM_WIN_BASIC);
  if (!elm_win_) {
    FT_LOG(Error) << "Could not create an Evas window.";
    return false;
  }

  // Please uncomment below and enable setWindowGeometry of window channel when
  // Tizen 5.5 or later was chosen as default.
  // elm_win_aux_hint_add(elm_win_, "wm.policy.win.user.geometry", "1");

  Ecore_Evas* ecore_evas =
      ecore_evas_ecore_evas_get(evas_object_evas_get(elm_win_));

  int32_t width, height;
  ecore_evas_screen_geometry_get(ecore_evas, nullptr, nullptr, &width, &height);
  if (width == 0 || height == 0) {
    FT_LOG(Error) << "Invalid screen size: " << width << " x " << height;
    return false;
  }

  if (initial_geometry_.width == 0) {
    initial_geometry_.width = width;
  }
  if (initial_geometry_.height == 0) {
    initial_geometry_.height = height;
  }

  evas_object_move(elm_win_, initial_geometry_.left, initial_geometry_.top);
  evas_object_resize(elm_win_, initial_geometry_.width,
                     initial_geometry_.height);
  evas_object_raise(elm_win_);

  image_ = evas_object_image_filled_add(evas_object_evas_get(elm_win_));
  evas_object_resize(image_, initial_geometry_.width, initial_geometry_.height);
  evas_object_move(image_, initial_geometry_.left, initial_geometry_.top);
  evas_object_image_size_set(image_, initial_geometry_.width,
                             initial_geometry_.height);
  evas_object_image_alpha_set(image_, EINA_TRUE);
  elm_win_resize_object_add(elm_win_, image_);

  return elm_win_ && image_;
}

void TizenWindowElementary::DestroyWindow() {
  evas_object_del(elm_win_);
  evas_object_del(image_);
}

void TizenWindowElementary::SetWindowOptions() {
  if (top_level_) {
    efl_util_set_notification_window_level(elm_win_,
                                           EFL_UTIL_NOTIFICATION_LEVEL_TOP);
  }

  if (transparent_) {
    elm_win_alpha_set(elm_win_, EINA_TRUE);
  } else {
    elm_win_alpha_set(elm_win_, EINA_FALSE);

    Evas_Object* bg = elm_bg_add(elm_win_);
    evas_object_color_set(bg, 0, 0, 0, 0);

    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(elm_win_, bg);
  }

  elm_win_indicator_mode_set(elm_win_, ELM_WIN_INDICATOR_SHOW);
  elm_win_indicator_opacity_set(elm_win_, ELM_WIN_INDICATOR_OPAQUE);

  // TODO: focusable_

  const int rotations[4] = {0, 90, 180, 270};
  elm_win_wm_rotation_available_rotations_set(elm_win_, &rotations[0], 4);
}

void TizenWindowElementary::RegisterEventHandlers() {
  rotatoin_changed_callback_ = [](void* data, Evas_Object* object,
                                  void* event_info) {
    auto* self = reinterpret_cast<TizenWindowElementary*>(data);
    if (self->view_) {
      if (self->elm_win_ == object) {
        // FIXME
        FT_UNIMPLEMENTED();
        self->view_->OnRotate(self->GetRotation());
        elm_win_wm_rotation_manual_rotation_done(self->elm_win_);
      }
    }
  };
  evas_object_smart_callback_add(elm_win_, "rotation,changed",
                                 rotatoin_changed_callback_, this);

  evas_object_callbacks_[EVAS_CALLBACK_MOUSE_DOWN] =
      [](void* data, Evas* evas, Evas_Object* object, void* event_info) {
        auto* self = reinterpret_cast<TizenWindowElementary*>(data);
        if (self->view_) {
          if (self->elm_win_ == object) {
            auto* mouse_event =
                reinterpret_cast<Evas_Event_Mouse_Down*>(event_info);
            self->view_->OnPointerDown(
                mouse_event->canvas.x, mouse_event->canvas.y,
                mouse_event->timestamp, kFlutterPointerDeviceKindTouch,
                mouse_event->button);
          }
        }
      };
  evas_object_event_callback_add(
      elm_win_, EVAS_CALLBACK_MOUSE_DOWN,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_DOWN], this);

  evas_object_callbacks_[EVAS_CALLBACK_MOUSE_UP] = [](void* data, Evas* evas,
                                                      Evas_Object* object,
                                                      void* event_info) {
    auto* self = reinterpret_cast<TizenWindowElementary*>(data);
    if (self->view_) {
      if (self->elm_win_ == object) {
        auto* mouse_event = reinterpret_cast<Evas_Event_Mouse_Up*>(event_info);
        self->view_->OnPointerUp(mouse_event->canvas.x, mouse_event->canvas.y,
                                 mouse_event->timestamp,
                                 kFlutterPointerDeviceKindTouch,
                                 mouse_event->button);
      }
    }
  };
  evas_object_event_callback_add(elm_win_, EVAS_CALLBACK_MOUSE_UP,
                                 evas_object_callbacks_[EVAS_CALLBACK_MOUSE_UP],
                                 this);

  evas_object_callbacks_[EVAS_CALLBACK_MOUSE_MOVE] =
      [](void* data, Evas* evas, Evas_Object* object, void* event_info) {
        auto* self = reinterpret_cast<TizenWindowElementary*>(data);
        if (self->view_) {
          if (self->elm_win_ == object) {
            auto* mouse_event =
                reinterpret_cast<Evas_Event_Mouse_Move*>(event_info);
            self->view_->OnPointerMove(
                mouse_event->cur.canvas.x, mouse_event->cur.canvas.y,
                mouse_event->timestamp, kFlutterPointerDeviceKindTouch,
                mouse_event->buttons);
          }
        }
      };
  evas_object_event_callback_add(
      elm_win_, EVAS_CALLBACK_MOUSE_MOVE,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_MOVE], this);

  evas_object_callbacks_[EVAS_CALLBACK_MOUSE_WHEEL] = [](void* data, Evas* evas,
                                                         Evas_Object* object,
                                                         void* event_info) {
    auto* self = reinterpret_cast<TizenWindowElementary*>(data);
    if (self->view_) {
      if (self->elm_win_ == object) {
        auto* wheel_event =
            reinterpret_cast<Ecore_Event_Mouse_Wheel*>(event_info);
        double delta_x = 0.0;
        double delta_y = 0.0;

        if (wheel_event->direction == kScrollDirectionVertical) {
          delta_y += wheel_event->z;
        } else if (wheel_event->direction == kScrollDirectionHorizontal) {
          delta_x += wheel_event->z;
        }

        self->view_->OnScroll(wheel_event->x, wheel_event->y, delta_x, delta_y,
                              kScrollOffsetMultiplier, wheel_event->timestamp,
                              kFlutterPointerDeviceKindTouch, 0);
      }
    }
  };
  evas_object_event_callback_add(
      elm_win_, EVAS_CALLBACK_MOUSE_WHEEL,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_WHEEL], this);

  // FIXME: ues EVAS_CALLBACK_KEY_DOWN, EVAS_CALLBACK_KEY_UP
  ecore_event_key_handlers_.push_back(ecore_event_handler_add(
      ECORE_EVENT_KEY_DOWN,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = reinterpret_cast<TizenWindowElementary*>(data);
        if (self->view_) {
          auto* key_event = reinterpret_cast<Ecore_Event_Key*>(event);
          if (key_event->window == self->GetWindowId()) {
            self->view_->OnKey(key_event, false);
            return ECORE_CALLBACK_DONE;
          }
        }
        return ECORE_CALLBACK_PASS_ON;
      },
      this));

  ecore_event_key_handlers_.push_back(ecore_event_handler_add(
      ECORE_EVENT_KEY_UP,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = reinterpret_cast<TizenWindowElementary*>(data);
        if (self->view_) {
          auto* key_event = reinterpret_cast<Ecore_Event_Key*>(event);
          if (key_event->window == self->GetWindowId()) {
            self->view_->OnKey(key_event, true);
            return ECORE_CALLBACK_DONE;
          }
        }
        return ECORE_CALLBACK_PASS_ON;
      },
      this));
}

void TizenWindowElementary::UnregisterEventHandlers() {
  evas_object_smart_callback_del(elm_win_, "rotation,changed",
                                 rotatoin_changed_callback_);

  evas_object_event_callback_del(
      elm_win_, EVAS_CALLBACK_MOUSE_DOWN,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_DOWN]);
  evas_object_event_callback_del(
      elm_win_, EVAS_CALLBACK_MOUSE_UP,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_UP]);
  evas_object_event_callback_del(
      elm_win_, EVAS_CALLBACK_MOUSE_MOVE,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_MOVE]);
  evas_object_event_callback_del(
      elm_win_, EVAS_CALLBACK_MOUSE_WHEEL,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_WHEEL]);

  for (Ecore_Event_Handler* handler : ecore_event_key_handlers_) {
    ecore_event_handler_del(handler);
  }
  ecore_event_key_handlers_.clear();
}

TizenWindow::Geometry TizenWindowElementary::GetWindowGeometry() {
  // FIXME : evas_object_geometry_get() and ecore_wl2_window_geometry_get() are
  // not equivalent.
  Geometry result;
  evas_object_geometry_get(elm_win_, &result.left, &result.top, &result.width,
                           &result.height);
  return result;
}

void TizenWindowElementary::SetWindowGeometry(Geometry geometry) {
  evas_object_resize(elm_win_, geometry.width, geometry.height);
  evas_object_move(elm_win_, geometry.left, geometry.top);

  evas_object_resize(image_, geometry.width, geometry.height);
  evas_object_move(image_, geometry.left, geometry.top);
}

TizenWindow::Geometry TizenWindowElementary::GetScreenGeometry() {
  Geometry result;
  Ecore_Evas* ecore_evas =
      ecore_evas_ecore_evas_get(evas_object_evas_get(elm_win_));
  ecore_evas_screen_geometry_get(ecore_evas, nullptr, nullptr, &result.width,
                                 &result.height);
  return result;
}

int32_t TizenWindowElementary::GetRotation() {
  return elm_win_rotation_get(elm_win_);
}

int32_t TizenWindowElementary::GetDpi() {
  Ecore_Evas* ecore_evas =
      ecore_evas_ecore_evas_get(evas_object_evas_get(elm_win_));
  int32_t xdpi, ydpi;
  ecore_evas_screen_dpi_get(ecore_evas, &xdpi, &ydpi);
  return xdpi;
}

uintptr_t TizenWindowElementary::GetWindowId() {
  return ecore_evas_window_get(
      ecore_evas_ecore_evas_get(evas_object_evas_get(elm_win_)));
}

void TizenWindowElementary::ResizeRenderTargetWithRotation(Geometry geometry,
                                                           int32_t angle) {
  TizenRendererEvasGL* renderer_evas_gl =
      reinterpret_cast<TizenRendererEvasGL*>(view_->engine()->renderer());
  renderer_evas_gl->ResizeSurface(geometry.width, geometry.height);
}

void TizenWindowElementary::SetPreferredOrientations(
    const std::vector<int>& rotations) {
  elm_win_wm_rotation_available_rotations_set(
      elm_win_, reinterpret_cast<const int*>(rotations.data()),
      rotations.size());
}

void TizenWindowElementary::BindKeys(const std::vector<std::string>& keys) {
  for (const std::string& key : keys) {
    eext_win_keygrab_set(elm_win_, key.c_str());
  }
}

void TizenWindowElementary::Show() {
  evas_object_show(image_);
  evas_object_show(elm_win_);
}

void TizenWindowElementary::OnGeometryChanged(Geometry geometry) {
  SetWindowGeometry(geometry);
  view_->OnResize(geometry.left, geometry.top, geometry.width, geometry.height);
}

}  // namespace flutter
