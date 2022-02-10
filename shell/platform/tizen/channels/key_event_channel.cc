// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "key_event_channel.h"

#include <codecvt>
#include <map>
#include <string>

#include "flutter/shell/platform/common/json_message_codec.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "flutter/keyevent";

constexpr char kKeyMapKey[] = "keymap";
constexpr char kKeyCodeKey[] = "keyCode";
constexpr char kScanCodeKey[] = "scanCode";
constexpr char kTypeKey[] = "type";
constexpr char kModifiersKey[] = "modifiers";
constexpr char kToolkitKey[] = "toolkit";
constexpr char kUnicodeScalarValuesKey[] = "unicodeScalarValues";
constexpr char kHandledKey[] = "handled";

constexpr char kKeyUp[] = "keyup";
constexpr char kKeyDown[] = "keydown";
constexpr char kGtkToolkit[] = "gtk";
constexpr char kLinuxKeyMap[] = "linux";

// Mapping from TV remote control key symbols to standard XKB scan codes.
//
// The values are originally defined in:
// - xkb-tizen-data/tizen_key_layout.txt.tv
// - flutter/keyboard_maps.dart (kLinuxToPhysicalKey)
const std::map<std::string, uint32_t> kSymbolToScanCode = {
    {"XF86AudioPlay", 0x000000d7},       // mediaPlay
    {"XF86AudioPlayPause", 0x000000ac},  // mediaPlayPause
    {"XF86Menu", 0x00000087},            // contextMenu
    {"XF86PlayBack", 0x000000ac},        // mediaPlayPause
    {"XF86SysMenu", 0x00000087},         // contextMenu
};

// Mapping from physical scan codes to logical (GTK) key codes.
//
// The values are originally defined in:
// - flutter/keyboard_maps.dart (kLinuxToPhysicalKey)
// - flutter/keyboard_maps.dart (kGtkToLogicalKey)
const std::map<uint32_t, uint32_t> kScanCodeToKeyCode = {
    {0x00000009, 65307},      // escape
    {0x0000000a, 49},         // digit1
    {0x0000000b, 50},         // digit2
    {0x0000000c, 51},         // digit3
    {0x0000000d, 52},         // digit4
    {0x0000000e, 53},         // digit5
    {0x0000000f, 54},         // digit6
    {0x00000010, 55},         // digit7
    {0x00000011, 56},         // digit8
    {0x00000012, 57},         // digit9
    {0x00000013, 48},         // digit0
    {0x00000014, 45},         // minus
    {0x00000015, 61},         // equal
    {0x00000016, 65288},      // backspace
    {0x00000017, 65289},      // tab
    {0x00000018, 81},         // keyQ
    {0x00000019, 87},         // keyW
    {0x0000001a, 69},         // keyE
    {0x0000001b, 82},         // keyR
    {0x0000001c, 84},         // keyT
    {0x0000001d, 89},         // keyY
    {0x0000001e, 85},         // keyU
    {0x0000001f, 73},         // keyI
    {0x00000020, 79},         // keyO
    {0x00000021, 80},         // keyP
    {0x00000022, 91},         // bracketLeft
    {0x00000023, 93},         // bracketRight
    {0x00000024, 65293},      // enter
    {0x00000025, 65507},      // controlLeft
    {0x00000026, 65},         // keyA
    {0x00000027, 83},         // keyS
    {0x00000028, 68},         // keyD
    {0x00000029, 70},         // keyF
    {0x0000002a, 71},         // keyG
    {0x0000002b, 72},         // keyH
    {0x0000002c, 74},         // keyJ
    {0x0000002d, 75},         // keyK
    {0x0000002e, 76},         // keyL
    {0x0000002f, 59},         // semicolon
    {0x00000030, 39},         // quote
    {0x00000031, 96},         // backquote
    {0x00000032, 65505},      // shiftLeft
    {0x00000033, 92},         // backslash
    {0x00000034, 90},         // keyZ
    {0x00000035, 88},         // keyX
    {0x00000036, 67},         // keyC
    {0x00000037, 86},         // keyV
    {0x00000038, 66},         // keyB
    {0x00000039, 78},         // keyN
    {0x0000003a, 77},         // keyM
    {0x0000003b, 44},         // comma
    {0x0000003c, 46},         // period
    {0x0000003d, 47},         // slash
    {0x0000003e, 65506},      // shiftRight
    {0x0000003f, 65450},      // numpadMultiply
    {0x00000040, 65513},      // altLeft
    {0x00000041, 32},         // space
    {0x00000042, 65509},      // capsLock
    {0x00000043, 65470},      // f1
    {0x00000044, 65471},      // f2
    {0x00000045, 65472},      // f3
    {0x00000046, 65473},      // f4
    {0x00000047, 65474},      // f5
    {0x00000048, 65475},      // f6
    {0x00000049, 65476},      // f7
    {0x0000004a, 65477},      // f8
    {0x0000004b, 65478},      // f9
    {0x0000004c, 65479},      // f10
    {0x0000004d, 65407},      // numLock
    {0x0000004e, 65300},      // scrollLock
    {0x0000004f, 65463},      // numpad7
    {0x00000050, 65464},      // numpad8
    {0x00000051, 65465},      // numpad9
    {0x00000052, 65453},      // numpadSubtract
    {0x00000053, 65460},      // numpad4
    {0x00000054, 65461},      // numpad5
    {0x00000055, 65462},      // numpad6
    {0x00000056, 65451},      // numpadAdd
    {0x00000057, 65457},      // numpad1
    {0x00000058, 65458},      // numpad2
    {0x00000059, 65459},      // numpad3
    {0x0000005a, 65456},      // numpad0
    {0x0000005b, 65454},      // numpadDecimal
    {0x0000005f, 65480},      // f11
    {0x00000060, 65481},      // f12
    {0x00000065, 65406},      // kanaMode
    {0x00000068, 65421},      // numpadEnter
    {0x00000069, 65508},      // controlRight
    {0x0000006a, 65455},      // numpadDivide
    {0x0000006b, 64797},      // printScreen
    {0x0000006c, 65514},      // altRight
    {0x0000006e, 65360},      // home
    {0x0000006f, 65362},      // arrowUp
    {0x00000070, 65365},      // pageUp
    {0x00000071, 65361},      // arrowLeft
    {0x00000072, 65363},      // arrowRight
    {0x00000073, 65367},      // end
    {0x00000074, 65364},      // arrowDown
    {0x00000075, 65366},      // pageDown
    {0x00000076, 65379},      // insert
    {0x00000077, 65535},      // delete
    {0x00000079, 269025042},  // audioVolumeMute
    {0x0000007a, 269025041},  // audioVolumeDown
    {0x0000007b, 269025043},  // audioVolumeUp
    {0x0000007c, 269025066},  // power
    {0x0000007d, 65469},      // numpadEqual
    {0x0000007f, 65299},      // pause
    {0x00000084, 165},        // intlYen
    {0x00000085, 65511},      // metaLeft
    {0x00000086, 65512},      // metaRight
    {0x00000087, 65383},      // contextMenu
    {0x00000088, 269025064},  // browserStop
    {0x0000008b, 65381},      // undo
    {0x0000008c, 65376},      // select
    {0x0000008d, 269025111},  // copy
    {0x0000008e, 269025131},  // open
    {0x0000008f, 269025133},  // paste
    {0x00000090, 65384},      // find
    {0x00000092, 65386},      // help
    {0x00000096, 269025071},  // sleep
    {0x00000097, 269025067},  // wakeUp
    {0x0000009e, 269025070},  // launchInternetBrowser
    {0x000000a3, 269025049},  // launchMail
    {0x000000a4, 269025072},  // browserFavorites
    {0x000000a6, 269025062},  // browserBack
    {0x000000a7, 269025063},  // browserForward
    {0x000000a9, 269025068},  // eject
    {0x000000ab, 269025047},  // mediaTrackNext
    {0x000000ad, 269025046},  // mediaTrackPrevious
    {0x000000ae, 269025045},  // mediaStop
    {0x000000af, 269025052},  // mediaRecord
    {0x000000b0, 269025086},  // mediaRewind
    {0x000000b1, 269025134},  // launchPhone
    {0x000000b4, 269025048},  // browserHome
    {0x000000b5, 269025065},  // browserRefresh
    {0x000000bd, 269025128},  // newKey
    {0x000000be, 65382},      // redo
    {0x000000bf, 65482},      // f13
    {0x000000c0, 65483},      // f14
    {0x000000c1, 65484},      // f15
    {0x000000c2, 65485},      // f16
    {0x000000c3, 65486},      // f17
    {0x000000c4, 65487},      // f18
    {0x000000c5, 65488},      // f19
    {0x000000c6, 65489},      // f20
    {0x000000c7, 65490},      // f21
    {0x000000c8, 65491},      // f22
    {0x000000c9, 65492},      // f23
    {0x000000ca, 65493},      // f24
    {0x000000d1, 269025073},  // mediaPause
    {0x000000d6, 269025110},  // close
    {0x000000d7, 269025044},  // mediaPlay
    {0x000000d8, 269025175},  // mediaFastForward
    {0x000000da, 65377},      // print
    {0x000000e1, 269025051},  // browserSearch
    {0x000000e8, 269025027},  // brightnessDown
    {0x000000e9, 269025026},  // brightnessUp
    {0x000000ed, 269025030},  // kbdIllumDown
    {0x000000ee, 269025029},  // kbdIllumUp
    {0x000000ef, 269025147},  // mailSend
    {0x000000f0, 269025138},  // mailReply
    {0x000000f1, 269025168},  // mailForward
    {0x000000f2, 269025143},  // save
    {0x00000190, 269025170},  // launchAudioBrowser
    {0x00000195, 269025056},  // launchCalendar
    {0x000001aa, 269025163},  // zoomIn
    {0x000001ab, 269025164},  // zoomOut
    {0x000001b8, 269025148},  // spellCheck
    {0x000001b9, 269025121},  // logOff
    {0x0000024d, 269025069},  // launchScreenSaver
};

// Mapping from Ecore modifiers to GTK modifiers.
//
// The values are originally defined in:
// - efl/Ecore_Input.h
// - flutter/raw_keyboard_linux.dart (GtkKeyHelper)
const std::map<int, int> kEcoreModifierToGtkModifier = {
    {0x0001, 1 << 0},   // SHIFT (modifierShift)
    {0x0002, 1 << 2},   // CTRL (modifierControl)
    {0x0004, 1 << 3},   // ALT (modifierMod1)
    {0x0008, 1 << 26},  // WIN (modifierMeta)
    {0x0010, 0},        // SCROLL (undefined)
    {0x0020, 1 << 4},   // NUM (modifierMod2)
    {0x0040, 1 << 1},   // CAPS (modifierCapsLock)
};

uint32_t Utf8ToUtf32CodePoint(const char* utf8) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
  for (auto wchar : converter.from_bytes(utf8)) {
    return wchar;
  }
  return 0;
}

}  // namespace

KeyEventChannel::KeyEventChannel(BinaryMessenger* messenger)
    : channel_(std::make_unique<BasicMessageChannel<rapidjson::Document>>(
          messenger,
          kChannelName,
          &JsonMessageCodec::GetInstance())) {}

KeyEventChannel::~KeyEventChannel() {}

void KeyEventChannel::SendKeyEvent(Ecore_Event_Key* key,
                                   bool is_down,
                                   std::function<void(bool)> callback) {
  uint32_t scan_code = key->keycode;
  auto iter1 = kSymbolToScanCode.find(key->key);
  if (iter1 != kSymbolToScanCode.end()) {
    scan_code = iter1->second;
  }

  uint32_t key_code = 0;
  auto iter2 = kScanCodeToKeyCode.find(scan_code);
  if (iter2 != kScanCodeToKeyCode.end()) {
    key_code = iter2->second;
  }

  int modifiers = 0;
  for (auto element : kEcoreModifierToGtkModifier) {
    if (element.first & key->modifiers) {
      modifiers |= element.second;
    }
  }

  uint32_t unicode_scalar_values = 0;
  if (key->compose) {
    unicode_scalar_values = Utf8ToUtf32CodePoint(key->compose);
  }

  rapidjson::Document event(rapidjson::kObjectType);
  auto& allocator = event.GetAllocator();
  event.AddMember(kKeyMapKey, kLinuxKeyMap, allocator);
  event.AddMember(kToolkitKey, kGtkToolkit, allocator);
  event.AddMember(kUnicodeScalarValuesKey, unicode_scalar_values, allocator);
  event.AddMember(kKeyCodeKey, key_code, allocator);
  event.AddMember(kScanCodeKey, scan_code, allocator);
  event.AddMember(kModifiersKey, modifiers, allocator);
  if (is_down) {
    event.AddMember(kTypeKey, kKeyDown, allocator);
  } else {
    event.AddMember(kTypeKey, kKeyUp, allocator);
  }
  channel_->Send(event, [callback = std::move(callback)](const uint8_t* reply,
                                                         size_t reply_size) {
    if (reply != nullptr) {
      auto decoded =
          JsonMessageCodec::GetInstance().DecodeMessage(reply, reply_size);
      bool handled = (*decoded)[kHandledKey].GetBool();
      callback(handled);
    }
  });
}

}  // namespace flutter
