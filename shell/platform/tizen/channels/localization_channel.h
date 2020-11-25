// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_LOCALIZATION_CHANNEL_H_
#define EMBEDDER_LOCALIZATION_CHANNEL_H_

#include "flutter/shell/platform/embedder/embedder.h"

class LocalizationChannel {
 public:
  explicit LocalizationChannel(FLUTTER_API_SYMBOL(FlutterEngine)
                                   flutter_engine);
  virtual ~LocalizationChannel();
  void SendLocales();

 private:
  void SendPlatformResolvedLocale();
  FlutterLocale* GetFlutterLocale(const char* locale);
  void DestroyFlutterLocale(FlutterLocale* flutterLocale);

  FLUTTER_API_SYMBOL(FlutterEngine) flutter_engine_;
};
#endif  // EMBEDDER_LOCALIZATION_CHANNEL_H_
