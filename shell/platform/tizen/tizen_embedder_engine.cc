// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_embedder_engine.h"

#include <system_info.h>

#include <filesystem>
#include <string>
#include <vector>

#include "flutter/shell/platform/tizen/tizen_log.h"

// Unique number associated with platform tasks.
static constexpr size_t kPlatformTaskRunnerIdentifier = 1;

static std::string GetDeviceProfile() {
  char* feature_profile;
  system_info_get_platform_string("http://tizen.org/feature/profile",
                                  &feature_profile);
  std::string profile(feature_profile);
  free(feature_profile);
  return profile;
}

static double GetDeviceDpi() {
  int feature_dpi;
  system_info_get_platform_int("http://tizen.org/feature/screen.dpi",
                               &feature_dpi);
  return (double)feature_dpi;
}

TizenEmbedderEngine::TizenEmbedderEngine(
    const FlutterWindowProperties& window_properties)
    : device_profile(GetDeviceProfile()), device_dpi(GetDeviceDpi()) {
#ifdef FLUTTER_TIZEN_4
  tizen_renderer = std::make_unique<TizenRendererEcoreWl>(
      *this, window_properties.x, window_properties.y, window_properties.width,
      window_properties.height);
#else
  tizen_renderer = std::make_unique<TizenRendererEcoreWl2>(
      *this, window_properties.x, window_properties.y, window_properties.width,
      window_properties.height);
#endif

  // Run flutter task on Tizen main loop.
  // Tizen engine has four threads (GPU thread, UI thread, IO thread, platform
  // thread). UI threads need to send flutter task to platform thread.
  event_loop_ = std::make_unique<TizenEventLoop>(
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

  tizen_vsync_waiter_ = std::make_unique<TizenVsyncWaiter>(this);
}

TizenEmbedderEngine::~TizenEmbedderEngine() {
  FT_LOGD("Destroy");
  tizen_renderer = nullptr;
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

bool TizenEmbedderEngine::RunEngine(
    const FlutterEngineProperties& engine_properties) {
  if (!tizen_renderer->IsValid()) {
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

  // Configure task runner interop.
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

  FlutterRendererConfig config = {};
  config.type = kOpenGL;
  config.open_gl.struct_size = sizeof(config.open_gl);
  config.open_gl.make_current = MakeContextCurrent;
  config.open_gl.make_resource_current = MakeResourceCurrent;
  config.open_gl.clear_current = ClearContext;
  config.open_gl.present = Present;
  config.open_gl.fbo_callback = GetActiveFbo;
  config.open_gl.surface_transformation = Transformation;
  config.open_gl.gl_proc_resolver = GlProcResolver;
  config.open_gl.gl_external_texture_frame_callback = OnAcquireExternalTexture;

  FlutterProjectArgs args = {};
  args.struct_size = sizeof(FlutterProjectArgs);
  args.assets_path = engine_properties.assets_path;
  args.icu_data_path = engine_properties.icu_data_path;
  args.command_line_argc = static_cast<int>(argv.size());
  args.command_line_argv = &argv[0];
  args.platform_message_callback = OnFlutterPlatformMessage;
  args.custom_task_runners = &custom_task_runners;
  args.vsync_callback = OnVsyncCallback;

  if (FlutterEngineRunsAOTCompiledDartCode()) {
    aot_data_ = LoadAotData(engine_properties.aot_library_path);
    if (!aot_data_) {
      FT_LOGE("Unable to start engine without AOT data.");
      return false;
    }
    args.aot_data = aot_data_.get();
  }

  auto result = FlutterEngineRun(FLUTTER_ENGINE_VERSION, &config, &args, this,
                                 &flutter_engine);
  if (result == kSuccess && flutter_engine != nullptr) {
    FT_LOGD("FlutterEngineRun Success!");
  } else {
    FT_LOGE("FlutterEngineRun Failure! result: %d", result);
    return false;
  }

  std::unique_ptr<FlutterTextureRegistrar> textures =
      std::make_unique<FlutterTextureRegistrar>();
  textures->flutter_engine = flutter_engine;
  plugin_registrar_ = std::make_unique<FlutterDesktopPluginRegistrar>();
  plugin_registrar_->engine = this;
  plugin_registrar_->texture_registrar = std::move(textures);

  internal_plugin_registrar_ =
      std::make_unique<flutter::PluginRegistrar>(plugin_registrar_.get());

  key_event_channel = std::make_unique<KeyEventChannel>(
      internal_plugin_registrar_->messenger());
  navigation_channel = std::make_unique<NavigationChannel>(
      internal_plugin_registrar_->messenger());
  platform_channel = std::make_unique<PlatformChannel>(
      internal_plugin_registrar_->messenger());
  settings_channel = std::make_unique<SettingsChannel>(
      internal_plugin_registrar_->messenger());
  text_input_channel = std::make_unique<TextInputChannel>(
      internal_plugin_registrar_->messenger(), this);
  localization_channel = std::make_unique<LocalizationChannel>(flutter_engine);
  localization_channel->SendLocales();
  lifecycle_channel = std::make_unique<LifecycleChannel>(flutter_engine);
  platform_view_channel = std::make_unique<PlatformViewChannel>(
      internal_plugin_registrar_->messenger(), this);

  key_event_handler_ = std::make_unique<KeyEventHandler>(this);
  touch_event_handler_ = std::make_unique<TouchEventHandler>(this);

  SetWindowOrientation(0);
  return true;
}

void TizenEmbedderEngine::OnRotationChange(int angle) {
  SetWindowOrientation(angle);
}

bool TizenEmbedderEngine::StopEngine() {
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

FlutterDesktopPluginRegistrarRef TizenEmbedderEngine::GetPluginRegistrar() {
  return plugin_registrar_.get();
}

void TizenEmbedderEngine::SetPluginRegistrarDestructionCallback(
    FlutterDesktopOnPluginRegistrarDestroyed callback) {
  plugin_registrar_destruction_callback_ = callback;
}

bool TizenEmbedderEngine::OnAcquireExternalTexture(
    void* user_data, int64_t texture_id, size_t width, size_t height,
    FlutterOpenGLTexture* texture) {
  TizenEmbedderEngine* tizen_embedder_engine =
      reinterpret_cast<TizenEmbedderEngine*>(user_data);
  return tizen_embedder_engine->plugin_registrar_->texture_registrar
      ->textures[texture_id]
      ->PopulateTextureWithIdentifier(width, height, texture);
}

void TizenEmbedderEngine::SendWindowMetrics(int32_t width, int32_t height,
                                            double pixel_ratio) {
  FlutterWindowMetricsEvent event;
  event.struct_size = sizeof(FlutterWindowMetricsEvent);
  event.width = width;
  event.height = height;
  if (pixel_ratio == 0.0) {
    // The scale factor is computed based on the display DPI and the current
    // profile. A fixed DPI value (72) is used on TVs. See:
    // https://docs.tizen.org/application/native/guides/ui/efl/multiple-screens
    double profile_factor = 1.0;
    if (device_profile == "wearable") {
      profile_factor = 0.4;
    } else if (device_profile == "mobile") {
      profile_factor = 0.7;
    } else if (device_profile == "tv") {
      profile_factor = 2.0;
    }
    double dpi = device_profile == "tv" ? 72.0 : device_dpi;
    double scale_factor = dpi / 90.0 * profile_factor;
    event.pixel_ratio = std::max(scale_factor, 1.0);
  } else {
    event.pixel_ratio = pixel_ratio;
  }
  FlutterEngineSendWindowMetricsEvent(flutter_engine, &event);
}

// This must be called at least once in order to initialize the value of
// transformation_.
void TizenEmbedderEngine::SetWindowOrientation(int32_t degree) {
  if (!tizen_renderer) {
    return;
  }

  tizen_renderer->SetRotate(degree);
  // Compute renderer transformation based on the angle of rotation.
  double rad = (360 - degree) * M_PI / 180;
  auto geometry = tizen_renderer->GetGeometry();
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
    tizen_renderer->ResizeWithRotation(geometry.x, geometry.y, height, width,
                                       degree);
    SendWindowMetrics(height, width, 0.0);
  } else {
    tizen_renderer->ResizeWithRotation(geometry.x, geometry.y, width, height,
                                       degree);
    SendWindowMetrics(width, height, 0.0);
  }
}

void TizenEmbedderEngine::SendLocales() { localization_channel->SendLocales(); }

void TizenEmbedderEngine::AppIsInactive() {
  lifecycle_channel->AppIsInactive();
}

void TizenEmbedderEngine::AppIsResumed() { lifecycle_channel->AppIsResumed(); }

void TizenEmbedderEngine::AppIsPaused() { lifecycle_channel->AppIsPaused(); }

void TizenEmbedderEngine::AppIsDetached() {
  lifecycle_channel->AppIsDetached();
}

void TizenEmbedderEngine::OnFlutterPlatformMessage(
    const FlutterPlatformMessage* engine_message, void* user_data) {
  if (engine_message->struct_size != sizeof(FlutterPlatformMessage)) {
    FT_LOGE("Invalid message size received. Expected: %zu, received %zu",
            sizeof(FlutterPlatformMessage), engine_message->struct_size);
    return;
  }
  FT_LOGD("%s", engine_message->channel);
  TizenEmbedderEngine* tizen_embedder_engine =
      reinterpret_cast<TizenEmbedderEngine*>(user_data);
  auto message =
      tizen_embedder_engine->ConvertToDesktopMessage(*engine_message);
  tizen_embedder_engine->message_dispatcher->HandleMessage(message);
}

void TizenEmbedderEngine::OnVsyncCallback(void* user_data, intptr_t baton) {
  TizenEmbedderEngine* tizen_embedder_engine =
      reinterpret_cast<TizenEmbedderEngine*>(user_data);
  tizen_embedder_engine->tizen_vsync_waiter_->AsyncWaitForVsync(baton);
}

// Converts a FlutterPlatformMessage to an equivalent FlutterDesktopMessage.
FlutterDesktopMessage TizenEmbedderEngine::ConvertToDesktopMessage(
    const FlutterPlatformMessage& engine_message) {
  FlutterDesktopMessage message = {};
  message.struct_size = sizeof(message);
  message.channel = engine_message.channel;
  message.message = engine_message.message;
  message.message_size = engine_message.message_size;
  message.response_handle = engine_message.response_handle;
  return message;
}

bool TizenEmbedderEngine::MakeContextCurrent(void* user_data) {
  return reinterpret_cast<TizenEmbedderEngine*>(user_data)
      ->tizen_renderer->OnMakeCurrent();
}

bool TizenEmbedderEngine::ClearContext(void* user_data) {
  return reinterpret_cast<TizenEmbedderEngine*>(user_data)
      ->tizen_renderer->OnClearCurrent();
}

bool TizenEmbedderEngine::Present(void* user_data) {
  return reinterpret_cast<TizenEmbedderEngine*>(user_data)
      ->tizen_renderer->OnPresent();
}

bool TizenEmbedderEngine::MakeResourceCurrent(void* user_data) {
  return reinterpret_cast<TizenEmbedderEngine*>(user_data)
      ->tizen_renderer->OnMakeResourceCurrent();
}

uint32_t TizenEmbedderEngine::GetActiveFbo(void* user_data) {
  return reinterpret_cast<TizenEmbedderEngine*>(user_data)
      ->tizen_renderer->OnGetFBO();
}

FlutterTransformation TizenEmbedderEngine::Transformation(void* user_data) {
  return reinterpret_cast<TizenEmbedderEngine*>(user_data)->transformation_;
}

void* TizenEmbedderEngine::GlProcResolver(void* user_data, const char* name) {
  return reinterpret_cast<TizenEmbedderEngine*>(user_data)
      ->tizen_renderer->OnProcResolver(name);
}
