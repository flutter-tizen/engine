// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "platform_view_channel.h"

#include "flutter/shell/platform/common/client_wrapper/include/flutter/plugin_registrar.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_message_codec.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_method_codec.h"
#include "flutter/shell/platform/common/json_method_codec.h"
#include "flutter/shell/platform/tizen/channels/text_input_channel.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/public/flutter_platform_view.h"
#include "flutter/shell/platform/tizen/tizen_log.h"

static constexpr char kChannelName[] = "flutter/platform_views";

template <typename T>
bool GetValueFromEncodableMap(const flutter::EncodableValue& arguments,
                              std::string key,
                              T* out) {
  if (auto pmap = std::get_if<flutter::EncodableMap>(&arguments)) {
    auto iter = pmap->find(flutter::EncodableValue(key));
    if (iter != pmap->end() && !iter->second.IsNull()) {
      if (auto pval = std::get_if<T>(&iter->second)) {
        *out = *pval;
        return true;
      }
    }
  }
  return false;
}

PlatformViewChannel::PlatformViewChannel(flutter::BinaryMessenger* messenger,
                                         FlutterTizenEngine* engine)
    : engine_(engine),
      channel_(
          std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
              messenger,
              kChannelName,
              &flutter::StandardMethodCodec::GetInstance())) {
  channel_->SetMethodCallHandler(
      [this](const flutter::MethodCall<flutter::EncodableValue>& call,
             std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>>
                 result) { HandleMethodCall(call, std::move(result)); });
}

PlatformViewChannel::~PlatformViewChannel() {
  Dispose();
}

void PlatformViewChannel::Dispose() {
  // Clean-up view_instances_
  for (auto const& [view_id, view_instance] : view_instances_) {
    delete view_instance;
  }
  view_instances_.clear();

  // Clean-up view_factories_
  for (auto const& [view_type, view_factory] : view_factories_) {
    view_factory->Dispose();
  }
  view_factories_.clear();
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

void PlatformViewChannel::DispatchCompositionUpdateEvent(
    const std::string& key) {
  auto instances = ViewInstances();
  auto it = instances.find(CurrentFocusedViewId());
  if (it != instances.end()) {
    it->second->DispatchCompositionUpdateEvent(key.c_str(), key.size());
  }
}

void PlatformViewChannel::DispatchCompositionEndEvent(const std::string& key) {
  auto instances = ViewInstances();
  auto it = instances.find(CurrentFocusedViewId());
  if (it != instances.end()) {
    it->second->DispatchCompositionEndEvent(key.c_str(), key.size());
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
    std::string view_type;
    int view_id = -1;
    double width = 0.0, height = 0.0;
    if (!GetValueFromEncodableMap(arguments, "viewType", &view_type) ||
        !GetValueFromEncodableMap(arguments, "id", &view_id) ||
        !GetValueFromEncodableMap(arguments, "width", &width) ||
        !GetValueFromEncodableMap(arguments, "height", &height)) {
      result->Error("Invalid arguments");
      return;
    }

    FT_LOGI(
        "PlatformViewChannel create viewType: %s id: %d width: %f height: %f ",
        view_type.c_str(), view_id, width, height);

    flutter::EncodableMap values = std::get<flutter::EncodableMap>(arguments);
    flutter::EncodableValue value = values[flutter::EncodableValue("params")];
    ByteMessage byte_message;
    if (std::holds_alternative<ByteMessage>(value)) {
      byte_message = std::get<ByteMessage>(value);
    }
    auto it = view_factories_.find(view_type);
    if (it != view_factories_.end()) {
      auto focused_view = view_instances_.find(CurrentFocusedViewId());
      if (focused_view != view_instances_.end()) {
        focused_view->second->SetFocus(false);
      }

      auto view_instance =
          it->second->Create(view_id, width, height, byte_message);
      if (view_instance) {
        view_instances_.insert(
            std::pair<int, PlatformView*>(view_id, view_instance));

        if (engine_ && engine_->text_input_channel) {
          Ecore_IMF_Context* context =
              engine_->text_input_channel->GetImfContext();
          view_instance->SetSoftwareKeyboardContext(context);
        }
        result->Success(flutter::EncodableValue(view_instance->GetTextureId()));
      } else {
        result->Error("Can't create a webview instance!!");
      }
    } else {
      FT_LOGE("can't find view type = %s", view_type.c_str());
      result->Error("Can't find view type");
    }
  } else if (method == "clearFocus") {
    int view_id = -1;
    if (std::holds_alternative<int>(arguments)) {
      view_id = std::get<int>(arguments);
    };
    auto it = view_instances_.find(view_id);
    if (view_id >= 0 && it != view_instances_.end()) {
      it->second->SetFocus(false);
      it->second->ClearFocus();
      result->Success();
    } else {
      result->Error("Can't find view id");
    }
  } else {
    int view_id = -1;
    if (!GetValueFromEncodableMap(arguments, "id", &view_id)) {
      result->Error("Invalid arguments");
      return;
    }

    auto it = view_instances_.find(view_id);
    if (view_id >= 0 && it != view_instances_.end()) {
      if (method == "dispose") {
        it->second->Dispose();
        result->Success();
      } else if (method == "resize") {
        double width = 0.0, height = 0.0;
        if (!GetValueFromEncodableMap(arguments, "width", &width) ||
            !GetValueFromEncodableMap(arguments, "height", &height)) {
          result->Error("Invalid arguments");
          return;
        }
        it->second->Resize(width, height);
        result->Success();
      } else if (method == "touch") {
        int type = 0, button = 0;
        double x = 0.0, y = 0.0, dx = 0.0, dy = 0.0;

        flutter::EncodableList event;
        if (!GetValueFromEncodableMap(arguments, "event", &event) ||
            event.size() != 6) {
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

        if (!it->second->IsFocused()) {
          auto focused_view = view_instances_.find(CurrentFocusedViewId());
          if (focused_view != view_instances_.end()) {
            focused_view->second->SetFocus(false);
          }

          it->second->SetFocus(true);
          if (channel_ != nullptr) {
            auto id = std::make_unique<flutter::EncodableValue>(view_id);
            channel_->InvokeMethod("viewFocused", std::move(id));
          }
        }

        result->Success();
      } else if (method == "setDirection") {
        FT_LOGW("PlatformViewChannel setDirection - not implemented");
        result->NotImplemented();
      } else {
        FT_LOGW("Unimplemented method: %s", method.c_str());
        result->NotImplemented();
      }
    } else {
      result->Error("Can't find view id");
    }
  }
}
