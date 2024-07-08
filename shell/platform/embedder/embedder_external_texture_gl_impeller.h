// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_EMBEDDER_EMBEDDER_EXTERNAL_TEXTURE_GL_IMPELLER_H_
#define FLUTTER_SHELL_PLATFORM_EMBEDDER_EMBEDDER_EXTERNAL_TEXTURE_GL_IMPELLER_H_

#include "flutter/common/graphics/texture.h"
#include "flutter/fml/macros.h"
#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/embedder/embedder_external_texture_gl.h"
#include "third_party/skia/include/core/SkSize.h"

namespace flutter {

class EmbedderExternalTextureGLImpeller : public flutter::Texture {
 public:
  EmbedderExternalTextureGLImpeller(
      int64_t texture_identifier,
      const EmbedderExternalTextureGL::ExternalTextureCallback& callback);

  ~EmbedderExternalTextureGLImpeller();

 private:
  const EmbedderExternalTextureGL::ExternalTextureCallback&
      external_texture_callback_;
  sk_sp<DlImage> last_image_;

  sk_sp<DlImage> ResolveTexture(int64_t texture_id,
                                PaintContext& context,
                                const SkISize& size);
  sk_sp<DlImage> ResolveGpuSurfaceTexture(FlutterOpenGLTexture* texture,
                                          PaintContext& context,
                                          const SkISize& size);
  sk_sp<DlImage> ResolvePixelBufferTexture(FlutterOpenGLTexture* texture,
                                           PaintContext& context,
                                           const SkISize& size);

  // |flutter::Texture|
  void Paint(PaintContext& context,
             const SkRect& bounds,
             bool freeze,
             const DlImageSampling sampling) override;

  // |flutter::Texture|
  void OnGrContextCreated() override;

  // |flutter::Texture|
  void OnGrContextDestroyed() override;

  // |flutter::Texture|
  void MarkNewFrameAvailable() override;

  // |flutter::Texture|
  void OnTextureUnregistered() override;

  FML_DISALLOW_COPY_AND_ASSIGN(EmbedderExternalTextureGLImpeller);
};

}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_EMBEDDER_EMBEDDER_EXTERNAL_TEXTURE_GL_IMPELLER_H_
