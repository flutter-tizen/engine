// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_WINDOW_ECORE_WL2_H_
#define EMBEDDER_TIZEN_WINDOW_ECORE_WL2_H_

#include "flutter/shell/platform/tizen/tizen_window.h"

#define EFL_BETA_API_SUPPORT
#include <Ecore_Wl2.h>
#include <tizen-extension-client-protocol.h>

namespace flutter {

class TizenWindowEcoreWl2 : public TizenWindow {
 public:
  TizenWindowEcoreWl2(Geometry geometry,
                      bool transparent,
                      bool focusable,
                      bool top_level);

  ~TizenWindowEcoreWl2();

  Geometry GetWindowGeometry() override;

  void SetWindowGeometry(Geometry geometry) override;

  Geometry GetScreenGeometry() override;

  void* GetRenderTarget() override { return ecore_wl2_egl_window_; }

  void* GetRenderTargetDisplay() override { return wl2_display_; }

  int32_t GetRotation() override;

  int32_t GetDpi() override;

  uintptr_t GetWindowId() override;

  void* GetWindowHandle() override { return ecore_wl2_window_; }

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

  void SetTizenPolicyNotificationLevel(int level);

  Ecore_Wl2_Display* ecore_wl2_display_ = nullptr;
  Ecore_Wl2_Window* ecore_wl2_window_ = nullptr;

  Ecore_Wl2_Egl_Window* ecore_wl2_egl_window_ = nullptr;
  wl_display* wl2_display_ = nullptr;
  std::vector<Ecore_Event_Handler*> ecore_event_handlers_;

  tizen_policy* tizen_policy_ = nullptr;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_WINDOW_ECORE_WL2_H_
