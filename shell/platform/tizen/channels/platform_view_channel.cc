// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform_view_channel.h"

#include "flutter/shell/platform/common/client_wrapper/include/flutter/plugin_registrar.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_message_codec.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_method_codec.h"
#include "flutter/shell/platform/common/json_method_codec.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/public/flutter_platform_view.h"
#include "flutter/shell/platform/tizen/tizen_log.h"

namespace flutter {

namespace {
constexpr char kChannelName[] = "flutter/platform_views";
}  // namespace

template <typename T>
bool GetValueFromEncodableMap(const EncodableValue& arguments,
                              std::string key,
                              T* out) {
  if (auto pmap = std::get_if<EncodableMap>(&arguments)) {
    auto iter = pmap->find(EncodableValue(key));
    if (iter != pmap->end() && !iter->second.IsNull()) {
      if (auto pval = std::get_if<T>(&iter->second)) {
        *out = *pval;
        return true;
      }
    }
  }
  return false;
}

PlatformViewChannel::PlatformViewChannel(BinaryMessenger* messenger)
    : channel_(std::make_unique<MethodChannel<EncodableValue>>(
          messenger,
          kChannelName,
          &StandardMethodCodec::GetInstance())) {
  channel_->SetMethodCallHandler(
      [this](const MethodCall<EncodableValue>& call,
             std::unique_ptr<MethodResult<EncodableValue>> result) {
        HandleMethodCall(call, std::move(result));
      });
}

PlatformViewChannel::~PlatformViewChannel() {
  Dispose();
}

void PlatformViewChannel::Dispose() {
  ClearViewInstances();
  ClearViewFactories();
}

void PlatformViewChannel::RemoveViewInstanceIfNeeded(int view_id) {
  auto it = view_instances_.find(view_id);
  if (view_id >= 0 && it != view_instances_.end()) {
    auto view_instance = it->second;
    view_instance->Dispose();
    delete view_instance;
    view_instances_.erase(it);
  }
}

void PlatformViewChannel::ClearViewInstances() {
  // Clean-up view_instances_
  for (auto const& [view_id, view_instance] : view_instances_) {
    view_instance->Dispose();
    delete view_instance;
  }
  view_instances_.clear();
}

void PlatformViewChannel::ClearViewFactories() {
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

int PlatformViewChannel::CurrentFocusedViewId() {
  for (auto it = view_instances_.begin(); it != view_instances_.end(); it++) {
    if (it->second->IsFocused()) {
      return it->second->GetViewId();
    }
  }
  return -1;
}

void PlatformViewChannel::HandleMethodCall(
    const MethodCall<EncodableValue>& call,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
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
    RemoveViewInstanceIfNeeded(view_id);

    EncodableMap values = std::get<EncodableMap>(arguments);
    EncodableValue value = values[EncodableValue("params")];
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
        result->Success(EncodableValue(view_instance->GetTextureId()));
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
  } else if (method == "dispose") {
    int view_id = -1;
    if (std::holds_alternative<int>(arguments)) {
      view_id = std::get<int>(arguments);
    };
    if (view_id < 0 || view_instances_.find(view_id) == view_instances_.end()) {
      result->Error("Can't find view id");
    } else {
      RemoveViewInstanceIfNeeded(view_id);
      result->Success();
    }
  } else {
    int view_id = -1;
    if (!GetValueFromEncodableMap(arguments, "id", &view_id)) {
      result->Error("Invalid arguments");
      return;
    }

    auto it = view_instances_.find(view_id);
    if (view_id >= 0 && it != view_instances_.end()) {
      if (method == "resize") {
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

        EncodableList event;
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
            auto id = std::make_unique<EncodableValue>(view_id);
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
}  // namespace flutter
