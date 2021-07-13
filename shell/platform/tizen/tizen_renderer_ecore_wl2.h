// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_RENDERER_ECORE_WL2_H_
#define EMBEDDER_TIZEN_RENDERER_ECORE_WL2_H_

#define EFL_BETA_API_SUPPORT
#include <EGL/egl.h>
#include <Ecore_Wl2.h>

#include "flutter/shell/platform/tizen/tizen_renderer.h"

namespace flutter {

class TizenRendererEcoreWl2 : public TizenRenderer {
 public:
  explicit TizenRendererEcoreWl2(WindowGeometry geometry,
                                 bool transparent,
                                 bool focusable,
                                 Delegate& delegate);
  virtual ~TizenRendererEcoreWl2();

  bool OnMakeCurrent() override;
  bool OnClearCurrent() override;
  bool OnMakeResourceCurrent() override;
  bool OnPresent() override;
  uint32_t OnGetFBO() override;
  void* OnProcResolver(const char* name) override;

  WindowGeometry GetCurrentGeometry() override;
  int32_t GetDpi() override;
  uintptr_t GetWindowId() override;
  void* GetWindowHandle() override;

  void ResizeWithRotation(int32_t x,
                          int32_t y,
                          int32_t width,
                          int32_t height,
                          int32_t angle) override;
  void SetRotate(int angle) override;
  void SetPreferredOrientations(const std::vector<int>& rotations) override;

 private:
  bool InitializeRenderer();
  void Show();
  void DestroyRenderer();

  bool SetupDisplay(int32_t* width, int32_t* height);
  bool SetupEcoreWlWindow(int32_t width, int32_t height);
  bool SetupEglWindow(int32_t width, int32_t height);
  EGLDisplay GetEGLDisplay();
  EGLNativeWindowType GetEGLNativeWindowType();
  void DestroyEglWindow();
  void DestroyEcoreWlWindow();
  void ShutdownDisplay();

  bool SetupEglSurface();
  bool ChooseEGLConfiguration();
  void PrintEGLError();
  void DestroyEglSurface();

  static Eina_Bool RotationEventCb(void* data, int type, void* event);
  void SendRotationChangeDone();

  Ecore_Wl2_Display* ecore_wl2_display_ = nullptr;
  Ecore_Wl2_Window* ecore_wl2_window_ = nullptr;
  Ecore_Wl2_Egl_Window* ecore_wl2_egl_window_ = nullptr;

  EGLConfig egl_config_;
  EGLDisplay egl_display_ = EGL_NO_DISPLAY;
  EGLContext egl_context_ = EGL_NO_CONTEXT;
  EGLSurface egl_surface_ = EGL_NO_SURFACE;
  EGLContext egl_resource_context_ = EGL_NO_CONTEXT;
  EGLSurface egl_resource_surface_ = EGL_NO_SURFACE;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_RENDERER_ECORE_WL2_H_
