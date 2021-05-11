// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_PLATFORM_CHANNEL_H_
#define EMBEDDER_PLATFORM_CHANNEL_H_

#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/method_channel.h"
#include "flutter/shell/platform/tizen/tizen_renderer.h"
#include "rapidjson/document.h"

class PlatformChannel {
 public:
  explicit PlatformChannel(flutter::BinaryMessenger* messenger,
                           TizenRenderer* renderer);
  virtual ~PlatformChannel();

 private:
  void HandleMethodCall(
      const flutter::MethodCall<rapidjson::Document>& call,
      std::unique_ptr<flutter::MethodResult<rapidjson::Document>> result);

  std::unique_ptr<flutter::MethodChannel<rapidjson::Document>> channel_;

  // A reference to the renderer object managed by FlutterTizenEngine.
  // This can be nullptr if the engine is running in headless mode.
  TizenRenderer* renderer_;
};

#endif  //  EMBEDDER_PLATFORM_CHANNEL_H_
