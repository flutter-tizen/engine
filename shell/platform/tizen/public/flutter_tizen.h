// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PUBLIC_FLUTTER_TIZEN_EMBEDDER_H_
#define PUBLIC_FLUTTER_TIZEN_EMBEDDER_H_

#include <stddef.h>
#include <stdint.h>

#include "flutter_export.h"
#include "flutter_messenger.h"
#include "flutter_plugin_registrar.h"

#if defined(__cplusplus)
extern "C" {
#endif

// Opaque reference to a Flutter window controller.
typedef struct FlutterWindowControllerState* FlutterWindowControllerRef;

// Properties for configuring the initial settings of a Flutter window.
typedef struct {
  // The display title.
  const char* title;
  int32_t x;
  int32_t y;
  // Width in screen coordinates.
  int32_t width;
  // Height in screen coordinates.
  int32_t height;
} FlutterWindowProperties;

// Properties for configuring a Flutter engine instance.
typedef struct {
  // The path to the flutter_assets folder for the application to be run.
  const char* assets_path;
  // The path to the icudtl.dat file for the version of Flutter you are using.
  const char* icu_data_path;
  // The path to the libapp.so file for the application to be run.
  const char* aot_library_path;
  // The switches to pass to the Flutter engine.
  //
  // See: https://github.com/flutter/engine/blob/master/shell/common/switches.h
  // for details. Not all arguments will apply.
  const char** switches;
  // The number of elements in |switches|.
  size_t switches_count;
} FlutterEngineProperties;

FLUTTER_EXPORT FlutterWindowControllerRef
FlutterCreateWindow(const FlutterWindowProperties& window_properties,
                    const FlutterEngineProperties& engine_properties);

// Returns the plugin registrar handle for the plugin with the given name.
//
// The name must be unique across the application.
FLUTTER_EXPORT FlutterDesktopPluginRegistrarRef
FlutterDesktopGetPluginRegistrar(FlutterWindowControllerRef controller,
                                 const char* plugin_name);

FLUTTER_EXPORT void FlutterDestoryWindow(FlutterWindowControllerRef controller);

FLUTTER_EXPORT bool FlutterRunsPrecompiledCode();

FLUTTER_EXPORT void FlutterNotifyLocaleChange(
    FlutterWindowControllerRef controller);

FLUTTER_EXPORT void FlutterNotifyLowMemoryWarning(
    FlutterWindowControllerRef controller);

FLUTTER_EXPORT void FlutterNotifyAppIsInactive(
    FlutterWindowControllerRef controller);

FLUTTER_EXPORT void FlutterNotifyAppIsResumed(
    FlutterWindowControllerRef controller);

FLUTTER_EXPORT void FlutterNotifyAppIsPaused(
    FlutterWindowControllerRef controller);

FLUTTER_EXPORT void FlutterNotifyAppIsDetached(
    FlutterWindowControllerRef controller);

FLUTTER_EXPORT void FlutterRotateWindow(FlutterWindowControllerRef controller,
                                        int32_t degree);

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // PUBLIC_FLUTTER_TIZEN_EMBEDDER_H_
