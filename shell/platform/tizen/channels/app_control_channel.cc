// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app_control_channel.h"

#include "flutter/shell/platform/common/client_wrapper/include/flutter/event_stream_handler_functions.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_method_codec.h"
#include "flutter/shell/platform/tizen/channels/encodable_value_holder.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "tizen/internal/app_control_method";
constexpr char kEventChannelName[] = "tizen/internal/app_control_event";

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
}

AppControlChannel::~AppControlChannel() {}

void AppControlChannel::NotifyAppControl(void* handle) {
  auto app_control =
      std::make_unique<AppControl>(static_cast<app_control_h>(handle));
  if (!app_control->handle()) {
    FT_LOG(Error) << "Could not create an instance of AppControl.";
    return;
  }
  if (!event_sink_) {
    FT_LOG(Info) << "EventChannel not set yet.";
    queue_.push(app_control.get());
  } else {
    SendAppControlDataEvent(app_control.get());
  }
  AppControlManager::GetInstance().Insert(std::move(app_control));
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

AppControl* AppControlChannel::GetAppControl(const EncodableValue* arguments) {
  auto map_ptr = std::get_if<EncodableMap>(arguments);
  if (!map_ptr) {
    FT_LOG(Error) << "Invalid arguments.";
    return nullptr;
  }

  EncodableValueHolder<int32_t> id(map_ptr, "id");
  if (!id) {
    FT_LOG(Error) << "Could not get proper id from arguments.";
    return nullptr;
  }

  auto app_control = AppControlManager::GetInstance().FindById(*id);
  if (!app_control) {
    FT_LOG(Error) << "Could not find AppControl with id " << *id;
    return nullptr;
  }
  return app_control;
}

void AppControlChannel::CreateAppControl(
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  auto app_control = std::make_unique<AppControl>();
  if (app_control->handle()) {
    result->Success(EncodableValue(app_control->id()));
    AppControlManager::GetInstance().Insert(std::move(app_control));
  } else {
    result->Error("Internal error", "Could not create AppControl.");
  }
}

void AppControlChannel::Dispose(
    AppControl* app_control,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  AppControlManager::GetInstance().Remove(app_control->id());
  result->Success();
}

void AppControlChannel::Reply(
    AppControl* app_control,
    const EncodableValue* arguments,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  auto map_ptr = std::get_if<EncodableMap>(arguments);
  if (!map_ptr) {
    result->Error("Invalid arguments");
    return;
  }

  EncodableValueHolder<int32_t> request_id(map_ptr, "requestId");
  if (!request_id) {
    result->Error("Invalid arguments", "Invalid requestId parameter");
    return;
  }
  auto request_app_control =
      AppControlManager::GetInstance().FindById(*request_id);
  if (!request_app_control) {
    result->Error("Invalid arguments",
                  "Could not find AppControl with the given ID.");
    return;
  }

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
    AppControl* app_control,
    const EncodableValue* arguments,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  auto map_ptr = std::get_if<EncodableMap>(arguments);
  if (!map_ptr) {
    result->Error("Invalid arguments");
    return;
  }

  EncodableValueHolder<bool> wait_for_reply(map_ptr, "waitForReply");
  if (wait_for_reply && *wait_for_reply) {
    auto result_ptr = result.release();
    auto on_reply = [result_ptr](const EncodableValue& response) {
      result_ptr->Success(response);
      delete result_ptr;
    };
    AppControlResult ret = app_control->SendLaunchRequestWithReply(on_reply);
    if (!ret) {
      result_ptr->Error(ret.message());
      delete result_ptr;
    }
  } else {
    AppControlResult ret = app_control->SendLaunchRequest();
    if (ret) {
      result->Success();
    } else {
      result->Error(ret.message());
    }
  }
}

void AppControlChannel::SendTerminateRequest(
    AppControl* app_control,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  AppControlResult ret = app_control->SendTerminateRequest();
  if (ret) {
    result->Success();
  } else {
    result->Error("Could not terminate", ret.message());
  }
}

void AppControlChannel::SetAppControlData(
    AppControl* app_control,
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

void AppControlChannel::SendAppControlDataEvent(AppControl* app_control) {
  EncodableValue map = app_control->SerializeAppControlToMap();
  if (!map.IsNull()) {
    event_sink_->Success(map);
  }
}

}  // namespace flutter
