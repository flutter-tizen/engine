// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_INPUT_METHOD_CONTEXT_H_
#define EMBEDDER_TIZEN_INPUT_METHOD_CONTEXT_H_

#define EFL_BETA_API_SUPPORT
#include <Ecore_IMF.h>
#include <Ecore_Input.h>

#include <functional>

namespace flutter {

using OnCommitCallback = std::function<void(std::string str)>;
using OnPreeditCallback = std::function<void(std::string str, int cursor_pos)>;
using OnInputPannelStateChangedCallback = std::function<void(int value)>;

class FlutterTizenEngine;

struct InputPannelGeometry {
  int32_t x = 0, y = 0, w = 0, h = 0;
};

class TizenInputMethodContext {
 public:
  TizenInputMethodContext(FlutterTizenEngine* engine);
  ~TizenInputMethodContext();

  bool FilterEvent(Ecore_Event_Key* event, const char* dev_name);

  InputPannelGeometry GetInputPannelGeometry();

  void ResetInputMethodContext();

  void ShowInputPannel();

  void HideInputPannel();

  void OnCommit(std::string str);

  void OnPreedit(std::string str, int cursor_pos);

  void OnInputPannelStateChanged(int state);

  void SetInputPannelLayout(std::string layout);

  void SetOnCommitCallback(OnCommitCallback callback) {
    on_commit_callback_ = callback;
  }

  void SetOnPreeditCallback(OnPreeditCallback callback) {
    on_preedit_callback_ = callback;
  }

  void SetOnInputPannelStateChangedCallback(
      OnInputPannelStateChangedCallback callback) {
    on_input_pannel_state_changed_callback_ = callback;
  }

 private:
  void Init();

  void Deinit();

  void RegisterEventCallbacks();

  void UnregisterEventCallbacks();

  void SetContextOptions();

  void SetInputPannelOptions();

  FlutterTizenEngine* engine_{nullptr};
  Ecore_IMF_Context* imf_context_{nullptr};
  OnCommitCallback on_commit_callback_;
  OnPreeditCallback on_preedit_callback_;
  OnInputPannelStateChangedCallback on_input_pannel_state_changed_callback_;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_INPUT_METHOD_CONTEXT_H_
