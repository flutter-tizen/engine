// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_vsync_waiter.h"

#include <eina_thread_queue.h>

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/tizen_log.h"

static const int kMessageQuit = -1;
static const int kMessageRequestVblank = 0;

namespace flutter {

typedef struct {
  Eina_Thread_Queue_Msg head;
  int event;
  intptr_t baton;
} Msg;

TizenVsyncWaiter::TizenVsyncWaiter(FlutterTizenEngine* engine)
    : engine_(engine) {
  vblank_thread_ = ecore_thread_feedback_run(RequestVblankLoop, nullptr,
                                             nullptr, nullptr, this, EINA_TRUE);
}

TizenVsyncWaiter::~TizenVsyncWaiter() {
  Send(kMessageQuit, 0);
  if (vblank_thread_) {
    ecore_thread_cancel(vblank_thread_);
    vblank_thread_ = nullptr;
  }
}

void TizenVsyncWaiter::AsyncWaitForVsync(intptr_t baton) {
  Send(kMessageRequestVblank, baton);
}

void TizenVsyncWaiter::Send(int event, intptr_t baton) {
  if (!vblank_thread_ || ecore_thread_check(vblank_thread_)) {
    FT_LOGE("vblank thread not valid");
    return;
  }

  if (!vblank_thread_queue_) {
    FT_LOGE("vblank thread queue not valid");
    return;
  }

  Msg* msg;
  void* ref;
  msg = static_cast<Msg*>(
      eina_thread_queue_send(vblank_thread_queue_, sizeof(Msg), &ref));
  msg->event = event;
  msg->baton = baton;
  eina_thread_queue_send_done(vblank_thread_queue_, ref);
}

void TizenVsyncWaiter::RequestVblankLoop(void* data, Ecore_Thread* thread) {
  TizenVsyncWaiter* tizen_vsync_waiter =
      reinterpret_cast<TizenVsyncWaiter*>(data);
  TdmClient tdm_client(tizen_vsync_waiter->engine_);
  if (!tdm_client.IsValid()) {
    FT_LOGE("Tdm client not valid");
    ecore_thread_cancel(thread);
    return;
  }
  Eina_Thread_Queue* vblank_thread_queue = eina_thread_queue_new();
  if (!vblank_thread_queue) {
    FT_LOGE("Vblank thread queue is not valid");
    ecore_thread_cancel(thread);
    return;
  }

  tizen_vsync_waiter->vblank_thread_queue_ = vblank_thread_queue;
  while (!ecore_thread_check(thread)) {
    void* ref;
    Msg* msg;
    msg = static_cast<Msg*>(eina_thread_queue_wait(vblank_thread_queue, &ref));
    if (msg) {
      eina_thread_queue_wait_done(vblank_thread_queue, ref);
    } else {
      FT_LOGE("Message is null");
      continue;
    }
    if (msg->event == kMessageQuit) {
      break;
    }
    tdm_client.WaitVblank(msg->baton);
  }
  if (vblank_thread_queue) {
    eina_thread_queue_free(vblank_thread_queue);
  }
}

TdmClient::TdmClient(FlutterTizenEngine* engine) {
  if (!CreateTdm()) {
    FT_LOGE("Create tdm client failed");
  }
  engine_ = engine;
}

TdmClient::~TdmClient() {
  DestroyTdm();
}

void TdmClient::WaitVblank(intptr_t baton) {
  baton_ = baton;
  tdm_error error = tdm_client_vblank_wait(vblank_, 1, VblankCallback, this);
  if (error != TDM_ERROR_NONE) {
    FT_LOGE("tdm client wait vblank error  %d", error);
    return;
  }
  tdm_client_handle_events(client_);
}

bool TdmClient::CreateTdm() {
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

void TdmClient::DestroyTdm() {
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

bool TdmClient::IsValid() {
  return vblank_ && client_;
}

void TdmClient::VblankCallback(tdm_client_vblank* vblank,
                               tdm_error error,
                               unsigned int sequence,
                               unsigned int tv_sec,
                               unsigned int tv_usec,
                               void* user_data) {
  TdmClient* client = reinterpret_cast<TdmClient*>(user_data);
  FT_ASSERT(client != nullptr);
  FT_ASSERT(client->engine_ != nullptr);

  uint64_t frame_start_time_nanos = tv_sec * 1e9 + tv_usec * 1e3;
  uint64_t frame_target_time_nanos = 16.6 * 1e6 + frame_start_time_nanos;
  client->engine_->OnVsync(client->baton_, frame_start_time_nanos,
                           frame_target_time_nanos);
}

}  // namespace flutter
