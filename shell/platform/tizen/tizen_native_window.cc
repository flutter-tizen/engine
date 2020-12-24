// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_native_window.h"

#include "flutter/shell/platform/tizen/tizen_log.h"

class TizenWl2Display {
 public:
  TizenWl2Display() {
    if (!ecore_wl2_init()) {
      FT_LOGE("Could not initialize ecore_wl2");
      return;
    }
    // ecore_wl2 DISPLAY
    wl2_display_ = ecore_wl2_display_connect(nullptr);
    if (wl2_display_ == nullptr) {
      FT_LOGE("Display not found");
      return;
    }
    ecore_wl2_sync();
  }

  ~TizenWl2Display() {
    if (wl2_display_) {
      ecore_wl2_display_disconnect(wl2_display_);
      wl2_display_ = nullptr;
    }
    ecore_wl2_shutdown();
  }
  Ecore_Wl2_Display* GetHandle() { return wl2_display_; }

 private:
  Ecore_Wl2_Display* wl2_display_{nullptr};
};
TizenWl2Display g_tizen_wl2_display;

TizenNativeEGLWindow::TizenNativeEGLWindow(
    TizenNativeWindow* tizen_native_window, int32_t w, int32_t h) {
  egl_window_ =
      ecore_wl2_egl_window_create(tizen_native_window->GetWindowHandle(), w, h);

  egl_display_ = eglGetDisplay((EGLNativeDisplayType)ecore_wl2_display_get(
      g_tizen_wl2_display.GetHandle()));
  if (egl_display_ == EGL_NO_DISPLAY) {
    FT_LOGE("Could not access EGL display");
    return;
  }

  EGLint major_version;
  EGLint minor_version;
  if (eglInitialize(egl_display_, &major_version, &minor_version) != EGL_TRUE) {
    FT_LOGE("Could not initialize EGL display");
    return;
  }

  FT_LOGD("eglInitialized: %d.%d", major_version, minor_version);

  if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) {
    FT_LOGE("Could not bind API");
    return;
  }
}

TizenNativeEGLWindow::~TizenNativeEGLWindow() {
  if (egl_window_) {
    ecore_wl2_egl_window_destroy(egl_window_);
    egl_window_ = nullptr;
  }
  if (egl_display_ != EGL_NO_CONTEXT) {
    eglTerminate(egl_display_);
  }
}

TizenNativeWindow::TizenNativeWindow(int32_t x, int32_t y, int32_t w,
                                     int32_t h) {
  if (g_tizen_wl2_display.GetHandle() == nullptr) {
    FT_LOGE("Faild to get display handle");
    return;
  }
  if (w == 0 || h == 0) {
    FT_LOGE("Failed to create because of the wrong size");
    return;
  }

  wl2_window_ = ecore_wl2_window_new(g_tizen_wl2_display.GetHandle(), nullptr,
                                     x, y, w, h);

  ecore_wl2_window_type_set(wl2_window_, ECORE_WL2_WINDOW_TYPE_TOPLEVEL);
  ecore_wl2_window_alpha_set(wl2_window_, EINA_FALSE);
  ecore_wl2_window_aux_hint_add(wl2_window_, 0, "wm.policy.win.user.geometry",
                                "1");
  ecore_wl2_window_show(wl2_window_);

  tizen_native_egl_window_ = std::make_shared<TizenNativeEGLWindow>(this, w, h);
  is_valid_ = true;
}

TizenNativeWindow::~TizenNativeWindow() {
  tizen_native_egl_window_ = nullptr;
  if (wl2_window_) {
    ecore_wl2_window_free(wl2_window_);
    wl2_window_ = nullptr;
  }
}

TizenNativeWindow::TizenNativeWindowGeometry TizenNativeWindow::GetGeometry() {
  TizenNativeWindowGeometry result;
  ecore_wl2_window_geometry_get(wl2_window_, &result.x, &result.y, &result.w,
                                &result.h);
  return result;
}
