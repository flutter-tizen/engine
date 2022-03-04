// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "accessibility_settings.h"

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/logger.h"

// SYSTEM_SETTINGS_KEY_MENU_SYSTEM_ACCESSIBILITY_HIGHCONTRAST = 10059 has been
// defined in system_settings_keys.h only for TV profile.
#define SYSTEM_SETTINGS_KEY_MENU_SYSTEM_ACCESSIBILITY_HIGHCONTRAST 10059

namespace flutter {

AccessibilitySettings::AccessibilitySettings(FlutterTizenEngine* engine)
    : engine_(engine), screen_reader_enabled_(false) {
  bool enabled = false;
  int result = system_settings_get_value_bool(
      SYSTEM_SETTINGS_KEY_ACCESSIBILITY_TTS, &enabled);
  if (result == SYSTEM_SETTINGS_ERROR_NONE) {
    if (enabled) {
      screen_reader_enabled_ = enabled;
      engine_->SetSemanticsEnabled(enabled);
    }
  } else {
    FT_LOG(Error) << "Failed to get value of accessibility tts.";
  }

  // add listener for accessibility tts
  system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_ACCESSIBILITY_TTS,
                                 OnScreenReaderStateChanged, this);

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
  system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_ACCESSIBILITY_TTS);
#ifdef TV_PROFILE
  system_settings_unset_changed_cb(system_settings_key_e(
      SYSTEM_SETTINGS_KEY_MENU_SYSTEM_ACCESSIBILITY_HIGHCONTRAST));
#endif
}

bool AccessibilitySettings::IsAccessibilityEnabled() {
  return screen_reader_enabled_;
}

void AccessibilitySettings::OnScreenReaderStateChanged(
    system_settings_key_e key,
    void* user_data) {
  auto* self = reinterpret_cast<AccessibilitySettings*>(user_data);
  bool enabled = false;
  int result = system_settings_get_value_bool(key, &enabled);
  if (result != SYSTEM_SETTINGS_ERROR_NONE) {
    FT_LOG(Error) << "Failed to get value of accessibility tts.";
    return;
  }

  if (enabled != self->screen_reader_enabled_) {
    self->screen_reader_enabled_ = enabled;
    self->engine_->SetSemanticsEnabled(enabled);
  }
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
