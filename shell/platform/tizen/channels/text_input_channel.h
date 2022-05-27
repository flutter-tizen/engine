// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TEXT_INPUT_CHANNEL_H_
#define EMBEDDER_TEXT_INPUT_CHANNEL_H_

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
  explicit TextInputChannel(BinaryMessenger* messenger,
                            TizenInputMethodContext* input_method_context);
  virtual ~TextInputChannel();

  bool IsSoftwareKeyboardShowing() { return is_software_keyboard_showing_; }

  void OnComposeBegin();

  void OnComposeChanged(const std::string& str, int cursor_pos);

  void OnComposeEnd();

  void OnCommit(const std::string& str);

  void OnInputPanelStateChanged(int state);

  bool SendKeyEvent(const char* key,
                    const char* string,
                    const char* compose,
                    uint32_t modifiers,
                    uint32_t keycode,
                    bool is_down);

 private:
  // Called when a method is called on |channel_|;
  void HandleMethodCall(
      const MethodCall<rapidjson::Document>& method_call,
      std::unique_ptr<MethodResult<rapidjson::Document>> result);

  // Sends the current state of the given model to the Flutter engine.
  void SendStateUpdate(const TextInputModel& model);

  void HandleUnfilteredEvent(const char* key,
                             const char* string,
                             uint32_t modifires);

  // Sends an action triggered by the Enter key to the Flutter engine.
  void EnterPressed(TextInputModel* model);

#ifdef TV_PROFILE
  // Sends an action triggered by the Select key to the Flutter engine.
  void SelectPressed(TextInputModel* model);
#endif
  // The MethodChannel used for communication with the Flutter engine.
  std::unique_ptr<MethodChannel<rapidjson::Document>> channel_;

  // The active model. nullptr if not set.
  std::unique_ptr<TextInputModel> active_model_;

  // The Tizen input method context. nullptr if not set.
  TizenInputMethodContext* input_method_context_ = nullptr;

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
};

}  // namespace flutter

#endif  // EMBEDDER_TEXT_INPUT_CHANNEL_H_
