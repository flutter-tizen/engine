#include <assert.h>
#include <unistd.h>
#include <chrono>
#include <iostream>
#include <thread>

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/public/flutter_tizen.h"

extern int gApp_width;
extern int gApp_height;

std::string TPK_ROOT_PATH = "/tpkroot";
std::string LIB_PATH = "/lib";
std::string RES_PATH = "/res";

class FlutterApp {
 public:
  explicit FlutterApp() {}
  virtual ~FlutterApp() {}

  bool OnCreate() {
    printf("Launching a Flutter application...\n");
    std::string assets_path;
    std::string icu_data_path;
    std::string aot_lib_path;
    FlutterDesktopEngineProperties engine_prop = {};
    std::vector<const char*> switches;

    std::string tpk_root;
    if (app_path_.empty()) {
      char path[256];
      getcwd(path, sizeof(path));
      tpk_root = path + std::string("/tpkroot");
    } else {
      tpk_root = app_path_;
    }

    assets_path = tpk_root + "/res/flutter_assets";
    icu_data_path = tpk_root + "/res/icudtl.dat";
    aot_lib_path = tpk_root + "/lib/libapp.so";

    switches.push_back("--disable-observatory");
    switches.push_back("--verbose-logging");
    switches.push_back("--enable-dart-profiling");
    switches.push_back("--enable-checked-mode");

    engine_prop.assets_path = assets_path.c_str();
    engine_prop.icu_data_path = icu_data_path.c_str();
    engine_prop.aot_library_path = aot_lib_path.c_str();
    engine_prop.switches = switches.data();
    engine_prop.switches_count = switches.size();
    engine_ = FlutterDesktopRunEngine(engine_prop, true);

    if (!engine_) {
      printf("Could not launch a Flutter application.\n");
      return false;
    }
    // RegisterPlugins(this);
    return true;
  }

  void OnTerminate() {
    printf("Shutting down the application...");

    FlutterDesktopShutdownEngine(engine_);
    engine_ = nullptr;

    ecore_shutdown();
  }

  int Run(int argc, char** argv) {
    ecore_init();
    elm_init(0, 0);
    elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
    elm_config_accel_preference_set("opengl");

    for (int i = 1; i < argc; i++) {
      if (strstr(argv[i], "--width=") == argv[i]) {
        gApp_width = std::atoi(argv[i] + strlen("--width="));
      } else if (strstr(argv[i], "--height=") == argv[i]) {
        gApp_height = std::atoi(argv[i] + strlen("--height="));
      } else if (strstr(argv[i], "--tpkroot=") == argv[i]) {
        app_path_ = argv[i] + strlen("--tpkroot=");
      }
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
  FlutterDesktopEngineRef engine_ = nullptr;
};

int main(int argc, char* argv[]) {
  auto app = new FlutterApp();
  return app->Run(argc, argv);
}
