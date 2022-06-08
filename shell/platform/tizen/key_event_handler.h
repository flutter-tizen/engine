// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_KEY_EVENT_HANDLER_H_
#define EMBEDDER_KEY_EVENT_HANDLER_H_

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "flutter/shell/platform/embedder/embedder.h"

namespace flutter {

class KeyEventHandler {
 public:
  using SendEventHandler =
      std::function<void(const FlutterKeyEvent& /* event */,
                         FlutterKeyEventCallback /* callback */,
                         void* /* user_data */)>;

  KeyEventHandler(SendEventHandler send_event);

  ~KeyEventHandler() {}

  void SendKey(const char* key,
               const char* string,
               const char* compose,
               uint32_t modifiers,
               uint32_t scan_code,
               bool is_down,
               std::function<void(bool)> callback);

 private:
  struct PendingResponse {
    std::function<void(bool, uint64_t)> callback;
    uint64_t response_id;
  };

  SendEventHandler send_event_;

  // Information for key events that have been sent to the framework but yet
  // to receive the response. Indexed by response IDs.
  std::map<uint64_t, std::unique_ptr<PendingResponse>> pending_responses_;

  // A self-incrementing integer, used as the ID for the next entry for
  // |pending_responses_|.
  uint64_t response_id_;
};

}  // namespace flutter

#endif  // EMBEDDER_KEY_EVENT_HANDLER_H_
