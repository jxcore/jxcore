// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_H_
#define SRC_JX_PROXY_H_
#include <string>
#include <stdlib.h>

#if defined(__ANDROID__) && defined(JXCORE_EMBEDDED)
#ifndef JXCORE_ALOG_TAG
#define JXCORE_ALOG_TAG "jxcore-log"
#endif
#include <android/log.h>
#define log_console(...) \
  __android_log_print(ANDROID_LOG_INFO, JXCORE_ALOG_TAG, __VA_ARGS__)
#define flush_console(...) \
  __android_log_print(ANDROID_LOG_INFO, JXCORE_ALOG_TAG, __VA_ARGS__)
#define error_console(...) \
  __android_log_print(ANDROID_LOG_ERROR, JXCORE_ALOG_TAG, __VA_ARGS__)
#define warn_console(...) \
  __android_log_print(ANDROID_LOG_WARN, JXCORE_ALOG_TAG, __VA_ARGS__)
#elif defined(WINONECORE)
static inline void DebuggerOutput_(const char* ctstr, ...) {
  char str[8192];
  va_list ap;
  va_start(ap, ctstr);
  int pos = sprintf_s(str, 8192, ctstr, ap);
  va_end(ap);
  str[pos] = '\0';

  OutputDebugStringA(str);
}
#define log_console(...) DebuggerOutput_(__VA_ARGS__)
#define warn_console(...) DebuggerOutput_(__VA_ARGS__)
#define error_console(...) DebuggerOutput_(__VA_ARGS__)
#else
#define log_console(...) fprintf(stdout, __VA_ARGS__)
#define flush_console(...)        \
  do {                            \
    fprintf(stdout, __VA_ARGS__); \
    fflush(stdout);               \
  } while (0)
#define error_console(...) fprintf(stderr, __VA_ARGS__)
#define warn_console(...) fprintf(stderr, __VA_ARGS__)
#endif
#endif
