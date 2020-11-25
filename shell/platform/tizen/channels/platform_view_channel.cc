// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "platform_view_channel.h"

#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/plugin_registrar.h"
#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/standard_method_codec.h"
#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/standard_message_codec.h"
#include "flutter/shell/platform/common/cpp/json_method_codec.h"
#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/public/flutter_platform_view.h"

static constexpr char kChannelName[] = "flutter/platform_views";

std::string extractStringFromMap(const flutter::EncodableValue& arguments,
                                 const char* key) {
  if (std::holds_alternative<flutter::EncodableMap>(arguments)) {
    flutter::EncodableMap values = std::get<flutter::EncodableMap>(arguments);
    flutter::EncodableValue value = values[flutter::EncodableValue(key)];
    if (std::holds_alternative<std::string>(value))
      return std::get<std::string>(value);
  }
  return std::string();
}
int extractIntFromMap(const flutter::EncodableValue& arguments,
                      const char* key) {
  if (std::holds_alternative<flutter::EncodableMap>(arguments)) {
    flutter::EncodableMap values = std::get<flutter::EncodableMap>(arguments);
    flutter::EncodableValue value = values[flutter::EncodableValue(key)];
    if (std::holds_alternative<int>(value)) return std::get<int>(value);
  }
  return -1;
}
double extractDoubleFromMap(const flutter::EncodableValue& arguments,
                            const char* key) {
  if (std::holds_alternative<flutter::EncodableMap>(arguments)) {
    flutter::EncodableMap values = std::get<flutter::EncodableMap>(arguments);
    flutter::EncodableValue value = values[flutter::EncodableValue(key)];
    if (std::holds_alternative<double>(value)) return std::get<double>(value);
  }
  return -1;
}

flutter::EncodableMap extractMapFromMap(
    const flutter::EncodableValue& arguments, const char* key) {
  if (std::holds_alternative<flutter::EncodableMap>(arguments)) {
    flutter::EncodableMap values = std::get<flutter::EncodableMap>(arguments);
    flutter::EncodableValue value = values[flutter::EncodableValue(key)];
    if (std::holds_alternative<flutter::EncodableMap>(value))
      return std::get<flutter::EncodableMap>(value);
  }
  return flutter::EncodableMap();
}

PlatformViewChannel::PlatformViewChannel(flutter::BinaryMessenger* messenger)
    : channel_(
          std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
              messenger, kChannelName,
              &flutter::StandardMethodCodec::GetInstance())) {
  channel_->SetMethodCallHandler(
      [this](const flutter::MethodCall<flutter::EncodableValue>& call,
             std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>>
                 result) { HandleMethodCall(call, std::move(result)); });
}

PlatformViewChannel::~PlatformViewChannel() {
  // Clean-up view_factories_
  for (auto const& [viewType, viewFactory] : view_factories_) {
    viewFactory->dispose();
  }
  view_factories_.clear();

  // Clean-up view_instances_
  for (auto const& [viewId, viewInstance] : view_instances_) {
    viewInstance->dispose();
    delete viewInstance;
  }
  view_instances_.clear();
}

void PlatformViewChannel::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  const auto method = call.method_name();
  const auto& arguments = *call.arguments();

  if (method == "create") {
    std::string viewType = extractStringFromMap(arguments, "viewType");
    int viewId = extractIntFromMap(arguments, "id");
    double width = extractDoubleFromMap(arguments, "width");
    double height = extractDoubleFromMap(arguments, "height");

    LoggerD(
        "PlatformViewChannel create viewType: %s id: %d width: %f height: %f ",
        viewType.c_str(), viewId, width, height);

    flutter::EncodableMap values = std::get<flutter::EncodableMap>(arguments);
    flutter::EncodableValue value = values[flutter::EncodableValue("params")];
    ByteMessage byteMessage;
    if (std::holds_alternative<ByteMessage>(value)) {
      byteMessage = std::get<ByteMessage>(value);
    }
    auto it = view_factories_.find(viewType);
    if (it != view_factories_.end()) {
      auto viewInstance =
          it->second->create(viewId, width, height, byteMessage);
      view_instances_.insert(
          std::pair<int, PlatformView*>(viewId, viewInstance));
      result->Success(flutter::EncodableValue(viewInstance->getTextureId()));
    } else {
      LoggerE("can't find view type = %s", viewType.c_str());
      result->Error("0", "can't find view type");
    }
  } else {
    int viewId = extractIntFromMap(arguments, "id");
    auto it = view_instances_.find(viewId);
    if (viewId >= 0 && it != view_instances_.end()) {
      if (method == "dispose") {
        LoggerD("PlatformViewChannel dispose");
        it->second->dispose();
        result->Success();
      } else if (method == "resize") {
        LoggerD("PlatformViewChannel resize");
        double width = extractDoubleFromMap(arguments, "width");
        double height = extractDoubleFromMap(arguments, "height");
        it->second->resize(width, height);
        result->NotImplemented();
      } else if (method == "touch") {
        LoggerD("PlatformViewChannel touch");
        result->NotImplemented();
      } else if (method == "setDirection") {
        LoggerD("PlatformViewChannel setDirection");
        result->NotImplemented();
      } else if (method == "clearFocus") {
        LoggerD("PlatformViewChannel clearFocus");
        it->second->clearFocus();
        result->NotImplemented();
      } else {
        LoggerD("Unimplemented method: %s", method.c_str());
        result->NotImplemented();
      }
    } else {
      LoggerE("can't find view id");
      result->Error("0", "can't find view id");
    }
  }
}
