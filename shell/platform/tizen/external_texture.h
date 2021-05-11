// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_H_
#define FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_H_

#include <stdint.h>
#include <memory>
#include <mutex>

#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/common/cpp/public/flutter_texture_registrar.h"

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

// An adaptation class of flutter engine and external texture interface.
class ExternalTexture {
 public:
  ExternalTexture();

  virtual ~ExternalTexture() = default;

  /**
   * Returns the unique id for the ExternalTextureGL instance.
   */
  int64_t TextureId() { return (int64_t)texture_id_; }

  virtual bool PopulateTexture(size_t width, size_t height,
                                     FlutterOpenGLTexture* opengl_texture) = 0;
  
 protected:
  std::unique_ptr<ExternalTextureGLState> state_;
 private:
  const long texture_id_{0};
};

#endif  // FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_GL_H_
