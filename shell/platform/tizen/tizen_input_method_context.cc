// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_input_method_context.h"

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace {

const char* GetEcoreImfContextAvailableID() {
  Eina_List* modules;

  modules = ecore_imf_context_available_ids_get();
  if (!modules) {
    return nullptr;
  }

  void* module;
  EINA_LIST_FREE(modules, module) { return static_cast<const char*>(module); }

  return nullptr;
}

bool TextInputTypeToEcoreIMFInputPanelLayout(
    std::string text_input_type,
    Ecore_IMF_Input_Panel_Layout* panel_layout) {
  FT_ASSERT(panel_layout);
  if (text_input_type == "TextInputType.text" ||
      text_input_type == "TextInputType.multiline") {
    *panel_layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;
  } else if (text_input_type == "TextInputType.number") {
    *panel_layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY;
  } else if (text_input_type == "TextInputType.phone") {
    *panel_layout = ECORE_IMF_INPUT_PANEL_LAYOUT_PHONENUMBER;
  } else if (text_input_type == "TextInputType.datetime") {
    *panel_layout = ECORE_IMF_INPUT_PANEL_LAYOUT_DATETIME;
  } else if (text_input_type == "TextInputType.emailAddress") {
    *panel_layout = ECORE_IMF_INPUT_PANEL_LAYOUT_EMAIL;
  } else if (text_input_type == "TextInputType.url") {
    *panel_layout = ECORE_IMF_INPUT_PANEL_LAYOUT_URL;
  } else if (text_input_type == "TextInputType.visiblePassword") {
    *panel_layout = ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD;
  } else if (text_input_type == "TextInputType.name" ||
             text_input_type == "TextInputType.address") {
    FT_LOG(Warn) << "The requested input type " << text_input_type
                 << "is not supported.";
    *panel_layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;
  } else {
    return false;
  }
  return true;
}

Ecore_IMF_Keyboard_Modifiers EcoreInputModifierToEcoreIMFModifier(
    unsigned int ecoreModifier) {
  unsigned int modifier(ECORE_IMF_KEYBOARD_MODIFIER_NONE);

  if (ecoreModifier & ECORE_EVENT_MODIFIER_SHIFT) {
    modifier |= ECORE_IMF_KEYBOARD_MODIFIER_SHIFT;
  }

  if (ecoreModifier & ECORE_EVENT_MODIFIER_ALT) {
    modifier |= ECORE_IMF_KEYBOARD_MODIFIER_ALT;
  }

  if (ecoreModifier & ECORE_EVENT_MODIFIER_CTRL) {
    modifier |= ECORE_IMF_KEYBOARD_MODIFIER_CTRL;
  }

  if (ecoreModifier & ECORE_EVENT_MODIFIER_WIN) {
    modifier |= ECORE_IMF_KEYBOARD_MODIFIER_WIN;
  }

  if (ecoreModifier & ECORE_EVENT_MODIFIER_ALTGR) {
    modifier |= ECORE_IMF_KEYBOARD_MODIFIER_ALTGR;
  }

  return static_cast<Ecore_IMF_Keyboard_Modifiers>(modifier);
}

Ecore_IMF_Keyboard_Locks EcoreInputModifierToEcoreIMFLock(
    unsigned int modifier) {
  // If no other matches, returns NONE.
  unsigned int lock(ECORE_IMF_KEYBOARD_LOCK_NONE);

  if (modifier & ECORE_EVENT_LOCK_NUM) {
    lock |= ECORE_IMF_KEYBOARD_LOCK_NUM;
  }

  if (modifier & ECORE_EVENT_LOCK_CAPS) {
    lock |= ECORE_IMF_KEYBOARD_LOCK_CAPS;
  }

  if (modifier & ECORE_EVENT_LOCK_SCROLL) {
    lock |= ECORE_IMF_KEYBOARD_LOCK_SCROLL;
  }

  return static_cast<Ecore_IMF_Keyboard_Locks>(lock);
}

void CommitCallback(void* data, Ecore_IMF_Context* ctx, void* event_info) {
  FT_ASSERT(data);

  flutter::TizenInputMethodContext* self =
      static_cast<flutter::TizenInputMethodContext*>(data);

  char* str = static_cast<char*>(event_info);

  self->OnCommit(str);
}

void PreeditCallback(void* data, Ecore_IMF_Context* ctx, void* event_info) {
  FT_ASSERT(data);
  flutter::TizenInputMethodContext* self =
      static_cast<flutter::TizenInputMethodContext*>(data);

  char* str = nullptr;
  int cursor_pos = 0;

  ecore_imf_context_preedit_string_get(ctx, &str, &cursor_pos);

  if (str) {
    self->OnPreedit(str, cursor_pos);
    free(str);
  }
}

void InputPanelStateChangedCallback(void* data,
                                    Ecore_IMF_Context* context,
                                    int value) {
  FT_ASSERT(data);
  flutter::TizenInputMethodContext* self =
      static_cast<flutter::TizenInputMethodContext*>(data);

  self->OnInputPannelStateChanged(value);
}

}  // namespace

namespace flutter {

TizenInputMethodContext::TizenInputMethodContext(FlutterTizenEngine* engine)
    : engine_(engine) {
  FT_ASSERT(engine_);
  Init();
}

TizenInputMethodContext::~TizenInputMethodContext() {
  Deinit();
}

void TizenInputMethodContext::Init() {
  FT_ASSERT(engine_);
  ecore_imf_init();

  const char* imf_id = ecore_imf_context_default_id_get();
  if (imf_id == nullptr) {
    // Try to get a fallback ID.
    imf_id = GetEcoreImfContextAvailableID();
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
  SetInputPannelOptions();
  RegisterEventCallbacks();
}

void TizenInputMethodContext::Deinit() {
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
  imf_event.modifiers = EcoreInputModifierToEcoreIMFModifier(event->modifiers);
  imf_event.locks = EcoreInputModifierToEcoreIMFLock(event->modifiers);
  imf_event.dev_name = dev_name;
  imf_event.keycode = event->keycode;

  bool ret = ecore_imf_context_filter_event(
      imf_context_, ECORE_IMF_EVENT_KEY_DOWN,
      reinterpret_cast<Ecore_IMF_Event*>(&imf_event));

  return ret;
}

InputPannelGeometry TizenInputMethodContext::GetInputPannelGeometry() {
  FT_ASSERT(imf_context_);
  InputPannelGeometry geometry;
  ecore_imf_context_input_panel_geometry_get(
      imf_context_, &geometry.x, &geometry.y, &geometry.w, &geometry.h);
  return geometry;
}

void TizenInputMethodContext::ResetInputMethodContext() {
  FT_ASSERT(imf_context_);
  ecore_imf_context_reset(imf_context_);
}

void TizenInputMethodContext::ShowInputPannel() {
  FT_ASSERT(imf_context_);
  ecore_imf_context_input_panel_show(imf_context_);
  ecore_imf_context_focus_in(imf_context_);
}

void TizenInputMethodContext::HideInputPannel() {
  FT_ASSERT(imf_context_);
  ecore_imf_context_focus_out(imf_context_);
  ecore_imf_context_input_panel_hide(imf_context_);
}

void TizenInputMethodContext::OnCommit(std::string str) {
  if (!on_commit_callback_) {
    FT_LOG(Warn) << "SetOnCommitCallback() has not been called.";
    return;
  }
  on_commit_callback_(str);
}

void TizenInputMethodContext::OnPreedit(std::string str, int cursor_pos) {
  if (!on_preedit_callback_) {
    FT_LOG(Warn) << "SetOnPreeditCallback() has not been called.";
    return;
  }
  on_preedit_callback_(str, cursor_pos);
}

void TizenInputMethodContext::OnInputPannelStateChanged(int state) {
  if (!on_input_pannel_state_changed_callback_) {
    FT_LOG(Warn)
        << "SetOnInputPannelStateChangedCallback() has not been called.";
    return;
  }
  on_input_pannel_state_changed_callback_(state);
}

void TizenInputMethodContext::SetInputPannelLayout(std::string input_type) {
  FT_ASSERT(imf_context_);
  Ecore_IMF_Input_Panel_Layout panel_layout;
  if (TextInputTypeToEcoreIMFInputPanelLayout(input_type, &panel_layout)) {
    ecore_imf_context_input_panel_layout_set(imf_context_, panel_layout);
  }
}

void TizenInputMethodContext::RegisterEventCallbacks() {
  FT_ASSERT(imf_context_);
  ecore_imf_context_event_callback_add(imf_context_, ECORE_IMF_CALLBACK_COMMIT,
                                       CommitCallback, this);
  ecore_imf_context_event_callback_add(
      imf_context_, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, PreeditCallback, this);
  ecore_imf_context_input_panel_event_callback_add(
      imf_context_, ECORE_IMF_INPUT_PANEL_STATE_EVENT,
      InputPanelStateChangedCallback, this);
}

void TizenInputMethodContext::UnregisterEventCallbacks() {
  FT_ASSERT(imf_context_);
  ecore_imf_context_event_callback_del(imf_context_, ECORE_IMF_CALLBACK_COMMIT,
                                       CommitCallback);
  ecore_imf_context_event_callback_del(
      imf_context_, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, PreeditCallback);
  ecore_imf_context_input_panel_event_callback_del(
      imf_context_, ECORE_IMF_INPUT_PANEL_STATE_EVENT,
      InputPanelStateChangedCallback);
}

void TizenInputMethodContext::SetContextOptions() {
  FT_ASSERT(imf_context_);
  ecore_imf_context_autocapital_type_set(imf_context_,
                                         ECORE_IMF_AUTOCAPITAL_TYPE_NONE);
  ecore_imf_context_prediction_allow_set(imf_context_, EINA_FALSE);
}

void TizenInputMethodContext::SetInputPannelOptions() {
  FT_ASSERT(imf_context_);
  ecore_imf_context_input_panel_layout_set(imf_context_,
                                           ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL);
  ecore_imf_context_input_panel_return_key_type_set(
      imf_context_, ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT);
  ecore_imf_context_input_panel_layout_variation_set(imf_context_, 0);
  ecore_imf_context_input_panel_language_set(
      imf_context_, ECORE_IMF_INPUT_PANEL_LANG_AUTOMATIC);
}

}  // namespace flutter
