// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_vsync_waiter.h"

#include <eina_thread_queue.h>

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/tizen_log.h"

static const int QUEUE_QUIT = -1;
static const int QUEUE_REQUEST_VBLANK = 0;

typedef struct {
  Eina_Thread_Queue_Msg head;
  int value;
} Msg;

static Eina_Thread_Queue* vblank_thread_queue{nullptr};

TizenVsyncWaiter::TizenVsyncWaiter(FlutterTizenEngine* engine)
    : engine_(engine) {
  if (!CreateTDMVblank()) {
    FT_LOGE("Failed to create TDM vblank");
    DestroyTDMVblank();
  } else {
    vblank_thread_queue = eina_thread_queue_new();
    vblank_thread_ =
        ecore_thread_feedback_run(RequestVblankLoop, NULL, VblankLoopFinish,
                                  VblankLoopFinish, this, EINA_TRUE);
  }
}

TizenVsyncWaiter::~TizenVsyncWaiter() {
  SendMessage(QUEUE_QUIT);
  if (vblank_thread_) {
    ecore_thread_cancel(vblank_thread_);
    vblank_thread_ = nullptr;
  }
  DestroyTDMVblank();
}

void TizenVsyncWaiter::AsyncWaitForVsync(intptr_t baton) {
  baton_ = baton;
  if (TDMValid()) {
    SendMessage(QUEUE_REQUEST_VBLANK);
  }
}

void TizenVsyncWaiter::SendMessage(int val) {
  if (!vblank_thread_queue || !vblank_thread_) {
    FT_LOGE("vblank thread or vblank thread queue not valid");
    return;
  }
  Msg* msg;
  void* ref;
  msg = (Msg*)eina_thread_queue_send(vblank_thread_queue, sizeof(Msg), &ref);
  msg->value = val;
  eina_thread_queue_send_done(vblank_thread_queue, ref);
}

void TizenVsyncWaiter::RequestVblankLoop(void* data, Ecore_Thread* thread) {
  TizenVsyncWaiter* tizen_vsync_waiter =
      reinterpret_cast<TizenVsyncWaiter*>(data);
  void* ref;
  Msg* msg;
  while (!ecore_thread_check(thread)) {
    if (!vblank_thread_queue) {
      FT_LOGE("Vblank thread queue is not valid");
      return;
    }
    msg = (Msg*)eina_thread_queue_wait(vblank_thread_queue, &ref);
    if (msg) {
      eina_thread_queue_wait_done(vblank_thread_queue, ref);
    } else {
      FT_LOGE("Message is null");
      continue;
    }
    if (msg->value == QUEUE_QUIT) {
      FT_LOGD("Message queue quit");
      return;
    }
    if (!tizen_vsync_waiter->TDMValid()) {
      FT_LOGE("TDM not valid");
      return;
    }
    tdm_error error = tdm_client_vblank_wait(tizen_vsync_waiter->vblank_, 1,
                                             TdmClientVblankCallback, data);
    if (error != TDM_ERROR_NONE) {
      FT_LOGE("tdm_client_vblank_wait error  %d", error);
      continue;
    }
    tdm_client_handle_events(tizen_vsync_waiter->client_);
  }
}

void TizenVsyncWaiter::VblankLoopFinish(void* data, Ecore_Thread* thread) {
  FT_LOGD("VblankLoopFinish.");
  if (vblank_thread_queue) {
    eina_thread_queue_free(vblank_thread_queue);
    vblank_thread_queue = nullptr;
  }
}

bool TizenVsyncWaiter::CreateTDMVblank() {
  tdm_error ret;
  client_ = tdm_client_create(&ret);
  if (ret != TDM_ERROR_NONE && client_ != NULL) {
    FT_LOGE("create client fail");
    return false;
  }

  output_ = tdm_client_get_output(client_, const_cast<char*>("default"), &ret);
  if (ret != TDM_ERROR_NONE && output_ != NULL) {
    FT_LOGE("get output fail");
    return false;
  }

  vblank_ = tdm_client_output_create_vblank(output_, &ret);
  if (ret != TDM_ERROR_NONE && vblank_ != NULL) {
    FT_LOGE("create vblank fail");
    return false;
  }

  tdm_client_vblank_set_enable_fake(vblank_, 1);
  return true;
}

void TizenVsyncWaiter::DestroyTDMVblank() {
  if (vblank_) {
    tdm_client_vblank_destroy(vblank_);
    vblank_ = nullptr;
  }
  output_ = nullptr;
  if (client_) {
    tdm_client_destroy(client_);
    client_ = nullptr;
  }
}

bool TizenVsyncWaiter::TDMValid() {
  return vblank_ && client_;
}

void TizenVsyncWaiter::TdmClientVblankCallback(tdm_client_vblank* vblank,
                                               tdm_error error,
                                               unsigned int sequence,
                                               unsigned int tv_sec,
                                               unsigned int tv_usec,
                                               void* user_data) {
  TizenVsyncWaiter* tizen_vsync_waiter =
      reinterpret_cast<TizenVsyncWaiter*>(user_data);
  FT_ASSERT(tizen_vsync_waiter != nullptr);
  FT_ASSERT(tizen_vsync_waiter->engine_ != nullptr);

  uint64_t frame_start_time_nanos = tv_sec * 1e9 + tv_usec * 1e3;
  uint64_t frame_target_time_nanos = 16.6 * 1e6 + frame_start_time_nanos;
  tizen_vsync_waiter->engine_->OnVsync(tizen_vsync_waiter->baton_,
                                       frame_start_time_nanos,
                                       frame_target_time_nanos);
}
