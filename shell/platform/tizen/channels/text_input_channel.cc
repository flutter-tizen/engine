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

bool IsASCIIPrintableKey(char c) {
  if (c >= 32 && c <= 126) {
    return true;
  }
  return false;
}
}  // namespace

TextInputChannel::TextInputChannel(BinaryMessenger* messenger,
                                   FlutterTizenEngine* engine)
    : channel_(std::make_unique<MethodChannel<rapidjson::Document>>(
          messenger,
          kChannelName,
          &JsonMethodCodec::GetInstance())),
      input_method_context_(std::make_unique<TizenInputMethodContext>(engine)) {
  channel_->SetMethodCallHandler(
      [this](const MethodCall<rapidjson::Document>& call,
             std::unique_ptr<MethodResult<rapidjson::Document>> result) {
        HandleMethodCall(call, std::move(result));
      });

  // Set input method callbacks
  input_method_context_->SetOnCommitCallback([this](std::string str) -> void {
    FT_LOGI("OnCommit str[%s]", str.data());
    text_editing_context_.edit_status_ = EditStatus::kCommit;
    ConsumeLastPreedit();
    active_model_->AddText(str);
    SendStateUpdate(*active_model_);
  });

  input_method_context_->SetOnPreeditCallback(
      [this](std::string str, int cursor_pos) -> void {
        text_editing_context_.edit_status_ = EditStatus::kPreeditStart;
        if (str.compare("") == 0) {
          text_editing_context_.edit_status_ = EditStatus::kPreeditEnd;
        }

        if (text_editing_context_.edit_status_ == EditStatus::kPreeditStart ||
            (text_editing_context_.edit_status_ == EditStatus::kPreeditEnd &&
             // For tv, fix me
             text_editing_context_.last_handled_ecore_event_keyname_.compare(
                 "Return") != 0)) {
          text_editing_context_.last_handled_ecore_event_keyname_ = "";
          ConsumeLastPreedit();
        }

        text_editing_context_.has_preedit_ = false;
        if (text_editing_context_.edit_status_ == EditStatus::kPreeditStart) {
          text_editing_context_.preedit_start_pos_ =
              active_model_->selection().base();
          active_model_->AddText(str);
          text_editing_context_.preedit_end_pos_ =
              active_model_->selection().base();
          text_editing_context_.has_preedit_ = true;
          SendStateUpdate(*active_model_);
          FT_LOGI("preedit start pos[%d], preedit end pos[%d]",
                  text_editing_context_.preedit_start_pos_,
                  text_editing_context_.preedit_end_pos_);
        }
      });

  input_method_context_->SetOnInputPannelStateChangedCallback(
      [this](int state) {
        if (state == ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
          // Fallback for HW back-key
          input_method_context_->HideInputPannel();
          input_method_context_->ResetInputMethodContext();
          ResetTextEditingContext();
          is_software_keyboard_showing_ = false;
        } else {
          is_software_keyboard_showing_ = true;
        }
      });
}

TextInputChannel::~TextInputChannel() {}

bool TextInputChannel::SendKeyEvent(Ecore_Event_Key* key, bool is_down) {
  if (!active_model_ || !is_down) {
    return false;
  }

  if (!FilterEvent(key) && !text_editing_context_.has_preedit_) {
    HandleUnfilteredEvent(key);
  }

  return true;
}

void TextInputChannel::HandleMethodCall(
    const MethodCall<rapidjson::Document>& method_call,
    std::unique_ptr<MethodResult<rapidjson::Document>> result) {
  const std::string& method = method_call.method_name();

  FT_LOGI("Handle Method : %s", method.data());

  if (method.compare(kShowMethod) == 0) {
    input_method_context_->ShowInputPannel();
  } else if (method.compare(kHideMethod) == 0) {
    input_method_context_->HideInputPannel();
    input_method_context_->ResetInputMethodContext();
    ResetTextEditingContext();
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
        input_method_context_->SetInputPannelLayout(input_type_);
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

#if defined(__X64_SHELL__)
  bool is_ime = false;
#elif defined(WEARABLE_PROFILE)
  // Hardware keyboard not supported on watches.
  bool is_ime = true;
  // FIXME: Only for wearable.
  if (is_ime && strcmp(event->key, "Select") == 0) {
    text_editing_context_.is_in_select_mode_ = true;
    FT_LOGI("Set select mode[true]");
  }
#else
  bool is_ime = strcmp(ecore_device_name_get(event->dev), "ime") == 0;
#endif

  if (ShouldNotFilterEvent(event->key, is_ime)) {
    ResetTextEditingContext();
    input_method_context_->ResetInputMethodContext();
    FT_LOGW("Force redirect IME key-event[%s] to fallback", event->keyname);
    return false;
  }

  handled = input_method_context_->FilterEvent(event, is_ime ? "ime" : "");

  if (handled) {
    text_editing_context_.last_handled_ecore_event_keyname_ = event->keyname;
  }

#ifdef WEARABLE_PROFILE
  if (!handled && !strcmp(event->key, "Return") &&
      text_editing_context_.is_in_select_mode_) {
    text_editing_context_.is_in_select_mode_ = false;
    handled = true;
    FT_LOGI("Set select mode[false]");
  }
#endif

  return handled;
}

void TextInputChannel::HandleUnfilteredEvent(Ecore_Event_Key* event) {
#ifdef MOBILE_PROFILE
  // FIXME: Only for mobile.
  if (text_editing_context_.edit_status_ == EditStatus::kPreeditEnd) {
    FT_LOGW("Ignore key-event[%s]!", event->keyname);
    ResetTextEditingContext();
    return;
  }
#endif

  bool select = !strcmp(event->key, "Select");
  bool is_filtered = true;
  if (!strcmp(event->key, "Left")) {
    if (active_model_->MoveCursorBack()) {
      SendStateUpdate(*active_model_);
    }
  } else if (!strcmp(event->key, "Right")) {
    if (active_model_->MoveCursorForward()) {
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
    if (active_model_->Backspace()) {
      SendStateUpdate(*active_model_);
    }
  } else if (!strcmp(event->key, "Delete")) {
    if (active_model_->Delete()) {
      SendStateUpdate(*active_model_);
    }
  } else if (!strcmp(event->key, "Return") ||
             (select && !text_editing_context_.is_in_select_mode_)) {
    EnterPressed(active_model_.get(), select);

  } else if (event->string && strlen(event->string) == 1 &&
             IsASCIIPrintableKey(event->string[0])) {
    active_model_->AddCodePoint(event->string[0]);
    SendStateUpdate(*active_model_);
  } else {
    is_filtered = false;
  }

  text_editing_context_.edit_status_ = EditStatus::kNone;
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

void TextInputChannel::ConsumeLastPreedit() {
  if (text_editing_context_.has_preedit_) {
    std::string before = active_model_->GetText();
    int count = text_editing_context_.preedit_end_pos_ -
                text_editing_context_.preedit_start_pos_;
    active_model_->DeleteSurrounding(-count, count);
    std::string after = active_model_->GetText();
    FT_LOGI("Consume last preedit count:[%d] text:[%s] -> [%s]", count,
            before.data(), after.data());
    SendStateUpdate(*active_model_);
  }
  text_editing_context_.has_preedit_ = false;
  text_editing_context_.preedit_end_pos_ = 0;
  text_editing_context_.preedit_start_pos_ = 0;
}

bool TextInputChannel::ShouldNotFilterEvent(std::string key, bool is_ime) {
  // Force redirect to HandleUnfilteredEvent(especially on TV)
  // If you don't do this, it will affects the input panel.
  // For example, when the left key of the input panel is pressed, the focus
  // of the input panel is shifted to left!
  // What we want is to move only the cursor on the text editor.
  if (is_ime && !text_editing_context_.is_in_select_mode_ &&
      (key == "Left" || key == "Right" || key == "Up" || key == "Down" ||
       key == "End" || key == "Home" || key == "BackSpace" || key == "Delete" ||
       key == "Select")) {
    return true;
  }
  return false;
}
}  // namespace flutter
