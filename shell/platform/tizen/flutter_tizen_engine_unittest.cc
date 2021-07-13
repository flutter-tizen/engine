// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
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
    engine_prop.assets_path = "foo/flutter_assets";
    engine_prop.icu_data_path = "foo/icudtl.dat";
    engine_prop.aot_library_path = "foo/libapp.so";

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

}  // namespace testing
}  // namespace flutter
