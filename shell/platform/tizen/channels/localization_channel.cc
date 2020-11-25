// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "localization_channel.h"

#include <utils_i18n.h>

#include <vector>

#include "flutter/shell/platform/tizen/logger.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"

static constexpr char kChannelName[] = "flutter/localization";

LocalizationChannel::LocalizationChannel(FLUTTER_API_SYMBOL(FlutterEngine)
                                             flutter_engine)
    : flutter_engine_(flutter_engine) {}

LocalizationChannel::~LocalizationChannel() {}

void LocalizationChannel::SendLocales() {
  int32_t count = i18n_ulocale_count_available();
  if (count > 0) {
    FlutterLocale* flutterLocale = nullptr;
    std::vector<FlutterLocale*> flutterLocales;
    for (int i = 0; i < count; i++) {
      const char* locale = i18n_ulocale_get_available(i);
      flutterLocale = GetFlutterLocale(locale);
      if (flutterLocale) {
        flutterLocales.push_back(flutterLocale);
      }
    }

    // send locales to engine
    FlutterEngineUpdateLocales(
        flutter_engine_,
        const_cast<const FlutterLocale**>(flutterLocales.data()),
        flutterLocales.size());

    for (auto it : flutterLocales) {
      DestroyFlutterLocale(it);
    }
  }

  SendPlatformResolvedLocale();
}

void LocalizationChannel::SendPlatformResolvedLocale() {
  const char* locale;
  int ret = i18n_ulocale_get_default(&locale);
  if (ret != I18N_ERROR_NONE) {
    LoggerE("i18n_ulocale_get_default() failed.");
    return;
  }

  FlutterLocale* flutterLocale = GetFlutterLocale(locale);
  if (!flutterLocale) {
    LoggerE("Language code is required but not present.");
    return;
  }

  rapidjson::Document document;
  auto& allocator = document.GetAllocator();

  document.SetObject();
  document.AddMember("method", "setPlatformResolvedLocale", allocator);

  rapidjson::Value language_code, country_code, script_code, variant_code;
  language_code.SetString(flutterLocale->language_code, allocator);
  country_code.SetString(
      flutterLocale->country_code ? flutterLocale->country_code : "",
      allocator);
  script_code.SetString(
      flutterLocale->script_code ? flutterLocale->script_code : "", allocator);
  variant_code.SetString(
      flutterLocale->variant_code ? flutterLocale->variant_code : "",
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
    LoggerE("document.Accept failed!");
    return;
  }

  FlutterPlatformMessage message = {};
  message.struct_size = sizeof(FlutterPlatformMessage);
  message.channel = kChannelName;
  message.message = reinterpret_cast<const uint8_t*>(buffer.GetString());
  message.message_size = buffer.GetSize();
  message.response_handle = nullptr;
  FlutterEngineSendPlatformMessage(flutter_engine_, &message);

  DestroyFlutterLocale(flutterLocale);
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
    LoggerE("i18n_ulocale_get_language failed!");
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

  FlutterLocale* flutterLocale = new FlutterLocale;
  flutterLocale->struct_size = sizeof(FlutterLocale);
  flutterLocale->language_code = language;
  flutterLocale->country_code = country;
  flutterLocale->script_code = script;
  flutterLocale->variant_code = variant;

  return flutterLocale;
}

void LocalizationChannel::DestroyFlutterLocale(FlutterLocale* flutterLocale) {
  if (flutterLocale) {
    if (flutterLocale->language_code) {
      delete[] flutterLocale->language_code;
      flutterLocale->language_code = nullptr;
    }

    if (flutterLocale->country_code) {
      delete[] flutterLocale->country_code;
      flutterLocale->country_code = nullptr;
    }

    if (flutterLocale->script_code) {
      delete[] flutterLocale->script_code;
      flutterLocale->script_code = nullptr;
    }

    if (flutterLocale->variant_code) {
      delete[] flutterLocale->variant_code;
      flutterLocale->variant_code = nullptr;
    }

    delete flutterLocale;
    flutterLocale = nullptr;
  }
}
