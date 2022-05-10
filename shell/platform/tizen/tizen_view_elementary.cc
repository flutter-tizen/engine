// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_view_elementary.h"

#include "flutter/shell/platform/tizen/flutter_tizen_view.h"
#include "flutter/shell/platform/tizen/logger.h"

#include <efl_extension.h>
#include <ui/efl_util.h>

namespace {

static const int kScrollDirectionVertical = 0;
static const int kScrollDirectionHorizontal = 1;
static const int kScrollOffsetMultiplier = 20;

uint32_t EvasModifierToEcoreEventModifiers(const Evas_Modifier* evas_modifier) {
  uint32_t modifiers = 0;
  if (evas_key_modifier_is_set(evas_modifier, "Control")) {
    modifiers |= ECORE_EVENT_MODIFIER_CTRL;
  }
  if (evas_key_modifier_is_set(evas_modifier, "Alt")) {
    modifiers |= ECORE_EVENT_MODIFIER_ALT;
  }
  if (evas_key_modifier_is_set(evas_modifier, "Shift")) {
    modifiers |= ECORE_EVENT_MODIFIER_SHIFT;
  }
  return modifiers;
}

void EvasObjectResize(Evas_Object* object, uint32_t width, uint32_t height) {
  evas_object_resize(object, width, height);
  evas_object_size_hint_min_set(object, width, height);
  evas_object_size_hint_max_set(object, width, height);
}

}  // namespace

namespace flutter {

TizenViewElementary::TizenViewElementary(TizenBaseHandle::Geometry geometry,
                                         Evas_Object* elm_parent)
    : TizenView(geometry), elm_parent_(elm_parent) {
  if (!CreateView()) {
    FT_LOG(Error) << "Failed to create a platform view.";
    return;
  }

  RegisterEventHandlers();
  PrepareInputMethod();
  Show();
}

TizenViewElementary::~TizenViewElementary() {
  UnregisterEventHandlers();
}

bool TizenViewElementary::CreateView() {
  elm_config_accel_preference_set("hw:opengl");

  Ecore_Evas* ecore_evas =
      ecore_evas_ecore_evas_get(evas_object_evas_get(elm_parent_));

  int32_t elm_parent_width, elm_parent_height;
  evas_object_geometry_get(elm_parent_, nullptr, nullptr, &elm_parent_width,
                           &elm_parent_height);

  if (initial_geometry_.width == 0) {
    initial_geometry_.width = elm_parent_width;
  }
  if (initial_geometry_.height == 0) {
    initial_geometry_.height = elm_parent_height;
  }

  int32_t ecore_evas_width, ecore_evas_height;
  ecore_evas_screen_geometry_get(ecore_evas, nullptr, nullptr,
                                 &ecore_evas_width, &ecore_evas_height);
  if (ecore_evas_width == 0 || ecore_evas_height == 0) {
    FT_LOG(Error) << "Invalid screen size: " << ecore_evas_width << " x "
                  << ecore_evas_height;
    return false;
  }

  if (initial_geometry_.width == 0) {
    initial_geometry_.width = ecore_evas_width;
  }
  if (initial_geometry_.height == 0) {
    initial_geometry_.height = ecore_evas_height;
  }

  container_ = elm_table_add(elm_parent_);
  EvasObjectResize(container_, initial_geometry_.width,
                   initial_geometry_.height);
  evas_object_size_hint_weight_set(container_, EVAS_HINT_EXPAND,
                                   EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(container_, EVAS_HINT_FILL, EVAS_HINT_FILL);

  image_ = evas_object_image_filled_add(evas_object_evas_get(container_));
  EvasObjectResize(image_, initial_geometry_.width, initial_geometry_.height);
  evas_object_image_size_set(image_, initial_geometry_.width,
                             initial_geometry_.height);
  evas_object_image_alpha_set(image_, EINA_TRUE);
  elm_table_pack(container_, image_, 0, 0, 1, 1);

  // FIXME: Button widgets can receive both mouse events and key events. But the
  // button widget is temporary. It should be changed to the appropriate object
  // that can be used as event layer.
  event_layer_ = elm_button_add(container_);
  elm_object_style_set(event_layer_, "transparent");
  evas_object_color_set(event_layer_, 0, 0, 0, 0);
  EvasObjectResize(event_layer_, initial_geometry_.width,
                   initial_geometry_.height);
  elm_table_pack(container_, event_layer_, 0, 0, 1, 1);

  if (!image_) {
    return false;
  }

  return true;
}

void TizenViewElementary::RegisterEventHandlers() {
  evas_object_callbacks_[EVAS_CALLBACK_MOUSE_DOWN] =
      [](void* data, Evas* evas, Evas_Object* object, void* event_info) {
        auto* self = reinterpret_cast<TizenViewElementary*>(data);
        if (self->view_) {
          if (self->event_layer_ == object) {
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
      event_layer_, EVAS_CALLBACK_MOUSE_DOWN,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_DOWN], this);

  evas_object_callbacks_[EVAS_CALLBACK_MOUSE_UP] = [](void* data, Evas* evas,
                                                      Evas_Object* object,
                                                      void* event_info) {
    auto* self = reinterpret_cast<TizenViewElementary*>(data);
    if (self->view_) {
      if (self->event_layer_ == object) {
        auto* mouse_event = reinterpret_cast<Evas_Event_Mouse_Up*>(event_info);
        self->view_->OnPointerUp(mouse_event->canvas.x, mouse_event->canvas.y,
                                 mouse_event->timestamp,
                                 kFlutterPointerDeviceKindTouch,
                                 mouse_event->button);
      }
    }
  };
  evas_object_event_callback_add(event_layer_, EVAS_CALLBACK_MOUSE_UP,
                                 evas_object_callbacks_[EVAS_CALLBACK_MOUSE_UP],
                                 this);

  evas_object_callbacks_[EVAS_CALLBACK_MOUSE_MOVE] =
      [](void* data, Evas* evas, Evas_Object* object, void* event_info) {
        auto* self = reinterpret_cast<TizenViewElementary*>(data);
        if (self->view_) {
          if (self->event_layer_ == object) {
            auto* mouse_event =
                reinterpret_cast<Evas_Event_Mouse_Move*>(event_info);
            mouse_event->event_flags = (Evas_Event_Flags)(
                mouse_event->event_flags & EVAS_EVENT_FLAG_ON_HOLD);
            self->view_->OnPointerMove(
                mouse_event->cur.canvas.x, mouse_event->cur.canvas.y,
                mouse_event->timestamp, kFlutterPointerDeviceKindTouch,
                mouse_event->buttons);
          }
        }
      };
  evas_object_event_callback_add(
      event_layer_, EVAS_CALLBACK_MOUSE_MOVE,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_MOVE], this);

  evas_object_callbacks_[EVAS_CALLBACK_MOUSE_WHEEL] = [](void* data, Evas* evas,
                                                         Evas_Object* object,
                                                         void* event_info) {
    auto* self = reinterpret_cast<TizenViewElementary*>(data);
    if (self->view_) {
      if (self->event_layer_ == object) {
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
      event_layer_, EVAS_CALLBACK_MOUSE_WHEEL,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_WHEEL], this);

  evas_object_callbacks_[EVAS_CALLBACK_KEY_DOWN] = [](void* data, Evas* evas,
                                                      Evas_Object* object,
                                                      void* event_info) {
    auto* self = reinterpret_cast<TizenViewElementary*>(data);
    if (self->view_) {
      if (self->event_layer_ == object) {
        auto* key_event = reinterpret_cast<Evas_Event_Key_Down*>(event_info);
        int handled = false;
        key_event->event_flags = EVAS_EVENT_FLAG_ON_HOLD;
        if (self->input_method_context_->IsInputPanelShown()) {
          handled =
              self->input_method_context_->HandleEvasEventKeyDown(key_event);
        }
        if (!handled) {
          self->view_->OnKey(
              key_event->key, key_event->string, key_event->compose,
              EvasModifierToEcoreEventModifiers(key_event->modifiers),
              key_event->keycode, true);
        }
      }
    }
  };
  evas_object_event_callback_add(event_layer_, EVAS_CALLBACK_KEY_DOWN,
                                 evas_object_callbacks_[EVAS_CALLBACK_KEY_DOWN],
                                 this);

  evas_object_callbacks_[EVAS_CALLBACK_KEY_UP] =
      [](void* data, Evas* evas, Evas_Object* object, void* event_info) {
        auto* self = reinterpret_cast<TizenViewElementary*>(data);
        if (self->view_) {
          if (self->event_layer_ == object) {
            auto* key_event = reinterpret_cast<Evas_Event_Key_Up*>(event_info);
            int handled = false;
            key_event->event_flags = EVAS_EVENT_FLAG_ON_HOLD;
            if (self->input_method_context_->IsInputPanelShown()) {
              handled =
                  self->input_method_context_->HandleEvasEventKeyUp(key_event);
            }
            if (!handled) {
              self->view_->OnKey(
                  key_event->key, key_event->string, key_event->compose,
                  EvasModifierToEcoreEventModifiers(key_event->modifiers),
                  key_event->keycode, false);
            }
          }
        }
      };
  evas_object_event_callback_add(event_layer_, EVAS_CALLBACK_KEY_UP,
                                 evas_object_callbacks_[EVAS_CALLBACK_KEY_UP],
                                 this);
}

void TizenViewElementary::UnregisterEventHandlers() {
  evas_object_event_callback_del(
      image_, EVAS_CALLBACK_MOUSE_DOWN,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_DOWN]);
  evas_object_event_callback_del(
      image_, EVAS_CALLBACK_MOUSE_UP,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_UP]);
  evas_object_event_callback_del(
      image_, EVAS_CALLBACK_MOUSE_MOVE,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_MOVE]);
  evas_object_event_callback_del(
      image_, EVAS_CALLBACK_MOUSE_WHEEL,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_WHEEL]);
  evas_object_event_callback_del(
      container_, EVAS_CALLBACK_KEY_DOWN,
      evas_object_callbacks_[EVAS_CALLBACK_KEY_DOWN]);
  evas_object_event_callback_del(container_, EVAS_CALLBACK_KEY_UP,
                                 evas_object_callbacks_[EVAS_CALLBACK_KEY_UP]);
}

TizenBaseHandle::Geometry TizenViewElementary::GetRenderTargetGeometry() {
  Geometry result;
  evas_object_geometry_get(image_, &result.left, &result.top, &result.width,
                           &result.height);
  return result;
}

void TizenViewElementary::SetRenderTargetGeometry(Geometry geometry) {
  evas_object_resize(image_, geometry.width, geometry.height);
  evas_object_size_hint_min_set(image_, geometry.width, geometry.height);
  evas_object_size_hint_max_set(image_, geometry.width, geometry.height);
  evas_object_move(image_, geometry.left, geometry.top);
}

TizenBaseHandle::Geometry TizenViewElementary::GetScreenGeometry() {
  Geometry result;
  evas_object_geometry_get(image_, &result.left, &result.top, &result.width,
                           &result.height);
  return result;
}

int32_t TizenViewElementary::GetDpi() {
  Ecore_Evas* ecore_evas =
      ecore_evas_ecore_evas_get(evas_object_evas_get(image_));
  int32_t xdpi, ydpi;
  ecore_evas_screen_dpi_get(ecore_evas, &xdpi, &ydpi);
  return xdpi;
}

uintptr_t TizenViewElementary::GetWindowId() {
  return ecore_evas_window_get(
      ecore_evas_ecore_evas_get(evas_object_evas_get(image_)));
}

void TizenViewElementary::ResizeRenderTargetWithRotation(Geometry geometry,
                                                         int32_t angle) {
  TizenRendererEvasGL* renderer_evas_gl =
      reinterpret_cast<TizenRendererEvasGL*>(view_->engine()->renderer());
  renderer_evas_gl->ResizeSurface(geometry.width, geometry.height);
}

void TizenViewElementary::BindKeys(const std::vector<std::string>& keys) {
  // Views do not need to have window info.
  // However, it is necessary to bind a special key for each profile(in this
  // case). This part is modified after refactoring related to the key event
  // code.
  Evas_Object* elm_win = (Evas_Object*)ecore_evas_data_get(
      ecore_evas_ecore_evas_get(evas_object_evas_get(image_)), "elm_win");
  if (elm_win) {
    for (const std::string& key : keys) {
      eext_win_keygrab_set(elm_win, key.c_str());
    }
  }
}

void TizenViewElementary::Show() {
  evas_object_show(container_);
  evas_object_show(image_);
  evas_object_show(event_layer_);
}

void TizenViewElementary::OnGeometryChanged(Geometry geometry) {
  SetRenderTargetGeometry(geometry);
  view_->OnResize(geometry.left, geometry.top, geometry.width, geometry.height);
}

void TizenViewElementary::PrepareInputMethod() {
  input_method_context_ =
      std::make_unique<TizenInputMethodContext>(GetWindowId());

  // Set input method callbacks.
  input_method_context_->SetOnPreeditStart(
      [this]() { view_->OnComposeBegin(); });
  input_method_context_->SetOnPreeditChanged(
      [this](std::string str, int cursor_pos) {
        view_->OnComposeChange(str, cursor_pos);
      });
  input_method_context_->SetOnPreeditEnd([this]() { view_->OnComposeEnd(); });
  input_method_context_->SetOnCommit(
      [this](std::string str) { view_->OnCommit(str); });
}

}  // namespace flutter
