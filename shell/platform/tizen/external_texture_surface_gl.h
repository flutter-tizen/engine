// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_SURFACE_GL_H_
#define FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_SURFACE_GL_H_

#include <stdint.h>
#include <tbm_bufmgr.h>
#include <tbm_drm_helper.h>
#include <tbm_surface.h>

#include <memory>
#include <mutex>

#include "flutter/shell/platform/common/cpp/public/flutter_texture_registrar.h"
#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/tizen/external_texture.h"

// An adaptation class of flutter engine and external texture interface.
class ExternalTextureSurfaceGL : public ExternalTexture {
 public:
  ExternalTextureSurfaceGL(FlutterDesktopGpuBufferTextureCallback texture_callback,
                    FlutterDesktopDestructionCallback destruction_callback,
                    void* user_data);

  virtual ~ExternalTextureSurfaceGL();

  /**
   * Returns the unique id for the ExternalTextureGL instance.
   */
  int64_t TextureId() override { return (int64_t)texture_id_; }

  /**
   * Accepts texture buffer copy request from the Flutter engine.
   * When the user side marks the texture_id as available, the Flutter engine
   * will callback to this method and ask for populate the |opengl_texture|
   * object, such as the texture type and the format of the pixel buffer and the
   * texture object.
   * Returns true on success, false on failure.
   */
  bool PopulateTexture(size_t width, size_t height,
                       FlutterOpenGLTexture* opengl_texture) override;
  static void OnCollectTexture(void* surface);

 private:
  std::unique_ptr<ExternalTextureGLState> state_;
  FlutterDesktopGpuBufferTextureCallback texture_callback_ = nullptr;
  FlutterDesktopDestructionCallback destruction_callback_ = nullptr;
  void* user_data_ = nullptr;
};

#endif  // FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_SURFACE_GL_H_
