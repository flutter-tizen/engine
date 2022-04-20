// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_FLUTTER_TIZEN_VIEW_H_
#define EMBEDDER_FLUTTER_TIZEN_VIEW_H_

#include <memory>

#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"

namespace flutter {

class FlutterTizenWindow;

class FlutterTizenView {
 public:
  FlutterTizenView(std::unique_ptr<FlutterTizenWindow> flutter_tizen_window);

  ~FlutterTizenView();

  // Configures the window instance with an instance of a running Flutter
  // engine.
  void SetFlutterTizenEngine(
      std::unique_ptr<FlutterTizenEngine> flutter_tizen_engine);

  FlutterTizenEngine* flutter_tizen_engine() {
    return flutter_tizen_engine_.get();
  }

  // Creates rendering surface for Flutter engine to draw into.
  // Should be called before calling FlutterEngineRun using this view.
  void CreateRenderSurface();

  // Destroys current rendering surface if one has been allocated.
  void DestroyRenderSurface();

  // Callbacks for clearing context, settings context and swapping buffers,
  // these are typically called on an engine-controlled (non-platform) thread.
  bool OnMakeCurrent();

  bool OnClearCurrent();

  bool OnMakeResourceCurrent();

  bool OnPresent();

  uint32_t OnGetFBO();

  void* OnProcResolver(const char* name);

  // ========== Window Event Delegate ==========
  void OnResize(int32_t left, int32_t top, int32_t width, int32_t height);

  void OnRotate(int32_t degree);

  void OnPointerMove(double x,
                     double y,
                     size_t timestamp,
                     FlutterPointerDeviceKind device_kind,
                     int32_t device_id);

  void OnPointerDown(double x,
                     double y,
                     size_t timestamp,
                     FlutterPointerDeviceKind device_kind,
                     int32_t device_id);

  void OnPointerUp(double x,
                   double y,
                   size_t timestamp,
                   FlutterPointerDeviceKind device_kind,
                   int32_t device_id);

  void OnScroll(double x,
                double y,
                double delta_x,
                double delta_y,
                int scroll_offset_multiplier,
                size_t timestamp,
                FlutterPointerDeviceKind device_kind,
                int32_t device_id);

  FlutterTransformation GetFlutterTransformation() {
    return flutter_trans_formation_;
  }

  void SendInitialGeometry();

 private:
  // Sends a window metrics update to the Flutter engine using current window
  // dimensions in physical
  void SendWindowMetrics(int32_t left,
                         int32_t top,
                         int32_t width,
                         int32_t height,
                         double pixel_ratio);

  // Reports pointer event to Flutter engine
  void SendFlutterPointerEvent(FlutterPointerPhase phase,
                               double x,
                               double y,
                               double delta_x,
                               double delta_y,
                               size_t timestamp,
                               FlutterPointerDeviceKind device_kind,
                               int device_id);

  // The engine associated with this view.
  std::unique_ptr<FlutterTizenEngine> flutter_tizen_engine_;

  // TODO
  std::unique_ptr<FlutterTizenWindow> flutter_tizen_window_;

  // The plugin registrar managing internal plugins.
  std::unique_ptr<PluginRegistrar> internal_plugin_registrar_;

  // A plugin that implements the Tizen window channel.
  std::unique_ptr<WindowChannel> window_channel_;

  // A plugin that implements the Flutter platform channel.
  std::unique_ptr<PlatformChannel> platform_channel_;

  // The current view rotation degree.
  int32_t rotation_degree_ = 0;

  // The current pointer state to distinguish move or hover event.
  bool pointer_state_ = false;

  // The current view transformation.
  FlutterTransformation flutter_trans_formation_ = {1.0, 0.0, 0.0, 0.0, 1.0,
                                                    0.0, 0.0, 0.0, 1.0};
};

}  // namespace flutter
#endif
