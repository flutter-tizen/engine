// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_SURFACE_H_
#define EMBEDDER_TIZEN_SURFACE_H_

#include <string>

class TizenSurface {
 public:
  TizenSurface(int32_t x, int32_t y, int32_t width, int32_t height);
  virtual ~TizenSurface();
  virtual bool OnMakeCurrent() = 0;
  virtual bool OnClearCurrent() = 0;
  virtual bool OnMakeResourceCurrent() = 0;
  virtual bool OnPresent() = 0;
  virtual uint32_t OnGetFBO() = 0;
  virtual void* OnProcResolver(const char* name) = 0;
  virtual bool IsValid() = 0;
  uint32_t GetWidth();
  uint32_t GetHeight();

 protected:
  const int32_t window_width_;
  const int32_t window_height_;
  int32_t x_;
  int32_t y_;
};

#endif  // EMBEDDER_TIZEN_SURFACE_H_
