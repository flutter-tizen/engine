// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "localization_channel.h"

namespace flutter {

LocalizationChannel::LocalizationChannel(FlutterTizenEngine* engine) {
  engine_ = nullptr;
}

LocalizationChannel::~LocalizationChannel() {}

void LocalizationChannel::SendLocales() {}

}  // namespace flutter
