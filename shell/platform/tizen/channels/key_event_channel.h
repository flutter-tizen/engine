// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_KEY_EVENT_CHANNEL_H_
#define EMBEDDER_KEY_EVENT_CHANNEL_H_

#include <Ecore_Input.h>

#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/basic_message_channel.h"
#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/common/cpp/json_message_codec.h"
#include "rapidjson/document.h"

class KeyEventChannel {
 public:
  explicit KeyEventChannel(flutter::BinaryMessenger* messenger);
  virtual ~KeyEventChannel();

  void SendKeyEvent(Ecore_Event_Key* key, bool is_down);

 private:
  std::unique_ptr<flutter::BasicMessageChannel<rapidjson::Document>> channel_;
};

#endif  //  EMBEDDER_KEY_EVENT_CHANNEL_H_
