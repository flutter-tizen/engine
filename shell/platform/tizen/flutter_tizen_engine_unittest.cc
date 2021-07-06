// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "gtest/gtest.h"

namespace flutter {
namespace testing {

void efl_init() {
  ecore_init();
  elm_init(0, 0);
}

class FlutterTizenEngineTestSimple : public ::testing::Test {
 protected:
  void SetUp() { efl_init(); }
};

TEST_F(FlutterTizenEngineTestSimple, Create_Headless) {
  flutter::FlutterTizenEngine* tizen_engine =
      new flutter::FlutterTizenEngine(false);
  EXPECT_TRUE(tizen_engine != nullptr);
  delete tizen_engine;
}

TEST_F(FlutterTizenEngineTestSimple, Create_Headed) {
  flutter::FlutterTizenEngine* tizen_engine =
      new flutter::FlutterTizenEngine(true);
  EXPECT_TRUE(tizen_engine != nullptr);
  delete tizen_engine;
}

class FlutterTizenEngineTest : public ::testing::Test {
 public:
  FlutterTizenEngineTest() {
    efl_init();

    std::string tpk_root;
    char path[256];
    EXPECT_TRUE(getcwd(path, sizeof(path)) != NULL);
    tpk_root = path + std::string("/tpkroot");

    assets_path_ = tpk_root + "/res/flutter_assets";
    icu_data_path_ = tpk_root + "/res/icudtl.dat";
    aot_lib_path_ = tpk_root + "/lib/libapp.so";

    switches_.push_back("--disable-observatory");
  }

  void init() {
    engine_prop_.assets_path = assets_path_.c_str();
    engine_prop_.icu_data_path = icu_data_path_.c_str();
    engine_prop_.aot_library_path = aot_lib_path_.c_str();
    engine_prop_.switches = switches_.data();
    engine_prop_.switches_count = switches_.size();
  }

 protected:
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

class FlutterTizenEngineHeadless : public FlutterTizenEngineTest {
 protected:
  void SetUp() {
    FlutterTizenEngineTest::init();
    auto engine = std::make_unique<flutter::FlutterTizenEngine>(false);
    engine_ = engine.release();
  }
};

TEST_F(FlutterTizenEngineHeadless, Run) {
  EXPECT_TRUE(engine_ != nullptr);
  EXPECT_TRUE(engine_->RunEngine(engine_prop_));
}

TEST_F(FlutterTizenEngineHeadless, Run_Twice) {
  EXPECT_TRUE(engine_ != nullptr);
  EXPECT_TRUE(engine_->RunEngine(engine_prop_));
  EXPECT_FALSE(engine_->RunEngine(engine_prop_));
}

TEST_F(FlutterTizenEngineHeadless, Stop) {
  EXPECT_TRUE(engine_ != nullptr);
  EXPECT_TRUE(engine_->RunEngine(engine_prop_));
  EXPECT_TRUE(engine_->StopEngine());
}

TEST_F(FlutterTizenEngineHeadless, Stop_Twice) {
  EXPECT_TRUE(engine_ != nullptr);
  EXPECT_TRUE(engine_->RunEngine(engine_prop_));
  EXPECT_TRUE(engine_->StopEngine());
  EXPECT_FALSE(engine_->StopEngine());
}

TEST_F(FlutterTizenEngineHeadless, GetPluginRegistrar) {
  EXPECT_TRUE(engine_ != nullptr);
  EXPECT_TRUE(engine_->RunEngine(engine_prop_));
  EXPECT_TRUE(engine_->GetPluginRegistrar() != nullptr);
}

TEST_F(FlutterTizenEngineHeadless, GetTextureRegistrar) {
  EXPECT_TRUE(engine_ != nullptr);
  EXPECT_TRUE(engine_->RunEngine(engine_prop_));
  EXPECT_TRUE(engine_->GetTextureRegistrar() == nullptr);
}

class FlutterTizenEngineHeaded : public FlutterTizenEngineTest {
 protected:
  void SetUp() {
    FlutterTizenEngineTest::init();
    auto engine = std::make_unique<flutter::FlutterTizenEngine>(true);
    engine_ = engine.release();
  }
};

TEST_F(FlutterTizenEngineHeaded, Run) {
  EXPECT_TRUE(engine_ != nullptr);
  EXPECT_TRUE(engine_->RunEngine(engine_prop_));
}

TEST_F(FlutterTizenEngineHeaded, Run_Twice) {
  EXPECT_TRUE(engine_ != nullptr);
  EXPECT_TRUE(engine_->RunEngine(engine_prop_));
  EXPECT_FALSE(engine_->RunEngine(engine_prop_));
}

TEST_F(FlutterTizenEngineHeaded, Stop) {
  EXPECT_TRUE(engine_ != nullptr);
  EXPECT_TRUE(engine_->RunEngine(engine_prop_));
  EXPECT_TRUE(engine_->StopEngine());
}

TEST_F(FlutterTizenEngineHeaded, Stop_Twice) {
  EXPECT_TRUE(engine_ != nullptr);
  EXPECT_TRUE(engine_->RunEngine(engine_prop_));
  EXPECT_TRUE(engine_->StopEngine());
  EXPECT_FALSE(engine_->StopEngine());
}

TEST_F(FlutterTizenEngineHeaded, GetPluginRegistrar) {
  EXPECT_TRUE(engine_ != nullptr);
  EXPECT_TRUE(engine_->RunEngine(engine_prop_));
  EXPECT_TRUE(engine_->GetPluginRegistrar() != nullptr);
}

TEST_F(FlutterTizenEngineHeaded, GetTextureRegistrar) {
  EXPECT_TRUE(engine_ != nullptr);
  EXPECT_TRUE(engine_->RunEngine(engine_prop_));
  EXPECT_TRUE(engine_->GetTextureRegistrar() != nullptr);
}

}  // namespace testing
}  // namespace flutter
