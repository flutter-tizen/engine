// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/flutter_tizen.h"

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/flutter_tizen_view.h"
#include "flutter/shell/platform/tizen/tizen_view_elementary.h"
#include "flutter/shell/platform/tizen/tizen_window_elementary.h"

namespace {

// Returns the engine corresponding to the given opaque API handle.
flutter::FlutterTizenEngine* EngineFromHandle(FlutterDesktopEngineRef ref) {
  return reinterpret_cast<flutter::FlutterTizenEngine*>(ref);
}

FlutterDesktopViewRef HandleForView(flutter::FlutterTizenView* view) {
  return reinterpret_cast<FlutterDesktopViewRef>(view);
}

}  // namespace

FlutterDesktopViewRef FlutterDesktopViewCreateFromNewWindow(
    const FlutterDesktopWindowProperties& window_properties,
    FlutterDesktopEngineRef engine) {
  flutter::TizenBaseHandle::Geometry window_geometry = {
      window_properties.x,
      window_properties.y,
      window_properties.width,
      window_properties.height,
  };

  std::unique_ptr<flutter::TizenWindow> window =
      std::make_unique<flutter::TizenWindowElementary>(
          window_geometry, window_properties.transparent,
          window_properties.focusable, window_properties.top_level);

  auto view = std::make_unique<flutter::FlutterTizenView>(std::move(window));

  // Take ownership of the engine, starting it if necessary.
  view->SetEngine(
      std::unique_ptr<flutter::FlutterTizenEngine>(EngineFromHandle(engine)));
  view->CreateRenderSurface();
  if (!view->engine()->IsRunning()) {
    if (!view->engine()->RunEngine()) {
      return nullptr;
    }
  }

  view->SendInitialGeometry();

  return HandleForView(view.release());
}

FlutterDesktopViewRef FlutterDesktopViewCreateFromNewView(
    const FlutterDesktopViewProperties& view_properties,
    FlutterDesktopEngineRef engine,
    void* elm_parent) {
  flutter::TizenBaseHandle::Geometry view_geometry = {
      0,
      0,
      view_properties.width,
      view_properties.height,
  };

  std::unique_ptr<flutter::TizenBaseHandle> view =
      std::make_unique<flutter::TizenViewElementary>(
          view_geometry, (Evas_Object*)(view_properties.elm_parent));

  auto flutter_view =
      std::make_unique<flutter::FlutterTizenView>(std::move(view));

  // Take ownership of the engine, starting it if necessary.
  flutter_view->SetEngine(
      std::unique_ptr<flutter::FlutterTizenEngine>(EngineFromHandle(engine)));
  flutter_view->CreateRenderSurface();
  if (!flutter_view->engine()->IsRunning()) {
    if (!flutter_view->engine()->RunEngine()) {
      return nullptr;
    }
  }

  flutter_view->SendInitialGeometry();

  return HandleForView(flutter_view.release());
}

void* FlutterDesktopViewGetEvasObject(FlutterDesktopEngineRef engine) {
  flutter::TizenViewElementary* tizenView =
      (flutter::TizenViewElementary*)EngineFromHandle(engine)->view()->handle();
  return tizenView->GetEvasContainer();
}

void FlutterDesktopViewResizeView(FlutterDesktopEngineRef engine,
                                  int32_t width,
                                  int32_t height) {
  flutter::TizenViewElementary* tizenView =
      (flutter::TizenViewElementary*)EngineFromHandle(engine)->view()->handle();
  flutter::TizenBaseHandle::Geometry view_geometry = {
      0,
      0,
      width,
      height,
  };
  tizenView->OnGeometryChanged(view_geometry);
}
