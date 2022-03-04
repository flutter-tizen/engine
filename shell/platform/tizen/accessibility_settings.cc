// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "accessibility_settings.h"

#ifdef TV_PROFILE
#include <system/system_settings.h>
#endif

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/logger.h"

// SYSTEM_SETTINGS_KEY_MENU_SYSTEM_ACCESSIBILITY_HIGHCONTRAST = 10059 has been
// defined in system_settings_keys.h only for TV profile.
#define SYSTEM_SETTINGS_KEY_MENU_SYSTEM_ACCESSIBILITY_HIGHCONTRAST 10059

namespace flutter {

AccessibilitySettings::AccessibilitySettings(FlutterTizenEngine* engine)
    : engine_(engine) {
#ifdef TV_PROFILE
  // Add listener for accessibility high contrast.
  system_settings_set_changed_cb(
      system_settings_key_e(
          SYSTEM_SETTINGS_KEY_MENU_SYSTEM_ACCESSIBILITY_HIGHCONTRAST),
      [](system_settings_key_e key, void* user_data) -> void {
        auto* self = reinterpret_cast<AccessibilitySettings*>(user_data);
        self->OnHighContrastStateChanged();
      },
      this);

  // Set initialized value of accessibility high contrast.
  if (engine_ != nullptr) {
    engine_->EnableAccessibilityFeature(GetHighContrastValue());
  }
#endif
}

AccessibilitySettings::~AccessibilitySettings() {
#ifdef TV_PROFILE
  system_settings_unset_changed_cb(system_settings_key_e(
      SYSTEM_SETTINGS_KEY_MENU_SYSTEM_ACCESSIBILITY_HIGHCONTRAST));
#endif
}

void AccessibilitySettings::OnHighContrastStateChanged() {
  if (engine_ != nullptr) {
    engine_->EnableAccessibilityFeature(GetHighContrastValue());
  }
}

bool AccessibilitySettings::GetHighContrastValue() {
  int enabled = 0;
#ifdef TV_PROFILE
  int ret = system_settings_get_value_int(
      system_settings_key_e(
          SYSTEM_SETTINGS_KEY_MENU_SYSTEM_ACCESSIBILITY_HIGHCONTRAST),
      &enabled);
  if (ret != SYSTEM_SETTINGS_ERROR_NONE) {
    FT_LOG(Error)
        << "Failed to get value of accessibility high contrast. ERROR CODE = "
        << ret;
  }
#endif
  return enabled;
}

}  // namespace flutter
