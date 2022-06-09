// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_KEY_EVENT_CHANNEL_H_
#define EMBEDDER_KEY_EVENT_CHANNEL_H_

#include <functional>
#include <map>
#include <memory>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/basic_message_channel.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/embedder/embedder.h"
#include "rapidjson/document.h"

namespace flutter {

class KeyEventChannel {
 public:
  using SendEventHandler =
      std::function<void(const FlutterKeyEvent& /* event */,
                         FlutterKeyEventCallback /* callback */,
                         void* /* user_data */)>;

  explicit KeyEventChannel(BinaryMessenger* messenger,
                           SendEventHandler send_event);
  virtual ~KeyEventChannel();

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

  std::unique_ptr<BasicMessageChannel<rapidjson::Document>> channel_;

  SendEventHandler send_event_;

  // Information for key events that have been sent to the framework but yet
  // to receive the response. Indexed by response IDs.
  std::map<uint64_t, std::unique_ptr<PendingResponse>> pending_responses_;

  // A self-incrementing integer, used as the ID for the next entry for
  // |pending_responses_|.
  uint64_t response_id_;

  void SendEmbedderEvent(const char* key,
                         const char* string,
                         const char* compose,
                         uint32_t modifiers,
                         uint32_t scan_code,
                         bool is_down,
                         std::function<void(bool)> callback);

  void SendChannelEvent(const char* key,
                        const char* string,
                        const char* compose,
                        uint32_t modifiers,
                        uint32_t scan_code,
                        bool is_down,
                        std::function<void(bool)> callback);
};

}  // namespace flutter

#endif  //  EMBEDDER_KEY_EVENT_CHANNEL_H_
