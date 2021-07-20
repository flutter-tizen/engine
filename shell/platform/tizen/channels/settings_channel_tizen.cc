// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "settings_channel.h"

#include <system/system_settings.h>

namespace flutter {

static void OnSettingsChangedCallback(system_settings_key_e key,
                                      void* user_data) {
  auto settings_channel = reinterpret_cast<SettingsChannel*>(user_data);
  settings_channel->SendSettingsEvent();
}

void SettingsChannel::Init() {
  system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR,
                                 OnSettingsChangedCallback, this);
}

void SettingsChannel::Dispose() {
  system_settings_unset_changed_cb(
      SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR);
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
