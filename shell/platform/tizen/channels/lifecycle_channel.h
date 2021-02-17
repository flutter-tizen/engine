// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_LIFECYCLE_CHANNEL_H_
#define EMBEDDER_LIFECYCLE_CHANNEL_H_

#include "flutter/shell/platform/embedder/embedder.h"

class LifecycleChannel {
 public:
  explicit LifecycleChannel(FLUTTER_API_SYMBOL(FlutterEngine) flutter_engine);
  virtual ~LifecycleChannel();
  void AppIsInactive();
  void AppIsResumed();
  void AppIsPaused();
  void AppIsDetached();
  void SendLifecycleMessage(const char message[]);

 private:
  FLUTTER_API_SYMBOL(FlutterEngine) flutter_engine_;
};

#endif  // EMBEDDER_LIFECYCLE_CHANNEL_H_
