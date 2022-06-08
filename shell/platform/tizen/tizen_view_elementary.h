// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_VIEW_ELEMENTARY_H_
#define EMBEDDER_TIZEN_VIEW_ELEMENTARY_H_

#include "flutter/shell/platform/tizen/tizen_view.h"

#define EFL_BETA_API_SUPPORT
#include <Ecore.h>
#include <Elementary.h>

#include <unordered_map>

namespace flutter {

class TizenViewElementary : public TizenView {
 public:
  TizenViewElementary(TizenViewBase::Geometry geometry, Evas_Object* parent);

  ~TizenViewElementary();

  Geometry GetRenderTargetGeometry() override;

  void SetRenderTargetGeometry(Geometry geometry) override;

  Geometry GetScreenGeometry() override;

  void* GetRenderTarget() override { return container_; }

  void* GetRenderTargetDisplay() override { return image_; }

  int32_t GetDpi() override;

  uintptr_t GetWindowId() override;

  void ResizeRenderTargetWithRotation(Geometry geometry,
                                      int32_t angle) override;

  void BindKeys(const std::vector<std::string>& keys) override;

  void Show() override;

  void OnGeometryChanged(Geometry geometry) override;

 private:
  bool CreateView();

  void RegisterEventHandlers();

  void UnregisterEventHandlers();

  void PrepareInputMethod();

  Evas_Object* parent_ = nullptr;
  Evas_Object* container_ = nullptr;
  Evas_Object* image_ = nullptr;
  Evas_Object* event_layer_ = nullptr;

  std::unordered_map<Evas_Callback_Type, Evas_Object_Event_Cb>
      evas_object_callbacks_;
  std::vector<Ecore_Event_Handler*> ecore_event_key_handlers_;

  bool scroll_hold_ = false;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_VIEW_ELEMENTARY_H_
