// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "lifecycle_channel.h"

#include "flutter/shell/platform/tizen/tizen_log.h"

static constexpr char kChannelName[] = "flutter/lifecycle";
static constexpr char kInactive[] = "AppLifecycleState.inactive";
static constexpr char kResumed[] = "AppLifecycleState.resumed";
static constexpr char kPaused[] = "AppLifecycleState.paused";
static constexpr char kDetached[] = "AppLifecycleState.detached";

LifecycleChannel::LifecycleChannel(FLUTTER_API_SYMBOL(FlutterEngine)
                                       flutter_engine)
    : flutter_engine_(flutter_engine) {}

LifecycleChannel::~LifecycleChannel() {}

void LifecycleChannel::SendLifecycleMessage(const char message[]) {
  FlutterPlatformMessage platformMessage = {};
  platformMessage.struct_size = sizeof(FlutterPlatformMessage);
  platformMessage.channel = kChannelName;
  platformMessage.message = reinterpret_cast<const uint8_t*>(message);
  platformMessage.message_size = strlen(message);
  platformMessage.response_handle = nullptr;
  FlutterEngineSendPlatformMessage(flutter_engine_, &platformMessage);
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
