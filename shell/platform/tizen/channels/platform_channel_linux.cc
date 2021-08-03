// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform_channel.h"

#include <cstdlib>

#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

void PlatformChannel::SystemNavigatorPop() {
  exit(EXIT_SUCCESS);
}

void PlatformChannel::PlaySystemSound(const std::string& sound_type) {
  FT_UNIMPLEMENTED();
}

void PlatformChannel::HapticFeedbackVibrate(const std::string& feedback_type) {
  FT_UNIMPLEMENTED();
}

void PlatformChannel::SetPreferredOrientations(
    const std::vector<std::string>& orientations) {
  FT_UNIMPLEMENTED();
}

}  // namespace flutter
