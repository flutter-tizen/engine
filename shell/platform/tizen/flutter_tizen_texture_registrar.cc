// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_tizen_texture_registrar.h"

#include <iostream>
#include <mutex>

#include "flutter/shell/platform/tizen/external_texture_pixel_gl.h"
#include "flutter/shell/platform/tizen/external_texture_surface_gl.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

FlutterTizenTextureRegistrar::FlutterTizenTextureRegistrar(
    FlutterTizenEngine* engine)
    : engine_(engine) {}

int64_t FlutterTizenTextureRegistrar::RegisterTexture(
    const FlutterDesktopTextureInfo* texture_info) {
  if (texture_info->type != kFlutterDesktopPixelBufferTexture &&
      texture_info->type != kFlutterDesktopGpuBufferTexture) {
    FT_LOG(Error) << "Attempted to register texture of unsupport type.";
    return -1;
  }

  if (texture_info->type == kFlutterDesktopPixelBufferTexture) {
    if (!texture_info->pixel_buffer_config.callback) {
      FT_LOG(Error) << "Invalid pixel buffer texture callback.";
      return -1;
    }
  }

  if (texture_info->type == kFlutterDesktopGpuBufferTexture) {
    if (!texture_info->gpu_buffer_config.callback) {
      FT_LOG(Error) << "Invalid GPU buffer texture callback.";
      return -1;
    }
  }
  auto texture_gl = CreateExternalTexture(texture_info);
  int64_t texture_id = texture_gl->TextureId();

  {
    std::lock_guard<std::mutex> lock(map_mutex_);
    textures_[texture_id] = std::move(texture_gl);
  }

  engine_->RegisterExternalTexture(texture_id);
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
  return engine_->UnregisterExternalTexture(texture_id);
}

bool FlutterTizenTextureRegistrar::MarkTextureFrameAvailable(
    int64_t texture_id) {
  return engine_->MarkExternalTextureFrameAvailable(texture_id);
}

bool FlutterTizenTextureRegistrar::PopulateTexture(
    int64_t texture_id,
    size_t width,
    size_t height,
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

std::unique_ptr<ExternalTexture>
FlutterTizenTextureRegistrar::CreateExternalTexture(
    const FlutterDesktopTextureInfo* texture_info) {
  switch (texture_info->type) {
    case kFlutterDesktopPixelBufferTexture:
      return std::make_unique<ExternalTexturePixelGL>(
          texture_info->pixel_buffer_config.callback,
          texture_info->pixel_buffer_config.user_data);
      break;
    case kFlutterDesktopGpuBufferTexture:
      ExternalTextureExtensionType gl_extension =
          ExternalTextureExtensionType::kNone;
      if (engine_->renderer()->IsSupportedExtention(
              "EGL_TIZEN_image_native_surface")) {
        gl_extension = ExternalTextureExtensionType::kNativeSurface;
      } else if (engine_->renderer()->IsSupportedExtention(
                     "EGL_EXT_image_dma_buf_import")) {
        gl_extension = ExternalTextureExtensionType::kDmaBuffer;
      }
      return std::make_unique<ExternalTextureSurfaceGL>(
          gl_extension, texture_info->gpu_buffer_config.callback,
          texture_info->gpu_buffer_config.destruction_callback,
          texture_info->gpu_buffer_config.user_data);
      break;
  }
}

}  // namespace flutter
