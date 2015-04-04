/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uv.h"
#include "internal.h"
#include "handle-inl.h"
#include "req-inl.h"
#include "winpthreads.h"

/* The only event loop we support right now */
static uv_loop_t uv_default_loop_;

/* uv_once intialization guards */
static uv_once_t uv_init_guard_ = UV_ONCE_INIT;
static uv_once_t uv_default_loop_init_guard_ = UV_ONCE_INIT;

static void uv__crt_invalid_parameter_handler(const wchar_t* expression,
                                              const wchar_t* function,
                                              const wchar_t* file,
                                              unsigned int line,
                                              uintptr_t reserved) {
  /* No-op. */
}

static BOOL first = FALSE;

static void uv_init(void) {
  int i;

  if (first == FALSE) {
    first = TRUE;
    for (i = 0; i < 65; i++) uv_mutex_init(&loopex[i]);
  }

  /* Tell Windows that we will handle critical errors. */
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX |
               SEM_NOOPENFILEERRORBOX);

/* Tell the CRT to not exit the application when an invalid parameter is */
/* passed. The main issue is that invalid FDs will trigger this behavior. */
#if !defined(__MINGW32__) || __MSVCRT_VERSION__ >= 0x800
  _set_invalid_parameter_handler(uv__crt_invalid_parameter_handler);
#endif

  /* Fetch winapi function pointers. This must be done first because other */
  /* intialization code might need these function pointers to be loaded. */
  uv_winapi_init();

  /* Initialize winsock */
  uv_winsock_init();

  /* Initialize FS */
  uv_fs_init();

  /* Initialize signal stuff */
  uv_signals_init();

  /* Initialize console */
  uv_console_init();

  /* Initialize utilities */
  uv__util_init();
}

static void uv_loop_init(uv_loop_t* loop) {
  /* Create an I/O completion port */
  loop->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
  if (loop->iocp == NULL) {
    uv_fatal_error(GetLastError(), "CreateIoCompletionPort");
  }

  /* To prevent uninitialized memory access, loop->time must be intialized */
  /* to zero before calling uv_update_time for the first time. */
  loop->time = 0;
  uv_update_time(loop);

  QUEUE_INIT(&loop->handle_queue);
  QUEUE_INIT(&loop->active_reqs);
  loop->active_handles = 0;

  loop->pending_reqs_tail = NULL;

  loop->endgame_handles = NULL;

  RB_INIT(&loop->timers);

  loop->check_handles = NULL;
  loop->prepare_handles = NULL;
  loop->idle_handles = NULL;

  loop->next_prepare_handle = NULL;
  loop->next_check_handle = NULL;
  loop->next_idle_handle = NULL;

  memset(&loop->poll_peer_sockets, 0, sizeof loop->poll_peer_sockets);

  loop->active_tcp_streams = 0;
  loop->active_udp_streams = 0;

  loop->timer_counter = 0;
  loop->stop_flag = 0;

  loop->last_err = uv_ok_;
}

static int uv_multithreaded_ = 0;
void uv_multithreaded() { uv_multithreaded_ = 1; }

static pthread_key_t tkey;
static int key_created = 0;

void uv_createThreadKey(int recreate) {
  if (key_created == 1 && recreate == 0) return;
  key_created = 1;

  if (recreate == 1 && key_created == 1) {
    pthread_key_delete(&tkey);
  }
  pthread_key_create(&tkey, NULL);
}

void uv_setThreadKeyId(int* id) {
  void* ptr;
  if (*id == 0 || (ptr = pthread_getspecific(tkey)) == NULL) {
    ptr = (void*)id;

    pthread_setspecific(tkey, ptr);
  }
}

int uv_getThreadKeyId() {
  int q;
  void* ptr = pthread_getspecific(tkey);

  if (ptr == NULL) {
    return -2;
  }

  q = (*((int*)ptr)) - 1;

  return q;
}

uv_loop_t* loops[65] = {NULL};

void uv_setThreadLoop(const int id, uv_loop_t* loop) { loops[id] = loop; }

static void uv_default_loop_init(void) {
  /* Initialize libuv itself first */
  uv__once_init();

  /* Initialize the main loop */
  uv_loop_init(&uv_default_loop_);
}

void uv__once_init(void) { uv_once(&uv_init_guard_, uv_init); }

uv_loop_t* uv_default_loop_ex(void) {
  uv_once(&uv_default_loop_init_guard_, uv_default_loop_init);
  return &uv_default_loop_;
}

uv_loop_t* uv_default_loop(void) {
  int tid;

  tid = uv_getThreadKeyId();
  if (tid == -1) {
    return uv_default_loop_ex();
  }

  if (loops[tid] == NULL) {
    loops[tid] = uv_loop_new();
  }

  return loops[tid];
}

uv_loop_t* uv_loop_new(void) {
  uv_loop_t* loop;

  /* Initialize libuv itself first */
  uv__once_init();

  loop = (uv_loop_t*)malloc(sizeof(uv_loop_t));

  if (!loop) {
    uv_fatal_error(ERROR_OUTOFMEMORY, "malloc");
  }

  uv_loop_init(loop);
  return loop;
}

void uv_loop_delete(uv_loop_t* loop) {
  int tid;
  if (loop != &uv_default_loop_) {
    int i;
    for (i = 0; i < ARRAY_SIZE(loop->poll_peer_sockets); i++) {
      SOCKET sock = loop->poll_peer_sockets[i];
      if (sock != 0 && sock != INVALID_SOCKET) {
        closesocket(sock);
      }
    }

    tid = uv_getThreadKeyId();
    loops[tid] = NULL;
    JX_FREE(core, loop);
  }
}

int uv__loop_configure(uv_loop_t* loop, uv_loop_option option, va_list ap) {
  return UV_ENOSYS;
}

int uv_backend_fd(const uv_loop_t* loop) { return -1; }

int uv_backend_timeout(const uv_loop_t* loop) { return 0; }

static void uv_poll(uv_loop_t* loop, int block) {
  DWORD bytes, timeout;
  ULONG_PTR key;
  OVERLAPPED* overlapped;
  uv_req_t* req;

  if (block) {
    timeout = uv_get_poll_timeout(loop);
  } else {
    timeout = 0;
  }

  GetQueuedCompletionStatus(loop->iocp, &bytes, &key, &overlapped, timeout);

  if (overlapped) {
    /* Package was dequeued */
    req = uv_overlapped_to_req(overlapped);
    uv_insert_pending_req(loop, req);
  } else if (GetLastError() != WAIT_TIMEOUT) {
    /* Serious error */
    uv_fatal_error(GetLastError(), "GetQueuedCompletionStatus");
  }
}

static void uv_poll_ex(uv_loop_t* loop, int block) {
  BOOL success;
  DWORD timeout;
  uv_req_t* req;
  OVERLAPPED_ENTRY overlappeds[128];
  ULONG count;
  ULONG i;
  uv_loop_t* sloop;

  if (block) {
    timeout = uv_get_poll_timeout(loop);
  } else {
    timeout = 0;
  }

  success = pGetQueuedCompletionStatusEx(
      loop->iocp, overlappeds, ARRAY_SIZE(overlappeds), &count, timeout, FALSE);

  if (success) {
    for (i = 0; i < count; i++) {
      /* Package was dequeued */
      req = uv_overlapped_to_req(overlappeds[i].lpOverlapped);

      sloop = getCorrectLoop(loop, req);
      uv_insert_pending_req(sloop, req);
      // uv_insert_pending_req(loop, req);
    }
  } else if (GetLastError() != WAIT_TIMEOUT) {
    /* Serious error */
    uv_fatal_error(GetLastError(), "GetQueuedCompletionStatusEx");
  }
}

static int uv__loop_alive(uv_loop_t* loop) {
  /*printf("la -> %d %d alive-> %d \n", loop->active_handles, loop->fakeHandle,
     loop->active_handles > loop->fakeHandle ||
          !QUEUE_EMPTY(&loop->active_reqs) ||
          loop->endgame_handles != NULL); fflush(stdout);*/
  return loop->active_handles > loop->fakeHandle ||
         !QUEUE_EMPTY(&loop->active_reqs) || loop->endgame_handles != NULL;
}

void uv_loop_alive(uv_loop_t* loop, int* a, int* b, int* c) {
  // printf("lap -> %d %d \n", loop->active_handles, loop->fakeHandle);
  // fflush(stdout);
  int n = loop->active_handles - loop->fakeHandle;
  *a = n >= 0 ? n : 0;
  *b = !QUEUE_EMPTY(&loop->active_reqs);
  *c = loop->endgame_handles != NULL;
}

static int threadMessages[65] = {0};  // 0 is main thread

void setThreadMessage(const int tid, const int haveMessage) {  //-1 to set All
  if (tid >= 0) {
    threadMessages[tid] = haveMessage;
  }
}

int threadHasMessage(const int tid) {
  if (tid == -1) return 0;
  return threadMessages[tid];
}

#define THREAD_ID_NOT_DEFINED -1
#define THREAD_ID_ALREADY_DEFINED -2

int uv_run_jx(uv_loop_t* loop, uv_run_mode mode, void (*triggerSync)(const int),
              const int tid) {
  int r;
  void (*poll)(uv_loop_t * loop, int block);

  if (pGetQueuedCompletionStatusEx)
    poll = &uv_poll_ex;
  else
    poll = &uv_poll;

  if (tid != THREAD_ID_ALREADY_DEFINED) {
    if (tid != THREAD_ID_NOT_DEFINED)
      loop->loopId = tid;
    else
      loop->loopId = 63;
  }

  if (!uv__loop_alive(loop)) return 0;

  r = uv__loop_alive(loop);
  while (r != 0 && loop->stop_flag == 0) {
    uv_update_time(loop);
    uv_process_timers(loop);

    /* Call idle callbacks if nothing to do. */
    if (loop->pending_reqs_tail == NULL && loop->endgame_handles == NULL) {
      uv_idle_invoke(loop);
    }

    uv_process_reqs(loop);
    uv_process_endgames(loop);
    uv_prepare_invoke(loop);

    if (tid > 0) {
      if (threadMessages[tid] != 0) {
        triggerSync(tid - 1000);
      }
    }

    if (mode != UV_RUN_PAUSE) {
      const int _tid = tid >= 0 ? tid : 63;
      (*poll)(loop, loop->idle_handles == NULL && threadMessages[_tid] == 0 &&
                        loop->pending_reqs_tail == NULL &&
                        loop->endgame_handles == NULL && !loop->stop_flag &&
                        (loop->active_handles > loop->fakeHandle ||
                         !QUEUE_EMPTY(&loop->active_reqs)) &&
                        !(mode & UV_RUN_NOWAIT));
    }

    uv_check_invoke(loop);
    r = uv__loop_alive(loop);
    if (mode & (UV_RUN_ONCE | UV_RUN_NOWAIT | UV_RUN_PAUSE)) break;
  }

  if (tid >= 0 && mode == UV_RUN_DEFAULT) {
    triggerSync(tid);
  }

  // if we force thread shutdown, there will be some remaining tasks etc.
  // It is highly possible they might leak, below impl. tries to workaround
  // this problem by looping the handles for 500 ms. at max.
  // TODO(obastemur) make it configurable per thread / timer sensitive
  if (loop->stop_flag != 0) {
    loop->stop_flag = 0;
    if (mode != UV_RUN_DEFAULT || r == 0) return r;

    int force_close = uv__loop_alive(loop);
    if (force_close) {
      uint64_t start_time = uv_hrtime();

      int ret_val = 1;
      while (ret_val) {
        ret_val = uv_run_jx(loop, UV_RUN_NOWAIT,
                             triggerSync, THREAD_ID_ALREADY_DEFINED);
        uint64_t end_time = uv_hrtime();
        if(end_time - start_time > 500 * 1024 * 1024) {
          break;
        }
      }
	}
  }

  return r;
}

int uv_run(uv_loop_t* loop, uv_run_mode mode) {
  return uv_run_jx(loop, mode, NULL, -1);
}
