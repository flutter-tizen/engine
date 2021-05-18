// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "external_texture_gl.h"

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

#include <atomic>
#include <iostream>

#include "flutter/shell/platform/tizen/tizen_log.h"

struct ExternalTextureGLState {
  GLuint gl_texture;
};

static std::atomic_long nextTextureId = {1};

static void MarkTbmSurfaceToUse(void* surface) {
#ifndef WEARABLE_PROFILE
  FT_ASSERT(surface);
  tbm_surface_h tbm_surface = (tbm_surface_h)surface;
  tbm_surface_internal_ref(tbm_surface);
#endif
}

static void UnmarkTbmSurfaceToUse(void* surface) {
  FT_ASSERT(surface);
  tbm_surface_h tbm_surface = (tbm_surface_h)surface;
#ifndef WEARABLE_PROFILE
  tbm_surface_internal_unref(tbm_surface);
#else
  tbm_surface_destroy(tbm_surface);
#endif
}

ExternalTextureGL::ExternalTextureGL()
    : state_(std::make_unique<ExternalTextureGLState>()),
      available_tbm_surface_(nullptr),
      texture_id_(nextTextureId++) {}

ExternalTextureGL::~ExternalTextureGL() {
  if (state_->gl_texture != 0) {
    glDeleteTextures(1, &state_->gl_texture);
  }

  // If there is a available_tbm_surface_ that is not populated, remove it
  if (available_tbm_surface_) {
    UnmarkTbmSurfaceToUse(available_tbm_surface_);
  }

  state_.release();
}

bool ExternalTextureGL::OnFrameAvailable(tbm_surface_h tbm_surface) {
  if (!tbm_surface) {
    FT_LOGE("[texture id:%ld] tbm_surface is null", texture_id_);
    return false;
  }

  if (available_tbm_surface_) {
    FT_LOGD(
        "[texture id:%ld] Discard! an available tbm surface that has not yet "
        "been used exists",
        texture_id_);
    return false;
  }

  tbm_surface_info_s info;
  if (tbm_surface_get_info(tbm_surface, &info) != TBM_SURFACE_ERROR_NONE) {
    FT_LOGD("[texture id:%ld] tbm_surface not valid, pass", texture_id_);
    return false;
  }

  available_tbm_surface_ = tbm_surface;
  MarkTbmSurfaceToUse(available_tbm_surface_);

  return true;
}

bool ExternalTextureGL::PopulateTextureWithIdentifier(
    size_t width,
    size_t height,
    FlutterOpenGLTexture* opengl_texture) {
  if (!available_tbm_surface_) {
    FT_LOGD("[texture id:%ld] available_tbm_surface_ is null", texture_id_);
    return false;
  }
  tbm_surface_info_s info;
  if (tbm_surface_get_info(available_tbm_surface_, &info) !=
      TBM_SURFACE_ERROR_NONE) {
    FT_LOGD("[texture id:%ld] tbm_surface is invalid", texture_id_);
    UnmarkTbmSurfaceToUse(available_tbm_surface_);
    available_tbm_surface_ = nullptr;
    return false;
  }

#ifdef TIZEN_RENDERER_EVAS_GL
  int attribs[] = {EVAS_GL_IMAGE_PRESERVED, GL_TRUE, 0};
  EvasGLImage egl_src_image = evasglCreateImageForContext(
      g_evas_gl, evas_gl_current_context_get(g_evas_gl),
      EVAS_GL_NATIVE_SURFACE_TIZEN, (void*)(intptr_t)available_tbm_surface_,
      attribs);
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
      (EGLClientBuffer)available_tbm_surface_, attribs);

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
  opengl_texture->destruction_callback = (VoidCallback)UnmarkTbmSurfaceToUse;

  // Abandon ownership of tbm_surface
  opengl_texture->user_data = available_tbm_surface_;
  available_tbm_surface_ = nullptr;

  opengl_texture->width = width;
  opengl_texture->height = height;
  return true;
}
