// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "localization_channel.h"

#include <utils_i18n.h>
#include <vector>

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/tizen_log.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"

static constexpr char kChannelName[] = "flutter/localization";

LocalizationChannel::LocalizationChannel(FlutterTizenEngine* engine)
    : engine_(engine) {}

LocalizationChannel::~LocalizationChannel() {}

void LocalizationChannel::SendLocales() {
  const char* defualt_locale = nullptr;
  FlutterLocale* flutter_locale = nullptr;
  std::vector<FlutterLocale*> flutter_locales;

  int ret = i18n_ulocale_set_default(getenv("LANG"));
  ret = i18n_ulocale_get_default(&defualt_locale);
  if (ret != I18N_ERROR_NONE) {
    FT_LOGE("i18n_ulocale_get_default() failed.");
    return;
  }

  std::string without_encoding_type(defualt_locale);
  auto n = without_encoding_type.find('.');
  without_encoding_type.erase(n, without_encoding_type.length() - n);

  flutter_locale = GetFlutterLocale(without_encoding_type.data());
  if (flutter_locale) {
    FT_LOGI("Choose Default locale[%s]", without_encoding_type.data());
    flutter_locales.push_back(flutter_locale);
  }

  int32_t count = i18n_ulocale_count_available();
  for (int i = 0; i < count; i++) {
    const char* locale = i18n_ulocale_get_available(i);
    if (without_encoding_type.compare(locale) != 0) {
      flutter_locale = GetFlutterLocale(locale);
      if (flutter_locale) {
        flutter_locales.push_back(flutter_locale);
      }
    }
  }

  FT_LOGI("Send %zu available locales", flutter_locales.size());
  // Send locales to engine
  engine_->UpdateLocales(
      const_cast<const FlutterLocale**>(flutter_locales.data()),
      flutter_locales.size());

  for (auto it : flutter_locales) {
    DestroyFlutterLocale(it);
  }
}

void LocalizationChannel::SendPlatformResolvedLocale() {
  const char* locale;
  int ret = i18n_ulocale_get_default(&locale);
  if (ret != I18N_ERROR_NONE) {
    FT_LOGE("i18n_ulocale_get_default() failed.");
    return;
  }

  FlutterLocale* flutter_locale = GetFlutterLocale(locale);
  if (!flutter_locale) {
    FT_LOGE("Language code is required but not present.");
    return;
  }

  rapidjson::Document document;
  auto& allocator = document.GetAllocator();

  document.SetObject();
  document.AddMember("method", "setPlatformResolvedLocale", allocator);

  rapidjson::Value language_code, country_code, script_code, variant_code;
  language_code.SetString(flutter_locale->language_code, allocator);
  country_code.SetString(
      flutter_locale->country_code ? flutter_locale->country_code : "",
      allocator);
  script_code.SetString(
      flutter_locale->script_code ? flutter_locale->script_code : "",
      allocator);
  variant_code.SetString(
      flutter_locale->variant_code ? flutter_locale->variant_code : "",
      allocator);

  rapidjson::Value args(rapidjson::kArrayType);
  args.Reserve(4, allocator);
  args.PushBack(language_code, allocator);
  args.PushBack(country_code, allocator);
  args.PushBack(script_code, allocator);
  args.PushBack(variant_code, allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  if (!document.Accept(writer)) {
    FT_LOGE("document.Accept failed!");
    return;
  }

  engine_->SendPlatformMessage(
      kChannelName, reinterpret_cast<const uint8_t*>(buffer.GetString()),
      buffer.GetSize(), nullptr, nullptr);

  DestroyFlutterLocale(flutter_locale);
}

FlutterLocale* LocalizationChannel::GetFlutterLocale(const char* locale) {
  int capacity = 128;
  char buffer[128] = {0};
  int bufSize;
  int error;
  char* language = nullptr;
  char* country = nullptr;
  char* script = nullptr;
  char* variant = nullptr;

  // set language code, language code is a required field
  error = i18n_ulocale_get_language(locale, buffer, capacity, &bufSize);
  if (error == I18N_ERROR_NONE && bufSize > 0) {
    language = new char[bufSize + 1];
    memcpy(language, buffer, bufSize);
    language[bufSize] = '\0';
  } else {
    FT_LOGE("i18n_ulocale_get_language failed!");
    return nullptr;
  }

  // set country code, country code is an optional field
  bufSize = i18n_ulocale_get_country(locale, buffer, capacity, &error);
  if (error == I18N_ERROR_NONE && bufSize > 0) {
    country = new char[bufSize + 1];
    memcpy(country, buffer, bufSize);
    country[bufSize] = '\0';
  }

  // set script code, script code is an optional field
  bufSize = i18n_ulocale_get_script(locale, buffer, capacity);
  if (bufSize > 0) {
    script = new char[bufSize + 1];
    memcpy(script, buffer, bufSize);
    script[bufSize] = '\0';
  }

  // set variant code, variant code is an optional field
  bufSize = i18n_ulocale_get_variant(locale, buffer, capacity);
  if (bufSize > 0) {
    variant = new char[bufSize + 1];
    memcpy(variant, buffer, bufSize);
    variant[bufSize] = '\0';
  }

  FlutterLocale* flutter_locale = new FlutterLocale;
  flutter_locale->struct_size = sizeof(FlutterLocale);
  flutter_locale->language_code = language;
  flutter_locale->country_code = country;
  flutter_locale->script_code = script;
  flutter_locale->variant_code = variant;

  return flutter_locale;
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
