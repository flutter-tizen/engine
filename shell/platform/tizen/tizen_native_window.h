// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_WINDOW_H_
#define EMBEDDER_TIZEN_WINDOW_H_
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#define EFL_BETA_API_SUPPORT
#include <Ecore_Wl2.h>

#include <memory>
class TizenNativeWindow;

class TizenNativeEGLWindow {
 public:
  TizenNativeEGLWindow(TizenNativeWindow* tizen_native_window, int32_t w,
                       int32_t h);
  ~TizenNativeEGLWindow();
  bool IsValid() {
    return egl_window_ != nullptr && egl_display_ != EGL_NO_DISPLAY;
  };

  Ecore_Wl2_Egl_Window* GetEglWindowHandle() { return egl_window_; };
  EGLDisplay GetEGLDisplayHandle() { return egl_display_; }

 private:
  Ecore_Wl2_Egl_Window* egl_window_ = nullptr;
  EGLDisplay egl_display_ = EGL_NO_DISPLAY;
};

class TizenNativeWindow {
 public:
  struct TizenNativeWindowGeometry {
    int32_t x{0}, y{0}, w{0}, h{0};
  };

  TizenNativeWindow(int32_t x, int32_t y, int32_t w, int32_t h);
  ~TizenNativeWindow();
  bool IsValid() { return is_valid_; }
  Ecore_Wl2_Window* GetWindowHandle() { return wl2_window_; }
  TizenNativeEGLWindow* GetTizenNativeEGLWindow() {
    return tizen_native_egl_window_.get();
  };
  TizenNativeWindowGeometry GetGeometry();

 private:
  std::unique_ptr<TizenNativeEGLWindow> tizen_native_egl_window_;
  Ecore_Wl2_Window* wl2_window_{nullptr};
  bool is_valid_{false};
};

#endif
