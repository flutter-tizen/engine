// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/flutter_tizen.h"

#include "flutter/shell/platform/common/client_wrapper/include/flutter/plugin_registrar.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_message_codec.h"
#include "flutter/shell/platform/common/incoming_message_dispatcher.h"
#include "flutter/shell/platform/tizen/flutter_project_bundle.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/flutter_tizen_view.h"
#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/public/flutter_platform_view.h"
#include "flutter/shell/platform/tizen/tizen_window.h"

namespace {

// Returns the engine corresponding to the given opaque API handle.
flutter::FlutterTizenEngine* EngineFromHandle(FlutterDesktopEngineRef ref) {
  return reinterpret_cast<flutter::FlutterTizenEngine*>(ref);
}

// Returns the opaque API handle for the given engine instance.
FlutterDesktopEngineRef HandleForEngine(flutter::FlutterTizenEngine* engine) {
  return reinterpret_cast<FlutterDesktopEngineRef>(engine);
}

// Returns the view corresponding to the given opaque API handle.
flutter::FlutterTizenView* ViewFromHandle(FlutterDesktopViewRef view) {
  return reinterpret_cast<flutter::FlutterTizenView*>(view);
}

// Returns the texture registrar corresponding to the given opaque API handle.
flutter::FlutterTizenTextureRegistrar* TextureRegistrarFromHandle(
    FlutterDesktopTextureRegistrarRef ref) {
  return reinterpret_cast<flutter::FlutterTizenTextureRegistrar*>(ref);
}

// Returns the opaque API handle for the given texture registrar instance.
FlutterDesktopTextureRegistrarRef HandleForTextureRegistrar(
    flutter::FlutterTizenTextureRegistrar* registrar) {
  return reinterpret_cast<FlutterDesktopTextureRegistrarRef>(registrar);
}

}  // namespace

FlutterDesktopEngineRef FlutterDesktopEngineCreate(
    const FlutterDesktopEngineProperties& engine_properties) {
  flutter::FlutterProjectBundle project(engine_properties);
  if (project.HasArgument("--verbose-logging")) {
    flutter::Logger::SetLoggingLevel(flutter::kLogLevelDebug);
  }
  std::string logging_port;
  if (project.GetArgumentValue("--tizen-logging-port", &logging_port)) {
    flutter::Logger::SetLoggingPort(std::stoi(logging_port));
  }
  flutter::Logger::Start();

  auto engine = std::make_unique<flutter::FlutterTizenEngine>(project);
  return HandleForEngine(engine.release());
}

bool FlutterDesktopEngineRun(const FlutterDesktopEngineRef engine) {
  return EngineFromHandle(engine)->RunEngine();
}

void FlutterDesktopEngineShutdown(FlutterDesktopEngineRef engine_ref) {
  flutter::Logger::Stop();

  flutter::FlutterTizenEngine* engine = EngineFromHandle(engine_ref);
  engine->StopEngine();
  delete engine;
}

void* FlutterDesktopPluginRegistrarGetNativeWindow(
    FlutterDesktopPluginRegistrarRef registrar) {
  flutter::TizenViewBase* tizen_view = registrar->engine->view()->tizen_view();
  if (tizen_view->GetType() == flutter::TizenViewType::kWindow) {
    auto* window = reinterpret_cast<flutter::TizenWindow*>(tizen_view);
    return window->GetWindowHandle();
  }
  return nullptr;
}

void FlutterDesktopPluginRegistrarEnableInputBlocking(
    FlutterDesktopPluginRegistrarRef registrar,
    const char* channel) {
  registrar->engine->message_dispatcher()->EnableInputBlockingForChannel(
      channel);
}

FlutterDesktopPluginRegistrarRef FlutterDesktopEngineGetPluginRegistrar(
    FlutterDesktopEngineRef engine,
    const char* plugin_name) {
  // Currently, one registrar acts as the registrar for all plugins, so the
  // name is ignored. It is part of the API to reduce churn in the future when
  // aligning more closely with the Flutter registrar system.
  return EngineFromHandle(engine)->plugin_registrar();
}

FlutterDesktopMessengerRef FlutterDesktopEngineGetMessenger(
    FlutterDesktopEngineRef engine) {
  return EngineFromHandle(engine)->messenger();
}

FlutterDesktopMessengerRef FlutterDesktopPluginRegistrarGetMessenger(
    FlutterDesktopPluginRegistrarRef registrar) {
  return registrar->engine->messenger();
}

void FlutterDesktopPluginRegistrarSetDestructionHandler(
    FlutterDesktopPluginRegistrarRef registrar,
    FlutterDesktopOnPluginRegistrarDestroyed callback) {
  registrar->engine->SetPluginRegistrarDestructionCallback(callback);
}

bool FlutterDesktopMessengerSend(FlutterDesktopMessengerRef messenger,
                                 const char* channel,
                                 const uint8_t* message,
                                 const size_t message_size) {
  return FlutterDesktopMessengerSendWithReply(messenger, channel, message,
                                              message_size, nullptr, nullptr);
}

bool FlutterDesktopMessengerSendWithReply(FlutterDesktopMessengerRef messenger,
                                          const char* channel,
                                          const uint8_t* message,
                                          const size_t message_size,
                                          const FlutterDesktopBinaryReply reply,
                                          void* user_data) {
  return messenger->engine->SendPlatformMessage(channel, message, message_size,
                                                reply, user_data);
}

void FlutterDesktopMessengerSendResponse(
    FlutterDesktopMessengerRef messenger,
    const FlutterDesktopMessageResponseHandle* handle,
    const uint8_t* data,
    size_t data_length) {
  messenger->engine->SendPlatformMessageResponse(handle, data, data_length);
}

void FlutterDesktopMessengerSetCallback(FlutterDesktopMessengerRef messenger,
                                        const char* channel,
                                        FlutterDesktopMessageCallback callback,
                                        void* user_data) {
  messenger->engine->message_dispatcher()->SetMessageCallback(channel, callback,
                                                              user_data);
}

void FlutterDesktopEngineNotifyAppControl(FlutterDesktopEngineRef engine,
                                          void* app_control) {
  EngineFromHandle(engine)->app_control_channel()->NotifyAppControl(
      app_control);
}

void FlutterDesktopEngineNotifyLocaleChange(FlutterDesktopEngineRef engine) {
  EngineFromHandle(engine)->SetupLocales();
}

void FlutterDesktopEngineNotifyLowMemoryWarning(
    FlutterDesktopEngineRef engine) {
  EngineFromHandle(engine)->NotifyLowMemoryWarning();
}

void FlutterDesktopEngineNotifyAppIsInactive(FlutterDesktopEngineRef engine) {
  EngineFromHandle(engine)->lifecycle_channel()->AppIsInactive();
}

void FlutterDesktopEngineNotifyAppIsResumed(FlutterDesktopEngineRef engine) {
  EngineFromHandle(engine)->lifecycle_channel()->AppIsResumed();
}

void FlutterDesktopEngineNotifyAppIsPaused(FlutterDesktopEngineRef engine) {
  EngineFromHandle(engine)->lifecycle_channel()->AppIsPaused();
}

void FlutterDesktopEngineNotifyAppIsDetached(FlutterDesktopEngineRef engine) {
  EngineFromHandle(engine)->lifecycle_channel()->AppIsDetached();
}

void FlutterDesktopViewResize(FlutterDesktopViewRef view,
                              int32_t width,
                              int32_t height) {
  ViewFromHandle(view)->Resize(width, height);
}

void FlutterDesktopRegisterViewFactory(
    FlutterDesktopPluginRegistrarRef registrar,
    const char* view_type,
    std::unique_ptr<PlatformViewFactory> view_factory) {
  registrar->engine->platform_view_channel()->ViewFactories().insert(
      std::pair<std::string, std::unique_ptr<PlatformViewFactory>>(
          view_type, std::move(view_factory)));
}

FlutterDesktopTextureRegistrarRef FlutterDesktopRegistrarGetTextureRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  return HandleForTextureRegistrar(registrar->engine->texture_registrar());
}

int64_t FlutterDesktopTextureRegistrarRegisterExternalTexture(
    FlutterDesktopTextureRegistrarRef texture_registrar,
    const FlutterDesktopTextureInfo* texture_info) {
  return TextureRegistrarFromHandle(texture_registrar)
      ->RegisterTexture(texture_info);
}

bool FlutterDesktopTextureRegistrarUnregisterExternalTexture(
    FlutterDesktopTextureRegistrarRef texture_registrar,
    int64_t texture_id) {
  return TextureRegistrarFromHandle(texture_registrar)
      ->UnregisterTexture(texture_id);
}

bool FlutterDesktopTextureRegistrarMarkExternalTextureFrameAvailable(
    FlutterDesktopTextureRegistrarRef texture_registrar,
    int64_t texture_id) {
  return TextureRegistrarFromHandle(texture_registrar)
      ->MarkTextureFrameAvailable(texture_id);
}
