// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_SETTINGS_CHANNEL_H_
#define EMBEDDER_SETTINGS_CHANNEL_H_

#include <system/system_settings.h>

#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/basic_message_channel.h"
#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/common/cpp/json_message_codec.h"
#include "rapidjson/document.h"

class SettingsChannel {
 public:
  explicit SettingsChannel(flutter::BinaryMessenger* messenger);
  virtual ~SettingsChannel();

 private:
  std::unique_ptr<flutter::BasicMessageChannel<rapidjson::Document>> channel_;
  static void OnSettingsChangedCallback(system_settings_key_e key,
                                        void* user_data);
  void SendSettingsEvent();
};

#endif  // EMBEDDER_SETTINGS_CHANNEL_H_
