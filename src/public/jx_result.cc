#include "../jx/commons.h"
#include "../jxcore.h"
#include "jx_result.h"

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

bool JX_IsFunction(JXValue *value) {
  return value->size_ > 0 && value->type_ == RT_Function;
}

bool JX_IsError(JXValue *value) {
  return value->size_ > 0 && value->type_ == RT_Error;
}

bool JX_IsInt32(JXValue *value) {
  return value->size_ > 0 && value->type_ == RT_Int32;
}

bool JX_IsDouble(JXValue *value) {
  return value->size_ > 0 && value->type_ == RT_Double;
}

bool JX_IsBoolean(JXValue *value) {
  return value->size_ > 0 && value->type_ == RT_Boolean;
}

bool JX_IsString(JXValue *value) {
  return value->size_ > 0 && value->type_ == RT_String;
}

bool JX_IsJSON(JXValue *value) {
  return value->size_ > 0 && value->type_ == RT_JSON;
}

bool JX_IsBuffer(JXValue *value) {
  return value->size_ > 0 && value->type_ == RT_Buffer;
}

bool JX_IsUndefined(JXValue *value) {
  return value->size_ == 0 || value->type_ == RT_Undefined;
}

bool JX_IsNull(JXValue *value) { return value->type_ == RT_Null; }

#define EMPTY_CHECK(x)                                \
  if (value == NULL) return x;                        \
  if (value->size_ == 0 || value->type_ == RT_Null || \
      value->type_ == RT_Undefined)                   \
  return x

#define UNWRAP_RESULT(x) \
  jxcore::JXValueWrapper *wrap = (jxcore::JXValueWrapper *)x

int32_t JX_GetInt32(JXValue *value) {
  EMPTY_CHECK(0);

  UNWRAP_RESULT(value->data_);
  UNWRAP_COM(value);
  ENTER_ENGINE_SCOPE();

  int32_t ret = INT32_TO_STD(wrap->value_);

  LEAVE_ENGINE_SCOPE();
  return ret;
}

double JX_GetDouble(JXValue *value) {
  EMPTY_CHECK(0);

  UNWRAP_RESULT(value->data_);
  UNWRAP_COM(value);
  ENTER_ENGINE_SCOPE();

  double ret = NUMBER_TO_STD(wrap->value_);

  LEAVE_ENGINE_SCOPE();
  return ret;
}

bool JX_GetBoolean(JXValue *value) {
  EMPTY_CHECK(false);

  UNWRAP_RESULT(value->data_);
  UNWRAP_COM(value);
  ENTER_ENGINE_SCOPE();

  bool ret = BOOLEAN_TO_STD(wrap->value_);

  LEAVE_ENGINE_SCOPE();
  return ret;
}

char *JX_GetString(JXValue *value) {
  EMPTY_CHECK(0);

  UNWRAP_COM(value);
  ENTER_ENGINE_SCOPE();
  UNWRAP_RESULT(value->data_);

  char *ret;
  if (value->type_ != RT_JSON) {
    ret = STRING_TO_STD(wrap->value_);
  } else {
    ret = jxcore::JX_Stringify(com, JS_VALUE_TO_OBJECT(wrap->value_),
                               &value->size_);
  }

  LEAVE_ENGINE_SCOPE();

  // SM allocates memory using JS_malloc
  // we need to free that memory
  // and allocate externally
#ifdef JS_ENGINE_MOZJS
  char *ret_ = strdup(ret);

  if (ret_ == NULL) {
	// out of memory ?
	return ret;
  }

  JS_free(__contextORisolate, ret);
  ret = ret_;
#endif

  return ret;
}

int32_t JX_GetDataLength(JXValue *value) {
  EMPTY_CHECK(0);

  return value->size_;
}

void JX_Free(JXValue *value) {
  assert(value != NULL && "JXResult object wasn't initialized");

  if (value->persistent_) return;

  UNWRAP_COM(value);
  ENTER_ENGINE_SCOPE();

  if (value->data_ == NULL || value->size_ == 0) return;

  if (value->type_ == RT_Function) {
    assert(sizeof(jxcore::JXFunctionWrapper) == value->size_ &&
           "Broken JXResult Function Object");

    jxcore::JXFunctionWrapper *wrap = (jxcore::JXFunctionWrapper *)value->data_;

    wrap->Dispose();
    delete (wrap);
  } else {
    _FREE_MEM_(value->data_);
  }
  value->data_ = NULL;
  value->size_ = 0;

  LEAVE_ENGINE_SCOPE();
}

bool JX_CallFunction(JXValue *fnc, JXValue *params, const int argc,
                     JXValue *out) {
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

void JX_SetInt32(JXValue *value, const int32_t val) {
  UNWRAP_COM(value);
  ENTER_ENGINE_SCOPE();
  UNWRAP_RESULT(value->data_);

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    value->data_ = (void *)wrap;
  } else if (!JS_IS_EMPTY(wrap->value_)) {
    JS_CLEAR_PERSISTENT(wrap->value_);
  }

  value->type_ = RT_Int32;
  value->size_ = sizeof(int32_t);

  wrap->value_ = JS_NEW_PERSISTENT_VALUE(STD_TO_INTEGER(val));

  LEAVE_ENGINE_SCOPE();
}

void JX_SetDouble(JXValue *value, const double val) {
  UNWRAP_COM(value);
  ENTER_ENGINE_SCOPE();
  UNWRAP_RESULT(value->data_);

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    value->data_ = (void *)wrap;
  } else if (!JS_IS_EMPTY(wrap->value_)) {
    JS_CLEAR_PERSISTENT(wrap->value_);
  }

  value->type_ = RT_Double;
  value->size_ = sizeof(double);

  wrap->value_ = JS_NEW_PERSISTENT_VALUE(STD_TO_NUMBER(val));

  LEAVE_ENGINE_SCOPE();
}

void JX_SetBoolean(JXValue *value, const bool val) {
  UNWRAP_COM(value);
  ENTER_ENGINE_SCOPE();
  UNWRAP_RESULT(value->data_);

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    value->data_ = (void *)wrap;
  } else if (!JS_IS_EMPTY(wrap->value_)) {
    JS_CLEAR_PERSISTENT(wrap->value_);
  }

  value->type_ = RT_Boolean;
  value->size_ = sizeof(bool);

  wrap->value_ = JS_NEW_PERSISTENT_VALUE(STD_TO_BOOLEAN(val));
  LEAVE_ENGINE_SCOPE();
}

void JX_SetString(JXValue *value, const char *val, const int32_t length) {
  UNWRAP_COM(value);
  ENTER_ENGINE_SCOPE();
  UNWRAP_RESULT(value->data_);

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    value->data_ = (void *)wrap;
  } else if (!JS_IS_EMPTY(wrap->value_)) {
    JS_CLEAR_PERSISTENT(wrap->value_);
  }

  value->type_ = RT_String;
  value->size_ = length;

  wrap->value_ =
      JS_NEW_PERSISTENT_VALUE(UTF8_TO_STRING_WITH_LENGTH(val, length));
  LEAVE_ENGINE_SCOPE();
}

void JX_SetJSON(JXValue *value, const char *val, const int32_t length) {
  UNWRAP_COM(value);
  ENTER_ENGINE_SCOPE();
  UNWRAP_RESULT(value->data_);

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    value->data_ = (void *)wrap;
  } else if (!JS_IS_EMPTY(wrap->value_)) {
    JS_CLEAR_PERSISTENT(wrap->value_);
  }

  value->type_ = RT_JSON;
  value->size_ = length;

  JS_HANDLE_VALUE hval = jxcore::JX_Parse(com, val, length);

  wrap->value_ = JS_NEW_PERSISTENT_VALUE(hval);
  LEAVE_ENGINE_SCOPE();
}

void JX_SetError(JXValue *value, const char *val, const int32_t length) {
  UNWRAP_COM(value);
  ENTER_ENGINE_SCOPE();
  UNWRAP_RESULT(value->data_);

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    value->data_ = (void *)wrap;
  } else if (!JS_IS_EMPTY(wrap->value_)) {
    JS_CLEAR_PERSISTENT(wrap->value_);
  }

  value->type_ = RT_Error;
  value->size_ = length;

  wrap->value_ =
      JS_NEW_PERSISTENT_VALUE(UTF8_TO_STRING_WITH_LENGTH(val, length));
  LEAVE_ENGINE_SCOPE();
}

void JX_SetBuffer(JXValue *value, const char *val, const int32_t length) {
  UNWRAP_COM(value);
  ENTER_ENGINE_SCOPE();
  UNWRAP_RESULT(value->data_);

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    value->data_ = (void *)wrap;
  } else if (!JS_IS_EMPTY(wrap->value_)) {
    JS_CLEAR_PERSISTENT(wrap->value_);
  }

  value->type_ = RT_Buffer;
  value->size_ = length;

  wrap->value_ =
      JS_NEW_PERSISTENT_VALUE(UTF8_TO_STRING_WITH_LENGTH(val, length));
  LEAVE_ENGINE_SCOPE();
}

void JX_SetUndefined(JXValue *value) { value->type_ = RT_Undefined; }

void JX_SetNull(JXValue *value) { value->type_ = RT_Null; }

bool JX_MakePersistent(JXValue *value) {
  assert(value->com_ != NULL && value->size_ != 0 &&
         "Empty, Null or Undefined JS Value can not be persistent");

  bool pre = value->persistent_;
  value->persistent_ = true;

  return !pre;
}

bool JX_ClearPersistent(JXValue *value) {
  assert(value->com_ != NULL && value->size_ != 0 &&
         "Empty, Null or Undefined JS Value can not be persistent");

  bool pre = value->persistent_;
  value->persistent_ = false;

  return pre;
}

bool JX_New(JXValue *value) {
  node::commons *com = node::commons::getInstance();

  // if returns false, that means the JXcore instance
  // wasn't initialized for the current thread
  if (com == NULL) return false;

  value->com_ = com;
  value->data_ = NULL;
  value->size_ = 0;
  value->persistent_ = false;
  value->type_ = RT_Undefined;

  return true;
}
