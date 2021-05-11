// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/flutter_tizen.h"

#include <inttypes.h>

#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/plugin_registrar.h"
#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/standard_message_codec.h"
#include "flutter/shell/platform/common/cpp/incoming_message_dispatcher.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/public/flutter_platform_view.h"
#include "flutter/shell/platform/tizen/public/flutter_tizen_texture_registrar.h"
#include "flutter/shell/platform/tizen/tizen_log.h"

// Returns the engine corresponding to the given opaque API handle.
static FlutterTizenEngine* EngineFromHandle(FlutterDesktopEngineRef ref) {
  return reinterpret_cast<FlutterTizenEngine*>(ref);
}

// Returns the opaque API handle for the given engine instance.
static FlutterDesktopEngineRef HandleForEngine(FlutterTizenEngine* engine) {
  return reinterpret_cast<FlutterDesktopEngineRef>(engine);
}

FlutterDesktopEngineRef FlutterDesktopRunEngine(
    const FlutterDesktopEngineProperties& engine_properties, bool headed) {
  StartLogging();

  auto engine = std::make_unique<FlutterTizenEngine>(headed);
  if (!engine->RunEngine(engine_properties)) {
    FT_LOGE("Failed to run the Flutter engine.");
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
    FlutterDesktopPluginRegistrarRef registrar, const char* channel) {
  registrar->engine->message_dispatcher->EnableInputBlockingForChannel(channel);
}

FlutterDesktopPluginRegistrarRef FlutterDesktopGetPluginRegistrar(
    FlutterDesktopEngineRef engine, const char* plugin_name) {
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

FlutterTextureRegistrarRef FlutterPluginRegistrarGetTexture(
    FlutterDesktopPluginRegistrarRef registrar) {
  return registrar->texture_registrar.get();
}

bool FlutterDesktopMessengerSend(FlutterDesktopMessengerRef messenger,
                                 const char* channel, const uint8_t* message,
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
  FlutterPlatformMessageResponseHandle* response_handle = nullptr;
  if (reply != nullptr && user_data != nullptr) {
    FlutterEngineResult result = FlutterPlatformMessageCreateResponseHandle(
        messenger->engine->flutter_engine, reply, user_data, &response_handle);
    if (result != kSuccess) {
      FT_LOGE("Failed to create response handle");
      return false;
    }
  }
  FlutterPlatformMessage platform_message = {
      sizeof(FlutterPlatformMessage),
      channel,
      message,
      message_size,
      response_handle,
  };
  FlutterEngineResult message_result = FlutterEngineSendPlatformMessage(
      messenger->engine->flutter_engine, &platform_message);
  if (response_handle != nullptr) {
    FlutterPlatformMessageReleaseResponseHandle(
        messenger->engine->flutter_engine, response_handle);
  }
  return message_result == kSuccess;
}

void FlutterDesktopMessengerSendResponse(
    FlutterDesktopMessengerRef messenger,
    const FlutterDesktopMessageResponseHandle* handle, const uint8_t* data,
    size_t data_length) {
  FlutterEngineSendPlatformMessageResponse(messenger->engine->flutter_engine,
                                           handle, data, data_length);
}

void FlutterDesktopMessengerSetCallback(FlutterDesktopMessengerRef messenger,
                                        const char* channel,
                                        FlutterDesktopMessageCallback callback,
                                        void* user_data) {
  messenger->engine->message_dispatcher->SetMessageCallback(channel, callback,
                                                            user_data);
}

void FlutterDesktopNotifyLocaleChange(FlutterDesktopEngineRef engine) {
  EngineFromHandle(engine)->localization_channel->SendLocales();
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

void FlutterDesktopNotifyLowMemoryWarning(FlutterDesktopEngineRef engine) {
  auto flutter_engine = EngineFromHandle(engine)->flutter_engine;
  FlutterEngineNotifyLowMemoryWarning(flutter_engine);
}

int64_t FlutterRegisterExternalTexture(
    FlutterTextureRegistrarRef texture_registrar) {
  FT_LOGD("FlutterDesktopRegisterExternalTexture");
  std::lock_guard<std::mutex> lock(texture_registrar->mutex);
  auto texture_gl = std::make_unique<ExternalTextureGL>();
  int64_t texture_id = texture_gl->TextureId();
  texture_registrar->textures[texture_id] = std::move(texture_gl);
  if (FlutterEngineRegisterExternalTexture(texture_registrar->flutter_engine,
                                           texture_id) == kSuccess) {
    return texture_id;
  }
  return -1;
}

bool FlutterUnregisterExternalTexture(
    FlutterTextureRegistrarRef texture_registrar, int64_t texture_id) {
  std::lock_guard<std::mutex> lock(texture_registrar->mutex);
  auto it = texture_registrar->textures.find(texture_id);
  if (it != texture_registrar->textures.end())
    texture_registrar->textures.erase(it);
  bool ret = FlutterEngineUnregisterExternalTexture(
                 texture_registrar->flutter_engine, texture_id) == kSuccess;
  return ret;
}

bool FlutterMarkExternalTextureFrameAvailable(
    FlutterTextureRegistrarRef texture_registrar, int64_t texture_id,
    void* tbm_surface) {
  std::lock_guard<std::mutex> lock(texture_registrar->mutex);
  auto it = texture_registrar->textures.find(texture_id);
  if (it == texture_registrar->textures.end()) {
    FT_LOGE("can't find texture texture_id = %" PRId64, texture_id);
    return false;
  }
  if (!texture_registrar->textures[texture_id]->OnFrameAvailable(
          (tbm_surface_h)tbm_surface)) {
    // If a texture that has not been used already exists, it can fail
    return false;
  }
  bool ret = FlutterEngineMarkExternalTextureFrameAvailable(
                 texture_registrar->flutter_engine, texture_id) == kSuccess;
  return ret;
}

void FlutterRegisterViewFactory(
    FlutterDesktopPluginRegistrarRef registrar, const char* view_type,
    std::unique_ptr<PlatformViewFactory> view_factory) {
  registrar->engine->platform_view_channel->ViewFactories().insert(
      std::pair<std::string, std::unique_ptr<PlatformViewFactory>>(
          view_type, std::move(view_factory)));
}

FlutterDesktopTextureRegistrarRef FlutterDesktopRegistrarGetTextureRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  FT_UNIMPLEMENTED();
  return nullptr;
}

int64_t FlutterDesktopTextureRegistrarRegisterExternalTexture(
    FlutterDesktopTextureRegistrarRef texture_registrar,
    const FlutterDesktopTextureInfo* texture_info) {
  FT_UNIMPLEMENTED();
  return -1;
}

bool FlutterDesktopTextureRegistrarUnregisterExternalTexture(
    FlutterDesktopTextureRegistrarRef texture_registrar, int64_t texture_id) {
  FT_UNIMPLEMENTED();
  return false;
}

bool FlutterDesktopTextureRegistrarMarkExternalTextureFrameAvailable(
    FlutterDesktopTextureRegistrarRef texture_registrar, int64_t texture_id) {
  FT_UNIMPLEMENTED();
  return false;
}
