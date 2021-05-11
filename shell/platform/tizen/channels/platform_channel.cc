// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform_channel.h"

#include <app.h>

#include <map>

#include "flutter/shell/platform/common/cpp/json_method_codec.h"
#include "flutter/shell/platform/tizen/tizen_log.h"

static constexpr char kChannelName[] = "flutter/platform";

PlatformChannel::PlatformChannel(flutter::BinaryMessenger* messenger,
                                 TizenRenderer* renderer)
    : channel_(std::make_unique<flutter::MethodChannel<rapidjson::Document>>(
          messenger, kChannelName, &flutter::JsonMethodCodec::GetInstance())),
      renderer_(renderer) {
  channel_->SetMethodCallHandler(
      [this](
          const flutter::MethodCall<rapidjson::Document>& call,
          std::unique_ptr<flutter::MethodResult<rapidjson::Document>> result) {
        HandleMethodCall(call, std::move(result));
      });
}

PlatformChannel::~PlatformChannel() {}

void PlatformChannel::HandleMethodCall(
    const flutter::MethodCall<rapidjson::Document>& call,
    std::unique_ptr<flutter::MethodResult<rapidjson::Document>> result) {
  const auto method = call.method_name();

  if (method == "SystemNavigator.pop") {
    ui_app_exit();
    result->Success();
  } else if (method == "SystemSound.play") {
    result->NotImplemented();
  } else if (method == "HapticFeedback.vibrate") {
    result->NotImplemented();
  } else if (method == "Clipboard.getData") {
    result->NotImplemented();
  } else if (method == "Clipboard.setData") {
    result->NotImplemented();
  } else if (method == "Clipboard.hasStrings") {
    result->NotImplemented();
  } else if (method == "SystemChrome.setPreferredOrientations") {
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
        FT_LOGD("Passed rotation: %s", rot.c_str());
        rotations.push_back(orientation_mapping.at(rot));
      }
      if (rotations.size() == 0) {
        // According do docs
        // https://api.flutter.dev/flutter/services/SystemChrome/setPreferredOrientations.html
        // "The empty list causes the application to defer to the operating
        // system default."
        FT_LOGD("No rotations passed, using default values");
        rotations = {0, 90, 180, 270};
      }
      renderer_->SetPreferredOrientations(rotations);
      result->Success();
    } else {
      result->Error("Not supported for service applications");
    }
  } else if (method == "SystemChrome.setApplicationSwitcherDescription") {
    result->NotImplemented();
  } else if (method == "SystemChrome.setEnabledSystemUIOverlays") {
    result->NotImplemented();
  } else if (method == "SystemChrome.restoreSystemUIOverlays") {
    result->NotImplemented();
  } else if (method == "SystemChrome.setSystemUIOverlayStyle") {
    result->NotImplemented();
  } else {
    FT_LOGI("Unimplemented method: %s", method.c_str());
    result->NotImplemented();
  }
}
