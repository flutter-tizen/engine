// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "external_texture_gl.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl32.h>

#include <atomic>
#include <iostream>

#include "flutter/shell/platform/tizen/tizen_log.h"

struct ExternalTextureGLState {
  GLuint gl_texture;
};

static std::atomic_long nextTextureId = {1};

ExternalTextureGL::ExternalTextureGL()
    : state_(std::make_unique<ExternalTextureGLState>()),
      texture_tbm_surface_(NULL),
      texture_id_(nextTextureId++) {}

ExternalTextureGL::~ExternalTextureGL() {
  mutex_.lock();
  if (state_->gl_texture != 0) {
    glDeleteTextures(1, &state_->gl_texture);
  }
  state_.release();
  DestructionTbmSurface();
  mutex_.unlock();
}

bool ExternalTextureGL::OnFrameAvailable(tbm_surface_h tbm_surface) {
  mutex_.lock();
  if (!tbm_surface) {
    FT_LOGE("tbm_surface is null");
    mutex_.unlock();
    return false;
  }
  if (texture_tbm_surface_) {
    FT_LOGD("texture_tbm_surface_ does not destruction, discard");
    mutex_.unlock();
    return false;
  }
  if (!tbm_surface_internal_is_valid(tbm_surface)) {
    FT_LOGD("tbm_surface not valid, pass");
    mutex_.unlock();
    return false;
  }
  texture_tbm_surface_ = tbm_surface;
  tbm_surface_internal_ref(texture_tbm_surface_);
  mutex_.unlock();
  return true;
}

bool ExternalTextureGL::PopulateTextureWithIdentifier(
    size_t width, size_t height, FlutterOpenGLTexture* opengl_texture) {
  mutex_.lock();
  if (!texture_tbm_surface_) {
    FT_LOGD("texture_tbm_surface_ is NULL");
    mutex_.unlock();
    return false;
  }
  if (!tbm_surface_internal_is_valid(texture_tbm_surface_)) {
    FT_LOGD("tbm_surface not valid");
    DestructionTbmSurface();
    mutex_.unlock();
    return false;
  }

  PFNEGLCREATEIMAGEKHRPROC n_eglCreateImageKHR =
      (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
  const EGLint attrs[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE,
                          EGL_NONE};
  EGLImageKHR eglSrcImage = n_eglCreateImageKHR(
      eglGetCurrentDisplay(), EGL_NO_CONTEXT, EGL_NATIVE_SURFACE_TIZEN,
      (EGLClientBuffer)texture_tbm_surface_, attrs);
  if (!eglSrcImage) {
    FT_LOGE("eglSrcImage create fail!!, errorcode == %d", eglGetError());
    mutex_.unlock();
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
  glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, eglSrcImage);
  if (eglSrcImage) {
    PFNEGLDESTROYIMAGEKHRPROC n_eglDestoryImageKHR =
        (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
    n_eglDestoryImageKHR(eglGetCurrentDisplay(), eglSrcImage);
  }
  opengl_texture->target = GL_TEXTURE_EXTERNAL_OES;
  opengl_texture->name = state_->gl_texture;
  opengl_texture->format = GL_RGBA8;
  opengl_texture->destruction_callback = (VoidCallback)DestructionCallback;
  opengl_texture->user_data = static_cast<void*>(this);
  opengl_texture->width = width;
  opengl_texture->height = height;
  mutex_.unlock();
  return true;
}

void ExternalTextureGL::DestructionTbmSurfaceWithLock() {
  mutex_.lock();
  DestructionTbmSurface();
  mutex_.unlock();
}

void ExternalTextureGL::DestructionTbmSurface() {
  if (!texture_tbm_surface_) {
    FT_LOGE("tbm_surface_h is NULL");
    return;
  }
  tbm_surface_internal_unref(texture_tbm_surface_);
  texture_tbm_surface_ = NULL;
}

void ExternalTextureGL::DestructionCallback(void* user_data) {
  ExternalTextureGL* externalTextureGL =
      reinterpret_cast<ExternalTextureGL*>(user_data);
  externalTextureGL->DestructionTbmSurfaceWithLock();
}
