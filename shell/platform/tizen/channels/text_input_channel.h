// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TEXT_INPUT_PLUGIN_H_
#define EMBEDDER_TEXT_INPUT_PLUGIN_H_

#define EFL_BETA_API_SUPPORT
#include <Ecore_IMF.h>
#include <Ecore_Input.h>
#include <Ecore_Wl2.h>

#include <string>

#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/method_channel.h"
#include "flutter/shell/platform/common/cpp/json_method_codec.h"
#include "flutter/shell/platform/common/cpp/text_input_model.h"

class TextInputChannel {
 public:
  explicit TextInputChannel(flutter::BinaryMessenger* messenger,
                            Ecore_Wl2_Window* ecoreWindow);
  virtual ~TextInputChannel();
  void OnKeyDown(Ecore_Event_Key* key);
  void OnCommit(const char* str);
  void OnPredit(const char* str, int cursorPos);
  void ShowSoftwareKeyboard();
  void HideSoftwareKeyboard();
  bool isSoftwareKeyboardShowing() { return isSoftwareKeyboardShowing_; }

 private:
  void HandleMethodCall(
      const flutter::MethodCall<rapidjson::Document>& method_call,
      std::unique_ptr<flutter::MethodResult<rapidjson::Document>> result);
  void SendStateUpdate(const flutter::TextInputModel& model);
  bool FilterEvent(Ecore_Event_Key* keyDownEvent);
  void NonIMFFallback(Ecore_Event_Key* keyDownEvent);
  void EnterPressed(flutter::TextInputModel* model);
  void SelectPressed(flutter::TextInputModel* model);
  void RegisterIMFCallback(Ecore_Wl2_Window* ecoreWindow);
  void UnregisterIMFCallback();

  std::unique_ptr<flutter::MethodChannel<rapidjson::Document>> channel_;
  std::unique_ptr<flutter::TextInputModel> active_model_;

 private:
  int client_id_;
  std::string input_type_;
  std::string input_action_;
  bool isSoftwareKeyboardShowing_;
  int lastPreeditStringLength_;
  Ecore_IMF_Context* imfContext_;
  bool isWearable_;
  bool inSelectMode_;
};
#endif
