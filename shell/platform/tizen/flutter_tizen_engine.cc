// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_tizen_engine.h"

#include <algorithm>
#include <string>
#include <vector>

#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/system_utils.h"
#include "flutter/shell/platform/tizen/tizen_input_method_context.h"

namespace flutter {

namespace {

// Unique number associated with platform tasks.
constexpr size_t kPlatformTaskRunnerIdentifier = 1;
#ifdef TIZEN_RENDERER_EVAS_GL
constexpr size_t kRenderTaskRunnerIdentifier = 2;
#endif

#if defined(MOBILE_PROFILE)
constexpr double kProfileFactor = 0.7;
#elif defined(WEARABLE_PROFILE)
constexpr double kProfileFactor = 0.4;
#elif defined(TV_PROFILE)
constexpr double kProfileFactor = 2.0;
#else
constexpr double kProfileFactor = 1.0;
#endif

// Converts a LanguageInfo struct to a FlutterLocale struct. |info| must outlive
// the returned value, since the returned FlutterLocale has pointers into it.
FlutterLocale CovertToFlutterLocale(const LanguageInfo& info) {
  FlutterLocale locale = {};
  locale.struct_size = sizeof(FlutterLocale);
  locale.language_code = info.language.c_str();
  if (!info.country.empty()) {
    locale.country_code = info.country.c_str();
  }
  if (!info.script.empty()) {
    locale.script_code = info.script.c_str();
  }
  if (!info.variant.empty()) {
    locale.variant_code = info.variant.c_str();
  }
  return locale;
}

}  // namespace

FlutterTizenEngine::FlutterTizenEngine(const FlutterProjectBundle& project)
    : project_(std::make_unique<FlutterProjectBundle>(project)),
      aot_data_(nullptr, nullptr) {
  embedder_api_.struct_size = sizeof(FlutterEngineProcTable);
  FlutterEngineGetProcAddresses(&embedder_api_);

  // Run flutter task on Tizen main loop.
  // Tizen engine has four threads (GPU thread, UI thread, IO thread, platform
  // thread). UI threads need to send flutter task to platform thread.
  event_loop_ = std::make_unique<TizenPlatformEventLoop>(
      std::this_thread::get_id(),  // main thread
      embedder_api_.GetCurrentTime, [this](const auto* task) {
        if (embedder_api_.RunTask(this->engine_, task) != kSuccess) {
          FT_LOG(Error) << "Could not post an engine task.";
        }
      });

  messenger_ = std::make_unique<FlutterDesktopMessenger>();
  messenger_->engine = this;
  message_dispatcher_ =
      std::make_unique<IncomingMessageDispatcher>(messenger_.get());

  plugin_registrar_ = std::make_unique<FlutterDesktopPluginRegistrar>();
  plugin_registrar_->engine = this;
}

FlutterTizenEngine::~FlutterTizenEngine() {
  renderer_ = nullptr;
}

void FlutterTizenEngine::InitializeRenderer(int32_t x,
                                            int32_t y,
                                            int32_t width,
                                            int32_t height,
                                            bool transparent,
                                            bool focusable) {
  TizenRenderer::WindowGeometry geometry = {x, y, width, height};

#ifdef TIZEN_RENDERER_EVAS_GL
  renderer_ = std::make_unique<TizenRendererEvasGL>(geometry, transparent,
                                                    focusable, *this);

  render_loop_ = std::make_unique<TizenRenderEventLoop>(
      std::this_thread::get_id(),  // main thread
      embedder_api_.GetCurrentTime,
      [this](const auto* task) {
        if (embedder_api_.RunTask(this->engine_, task) != kSuccess) {
          FT_LOG(Error) << "Could not post an engine task.";
        }
      },
      renderer_.get());
#else
  renderer_ = std::make_unique<TizenRendererEcoreWl2>(geometry, transparent,
                                                      focusable, *this);

  tizen_vsync_waiter_ = std::make_unique<TizenVsyncWaiter>(this);
#endif
}

bool FlutterTizenEngine::RunEngine(const char* entrypoint) {
  if (engine_ != nullptr) {
    FT_LOG(Error) << "The engine has already started.";
    return false;
  }
  if (IsHeaded() && !renderer_->IsValid()) {
    FT_LOG(Error) << "The display was not valid.";
    return false;
  }

  if (!project_->HasValidPaths()) {
    FT_LOG(Error) << "Missing or unresolvable paths to assets.";
    return false;
  }
  std::string assets_path_string = project_->assets_path().u8string();
  std::string icu_path_string = project_->icu_path().u8string();
  if (embedder_api_.RunsAOTCompiledDartCode()) {
    aot_data_ = project_->LoadAotData(embedder_api_);
    if (!aot_data_) {
      FT_LOG(Error) << "Unable to start engine without AOT data.";
      return false;
    }
  }

  // FlutterProjectArgs is expecting a full argv, so when processing it for
  // flags the first item is treated as the executable and ignored. Add a dummy
  // value so that all provided arguments are used.
  std::vector<std::string> switches = project_->switches();
  std::vector<const char*> argv = {"placeholder"};
  std::transform(
      switches.begin(), switches.end(), std::back_inserter(argv),
      [](const std::string& arg) -> const char* { return arg.c_str(); });

  if (std::find(switches.begin(), switches.end(), "--verbose-logging") !=
      switches.end()) {
    Logger::SetLoggingLevel(kLogLevelDebug);
  }

  const std::vector<std::string>& entrypoint_args =
      project_->dart_entrypoint_arguments();
  std::vector<const char*> entrypoint_argv;
  std::transform(
      entrypoint_args.begin(), entrypoint_args.end(),
      std::back_inserter(entrypoint_argv),
      [](const std::string& arg) -> const char* { return arg.c_str(); });

  // Configure task runners.
  FlutterTaskRunnerDescription platform_task_runner = {};
  platform_task_runner.struct_size = sizeof(FlutterTaskRunnerDescription);
  platform_task_runner.user_data = event_loop_.get();
  platform_task_runner.runs_task_on_current_thread_callback =
      [](void* data) -> bool {
    return static_cast<TizenEventLoop*>(data)->RunsTasksOnCurrentThread();
  };
  platform_task_runner.post_task_callback =
      [](FlutterTask task, uint64_t target_time_nanos, void* data) -> void {
    static_cast<TizenEventLoop*>(data)->PostTask(task, target_time_nanos);
  };
  platform_task_runner.identifier = kPlatformTaskRunnerIdentifier;
  FlutterCustomTaskRunners custom_task_runners = {};
  custom_task_runners.struct_size = sizeof(FlutterCustomTaskRunners);
  custom_task_runners.platform_task_runner = &platform_task_runner;

#ifdef TIZEN_RENDERER_EVAS_GL
  FlutterTaskRunnerDescription render_task_runner = {};
  if (IsHeaded()) {
    render_task_runner.struct_size = sizeof(FlutterTaskRunnerDescription);
    render_task_runner.user_data = render_loop_.get();
    render_task_runner.runs_task_on_current_thread_callback =
        [](void* data) -> bool {
      return static_cast<TizenEventLoop*>(data)->RunsTasksOnCurrentThread();
    };
    render_task_runner.post_task_callback =
        [](FlutterTask task, uint64_t target_time_nanos, void* data) -> void {
      static_cast<TizenEventLoop*>(data)->PostTask(task, target_time_nanos);
    };
    render_task_runner.identifier = kRenderTaskRunnerIdentifier;
    custom_task_runners.render_task_runner = &render_task_runner;
  }
#endif

  FlutterProjectArgs args = {};
  args.struct_size = sizeof(FlutterProjectArgs);
  args.assets_path = assets_path_string.c_str();
  args.icu_data_path = icu_path_string.c_str();
  args.command_line_argc = static_cast<int>(argv.size());
  args.command_line_argv = argv.size() > 0 ? argv.data() : nullptr;
  args.dart_entrypoint_argc = static_cast<int>(entrypoint_argv.size());
  args.dart_entrypoint_argv =
      entrypoint_argv.size() > 0 ? entrypoint_argv.data() : nullptr;
  args.platform_message_callback =
      [](const FlutterPlatformMessage* engine_message, void* user_data) {
        if (engine_message->struct_size != sizeof(FlutterPlatformMessage)) {
          FT_LOG(Error) << "Invalid message size received. Expected: "
                        << sizeof(FlutterPlatformMessage) << ", but received "
                        << engine_message->struct_size;
          return;
        }
        auto engine = reinterpret_cast<FlutterTizenEngine*>(user_data);
        auto message = engine->ConvertToDesktopMessage(*engine_message);
        engine->message_dispatcher_->HandleMessage(message);
      };
  args.custom_task_runners = &custom_task_runners;
#ifndef TIZEN_RENDERER_EVAS_GL
  if (IsHeaded()) {
    args.vsync_callback = [](void* user_data, intptr_t baton) -> void {
      reinterpret_cast<FlutterTizenEngine*>(user_data)
          ->tizen_vsync_waiter_->AsyncWaitForVsync(baton);
    };
  }
#endif
  if (aot_data_) {
    args.aot_data = aot_data_.get();
  }
  if (entrypoint) {
    args.custom_dart_entrypoint = entrypoint;
  }

  FlutterRendererConfig renderer_config = GetRendererConfig();

  auto result = embedder_api_.Run(FLUTTER_ENGINE_VERSION, &renderer_config,
                                  &args, this, &engine_);
  if (result != kSuccess || engine_ == nullptr) {
    FT_LOG(Error) << "Failed to start the Flutter engine with error: "
                  << result;
    return false;
  }

  internal_plugin_registrar_ =
      std::make_unique<PluginRegistrar>(plugin_registrar_.get());
#ifndef __X64_SHELL__
  app_control_channel_ = std::make_unique<AppControlChannel>(
      internal_plugin_registrar_->messenger());
#endif
  lifecycle_channel_ = std::make_unique<LifecycleChannel>(
      internal_plugin_registrar_->messenger());
  platform_channel_ = std::make_unique<PlatformChannel>(
      internal_plugin_registrar_->messenger(), renderer_.get());
  settings_channel_ = std::make_unique<SettingsChannel>(
      internal_plugin_registrar_->messenger());

  if (IsHeaded()) {
    texture_registrar_ = std::make_unique<FlutterTizenTextureRegistrar>(this);
    key_event_channel_ = std::make_unique<KeyEventChannel>(
        internal_plugin_registrar_->messenger());
    navigation_channel_ = std::make_unique<NavigationChannel>(
        internal_plugin_registrar_->messenger());
    platform_view_channel_ = std::make_unique<PlatformViewChannel>(
        internal_plugin_registrar_->messenger());
    text_input_channel_ = std::make_unique<TextInputChannel>(
        internal_plugin_registrar_->messenger(),
        std::make_unique<TizenInputMethodContext>(this));
    key_event_handler_ = std::make_unique<KeyEventHandler>(this);
    touch_event_handler_ = std::make_unique<TouchEventHandler>(this);

    SetWindowOrientation(0);
  }

  SetupLocales();

  return true;
}

bool FlutterTizenEngine::StopEngine() {
  if (engine_) {
    if (platform_view_channel_) {
      platform_view_channel_->Dispose();
    }
    if (plugin_registrar_destruction_callback_) {
      plugin_registrar_destruction_callback_(plugin_registrar_.get());
    }
    FlutterEngineResult result = embedder_api_.Shutdown(engine_);
    engine_ = nullptr;
    return (result == kSuccess);
  }
  return false;
}

void FlutterTizenEngine::SetPluginRegistrarDestructionCallback(
    FlutterDesktopOnPluginRegistrarDestroyed callback) {
  plugin_registrar_destruction_callback_ = callback;
}

bool FlutterTizenEngine::SendPlatformMessage(
    const char* channel,
    const uint8_t* message,
    const size_t message_size,
    const FlutterDesktopBinaryReply reply,
    void* user_data) {
  FlutterPlatformMessageResponseHandle* response_handle = nullptr;
  if (reply != nullptr && user_data != nullptr) {
    FlutterEngineResult result =
        embedder_api_.PlatformMessageCreateResponseHandle(
            engine_, reply, user_data, &response_handle);
    if (result != kSuccess) {
      FT_LOG(Error) << "Failed to create a response handle.";
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

  FlutterEngineResult message_result =
      embedder_api_.SendPlatformMessage(engine_, &platform_message);
  if (response_handle != nullptr) {
    embedder_api_.PlatformMessageReleaseResponseHandle(engine_,
                                                       response_handle);
  }
  return message_result == kSuccess;
}

void FlutterTizenEngine::SendPlatformMessageResponse(
    const FlutterDesktopMessageResponseHandle* handle,
    const uint8_t* data,
    size_t data_length) {
  embedder_api_.SendPlatformMessageResponse(engine_, handle, data, data_length);
}

void FlutterTizenEngine::SendPointerEvent(const FlutterPointerEvent& event) {
  embedder_api_.SendPointerEvent(engine_, &event, 1);
}

void FlutterTizenEngine::SendWindowMetrics(int32_t width,
                                           int32_t height,
                                           double pixel_ratio) {
  FlutterWindowMetricsEvent event = {};
  event.struct_size = sizeof(FlutterWindowMetricsEvent);
  event.width = width;
  event.height = height;
  if (pixel_ratio == 0.0) {
    // The scale factor is computed based on the display DPI and the current
    // profile. A fixed DPI value (72) is used on TVs. See:
    // https://docs.tizen.org/application/native/guides/ui/efl/multiple-screens
#ifdef TV_PROFILE
    double dpi = 72.0;
#else
    double dpi = static_cast<double>(renderer_->GetDpi());
#endif
    double scale_factor = dpi / 90.0 * kProfileFactor;
    event.pixel_ratio = std::max(scale_factor, 1.0);
  } else {
    event.pixel_ratio = pixel_ratio;
  }
  embedder_api_.SendWindowMetricsEvent(engine_, &event);
}

// This must be called at least once in order to initialize the value of
// transformation_.
void FlutterTizenEngine::SetWindowOrientation(int32_t degree) {
  if (!renderer_->IsValid()) {
    return;
  }

  renderer_->SetRotate(degree);
  // Compute renderer transformation based on the angle of rotation.
  double rad = (360 - degree) * M_PI / 180;
  auto geometry = renderer_->GetCurrentGeometry();
  double width = geometry.w;
  double height = geometry.h;

  double trans_x = 0.0, trans_y = 0.0;
  if (degree == 90) {
    trans_y = height;
  } else if (degree == 180) {
    trans_x = width;
    trans_y = height;
  } else if (degree == 270) {
    trans_x = width;
  }
  transformation_ = {
      cos(rad), -sin(rad), trans_x,  // x
      sin(rad), cos(rad),  trans_y,  // y
      0.0,      0.0,       1.0       // perspective
  };
  touch_event_handler_->rotation = degree;
  if (degree == 90 || degree == 270) {
    renderer_->ResizeWithRotation(geometry.x, geometry.y, height, width,
                                  degree);
    SendWindowMetrics(height, width, 0.0);
  } else {
    renderer_->ResizeWithRotation(geometry.x, geometry.y, width, height,
                                  degree);
    SendWindowMetrics(width, height, 0.0);
  }
}

void FlutterTizenEngine::OnOrientationChange(int32_t degree) {
  SetWindowOrientation(degree);
}

void FlutterTizenEngine::OnVsync(intptr_t baton,
                                 uint64_t frame_start_time_nanos,
                                 uint64_t frame_target_time_nanos) {
  embedder_api_.OnVsync(engine_, baton, frame_start_time_nanos,
                        frame_target_time_nanos);
}

void FlutterTizenEngine::SetupLocales() {
  std::vector<LanguageInfo> languages = GetPreferredLanguageInfo();
  std::vector<FlutterLocale> flutter_locales;
  flutter_locales.reserve(languages.size());
  for (const auto& info : languages) {
    flutter_locales.push_back(CovertToFlutterLocale(info));
  }
  // Convert the locale list to the locale pointer list that must be provided.
  std::vector<const FlutterLocale*> flutter_locale_list;
  flutter_locale_list.reserve(flutter_locales.size());
  std::transform(
      flutter_locales.begin(), flutter_locales.end(),
      std::back_inserter(flutter_locale_list),
      [](const auto& arg) -> const auto* { return &arg; });

  embedder_api_.UpdateLocales(engine_, flutter_locale_list.data(),
                              flutter_locale_list.size());
}

void FlutterTizenEngine::NotifyLowMemoryWarning() {
  embedder_api_.NotifyLowMemoryWarning(engine_);
}

bool FlutterTizenEngine::RegisterExternalTexture(int64_t texture_id) {
  return (embedder_api_.RegisterExternalTexture(engine_, texture_id) ==
          kSuccess);
}

bool FlutterTizenEngine::UnregisterExternalTexture(int64_t texture_id) {
  return (embedder_api_.UnregisterExternalTexture(engine_, texture_id) ==
          kSuccess);
}

bool FlutterTizenEngine::MarkExternalTextureFrameAvailable(int64_t texture_id) {
  return (embedder_api_.MarkExternalTextureFrameAvailable(
              engine_, texture_id) == kSuccess);
}

// The Flutter Engine calls out to this function when new platform messages are
// available.

// Converts a FlutterPlatformMessage to an equivalent FlutterDesktopMessage.
FlutterDesktopMessage FlutterTizenEngine::ConvertToDesktopMessage(
    const FlutterPlatformMessage& engine_message) {
  FlutterDesktopMessage message = {};
  message.struct_size = sizeof(message);
  message.channel = engine_message.channel;
  message.message = engine_message.message;
  message.message_size = engine_message.message_size;
  message.response_handle = engine_message.response_handle;
  return message;
}

FlutterRendererConfig FlutterTizenEngine::GetRendererConfig() {
  FlutterRendererConfig config = {};
  if (IsHeaded()) {
    config.type = kOpenGL;
    config.open_gl.struct_size = sizeof(config.open_gl);
    config.open_gl.make_current = [](void* user_data) -> bool {
      return reinterpret_cast<FlutterTizenEngine*>(user_data)
          ->renderer_->OnMakeCurrent();
    };
    config.open_gl.make_resource_current = [](void* user_data) -> bool {
      return reinterpret_cast<FlutterTizenEngine*>(user_data)
          ->renderer_->OnMakeResourceCurrent();
    };
    config.open_gl.clear_current = [](void* user_data) -> bool {
      return reinterpret_cast<FlutterTizenEngine*>(user_data)
          ->renderer_->OnClearCurrent();
    };
    config.open_gl.present = [](void* user_data) -> bool {
      return reinterpret_cast<FlutterTizenEngine*>(user_data)
          ->renderer_->OnPresent();
    };
    config.open_gl.fbo_callback = [](void* user_data) -> uint32_t {
      return reinterpret_cast<FlutterTizenEngine*>(user_data)
          ->renderer_->OnGetFBO();
    };
    config.open_gl.surface_transformation =
        [](void* user_data) -> FlutterTransformation {
      return reinterpret_cast<FlutterTizenEngine*>(user_data)->transformation_;
    };
    config.open_gl.gl_proc_resolver = [](void* user_data,
                                         const char* name) -> void* {
      return reinterpret_cast<FlutterTizenEngine*>(user_data)
          ->renderer_->OnProcResolver(name);
    };
    config.open_gl.gl_external_texture_frame_callback =
        [](void* user_data, int64_t texture_id, size_t width, size_t height,
           FlutterOpenGLTexture* texture) -> bool {
      auto engine = reinterpret_cast<FlutterTizenEngine*>(user_data);
      if (!engine->texture_registrar()) {
        return false;
      }
      return engine->texture_registrar()->PopulateTexture(texture_id, width,
                                                          height, texture);
    };
  } else {
    config.type = kSoftware;
    config.software.struct_size = sizeof(config.software);
    config.software.surface_present_callback =
        [](void* user_data, const void* allocation, size_t row_bytes,
           size_t height) -> bool { return true; };
  }
  return config;
}

}  // namespace flutter
