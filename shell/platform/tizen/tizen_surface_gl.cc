// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_surface_gl.h"

#include "flutter/shell/platform/tizen/logger.h"

TizenSurfaceGL::TizenSurfaceGL(int32_t x, int32_t y, int32_t width,
                               int32_t height)
    : TizenSurface(x, y, width, height) {
  LoggerD("x =[%d] y = [%d], width = [%d], height = [%d]", x, y, width, height);
  InitalizeDisplay();
}

bool TizenSurfaceGL::IsValid() { return display_valid_; }

bool TizenSurfaceGL::OnMakeCurrent() {
  if (!display_valid_) {
    LoggerE("Invalid Display");
    return false;
  }
  if (eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_) !=
      EGL_TRUE) {
    LoggerE("Could not make the onscreen context current");
    return false;
  }
  return true;
}

bool TizenSurfaceGL::OnMakeResourceCurrent() {
  if (!display_valid_) {
    LoggerE("Invalid Display");
    return false;
  }
  if (eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                     egl_resource_context_) != EGL_TRUE) {
    LoggerE("Could not make the onscreen context current");
    return false;
  }
  return true;
}

bool TizenSurfaceGL::OnClearCurrent() {
  if (!display_valid_) {
    LoggerE("Invalid display");
    return false;
  }

  if (eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                     EGL_NO_CONTEXT) != EGL_TRUE) {
    LoggerE("Could not clear context");
    return false;
  }
  return true;
}

bool TizenSurfaceGL::OnPresent() {
  if (!display_valid_) {
    LoggerE("Invalid display");
    return false;
  }

  if (eglSwapBuffers(egl_display_, egl_surface_) != EGL_TRUE) {
    LoggerE("Could not swap EGl buffer");
    return false;
  }
  return true;
}

uint32_t TizenSurfaceGL::OnGetFBO() {
  if (!display_valid_) {
    LoggerE("Invalid display");
    return 999;
  }
  LoggerD("OnApplicationGetOnscreenFBO");
  return 0;  // FBO0
}

void* TizenSurfaceGL::OnProcResolver(const char* name) {
  auto address = eglGetProcAddress(name);
  if (address != nullptr) {
    return reinterpret_cast<void*>(address);
  }
  LoggerW("Could not resolve: %s", name);
  return nullptr;
}

TizenSurfaceGL::~TizenSurfaceGL() {
  if (IsValid()) {
    display_valid_ = false;
    Destroy();
  }
}

bool TizenSurfaceGL::InitalizeDisplay() {
  LoggerD("x =[%d] y = [%d], width = [%d], height = [%d]", x_, y_,
          window_width_, window_height_);
  if (window_width_ == 0 || window_height_ == 0) {
    return false;
  }

  // ecore_wl2 INIT
  if (!ecore_wl2_init()) {
    LoggerE("Could not initialize ecore_wl2");
    return false;
  }

  // ecore_wl2 DISPLAY
  wl2_display_ = ecore_wl2_display_connect(nullptr);
  if (nullptr == wl2_display_) {
    LoggerE("Display not found");
    return false;
  }
  LoggerD("wl2_display_: %p", wl2_display_);
  ecore_wl2_sync();

  // ecore_wl2 WINDOW
  wl2_window_ = ecore_wl2_window_new(wl2_display_, nullptr, x_, y_,
                                     window_width_, window_height_);
  LoggerD("wl2_window_: %p", wl2_window_);

  ecore_wl2_window_type_set(wl2_window_, ECORE_WL2_WINDOW_TYPE_TOPLEVEL);
  ecore_wl2_window_alpha_set(wl2_window_, EINA_FALSE);
  // ecore_wl2_sync();

  // ecore_wl2 SHOW
  ecore_wl2_window_show(wl2_window_);

  ecore_wl2_window_position_set(wl2_window_, x_, y_);
  ecore_wl2_window_geometry_set(wl2_window_, x_, y_, window_width_,
                                window_height_);

  // egl WINDOW
  egl_window_ =
      ecore_wl2_egl_window_create(wl2_window_, window_width_, window_height_);
  LoggerD("egl_window_: %p", egl_window_);

  if (!egl_window_) {
    LoggerE("Could not create EGL window");
    return false;
  }

  // egl DISPLAY
  egl_display_ =
      eglGetDisplay((EGLNativeDisplayType)ecore_wl2_display_get(wl2_display_));
  if (egl_display_ == EGL_NO_DISPLAY) {
    LoggerE("Could not access EGL display");
    return false;
  }
  LoggerD("egl_display_: %p", egl_display_);

  // egl INTIALIZE
  EGLint majorVersion;
  EGLint minorVersion;
  if (eglInitialize(egl_display_, &majorVersion, &minorVersion) != EGL_TRUE) {
    LoggerE("Could not initialize EGL display");
    return false;
  }
  LoggerD("eglInitialized: %d.%d", majorVersion, minorVersion);

  // egl BINDAPI
  if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) {
    LoggerE("Could not bind API");
    return false;
  }

  // egl CONFIG
  EGLint config_count;
  eglGetConfigs(egl_display_, NULL, 0, &config_count);
  LoggerD("config_count: %d", config_count);

  EGLConfig egl_config = nullptr;

  {
    EGLint attribs[] = {
        // clang-format off
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_DEPTH_SIZE, 0,
      EGL_STENCIL_SIZE, 0,
      EGL_NONE,  // termination sentinel
        // clang-format on
    };

    if (eglChooseConfig(egl_display_, attribs, &egl_config, 1, &config_count) !=
        EGL_TRUE) {
      LoggerE("Error when attempting to choose an EGL surface config");
      return false;
    }

    if (config_count == 0 || egl_config == nullptr) {
      LoggerE("No matching config");
      return false;
    }
  }

  // egl CONTEXT
  const EGLint attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

  egl_context_ = eglCreateContext(egl_display_, egl_config, nullptr, attribs);
  if (egl_context_ == EGL_NO_CONTEXT) {
    LoggerE("Could not create an onscreen context");
    return false;
  }

  egl_resource_context_ =
      eglCreateContext(egl_display_, egl_config, egl_context_, attribs);
  if (egl_resource_context_ == EGL_NO_CONTEXT) {
    LoggerE("Could not create an resource  context");
    return false;
  }

  // egl SURFACE
  {
    egl_surface_ = eglCreateWindowSurface(
        egl_display_, egl_config, ecore_wl2_egl_window_native_get(egl_window_),
        nullptr);

    if (egl_surface_ == EGL_NO_SURFACE) {
      return false;
    }
  }
  LoggerD("egl_surface_: %p", egl_surface_);
  display_valid_ = true;
  return true;
}

void TizenSurfaceGL::Destroy() {
  if (egl_window_) {
    ecore_wl2_egl_window_destroy(egl_window_);
    egl_window_ = nullptr;
  }
  if (wl2_window_) {
    ecore_wl2_window_free(wl2_window_);
    wl2_window_ = nullptr;
  }
  if (wl2_display_) {
    ecore_wl2_display_destroy(wl2_display_);
    wl2_display_ = nullptr;
  }
  ecore_wl2_shutdown();
}
