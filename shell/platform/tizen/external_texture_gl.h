// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_GL_H_
#define FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_GL_H_

#include <stdint.h>
#include <tbm_bo.h>
#include <tbm_bufmgr.h>
#include <tbm_drm_helper.h>
#include <tbm_surface.h>
#include <tbm_surface_internal.h>

#include <memory>
#include <mutex>

#include "flutter/shell/platform/embedder/embedder.h"

typedef struct ExternalTextureGLState ExternalTextureGLState;

// An adaptation class of flutter engine and external texture interface.
class ExternalTextureGL {
 public:
  ExternalTextureGL();

  virtual ~ExternalTextureGL();

  /**
   * Returns the unique id for the ExternalTextureGL instance.
   */
  int64_t texture_id() { return (int64_t)texture_id_; }

  /**
   * Accepts texture buffer copy request from the Flutter engine.
   * When the user side marks the texture_id as available, the Flutter engine
   * will callback to this method and ask for populate the |opengl_texture|
   * object, such as the texture type and the format of the pixel buffer and the
   * texture object.
   * Returns true on success, false on failure.
   */
  bool PopulateTextureWithIdentifier(size_t width, size_t height,
                                     FlutterOpenGLTexture* opengl_texture);
  bool OnFrameAvailable(tbm_surface_h tbm_surface);
  void DestructionTbmSurface();

 private:
  std::unique_ptr<ExternalTextureGLState> state_;
  std::mutex mutex_;
  tbm_surface_h texture_tbm_surface_;
  static void destructionCallback(void* user_data);
  const long texture_id_;
};

#endif  // FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_GL_H_
