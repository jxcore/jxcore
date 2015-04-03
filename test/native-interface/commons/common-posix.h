// Copyright & License details are available under JXCORE_LICENSE file

#ifndef COMMON_POSIX_H_
#define COMMON_POSIX_H_

#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#if defined(JS_ENGINE_V8) || defined(JS_ENGINE_MOZJS)
#include "public/jx.h"
#else
#include "jx.h"
#endif

#define flush_console(...)        \
  do {                            \
    fprintf(stdout, __VA_ARGS__); \
    fflush(stdout);               \
  } while (0)

void ConvertResult(JXValue *result, std::string &to_result) {
  switch (result->type_) {
    case RT_Null:
      to_result = "null";
      break;
    case RT_Undefined:
      to_result = "undefined";
      break;
    case RT_Boolean:
      to_result = JX_GetBoolean(result) ? "true" : "false";
      break;
    case RT_Int32: {
      std::stringstream ss;
      ss << JX_GetInt32(result);
      to_result = ss.str();
    } break;
    case RT_Double: {
      std::stringstream ss;
      ss << JX_GetDouble(result);
      to_result = ss.str();
    } break;
    case RT_Buffer:
    case RT_JSON:
    case RT_String: {
      char *_ = JX_GetString(result);
      to_result = _;
      free(_);
    } break;
    case RT_Error: {
      to_result = JX_GetString(result);
    } break;
    default:
      to_result = "null";
      return;
  }
}

#endif /* COMMON_POSIX_H_ */
