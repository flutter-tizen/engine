// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_RENDERER_ECORE_WL2_H
#define EMBEDDER_TIZEN_RENDERER_ECORE_WL2_H

#include "tizen_renderer.h"

#define EFL_BETA_API_SUPPORT
#include <Ecore_Wl2.h>

class TizenRendererEcoreWl2 : public TizenRenderer {
 public:
  TizenRendererEcoreWl2(TizenRenderer::Delegate &delegate, int32_t x, int32_t y,
                        int32_t w, int32_t h);
  ~TizenRendererEcoreWl2();
  TizenWindowGeometry GetGeometry() override;
  int GetEcoreWindowId() override;
  void ResizeWithRotation(int32_t x, int32_t y, int32_t width, int32_t height,
                          int32_t angle) override;
  void Show() override;
  void SetRotate(int angle) override;

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
  Ecore_Wl2_Display *ecore_wl2_display_ = nullptr;
  Ecore_Wl2_Window *ecore_wl2_window_ = nullptr;
  Ecore_Wl2_Egl_Window *ecore_wl2_egl_window_ = nullptr;
  static Eina_Bool RotationEventCb(void *data, int type, void *event);
};
#endif //EMBEDDER_TIZEN_RENDERER_ECORE_WL2_H
