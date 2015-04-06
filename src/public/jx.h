// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_PUBLIC_JX_H
#define SRC_PUBLIC_JX_H

#include "jx_result.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*JX_CALLBACK)(JXValue *result, int argc);

// callback target is per application not per instance
// it doesn't matter how many instances you create
// you may have only one callback target
// from the instance JavaScript, you can read process.threadId
// and send it as a parameter if you need to know which instance is
// sending the call. You should call below method only once per application.
JXCORE_EXTERN(void) 
JX_Initialize(const char *home_folder, JX_CALLBACK callback);

JXCORE_EXTERN(void)  
JX_InitializeNewEngine();

JXCORE_EXTERN(bool) 
JX_Evaluate(const char *data, const char *script_name, JXValue *result);

JXCORE_EXTERN(void) 
JX_DefineMainFile(const char *data);

JXCORE_EXTERN(void) 
JX_DefineFile(const char *name, const char *script);

JXCORE_EXTERN(void) 
JX_StartEngine();

JXCORE_EXTERN(void) 
JX_DefineExtension(const char *name, JX_CALLBACK callback);

JXCORE_EXTERN(int) 
JX_LoopOnce();

JXCORE_EXTERN(int)
JX_Loop();

JXCORE_EXTERN(bool) 
JX_IsSpiderMonkey();

JXCORE_EXTERN(bool) 
JX_IsV8();

// returns threadId for the actual instance
// -1 : there is no active instance for the current thread
// 0 to 63 threadIds. (JS side: process.threadId + 1)
JXCORE_EXTERN(int) 
JX_GetThreadId();

JXCORE_EXTERN(void) 
JX_StopEngine();

#ifdef __cplusplus
}
#endif

#endif  // SRC_PUBLIC_JX_H
