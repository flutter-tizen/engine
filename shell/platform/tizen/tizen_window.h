// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_WINDOW_H_
#define EMBEDDER_TIZEN_WINDOW_H_

#include "flutter/shell/platform/tizen/tizen_view_base.h"

#include <cstdint>
#include <string>
#include <vector>

namespace flutter {

class FlutterTizenView;

class TizenWindow : public TizenViewBase {
 public:
  TizenWindow() = default;
  virtual ~TizenWindow() = default;

  virtual void* GetWindowHandle();

  virtual int32_t GetRotation() = 0;

  virtual void SetPreferredOrientations(const std::vector<int>& rotations);

  std::string GetType() override { return "window"; };

 protected:
  explicit TizenWindow(TizenViewBase::Geometry geometry,
                       bool transparent,
                       bool focusable,
                       bool top_level)
      : initial_geometry_(geometry),
        transparent_(transparent),
        focusable_(focusable),
        top_level_(top_level) {}

  TizenViewBase::Geometry initial_geometry_ = {0, 0, 0, 0};
  bool transparent_ = false;
  bool focusable_ = false;
  bool top_level_ = false;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_WINDOW_H_
