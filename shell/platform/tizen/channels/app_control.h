// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_APP_CONTROL_H_
#define EMBEDDER_APP_CONTROL_H_

#include <app.h>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/encodable_value.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/event_channel.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

struct AppControlResult {
  AppControlResult() : error_code(APP_CONTROL_ERROR_NONE){};
  AppControlResult(int code) : error_code(code) {}

  // Returns false on error.
  operator bool() const { return (APP_CONTROL_ERROR_NONE == error_code); }

  std::string message() { return get_error_message(error_code); }

  int error_code;
};

class AppControlChannel;

class AppControl {
 public:
  AppControl(app_control_h app_control);
  ~AppControl();

  int GetId() { return id_; }
  app_control_h Handle() { return handle_; }

  AppControlResult GetOperation(std::string& operation);
  AppControlResult SetOperation(const std::string& operation);
  AppControlResult GetUri(std::string& uri);
  AppControlResult SetUri(const std::string& uri);
  AppControlResult GetMime(std::string& mime);
  AppControlResult SetMime(const std::string& mime);
  AppControlResult GetCategory(std::string& category);
  AppControlResult SetCategory(const std::string& category);
  AppControlResult GetAppId(std::string& app_id);
  AppControlResult SetAppId(const std::string& app_id);
  AppControlResult GetCaller(std::string& caller);
  AppControlResult GetLaunchMode(std::string& launch_mode);
  AppControlResult SetLaunchMode(const std::string& launch_mode);

  EncodableValue SerializeAppControlToMap();

  AppControlResult SendLaunchRequest();
  AppControlResult SendLaunchRequestWithReply(
      std::shared_ptr<EventSink<EncodableValue>> reply_sink,
      AppControlChannel* manager);
  AppControlResult SendTerminateRequest();

  AppControlResult Reply(std::shared_ptr<AppControl> reply,
                         const std::string& result);

  AppControlResult GetExtraData(EncodableMap& value);
  AppControlResult SetExtraData(const EncodableMap& value);

  void SetManager(AppControlChannel* manager);
  AppControlChannel* GetManager();

 private:
  AppControlResult GetString(std::string& str, int func(app_control_h, char**));
  AppControlResult SetString(const std::string& str,
                             int func(app_control_h, const char*));

  AppControlResult AddExtraData(std::string key, EncodableValue value);
  AppControlResult AddExtraDataList(std::string& key, EncodableList& list);

  app_control_h handle_;
  int id_;
  static int next_id_;
  std::shared_ptr<EventSink<EncodableValue>> reply_sink_;

  AppControlChannel* manager_;
};

}  // namespace flutter

#endif  // EMBEDDER_APP_CONTROL_H_
