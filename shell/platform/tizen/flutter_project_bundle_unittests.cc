// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/flutter_project_bundle.h"
#include "gtest/gtest.h"

namespace flutter {
namespace testing {

TEST(FlutterProjectBundle, BasicPropertiesAbsolutePaths) {
  FlutterDesktopEngineProperties properties = {};
  properties.assets_path = "/foo/flutter_assets";
  properties.icu_data_path = "/foo/icudtl.dat";

  FlutterProjectBundle project(properties);

  EXPECT_TRUE(project.HasValidPaths());
  EXPECT_EQ(project.assets_path().string(), "/foo/flutter_assets");
  EXPECT_EQ(project.icu_path().string(), "/foo/icudtl.dat");
}

TEST(FlutterProjectBundle, BasicPropertiesRelativePaths) {
  FlutterDesktopEngineProperties properties = {};
  properties.assets_path = "foo/flutter_assets";
  properties.icu_data_path = "foo/icudtl.dat";

  FlutterProjectBundle project(properties);

  EXPECT_TRUE(project.HasValidPaths());
  EXPECT_TRUE(project.assets_path().is_absolute());
  EXPECT_EQ(project.assets_path().filename().string(), "flutter_assets");
  EXPECT_TRUE(project.icu_path().is_absolute());
  EXPECT_EQ(project.icu_path().filename().string(), "icudtl.dat");
}

}  // namespace testing
}  // namespace flutter
