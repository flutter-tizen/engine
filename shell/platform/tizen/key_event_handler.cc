// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "key_event_handler.h"

#include <iomanip>

#include "flutter/shell/platform/tizen/key_mapping.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

static uint64_t apply_id_plane(uint64_t logical_id, uint64_t plane) {
  return (logical_id & kValueMask) | plane;
}

static uint64_t event_to_physical_key(int scan_code) {
  auto found = xkb_to_physical_key_map.find(scan_code);
  if (found != xkb_to_physical_key_map.end()) {
    return found->second;
  }
  return apply_id_plane(scan_code, kGtkPlane);
}

const std::map<std::string, uint64_t> kKeySymbolToLogicalKeyCode = {
    {"XF86Back", 0x0010000001b},    // escape
    {"1", 0x00000000031},           // digit1
    {"2", 0x00000000032},           // digit2
    {"3", 0x00000000033},           // digit3
    {"4", 0x00000000034},           // digit4
    {"exclam", 0x00000000021},      // digit1
    {"at", 0x00000000040},          // digit2
    {"numbersign", 0x00000000023},  // digit3
    {"dollar", 0x00000000024},      // digit4
    {"Shift_L", 0x00200000102},     // shiftLeft
};

static uint64_t event_to_logical_key(const std::string& key) {
  // if (key == nullptr) {
  //   return 0;
  // }
  auto found = kKeySymbolToLogicalKeyCode.find(key);
  if (found != kKeySymbolToLogicalKeyCode.end()) {
    return found->second;
  }
  // if (key[1] == '\n') {
  //   // EASCII range
  //   if ((int32_t)key[0] < 256) {
  //     return apply_id_plane(key[0], kUnicodePlane);
  //   }
  //   // Auto-generate key
  //   return apply_id_plane(key[0], kGtkPlane);
  // }
  return apply_id_plane(0, kGtkPlane);
}

KeyEventHandler::KeyEventHandler(SendEventHandler send_event)
    : send_event_(send_event) {}

void KeyEventHandler::SendKey(const char* key,
                              const char* string,
                              const char* compose,
                              uint32_t modifiers,
                              uint32_t scan_code,
                              bool is_down,
                              std::function<void(bool)> callback) {
  FlutterKeyEventType type =
      is_down ? kFlutterKeyEventTypeDown : kFlutterKeyEventTypeUp;
  uint64_t physical_key = event_to_physical_key(scan_code);
  uint64_t logical_key = event_to_logical_key(key);
  // TODO: Set to nullptr when appropriate.
  const char* character = is_down ? key : nullptr;

  // TODO: For debugging
  if (is_down) {
    FT_LOG(Error) << "Physical: 0x" << std::setw(16) << std::setfill('0')
                  << std::right << std::hex << physical_key;
    FT_LOG(Error) << "Logical: 0x" << std::setw(16) << std::setfill('0')
                  << std::right << std::hex << logical_key;
  }

  FlutterKeyEvent key_data{
      .struct_size = sizeof(FlutterKeyEvent),
      .timestamp = static_cast<double>(
          std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::high_resolution_clock::now().time_since_epoch())
              .count()),
      .type = type,
      .physical = physical_key,
      .logical = logical_key,
      .character = character,
      .synthesized = false,
  };

  uint64_t response_id = response_id_++;
  PendingResponse pending{
      .callback =
          [this, callback = std::move(callback)](bool handled,
                                                 uint64_t response_id) {
            auto found = pending_responses_.find(response_id);
            if (found != pending_responses_.end()) {
              pending_responses_.erase(found);
            }
            callback(handled);
          },
      .response_id = response_id,
  };
  pending_responses_[response_id] = std::make_unique<PendingResponse>(pending);

  send_event_(
      key_data,
      [](bool handled, void* user_data) {
        auto* pending = reinterpret_cast<PendingResponse*>(user_data);
        pending->callback(handled, pending->response_id);
      },
      pending_responses_[response_id].get());
}

}  // namespace flutter
