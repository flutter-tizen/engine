// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_TIZEN_PUBLIC_FLUTTER_PLATFORM_VIEW_H_
#define FLUTTER_SHELL_PLATFORM_TIZEN_PUBLIC_FLUTTER_PLATFORM_VIEW_H_

#include <stddef.h>
#include <stdint.h>

#include "flutter_export.h"

using ByteMessage = std::vector<uint8_t>;

class PlatformView {
 public:
  PlatformView(flutter::PluginRegistrar* registrar, int viewId)
      : registrar_(registrar), viewId_(viewId), textureId_(0) {}
  virtual ~PlatformView() {}
  int getViewId() { return viewId_; }
  int getTextureId() { return textureId_; }
  void setTextureId(int textureId) { textureId_ = textureId; }
  flutter::PluginRegistrar* getPluginRegistrar() { return registrar_; }
  virtual void dispose() = 0;
  virtual void resize(double width, double height) = 0;
  virtual void touch() = 0;
  virtual void setDirection(int direction) = 0;
  virtual void clearFocus() = 0;

 private:
  flutter::PluginRegistrar* registrar_;
  int viewId_;
  int textureId_;
};

class PlatformViewFactory {
 public:
  PlatformViewFactory(flutter::PluginRegistrar* registrar)
      : registrar_(registrar),
        codec_(flutter::StandardMessageCodec::GetInstance(nullptr)) {}
  virtual ~PlatformViewFactory() {}
  flutter::PluginRegistrar* getPluginRegistrar() { return registrar_; }
  const flutter::MessageCodec<flutter::EncodableValue>& getCodec() {
    return codec_;
  }
  virtual PlatformView* create(int viewId, double width, double height,
                               const ByteMessage& createParams) = 0;
  virtual void dispose() = 0;

 private:
  flutter::PluginRegistrar* registrar_;
  const flutter::MessageCodec<flutter::EncodableValue>& codec_;
};

#if defined(__cplusplus)
extern "C" {
#endif

FLUTTER_EXPORT void FlutterRegisterViewFactory(
    FlutterDesktopPluginRegistrarRef registrar, const char* view_type,
    std::unique_ptr<PlatformViewFactory> view_factory);

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // FLUTTER_SHELL_PLATFORM_TIZEN_PUBLIC_FLUTTER_PLATFORM_VIEW_H_