// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "key_event_channel.h"

#include <map>

#include "flutter/shell/platform/tizen/tizen_log.h"

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

// Mapping from physical (xkb) to logical (GTK) key codes.
// The values are defined in:
// - flutter/keyboard_maps.dart (kLinuxToPhysicalKey, kGtkToLogicalKey)
const std::map<int, int> kKeyCodeMap = {
    {0x00000009, 65307},      // LogicalKeyboardKey.escape
    {0x0000000a, 49},         // LogicalKeyboardKey.digit1
    {0x0000000b, 50},         // LogicalKeyboardKey.digit2
    {0x0000000c, 51},         // LogicalKeyboardKey.digit3
    {0x0000000d, 52},         // LogicalKeyboardKey.digit4
    {0x0000000e, 53},         // LogicalKeyboardKey.digit5
    {0x0000000f, 54},         // LogicalKeyboardKey.digit6
    {0x00000010, 55},         // LogicalKeyboardKey.digit7
    {0x00000011, 56},         // LogicalKeyboardKey.digit8
    {0x00000012, 57},         // LogicalKeyboardKey.digit9
    {0x00000013, 48},         // LogicalKeyboardKey.digit0
    {0x00000014, 45},         // LogicalKeyboardKey.minus
    {0x00000015, 61},         // LogicalKeyboardKey.equal
    {0x00000016, 65288},      // LogicalKeyboardKey.backspace
    {0x00000017, 65289},      // LogicalKeyboardKey.tab
    {0x00000018, 81},         // LogicalKeyboardKey.keyQ
    {0x00000019, 87},         // LogicalKeyboardKey.keyW
    {0x0000001a, 69},         // LogicalKeyboardKey.keyE
    {0x0000001b, 82},         // LogicalKeyboardKey.keyR
    {0x0000001c, 84},         // LogicalKeyboardKey.keyT
    {0x0000001d, 89},         // LogicalKeyboardKey.keyY
    {0x0000001e, 85},         // LogicalKeyboardKey.keyU
    {0x0000001f, 73},         // LogicalKeyboardKey.keyI
    {0x00000020, 79},         // LogicalKeyboardKey.keyO
    {0x00000021, 80},         // LogicalKeyboardKey.keyP
    {0x00000022, 91},         // LogicalKeyboardKey.bracketLeft
    {0x00000023, 93},         // LogicalKeyboardKey.bracketRight
    {0x00000024, 65293},      // LogicalKeyboardKey.enter
    {0x00000025, 65507},      // LogicalKeyboardKey.controlLeft
    {0x00000026, 65},         // LogicalKeyboardKey.keyA
    {0x00000027, 83},         // LogicalKeyboardKey.keyS
    {0x00000028, 68},         // LogicalKeyboardKey.keyD
    {0x00000029, 70},         // LogicalKeyboardKey.keyF
    {0x0000002a, 71},         // LogicalKeyboardKey.keyG
    {0x0000002b, 72},         // LogicalKeyboardKey.keyH
    {0x0000002c, 74},         // LogicalKeyboardKey.keyJ
    {0x0000002d, 75},         // LogicalKeyboardKey.keyK
    {0x0000002e, 76},         // LogicalKeyboardKey.keyL
    {0x0000002f, 59},         // LogicalKeyboardKey.semicolon
    {0x00000030, 39},         // LogicalKeyboardKey.quote
    {0x00000031, 96},         // LogicalKeyboardKey.backquote
    {0x00000032, 65505},      // LogicalKeyboardKey.shiftLeft
    {0x00000033, 92},         // LogicalKeyboardKey.backslash
    {0x00000034, 90},         // LogicalKeyboardKey.keyZ
    {0x00000035, 88},         // LogicalKeyboardKey.keyX
    {0x00000036, 67},         // LogicalKeyboardKey.keyC
    {0x00000037, 86},         // LogicalKeyboardKey.keyV
    {0x00000038, 66},         // LogicalKeyboardKey.keyB
    {0x00000039, 78},         // LogicalKeyboardKey.keyN
    {0x0000003a, 77},         // LogicalKeyboardKey.keyM
    {0x0000003b, 44},         // LogicalKeyboardKey.comma
    {0x0000003c, 46},         // LogicalKeyboardKey.period
    {0x0000003d, 47},         // LogicalKeyboardKey.slash
    {0x0000003e, 65506},      // LogicalKeyboardKey.shiftRight
    {0x0000003f, 65450},      // LogicalKeyboardKey.numpadMultiply
    {0x00000040, 65513},      // LogicalKeyboardKey.altLeft
    {0x00000041, 32},         // LogicalKeyboardKey.space
    {0x00000042, 65509},      // LogicalKeyboardKey.capsLock
    {0x00000043, 65470},      // LogicalKeyboardKey.f1
    {0x00000044, 65471},      // LogicalKeyboardKey.f2
    {0x00000045, 65472},      // LogicalKeyboardKey.f3
    {0x00000046, 65473},      // LogicalKeyboardKey.f4
    {0x00000047, 65474},      // LogicalKeyboardKey.f5
    {0x00000048, 65475},      // LogicalKeyboardKey.f6
    {0x00000049, 65476},      // LogicalKeyboardKey.f7
    {0x0000004a, 65477},      // LogicalKeyboardKey.f8
    {0x0000004b, 65478},      // LogicalKeyboardKey.f9
    {0x0000004c, 65479},      // LogicalKeyboardKey.f10
    {0x0000004d, 65407},      // LogicalKeyboardKey.numLock
    {0x0000004e, 65300},      // LogicalKeyboardKey.scrollLock
    {0x0000004f, 65463},      // LogicalKeyboardKey.numpad7
    {0x00000050, 65464},      // LogicalKeyboardKey.numpad8
    {0x00000051, 65465},      // LogicalKeyboardKey.numpad9
    {0x00000052, 65453},      // LogicalKeyboardKey.numpadSubtract
    {0x00000053, 65460},      // LogicalKeyboardKey.numpad4
    {0x00000054, 65461},      // LogicalKeyboardKey.numpad5
    {0x00000055, 65462},      // LogicalKeyboardKey.numpad6
    {0x00000056, 65451},      // LogicalKeyboardKey.numpadAdd
    {0x00000057, 65457},      // LogicalKeyboardKey.numpad1
    {0x00000058, 65458},      // LogicalKeyboardKey.numpad2
    {0x00000059, 65459},      // LogicalKeyboardKey.numpad3
    {0x0000005a, 65456},      // LogicalKeyboardKey.numpad0
    {0x0000005b, 65454},      // LogicalKeyboardKey.numpadDecimal
    {0x0000005f, 65480},      // LogicalKeyboardKey.f11
    {0x00000060, 65481},      // LogicalKeyboardKey.f12
    {0x00000065, 65406},      // LogicalKeyboardKey.kanaMode
    {0x00000068, 65421},      // LogicalKeyboardKey.numpadEnter
    {0x00000069, 65508},      // LogicalKeyboardKey.controlRight
    {0x0000006a, 65455},      // LogicalKeyboardKey.numpadDivide
    {0x0000006b, 64797},      // LogicalKeyboardKey.printScreen
    {0x0000006c, 65514},      // LogicalKeyboardKey.altRight
    {0x0000006e, 65360},      // LogicalKeyboardKey.home
    {0x0000006f, 65362},      // LogicalKeyboardKey.arrowUp
    {0x00000070, 65365},      // LogicalKeyboardKey.pageUp
    {0x00000071, 65361},      // LogicalKeyboardKey.arrowLeft
    {0x00000072, 65363},      // LogicalKeyboardKey.arrowRight
    {0x00000073, 65367},      // LogicalKeyboardKey.end
    {0x00000074, 65364},      // LogicalKeyboardKey.arrowDown
    {0x00000075, 65366},      // LogicalKeyboardKey.pageDown
    {0x00000076, 65379},      // LogicalKeyboardKey.insert
    {0x00000077, 65535},      // LogicalKeyboardKey.delete
    {0x00000079, 269025042},  // LogicalKeyboardKey.audioVolumeMute
    {0x0000007a, 269025041},  // LogicalKeyboardKey.audioVolumeDown
    {0x0000007b, 269025043},  // LogicalKeyboardKey.audioVolumeUp
    {0x0000007c, 269025066},  // LogicalKeyboardKey.power
    {0x0000007d, 65469},      // LogicalKeyboardKey.numpadEqual
    {0x0000007f, 65299},      // LogicalKeyboardKey.pause
    {0x00000084, 165},        // LogicalKeyboardKey.intlYen
    {0x00000085, 65511},      // LogicalKeyboardKey.metaLeft
    {0x00000086, 65512},      // LogicalKeyboardKey.metaRight
    {0x00000087, 65383},      // LogicalKeyboardKey.contextMenu
    {0x00000088, 269025064},  // LogicalKeyboardKey.browserStop
    {0x0000008b, 65381},      // LogicalKeyboardKey.undo
    {0x0000008c, 65376},      // LogicalKeyboardKey.select
    {0x0000008d, 269025111},  // LogicalKeyboardKey.copy
    {0x0000008e, 269025131},  // LogicalKeyboardKey.open
    {0x0000008f, 269025133},  // LogicalKeyboardKey.paste
    {0x00000090, 65384},      // LogicalKeyboardKey.find
    {0x00000092, 65386},      // LogicalKeyboardKey.help
    {0x00000096, 269025071},  // LogicalKeyboardKey.sleep
    {0x00000097, 269025067},  // LogicalKeyboardKey.wakeUp
    {0x0000009e, 269025070},  // LogicalKeyboardKey.launchInternetBrowser
    {0x000000a3, 269025049},  // LogicalKeyboardKey.launchMail
    {0x000000a4, 269025072},  // LogicalKeyboardKey.browserFavorites
    {0x000000a6, 269025062},  // LogicalKeyboardKey.browserBack
    {0x000000a7, 269025063},  // LogicalKeyboardKey.browserForward
    {0x000000a9, 269025068},  // LogicalKeyboardKey.eject
    {0x000000ab, 269025047},  // LogicalKeyboardKey.mediaTrackNext
    {0x000000ad, 269025046},  // LogicalKeyboardKey.mediaTrackPrevious
    {0x000000ae, 269025045},  // LogicalKeyboardKey.mediaStop
    {0x000000af, 269025052},  // LogicalKeyboardKey.mediaRecord
    {0x000000b0, 269025086},  // LogicalKeyboardKey.mediaRewind
    {0x000000b1, 269025134},  // LogicalKeyboardKey.launchPhone
    {0x000000b4, 269025048},  // LogicalKeyboardKey.browserHome
    {0x000000b5, 269025065},  // LogicalKeyboardKey.browserRefresh
    {0x000000bd, 269025128},  // LogicalKeyboardKey.newKey
    {0x000000be, 65382},      // LogicalKeyboardKey.redo
    {0x000000bf, 65482},      // LogicalKeyboardKey.f13
    {0x000000c0, 65483},      // LogicalKeyboardKey.f14
    {0x000000c1, 65484},      // LogicalKeyboardKey.f15
    {0x000000c2, 65485},      // LogicalKeyboardKey.f16
    {0x000000c3, 65486},      // LogicalKeyboardKey.f17
    {0x000000c4, 65487},      // LogicalKeyboardKey.f18
    {0x000000c5, 65488},      // LogicalKeyboardKey.f19
    {0x000000c6, 65489},      // LogicalKeyboardKey.f20
    {0x000000c7, 65490},      // LogicalKeyboardKey.f21
    {0x000000c8, 65491},      // LogicalKeyboardKey.f22
    {0x000000c9, 65492},      // LogicalKeyboardKey.f23
    {0x000000ca, 65493},      // LogicalKeyboardKey.f24
    {0x000000d1, 269025073},  // LogicalKeyboardKey.mediaPause
    {0x000000d6, 269025110},  // LogicalKeyboardKey.close
    {0x000000d7, 269025044},  // LogicalKeyboardKey.mediaPlay
    {0x000000d8, 269025175},  // LogicalKeyboardKey.mediaFastForward
    {0x000000da, 65377},      // LogicalKeyboardKey.print
    {0x000000e1, 269025051},  // LogicalKeyboardKey.browserSearch
    {0x000000e8, 269025027},  // LogicalKeyboardKey.brightnessDown
    {0x000000e9, 269025026},  // LogicalKeyboardKey.brightnessUp
    {0x000000ed, 269025030},  // LogicalKeyboardKey.kbdIllumDown
    {0x000000ee, 269025029},  // LogicalKeyboardKey.kbdIllumUp
    {0x000000ef, 269025147},  // LogicalKeyboardKey.mailSend
    {0x000000f0, 269025138},  // LogicalKeyboardKey.mailReply
    {0x000000f1, 269025168},  // LogicalKeyboardKey.mailForward
    {0x000000f2, 269025143},  // LogicalKeyboardKey.save
    {0x00000190, 269025170},  // LogicalKeyboardKey.launchAudioBrowser
    {0x00000195, 269025056},  // LogicalKeyboardKey.launchCalendar
    {0x000001aa, 269025163},  // LogicalKeyboardKey.zoomIn
    {0x000001ab, 269025164},  // LogicalKeyboardKey.zoomOut
    {0x000001b8, 269025148},  // LogicalKeyboardKey.spellCheck
    {0x000001b9, 269025121},  // LogicalKeyboardKey.logOff
    {0x0000024d, 269025069},  // LogicalKeyboardKey.launchScreenSaver
};

// The values are defined in:
// - efl/Ecore_Input.h
// - flutter/raw_keyboard_linux.dart (GtkKeyHelper)
const std::map<int, int> kModifierMap = {
    {0x0001, 1 << 0},   // SHIFT (modifierShift)
    {0x0002, 1 << 2},   // CTRL (modifierControl)
    {0x0004, 1 << 3},   // ALT (modifierMod1)
    {0x0008, 1 << 28},  // WIN (modifierMeta)
    {0x0010, 0},        // SCROLL (undefined)
    {0x0020, 1 << 4},   // NUM (modifierMod2)
    {0x0040, 1 << 1},   // CAPS (modifierCapsLock)
};

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
  int gtk_keycode = 0;
  if (kKeyCodeMap.count(key->keycode) > 0) {
    gtk_keycode = kKeyCodeMap.at(key->keycode);
  }
  int gtk_modifiers = 0;
  for (auto element : kModifierMap) {
    if (element.first & key->modifiers) {
      gtk_modifiers |= element.second;
    }
  }

  rapidjson::Document event(rapidjson::kObjectType);
  auto& allocator = event.GetAllocator();
  event.AddMember(kKeyMapKey, kLinuxKeyMap, allocator);
  event.AddMember(kToolkitKey, kGtkToolkit, allocator);
  event.AddMember(kUnicodeScalarValuesKey, 0, allocator);
  event.AddMember(kKeyCodeKey, gtk_keycode, allocator);
  event.AddMember(kScanCodeKey, key->keycode, allocator);
  event.AddMember(kModifiersKey, gtk_modifiers, allocator);
  if (is_down) {
    event.AddMember(kTypeKey, kKeyDown, allocator);
  } else {
    event.AddMember(kTypeKey, kKeyUp, allocator);
  }
  channel_->Send(event, [callback = std::move(callback)](const uint8_t* reply,
                                                         size_t reply_size) {
    if (reply != nullptr) {
      auto decoded = flutter::JsonMessageCodec::GetInstance().DecodeMessage(
          reply, reply_size);
      bool handled = (*decoded)[kHandledKey].GetBool();
      callback(handled);
    }
  });
}

}  // namespace flutter
