// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform_channel.h"

#include <app.h>
#include <feedback.h>

#include <map>

#include "flutter/shell/platform/common/json_method_codec.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "flutter/platform";

constexpr char kGetClipboardDataMethod[] = "Clipboard.getData";
constexpr char kSetClipboardDataMethod[] = "Clipboard.setData";
constexpr char kPlaySoundMethod[] = "SystemSound.play";
constexpr char kHapticFeedbackVibrateMethod[] = "HapticFeedback.vibrate";
constexpr char kSystemNavigatorPopMethod[] = "SystemNavigator.pop";
constexpr char kSetPreferredOrientationsMethod[] =
    "SystemChrome.setPreferredOrientations";
constexpr char kSetApplicationSwitcherDescriptionMethod[] =
    "SystemChrome.setApplicationSwitcherDescription";
constexpr char kSetEnabledSystemUIOverlaysMethod[] =
    "SystemChrome.setEnabledSystemUIOverlays";
constexpr char kRestoreSystemUIOverlaysMethod[] =
    "SystemChrome.restoreSystemUIOverlays";
constexpr char kSetSystemUIOverlayStyleMethod[] =
    "SystemChrome.setSystemUIOverlayStyle";

constexpr char kSoundTypeClick[] = "SystemSoundType.click";

class FeedbackManager {
 public:
  enum class ResultCode {
    kOk,
    kNotSupportedError,
    kPermissionDeniedError,
    kUnknownError
  };

  enum class FeedbackPattern {
    kClick = FEEDBACK_PATTERN_TAP,
    kAlert = FEEDBACK_PATTERN_GENERAL,
    kSip = FEEDBACK_PATTERN_SIP
  };

  enum class FeedbackType {
    kVibration = FEEDBACK_TYPE_VIBRATION,
    kSound = FEEDBACK_TYPE_SOUND
  };

  static std::string GetVibrateVariantName(const char* haptic_feedback_type) {
    if (!haptic_feedback_type) {
      return "HapticFeedback.vibrate";
    }

    const size_t kPrefixToRemoveLen = strlen("HapticFeedbackType.");

    assert(strlen(haptic_feedback_type) >= kPrefixToRemoveLen);

    const std::string kHapticFeedbackPrefix = "HapticFeedback.";

    return kHapticFeedbackPrefix +
           std::string{haptic_feedback_type + kPrefixToRemoveLen};
  }

  static std::string GetErrorMessage(ResultCode result_code,
                                     const std::string& method_name,
                                     const std::string& args = "") {
    const auto method_name_with_args = method_name + "(" + args + ")";

    switch (result_code) {
      case ResultCode::kNotSupportedError:
        return method_name_with_args + " is not supported";
      case ResultCode::kPermissionDeniedError:
        return std::string{"No permission to run "} + method_name_with_args +
               ". Add \"http://tizen.org/privilege/haptic\" privilege to "
               "tizen-manifest.xml to use this method";
      case ResultCode::kUnknownError:
      default:
        return std::string{"An unknown error on "} + method_name_with_args +
               " call";
    }
  }

  static FeedbackManager& GetInstance() {
    static FeedbackManager instance;
    return instance;
  }

  FeedbackManager(const FeedbackManager&) = delete;
  FeedbackManager& operator=(const FeedbackManager&) = delete;

  ResultCode Play(FeedbackType type, FeedbackPattern pattern) {
    if (ResultCode::kOk != initialization_status_) {
      return initialization_status_;
    }

    auto ret = feedback_play_type(static_cast<feedback_type_e>(type),
                                  static_cast<feedback_pattern_e>(pattern));
    if (FEEDBACK_ERROR_NONE == ret) {
      return ResultCode::kOk;
    }
    FT_LOG(Error) << "feedback_play_type() failed with error: "
                  << get_error_message(ret);

    return NativeErrorToResultCode(ret);
  }

 private:
  static ResultCode NativeErrorToResultCode(int native_error_code) {
    switch (native_error_code) {
      case FEEDBACK_ERROR_NONE:
        return ResultCode::kOk;
      case FEEDBACK_ERROR_NOT_SUPPORTED:
        return ResultCode::kNotSupportedError;
      case FEEDBACK_ERROR_PERMISSION_DENIED:
        return ResultCode::kPermissionDeniedError;
      case FEEDBACK_ERROR_OPERATION_FAILED:
      case FEEDBACK_ERROR_INVALID_PARAMETER:
      case FEEDBACK_ERROR_NOT_INITIALIZED:
      default:
        return ResultCode::kUnknownError;
    }
  }

  FeedbackManager() {
    auto ret = feedback_initialize();
    if (FEEDBACK_ERROR_NONE != ret) {
      FT_LOG(Error) << "feedback_initialize() failed with error: "
                    << get_error_message(ret);
      initialization_status_ = NativeErrorToResultCode(ret);
      return;
    }

    initialization_status_ = ResultCode::kOk;
  }

  ~FeedbackManager() {
    auto ret = feedback_deinitialize();
    if (FEEDBACK_ERROR_NONE != ret) {
      FT_LOG(Error) << "feedback_deinitialize() failed with error: "
                    << get_error_message(ret);
      return;
    }
  }

  ResultCode initialization_status_ = ResultCode::kUnknownError;
};

}  // namespace

// Clipboard constants and variables
namespace clipboard {

// naive implementation using std::string as a container of internal clipboard
// data
std::string string_clipboard = "";

static constexpr char kTextKey[] = "text";
static constexpr char kTextPlainFormat[] = "text/plain";
static constexpr char kUnknownClipboardFormatError[] =
    "Unknown clipboard format error";
static constexpr char kUnknownClipboardError[] =
    "Unknown error during clipboard data retrieval";

void GetData(const MethodCall<rapidjson::Document>& call,
             std::unique_ptr<MethodResult<rapidjson::Document>> result) {
  const rapidjson::Value& format = call.arguments()[0];

  // https://api.flutter.dev/flutter/services/Clipboard/kTextPlain-constant.html
  // API supports only kTextPlain format
  if (strcmp(format.GetString(), kTextPlainFormat) != 0) {
    result->Error(kUnknownClipboardFormatError,
                  "Clipboard API only supports text.");
    return;
  }

  rapidjson::Document document;
  document.SetObject();
  rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
  document.AddMember(rapidjson::Value(kTextKey, allocator),
                     rapidjson::Value(string_clipboard, allocator), allocator);
  result->Success(document);
}

void SetData(const MethodCall<rapidjson::Document>& call,
             std::unique_ptr<MethodResult<rapidjson::Document>> result) {
  const rapidjson::Value& document = *call.arguments();
  rapidjson::Value::ConstMemberIterator itr = document.FindMember(kTextKey);
  if (itr == document.MemberEnd()) {
    result->Error(kUnknownClipboardError, "Invalid message format");
    return;
  }
  string_clipboard = itr->value.GetString();
  result->Success();
}

}  // namespace clipboard

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

  if (method == kSystemNavigatorPopMethod) {
    ui_app_exit();
    result->Success();
  } else if (method == kPlaySoundMethod) {
    const std::string pattern_str = call.arguments()[0].GetString();

    const FeedbackManager::FeedbackPattern pattern =
        (pattern_str == kSoundTypeClick)
            ? FeedbackManager::FeedbackPattern::kClick
            : FeedbackManager::FeedbackPattern::kAlert;

    auto ret = FeedbackManager::GetInstance().Play(
        FeedbackManager::FeedbackType::kSound, pattern);
    if (FeedbackManager::ResultCode::kOk == ret) {
      result->Success();
      return;
    }

    const auto error_cause =
        FeedbackManager::GetErrorMessage(ret, kPlaySoundMethod, pattern_str);
    const std::string error_message = "Could not play sound";
    FT_LOG(Error) << error_cause << ": " << error_message;

    result->Error(error_cause, error_message);

  } else if (method == kHapticFeedbackVibrateMethod) {
    /*
     * We use a single type of vibration (FEEDBACK_PATTERN_SIP) to implement
     * HapticFeedback's vibrate, lightImpact, mediumImpact, heavyImpact
     * and selectionClick methods, because Tizen's "feedback" module
     * has no dedicated vibration types for them.
     * Thus, we ignore the "arguments" contents for "HapticFeedback.vibrate"
     * calls.
     */

    auto ret = FeedbackManager::GetInstance().Play(
        FeedbackManager::FeedbackType::kVibration,
        FeedbackManager::FeedbackPattern::kSip);
    if (FeedbackManager::ResultCode::kOk == ret) {
      result->Success();
      return;
    }

    const auto vibrate_variant_name =
        FeedbackManager::GetVibrateVariantName(call.arguments()[0].GetString());
    const auto error_cause =
        FeedbackManager::GetErrorMessage(ret, vibrate_variant_name);
    const std::string error_message = "Could not vibrate";

    FT_LOG(Error) << error_cause << ": " << error_message;

    result->Error(error_cause, error_message);
  } else if (method == kGetClipboardDataMethod) {
    clipboard::GetData(call, std::move(result));
  } else if (method == kSetClipboardDataMethod) {
    clipboard::SetData(call, std::move(result));
  } else if (method == kSetPreferredOrientationsMethod) {
    if (renderer_) {
      static const std::string kPortraitUp = "DeviceOrientation.portraitUp";
      static const std::string kPortraitDown = "DeviceOrientation.portraitDown";
      static const std::string kLandscapeLeft =
          "DeviceOrientation.landscapeLeft";
      static const std::string kLandscapeRight =
          "DeviceOrientation.landscapeRight";
      static const std::map<std::string, int> orientation_mapping = {
          {kPortraitUp, 0},
          {kLandscapeLeft, 90},
          {kPortraitDown, 180},
          {kLandscapeRight, 270},
      };

      const auto& list = call.arguments()[0];
      std::vector<int> rotations;
      for (rapidjson::Value::ConstValueIterator itr = list.Begin();
           itr != list.End(); ++itr) {
        const std::string& rot = itr->GetString();
        rotations.push_back(orientation_mapping.at(rot));
      }
      if (rotations.size() == 0) {
        // According do docs
        // https://api.flutter.dev/flutter/services/SystemChrome/setPreferredOrientations.html
        // "The empty list causes the application to defer to the operating
        // system default."
        rotations = {0, 90, 180, 270};
      }
      renderer_->SetPreferredOrientations(rotations);
      result->Success();
    } else {
      result->Error("Not supported for service applications");
    }
  } else if (method == kSetApplicationSwitcherDescriptionMethod) {
    result->NotImplemented();
  } else if (method == kSetEnabledSystemUIOverlaysMethod) {
    result->NotImplemented();
  } else if (method == kRestoreSystemUIOverlaysMethod) {
    result->NotImplemented();
  } else if (method == kSetSystemUIOverlayStyleMethod) {
    result->NotImplemented();
  } else {
    FT_LOG(Info) << "Unimplemented method: " << method;
    result->NotImplemented();
  }
}

}  // namespace flutter
