// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_PUBLIC_JX_RESULT_H
#define SRC_PUBLIC_JX_RESULT_H

#include <stdlib.h>
#include <stdint.h>

#ifdef _WIN32
#define JXCORE_EXTERN(x) __declspec(dllexport) x
#else
#define JXCORE_EXTERN(x) x
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum _JXType {
  RT_Int32 = 1,
  RT_Double = 2,
  RT_Boolean = 3,
  RT_String = 4,
  RT_Object = 5,
  RT_Buffer = 6,
  RT_Undefined = 7,
  RT_Null = 8,
  RT_Error = 9,
  RT_Function = 10
};

typedef enum _JXType JXResultType;
typedef enum _JXType JXValueType;

struct _JXValue {
  // internal use only
  void *com_;
  bool persistent_;
  bool was_stored_;

  void *data_;
  size_t size_;
  JXValueType type_;
};

typedef struct _JXValue JXResult;
typedef struct _JXValue JXValue;

JXCORE_EXTERN(bool)
JX_CallFunction(JXValue *fnc, JXValue *params, const int argc, JXValue *out);

JXCORE_EXTERN(bool)
JX_MakePersistent(JXValue *value);

JXCORE_EXTERN(bool)
JX_ClearPersistent(JXValue *value);

JXCORE_EXTERN(bool)
JX_IsFunction(JXValue *value);

JXCORE_EXTERN(bool)
JX_IsError(JXValue *value);

JXCORE_EXTERN(bool)
JX_IsInt32(JXValue *value);

JXCORE_EXTERN(bool)
JX_IsDouble(JXValue *value);

JXCORE_EXTERN(bool)
JX_IsBoolean(JXValue *value);

JXCORE_EXTERN(bool)
JX_IsString(JXValue *value);

JXCORE_EXTERN(bool)
JX_IsJSON(JXValue *value);

JXCORE_EXTERN(bool)
JX_IsBuffer(JXValue *value);

JXCORE_EXTERN(bool)
JX_IsUndefined(JXValue *value);

JXCORE_EXTERN(bool)
JX_IsNull(JXValue *value);

JXCORE_EXTERN(bool)
JX_IsNullOrUndefined(JXValue *value);

JXCORE_EXTERN(bool)
JX_IsObject(JXValue *value);

JXCORE_EXTERN(int32_t)
JX_GetInt32(JXValue *value);

JXCORE_EXTERN(double)
JX_GetDouble(JXValue *value);

JXCORE_EXTERN(bool)
JX_GetBoolean(JXValue *value);

// for String, JSON, Error and Buffer
// call free on return value when you are done with it
JXCORE_EXTERN(char *)
JX_GetString(JXValue *value);

JXCORE_EXTERN(int32_t)
JX_GetDataLength(JXValue *value);

JXCORE_EXTERN(void)
JX_SetInt32(JXValue *value, const int32_t val);

JXCORE_EXTERN(void)
JX_SetDouble(JXValue *value, const double val);

JXCORE_EXTERN(void)
JX_SetBoolean(JXValue *value, const bool val);

#ifdef __cplusplus
JXCORE_EXTERN(void)
JX_SetString(JXValue *value, const char *val, const int32_t length = 0);
#else
JXCORE_EXTERN(void)
JX_SetString(JXValue *value, const char *val, const int32_t length);
#endif

#ifdef __cplusplus
JXCORE_EXTERN(void)
JX_SetUCString(JXValue *value, const uint16_t *val, const int32_t length = 0);
#else
JXCORE_EXTERN(void)
JX_SetUCString(JXValue *value, const uint16_t *val, const int32_t length);
#endif

#ifdef __cplusplus
JXCORE_EXTERN(void)
JX_SetJSON(JXValue *value, const char *val, const int32_t length = 0);
#else
JXCORE_EXTERN(void)
JX_SetJSON(JXValue *value, const char *val, const int32_t length);
#endif

#ifdef __cplusplus
JXCORE_EXTERN(void)
JX_SetError(JXValue *value, const char *val, const int32_t length = 0);
#else
JXCORE_EXTERN(void)
JX_SetError(JXValue *value, const char *val, const int32_t length);
#endif

#ifdef __cplusplus
JXCORE_EXTERN(void)
JX_SetBuffer(JXValue *value, const char *val, const int32_t length = 0);
#else
JXCORE_EXTERN(void)
JX_SetBuffer(JXValue *value, const char *val, const int32_t length);
#endif

JXCORE_EXTERN(void)
JX_SetUndefined(JXValue *value);

JXCORE_EXTERN(void)
JX_SetNull(JXValue *value);

JXCORE_EXTERN(void)
JX_SetObject(JXValue *host, JXValue *val);

// do not use this for method parameters, jxcore cleanups them
// Beware JX_Evaluate, this methods needs to be called to cleanup JXResult
JXCORE_EXTERN(void)
JX_Free(JXValue *value);

JXCORE_EXTERN(bool)
JX_New(JXValue *value);

JXCORE_EXTERN(bool)
JX_CreateEmptyObject(JXValue *value);

JXCORE_EXTERN(bool)
JX_CreateArrayObject(JXValue *value);

JXCORE_EXTERN(void)
JX_SetNamedProperty(JXValue *object, const char *name, JXValue *prop);

JXCORE_EXTERN(void)
JX_SetIndexedProperty(JXValue *object, const unsigned index, JXValue *prop);

JXCORE_EXTERN(void)
JX_GetNamedProperty(JXValue *object, const char *name, JXValue *out);

JXCORE_EXTERN(void)
JX_GetIndexedProperty(JXValue *object, const int index, JXValue *out);

// if you have a JXValue around, this method brings threadId much faster
JXCORE_EXTERN(int)
JX_GetThreadIdByValue(JXValue *value);

#ifdef __cplusplus
}
#endif

#endif  // SRC_PUBLIC_JX_RESULT_H
