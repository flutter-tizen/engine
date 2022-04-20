// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/flutter_tizen.h"

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/flutter_tizen_view.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace {

// Returns the engine corresponding to the given opaque API handle.
flutter::FlutterTizenEngine* EngineFromHandle(FlutterDesktopEngineRef ref) {
  return reinterpret_cast<flutter::FlutterTizenEngine*>(ref);
}

FlutterDesktopViewRef HandleForView(flutter::FlutterTizenView* view) {
  return reinterpret_cast<FlutterDesktopViewRef>(view);
}

}  // namespace

FlutterDesktopViewRef FlutterDesktopViewCreateUsingNewWindow(
    const FlutterDesktopWindowProperties& window_properties,
    FlutterDesktopEngineRef engine) {
  // Todo : Use view and window imple;

  // EngineFromHandle(engine)->InitializeRenderer(
  //     window_properties.x, window_properties.y, window_properties.width,
  //     window_properties.height, window_properties.transparent,
  //     window_properties.focusable, window_properties.top_level);
  // auto flutter_tizen_view_ = std::make_unique<flutter::FlutterTizenView>();

  // flutter::TizenWindowEcoreWl2 window;

  flutter_tizen_view_->SetFlutterTizenEngine(
      std::unique_ptr<flutter::FlutterTizenEngine>(EngineFromHandle(engine)));

  return HandleForView(flutter_tizen_view_.release());
}
