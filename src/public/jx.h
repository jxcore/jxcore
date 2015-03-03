// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_PUBLIC_JX_H
#define SRC_PUBLIC_JX_H

#include "jx_result.h"

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(JS_ENGINE_V8) && !defined(JS_ENGINE_MOZJS) && defined(__IOS__)
// ugly but takes care of XCODE 6 i386 compile bug
size_t fwrite$UNIX2003(const void *a, size_t b, size_t c, FILE *d) {
  return fwrite(a, b, c, d);
}
char *strerror$UNIX2003(int errnum) { return strerror(errnum); }
time_t mktime$UNIX2003(struct tm *a) { return mktime(a); }
double strtod$UNIX2003(const char *a, char **b) { return strtod(a, b); }
void fputs$UNIX2003(const char *restrict c, FILE *restrict f) { fputs(c, f); }
#endif

typedef void (*JX_CALLBACK)(JXResult *result, int argc);

void JX_Initialize(const char *home_folder, JX_CALLBACK callback);

bool JX_Evaluate(const char *data, const char *script_name, JXResult *result);

void JX_DefineMainFile(const char *data);

void JX_DefineFile(const char *name, const char *script);

void JX_StartEngine();

int JX_LoopOnce();

void JX_StopEngine();

#ifdef __cplusplus
}
#endif

#endif  // SRC_PUBLIC_JX_H
