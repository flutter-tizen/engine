// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_RENDERER_EVAS_GL_H
#define EMBEDDER_TIZEN_RENDERER_EVAS_GL_H

#undef EFL_BETA_API_SUPPORT
#include <Ecore.h>
#include <Elementary.h>
#include <Evas_GL.h>

#include "flutter/shell/platform/tizen/tizen_renderer.h"

class TizenRendererEvasGL : public TizenRenderer {
 public:
  explicit TizenRendererEvasGL(TizenRenderer::Delegate& delegate);
  virtual ~TizenRendererEvasGL();

  bool OnMakeCurrent() override;
  bool OnClearCurrent() override;
  bool OnMakeResourceCurrent() override;
  bool OnPresent() override;
  uint32_t OnGetFBO() override;
  void* OnProcResolver(const char* name) override;

  TizenWindowGeometry GetGeometry() override;
  int32_t GetDpi() override;
  uintptr_t GetWindowId() override;

  void ResizeWithRotation(int32_t x, int32_t y, int32_t width, int32_t height,
                          int32_t angle) override;
  void SetRotate(int angle) override;
  void SetPreferredOrientations(const std::vector<int>& rotations) override;

  void* GetImageHandle();

 private:
  void ClearColor(float r, float g, float b, float a);

  bool InitializeRenderer();
  void Show();
  void DestroyRenderer();

  bool SetupEvasGL();
  void* SetupEvasWindow(int32_t& width, int32_t& height);
  void DestroyEvasGL();
  void DestroyEvasWindow();

  static void RotationEventCb(void* data, Evas_Object* obj, void* event_info);
  void SendRotationChangeDone();

  Evas_Object* evas_window_{nullptr};
  Evas_Object* graphics_adapter_{nullptr};

  Evas_GL_Config* gl_config_{nullptr};
  Evas_GL* evas_gl_{nullptr};

  Evas_GL_Context* gl_context_{nullptr};
  Evas_GL_Context* gl_resource_context_{nullptr};

  Evas_GL_Surface* gl_surface_{nullptr};
  Evas_GL_Surface* gl_resource_surface_{nullptr};
};

#endif  // EMBEDDER_TIZEN_RENDERER_ECORE_WL2_H
