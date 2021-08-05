// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_FLUTTER_TIZEN_ENGINE_H_
#define EMBEDDER_FLUTTER_TIZEN_ENGINE_H_

#include <memory>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/plugin_registrar.h"
#include "flutter/shell/platform/common/incoming_message_dispatcher.h"
#include "flutter/shell/platform/embedder/embedder.h"
#ifndef __X64_SHELL__
#include "flutter/shell/platform/tizen/channels/app_control_channel.h"
#endif
#include "flutter/shell/platform/tizen/channels/key_event_channel.h"
#include "flutter/shell/platform/tizen/channels/lifecycle_channel.h"
#include "flutter/shell/platform/tizen/channels/navigation_channel.h"
#include "flutter/shell/platform/tizen/channels/platform_channel.h"
#include "flutter/shell/platform/tizen/channels/platform_view_channel.h"
#include "flutter/shell/platform/tizen/channels/settings_channel.h"
#include "flutter/shell/platform/tizen/channels/text_input_channel.h"
#include "flutter/shell/platform/tizen/flutter_project_bundle.h"
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
  flutter::FlutterTizenEngine* engine = nullptr;
};

// State associated with the messenger used to communicate with the engine.
struct FlutterDesktopMessenger {
  // The engine that owns this state object.
  flutter::FlutterTizenEngine* engine = nullptr;
};

namespace flutter {

// Manages state associated with the underlying FlutterEngine.
class FlutterTizenEngine : public TizenRenderer::Delegate {
 public:
  // Creates a new Flutter engine object configured to run |project|.
  explicit FlutterTizenEngine(const FlutterProjectBundle& project);

  virtual ~FlutterTizenEngine();

  // Prevent copying.
  FlutterTizenEngine(FlutterTizenEngine const&) = delete;
  FlutterTizenEngine& operator=(FlutterTizenEngine const&) = delete;

  // Sets up an instance of TizenRenderer.
  void InitializeRenderer(int32_t x,
                          int32_t y,
                          int32_t width,
                          int32_t height,
                          bool transparent,
                          bool focusable);

  // Starts running the engine with the given entrypoint. If null, defaults to
  // main().
  //
  // Returns false if the engine couldn't be started.
  bool RunEngine(const char* entrypoint);

  // Stops the engine.
  bool StopEngine();

  FlutterDesktopMessengerRef messenger() { return messenger_.get(); }

  IncomingMessageDispatcher* message_dispatcher() {
    return message_dispatcher_.get();
  }

  FlutterDesktopPluginRegistrarRef plugin_registrar() {
    return plugin_registrar_.get();
  }

  FlutterTizenTextureRegistrar* texture_registrar() {
    return texture_registrar_.get();
  }

  TizenRenderer* renderer() { return renderer_.get(); }

#ifndef __X64_SHELL__
  AppControlChannel* app_control_channel() {
    return app_control_channel_.get();
  }
#endif

  KeyEventChannel* key_event_channel() { return key_event_channel_.get(); }

  LifecycleChannel* lifecycle_channel() { return lifecycle_channel_.get(); }

  NavigationChannel* navigation_channel() { return navigation_channel_.get(); }

  PlatformViewChannel* platform_view_channel() {
    return platform_view_channel_.get();
  }

  TextInputChannel* text_input_channel() { return text_input_channel_.get(); }

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

  // Passes locale information to the Flutter engine.
  void SetupLocales();

  // Posts a low memory notification to the Flutter engine.
  void NotifyLowMemoryWarning();

  // Attempts to register the texture with the given |texture_id|.
  bool RegisterExternalTexture(int64_t texture_id);

  // Attempts to unregister the texture with the given |texture_id|.
  bool UnregisterExternalTexture(int64_t texture_id);

  // Notifies the engine about a new frame being available for the
  // given |texture_id|.
  bool MarkExternalTextureFrameAvailable(int64_t texture_id);

 private:
  friend class EngineModifier;

  // Whether the engine is running in headed or headless mode.
  bool IsHeaded() { return renderer_ != nullptr; }

  FlutterDesktopMessage ConvertToDesktopMessage(
      const FlutterPlatformMessage& engine_message);

  // Creates and returns a FlutterRendererConfig depending on the current
  // display mode (headed or headless).
  // The user_data received by the render callbacks refers to the
  // FlutterTizenEngine.
  FlutterRendererConfig GetRendererConfig();

  // The Flutter engine instance.
  FLUTTER_API_SYMBOL(FlutterEngine) engine_ = nullptr;

  // The proc table of the embedder APIs.
  FlutterEngineProcTable embedder_api_ = {};

  // The data required for configuring a Flutter engine instance.
  std::unique_ptr<FlutterProjectBundle> project_;

  // AOT data for this engine instance, if applicable.
  UniqueAotDataPtr aot_data_;

  // An event dispatcher for Ecore key events.
  std::unique_ptr<KeyEventHandler> key_event_handler_;

  // An event dispatcher for Ecore mouse events.
  std::unique_ptr<TouchEventHandler> touch_event_handler_;

  // The plugin messenger handle given to API clients.
  std::unique_ptr<FlutterDesktopMessenger> messenger_;

  // Message dispatch manager for messages from the Flutter engine.
  std::unique_ptr<IncomingMessageDispatcher> message_dispatcher_;

  // The plugin registrar handle given to API clients.
  std::unique_ptr<FlutterDesktopPluginRegistrar> plugin_registrar_;

  // The texture registrar.
  std::unique_ptr<FlutterTizenTextureRegistrar> texture_registrar_;

  // A callback to be called when the engine (and thus the plugin registrar)
  // is being destroyed.
  FlutterDesktopOnPluginRegistrarDestroyed
      plugin_registrar_destruction_callback_{nullptr};

  // The plugin registrar managing internal plugins.
  std::unique_ptr<PluginRegistrar> internal_plugin_registrar_;

#ifndef __X64_SHELL__
  // A plugin that implements Tizen app_control channels.
  std::unique_ptr<AppControlChannel> app_control_channel_;
#endif

  // A plugin that implements the Flutter keyevent channel.
  std::unique_ptr<KeyEventChannel> key_event_channel_;

  // A plugin that implements the Flutter lifecycle channel.
  std::unique_ptr<LifecycleChannel> lifecycle_channel_;

  // A plugin that implements the Flutter navigation channel.
  std::unique_ptr<NavigationChannel> navigation_channel_;

  // A plugin that implements the Flutter platform channel.
  std::unique_ptr<PlatformChannel> platform_channel_;

  // A plugin that implements the Flutter platform_views channel.
  std::unique_ptr<PlatformViewChannel> platform_view_channel_;

  // A plugin that implements the Flutter settings channel.
  std::unique_ptr<SettingsChannel> settings_channel_;

  // A plugin that implements the Flutter textinput channel.
  std::unique_ptr<TextInputChannel> text_input_channel_;

  // The event loop for the main thread that allows for delayed task execution.
  std::unique_ptr<TizenPlatformEventLoop> event_loop_;

#ifdef TIZEN_RENDERER_EVAS_GL
  std::unique_ptr<TizenRenderEventLoop> render_loop_;
#endif

  // An interface between the Flutter rasterizer and the platform.
  std::unique_ptr<TizenRenderer> renderer_;

#ifndef TIZEN_RENDERER_EVAS_GL
  // The vsync waiter for the embedder.
  std::unique_ptr<TizenVsyncWaiter> tizen_vsync_waiter_;
#endif

  // The current renderer transformation.
  FlutterTransformation transformation_;
};

}  // namespace flutter

#endif  // EMBEDDER_FLUTTER_TIZEN_ENGINE_H_
