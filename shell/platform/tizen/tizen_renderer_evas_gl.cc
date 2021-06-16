// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_renderer_evas_gl.h"

#include <Evas_GL_GLES3_Helpers.h>
Evas_GL* g_evas_gl = nullptr;
EVAS_GL_GLOBAL_GLES3_DEFINE();

#include "flutter/shell/platform/tizen/tizen_log.h"

TizenRendererEvasGL::TizenRendererEvasGL(TizenRenderer::Delegate& delegate)
    : TizenRenderer(delegate) {
  InitializeRenderer();

  // Clear once to remove noise.
  OnMakeCurrent();
  ClearColor(0, 0, 0, 0);
  OnPresent();
}

TizenRendererEvasGL::~TizenRendererEvasGL() {
  DestroyRenderer();
}

void TizenRendererEvasGL::ClearColor(float r, float g, float b, float a) {
  glClearColor(r, g, b, a);
  glClear(GL_COLOR_BUFFER_BIT);
}

bool TizenRendererEvasGL::OnMakeCurrent() {
  if (!IsValid()) {
    FT_LOGE("Invalid TizenRenderer");
    return false;
  }
  if (evas_gl_make_current(evas_gl_, gl_surface_, gl_context_) != EINA_TRUE) {
    return false;
  }

  return true;
}

bool TizenRendererEvasGL::OnClearCurrent() {
  if (!IsValid()) {
    FT_LOGE("Invalid TizenRenderer");
    return false;
  }
  if (evas_gl_make_current(evas_gl_, NULL, NULL) != EINA_TRUE) {
    return false;
  }
  return true;
}

bool TizenRendererEvasGL::OnMakeResourceCurrent() {
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

bool TizenRendererEvasGL::OnPresent() {
  if (!IsValid()) {
    FT_LOGE("Invalid TizenRenderer");
    return false;
  }

  if (received_rotation_) {
    SendRotationChangeDone();
    received_rotation_ = false;
  }

  return true;
}

uint32_t TizenRendererEvasGL::OnGetFBO() {
  if (!is_valid_) {
    FT_LOGE("Invalid TizenRenderer");
    return 999;
  }
  return 0;
}

void* TizenRendererEvasGL::OnProcResolver(const char* name) {
  auto address = evas_gl_proc_address_get(evas_gl_, name);
  if (address != nullptr) {
    return reinterpret_cast<void*>(address);
  }
#define GL_FUNC(FunctionName)                     \
  else if (strcmp(name, #FunctionName) == 0) {    \
    return reinterpret_cast<void*>(FunctionName); \
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
  GL_FUNC(glBlendEquationSeparate)
  GL_FUNC(glBlendFunc)
  GL_FUNC(glBlendFuncSeparate)
  GL_FUNC(glBufferData)
  GL_FUNC(glBufferSubData)
  GL_FUNC(glCheckFramebufferStatus)
  GL_FUNC(glClear)
  GL_FUNC(glClearColor)
  GL_FUNC(glClearDepthf)
  GL_FUNC(glClearStencil)
  GL_FUNC(glColorMask)
  GL_FUNC(glCompileShader)
  GL_FUNC(glCompressedTexImage2D)
  GL_FUNC(glCompressedTexSubImage2D)
  GL_FUNC(glCopyTexImage2D)
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
  GL_FUNC(glDepthFunc)
  GL_FUNC(glDepthMask)
  GL_FUNC(glDepthRangef)
  GL_FUNC(glDetachShader)
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
  GL_FUNC(glGetActiveAttrib)
  GL_FUNC(glGetActiveUniform)
  GL_FUNC(glGetAttachedShaders)
  GL_FUNC(glGetAttribLocation)
  GL_FUNC(glGetBooleanv)
  GL_FUNC(glGetBufferParameteriv)
  GL_FUNC(glGetError)
  GL_FUNC(glGetFloatv)
  GL_FUNC(glGetFramebufferAttachmentParameteriv)
  GL_FUNC(glGetIntegerv)
  GL_FUNC(glGetProgramiv)
  GL_FUNC(glGetProgramInfoLog)
  GL_FUNC(glGetRenderbufferParameteriv)
  GL_FUNC(glGetShaderiv)
  GL_FUNC(glGetShaderInfoLog)
  GL_FUNC(glGetShaderPrecisionFormat)
  GL_FUNC(glGetShaderSource)
  GL_FUNC(glGetString)
  GL_FUNC(glGetTexParameterfv)
  GL_FUNC(glGetTexParameteriv)
  GL_FUNC(glGetUniformfv)
  GL_FUNC(glGetUniformiv)
  GL_FUNC(glGetUniformLocation)
  GL_FUNC(glGetVertexAttribfv)
  GL_FUNC(glGetVertexAttribiv)
  GL_FUNC(glGetVertexAttribPointerv)
  GL_FUNC(glHint)
  GL_FUNC(glIsBuffer)
  GL_FUNC(glIsEnabled)
  GL_FUNC(glIsFramebuffer)
  GL_FUNC(glIsProgram)
  GL_FUNC(glIsRenderbuffer)
  GL_FUNC(glIsShader)
  GL_FUNC(glIsTexture)
  GL_FUNC(glLineWidth)
  GL_FUNC(glLinkProgram)
  GL_FUNC(glPixelStorei)
  GL_FUNC(glPolygonOffset)
  GL_FUNC(glReadPixels)
  GL_FUNC(glReleaseShaderCompiler)
  GL_FUNC(glRenderbufferStorage)
  GL_FUNC(glSampleCoverage)
  GL_FUNC(glScissor)
  GL_FUNC(glShaderBinary)
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
  GL_FUNC(glValidateProgram)
  GL_FUNC(glVertexAttrib1f)
  GL_FUNC(glVertexAttrib1fv)
  GL_FUNC(glVertexAttrib2f)
  GL_FUNC(glVertexAttrib2fv)
  GL_FUNC(glVertexAttrib3f)
  GL_FUNC(glVertexAttrib3fv)
  GL_FUNC(glVertexAttrib4f)
  GL_FUNC(glVertexAttrib4fv)
  GL_FUNC(glVertexAttribPointer)
  GL_FUNC(glViewport)
  GL_FUNC(glGetProgramBinaryOES)
  GL_FUNC(glProgramBinaryOES)
  GL_FUNC(glMapBufferOES)
  GL_FUNC(glUnmapBufferOES)
  GL_FUNC(glGetBufferPointervOES)
  GL_FUNC(glTexImage3DOES)
  GL_FUNC(glTexSubImage3DOES)
  GL_FUNC(glCopyTexSubImage3DOES)
  GL_FUNC(glCompressedTexImage3DOES)
  GL_FUNC(glCompressedTexSubImage3DOES)
  GL_FUNC(glFramebufferTexture3DOES)
  GL_FUNC(glBindVertexArrayOES)
  GL_FUNC(glDeleteVertexArraysOES)
  GL_FUNC(glGenVertexArraysOES)
  GL_FUNC(glIsVertexArrayOES)
  GL_FUNC(glGetPerfMonitorGroupsAMD)
  GL_FUNC(glGetPerfMonitorCountersAMD)
  GL_FUNC(glGetPerfMonitorGroupStringAMD)
  GL_FUNC(glGetPerfMonitorCounterStringAMD)
  GL_FUNC(glGetPerfMonitorCounterInfoAMD)
  GL_FUNC(glGenPerfMonitorsAMD)
  GL_FUNC(glDeletePerfMonitorsAMD)
  GL_FUNC(glSelectPerfMonitorCountersAMD)
  GL_FUNC(glBeginPerfMonitorAMD)
  GL_FUNC(glEndPerfMonitorAMD)
  GL_FUNC(glGetPerfMonitorCounterDataAMD)
  GL_FUNC(glCopyTextureLevelsAPPLE)
  GL_FUNC(glRenderbufferStorageMultisampleAPPLE)
  GL_FUNC(glResolveMultisampleFramebufferAPPLE)
  GL_FUNC(glFenceSyncAPPLE)
  GL_FUNC(glIsSyncAPPLE)
  GL_FUNC(glDeleteSyncAPPLE)
  GL_FUNC(glClientWaitSyncAPPLE)
  GL_FUNC(glWaitSyncAPPLE)
  GL_FUNC(glGetInteger64vAPPLE)
  GL_FUNC(glGetSyncivAPPLE)
  GL_FUNC(glDiscardFramebufferEXT)
  GL_FUNC(glMapBufferRangeEXT)
  GL_FUNC(glFlushMappedBufferRangeEXT)
  GL_FUNC(glMultiDrawArraysEXT)
  GL_FUNC(glMultiDrawElementsEXT)
  GL_FUNC(glRenderbufferStorageMultisampleEXT)
  GL_FUNC(glFramebufferTexture2DMultisampleEXT)
  GL_FUNC(glGetGraphicsResetStatusEXT)
  GL_FUNC(glReadnPixelsEXT)
  GL_FUNC(glGetnUniformfvEXT)
  GL_FUNC(glGetnUniformivEXT)
  GL_FUNC(glTexStorage1DEXT)
  GL_FUNC(glTexStorage2DEXT)
  GL_FUNC(glTexStorage3DEXT)
  GL_FUNC(glTextureStorage1DEXT)
  GL_FUNC(glTextureStorage2DEXT)
  GL_FUNC(glTextureStorage3DEXT)
  GL_FUNC(glRenderbufferStorageMultisampleIMG)
  GL_FUNC(glFramebufferTexture2DMultisampleIMG)
  GL_FUNC(glDeleteFencesNV)
  GL_FUNC(glGenFencesNV)
  GL_FUNC(glIsFenceNV)
  GL_FUNC(glTestFenceNV)
  GL_FUNC(glGetFenceivNV)
  GL_FUNC(glFinishFenceNV)
  GL_FUNC(glSetFenceNV)
  GL_FUNC(glGetDriverControlsQCOM)
  GL_FUNC(glGetDriverControlStringQCOM)
  GL_FUNC(glEnableDriverControlQCOM)
  GL_FUNC(glDisableDriverControlQCOM)
  GL_FUNC(glExtGetTexturesQCOM)
  GL_FUNC(glExtGetBuffersQCOM)
  GL_FUNC(glExtGetRenderbuffersQCOM)
  GL_FUNC(glExtGetFramebuffersQCOM)
  GL_FUNC(glExtGetTexLevelParameterivQCOM)
  GL_FUNC(glExtTexObjectStateOverrideiQCOM)
  GL_FUNC(glExtGetTexSubImageQCOM)
  GL_FUNC(glExtGetBufferPointervQCOM)
  GL_FUNC(glExtGetShadersQCOM)
  GL_FUNC(glExtGetProgramsQCOM)
  GL_FUNC(glExtIsProgramBinaryQCOM)
  GL_FUNC(glExtGetProgramBinarySourceQCOM)
  GL_FUNC(glStartTilingQCOM)
  GL_FUNC(glEndTilingQCOM)
  GL_FUNC(glEvasGLImageTargetTexture2DOES)
  GL_FUNC(glEvasGLImageTargetRenderbufferStorageOES)
  GL_FUNC(glBeginQuery)
  GL_FUNC(glBeginTransformFeedback)
  GL_FUNC(glBindBufferBase)
  GL_FUNC(glBindBufferRange)
  GL_FUNC(glBindSampler)
  GL_FUNC(glBindTransformFeedback)
  GL_FUNC(glBindVertexArray)
  GL_FUNC(glBlitFramebuffer)
  GL_FUNC(glClearBufferfi)
  GL_FUNC(glClearBufferfv)
  GL_FUNC(glClearBufferiv)
  GL_FUNC(glClearBufferuiv)
  GL_FUNC(glClientWaitSync)
  GL_FUNC(glCompressedTexImage3D)
  GL_FUNC(glCompressedTexSubImage3D)
  GL_FUNC(glCopyBufferSubData)
  GL_FUNC(glCopyTexSubImage3D)
  GL_FUNC(glDeleteQueries)
  GL_FUNC(glDeleteSamplers)
  GL_FUNC(glDeleteSync)
  GL_FUNC(glDeleteTransformFeedbacks)
  GL_FUNC(glDeleteVertexArrays)
  GL_FUNC(glDrawArraysInstanced)
  GL_FUNC(glDrawBuffers)
  GL_FUNC(glDrawElementsInstanced)
  GL_FUNC(glDrawRangeElements)
  GL_FUNC(glEndQuery)
  GL_FUNC(glEndTransformFeedback)
  GL_FUNC(glFenceSync)
  GL_FUNC(glFlushMappedBufferRange)
  GL_FUNC(glFramebufferTextureLayer)
  GL_FUNC(glGenQueries)
  GL_FUNC(glGenSamplers)
  GL_FUNC(glGenTransformFeedbacks)
  GL_FUNC(glGenVertexArrays)
  GL_FUNC(glGetActiveUniformBlockiv)
  GL_FUNC(glGetActiveUniformBlockName)
  GL_FUNC(glGetActiveUniformsiv)
  GL_FUNC(glGetBufferParameteri64v)
  GL_FUNC(glGetBufferPointerv)
  GL_FUNC(glGetFragDataLocation)
  GL_FUNC(glGetInteger64i_v)
  GL_FUNC(glGetInteger64v)
  GL_FUNC(glGetIntegeri_v)
  GL_FUNC(glGetInternalformativ)
  GL_FUNC(glGetProgramBinary)
  GL_FUNC(glGetQueryiv)
  GL_FUNC(glGetQueryObjectuiv)
  GL_FUNC(glGetSamplerParameterfv)
  GL_FUNC(glGetSamplerParameteriv)
  GL_FUNC(glGetStringi)
  GL_FUNC(glGetSynciv)
  GL_FUNC(glGetTransformFeedbackVarying)
  GL_FUNC(glGetUniformBlockIndex)
  GL_FUNC(glGetUniformIndices)
  GL_FUNC(glGetUniformuiv)
  GL_FUNC(glGetVertexAttribIiv)
  GL_FUNC(glGetVertexAttribIuiv)
  GL_FUNC(glInvalidateFramebuffer)
  GL_FUNC(glInvalidateSubFramebuffer)
  GL_FUNC(glIsQuery)
  GL_FUNC(glIsSampler)
  GL_FUNC(glIsSync)
  GL_FUNC(glIsTransformFeedback)
  GL_FUNC(glIsVertexArray)
  GL_FUNC(glMapBufferRange)
  GL_FUNC(glPauseTransformFeedback)
  GL_FUNC(glProgramBinary)
  GL_FUNC(glProgramParameteri)
  GL_FUNC(glReadBuffer)
  GL_FUNC(glRenderbufferStorageMultisample)
  GL_FUNC(glResumeTransformFeedback)
  GL_FUNC(glSamplerParameterf)
  GL_FUNC(glSamplerParameterfv)
  GL_FUNC(glSamplerParameteri)
  GL_FUNC(glSamplerParameteriv)
  GL_FUNC(glTexImage3D)
  GL_FUNC(glTexStorage2D)
  GL_FUNC(glTexStorage3D)
  GL_FUNC(glTexSubImage3D)
  GL_FUNC(glTransformFeedbackVaryings)
  GL_FUNC(glUniform1ui)
  GL_FUNC(glUniform1uiv)
  GL_FUNC(glUniform2ui)
  GL_FUNC(glUniform2uiv)
  GL_FUNC(glUniform3ui)
  GL_FUNC(glUniform3uiv)
  GL_FUNC(glUniform4ui)
  GL_FUNC(glUniform4uiv)
  GL_FUNC(glUniformBlockBinding)
  GL_FUNC(glUniformMatrix2x3fv)
  GL_FUNC(glUniformMatrix3x2fv)
  GL_FUNC(glUniformMatrix2x4fv)
  GL_FUNC(glUniformMatrix4x2fv)
  GL_FUNC(glUniformMatrix3x4fv)
  GL_FUNC(glUniformMatrix4x3fv)
  GL_FUNC(glUnmapBuffer)
  GL_FUNC(glVertexAttribDivisor)
  GL_FUNC(glVertexAttribI4i)
  GL_FUNC(glVertexAttribI4iv)
  GL_FUNC(glVertexAttribI4ui)
  GL_FUNC(glVertexAttribI4uiv)
  GL_FUNC(glVertexAttribIPointer)
  GL_FUNC(glWaitSync)
  GL_FUNC(glDispatchCompute)
  GL_FUNC(glDispatchComputeIndirect)
  GL_FUNC(glDrawArraysIndirect)
  GL_FUNC(glDrawElementsIndirect)
  GL_FUNC(glFramebufferParameteri)
  GL_FUNC(glGetFramebufferParameteriv)
  GL_FUNC(glGetProgramInterfaceiv)
  GL_FUNC(glGetProgramResourceIndex)
  GL_FUNC(glGetProgramResourceName)
  GL_FUNC(glGetProgramResourceiv)
  GL_FUNC(glGetProgramResourceLocation)
  GL_FUNC(glUseProgramStages)
  GL_FUNC(glActiveShaderProgram)
  GL_FUNC(glCreateShaderProgramv)
  GL_FUNC(glBindProgramPipeline)
  GL_FUNC(glDeleteProgramPipelines)
  GL_FUNC(glGenProgramPipelines)
  GL_FUNC(glIsProgramPipeline)
  GL_FUNC(glGetProgramPipelineiv)
  GL_FUNC(glProgramUniform1i)
  GL_FUNC(glProgramUniform2i)
  GL_FUNC(glProgramUniform3i)
  GL_FUNC(glProgramUniform4i)
  GL_FUNC(glProgramUniform1ui)
  GL_FUNC(glProgramUniform2ui)
  GL_FUNC(glProgramUniform3ui)
  GL_FUNC(glProgramUniform4ui)
  GL_FUNC(glProgramUniform1f)
  GL_FUNC(glProgramUniform2f)
  GL_FUNC(glProgramUniform3f)
  GL_FUNC(glProgramUniform4f)
  GL_FUNC(glProgramUniform1iv)
  GL_FUNC(glProgramUniform2iv)
  GL_FUNC(glProgramUniform3iv)
  GL_FUNC(glProgramUniform4iv)
  GL_FUNC(glProgramUniform1uiv)
  GL_FUNC(glProgramUniform2uiv)
  GL_FUNC(glProgramUniform3uiv)
  GL_FUNC(glProgramUniform4uiv)
  GL_FUNC(glProgramUniform1fv)
  GL_FUNC(glProgramUniform2fv)
  GL_FUNC(glProgramUniform3fv)
  GL_FUNC(glProgramUniform4fv)
  GL_FUNC(glProgramUniformMatrix2fv)
  GL_FUNC(glProgramUniformMatrix3fv)
  GL_FUNC(glProgramUniformMatrix4fv)
  GL_FUNC(glProgramUniformMatrix2x3fv)
  GL_FUNC(glProgramUniformMatrix3x2fv)
  GL_FUNC(glProgramUniformMatrix2x4fv)
  GL_FUNC(glProgramUniformMatrix4x2fv)
  GL_FUNC(glProgramUniformMatrix3x4fv)
  GL_FUNC(glProgramUniformMatrix4x3fv)
  GL_FUNC(glValidateProgramPipeline)
  GL_FUNC(glGetProgramPipelineInfoLog)
  GL_FUNC(glBindImageTexture)
  GL_FUNC(glGetBooleani_v)
  GL_FUNC(glMemoryBarrier)
  GL_FUNC(glMemoryBarrierByRegion)
  GL_FUNC(glTexStorage2DMultisample)
  GL_FUNC(glGetMultisamplefv)
  GL_FUNC(glSampleMaski)
  GL_FUNC(glGetTexLevelParameteriv)
  GL_FUNC(glGetTexLevelParameterfv)
  GL_FUNC(glBindVertexBuffer)
  GL_FUNC(glVertexAttribFormat)
  GL_FUNC(glVertexAttribIFormat)
  GL_FUNC(glVertexAttribBinding)
  GL_FUNC(glVertexBindingDivisor)
  GL_FUNC(glBlendBarrier)
  GL_FUNC(glCopyImageSubData)
  GL_FUNC(glDebugMessageControl)
  GL_FUNC(glDebugMessageInsert)
  GL_FUNC(glDebugMessageCallback)
  GL_FUNC(glGetDebugMessageLog)
  GL_FUNC(glPushDebugGroup)
  GL_FUNC(glPopDebugGroup)
  GL_FUNC(glObjectLabel)
  GL_FUNC(glGetObjectLabel)
  GL_FUNC(glObjectPtrLabel)
  GL_FUNC(glGetObjectPtrLabel)
  GL_FUNC(glGetPointerv)
  GL_FUNC(glEnablei)
  GL_FUNC(glDisablei)
  GL_FUNC(glBlendEquationi)
  GL_FUNC(glBlendEquationSeparatei)
  GL_FUNC(glBlendFunci)
  GL_FUNC(glBlendFuncSeparatei)
  GL_FUNC(glColorMaski)
  GL_FUNC(glIsEnabledi)
  GL_FUNC(glDrawElementsBaseVertex)
  GL_FUNC(glDrawRangeElementsBaseVertex)
  GL_FUNC(glDrawElementsInstancedBaseVertex)
  GL_FUNC(glFramebufferTexture)
  GL_FUNC(glPrimitiveBoundingBox)
  GL_FUNC(glGetGraphicsResetStatus)
  GL_FUNC(glReadnPixels)
  GL_FUNC(glGetnUniformfv)
  GL_FUNC(glGetnUniformiv)
  GL_FUNC(glGetnUniformuiv)
  GL_FUNC(glMinSampleShading)
  GL_FUNC(glPatchParameteri)
  GL_FUNC(glTexParameterIiv)
  GL_FUNC(glTexParameterIuiv)
  GL_FUNC(glGetTexParameterIiv)
  GL_FUNC(glGetTexParameterIuiv)
  GL_FUNC(glSamplerParameterIiv)
  GL_FUNC(glSamplerParameterIuiv)
  GL_FUNC(glGetSamplerParameterIiv)
  GL_FUNC(glGetSamplerParameterIuiv)
  GL_FUNC(glTexBuffer)
  GL_FUNC(glTexBufferRange)
  GL_FUNC(glTexStorage3DMultisample)
#undef GL_FUNC

  FT_LOGW("Could not resolve: %s", name);
  return nullptr;
}

TizenRenderer::TizenWindowGeometry TizenRendererEvasGL::GetGeometry() {
  TizenWindowGeometry result;
  evas_object_geometry_get(evas_window_, &result.x, &result.y, &result.w,
                           &result.h);
  return result;
}

int32_t TizenRendererEvasGL::GetDpi() {
  auto* ecore_evas =
      ecore_evas_ecore_evas_get(evas_object_evas_get(evas_window_));
  int32_t xdpi, ydpi;
  ecore_evas_screen_dpi_get(ecore_evas, &xdpi, &ydpi);
  return xdpi;
}

uintptr_t TizenRendererEvasGL::GetWindowId() {
  return ecore_evas_window_get(
      ecore_evas_ecore_evas_get(evas_object_evas_get(evas_window_)));
}

Evas_Object* TizenRendererEvasGL::GetImageHandle() {
  return graphics_adapter_;
}

bool TizenRendererEvasGL::InitializeRenderer() {
  if (!SetupEvasGL()) {
    FT_LOGE("SetupEvasGL fail");
    return false;
  }
  Show();
  is_valid_ = true;
  return true;
}

void TizenRendererEvasGL::Show() {
  evas_object_show(GetImageHandle());
  evas_object_show(evas_window_);
}

void TizenRendererEvasGL::DestroyRenderer() {
  DestroyEvasGL();
  DestroyEvasWindow();
}

bool TizenRendererEvasGL::SetupEvasGL() {
  int32_t width, height;
  evas_gl_ = evas_gl_new(evas_object_evas_get(SetupEvasWindow(width, height)));
  if (!evas_gl_) {
    FT_LOGE("SetupEvasWindow fail");
    return false;
  }

  g_evas_gl = evas_gl_;
  gl_config_ = evas_gl_config_new();
  gl_config_->color_format = EVAS_GL_RGBA_8888;
  gl_config_->depth_bits = EVAS_GL_DEPTH_NONE;
  gl_config_->stencil_bits = EVAS_GL_STENCIL_NONE;

  gl_context_ =
      evas_gl_context_version_create(evas_gl_, NULL, EVAS_GL_GLES_3_X);
  gl_resource_context_ =
      evas_gl_context_version_create(evas_gl_, gl_context_, EVAS_GL_GLES_3_X);

  if (gl_context_ == nullptr) {
    FT_LOGW(
        "Failed to create evas gl context with EVAS_GL_GLES_3_X, try to use "
        "EVAS_GL_GLES_2_X,");
    gl_context_ =
        evas_gl_context_version_create(evas_gl_, NULL, EVAS_GL_GLES_2_X);
    gl_resource_context_ =
        evas_gl_context_version_create(evas_gl_, gl_context_, EVAS_GL_GLES_2_X);
  }
  if (gl_context_ == nullptr) {
    FT_LOGE("Failed to create evas gl context!");
    FT_RELEASE_ASSERT_NOT_REACHED();
  }

  EVAS_GL_GLOBAL_GLES3_USE(g_evas_gl, gl_context_);
  gl_surface_ = evas_gl_surface_create(evas_gl_, gl_config_, width, height);

  gl_resource_surface_ =
      evas_gl_pbuffer_surface_create(evas_gl_, gl_config_, width, height, NULL);

  Evas_Native_Surface ns;
  evas_gl_native_surface_get(evas_gl_, gl_surface_, &ns);
  evas_object_image_native_surface_set(GetImageHandle(), &ns);

  return true;
}

Evas_Object* TizenRendererEvasGL::SetupEvasWindow(int32_t& width,
                                                  int32_t& height) {
  elm_config_accel_preference_set("hw:opengl");

  evas_window_ = elm_win_add(NULL, NULL, ELM_WIN_BASIC);
  auto* ecore_evas =
      ecore_evas_ecore_evas_get(evas_object_evas_get(evas_window_));
  int32_t x, y;
  ecore_evas_screen_geometry_get(ecore_evas, &x, &y, &width, &height);
  if (width == 0 || height == 0) {
    FT_LOGE("Invalid screen size: %d x %d", width, height);
    return nullptr;
  }

  elm_win_alpha_set(evas_window_, EINA_FALSE);
  evas_object_move(evas_window_, 0, 0);
  evas_object_resize(evas_window_, width, height);
  evas_object_raise(evas_window_);

  Evas_Object* bg = elm_bg_add(evas_window_);
  evas_object_color_set(bg, 0x00, 0x00, 0x00, 0x00);

  evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  elm_win_resize_object_add(evas_window_, bg);

  graphics_adapter_ =
      evas_object_image_filled_add(evas_object_evas_get(evas_window_));
  evas_object_resize(graphics_adapter_, width, height);
  evas_object_move(graphics_adapter_, 0, 0);
  evas_object_image_size_set(graphics_adapter_, width, height);
  evas_object_image_alpha_set(graphics_adapter_, EINA_TRUE);
  elm_win_resize_object_add(evas_window_, graphics_adapter_);

  const int rotations[4] = {0, 90, 180, 270};
  elm_win_wm_rotation_available_rotations_set(evas_window_, &rotations[0], 4);
  evas_object_smart_callback_add(evas_window_, "rotation,changed",
                                 RotationEventCb, this);
  return evas_window_;
}

void TizenRendererEvasGL::DestroyEvasGL() {
  evas_gl_surface_destroy(evas_gl_, gl_surface_);
  evas_gl_surface_destroy(evas_gl_, gl_resource_surface_);

  evas_gl_context_destroy(evas_gl_, gl_context_);
  evas_gl_context_destroy(evas_gl_, gl_resource_context_);

  evas_gl_config_free(gl_config_);
  evas_gl_free(evas_gl_);
}

void TizenRendererEvasGL::DestroyEvasWindow() {
  evas_object_del(evas_window_);
  evas_object_del(graphics_adapter_);
}

void TizenRendererEvasGL::RotationEventCb(void* data,
                                          Evas_Object* obj,
                                          void* event_info) {
  auto* self = reinterpret_cast<TizenRendererEvasGL*>(data);
  FT_UNIMPLEMENTED();
  self->delegate_.OnOrientationChange(0);
}

void TizenRendererEvasGL::SetRotate(int angle) {
  elm_win_rotation_set(evas_window_, angle);
  received_rotation_ = true;
}

void TizenRendererEvasGL::ResizeWithRotation(int32_t x,
                                             int32_t y,
                                             int32_t width,
                                             int32_t height,
                                             int32_t angle) {
  evas_object_move(evas_window_, x, y);
  evas_object_resize(evas_window_, width, height);
  SetRotate(angle);
}

void TizenRendererEvasGL::SendRotationChangeDone() {
  elm_win_wm_rotation_manual_rotation_done(evas_window_);
}

void TizenRendererEvasGL::SetPreferredOrientations(
    const std::vector<int>& rotations) {
  elm_win_wm_rotation_available_rotations_set(
      evas_window_, static_cast<const int*>(rotations.data()),
      rotations.size());
}
