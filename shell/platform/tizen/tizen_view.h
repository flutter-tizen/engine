// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_VIEW_H_
#define EMBEDDER_TIZEN_VIEW_H_

#include <string>

#include "flutter/shell/platform/tizen/tizen_view_base.h"

namespace flutter {

class FlutterTizenView;

class TizenView : public TizenViewBase {
 public:
  TizenView() = default;
  virtual ~TizenView() = default;

  virtual void* GetRenderTargetContainer() = 0;

  TizenViewType GetType() override { return TizenViewType::kView; };

 protected:
  explicit TizenView(int width, int height)
      : initial_width_(width), initial_height_(height) {}

  int initial_width_ = 0;
  int initial_height_ = 0;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_VIEW_H_
