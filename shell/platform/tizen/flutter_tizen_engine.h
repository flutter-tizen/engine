// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_FLUTTER_TIZEN_ENGINE_H_
#define EMBEDDER_FLUTTER_TIZEN_ENGINE_H_

#include <map>
#include <memory>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/plugin_registrar.h"
#include "flutter/shell/platform/common/incoming_message_dispatcher.h"
#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/tizen/channels/key_event_channel.h"
#include "flutter/shell/platform/tizen/channels/lifecycle_channel.h"
#include "flutter/shell/platform/tizen/channels/localization_channel.h"
#include "flutter/shell/platform/tizen/channels/navigation_channel.h"
#include "flutter/shell/platform/tizen/channels/platform_channel.h"
#include "flutter/shell/platform/tizen/channels/platform_view_channel.h"
#include "flutter/shell/platform/tizen/channels/settings_channel.h"
#include "flutter/shell/platform/tizen/channels/text_input_channel.h"
#include "flutter/shell/platform/tizen/flutter_tizen_texture_registrar.h"
#include "flutter/shell/platform/tizen/key_event_handler.h"
#include "flutter/shell/platform/tizen/public/flutter_tizen.h"
#include "flutter/shell/platform/tizen/tizen_event_loop.h"
#include "flutter/shell/platform/tizen/tizen_renderer.h"
#ifdef TIZEN_RENDERER_EVAS_GL
#include "flutter/shell/platform/tizen/tizen_renderer_evas_gl.h"
#else
#include "flutter/shell/platform/tizen/tizen_renderer_ecore_wl2.h"
#include "flutter/shell/platform/tizen/tizen_vsync_waiter.h"
#endif
#include "flutter/shell/platform/tizen/touch_event_handler.h"

// State associated with the plugin registrar.
struct FlutterDesktopPluginRegistrar {
  // The engine that owns this state object.
  FlutterTizenEngine* engine;
};

// State associated with the messenger used to communicate with the engine.
struct FlutterDesktopMessenger {
  // The engine that owns this state object.
  FlutterTizenEngine* engine = nullptr;
};

// Custom deleter for FlutterEngineAOTData.
struct AOTDataDeleter {
  void operator()(FlutterEngineAOTData aot_data) {
    FlutterEngineCollectAOTData(aot_data);
  }
};

using UniqueAotDataPtr = std::unique_ptr<_FlutterEngineAOTData, AOTDataDeleter>;

enum DeviceProfile { kUnknown, kMobile, kWearable, kTV, kCommon };

// Manages state associated with the underlying FlutterEngine.
class FlutterTizenEngine : public TizenRenderer::Delegate {
 public:
  explicit FlutterTizenEngine(bool headed);
  virtual ~FlutterTizenEngine();

  // Prevent copying.
  FlutterTizenEngine(FlutterTizenEngine const&) = delete;
  FlutterTizenEngine& operator=(FlutterTizenEngine const&) = delete;

  void InitializeRenderer();
  bool RunEngine(const FlutterDesktopEngineProperties& engine_properties);
  bool StopEngine();

  // Returns the currently configured Plugin Registrar.
  FlutterDesktopPluginRegistrarRef GetPluginRegistrar();

  FlutterTizenTextureRegistrar* GetTextureRegistrar();

  // Sets |callback| to be called when the plugin registrar is destroyed.
  void SetPluginRegistrarDestructionCallback(
      FlutterDesktopOnPluginRegistrarDestroyed callback);

  // Sends the given message to the engine, calling |reply| with |user_data|
  // when a reponse is received from the engine if they are non-null.
  bool SendPlatformMessage(const char* channel,
                           const uint8_t* message,
                           const size_t message_size,
                           const FlutterDesktopBinaryReply reply,
                           void* user_data);

  // Sends the given data as the response to an earlier platform message.
  void SendPlatformMessageResponse(
      const FlutterDesktopMessageResponseHandle* handle,
      const uint8_t* data,
      size_t data_length);

  // Informs the engine of an incoming pointer event.
  void SendPointerEvent(const FlutterPointerEvent& event);

  // Sends a window metrics update to the Flutter engine using current window
  // dimensions in physical
  void SendWindowMetrics(int32_t width, int32_t height, double pixel_ratio);

  void SetWindowOrientation(int32_t degree);
  void OnOrientationChange(int32_t degree) override;
  void OnVsync(intptr_t baton,
               uint64_t frame_start_time_nanos,
               uint64_t frame_target_time_nanos);

  void NotifyLowMemoryWarning();
  void UpdateLocales(const FlutterLocale** locales, size_t locales_count);

  // Attempts to register the texture with the given |texture_id|.
  bool RegisterExternalTexture(int64_t texture_id);

  // Attempts to unregister the texture with the given |texture_id|.
  bool UnregisterExternalTexture(int64_t texture_id);

  // Notifies the engine about a new frame being available for the
  // given |texture_id|.
  bool MarkExternalTextureFrameAvailable(int64_t texture_id);

  // The plugin messenger handle given to API clients.
  std::unique_ptr<FlutterDesktopMessenger> messenger;

  // Message dispatch manager for messages from the Flutter engine.
  std::unique_ptr<flutter::IncomingMessageDispatcher> message_dispatcher;

  // The interface between the Flutter rasterizer and the platform.
  std::unique_ptr<TizenRenderer> renderer;

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

 private:
  bool IsHeaded() { return renderer != nullptr; }
  UniqueAotDataPtr LoadAotData(std::string aot_data_path);
  FlutterDesktopMessage ConvertToDesktopMessage(
      const FlutterPlatformMessage& engine_message);
  FlutterRendererConfig GetRendererConfig();

  FlutterEngineProcTable embedder_api_ = {};

  // The Flutter engine instance.
  FLUTTER_API_SYMBOL(FlutterEngine) engine_;

  // The handlers listening to platform events.
  std::unique_ptr<KeyEventHandler> key_event_handler_;
  std::unique_ptr<TouchEventHandler> touch_event_handler_;

  // The plugin registrar handle given to API clients.
  std::unique_ptr<FlutterDesktopPluginRegistrar> plugin_registrar_;

  // The texture registrar.
  std::unique_ptr<FlutterTizenTextureRegistrar> texture_registrar_;

  // A callback to be called when the engine (and thus the plugin registrar)
  // is being destroyed.
  FlutterDesktopOnPluginRegistrarDestroyed
      plugin_registrar_destruction_callback_{nullptr};

  // The plugin registrar managing internal plugins.
  std::unique_ptr<flutter::PluginRegistrar> internal_plugin_registrar_;

  // The event loop for the main thread that allows for delayed task execution.
  std::unique_ptr<TizenPlatformEventLoop> event_loop_;

#ifdef TIZEN_RENDERER_EVAS_GL
  std::unique_ptr<TizenRenderEventLoop> render_loop_;
#endif

#ifndef TIZEN_RENDERER_EVAS_GL
  // The vsync waiter for the embedder.
  std::unique_ptr<TizenVsyncWaiter> tizen_vsync_waiter_;
#endif

  // AOT data for this engine instance, if applicable.
  UniqueAotDataPtr aot_data_;

  // The current renderer transformation.
  FlutterTransformation transformation_;
};

#endif  // EMBEDDER_FLUTTER_TIZEN_ENGINE_H_
