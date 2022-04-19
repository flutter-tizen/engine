// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_tizen_view.h"

#include "flutter/shell/platform/tizen/flutter_tizen_window.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace {

#if defined(MOBILE_PROFILE)
constexpr double kProfileFactor = 0.7;
#elif defined(WEARABLE_PROFILE)
constexpr double kProfileFactor = 0.4;
#elif defined(TV_PROFILE)
constexpr double kProfileFactor = 2.0;
#else
constexpr double kProfileFactor = 1.0;
#endif

}  // namespace

namespace flutter {

FlutterTizenView::FlutterTizenView(
    std::unique_ptr<FlutterTizenWindow> flutter_tizen_window)
    : flutter_tizen_window_(std::move(flutter_tizen_window)) {
  flutter_tizen_window_->SetFlutterTizenView(this);
}

FlutterTizenView::~FlutterTizenView() {
  FT_LOG(Info) << "enter";
}

void FlutterTizenView::SetFlutterTizenEngine(
    std::unique_ptr<FlutterTizenEngine> flutter_tizen_engine) {
  flutter_tizen_engine_ = std::move(flutter_tizen_engine);
  flutter_tizen_engine_->SetFlutterTizenView(this);

  // registrar
  // keyboard?
  // platform handler?
  // corsor handler?
  // touch_event_handler_ = std::make_unique<TouchEventHandler>(this);
  FT_LOG(Info) << "done";
}

void FlutterTizenView::CreateRenderSurface() {
  if (flutter_tizen_engine_ && flutter_tizen_engine_->renderer()) {
    auto geometry = flutter_tizen_window_->GetWindowGeometry();
    flutter_tizen_engine_->renderer()->CreateSurface(
        flutter_tizen_window_->GetRenderTarget(),
        flutter_tizen_window_->GetRenderTargetDisplay(), geometry.width,
        geometry.height);
  }
}

bool FlutterTizenView::OnMakeCurrent() {
  return flutter_tizen_engine_->renderer()->OnMakeCurrent();
}

bool FlutterTizenView::OnClearCurrent() {
  return flutter_tizen_engine_->renderer()->OnClearCurrent();
}

bool FlutterTizenView::OnMakeResourceCurrent() {
  return flutter_tizen_engine_->renderer()->OnMakeResourceCurrent();
}

bool FlutterTizenView::OnPresent() {
  return flutter_tizen_engine_->renderer()->OnPresent();
}
uint32_t FlutterTizenView::OnGetFBO() {
  return flutter_tizen_engine_->renderer()->OnGetFBO();
}

void* FlutterTizenView::OnProcResolver(const char* name) {
  return flutter_tizen_engine_->renderer()->OnProcResolver(name);
}

void FlutterTizenView::OnRotate(int32_t degree) {
  rotation_degree_ = degree;
  // Compute renderer transformation based on the angle of rotation.
  double rad = (360 - rotation_degree_) * M_PI / 180;
  auto geometry = flutter_tizen_window_->GetWindowGeometry();
  int32_t width = geometry.width;
  int32_t height = geometry.height;

  double trans_x = 0.0, trans_y = 0.0;
  if (rotation_degree_ == 90) {
    trans_y = height;
  } else if (rotation_degree_ == 180) {
    trans_x = width;
    trans_y = height;
  } else if (rotation_degree_ == 270) {
    trans_x = width;
  }

  // view settransform
  flutter_trans_formation_ = {
      cos(rad), -sin(rad), trans_x,  // x
      sin(rad), cos(rad),  trans_y,  // y
      0.0,      0.0,       1.0       // perspective
  };

  // touch_event_handler_->rotation = degree;

  if (rotation_degree_ == 90 || rotation_degree_ == 270) {
    std::swap(width, height);
  }

  flutter_tizen_window_->ResizeWithRotation(
      {geometry.left, geometry.top, width, height}, rotation_degree_);

  // Window position does not change on rotation regardless of its orientation.
  SendWindowMetrics(geometry.left, geometry.top, width, height, 0.0);
}

void FlutterTizenView::OnPointerMove(double x,
                                     double y,
                                     size_t timestamp,
                                     FlutterPointerDeviceKind device_kind,
                                     int32_t device_id) {
  if (pointer_state_) {
    SendFlutterPointerEvent(kMove, x, y, 0, 0, timestamp, device_kind,
                            device_id);
  }
}

void FlutterTizenView::OnPointerDown(double x,
                                     double y,
                                     size_t timestamp,
                                     FlutterPointerDeviceKind device_kind,
                                     int32_t device_id) {
  pointer_state_ = true;
  SendFlutterPointerEvent(kDown, x, y, 0, 0, timestamp, device_kind, device_id);
}

void FlutterTizenView::OnPointerUp(double x,
                                   double y,
                                   size_t timestamp,
                                   FlutterPointerDeviceKind device_kind,
                                   int32_t device_id) {
  pointer_state_ = false;
  SendFlutterPointerEvent(kUp, x, y, 0, 0, timestamp, device_kind, device_id);
}

void FlutterTizenView::OnScroll(double x,
                                double y,
                                double delta_x,
                                double delta_y,
                                int scroll_offset_multiplier,
                                size_t timestamp,
                                FlutterPointerDeviceKind device_kind,
                                int32_t device_id) {
  SendFlutterPointerEvent(
      pointer_state_ ? kMove : kHover, x, y, delta_x * scroll_offset_multiplier,
      delta_y * scroll_offset_multiplier, timestamp, device_kind, device_id);
}

void FlutterTizenView::SendWindowMetrics(int32_t left,
                                         int32_t top,
                                         int32_t width,
                                         int32_t height,
                                         double pixel_ratio) {
  double computed_pixel_ratio = pixel_ratio;
  if (pixel_ratio == 0.0) {
    // The scale factor is computed based on the display DPI and the current
    // profile. A fixed DPI value (72) is used on TVs. See:
    // https://docs.tizen.org/application/native/guides/ui/efl/multiple-screens
#ifdef TV_PROFILE
    double dpi = 72.0;
#else
    double dpi = static_cast<double>(flutter_tizen_window_->GetDpi());
#endif
    double scale_factor = dpi / 90.0 * kProfileFactor;
    computed_pixel_ratio = std::max(scale_factor, 1.0);
  } else {
    computed_pixel_ratio = pixel_ratio;
  }

  flutter_tizen_engine_->SendWindowMetrics(left, top, width, height,
                                           computed_pixel_ratio);
}

void FlutterTizenView::SendFlutterPointerEvent(
    FlutterPointerPhase phase,
    double x,
    double y,
    double delta_x,
    double delta_y,
    size_t timestamp,
    FlutterPointerDeviceKind device_kind,
    int device_id) {
  auto geometry = flutter_tizen_window_->GetWindowGeometry();
  double new_x = x, new_y = y;

  if (rotation_degree_ == 90) {
    new_x = geometry.height - y;
    new_y = x;
  } else if (rotation_degree_ == 180) {
    new_x = geometry.width - x;
    new_y = geometry.height - y;
  } else if (rotation_degree_ == 270) {
    new_x = y;
    new_y = geometry.width - x;
  }

  FlutterPointerEvent event = {};
  event.struct_size = sizeof(event);
  event.phase = phase;
  event.x = new_x;
  event.y = new_y;
  if (delta_x != 0 || delta_y != 0) {
    event.signal_kind = kFlutterPointerSignalKindScroll;
  }
  event.scroll_delta_x = delta_x;
  event.scroll_delta_y = delta_y;
  event.timestamp = timestamp * 1000;
  event.device = device_id;
  event.device_kind = kFlutterPointerDeviceKindTouch;

  flutter_tizen_engine_->SendPointerEvent(event);
}

}  // namespace flutter
