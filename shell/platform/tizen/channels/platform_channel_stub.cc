// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform_channel.h"

namespace flutter {

PlatformChannel::PlatformChannel(BinaryMessenger* messenger,
                                 TizenRenderer* renderer) {
  renderer_ = nullptr;
}

PlatformChannel::~PlatformChannel() {}

void PlatformChannel::HandleMethodCall(
    const MethodCall<rapidjson::Document>& call,
    std::unique_ptr<MethodResult<rapidjson::Document>> result) {}

namespace clipboard {

void GetData(const MethodCall<rapidjson::Document>& call,
             std::unique_ptr<MethodResult<rapidjson::Document>> result) {}

void SetData(const MethodCall<rapidjson::Document>& call,
             std::unique_ptr<MethodResult<rapidjson::Document>> result) {}

}  // namespace clipboard

}  // namespace flutter
