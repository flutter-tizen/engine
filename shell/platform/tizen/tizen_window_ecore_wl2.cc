// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_window_ecore_wl2.h"

#include "flutter/shell/platform/tizen/flutter_tizen_view.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace {

static const int kScrollDirectionVertical = 0;
static const int kScrollDirectionHorizontal = 1;
static const int kScrollOffsetMultiplier = 20;

}  // namespace

namespace flutter {

TizenWindowEcoreWl2::TizenWindowEcoreWl2(Geometry geometry,
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

TizenWindowEcoreWl2::~TizenWindowEcoreWl2() {
  UnregisterEventHandlers();
  DestroyWindow();
}

bool TizenWindowEcoreWl2::CreateWindow() {
  if (!ecore_wl2_init()) {
    FT_LOG(Error) << "Could not initialize Ecore Wl2.";
    return false;
  }

  ecore_wl2_display_ = ecore_wl2_display_connect(nullptr);
  if (!ecore_wl2_display_) {
    FT_LOG(Error) << "Ecore Wl2 display not found.";
    return false;
  }
  wl2_display_ = ecore_wl2_display_get(ecore_wl2_display_);

  ecore_wl2_sync();

  int32_t width, height;
  ecore_wl2_display_screen_size_get(ecore_wl2_display_, &width, &height);
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

  ecore_wl2_window_ = ecore_wl2_window_new(
      ecore_wl2_display_, nullptr, initial_geometry_.left,
      initial_geometry_.top, initial_geometry_.width, initial_geometry_.height);

  ecore_wl2_egl_window_ = ecore_wl2_egl_window_create(
      ecore_wl2_window_, initial_geometry_.width, initial_geometry_.height);

  return ecore_wl2_egl_window_ && wl2_display_;
}

void TizenWindowEcoreWl2::SetWindowOptions() {
  // Change the window type to use the tizen policy for notification window
  // according to top_level_.
  // Note: ECORE_WL2_WINDOW_TYPE_TOPLEVEL is similar to "ELM_WIN_BASIC" and it
  // does not mean that the window always will be overlaid on other apps :(
  ecore_wl2_window_type_set(ecore_wl2_window_,
                            top_level_ ? ECORE_WL2_WINDOW_TYPE_NOTIFICATION
                                       : ECORE_WL2_WINDOW_TYPE_TOPLEVEL);
  if (top_level_) {
    SetTizenPolicyNotificationLevel(TIZEN_POLICY_LEVEL_TOP);
  }

  ecore_wl2_window_position_set(ecore_wl2_window_, initial_geometry_.left,
                                initial_geometry_.top);
  ecore_wl2_window_aux_hint_add(ecore_wl2_window_, 0,
                                "wm.policy.win.user.geometry", "1");

  if (transparent_) {
    ecore_wl2_window_alpha_set(ecore_wl2_window_, EINA_TRUE);
  } else {
    ecore_wl2_window_alpha_set(ecore_wl2_window_, EINA_FALSE);
  }

  if (!focusable_) {
    ecore_wl2_window_focus_skip_set(ecore_wl2_window_, EINA_TRUE);
  }

  ecore_wl2_window_indicator_state_set(ecore_wl2_window_,
                                       ECORE_WL2_INDICATOR_STATE_ON);
  ecore_wl2_window_indicator_opacity_set(ecore_wl2_window_,
                                         ECORE_WL2_INDICATOR_OPAQUE);
  ecore_wl2_indicator_visible_type_set(ecore_wl2_window_,
                                       ECORE_WL2_INDICATOR_VISIBLE_TYPE_SHOWN);

  int rotations[4] = {0, 90, 180, 270};
  ecore_wl2_window_available_rotations_set(ecore_wl2_window_, rotations,
                                           sizeof(rotations) / sizeof(int));
}

void TizenWindowEcoreWl2::RegisterEventHandlers() {
  ecore_event_handlers_.push_back(ecore_event_handler_add(
      ECORE_WL2_EVENT_WINDOW_ROTATE,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = reinterpret_cast<TizenWindowEcoreWl2*>(data);
        if (self->view_) {
          auto* rotation_event =
              reinterpret_cast<Ecore_Wl2_Event_Window_Rotation*>(event);
          if (rotation_event->win == self->GetWindowId()) {
            int32_t degree = rotation_event->angle;
            self->view_->OnRotate(degree);
            Geometry geometry = self->GetWindowGeometry();
            ecore_wl2_window_rotation_change_done_send(
                self->ecore_wl2_window_, rotation_event->rotation,
                geometry.width, geometry.height);
            return ECORE_CALLBACK_DONE;
          }
        }
        return ECORE_CALLBACK_PASS_ON;
      },
      this));

  ecore_event_handlers_.push_back(ecore_event_handler_add(
      ECORE_WL2_EVENT_WINDOW_CONFIGURE,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = reinterpret_cast<TizenWindowEcoreWl2*>(data);
        if (self->view_) {
          auto* configure_event =
              reinterpret_cast<Ecore_Wl2_Event_Window_Configure*>(event);
          if (configure_event->win == self->GetWindowId()) {
            self->view_->OnResize(configure_event->x, configure_event->y,
                                  configure_event->w, configure_event->h);
            ecore_wl2_window_commit(self->ecore_wl2_window_, EINA_FALSE);
            return ECORE_CALLBACK_DONE;
          }
        }
        return ECORE_CALLBACK_PASS_ON;
      },
      this));

  ecore_event_handlers_.push_back(ecore_event_handler_add(
      ECORE_EVENT_MOUSE_BUTTON_DOWN,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = reinterpret_cast<TizenWindowEcoreWl2*>(data);
        if (self->view_) {
          auto* button_event =
              reinterpret_cast<Ecore_Event_Mouse_Button*>(event);
          if (button_event->window == self->GetWindowId()) {
            self->view_->OnPointerDown(
                button_event->x, button_event->y, button_event->timestamp,
                kFlutterPointerDeviceKindTouch, button_event->multi.device);
            return ECORE_CALLBACK_DONE;
          }
        }
        return ECORE_CALLBACK_PASS_ON;
      },
      this));

  ecore_event_handlers_.push_back(ecore_event_handler_add(
      ECORE_EVENT_MOUSE_BUTTON_UP,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = reinterpret_cast<TizenWindowEcoreWl2*>(data);
        if (self->view_) {
          auto* button_event =
              reinterpret_cast<Ecore_Event_Mouse_Button*>(event);
          if (button_event->window == self->GetWindowId()) {
            self->view_->OnPointerUp(
                button_event->x, button_event->y, button_event->timestamp,
                kFlutterPointerDeviceKindTouch, button_event->multi.device);
            return ECORE_CALLBACK_DONE;
          }
        }
        return ECORE_CALLBACK_PASS_ON;
      },
      this));

  ecore_event_handlers_.push_back(ecore_event_handler_add(
      ECORE_EVENT_MOUSE_MOVE,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = reinterpret_cast<TizenWindowEcoreWl2*>(data);
        if (self->view_) {
          auto* move_event = reinterpret_cast<Ecore_Event_Mouse_Move*>(event);
          if (move_event->window == self->GetWindowId()) {
            self->view_->OnPointerMove(
                move_event->x, move_event->y, move_event->timestamp,
                kFlutterPointerDeviceKindTouch, move_event->multi.device);
            return ECORE_CALLBACK_DONE;
          }
        }
        return ECORE_CALLBACK_PASS_ON;
      },
      this));

  ecore_event_handlers_.push_back(ecore_event_handler_add(
      ECORE_EVENT_MOUSE_WHEEL,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = reinterpret_cast<TizenWindowEcoreWl2*>(data);
        if (self->view_) {
          auto* wheel_event = reinterpret_cast<Ecore_Event_Mouse_Wheel*>(event);
          if (wheel_event->window == self->GetWindowId()) {
            double delta_x = 0.0;
            double delta_y = 0.0;

            if (wheel_event->direction == kScrollDirectionVertical) {
              delta_y += wheel_event->z;
            } else if (wheel_event->direction == kScrollDirectionHorizontal) {
              delta_x += wheel_event->z;
            }

            self->view_->OnScroll(wheel_event->x, wheel_event->y, delta_x,
                                  delta_y, kScrollOffsetMultiplier,
                                  wheel_event->timestamp,
                                  kFlutterPointerDeviceKindTouch, 0);
            return ECORE_CALLBACK_DONE;
          }
        }
        return ECORE_CALLBACK_PASS_ON;
      },
      this));

  ecore_event_handlers_.push_back(ecore_event_handler_add(
      ECORE_EVENT_KEY_DOWN,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = reinterpret_cast<TizenWindowEcoreWl2*>(data);
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

  ecore_event_handlers_.push_back(ecore_event_handler_add(
      ECORE_EVENT_KEY_UP,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = reinterpret_cast<TizenWindowEcoreWl2*>(data);
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
}

void TizenWindowEcoreWl2::UnregisterEventHandlers() {
  for (Ecore_Event_Handler* handler : ecore_event_handlers_) {
    ecore_event_handler_del(handler);
  }
  ecore_event_handlers_.clear();
}

void TizenWindowEcoreWl2::DestroyWindow() {
  if (ecore_wl2_egl_window_) {
    ecore_wl2_egl_window_destroy(ecore_wl2_egl_window_);
    ecore_wl2_egl_window_ = nullptr;
  }

  if (ecore_wl2_window_) {
    ecore_wl2_window_free(ecore_wl2_window_);
    ecore_wl2_window_ = nullptr;
  }

  if (ecore_wl2_display_) {
    ecore_wl2_display_disconnect(ecore_wl2_display_);
    ecore_wl2_display_ = nullptr;
  }
  ecore_wl2_shutdown();
}

TizenWindow::Geometry TizenWindowEcoreWl2::GetWindowGeometry() {
  Geometry result;
  ecore_wl2_window_geometry_get(ecore_wl2_window_, &result.left, &result.top,
                                &result.width, &result.height);
  return result;
}

void TizenWindowEcoreWl2::SetWindowGeometry(Geometry geometry) {
  ecore_wl2_window_geometry_set(ecore_wl2_window_, geometry.left, geometry.top,
                                geometry.width, geometry.height);
  // FIXME: The changes set in `ecore_wl2_window_geometry_set` seems to apply
  // only after calling `ecore_wl2_window_position_set`. Call a more appropriate
  // API that flushes geometry settings to the compositor.
  ecore_wl2_window_position_set(ecore_wl2_window_, geometry.left, geometry.top);
}

TizenWindow::Geometry TizenWindowEcoreWl2::GetScreenGeometry() {
  Geometry result = {};
  ecore_wl2_display_screen_size_get(ecore_wl2_display_, &result.width,
                                    &result.height);
  return result;
}

int32_t TizenWindowEcoreWl2::GetRotation() {
  return ecore_wl2_window_rotation_get(ecore_wl2_window_);
}

int32_t TizenWindowEcoreWl2::GetDpi() {
  Ecore_Wl2_Output* output = ecore_wl2_window_output_find(ecore_wl2_window_);
  if (!output) {
    FT_LOG(Error) << "Could not find an output associated with the window.";
    return 0;
  }
  return ecore_wl2_output_dpi_get(output);
}

uintptr_t TizenWindowEcoreWl2::GetWindowId() {
  return ecore_wl2_window_id_get(ecore_wl2_window_);
}

void TizenWindowEcoreWl2::ResizeRenderTargetWithRotation(Geometry geometry,
                                                         int32_t angle) {
  ecore_wl2_egl_window_resize_with_rotation(
      ecore_wl2_egl_window_, geometry.left, geometry.top, geometry.width,
      geometry.height, angle);
}

void TizenWindowEcoreWl2::SetPreferredOrientations(
    const std::vector<int>& rotations) {
  ecore_wl2_window_available_rotations_set(ecore_wl2_window_, rotations.data(),
                                           rotations.size());
}

void TizenWindowEcoreWl2::BindKeys(const std::vector<std::string>& keys) {
  for (const std::string& key : keys) {
    ecore_wl2_window_keygrab_set(ecore_wl2_window_, key.c_str(), 0, 0, 0,
                                 ECORE_WL2_WINDOW_KEYGRAB_TOPMOST);
  }
}

void TizenWindowEcoreWl2::Show() {
  ecore_wl2_window_show(ecore_wl2_window_);
}

void TizenWindowEcoreWl2::OnGeometryChanged(Geometry geometry) {
  // This implementation mimics the situation in which the handler of
  // ECORE_WL2_EVENT_WINDOW_CONFIGURE is called.
  SetWindowGeometry(geometry);
  view_->OnResize(geometry.left, geometry.top, geometry.width, geometry.height);
}

void TizenWindowEcoreWl2::SetTizenPolicyNotificationLevel(int level) {
  Eina_Iterator* iter = ecore_wl2_display_globals_get(ecore_wl2_display_);
  struct wl_registry* registry =
      ecore_wl2_display_registry_get(ecore_wl2_display_);

  if (iter && registry) {
    Ecore_Wl2_Global* global = nullptr;

    // Retrieve global objects to bind tizen policy
    EINA_ITERATOR_FOREACH(iter, global) {
      if (strcmp(global->interface, tizen_policy_interface.name) == 0) {
        tizen_policy_ = static_cast<tizen_policy*>(
            wl_registry_bind(registry, global->id, &tizen_policy_interface, 1));
        break;
      }
    }
  }
  eina_iterator_free(iter);

  if (tizen_policy_ == nullptr) {
    FT_LOG(Error)
        << "Failed to initialize the tizen policy handle, the top_level "
           "attribute is ignored.";
    return;
  }

  tizen_policy_set_notification_level(
      tizen_policy_, ecore_wl2_window_surface_get(ecore_wl2_window_), level);
}

}  // namespace flutter
