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

class FlutterTizenView {
 public:
  FlutterTizenView();

  // Configures the window instance with an instance of a running Flutter
  // engine.
  void SetFlutterTizenEngine(
      std::unique_ptr<FlutterTizenEngine> flutter_tizen_engine);

  FlutterTizenEngine* flutter_tizen_engine() {
    return flutter_tizen_engine_.get();
  }

  // Callbacks for clearing context, settings context and swapping buffers,
  // these are typically called on an engine-controlled (non-platform) thread.
  bool OnMakeCurrent();
  bool OnClearCurrent();
  bool OnMakeResourceCurrent();
  bool OnPresent();
  uint32_t OnGetFBO() ;
  void* OnProcResolver(const char* name);

 private:
  // The engine associated with this view.
  std::unique_ptr<FlutterTizenEngine> flutter_tizen_engine_;
};

}  // namespace flutter
#endif
