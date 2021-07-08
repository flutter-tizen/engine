// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_renderer.h"

namespace flutter {

TizenRenderer::TizenRenderer(WindowGeometry geometry, Delegate& delegate)
    : initial_geometry_(geometry), delegate_(delegate) {}

TizenRenderer::~TizenRenderer() = default;

}  // namespace flutter
