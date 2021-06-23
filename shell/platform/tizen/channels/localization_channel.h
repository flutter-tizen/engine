// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_LOCALIZATION_CHANNEL_H_
#define EMBEDDER_LOCALIZATION_CHANNEL_H_

#include "flutter/shell/platform/embedder/embedder.h"

namespace flutter {

class FlutterTizenEngine;

class LocalizationChannel {
 public:
  explicit LocalizationChannel(FlutterTizenEngine* engine);
  virtual ~LocalizationChannel();

  void SendLocales();

 private:
  void SendPlatformResolvedLocale();
  FlutterLocale* GetFlutterLocale(const char* locale);
  void DestroyFlutterLocale(FlutterLocale* flutter_locale);

  FlutterTizenEngine* engine_;
};

}  // namespace flutter

#endif  // EMBEDDER_LOCALIZATION_CHANNEL_H_
