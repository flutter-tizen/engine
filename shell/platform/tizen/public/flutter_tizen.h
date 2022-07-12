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

// Opaque reference to a Flutter view instance.
struct FlutterDesktopView;
typedef struct FlutterDesktopView* FlutterDesktopViewRef;

// Properties for configuring the initial settings of a Flutter window.
typedef struct {
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
  // Whether the window should be on top layer or not.
  bool top_level;
} FlutterDesktopWindowProperties;

// Properties for configuring the initial settings of a Flutter view.
typedef struct {
  // The width of the view, or the maximum width if the value is zero.
  int32_t width;
  // The height of the view, or the maximum height if the value is zero.
  int32_t height;
} FlutterDesktopViewProperties;

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

// ========== Engine ==========

// Creates a Flutter engine with the given properties.
FLUTTER_EXPORT FlutterDesktopEngineRef FlutterDesktopEngineCreate(
    const FlutterDesktopEngineProperties& engine_properties);

// Runs an instance of a Flutter engine with the given properties.
//
// If view is not specified, the engine is run in headless mode.
FLUTTER_EXPORT bool FlutterDesktopEngineRun(
    const FlutterDesktopEngineRef engine);

// Shuts down the given engine instance.
//
// |engine| is no longer valid after this call.
FLUTTER_EXPORT void FlutterDesktopEngineShutdown(
    FlutterDesktopEngineRef engine);

// Returns the plugin registrar handle for the plugin with the given name.
//
// The name must be unique across the application.
FLUTTER_EXPORT FlutterDesktopPluginRegistrarRef
FlutterDesktopEngineGetPluginRegistrar(FlutterDesktopEngineRef engine,
                                       const char* plugin_name);

// Returns the messenger associated with the engine.
FLUTTER_EXPORT FlutterDesktopMessengerRef
FlutterDesktopEngineGetMessenger(FlutterDesktopEngineRef engine);

// Posts an app control to the engine instance.
FLUTTER_EXPORT void FlutterDesktopEngineNotifyAppControl(
    FlutterDesktopEngineRef engine,
    void* app_control);

// Posts a locale change notification to the engine instance.
FLUTTER_EXPORT void FlutterDesktopEngineNotifyLocaleChange(
    FlutterDesktopEngineRef engine);

// Posts a low memory notification to the engine instance.
FLUTTER_EXPORT void FlutterDesktopEngineNotifyLowMemoryWarning(
    FlutterDesktopEngineRef engine);

// Notifies the engine that the app is in an inactive state and not receiving
// user input.
FLUTTER_EXPORT void FlutterDesktopEngineNotifyAppIsInactive(
    FlutterDesktopEngineRef engine);

// Notifies the engine that the app is visible and responding to user input.
FLUTTER_EXPORT void FlutterDesktopEngineNotifyAppIsResumed(
    FlutterDesktopEngineRef engine);

// Notifies the engine that the app is not currently visible to the user, not
// responding to user input, and running in the background.
FLUTTER_EXPORT void FlutterDesktopEngineNotifyAppIsPaused(
    FlutterDesktopEngineRef engine);

// Notifies the engine that the engine is detached from any host views.
FLUTTER_EXPORT void FlutterDesktopEngineNotifyAppIsDetached(
    FlutterDesktopEngineRef engine);

// ========== View ==========

// Creates a view that hosts and displays the given engine instance.
FLUTTER_EXPORT FlutterDesktopViewRef FlutterDesktopViewCreateFromNewWindow(
    const FlutterDesktopWindowProperties& window_properties,
    FlutterDesktopEngineRef engine);

// Creates a view that hosts and displays the given engine instance.
//
// The type of parent should be Evas_Object*, Cast Evas_Object* to void*.
// @warning This API is a work-in-progress and may change.
FLUTTER_EXPORT FlutterDesktopViewRef FlutterDesktopViewCreateFromElmParent(
    const FlutterDesktopViewProperties& view_properties,
    FlutterDesktopEngineRef engine,
    void* parent);

// Destroys the view.
//
// The engine owned by the view will also be shut down implicitly.
// @warning This API is a work-in-progress and may change.
FLUTTER_EXPORT void FlutterDesktopViewDestroy(FlutterDesktopViewRef view);

// Returns a handle to evas object that the FlutterView is drawn to.
//
// Cast the returned void* to Evas_Object*.
// @warning This API is a work-in-progress and may change.
FLUTTER_EXPORT void* FlutterDesktopViewGetEvasObject(
    FlutterDesktopViewRef view);

// Resizes the view.
// @warning This API is a work-in-progress and may change.
FLUTTER_EXPORT void FlutterDesktopViewResize(FlutterDesktopViewRef view,
                                             int32_t width,
                                             int32_t height);

// ========== Plugin Registrar (extensions) ==========

// Returns the window associated with this registrar's engine instance.
//
// If the app runs on a wearable device, cast void* to Evas_Object*,
// otherwise cast it to Ecore_Wl2_Window*.
FLUTTER_EXPORT void* FlutterDesktopPluginRegistrarGetNativeWindow(
    FlutterDesktopPluginRegistrarRef registrar);

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // FLUTTER_SHELL_PLATFORM_TIZEN_PUBLIC_FLUTTER_TIZEN_H_
