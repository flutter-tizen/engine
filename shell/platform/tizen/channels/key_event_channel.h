// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_KEY_EVENT_CHANNEL_H_
#define EMBEDDER_KEY_EVENT_CHANNEL_H_

#include <Ecore_Input.h>

#include <memory>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/basic_message_channel.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"
#include "rapidjson/document.h"

namespace flutter {

class KeyEventChannel {
 public:
  explicit KeyEventChannel(BinaryMessenger* messenger);
  virtual ~KeyEventChannel();

  void SendKeyEvent(Ecore_Event_Key* key,
                    bool is_down,
                    std::function<void(bool)> callback);

 private:
  std::unique_ptr<BasicMessageChannel<rapidjson::Document>> channel_;
};

}  // namespace flutter

#endif  //  EMBEDDER_KEY_EVENT_CHANNEL_H_
