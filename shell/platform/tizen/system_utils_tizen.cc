// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_utils.h"

#include <utils_i18n.h>

#include "flutter/shell/platform/tizen/tizen_log.h"

namespace flutter {

std::vector<LanguageInfo> GetPreferredLanguageInfo() {
  std::vector<LanguageInfo> languages;
  const char* locale;

  i18n_ulocale_set_default(getenv("LANG"));
  int ret = i18n_ulocale_get_default(&locale);
  if (ret != I18N_ERROR_NONE) {
    FT_LOGE("i18n_ulocale_get_default() failed.");
    return languages;
  }
  std::string preferred_locale(locale);
  size_t codeset_pos = preferred_locale.find(".");
  preferred_locale = preferred_locale.substr(0, codeset_pos);

  int32_t count = i18n_ulocale_count_available();
  languages.reserve(count);
  for (int i = 0; i < count; i++) {
    LanguageInfo info;
    int ret;
    char buffer[128] = {0};
    int32_t size;

    // The "language" field is required.
    locale = i18n_ulocale_get_available(i);
    ret = i18n_ulocale_get_language(locale, buffer, sizeof(buffer), &size);
    if (ret != I18N_ERROR_NONE || size == 0) {
      continue;
    }
    info.language = std::string(buffer, size);

    // "country", "script", and "variant" are optional.
    size = i18n_ulocale_get_country(locale, buffer, sizeof(buffer), &ret);
    if (ret == I18N_ERROR_NONE && size > 0) {
      info.country = std::string(buffer, size);
    }
    size = i18n_ulocale_get_script(locale, buffer, sizeof(buffer));
    if (size > 0) {
      info.script = std::string(buffer, size);
    }
    size = i18n_ulocale_get_variant(locale, buffer, sizeof(buffer));
    if (size > 0) {
      info.variant = std::string(buffer, size);
    }

    if (preferred_locale.compare(locale) == 0) {
      languages.insert(languages.begin(), info);
    } else {
      languages.push_back(info);
    }
  }
  FT_LOGI("Found %zu locales.", languages.size());

  return languages;
}

}  // namespace flutter
