// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_vsync_waiter.h"

#include "flutter/shell/platform/tizen/logger.h"

TizenVsyncWaiter::TizenVsyncWaiter()
    : client_(NULL),
      output_(NULL),
      vblank_(NULL),
      flutter_engine_(nullptr),
      baton_(0),
      vblank_ecore_pipe_(NULL) {
  if (CreateTDMVblank()) {
    std::thread t(CreateVblankEventLoop, this);
    t.join();
  } else {
    LoggerE("CreateVsyncVaiter fail");
  }
}

void TizenVsyncWaiter::CreateVblankEventLoop(void* data) {
  TizenVsyncWaiter* tizen_vsync_waiter =
      reinterpret_cast<TizenVsyncWaiter*>(data);
  if (!ecore_init()) {
    LoggerE("ERROR: Cannot init Ecore!");
    return;
  }
  tizen_vsync_waiter->vblank_ecore_pipe_ =
      ecore_pipe_add(VblankEventLoopCallback, tizen_vsync_waiter);
  LoggerD("ecore_init successful");
  ecore_main_loop_begin();
  ecore_shutdown();
}

void TizenVsyncWaiter::VblankEventLoopCallback(void* data, void* buffer,
                                               unsigned int nbyte) {
  TizenVsyncWaiter* tizen_vsync_waiter =
      reinterpret_cast<TizenVsyncWaiter*>(data);
  int* event_type = reinterpret_cast<int*>(buffer);
  if ((*event_type) == VBLANK_LOOP_REQUEST) {
    tizen_vsync_waiter->AsyncWaitForVsyncCallback();
  } else if ((*event_type) == VBLANK_LOOP_DEL_PIPE) {
    tizen_vsync_waiter->DeleteVblankEventPipe();
  }
}

void TizenVsyncWaiter::AsyncWaitForVsyncCallback() {
  tdm_error ret;
  ret = tdm_client_vblank_wait(vblank_, 1, TdmClientVblankCallback, this);
  if (ret != TDM_ERROR_NONE) {
    LoggerE("ERROR, ret = %d", ret);
    return;
  }
  tdm_client_handle_events(client_);
}

void TizenVsyncWaiter::DeleteVblankEventPipe() {
  if (vblank_ecore_pipe_) {
    ecore_pipe_del(vblank_ecore_pipe_);
    vblank_ecore_pipe_ = NULL;
  }
  ecore_main_loop_quit();
}

bool TizenVsyncWaiter::CreateTDMVblank() {
  tdm_error ret;
  client_ = tdm_client_create(&ret);
  if (ret != TDM_ERROR_NONE && client_ != NULL) {
    LoggerE("create client fail");
    return false;
  }

  output_ = tdm_client_get_output(client_, const_cast<char*>("default"), &ret);
  if (ret != TDM_ERROR_NONE && output_ != NULL) {
    LoggerE("get output fail");
    return false;
  }

  vblank_ = tdm_client_output_create_vblank(output_, &ret);
  if (ret != TDM_ERROR_NONE && vblank_ != NULL) {
    LoggerE("create vblank fail");
    return false;
  }

  return true;
}

void TizenVsyncWaiter::TdmClientVblankCallback(
    tdm_client_vblank* vblank, tdm_error error, unsigned int sequence,
    unsigned int tv_sec, unsigned int tv_usec, void* user_data) {
  TizenVsyncWaiter* tizen_vsync_waiter =
      reinterpret_cast<TizenVsyncWaiter*>(user_data);
  if (tizen_vsync_waiter == nullptr) {
    LoggerE("tizen_vsync_waiter is null");
    return;
  }
  if (tizen_vsync_waiter->flutter_engine_ == nullptr) {
    LoggerI("flutter engine creation is not completed");
    return;
  }
  uint64_t frame_start_time_nanos = tv_sec * 1e9 + tv_usec * 1e3;
  uint64_t frame_target_time_nanos = 16.6 * 1e6 + frame_start_time_nanos;
  FlutterEngineOnVsync(tizen_vsync_waiter->flutter_engine_,
                       tizen_vsync_waiter->baton_, frame_start_time_nanos,
                       frame_target_time_nanos);
}

bool TizenVsyncWaiter::AsyncWaitForVsync() {
  if (nullptr == flutter_engine_) {
    LoggerD("flutter_engine_ is null");
    return false;
  }
  if (vblank_ecore_pipe_) {
    int event_type = VBLANK_LOOP_REQUEST;
    ecore_pipe_write(vblank_ecore_pipe_, &event_type, sizeof(event_type));
  }
  return true;
}

TizenVsyncWaiter::~TizenVsyncWaiter() {
  if (vblank_ecore_pipe_) {
    int event_type = VBLANK_LOOP_DEL_PIPE;
    ecore_pipe_write(vblank_ecore_pipe_, &event_type, sizeof(event_type));
  }
  if (vblank_) {
    tdm_client_vblank_destroy(vblank_);
  }
  if (client_) {
    tdm_client_destroy(client_);
  }
}

void TizenVsyncWaiter::AsyncWaitForVsync(intptr_t baton) {
  baton_ = baton;
  AsyncWaitForVsync();
}

void TizenVsyncWaiter::AsyncWaitForRunEngineSuccess(
    FLUTTER_API_SYMBOL(FlutterEngine) flutter_engine) {
  flutter_engine_ = flutter_engine;
  if (baton_ == 0) {
    LoggerD("baton_ == 0");
    return;
  }
  AsyncWaitForVsync();
}
