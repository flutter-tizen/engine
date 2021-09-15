// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app_control.h"

#include "flutter/shell/platform/tizen/channels/app_control_channel.h"
#include "flutter/shell/platform/tizen/logger.h"

intptr_t NativeInitializeDartApi(void* data) {
  return Dart_InitializeApiDL(data);
}

int32_t NativeCreateAppControl(Dart_Handle handle) {
  auto app_control = std::make_unique<flutter::AppControl>();
  if (!app_control->handle()) {
    return -1;
  }
  auto id = app_control->id();
  Dart_NewFinalizableHandle_DL(
      handle, app_control.get(), 64,
      [](void* isolate_callback_data, void* peer) {
        auto app_control = reinterpret_cast<flutter::AppControl*>(peer);
        flutter::AppControlManager::GetInstance().Remove(app_control->id());
      });
  flutter::AppControlManager::GetInstance().Insert(std::move(app_control));
  return id;
}

bool NativeAttachAppControl(int32_t id, Dart_Handle handle) {
  auto app_control = flutter::AppControlManager::GetInstance().FindById(id);
  if (!app_control || !app_control->handle()) {
    return false;
  }
  Dart_NewFinalizableHandle_DL(
      handle, app_control, 64, [](void* isolate_callback_data, void* peer) {
        auto app_control = reinterpret_cast<flutter::AppControl*>(peer);
        flutter::AppControlManager::GetInstance().Remove(app_control->id());
      });
  return true;
}

namespace flutter {

int32_t AppControl::next_id_ = 0;

AppControl::AppControl() : id_(next_id_++) {
  app_control_h handle = nullptr;
  AppControlResult ret = app_control_create(&handle);
  if (!ret) {
    FT_LOG(Error) << "app_control_create() failed: " << ret.message();
    return;
  }
  handle_ = handle;
}

AppControl::AppControl(app_control_h handle) : id_(next_id_++) {
  app_control_h clone = nullptr;
  AppControlResult ret = app_control_clone(&clone, handle);
  if (!ret) {
    FT_LOG(Error) << "app_control_clone() failed: " << ret.message();
    return;
  }
  handle_ = clone;
}

AppControl::~AppControl() {
  if (handle_) {
    app_control_destroy(handle_);
  }
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

bool AppControl::IsReplyRequested() {
  bool requested = false;
  app_control_is_reply_requested(handle_, &requested);
  return requested;
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
  map[EncodableValue("id")] = EncodableValue(id());
  map[EncodableValue("appId")] = EncodableValue(app_id);
  map[EncodableValue("operation")] = EncodableValue(operation);
  map[EncodableValue("mime")] = EncodableValue(mime);
  map[EncodableValue("category")] = EncodableValue(category);
  map[EncodableValue("uri")] = EncodableValue(uri);
  map[EncodableValue("callerAppId")] = EncodableValue(caller_id);
  map[EncodableValue("launchMode")] = EncodableValue(launch_mode);
  map[EncodableValue("extraData")] = EncodableValue(extra_data);
  map[EncodableValue("shouldReply")] = EncodableValue(IsReplyRequested());

  return EncodableValue(map);
}

AppControlResult AppControl::SendLaunchRequest() {
  return app_control_send_launch_request(handle_, nullptr, nullptr);
}

AppControlResult AppControl::SendLaunchRequestWithReply(
    ReplyCallback on_reply) {
  auto reply_callback = [](app_control_h request, app_control_h reply,
                           app_control_result_e result, void* user_data) {
    AppControl* app_control = static_cast<AppControl*>(user_data);
    auto app_control_reply = std::make_unique<AppControl>(reply);
    EncodableMap map;
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
    app_control->on_reply_(EncodableValue(map));
    app_control->on_reply_ = nullptr;
    AppControlManager::GetInstance().Insert(std::move(app_control_reply));
  };
  on_reply_ = on_reply;
  return app_control_send_launch_request(handle_, reply_callback, this);
}

AppControlResult AppControl::SendTerminateRequest() {
  AppControlResult ret = app_control_send_terminate_request(handle_);
  return ret;
}

AppControlResult AppControl::Reply(AppControl* reply,
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
      reply->handle(), this->handle_, result_e);
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
