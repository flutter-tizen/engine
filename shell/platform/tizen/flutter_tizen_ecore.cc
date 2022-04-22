// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/flutter_tizen.h"

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/flutter_tizen_view.h"
#include "flutter/shell/platform/tizen/tizen_window_ecore_wl2.h"

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
  std::unique_ptr<flutter::TizenWindow> flutter_tizen_window =
      std::make_unique<flutter::TizenWindowEcoreWl2>(
          flutter::TizenWindow::Geometry(
              {window_properties.x, window_properties.y,
               window_properties.width, window_properties.height}),
          window_properties.transparent, window_properties.focusable,
          window_properties.top_level);

  // Take ownership of the engine, starting it if necessary.
  auto flutter_tizen_view = std::make_unique<flutter::FlutterTizenView>(
      std::move(flutter_tizen_window));

  flutter_tizen_view->SetFlutterTizenEngine(
      std::unique_ptr<flutter::FlutterTizenEngine>(EngineFromHandle(engine)));

  flutter_tizen_view->CreateRenderSurface();

  if (!flutter_tizen_view->flutter_tizen_engine()->IsRunning()) {
    flutter_tizen_view->flutter_tizen_engine()->RunEngine();
  }

  flutter_tizen_view->SendInitialGeometry();

  return HandleForView(flutter_tizen_view.release());
}
