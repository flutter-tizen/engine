// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "text_input_channel.h"

#include <Ecore.h>

#include "flutter/shell/platform/common/json_method_codec.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/tizen_log.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "flutter/textinput";

constexpr char kSetEditingStateMethod[] = "TextInput.setEditingState";
constexpr char kClearClientMethod[] = "TextInput.clearClient";
constexpr char kSetClientMethod[] = "TextInput.setClient";
constexpr char kShowMethod[] = "TextInput.show";
constexpr char kHideMethod[] = "TextInput.hide";
constexpr char kMultilineInputType[] = "TextInputType.multiline";
constexpr char kUpdateEditingStateMethod[] =
    "TextInputClient.updateEditingState";
constexpr char kPerformActionMethod[] = "TextInputClient.performAction";
constexpr char kSetPlatformViewClient[] = "TextInput.setPlatformViewClient";
constexpr char kTextInputAction[] = "inputAction";
constexpr char kTextInputType[] = "inputType";
constexpr char kTextInputTypeName[] = "name";
constexpr char kComposingBaseKey[] = "composingBase";
constexpr char kComposingExtentKey[] = "composingExtent";
constexpr char kSelectionAffinityKey[] = "selectionAffinity";
constexpr char kAffinityDownstream[] = "TextAffinity.downstream";
constexpr char kSelectionBaseKey[] = "selectionBase";
constexpr char kSelectionExtentKey[] = "selectionExtent";
constexpr char kSelectionIsDirectionalKey[] = "selectionIsDirectional";
constexpr char kTextKey[] = "text";
constexpr char kBadArgumentError[] = "Bad Arguments";
constexpr char kInternalConsistencyError[] = "Internal Consistency Error";

}  // namespace

static const char* GetImfMethod() {
  Eina_List* modules;

  modules = ecore_imf_context_available_ids_get();
  if (!modules) {
    return nullptr;
  }

  void* module;
  EINA_LIST_FREE(modules, module) { return static_cast<const char*>(module); }

  return nullptr;
}

static bool IsASCIIPrintableKey(char c) {
  if (c >= 32 && c <= 126) {
    return true;
  }
  return false;
}

static bool TextInputTypeToEcoreIMFInputPanelLayout(
    std::string text_input_type,
    Ecore_IMF_Input_Panel_Layout* panel_layout) {
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
    FT_LOGW(
        "Actual requested text input type is [%s], but select "
        "TextInputType.text as fallback type",
        text_input_type.c_str());
    *panel_layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;
  } else {
    return false;
  }
  return true;
}

void TextInputChannel::CommitCallback(void* data,
                                      Ecore_IMF_Context* ctx,
                                      void* event_info) {
  TextInputChannel* self = static_cast<TextInputChannel*>(data);
  if (!self) {
    return;
  }
  char* str = static_cast<char*>(event_info);
  if (self->engine_ && self->engine_->platform_view_channel &&
      self->engine_->platform_view_channel->CurrentFocusedViewId() > -1) {
    self->engine_->platform_view_channel->DispatchCompositionEndEvent(str);
    return;
  }

  self->OnCommit(str);
}

void TextInputChannel::PreeditCallback(void* data,
                                       Ecore_IMF_Context* ctx,
                                       void* event_info) {
  TextInputChannel* self = static_cast<TextInputChannel*>(data);
  if (!self) {
    return;
  }

  char* str = nullptr;
  int cursor_pos;
  ecore_imf_context_preedit_string_get(ctx, &str, &cursor_pos);
  if (str) {
    if (self->engine_ && self->engine_->platform_view_channel &&
        self->engine_->platform_view_channel->CurrentFocusedViewId() > -1) {
      self->OnPreeditForPlatformView(str, cursor_pos);
    } else {
      self->OnPreedit(str, cursor_pos);
    }
    free(str);
  }
}

void TextInputChannel::PrivateCommandCallback(void* data,
                                              Ecore_IMF_Context* ctx,
                                              void* event_info) {
  FT_UNIMPLEMENTED();
}

void TextInputChannel::DeleteSurroundingCallback(void* data,
                                                 Ecore_IMF_Context* ctx,
                                                 void* event_info) {
  FT_UNIMPLEMENTED();
}

void TextInputChannel::InputPanelStateChangedCallback(
    void* data,
    Ecore_IMF_Context* context,
    int value) {
  FT_LOGI("Change input panel state[%d]", value);
  if (!data) {
    FT_LOGW("No Data");
    return;
  }
  TextInputChannel* self = static_cast<TextInputChannel*>(data);
  switch (value) {
    case ECORE_IMF_INPUT_PANEL_STATE_SHOW:
      self->SetSoftwareKeyboardShowing();
      break;
    case ECORE_IMF_INPUT_PANEL_STATE_HIDE:
      self->HideSoftwareKeyboard();  // FIXME: Fallback for HW back-key
      break;
    case ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW:
      break;
    default:
      break;
  }
}

void TextInputChannel::InputPanelGeometryChangedCallback(
    void* data,
    Ecore_IMF_Context* context,
    int value) {
  if (!data) {
    FT_LOGW("No Data");
    return;
  }
  TextInputChannel* self = static_cast<TextInputChannel*>(data);
  ecore_imf_context_input_panel_geometry_get(
      self->imf_context_, &self->current_keyboard_geometry_.x,
      &self->current_keyboard_geometry_.y, &self->current_keyboard_geometry_.w,
      &self->current_keyboard_geometry_.h);

  FT_LOGI(
      "Current keyboard geometry x:[%d] y:[%d] w:[%d] h:[%d]",
      self->current_keyboard_geometry_.x, self->current_keyboard_geometry_.y,
      self->current_keyboard_geometry_.w, self->current_keyboard_geometry_.h);
}

Eina_Bool TextInputChannel::RetrieveSurroundingCallback(void* data,
                                                        Ecore_IMF_Context* ctx,
                                                        char** text,
                                                        int* cursor_pos) {
  FT_UNIMPLEMENTED();
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

TextInputChannel::TextInputChannel(BinaryMessenger* messenger,
                                   FlutterTizenEngine* engine)
    : channel_(std::make_unique<MethodChannel<rapidjson::Document>>(
          messenger,
          kChannelName,
          &JsonMethodCodec::GetInstance())),
      engine_(engine) {
  channel_->SetMethodCallHandler(
      [this](const MethodCall<rapidjson::Document>& call,
             std::unique_ptr<MethodResult<rapidjson::Document>> result) {
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
    ecore_imf_context_client_window_set(
        imf_context_,
        reinterpret_cast<void*>(engine_->renderer->GetWindowId()));
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
  if (!FilterEvent(key) && !have_preedit_) {
    NonIMFFallback(key);
  }
}

void TextInputChannel::HandleMethodCall(
    const MethodCall<rapidjson::Document>& method_call,
    std::unique_ptr<MethodResult<rapidjson::Document>> result) {
  const std::string& method = method_call.method_name();

  FT_LOGI("Handle Method : %s", method.data());

  if (method.compare(kShowMethod) == 0) {
    ShowSoftwareKeyboard();
  } else if (method.compare(kHideMethod) == 0) {
    HideSoftwareKeyboard();
  } else if (method.compare(kSetPlatformViewClient) == 0) {
    FT_UNIMPLEMENTED();
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
    active_model_ = std::make_unique<TextInputModel>();
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
    active_model_->SetText(text->value.GetString());
    active_model_->SetSelection(TextRange(selection_base->value.GetInt(),
                                          selection_extent->value.GetInt()));
  } else {
    result->NotImplemented();
    return;
  }
  // All error conditions return early, so if nothing has gone wrong indicate
  // success.
  result->Success();
}

void TextInputChannel::SendStateUpdate(const TextInputModel& model) {
  auto args = std::make_unique<rapidjson::Document>(rapidjson::kArrayType);
  auto& allocator = args->GetAllocator();
  args->PushBack(client_id_, allocator);

  TextRange selection = model.selection();
  rapidjson::Value editing_state(rapidjson::kObjectType);
  editing_state.AddMember(kComposingBaseKey, -1, allocator);
  editing_state.AddMember(kComposingExtentKey, -1, allocator);
  editing_state.AddMember(kSelectionAffinityKey, kAffinityDownstream,
                          allocator);
  editing_state.AddMember(kSelectionBaseKey, selection.base(), allocator);
  editing_state.AddMember(kSelectionExtentKey, selection.extent(), allocator);
  editing_state.AddMember(kSelectionIsDirectionalKey, false, allocator);
  editing_state.AddMember(
      kTextKey, rapidjson::Value(model.GetText(), allocator).Move(), allocator);
  args->PushBack(editing_state, allocator);

  FT_LOGI("Send text[%s]", model.GetText().data());
  channel_->InvokeMethod(kUpdateEditingStateMethod, std::move(args));
}

bool TextInputChannel::FilterEvent(Ecore_Event_Key* event) {
  bool handled = false;

#ifdef WEARABLE_PROFILE
  // Hardware keyboard not supported on watches.
  bool is_ime = true;
#else
  bool is_ime = strcmp(ecore_device_name_get(event->dev), "ime") == 0;
#endif

  Ecore_IMF_Event_Key_Down imf_event;
  imf_event.keyname = event->keyname;
  imf_event.key = event->key;
  imf_event.string = event->string;
  imf_event.compose = event->compose;
  imf_event.timestamp = event->timestamp;
  imf_event.modifiers = EcoreInputModifierToEcoreIMFModifier(event->modifiers);
  imf_event.locks = EcoreInputModifierToEcoreIMFLock(event->modifiers);
  imf_event.dev_name = is_ime ? "ime" : "";
  imf_event.keycode = event->keycode;

  if (is_ime && strcmp(event->key, "Select") == 0) {
    if (engine_->device_profile == DeviceProfile::kWearable) {
      // FIXME: for wearable
      in_select_mode_ = true;
      FT_LOGI("Set select mode[true]");
    }
  }

  if (is_ime) {
    if (!strcmp(event->key, "Left") || !strcmp(event->key, "Right") ||
        !strcmp(event->key, "Up") || !strcmp(event->key, "Down") ||
        !strcmp(event->key, "End") || !strcmp(event->key, "Home") ||
        !strcmp(event->key, "BackSpace") || !strcmp(event->key, "Delete") ||
        (!strcmp(event->key, "Select") && !in_select_mode_)) {
      // Force redirect to fallback!(especially on TV)
      // If you don't do this, it affects the input panel.
      // For example, when the left key of the input panel is pressed, the focus
      // of the input panel is shifted to left!
      // What we want is to move only the cursor on the text editor.
      ResetCurrentContext();
      FT_LOGW("Force redirect IME key-event[%s] to fallback", event->keyname);
      return false;
    }
  }

  handled = ecore_imf_context_filter_event(
      imf_context_, ECORE_IMF_EVENT_KEY_DOWN,
      reinterpret_cast<Ecore_IMF_Event*>(&imf_event));

  if (handled) {
    last_handled_ecore_event_keyname_ = event->keyname;
  }

  if (!handled && !strcmp(event->key, "Return") && in_select_mode_ &&
      engine_->device_profile == DeviceProfile::kWearable) {
    in_select_mode_ = false;
    handled = true;
    FT_LOGI("Set select mode[false]");
  }

  return handled;
}

void TextInputChannel::NonIMFFallback(Ecore_Event_Key* event) {
  // For mobile, fix me!
  if (engine_->device_profile == DeviceProfile::kMobile &&
      edit_status_ == EditStatus::kPreeditEnd) {
    SetEditStatus(EditStatus::kNone);
    FT_LOGW("Ignore key-event[%s]!", event->keyname);
    return;
  }

  bool select = !strcmp(event->key, "Select");
  bool is_filtered = true;
  if (!strcmp(event->key, "Left")) {
    if (active_model_ && active_model_->MoveCursorBack()) {
      SendStateUpdate(*active_model_);
    }
  } else if (!strcmp(event->key, "Right")) {
    if (active_model_ && active_model_->MoveCursorForward()) {
      SendStateUpdate(*active_model_);
    }
  } else if (!strcmp(event->key, "End")) {
    if (active_model_) {
      active_model_->MoveCursorToEnd();
      SendStateUpdate(*active_model_);
    }
  } else if (!strcmp(event->key, "Home")) {
    if (active_model_) {
      active_model_->MoveCursorToBeginning();
      SendStateUpdate(*active_model_);
    }
  } else if (!strcmp(event->key, "BackSpace")) {
    if (active_model_ && active_model_->Backspace()) {
      SendStateUpdate(*active_model_);
    }
  } else if (!strcmp(event->key, "Delete")) {
    if (active_model_ && active_model_->Delete()) {
      SendStateUpdate(*active_model_);
    }
  } else if (!strcmp(event->key, "Return") || (select && !in_select_mode_)) {
    if (active_model_) {
      EnterPressed(active_model_.get(), select);
    }
  } else if (event->string && strlen(event->string) == 1 &&
             IsASCIIPrintableKey(event->string[0])) {
    if (active_model_) {
      active_model_->AddCodePoint(event->string[0]);
      SendStateUpdate(*active_model_);
    }
  } else {
    is_filtered = false;
  }
  if (!active_model_ && is_filtered) {
    engine_->platform_view_channel->SendKeyEvent(event, true);
  }
  SetEditStatus(EditStatus::kNone);
}

void TextInputChannel::EnterPressed(TextInputModel* model, bool select) {
  if (!select && input_type_ == kMultilineInputType) {
    model->AddCodePoint('\n');
    SendStateUpdate(*model);
  }
  auto args = std::make_unique<rapidjson::Document>(rapidjson::kArrayType);
  auto& allocator = args->GetAllocator();
  args->PushBack(client_id_, allocator);
  args->PushBack(rapidjson::Value(input_action_, allocator).Move(), allocator);

  channel_->InvokeMethod(kPerformActionMethod, std::move(args));
}

void TextInputChannel::OnCommit(std::string str) {
  FT_LOGI("OnCommit str[%s]", str.data());
  SetEditStatus(EditStatus::kCommit);

  ConsumeLastPreedit();

  active_model_->AddText(str);

  SendStateUpdate(*active_model_);
  SetEditStatus(EditStatus::kNone);
}

void TextInputChannel::OnPreedit(std::string str, int cursor_pos) {
  FT_LOGI("OnPreedit str[%s], cursor_pos[%d]", str.data(), cursor_pos);
  SetEditStatus(EditStatus::kPreeditStart);
  if (str.compare("") == 0) {
    SetEditStatus(EditStatus::kPreeditEnd);
  }

  if (edit_status_ == EditStatus::kPreeditStart ||
      (edit_status_ == EditStatus::kPreeditEnd &&
       // For tv, fix me
       last_handled_ecore_event_keyname_.compare("Return") != 0)) {
    last_handled_ecore_event_keyname_ = "";
    ConsumeLastPreedit();
  }

  have_preedit_ = false;
  if (edit_status_ == EditStatus::kPreeditStart) {
    preedit_start_pos_ = active_model_->selection().base();
    active_model_->AddText(str);
    preedit_end_pos_ = active_model_->selection().base();
    have_preedit_ = true;
    SendStateUpdate(*active_model_);
    FT_LOGI("preedit start pos[%d], preedit end pos[%d]", preedit_start_pos_,
            preedit_end_pos_);
  }
}
void TextInputChannel::OnPreeditForPlatformView(std::string str,
                                                int cursor_pos) {
  FT_LOGI("OnPreeditForPlatformView str[%s], cursor_pos[%d]", str.data(),
          cursor_pos);

  SetEditStatus(EditStatus::kPreeditStart);
  if (str.compare("") == 0) {
    SetEditStatus(EditStatus::kPreeditEnd);
  }

  if (edit_status_ == EditStatus::kPreeditStart ||
      (edit_status_ == EditStatus::kPreeditEnd &&
       // For tv, fix me
       last_handled_ecore_event_keyname_.compare("Return") != 0)) {
    last_handled_ecore_event_keyname_ = "";
  }

  have_preedit_ = false;
  if (edit_status_ == EditStatus::kPreeditStart) {
    engine_->platform_view_channel->DispatchCompositionUpdateEvent(str);
  }
}

void TextInputChannel::ShowSoftwareKeyboard() {
  if (imf_context_ && !is_software_keyboard_showing_) {
    is_software_keyboard_showing_ = true;
    Ecore_IMF_Input_Panel_Layout panel_layout;
    if (TextInputTypeToEcoreIMFInputPanelLayout(input_type_, &panel_layout)) {
      ecore_imf_context_input_panel_layout_set(imf_context_, panel_layout);
    }
    ecore_imf_context_input_panel_show(imf_context_);
    ecore_imf_context_focus_in(imf_context_);
  }
}

void TextInputChannel::HideSoftwareKeyboard() {
  if (imf_context_ && is_software_keyboard_showing_) {
    is_software_keyboard_showing_ = false;
    ResetCurrentContext();
    ecore_imf_context_focus_out(imf_context_);
  }
}

void TextInputChannel::SetEditStatus(EditStatus edit_status) {
  edit_status_ = edit_status;
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

void TextInputChannel::ConsumeLastPreedit() {
  if (have_preedit_) {
    std::string before = active_model_->GetText();
    int count = preedit_end_pos_ - preedit_start_pos_;
    active_model_->DeleteSurrounding(-count, count);
    std::string after = active_model_->GetText();
    FT_LOGI("Consume last preedit count:[%d] text:[%s] -> [%s]", count,
            before.data(), after.data());
    SendStateUpdate(*active_model_);
  }
  have_preedit_ = false;
  preedit_end_pos_ = 0;
  preedit_start_pos_ = 0;
}

void TextInputChannel::ResetCurrentContext() {
  SetEditStatus(EditStatus::kNone);
  ecore_imf_context_reset(imf_context_);
  preedit_start_pos_ = 0;
  preedit_end_pos_ = 0;
  have_preedit_ = false;
}

}  // namespace flutter
