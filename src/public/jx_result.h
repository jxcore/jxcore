// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_PUBLIC_JX_RESULT_H
#define SRC_PUBLIC_JX_RESULT_H

#include <stdlib.h>
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
  RT_Null = 8
} JXResultType;

struct _JXResult {
  // internal use only
  void *context_;

  void *data_;
  size_t size_;
  JXResultType type_;
};

typedef struct _JXResult JXResult;

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
// for String, JSON, and Buffer
char *JX_GetString(JXResult *result);
int32_t JX_GetDataLength(JXResult *result);

void JX_FreeResultData(JXResult *result);

#ifdef __cplusplus
}
#endif

#endif  // SRC_PUBLIC_JX_RESULT_H
