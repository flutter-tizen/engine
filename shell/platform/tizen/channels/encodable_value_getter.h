// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/common/client_wrapper/include/flutter/encodable_value.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

template <typename T>
struct EncodableMapValueGetter {
  EncodableMapValueGetter(const EncodableMap* encodable_map,
                          const std::string& key) {
    auto iter = encodable_map->find(EncodableValue(key));
    if (iter != encodable_map->end() && !iter->second.IsNull()) {
      value = std::get_if<T>(&iter->second);
    }
  }

  ~EncodableMapValueGetter() {}

  const T& operator*() { return *value; }

  const T* operator->() { return value; }

  explicit operator bool() const { return value; }

  const T* value = nullptr;
};

}  // namespace flutter
