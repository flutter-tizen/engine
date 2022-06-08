// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef KEYBOARD_MAP_H_
#define KEYBOARD_MAP_H_

#include <cinttypes>
#include <map>

// Maps XKB specific key code values to Flutter's physical key code values.
extern std::map<uint64_t, uint64_t> xkb_to_physical_key_map;

// Maps GDK keyval values to Flutter's logical key code values.
extern std::map<uint64_t, uint64_t> gtk_keyval_to_logical_key_map;

// void initialize_modifier_bit_to_checked_keys(GHashTable* table);

// void initialize_lock_bit_to_checked_keys(GHashTable* table);

// Mask for the 32-bit value portion of the key code.
extern const uint64_t kValueMask;

// The plane value for keys which have a Unicode representation.
extern const uint64_t kUnicodePlane;

// The plane value for the private keys defined by the GTK embedding.
extern const uint64_t kGtkPlane;

#endif  // KEYBOARD_MAP_H_
