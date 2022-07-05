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
  flutter::TizenGeometry window_geometry = {
      window_properties.x, window_properties.y, window_properties.width,
      window_properties.height};

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

FlutterDesktopViewRef FlutterDesktopViewCreateFromElmParent(
    const FlutterDesktopViewProperties& view_properties,
    FlutterDesktopEngineRef engine,
    void* parent) {
  std::unique_ptr<flutter::TizenViewBase> tizen_view =
      std::make_unique<flutter::TizenViewElementary>(
          view_properties.width, view_properties.height,
          static_cast<Evas_Object*>(parent));

  auto view =
      std::make_unique<flutter::FlutterTizenView>(std::move(tizen_view));

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

void* FlutterDesktopViewGetEvasObject(FlutterDesktopViewRef view_ref) {
  auto* view = reinterpret_cast<flutter::FlutterTizenView*>(view_ref);
  if (view->tizen_view()->GetType() == flutter::TizenViewType::kView) {
    auto* tizen_view =
        reinterpret_cast<flutter::TizenView*>(view->tizen_view());
    return tizen_view->GetRenderTargetContainer();
  }
  return nullptr;
}
