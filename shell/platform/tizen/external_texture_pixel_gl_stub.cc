// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/external_texture_pixel_gl.h"

#ifdef TIZEN_RENDERER_EVAS_GL
#undef EFL_BETA_API_SUPPORT
#include "tizen_evas_gl_helper.h"
extern Evas_GL* g_evas_gl;
EVAS_GL_GLOBAL_GLES3_DECLARE();
#else
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl32.h>
#endif

namespace flutter {

ExternalTexturePixelGL::ExternalTexturePixelGL(
    FlutterDesktopPixelBufferTextureCallback texture_callback,
    void* user_data)
    : ExternalTexture(),
      texture_callback_(texture_callback),
      user_data_(user_data) {}

bool ExternalTexturePixelGL::PopulateTexture(
    size_t width,
    size_t height,
    FlutterOpenGLTexture* opengl_texture) {
  return CopyPixelBuffer(width, height);
}

bool ExternalTexturePixelGL::CopyPixelBuffer(size_t& width, size_t& height) {
  if (texture_callback_ && user_data_) {
    return true;
  }
  return false;
}
}  // namespace flutter
