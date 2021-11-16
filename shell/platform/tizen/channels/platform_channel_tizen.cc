// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform_channel.h"

#include <app.h>
#include <feedback.h>
#ifdef COMMON_PROFILE
#include <tzsh.h>
#include <tzsh_softkey.h>
#endif

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
    int ret = feedback_initialize();
    if (ret != FEEDBACK_ERROR_NONE) {
      FT_LOG(Error) << "feedback_initialize() failed with error: "
                    << get_error_message(ret);
      return;
    }
    initialized_ = true;
  }

  ~FeedbackManager() {
    if (initialized_) {
      feedback_deinitialize();
    }
  }

  void Play(feedback_type_e type, feedback_pattern_e pattern) {
    if (!initialized_) {
      return;
    }
    int ret = feedback_play_type(type, pattern);
    if (ret == FEEDBACK_ERROR_PERMISSION_DENIED) {
      FT_LOG(Error)
          << "Permission denied. Add \"http://tizen.org/privilege/haptic\" "
             "privilege to tizen-manifest.xml to use haptic feedbacks.";
    } else if (ret != FEEDBACK_ERROR_NONE) {
      FT_LOG(Error) << "feedback_play_type() failed with error: "
                    << get_error_message(ret);
    }
  }

  bool initialized_ = false;
};

#ifdef COMMON_PROFILE
class TizenWindowSystemShell {
 public:
  static TizenWindowSystemShell& GetInstance() {
    static TizenWindowSystemShell instance;
    return instance;
  }

  TizenWindowSystemShell(const TizenWindowSystemShell&) = delete;
  TizenWindowSystemShell& operator=(const TizenWindowSystemShell&) = delete;

  void InitializeSoftkey(uint32_t window_id) {
    if (tizen_shell_softkey_ || !tizen_shell_) {
      return;
    }
    tizen_shell_softkey_ = tzsh_softkey_create(tizen_shell_, window_id);
    if (!tizen_shell_softkey_) {
      int ret = get_last_result();
      if (ret == TZSH_ERROR_PERMISSION_DENIED) {
        FT_LOG(Error) << "Permission denied. You need a "
                         "\"http://tizen.org/privilege/windowsystem.admin\" "
                         "privilege to use this method.";
      } else {
        FT_LOG(Error) << "tzsh_softkey_create() failed with error: "
                      << get_error_message(ret);
      }
    }
  }

  bool IsSoftkeyShown() { return is_softkey_shown_; }

  void ShowSoftkey() {
    if (!tizen_shell_softkey_) {
      return;
    }
    int ret = tzsh_softkey_global_show(tizen_shell_softkey_);
    if (ret != TZSH_ERROR_NONE) {
      FT_LOG(Error) << "tzsh_softkey_global_show() failed with error: "
                    << get_error_message(ret);
      return;
    }
    is_softkey_shown_ = true;
  }

  void HideSoftkey() {
    if (!tizen_shell_softkey_) {
      return;
    }
    // Always show the softkey before hiding it again, to avoid subtle bugs.
    tzsh_softkey_global_show(tizen_shell_softkey_);
    int ret = tzsh_softkey_global_hide(tizen_shell_softkey_);
    if (ret != TZSH_ERROR_NONE) {
      FT_LOG(Error) << "tzsh_softkey_global_hide() failed with error: "
                    << get_error_message(ret);
      return;
    }
    is_softkey_shown_ = false;
  }

 private:
  TizenWindowSystemShell() {
    tizen_shell_ = tzsh_create(TZSH_TOOLKIT_TYPE_EFL);
    if (!tizen_shell_) {
      FT_LOG(Error) << "tzsh_create() failed with error: "
                    << get_error_message(get_last_result());
    }
  }

  ~TizenWindowSystemShell() {
    if (tizen_shell_softkey_) {
      tzsh_softkey_destroy(tizen_shell_softkey_);
    }
    if (tizen_shell_) {
      tzsh_destroy(tizen_shell_);
    }
  }

  tzsh_h tizen_shell_ = nullptr;
  tzsh_softkey_h tizen_shell_softkey_ = nullptr;
  bool is_softkey_shown_ = true;
};
#endif

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

void PlatformChannel::RestoreSystemUIOverlays() {
#ifdef COMMON_PROFILE
  if (!renderer_) {
    return;
  }
  auto& tizen_shell = TizenWindowSystemShell::GetInstance();
  tizen_shell.InitializeSoftkey(renderer_->GetWindowId());

  if (tizen_shell.IsSoftkeyShown()) {
    tizen_shell.ShowSoftkey();
  } else {
    tizen_shell.HideSoftkey();
  }
#else
  FT_UNIMPLEMENTED();
#endif
}

void PlatformChannel::SetEnabledSystemUIOverlays(
    const std::vector<std::string>& overlays) {
#ifdef COMMON_PROFILE
  if (!renderer_) {
    return;
  }
  auto& tizen_shell = TizenWindowSystemShell::GetInstance();
  tizen_shell.InitializeSoftkey(renderer_->GetWindowId());

  if (std::find(overlays.begin(), overlays.end(), "SystemUiOverlay.bottom") !=
      overlays.end()) {
    tizen_shell.ShowSoftkey();
  } else {
    tizen_shell.HideSoftkey();
  }
#else
  FT_UNIMPLEMENTED();
#endif
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
  for (const auto& orientation : orientations) {
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
