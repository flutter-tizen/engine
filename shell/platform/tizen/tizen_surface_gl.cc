// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_surface_gl.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "flutter/shell/platform/tizen/tizen_log.h"

template <class T>
using EGLResult = std::pair<bool, T>;

static EGLResult<EGLContext> CreateContext(EGLDisplay display, EGLConfig config,
                                           EGLContext share = EGL_NO_CONTEXT) {
  EGLint attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

  EGLContext context = eglCreateContext(display, config, share, attributes);
  if (context == EGL_NO_CONTEXT) {
    LogLastEGLError();
  }
  return {context != EGL_NO_CONTEXT, context};
}

static EGLResult<EGLConfig> ChooseEGLConfiguration(EGLDisplay display) {
  EGLint attributes[] = {
      // clang-format off
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
      EGL_RED_SIZE,        8,
      EGL_GREEN_SIZE,      8,
      EGL_BLUE_SIZE,       8,
      EGL_ALPHA_SIZE,      8,
      EGL_DEPTH_SIZE,      0,
      EGL_STENCIL_SIZE,    0,
      EGL_NONE,            // termination sentinel
      // clang-format on
  };

  EGLint config_count = 0;
  EGLConfig egl_config = nullptr;

  if (eglChooseConfig(display, attributes, &egl_config, 1, &config_count) !=
      EGL_TRUE) {
    LogLastEGLError();
    return {false, nullptr};
  }

  bool success = config_count > 0 && egl_config != nullptr;

  return {success, success ? egl_config : nullptr};
}

TizenEGLSurface::~TizenEGLSurface() {
  if (eglDestroySurface(tizen_native_egl_window_->GetEGLDisplayHandle(),
                        egl_surface_) != EGL_TRUE) {
    LogLastEGLError();
  }
  tizen_native_egl_window_ = nullptr;
}

TizenEGLContext::TizenEGLContext(
    std::shared_ptr<TizenNativeEGLWindow> tizen_native_egl_window)
    : tizen_native_egl_window_(tizen_native_egl_window) {
  EGLDisplay egl_display = tizen_native_egl_window_->GetEGLDisplayHandle();
  auto config = ChooseEGLConfiguration(egl_display);
  if (!config.first) {
    FT_LOGE("Failed to ChooseEGLConfiguration");
    return;
  }
  egl_config_ = config.second;

  auto ctx = CreateContext(egl_display, egl_config_, EGL_NO_CONTEXT);
  if (!ctx.first) {
    FT_LOGE("Failed to create egl context");
    return;
  }
  egl_context_ = ctx.second;

  auto resource_ctx = CreateContext(egl_display, egl_config_, egl_context_);
  if (!resource_ctx.first) {
    FT_LOGE("Failed to create egl resource context");
    return;
  }
  egl_resource_context_ = resource_ctx.second;
}

TizenEGLContext::~TizenEGLContext() {
  if (eglDestroyContext(tizen_native_egl_window_->GetEGLDisplayHandle(),
                        egl_context_) != EGL_TRUE) {
    FT_LOGE("Failed to destroy egl context");
    LogLastEGLError();
  }
  if (eglDestroyContext(tizen_native_egl_window_->GetEGLDisplayHandle(),
                        egl_resource_context_) != EGL_TRUE) {
    FT_LOGE("Failed to destroy egl resource context");
    LogLastEGLError();
  }
  tizen_native_egl_window_ = nullptr;
}

bool TizenEGLContext::IsValid() {
  return tizen_native_egl_window_ && tizen_native_egl_window_->IsValid() &&
         egl_config_ != nullptr && egl_context_ != EGL_NO_CONTEXT &&
         egl_resource_context_ != EGL_NO_CONTEXT;
}

std::unique_ptr<TizenEGLSurface>
TizenEGLContext::CreateTizenEGLWindowSurface() {
  const EGLint attribs[] = {EGL_NONE};
  EGLSurface surface = eglCreateWindowSurface(
      tizen_native_egl_window_->GetEGLDisplayHandle(), egl_config_,
      ecore_wl2_egl_window_native_get(
          tizen_native_egl_window_->GetEglWindowHandle()),
      attribs);
  if (surface == EGL_NO_SURFACE) {
    LogLastEGLError();
  }
  return std::make_unique<TizenEGLSurface>(tizen_native_egl_window_, surface);
}

std::unique_ptr<TizenEGLSurface>
TizenEGLContext::CreateTizenEGLPbufferSurface() {
  const EGLint attribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
  EGLSurface surface = eglCreatePbufferSurface(
      tizen_native_egl_window_->GetEGLDisplayHandle(), egl_config_, attribs);
  if (surface == EGL_NO_SURFACE) {
    LogLastEGLError();
  }
  return std::make_unique<TizenEGLSurface>(tizen_native_egl_window_, surface);
}

TizenSurfaceGL::TizenSurfaceGL(
    std::shared_ptr<TizenNativeWindow> tizen_native_window)
    : tizen_native_window_(tizen_native_window) {
  if (!tizen_native_window_->IsValid()) {
    FT_LOGE("Invalid native window");
    return;
  }

  tizen_context_gl_ = std::make_unique<TizenEGLContext>(
      tizen_native_window_->GetTizenNativeEGLWindow());
  if (!tizen_context_gl_->IsValid()) {
    FT_LOGE("Invalid context gl");
    return;
  }

  tizen_egl_window_surface_ = tizen_context_gl_->CreateTizenEGLWindowSurface();
  if (!tizen_egl_window_surface_->IsValid()) {
    FT_LOGE("Invalid egl window surface");
    return;
  }

  tizen_egl_pbuffer_surface_ =
      tizen_context_gl_->CreateTizenEGLPbufferSurface();
  if (!tizen_egl_pbuffer_surface_->IsValid()) {
    FT_LOGE("Invalid egl puffer surface");
    return;
  }

  is_valid_ = true;
}

bool TizenSurfaceGL::OnMakeCurrent() {
  if (!is_valid_) {
    FT_LOGE("Invalid TizenSurfaceGL");
    return false;
  }
  if (eglMakeCurrent(tizen_native_window_->GetTizenNativeEGLWindow()
                         ->GetEGLDisplayHandle(),
                     tizen_egl_window_surface_->GetEGLSurfaceHandle(),
                     tizen_egl_window_surface_->GetEGLSurfaceHandle(),
                     tizen_context_gl_->GetEGLContextHandle()) != EGL_TRUE) {
    FT_LOGE("Could not make the onscreen context current");
    LogLastEGLError();
    return false;
  }
  return true;
}

bool TizenSurfaceGL::OnMakeResourceCurrent() {
  if (!is_valid_) {
    FT_LOGE("Invalid TizenSurfaceGL");
    return false;
  }
  if (eglMakeCurrent(tizen_native_window_->GetTizenNativeEGLWindow()
                         ->GetEGLDisplayHandle(),
                     tizen_egl_pbuffer_surface_->GetEGLSurfaceHandle(),
                     tizen_egl_pbuffer_surface_->GetEGLSurfaceHandle(),
                     tizen_context_gl_->GetEGLResourceContextHandle()) !=
      EGL_TRUE) {
    FT_LOGE("Could not make the offscreen context current");
    LogLastEGLError();
    return false;
  }
  return true;
}

bool TizenSurfaceGL::OnClearCurrent() {
  if (!is_valid_) {
    FT_LOGE("Invalid TizenSurfaceGL");
    return false;
  }

  if (eglMakeCurrent(tizen_native_window_->GetTizenNativeEGLWindow()
                         ->GetEGLDisplayHandle(),
                     EGL_NO_SURFACE, EGL_NO_SURFACE,
                     EGL_NO_CONTEXT) != EGL_TRUE) {
    FT_LOGE("Could not clear context");
    LogLastEGLError();
    return false;
  }
  return true;
}

bool TizenSurfaceGL::OnPresent() {
  if (!is_valid_) {
    FT_LOGE("Invalid TizenSurfaceGL");
    return false;
  }

  if (eglSwapBuffers(tizen_native_window_->GetTizenNativeEGLWindow()
                         ->GetEGLDisplayHandle(),
                     tizen_egl_window_surface_->GetEGLSurfaceHandle()) !=
      EGL_TRUE) {
    FT_LOGE("Could not swap EGl buffer");
    LogLastEGLError();
    return false;
  }
  return true;
}

uint32_t TizenSurfaceGL::OnGetFBO() {
  if (!is_valid_) {
    FT_LOGE("Invalid TizenSurfaceGL");
    return 999;
  }
  FT_LOGD("OnApplicationGetOnscreenFBO");
  return 0;  // FBO0
}

#define GL_FUNC(FunctionName)                     \
  else if (strcmp(name, #FunctionName) == 0) {    \
    return reinterpret_cast<void*>(FunctionName); \
  }

void* TizenSurfaceGL::OnProcResolver(const char* name) {
  auto address = eglGetProcAddress(name);
  if (address != nullptr) {
    return reinterpret_cast<void*>(address);
  }
  GL_FUNC(eglGetCurrentDisplay)
  GL_FUNC(eglQueryString)
  GL_FUNC(glActiveTexture)
  GL_FUNC(glAttachShader)
  GL_FUNC(glBindAttribLocation)
  GL_FUNC(glBindBuffer)
  GL_FUNC(glBindFramebuffer)
  GL_FUNC(glBindRenderbuffer)
  GL_FUNC(glBindTexture)
  GL_FUNC(glBlendColor)
  GL_FUNC(glBlendEquation)
  GL_FUNC(glBlendFunc)
  GL_FUNC(glBufferData)
  GL_FUNC(glBufferSubData)
  GL_FUNC(glCheckFramebufferStatus)
  GL_FUNC(glClear)
  GL_FUNC(glClearColor)
  GL_FUNC(glClearStencil)
  GL_FUNC(glColorMask)
  GL_FUNC(glCompileShader)
  GL_FUNC(glCompressedTexImage2D)
  GL_FUNC(glCompressedTexSubImage2D)
  GL_FUNC(glCopyTexSubImage2D)
  GL_FUNC(glCreateProgram)
  GL_FUNC(glCreateShader)
  GL_FUNC(glCullFace)
  GL_FUNC(glDeleteBuffers)
  GL_FUNC(glDeleteFramebuffers)
  GL_FUNC(glDeleteProgram)
  GL_FUNC(glDeleteRenderbuffers)
  GL_FUNC(glDeleteShader)
  GL_FUNC(glDeleteTextures)
  GL_FUNC(glDepthMask)
  GL_FUNC(glDisable)
  GL_FUNC(glDisableVertexAttribArray)
  GL_FUNC(glDrawArrays)
  GL_FUNC(glDrawElements)
  GL_FUNC(glEnable)
  GL_FUNC(glEnableVertexAttribArray)
  GL_FUNC(glFinish)
  GL_FUNC(glFlush)
  GL_FUNC(glFramebufferRenderbuffer)
  GL_FUNC(glFramebufferTexture2D)
  GL_FUNC(glFrontFace)
  GL_FUNC(glGenBuffers)
  GL_FUNC(glGenerateMipmap)
  GL_FUNC(glGenFramebuffers)
  GL_FUNC(glGenRenderbuffers)
  GL_FUNC(glGenTextures)
  GL_FUNC(glGetBufferParameteriv)
  GL_FUNC(glGetError)
  GL_FUNC(glGetFramebufferAttachmentParameteriv)
  GL_FUNC(glGetIntegerv)
  GL_FUNC(glGetProgramInfoLog)
  GL_FUNC(glGetProgramiv)
  GL_FUNC(glGetRenderbufferParameteriv)
  GL_FUNC(glGetShaderInfoLog)
  GL_FUNC(glGetShaderiv)
  GL_FUNC(glGetShaderPrecisionFormat)
  GL_FUNC(glGetString)
  GL_FUNC(glGetUniformLocation)
  GL_FUNC(glIsTexture)
  GL_FUNC(glLineWidth)
  GL_FUNC(glLinkProgram)
  GL_FUNC(glPixelStorei)
  GL_FUNC(glReadPixels)
  GL_FUNC(glRenderbufferStorage)
  GL_FUNC(glScissor)
  GL_FUNC(glShaderSource)
  GL_FUNC(glStencilFunc)
  GL_FUNC(glStencilFuncSeparate)
  GL_FUNC(glStencilMask)
  GL_FUNC(glStencilMaskSeparate)
  GL_FUNC(glStencilOp)
  GL_FUNC(glStencilOpSeparate)
  GL_FUNC(glTexImage2D)
  GL_FUNC(glTexParameterf)
  GL_FUNC(glTexParameterfv)
  GL_FUNC(glTexParameteri)
  GL_FUNC(glTexParameteriv)
  GL_FUNC(glTexSubImage2D)
  GL_FUNC(glUniform1f)
  GL_FUNC(glUniform1fv)
  GL_FUNC(glUniform1i)
  GL_FUNC(glUniform1iv)
  GL_FUNC(glUniform2f)
  GL_FUNC(glUniform2fv)
  GL_FUNC(glUniform2i)
  GL_FUNC(glUniform2iv)
  GL_FUNC(glUniform3f)
  GL_FUNC(glUniform3fv)
  GL_FUNC(glUniform3i)
  GL_FUNC(glUniform3iv)
  GL_FUNC(glUniform4f)
  GL_FUNC(glUniform4fv)
  GL_FUNC(glUniform4i)
  GL_FUNC(glUniform4iv)
  GL_FUNC(glUniformMatrix2fv)
  GL_FUNC(glUniformMatrix3fv)
  GL_FUNC(glUniformMatrix4fv)
  GL_FUNC(glUseProgram)
  GL_FUNC(glVertexAttrib1f)
  GL_FUNC(glVertexAttrib2fv)
  GL_FUNC(glVertexAttrib3fv)
  GL_FUNC(glVertexAttrib4fv)
  GL_FUNC(glVertexAttribPointer)
  GL_FUNC(glViewport)
  FT_LOGD("Could not resolve: %s", name);
  return nullptr;
}
#undef GL_FUNC

TizenSurfaceGL::~TizenSurfaceGL() {
  OnClearCurrent();
  tizen_egl_window_surface_ = nullptr;
  tizen_egl_pbuffer_surface_ = nullptr;
  tizen_context_gl_ = nullptr;
  tizen_native_window_ = nullptr;
}
