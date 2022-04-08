// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_tizen_view.h"

namespace flutter {

FlutterTizenView::FlutterTizenView() {}

void FlutterTizenView::SetFlutterTizenEngine(
    std::unique_ptr<FlutterTizenEngine> flutter_tizen_engine) {
  flutter_tizen_engine_ = std::move(flutter_tizen_engine);

  // engine->setFlutterTizenView
}

}  // namespace flutter