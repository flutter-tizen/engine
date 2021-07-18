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

bool SettingsChannel::GetSettingValueOf24Format(bool* value) {
  // This is a temporary implementation because I didn't know how to properly implement it.
  // I believe someone will replace it with a better implementation.
  // This function operates normally only in the following locale.
  // en_US.UTF-8, ko_KR.UTF-8
  char buf[64];
  std::time_t current = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  if(!strftime(buf, sizeof(buf), "%c", std::localtime(&current))){
    return false;
  }
  std::string current_local_time(buf);
  if ((current_local_time.find("AM")!=std::string::npos)
  || (current_local_time.find("PM")!=std::string::npos)
  || (current_local_time.find("오전")!=std::string::npos)
  || (current_local_time.find("오후")!=std::string::npos)
  ){
    *value = true;
  }
  *value = false;
  return true;
}

}  // namespace flutter
