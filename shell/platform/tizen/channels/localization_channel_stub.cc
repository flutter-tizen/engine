// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "localization_channel.h"

namespace flutter {

void LocalizationChannel::SendPlatformResolvedLocale() {}

std::vector<FlutterLocale*> LocalizationChannel::GetFlutterLocales() {
  FlutterLocale* flutter_locale = GetFlutterLocale(nullptr);
  std::vector<FlutterLocale*> flutter_locales;
  flutter_locales.push_back(flutter_locale);
  return flutter_locales;
}

FlutterLocale* LocalizationChannel::GetFlutterLocale(const char* locale) {
  FlutterLocale* flutter_locale = new FlutterLocale;
  flutter_locale->struct_size = sizeof(FlutterLocale);
  flutter_locale->language_code = new char[1];
  flutter_locale->country_code = new char[1];
  flutter_locale->script_code = new char[1];
  flutter_locale->variant_code = new char[1];
  return flutter_locale;
}

}  // namespace flutter
