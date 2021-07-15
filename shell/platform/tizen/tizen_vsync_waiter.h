// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_VSYNC_WAITER_H_
#define EMBEDDER_TIZEN_VSYNC_WAITER_H_

#include <Ecore.h>
#include <tdm_client.h>

#include "flutter/shell/platform/embedder/embedder.h"

namespace flutter {

class FlutterTizenEngine;

class TdmClient {
 public:
  TdmClient(FlutterTizenEngine* engine);
  virtual ~TdmClient();
  bool CreateTdm();
  void DestroyTdm();
  bool IsValid();
  void WaitVblank(intptr_t baton);
  static void VblankCallback(tdm_client_vblank* vblank,
                             tdm_error error,
                             unsigned int sequence,
                             unsigned int tv_sec,
                             unsigned int tv_usec,
                             void* user_data);

 private:
  tdm_client* client_{nullptr};
  tdm_client_output* output_{nullptr};
  tdm_client_vblank* vblank_{nullptr};
  FlutterTizenEngine* engine_{nullptr};
  intptr_t baton_{0};
};

class TizenVsyncWaiter {
 public:
  TizenVsyncWaiter(FlutterTizenEngine* engine);
  virtual ~TizenVsyncWaiter();
  void AsyncWaitForVsync(intptr_t baton);

 private:
  void Send(int event, intptr_t baton);
  static void RequestVblankLoop(void* data, Ecore_Thread* thread);
  Ecore_Thread* vblank_thread_{nullptr};
  Eina_Thread_Queue* vblank_thread_queue_{nullptr};
  FlutterTizenEngine* engine_{nullptr};
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_VSYNC_WAITER_H_
