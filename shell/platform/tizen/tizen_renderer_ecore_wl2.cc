// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_renderer_ecore_wl2.h"

#include "flutter/shell/platform/tizen/tizen_log.h"

TizenRendererEcoreWl2::TizenRendererEcoreWl2(TizenRenderer::Delegate &delegate,
                                             int32_t x, int32_t y, int32_t w,
                                             int32_t h)
    : TizenRenderer(delegate) {
  InitializeRenderer(x, y, w, h);
}

TizenRendererEcoreWl2::~TizenRendererEcoreWl2() { DestoryRenderer(); }

bool TizenRendererEcoreWl2::SetupDisplay() {
  if (!ecore_wl2_init()) {
    FT_LOGE("Could not initialize ecore_wl2");
    return false;
  }
  ecore_wl2_display_ = ecore_wl2_display_connect(nullptr);
  if (ecore_wl2_display_ == nullptr) {
    FT_LOGE("Disyplay not found");
    return false;
  }
  FT_LOGD("ecore_wl2_display_: %p", ecore_wl2_display_);
  ecore_wl2_sync();
  return true;
}

bool TizenRendererEcoreWl2::SetupEcoreWlWindow(int32_t x, int32_t y, int32_t w,
                                               int32_t h) {
  if (w == 0 || h == 0) {
    FT_LOGE("Failed to create because of the wrong size");
    return false;
  }
  ecore_wl2_window_ =
      ecore_wl2_window_new(ecore_wl2_display_, nullptr, x, y, w, h);
  ecore_wl2_window_type_set(ecore_wl2_window_, ECORE_WL2_WINDOW_TYPE_TOPLEVEL);
  ecore_wl2_window_alpha_set(ecore_wl2_window_, EINA_FALSE);
  ecore_wl2_window_position_set(ecore_wl2_window_, x, y);
  ecore_wl2_window_aux_hint_add(ecore_wl2_window_, 0,
                                "wm.policy.win.user.geometry", "1");
  int rotations[4] = {0, 90, 180, 270};
  ecore_wl2_window_available_rotations_set(ecore_wl2_window_, rotations,
                                           sizeof(rotations) / sizeof(int));
  ecore_event_handler_add(ECORE_WL2_EVENT_WINDOW_ROTATE, RotationEventCb, this);
  return true;
}

Eina_Bool TizenRendererEcoreWl2::RotationEventCb(void *data, int type,
                                                 void *event) {
  auto *self = reinterpret_cast<TizenRendererEcoreWl2 *>(data);
  Ecore_Wl2_Event_Window_Rotation *ev =
      reinterpret_cast<Ecore_Wl2_Event_Window_Rotation *>(event);
  self->delegate_.OnRotationChange(ev->angle);
  return ECORE_CALLBACK_PASS_ON;
}

void TizenRendererEcoreWl2::Show() { ecore_wl2_window_show(ecore_wl2_window_); }

void TizenRendererEcoreWl2::SetRotate(int angle) {
  ecore_wl2_window_rotation_set(ecore_wl2_window_, angle);
  received_rotation = true;
}

void TizenRendererEcoreWl2::ResizeWithRotation(int32_t x, int32_t y,
                                               int32_t width, int32_t height,
                                               int32_t angle) {
  ecore_wl2_egl_window_resize_with_rotation(ecore_wl2_egl_window_, x, y, width,
                                            height, angle);
}

void TizenRendererEcoreWl2::SendRotationChangeDone() {
  int x, y, w, h;
  ecore_wl2_window_geometry_get(ecore_wl2_window_, &x, &y, &w, &h);
  ecore_wl2_window_rotation_change_done_send(
      ecore_wl2_window_, ecore_wl2_window_rotation_get(ecore_wl2_window_), w,
      h);
}

bool TizenRendererEcoreWl2::SetupEglWindow(int32_t w, int32_t h) {
  ecore_wl2_egl_window_ = ecore_wl2_egl_window_create(ecore_wl2_window_, w, h);
  return ecore_wl2_egl_window_ != nullptr;
}

EGLDisplay TizenRendererEcoreWl2::GetEGLDisplay() {
  return eglGetDisplay(
      (EGLNativeDisplayType)ecore_wl2_display_get(ecore_wl2_display_));
}

EGLNativeWindowType TizenRendererEcoreWl2::GetEGLNativeWindowType() {
  return (EGLNativeWindowType)ecore_wl2_egl_window_native_get(
      ecore_wl2_egl_window_);
}

void TizenRendererEcoreWl2::DestoryEglWindow() {
  if (ecore_wl2_egl_window_) {
    ecore_wl2_egl_window_destroy(ecore_wl2_egl_window_);
    ecore_wl2_egl_window_ = nullptr;
  }
}

void TizenRendererEcoreWl2::DestoryEcoreWlWindow() {
  if (ecore_wl2_window_) {
    ecore_wl2_window_free(ecore_wl2_window_);
    ecore_wl2_window_ = nullptr;
  }
}

void TizenRendererEcoreWl2::ShutdownDisplay() {
  if (ecore_wl2_display_) {
    ecore_wl2_display_disconnect(ecore_wl2_display_);
    ecore_wl2_display_ = nullptr;
  }
  ecore_wl2_shutdown();
}

TizenRenderer::TizenWindowGeometry TizenRendererEcoreWl2::GetGeometry() {
  TizenWindowGeometry result;
  ecore_wl2_window_geometry_get(ecore_wl2_window_, &result.x, &result.y,
                                &result.w, &result.h);
  return result;
}

int TizenRendererEcoreWl2::GetEcoreWindowId() {
  return ecore_wl2_window_id_get(ecore_wl2_window_);
}
