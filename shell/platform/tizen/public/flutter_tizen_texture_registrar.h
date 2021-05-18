// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_TIZEN_PUBLIC_FLUTTER_TEXTURE_REGISTRAR_H_
#define FLUTTER_SHELL_PLATFORM_TIZEN_PUBLIC_FLUTTER_TEXTURE_REGISTRAR_H_

#include <stddef.h>
#include <stdint.h>

#include "flutter_export.h"

#if defined(__cplusplus)
extern "C" {
#endif

// Opaque reference to a texture registrar.
typedef struct FlutterTextureRegistrar* FlutterTextureRegistrarRef;

// Returns the texture registrar associated with this registrar.
FLUTTER_EXPORT FlutterTextureRegistrarRef
FlutterPluginRegistrarGetTexture(FlutterDesktopPluginRegistrarRef registrar);

// Registers a new texture with the Flutter engine and returns the texture ID,
FLUTTER_EXPORT int64_t
FlutterRegisterExternalTexture(FlutterTextureRegistrarRef texture_registrar);

// Unregisters an existing texture from the Flutter engine for a |texture_id|.
// Returns true on success, false on failure.
FLUTTER_EXPORT bool FlutterUnregisterExternalTexture(
    FlutterTextureRegistrarRef texture_registrar,
    int64_t texture_id);

// Marks that a new texture frame is available for a given |texture_id|.
// Returns true on success, false on failure.
FLUTTER_EXPORT bool FlutterMarkExternalTextureFrameAvailable(
    FlutterTextureRegistrarRef texture_registrar,
    int64_t texture_id,
    void* tbm_surface);

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // FLUTTER_SHELL_PLATFORM_TIZEN_PUBLIC_FLUTTER_TEXTURE_REGISTRAR_H_
