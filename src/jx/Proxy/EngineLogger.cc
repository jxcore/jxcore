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

#include <map>
#include <string>
#include <stdlib.h>

#ifdef JXCORE_PRINT_NATIVE_CALLS

uint64_t JX_Logger::cumulo_ = 0;

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

LogDetailsMap logs;

void JX_Logger::Log(bool first) {
  if (!first) {
    uint64_t diff = 0;
    if (enter_ != 0)
      diff = uv_hrtime() - enter_;
    else
      return;

    LogDetails*& entry = logs[name_];
    if (!entry) {
      entry = new LogDetails();
    }

    uint64_t fix = (cumulo_ - my_cumulo_);

    if (fix > diff) fix = 0;

    entry->total_time_spent_ += diff - fix;
    entry->total_calls_++;

    cumulo_ += diff;
  } else {
    enter_ = uv_hrtime();
    my_cumulo_ = cumulo_;
  }
}

void JX_Logger::Print() {
#if defined(JS_ENGINE_V8)
  const char *engine_name = "V8";
#elif defined(JS_ENGINE_MOZJS)
  const char *engine_name = "MozJS";
#endif

  printf("JXcore's native logs for the actual JXEngine(%s) instance :\n\n",
         engine_name);
  printf("     Total-Time     Calls     Time/Call  Name\n");

  for (LogDetailsMap::iterator it = logs.begin(); it != logs.end(); it++) {
    // do not show any measurement for the samples less than thresholds
    double total_time = (double)it->second->total_time_spent_ / 1000;
    if (it->second->total_calls_ >= JXCORE_PRINT_NATIVE_CALLS_MIN_COUNT
        && total_time >= JXCORE_PRINT_NATIVE_CALLS_MIN_TIME) {
      printf("%15.3f %9ld %13.3f  %s\n",
        total_time, it->second->total_calls_,
        total_time / it->second->total_calls_,
        it->first.c_str());
    }
    delete it->second;
  }

  fflush(stdout);
  logs.clear();
}
#endif
