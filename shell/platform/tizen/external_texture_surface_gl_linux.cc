// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#include "external_texture_surface_gl.h"
#pragma clang diagnostic pop

#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

ExternalTextureSurfaceGL::ExternalTextureSurfaceGL(
    FlutterDesktopGpuBufferTextureCallback texture_callback,
    FlutterDesktopGpuBufferDestructionCallback destruction_callback,
    void* user_data)
    : ExternalTexture(),
      texture_callback_(texture_callback),
      destruction_callback_(destruction_callback),
      user_data_(user_data) {}

ExternalTextureSurfaceGL::~ExternalTextureSurfaceGL() {
  FT_UNIMPLEMENTED();
}

bool ExternalTextureSurfaceGL::PopulateTexture(
    size_t width,
    size_t height,
    FlutterOpenGLTexture* opengl_texture) {
  FT_UNIMPLEMENTED();
  return false;
}

void ExternalTextureSurfaceGL::OnDestruction() {
  FT_UNIMPLEMENTED();
}

}  // namespace flutter
