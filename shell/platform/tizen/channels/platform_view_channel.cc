// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "platform_view_channel.h"

#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/plugin_registrar.h"
#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/standard_message_codec.h"
#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/standard_method_codec.h"
#include "flutter/shell/platform/common/cpp/json_method_codec.h"
#include "flutter/shell/platform/tizen/public/flutter_platform_view.h"
#include "flutter/shell/platform/tizen/tizen_log.h"

static constexpr char kChannelName[] = "flutter/platform_views";

std::string ExtractStringFromMap(const flutter::EncodableValue& arguments,
                                 const char* key) {
  if (std::holds_alternative<flutter::EncodableMap>(arguments)) {
    flutter::EncodableMap values = std::get<flutter::EncodableMap>(arguments);
    flutter::EncodableValue value = values[flutter::EncodableValue(key)];
    if (std::holds_alternative<std::string>(value))
      return std::get<std::string>(value);
  }
  return std::string();
}
int ExtractIntFromMap(const flutter::EncodableValue& arguments,
                      const char* key) {
  if (std::holds_alternative<flutter::EncodableMap>(arguments)) {
    flutter::EncodableMap values = std::get<flutter::EncodableMap>(arguments);
    flutter::EncodableValue value = values[flutter::EncodableValue(key)];
    if (std::holds_alternative<int>(value)) return std::get<int>(value);
  }
  return -1;
}
double ExtractDoubleFromMap(const flutter::EncodableValue& arguments,
                            const char* key) {
  if (std::holds_alternative<flutter::EncodableMap>(arguments)) {
    flutter::EncodableMap values = std::get<flutter::EncodableMap>(arguments);
    flutter::EncodableValue value = values[flutter::EncodableValue(key)];
    if (std::holds_alternative<double>(value)) return std::get<double>(value);
  }
  return -1;
}

flutter::EncodableMap ExtractMapFromMap(
    const flutter::EncodableValue& arguments, const char* key) {
  if (std::holds_alternative<flutter::EncodableMap>(arguments)) {
    flutter::EncodableMap values = std::get<flutter::EncodableMap>(arguments);
    flutter::EncodableValue value = values[flutter::EncodableValue(key)];
    if (std::holds_alternative<flutter::EncodableMap>(value))
      return std::get<flutter::EncodableMap>(value);
  }
  return flutter::EncodableMap();
}

flutter::EncodableList ExtractListFromMap(
    const flutter::EncodableValue& arguments, const char* key) {
  if (std::holds_alternative<flutter::EncodableMap>(arguments)) {
    flutter::EncodableMap values = std::get<flutter::EncodableMap>(arguments);
    flutter::EncodableValue value = values[flutter::EncodableValue(key)];
    if (std::holds_alternative<flutter::EncodableList>(value))
      return std::get<flutter::EncodableList>(value);
  }
  return flutter::EncodableList();
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
    viewFactory->Dispose();
  }
  view_factories_.clear();

  // Clean-up view_instances_
  for (auto const& [viewId, viewInstance] : view_instances_) {
    viewInstance->Dispose();
    delete viewInstance;
  }
  view_instances_.clear();
}

void PlatformViewChannel::SendKeyEvent(Ecore_Event_Key* key, bool is_down) {
  auto instances = ViewInstances();
  auto it = instances.find(CurrentFocusedViewId());
  if (it != instances.end()) {
    if (is_down) {
      it->second->DispatchKeyDownEvent(key);
    } else {
      it->second->DispatchKeyUpEvent(key);
    }
  }
}

int PlatformViewChannel::CurrentFocusedViewId() {
  for (auto it = view_instances_.begin(); it != view_instances_.end(); it++) {
    if (it->second->IsFocused()) {
      return it->second->GetViewId();
    }
  }
  return -1;
}

void PlatformViewChannel::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  const auto method = call.method_name();
  const auto& arguments = *call.arguments();

  if (method == "create") {
    std::string viewType = ExtractStringFromMap(arguments, "viewType");
    int viewId = ExtractIntFromMap(arguments, "id");
    double width = ExtractDoubleFromMap(arguments, "width");
    double height = ExtractDoubleFromMap(arguments, "height");

    FT_LOGD(
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
      auto focuesdView = view_instances_.find(CurrentFocusedViewId());
      if (focuesdView != view_instances_.end()) {
        focuesdView->second->SetFocus(false);
      }

      auto viewInstance =
          it->second->Create(viewId, width, height, byteMessage);
      viewInstance->SetFocus(true);
      view_instances_.insert(
          std::pair<int, PlatformView*>(viewId, viewInstance));

      if (channel_ != nullptr) {
        auto id = std::make_unique<flutter::EncodableValue>(viewId);
        channel_->InvokeMethod("viewFocused", std::move(id));
      }

      result->Success(flutter::EncodableValue(viewInstance->GetTextureId()));
    } else {
      FT_LOGE("can't find view type = %s", viewType.c_str());
      result->Error("0", "can't find view type");
    }
  } else {
    int viewId = ExtractIntFromMap(arguments, "id");
    auto it = view_instances_.find(viewId);
    if (viewId >= 0 && it != view_instances_.end()) {
      if (method == "dispose") {
        FT_LOGD("PlatformViewChannel dispose");
        it->second->Dispose();
        result->Success();
      } else if (method == "resize") {
        FT_LOGD("PlatformViewChannel resize");
        double width = ExtractDoubleFromMap(arguments, "width");
        double height = ExtractDoubleFromMap(arguments, "height");
        it->second->Resize(width, height);
        result->NotImplemented();
      } else if (method == "touch") {
        int type, button;
        double x, y, dx, dy;

        flutter::EncodableList event = ExtractListFromMap(arguments, "event");
        if (event.size() != 6) {
          result->Error("Invalid Arguments");
          return;
        }
        type = std::get<int>(event[0]);
        button = std::get<int>(event[1]);
        x = std::get<double>(event[2]);
        y = std::get<double>(event[3]);
        dx = std::get<double>(event[4]);
        dy = std::get<double>(event[5]);

        it->second->Touch(type, button, x, y, dx, dy);
        result->Success();
      } else if (method == "setDirection") {
        FT_LOGD("PlatformViewChannel setDirection");
        result->NotImplemented();
      } else if (method == "clearFocus") {
        FT_LOGD("PlatformViewChannel clearFocus");
        it->second->ClearFocus();
        result->NotImplemented();
      } else {
        FT_LOGD("Unimplemented method: %s", method.c_str());
        result->NotImplemented();
      }
    } else {
      FT_LOGE("can't find view id");
      result->Error("0", "can't find view id");
    }
  }
}
