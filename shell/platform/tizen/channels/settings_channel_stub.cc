// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "settings_channel.h"

namespace flutter {

SettingsChannel::SettingsChannel(BinaryMessenger* messenger) {}

SettingsChannel::~SettingsChannel() {}

void SettingsChannel::SendSettingsEvent() {}

void SettingsChannel::OnSettingsChangedCallback(system_settings_key_e key,
                                                void* user_data) {}

}  // namespace flutter
