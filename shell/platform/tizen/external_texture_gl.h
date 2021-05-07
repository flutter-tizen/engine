// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_GL_H_
#define FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_GL_H_

#include <media_packet.h>
#include <stdint.h>
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
  int64_t TextureId() { return (int64_t)texture_id_; }

  /**
   * Accepts texture buffer copy request from the Flutter engine.
   * When the user side marks the texture_id as available, the Flutter engine
   * will callback to this method and ask for populate the |opengl_texture|
   * object, such as the texture type and the format of the pixel buffer and the
   * texture object.
   * Returns true on success, false on failure.
   */
  virtual bool PopulateTextureWithIdentifier(
      size_t width, size_t height, FlutterOpenGLTexture* opengl_texture) {
    // assert;
    return false;
  }
  virtual bool OnFrameAvailable(void* external_image) {
    // assert;
    return false;
  }

 protected:
  bool MakeTextureFromExternalImage(tbm_surface_h surface, void* external_image, size_t width,
                                    size_t height,
                                    FlutterOpenGLTexture* opengl_texture,
                                    VoidCallback destruction_callback);
private:
  std::unique_ptr<ExternalTextureGLState> state_;
  std::mutex mutex_;
  const long texture_id_{0};
};

class ExternalTextureTbm : public ExternalTextureGL {
 public:
  ExternalTextureTbm();
  ~ExternalTextureTbm();
  virtual bool PopulateTextureWithIdentifier(
      size_t width, size_t height,
      FlutterOpenGLTexture* opengl_texture) override;
  virtual bool OnFrameAvailable(void* external_image) override;

 private:
  tbm_surface_h available_tbm_surface_{nullptr};
};

class ExternalTextureMediaPacket : public ExternalTextureGL {
 public:
  ExternalTextureMediaPacket();
  ~ExternalTextureMediaPacket();
  virtual bool PopulateTextureWithIdentifier(
      size_t width, size_t height,
      FlutterOpenGLTexture* opengl_texture) override;
  virtual bool OnFrameAvailable(void* external_image) override;

 private:
  media_packet_h available_media_packet_{nullptr};
};

#endif  // FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_GL_H_
