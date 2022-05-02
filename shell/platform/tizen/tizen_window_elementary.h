// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_WINDOW_ELEMENTARY_H_
#define EMBEDDER_TIZEN_WINDOW_ELEMENTARY_H_

#include "flutter/shell/platform/tizen/tizen_window.h"

#define EFL_BETA_API_SUPPORT
#include <Ecore.h>
#include <Elementary.h>

#include <unordered_map>

namespace flutter {

class TizenWindowElementary : public TizenWindow {
 public:
  TizenWindowElementary(Geometry geometry,
                        bool transparent,
                        bool focusable,
                        bool top_level);

  ~TizenWindowElementary();

  Geometry GetWindowGeometry() override;

  void SetWindowGeometry(Geometry geometry) override;

  Geometry GetScreenGeometry() override;

  void* GetRenderTarget() override { return elm_win_; }

  void* GetRenderTargetDisplay() override { return image_; }

  int32_t GetRotation() override;

  int32_t GetDpi() override;

  uintptr_t GetWindowId() override;

  void* GetWindowHandle() override { return elm_win_; }

  void ResizeRenderTargetWithRotation(Geometry geometry,
                                      int32_t angle) override;

  void SetPreferredOrientations(const std::vector<int>& rotations) override;

  void BindKeys(const std::vector<std::string>& keys) override;

  void Show() override;

  void OnGeometryChanged(Geometry geometry) override;

 private:
  bool CreateWindow();

  void DestroyWindow();

  void SetWindowOptions();

  void RegisterEventHandlers();

  void UnregisterEventHandlers();

  Evas_Object* elm_win_ = nullptr;
  Evas_Object* image_ = nullptr;

  Evas_Smart_Cb rotatoin_changed_callback_;
  std::unordered_map<Evas_Callback_Type, Evas_Object_Event_Cb>
      evas_object_callbacks_;
  std::vector<Ecore_Event_Handler*> ecore_event_key_handlers_;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_WINDOW_ELEMENTARY_H_
