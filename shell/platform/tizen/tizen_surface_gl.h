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
#include "flutter/shell/platform/tizen/tizen_native_window.h"
#include "flutter/shell/platform/tizen/tizen_surface.h"

class TizenEGLSurface {
 public:
  TizenEGLSurface(std::shared_ptr<TizenNativeEGLWindow> tizen_native_egl_window,
                  EGLSurface egl_surface)
      : tizen_native_egl_window_(tizen_native_egl_window),
        egl_surface_(egl_surface){};
  ~TizenEGLSurface();
  bool IsValid() { return egl_surface_ != EGL_NO_SURFACE; }
  EGLSurface GetEGLSurfaceHandle() { return egl_surface_; };

 private:
  std::shared_ptr<TizenNativeEGLWindow> tizen_native_egl_window_;
  EGLSurface egl_surface_{EGL_NO_SURFACE};
};

class TizenEGLContext {
 public:
  TizenEGLContext(
      std::shared_ptr<TizenNativeEGLWindow> tizen_native_egl_window);
  ~TizenEGLContext();
  bool IsValid();
  std::unique_ptr<TizenEGLSurface> CreateTizenEGLWindowSurface();
  std::unique_ptr<TizenEGLSurface> CreateTizenEGLPbufferSurface();
  EGLContext GetEGLContextHandle() { return egl_context_; }
  EGLContext GetEGLResourceContextHandle() { return egl_resource_context_; }

 public:
  std::shared_ptr<TizenNativeEGLWindow> tizen_native_egl_window_;
  EGLConfig egl_config_{nullptr};
  EGLContext egl_context_{EGL_NO_CONTEXT};
  EGLContext egl_resource_context_{EGL_NO_CONTEXT};
};

class TizenSurfaceGL : public TizenSurface {
 public:
  TizenSurfaceGL(std::shared_ptr<TizenNativeWindow> tizen_native_window);
  ~TizenSurfaceGL();
  bool OnMakeCurrent() override;
  bool OnClearCurrent() override;
  bool OnMakeResourceCurrent() override;
  bool OnPresent() override;
  uint32_t OnGetFBO() override;
  void* OnProcResolver(const char* name) override;
  bool IsValid() override { return is_valid_; };
  void SetSize(int32_t width, int32_t height) override;

 private:
  bool is_valid_{false};
  std::shared_ptr<TizenNativeWindow> tizen_native_window_;
  std::unique_ptr<TizenEGLContext> tizen_context_gl_;
  std::unique_ptr<TizenEGLSurface> tizen_egl_window_surface_;
  std::unique_ptr<TizenEGLSurface> tizen_egl_pbuffer_surface_;
};

#endif  // EMBEDDER_TIZEN_SURFACE_GL_H_
