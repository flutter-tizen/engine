// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "lifecycle_channel.h"

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/tizen_log.h"

static constexpr char kChannelName[] = "flutter/lifecycle";
static constexpr char kInactive[] = "AppLifecycleState.inactive";
static constexpr char kResumed[] = "AppLifecycleState.resumed";
static constexpr char kPaused[] = "AppLifecycleState.paused";
static constexpr char kDetached[] = "AppLifecycleState.detached";

LifecycleChannel::LifecycleChannel(FlutterTizenEngine* engine)
    : engine_(engine) {}

LifecycleChannel::~LifecycleChannel() {}

void LifecycleChannel::SendLifecycleMessage(const char message[]) {
  engine_->SendPlatformMessage(kChannelName,
                               reinterpret_cast<const uint8_t*>(message),
                               strlen(message), nullptr, nullptr);
}

void LifecycleChannel::AppIsInactive() {
  FT_LOGD("send app lifecycle state inactive.");
  SendLifecycleMessage(kInactive);
}

void LifecycleChannel::AppIsResumed() {
  FT_LOGD("send app lifecycle state resumed.");
  SendLifecycleMessage(kResumed);
}

void LifecycleChannel::AppIsPaused() {
  FT_LOGD("send app lifecycle state paused.");
  SendLifecycleMessage(kPaused);
}

void LifecycleChannel::AppIsDetached() {
  FT_LOGD("send app lifecycle state detached.");
  SendLifecycleMessage(kDetached);
}
