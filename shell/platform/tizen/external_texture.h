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
#include <Ecore.h>
#include <Elementary.h>
#include <Evas_GL_GLES3_Helpers.h>
extern Evas_GL* g_evas_gl;
EVAS_GL_GLOBAL_GLES3_DECLARE();
#else
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl32.h>
#endif

struct ExternalTextureGLState {
  GLuint gl_texture;
};

static std::atomic_long kNextTextureId = {1};

// An adaptation class of flutter engine and external texture interface.
class ExternalTexture : public std::enable_shared_from_this<ExternalTexture> {
 public:
  ExternalTexture()
      : state_(std::make_unique<ExternalTextureGLState>()),
        texture_id_(kNextTextureId++) {}
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
