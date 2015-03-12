// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_PUBLIC_JX_H
#define SRC_PUBLIC_JX_H

#include "jx_result.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*JX_CALLBACK)(JXResult *result, int argc);

// callback target is per application not per instance
// it doesn't matter how many instances you create
// you may have only one callback target
// from the instance JavaScript, you can read process.threadId
// and send it as a parameter if you need to know which instance is
// sending the call. You should call below method only once per application.
void JX_Initialize(const char *home_folder, JX_CALLBACK callback);

void JX_InitializeNewEngine();

bool JX_Evaluate(const char *data, const char *script_name, JXResult *result);

void JX_DefineMainFile(const char *data);

void JX_DefineFile(const char *name, const char *script);

void JX_StartEngine();

void JX_DefineExtension(const char *name, JX_CALLBACK callback);

int JX_LoopOnce();

int JX_Loop();

bool JX_IsSpiderMonkey();

bool JX_IsV8();

// returns threadId for the actual instance
// -1 : there is no active instance for the current thread
// 0 to 63 threadIds. (JS side: process.threadId + 1)
int JX_GetThreadId();

void JX_StopEngine();

#ifdef __cplusplus
}
#endif

#endif  // SRC_PUBLIC_JX_H
