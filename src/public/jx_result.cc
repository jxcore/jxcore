#include "../jx/commons.h"
#include "../jxcore.h"
#include "jx_result.h"
#include "node_buffer.h"
#ifdef JS_ENGINE_MOZJS
#include <limits>  // INT_MAX
#endif

class auto_state {
  jxcore::JXEngine *engine_;
  node::commons *com_;

 public:
  auto_state(jxcore::JXEngine *engine, node::commons *com)
      : engine_(engine), com_(com) {
    engine_->EnterScope();
#ifdef JS_ENGINE_V8
    com->node_isolate->Enter();
#endif
  }

  ~auto_state() {
    engine_->LeaveScope();
#ifdef JS_ENGINE_V8
    com_->node_isolate->Exit();
#endif
  }
};

#define _FREE_MEM_(x)                                             \
  jxcore::JXValueWrapper *wrap##x_ = (jxcore::JXValueWrapper *)x; \
  delete wrap##x_

#define UNWRAP_COM(arg)                            \
  assert(arg->com_ && "com_ can not be NULL");     \
  node::commons *com = (node::commons *)arg->com_; \
  JS_DEFINE_STATE_MARKER(com);                     \
  jxcore::JXEngine *engine =                       \
      jxcore::JXEngine::GetInstanceByThreadId(com->threadId)

#ifdef JS_ENGINE_V8
#ifdef V8_IS_3_14
#define ENTER_ENGINE_SCOPE()         \
  JS_ENGINE_LOCKER();                \
  auto_state __state__(engine, com); \
  v8::Context::Scope context_scope(engine->getContext())
#else
#define ENTER_ENGINE_SCOPE()         \
  JS_ENGINE_LOCKER();                \
  auto_state __state__(engine, com); \
  v8::Context::Scope context_scope(engine->getContext())
#endif

#define RUN_IN_SCOPE(x)                         \
  if (engine != NULL && !engine->IsInScope()) { \
    ENTER_ENGINE_SCOPE();                       \
    x                                           \
  } else {                                      \
    x                                           \
  }
#elif JS_ENGINE_MOZJS
#define ENTER_ENGINE_SCOPE()
#define RUN_IN_SCOPE(x)                         \
  if (engine != NULL && !engine->IsInScope()) { \
    auto_state __state__(engine, com);          \
    x                                           \
  } else {                                      \
    x                                           \
  }
#endif

#define NULL_CHECK \
  if (value == NULL) return false;

JXCORE_EXTERN(bool)
JX_IsFunction(JXValue *value) {
  NULL_CHECK
  return value->size_ > 0 && value->type_ == RT_Function;
}

JXCORE_EXTERN(bool)
JX_IsError(JXValue *value) {
  NULL_CHECK
  return value->size_ > 0 && value->type_ == RT_Error;
}

JXCORE_EXTERN(bool)
JX_IsInt32(JXValue *value) {
  NULL_CHECK
  return value->size_ > 0 && value->type_ == RT_Int32;
}

JXCORE_EXTERN(bool)
JX_IsDouble(JXValue *value) {
  NULL_CHECK
  return value->size_ > 0 && value->type_ == RT_Double;
}

JXCORE_EXTERN(bool)
JX_IsBoolean(JXValue *value) {
  NULL_CHECK
  return value->size_ > 0 && value->type_ == RT_Boolean;
}

JXCORE_EXTERN(bool)
JX_IsString(JXValue *value) {
  NULL_CHECK
  return value->size_ >= 0 && value->type_ == RT_String;
}

JXCORE_EXTERN(bool)
JX_IsJSON(JXValue *value) { return JX_IsObject(value); }

JXCORE_EXTERN(bool)
JX_IsBuffer(JXValue *value) {
  NULL_CHECK
  return value->size_ > 0 && value->type_ == RT_Buffer;
}

JXCORE_EXTERN(bool)
JX_IsUndefined(JXValue *value) {
  NULL_CHECK
  return value->type_ == RT_Undefined;
}

JXCORE_EXTERN(bool)
JX_IsNull(JXValue *value) {
  NULL_CHECK
  return value->type_ == RT_Null;
}

JXCORE_EXTERN(bool)
JX_IsNullOrUndefined(JXValue *value) {
  NULL_CHECK
  return value->type_ == RT_Null || value->type_ == RT_Undefined;
}

JXCORE_EXTERN(bool)
JX_IsObject(JXValue *value) {
  NULL_CHECK
  return value->type_ == RT_Object;
}

#define EMPTY_CHECK(x)         \
  if (value == NULL) return x; \
  if (value->type_ == RT_Null || value->type_ == RT_Undefined) return x

#define UNWRAP_RESULT(x) \
  jxcore::JXValueWrapper *wrap = (jxcore::JXValueWrapper *)x

JXCORE_EXTERN(int32_t)
JX_GetInt32(JXValue *value) {
  EMPTY_CHECK(0);

  UNWRAP_RESULT(value->data_);
  UNWRAP_COM(value);

  int32_t ret;

  RUN_IN_SCOPE({ ret = INT32_TO_STD(JS_TYPE_TO_LOCAL_VALUE(wrap->value_)); });

  return ret;
}

JXCORE_EXTERN(double)
JX_GetDouble(JXValue *value) {
  EMPTY_CHECK(0);

  UNWRAP_RESULT(value->data_);
  UNWRAP_COM(value);

  double ret;

  RUN_IN_SCOPE({ ret = NUMBER_TO_STD(JS_TYPE_TO_LOCAL_VALUE(wrap->value_)); });

  return ret;
}

JXCORE_EXTERN(bool)
JX_GetBoolean(JXValue *value) {
  EMPTY_CHECK(false);

  UNWRAP_RESULT(value->data_);
  UNWRAP_COM(value);

  bool ret;

  RUN_IN_SCOPE({ ret = BOOLEAN_TO_STD(JS_TYPE_TO_LOCAL_VALUE(wrap->value_)); });

  return ret;
}

// SM allocates memory using JS_malloc
// we need to free that memory
// and allocate externally
#ifdef JS_ENGINE_MOZJS
#define MOZ_CLEAR_                  \
  char *ret_ = strdup(ret);         \
  if (ret_ == NULL) {               \
    return ret;                     \
  }                                 \
                                    \
  JS_free(__contextORisolate, ret); \
  ret = ret_;
#else
#define MOZ_CLEAR_
#endif

JXCORE_EXTERN(char *)
JX_GetString(JXValue *value) {
  EMPTY_CHECK(0);

  UNWRAP_COM(value);
  UNWRAP_RESULT(value->data_);

  char *ret;
  RUN_IN_SCOPE({
    switch (value->type_) {
      case RT_String: {
        JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(wrap->value_);
        ret = strdup(STRING_TO_STD(JS_VALUE_TO_STRING(objl)));
      } break;
      case RT_Object: {
        JS_LOCAL_OBJECT obj = JS_OBJECT_FROM_PERSISTENT(wrap->value_);

        {
          ret = jxcore::JX_Stringify(com, obj, &value->size_);
          MOZ_CLEAR_
        }

        bool get_message = false;
        JS_LOCAL_STRING str_msg;
        if (strlen(ret) == 2 && ret[1] == '}') {
          str_msg = STD_TO_STRING("message");
          if (JS_HAS_NAME(obj, str_msg)) {
            free(ret);
            get_message = true;
          }
        }

        if (get_message) {
          JS_LOCAL_VALUE msg = JS_GET_NAME(obj, str_msg);
          JS_LOCAL_VALUE name = JS_GET_NAME(obj, STD_TO_STRING("name"));
          std::string err_msg = STRING_TO_STD(name);
          err_msg += ": ";
          err_msg += STRING_TO_STD(msg);
          ret = strdup(err_msg.c_str());
        }
      } break;
      default: {
        JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(wrap->value_);
        // calls JavaScript .toString
        ret = strdup(STRING_TO_STD(JS_VALUE_TO_STRING(objl)));
      }
    }
  });

  return ret;
}

JXCORE_EXTERN(int32_t)
JX_GetDataLength(JXValue *value) {
  EMPTY_CHECK(0);

  return value->size_;
}

JXCORE_EXTERN(char *)
JX_GetBuffer(JXValue *value) {
  EMPTY_CHECK(NULL);

  UNWRAP_COM(value);
  UNWRAP_RESULT(value->data_);

  char *data = NULL;
  RUN_IN_SCOPE({
    if (value->type_ == RT_Buffer) {
      JS_LOCAL_OBJECT obj = JS_OBJECT_FROM_PERSISTENT(wrap->value_);
      data = BUFFER__DATA(obj);
    }
  });

  return data;
}

JXCORE_EXTERN(void)
JX_Free(JXValue *value) {
  assert(value != NULL && "JXResult object wasn't initialized");

  if (value->persistent_) return;

  UNWRAP_COM(value);

  if (value->data_ == NULL || value->type_ == RT_Undefined ||
      value->type_ == RT_Null)
    return;

  RUN_IN_SCOPE({
    if (value->type_ == RT_Function) {
      assert(sizeof(jxcore::JXFunctionWrapper) == value->size_ &&
             "Broken JXResult Function Object");

      jxcore::JXFunctionWrapper *wrap =
          (jxcore::JXFunctionWrapper *)value->data_;

      wrap->Dispose();
      delete (wrap);
    } else {
      _FREE_MEM_(value->data_);
    }
  });
  value->data_ = NULL;
  value->size_ = 0;
  value->type_ = RT_Undefined;

  if (value->was_stored_) {
    // compiler will be optimizing this anyways..
    value->was_stored_ = false;
    delete value;
  }
}

JXCORE_EXTERN(bool)
JX_CallFunction(JXValue *fnc, JXValue *params, const int argc, JXValue *out) {
  UNWRAP_COM(fnc);

  out->com_ = fnc->com_;
  out->data_ = NULL;
  out->size_ = 0;
  out->was_stored_ = false;
  out->type_ = RT_Undefined;

  if (fnc->type_ != RT_Function || com == NULL ||
      sizeof(jxcore::JXFunctionWrapper) != fnc->size_) {
    JX_SetUndefined(out);
    return false;
  }

  jxcore::JXFunctionWrapper *wrap = (jxcore::JXFunctionWrapper *)fnc->data_;

  bool done = false;
  bool ret;

  JS_HANDLE_VALUE res;
  RUN_IN_SCOPE({
    JS_HANDLE_VALUE *arr =
        (JS_HANDLE_VALUE *)malloc(sizeof(JS_HANDLE_VALUE) * argc);

    for (int i = 0; i < argc; i++) {
      if (params[i].type_ == RT_Undefined || params[i].type_ == RT_Null ||
          params[i].data_ == NULL) {
        arr[i] = params[i].type_ == RT_Undefined ? JS_UNDEFINED() : JS_NULL();
      } else {
        jxcore::JXValueWrapper *wrap =
            (jxcore::JXValueWrapper *)params[i].data_;
        arr[i] = JS_TYPE_TO_LOCAL_VALUE(wrap->value_);
      }
    }
    res = wrap->Call(argc, arr, &done);
    free(arr);

    if (!done) {
      JX_SetUndefined(out);
      ret = false;
    } else {
      ret = jxcore::JXEngine::ConvertToJXResult(com, res, out);
    }
  });

  return ret;
}

JXCORE_EXTERN(void)
JX_SetInt32(JXValue *value, const int32_t val) {
  UNWRAP_COM(value);
  UNWRAP_RESULT(value->data_);

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    value->data_ = (void *)wrap;
  } else if (!JS_IS_EMPTY(wrap->value_)) {
    JS_CLEAR_PERSISTENT(wrap->value_);
  }

  value->type_ = RT_Int32;
  value->size_ = sizeof(int32_t);

  RUN_IN_SCOPE({ JS_NEW_PERSISTENT_VALUE(wrap->value_, STD_TO_INTEGER(val)); });
}

JXCORE_EXTERN(void)
JX_SetDouble(JXValue *value, const double val) {
  UNWRAP_COM(value);
  UNWRAP_RESULT(value->data_);

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    value->data_ = (void *)wrap;
  } else if (!JS_IS_EMPTY(wrap->value_)) {
    JS_CLEAR_PERSISTENT(wrap->value_);
  }

  value->type_ = RT_Double;
  value->size_ = sizeof(double);

  RUN_IN_SCOPE({ JS_NEW_PERSISTENT_VALUE(wrap->value_, STD_TO_NUMBER(val)); });
}

JXCORE_EXTERN(void)
JX_SetBoolean(JXValue *value, const bool val) {
  UNWRAP_COM(value);
  UNWRAP_RESULT(value->data_);

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    value->data_ = (void *)wrap;
  } else if (!JS_IS_EMPTY(wrap->value_)) {
    JS_CLEAR_PERSISTENT(wrap->value_);
  }

  value->type_ = RT_Boolean;
  value->size_ = sizeof(bool);

  RUN_IN_SCOPE({ JS_NEW_PERSISTENT_VALUE(wrap->value_, STD_TO_BOOLEAN(val)); });
}

#define SET_STRING(type, ct)                                         \
  UNWRAP_COM(value);                                                 \
  UNWRAP_RESULT(value->data_);                                       \
                                                                     \
  if (wrap == 0) {                                                   \
    wrap = new jxcore::JXValueWrapper();                             \
    value->data_ = (void *)wrap;                                     \
  } else {                                                           \
    JS_CLEAR_PERSISTENT(wrap->value_);                               \
  }                                                                  \
                                                                     \
  value->type_ = type;                                               \
  value->size_ = length;                                             \
                                                                     \
  RUN_IN_SCOPE({                                                     \
    JS_NEW_PERSISTENT_VALUE(wrap->value_,                            \
                            NewString<ct>(com, val, &value->size_)); \
  })

template <class t>
JS_HANDLE_VALUE NewString_(node::commons *com, const t *val, size_t *str_len) {
  JS_ENTER_SCOPE_WITH(com->node_isolate);
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_STRING str_val = UTF8_TO_STRING(val);
  *str_len = JS_GET_STRING_LENGTH(str_val);

  return JS_LEAVE_SCOPE(str_val);
}

template <class t>
JS_HANDLE_VALUE NewString(node::commons *com, const t *val, size_t *length) {
  JS_ENTER_SCOPE_WITH(com->node_isolate);
  JS_DEFINE_STATE_MARKER(com);

  if (*length > 0) {
    JS_LOCAL_STRING str_val = UTF8_TO_STRING_WITH_LENGTH(val, *length);
    return JS_LEAVE_SCOPE(str_val);
  } else {
    JS_LOCAL_STRING str_val = JS_VALUE_TO_STRING(NewString_(com, val, length));
    return JS_LEAVE_SCOPE(str_val);
  }
}

JXCORE_EXTERN(void)
JX_SetString(JXValue *value, const char *val, const int32_t length) {
  SET_STRING(RT_String, char);
}

JXCORE_EXTERN(void)
JX_SetUCString(JXValue *value, const uint16_t *val, const int32_t _length) {
  int32_t length = _length;
#ifdef JS_ENGINE_MOZJS
  if (length == 0) {
    for (length = 0; *(val + length) != uint16_t(0); length++) {
      if (length + 2 == INT_MAX) {
        assert(0 && "Memory corruption!");
      }
    }
  }
#endif

  SET_STRING(RT_String, uint16_t);
}

JXCORE_EXTERN(void)
JX_SetJSON(JXValue *value, const char *val, const int32_t length) {
  UNWRAP_COM(value);
  UNWRAP_RESULT(value->data_);

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    value->data_ = (void *)wrap;
  } else if (!JS_IS_EMPTY(wrap->value_)) {
    JS_CLEAR_PERSISTENT(wrap->value_);
  }

  value->type_ = RT_Object;
  value->size_ = length == 0 ? strlen(val) : length;

  RUN_IN_SCOPE({
    JS_HANDLE_VALUE hval = jxcore::JX_Parse(com, val, length);
    JS_NEW_PERSISTENT_VALUE(wrap->value_, hval);
  });
}

JXCORE_EXTERN(void)
JX_SetError(JXValue *value, const char *val, const int32_t length) {
  SET_STRING(RT_Error, char);
}

JXCORE_EXTERN(void)
JX_SetBuffer(JXValue *value, const char *val, const int32_t length) {
  UNWRAP_COM(value);
  UNWRAP_RESULT(value->data_);

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    value->data_ = (void *)wrap;
  } else if (!JS_IS_EMPTY(wrap->value_)) {
    JS_CLEAR_PERSISTENT(wrap->value_);
  }

  value->type_ = RT_Buffer;
  value->size_ = (length == 0 && val != NULL) ? strlen(val) : length;

  RUN_IN_SCOPE({
    node::Buffer *buff = node::Buffer::New(val, length, com);
    JS_LOCAL_OBJECT hval = JS_OBJECT_FROM_PERSISTENT(buff->handle_);
    JS_NEW_PERSISTENT_VALUE(wrap->value_, hval);
  });
}

JXCORE_EXTERN(void)
JX_SetUndefined(JXValue *value) { value->type_ = RT_Undefined; }

JXCORE_EXTERN(void)
JX_SetNull(JXValue *value) { value->type_ = RT_Null; }

JXCORE_EXTERN(void)
JX_SetObject(JXValue *value_to, JXValue *value_from) {
  UNWRAP_COM(value_to);
  UNWRAP_RESULT(value_to->data_);

  assert(value_from->type_ == RT_Object && "value_from must be an Object");

  if (wrap == NULL) {
    wrap = new jxcore::JXValueWrapper();
    value_to->data_ = (void *)wrap;
  } else if (!JS_IS_EMPTY(wrap->value_)) {
    JS_CLEAR_PERSISTENT(wrap->value_);
  }

  value_to->type_ = RT_Object;
  value_to->size_ = 1;

  jxcore::JXValueWrapper *wrap_from =
      (jxcore::JXValueWrapper *)value_from->data_;
  RUN_IN_SCOPE({ JS_NEW_PERSISTENT_VALUE(wrap->value_, wrap_from->value_); });
  value_to->persistent_ = false;
}

JXCORE_EXTERN(bool)
JX_MakePersistent(JXValue *value) {
  assert(value->com_ != NULL && value->type_ != RT_Undefined &&
         value->type_ != RT_Null &&
         "Empty, Null or Undefined JS Value can not be persistent");

  bool pre = value->persistent_;
  value->persistent_ = true;

  return !pre;
}

JXCORE_EXTERN(bool)
JX_ClearPersistent(JXValue *value) {
  assert(value->com_ != NULL && value->type_ != RT_Undefined &&
         value->type_ != RT_Null &&
         "Empty, Null or Undefined JS Value can not be persistent");

  bool pre = value->persistent_;
  value->persistent_ = false;

  return pre;
}

JXCORE_EXTERN(bool)
JX_New(JXValue *value) {
  node::commons *com = node::commons::getInstance();

  // if returns false, that means the JXcore instance
  // wasn't initialized for the current thread
  if (com == NULL) return false;

  value->com_ = com;
  value->data_ = NULL;
  value->size_ = 0;
  value->persistent_ = false;
  value->type_ = RT_Undefined;
  value->was_stored_ = false;

  return true;
}

JXCORE_EXTERN(bool)
JX_CreateEmptyObject(JXValue *value) {
  node::commons *com = node::commons::getInstance();

  if (com == NULL) return false;
  jxcore::JXEngine *engine =
      jxcore::JXEngine::GetInstanceByThreadId(com->threadId);
  JS_DEFINE_STATE_MARKER(com);

  value->com_ = com;

  jxcore::JXValueWrapper *wrap = new jxcore::JXValueWrapper();
  RUN_IN_SCOPE(
      { JS_NEW_PERSISTENT_OBJECT(wrap->value_, JS_NEW_EMPTY_OBJECT()); });
  value->data_ = wrap;

  value->size_ = 1;
  value->persistent_ = false;
  value->type_ = RT_Object;
  value->was_stored_ = false;

  return true;
}

JXCORE_EXTERN(bool)
JX_CreateArrayObject(JXValue *value) {
  node::commons *com = node::commons::getInstance();

  if (com == NULL) return false;
  jxcore::JXEngine *engine =
      jxcore::JXEngine::GetInstanceByThreadId(com->threadId);
  JS_DEFINE_STATE_MARKER(com);

  value->com_ = com;

  jxcore::JXValueWrapper *wrap = new jxcore::JXValueWrapper();
  RUN_IN_SCOPE({ JS_NEW_PERSISTENT_OBJECT(wrap->value_, JS_NEW_ARRAY()); });
  value->data_ = wrap;

  value->size_ = 1;
  value->persistent_ = false;
  value->type_ = RT_Object;
  value->was_stored_ = false;

  return true;
}

JXCORE_EXTERN(void)
JX_SetNamedProperty(JXValue *object, const char *name, JXValue *prop) {
  UNWRAP_COM(object);
  UNWRAP_RESULT(object->data_);

  assert(object->type_ == RT_Object && "object must be an Object");

  jxcore::JXValueWrapper *wrap_prop = NULL;

  if (prop->type_ != RT_Undefined && prop->type_ != RT_Null &&
      prop->data_ != NULL) {
    wrap_prop = (jxcore::JXValueWrapper *)prop->data_;
  }

  RUN_IN_SCOPE({
    JS_LOCAL_VALUE val = wrap_prop != NULL
                             ? JS_TYPE_TO_LOCAL_VALUE(wrap_prop->value_)
                             : JS_NULL();
    JS_LOCAL_OBJECT obj = JS_OBJECT_FROM_PERSISTENT(wrap->value_);
    JS_NAME_SET(obj, JS_STRING_ID(name), val);
  });
}

JXCORE_EXTERN(void)
JX_SetIndexedProperty(JXValue *object, const unsigned index, JXValue *prop) {
  UNWRAP_COM(object);
  UNWRAP_RESULT(object->data_);

  assert(object->type_ == RT_Object && "object must be an Object");

  jxcore::JXValueWrapper *wrap_prop = NULL;

  if (prop->type_ != RT_Undefined && prop->type_ != RT_Null &&
      prop->data_ != NULL) {
    wrap_prop = (jxcore::JXValueWrapper *)prop->data_;
  }

  RUN_IN_SCOPE({
    JS_LOCAL_VALUE val = wrap_prop != NULL
                             ? JS_TYPE_TO_LOCAL_VALUE(wrap_prop->value_)
                             : JS_NULL();
    JS_LOCAL_OBJECT obj = JS_OBJECT_FROM_PERSISTENT(wrap->value_);
    JS_INDEX_SET(obj, index, val);
  });
}

JXCORE_EXTERN(void)
JX_GetNamedProperty(JXValue *object, const char *name, JXValue *out) {
  UNWRAP_COM(object);
  UNWRAP_RESULT(object->data_);

  assert(object->type_ == RT_Object && "object must be an Object");

  RUN_IN_SCOPE({
    JS_LOCAL_OBJECT obj = JS_OBJECT_FROM_PERSISTENT(wrap->value_);
    JS_LOCAL_VALUE sub_obj;
    if (JS_HAS_NAME(obj, JS_STRING_ID(name)))
      sub_obj = JS_GET_NAME(obj, JS_STRING_ID(name));
    else
      sub_obj = JS_UNDEFINED();

    out->data_ = NULL;
    out->size_ = 0;
    jxcore::JXEngine::ConvertToJXResult(com, sub_obj, out);
    out->com_ = com;
    out->was_stored_ = false;
    out->persistent_ = false;
  });
}

JXCORE_EXTERN(void)
JX_GetIndexedProperty(JXValue *object, const int index, JXValue *out) {
  UNWRAP_COM(object);
  UNWRAP_RESULT(object->data_);

  assert(object->type_ == RT_Object && "object must be an Object");

  RUN_IN_SCOPE({
    JS_LOCAL_OBJECT obj = JS_OBJECT_FROM_PERSISTENT(wrap->value_);
    JS_LOCAL_VALUE sub_obj = JS_GET_INDEX(obj, index);

    out->data_ = NULL;
    out->size_ = 0;
    jxcore::JXEngine::ConvertToJXResult(com, sub_obj, out);
    out->com_ = com;
    out->was_stored_ = false;
    out->persistent_ = false;
  });
}

JXCORE_EXTERN(int)
JX_GetThreadIdByValue(JXValue *value) {
  assert(value->com_ &&
         "You should not call JX_GetThreadIdByValue for an undefined JXValue "
         "variable");
  return ((node::commons *)value->com_)->threadId;
}

JXCORE_EXTERN(void)
JX_GetGlobalObject(JXValue *out) {
  node::commons *com = node::commons::getInstance();
  JS_DEFINE_STATE_MARKER(com);

  jxcore::JXEngine *engine =
      jxcore::JXEngine::GetInstanceByThreadId(com->threadId);

  RUN_IN_SCOPE({
    JS_LOCAL_OBJECT obj = JS_GET_GLOBAL();

    out->data_ = NULL;
    out->size_ = 0;
    jxcore::JXEngine::ConvertToJXResult(com, obj, out);
    out->com_ = com;
    out->was_stored_ = false;
    out->persistent_ = false;
  });
}

JXCORE_EXTERN(void)
JX_GetProcessObject(JXValue *out) {
  node::commons *com = node::commons::getInstance();
  JS_DEFINE_STATE_MARKER(com);

  jxcore::JXEngine *engine =
      jxcore::JXEngine::GetInstanceByThreadId(com->threadId);

  RUN_IN_SCOPE({
    JS_HANDLE_OBJECT obj = com->getProcess();

    out->data_ = NULL;
    out->size_ = 0;
    jxcore::JXEngine::ConvertToJXResult(com, obj, out);
    out->com_ = com;
    out->was_stored_ = false;
    out->persistent_ = false;
  });
}

JXCORE_EXTERN(void)
JX_WrapObject(JXValue *object, void *ptr) {
  UNWRAP_COM(object);
  UNWRAP_RESULT(object->data_);

  assert(object->type_ == RT_Object && "object must be an Object");

  RUN_IN_SCOPE({
    JS_LOCAL_OBJECT obj = JS_OBJECT_FROM_PERSISTENT(wrap->value_);

    JS_SET_POINTER_DATA(obj, ptr);
  });
}

JXCORE_EXTERN(void *)
JX_UnwrapObject(JXValue *object) {
  UNWRAP_COM(object);
  UNWRAP_RESULT(object->data_);

  assert(object->type_ == RT_Object && "object must be an Object");

  RUN_IN_SCOPE({
    JS_LOCAL_OBJECT obj = JS_OBJECT_FROM_PERSISTENT(wrap->value_);

    return JS_GET_POINTER_DATA(obj);
  });

  // make compiler happy
  return NULL;
}
