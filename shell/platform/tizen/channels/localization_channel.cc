// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "localization_channel.h"

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/tizen_log.h"

namespace flutter {

LocalizationChannel::LocalizationChannel(FlutterTizenEngine* engine)
    : engine_(engine) {
  SendLocales();
}

LocalizationChannel::~LocalizationChannel() {}

void LocalizationChannel::SendLocales() {
  std::vector<FlutterLocale*> flutter_locales = GetFlutterLocales();

  FT_LOGI("Send %zu available locales", flutter_locales.size());
  // Send locales to engine
  engine_->UpdateLocales(
      const_cast<const FlutterLocale**>(flutter_locales.data()),
      flutter_locales.size());

  for (auto it : flutter_locales) {
    DestroyFlutterLocale(it);
  }
}

void LocalizationChannel::DestroyFlutterLocale(FlutterLocale* flutter_locale) {
  if (flutter_locale) {
    if (flutter_locale->language_code) {
      delete[] flutter_locale->language_code;
      flutter_locale->language_code = nullptr;
    }

    if (flutter_locale->country_code) {
      delete[] flutter_locale->country_code;
      flutter_locale->country_code = nullptr;
    }

    if (flutter_locale->script_code) {
      delete[] flutter_locale->script_code;
      flutter_locale->script_code = nullptr;
    }

    if (flutter_locale->variant_code) {
      delete[] flutter_locale->variant_code;
      flutter_locale->variant_code = nullptr;
    }

    delete flutter_locale;
    flutter_locale = nullptr;
  }
}

}  // namespace flutter
