// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_PUBLIC_JX_H
#define SRC_PUBLIC_JX_H

#include "jx_result.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*JX_CALLBACK)(JXResult *result, int argc);

void JX_Initialize(const char *home_folder, JX_CALLBACK callback);

bool JX_Evaluate(const char *data, const char *script_name, JXResult *result);

void JX_DefineMainFile(const char *data);

void JX_DefineFile(const char *name, const char *script);

void JX_StartEngine();

void JX_DefineExtension(const char *name, JX_CALLBACK callback);

int JX_LoopOnce();

int JX_Loop();

bool JX_IsSpiderMonkey();

bool JX_IsV8();

void JX_StopEngine();

#ifdef __cplusplus
}
#endif

#endif  // SRC_PUBLIC_JX_H
