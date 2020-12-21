// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_vsync_waiter.h"

#include <Ecore.h>

#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/tizen_embedder_engine.h"

static std::atomic<Ecore_Pipe*> g_vblank_ecore_pipe = nullptr;

static const int VBLANK_LOOP_REQUEST = 1;
static const int VBLANK_LOOP_DEL_PIPE = 2;

static void SendVblankLoopRequest(int event_type) {
  if (ecore_pipe_write(g_vblank_ecore_pipe.load(), &event_type,
                       sizeof(event_type)) == EINA_FALSE) {
    LoggerE("Failed to Send Reqeust [%s]", event_type == VBLANK_LOOP_REQUEST
                                               ? "VBLANK_LOOP_REQUEST"
                                               : "VBLANK_LOOP_DEL_PIPE");
  }
}

TizenVsyncWaiter::TizenVsyncWaiter(TizenEmbedderEngine* engine)
    : engine_(engine) {
  if (!CreateTDMVblank()) {
    LoggerE("Failed to create TDM vblank");
    return;
  }

  std::thread t(
      [this](void* data) {
        if (!ecore_init()) {
          LoggerE("Failed to init Ecore");
          return;
        }
        Ecore_Pipe* vblank_ecore_pipe = ecore_pipe_add(
            [](void* data, void* buffer, unsigned int nbyte) {
              TizenVsyncWaiter* tizen_vsync_waiter =
                  reinterpret_cast<TizenVsyncWaiter*>(data);
              int event_type = *(reinterpret_cast<int*>(buffer));
              if (event_type == VBLANK_LOOP_REQUEST) {
                tizen_vsync_waiter->HandleVblankLoopRequest();
              } else if (event_type == VBLANK_LOOP_DEL_PIPE) {
                if (g_vblank_ecore_pipe.load()) {
                  ecore_pipe_del(g_vblank_ecore_pipe);
                  g_vblank_ecore_pipe = NULL;
                }
                ecore_main_loop_quit();
              }
            },
            this);

        g_vblank_ecore_pipe.store(vblank_ecore_pipe);
        ecore_main_loop_begin();
        ecore_shutdown();
      },
      nullptr);
  t.join();

  if (g_vblank_ecore_pipe.load() == nullptr) {
    LoggerE("Failed to create Ecore Pipe");
  }
}

TizenVsyncWaiter::~TizenVsyncWaiter() {
  if (g_vblank_ecore_pipe.load()) {
    SendVblankLoopRequest(VBLANK_LOOP_DEL_PIPE);
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
  SendVblankLoopRequest(VBLANK_LOOP_REQUEST);
}

bool TizenVsyncWaiter::IsValid() {
  return g_vblank_ecore_pipe.load() && client_ && output_ && vblank_ &&
         engine_->flutter_engine;
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

  tdm_client_vblank_set_enable_fake(vblank_, 1);

  return true;
}

void TizenVsyncWaiter::TdmClientVblankCallback(
    tdm_client_vblank* vblank, tdm_error error, unsigned int sequence,
    unsigned int tv_sec, unsigned int tv_usec, void* user_data) {
  TizenVsyncWaiter* tizen_vsync_waiter =
      reinterpret_cast<TizenVsyncWaiter*>(user_data);

  uint64_t frame_start_time_nanos = tv_sec * 1e9 + tv_usec * 1e3;
  uint64_t frame_target_time_nanos = 16.6 * 1e6 + frame_start_time_nanos;
  FlutterEngineOnVsync(tizen_vsync_waiter->engine_->flutter_engine,
                       tizen_vsync_waiter->baton_, frame_start_time_nanos,
                       frame_target_time_nanos);
}

void TizenVsyncWaiter::HandleVblankLoopRequest() {
  tdm_error ret;
  ret = tdm_client_vblank_wait(vblank_, 1, TdmClientVblankCallback, this);
  if (ret != TDM_ERROR_NONE) {
    LoggerE("ERROR, ret = %d", ret);
    return;
  }
  tdm_client_handle_events(client_);
}
