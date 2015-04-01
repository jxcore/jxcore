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

#ifndef SRC_JX_PROXY_ENGINELOGGER_H_
#define SRC_JX_PROXY_ENGINELOGGER_H_

// Enabling PRINT_NATIVE_CALLS prints out all the native method calls and time
// consumed
// Works on single thread!
// #define JXCORE_PRINT_NATIVE_CALLS 1

// Enabling JXCORE_PRINT_NATIVE_CALLS_FILE_LINE adds the source file and line
// after the native method in the log report.
// Recommend this is enabled to differentiate different JS_LOCAL_METHOD(New)
// calls which may not be unique, such as LOCAL::New and LOCAL::SetAutoPadding
#define JXCORE_PRINT_NATIVE_CALLS_FILE_LINE 1

// Minimum number of native function call invocations to make it into report.
#ifndef JXCORE_PRINT_NATIVE_CALLS_MIN_COUNT
#define JXCORE_PRINT_NATIVE_CALLS_MIN_COUNT 1
#endif

// Minimum native function call total time to make it into report.
#ifndef JXCORE_PRINT_NATIVE_CALLS_MIN_TIME
#define JXCORE_PRINT_NATIVE_CALLS_MIN_TIME 1000.0
#endif

#ifdef DEBUG
void SOFT_BREAK_POINT();
#else
#ifndef SOFT_BREAK_POINT()
#define SOFT_BREAK_POINT()
#endif
#endif

#ifdef JXCORE_PRINT_NATIVE_CALLS
#include "uv.h"

class JX_Logger {
 public:
  JX_Logger(const char* name);
  ~JX_Logger();
  static void Print();
 private:
  JX_Logger();
  JX_Logger(const JX_Logger&);
  const char* name_;
  uint64_t time_enter_;
};

#ifdef JXCORE_PRINT_NATIVE_CALLS_FILE_LINE
#define JX_STRINGIZE_LINE2(L) #L
#define JX_STRINGIZE_LINE(L)  JX_STRINGIZE_LINE2(L)
#define JX_FILE_AND_LINE      " " __FILE__ ":" JX_STRINGIZE_LINE(__LINE__)
#else
#define JX_FILE_AND_LINE
#endif

#define ENGINE_LOG_THIS(cname, mname) \
  JX_Logger ___jxlog___(cname "::" mname JX_FILE_AND_LINE)

#define ENGINE_LOG_THAT(cname, mname, n) \
  JX_Logger ___jxlog___##n(cname "::" mname JX_FILE_AND_LINE)

#define ENGINE_PRINT_LOGS() JX_Logger::Print()
#else
#define ENGINE_LOG_THIS(cname, mname)
#define ENGINE_LOG_THAT(cname, mname, n)
#define ENGINE_PRINT_LOGS()
#endif

#endif  // SRC_JX_PROXY_ENGINELOGGER_H_
