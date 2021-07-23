// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_TIZEN_PUBLIC_FLUTTER_TIZEN_H_
#define FLUTTER_SHELL_PLATFORM_TIZEN_PUBLIC_FLUTTER_TIZEN_H_

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

// Properties for configuring the initial settings of a Flutter window.
typedef struct {
  // Whether the app is headed or headless. Other properties are ignored if
  // this value is set to false.
  bool headed;
  // The x-coordinate of the top left corner of the window.
  int32_t x;
  // The y-coordinate of the top left corner of the window.
  int32_t y;
  // The width of the window, or the maximum width if the value is zero.
  int32_t width;
  // The height of the window, or the maximum height if the value is zero.
  int32_t height;
  // Whether the window should have a transparent background or not.
  bool transparent;
  // Whether the window should be focusable or not.
  bool focusable;
} FlutterDesktopWindowProperties;

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
  // The optional entrypoint in the Dart project. If the value is null or
  // empty, defaults to main().
  const char* entrypoint;
  // Number of elements in the array passed in as dart_entrypoint_argv.
  int dart_entrypoint_argc;
  // Array of Dart entrypoint arguments. This is deep copied during the call
  // to FlutterDesktopRunEngine.
  const char** dart_entrypoint_argv;
} FlutterDesktopEngineProperties;

// Runs an instance of a Flutter engine with the given properties.
//
// If |headed| is false, the engine is run in headless mode.
FLUTTER_EXPORT FlutterDesktopEngineRef FlutterDesktopRunEngine(
    const FlutterDesktopWindowProperties& window_properties,
    const FlutterDesktopEngineProperties& engine_properties);

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

// Posts a locale change notification to the engine instance.
FLUTTER_EXPORT void FlutterDesktopNotifyLocaleChange(
    FlutterDesktopEngineRef engine);

// Posts a low memory notification to the engine instance.
FLUTTER_EXPORT void FlutterDesktopNotifyLowMemoryWarning(
    FlutterDesktopEngineRef engine);

// Notifies the engine that the app is in an inactive state and not receiving
// user input.
FLUTTER_EXPORT void FlutterDesktopNotifyAppIsInactive(
    FlutterDesktopEngineRef engine);

// Notifies the engine that the app is visible and responding to user input.
FLUTTER_EXPORT void FlutterDesktopNotifyAppIsResumed(
    FlutterDesktopEngineRef engine);

// Notifies the engine that the app is not currently visible to the user, not
// responding to user input, and running in the background.
FLUTTER_EXPORT void FlutterDesktopNotifyAppIsPaused(
    FlutterDesktopEngineRef engine);

// Notifies the engine that the engine is detached from any host views.
FLUTTER_EXPORT void FlutterDesktopNotifyAppIsDetached(
    FlutterDesktopEngineRef engine);

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // FLUTTER_SHELL_PLATFORM_TIZEN_PUBLIC_FLUTTER_TIZEN_H_
