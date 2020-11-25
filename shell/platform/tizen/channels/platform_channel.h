// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_PLATFORM_CHANNEL_H_
#define EMBEDDER_PLATFORM_CHANNEL_H_

#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/method_channel.h"
#include "rapidjson/document.h"

class PlatformChannel {
 public:
  explicit PlatformChannel(flutter::BinaryMessenger* messenger);
  virtual ~PlatformChannel();

 private:
  std::unique_ptr<flutter::MethodChannel<rapidjson::Document>> channel_;

  void HandleMethodCall(
      const flutter::MethodCall<rapidjson::Document>& call,
      std::unique_ptr<flutter::MethodResult<rapidjson::Document>> result);
};

#endif  //  EMBEDDER_PLATFORM_CHANNEL_H_
