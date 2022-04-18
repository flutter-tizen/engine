// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "window_channel.h"

#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_method_codec.h"
#include "flutter/shell/platform/tizen/channels/encodable_value_holder.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "tizen/internal/window";

}  // namespace

WindowChannel::WindowChannel(BinaryMessenger* messenger,
                             TizenRenderer* renderer,
                             TizenRenderer::Delegate* delegate)
    : renderer_(renderer), delegate_(delegate) {
  channel_ = std::make_unique<MethodChannel<EncodableValue>>(
      messenger, kChannelName, &StandardMethodCodec::GetInstance());
  channel_->SetMethodCallHandler(
      [this](const MethodCall<EncodableValue>& call,
             std::unique_ptr<MethodResult<EncodableValue>> result) {
        this->HandleMethodCall(call, std::move(result));
      });
}

WindowChannel::~WindowChannel() {}

void WindowChannel::HandleMethodCall(
    const MethodCall<EncodableValue>& method_call,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  const std::string& method_name = method_call.method_name();

  if (method_name == "getWindowGeometry") {
    TizenRenderer::Geometry geometry = renderer_->GetWindowGeometry();
    EncodableMap map;
    map[EncodableValue("x")] = EncodableValue(geometry.x);
    map[EncodableValue("y")] = EncodableValue(geometry.y);
    map[EncodableValue("width")] = EncodableValue(geometry.w);
    map[EncodableValue("height")] = EncodableValue(geometry.h);
    result->Success(EncodableValue(map));
  } else if (method_name == "setWindowGeometry") {
#ifdef TIZEN_RENDERER_EVAS_GL
    FT_LOG(Error) << "setWindowGeometry is not supported on Evas GL.";
    result->NotImplemented();
#else
    const auto* arguments = std::get_if<EncodableMap>(method_call.arguments());
    if (!arguments) {
      result->Error("Invalid arguments");
      return;
    }
    EncodableValueHolder<int32_t> x(arguments, "x");
    EncodableValueHolder<int32_t> y(arguments, "y");
    EncodableValueHolder<int32_t> width(arguments, "width");
    EncodableValueHolder<int32_t> height(arguments, "height");

    TizenRenderer::Geometry geometry = renderer_->GetWindowGeometry();

    delegate_->OnGeometryChange(x ? *x : geometry.x, y ? *y : geometry.y,
                                width ? *width : geometry.w,
                                height ? *height : geometry.h);
    result->Success();
#endif
  } else if (method_name == "getScreenGeometry") {
    TizenRenderer::Geometry geometry = renderer_->GetScreenGeometry();
    EncodableMap map;
    map[EncodableValue("width")] = EncodableValue(geometry.w);
    map[EncodableValue("height")] = EncodableValue(geometry.h);
    result->Success(EncodableValue(map));
  } else {
    result->NotImplemented();
  }
}

}  // namespace flutter
