// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_PLATFORM_VIEW_CHANNEL_H_
#define EMBEDDER_PLATFORM_VIEW_CHANNEL_H_

#include <Ecore_Input.h>

#include <map>

#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/method_channel.h"
#include "rapidjson/document.h"

class TizenEmbedderEngine;
class PlatformView;
class PlatformViewFactory;
class PlatformViewChannel {
 public:
  explicit PlatformViewChannel(flutter::BinaryMessenger* messenger,
                               TizenEmbedderEngine* engine);
  virtual ~PlatformViewChannel();
  void Dispose();
  std::map<std::string, std::unique_ptr<PlatformViewFactory>>& ViewFactories() {
    return view_factories_;
  }
  std::map<int, PlatformView*>& ViewInstances() { return view_instances_; }

  void SendKeyEvent(Ecore_Event_Key* key, bool is_down);
  int CurrentFocusedViewId();

 private:
  TizenEmbedderEngine* engine_;
  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel_;
  std::map<std::string, std::unique_ptr<PlatformViewFactory>> view_factories_;
  std::map<int, PlatformView*> view_instances_;

  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

#endif  //  EMBEDDER_PLATFORM_VIEW_CHANNEL_H_
