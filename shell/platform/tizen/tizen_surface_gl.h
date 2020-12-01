// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_SURFACE_GL_H_
#define EMBEDDER_TIZEN_SURFACE_GL_H_

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <wayland-client.h>
#include <wayland-egl.h>

#include <algorithm>
#include <string>
#include <vector>
#define EFL_BETA_API_SUPPORT
#include <Ecore_Input.h>
#include <Ecore_Wl2.h>

#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/tizen/tizen_surface.h"

class TizenSurfaceGL : public TizenSurface {
 public:
  TizenSurfaceGL(int32_t x, int32_t y, int32_t width, int32_t height);
  ~TizenSurfaceGL();
  bool OnMakeCurrent();
  bool OnClearCurrent();
  bool OnMakeResourceCurrent();
  bool OnPresent();
  uint32_t OnGetFBO();
  void* OnProcResolver(const char* name);
  bool IsValid();
  bool InitalizeDisplay();
  void Destroy();
  Ecore_Wl2_Window* wl2_window() { return wl2_window_; }

 private:
  bool display_valid_ = false;
  EGLDisplay egl_display_ = EGL_NO_DISPLAY;
  EGLSurface egl_surface_ = EGL_NO_SURFACE;
  EGLSurface egl_resource_surface_ = EGL_NO_SURFACE;
  EGLContext egl_context_ = EGL_NO_CONTEXT;
  EGLContext egl_resource_context_ = EGL_NO_CONTEXT;
  Ecore_Wl2_Egl_Window* egl_window_ = nullptr;
  Ecore_Wl2_Display* wl2_display_ = nullptr;
  Ecore_Wl2_Window* wl2_window_ = nullptr;
};

#endif  // EMBEDDER_TIZEN_SURFACE_GL_H_
