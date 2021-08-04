// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_APP_CONTROL_CHANNEL_H_
#define EMBEDDER_APP_CONTROL_CHANNEL_H_

#include <queue>
#include <unordered_map>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/encodable_value.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/event_channel.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/method_channel.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_method_codec.h"
#include "flutter/shell/platform/tizen/logger.h"

#include "app_control.h"

namespace flutter {

class AppControlChannel {
 public:
  explicit AppControlChannel(BinaryMessenger* messenger);
  virtual ~AppControlChannel();

  void NotifyAppControl(void* app_control);

  void AddExistingAppControl(std::shared_ptr<AppControl> app_control) {
    map_.insert({app_control->GetId(), app_control});
  }

 private:
  void HandleMethodCall(const MethodCall<EncodableValue>& method_call,
                        std::unique_ptr<MethodResult<EncodableValue>> result);
  void RegisterEventHandler(
      std::unique_ptr<flutter::EventSink<EncodableValue>> events);
  void UnregisterEventHandler();
  void SendAlreadyQueuedEvents();

  void RegisterReplyHandler(
      std::unique_ptr<flutter::EventSink<EncodableValue>> events);
  void UnregisterReplyHandler();

  template <typename T>
  bool GetValueFromArgs(const EncodableValue* args, const char* key, T& out);

  std::shared_ptr<AppControl> GetAppControl(const EncodableValue* args);

  void CreateAppControl(const EncodableValue* args,
                        std::unique_ptr<MethodResult<EncodableValue>> result);

  void Dispose(std::shared_ptr<AppControl> app_control,
               std::unique_ptr<MethodResult<EncodableValue>> result);
  void Reply(std::shared_ptr<AppControl> app_control,
             const EncodableValue* arguments,
             std::unique_ptr<MethodResult<EncodableValue>> result);
  void SendLaunchRequest(std::shared_ptr<AppControl> app_control,
                         const EncodableValue* arguments,
                         std::unique_ptr<MethodResult<EncodableValue>> result);
  void SendTerminateRequest(
      std::shared_ptr<AppControl> app_control,
      const EncodableValue* arguments,
      std::unique_ptr<MethodResult<EncodableValue>> result);

  void SetAppControlData(std::shared_ptr<AppControl> app_control,
                         const EncodableValue* arguments,
                         std::unique_ptr<MethodResult<EncodableValue>> result);
  void SendAppControlDataEvent(std::shared_ptr<AppControl> app_control);

  std::unique_ptr<MethodChannel<EncodableValue>> method_channel_;
  std::unique_ptr<EventChannel<EncodableValue>> event_channel_;
  std::unique_ptr<EventChannel<EncodableValue>> reply_channel_;
  std::unique_ptr<EventSink<EncodableValue>> event_sink_;
  std::shared_ptr<EventSink<EncodableValue>> reply_sink_;

  // We need this queue, because there is no quarantee
  // that EventChannel on Dart side will be registered
  // before native OnAppControl event
  std::queue<std::shared_ptr<AppControl>> queue_;

  std::unordered_map<int, std::shared_ptr<AppControl>> map_;
};

}  // namespace flutter

#endif  // EMBEDDER_APP_CONTROL_CHANNEL_H_
