// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_PUBLIC_JX_RESULT_H
#define SRC_PUBLIC_JX_RESULT_H

#include <stdlib.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
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
} JXResultType;

struct _JXResult {
  // internal use only
  void *context_;

  void *data_;
  size_t size_;
  JXResultType type_;
};

typedef struct _JXResult JXResult;

bool JX_ResultIsFunction(JXResult *result);
bool JX_ResultIsError(JXResult *result);
bool JX_ResultIsInt32(JXResult *result);
bool JX_ResultIsDouble(JXResult *result);
bool JX_ResultIsBoolean(JXResult *result);
bool JX_ResultIsString(JXResult *result);
bool JX_ResultIsJSON(JXResult *result);
bool JX_ResultIsBuffer(JXResult *result);
bool JX_ResultIsUndefined(JXResult *result);
bool JX_ResultIsNull(JXResult *result);

int32_t JX_GetInt32(JXResult *result);
double JX_GetDouble(JXResult *result);
bool JX_GetBoolean(JXResult *result);
// for String, JSON, Error and Buffer
char *JX_GetString(JXResult *result);
int32_t JX_GetDataLength(JXResult *result);

void JX_SetInt32(JXResult *result, const int32_t val);
void JX_SetDouble(JXResult *result, const double val);
void JX_SetBoolean(JXResult *result, const bool val);
void JX_SetString(JXResult *result, const char *val, const int32_t length);
void JX_SetJSON(JXResult *result, const char *val, const int32_t length);
void JX_SetError(JXResult *result, const char *val, const int32_t length);
void JX_SetBuffer(JXResult *result, const char *val, const int32_t length);
void JX_SetUndefined(JXResult *result);
void JX_SetNull(JXResult *result);

// do not use this for method parameters, jxcore already cleanup them after the
// call
// for JX_Evaluate, this methods needs to be called to cleanup JXResult
void JX_FreeResultData(JXResult *result);

#ifdef __cplusplus
}
#endif

#endif  // SRC_PUBLIC_JX_RESULT_H
