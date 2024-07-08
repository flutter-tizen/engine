// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/embedder/embedder_external_texture_gl_impeller.h"

#include "flutter/fml/logging.h"
#include "flutter/impeller/display_list/dl_image_impeller.h"
#include "flutter/impeller/renderer/backend/gles/context_gles.h"
#include "flutter/impeller/renderer/backend/gles/texture_gles.h"
#include "impeller/aiks/aiks_context.h"
#include "impeller/renderer/backend/gles/gles.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"

namespace flutter {

EmbedderExternalTextureGLImpeller::EmbedderExternalTextureGLImpeller(
    int64_t texture_identifier,
    const EmbedderExternalTextureGL::ExternalTextureCallback& callback)
    : Texture(texture_identifier), external_texture_callback_(callback) {
  FML_DCHECK(external_texture_callback_);
}

EmbedderExternalTextureGLImpeller::~EmbedderExternalTextureGLImpeller() =
    default;

// |flutter::Texture|
void EmbedderExternalTextureGLImpeller::Paint(PaintContext& context,
                                              const SkRect& bounds,
                                              bool freeze,
                                              const DlImageSampling sampling) {
  if (last_image_ == nullptr) {
    last_image_ =
        ResolveTexture(Id(),                                           //
                       context,                                        //
                       SkISize::Make(bounds.width(), bounds.height())  //
        );
  }

  DlCanvas* canvas = context.canvas;
  const DlPaint* paint = context.paint;

  if (last_image_) {
    SkRect image_bounds = SkRect::Make(last_image_->bounds());
    if (bounds != image_bounds) {
      canvas->DrawImageRect(last_image_, image_bounds, bounds, sampling, paint);
    } else {
      canvas->DrawImage(last_image_, {bounds.x(), bounds.y()}, sampling, paint);
    }
  }
}

// |flutter::Texture|
void EmbedderExternalTextureGLImpeller::OnGrContextCreated() {}

// |flutter::Texture|
void EmbedderExternalTextureGLImpeller::OnGrContextDestroyed() {}

// |flutter::Texture|
void EmbedderExternalTextureGLImpeller::MarkNewFrameAvailable() {
  last_image_ = nullptr;
}

// |flutter::Texture|
void EmbedderExternalTextureGLImpeller::OnTextureUnregistered() {}

sk_sp<DlImage> EmbedderExternalTextureGLImpeller::ResolveTexture(
    int64_t texture_id,
    PaintContext& context,
    const SkISize& size) {
  std::unique_ptr<FlutterOpenGLTexture> texture =
      external_texture_callback_(texture_id, size.width(), size.height());
  if (!texture) {
    return nullptr;
  }
  size_t width = size.width();
  size_t height = size.height();
  if (texture->width != 0 && texture->height != 0) {
    width = texture->width;
    height = texture->height;
  }
  if (texture->impeller_texture_type ==
      FlutterGLImpellerTextureType::kFlutterGLImpellerTexturePixelBuffer) {
    return ResolvePixelBufferTexture(texture.get(), context,
                                     SkISize::Make(width, height));
  } else {
    return ResolveGpuSurfaceTexture(texture.get(), context,
                                    SkISize::Make(width, height));
  }
}

sk_sp<DlImage> EmbedderExternalTextureGLImpeller::ResolvePixelBufferTexture(
    FlutterOpenGLTexture* texture,
    PaintContext& context,
    const SkISize& size) {
  impeller::TextureDescriptor desc;
  desc.type = impeller::TextureType::kTexture2D;
  impeller::AiksContext* aiks_context = context.aiks_context;
  const auto& gl_context =
      impeller::ContextGLES::Cast(*aiks_context->GetContext());
  desc.storage_mode = impeller::StorageMode::kDevicePrivate;
  desc.format = impeller::PixelFormat::kR8G8B8A8UNormInt;
  desc.size = {static_cast<int>(size.width()), static_cast<int>(size.height())};
  desc.mip_count = 1;

  auto textureGLES =
      std::make_shared<impeller::TextureGLES>(gl_context.GetReactor(), desc);
  if (!textureGLES->SetContents(texture->buffer, texture->buffer_size)) {
    if (texture->destruction_callback) {
      texture->destruction_callback(texture->user_data);
    }
    return nullptr;
  }
  if (texture->destruction_callback) {
    texture->destruction_callback(texture->user_data);
  }
  return impeller::DlImageImpeller::Make(textureGLES);
}

sk_sp<DlImage> EmbedderExternalTextureGLImpeller::ResolveGpuSurfaceTexture(
    FlutterOpenGLTexture* texture,
    PaintContext& context,
    const SkISize& size) {
  impeller::TextureDescriptor desc;
  desc.type = impeller::TextureType::kTextureExternalOES;
  impeller::AiksContext* aiks_context = context.aiks_context;
  const auto& gl_context =
      impeller::ContextGLES::Cast(*aiks_context->GetContext());
  desc.storage_mode = impeller::StorageMode::kDevicePrivate;
  desc.format = impeller::PixelFormat::kR8G8B8A8UNormInt;
  desc.size = {static_cast<int>(size.width()), static_cast<int>(size.height())};
  desc.mip_count = 1;
  auto textureGLES = std::make_shared<impeller::TextureGLES>(
      gl_context.GetReactor(), desc,
      impeller::TextureGLES::IsWrapped::kWrapped);
  textureGLES->SetCoordinateSystem(
      impeller::TextureCoordinateSystem::kUploadFromHost);
  if (!textureGLES->Bind()) {
    if (texture->destruction_callback) {
      texture->destruction_callback(texture->user_data);
    }
    return nullptr;
  }

  if (!texture->bind_callback(texture->user_data)) {
    if (texture->destruction_callback) {
      texture->destruction_callback(texture->user_data);
    }
    return nullptr;
  }

  if (texture->destruction_callback) {
    texture->destruction_callback(texture->user_data);
  }

  return impeller::DlImageImpeller::Make(textureGLES);
}

}  // namespace flutter
