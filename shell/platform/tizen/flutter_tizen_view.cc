// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_tizen_view.h"

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

constexpr char kBackKey[] = "XF86Back";
constexpr char kExitKey[] = "XF86Exit";

// Keys that should always be handled by the app first but not by the system.
const std::vector<std::string> kBindableSystemKeys = {
    "XF86Menu",           "XF86Back",        "XF86AudioPlay",
    "XF86AudioPause",     "XF86AudioStop",   "XF86AudioNext",
    "XF86AudioPrev",      "XF86AudioRewind", "XF86AudioForward",
    "XF86AudioPlayPause", "XF86AudioRecord", "XF86LowerChannel",
    "XF86RaiseChannel",   "XF86ChannelList", "XF86PreviousChannel",
    "XF86SysMenu",        "XF86SimpleMenu",  "XF86History",
    "XF86Favorites",      "XF86Info",        "XF86Red",
    "XF86Green",          "XF86Yellow",      "XF86Blue",
    "XF86Subtitle",       "XF86PlayBack",    "XF86ChannelGuide",
    "XF86Caption",        "XF86Exit",
};

}  // namespace

namespace flutter {

FlutterTizenView::FlutterTizenView(std::unique_ptr<TizenWindow> window)
    : window_(std::move(window)) {
  window_->SetView(this);
  window_->BindKeys(kBindableSystemKeys);
}

FlutterTizenView::~FlutterTizenView() {}

void FlutterTizenView::SetEngine(std::unique_ptr<FlutterTizenEngine> engine) {
  engine_ = std::move(engine);
  engine_->SetView(this);

  internal_plugin_registrar_ =
      std::make_unique<PluginRegistrar>(engine_->plugin_registrar());

  // Set up window dependent channels.
  BinaryMessenger* messenger = internal_plugin_registrar_->messenger();
  platform_channel_ =
      std::make_unique<PlatformChannel>(messenger, window_.get());
  window_channel_ = std::make_unique<WindowChannel>(messenger, window_.get());
  text_input_channel_ = std::make_unique<TextInputChannel>(
      internal_plugin_registrar_->messenger(),
      std::make_unique<TizenInputMethodContext>(window_.get()));

  OnRotate(window_->GetRotation());
}

void FlutterTizenView::CreateRenderSurface() {
  if (engine_ && engine_->renderer()) {
    TizenWindow::Geometry geometry = window_->GetWindowGeometry();
    engine_->renderer()->CreateSurface(window_->GetRenderTarget(),
                                       window_->GetRenderTargetDisplay(),
                                       geometry.width, geometry.height);
  }
}

void FlutterTizenView::DestroyRenderSurface() {
  if (engine_ && engine_->renderer()) {
    engine_->renderer()->DestroySurface();
  }
}

bool FlutterTizenView::OnMakeCurrent() {
  return engine_->renderer()->OnMakeCurrent();
}

bool FlutterTizenView::OnClearCurrent() {
  return engine_->renderer()->OnClearCurrent();
}

bool FlutterTizenView::OnMakeResourceCurrent() {
  return engine_->renderer()->OnMakeResourceCurrent();
}

bool FlutterTizenView::OnPresent() {
  return engine_->renderer()->OnPresent();
}

uint32_t FlutterTizenView::OnGetFBO() {
  return engine_->renderer()->OnGetFBO();
}

void* FlutterTizenView::OnProcResolver(const char* name) {
  return engine_->renderer()->OnProcResolver(name);
}

void FlutterTizenView::OnResize(int32_t left,
                                int32_t top,
                                int32_t width,
                                int32_t height) {
  if (rotation_degree_ == 90 || rotation_degree_ == 270) {
    std::swap(width, height);
  }

  window_->ResizeRenderTargetWithRotation({left, top, width, height},
                                          rotation_degree_);
  SendWindowMetrics(left, top, width, height, 0.0);
}

void FlutterTizenView::OnRotate(int32_t degree) {
  rotation_degree_ = degree;
  // Compute renderer transformation based on the angle of rotation.
  double rad = (360 - rotation_degree_) * M_PI / 180;
  TizenWindow::Geometry geometry = window_->GetWindowGeometry();
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

  flutter_transformation_ = {
      cos(rad), -sin(rad), trans_x,  // x
      sin(rad), cos(rad),  trans_y,  // y
      0.0,      0.0,       1.0       // perspective
  };

  if (rotation_degree_ == 90 || rotation_degree_ == 270) {
    std::swap(width, height);
  }

  window_->ResizeRenderTargetWithRotation(
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

void FlutterTizenView::OnKey(Ecore_Event_Key* event, bool is_down) {
  if (is_down) {
    FT_LOG(Info) << "Key symbol: " << event->key << ", code: 0x" << std::setw(8)
                 << std::setfill('0') << std::right << std::hex
                 << event->keycode;
  }

  if (text_input_channel_) {
    if (text_input_channel_->SendKeyEvent(event, is_down)) {
      return;
    }
  }

  if (engine_->platform_view_channel()) {
    engine_->platform_view_channel()->SendKeyEvent(event, is_down);
  }

  if (engine_->key_event_channel()) {
    engine_->key_event_channel()->SendKeyEvent(
        event, is_down,
        [engine = engine_.get(), symbol = std::string(event->key),
         is_down](bool handled) {
          if (handled) {
            return;
          }
          if (symbol == kBackKey && !is_down) {
            if (engine->navigation_channel()) {
              engine->navigation_channel()->PopRoute();
            }
          } else if (symbol == kExitKey && !is_down) {
            ui_app_exit();
          }
        });
  }
}

void FlutterTizenView::SendInitialGeometry() {
  OnRotate(window_->GetRotation());
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
    double dpi = static_cast<double>(window_->GetDpi());
#endif
    double scale_factor = dpi / 90.0 * kProfileFactor;
    computed_pixel_ratio = std::max(scale_factor, 1.0);
  } else {
    computed_pixel_ratio = pixel_ratio;
  }

  engine_->SendWindowMetrics(left, top, width, height, computed_pixel_ratio);
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
  TizenWindow::Geometry geometry = window_->GetWindowGeometry();
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

  engine_->SendPointerEvent(event);
}

}  // namespace flutter
