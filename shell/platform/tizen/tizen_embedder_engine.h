// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_EMBEDDER_ENGINE_H_
#define EMBEDDER_TIZEN_EMBEDDER_ENGINE_H_

#include <memory>

#include "flutter/shell/platform/common/cpp/client_wrapper/include/flutter/plugin_registrar.h"
#include "flutter/shell/platform/common/cpp/incoming_message_dispatcher.h"
#include "flutter/shell/platform/tizen/channels/key_event_channel.h"
#include "flutter/shell/platform/tizen/channels/lifecycle_channel.h"
#include "flutter/shell/platform/tizen/channels/localization_channel.h"
#include "flutter/shell/platform/tizen/channels/navigation_channel.h"
#include "flutter/shell/platform/tizen/channels/platform_channel.h"
#include "flutter/shell/platform/tizen/channels/platform_view_channel.h"
#include "flutter/shell/platform/tizen/channels/settings_channel.h"
#include "flutter/shell/platform/tizen/channels/text_input_channel.h"
#include "flutter/shell/platform/tizen/external_texture_gl.h"
#include "flutter/shell/platform/tizen/key_event_handler.h"
#include "flutter/shell/platform/tizen/public/flutter_texture_registrar.h"
#include "flutter/shell/platform/tizen/public/flutter_tizen.h"
#include "flutter/shell/platform/tizen/tizen_event_loop.h"
#include "flutter/shell/platform/tizen/tizen_renderer.h"
#ifdef FLUTTER_TIZEN_4
#include "flutter/shell/platform/tizen/tizen_renderer_ecore_wl.h"
#else
#include "flutter/shell/platform/tizen/tizen_renderer_ecore_wl2.h"
#endif
#include "flutter/shell/platform/tizen/tizen_vsync_waiter.h"
#include "flutter/shell/platform/tizen/touch_event_handler.h"

// State associated with the plugin registrar.
struct FlutterDesktopPluginRegistrar {
  // The engine that owns this state object.
  TizenEmbedderEngine* engine;

  // The plugin texture registrar handle given to API clients.
  std::unique_ptr<FlutterTextureRegistrar> texture_registrar;
};

// State associated with the messenger used to communicate with the engine.
struct FlutterDesktopMessenger {
  // The engine that owns this state object.
  TizenEmbedderEngine* engine = nullptr;
};

// Custom deleter for FlutterEngineAOTData.
struct AOTDataDeleter {
  void operator()(FlutterEngineAOTData aot_data) {
    FlutterEngineCollectAOTData(aot_data);
  }
};

// State associated with the texture registrar.
struct FlutterTextureRegistrar {
  FLUTTER_API_SYMBOL(FlutterEngine) flutter_engine;

  // The texture registrar managing external texture adapters.
  std::map<int64_t, std::unique_ptr<ExternalTextureGL>> textures;
};

using UniqueAotDataPtr = std::unique_ptr<_FlutterEngineAOTData, AOTDataDeleter>;

enum DeviceProfile { kUnknown, kMobile, kWearable, kTV };

// Manages state associated with the underlying FlutterEngine.
class TizenEmbedderEngine : public TizenRenderer::Delegate {
 public:
  explicit TizenEmbedderEngine(
      const FlutterWindowProperties& window_properties);
  virtual ~TizenEmbedderEngine();
  bool RunEngine(const FlutterEngineProperties& engine_properties);
  bool StopEngine();

  // Returns the currently configured Plugin Registrar.
  FlutterDesktopPluginRegistrarRef GetPluginRegistrar();

  // Sets |callback| to be called when the plugin registrar is destroyed.
  void SetPluginRegistrarDestructionCallback(
      FlutterDesktopOnPluginRegistrarDestroyed callback);

  void SendWindowMetrics(int32_t width, int32_t height, double pixel_ratio);
  void SetWindowOrientation(int32_t degree);
  void SendLocales();
  void AppIsInactive();
  void AppIsResumed();
  void AppIsPaused();
  void AppIsDetached();
  void OnRotationChange(int degree) override;

  // The Flutter engine instance.
  FLUTTER_API_SYMBOL(FlutterEngine) flutter_engine;

  // The plugin messenger handle given to API clients.
  std::unique_ptr<FlutterDesktopMessenger> messenger;

  // Message dispatch manager for messages from the Flutter engine.
  std::unique_ptr<flutter::IncomingMessageDispatcher> message_dispatcher;

  // The interface between the Flutter rasterizer and the platform.
  std::unique_ptr<TizenRenderer> tizen_renderer;

  // The system channels for communicating between Flutter and the platform.
  std::unique_ptr<KeyEventChannel> key_event_channel;
  std::unique_ptr<LifecycleChannel> lifecycle_channel;
  std::unique_ptr<LocalizationChannel> localization_channel;
  std::unique_ptr<NavigationChannel> navigation_channel;
  std::unique_ptr<PlatformChannel> platform_channel;
  std::unique_ptr<SettingsChannel> settings_channel;
  std::unique_ptr<TextInputChannel> text_input_channel;
  std::unique_ptr<PlatformViewChannel> platform_view_channel;

  const DeviceProfile device_profile;
  const double device_dpi;

 private:
  static bool MakeContextCurrent(void* user_data);
  static bool ClearContext(void* user_data);
  static bool Present(void* user_data);
  static bool MakeResourceCurrent(void* user_data);
  static uint32_t GetActiveFbo(void* user_data);
  static FlutterTransformation Transformation(void* user_data);
  static void* GlProcResolver(void* user_data, const char* name);
  static void OnFlutterPlatformMessage(
      const FlutterPlatformMessage* engine_message, void* user_data);
  static void OnVsyncCallback(void* user_data, intptr_t baton);

  FlutterDesktopMessage ConvertToDesktopMessage(
      const FlutterPlatformMessage& engine_message);
  static bool OnAcquireExternalTexture(void* user_data, int64_t texture_id,
                                       size_t width, size_t height,
                                       FlutterOpenGLTexture* texture);

  // The handlers listening to platform events.
  std::unique_ptr<KeyEventHandler> key_event_handler_;
  std::unique_ptr<TouchEventHandler> touch_event_handler_;

  // The plugin registrar handle given to API clients.
  std::unique_ptr<FlutterDesktopPluginRegistrar> plugin_registrar_;

  // A callback to be called when the engine (and thus the plugin registrar)
  // is being destroyed.
  FlutterDesktopOnPluginRegistrarDestroyed
      plugin_registrar_destruction_callback_{nullptr};

  // The plugin registrar managing internal plugins.
  std::unique_ptr<flutter::PluginRegistrar> internal_plugin_registrar_;

  // The event loop for the main thread that allows for delayed task execution.
  std::unique_ptr<TizenEventLoop> event_loop_;

  // The vsync waiter for the embedder.
  std::unique_ptr<TizenVsyncWaiter> tizen_vsync_waiter_;

  // AOT data for this engine instance, if applicable.
  UniqueAotDataPtr aot_data_;

  // The current renderer transformation.
  FlutterTransformation transformation_;
};

#endif  // EMBEDDER_TIZEN_EMBEDDER_ENGINE_H_
