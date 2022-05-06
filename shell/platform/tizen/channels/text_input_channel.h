// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TEXT_INPUT_CHANNEL_H_
#define EMBEDDER_TEXT_INPUT_CHANNEL_H_

#define EFL_BETA_API_SUPPORT
#include <Ecore_IMF.h>
#include <Ecore_Input.h>

#include <memory>
#include <string>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/method_channel.h"
#include "flutter/shell/platform/common/text_input_model.h"
#include "flutter/shell/platform/tizen/tizen_input_method_context.h"
#include "rapidjson/document.h"

namespace flutter {

class TextInputChannel {
 public:
  explicit TextInputChannel(
      BinaryMessenger* messenger,
      std::unique_ptr<TizenInputMethodContext> input_method_context);
  virtual ~TextInputChannel();

  bool IsSoftwareKeyboardShowing() { return is_software_keyboard_showing_; }

  void OnComposeBegin();

  void OnComposeChanged(std::string str, int cursor_pos);

  void OnComposeEnd();

  void OnCommit(std::string str);

  void OnInputPanelStateChanged(int state);

  bool SendKeyEvent(Ecore_Event_Key* key, bool is_down);

 private:
  // Called when a method is called on |channel_|;
  void HandleMethodCall(
      const MethodCall<rapidjson::Document>& method_call,
      std::unique_ptr<MethodResult<rapidjson::Document>> result);

  // Sends the current state of the given model to the Flutter engine.
  void SendStateUpdate(const TextInputModel& model);

  bool FilterEvent(Ecore_Event_Key* event, bool is_down);

  void HandleUnfilteredEvent(Ecore_Event_Key* event);

  // Sends an action triggered by the Enter key to the Flutter engine.
  void EnterPressed(TextInputModel* model, bool select);

  void Reset();

  bool ShouldNotFilterEvent(std::string key, bool is_ime);

  // The MethodChannel used for communication with the Flutter engine.
  std::unique_ptr<MethodChannel<rapidjson::Document>> channel_;

  // The active model. nullptr if not set.
  std::unique_ptr<TextInputModel> active_model_;

  // The Tizen input method context. nullptr if not set.
  std::unique_ptr<TizenInputMethodContext> input_method_context_;

  // The active client id.
  int client_id_ = 0;

  // A flag indicating whether the software keyboard is being shown
  bool is_software_keyboard_showing_ = false;

  // An action requested by the user on the input client. See available options:
  // https://api.flutter.dev/flutter/services/TextInputAction-class.html
  std::string input_action_;

  // Keyboard type of the client. See available options:
  // https://api.flutter.dev/flutter/services/TextInputType-class.html
  std::string input_type_;

  bool is_in_select_mode_ = false;
};

}  // namespace flutter

#endif  // EMBEDDER_TEXT_INPUT_CHANNEL_H_
