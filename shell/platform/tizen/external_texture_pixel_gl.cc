#include "flutter/shell/platform/tizen/external_texture_pixel_gl.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl32.h>

bool ExternalTexturePixelGL::PopulateTexture(
    size_t width, size_t height, FlutterOpenGLTexture* opengl_texture) {
  if (!CopyPixelBuffer(width, height)) {
    return false;
  }

  // Populate the texture object used by the engine.
  opengl_texture->target = GL_TEXTURE_2D;
  opengl_texture->name = state_->gl_texture;
  opengl_texture->format = GL_RGBA8;
  opengl_texture->destruction_callback = nullptr;
  opengl_texture->user_data = nullptr;
  opengl_texture->width = width;
  opengl_texture->height = height;
  return true;
}

ExternalTexturePixelGL::ExternalTexturePixelGL(
    FlutterDesktopPixelBufferTextureCallback texture_callback,
    void* user_data)
    : ExternalTexture(),
      state_(std::make_unique<ExternalTextureGLState>()),
      texture_callback_(texture_callback),
      user_data_(user_data) {}

bool ExternalTexturePixelGL::CopyPixelBuffer(size_t& width, size_t& height) {
  const FlutterDesktopPixelBuffer* pixel_buffer =
      texture_callback_(width, height, user_data_);

  /*
   const auto& gl = GlProcs();
   if (!gl.valid || !pixel_buffer || !pixel_buffer->buffer) {
     return false;
   }
   */
   width = pixel_buffer->width;
   height = pixel_buffer->height;
 /*
   if (state_->gl_texture == 0) {
     gl.glGenTextures(1, &state_->gl_texture);

     gl.glBindTexture(GL_TEXTURE_2D, state_->gl_texture);

     gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
     gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

     gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   } else {
     gl.glBindTexture(GL_TEXTURE_2D, state_->gl_texture);
   }
   gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pixel_buffer->width,
                   pixel_buffer->height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                   pixel_buffer->buffer);
 */
  return true;
}