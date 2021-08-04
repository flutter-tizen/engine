// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform_channel.h"

#include <app.h>
#include <feedback.h>

#include <map>

#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

namespace {

constexpr char kSoundTypeClick[] = "SystemSoundType.click";
constexpr char kPortraitUp[] = "DeviceOrientation.portraitUp";
constexpr char kPortraitDown[] = "DeviceOrientation.portraitDown";
constexpr char kLandscapeLeft[] = "DeviceOrientation.landscapeLeft";
constexpr char kLandscapeRight[] = "DeviceOrientation.landscapeRight";

class FeedbackManager {
 public:
  static FeedbackManager& GetInstance() {
    static FeedbackManager instance;
    return instance;
  }

  FeedbackManager(const FeedbackManager&) = delete;
  FeedbackManager& operator=(const FeedbackManager&) = delete;

  void PlaySound(const std::string& sound_type) {
    auto pattern = (sound_type == kSoundTypeClick) ? FEEDBACK_PATTERN_TAP
                                                   : FEEDBACK_PATTERN_GENERAL;
    Play(FEEDBACK_TYPE_SOUND, pattern);
  }

  void Vibrate(const std::string& feedback_type) {
    // We use a single type of vibration (FEEDBACK_PATTERN_SIP) to implement
    // HapticFeedback's vibrate, lightImpact, mediumImpact, heavyImpact, and
    // selectionClick methods, because Tizen's "feedback" module has no
    // dedicated vibration types for them.
    // Thus, we ignore the feedback_type argument for "HapticFeedback.vibrate"
    // calls.
    Play(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_SIP);
  }

 private:
  FeedbackManager() {
    auto ret = feedback_initialize();
    if (FEEDBACK_ERROR_NONE != ret) {
      FT_LOG(Error) << "feedback_initialize() failed with error: "
                    << get_error_message(ret);
      return;
    }
    initialized_ = true;
  }

  ~FeedbackManager() {
    auto ret = feedback_deinitialize();
    if (FEEDBACK_ERROR_NONE != ret) {
      FT_LOG(Error) << "feedback_deinitialize() failed with error: "
                    << get_error_message(ret);
    }
  }

  void Play(feedback_type_e type, feedback_pattern_e pattern) {
    if (!initialized_) {
      return;
    }
    auto ret = feedback_play_type(type, pattern);
    if (FEEDBACK_ERROR_PERMISSION_DENIED == ret) {
      FT_LOG(Error)
          << "Permission denied. Add \"http://tizen.org/privilege/haptic\" "
             "privilege to tizen-manifest.xml to use haptic feedbacks.";
    } else if (FEEDBACK_ERROR_NONE != ret) {
      FT_LOG(Error) << "feedback_play_type() failed with error: "
                    << get_error_message(ret);
    }
  }

  bool initialized_ = false;
};

}  // namespace

void PlatformChannel::SystemNavigatorPop() {
  ui_app_exit();
}

void PlatformChannel::PlaySystemSound(const std::string& sound_type) {
  FeedbackManager::GetInstance().PlaySound(sound_type);
}

void PlatformChannel::HapticFeedbackVibrate(const std::string& feedback_type) {
  FeedbackManager::GetInstance().Vibrate(feedback_type);
}

void PlatformChannel::SetPreferredOrientations(
    const std::vector<std::string>& orientations) {
  if (!renderer_) {
    return;
  }
  static const std::map<std::string, int> orientation_mapping = {
      {kPortraitUp, 0},
      {kLandscapeLeft, 90},
      {kPortraitDown, 180},
      {kLandscapeRight, 270},
  };
  std::vector<int> rotations;
  for (auto orientation : orientations) {
    rotations.push_back(orientation_mapping.at(orientation));
  }
  if (rotations.size() == 0) {
    // According do docs
    // https://api.flutter.dev/flutter/services/SystemChrome/setPreferredOrientations.html
    // "The empty list causes the application to defer to the operating
    // system default."
    rotations = {0, 90, 180, 270};
  }
  renderer_->SetPreferredOrientations(rotations);
}

}  // namespace flutter
