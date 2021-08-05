// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app_control_channel.h"

#include "flutter/shell/platform/common/client_wrapper/include/flutter/event_stream_handler_functions.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_method_codec.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "tizen/internal/app_control_method";
constexpr char kEventChannelName[] = "tizen/internal/app_control_event";
constexpr char kReplyChannelName[] = "tizen/internal/app_control_reply";

}  // namespace

AppControlChannel::AppControlChannel(BinaryMessenger* messenger) {
  method_channel_ = std::make_unique<MethodChannel<EncodableValue>>(
      messenger, kChannelName, &StandardMethodCodec::GetInstance());

  method_channel_->SetMethodCallHandler([this](const auto& call, auto result) {
    this->HandleMethodCall(call, std::move(result));
  });

  event_channel_ = std::make_unique<EventChannel<EncodableValue>>(
      messenger, kEventChannelName, &StandardMethodCodec::GetInstance());

  auto event_channel_handler = std::make_unique<StreamHandlerFunctions<>>(
      [this](const EncodableValue* arguments,
             std::unique_ptr<EventSink<>>&& events)
          -> std::unique_ptr<StreamHandlerError<>> {
        RegisterEventHandler(std::move(events));
        return nullptr;
      },
      [this](const EncodableValue* arguments)
          -> std::unique_ptr<StreamHandlerError<>> {
        UnregisterEventHandler();
        return nullptr;
      });

  event_channel_->SetStreamHandler(std::move(event_channel_handler));

  reply_channel_ = std::make_unique<EventChannel<EncodableValue>>(
      messenger, kReplyChannelName, &StandardMethodCodec::GetInstance());

  auto reply_channel_handler = std::make_unique<StreamHandlerFunctions<>>(
      [this](const EncodableValue* arguments,
             std::unique_ptr<EventSink<>>&& events)
          -> std::unique_ptr<StreamHandlerError<>> {
        RegisterReplyHandler(std::move(events));
        return nullptr;
      },
      [this](const EncodableValue* arguments)
          -> std::unique_ptr<StreamHandlerError<>> {
        UnregisterReplyHandler();
        return nullptr;
      });

  reply_channel_->SetStreamHandler(std::move(reply_channel_handler));
}

AppControlChannel::~AppControlChannel() {}

void AppControlChannel::NotifyAppControl(void* app_control) {
  app_control_h clone = nullptr;
  app_control_h handle = static_cast<app_control_h>(app_control);
  AppControlResult ret = app_control_clone(&clone, handle);
  if (!ret) {
    FT_LOG(Error) << "Could not clone app control: " << ret.message();
    return;
  }
  auto app = std::make_shared<AppControl>(clone);
  if (!event_sink_) {
    queue_.push(app);
    FT_LOG(Info) << "EventChannel not set yet.";
  } else {
    SendAppControlDataEvent(app);
  }
  map_.insert({app->GetId(), app});
}

void AppControlChannel::HandleMethodCall(
    const MethodCall<EncodableValue>& method_call,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  const auto arguments = method_call.arguments();
  const auto& method_name = method_call.method_name();

  // AppControl is not needed.
  if (method_name.compare("create") == 0) {
    CreateAppControl(arguments, std::move(result));
    return;
  }

  // AppControl is needed.
  auto app_control = GetAppControl(arguments);
  if (app_control == nullptr) {
    result->Error("Could not find app_control", "Invalid id provided");
    return;
  }

  if (method_name.compare("dispose") == 0) {
    Dispose(app_control, std::move(result));
  } else if (method_name.compare("reply") == 0) {
    Reply(app_control, arguments, std::move(result));
  } else if (method_name.compare("sendLaunchRequest") == 0) {
    SendLaunchRequest(app_control, arguments, std::move(result));
  } else if (method_name.compare("setAppControlData") == 0) {
    SetAppControlData(app_control, arguments, std::move(result));
  } else if (method_name.compare("sendTerminateRequest") == 0) {
    SendTerminateRequest(app_control, arguments, std::move(result));
  } else {
    result->NotImplemented();
  }
}

void AppControlChannel::RegisterEventHandler(
    std::unique_ptr<EventSink<EncodableValue>> events) {
  event_sink_ = std::move(events);
  SendAlreadyQueuedEvents();
}

void AppControlChannel::UnregisterEventHandler() {
  event_sink_.reset();
}

void AppControlChannel::SendAlreadyQueuedEvents() {
  while (!queue_.empty()) {
    SendAppControlDataEvent(queue_.front());
    queue_.pop();
  }
}

void AppControlChannel::RegisterReplyHandler(
    std::unique_ptr<EventSink<EncodableValue>> events) {
  reply_sink_ = std::move(events);
}

void AppControlChannel::UnregisterReplyHandler() {
  reply_sink_.reset();
}

template <typename T>
bool AppControlChannel::GetValueFromArgs(const EncodableValue* args,
                                         const char* key,
                                         T& out) {
  if (std::holds_alternative<EncodableMap>(*args)) {
    EncodableMap map = std::get<EncodableMap>(*args);
    if (map.find(EncodableValue(key)) != map.end()) {
      EncodableValue value = map[EncodableValue(key)];
      if (std::holds_alternative<T>(value)) {
        out = std::get<T>(value);
        return true;
      }
    }
    FT_LOG(Info) << "Key " << key << " not found.";
  }
  return false;
}

std::shared_ptr<AppControl> AppControlChannel::GetAppControl(
    const EncodableValue* args) {
  int id;
  if (!GetValueFromArgs<int>(args, "id", id)) {
    FT_LOG(Error) << "Could not get proper id from arguments.";
    return nullptr;
  }

  if (map_.find(id) == map_.end()) {
    FT_LOG(Error) << "Could not find AppControl with id " << id;
    return nullptr;
  }
  return map_[id];
}

void AppControlChannel::CreateAppControl(
    const EncodableValue* args,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  app_control_h app_control = nullptr;
  AppControlResult ret = app_control_create(&app_control);
  if (!ret) {
    result->Error("Could not create AppControl", ret.message());
  }
  auto app = std::make_unique<AppControl>(app_control);
  int id = app->GetId();
  map_.insert({id, std::move(app)});
  result->Success(EncodableValue(id));
}

void AppControlChannel::Dispose(
    std::shared_ptr<AppControl> app_control,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  map_.erase(app_control->GetId());
  result->Success();
}

void AppControlChannel::Reply(
    std::shared_ptr<AppControl> app_control,
    const EncodableValue* arguments,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  int request_id;
  if (!GetValueFromArgs<int>(arguments, "requestId", request_id) ||
      map_.find(request_id) == map_.end()) {
    result->Error("Could not reply", "Invalid request app control");
    return;
  }

  auto request_app_control = map_[request_id];
  std::string result_str;
  if (!GetValueFromArgs<std::string>(arguments, "result", result_str)) {
    result->Error("Could not reply", "Invalid result parameter");
    return;
  }
  AppControlResult ret = app_control->Reply(request_app_control, result_str);
  if (ret) {
    result->Success();
  } else {
    result->Error("Could not reply to app control", ret.message());
  }
}

void AppControlChannel::SendLaunchRequest(
    std::shared_ptr<AppControl> app_control,
    const EncodableValue* arguments,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  bool wait_for_reply = false;
  GetValueFromArgs<bool>(arguments, "waitForReply", wait_for_reply);
  AppControlResult ret;
  if (wait_for_reply) {
    ret = app_control->SendLaunchRequestWithReply(std::move(reply_sink_), this);
  } else {
    ret = app_control->SendLaunchRequest();
  }

  if (ret) {
    result->Success();
  } else {
    result->Error(ret.message());
  }
}

void AppControlChannel::SendTerminateRequest(
    std::shared_ptr<AppControl> app_control,
    const EncodableValue* arguments,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  AppControlResult ret = app_control->SendTerminateRequest();
  if (ret) {
    result->Success();
  } else {
    result->Error("Could not terminate", ret.message());
  }
}

void AppControlChannel::SetAppControlData(
    std::shared_ptr<AppControl> app_control,
    const EncodableValue* arguments,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  std::string app_id, operation, mime, category, uri, launch_mode;
  EncodableMap extra_data;
  GetValueFromArgs<std::string>(arguments, "appId", app_id);
  GetValueFromArgs<std::string>(arguments, "operation", operation);
  GetValueFromArgs<std::string>(arguments, "mime", mime);
  GetValueFromArgs<std::string>(arguments, "category", category);
  GetValueFromArgs<std::string>(arguments, "launchMode", launch_mode);
  GetValueFromArgs<std::string>(arguments, "uri", uri);
  GetValueFromArgs<EncodableMap>(arguments, "extraData", extra_data);

  AppControlResult results[7];
  results[0] = app_control->SetAppId(app_id);
  if (!operation.empty()) {
    results[1] = app_control->SetOperation(operation);
  }
  if (!mime.empty()) {
    results[2] = app_control->SetMime(mime);
  }
  if (!category.empty()) {
    results[3] = app_control->SetCategory(category);
  }
  if (!uri.empty()) {
    results[4] = app_control->SetUri(uri);
  }
  if (!launch_mode.empty()) {
    results[5] = app_control->SetLaunchMode(launch_mode);
  }
  results[6] = app_control->SetExtraData(extra_data);
  for (int i = 0; i < 7; i++) {
    if (!results[i]) {
      result->Error("Could not set value for app control",
                    results[i].message());
    }
  }
  result->Success();
}

void AppControlChannel::SendAppControlDataEvent(
    std::shared_ptr<AppControl> app_control) {
  EncodableValue map = app_control->SerializeAppControlToMap();
  if (!map.IsNull()) {
    event_sink_->Success(map);
  }
}

}  // namespace flutter
