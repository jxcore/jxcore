/* Copyright (c) 2014, Oguz Bastemur (oguz@bastemur.com)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "EngineLogger.h"

#ifdef JXCORE_PRINT_NATIVE_CALLS

#include <map>
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

class LogDetails {
 public:
  long total_calls_;
  uint64_t total_time_spent_;

  LogDetails() {
    total_calls_ = 0;
    total_time_spent_ = 0;
  }
};

typedef std::map<std::string, LogDetails *> LogDetailsMap;

static LogDetailsMap s_logs; // NOTE: not threadsafe

JX_Logger::JX_Logger(const char* name) {
#ifdef JXCORE_PRINT_LOGS_ASAP
  log_console("JXcore EngineLogger: %s\n", name);
#else
  name_ = name;
  time_enter_ = uv_hrtime();
#endif
}

JX_Logger::~JX_Logger() {
#ifndef JXCORE_PRINT_LOGS_ASAP
  if (time_enter_) {
    const uint64_t now = uv_hrtime();
    if (now > time_enter_) {
      LogDetails*& entry = s_logs[name_];
      if (!entry) {
        entry = new LogDetails();
      }
      entry->total_time_spent_ += now - time_enter_;
      entry->total_calls_++;
    }
  }
#endif
}

void JX_Logger::Print() {
#ifndef JXCORE_PRINT_LOGS_ASAP
#if defined(JS_ENGINE_V8)
  const char *engine_name = "V8";
#elif defined(JS_ENGINE_MOZJS)
  const char *engine_name = "MozJS";
#endif

  log_console("JXcore native calls for the JXEngine(%s) instance:\n\n",
         engine_name);
  log_console("     Total-Time     Calls     Time/Call  Name\n");

  for (LogDetailsMap::iterator it = s_logs.begin(), E = s_logs.end();
      it != E; ++it) {
    // do not show any measurement for the samples less than thresholds
    double total_time = (double)it->second->total_time_spent_ / 1000;
    if (it->second->total_calls_ >= JXCORE_PRINT_NATIVE_CALLS_MIN_COUNT
        && total_time >= JXCORE_PRINT_NATIVE_CALLS_MIN_TIME) {
      log_console("%15.3f %9ld %13.3f  %s\n",
        total_time, it->second->total_calls_,
        total_time / it->second->total_calls_,
        it->first.c_str());
    }
    delete it->second;
  }

  fflush(stdout);
  s_logs.clear();
#endif
}
#endif
