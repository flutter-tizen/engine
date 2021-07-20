// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_renderer_ecore_wl2.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

TizenRendererEcoreWl2::TizenRendererEcoreWl2(WindowGeometry geometry,
                                             bool transparent,
                                             bool focusable,
                                             Delegate& delegate)
    : TizenRenderer(geometry, transparent, focusable, delegate) {
  InitializeRenderer();
}

TizenRendererEcoreWl2::~TizenRendererEcoreWl2() {
  DestroyRenderer();
}

bool TizenRendererEcoreWl2::OnMakeCurrent() {
  if (!IsValid()) {
    return false;
  }
  if (eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_) !=
      EGL_TRUE) {
    PrintEGLError();
    return false;
  }
  return true;
}

bool TizenRendererEcoreWl2::OnClearCurrent() {
  if (!IsValid()) {
    return false;
  }
  if (eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                     EGL_NO_CONTEXT) != EGL_TRUE) {
    PrintEGLError();
    return false;
  }
  return true;
}

bool TizenRendererEcoreWl2::OnMakeResourceCurrent() {
  if (!IsValid()) {
    return false;
  }
  if (eglMakeCurrent(egl_display_, egl_resource_surface_, egl_resource_surface_,
                     egl_resource_context_) != EGL_TRUE) {
    PrintEGLError();
    return false;
  }
  return true;
}

bool TizenRendererEcoreWl2::OnPresent() {
  if (!IsValid()) {
    return false;
  }

  if (received_rotation_) {
    SendRotationChangeDone();
    received_rotation_ = false;
  }

  if (eglSwapBuffers(egl_display_, egl_surface_) != EGL_TRUE) {
    PrintEGLError();
    return false;
  }
  return true;
}

uint32_t TizenRendererEcoreWl2::OnGetFBO() {
  if (!is_valid_) {
    return 999;
  }
  return 0;
}

void* TizenRendererEcoreWl2::OnProcResolver(const char* name) {
  auto address = eglGetProcAddress(name);
  if (address != nullptr) {
    return reinterpret_cast<void*>(address);
  }
#define GL_FUNC(FunctionName)                     \
  else if (strcmp(name, #FunctionName) == 0) {    \
    return reinterpret_cast<void*>(FunctionName); \
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
#undef GL_FUNC

  FT_LOG(Warn) << "Could not resolve: " << name;
  return nullptr;
}

TizenRenderer::WindowGeometry TizenRendererEcoreWl2::GetCurrentGeometry() {
  WindowGeometry result;
  ecore_wl2_window_geometry_get(ecore_wl2_window_, &result.x, &result.y,
                                &result.w, &result.h);
  return result;
}

int32_t TizenRendererEcoreWl2::GetDpi() {
  auto* output = ecore_wl2_window_output_find(ecore_wl2_window_);
  if (!output) {
    FT_LOG(Error) << "Could not find an output associated with the window.";
    return 0;
  }
  return ecore_wl2_output_dpi_get(output);
}

uintptr_t TizenRendererEcoreWl2::GetWindowId() {
  return ecore_wl2_window_id_get(ecore_wl2_window_);
}

void* TizenRendererEcoreWl2::GetWindowHandle() {
  return ecore_wl2_window_;
}

bool TizenRendererEcoreWl2::InitializeRenderer() {
  int32_t width, height;
  if (!SetupDisplay(&width, &height)) {
    FT_LOG(Error) << "SetupDisplay() failed.";
    return false;
  }
  if (!SetupEcoreWlWindow(width, height)) {
    FT_LOG(Error) << "SetupEcoreWlWindow() failed.";
    return false;
  }
  if (!SetupEglWindow(width, height)) {
    FT_LOG(Error) << "SetupEglWindow() failed.";
    return false;
  }
  if (!SetupEglSurface()) {
    FT_LOG(Error) << "SetupEglSurface() failed.";
    return false;
  }
  Show();
  is_valid_ = true;
  return true;
}

void TizenRendererEcoreWl2::Show() {
  ecore_wl2_window_show(ecore_wl2_window_);
}

void TizenRendererEcoreWl2::DestroyRenderer() {
  DestroyEglSurface();
  DestroyEglWindow();
  DestroyEcoreWlWindow();
  ShutdownDisplay();
}

bool TizenRendererEcoreWl2::SetupDisplay(int32_t* width, int32_t* height) {
  if (!ecore_wl2_init()) {
    FT_LOG(Error) << "Could not initialize ecore_wl2.";
    return false;
  }
  ecore_wl2_display_ = ecore_wl2_display_connect(nullptr);
  if (ecore_wl2_display_ == nullptr) {
    FT_LOG(Error) << "Display not found.";
    return false;
  }
  ecore_wl2_sync();

  ecore_wl2_display_screen_size_get(ecore_wl2_display_, width, height);
  if (*width == 0 || *height == 0) {
    FT_LOG(Error) << "Invalid screen size: " << *width << " x " << *height;
    return false;
  }
  if (initial_geometry_.w > 0) {
    *width = initial_geometry_.w;
  }
  if (initial_geometry_.h > 0) {
    *height = initial_geometry_.h;
  }

  return true;
}

bool TizenRendererEcoreWl2::SetupEcoreWlWindow(int32_t width, int32_t height) {
  int32_t x = initial_geometry_.x;
  int32_t y = initial_geometry_.y;

  ecore_wl2_window_ =
      ecore_wl2_window_new(ecore_wl2_display_, nullptr, x, y, width, height);
  ecore_wl2_window_type_set(ecore_wl2_window_, ECORE_WL2_WINDOW_TYPE_TOPLEVEL);
  ecore_wl2_window_position_set(ecore_wl2_window_, x, y);
  ecore_wl2_window_aux_hint_add(ecore_wl2_window_, 0,
                                "wm.policy.win.user.geometry", "1");

  if (transparent_) {
    ecore_wl2_window_alpha_set(ecore_wl2_window_, EINA_TRUE);
  } else {
    ecore_wl2_window_alpha_set(ecore_wl2_window_, EINA_FALSE);
  }
  if (!focusable_) {
    ecore_wl2_window_focus_skip_set(ecore_wl2_window_, EINA_TRUE);
  }

  int rotations[4] = {0, 90, 180, 270};
  ecore_wl2_window_available_rotations_set(ecore_wl2_window_, rotations,
                                           sizeof(rotations) / sizeof(int));
  ecore_event_handler_add(ECORE_WL2_EVENT_WINDOW_ROTATE, RotationEventCb, this);

  return true;
}

bool TizenRendererEcoreWl2::SetupEglWindow(int32_t width, int32_t height) {
  ecore_wl2_egl_window_ =
      ecore_wl2_egl_window_create(ecore_wl2_window_, width, height);
  return ecore_wl2_egl_window_ != nullptr;
}

EGLDisplay TizenRendererEcoreWl2::GetEGLDisplay() {
  return eglGetDisplay(ecore_wl2_display_get(ecore_wl2_display_));
}

EGLNativeWindowType TizenRendererEcoreWl2::GetEGLNativeWindowType() {
  return ecore_wl2_egl_window_native_get(ecore_wl2_egl_window_);
}

void TizenRendererEcoreWl2::DestroyEglWindow() {
  if (ecore_wl2_egl_window_) {
    ecore_wl2_egl_window_destroy(ecore_wl2_egl_window_);
    ecore_wl2_egl_window_ = nullptr;
  }
}

void TizenRendererEcoreWl2::DestroyEcoreWlWindow() {
  if (ecore_wl2_window_) {
    ecore_wl2_window_free(ecore_wl2_window_);
    ecore_wl2_window_ = nullptr;
  }
}

void TizenRendererEcoreWl2::ShutdownDisplay() {
  if (ecore_wl2_display_) {
    ecore_wl2_display_disconnect(ecore_wl2_display_);
    ecore_wl2_display_ = nullptr;
  }
  ecore_wl2_shutdown();
}

bool TizenRendererEcoreWl2::SetupEglSurface() {
  if (!ChooseEGLConfiguration()) {
    FT_LOG(Error) << "ChooseEGLConfiguration() failed.";
    return false;
  }

  const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
  egl_context_ = eglCreateContext(egl_display_, egl_config_, EGL_NO_CONTEXT,
                                  context_attribs);
  if (EGL_NO_CONTEXT == egl_context_) {
    PrintEGLError();
    return false;
  }

  egl_resource_context_ = eglCreateContext(egl_display_, egl_config_,
                                           egl_context_, context_attribs);
  if (EGL_NO_CONTEXT == egl_resource_context_) {
    PrintEGLError();
    return false;
  }

  EGLint* ptr = nullptr;
  egl_surface_ = eglCreateWindowSurface(egl_display_, egl_config_,
                                        GetEGLNativeWindowType(), ptr);
  if (egl_surface_ == EGL_NO_SURFACE) {
    FT_LOG(Error) << "eglCreateWindowSurface() failed.";
    return false;
  }

  const EGLint attribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
  egl_resource_surface_ =
      eglCreatePbufferSurface(egl_display_, egl_config_, attribs);
  if (egl_resource_surface_ == EGL_NO_SURFACE) {
    FT_LOG(Error) << "eglCreatePbufferSurface() failed.";
    return false;
  }

  return true;
}

bool TizenRendererEcoreWl2::ChooseEGLConfiguration() {
  // egl CONTEXT
  EGLint config_attribs[] = {
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
  int buffer_size = 32;
  egl_display_ = GetEGLDisplay();
  if (EGL_NO_DISPLAY == egl_display_) {
    FT_LOG(Error) << "eglGetDisplay() failed.";
    return false;
  }

  if (!eglInitialize(egl_display_, &major, &minor)) {
    PrintEGLError();
    return false;
  }

  if (!eglBindAPI(EGL_OPENGL_ES_API)) {
    PrintEGLError();
    return false;
  }

  EGLint num_config = 0;
  // Query all framebuffer configurations.
  if (!eglGetConfigs(egl_display_, NULL, 0, &num_config)) {
    PrintEGLError();
    return false;
  }
  EGLConfig* configs = (EGLConfig*)calloc(num_config, sizeof(EGLConfig));
  EGLint num;
  // Get the List of EGL framebuffer configuration matches with config_attribs
  // in list "configs".
  if (!eglChooseConfig(egl_display_, config_attribs, configs, num_config,
                       &num)) {
    free(configs);
    configs = NULL;
    PrintEGLError();
    return false;
  }

  EGLint size;
  for (int i = 0; i < num; i++) {
    eglGetConfigAttrib(egl_display_, configs[i], EGL_BUFFER_SIZE, &size);
    if (buffer_size == size) {
      egl_config_ = configs[i];
      break;
    }
  }
  free(configs);
  configs = NULL;
  return true;
}

void TizenRendererEcoreWl2::PrintEGLError() {
  EGLint error = eglGetError();
  switch (error) {
#define CASE_PRINT(value)                     \
  case value: {                               \
    FT_LOG(Error) << "EGL error: " << #value; \
    break;                                    \
  }
    CASE_PRINT(EGL_NOT_INITIALIZED)
    CASE_PRINT(EGL_BAD_ACCESS)
    CASE_PRINT(EGL_BAD_ALLOC)
    CASE_PRINT(EGL_BAD_ATTRIBUTE)
    CASE_PRINT(EGL_BAD_CONTEXT)
    CASE_PRINT(EGL_BAD_CONFIG)
    CASE_PRINT(EGL_BAD_CURRENT_SURFACE)
    CASE_PRINT(EGL_BAD_DISPLAY)
    CASE_PRINT(EGL_BAD_SURFACE)
    CASE_PRINT(EGL_BAD_MATCH)
    CASE_PRINT(EGL_BAD_PARAMETER)
    CASE_PRINT(EGL_BAD_NATIVE_PIXMAP)
    CASE_PRINT(EGL_BAD_NATIVE_WINDOW)
    CASE_PRINT(EGL_CONTEXT_LOST)
#undef CASE_PRINT
    default: {
      FT_LOG(Error) << "Unknown EGL error: " << error;
    }
  }
}

void TizenRendererEcoreWl2::DestroyEglSurface() {
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

Eina_Bool TizenRendererEcoreWl2::RotationEventCb(void* data,
                                                 int type,
                                                 void* event) {
  auto* self = reinterpret_cast<TizenRendererEcoreWl2*>(data);
  auto* ev = reinterpret_cast<Ecore_Wl2_Event_Window_Rotation*>(event);
  self->delegate_.OnOrientationChange(ev->angle);
  return ECORE_CALLBACK_PASS_ON;
}

void TizenRendererEcoreWl2::SetRotate(int angle) {
  ecore_wl2_window_rotation_set(ecore_wl2_window_, angle);
  received_rotation_ = true;
}

void TizenRendererEcoreWl2::ResizeWithRotation(int32_t x,
                                               int32_t y,
                                               int32_t width,
                                               int32_t height,
                                               int32_t angle) {
  ecore_wl2_egl_window_resize_with_rotation(ecore_wl2_egl_window_, x, y, width,
                                            height, angle);
}

void TizenRendererEcoreWl2::SendRotationChangeDone() {
  int x, y, w, h;
  ecore_wl2_window_geometry_get(ecore_wl2_window_, &x, &y, &w, &h);
  ecore_wl2_window_rotation_change_done_send(
      ecore_wl2_window_, ecore_wl2_window_rotation_get(ecore_wl2_window_), w,
      h);
}

void TizenRendererEcoreWl2::SetPreferredOrientations(
    const std::vector<int>& rotations) {
  ecore_wl2_window_available_rotations_set(ecore_wl2_window_, rotations.data(),
                                           rotations.size());
}

}  // namespace flutter
