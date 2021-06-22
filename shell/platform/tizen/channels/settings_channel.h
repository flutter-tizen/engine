// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_SETTINGS_CHANNEL_H_
#define EMBEDDER_SETTINGS_CHANNEL_H_

#ifndef __X64_SHELL__
#include <system/system_settings.h>
#else
typedef enum {
  SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, /**< (bool) Indicates whether
                                                   the 24-hour clock is used. If
                                                   the value is @c false, the
                                                   12-hour clock is used. */
  SYSTEM_SETTINGS_KEY_MAX,
} system_settings_key_e;

typedef enum {
  SYSTEM_SETTINGS_ERROR_NONE, /**< Successful */
} system_settings_error_e;

typedef void (*system_settings_changed_cb)(system_settings_key_e key,
                                           void* user_data);
#endif

#include <memory>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/basic_message_channel.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"
#include "rapidjson/document.h"

namespace flutter {

class SettingsChannel {
 public:
  explicit SettingsChannel(BinaryMessenger* messenger);
  virtual ~SettingsChannel();

 private:
  static void OnSettingsChangedCallback(system_settings_key_e key,
                                        void* user_data);
  void SendSettingsEvent();

  std::unique_ptr<BasicMessageChannel<rapidjson::Document>> channel_;
};

}  // namespace flutter

#endif  // EMBEDDER_SETTINGS_CHANNEL_H_
