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

  void OnRotate(int32_t degree);

  FlutterTransformation GetFlutterTransformation() {
    return flutter_trans_formation_;
  }

 private:
  // Sends a window metrics update to the Flutter engine using current window
  // dimensions in physical
  void SendWindowMetrics(int32_t left,
                         int32_t top,
                         int32_t width,
                         int32_t height,
                         double pixel_ratio);

  // The engine associated with this view.
  std::unique_ptr<FlutterTizenEngine> flutter_tizen_engine_;

  // TODO
 public:
  std::unique_ptr<FlutterTizenWindow> flutter_tizen_window_;

 private:
  // The current renderer transformation.
  FlutterTransformation flutter_trans_formation_ = {1.0, 0.0, 0.0, 0.0, 1.0,
                                                    0.0, 0.0, 0.0, 1.0};
};

}  // namespace flutter
#endif
