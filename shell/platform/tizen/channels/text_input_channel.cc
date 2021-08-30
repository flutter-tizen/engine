// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "text_input_channel.h"

#include <Ecore.h>

#include "flutter/shell/platform/common/json_method_codec.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/logger.h"

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

TextInputChannel::TextInputChannel(
    BinaryMessenger* messenger,
    std::unique_ptr<TizenInputMethodContext> input_method_context)
    : channel_(std::make_unique<MethodChannel<rapidjson::Document>>(
          messenger,
          kChannelName,
          &JsonMethodCodec::GetInstance())),
      input_method_context_(std::move(input_method_context)) {
  channel_->SetMethodCallHandler(
      [this](const MethodCall<rapidjson::Document>& call,
             std::unique_ptr<MethodResult<rapidjson::Document>> result) {
        HandleMethodCall(call, std::move(result));
      });

  // Set input method callbacks.
  input_method_context_->SetOnPreeditStart([this]() {
    FT_LOG(Debug) << "onPreeditStart";
    text_editing_context_.edit_status_ = EditStatus::kPreeditStart;
    active_model_->BeginComposing();
  });

  input_method_context_->SetOnPreeditChanged(
      [this](std::string str, int cursor_pos) -> void {
        FT_LOG(Debug) << "onPreedit: str[" << str << "] cursor_pos["
                      << cursor_pos << "]";
        if (str == "") {
          // Enter pre-edit end stage.
          return;
        }
        active_model_->UpdateComposingText(str);
        SendStateUpdate(*active_model_);
      });

  input_method_context_->SetOnPreeditEnd([this]() {
    text_editing_context_.edit_status_ = EditStatus::kPreeditEnd;
    FT_LOG(Debug) << "onPreeditEnd";

    // Delete preedit-string, it will be committed.
    int count = active_model_->composing_range().extent() -
                active_model_->composing_range().base();

    active_model_->CommitComposing();
    active_model_->EndComposing();

    active_model_->DeleteSurrounding(-count, count);

    SendStateUpdate(*active_model_);
  });

  input_method_context_->SetOnCommit([this](std::string str) -> void {
    FT_LOG(Debug) << "OnCommit: str[" << str << "]";
    text_editing_context_.edit_status_ = EditStatus::kCommit;
    active_model_->AddText(str);
    if (active_model_->composing()) {
      active_model_->CommitComposing();
      active_model_->EndComposing();
    }
    SendStateUpdate(*active_model_);
  });

  input_method_context_->SetOnInputPannelStateChanged([this](int state) {
    if (state == ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
      // Fallback for HW back-key.
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

  if (!FilterEvent(key)) {
    HandleUnfilteredEvent(key);
  }

  return true;
}

void TextInputChannel::HandleMethodCall(
    const MethodCall<rapidjson::Document>& method_call,
    std::unique_ptr<MethodResult<rapidjson::Document>> result) {
  const std::string& method = method_call.method_name();
  FT_LOG(Debug) << "method: " << method;

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
    input_method_context_->ResetInputMethodContext();
    ResetTextEditingContext();

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
    auto selection_base_value = selection_base->value.GetInt();
    auto selection_extent_value = selection_extent->value.GetInt();

    active_model_->SetText(text->value.GetString());
    active_model_->SetSelection(
        TextRange(selection_base_value, selection_extent_value));

    auto composing_base = args.FindMember(kComposingBaseKey);
    auto composing_extent = args.FindMember(kComposingBaseKey);
    auto composing_base_value = composing_base != args.MemberEnd()
                                    ? composing_base->value.GetInt()
                                    : -1;
    auto composing_extent_value = composing_extent != args.MemberEnd()
                                      ? composing_extent->value.GetInt()
                                      : -1;

    if (composing_base_value == -1 && composing_extent_value == -1) {
      active_model_->EndComposing();
    } else {
      size_t composing_start =
          std::min(composing_base_value, composing_extent_value);
      size_t cursor_offset = selection_base_value - composing_start;

      active_model_->SetComposingRange(
          flutter::TextRange(composing_base_value, composing_extent_value),
          cursor_offset);
    }
    SendStateUpdate(*active_model_);
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
  int composing_base =
      active_model_->composing() ? active_model_->composing_range().base() : -1;
  int composing_extent = active_model_->composing()
                             ? active_model_->composing_range().extent()
                             : -1;

  editing_state.AddMember(kComposingBaseKey, composing_base, allocator);
  editing_state.AddMember(kComposingExtentKey, composing_extent, allocator);
  editing_state.AddMember(kSelectionAffinityKey, kAffinityDownstream,
                          allocator);
  editing_state.AddMember(kSelectionBaseKey, selection.base(), allocator);
  editing_state.AddMember(kSelectionExtentKey, selection.extent(), allocator);
  editing_state.AddMember(kSelectionIsDirectionalKey, false, allocator);
  editing_state.AddMember(
      kTextKey, rapidjson::Value(model.GetText(), allocator).Move(), allocator);
  args->PushBack(editing_state, allocator);

  FT_LOG(Debug) << "Send text:[" << model.GetText() << "]";
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
    FT_LOG(Debug) << "Entering select mode.";
  }
#else
  bool is_ime = strcmp(ecore_device_name_get(event->dev), "ime") == 0;
#endif

  if (ShouldNotFilterEvent(event->key, is_ime)) {
    ResetTextEditingContext();
    input_method_context_->ResetInputMethodContext();
    FT_LOG(Info) << "Force redirect an IME key event: " << event->keyname;
    return false;
  }

  handled = input_method_context_->FilterEvent(event, is_ime ? "ime" : "");

#ifdef WEARABLE_PROFILE
  if (!handled && !strcmp(event->key, "Return") &&
      text_editing_context_.is_in_select_mode_) {
    text_editing_context_.is_in_select_mode_ = false;
    handled = true;
    FT_LOG(Debug) << "Leaving select mode.";
  }
#endif

  return handled;
}

void TextInputChannel::HandleUnfilteredEvent(Ecore_Event_Key* event) {
  text_editing_context_.edit_status_ = EditStatus::kNone;
#ifdef MOBILE_PROFILE
  // FIXME: Only for mobile.
  if (text_editing_context_.edit_status_ == EditStatus::kPreeditEnd) {
    FT_LOG(Debug) << "Ignore a key event: " << event->keyname;
    ResetTextEditingContext();
    return;
  }
#endif
  bool select = !strcmp(event->key, "Select");
  bool shift = event->modifiers & ECORE_SHIFT;
  bool needs_update = false;
  std::string key = event->key;

  if (key == "Left") {
    if (shift) {
      TextRange selection = active_model_->selection();
      needs_update = active_model_->SetSelection(
          TextRange(selection.base(), selection.extent() - 1));
    } else {
      needs_update = active_model_->MoveCursorBack();
    }
  } else if (key == "Right") {
    if (shift) {
      TextRange selection = active_model_->selection();
      needs_update = active_model_->SetSelection(
          TextRange(selection.base(), selection.extent() + 1));
    } else {
      needs_update = active_model_->MoveCursorForward();
    }
  } else if (key == "End") {
    if (shift) {
      needs_update = active_model_->SelectToEnd();
    } else {
      needs_update = active_model_->MoveCursorToEnd();
    }
  } else if (key == "Home") {
    if (shift) {
      needs_update = active_model_->SelectToBeginning();
    } else {
      needs_update = active_model_->MoveCursorToBeginning();
    }
  } else if (key == "BackSpace") {
    needs_update = active_model_->Backspace();
  } else if (key == "Delete") {
    needs_update = active_model_->Delete();
  } else if (event->string && strlen(event->string) == 1 &&
             IsASCIIPrintableKey(event->string[0])) {
    active_model_->AddCodePoint(event->string[0]);
    needs_update = true;
  } else if (key == "Return" ||
             (select && !text_editing_context_.is_in_select_mode_)) {
    EnterPressed(active_model_.get(), select);
    return;
  } else {
    FT_LOG(Warn) << "Key[" << key << "] is unhandled.";
  }

  if (needs_update) {
    SendStateUpdate(*active_model_);
  }
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
