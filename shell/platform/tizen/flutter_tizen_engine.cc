// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_tizen_engine.h"

#include <system_info.h>

#include <filesystem>
#include <string>
#include <vector>

#include "flutter/shell/platform/tizen/tizen_log.h"

// Unique number associated with platform tasks.
static constexpr size_t kPlatformTaskRunnerIdentifier = 1;
#ifdef TIZEN_RENDERER_EVAS_GL
static constexpr size_t kRenderTaskRunnerIdentifier = 2;
#endif

static DeviceProfile GetDeviceProfile() {
  char* feature_profile;
  system_info_get_platform_string("http://tizen.org/feature/profile",
                                  &feature_profile);
  std::string profile(feature_profile);
  free(feature_profile);

  if (profile == "mobile") {
    return DeviceProfile::kMobile;
  } else if (profile == "wearable") {
    return DeviceProfile::kWearable;
  } else if (profile == "tv") {
    return DeviceProfile::kTV;
  } else if (profile == "common") {
    return DeviceProfile::kCommon;
  }
  return DeviceProfile::kUnknown;
}

FlutterTizenEngine::FlutterTizenEngine(bool headed)
    : device_profile(GetDeviceProfile()) {
  // Run flutter task on Tizen main loop.
  // Tizen engine has four threads (GPU thread, UI thread, IO thread, platform
  // thread). UI threads need to send flutter task to platform thread.
  event_loop_ = std::make_unique<TizenPlatformEventLoop>(
      std::this_thread::get_id(),  // main thread
      [this](const auto* task) {
        if (FlutterEngineRunTask(this->flutter_engine, task) != kSuccess) {
          FT_LOGE("Could not post an engine task.");
        }
      });

  messenger = std::make_unique<FlutterDesktopMessenger>();
  messenger->engine = this;
  message_dispatcher =
      std::make_unique<flutter::IncomingMessageDispatcher>(messenger.get());

  plugin_registrar_ = std::make_unique<FlutterDesktopPluginRegistrar>();
  plugin_registrar_->engine = this;

  if (headed) {
    InitializeRenderer();
  }
}

FlutterTizenEngine::~FlutterTizenEngine() {
  renderer = nullptr;
}

void FlutterTizenEngine::InitializeRenderer() {
#ifdef TIZEN_RENDERER_EVAS_GL
  renderer = std::make_unique<TizenRendererEvasGL>(*this);

  render_loop_ = std::make_unique<TizenRenderEventLoop>(
      std::this_thread::get_id(),  // main thread
      [this](const auto* task) {
        if (FlutterEngineRunTask(this->flutter_engine, task) != kSuccess) {
          FT_LOGE("Could not post an engine task.");
        }
      },
      renderer.get());
#else
  renderer = std::make_unique<TizenRendererEcoreWl2>(*this);

  tizen_vsync_waiter_ = std::make_unique<TizenVsyncWaiter>(this);
#endif
}

// Attempts to load AOT data from the given path, which must be absolute and
// non-empty. Logs and returns nullptr on failure.
UniqueAotDataPtr LoadAotData(std::string aot_data_path) {
  if (aot_data_path.empty()) {
    FT_LOGE(
        "Attempted to load AOT data, but no aot_library_path was provided.");
    return nullptr;
  }
  if (!std::filesystem::exists(aot_data_path)) {
    FT_LOGE("Can't load AOT data from %s; no such file.",
            aot_data_path.c_str());
    return nullptr;
  }
  FlutterEngineAOTDataSource source = {};
  source.type = kFlutterEngineAOTDataSourceTypeElfPath;
  source.elf_path = aot_data_path.c_str();
  FlutterEngineAOTData data = nullptr;
  auto result = FlutterEngineCreateAOTData(&source, &data);
  if (result != kSuccess) {
    FT_LOGE("Failed to load AOT data from: %s", aot_data_path.c_str());
    return nullptr;
  }
  return UniqueAotDataPtr(data);
}

bool FlutterTizenEngine::RunEngine(
    const FlutterDesktopEngineProperties& engine_properties) {
  if (IsHeaded() && !renderer->IsValid()) {
    FT_LOGE("The display was not valid.");
    return false;
  }

  // FlutterProjectArgs is expecting a full argv, so when processing it for
  // flags the first item is treated as the executable and ignored. Add a dummy
  // value so that all provided arguments are used.
  std::vector<const char*> argv = {"placeholder"};
  if (engine_properties.switches_count > 0) {
    argv.insert(argv.end(), &engine_properties.switches[0],
                &engine_properties.switches[engine_properties.switches_count]);
  }

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
  args.assets_path = engine_properties.assets_path;
  args.icu_data_path = engine_properties.icu_data_path;
  args.command_line_argc = static_cast<int>(argv.size());
  args.command_line_argv = &argv[0];
  args.platform_message_callback = OnFlutterPlatformMessage;
  args.custom_task_runners = &custom_task_runners;
#ifndef TIZEN_RENDERER_EVAS_GL
  if (IsHeaded()) {
    args.vsync_callback = [](void* user_data, intptr_t baton) -> void {
      reinterpret_cast<FlutterTizenEngine*>(user_data)
          ->tizen_vsync_waiter_->AsyncWaitForVsync(baton);
    };
  }
#endif

  if (FlutterEngineRunsAOTCompiledDartCode()) {
    aot_data_ = LoadAotData(engine_properties.aot_library_path);
    if (!aot_data_) {
      FT_LOGE("Unable to start engine without AOT data.");
      return false;
    }
    args.aot_data = aot_data_.get();
  }

  FlutterRendererConfig renderer_config = GetRendererConfig();

  auto result = FlutterEngineRun(FLUTTER_ENGINE_VERSION, &renderer_config,
                                 &args, this, &flutter_engine);
  if (result == kSuccess && flutter_engine != nullptr) {
    FT_LOGD("FlutterEngineRun Success!");
  } else {
    FT_LOGE("FlutterEngineRun Failure! result: %d", result);
    return false;
  }

  internal_plugin_registrar_ =
      std::make_unique<flutter::PluginRegistrar>(plugin_registrar_.get());

  platform_channel = std::make_unique<PlatformChannel>(
      internal_plugin_registrar_->messenger(), renderer.get());
  settings_channel = std::make_unique<SettingsChannel>(
      internal_plugin_registrar_->messenger());
  localization_channel = std::make_unique<LocalizationChannel>(flutter_engine);
  localization_channel->SendLocales();
  lifecycle_channel = std::make_unique<LifecycleChannel>(flutter_engine);

  if (IsHeaded()) {
    auto texture_registrar = std::make_unique<FlutterTextureRegistrar>();
    texture_registrar->flutter_engine = flutter_engine;
    plugin_registrar_->texture_registrar = std::move(texture_registrar);

    key_event_channel = std::make_unique<KeyEventChannel>(
        internal_plugin_registrar_->messenger());
    navigation_channel = std::make_unique<NavigationChannel>(
        internal_plugin_registrar_->messenger());
    text_input_channel = std::make_unique<TextInputChannel>(
        internal_plugin_registrar_->messenger(), this);
    platform_view_channel = std::make_unique<PlatformViewChannel>(
        internal_plugin_registrar_->messenger(), this);
    key_event_handler_ = std::make_unique<KeyEventHandler>(this);
    touch_event_handler_ = std::make_unique<TouchEventHandler>(this);

    SetWindowOrientation(0);
  }

  return true;
}

bool FlutterTizenEngine::StopEngine() {
  if (flutter_engine) {
    if (platform_view_channel) {
      platform_view_channel->Dispose();
    }
    if (plugin_registrar_destruction_callback_) {
      plugin_registrar_destruction_callback_(plugin_registrar_.get());
    }
    FlutterEngineResult result = FlutterEngineShutdown(flutter_engine);
    flutter_engine = nullptr;
    return (result == kSuccess);
  }
  return false;
}

FlutterDesktopPluginRegistrarRef FlutterTizenEngine::GetPluginRegistrar() {
  return plugin_registrar_.get();
}

void FlutterTizenEngine::SetPluginRegistrarDestructionCallback(
    FlutterDesktopOnPluginRegistrarDestroyed callback) {
  plugin_registrar_destruction_callback_ = callback;
}

void FlutterTizenEngine::SendWindowMetrics(int32_t width,
                                           int32_t height,
                                           double pixel_ratio) {
  FlutterWindowMetricsEvent event;
  event.struct_size = sizeof(FlutterWindowMetricsEvent);
  event.width = width;
  event.height = height;
  if (pixel_ratio == 0.0) {
    // The scale factor is computed based on the display DPI and the current
    // profile. A fixed DPI value (72) is used on TVs. See:
    // https://docs.tizen.org/application/native/guides/ui/efl/multiple-screens
    double dpi = 72.0;
    if (renderer && device_profile != DeviceProfile::kTV) {
      dpi = (double)renderer->GetDpi();
    }
    double profile_factor = 1.0;
    if (device_profile == DeviceProfile::kWearable) {
      profile_factor = 0.4;
    } else if (device_profile == DeviceProfile::kMobile) {
      profile_factor = 0.7;
    } else if (device_profile == DeviceProfile::kTV) {
      profile_factor = 2.0;
    }
    double scale_factor = dpi / 90.0 * profile_factor;
    event.pixel_ratio = std::max(scale_factor, 1.0);
  } else {
    event.pixel_ratio = pixel_ratio;
  }
  FlutterEngineSendWindowMetricsEvent(flutter_engine, &event);
}

// This must be called at least once in order to initialize the value of
// transformation_.
void FlutterTizenEngine::SetWindowOrientation(int32_t degree) {
  if (!renderer) {
    return;
  }

  renderer->SetRotate(degree);
  // Compute renderer transformation based on the angle of rotation.
  double rad = (360 - degree) * M_PI / 180;
  auto geometry = renderer->GetGeometry();
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
  text_input_channel->rotation = degree;
  if (degree == 90 || degree == 270) {
    renderer->ResizeWithRotation(geometry.x, geometry.y, height, width, degree);
    SendWindowMetrics(height, width, 0.0);
  } else {
    renderer->ResizeWithRotation(geometry.x, geometry.y, width, height, degree);
    SendWindowMetrics(width, height, 0.0);
  }
}

void FlutterTizenEngine::OnOrientationChange(int32_t degree) {
  SetWindowOrientation(degree);
}

// The Flutter Engine calls out to this function when new platform messages are
// available.
void FlutterTizenEngine::OnFlutterPlatformMessage(
    const FlutterPlatformMessage* engine_message,
    void* user_data) {
  if (engine_message->struct_size != sizeof(FlutterPlatformMessage)) {
    FT_LOGE("Invalid message size received. Expected: %zu, but received %zu",
            sizeof(FlutterPlatformMessage), engine_message->struct_size);
    return;
  }
  auto engine = reinterpret_cast<FlutterTizenEngine*>(user_data);
  auto message = engine->ConvertToDesktopMessage(*engine_message);
  engine->message_dispatcher->HandleMessage(message);
}

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
          ->renderer->OnMakeCurrent();
    };
    config.open_gl.make_resource_current = [](void* user_data) -> bool {
      return reinterpret_cast<FlutterTizenEngine*>(user_data)
          ->renderer->OnMakeResourceCurrent();
    };
    config.open_gl.clear_current = [](void* user_data) -> bool {
      return reinterpret_cast<FlutterTizenEngine*>(user_data)
          ->renderer->OnClearCurrent();
    };
    config.open_gl.present = [](void* user_data) -> bool {
      return reinterpret_cast<FlutterTizenEngine*>(user_data)
          ->renderer->OnPresent();
    };
    config.open_gl.fbo_callback = [](void* user_data) -> uint32_t {
      return reinterpret_cast<FlutterTizenEngine*>(user_data)
          ->renderer->OnGetFBO();
    };
    config.open_gl.surface_transformation =
        [](void* user_data) -> FlutterTransformation {
      return reinterpret_cast<FlutterTizenEngine*>(user_data)->transformation_;
    };
    config.open_gl.gl_proc_resolver = [](void* user_data,
                                         const char* name) -> void* {
      return reinterpret_cast<FlutterTizenEngine*>(user_data)
          ->renderer->OnProcResolver(name);
    };
    config.open_gl.gl_external_texture_frame_callback =
        [](void* user_data, int64_t texture_id, size_t width, size_t height,
           FlutterOpenGLTexture* texture) -> bool {
      auto engine = reinterpret_cast<FlutterTizenEngine*>(user_data);
      auto texture_registrar =
          engine->plugin_registrar_->texture_registrar.get();
      std::lock_guard<std::mutex> lock(texture_registrar->mutex);
      auto it = texture_registrar->textures.find(texture_id);
      int ret = false;
      if (it != texture_registrar->textures.end()) {
        ret = it->second->PopulateTextureWithIdentifier(width, height, texture);
      }
      return ret;
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
