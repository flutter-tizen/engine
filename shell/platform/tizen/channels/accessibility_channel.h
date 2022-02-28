// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_ACCESSIBILITY_CHANNEL_H_
#define EMBEDDER_ACCESSIBILITY_CHANNEL_H_

#include <functional>
#include <memory>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/basic_message_channel.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"

namespace flutter {

class AccessibilityChannel {
 public:
  explicit AccessibilityChannel(BinaryMessenger* messenger, bool enabled);
  virtual ~AccessibilityChannel();

  void SetState(bool enabled);

 private:
  bool enabled_;
  std::unique_ptr<BasicMessageChannel<EncodableValue>> channel_;
};

}  // namespace flutter

#endif  // EMBEDDER_ACCESSIBILITY_CHANNEL_H_
