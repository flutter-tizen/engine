// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_KEY_MAPPING_H_
#define EMBEDDER_KEY_MAPPING_H_

#include <cinttypes>
#include <map>
#include <string>

// Maps XKB specific key code values to Flutter's physical key code values.
extern std::map<uint64_t, uint64_t> kXkbToPhysicalKeyCode;

// Maps Tizen key symbols to Flutter's logical key code values.
extern std::map<std::string, uint64_t> kKeySymbolToLogicalKeyCode;

// Mask for the 32-bit value portion of the key code.
const uint64_t kValueMask = 0x000ffffffff;

// The plane value for keys which have a Unicode representation.
const uint64_t kUnicodePlane = 0x00000000000;

// The plane value for the private keys defined by the GTK embedding.
const uint64_t kGtkPlane = 0x01500000000;

#endif  // EMBEDDER_KEY_MAPPING_H_
