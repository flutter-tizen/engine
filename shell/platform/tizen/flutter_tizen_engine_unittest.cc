// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"

#include "flutter/shell/platform/embedder/test_utils/proc_table_replacement.h"
#include "flutter/shell/platform/tizen/testing/engine_modifier.h"
#include "gtest/gtest.h"

namespace flutter {
namespace testing {

class FlutterTizenEngineTest : public ::testing::Test {
 public:
  FlutterTizenEngineTest() {
    ecore_init();
    elm_init(0, nullptr);
  }

 protected:
  void SetUp() {
    FlutterDesktopEngineProperties engine_prop = {};
    engine_prop.assets_path = "/foo/flutter_assets";
    engine_prop.icu_data_path = "/foo/icudtl.dat";
    engine_prop.aot_library_path = "/foo/libapp.so";

    FlutterProjectBundle project(engine_prop);
    auto engine = std::make_unique<FlutterTizenEngine>(project);
    engine_ = engine.release();
  }

  void TearDown() {
    if (engine_) {
      delete engine_;
    }
    engine_ = nullptr;
  }

  FlutterTizenEngine* engine_;
};

class FlutterTizenEngineTestHeaded : public FlutterTizenEngineTest {
 protected:
  void SetUp() {
    FlutterTizenEngineTest::SetUp();
    engine_->InitializeRenderer(0, 0, 800, 600, false, true);
  }
};

TEST_F(FlutterTizenEngineTest, Run) {
  EXPECT_TRUE(engine_ != nullptr);
  EXPECT_TRUE(engine_->RunEngine());
}

TEST_F(FlutterTizenEngineTest, Run_Twice) {
  EXPECT_TRUE(engine_->RunEngine());
  EXPECT_FALSE(engine_->RunEngine());
}

TEST_F(FlutterTizenEngineTest, Stop) {
  EXPECT_TRUE(engine_->RunEngine());
  EXPECT_TRUE(engine_->StopEngine());
}

TEST_F(FlutterTizenEngineTest, Stop_Twice) {
  EXPECT_TRUE(engine_->RunEngine());
  EXPECT_TRUE(engine_->StopEngine());
  EXPECT_FALSE(engine_->StopEngine());
}

TEST_F(FlutterTizenEngineTest, GetPluginRegistrar) {
  EXPECT_TRUE(engine_->RunEngine());
  EXPECT_TRUE(engine_->GetPluginRegistrar() != nullptr);
}

TEST_F(FlutterTizenEngineTest, GetTextureRegistrar) {
  EXPECT_TRUE(engine_->RunEngine());
  EXPECT_TRUE(engine_->GetTextureRegistrar() == nullptr);
}

TEST_F(FlutterTizenEngineTestHeaded, GetTextureRegistrar) {
  EXPECT_TRUE(engine_->RunEngine());
  EXPECT_TRUE(engine_->GetTextureRegistrar() != nullptr);
}

TEST_F(FlutterTizenEngineTest, RunDoesExpectedInitialization) {
  EngineModifier modifier(engine_);
  bool run_called = false;
  modifier.embedder_api().Run = MOCK_ENGINE_PROC(
      Run, ([&run_called, engine_instance = engine_](
                size_t version, const FlutterRendererConfig* config,
                const FlutterProjectArgs* args, void* user_data,
                FLUTTER_API_SYMBOL(FlutterEngine) * engine_out) {
        run_called = true;
        *engine_out = reinterpret_cast<FLUTTER_API_SYMBOL(FlutterEngine)>(1);

        EXPECT_EQ(version, (size_t)FLUTTER_ENGINE_VERSION);
        EXPECT_NE(config, nullptr);
        EXPECT_EQ(user_data, engine_instance);

        EXPECT_STREQ(args->assets_path, "/foo/flutter_assets");
        EXPECT_STREQ(args->icu_data_path, "/foo/icudtl.dat");
        EXPECT_EQ(args->dart_entrypoint_argc, 0);
        EXPECT_NE(args->platform_message_callback, nullptr);
        EXPECT_NE(args->custom_task_runners, nullptr);
        EXPECT_EQ(args->custom_dart_entrypoint, nullptr);

        return kSuccess;
      }));

  // It should send locale info.
  bool update_locales_called = false;
  modifier.embedder_api().UpdateLocales = MOCK_ENGINE_PROC(
      UpdateLocales,
      ([&update_locales_called](auto engine, const FlutterLocale** locales,
                                size_t locales_count) {
        update_locales_called = true;
        EXPECT_GT(locales_count, (size_t)0);
        EXPECT_NE(locales, nullptr);

        return kSuccess;
      }));

  // And it should send initial settings info.
  bool settings_message_sent = false;
  modifier.embedder_api().SendPlatformMessage = MOCK_ENGINE_PROC(
      SendPlatformMessage,
      ([&settings_message_sent](auto engine, auto message) {
        if (std::string(message->channel) == std::string("flutter/settings")) {
          settings_message_sent = true;
        }

        return kSuccess;
      }));

  engine_->RunEngine();

  EXPECT_TRUE(run_called);
  EXPECT_TRUE(update_locales_called);
  EXPECT_TRUE(settings_message_sent);

  modifier.embedder_api().Shutdown = [](auto engine) { return kSuccess; };
}

}  // namespace testing
}  // namespace flutter
