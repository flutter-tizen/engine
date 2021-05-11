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

// Opaque reference to a Flutter engine instance.
struct FlutterDesktopEngine;
typedef struct FlutterDesktopEngine* FlutterDesktopEngineRef;

// Properties for configuring a Flutter engine instance.
typedef struct {
  // The path to the flutter_assets folder for the application to be run.
  const char* assets_path;
  // The path to the icudtl.dat file for the version of Flutter you are using.
  const char* icu_data_path;
  // The path to the AOT libary file for your application, if any.
  const char* aot_library_path;
  // The switches to pass to the Flutter engine.
  //
  // See: https://github.com/flutter/engine/blob/master/shell/common/switches.h
  // for details. Not all arguments will apply.
  const char** switches;
  // The number of elements in |switches|.
  size_t switches_count;
} FlutterDesktopEngineProperties;

// Runs an instance of a Flutter engine with the given properties.
//
// If |headed| is false, the engine is run in headless mode.
FLUTTER_EXPORT FlutterDesktopEngineRef FlutterDesktopRunEngine(
    const FlutterDesktopEngineProperties& engine_properties, bool headed);

// Shuts down the given engine instance.
//
// |engine| is no longer valid after this call.
FLUTTER_EXPORT void FlutterDesktopShutdownEngine(
    FlutterDesktopEngineRef engine);

// Returns the plugin registrar handle for the plugin with the given name.
//
// The name must be unique across the application.
FLUTTER_EXPORT FlutterDesktopPluginRegistrarRef
FlutterDesktopGetPluginRegistrar(FlutterDesktopEngineRef engine,
                                 const char* plugin_name);

// Returns the messenger associated with the engine.
FLUTTER_EXPORT FlutterDesktopMessengerRef
FlutterDesktopEngineGetMessenger(FlutterDesktopEngineRef engine);

FLUTTER_EXPORT void FlutterDesktopNotifyLocaleChange(
    FlutterDesktopEngineRef engine);

FLUTTER_EXPORT void FlutterDesktopNotifyLowMemoryWarning(
    FlutterDesktopEngineRef engine);

FLUTTER_EXPORT void FlutterDesktopNotifyAppIsInactive(
    FlutterDesktopEngineRef engine);

FLUTTER_EXPORT void FlutterDesktopNotifyAppIsResumed(
    FlutterDesktopEngineRef engine);

FLUTTER_EXPORT void FlutterDesktopNotifyAppIsPaused(
    FlutterDesktopEngineRef engine);

FLUTTER_EXPORT void FlutterDesktopNotifyAppIsDetached(
    FlutterDesktopEngineRef engine);

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // PUBLIC_FLUTTER_TIZEN_EMBEDDER_H_
