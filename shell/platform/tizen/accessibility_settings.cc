// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "accessibility_settings.h"

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/logger.h"

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
}

AccessibilitySettings::~AccessibilitySettings() {
  system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_ACCESSIBILITY_TTS);
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

}  // namespace flutter
