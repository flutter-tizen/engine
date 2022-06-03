// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_BASE_HANDLE_H_
#define EMBEDDER_TIZEN_BASE_HANDLE_H_

#include <cstdint>
#include <string>
#include <vector>

#include "flutter/shell/platform/tizen/tizen_input_method_context.h"

namespace flutter {

class FlutterTizenView;

class TizenViewBase {
 public:
  struct Geometry {
    int32_t left = 0, top = 0, width = 0, height = 0;
  };

  TizenViewBase(){};

  virtual ~TizenViewBase() = default;

  // Returns a valid pointer the platform object that rendering can be bound to
  // by rendering backend.
  virtual void* GetRenderTarget();

  virtual void* GetRenderTargetDisplay();

  virtual uintptr_t GetWindowId();

  // Returns the geometry of the current render target.
  virtual Geometry GetRenderTargetGeometry() = 0;

  // Set the geometry of the current render target.
  virtual void SetRenderTargetGeometry(Geometry geometry) = 0;

  // Returns the geometry of the display screen.
  virtual Geometry GetScreenGeometry() = 0;

  // Returns the dpi of the screen.
  virtual int32_t GetDpi() = 0;

  // Sets the delegate used to communicate state changes from render target to
  // view such as key presses, mouse position updates etc.
  void SetView(FlutterTizenView* view) { view_ = view; }

  virtual void BindKeys(const std::vector<std::string>& keys) = 0;

  virtual void ResizeRenderTargetWithRotation(Geometry geometry,
                                              int32_t degree) = 0;

  // FIXME
  // This is a temporary implementation that is only used by the window channel.
  virtual void OnGeometryChanged(Geometry geometry) = 0;

  virtual void Show() = 0;

  virtual TizenInputMethodContext* input_method_context() = 0;

  virtual std::string GetType() = 0;

 protected:
  FlutterTizenView* view_ = nullptr;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_BASE_HANDLE_H_
