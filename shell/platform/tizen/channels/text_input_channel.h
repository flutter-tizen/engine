// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TEXT_INPUT_PLUGIN_H_
#define EMBEDDER_TEXT_INPUT_PLUGIN_H_

#define EFL_BETA_API_SUPPORT
#include <Ecore_IMF.h>
#include <Ecore_Input.h>

#include <string>

#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/method_channel.h"
#include "flutter/shell/platform/common/cpp/json_method_codec.h"
#include "flutter/shell/platform/common/cpp/text_input_model.h"

class TizenEmbedderEngine;
class TextInputChannel {
 public:
  struct SoftwareKeyboardGeometry {
    int32_t x = 0, y = 0, w = 0, h = 0;
  };
  explicit TextInputChannel(flutter::BinaryMessenger* messenger,
                            TizenEmbedderEngine* engine);
  virtual ~TextInputChannel();
  void OnKeyDown(Ecore_Event_Key* key);
  void OnCommit(const char* str);
  void OnPredit(const char* str, int cursorPos);
  void ShowSoftwareKeyboard();
  void HideSoftwareKeyboard();
  bool IsSoftwareKeyboardShowing() { return is_software_keyboard_showing_; }
  SoftwareKeyboardGeometry GetCurrentKeyboardGeometry() {
    return current_keyboard_geometry_;
  }

  Ecore_IMF_Context* GetImfContext() { return imf_context_; }

  int32_t rotation = 0;

 private:
  void HandleMethodCall(
      const flutter::MethodCall<rapidjson::Document>& method_call,
      std::unique_ptr<flutter::MethodResult<rapidjson::Document>> result);
  void SendStateUpdate(const flutter::TextInputModel& model);
  bool FilterEvent(Ecore_Event_Key* keyDownEvent);
  void NonIMFFallback(Ecore_Event_Key* keyDownEvent);
  void EnterPressed(flutter::TextInputModel* model);
  void SelectPressed(flutter::TextInputModel* model);
  void RegisterIMFCallback();
  void UnregisterIMFCallback();

  std::unique_ptr<flutter::MethodChannel<rapidjson::Document>> channel_;
  std::unique_ptr<flutter::TextInputModel> active_model_;

  static void CommitCallback(void* data, Ecore_IMF_Context* ctx,
                             void* event_info);
  static void PreeditCallback(void* data, Ecore_IMF_Context* ctx,
                              void* event_info);
  static void PrivateCommandCallback(void* data, Ecore_IMF_Context* ctx,
                                     void* event_info);
  static void DeleteSurroundingCallback(void* data, Ecore_IMF_Context* ctx,
                                        void* event_info);
  static void InputPanelStateChangedCallback(void* data,
                                             Ecore_IMF_Context* context,
                                             int value);
  static void InputPanelGeometryChangedCallback(void* data,
                                                Ecore_IMF_Context* context,
                                                int value);
  static Eina_Bool RetrieveSurroundingCallback(void* data,
                                               Ecore_IMF_Context* ctx,
                                               char** text, int* cursor_pos);

  int client_id_;
  std::string input_type_;
  std::string input_action_;
  bool is_software_keyboard_showing_;
  int last_preedit_string_length_;
  Ecore_IMF_Context* imf_context_;
  bool in_select_mode_;
  TizenEmbedderEngine* engine_;
  SoftwareKeyboardGeometry current_keyboard_geometry_;
};
#endif
