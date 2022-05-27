// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_WINDOW_H_
#define EMBEDDER_TIZEN_WINDOW_H_

#include <cstdint>
#include <string>
#include <vector>

#include "flutter/shell/platform/tizen/tizen_input_method_context.h"

namespace flutter {

class FlutterTizenView;

class TizenWindow {
 public:
  struct Geometry {
    int32_t left = 0, top = 0, width = 0, height = 0;
  };

  virtual ~TizenWindow() = default;

  // Sets the delegate used to communicate state changes from window to view
  // such as key presses, mouse position updates etc.
  void SetView(FlutterTizenView* view) { view_ = view; }

  // Returns the geometry of the current window.
  virtual Geometry GetWindowGeometry() = 0;

  // Set the geometry of the current window.
  virtual void SetWindowGeometry(Geometry geometry) = 0;

  // Returns the geometry of the display screen.
  virtual Geometry GetScreenGeometry() = 0;

  // Returns a valid pointer the platform object that rendering can be bound to
  // by rendering backend.
  virtual void* GetRenderTarget() = 0;

  virtual void* GetRenderTargetDisplay() = 0;

  virtual int32_t GetRotation() = 0;

  // Returns the dpi of the screen.
  virtual int32_t GetDpi() = 0;

  virtual uintptr_t GetWindowId() = 0;

  virtual void* GetWindowHandle() = 0;

  virtual void ResizeRenderTargetWithRotation(Geometry geometry,
                                              int32_t degree) = 0;

  virtual void SetPreferredOrientations(const std::vector<int>& rotations) = 0;

  virtual void BindKeys(const std::vector<std::string>& keys) = 0;

  virtual void Show() = 0;

  // FIXME
  // This is a temporary implementation that is only used by the window channel.
  virtual void OnGeometryChanged(Geometry geometry) = 0;

  TizenInputMethodContext* input_method_context() {
    return input_method_context_.get();
  }

 protected:
  explicit TizenWindow(Geometry geometry,
                       bool transparent,
                       bool focusable,
                       bool top_level)
      : initial_geometry_(geometry),
        transparent_(transparent),
        focusable_(focusable),
        top_level_(top_level) {}

  Geometry initial_geometry_ = {0, 0, 0, 0};
  bool transparent_ = false;
  bool focusable_ = false;
  bool top_level_ = false;

  FlutterTizenView* view_ = nullptr;

  // The Tizen input method context. nullptr if not set.
  std::unique_ptr<TizenInputMethodContext> input_method_context_;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_WINDOW_H_
