// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_KEY_EVENT_CHANNEL_H_
#define EMBEDDER_KEY_EVENT_CHANNEL_H_

#include <deque>
#include <functional>
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
  std::unique_ptr<BasicMessageChannel<rapidjson::Document>> channel_;
  SendEventHandler send_event_;

  struct PendingEvent {
    // Self-incrementing ID attached to an event sent to the framework.
    uint64_t sequence_id;
    // The number of delegates (either the embedder API or the platform channel)
    // that haven't replied.
    size_t unreplied;
    // Whether any replied delegates reported true (handled).
    bool any_handled;
    // Where to report the delegates' result to.
    std::function<void(bool)> callback;
  };

  // The queue of key events that have been sent to the framework but have not
  // yet received a response.
  std::deque<std::unique_ptr<PendingEvent>> pending_events_;

  // A self-incrementing integer used as the ID for the next entry for
  // |pending_events_|.
  uint64_t last_sequence_id_;

  void SendEmbedderEvent(const char* key,
                         const char* string,
                         const char* compose,
                         uint32_t modifiers,
                         uint32_t scan_code,
                         bool is_down,
                         uint64_t sequence_id);

  void SendChannelEvent(const char* key,
                        const char* string,
                        const char* compose,
                        uint32_t modifiers,
                        uint32_t scan_code,
                        bool is_down,
                        uint64_t sequence_id);

  void ResolvePendingEvent(uint64_t sequence_id, bool handled);
};

}  // namespace flutter

#endif  //  EMBEDDER_KEY_EVENT_CHANNEL_H_
