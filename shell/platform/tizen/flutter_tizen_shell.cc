// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <unistd.h>

#include <string>
#include <vector>

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/public/flutter_tizen.h"

class FlutterApp {
 public:
  explicit FlutterApp() {}
  virtual ~FlutterApp() {}

  bool OnCreate() {
    printf("Launching a Flutter application...\n");

    FlutterDesktopWindowProperties window_prop = {};
    window_prop.headed = true;
    window_prop.x = 0;
    window_prop.y = 0;
    window_prop.width = window_width_;
    window_prop.height = window_height_;
    window_prop.transparent = false;
    window_prop.focusable = true;

    std::string assets_path = app_path_ + "/res/flutter_assets";
    std::string icu_data_path = app_path_ + "/res/icudtl.dat";
    std::string aot_lib_path = app_path_ + "/lib/libapp.so";

    std::vector<const char*> switches;
    switches.push_back("--disable-observatory");
    switches.push_back("--verbose-logging");
    switches.push_back("--enable-dart-profiling");
    switches.push_back("--enable-checked-mode");

    FlutterDesktopEngineProperties engine_prop = {};
    engine_prop.assets_path = assets_path.c_str();
    engine_prop.icu_data_path = icu_data_path.c_str();
    engine_prop.aot_library_path = aot_lib_path.c_str();
    engine_prop.switches = switches.data();
    engine_prop.switches_count = switches.size();
    engine_ = FlutterDesktopRunEngine(window_prop, engine_prop);

    if (!engine_) {
      printf("Could not launch a Flutter application.\n");
      return false;
    }

    return true;
  }

  void OnTerminate() {
    printf("Shutting down the application...\n");

    FlutterDesktopShutdownEngine(engine_);
    engine_ = nullptr;

    ecore_shutdown();
  }

  int Run(int argc, char** argv) {
    ecore_init();
    elm_init(0, nullptr);
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
    elm_config_accel_preference_set("opengl");

    for (int i = 1; i < argc; i++) {
      if (strstr(argv[i], "--width=") == argv[i]) {
        window_width_ = std::atoi(argv[i] + strlen("--width="));
      } else if (strstr(argv[i], "--height=") == argv[i]) {
        window_height_ = std::atoi(argv[i] + strlen("--height="));
      } else if (strstr(argv[i], "--tpkroot=") == argv[i]) {
        app_path_ = argv[i] + strlen("--tpkroot=");
      }
    }

    if (app_path_.empty()) {
      char path[256];
      getcwd(path, sizeof(path));
      app_path_ = path + std::string("/tpkroot");
    }

    ecore_idler_add(
        [](void* data) -> Eina_Bool {
          FlutterApp* app = (FlutterApp*)data;
          app->OnCreate();
          return ECORE_CALLBACK_CANCEL;
        },
        this);
    ecore_main_loop_begin();
    OnTerminate();

    return 0;
  }

  FlutterDesktopPluginRegistrarRef GetRegistrarForPlugin(
      const std::string& plugin_name) {
    if (engine_) {
      return FlutterDesktopGetPluginRegistrar(engine_, plugin_name.c_str());
    }
    return nullptr;
  }

 private:
  std::string app_path_ = {};
  int32_t window_width_ = 0;
  int32_t window_height_ = 0;

  FlutterDesktopEngineRef engine_ = nullptr;
};

int main(int argc, char* argv[]) {
  auto app = new FlutterApp();
  return app->Run(argc, argv);
}
