// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_RENDERER_ECORE_WL2_H_
#define EMBEDDER_TIZEN_RENDERER_ECORE_WL2_H_

#define EFL_BETA_API_SUPPORT
#include <EGL/egl.h>
#include <Ecore_Wl2.h>
#include <tizen-extension-client-protocol.h>

#include <string>

#include "flutter/shell/platform/tizen/tizen_renderer.h"

namespace flutter {

class TizenRendererEcoreWl2 : public TizenRenderer {
 public:
  explicit TizenRendererEcoreWl2(Geometry geometry,
                                 bool transparent,
                                 bool focusable,
                                 bool top_level,
                                 Delegate& delegate);
  virtual ~TizenRendererEcoreWl2();

  bool OnMakeCurrent() override;
  bool OnClearCurrent() override;
  bool OnMakeResourceCurrent() override;
  bool OnPresent() override;
  uint32_t OnGetFBO() override;
  void* OnProcResolver(const char* name) override;

  Geometry GetWindowGeometry() override;
  Geometry GetScreenGeometry() override;
  int32_t GetDpi() override;
  uintptr_t GetWindowId() override;

  void* GetWindowHandle() override { return ecore_wl2_window_; }

  void SetRotate(int angle) override;
  void SetGeometry(int32_t x,
                   int32_t y,
                   int32_t width,
                   int32_t height) override;
  void ResizeWithRotation(int32_t x,
                          int32_t y,
                          int32_t width,
                          int32_t height,
                          int32_t angle) override;
  void SetPreferredOrientations(const std::vector<int>& rotations) override;

  bool IsSupportedExtension(const char* name) override;

 private:
  void Show();

  bool SetupEcoreWl2();
  bool SetupEGL();

  bool ChooseEGLConfiguration();
  void PrintEGLError();

  void DestroyEcoreWl2();
  void DestroyEGL();

  static Eina_Bool RotationEventCb(void* data, int type, void* event);
  void SendRotationChangeDone();

  void SetTizenPolicyNotificationLevel(int level);

  Ecore_Wl2_Display* ecore_wl2_display_ = nullptr;
  Ecore_Wl2_Window* ecore_wl2_window_ = nullptr;
  Ecore_Wl2_Egl_Window* ecore_wl2_egl_window_ = nullptr;

  EGLConfig egl_config_ = nullptr;
  EGLDisplay egl_display_ = EGL_NO_DISPLAY;
  EGLContext egl_context_ = EGL_NO_CONTEXT;
  EGLSurface egl_surface_ = EGL_NO_SURFACE;
  EGLContext egl_resource_context_ = EGL_NO_CONTEXT;
  EGLSurface egl_resource_surface_ = EGL_NO_SURFACE;

  std::string egl_extension_str_;

  tizen_policy* tizen_policy_ = nullptr;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_RENDERER_ECORE_WL2_H_
