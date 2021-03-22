// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TEXT_INPUT_CHANNEL_H_
#define EMBEDDER_TEXT_INPUT_CHANNEL_H_

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
  enum EditStatus { kNone, kPreeditStart, kPreeditEnd, kCommit };
  explicit TextInputChannel(flutter::BinaryMessenger* messenger,
                            TizenEmbedderEngine* engine);
  virtual ~TextInputChannel();
  void OnKeyDown(Ecore_Event_Key* key);
  void OnCommit(std::string str);
  void OnPreedit(std::string str, int cursor_pos);
  void OnPreeditForPlatformView(std::string str, int cursor_pos);

  void ShowSoftwareKeyboard();
  void HideSoftwareKeyboard();
  bool IsSoftwareKeyboardShowing() { return is_software_keyboard_showing_; }
  void SetSoftwareKeyboardShowing() { is_software_keyboard_showing_ = true; }

  void SetEditStatus(EditStatus edit_status);
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
  void EnterPressed(flutter::TextInputModel* model, bool select);
  void RegisterIMFCallback();
  void UnregisterIMFCallback();
  void ConsumeLastPreedit();
  void ResetCurrentContext();

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

  int client_id_{0};
  SoftwareKeyboardGeometry current_keyboard_geometry_;
  bool is_software_keyboard_showing_{false};
  std::string input_action_;
  std::string input_type_;

  EditStatus edit_status_{EditStatus::kNone};
  bool have_preedit_{false};
  bool in_select_mode_{false};
  int preedit_end_pos_{0};
  int preedit_start_pos_{0};
  std::string last_handled_ecore_event_keyname_;
  TizenEmbedderEngine* engine_{nullptr};
  Ecore_IMF_Context* imf_context_{nullptr};
};

#endif  // EMBEDDER_TEXT_INPUT_CHANNEL_H_
