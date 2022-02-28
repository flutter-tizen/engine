// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "accessibility_channel.h"

#include <Elementary.h>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_message_codec.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "flutter/accessibility";

}  // namespace

AccessibilityChannel::AccessibilityChannel(BinaryMessenger* messenger,
                                           bool enabled)
    : enabled_(enabled),
      channel_(std::make_unique<BasicMessageChannel<EncodableValue>>(
          messenger,
          kChannelName,
          &StandardMessageCodec::GetInstance())) {
  channel_->SetMessageHandler([&](const auto& message, auto reply) {
    if (enabled_) {
      if (std::holds_alternative<flutter::EncodableMap>(message)) {
        auto eMessage = std::get<flutter::EncodableMap>(message);
        auto& eType = eMessage[flutter::EncodableValue("type")];
        auto& eData = eMessage[flutter::EncodableValue("data")];
        if (std::holds_alternative<std::string>(eType)) {
          std::string type = std::get<std::string>(eType);
          FT_LOG(Info) << "Recevie " << type
                       << " event from accessibility channel";

          if (type.compare("announce") == 0) {
            if (std::holds_alternative<flutter::EncodableMap>(eData)) {
              auto data = std::get<flutter::EncodableMap>(eData);
              auto& eMsg = data[flutter::EncodableValue("message")];
              if (std::holds_alternative<std::string>(eMsg)) {
                std::string msg = std::get<std::string>(eMsg);
                elm_atspi_bridge_utils_say(msg.c_str(), true, nullptr, nullptr);
              }
            }
            reply(flutter::EncodableValue());
          }
        }
      }
    }
  });
}

AccessibilityChannel::~AccessibilityChannel() {
  channel_->SetMessageHandler(nullptr);
}

void AccessibilityChannel::SetState(bool enabled) {
  enabled_ = enabled;
}

}  // namespace flutter
