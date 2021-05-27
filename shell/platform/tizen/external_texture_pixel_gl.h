// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_EXTERNAL_TEXTURE_PIXEL_GL_H
#define EMBEDDER_EXTERNAL_TEXTURE_PIXEL_GL_H

#include <memory>

#include "flutter/shell/platform/common/cpp/public/flutter_texture_registrar.h"
#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/tizen/external_texture.h"

// An adaptation class of flutter engine and external texture interface.
class ExternalTexturePixelGL : public ExternalTexture {
 public:
  ExternalTexturePixelGL(
      FlutterDesktopPixelBufferTextureCallback texture_callback,
      void* user_data);

  ~ExternalTexturePixelGL() = default;

  bool PopulateTexture(size_t width,
                       size_t height,
                       FlutterOpenGLTexture* opengl_texture) override;

  bool CopyPixelBuffer(size_t& width, size_t& height);

 private:
  FlutterDesktopPixelBufferTextureCallback texture_callback_ = nullptr;
  void* user_data_ = nullptr;
};

#endif  // EMBEDDER_EXTERNAL_TEXTURE_PIXEL_GL_H
