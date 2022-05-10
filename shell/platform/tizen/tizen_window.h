// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_WINDOW_H_
#define EMBEDDER_TIZEN_WINDOW_H_

#include <cstdint>
#include <string>
#include <vector>

#include "flutter/shell/platform/tizen/tizen_base_handle.h"

namespace flutter {

class FlutterTizenView;

class TizenWindow : public TizenBaseHandle {
 public:
  TizenWindow();
  virtual ~TizenWindow() = default;

 protected:
  explicit TizenWindow(TizenBaseHandle::Geometry geometry,
                       bool transparent,
                       bool focusable,
                       bool top_level)
      : initial_geometry_(geometry),
        transparent_(transparent),
        focusable_(focusable),
        top_level_(top_level) {}

  TizenBaseHandle::Geometry initial_geometry_ = {0, 0, 0, 0};
  bool transparent_ = false;
  bool focusable_ = false;
  bool top_level_ = false;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_WINDOW_H_
