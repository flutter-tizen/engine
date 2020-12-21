// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_VSYNC_WAITER_H_
#define EMBEDDER_TIZEN_VSYNC_WAITER_H_

#include <tdm_client.h>

#include <thread>

#include "flutter/shell/platform/embedder/embedder.h"

class TizenEmbedderEngine;

class TizenVsyncWaiter {
 public:
  TizenVsyncWaiter(TizenEmbedderEngine* engine);
  virtual ~TizenVsyncWaiter();
  void AsyncWaitForVsync(intptr_t baton);

  bool IsValid();

 private:
  bool CreateTDMVblank();
  void HandleVblankLoopRequest();
  static void TdmClientVblankCallback(tdm_client_vblank* vblank,
                                      tdm_error error, unsigned int sequence,
                                      unsigned int tv_sec, unsigned int tv_usec,
                                      void* user_data);

  tdm_client* client_{nullptr};
  tdm_client_output* output_{nullptr};
  tdm_client_vblank* vblank_{nullptr};
  TizenEmbedderEngine* engine_{nullptr};
  intptr_t baton_{0};
};

#endif  // EMBEDDER_TIZEN_VSYNC_WAITER_H_
