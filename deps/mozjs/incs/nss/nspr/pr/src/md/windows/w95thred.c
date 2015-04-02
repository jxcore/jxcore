/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "primpl.h"
#include <process.h>  /* for _beginthreadex() */

#if defined(_MSC_VER) && _MSC_VER <= 1200
/*
 * VC++ 6.0 doesn't have DWORD_PTR.
 */

typedef DWORD DWORD_PTR;
#endif /* _MSC_VER <= 1200 */

/* --- globals ------------------------------------------------ */
#ifdef _PR_USE_STATIC_TLS
__declspec(thread) struct PRThread  *_pr_thread_last_run;
__declspec(thread) struct PRThread  *_pr_currentThread;
__declspec(thread) struct _PRCPU    *_pr_currentCPU;
#else
DWORD _pr_currentThreadIndex;
DWORD _pr_lastThreadIndex;
DWORD _pr_currentCPUIndex;
#endif
int                           _pr_intsOff = 0; 
_PRInterruptTable             _pr_interruptTable[] = { { 0 } };

void
_PR_MD_EARLY_INIT()
{
#ifndef _PR_USE_STATIC_TLS
    _pr_currentThreadIndex = TlsAlloc();
    _pr_lastThreadIndex = TlsAlloc();
    _pr_currentCPUIndex = TlsAlloc();
#endif
}

void _PR_MD_CLEANUP_BEFORE_EXIT(void)
{
    _PR_NT_FreeSids();

    _PR_MD_CleanupSockets();

    WSACleanup();

#ifndef _PR_USE_STATIC_TLS
    TlsFree(_pr_currentThreadIndex);
    TlsFree(_pr_lastThreadIndex);
    TlsFree(_pr_currentCPUIndex);
#endif
}

PRStatus
_PR_MD_INIT_THREAD(PRThread *thread)
{
    if (thread->flags & (_PR_PRIMORDIAL | _PR_ATTACHED)) {
        /*
        ** Warning:
        ** --------
        ** NSPR requires a real handle to every thread.
        ** GetCurrentThread() returns a pseudo-handle which
        ** is not suitable for some thread operations (e.g.,
        ** suspending).  Therefore, get a real handle from
        ** the pseudo handle via DuplicateHandle(...)
        */
        DuplicateHandle(
                GetCurrentProcess(),     /* Process of source handle */
                GetCurrentThread(),      /* Pseudo Handle to dup */
                GetCurrentProcess(),     /* Process of handle */
                &(thread->md.handle),    /* resulting handle */
                0L,                      /* access flags */
                FALSE,                   /* Inheritable */
                DUPLICATE_SAME_ACCESS);  /* Options */
    }

    /* Create the blocking IO semaphore */
    thread->md.blocked_sema = CreateSemaphore(NULL, 0, 1, NULL);
    if (thread->md.blocked_sema == NULL)
        return PR_FAILURE;
	else
		return PR_SUCCESS;
}

static unsigned __stdcall
pr_root(void *arg)
{
    PRThread *thread = (PRThread *)arg;
    thread->md.start(thread);
    return 0;
}

PRStatus 
_PR_MD_CREATE_THREAD(PRThread *thread, 
                  void (*start)(void *), 
                  PRThreadPriority priority, 
                  PRThreadScope scope, 
                  PRThreadState state, 
                  PRUint32 stackSize)
{

    thread->md.start = start;
    thread->md.handle = (HANDLE) _beginthreadex(
                    NULL,
                    thread->stack->stackSize,
                    pr_root,
                    (void *)thread,
                    CREATE_SUSPENDED | STACK_SIZE_PARAM_IS_A_RESERVATION,
                    &(thread->id));
    if(!thread->md.handle) {
        return PR_FAILURE;
    }

    thread->md.id = thread->id;
    /*
     * On windows, a thread is created with a thread priority of
     * THREAD_PRIORITY_NORMAL.
     */
    if (priority != PR_PRIORITY_NORMAL) {
        _PR_MD_SET_PRIORITY(&(thread->md), priority);
    }

    /* Activate the thread */
    if ( ResumeThread( thread->md.handle ) != -1)
        return PR_SUCCESS;

    return PR_FAILURE;
}

void    
_PR_MD_YIELD(void)
{
    /* Can NT really yield at all? */
    Sleep(0);
}

void     
_PR_MD_SET_PRIORITY(_MDThread *thread, PRThreadPriority newPri)
{
    int nativePri;
    BOOL rv;

    if (newPri < PR_PRIORITY_FIRST) {
        newPri = PR_PRIORITY_FIRST;
    } else if (newPri > PR_PRIORITY_LAST) {
        newPri = PR_PRIORITY_LAST;
    }
    switch (newPri) {
        case PR_PRIORITY_LOW:
            nativePri = THREAD_PRIORITY_BELOW_NORMAL;
            break;
        case PR_PRIORITY_NORMAL:
            nativePri = THREAD_PRIORITY_NORMAL;
            break;
        case PR_PRIORITY_HIGH:
            nativePri = THREAD_PRIORITY_ABOVE_NORMAL;
            break;
        case PR_PRIORITY_URGENT:
            nativePri = THREAD_PRIORITY_HIGHEST;
    }
    rv = SetThreadPriority(thread->handle, nativePri);
    PR_ASSERT(rv);
    if (!rv) {
	PR_LOG(_pr_thread_lm, PR_LOG_MIN,
                ("PR_SetThreadPriority: can't set thread priority\n"));
    }
    return;
}

const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
   DWORD dwType; // Must be 0x1000.
   LPCSTR szName; // Pointer to name (in user addr space).
   DWORD dwThreadID; // Thread ID (-1=caller thread).
   DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void
_PR_MD_SET_CURRENT_THREAD_NAME(const char *name)
{
#ifdef _MSC_VER
   THREADNAME_INFO info;

   if (!IsDebuggerPresent())
      return;

   info.dwType = 0x1000;
   info.szName = (char*) name;
   info.dwThreadID = -1;
   info.dwFlags = 0;

   __try {
      RaiseException(MS_VC_EXCEPTION,
                     0,
                     sizeof(info) / sizeof(ULONG_PTR),
                     (ULONG_PTR*)&info);
   } __except(EXCEPTION_CONTINUE_EXECUTION) {
   }
#endif
}

void
_PR_MD_CLEAN_THREAD(PRThread *thread)
{
    BOOL rv;

    if (thread->md.blocked_sema) {
        rv = CloseHandle(thread->md.blocked_sema);
        PR_ASSERT(rv);
        thread->md.blocked_sema = 0;
    }

    if (thread->md.handle) {
        rv = CloseHandle(thread->md.handle);
        PR_ASSERT(rv);
        thread->md.handle = 0;
    }
}

void
_PR_MD_EXIT_THREAD(PRThread *thread)
{
    _PR_MD_CLEAN_THREAD(thread);
    _PR_MD_SET_CURRENT_THREAD(NULL);
}


void
_PR_MD_EXIT(PRIntn status)
{
    _exit(status);
}

PRInt32 _PR_MD_SETTHREADAFFINITYMASK(PRThread *thread, PRUint32 mask )
{
#ifdef WINCE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return -1;
#else
    DWORD_PTR rv;

    rv = SetThreadAffinityMask(thread->md.handle, mask);

    return rv?0:-1;
#endif
}

PRInt32 _PR_MD_GETTHREADAFFINITYMASK(PRThread *thread, PRUint32 *mask)
{
#ifdef WINCE
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return -1;
#else
    BOOL rv;
    DWORD_PTR process_mask;
    DWORD_PTR system_mask;

    rv = GetProcessAffinityMask(GetCurrentProcess(),
            &process_mask, &system_mask);
    if (rv)
        *mask = (PRUint32)process_mask;

    return rv?0:-1;
#endif
}

void 
_PR_MD_SUSPEND_CPU(_PRCPU *cpu) 
{
    _PR_MD_SUSPEND_THREAD(cpu->thread);
}

void
_PR_MD_RESUME_CPU(_PRCPU *cpu)
{
    _PR_MD_RESUME_THREAD(cpu->thread);
}

void
_PR_MD_SUSPEND_THREAD(PRThread *thread)
{
    if (_PR_IS_NATIVE_THREAD(thread)) {
        DWORD previousSuspendCount;
        /* XXXMB - SuspendThread() is not a blocking call; how do we
         * know when the thread is *REALLY* suspended?
         */
        previousSuspendCount = SuspendThread(thread->md.handle);
        PR_ASSERT(previousSuspendCount == 0);
    }
}

void
_PR_MD_RESUME_THREAD(PRThread *thread)
{
    if (_PR_IS_NATIVE_THREAD(thread)) {
        DWORD previousSuspendCount;
        previousSuspendCount = ResumeThread(thread->md.handle);
        PR_ASSERT(previousSuspendCount == 1);
    }
}

PRThread*
_MD_CURRENT_THREAD(void)
{
PRThread *thread;

	thread = _MD_GET_ATTACHED_THREAD();

   	if (NULL == thread) {
		thread = _PRI_AttachThread(
            PR_SYSTEM_THREAD, PR_PRIORITY_NORMAL, NULL, 0);
	}
	PR_ASSERT(thread != NULL);
	return thread;
}

#ifdef NSPR_STATIC

// The following code is from Chromium src/base/thread_local_storage_win.cc,
// r11329.

// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Thread Termination Callbacks.
// Windows doesn't support a per-thread destructor with its
// TLS primitives.  So, we build it manually by inserting a
// function to be called on each thread's exit.
// This magic is from http://www.codeproject.com/threads/tls.asp
// and it works for VC++ 7.0 and later.

// Force a reference to _tls_used to make the linker create the TLS directory
// if it's not already there.  (e.g. if __declspec(thread) is not used).
// Force a reference to p_thread_callback_nspr to prevent whole program
// optimization from discarding the variable.
#ifdef _WIN64

#pragma comment(linker, "/INCLUDE:_tls_used")
#pragma comment(linker, "/INCLUDE:p_thread_callback_nspr")

#else  // _WIN64

#pragma comment(linker, "/INCLUDE:__tls_used")
#pragma comment(linker, "/INCLUDE:_p_thread_callback_nspr")

#endif  // _WIN64

// Static callback function to call with each thread termination.
static void NTAPI PR_OnThreadExit(PVOID module, DWORD reason, PVOID reserved)
{
PRThread *me;

    switch (reason) {
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            if (_pr_initialized) {
                me = _MD_GET_ATTACHED_THREAD();
                if ((me != NULL) && (me->flags & _PR_ATTACHED))
                    _PRI_DetachThread();
            }
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
}

// .CRT$XLA to .CRT$XLZ is an array of PIMAGE_TLS_CALLBACK pointers that are
// called automatically by the OS loader code (not the CRT) when the module is
// loaded and on thread creation. They are NOT called if the module has been
// loaded by a LoadLibrary() call. It must have implicitly been loaded at
// process startup.
// By implicitly loaded, I mean that it is directly referenced by the main EXE
// or by one of its dependent DLLs. Delay-loaded DLL doesn't count as being
// implicitly loaded.
//
// See VC\crt\src\tlssup.c for reference.

// The linker must not discard p_thread_callback_nspr.  (We force a reference
// to this variable with a linker /INCLUDE:symbol pragma to ensure that.) If
// this variable is discarded, the PR_OnThreadExit function will never be
// called.
#ifdef _WIN64

// .CRT section is merged with .rdata on x64 so it must be constant data.
#pragma const_seg(".CRT$XLB")
// When defining a const variable, it must have external linkage to be sure the
// linker doesn't discard it.
extern const PIMAGE_TLS_CALLBACK p_thread_callback_nspr;
const PIMAGE_TLS_CALLBACK p_thread_callback_nspr = PR_OnThreadExit;

// Reset the default section.
#pragma const_seg()

#else  // _WIN64

#pragma data_seg(".CRT$XLB")
PIMAGE_TLS_CALLBACK p_thread_callback_nspr = PR_OnThreadExit;

// Reset the default section.
#pragma data_seg()

#endif  // _WIN64

#endif  // NSPR_STATIC
