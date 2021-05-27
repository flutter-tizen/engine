// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_EXTERNAL_TEXTURE_H_
#define EMBEDDER_EXTERNAL_TEXTURE_H_

#include <atomic>
#include <memory>
#include "flutter/shell/platform/common/cpp/public/flutter_texture_registrar.h"
#include "flutter/shell/platform/embedder/embedder.h"

#ifdef TIZEN_RENDERER_EVAS_GL
#undef EFL_BETA_API_SUPPORT
#include <Evas_GL.h>
#else
#include <GLES2/gl2.h>
#endif

struct ExternalTextureGLState {
  GLuint gl_texture;
};

static std::atomic_long next_texture_id = {1};

// An adaptation class of flutter engine and external texture interface.
class ExternalTexture : public std::enable_shared_from_this<ExternalTexture> {
 public:
  ExternalTexture()
      : state_(std::make_unique<ExternalTextureGLState>()),
        texture_id_(next_texture_id++) {}
  virtual ~ExternalTexture() = default;

  /**
   * Returns the unique id for the ExternalTextureGL instance.
   */
  int64_t TextureId() { return (int64_t)texture_id_; }

  virtual bool PopulateTexture(size_t width,
                               size_t height,
                               FlutterOpenGLTexture* opengl_texture) = 0;
  virtual void OnDestruction(){};

 protected:
  std::unique_ptr<ExternalTextureGLState> state_;
  const long texture_id_{0};
};

#endif  // EMBEDDER_EXTERNAL_TEXTURE_H_
