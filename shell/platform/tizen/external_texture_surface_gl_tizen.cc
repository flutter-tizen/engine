// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "external_texture_surface_gl.h"

#include <tbm_surface.h>

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

#include <tbm_bufmgr.h>
#include <tbm_surface_internal.h>
#ifndef EGL_DMA_BUF_PLANE3_FD_EXT
#define EGL_DMA_BUF_PLANE3_FD_EXT 0x3440
#endif
#ifndef EGL_DMA_BUF_PLANE3_OFFSET_EXT
#define EGL_DMA_BUF_PLANE3_OFFSET_EXT 0x3441
#endif
#ifndef EGL_DMA_BUF_PLANE3_PITCH_EXT
#define EGL_DMA_BUF_PLANE3_PITCH_EXT 0x3442
#endif
#endif

#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

static void OnCollectTexture(void* textureGL) {
  auto* weak_texture =
      reinterpret_cast<std::weak_ptr<ExternalTexture>*>(textureGL);
  auto strong_texture = weak_texture->lock();
  delete weak_texture;
  if (strong_texture) {
    strong_texture->OnDestruction();
  }
}

ExternalTextureSurfaceGL::ExternalTextureSurfaceGL(
    ExternalTextureExtensionType gl_extention,
    FlutterDesktopGpuBufferTextureCallback texture_callback,
    FlutterDesktopGpuBufferDestructionCallback destruction_callback,
    void* user_data)
    : ExternalTexture(gl_extention),
      texture_callback_(texture_callback),
      destruction_callback_(destruction_callback),
      user_data_(user_data) {}

ExternalTextureSurfaceGL::~ExternalTextureSurfaceGL() {
  if (state_->gl_texture != 0) {
    glDeleteTextures(1, &state_->gl_texture);
  }
  state_.release();
}

bool ExternalTextureSurfaceGL::PopulateTexture(
    size_t width,
    size_t height,
    FlutterOpenGLTexture* opengl_texture) {
  if (!texture_callback_) {
    return false;
  }
  const FlutterDesktopGpuBuffer* gpu_buffer =
      texture_callback_(width, height, user_data_);
  if (!gpu_buffer) {
    FT_LOG(Info) << "gpu_buffer is null for texture ID: " << texture_id_;
    return false;
  }

  if (!gpu_buffer->buffer) {
    FT_LOG(Info) << "tbm_surface is null for texture ID: " << texture_id_;
    return false;
  }
  const tbm_surface_h tbm_surface =
      reinterpret_cast<tbm_surface_h>(const_cast<void*>(gpu_buffer->buffer));

  tbm_surface_info_s info;
  if (tbm_surface_get_info(tbm_surface, &info) != TBM_SURFACE_ERROR_NONE) {
    FT_LOG(Info) << "tbm_surface is invalid for texture ID: " << texture_id_;
    return false;
  }

#ifdef TIZEN_RENDERER_EVAS_GL
  EvasGLImage egl_src_image = nullptr;
  if (state_->gl_extention == ExternalTextureExtensionType::kNativeSurface) {
    int attribs[] = {EVAS_GL_IMAGE_PRESERVED, GL_TRUE, 0};
    egl_src_image = evasglCreateImageForContext(
        g_evas_gl, evas_gl_current_context_get(g_evas_gl),
        EVAS_GL_NATIVE_SURFACE_TIZEN, tbm_surface, attribs);
  } else if (state_->gl_extention == ExternalTextureExtensionType::kDmaBuffer) {
    FT_LOG(Error)
        << "EGL_EXT_image_dma_buf_import is not supported this renderer.";
    return false;
  }
  if (!egl_src_image) {
    return false;
  }
  if (state_->gl_texture == 0) {
    glGenTextures(1, &state_->gl_texture);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, state_->gl_texture);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S,
                    GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T,
                    GL_CLAMP_TO_BORDER);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  } else {
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, state_->gl_texture);
  }
  glEvasGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, egl_src_image);
  if (egl_src_image) {
    evasglDestroyImage(egl_src_image);
  }
#else
  PFNEGLCREATEIMAGEKHRPROC n_eglCreateImageKHR =
      reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(
          eglGetProcAddress("eglCreateImageKHR"));
  EGLImageKHR egl_src_image = nullptr;

  if (state_->gl_extention == ExternalTextureExtensionType::kNativeSurface) {
    const EGLint attribs[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE,
                              EGL_NONE};
    egl_src_image =
        n_eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT,
                            EGL_NATIVE_SURFACE_TIZEN, tbm_surface, attribs);
  } else if (state_->gl_extention == ExternalTextureExtensionType::kDmaBuffer) {
    EGLint attribs[50];
    int atti = 0;
    int plane_fd_ext[4] = {EGL_DMA_BUF_PLANE0_FD_EXT, EGL_DMA_BUF_PLANE1_FD_EXT,
                           EGL_DMA_BUF_PLANE2_FD_EXT,
                           EGL_DMA_BUF_PLANE3_FD_EXT};
    int plane_offset_ext[4] = {
        EGL_DMA_BUF_PLANE0_OFFSET_EXT, EGL_DMA_BUF_PLANE1_OFFSET_EXT,
        EGL_DMA_BUF_PLANE2_OFFSET_EXT, EGL_DMA_BUF_PLANE3_OFFSET_EXT};
    int plane_pitch_ext[4] = {
        EGL_DMA_BUF_PLANE0_PITCH_EXT, EGL_DMA_BUF_PLANE1_PITCH_EXT,
        EGL_DMA_BUF_PLANE2_PITCH_EXT, EGL_DMA_BUF_PLANE3_PITCH_EXT};

    attribs[atti++] = EGL_WIDTH;
    attribs[atti++] = info.width;
    attribs[atti++] = EGL_HEIGHT;
    attribs[atti++] = info.height;
    attribs[atti++] = EGL_LINUX_DRM_FOURCC_EXT;
    attribs[atti++] = info.format;

    int num_planes = tbm_surface_internal_get_num_planes(info.format);
    for (int i = 0; i < num_planes; i++) {
      int bo_idx = tbm_surface_internal_get_plane_bo_idx(tbm_surface, i);
      tbm_bo tbo = tbm_surface_internal_get_bo(tbm_surface, bo_idx);
      attribs[atti++] = plane_fd_ext[i];
      attribs[atti++] = static_cast<int>(
          reinterpret_cast<size_t>(tbm_bo_get_handle(tbo, TBM_DEVICE_3D).ptr));
      attribs[atti++] = plane_offset_ext[i];
      attribs[atti++] = info.planes[i].offset;
      attribs[atti++] = plane_pitch_ext[i];
      attribs[atti++] = info.planes[i].stride;
    }
    attribs[atti++] = EGL_NONE;
    egl_src_image =
        n_eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT,
                            EGL_LINUX_DMA_BUF_EXT, nullptr, attribs);
  }

  if (!egl_src_image) {
    if (state_->gl_extention != ExternalTextureExtensionType::kNone) {
      FT_LOG(Error) << "eglCreateImageKHR failed with an error "
                    << eglGetError() << " for texture ID: " << texture_id_;
    } else {
      FT_LOG(Error) << "Either EGL_TIZEN_image_native_surface or "
                       "EGL_EXT_image_dma_buf_import shoule be supported.";
    }
    return false;
  }
  if (state_->gl_texture == 0) {
    glGenTextures(1, &state_->gl_texture);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, state_->gl_texture);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S,
                    GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T,
                    GL_CLAMP_TO_BORDER);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  } else {
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, state_->gl_texture);
  }
  PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES =
      reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(
          eglGetProcAddress("glEGLImageTargetTexture2DOES"));
  glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, egl_src_image);
  if (egl_src_image) {
    PFNEGLDESTROYIMAGEKHRPROC n_eglDestroyImageKHR =
        reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(
            eglGetProcAddress("eglDestroyImageKHR"));
    n_eglDestroyImageKHR(eglGetCurrentDisplay(), egl_src_image);
  }
#endif

  opengl_texture->target = GL_TEXTURE_EXTERNAL_OES;
  opengl_texture->name = state_->gl_texture;
  opengl_texture->format = GL_RGBA8;
  opengl_texture->destruction_callback = OnCollectTexture;
  auto* weak_texture = new std::weak_ptr<ExternalTexture>(shared_from_this());
  opengl_texture->user_data = weak_texture;
  opengl_texture->width = width;
  opengl_texture->height = height;
  return true;
}

void ExternalTextureSurfaceGL::OnDestruction() {
  if (destruction_callback_) {
    destruction_callback_(user_data_);
  }
}

}  // namespace flutter
