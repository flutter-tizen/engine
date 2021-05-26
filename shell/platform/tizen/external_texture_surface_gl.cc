// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "external_texture_surface_gl.h"

#include "flutter/shell/platform/common/cpp/public/flutter_texture_registrar.h"

#ifdef TIZEN_RENDERER_EVAS_GL
#undef EFL_BETA_API_SUPPORT
#include <Evas_GL_GLES3_Helpers.h>
extern Evas_GL* g_evas_gl;
EVAS_GL_GLOBAL_GLES3_DECLARE();
#else
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl32.h>
#endif

#include <tbm_surface.h>

#include "flutter/shell/platform/tizen/tizen_log.h"

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
    FlutterDesktopGpuBufferTextureCallback texture_callback,
    FlutterDesktopGpuBufferDestructionCallback destruction_callback,
    void* user_data)
    : ExternalTexture(),
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
  const FlutterDesktopGpuBuffer* gpu_buffer =
      texture_callback_(width, height, user_data_);
  if (!gpu_buffer) {
    FT_LOGD("[texture id:%ld] gpu_buffer is null", texture_id_);
    return false;
  }

  if (!gpu_buffer->buffer) {
    FT_LOGD("[texture id:%ld] tbm_surface_ is null", texture_id_);
    return false;
  }
  const tbm_surface_h tbm_surface =
      reinterpret_cast<tbm_surface_h>(const_cast<void*>(gpu_buffer->buffer));

  tbm_surface_info_s info;
  if (tbm_surface_get_info(tbm_surface, &info) != TBM_SURFACE_ERROR_NONE) {
    FT_LOGD("[texture id:%ld] tbm_surface is invalid", texture_id_);
    return false;
  }

#ifdef TIZEN_RENDERER_EVAS_GL
  int attribs[] = {EVAS_GL_IMAGE_PRESERVED, GL_TRUE, 0};
  EvasGLImage egl_src_image = evasglCreateImageForContext(
      g_evas_gl, evas_gl_current_context_get(g_evas_gl),
      EVAS_GL_NATIVE_SURFACE_TIZEN, (void*)(intptr_t)tbm_surface, attribs);
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
      (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
  const EGLint attribs[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE,
                            EGL_NONE};
  EGLImageKHR egl_src_image = n_eglCreateImageKHR(
      eglGetCurrentDisplay(), EGL_NO_CONTEXT, EGL_NATIVE_SURFACE_TIZEN,
      (EGLClientBuffer)tbm_surface, attribs);

  if (!egl_src_image) {
    FT_LOGE("[texture id:%ld] egl_src_image create fail!!, errorcode == %d",
            texture_id_, eglGetError());
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
      (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress(
          "glEGLImageTargetTexture2DOES");
  glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, egl_src_image);
  if (egl_src_image) {
    PFNEGLDESTROYIMAGEKHRPROC n_eglDestroyImageKHR =
        (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
    n_eglDestroyImageKHR(eglGetCurrentDisplay(), egl_src_image);
  }
#endif

  opengl_texture->target = GL_TEXTURE_EXTERNAL_OES;
  opengl_texture->name = state_->gl_texture;
  opengl_texture->format = GL_RGBA8;
  opengl_texture->destruction_callback = (VoidCallback)OnCollectTexture;
  auto* weak_texture = new std::weak_ptr<ExternalTexture>(shared_from_this());
  opengl_texture->user_data = weak_texture;
  opengl_texture->width = width;
  opengl_texture->height = height;
  return true;
}

void ExternalTextureSurfaceGL::OnDestruction() {
  destruction_callback_(user_data_);
}
