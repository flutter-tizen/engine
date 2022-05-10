// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_VIEW_H_
#define EMBEDDER_TIZEN_VIEW_H_

#include <cstdint>

#include "flutter/shell/platform/tizen/tizen_base_handle.h"

namespace flutter {

class FlutterTizenView;

class TizenView : public TizenBaseHandle {
 public:
  TizenView();

  void* GetWindowHandle() override { return nullptr; }

  void* GetRenderTarget() override { return nullptr; }

  int32_t GetRotation() override { return 0; };

  void SetPreferredOrientations(const std::vector<int>& rotations) override{};

  virtual ~TizenView() = default;

 protected:
  explicit TizenView(TizenBaseHandle::Geometry geometry)
      : initial_geometry_(geometry) {}

  TizenBaseHandle::Geometry initial_geometry_ = {0, 0, 0, 0};
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_VIEW_H_
