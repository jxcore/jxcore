// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_PUBLIC_JX_RESULT_H
#define SRC_PUBLIC_JX_RESULT_H

#include <stdlib.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

enum _JXType{
  RT_Int32 = 1,
  RT_Double = 2,
  RT_Boolean = 3,
  RT_String = 4,
  RT_JSON = 5,
  RT_Buffer = 6,
  RT_Undefined = 7,
  RT_Null = 8,
  RT_Error = 9,
  RT_Function = 10,
};

typedef enum _JXType JXResultType;
typedef enum _JXType JXValueType;

struct _JXValue {
  // internal use only
  void *com_;
  bool persistent_;

  void *data_;
  size_t size_;
  JXValueType type_;
};

typedef struct _JXValue JXResult;
typedef struct _JXValue JXValue;

bool JX_CallFunction(JXValue *fnc, JXValue *params, const int argc,
                     JXValue *out);

bool JX_MakePersistent(JXValue *value);
bool JX_ClearPersistent(JXValue *value);

bool JX_IsFunction(JXValue *value);
bool JX_IsError(JXValue *value);
bool JX_IsInt32(JXValue *value);
bool JX_IsDouble(JXValue *value);
bool JX_IsBoolean(JXValue *value);
bool JX_IsString(JXValue *value);
bool JX_IsJSON(JXValue *value);
bool JX_IsBuffer(JXValue *value);
bool JX_IsUndefined(JXValue *value);
bool JX_IsNull(JXValue *value);

int32_t JX_GetInt32(JXValue *value);
double JX_GetDouble(JXValue *value);
bool JX_GetBoolean(JXValue *value);
// for String, JSON, Error and Buffer
char *JX_GetString(JXValue *value);
int32_t JX_GetDataLength(JXValue *value);

void JX_SetInt32(JXValue *value, const int32_t val);
void JX_SetDouble(JXValue *value, const double val);
void JX_SetBoolean(JXValue *value, const bool val);
void JX_SetString(JXValue *value, const char *val, const int32_t length);
void JX_SetJSON(JXValue *value, const char *val, const int32_t length);
void JX_SetError(JXValue *value, const char *val, const int32_t length);
void JX_SetBuffer(JXValue *value, const char *val, const int32_t length);
void JX_SetUndefined(JXValue *value);
void JX_SetNull(JXValue *value);

// do not use this for method parameters, jxcore cleanups them
// Beware JX_Evaluate, this methods needs to be called to cleanup JXResult
void JX_Free(JXValue *value);
bool JX_New(JXValue *value);

#ifdef __cplusplus
}
#endif

#endif  // SRC_PUBLIC_JX_RESULT_H
