// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform_channel.h"

#include <app.h>
#ifdef COMMON_PROFILE
#include <tzsh.h>
#include <tzsh_softkey.h>
#endif

#include <map>

#include "flutter/shell/platform/common/json_method_codec.h"
#include "flutter/shell/platform/tizen/channels/feedback_manager.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "flutter/platform";

constexpr char kGetClipboardDataMethod[] = "Clipboard.getData";
constexpr char kSetClipboardDataMethod[] = "Clipboard.setData";
constexpr char kClipboardHasStringsMethod[] = "Clipboard.hasStrings";
constexpr char kPlaySoundMethod[] = "SystemSound.play";
constexpr char kHapticFeedbackVibrateMethod[] = "HapticFeedback.vibrate";
constexpr char kSystemNavigatorPopMethod[] = "SystemNavigator.pop";
constexpr char kRestoreSystemUIOverlaysMethod[] =
    "SystemChrome.restoreSystemUIOverlays";
constexpr char kSetApplicationSwitcherDescriptionMethod[] =
    "SystemChrome.setApplicationSwitcherDescription";
constexpr char kSetEnabledSystemUIModeMethod[] =
    "SystemChrome.setEnabledSystemUIMode";
constexpr char kSetEnabledSystemUIOverlaysMethod[] =
    "SystemChrome.setEnabledSystemUIOverlays";
constexpr char kSetPreferredOrientationsMethod[] =
    "SystemChrome.setPreferredOrientations";
constexpr char kSetSystemUIOverlayStyleMethod[] =
    "SystemChrome.setSystemUIOverlayStyle";

constexpr char kTextKey[] = "text";
constexpr char kValueKey[] = "value";
constexpr char kTextPlainFormat[] = "text/plain";
constexpr char kUnknownClipboardFormatError[] =
    "Unknown clipboard format error";
constexpr char kUnknownClipboardError[] =
    "Unknown error during clipboard data retrieval";

constexpr char kSoundTypeClick[] = "SystemSoundType.click";
constexpr char kPortraitUp[] = "DeviceOrientation.portraitUp";
constexpr char kPortraitDown[] = "DeviceOrientation.portraitDown";
constexpr char kLandscapeLeft[] = "DeviceOrientation.landscapeLeft";
constexpr char kLandscapeRight[] = "DeviceOrientation.landscapeRight";

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

// Naive implementation using std::string as a container of internal clipboard
// data.
std::string text_clipboard = "";

}  // namespace

PlatformChannel::PlatformChannel(BinaryMessenger* messenger,
                                 TizenRenderer* renderer)
    : channel_(std::make_unique<MethodChannel<rapidjson::Document>>(
          messenger,
          kChannelName,
          &JsonMethodCodec::GetInstance())),
      renderer_(renderer) {
  channel_->SetMethodCallHandler(
      [this](const MethodCall<rapidjson::Document>& call,
             std::unique_ptr<MethodResult<rapidjson::Document>> result) {
        HandleMethodCall(call, std::move(result));
      });
}

PlatformChannel::~PlatformChannel() {}

void PlatformChannel::HandleMethodCall(
    const MethodCall<rapidjson::Document>& call,
    std::unique_ptr<MethodResult<rapidjson::Document>> result) {
  const auto method = call.method_name();
  const auto arguments = call.arguments();

  if (method == kSystemNavigatorPopMethod) {
    SystemNavigatorPop();
    result->Success();
  } else if (method == kPlaySoundMethod) {
    PlaySystemSound(arguments[0].GetString());
    result->Success();
  } else if (method == kHapticFeedbackVibrateMethod) {
    std::string type;
    if (arguments->IsString()) {
      type = arguments[0].GetString();
    }
    HapticFeedbackVibrate(type);
    result->Success();
  } else if (method == kGetClipboardDataMethod) {
    // https://api.flutter.dev/flutter/services/Clipboard/kTextPlain-constant.html
    // The API supports only kTextPlain format.
    if (strcmp(arguments[0].GetString(), kTextPlainFormat) != 0) {
      result->Error(kUnknownClipboardFormatError,
                    "Clipboard API only supports text.");
      return;
    }
    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
    document.AddMember(rapidjson::Value(kTextKey, allocator),
                       rapidjson::Value(text_clipboard, allocator), allocator);
    result->Success(document);
  } else if (method == kSetClipboardDataMethod) {
    const rapidjson::Value& document = *arguments;
    auto iter = document.FindMember(kTextKey);
    if (iter == document.MemberEnd()) {
      result->Error(kUnknownClipboardError, "Invalid message format.");
      return;
    }
    text_clipboard = iter->value.GetString();
    result->Success();
  } else if (method == kClipboardHasStringsMethod) {
    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
    document.AddMember(rapidjson::Value(kValueKey, allocator),
                       rapidjson::Value(!text_clipboard.empty()), allocator);
    result->Success(document);
  } else if (method == kRestoreSystemUIOverlaysMethod) {
    RestoreSystemUIOverlays();
    result->Success();
  } else if (method == kSetEnabledSystemUIOverlaysMethod) {
    const auto& list = arguments[0];
    std::vector<std::string> overlays;
    for (auto iter = list.Begin(); iter != list.End(); ++iter) {
      overlays.push_back(iter->GetString());
    }
    SetEnabledSystemUIOverlays(overlays);
    result->Success();
  } else if (method == kSetPreferredOrientationsMethod) {
    const auto& list = arguments[0];
    std::vector<std::string> orientations;
    for (auto iter = list.Begin(); iter != list.End(); ++iter) {
      orientations.push_back(iter->GetString());
    }
    SetPreferredOrientations(orientations);
    result->Success();
  } else if (method == kSetApplicationSwitcherDescriptionMethod) {
    result->NotImplemented();
  } else if (method == kSetEnabledSystemUIModeMethod) {
    result->NotImplemented();
  } else if (method == kSetSystemUIOverlayStyleMethod) {
    result->NotImplemented();
  } else {
    FT_LOG(Info) << "Unimplemented method: " << method;
    result->NotImplemented();
  }
}

void PlatformChannel::SystemNavigatorPop() {
  ui_app_exit();
}

void PlatformChannel::PlaySystemSound(const std::string& sound_type) {
  if (sound_type == kSoundTypeClick) {
    FeedbackManager::GetInstance().PlayTapSound();
  } else {
    FeedbackManager::GetInstance().PlaySound();
  }
}

void PlatformChannel::HapticFeedbackVibrate(const std::string& feedback_type) {
  FeedbackManager::GetInstance().Vibrate();
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
    // The empty list causes the application to defer to the operating system
    // default.
    rotations = {0, 90, 180, 270};
  }
  renderer_->SetPreferredOrientations(rotations);
}

}  // namespace flutter
