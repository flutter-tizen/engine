// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_ACCESSIBILITY_SETTINGS_H_
#define EMBEDDER_ACCESSIBILITY_SETTINGS_H_

namespace flutter {

class FlutterTizenEngine;

class AccessibilitySettings {
 public:
  explicit AccessibilitySettings(FlutterTizenEngine* engine);
  virtual ~AccessibilitySettings();

  void OnHighContrastStateChanged();

 private:
  bool GetHighContrastValue();

  FlutterTizenEngine* engine_;
};

}  // namespace flutter

#endif  // EMBEDDER_ACCESSIBILITY_SETTINGS_H_
