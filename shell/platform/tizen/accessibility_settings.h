// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ACCESSIBILITY_SETTINGS_H_
#define ACCESSIBILITY_SETTINGS_H_

#include <system/system_settings.h>

namespace flutter {

class FlutterTizenEngine;

class AccessibilitySettings {
 public:
  explicit AccessibilitySettings(FlutterTizenEngine* engine);
  virtual ~AccessibilitySettings();

  bool IsAccessibilityEnabled();

 private:
  static void OnScreenReaderStateChanged(system_settings_key_e key,
                                         void* user_data);

  FlutterTizenEngine* engine_;
  bool screen_reader_enabled_;
};

}  // namespace flutter

#endif  // ACCESSIBILITY_SETTINGS_H_
