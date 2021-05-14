// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/flutter_tizen_texture_registrar.h"

#include <iostream>
#include <mutex>

#include "flutter/shell/platform/tizen/external_texture_pixel_gl.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/tizen_log.h"

FlutterTizenTextureRegistrar::FlutterTizenTextureRegistrar(
    FlutterTizenEngine* engine)
    : engine_(engine) {}

int64_t FlutterTizenTextureRegistrar::RegisterTexture(
    const FlutterDesktopTextureInfo* texture_info) {
  if (texture_info->type != kFlutterDesktopPixelBufferTexture &&
      texture_info->type != kFlutterDesktopGpuBufferTexture) {
    FT_LOGE("Attempted to register texture of unsupport type.");
    return -1;
  }
  std::unique_ptr<ExternalTexture> texture_gl;
  if (texture_info->type == kFlutterDesktopPixelBufferTexture) {
    if (!texture_info->pixel_buffer_config.callback) {
      FT_LOGE("Invalid pixel buffer texture callback.");
      return -1;
    }
    texture_gl = std::make_unique<ExternalTexturePixelGL>(
        texture_info->pixel_buffer_config.callback,
        texture_info->pixel_buffer_config.user_data);
  }

  if (texture_info->type == kFlutterDesktopGpuBufferTexture) {
    if (!texture_info->gpu_buffer_config.callback) {
      FT_LOGE("Invalid gpu buffer texture callback.");
      return -1;
    }
    texture_gl = std::make_unique<ExternalTextureGL>(
        texture_info->gpu_buffer_config.callback,
        texture_info->gpu_buffer_config.destructionCallback,
        texture_info->gpu_buffer_config.user_data);
  }

  int64_t texture_id = texture_gl->TextureId();

  {
    std::lock_guard<std::mutex> lock(map_mutex_);
    textures_[texture_id] = std::move(texture_gl);
  }

  FlutterEngineRegisterExternalTexture(engine_->flutter_engine, texture_id);
  return texture_id;
}

bool FlutterTizenTextureRegistrar::UnregisterTexture(int64_t texture_id) {
  {
    std::lock_guard<std::mutex> lock(map_mutex_);
    auto it = textures_.find(texture_id);
    if (it == textures_.end()) {
      return false;
    }
    textures_.erase(it);
  }

  return FlutterEngineUnregisterExternalTexture(engine_->flutter_engine,
                                                texture_id) == kSuccess;
}

bool FlutterTizenTextureRegistrar::MarkTextureFrameAvailable(
    int64_t texture_id) {
  return FlutterEngineMarkExternalTextureFrameAvailable(engine_->flutter_engine,
                                                        texture_id) == kSuccess;
}

bool FlutterTizenTextureRegistrar::PopulateTexture(
    int64_t texture_id, size_t width, size_t height,
    FlutterOpenGLTexture* opengl_texture) {
  ExternalTexture* texture;
  {
    std::lock_guard<std::mutex> lock(map_mutex_);
    auto it = textures_.find(texture_id);
    if (it == textures_.end()) {
      return false;
    }
    texture = it->second.get();
  }
  return texture->PopulateTexture(width, height, opengl_texture);
}
