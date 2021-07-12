// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "external_texture_surface_gl.h"

#include "flutter/shell/platform/common/public/flutter_texture_registrar.h"
#include "flutter/shell/platform/tizen/tizen_log.h"

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
  state_.release();
}

bool ExternalTextureSurfaceGL::PopulateTexture(
    size_t width,
    size_t height,
    FlutterOpenGLTexture* opengl_texture) {
  if (!texture_callback_) {
    return false;
  }

  const FlutterDesktopGpuBuffer* gpu_buffer =
      texture_callback_(width, height, user_data_);
  if (!gpu_buffer) {
    FT_LOGI("[texture id:%ld] gpu_buffer is null", texture_id_);
    return false;
  }
  if (!gpu_buffer->buffer) {
    FT_LOGI("[texture id:%ld] tbm_surface_ is null", texture_id_);
    return false;
  }
  FT_UNIMPLEMENTED();
  return false;
}

void ExternalTextureSurfaceGL::OnDestruction() {
  if (destruction_callback_) {
    destruction_callback_(user_data_);
  }
}

}  // namespace flutter
