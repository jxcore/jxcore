// Copyright & License details are available under JXCORE_LICENSE file

// !!! IT'S ALL INTERNAL. DO NOT INCLUDE THIS FILE !!!

/*
 * The approach here is;
 * - Define constant amount of method slots and make sure the wrapper
 * reaches to them within a constant time.
 *
 * - Under any condition do not expose underlying engine related
 * stuff
 */

#ifndef SRC_PUBLIC_JX_WRAPPER_INTERNAL_H_
#define SRC_PUBLIC_JX_WRAPPER_INTERNAL_H_

// allocates one extra JXResult memory at the end of the array
// Uses that one for a return value
#define CONVERT_ARG_TO_RESULT(results, context)                   \
  JXResult *results = NULL;                                       \
  const int len = args.Length();                                  \
  {                                                               \
    results = (JXResult *)malloc(sizeof(JXResult) * (len + 1));   \
    for (int i = 0; i < len; i++) {                               \
      JS_HANDLE_VALUE val = args.GetItem(i);                      \
      results[i].context_ = context;                              \
      results[i].data_ = NULL;                                    \
      results[i].size_ = 0;                                       \
      results[i].type_ = RT_Undefined;                            \
      jxcore::JXEngine::ConvertToJXResult(com, val, &results[i]); \
    }                                                             \
    results[len].context_ = context;                              \
    results[len].data_ = NULL;                                    \
    results[len].size_ = 0;                                       \
    results[len].type_ = RT_Undefined;                            \
  }

#define MAX_WRAPPERS_COUNT 72

#define DEFINE_WRAPPER_VARIABLES()                      \
  static JS_NATIVE_METHOD wrappers[MAX_WRAPPERS_COUNT]; \
  static JX_CALLBACK extensions[MAX_WRAPPERS_COUNT]

#define ME(n)                                                        \
  JS_LOCAL_METHOD(extensionCallback##n) {                            \
    CONVERT_ARG_TO_RESULT(results, __contextORisolate);              \
    extensions[n](results, len);                                     \
    if (results[len].type_ != RT_Undefined) {                        \
      if (results[len].type_ == RT_Error) {                          \
        std::string msg = JX_GetString(&results[len]);               \
        JX_FreeResultData(&results[len]);                            \
        THROW_EXCEPTION(msg.c_str());                                \
      }                                                              \
      JS_HANDLE_VALUE ret_val =                                      \
          jxcore::JXEngine::ConvertFromJXResult(com, &results[len]); \
      JX_FreeResultData(&results[len]);                              \
      RETURN_PARAM(ret_val);                                         \
    }                                                                \
  }                                                                  \
  JS_METHOD_END

#define _MEQ(x, y, z) \
  ME(x);              \
  ME(y);              \
  ME(z);

#define __MEQ(x, y, z, a, b, c)                            \
  _MEQ(x, y, z) _MEQ(a, b, c) void defineWrapperSet##x() { \
    wrappers[x] = extensionCallback##x;                    \
    wrappers[y] = extensionCallback##y;                    \
    wrappers[z] = extensionCallback##z;                    \
    wrappers[a] = extensionCallback##a;                    \
    wrappers[b] = extensionCallback##b;                    \
    wrappers[c] = extensionCallback##c;                    \
  }

#define MEQ(a, a1, b, b1, c, c1, x, x1, y, y1, z, z1) \
  __MEQ(a, a1, b, b1, c, c1) __MEQ(x, x1, y, y1, z, z1)

#define _DEFINE_WRAPPERS(a, b) \
  defineWrapperSet##a();       \
  defineWrapperSet##b()

#define DEFINE_WRAPPER_HOSTS()                            \
  MEQ(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11)               \
      MEQ(12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23) \
      MEQ(24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35) \
      MEQ(36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47) \
      MEQ(48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59) \
      MEQ(60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71)

#define DEFINE_WRAPPERS()   \
  _DEFINE_WRAPPERS(0, 6);   \
  _DEFINE_WRAPPERS(12, 18); \
  _DEFINE_WRAPPERS(24, 30); \
  _DEFINE_WRAPPERS(36, 42); \
  _DEFINE_WRAPPERS(48, 54); \
  _DEFINE_WRAPPERS(60, 66)

#endif  // SRC_PUBLIC_JX_WRAPPER_INTERNAL_H_
