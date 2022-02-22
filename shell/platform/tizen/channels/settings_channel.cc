// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "settings_channel.h"

#include <system/system_settings.h>

#include "flutter/shell/platform/common/json_message_codec.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "flutter/settings";

constexpr char kTextScaleFactorKey[] = "textScaleFactor";
constexpr char kAlwaysUse24HourFormatKey[] = "alwaysUse24HourFormat";
constexpr char kPlatformBrightnessKey[] = "platformBrightness";

}  // namespace

SettingsChannel::SettingsChannel(BinaryMessenger* messenger)
    : channel_(std::make_unique<BasicMessageChannel<rapidjson::Document>>(
          messenger,
          kChannelName,
          &JsonMessageCodec::GetInstance())) {
  system_settings_set_changed_cb(
      SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR,
      [](system_settings_key_e key, void* user_data) -> void {
        auto* self = reinterpret_cast<SettingsChannel*>(user_data);
        self->SendSettingsEvent();
      },
      this);
  SendSettingsEvent();
}

SettingsChannel::~SettingsChannel() {
  system_settings_unset_changed_cb(
      SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR);
}

void SettingsChannel::SendSettingsEvent() {
  rapidjson::Document event(rapidjson::kObjectType);
  auto& allocator = event.GetAllocator();
  event.AddMember(kTextScaleFactorKey, 1.0, allocator);
  event.AddMember(kAlwaysUse24HourFormatKey, Prefer24HourTime(), allocator);
  event.AddMember(kPlatformBrightnessKey, "light", allocator);
  channel_->Send(event);
}

bool SettingsChannel::Prefer24HourTime() {
  bool value = false;
  if (system_settings_get_value_bool(
          SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, &value) ==
      SYSTEM_SETTINGS_ERROR_NONE) {
    return value;
  }
  return false;
}

}  // namespace flutter
