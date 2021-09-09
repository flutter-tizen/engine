// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app_control_channel.h"

#include "flutter/shell/platform/common/client_wrapper/include/flutter/event_stream_handler_functions.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_method_codec.h"
#include "flutter/shell/platform/tizen/channels/encodable_value_holder.h"

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
    CreateAppControl(std::move(result));
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
    SendTerminateRequest(app_control, std::move(result));
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

std::shared_ptr<AppControl> AppControlChannel::GetAppControl(
    const EncodableValue* arguments) {
  auto map_ptr = std::get_if<EncodableMap>(arguments);
  if (!map_ptr) {
    FT_LOG(Error) << "Invalid arguments.";
    return nullptr;
  }

  EncodableValueHolder<int> id(map_ptr, "id");
  if (!id) {
    FT_LOG(Error) << "Could not get proper id from arguments.";
    return nullptr;
  }

  if (map_.find(*id) == map_.end()) {
    FT_LOG(Error) << "Could not find AppControl with id " << *id;
    return nullptr;
  }
  return map_[*id];
}

void AppControlChannel::CreateAppControl(
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
  auto map_ptr = std::get_if<EncodableMap>(arguments);
  if (!map_ptr) {
    result->Error("Invalid arguments");
    return;
  }

  EncodableValueHolder<int> request_id(map_ptr, "requestId");
  if (!request_id || map_.find(*request_id) == map_.end()) {
    result->Error("Could not reply", "Invalid request app control");
    return;
  }

  auto request_app_control = map_[*request_id];
  EncodableValueHolder<std::string> result_str(map_ptr, "result");
  if (!result_str) {
    result->Error("Could not reply", "Invalid result parameter");
    return;
  }
  AppControlResult ret = request_app_control->Reply(app_control, *result_str);
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
  auto map_ptr = std::get_if<EncodableMap>(arguments);
  if (!map_ptr) {
    result->Error("Invalid arguments");
    return;
  }

  EncodableValueHolder<bool> wait_for_reply(map_ptr, "waitForReply");

  AppControlResult ret;
  if (wait_for_reply && *wait_for_reply) {
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
  auto map_ptr = std::get_if<EncodableMap>(arguments);
  if (!map_ptr) {
    result->Error("Invalid arguments");
    return;
  }

  EncodableValueHolder<std::string> app_id(map_ptr, "appId");
  EncodableValueHolder<std::string> operation(map_ptr, "operation");
  EncodableValueHolder<std::string> mime(map_ptr, "mime");
  EncodableValueHolder<std::string> category(map_ptr, "category");
  EncodableValueHolder<std::string> launch_mode(map_ptr, "launchMode");
  EncodableValueHolder<std::string> uri(map_ptr, "uri");
  EncodableValueHolder<EncodableMap> extra_data(map_ptr, "extraData");

  std::vector<AppControlResult> results;

  if (app_id) {
    results.emplace_back(app_control->SetAppId(*app_id));
  }
  if (operation) {
    results.emplace_back(app_control->SetOperation(*operation));
  }
  if (mime) {
    results.emplace_back(app_control->SetMime(*mime));
  }
  if (category) {
    results.emplace_back(app_control->SetCategory(*category));
  }
  if (uri) {
    results.emplace_back(app_control->SetUri(*uri));
  }
  if (launch_mode) {
    results.emplace_back(app_control->SetLaunchMode(*launch_mode));
  }
  if (extra_data) {
    results.emplace_back(app_control->SetExtraData(*extra_data));
  }
  for (size_t i = 0; i < results.size(); i++) {
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
