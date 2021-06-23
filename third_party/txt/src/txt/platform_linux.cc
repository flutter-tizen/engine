// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "txt/platform.h"

#ifdef FLUTTER_USE_FONTCONFIG
#include "third_party/skia/include/ports/SkFontMgr_fontconfig.h"
#else
#include "third_party/skia/include/ports/SkFontMgr_directory.h"
#endif

namespace txt {

std::vector<std::string> GetDefaultFontFamilies() {
  return {"TizenDefaultFont", "SamsungOneUI"};
}

sk_sp<SkFontMgr> GetDefaultFontManager(uint32_t font_initialization_data) {
#ifdef FLUTTER_USE_FONTCONFIG
  return SkFontMgr::RefDefault();
#else
  return SkFontMgr_New_Custom_Directory("/usr/share/fonts/");
#endif
}

}  // namespace txt
