// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_tizen_accessibility_bridge_delegate.h"

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/flutter_tizen_platform_node_delegate.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

FlutterTizenAccessibilityBridgeDelegate::
    FlutterTizenAccessibilityBridgeDelegate(FlutterTizenEngine* engine)
    : engine_(engine) {}

void FlutterTizenAccessibilityBridgeDelegate::OnAccessibilityEvent(
    ui::AXEventGenerator::TargetedEvent targeted_event) {
  auto bridge = engine_->GetAccessibilityBridge().lock();
  if (!bridge) {
    FT_LOG(Error) << "Accessibility bridge is deallocated";
    return;
  }

  if (!targeted_event.node) {
    FT_LOG(Error) << "AXNode of targeted event is null";
    return;
  }
  auto platform_node_delegate =
      bridge->GetFlutterPlatformNodeDelegateFromID(targeted_event.node->id())
          .lock();
  if (!platform_node_delegate) {
    FT_LOG(Error) << "Platform node delegate is deallocated";
    return;
  }
  auto tizen_platform_node_delegate =
      std::static_pointer_cast<FlutterTizenPlatformNodeDelegate>(
          platform_node_delegate);
  tizen_platform_node_delegate->NotifyAccessibilityEvent(
      targeted_event.event_params.event);
}

void FlutterTizenAccessibilityBridgeDelegate::DispatchAccessibilityAction(
    AccessibilityNodeId target,
    FlutterSemanticsAction action,
    fml::MallocMapping data) {
  if (engine_) {
    engine_->DispatchAccessibilityAction(target, action, std::move(data));
  }
}

std::shared_ptr<FlutterPlatformNodeDelegate>
FlutterTizenAccessibilityBridgeDelegate::CreateFlutterPlatformNodeDelegate() {
  return std::make_shared<FlutterTizenPlatformNodeDelegate>();
}

}  // namespace flutter
