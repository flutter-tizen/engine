// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app_control.h"

#include "flutter/shell/platform/tizen/channels/app_control_channel.h"

namespace flutter {

int AppControl::next_id_ = 0;

AppControl::AppControl(app_control_h app_control) : id_(next_id_++) {
  handle_ = app_control;
}

AppControl::~AppControl() {
  app_control_destroy(handle_);
}

AppControlResult AppControl::GetString(std::string& str,
                                       int func(app_control_h, char**)) {
  char* op;
  AppControlResult ret = func(handle_, &op);
  if (!ret) {
    return ret;
  }
  if (op != nullptr) {
    str = std::string{op};
    free(op);
  } else {
    str = "";
  }
  return AppControlResult(APP_CONTROL_ERROR_NONE);
}

AppControlResult AppControl::SetString(const std::string& str,
                                       int func(app_control_h, const char*)) {
  int ret = func(handle_, str.c_str());
  return AppControlResult(ret);
}

bool OnAppControlExtraDataCallback(app_control_h app,
                                   const char* key,
                                   void* user_data) {
  auto extra_data = static_cast<EncodableMap*>(user_data);
  bool is_array = false;
  int ret = app_control_is_extra_data_array(app, key, &is_array);
  if (ret != APP_CONTROL_ERROR_NONE) {
    FT_LOG(Error) << "app_control_is_extra_data_array() failed at key " << key;
    return false;
  }

  if (is_array) {
    char** strings = nullptr;
    int length = 0;
    ret = app_control_get_extra_data_array(app, key, &strings, &length);
    if (ret != APP_CONTROL_ERROR_NONE) {
      FT_LOG(Error) << "app_control_get_extra_data_array() failed at key "
                    << key;
      return false;
    }
    EncodableList list;
    for (int i = 0; i < length; i++) {
      list.push_back(EncodableValue(std::string(strings[i])));
      free(strings[i]);
    }
    free(strings);
    extra_data->insert(
        {EncodableValue(std::string(key)), EncodableValue(list)});
  } else {
    char* value = nullptr;
    ret = app_control_get_extra_data(app, key, &value);
    if (ret != APP_CONTROL_ERROR_NONE) {
      FT_LOG(Error) << "app_control_get_extra_data() failed at key " << key;
      return false;
    }
    extra_data->insert(
        {EncodableValue(std::string(key)), EncodableValue(std::string(value))});
    free(value);
  }

  return true;
}

AppControlResult AppControl::GetExtraData(EncodableMap& value) {
  EncodableMap extra_data;
  int ret = app_control_foreach_extra_data(
      handle_, OnAppControlExtraDataCallback, &extra_data);
  if (ret == APP_CONTROL_ERROR_NONE) {
    value = std::move(extra_data);
  }
  return AppControlResult(ret);
}

AppControlResult AppControl::SetExtraData(const EncodableMap& map) {
  for (const auto& v : map) {
    if (!std::holds_alternative<std::string>(v.first)) {
      FT_LOG(Error) << "Key for extra data has to be string, omitting.";
      continue;
    }
    std::string key = std::get<std::string>(v.first);
    AppControlResult ret = AddExtraData(key, v.second);
    if (!ret) {
      FT_LOG(Error) << "Invalid data at " << key << ", omitting.";
      continue;
    }
  }
  return AppControlResult();
}

void AppControl::SetManager(AppControlChannel* manager) {
  manager_ = manager;
}

AppControlChannel* AppControl::GetManager() {
  return manager_;
}

AppControlResult AppControl::GetOperation(std::string& operation) {
  return GetString(operation, app_control_get_operation);
}

AppControlResult AppControl::SetOperation(const std::string& operation) {
  return SetString(operation, app_control_set_operation);
}

AppControlResult AppControl::GetUri(std::string& uri) {
  return GetString(uri, app_control_get_uri);
}

AppControlResult AppControl::SetUri(const std::string& uri) {
  return SetString(uri, app_control_set_uri);
}

AppControlResult AppControl::GetMime(std::string& mime) {
  return GetString(mime, app_control_get_mime);
}

AppControlResult AppControl::SetMime(const std::string& mime) {
  return SetString(mime, app_control_set_mime);
}

AppControlResult AppControl::GetCategory(std::string& category) {
  return GetString(category, app_control_get_category);
}

AppControlResult AppControl::SetCategory(const std::string& category) {
  return SetString(category, app_control_set_category);
}

AppControlResult AppControl::GetAppId(std::string& app_id) {
  return GetString(app_id, app_control_get_app_id);
}

AppControlResult AppControl::SetAppId(const std::string& app_id) {
  return SetString(app_id, app_control_set_app_id);
}

AppControlResult AppControl::GetCaller(std::string& caller) {
  return GetString(caller, app_control_get_caller);
}

AppControlResult AppControl::GetLaunchMode(std::string& launch_mode) {
  app_control_launch_mode_e launch_mode_e;
  int ret = app_control_get_launch_mode(handle_, &launch_mode_e);
  if (ret != APP_CONTROL_ERROR_NONE) {
    return AppControlResult(ret);
  }
  launch_mode =
      (launch_mode_e == APP_CONTROL_LAUNCH_MODE_SINGLE ? "single" : "group");
  return AppControlResult(APP_CONTROL_ERROR_NONE);
}

AppControlResult AppControl::SetLaunchMode(const std::string& launch_mode) {
  app_control_launch_mode_e launch_mode_e;
  if (launch_mode.compare("single")) {
    launch_mode_e = APP_CONTROL_LAUNCH_MODE_SINGLE;
  } else {
    launch_mode_e = APP_CONTROL_LAUNCH_MODE_GROUP;
  }
  int ret = app_control_set_launch_mode(handle_, launch_mode_e);
  return AppControlResult(ret);
}

EncodableValue AppControl::SerializeAppControlToMap() {
  std::string app_id, operation, mime, category, uri, caller_id, launch_mode;
  AppControlResult results[7];
  results[0] = GetAppId(app_id);
  results[1] = GetOperation(operation);
  results[2] = GetMime(mime);
  results[3] = GetCategory(category);
  results[4] = GetUri(uri);
  results[5] = GetLaunchMode(launch_mode);
  // Caller Id is optional.
  GetCaller(caller_id);
  EncodableMap extra_data;
  results[6] = GetExtraData(extra_data);
  for (int i = 0; i < 7; i++) {
    if (!results[i]) {
      return EncodableValue();
    }
  }
  EncodableMap map;
  map[EncodableValue("id")] = EncodableValue(GetId());
  map[EncodableValue("appId")] = EncodableValue(app_id);
  map[EncodableValue("operation")] = EncodableValue(operation);
  map[EncodableValue("mime")] = EncodableValue(mime);
  map[EncodableValue("category")] = EncodableValue(category);
  map[EncodableValue("uri")] = EncodableValue(uri);
  map[EncodableValue("callerId")] = EncodableValue(caller_id);
  map[EncodableValue("launchMode")] = EncodableValue(launch_mode);
  map[EncodableValue("extraData")] = EncodableValue(extra_data);

  return EncodableValue(map);
}

AppControlResult AppControl::SendLaunchRequest() {
  AppControlResult ret =
      app_control_send_launch_request(handle_, nullptr, nullptr);
  return ret;
}

AppControlResult AppControl::SendLaunchRequestWithReply(
    std::shared_ptr<EventSink<EncodableValue>> reply_sink,
    AppControlChannel* manager) {
  SetManager(manager);
  auto on_reply = [](app_control_h request, app_control_h reply,
                     app_control_result_e result, void* user_data) {
    AppControl* app_control = static_cast<AppControl*>(user_data);
    app_control_h clone = nullptr;
    AppControlResult ret = app_control_clone(&clone, reply);
    if (!ret) {
      FT_LOG(Error) << "Could not clone app_control: " << ret.message();
      return;
    }

    std::shared_ptr<AppControl> app_control_reply =
        std::make_shared<AppControl>(clone);
    EncodableMap map;
    map[EncodableValue("id")] = EncodableValue(app_control->GetId());
    map[EncodableValue("reply")] =
        app_control_reply->SerializeAppControlToMap();
    if (result == APP_CONTROL_RESULT_APP_STARTED) {
      map[EncodableValue("result")] = EncodableValue("appStarted");
    } else if (result == APP_CONTROL_RESULT_SUCCEEDED) {
      map[EncodableValue("result")] = EncodableValue("succeeded");
    } else if (result == APP_CONTROL_RESULT_FAILED) {
      map[EncodableValue("result")] = EncodableValue("failed");
    } else if (result == APP_CONTROL_RESULT_CANCELED) {
      map[EncodableValue("result")] = EncodableValue("canceled");
    }

    app_control->reply_sink_->Success(EncodableValue(map));
    app_control->GetManager()->AddExistingAppControl(
        std::move(app_control_reply));
  };
  reply_sink_ = std::move(reply_sink);
  AppControlResult ret =
      app_control_send_launch_request(handle_, on_reply, this);
  return ret;
}

AppControlResult AppControl::SendTerminateRequest() {
  AppControlResult ret = app_control_send_terminate_request(handle_);
  return ret;
}

AppControlResult AppControl::Reply(std::shared_ptr<AppControl> reply,
                                   const std::string& result) {
  app_control_result_e result_e;
  if (result == "appStarted") {
    result_e = APP_CONTROL_RESULT_APP_STARTED;
  } else if (result == "succeeded") {
    result_e = APP_CONTROL_RESULT_SUCCEEDED;
  } else if (result == "failed") {
    result_e = APP_CONTROL_RESULT_FAILED;
  } else if (result == "canceled") {
    result_e = APP_CONTROL_RESULT_CANCELED;
  } else {
    return AppControlResult(APP_CONTROL_ERROR_INVALID_PARAMETER);
  }
  AppControlResult ret = app_control_reply_to_launch_request(
      reply->Handle(), this->handle_, result_e);
  return ret;
}

AppControlResult AppControl::AddExtraData(std::string key,
                                          EncodableValue value) {
  bool is_array = std::holds_alternative<EncodableList>(value);
  if (is_array) {
    EncodableList& list = std::get<EncodableList>(value);
    return AddExtraDataList(key, list);
  } else {
    bool is_string = std::holds_alternative<std::string>(value);
    if (is_string) {
      int ret = app_control_add_extra_data(
          handle_, key.c_str(), std::get<std::string>(value).c_str());
      return AppControlResult(ret);
    } else {
      return AppControlResult(APP_ERROR_INVALID_PARAMETER);
    }
  }
  return AppControlResult(APP_CONTROL_ERROR_NONE);
}

AppControlResult AppControl::AddExtraDataList(std::string& key,
                                              EncodableList& list) {
  size_t length = list.size();
  auto strings = std::vector<const char*>(length);
  for (size_t i = 0; i < length; i++) {
    bool is_string = std::holds_alternative<std::string>(list[i]);
    if (is_string) {
      strings[i] = std::get<std::string>(list[i]).c_str();
    } else {
      return AppControlResult(APP_ERROR_INVALID_PARAMETER);
    }
  }
  int ret = app_control_add_extra_data_array(handle_, key.c_str(),
                                             strings.data(), length);
  return AppControlResult(ret);
}

}  // namespace flutter
