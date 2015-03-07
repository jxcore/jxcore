#include "jx_result.h"
#include "../jx/commons.h"

bool JX_ResultIsError(JXResult *result) {
  return result->size_ > 0 && result->type_ == RT_Error;
}

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

// For MozJS implementation, JXString memory is managed by SpiderMonkey.
// In order not to call strdup on JSON, and String return types, below will
// use SM interface for other types to make it consistent.
#ifdef JS_ENGINE_V8
#define ALLOC_MEMORY(type, size) (type *) malloc(sizeof(type) * size)
#elif defined(JS_ENGINE_MOZJS)
#define ALLOC_MEMORY(type, size) \
  (type *) JS_malloc(__contextORisolate, sizeof(type) * size)
#endif

#ifdef JS_ENGINE_V8
#define SET_ASSERT() \
	assert(result->context_  && "context can not be NULL"); \
	JS_DEFINE_STATE_MARKER_((v8::Isolate*)result->context_)
#elif defined(JS_ENGINE_MOZJS)
#define SET_ASSERT() \
	assert(result->context_  && "context can not be NULL"); \
	JS_DEFINE_STATE_MARKER_((JSContext*)result->context_)
#endif

#define SET_BASIC_TYPE(type, definer) \
  type *mem = ALLOC_MEMORY(type, 1); \
  memcpy(mem, &val, sizeof(type)); \
  result->data_ = mem; \
  result->size_ = sizeof(type); \
  result->type_ = definer

void JX_SetInt32(JXResult *result, const int32_t val) {
  SET_ASSERT();
  SET_BASIC_TYPE(int32_t, RT_Int32);
}

void JX_SetDouble(JXResult *result, const double val) {
  SET_ASSERT();
  SET_BASIC_TYPE(double, RT_Double);
}

void JX_SetBoolean(JXResult *result, const bool val) {
  SET_ASSERT();
  SET_BASIC_TYPE(bool, RT_Boolean);
}

#define SET_STRING_TYPE(definer, length) \
  char *mem = ALLOC_MEMORY(char, length + 1); \
  memcpy(mem, val, sizeof(char) * length); \
  mem[length] = '\0'; \
  result->data_ = mem; \
  result->size_ = length; \
  result->type_ = definer

void JX_SetString(JXResult *result, const char* val, const int32_t length) {
  SET_ASSERT();
  SET_STRING_TYPE(RT_String, length);
}

void JX_SetJSON(JXResult *result, const char* val, const int32_t length) {
  SET_ASSERT();
  SET_STRING_TYPE(RT_JSON, length);
}

void JX_SetError(JXResult *result, const char* val, const int32_t length) {
  SET_ASSERT();
  SET_STRING_TYPE(RT_Error, length);
}

void JX_SetBuffer(JXResult *result, const char* val, const int32_t length) {
  SET_ASSERT();
  SET_STRING_TYPE(RT_Buffer, length);
}

void JX_SetUndefined(JXResult *result) {
  result->type_ = RT_Undefined;
}

void JX_SetNull(JXResult *result) {
  result->type_ = RT_Null;
}

