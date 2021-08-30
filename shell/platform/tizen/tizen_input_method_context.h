// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_INPUT_METHOD_CONTEXT_H_
#define EMBEDDER_TIZEN_INPUT_METHOD_CONTEXT_H_

#define EFL_BETA_API_SUPPORT
#include <Ecore_IMF.h>
#include <Ecore_Input.h>

#include <functional>
#include <unordered_map>

namespace flutter {

using OnCommit = std::function<void(std::string str)>;
using OnPreeditChanged = std::function<void(std::string str, int cursor_pos)>;
using OnPreeditStart = std::function<void()>;
using OnPreeditEnd = std::function<void()>;
using OnInputPannelStateChanged = std::function<void(int value)>;

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

  void SetInputPannelLayout(std::string layout);

  void SetOnCommit(OnCommit callback) { on_commit_ = callback; }

  void SetOnPreeditChanged(OnPreeditChanged callback) {
    on_preedit_changed_ = callback;
  }

  void SetOnPreeditStart(OnPreeditStart callback) {
    on_preedit_start_ = callback;
  }

  void SetOnPreeditEnd(OnPreeditEnd callback) { on_preedit_end_ = callback; }

  void SetOnInputPannelStateChanged(OnInputPannelStateChanged callback) {
    on_input_pannel_state_changed_ = callback;
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
  OnCommit on_commit_;
  OnPreeditChanged on_preedit_changed_;
  OnPreeditStart on_preedit_start_;
  OnPreeditEnd on_preedit_end_;
  OnInputPannelStateChanged on_input_pannel_state_changed_;
  std::unordered_map<Ecore_IMF_Callback_Type, Ecore_IMF_Event_Cb>
      event_callbacks_;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_INPUT_METHOD_CONTEXT_H_
