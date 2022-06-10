// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_KEY_MAPPING_H_
#define EMBEDDER_KEY_MAPPING_H_

#include <map>
#include <string>

// Maps device-specific key symbols to XKB scan codes.
//
// The values are originally defined in:
// - xkb-tizen-data/tizen_key_layout.txt.tv
// - flutter/keyboard_maps.dart (kLinuxToPhysicalKey)
extern std::map<std::string, uint32_t> kSymbolToScanCode;

// Maps XKB scan codes to GTK key codes.
//
// The values are originally defined in:
// - flutter/keyboard_maps.dart (kLinuxToPhysicalKey)
// - flutter/keyboard_maps.dart (kGtkToLogicalKey)
extern std::map<uint32_t, uint32_t> kScanCodeToGtkKeyCode;

// Mapping from Ecore modifiers to GTK modifiers.
//
// The values are originally defined in:
// - efl/Ecore_Input.h
// - flutter/raw_keyboard_linux.dart (GtkKeyHelper)
extern std::map<int, int> kEcoreModifierToGtkModifier;

// Maps XKB scan codes to Flutter's physical key codes.
//
// This is a copy of the Linux embedder's |xkb_to_physical_key_map|.
extern std::map<uint64_t, uint64_t> kScanCodeToPhysicalKeyCode;

// Maps Tizen key symbols to Flutter's logical key codes.
extern std::map<std::string, uint64_t> kSymbolToLogicalKeyCode;

// Mask for the 32-bit value portion of the key code.
const uint64_t kValueMask = 0x000ffffffff;

// The plane value for keys which have a Unicode representation.
const uint64_t kUnicodePlane = 0x00000000000;

// The plane value for the private keys defined by the GTK embedding.
const uint64_t kGtkPlane = 0x01500000000;

#endif  // EMBEDDER_KEY_MAPPING_H_
