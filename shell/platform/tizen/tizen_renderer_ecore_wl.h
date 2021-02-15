// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_RENDERER_ECORE_WL_H
#define EMBEDDER_TIZEN_RENDERER_ECORE_WL_H

#include <wayland-client.h>
#include <wayland-egl.h>

#include "tizen_renderer.h"
#define EFL_BETA_API_SUPPORT
#include <Ecore_Wayland.h>
class TizenRendererEcoreWl : public TizenRenderer {
 public:
  TizenRendererEcoreWl(TizenRenderer::Delegate& delegate, int32_t x, int32_t y,
                       int32_t w, int32_t h);
  ~TizenRendererEcoreWl();
  TizenRenderer::TizenWindowGeometry GetGeometry() override;
  void Show() override;
  void SetRotate(int angle) override;
  void ResizeWithRotation(int32_t x, int32_t y, int32_t width, int32_t height,
                          int32_t degree) override;
  int GetEcoreWindowId() override;

 protected:
  void DestoryEglWindow() override;
  void DestoryEcoreWlWindow() override;
  EGLDisplay GetEGLDisplay() override;
  EGLNativeWindowType GetEGLNativeWindowType() override;
  bool SetupDisplay() override;
  bool SetupEcoreWlWindow(int32_t x, int32_t y, int32_t w, int32_t h) override;
  bool SetupEglWindow(int32_t w, int32_t h) override;
  void ShutdownDisplay() override;
  void SendRotationChangeDone() override;

 private:
  Ecore_Wl_Window* ecore_wl_window_ = nullptr;
  wl_egl_window* wl_egl_window_ = nullptr;
  wl_display* wl_display_ = nullptr;
  static Eina_Bool RotationEventCb(void* data, int type, void* event);
};
#endif //EMBEDDER_TIZEN_RENDERER_ECORE_WL_H
