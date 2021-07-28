// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "external_texture_pixel_gl.h"

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
