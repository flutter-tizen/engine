// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_input_method_context.h"

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace {

const char* GetEcoreImfContextAvailableId() {
  Eina_List* modules;

  modules = ecore_imf_context_available_ids_get();
  if (modules) {
    void* module;
    EINA_LIST_FREE(modules, module) { return static_cast<const char*>(module); }
  }
  return nullptr;
}

Ecore_IMF_Input_Panel_Layout TextInputTypeToEcoreIMFInputPanelLayout(
    const std::string& text_input_type) {
  FT_ASSERT(panel_layout);
  if (text_input_type == "TextInputType.text" ||
      text_input_type == "TextInputType.multiline") {
    return ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;
  } else if (text_input_type == "TextInputType.number") {
    return ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY;
  } else if (text_input_type == "TextInputType.phone") {
    return ECORE_IMF_INPUT_PANEL_LAYOUT_PHONENUMBER;
  } else if (text_input_type == "TextInputType.datetime") {
    return ECORE_IMF_INPUT_PANEL_LAYOUT_DATETIME;
  } else if (text_input_type == "TextInputType.emailAddress") {
    return ECORE_IMF_INPUT_PANEL_LAYOUT_EMAIL;
  } else if (text_input_type == "TextInputType.url") {
    return ECORE_IMF_INPUT_PANEL_LAYOUT_URL;
  } else if (text_input_type == "TextInputType.visiblePassword") {
    return ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD;
  } else {
    FT_LOG(Warn) << "The requested input type " << text_input_type
                 << " is not supported.";
    return ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;
  }
}

Ecore_IMF_Keyboard_Modifiers EcoreInputModifiersToEcoreIMFModifiers(
    unsigned int ecore_modifiers) {
  unsigned int modifiers(ECORE_IMF_KEYBOARD_MODIFIER_NONE);
  if (ecore_modifiers & ECORE_EVENT_MODIFIER_SHIFT) {
    modifiers |= ECORE_IMF_KEYBOARD_MODIFIER_SHIFT;
  }
  if (ecore_modifiers & ECORE_EVENT_MODIFIER_ALT) {
    modifiers |= ECORE_IMF_KEYBOARD_MODIFIER_ALT;
  }
  if (ecore_modifiers & ECORE_EVENT_MODIFIER_CTRL) {
    modifiers |= ECORE_IMF_KEYBOARD_MODIFIER_CTRL;
  }
  if (ecore_modifiers & ECORE_EVENT_MODIFIER_WIN) {
    modifiers |= ECORE_IMF_KEYBOARD_MODIFIER_WIN;
  }
  if (ecore_modifiers & ECORE_EVENT_MODIFIER_ALTGR) {
    modifiers |= ECORE_IMF_KEYBOARD_MODIFIER_ALTGR;
  }
  return static_cast<Ecore_IMF_Keyboard_Modifiers>(modifiers);
}

Ecore_IMF_Keyboard_Locks EcoreInputModifiersToEcoreIMFLocks(
    unsigned int modifiers) {
  // If no other matches, returns NONE.
  unsigned int locks(ECORE_IMF_KEYBOARD_LOCK_NONE);
  if (modifiers & ECORE_EVENT_LOCK_NUM) {
    locks |= ECORE_IMF_KEYBOARD_LOCK_NUM;
  }
  if (modifiers & ECORE_EVENT_LOCK_CAPS) {
    locks |= ECORE_IMF_KEYBOARD_LOCK_CAPS;
  }
  if (modifiers & ECORE_EVENT_LOCK_SCROLL) {
    locks |= ECORE_IMF_KEYBOARD_LOCK_SCROLL;
  }
  return static_cast<Ecore_IMF_Keyboard_Locks>(locks);
}

}  // namespace

namespace flutter {

TizenInputMethodContext::TizenInputMethodContext(FlutterTizenEngine* engine)
    : engine_(engine) {
  FT_ASSERT(engine_);
  ecore_imf_init();

  const char* imf_id = ecore_imf_context_default_id_get();
  if (imf_id == nullptr) {
    // Try to get a fallback ID.
    imf_id = GetEcoreImfContextAvailableId();
  }
  if (imf_id == nullptr) {
    FT_LOG(Error) << "Failed to get an IMF context ID.";
    return;
  }

  imf_context_ = ecore_imf_context_add(imf_id);
  if (imf_context_ == nullptr) {
    FT_LOG(Error) << "Failed to create Ecore_IMF_Context.";
    return;
  }

  ecore_imf_context_client_window_set(
      imf_context_,
      reinterpret_cast<void*>(engine_->renderer()->GetWindowId()));
  SetContextOptions();
  SetInputPanelOptions();
  RegisterEventCallbacks();
}

TizenInputMethodContext::~TizenInputMethodContext() {
  UnregisterEventCallbacks();

  if (imf_context_) {
    ecore_imf_context_del(imf_context_);
  }

  ecore_imf_shutdown();
}

bool TizenInputMethodContext::FilterEvent(Ecore_Event_Key* event,
                                          const char* dev_name) {
  FT_ASSERT(imf_context_);
  FT_ASSERT(event);
  FT_ASSERT(dev_name);
  Ecore_IMF_Event_Key_Down imf_event;

  imf_event.keyname = event->keyname;
  imf_event.key = event->key;
  imf_event.string = event->string;
  imf_event.compose = event->compose;
  imf_event.timestamp = event->timestamp;
  imf_event.modifiers =
      EcoreInputModifiersToEcoreIMFModifiers(event->modifiers);
  imf_event.locks = EcoreInputModifiersToEcoreIMFLocks(event->modifiers);
  imf_event.dev_name = dev_name;
  imf_event.keycode = event->keycode;

  return ecore_imf_context_filter_event(
      imf_context_, ECORE_IMF_EVENT_KEY_DOWN,
      reinterpret_cast<Ecore_IMF_Event*>(&imf_event));
}

InputPanelGeometry TizenInputMethodContext::GetInputPanelGeometry() {
  FT_ASSERT(imf_context_);
  InputPanelGeometry geometry;
  ecore_imf_context_input_panel_geometry_get(
      imf_context_, &geometry.x, &geometry.y, &geometry.w, &geometry.h);
  return geometry;
}

void TizenInputMethodContext::ResetInputMethodContext() {
  FT_ASSERT(imf_context_);
  ecore_imf_context_reset(imf_context_);
}

void TizenInputMethodContext::ShowInputPanel() {
  FT_ASSERT(imf_context_);
  ecore_imf_context_input_panel_show(imf_context_);
  ecore_imf_context_focus_in(imf_context_);
}

void TizenInputMethodContext::HideInputPanel() {
  FT_ASSERT(imf_context_);
  ecore_imf_context_focus_out(imf_context_);
  ecore_imf_context_input_panel_hide(imf_context_);
}

void TizenInputMethodContext::SetInputPanelLayout(
    const std::string& input_type) {
  FT_ASSERT(imf_context_);
  auto panel_layout = TextInputTypeToEcoreIMFInputPanelLayout(input_type);
  ecore_imf_context_input_panel_layout_set(imf_context_, panel_layout);
}

void TizenInputMethodContext::SetInputPanelLayoutVariation(bool is_signed,
                                                           bool is_decimal) {
  Ecore_IMF_Input_Panel_Layout_Numberonly_Variation variation;
  if (is_signed && is_decimal) {
    variation =
        ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_SIGNED_AND_DECIMAL;
  } else if (is_signed) {
    variation = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_SIGNED;
  } else if (is_decimal) {
    variation = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_DECIMAL;
  } else {
    variation = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_NORMAL;
  }
  ecore_imf_context_input_panel_layout_variation_set(imf_context_, variation);
}

void TizenInputMethodContext::RegisterEventCallbacks() {
  FT_ASSERT(imf_context_);

  // commit callback
  event_callbacks_[ECORE_IMF_CALLBACK_COMMIT] =
      [](void* data, Ecore_IMF_Context* ctx, void* event_info) {
        flutter::TizenInputMethodContext* self =
            static_cast<flutter::TizenInputMethodContext*>(data);
        char* str = static_cast<char*>(event_info);
        if (self->on_commit_) {
          self->on_commit_(str);
        }
      };
  ecore_imf_context_event_callback_add(
      imf_context_, ECORE_IMF_CALLBACK_COMMIT,
      event_callbacks_[ECORE_IMF_CALLBACK_COMMIT], this);

  // pre-edit start callback
  event_callbacks_[ECORE_IMF_CALLBACK_PREEDIT_START] =
      [](void* data, Ecore_IMF_Context* ctx, void* event_info) {
        flutter::TizenInputMethodContext* self =
            static_cast<flutter::TizenInputMethodContext*>(data);
        if (self->on_preedit_start_) {
          self->on_preedit_start_();
        }
      };
  ecore_imf_context_event_callback_add(
      imf_context_, ECORE_IMF_CALLBACK_PREEDIT_START,
      event_callbacks_[ECORE_IMF_CALLBACK_PREEDIT_START], this);

  // pre-edit end callback
  event_callbacks_[ECORE_IMF_CALLBACK_PREEDIT_END] =
      [](void* data, Ecore_IMF_Context* ctx, void* event_info) {
        flutter::TizenInputMethodContext* self =
            static_cast<flutter::TizenInputMethodContext*>(data);
        if (self->on_preedit_end_) {
          self->on_preedit_end_();
        }
      };
  ecore_imf_context_event_callback_add(
      imf_context_, ECORE_IMF_CALLBACK_PREEDIT_END,
      event_callbacks_[ECORE_IMF_CALLBACK_PREEDIT_END], this);

  // pre-edit changed callback
  event_callbacks_[ECORE_IMF_CALLBACK_PREEDIT_CHANGED] =
      [](void* data, Ecore_IMF_Context* ctx, void* event_info) {
        flutter::TizenInputMethodContext* self =
            static_cast<flutter::TizenInputMethodContext*>(data);
        if (self->on_preedit_changed_) {
          char* str = nullptr;
          int cursor_pos = 0;
          ecore_imf_context_preedit_string_get(ctx, &str, &cursor_pos);
          if (str) {
            self->on_preedit_changed_(str, cursor_pos);
            free(str);
          }
        }
      };
  ecore_imf_context_event_callback_add(
      imf_context_, ECORE_IMF_CALLBACK_PREEDIT_CHANGED,
      event_callbacks_[ECORE_IMF_CALLBACK_PREEDIT_CHANGED], this);

  // input panel state callback
  ecore_imf_context_input_panel_event_callback_add(
      imf_context_, ECORE_IMF_INPUT_PANEL_STATE_EVENT,
      [](void* data, Ecore_IMF_Context* context, int value) {
        flutter::TizenInputMethodContext* self =
            static_cast<flutter::TizenInputMethodContext*>(data);
        if (self->on_input_panel_state_changed_) {
          self->on_input_panel_state_changed_(value);
        }
      },
      this);
}

void TizenInputMethodContext::UnregisterEventCallbacks() {
  FT_ASSERT(imf_context_);
  ecore_imf_context_event_callback_del(
      imf_context_, ECORE_IMF_CALLBACK_COMMIT,
      event_callbacks_[ECORE_IMF_CALLBACK_COMMIT]);
  ecore_imf_context_event_callback_del(
      imf_context_, ECORE_IMF_CALLBACK_PREEDIT_CHANGED,
      event_callbacks_[ECORE_IMF_CALLBACK_PREEDIT_CHANGED]);
  ecore_imf_context_event_callback_del(
      imf_context_, ECORE_IMF_CALLBACK_PREEDIT_START,
      event_callbacks_[ECORE_IMF_CALLBACK_PREEDIT_START]);
  ecore_imf_context_event_callback_del(
      imf_context_, ECORE_IMF_CALLBACK_PREEDIT_END,
      event_callbacks_[ECORE_IMF_CALLBACK_PREEDIT_END]);
  ecore_imf_context_input_panel_event_callback_clear(imf_context_);
}

void TizenInputMethodContext::SetContextOptions() {
  FT_ASSERT(imf_context_);
  ecore_imf_context_autocapital_type_set(imf_context_,
                                         ECORE_IMF_AUTOCAPITAL_TYPE_NONE);
  ecore_imf_context_prediction_allow_set(imf_context_, EINA_FALSE);
}

void TizenInputMethodContext::SetInputPanelOptions() {
  FT_ASSERT(imf_context_);
  ecore_imf_context_input_panel_layout_set(imf_context_,
                                           ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL);
  ecore_imf_context_input_panel_return_key_type_set(
      imf_context_, ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT);
  ecore_imf_context_input_panel_language_set(
      imf_context_, ECORE_IMF_INPUT_PANEL_LANG_AUTOMATIC);
}

}  // namespace flutter
