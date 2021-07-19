// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "settings_channel.h"

#include <chrono>
#include <ctime>

namespace flutter {

void SettingsChannel::Init() {
  std::locale::global(std::locale(""));
}

void SettingsChannel::Dispose() {}

bool SettingsChannel::Prefer24HourTime() {
  return false;
}

}  // namespace flutter
