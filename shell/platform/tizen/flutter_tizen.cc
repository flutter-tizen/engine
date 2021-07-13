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
#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/public/flutter_platform_view.h"

// Returns the engine corresponding to the given opaque API handle.
static flutter::FlutterTizenEngine* EngineFromHandle(
    FlutterDesktopEngineRef ref) {
  return reinterpret_cast<flutter::FlutterTizenEngine*>(ref);
}

// Returns the opaque API handle for the given engine instance.
static FlutterDesktopEngineRef HandleForEngine(
    flutter::FlutterTizenEngine* engine) {
  return reinterpret_cast<FlutterDesktopEngineRef>(engine);
}

FlutterDesktopEngineRef FlutterDesktopRunEngine(
    const FlutterDesktopWindowProperties& window_properties,
    const FlutterDesktopEngineProperties& engine_properties) {
#ifndef __X64_SHELL__
  flutter::Logger::Start();
#endif

  flutter::FlutterProjectBundle project(engine_properties);
  auto engine = std::make_unique<flutter::FlutterTizenEngine>(project);
  if (window_properties.headed) {
    engine->InitializeRenderer(
        window_properties.x, window_properties.y, window_properties.width,
        window_properties.height, window_properties.transparent,
        window_properties.focusable);
  }
  if (!engine->RunEngine()) {
    FT_LOG(Error) << "Failed to start the Flutter engine.";
    return nullptr;
  }
  return HandleForEngine(engine.release());
}

void FlutterDesktopShutdownEngine(FlutterDesktopEngineRef engine_ref) {
  auto engine = EngineFromHandle(engine_ref);
  engine->StopEngine();
  delete engine;
}

void FlutterDesktopPluginRegistrarEnableInputBlocking(
    FlutterDesktopPluginRegistrarRef registrar,
    const char* channel) {
  registrar->engine->message_dispatcher->EnableInputBlockingForChannel(channel);
}

FlutterDesktopPluginRegistrarRef FlutterDesktopGetPluginRegistrar(
    FlutterDesktopEngineRef engine,
    const char* plugin_name) {
  // Currently, one registrar acts as the registrar for all plugins, so the
  // name is ignored. It is part of the API to reduce churn in the future when
  // aligning more closely with the Flutter registrar system.
  return EngineFromHandle(engine)->GetPluginRegistrar();
}

FlutterDesktopMessengerRef FlutterDesktopEngineGetMessenger(
    FlutterDesktopEngineRef engine) {
  return EngineFromHandle(engine)->messenger.get();
}

FlutterDesktopMessengerRef FlutterDesktopPluginRegistrarGetMessenger(
    FlutterDesktopPluginRegistrarRef registrar) {
  return registrar->engine->messenger.get();
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
  messenger->engine->message_dispatcher->SetMessageCallback(channel, callback,
                                                            user_data);
}

void FlutterDesktopNotifyLocaleChange(FlutterDesktopEngineRef engine) {
  EngineFromHandle(engine)->SetupLocales();
}

void FlutterDesktopNotifyLowMemoryWarning(FlutterDesktopEngineRef engine) {
  EngineFromHandle(engine)->NotifyLowMemoryWarning();
}

void FlutterDesktopNotifyAppIsInactive(FlutterDesktopEngineRef engine) {
  EngineFromHandle(engine)->lifecycle_channel->AppIsInactive();
}

void FlutterDesktopNotifyAppIsResumed(FlutterDesktopEngineRef engine) {
  EngineFromHandle(engine)->lifecycle_channel->AppIsResumed();
}

void FlutterDesktopNotifyAppIsPaused(FlutterDesktopEngineRef engine) {
  EngineFromHandle(engine)->lifecycle_channel->AppIsPaused();
}

void FlutterDesktopNotifyAppIsDetached(FlutterDesktopEngineRef engine) {
  EngineFromHandle(engine)->lifecycle_channel->AppIsDetached();
}

void FlutterRegisterViewFactory(
    FlutterDesktopPluginRegistrarRef registrar,
    const char* view_type,
    std::unique_ptr<PlatformViewFactory> view_factory) {
  view_factory->SetWindow(registrar->engine->renderer->GetWindowHandle());
  registrar->engine->platform_view_channel->ViewFactories().insert(
      std::pair<std::string, std::unique_ptr<PlatformViewFactory>>(
          view_type, std::move(view_factory)));
}

// Returns the texture registrar corresponding to the given opaque API handle.
static flutter::FlutterTizenTextureRegistrar* TextureRegistrarFromHandle(
    FlutterDesktopTextureRegistrarRef ref) {
  return reinterpret_cast<flutter::FlutterTizenTextureRegistrar*>(ref);
}

// Returns the opaque API handle for the given texture registrar instance.
static FlutterDesktopTextureRegistrarRef HandleForTextureRegistrar(
    flutter::FlutterTizenTextureRegistrar* registrar) {
  return reinterpret_cast<FlutterDesktopTextureRegistrarRef>(registrar);
}

FlutterDesktopTextureRegistrarRef FlutterDesktopRegistrarGetTextureRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  return HandleForTextureRegistrar(registrar->engine->GetTextureRegistrar());
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
