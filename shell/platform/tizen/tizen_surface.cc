// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_surface.h"

TizenSurface::TizenSurface(int32_t x, int32_t y, int32_t width, int32_t height)
    : window_width_(width), window_height_(height), x_(x), y_(y) {}

TizenSurface::~TizenSurface() {}

uint32_t TizenSurface::GetWidth() { return window_width_; }

uint32_t TizenSurface::GetHeight() { return window_height_; }
