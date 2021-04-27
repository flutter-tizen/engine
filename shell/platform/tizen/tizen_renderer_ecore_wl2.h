// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_RENDERER_ECORE_WL2_H
#define EMBEDDER_TIZEN_RENDERER_ECORE_WL2_H

#define EFL_BETA_API_SUPPORT
#include <EGL/egl.h>
#include <Ecore_Wl2.h>

#include "flutter/shell/platform/tizen/tizen_renderer.h"

class TizenRendererEcoreWl2 : public TizenRenderer {
 public:
  explicit TizenRendererEcoreWl2(TizenRenderer::Delegate &delegate, int32_t x,
                                 int32_t y, int32_t w, int32_t h);
  virtual ~TizenRendererEcoreWl2();

  bool OnMakeCurrent() override;
  bool OnClearCurrent() override;
  bool OnMakeResourceCurrent() override;
  bool OnPresent() override;
  uint32_t OnGetFBO() override;
  void *OnProcResolver(const char *name) override;

  TizenWindowGeometry GetGeometry() override;
  uintptr_t GetWindowId() override;

  void ResizeWithRotation(int32_t x, int32_t y, int32_t width, int32_t height,
                          int32_t angle) override;
  void SetRotate(int angle) override;

 private:
  bool InitializeRenderer(int32_t x, int32_t y, int32_t w, int32_t h);
  void Show();
  void DestroyRenderer();

  bool SetupDisplay();
  bool SetupEcoreWlWindow(int32_t x, int32_t y, int32_t w, int32_t h);
  bool SetupEglWindow(int32_t w, int32_t h);
  EGLDisplay GetEGLDisplay();
  EGLNativeWindowType GetEGLNativeWindowType();
  void DestroyEglWindow();
  void DestroyEcoreWlWindow();
  void ShutdownDisplay();

  bool SetupEglSurface();
  bool ChooseEGLConfiguration();
  void PrintEGLError();
  void DestroyEglSurface();

  static Eina_Bool RotationEventCb(void *data, int type, void *event);
  void SendRotationChangeDone();

  Ecore_Wl2_Display *ecore_wl2_display_ = nullptr;
  Ecore_Wl2_Window *ecore_wl2_window_ = nullptr;
  Ecore_Wl2_Egl_Window *ecore_wl2_egl_window_ = nullptr;

  EGLConfig egl_config_;
  EGLDisplay egl_display_ = EGL_NO_DISPLAY;
  EGLContext egl_context_ = EGL_NO_CONTEXT;
  EGLSurface egl_surface_ = EGL_NO_SURFACE;
  EGLContext egl_resource_context_ = EGL_NO_CONTEXT;
  EGLSurface egl_resource_surface_ = EGL_NO_SURFACE;
};

#endif  // EMBEDDER_TIZEN_RENDERER_ECORE_WL2_H
