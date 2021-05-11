// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "settings_channel.h"

static constexpr char kChannelName[] = "flutter/settings";
static constexpr char kTextScaleFactorKey[] = "textScaleFactor";
static constexpr char kAlwaysUse24HourFormatKey[] = "alwaysUse24HourFormat";
static constexpr char kPlatformBrightnessKey[] = "platformBrightness";

SettingsChannel::SettingsChannel(flutter::BinaryMessenger* messenger)
    : channel_(
          std::make_unique<flutter::BasicMessageChannel<rapidjson::Document>>(
              messenger, kChannelName,
              &flutter::JsonMessageCodec::GetInstance())) {
  system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR,
                                 OnSettingsChangedCallback, this);
  SendSettingsEvent();
}

SettingsChannel::~SettingsChannel() {
  system_settings_unset_changed_cb(
      SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR);
}

void SettingsChannel::SendSettingsEvent() {
  rapidjson::Document event(rapidjson::kObjectType);
  auto& allocator = event.GetAllocator();
  bool value = false;
  int ret = system_settings_get_value_bool(
      SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, &value);
  if (ret == SYSTEM_SETTINGS_ERROR_NONE) {
    event.AddMember(kTextScaleFactorKey, 1.0, allocator);
    event.AddMember(kPlatformBrightnessKey, "light", allocator);
    event.AddMember(kAlwaysUse24HourFormatKey, value, allocator);
    channel_->Send(event);
  }
}

void SettingsChannel::OnSettingsChangedCallback(system_settings_key_e key,
                                                void* user_data) {
  auto settings_channel = reinterpret_cast<SettingsChannel*>(user_data);
  settings_channel->SendSettingsEvent();
}
