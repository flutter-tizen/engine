// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "lifecycle_channel.h"

#include "flutter/shell/platform/tizen/channels/string_codec.h"
#include "flutter/shell/platform/tizen/tizen_log.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "flutter/lifecycle";

constexpr char kInactive[] = "AppLifecycleState.inactive";
constexpr char kResumed[] = "AppLifecycleState.resumed";
constexpr char kPaused[] = "AppLifecycleState.paused";
constexpr char kDetached[] = "AppLifecycleState.detached";

}  // namespace

LifecycleChannel::LifecycleChannel(BinaryMessenger* messenger)
    : channel_(std::make_unique<BasicMessageChannel<EncodableValue>>(
          messenger,
          kChannelName,
          &StringCodec::GetInstance())) {}

LifecycleChannel::~LifecycleChannel() {}

void LifecycleChannel::AppIsInactive() {
  FT_LOGI("Sending %s message.", kInactive);
  channel_->Send(EncodableValue(kInactive));
}

void LifecycleChannel::AppIsResumed() {
  FT_LOGI("Sending %s message.", kResumed);
  channel_->Send(EncodableValue(kResumed));
}

void LifecycleChannel::AppIsPaused() {
  FT_LOGI("Sending %s message.", kPaused);
  channel_->Send(EncodableValue(kPaused));
}

void LifecycleChannel::AppIsDetached() {
  FT_LOGI("Sending %s message.", kDetached);
  channel_->Send(EncodableValue(kDetached));
}

}  // namespace flutter
