// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "gtest/gtest.h"

namespace flutter {
namespace testing {

class FlutterTizenEngineTestSimple : public ::testing::Test {
 protected:
  void SetUp() { ecore_init(); }
};

TEST_F(FlutterTizenEngineTestSimple, Create_Headless) {
  flutter::FlutterTizenEngine* tizen_engine =
      new flutter::FlutterTizenEngine(false);
  EXPECT_TRUE(tizen_engine != nullptr);
  delete tizen_engine;
}

// TODO
TEST_F(FlutterTizenEngineTestSimple, DISABLED_Create_Headed) {
  flutter::FlutterTizenEngine* tizen_engine =
      new flutter::FlutterTizenEngine(true);
  EXPECT_TRUE(tizen_engine != nullptr);
  delete tizen_engine;
}

class FlutterTizenEngineTest : public ::testing::Test {
 public:
  FlutterTizenEngineTest() {
    ecore_init();

    std::string tpk_root;
    char path[256];
    EXPECT_TRUE(getcwd(path, sizeof(path)) != NULL);
    tpk_root = path + std::string("/tpkroot");

    assets_path_ = tpk_root + "/res/flutter_assets";
    icu_data_path_ = tpk_root + "/res/icudtl.dat";
    aot_lib_path_ = tpk_root + "/lib/libapp.so";

    switches_.push_back("--disable-observatory");
  }

 protected:
  void SetUp() {
    engine_prop_.assets_path = assets_path_.c_str();
    engine_prop_.icu_data_path = icu_data_path_.c_str();
    engine_prop_.aot_library_path = aot_lib_path_.c_str();
    engine_prop_.switches = switches_.data();
    engine_prop_.switches_count = switches_.size();

    auto engine = std::make_unique<flutter::FlutterTizenEngine>(false);
    engine_ = engine.release();
  }

  void TearDown() {
    if (engine_) {
      delete engine_;
    }
    engine_ = nullptr;
  }

  std::string assets_path_;
  std::string icu_data_path_;
  std::string aot_lib_path_;
  flutter::FlutterTizenEngine* engine_;
  FlutterDesktopEngineProperties engine_prop_ = {};
  std::vector<const char*> switches_;
};

TEST_F(FlutterTizenEngineTest, Run) {
  EXPECT_TRUE(engine_ != nullptr);
  EXPECT_TRUE(engine_->RunEngine(engine_prop_));
  EXPECT_TRUE(true);
}

// TODO
TEST_F(FlutterTizenEngineTest, DISABLED_Run_Twice) {
  EXPECT_TRUE(engine_ != nullptr);
  EXPECT_TRUE(engine_->RunEngine(engine_prop_));
  EXPECT_FALSE(engine_->RunEngine(engine_prop_));
  EXPECT_TRUE(true);
}

TEST_F(FlutterTizenEngineTest, Stop) {
  EXPECT_TRUE(engine_ != nullptr);
  EXPECT_TRUE(engine_->RunEngine(engine_prop_));
  EXPECT_TRUE(engine_->StopEngine());
}

TEST_F(FlutterTizenEngineTest, Stop_Twice) {
  EXPECT_TRUE(engine_ != nullptr);
  EXPECT_TRUE(engine_->RunEngine(engine_prop_));
  EXPECT_TRUE(engine_->StopEngine());
  EXPECT_FALSE(engine_->StopEngine());
}

TEST_F(FlutterTizenEngineTest, GetPluginRegistrar) {
  EXPECT_TRUE(engine_ != nullptr);
  EXPECT_TRUE(engine_->GetPluginRegistrar() != nullptr);
}

// TODO
TEST_F(FlutterTizenEngineTest, DISABLED_GetTextureRegistrar) {
  EXPECT_TRUE(engine_ != nullptr);
  EXPECT_TRUE(engine_->GetTextureRegistrar() != nullptr);
}

}  // namespace testing
}  // namespace flutter
