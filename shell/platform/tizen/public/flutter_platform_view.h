// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_TIZEN_PUBLIC_FLUTTER_PLATFORM_VIEW_H_
#define FLUTTER_SHELL_PLATFORM_TIZEN_PUBLIC_FLUTTER_PLATFORM_VIEW_H_

#include <Ecore_IMF.h>
#include <Ecore_Input.h>
#include <stddef.h>
#include <stdint.h>

#include "flutter_export.h"

using ByteMessage = std::vector<uint8_t>;

class PlatformView {
 public:
  PlatformView(flutter::PluginRegistrar* registrar, int viewId)
      : registrar_(registrar),
        viewId_(viewId),
        textureId_(0),
        isFocused_(false) {}
  virtual ~PlatformView() {}
  int GetViewId() { return viewId_; }
  int GetTextureId() { return textureId_; }
  void SetTextureId(int textureId) { textureId_ = textureId; }
  flutter::PluginRegistrar* GetPluginRegistrar() { return registrar_; }
  virtual void Dispose() = 0;
  virtual void Resize(double width, double height) = 0;
  virtual void Touch(int type, int button, double x, double y, double dx,
                     double dy) = 0;
  virtual void SetDirection(int direction) = 0;
  virtual void ClearFocus() = 0;
  void SetFocus(bool f) { isFocused_ = f; }
  bool IsFocused() { return isFocused_; }

  // Key input event
  virtual void DispatchKeyDownEvent(Ecore_Event_Key* key) = 0;
  virtual void DispatchKeyUpEvent(Ecore_Event_Key* key) = 0;

  virtual void SetSoftwareKeyboardContext(Ecore_IMF_Context* context) = 0;

 private:
  flutter::PluginRegistrar* registrar_;
  int viewId_;
  int textureId_;
  bool isFocused_;
};

class PlatformViewFactory {
 public:
  PlatformViewFactory(flutter::PluginRegistrar* registrar)
      : registrar_(registrar),
        codec_(flutter::StandardMessageCodec::GetInstance(nullptr)) {}
  virtual ~PlatformViewFactory() {}
  flutter::PluginRegistrar* GetPluginRegistrar() { return registrar_; }
  const flutter::MessageCodec<flutter::EncodableValue>& GetCodec() {
    return codec_;
  }
  virtual PlatformView* Create(int viewId, double width, double height,
                               const ByteMessage& createParams) = 0;
  virtual void Dispose() = 0;

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