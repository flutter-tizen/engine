// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_FLUTTER_TIZEN_ACCESSIBILITY_BRIDGE_DELEGATE_H_
#define EMBEDDER_FLUTTER_TIZEN_ACCESSIBILITY_BRIDGE_DELEGATE_H_

#include "flutter/shell/platform/common/accessibility_bridge.h"

namespace flutter {

class FlutterTizenEngine;

// The tizen implementation of AccessibilityBridge::AccessibilityBridgeDelegate.
// This delegate is used to create AccessibilityBridge in the tizen platform.
class FlutterTizenAccessibilityBridgeDelegate
    : public AccessibilityBridge::AccessibilityBridgeDelegate {
 public:
  explicit FlutterTizenAccessibilityBridgeDelegate(FlutterTizenEngine* engine);
  virtual ~FlutterTizenAccessibilityBridgeDelegate() = default;

  // |AccessibilityBridge::AccessibilityBridgeDelegate|
  void OnAccessibilityEvent(
      ui::AXEventGenerator::TargetedEvent targeted_event) override;

  // |AccessibilityBridge::AccessibilityBridgeDelegate|
  void DispatchAccessibilityAction(AccessibilityNodeId target,
                                   FlutterSemanticsAction action,
                                   fml::MallocMapping data) override;

  // |AccessibilityBridge::AccessibilityBridgeDelegate|
  std::shared_ptr<FlutterPlatformNodeDelegate>
  CreateFlutterPlatformNodeDelegate() override;

 private:
  FlutterTizenEngine* engine_;
};

}  // namespace flutter

#endif  // EMBEDDER_FLUTTER_TIZEN_ACCESSIBILITY_BRIDGE_DELEGATE_H_
