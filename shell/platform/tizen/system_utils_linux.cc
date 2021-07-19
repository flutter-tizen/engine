// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "system_utils.h"

#include <cstdlib>

namespace flutter {

namespace {

const char* GetLocaleStringFromEnvironment() {
  const char* retval;
  retval = getenv("LANGUAGE");
  if (retval && (retval[0] != '\0')) {
    return retval;
  }
  retval = getenv("LC_ALL");
  if (retval && (retval[0] != '\0')) {
    return retval;
  }
  retval = getenv("LC_MESSAGES");
  if (retval && (retval[0] != '\0')) {
    return retval;
  }
  retval = getenv("LANG");
  if (retval && (retval[0] != '\0')) {
    return retval;
  }
  return "";
}

}  // namespace

std::vector<LanguageInfo> GetPreferredLanguageInfo() {
  std::vector<LanguageInfo> languages;

  std::string locale(GetLocaleStringFromEnvironment());
  if (locale.empty()) {
    // This is the default locale if none is specified according to ISO C.
    locale = "C";
  }
  LanguageInfo info;
  size_t country_pos = locale.find("_");
  size_t codeset_pos = locale.find(".");
  info.language = locale.substr(0, country_pos);
  if (country_pos != std::string::npos) {
    info.country = locale.substr(country_pos + 1, codeset_pos);
  }
  languages.push_back(info);

  return languages;
}

}  // namespace flutter
