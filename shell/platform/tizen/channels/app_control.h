// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_APP_CONTROL_H_
#define EMBEDDER_APP_CONTROL_H_

#include <app.h>

#include <unordered_map>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/encodable_value.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/event_channel.h"
#include "flutter/shell/platform/common/public/flutter_export.h"
#include "third_party/dart/runtime/include/dart_api_dl.h"

// Called by Dart code through FFI to initialize dart_api_dl.h.
DART_EXPORT FLUTTER_EXPORT intptr_t NativeInitializeDartApi(void* data);

// Creates an internally managed instance of AppControl and associates with
// |handle|.
//
// A finalizer is attached to the created instance and invoked when the
// associated |handle| is disposed by GC.
//
// Returns a unique AppControl ID on success, otherwise -1.
DART_EXPORT FLUTTER_EXPORT int32_t NativeCreateAppControl(Dart_Handle handle);

// Finds an instance of AppControl with |id| and associates with |handle|.
//
// A finalizer is attached to the instance and invoked when the associated
// |handle| is disposed by GC.
//
// Returns false if an instance of AppControl with the given |id| could not
// be found, otherwise true.
DART_EXPORT FLUTTER_EXPORT bool NativeAttachAppControl(int32_t id,
                                                       Dart_Handle handle);

namespace flutter {

struct AppControlResult {
  AppControlResult() : error_code(APP_CONTROL_ERROR_NONE){};
  AppControlResult(int code) : error_code(code) {}

  // Returns false on error.
  operator bool() const { return (APP_CONTROL_ERROR_NONE == error_code); }

  std::string message() { return get_error_message(error_code); }

  int error_code;
};

class AppControl {
 public:
  using ReplyCallback = std::function<void(const EncodableValue& response)>;

  explicit AppControl();
  explicit AppControl(app_control_h handle);
  virtual ~AppControl();

  int32_t id() { return id_; }
  app_control_h handle() { return handle_; }

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
  bool IsReplyRequested();

  EncodableValue SerializeAppControlToMap();

  AppControlResult SendLaunchRequest();
  AppControlResult SendLaunchRequestWithReply(ReplyCallback on_reply);
  AppControlResult SendTerminateRequest();

  AppControlResult Reply(AppControl* reply, const std::string& result);

  AppControlResult GetExtraData(EncodableMap& value);
  AppControlResult SetExtraData(const EncodableMap& value);

 private:
  AppControlResult GetString(std::string& str, int func(app_control_h, char**));
  AppControlResult SetString(const std::string& str,
                             int func(app_control_h, const char*));

  AppControlResult AddExtraData(std::string key, EncodableValue value);
  AppControlResult AddExtraDataList(std::string& key, EncodableList& list);

  app_control_h handle_ = nullptr;
  int32_t id_;
  static int32_t next_id_;
  ReplyCallback on_reply_ = nullptr;
};

class AppControlManager {
 public:
  // Returns an instance of this class.
  static AppControlManager& GetInstance() {
    static AppControlManager instance;
    return instance;
  }

  void Insert(std::unique_ptr<AppControl> app_control) {
    map_.insert({app_control->id(), std::move(app_control)});
  }

  void Remove(int32_t id) { map_.erase(id); }

  AppControl* FindById(const int32_t id) {
    if (map_.find(id) == map_.end()) {
      return nullptr;
    }
    return map_[id].get();
  }

 private:
  explicit AppControlManager() {}
  ~AppControlManager() {}

  std::unordered_map<int32_t, std::unique_ptr<AppControl>> map_;
};

}  // namespace flutter

#endif  // EMBEDDER_APP_CONTROL_H_
