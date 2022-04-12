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

  flutter_tizen_engine_->SetFlutterTizenView(this);
}

bool FlutterTizenView::OnMakeCurrent() {
  return flutter_tizen_engine_->renderer()->OnMakeCurrent();
}

bool FlutterTizenView::OnClearCurrent() {
  return flutter_tizen_engine_->renderer()->OnClearCurrent();
}

bool FlutterTizenView::OnMakeResourceCurrent() {
  return flutter_tizen_engine_->renderer()->OnMakeResourceCurrent();
}

bool FlutterTizenView::OnPresent() {
  return flutter_tizen_engine_->renderer()->OnPresent();
}
uint32_t FlutterTizenView::OnGetFBO() {
  return flutter_tizen_engine_->renderer()->OnGetFBO();
}

void* FlutterTizenView::OnProcResolver(const char* name) {
  return flutter_tizen_engine_->renderer()->OnProcResolver(name);
}

}  // namespace flutter