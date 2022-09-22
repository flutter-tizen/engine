// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_VSYNC_WAITER_H_
#define EMBEDDER_TIZEN_VSYNC_WAITER_H_

#include <tdm_client.h>

#include <memory>
#include <mutex>

namespace flutter {

class FlutterTizenEngine;

class TizenVsyncWaiter {
 public:
  TizenVsyncWaiter(FlutterTizenEngine* engine);
  virtual ~TizenVsyncWaiter();

  void AsyncWaitForVsync(intptr_t baton);

 private:
  static void VblankCallback(tdm_client_vblank* vblank,
                             tdm_error error,
                             unsigned int sequence,
                             unsigned int tv_sec,
                             unsigned int tv_usec,
                             void* user_data);

  FlutterTizenEngine* engine_ = nullptr;
  std::mutex engine_mutex_;

  tdm_client* client_ = nullptr;
  tdm_client_output* output_ = nullptr;
  tdm_client_vblank* vblank_ = nullptr;

  intptr_t baton_ = 0;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_VSYNC_WAITER_H_
