#include "jx_result.h"
#include "../jx/commons.h"

bool JX_ResultIsInt32(JXResult *result) {
  return result->size_ > 0 && result->type_ == RT_Int32;
}

bool JX_ResultIsDouble(JXResult *result) {
  return result->size_ > 0 && result->type_ == RT_Double;
}

bool JX_ResultIsBoolean(JXResult *result) {
  return result->size_ > 0 && result->type_ == RT_Boolean;
}

bool JX_ResultIsString(JXResult *result) {
  return result->size_ > 0 && result->type_ == RT_String;
}

bool JX_ResultIsJSON(JXResult *result) {
  return result->size_ > 0 && result->type_ == RT_JSON;
}

bool JX_ResultIsBuffer(JXResult *result) {
  return result->size_ > 0 && result->type_ == RT_Buffer;
}

bool JX_ResultIsUndefined(JXResult *result) {
  return result->size_ == 0 || result->type_ == RT_Undefined;
}

bool JX_ResultIsNull(JXResult *result) {
  return result->type_ == RT_Null;
}

#define EMPTY_CHECK(x)                                  \
  if (result == NULL) return x;                         \
  if (result->size_ == 0 || result->type_ == RT_Null || \
      result->type_ == RT_Undefined)                    \
  return x

int32_t JX_GetInt32(JXResult *result) {
  EMPTY_CHECK(0);

  return *((int32_t *)result->data_);
}

double JX_GetDouble(JXResult *result) {
  EMPTY_CHECK(0);

  return *((double *)result->data_);
}

bool JX_GetBoolean(JXResult *result) {
  EMPTY_CHECK(false);

  return *((bool *)result->data_);
}

char *JX_GetString(JXResult *result) {
  EMPTY_CHECK(0);

  return (char *)result->data_;
}

int32_t JX_GetDataLength(JXResult *result) {
  EMPTY_CHECK(0);

  return result->size_;
}

void JX_FreeResultData(JXResult *result) {
  if (result == NULL) return;

  if (result->data_ == NULL || result->size_ == 0) return;

#ifdef JS_ENGINE_MOZJS
  assert(result->context_ != NULL && "Broken JXResult object");
  JS_free((JSContext *)result->context_, result->data_);
#else
  free(result->data_);
#endif

  result->data_ = NULL;
  result->size_ = 0;
}
