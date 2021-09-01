// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform_view_channel.h"

#include "flutter/shell/platform/common/client_wrapper/include/flutter/plugin_registrar.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_message_codec.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_method_codec.h"
#include "flutter/shell/platform/common/json_method_codec.h"
#include "flutter/shell/platform/tizen/channels/encodable_value_holder.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/public/flutter_platform_view.h"

namespace flutter {

namespace {
constexpr char kChannelName[] = "flutter/platform_views";
}  // namespace

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
  const auto arguments = call.arguments();

  if (method == "create") {
    OnCreate(arguments, std::move(result));
  } else if (method == "clearFocus") {
    OnClearFocus(arguments, std::move(result));
  } else if (method == "dispose") {
    OnDispose(arguments, std::move(result));
  } else if (method == "resize") {
    OnResize(arguments, std::move(result));
  } else if (method == "touch") {
    OnTouch(arguments, std::move(result));
  } else {
    FT_LOG(Warn) << "Unimplemented method: " << method;
    result->NotImplemented();
  }
}

void PlatformViewChannel::OnCreate(
    const EncodableValue* arguments,
    std::unique_ptr<MethodResult<EncodableValue>>&& result) {
  auto map_ptr = std::get_if<EncodableMap>(arguments);
  if (!map_ptr) {
    result->Error("Invalid arguments");
    return;
  }

  EncodableValueHolder<std::string> view_type(map_ptr, "viewType");
  EncodableValueHolder<int> view_id(map_ptr, "id");
  EncodableValueHolder<double> width(map_ptr, "width");
  EncodableValueHolder<double> height(map_ptr, "height");

  if (!view_type || !view_id || !width || !height) {
    result->Error("Invalid arguments");
    return;
  }

  FT_LOG(Info) << "Creating a platform view: " << *view_type.value;
  RemoveViewInstanceIfNeeded(*view_id);

  EncodableValueHolder<ByteMessage> params(map_ptr, "params");
  ByteMessage byte_message;
  if (params) {
    byte_message = *params;
  }
  auto it = view_factories_.find(*view_type);
  if (it != view_factories_.end()) {
    auto focused_view = view_instances_.find(CurrentFocusedViewId());
    if (focused_view != view_instances_.end()) {
      focused_view->second->SetFocus(false);
    }
    auto view_instance =
        it->second->Create(*view_id, *width, *height, byte_message);
    if (view_instance) {
      view_instances_.insert(
          std::pair<int, PlatformView*>(*view_id, view_instance));
      result->Success(EncodableValue(view_instance->GetTextureId()));
    } else {
      result->Error("Can't create a webview instance!!");
    }
  } else {
    FT_LOG(Error) << "Can't find view type: " << *view_type;
    result->Error("Can't find view type");
  }
}

void PlatformViewChannel::OnClearFocus(
    const EncodableValue* arguments,
    std::unique_ptr<MethodResult<EncodableValue>>&& result) {
  auto view_id_ptr = std::get_if<int>(arguments);
  if (!view_id_ptr) {
    result->Error("Invalid arguments");
    return;
  }

  auto it = view_instances_.find(*view_id_ptr);
  if (it == view_instances_.end()) {
    result->Error("Can't find view id");
    return;
  }

  it->second->SetFocus(false);
  it->second->ClearFocus();
  result->Success();
}

void PlatformViewChannel::OnDispose(
    const EncodableValue* arguments,
    std::unique_ptr<MethodResult<EncodableValue>>&& result) {
  auto map_ptr = std::get_if<EncodableMap>(arguments);
  if (!map_ptr) {
    result->Error("Invalid arguments");
    return;
  }

  EncodableValueHolder<int> view_id(map_ptr, "id");

  if (!view_id) {
    result->Error("Invalid arguments");
    return;
  }

  if (view_instances_.find(*view_id) == view_instances_.end()) {
    result->Error("Can't find view id");
    return;
  }

  RemoveViewInstanceIfNeeded(*view_id);
  result->Success();
}

void PlatformViewChannel::OnResize(
    const EncodableValue* arguments,
    std::unique_ptr<MethodResult<EncodableValue>>&& result) {
  auto map_ptr = std::get_if<EncodableMap>(arguments);
  if (!map_ptr) {
    result->Error("Invalid arguments");
    return;
  }

  EncodableValueHolder<int> view_id(map_ptr, "id");
  EncodableValueHolder<double> width(map_ptr, "width");
  EncodableValueHolder<double> height(map_ptr, "height");

  if (!view_id || !width || !height) {
    result->Error("Invalid arguments");
    return;
  }

  auto it = view_instances_.find(*view_id);
  if (it == view_instances_.end()) {
    result->Error("Can't find view id");
    return;
  }

  it->second->Resize(*width, *height);
  result->Success();
}

void PlatformViewChannel::OnTouch(
    const EncodableValue* arguments,
    std::unique_ptr<MethodResult<EncodableValue>>&& result) {
  auto map_ptr = std::get_if<EncodableMap>(arguments);
  if (!map_ptr) {
    result->Error("Invalid arguments");
    return;
  }

  int type = 0, button = 0;
  double x = 0.0, y = 0.0, dx = 0.0, dy = 0.0;

  EncodableValueHolder<EncodableList> event(map_ptr, "event");
  EncodableValueHolder<int> view_id(map_ptr, "id");

  if (!view_id || !event || event->size() != 6) {
    result->Error("Invalid Arguments");
    return;
  }

  auto it = view_instances_.find(*view_id);
  if (it == view_instances_.end()) {
    result->Error("Can't find view id");
    return;
  }

  type = std::get<int>(event->at(0));
  button = std::get<int>(event->at(1));
  x = std::get<double>(event->at(2));
  y = std::get<double>(event->at(3));
  dx = std::get<double>(event->at(4));
  dy = std::get<double>(event->at(5));

  it->second->Touch(type, button, x, y, dx, dy);

  if (!it->second->IsFocused()) {
    auto focused_view = view_instances_.find(CurrentFocusedViewId());
    if (focused_view != view_instances_.end()) {
      focused_view->second->SetFocus(false);
    }

    it->second->SetFocus(true);
    if (channel_ != nullptr) {
      auto id = std::make_unique<EncodableValue>(*view_id);
      channel_->InvokeMethod("viewFocused", std::move(id));
    }
  }

  result->Success();
}
}  // namespace flutter
