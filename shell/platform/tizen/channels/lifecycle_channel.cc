// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "lifecycle_channel.h"

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/tizen_log.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "flutter/lifecycle";

constexpr char kInactive[] = "AppLifecycleState.inactive";
constexpr char kResumed[] = "AppLifecycleState.resumed";
constexpr char kPaused[] = "AppLifecycleState.paused";
constexpr char kDetached[] = "AppLifecycleState.detached";

}  // namespace

LifecycleChannel::LifecycleChannel(FlutterTizenEngine* engine)
    : engine_(engine) {}

LifecycleChannel::~LifecycleChannel() {}

void LifecycleChannel::SendLifecycleMessage(const char message[]) {
  engine_->SendPlatformMessage(kChannelName,
                               reinterpret_cast<const uint8_t*>(message),
                               strlen(message), nullptr, nullptr);
}

void LifecycleChannel::AppIsInactive() {
  FT_LOGI("send app lifecycle state inactive.");
  SendLifecycleMessage(kInactive);
}

void LifecycleChannel::AppIsResumed() {
  FT_LOGI("send app lifecycle state resumed.");
  SendLifecycleMessage(kResumed);
}

void LifecycleChannel::AppIsPaused() {
  FT_LOGI("send app lifecycle state paused.");
  SendLifecycleMessage(kPaused);
}

void LifecycleChannel::AppIsDetached() {
  FT_LOGI("send app lifecycle state detached.");
  SendLifecycleMessage(kDetached);
}

}  // namespace flutter
