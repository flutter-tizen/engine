// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "text_input_channel.h"

#include <Ecore.h>
#include <Ecore_IMF_Evas.h>

#include "flutter/shell/platform/tizen/tizen_embedder_engine.h"
#include "flutter/shell/platform/tizen/tizen_log.h"
#include "stdlib.h"
#include "string.h"

static constexpr char kSetEditingStateMethod[] = "TextInput.setEditingState";
static constexpr char kClearClientMethod[] = "TextInput.clearClient";
static constexpr char kSetClientMethod[] = "TextInput.setClient";
static constexpr char kShowMethod[] = "TextInput.show";
static constexpr char kHideMethod[] = "TextInput.hide";
static constexpr char kMultilineInputType[] = "TextInputType.multiline";
static constexpr char kUpdateEditingStateMethod[] =
    "TextInputClient.updateEditingState";
static constexpr char kPerformActionMethod[] = "TextInputClient.performAction";
static constexpr char kSetPlatformViewClient[] =
    "TextInput.setPlatformViewClient";
static constexpr char kTextInputAction[] = "inputAction";
static constexpr char kTextInputType[] = "inputType";
static constexpr char kTextInputTypeName[] = "name";
static constexpr char kComposingBaseKey[] = "composingBase";
static constexpr char kComposingExtentKey[] = "composingExtent";
static constexpr char kSelectionAffinityKey[] = "selectionAffinity";
static constexpr char kAffinityDownstream[] = "TextAffinity.downstream";
static constexpr char kSelectionBaseKey[] = "selectionBase";
static constexpr char kSelectionExtentKey[] = "selectionExtent";
static constexpr char kSelectionIsDirectionalKey[] = "selectionIsDirectional";
static constexpr char kTextKey[] = "text";
static constexpr char kChannelName[] = "flutter/textinput";
static constexpr char kBadArgumentError[] = "Bad Arguments";
static constexpr char kInternalConsistencyError[] =
    "Internal Consistency Error";

static const char* GetImfMethod() {
  Eina_List* modules;

  modules = ecore_imf_context_available_ids_get();
  if (!modules) return nullptr;

  void* module;
  EINA_LIST_FREE(modules, module) { return (const char*)module; }

  return nullptr;
}

static bool IsASCIIPrintableKey(char c) {
  if (c >= 32 && c <= 126) {
    return true;
  }
  return false;
}

void TextInputChannel::CommitCallback(void* data, Ecore_IMF_Context* ctx,
                                      void* event_info) {
  TextInputChannel* self = (TextInputChannel*)data;
  char* str = (char*)event_info;
  self->OnCommit(str);
}

void TextInputChannel::PreeditCallback(void* data, Ecore_IMF_Context* ctx,
                                       void* event_info) {
  TextInputChannel* self = (TextInputChannel*)data;
  char* preedit_string = nullptr;
  int cursor_pos;
  ecore_imf_context_preedit_string_get(ctx, &preedit_string, &cursor_pos);
  if (preedit_string) {
    self->OnPredit(preedit_string, cursor_pos);
    free(preedit_string);
  }
}

void TextInputChannel::PrivateCommandCallback(void* data,
                                              Ecore_IMF_Context* ctx,
                                              void* event_info) {
  // TODO
  FT_LOGD_UNIMPLEMENTED();
}

void TextInputChannel::DeleteSurroundingCallback(void* data,
                                                 Ecore_IMF_Context* ctx,
                                                 void* event_info) {
  // TODO
  FT_LOGD_UNIMPLEMENTED();
}

void TextInputChannel::InputPanelStateChangedCallback(
    void* data, Ecore_IMF_Context* context, int value) {
  if (!data) {
    FT_LOGD("[No Data]\n");
    return;
  }
  TextInputChannel* self = (TextInputChannel*)data;
  switch (value) {
    case ECORE_IMF_INPUT_PANEL_STATE_SHOW: {
      FT_LOGD("[PANEL_STATE_SHOW]\n");
      if (self->engine_->device_profile ==
          "mobile") {  // FIXME : Needs improvement on other devices.
        ecore_timer_add(
            0.25,
            [](void* data) -> Eina_Bool {
              TextInputChannel* self = (TextInputChannel*)data;
              auto window_geometry =
                  self->engine_->tizen_native_window->GetGeometry();
              int32_t surface_w = window_geometry.w;
              int32_t surface_h =
                  window_geometry.h - self->current_keyboard_geometry_.h;

              self->engine_->tizen_native_window->GetTizenNativeEGLWindow()
                  ->ResizeWithRotation(0, 0, surface_w, surface_h, 0);
              if (self->rotation == 90 || self->rotation == 270) {
                self->engine_->SendWindowMetrics(surface_h, surface_w, 0);
              } else {
                self->engine_->SendWindowMetrics(surface_w, surface_h, 0);
              }

              return ECORE_CALLBACK_CANCEL;
            },
            self);
      }
    } break;
    case ECORE_IMF_INPUT_PANEL_STATE_HIDE:
      self->HideSoftwareKeyboard();  // FIXME: Fallback for HW back-key
      FT_LOGD("[PANEL_STATE_HIDE]\n");
      break;
    case ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW:
      FT_LOGD("[PANEL_STATE_WILL_SHOW]\n");
      break;
    default:
      FT_LOGD("[PANEL_STATE_EVENT (default: %d)]\n", value);
      break;
  }
}

void TextInputChannel::InputPanelGeometryChangedCallback(
    void* data, Ecore_IMF_Context* context, int value) {
  if (!data) {
    FT_LOGD("[No Data]\n");
    return;
  }
  TextInputChannel* self = (TextInputChannel*)data;
  ecore_imf_context_input_panel_geometry_get(
      self->imf_context_, &self->current_keyboard_geometry_.x,
      &self->current_keyboard_geometry_.y, &self->current_keyboard_geometry_.w,
      &self->current_keyboard_geometry_.h);

  FT_LOGD(
      "[Current keyboard geometry] x:%d y:%d w:%d h:%d\n",
      self->current_keyboard_geometry_.x, self->current_keyboard_geometry_.y,
      self->current_keyboard_geometry_.w, self->current_keyboard_geometry_.h);
}

Eina_Bool TextInputChannel::RetrieveSurroundingCallback(void* data,
                                                        Ecore_IMF_Context* ctx,
                                                        char** text,
                                                        int* cursor_pos) {
  // TODO
  if (text) {
    *text = strdup("");
  }
  if (cursor_pos) {
    *cursor_pos = 0;
  }
  return EINA_TRUE;
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

Ecore_IMF_Device_Class EoreDeviceClassToEcoreIMFDeviceClass(
    Ecore_Device_Class ecoreDeviceClass) {
  switch (ecoreDeviceClass) {
    case ECORE_DEVICE_CLASS_SEAT:
      return ECORE_IMF_DEVICE_CLASS_SEAT;
    case ECORE_DEVICE_CLASS_KEYBOARD:
      return ECORE_IMF_DEVICE_CLASS_KEYBOARD;
    case ECORE_DEVICE_CLASS_MOUSE:
      return ECORE_IMF_DEVICE_CLASS_MOUSE;
    case ECORE_DEVICE_CLASS_TOUCH:
      return ECORE_IMF_DEVICE_CLASS_TOUCH;
    case ECORE_DEVICE_CLASS_PEN:
      return ECORE_IMF_DEVICE_CLASS_PEN;
    case ECORE_DEVICE_CLASS_POINTER:
      return ECORE_IMF_DEVICE_CLASS_POINTER;
    case ECORE_DEVICE_CLASS_GAMEPAD:
      return ECORE_IMF_DEVICE_CLASS_GAMEPAD;
    case ECORE_DEVICE_CLASS_NONE:
    default:
      return ECORE_IMF_DEVICE_CLASS_NONE;
  }
}

Ecore_IMF_Device_Subclass EoreDeviceSubClassToEcoreIMFDeviceSubClass(
    Ecore_Device_Subclass ecoreDeviceSubclass) {
  switch (ecoreDeviceSubclass) {
    case ECORE_DEVICE_SUBCLASS_FINGER:
      return ECORE_IMF_DEVICE_SUBCLASS_FINGER;
    case ECORE_DEVICE_SUBCLASS_FINGERNAIL:
      return ECORE_IMF_DEVICE_SUBCLASS_FINGERNAIL;
    case ECORE_DEVICE_SUBCLASS_KNUCKLE:
      return ECORE_IMF_DEVICE_SUBCLASS_KNUCKLE;
    case ECORE_DEVICE_SUBCLASS_PALM:
      return ECORE_IMF_DEVICE_SUBCLASS_PALM;
    case ECORE_DEVICE_SUBCLASS_HAND_SIZE:
      return ECORE_IMF_DEVICE_SUBCLASS_HAND_SIZE;
    case ECORE_DEVICE_SUBCLASS_HAND_FLAT:
      return ECORE_IMF_DEVICE_SUBCLASS_HAND_FLAT;
    case ECORE_DEVICE_SUBCLASS_PEN_TIP:
      return ECORE_IMF_DEVICE_SUBCLASS_PEN_TIP;
    case ECORE_DEVICE_SUBCLASS_TRACKPAD:
      return ECORE_IMF_DEVICE_SUBCLASS_TRACKPAD;
    case ECORE_DEVICE_SUBCLASS_TRACKPOINT:
      return ECORE_IMF_DEVICE_SUBCLASS_TRACKPOINT;
    case ECORE_DEVICE_SUBCLASS_TRACKBALL:
      return ECORE_IMF_DEVICE_SUBCLASS_TRACKBALL;
    case ECORE_DEVICE_SUBCLASS_REMOCON:
    case ECORE_DEVICE_SUBCLASS_VIRTUAL_KEYBOARD:
      // FT_LOGW("There is no corresponding type");
    case ECORE_DEVICE_SUBCLASS_NONE:
    default:
      return ECORE_IMF_DEVICE_SUBCLASS_NONE;
  }
}

TextInputChannel::TextInputChannel(flutter::BinaryMessenger* messenger,
                                   TizenEmbedderEngine* engine)
    : channel_(std::make_unique<flutter::MethodChannel<rapidjson::Document>>(
          messenger, kChannelName, &flutter::JsonMethodCodec::GetInstance())),
      active_model_(nullptr),
      is_software_keyboard_showing_(false),
      last_preedit_string_length_(0),
      imf_context_(nullptr),
      in_select_mode_(false),
      engine_(engine) {
  channel_->SetMethodCallHandler(
      [this](
          const flutter::MethodCall<rapidjson::Document>& call,
          std::unique_ptr<flutter::MethodResult<rapidjson::Document>> result) {
        HandleMethodCall(call, std::move(result));
      });

  ecore_imf_init();
  // Register IMF callbacks
  if (ecore_imf_context_default_id_get()) {
    imf_context_ = ecore_imf_context_add(ecore_imf_context_default_id_get());
  } else if (GetImfMethod() != nullptr) {
    imf_context_ = ecore_imf_context_add(GetImfMethod());
  }
  if (imf_context_) {
    Ecore_Wl2_Window* ecoreWindow =
        engine_->tizen_native_window->GetWindowHandle();
    ecore_imf_context_client_window_set(
        imf_context_, (void*)ecore_wl2_window_id_get(ecoreWindow));
    RegisterIMFCallback();
  } else {
    FT_LOGE("Failed to create imfContext");
  }
}

TextInputChannel::~TextInputChannel() {
  if (imf_context_) {
    UnregisterIMFCallback();
    ecore_imf_context_del(imf_context_);
    ecore_imf_shutdown();
  }
}

void TextInputChannel::OnKeyDown(Ecore_Event_Key* key) {
  if (active_model_ && !FilterEvent(key)) {
    NonIMFFallback(key);
  }
}

void TextInputChannel::HandleMethodCall(
    const flutter::MethodCall<rapidjson::Document>& method_call,
    std::unique_ptr<flutter::MethodResult<rapidjson::Document>> result) {
  const std::string& method = method_call.method_name();

  FT_LOGD("Method : %s", method.data());

  if (method.compare(kShowMethod) == 0) {
    ShowSoftwareKeyboard();
  } else if (method.compare(kHideMethod) == 0) {
    HideSoftwareKeyboard();
  } else if (method.compare(kSetPlatformViewClient) == 0) {
    // TODO: implement if necessary
  } else if (method.compare(kClearClientMethod) == 0) {
    active_model_ = nullptr;
  } else if (method.compare(kSetClientMethod) == 0) {
    if (!method_call.arguments() || method_call.arguments()->IsNull()) {
      result->Error(kBadArgumentError, "Method invoked without args");
      return;
    }
    const rapidjson::Document& args = *method_call.arguments();

    // TODO(awdavies): There's quite a wealth of arguments supplied with this
    // method, and they should be inspected/used.
    const rapidjson::Value& client_id_json = args[0];
    const rapidjson::Value& client_config = args[1];
    if (client_id_json.IsNull()) {
      result->Error(kBadArgumentError, "Could not set client, ID is null.");
      return;
    }
    if (client_config.IsNull()) {
      result->Error(kBadArgumentError,
                    "Could not set client, missing arguments.");
    }
    client_id_ = client_id_json.GetInt();
    input_action_ = "";
    auto input_action_json = client_config.FindMember(kTextInputAction);
    if (input_action_json != client_config.MemberEnd() &&
        input_action_json->value.IsString()) {
      input_action_ = input_action_json->value.GetString();
    }
    input_type_ = "";
    auto input_type_info_json = client_config.FindMember(kTextInputType);
    if (input_type_info_json != client_config.MemberEnd() &&
        input_type_info_json->value.IsObject()) {
      auto input_type_json =
          input_type_info_json->value.FindMember(kTextInputTypeName);
      if (input_type_json != input_type_info_json->value.MemberEnd() &&
          input_type_json->value.IsString()) {
        input_type_ = input_type_json->value.GetString();
      }
    }
    active_model_ = std::make_unique<flutter::TextInputModel>();
  } else if (method.compare(kSetEditingStateMethod) == 0) {
    if (!method_call.arguments() || method_call.arguments()->IsNull()) {
      result->Error(kBadArgumentError, "Method invoked without args");
      return;
    }
    const rapidjson::Document& args = *method_call.arguments();

    if (active_model_ == nullptr) {
      result->Error(
          kInternalConsistencyError,
          "Set editing state has been invoked, but no client is set.");
      return;
    }
    auto text = args.FindMember(kTextKey);
    if (text == args.MemberEnd() || text->value.IsNull()) {
      result->Error(kBadArgumentError,
                    "Set editing state has been invoked, but without text.");
      return;
    }
    auto selection_base = args.FindMember(kSelectionBaseKey);
    auto selection_extent = args.FindMember(kSelectionExtentKey);
    if (selection_base == args.MemberEnd() || selection_base->value.IsNull() ||
        selection_extent == args.MemberEnd() ||
        selection_extent->value.IsNull()) {
      result->Error(kInternalConsistencyError,
                    "Selection base/extent values invalid.");
      return;
    }
    active_model_->SetEditingState(selection_base->value.GetInt(),
                                   selection_extent->value.GetInt(),
                                   text->value.GetString());
  } else {
    result->NotImplemented();
    return;
  }
  // All error conditions return early, so if nothing has gone wrong indicate
  // success.
  result->Success();
}

void TextInputChannel::SendStateUpdate(const flutter::TextInputModel& model) {
  auto args = std::make_unique<rapidjson::Document>(rapidjson::kArrayType);
  auto& allocator = args->GetAllocator();
  args->PushBack(client_id_, allocator);

  rapidjson::Value editing_state(rapidjson::kObjectType);
  editing_state.AddMember(kComposingBaseKey, -1, allocator);
  editing_state.AddMember(kComposingExtentKey, -1, allocator);
  editing_state.AddMember(kSelectionAffinityKey, kAffinityDownstream,
                          allocator);
  editing_state.AddMember(kSelectionBaseKey, model.selection_base(), allocator);
  editing_state.AddMember(kSelectionExtentKey, model.selection_extent(),
                          allocator);
  editing_state.AddMember(kSelectionIsDirectionalKey, false, allocator);
  editing_state.AddMember(
      kTextKey, rapidjson::Value(model.GetText(), allocator).Move(), allocator);
  args->PushBack(editing_state, allocator);

  channel_->InvokeMethod(kUpdateEditingStateMethod, std::move(args));
}

bool TextInputChannel::FilterEvent(Ecore_Event_Key* keyDownEvent) {
  FT_LOGD("NonIMFFallback key name [%s]", keyDownEvent->keyname);
  bool handled = false;
  const char* device = ecore_device_name_get(keyDownEvent->dev);

  Ecore_IMF_Event_Key_Down ecoreKeyDownEvent;
  ecoreKeyDownEvent.keyname = keyDownEvent->keyname;
  ecoreKeyDownEvent.key = keyDownEvent->key;
  ecoreKeyDownEvent.string = keyDownEvent->string;
  ecoreKeyDownEvent.compose = keyDownEvent->compose;
  ecoreKeyDownEvent.timestamp = keyDownEvent->timestamp;
  ecoreKeyDownEvent.modifiers =
      EcoreInputModifierToEcoreIMFModifier(keyDownEvent->modifiers);
  ecoreKeyDownEvent.locks =
      EcoreInputModifierToEcoreIMFLock(keyDownEvent->modifiers);
  ecoreKeyDownEvent.dev_name = device;
  ecoreKeyDownEvent.dev_class = EoreDeviceClassToEcoreIMFDeviceClass(
      ecore_device_class_get(keyDownEvent->dev));
  ecoreKeyDownEvent.dev_subclass = EoreDeviceSubClassToEcoreIMFDeviceSubClass(
      ecore_device_subclass_get(keyDownEvent->dev));
  ecoreKeyDownEvent.keycode = keyDownEvent->keycode;

  bool isIME = strcmp(device, "ime") == 0;
  if (isIME && strcmp(keyDownEvent->key, "Select") == 0) {
    if (engine_->device_profile == "wearable") {
      in_select_mode_ = true;
    } else {
      SelectPressed(active_model_.get());
      return true;
    }
  }

  if (isIME && strcmp(keyDownEvent->key, "Left") == 0) {
    if (active_model_->MoveCursorBack()) {
      SendStateUpdate(*active_model_);
      return true;
    }
  } else if (isIME && strcmp(keyDownEvent->key, "Right") == 0) {
    if (active_model_->MoveCursorForward()) {
      SendStateUpdate(*active_model_);
      return true;
    }
  } else if (isIME && strcmp(keyDownEvent->key, "End") == 0) {
    active_model_->MoveCursorToEnd();
    SendStateUpdate(*active_model_);
    return true;
  } else if (isIME && strcmp(keyDownEvent->key, "Home") == 0) {
    active_model_->MoveCursorToBeginning();
    SendStateUpdate(*active_model_);
    return true;
  } else if (isIME && strcmp(keyDownEvent->key, "BackSpace") == 0) {
    if (active_model_->Backspace()) {
      SendStateUpdate(*active_model_);
      return true;
    }
  } else if (isIME && strcmp(keyDownEvent->key, "Delete") == 0) {
    if (active_model_->Delete()) {
      SendStateUpdate(*active_model_);
      return true;
    }
  } else {
    handled = ecore_imf_context_filter_event(
        imf_context_, ECORE_IMF_EVENT_KEY_DOWN,
        reinterpret_cast<Ecore_IMF_Event*>(&ecoreKeyDownEvent));
  }

  if (!handled && !strcmp(keyDownEvent->key, "Return")) {
    if (in_select_mode_) {
      in_select_mode_ = false;
      handled = true;
    } else {
      ecore_imf_context_reset(imf_context_);
      EnterPressed(active_model_.get());
    }
  }

  return handled;
}

void TextInputChannel::NonIMFFallback(Ecore_Event_Key* keyDownEvent) {
  FT_LOGD("NonIMFFallback key name [%s]", keyDownEvent->keyname);
  if (strcmp(keyDownEvent->key, "Left") == 0) {
    if (active_model_->MoveCursorBack()) {
      SendStateUpdate(*active_model_);
    }
  } else if (strcmp(keyDownEvent->key, "Right") == 0) {
    if (active_model_->MoveCursorForward()) {
      SendStateUpdate(*active_model_);
    }
  } else if (strcmp(keyDownEvent->key, "End") == 0) {
    active_model_->MoveCursorToEnd();
    SendStateUpdate(*active_model_);
  } else if (strcmp(keyDownEvent->key, "Home") == 0) {
    active_model_->MoveCursorToBeginning();
    SendStateUpdate(*active_model_);
  } else if (strcmp(keyDownEvent->key, "BackSpace") == 0) {
    if (active_model_->Backspace()) {
      SendStateUpdate(*active_model_);
    }
  } else if (strcmp(keyDownEvent->key, "Delete") == 0) {
    if (active_model_->Delete()) {
      SendStateUpdate(*active_model_);
    }
  } else if (strcmp(keyDownEvent->key, "Return") == 0) {
    EnterPressed(active_model_.get());
  } else if (keyDownEvent->string && strlen(keyDownEvent->string) == 1 &&
             IsASCIIPrintableKey(keyDownEvent->string[0])) {
    active_model_->AddCodePoint(keyDownEvent->string[0]);
    SendStateUpdate(*active_model_);
  }
}

void TextInputChannel::EnterPressed(flutter::TextInputModel* model) {
  if (input_type_ == kMultilineInputType) {
    model->AddCodePoint('\n');
    SendStateUpdate(*model);
  }
  auto args = std::make_unique<rapidjson::Document>(rapidjson::kArrayType);
  auto& allocator = args->GetAllocator();
  args->PushBack(client_id_, allocator);
  args->PushBack(rapidjson::Value(input_action_, allocator).Move(), allocator);

  channel_->InvokeMethod(kPerformActionMethod, std::move(args));
}

void TextInputChannel::SelectPressed(flutter::TextInputModel* model) {
  auto args = std::make_unique<rapidjson::Document>(rapidjson::kArrayType);
  auto& allocator = args->GetAllocator();
  args->PushBack(client_id_, allocator);
  args->PushBack(rapidjson::Value(input_action_, allocator).Move(), allocator);

  channel_->InvokeMethod(kPerformActionMethod, std::move(args));
}

void TextInputChannel::OnCommit(const char* str) {
  for (int i = last_preedit_string_length_; i > 0; i--) {
    active_model_->Backspace();
  }
  active_model_->AddText(str);
  SendStateUpdate(*active_model_);
  last_preedit_string_length_ = 0;
}

void TextInputChannel::OnPredit(const char* str, int cursorPos) {
  if (strcmp(str, "") == 0) {
    last_preedit_string_length_ = 0;
    return;
  }

  for (int i = last_preedit_string_length_; i > 0; i--) {
    active_model_->Backspace();
  }

  active_model_->AddText(str);
  SendStateUpdate(*active_model_);
  last_preedit_string_length_ = cursorPos;
}

void TextInputChannel::ShowSoftwareKeyboard() {
  FT_LOGD("ShowPanel() [is_software_keyboard_showing_:%d] \n",
          is_software_keyboard_showing_);
  if (imf_context_ && !is_software_keyboard_showing_) {
    is_software_keyboard_showing_ = true;
    ecore_imf_context_input_panel_show(imf_context_);
    ecore_imf_context_focus_in(imf_context_);
  }
}

void TextInputChannel::HideSoftwareKeyboard() {
  FT_LOGD("HidePanel() [is_software_keyboard_showing_:%d] \n",
          is_software_keyboard_showing_);
  if (imf_context_ && is_software_keyboard_showing_) {
    is_software_keyboard_showing_ = false;

    if (engine_->device_profile ==
        "mobile") {  // FIXME : Needs improvement on other devices.
      auto window_geometry = engine_->tizen_native_window->GetGeometry();

      if (rotation == 90 || rotation == 270) {
        engine_->SendWindowMetrics(window_geometry.h, window_geometry.w, 0);
      } else {
        engine_->SendWindowMetrics(window_geometry.w, window_geometry.h, 0);
      }
      engine_->tizen_native_window->GetTizenNativeEGLWindow()
          ->ResizeWithRotation(0, 0, window_geometry.w, window_geometry.h, 0);
      ecore_timer_add(
          0.05,
          [](void* data) -> Eina_Bool {
            Ecore_IMF_Context* imfContext = (Ecore_IMF_Context*)data;
            ecore_imf_context_reset(imfContext);
            ecore_imf_context_focus_out(imfContext);
            ecore_imf_context_input_panel_hide(imfContext);
            return ECORE_CALLBACK_CANCEL;
          },
          imf_context_);
    } else {
      ecore_imf_context_reset(imf_context_);
      ecore_imf_context_focus_out(imf_context_);
      ecore_imf_context_input_panel_hide(imf_context_);
    }
  }
}

void TextInputChannel::RegisterIMFCallback() {
  // ecore_imf_context_input_panel_enabled_set(imf_context_, false);
  ecore_imf_context_event_callback_add(imf_context_, ECORE_IMF_CALLBACK_COMMIT,
                                       CommitCallback, this);
  ecore_imf_context_event_callback_add(
      imf_context_, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, PreeditCallback, this);
  ecore_imf_context_event_callback_add(imf_context_,
                                       ECORE_IMF_CALLBACK_DELETE_SURROUNDING,
                                       DeleteSurroundingCallback, this);
  ecore_imf_context_event_callback_add(imf_context_,
                                       ECORE_IMF_CALLBACK_PRIVATE_COMMAND_SEND,
                                       PrivateCommandCallback, this);
  ecore_imf_context_input_panel_event_callback_add(
      imf_context_, ECORE_IMF_INPUT_PANEL_STATE_EVENT,
      InputPanelStateChangedCallback, this);
  ecore_imf_context_input_panel_event_callback_add(
      imf_context_, ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT,
      InputPanelGeometryChangedCallback, this);
  ecore_imf_context_retrieve_surrounding_callback_set(
      imf_context_, RetrieveSurroundingCallback, this);

  // These APIs have to be set when IMF's setting status is changed.
  ecore_imf_context_autocapital_type_set(imf_context_,
                                         ECORE_IMF_AUTOCAPITAL_TYPE_NONE);
  ecore_imf_context_prediction_allow_set(imf_context_, EINA_FALSE);
  ecore_imf_context_input_panel_layout_set(imf_context_,
                                           ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL);
  ecore_imf_context_input_panel_return_key_type_set(
      imf_context_, ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT);
  ecore_imf_context_input_panel_layout_variation_set(imf_context_, 0);
  ecore_imf_context_input_panel_language_set(
      imf_context_, ECORE_IMF_INPUT_PANEL_LANG_AUTOMATIC);
}

void TextInputChannel::UnregisterIMFCallback() {
  ecore_imf_context_event_callback_del(imf_context_, ECORE_IMF_CALLBACK_COMMIT,
                                       CommitCallback);
  ecore_imf_context_event_callback_del(
      imf_context_, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, PreeditCallback);
  ecore_imf_context_event_callback_del(imf_context_,
                                       ECORE_IMF_CALLBACK_DELETE_SURROUNDING,
                                       DeleteSurroundingCallback);
  ecore_imf_context_event_callback_del(imf_context_,
                                       ECORE_IMF_CALLBACK_PRIVATE_COMMAND_SEND,
                                       PrivateCommandCallback);
  ecore_imf_context_input_panel_event_callback_del(
      imf_context_, ECORE_IMF_INPUT_PANEL_STATE_EVENT,
      InputPanelStateChangedCallback);
  ecore_imf_context_input_panel_event_callback_del(
      imf_context_, ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT,
      InputPanelGeometryChangedCallback);
}
