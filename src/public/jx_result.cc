#include "jx_result.h"
#include "../jx/commons.h"
#include "../jxcore.h"

#define _FREE_MEM_(x)                                             \
  jxcore::JXValueWrapper *wrap##x_ = (jxcore::JXValueWrapper *)x; \
  delete wrap##x_

#define UNWRAP_COM(arg)                            \
  assert(arg->com_ && "com_ can not be NULL");     \
  node::commons *com = (node::commons *)arg->com_; \
  JS_DEFINE_STATE_MARKER(com)

#ifdef JS_ENGINE_V8
#define ENTER_ENGINE_SCOPE()                                  \
  jxcore::JXEngine *engine =                                  \
      jxcore::JXEngine::GetInstanceByThreadId(com->threadId); \
  bool __isolate_enter = false;                               \
  if (engine != NULL)                                         \
    if (!engine->IsInScope()) {                               \
      __isolate_enter = true;                                 \
      com->node_isolate->Enter();                             \
    }

#define LEAVE_ENGINE_SCOPE() \
  if (__isolate_enter) com->node_isolate->Exit()
#elif JS_ENGINE_MOZJS
#define ENTER_ENGINE_SCOPE()

#define LEAVE_ENGINE_SCOPE()
#endif

bool JX_ResultIsFunction(JXResult *result) {
  return result->size_ > 0 && result->type_ == RT_Function;
}

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

bool JX_ResultIsNull(JXResult *result) { return result->type_ == RT_Null; }

#define EMPTY_CHECK(x)                                  \
  if (result == NULL) return x;                         \
  if (result->size_ == 0 || result->type_ == RT_Null || \
      result->type_ == RT_Undefined)                    \
  return x

#define UNWRAP_RESULT(x) \
  jxcore::JXValueWrapper *wrap = (jxcore::JXValueWrapper *)x

int32_t JX_GetInt32(JXResult *result) {
  EMPTY_CHECK(0);

  UNWRAP_RESULT(result->data_);
  UNWRAP_COM(result);
  ENTER_ENGINE_SCOPE();

  int32_t ret = INT32_TO_STD(wrap->value_);

  LEAVE_ENGINE_SCOPE();
  return ret;
}

double JX_GetDouble(JXResult *result) {
  EMPTY_CHECK(0);

  UNWRAP_RESULT(result->data_);
  UNWRAP_COM(result);
  ENTER_ENGINE_SCOPE();

  double ret = NUMBER_TO_STD(wrap->value_);

  LEAVE_ENGINE_SCOPE();
  return ret;
}

bool JX_GetBoolean(JXResult *result) {
  EMPTY_CHECK(false);

  UNWRAP_RESULT(result->data_);
  UNWRAP_COM(result);
  ENTER_ENGINE_SCOPE();

  bool ret = BOOLEAN_TO_STD(wrap->value_);

  LEAVE_ENGINE_SCOPE();
  return ret;
}

char *JX_GetString(JXResult *result) {
  EMPTY_CHECK(0);

  UNWRAP_COM(result);
  ENTER_ENGINE_SCOPE();
  UNWRAP_RESULT(result->data_);

  char *ret;
  if (result->type_ != RT_JSON) {
    ret = strdup(STRING_TO_STD(wrap->value_));
  } else {
    ret = jxcore::JX_Stringify(com, JS_VALUE_TO_OBJECT(wrap->value_),
                               &result->size_);
  }

  LEAVE_ENGINE_SCOPE();

  return ret;
}

int32_t JX_GetDataLength(JXResult *result) {
  EMPTY_CHECK(0);

  return result->size_;
}

void JX_FreeResultData(JXResult *result) {
  assert(result != NULL && "JXResult object wasn't initialized");

  if (result->persistent_) return;

  UNWRAP_COM(result);
  ENTER_ENGINE_SCOPE();

  if (result->data_ == NULL || result->size_ == 0) return;

  if (result->type_ == RT_Function) {
    assert(sizeof(jxcore::JXFunctionWrapper) == result->size_ &&
           "Broken JXResult Function Object");

    jxcore::JXFunctionWrapper *wrap =
        (jxcore::JXFunctionWrapper *)result->data_;

    wrap->Dispose();
    delete (wrap);
  } else {
    _FREE_MEM_(result->data_);
  }
  result->data_ = NULL;
  result->size_ = 0;

  LEAVE_ENGINE_SCOPE();
}

bool JX_CallFunction(JXResult *fnc, JXResult *params, const int argc,
                     JXResult *out) {
  UNWRAP_COM(fnc);

  out->com_ = fnc->com_;
  out->data_ = NULL;
  out->size_ = 0;
  out->type_ = RT_Undefined;

  if (fnc->type_ != RT_Function || com == NULL ||
      sizeof(jxcore::JXFunctionWrapper) != fnc->size_) {
    JX_SetUndefined(out);
    return false;
  }

  ENTER_ENGINE_SCOPE();

  jxcore::JXFunctionWrapper *wrap = (jxcore::JXFunctionWrapper *)fnc->data_;

  JS_HANDLE_VALUE *arr =
      (JS_HANDLE_VALUE *)malloc(sizeof(JS_HANDLE_VALUE) * argc);

  for (int i = 0; i < argc; i++) {
    jxcore::JXValueWrapper *wrap = (jxcore::JXValueWrapper *)params[i].data_;
    arr[i] = wrap->value_;
  }

  bool done = false;
  JS_HANDLE_VALUE res = wrap->Call(argc, arr, &done);
  free(arr);

  if (!done) {
    JX_SetUndefined(out);
    LEAVE_ENGINE_SCOPE();
    return false;
  }

  bool ret = jxcore::JXEngine::ConvertToJXResult(com, res, out);

  LEAVE_ENGINE_SCOPE();
  return ret;
}

void JX_SetInt32(JXResult *result, const int32_t val) {
  UNWRAP_COM(result);
  ENTER_ENGINE_SCOPE();
  UNWRAP_RESULT(result->data_);

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    result->data_ = (void *)wrap;
  }

  result->type_ = RT_Int32;
  result->size_ = sizeof(int32_t);

  wrap->value_ = JS_NEW_PERSISTENT_VALUE(STD_TO_INTEGER(val));

  LEAVE_ENGINE_SCOPE();
}

void JX_SetDouble(JXResult *result, const double val) {
  UNWRAP_COM(result);
  ENTER_ENGINE_SCOPE();
  UNWRAP_RESULT(result->data_);

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    result->data_ = (void *)wrap;
  }

  result->type_ = RT_Double;
  result->size_ = sizeof(double);

  wrap->value_ = JS_NEW_PERSISTENT_VALUE(STD_TO_NUMBER(val));

  LEAVE_ENGINE_SCOPE();
}

void JX_SetBoolean(JXResult *result, const bool val) {
  UNWRAP_COM(result);
  ENTER_ENGINE_SCOPE();
  UNWRAP_RESULT(result->data_);

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    result->data_ = (void *)wrap;
  }

  result->type_ = RT_Boolean;
  result->size_ = sizeof(bool);

  wrap->value_ = JS_NEW_PERSISTENT_VALUE(STD_TO_BOOLEAN(val));
  LEAVE_ENGINE_SCOPE();
}

void JX_SetString(JXResult *result, const char *val, const int32_t length) {
  UNWRAP_COM(result);
  ENTER_ENGINE_SCOPE();
  UNWRAP_RESULT(result->data_);

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    result->data_ = (void *)wrap;
  }

  result->type_ = RT_String;
  result->size_ = length;

  wrap->value_ =
      JS_NEW_PERSISTENT_VALUE(UTF8_TO_STRING_WITH_LENGTH(val, length));
  LEAVE_ENGINE_SCOPE();
}

void JX_SetJSON(JXResult *result, const char *val, const int32_t length) {
  UNWRAP_COM(result);
  ENTER_ENGINE_SCOPE();
  UNWRAP_RESULT(result->data_);

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    result->data_ = (void *)wrap;
  }

  result->type_ = RT_JSON;
  result->size_ = length;

  JS_HANDLE_VALUE hval = jxcore::JX_Parse(com, val, length);

  wrap->value_ = JS_NEW_PERSISTENT_VALUE(hval);
  LEAVE_ENGINE_SCOPE();
}

void JX_SetError(JXResult *result, const char *val, const int32_t length) {
  UNWRAP_COM(result);
  ENTER_ENGINE_SCOPE();
  UNWRAP_RESULT(result->data_);

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    result->data_ = (void *)wrap;
  }

  result->type_ = RT_Error;
  result->size_ = length;

  wrap->value_ =
      JS_NEW_PERSISTENT_VALUE(UTF8_TO_STRING_WITH_LENGTH(val, length));
  LEAVE_ENGINE_SCOPE();
}

void JX_SetBuffer(JXResult *result, const char *val, const int32_t length) {
  UNWRAP_COM(result);
  ENTER_ENGINE_SCOPE();
  UNWRAP_RESULT(result->data_);

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    result->data_ = (void *)wrap;
  }

  result->type_ = RT_Buffer;
  result->size_ = length;

  wrap->value_ =
      JS_NEW_PERSISTENT_VALUE(UTF8_TO_STRING_WITH_LENGTH(val, length));
  LEAVE_ENGINE_SCOPE();
}

void JX_SetUndefined(JXResult *result) { result->type_ = RT_Undefined; }

void JX_SetNull(JXResult *result) { result->type_ = RT_Null; }

bool JX_MakePersistent(JXResult *result) {
  assert(result->com_ != NULL && result->size_ != 0 &&
         "Empty, Null or Undefined JS Value can not be persistent");

  bool pre = result->persistent_;
  result->persistent_ = true;

  return !pre;
}

bool JX_ClearPersistent(JXResult *result) {
  assert(result->com_ != NULL && result->size_ != 0 &&
         "Empty, Null or Undefined JS Value can not be persistent");

  bool pre = result->persistent_;
  result->persistent_ = false;

  return pre;
}
