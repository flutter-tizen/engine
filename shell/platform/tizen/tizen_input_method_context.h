// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_INPUT_METHOD_CONTEXT_H_
#define EMBEDDER_TIZEN_INPUT_METHOD_CONTEXT_H_

#include <Ecore_IMF.h>
#include <Ecore_IMF_Evas.h>
#include <Ecore_Input.h>

#include <functional>
#include <string>
#include <unordered_map>

namespace flutter {

using OnCommit = std::function<void(std::string str)>;
using OnPreeditChanged = std::function<void(std::string str, int cursor_pos)>;
using OnPreeditStart = std::function<void()>;
using OnPreeditEnd = std::function<void()>;

struct InputPanelGeometry {
  int32_t x = 0, y = 0, w = 0, h = 0;
};

class TizenInputMethodContext {
 public:
  TizenInputMethodContext(uintptr_t window_id);
  ~TizenInputMethodContext();

  bool HandleEcoreEventKey(Ecore_Event_Key* event, bool is_down);

  bool HandleEvasEventKeyDown(Evas_Event_Key_Down* event);

  bool HandleEvasEventKeyUp(Evas_Event_Key_Up* event);

  InputPanelGeometry GetInputPanelGeometry();

  void ResetInputMethodContext();

  void ShowInputPanel();

  void HideInputPanel();

  bool IsInputPanelShown();

  void SetInputPanelLayout(const std::string& layout);

  void SetInputPanelLayoutVariation(bool is_signed, bool is_decimal);

  void SetOnCommit(OnCommit callback) { on_commit_ = callback; }

  void SetOnPreeditChanged(OnPreeditChanged callback) {
    on_preedit_changed_ = callback;
  }

  void SetOnPreeditStart(OnPreeditStart callback) {
    on_preedit_start_ = callback;
  }

  void SetOnPreeditEnd(OnPreeditEnd callback) { on_preedit_end_ = callback; }

 private:
  void RegisterEventCallbacks();
  void UnregisterEventCallbacks();

  void SetContextOptions();
  void SetInputPanelOptions();

  bool ShouldIgnoreKey(std::string key, bool is_ime);

  Ecore_IMF_Context* imf_context_ = nullptr;
  OnCommit on_commit_;
  OnPreeditChanged on_preedit_changed_;
  OnPreeditStart on_preedit_start_;
  OnPreeditEnd on_preedit_end_;
  std::unordered_map<Ecore_IMF_Callback_Type, Ecore_IMF_Event_Cb>
      event_callbacks_;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_INPUT_METHOD_CONTEXT_H_
