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
  system_settings_set_changed_cb(
      SYSTEM_SETTINGS_KEY_FONT_SIZE,
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
  system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_FONT_SIZE);
}

void SettingsChannel::SendSettingsEvent() {
  rapidjson::Document event(rapidjson::kObjectType);
  auto& allocator = event.GetAllocator();
  event.AddMember(kTextScaleFactorKey, GetTextScaleFactor(), allocator);
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

float SettingsChannel::GetTextScaleFactor() {
  const float small = 0.8;
  const float normal = 1.0;
  const float large = 1.5;
  const float huge = 1.9;
  const float giant = 2.5;

  int value = 1;
  if (system_settings_get_value_int(SYSTEM_SETTINGS_KEY_FONT_SIZE, &value) ==
      SYSTEM_SETTINGS_ERROR_NONE) {
    float scale = normal;
    switch (value) {
      case SYSTEM_SETTINGS_FONT_SIZE_SMALL:
        scale = small;
        break;
      case SYSTEM_SETTINGS_FONT_SIZE_LARGE:
        scale = large;
        break;
      case SYSTEM_SETTINGS_FONT_SIZE_HUGE:
        scale = huge;
        break;
      case SYSTEM_SETTINGS_FONT_SIZE_GIANT:
        scale = giant;
        break;
      default:
        break;
    }
    return scale;
  }
  return 1.0;
}

}  // namespace flutter
