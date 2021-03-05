// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_renderer.h"

#ifndef FLUTTER_TIZEN_EVASGL
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#include <Evas_GL_GLES3_Helpers.h>
Evas_GL* g_evas_gl = nullptr;
EVAS_GL_GLOBAL_GLES3_DEFINE();
#endif

#include "flutter/shell/platform/tizen/tizen_log.h"

TizenRenderer::TizenRenderer(TizenRenderer::Delegate& delegate)
    : delegate_(delegate) {}

TizenRenderer::~TizenRenderer() = default;

#ifndef FLUTTER_TIZEN_EVASGL

bool TizenRenderer::OnMakeCurrent() {
  if (!IsValid()) {
    FT_LOGE("Invalid TizenRenderer");
    return false;
  }
  if (eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_) !=
      EGL_TRUE) {
    FT_LOGE("Could not make the onscreen context current");
    PrintEGLError();
    return false;
  }
  return true;
}

bool TizenRenderer::OnClearCurrent() {
  if (!IsValid()) {
    FT_LOGE("Invalid TizenRenderer");
    return false;
  }
  if (eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                     EGL_NO_CONTEXT) != EGL_TRUE) {
    FT_LOGE("Could not clear context");
    PrintEGLError();
    return false;
  }
  return true;
}

bool TizenRenderer::OnMakeResourceCurrent() {
  if (!IsValid()) {
    FT_LOGE("Invalid TizenRenderer");
    return false;
  }
  if (eglMakeCurrent(egl_display_, egl_resource_surface_, egl_resource_surface_,
                     egl_resource_context_) != EGL_TRUE) {
    FT_LOGE("Could not make the offscreen context current");
    PrintEGLError();
    return false;
  }
  return true;
}

bool TizenRenderer::OnPresent() {
  if (!is_valid_) {
    FT_LOGE("Invalid TizenRenderer");
    return false;
  }

  if (received_rotation) {
    SendRotationChangeDone();
    received_rotation = false;
  }

  if (eglSwapBuffers(egl_display_, egl_surface_) != EGL_TRUE) {
    FT_LOGE("Could not swap EGl buffer");
    PrintEGLError();
    return false;
  }
  return true;
}

uint32_t TizenRenderer::OnGetFBO() {
  if (!is_valid_) {
    FT_LOGE("Invalid TizenRenderer");
    return 999;
  }
  FT_LOGD("OnGetFBO");
  return 0;
}

#define GL_FUNC(FunctionName)                     \
  else if (strcmp(name, #FunctionName) == 0) {    \
    return reinterpret_cast<void*>(FunctionName); \
  }
void* TizenRenderer::OnProcResolver(const char* name) {
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

  FT_LOGW("Could not resolve: %s", name);
  return nullptr;
}
#undef GL_FUNC

bool TizenRenderer::InitializeRenderer(int32_t x, int32_t y, int32_t w,
                                       int32_t h) {
  if (!SetupDisplay()) {
    FT_LOGE("setupDisplay fail");
    return false;
  }
  if (!SetupEcoreWlWindow(x, y, w, h)) {
    FT_LOGE("SetupEcoreWlWindow fail");
    return false;
  }

  if (!SetupEglWindow(w, h)) {
    FT_LOGE("SetupEglWindow fail");
    return false;
  }

  if (!SetupEglSurface()) {
    FT_LOGE("setupEglSurface fail");
    return false;
  }
  Show();
  is_valid_ = true;
  return true;
}

bool TizenRenderer::IsValid() { return is_valid_; }

bool TizenRenderer::SetupEglSurface() {
  if (!ChooseEGLConfiguration()) {
    FT_LOGE("ChooseEGLConfiguration fail");
    return false;
  }
  const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
  egl_context_ = eglCreateContext(egl_display_, egl_config_, EGL_NO_CONTEXT,
                                  contextAttribs);
  if (EGL_NO_CONTEXT == egl_context_) {
    PrintEGLError();
    return false;
  }

  egl_resource_context_ =
      eglCreateContext(egl_display_, egl_config_, egl_context_, contextAttribs);
  if (EGL_NO_CONTEXT == egl_resource_context_) {
    PrintEGLError();
    return false;
  }
  EGLint* ptr = nullptr;
  const EGLint attribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
  egl_surface_ = eglCreateWindowSurface(egl_display_, egl_config_,
                                        GetEGLNativeWindowType(), ptr);
  if (egl_surface_ == EGL_NO_SURFACE) {
    FT_LOGE("eglCreateWindowSurface failed");
    return false;
  }
  egl_resource_surface_ =
      eglCreatePbufferSurface(egl_display_, egl_config_, attribs);
  if (egl_resource_surface_ == EGL_NO_SURFACE) {
    FT_LOGE("eglCreatePbufferSurface is Failed");
    return false;
  }
  return true;
}

bool TizenRenderer::ChooseEGLConfiguration() {
  // egl CONTEXT
  EGLint configAttribs[] = {
      // clang-format off
      EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
      EGL_RED_SIZE,        8,
      EGL_GREEN_SIZE,      8,
      EGL_BLUE_SIZE,       8,
      EGL_ALPHA_SIZE,      EGL_DONT_CARE,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_SAMPLE_BUFFERS,  EGL_DONT_CARE,
      EGL_SAMPLES,         EGL_DONT_CARE,
      EGL_NONE
      // clang-format on
  };

  EGLint major = 0;
  EGLint minor = 0;
  int bufferSize = 32;
  egl_display_ = GetEGLDisplay();
  if (EGL_NO_DISPLAY == egl_display_) {
    FT_LOGE("EGL Get Display is failed");
    return false;
  }

  if (!eglInitialize(egl_display_, &major, &minor)) {
    FT_LOGE("EGL Intialize is Failed major [%d] minor [%d]", major, minor);
    PrintEGLError();
    return false;
  }

  if (!eglBindAPI(EGL_OPENGL_ES_API)) {
    PrintEGLError();
    return false;
  }

  EGLint numOfConfig = 0;
  // Query all framebuffer configurations
  if (!eglGetConfigs(egl_display_, NULL, 0, &numOfConfig)) {
    FT_LOGE("eglGetConfigs is Failed!!");
    PrintEGLError();
    return false;
  }
  EGLConfig* configs = (EGLConfig*)calloc(numOfConfig, sizeof(EGLConfig));
  EGLint n;
  // Get the List of EGL framebuffer configuration matches with configAttribs in
  // list "configs"
  if (!eglChooseConfig(egl_display_, configAttribs, configs, numOfConfig, &n)) {
    free(configs);
    configs = NULL;
    PrintEGLError();
    return false;
  }

  EGLint size;
  for (int i = 0; i < n; i++) {
    eglGetConfigAttrib(egl_display_, configs[i], EGL_BUFFER_SIZE, &size);
    if (bufferSize == size) {
      egl_config_ = configs[i];
      break;
    }
  }
  free(configs);
  configs = NULL;
  return true;
}

void TizenRenderer::PrintEGLError() {
  EGLint error = eglGetError();
  switch (error) {
    case EGL_BAD_DISPLAY: {
      FT_LOGE("EGL_BAD_DISPLAY : Display is not an EGL display connection");
      break;
    }
    case EGL_NOT_INITIALIZED: {
      FT_LOGE("EGL_NOT_INITIALIZED : Display has not been initialized");
      break;
    }
    case EGL_BAD_SURFACE: {
      FT_LOGE("EGL_BAD_SURFACE : Draw or read is not an EGL surface");
      break;
    }
    case EGL_BAD_CONTEXT: {
      FT_LOGE("EGL_BAD_CONTEXT : Context is not an EGL rendering context");
      break;
    }
    case EGL_BAD_CONFIG: {
      FT_LOGE(
          "EGL_BAD_CONFIG : Config is not an EGL frame buffer configuration");
      break;
    }
    case EGL_BAD_MATCH: {
      FT_LOGE(
          "EGL_BAD_MATCH : Draw or read are not compatible with context, or if "
          "context is set to EGL_NO_CONTEXT and draw or read are not set to "
          "EGL_NO_SURFACE, or if draw or read are set to EGL_NO_SURFACE and "
          "context is not set to EGL_NO_CONTEXT\n");
      break;
    }
    case EGL_BAD_ACCESS: {
      FT_LOGE("EGL_BAD_ACCESS : Context is current to some other thread");
      break;
    }
    case EGL_BAD_NATIVE_PIXMAP: {
      FT_LOGE(
          "EGL_BAD_NATIVE_PIXMAP : A native pixmap underlying either draw or "
          "read is no longer valid");
      break;
    }
    case EGL_BAD_NATIVE_WINDOW: {
      FT_LOGE(
          "EGL_BAD_NATIVE_WINDOW : A native window underlying either draw or "
          "read is no longer valid");
      break;
    }
    case EGL_BAD_CURRENT_SURFACE: {
      FT_LOGE(
          "EGL_BAD_CURRENT_SURFACE : The previous context has unflushed "
          "commands and the previous surface is no longer valid");
      break;
    }
    case EGL_BAD_ALLOC: {
      FT_LOGE(
          "EGL_BAD_ALLOC : Allocation of ancillary buffers for draw or read "
          "were delayed until eglMakeCurrent is called, and there are not "
          "enough resources to allocate them");
      break;
    }
    case EGL_CONTEXT_LOST: {
      FT_LOGE(
          "EGL_CONTEXT_LOST : A power management event has occurred. The "
          "application must destroy all contexts and reinitialise OpenGL ES "
          "state and objects to continue rendering");
      break;
    }
    case EGL_BAD_PARAMETER: {
      FT_LOGE("Invalid parameter is passed");
      break;
    }
    case EGL_BAD_ATTRIBUTE: {
      FT_LOGE(
          "The parameter configAttribs contains an invalid frame buffer "
          "configuration attribute or an attribute value that is unrecognized "
          "or out of range");
      break;
    }
    default: {
      FT_LOGE("Unknown error with code: %d", error);
      break;
    }
  }
}

void TizenRenderer::DestoryRenderer() {
  DestoryEglSurface();
  DestoryEglWindow();
  DestoryEcoreWlWindow();
  ShutdownDisplay();
}

void TizenRenderer::DestoryEglSurface() {
  if (EGL_NO_DISPLAY != egl_display_) {
    eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                   EGL_NO_CONTEXT);

    if (EGL_NO_SURFACE != egl_surface_) {
      eglDestroySurface(egl_display_, egl_surface_);
      egl_surface_ = EGL_NO_SURFACE;
    }

    if (EGL_NO_CONTEXT != egl_context_) {
      eglDestroyContext(egl_display_, egl_context_);
      egl_context_ = EGL_NO_CONTEXT;
    }

    if (EGL_NO_SURFACE != egl_resource_surface_) {
      eglDestroySurface(egl_display_, egl_resource_surface_);
      egl_resource_surface_ = EGL_NO_SURFACE;
    }

    if (EGL_NO_CONTEXT != egl_resource_context_) {
      eglDestroyContext(egl_display_, egl_resource_context_);
      egl_resource_context_ = EGL_NO_CONTEXT;
    }

    eglTerminate(egl_display_);
    egl_display_ = EGL_NO_DISPLAY;
  }
}
#else

void TizenRenderer::ClearColor(float r, float g, float b, float a) {
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT);
}

bool TizenRenderer::OnMakeCurrent() {
  if (!IsValid()) {
    FT_LOGE("Invalid TizenRenderer");
    return false;
  }
  if (evas_gl_make_current(evas_gl_, gl_surface_, gl_context_) != EINA_TRUE) {
    return false;
  }
  evas_object_image_pixels_dirty_set((Evas_Object*)GetImageHandle(), EINA_TRUE);
  return true;
}

bool TizenRenderer::OnClearCurrent() {
  if (!IsValid()) {
    FT_LOGE("Invalid TizenRenderer");
    return false;
  }
  if (evas_gl_make_current(evas_gl_, NULL, NULL) != EINA_TRUE) {
    return false;
  }
  return true;
}

bool TizenRenderer::OnMakeResourceCurrent() {
  if (!IsValid()) {
    FT_LOGE("Invalid TizenRenderer");
    return false;
  }
  if (evas_gl_make_current(evas_gl_, gl_resource_surface_,
                           gl_resource_context_) != EINA_TRUE) {
    return false;
  }
  return true;
}

bool TizenRenderer::OnPresent() {
  if (!is_valid_) {
    FT_LOGE("Invalid TizenRenderer");
    return false;
  }
  if (received_rotation) {
    SendRotationChangeDone();
    received_rotation = false;
  }
  return true;
}

uint32_t TizenRenderer::OnGetFBO() {
  if (!is_valid_) {
    FT_LOGE("Invalid TizenRenderer");
    return 999;
  }
  FT_LOGD("OnGetFBO");
  return 0;
}

#define GL_FUNC(FunctionName)                     \
  else if (strcmp(name, #FunctionName) == 0) {    \
    return reinterpret_cast<void*>(FunctionName); \
  }
void* TizenRenderer::OnProcResolver(const char* name) {
  auto address = evas_gl_proc_address_get(evas_gl_, name);
  if (address != nullptr) {
    return reinterpret_cast<void*>(address);
  }
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
  GL_FUNC(glGetStringi)
  FT_LOGD("Could not resolve: %s", name);
  return nullptr;
}
#undef GL_FUNC

bool TizenRenderer::InitializeRenderer(int32_t x, int32_t y, int32_t w,
                                       int32_t h) {
  if (!SetupEvasGL(x, y, w, h)) {
    FT_LOGE("SetupEvasGL fail");
    return false;
  }
  Show();
  is_valid_ = true;
  return true;
}

bool TizenRenderer::IsValid() { return is_valid_; }

bool TizenRenderer::SetupEvasGL(int32_t x, int32_t y, int32_t w, int32_t h) {
  evas_gl_ = evas_gl_new(
      evas_object_evas_get((Evas_Object*)SetupEvasWindow(x, y, w, h)));

  if (!evas_gl_) {
    FT_LOGE("SetupEvasWindow fail");
    return false;
  }

  g_evas_gl = evas_gl_;

  gl_config_ = evas_gl_config_new();
  gl_config_->color_format = EVAS_GL_RGBA_8888;
  gl_config_->depth_bits = EVAS_GL_DEPTH_NONE;
  gl_config_->stencil_bits = EVAS_GL_STENCIL_NONE;
  gl_config_->options_bits = EVAS_GL_OPTIONS_NONE;

#define EVAS_GL_OPTIONS_DIRECT_MEMORY_OPTIMIZE (1 << 12)
#define EVAS_GL_OPTIONS_DIRECT_OVERRIDE (1 << 13)
  // gl_config_->options_bits = (Evas_GL_Options_Bits)(
  //     EVAS_GL_OPTIONS_DIRECT | EVAS_GL_OPTIONS_DIRECT_OVERRIDE |
  //     EVAS_GL_OPTIONS_DIRECT_MEMORY_OPTIMIZE |
  //     EVAS_GL_OPTIONS_CLIENT_SIDE_ROTATION);

  gl_context_ = evas_gl_context_create(evas_gl_, NULL);
  gl_resource_context_ = evas_gl_context_create(evas_gl_, gl_context_);
  EVAS_GL_GLOBAL_GLES3_USE(g_evas_gl, gl_context_);
  gl_surface_ = evas_gl_surface_create(evas_gl_, gl_config_, w, h);

  gl_resource_surface_ =
      evas_gl_pbuffer_surface_create(evas_gl_, gl_config_, w, h, NULL);

  Evas_Native_Surface ns;
  evas_gl_native_surface_get(evas_gl_, gl_surface_, &ns);
  evas_object_image_native_surface_set((Evas_Object*)GetImageHandle(), &ns);
  pixelDirtyCallback_ = [](void* data, Evas_Object* o) {};
  evas_object_image_pixels_get_callback_set((Evas_Object*)GetImageHandle(),
                                            pixelDirtyCallback_, NULL);
  return true;
}

void TizenRenderer::DestoryRenderer() {
  DestoryEvasGL();
  DestoryEvasWindow();
}

void TizenRenderer::DestoryEvasGL() {
  evas_gl_surface_destroy(evas_gl_, gl_surface_);
  evas_gl_surface_destroy(evas_gl_, gl_resource_surface_);

  evas_gl_context_destroy(evas_gl_, gl_context_);
  evas_gl_context_destroy(evas_gl_, gl_resource_context_);

  evas_gl_config_free(gl_config_);
  evas_gl_free(evas_gl_);
}

#endif