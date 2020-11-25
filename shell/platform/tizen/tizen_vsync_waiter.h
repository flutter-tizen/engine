// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_VSYNC_WAITER_H_
#define EMBEDDER_TIZEN_VSYNC_WAITER_H_

#include <Ecore.h>
#include <tdm_client.h>

#include <thread>

#include "flutter/shell/platform/embedder/embedder.h"

class TizenVsyncWaiter {
 public:
  TizenVsyncWaiter();
  virtual ~TizenVsyncWaiter();
  bool CreateTDMVblank();
  bool AsyncWaitForVsync();
  void AsyncWaitForVsync(intptr_t baton);
  void AsyncWaitForRunEngineSuccess(FLUTTER_API_SYMBOL(FlutterEngine)
                                        flutter_engine);

 private:
  static const int VBLANK_LOOP_REQUEST = 1;
  static const int VBLANK_LOOP_DEL_PIPE = 2;
  void AsyncWaitForVsyncCallback();
  void DeleteVblankEventPipe();
  static void CreateVblankEventLoop(void* data);
  static void TdmClientVblankCallback(tdm_client_vblank* vblank,
                                      tdm_error error, unsigned int sequence,
                                      unsigned int tv_sec, unsigned int tv_usec,
                                      void* user_data);
  static void VblankEventLoopCallback(void* data, void* buffer,
                                      unsigned int nbyte);
  tdm_client* client_;
  tdm_client_output* output_;
  tdm_client_vblank* vblank_;
  FLUTTER_API_SYMBOL(FlutterEngine) flutter_engine_;
  intptr_t baton_;
  Ecore_Pipe* vblank_ecore_pipe_;
};

#endif  // EMBEDDER_TIZEN_VSYNC_WAITER_H_
