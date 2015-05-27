// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_MOZILLA_MOZTYPES_H_
#define SRC_JX_PROXY_MOZILLA_MOZTYPES_H_

#define JS_CORE_REFERENCE(x) x.GetRawValue()

// MOZ_T_ members are private macros, do not use them
#define MOZ_T_VALUE MozJS::Value
#define MOZ_T_OBJECT MozJS::Value
#define MOZ_T_STRING MozJS::String
#define MOZ_T_SCRIPT MozJS::Script
#define MOZ_T_OBJECT_TEMPLATE MozJS::Value
#define MOZ_T_ARRAY MozJS::Value
#define MOZ_T_BOOLEAN MozJS::Value
#define MOZ_T_INTEGER MozJS::Value
#define MOZ_T_NUMBER MozJS::Value
#define MOZ_T_FUNCTION MozJS::Value
#define MOZ_T_FUNCTION_TEMPLATE MozJS::Value
#define MOZ_T_UNDEFINED JSVAL_VOID
#define MOZ_T_CONTEXT JSContext *
#define MOZ_T_NULL JSVAL_NULL
#define MOZ_T_LOCAL(x) x
#define MOZ_T_HANDLE(x) x
#define MOZ_T_PERSISTENT(x) x

#define JS_UNDEFINED() MOZ_T_VALUE::Undefined(JS_GET_STATE_MARKER())
#define JS_NULL() MOZ_T_VALUE::Null(JS_GET_STATE_MARKER())

#define JS_NEW_ERROR_VALUE(x) ENGINE_NS::Exception::Error(x).GetErrorObject()
#define JS_NEW_EMPTY_OBJECT()                                       \
  MOZ_T_OBJECT(MOZ_T_OBJECT::NewEmptyObject(JS_GET_STATE_MARKER()), \
               JS_GET_STATE_MARKER())

#define JS_NEW_OBJECT_TEMPLATE() JS_NEW_EMPTY_OBJECT()
#define JS_NEW_OBJECT(x) JS_NewObject(JS_GET_STATE_MARKER(), x.cs, x.obj, 0L)

#define JS_NEW_ARRAY()                                      \
  MOZ_T_OBJECT(JS_NewArrayObject(JS_GET_STATE_MARKER(), 0), \
               JS_GET_STATE_MARKER())

#define JS_NEW_ARRAY_WITH_COUNT(n)                                       \
  MOZ_T_OBJECT(JS_NewArrayObject(JS_GET_STATE_MARKER(), n >= 0 ? n : 0), \
               JS_GET_STATE_MARKER())

#define JS_NEW_FUNCTION_TEMPLATE(x) MOZ_T_OBJECT(x, true, JS_GET_STATE_MARKER())

#define JS_NEW_FUNCTION_CALL_TEMPLATE(x) \
  MOZ_T_OBJECT(x, false, JS_GET_STATE_MARKER())

#define JS_NEW_EMPTY_FUNCTION_TEMPLATE() \
  MOZ_T_OBJECT::NewEmptyFunction(JS_GET_STATE_MARKER())

#define JS_NEW_CONTEXT(a, b) MozJS::Isolate::New(a, b)
#define JS_NEW_EMPTY_CONTEXT() MozJS::Isolate::New()
#define JS_NEW_PERSISTENT_CONTEXT(x) x
#define JS_NEW_LOCAL_CONTEXT(x) (x)->GetRaw()
#define JS_DISPOSE_PERSISTENT_CONTEXT(x)                               \
  do {                                                                 \
    node::commons *__com =                                             \
        node::commons::getInstanceByThreadId(JS_GetThreadId(x->ctx_)); \
    __com->free_context_list_.push_back(x->ctx_);                      \
    free(x);                                                           \
  } while (0)

#define JS_CAST_VALUE(x) (x)
#define JS_CAST_OBJECT(x) (x)
#define JS_CAST_ARRAY(x) (x)
#define JS_CAST_STRING(x) (x)
#define JS_CAST_FUNCTION(x) (x)
#define JS_CAST_FUNCTION_TEMPLATE(x) (x)

// for MozJS there is no difference among LOCAL, HANDLE, or PERSISTENT at the
// first place. However, you should still use them. For example
// JS_NEW_PERSISTENT_* calls will create a persistent JS native member

#define JS_HANDLE_CONTEXT MOZ_T_CONTEXT
#define JS_HANDLE_OBJECT MOZ_T_OBJECT
#define JS_HANDLE_OBJECT_REF MOZ_T_OBJECT &
#define JS_HANDLE_STRING MOZ_T_STRING
#define JS_HANDLE_VALUE MOZ_T_VALUE
#define JS_HANDLE_VALUE_REF MOZ_T_VALUE &
#define JS_HANDLE_OBJECT_TEMPLATE MOZ_T_HANDLE(MOZ_T_OBJECT_TEMPLATE)
#define JS_HANDLE_ARRAY MOZ_T_HANDLE(MOZ_T_ARRAY)
#define JS_HANDLE_INTEGER MOZ_T_HANDLE(MOZ_T_INTEGER)
#define JS_HANDLE_INTEGER_ MOZ_T_HANDLE(MOZ_T_INTEGER)()
#define JS_HANDLE_BOOLEAN MOZ_T_HANDLE(MOZ_T_BOOLEAN)
#define JS_HANDLE_FUNCTION MOZ_T_HANDLE(MOZ_T_FUNCTION)
#define JS_HANDLE_FUNCTION_REF JS_HANDLE_FUNCTION &
#define JS_HANDLE_FUNCTION_TEMPLATE MOZ_T_HANDLE(MOZ_T_FUNCTION_TEMPLATE)
#define JS_HANDLE_SCRIPT MOZ_T_HANDLE(MOZ_T_SCRIPT)

#define JS_LOCAL_OBJECT MOZ_T_LOCAL(MOZ_T_OBJECT)
#define JS_LOCAL_OBJECT_TEMPLATE MOZ_T_LOCAL(MOZ_T_OBJECT_TEMPLATE)
#define JS_LOCAL_STRING MOZ_T_LOCAL(MOZ_T_STRING)
#define JS_LOCAL_SCRIPT MOZ_T_LOCAL(MOZ_T_SCRIPT)
#define JS_LOCAL_VALUE MOZ_T_LOCAL(MOZ_T_VALUE)
#define JS_LOCAL_ARRAY MOZ_T_LOCAL(MOZ_T_ARRAY)
#define JS_LOCAL_INTEGER MOZ_T_LOCAL(MOZ_T_INTEGER)
#define JS_LOCAL_BOOLEAN MOZ_T_LOCAL(MOZ_T_BOOLEAN)
#define JS_LOCAL_FUNCTION MOZ_T_LOCAL(MOZ_T_FUNCTION)
#define JS_LOCAL_FUNCTION_TEMPLATE MOZ_T_LOCAL(MOZ_T_FUNCTION_TEMPLATE)
#define TO_LOCAL_FUNCTION(x) x
#define JS_LOCAL_CONTEXT MOZ_T_CONTEXT

#define JS_PERSISTENT_VALUE MOZ_T_PERSISTENT(MOZ_T_VALUE)
#define JS_PERSISTENT_VALUE_REF JS_PERSISTENT_VALUE &
#define JS_PERSISTENT_OBJECT MOZ_T_PERSISTENT(MOZ_T_OBJECT)
#define JS_PERSISTENT_OBJECT_TEMPLATE MOZ_T_PERSISTENT(MOZ_T_OBJECT)
#define JS_PERSISTENT_ARRAY MOZ_T_PERSISTENT(MOZ_T_OBJECT)
#define JS_PERSISTENT_STRING MOZ_T_PERSISTENT(MOZ_T_STRING)
#define JS_PERSISTENT_FUNCTION MOZ_T_PERSISTENT(MOZ_T_FUNCTION)

#define JS_PERSISTENT_FUNCTION_TEMPLATE \
  MOZ_T_PERSISTENT(MOZ_T_FUNCTION_TEMPLATE)

#define JS_PERSISTENT_CONTEXT MozJS::Isolate *
#define JS_PERSISTENT_SCRIPT MOZ_T_PERSISTENT(MOZ_T_SCRIPT)
#define JS_NEW_PERSISTENT_OBJECT(x) (x).RootCopy()
#define JS_NEW_PERSISTENT_OBJECT_TEMPLATE(x) (x).RootCopy()

#define JS_NEW_PERSISTENT_ARRAY()                           \
  MOZ_T_OBJECT(JS_NewArrayObject(JS_GET_STATE_MARKER(), 0), \
               JS_GET_STATE_MARKER()).RootCopy()

#define JS_NEW_EMPTY_PERSISTENT_OBJECT()                            \
  MOZ_T_OBJECT(MOZ_T_OBJECT::NewEmptyObject(JS_GET_STATE_MARKER()), \
               JS_GET_STATE_MARKER()).RootCopy()

#define JS_NEW_PERSISTENT_STRING(x) MOZ_T_STRING(x).RootCopy()
#define JS_NEW_PERSISTENT_FUNCTION(x) (x).RootCopy()
#define JS_NEW_PERSISTENT_FUNCTION_TEMPLATE(x) (x).RootCopy()
#define JS_NEW_PERSISTENT_SCRIPT(x) (x).RootCopy()
#define JS_NEW_PERSISTENT_VALUE(x) (x).RootCopy()

#define JS_CLEAR_PERSISTENT_(x) \
  if (x.IsRooted()) {           \
    x.RemoveRoot();             \
    x.Clear();                  \
  }

#define JS_CLEAR_PERSISTENT(x) JS_CLEAR_PERSISTENT_(x)

#define JS_TYPE_TO_LOCAL_VALUE(x) (x)
#define JS_TYPE_TO_LOCAL_OBJECT(x) (x)
#define JS_TYPE_TO_LOCAL_STRING(x) (x)
#define JS_TYPE_TO_LOCAL_FUNCTION(x) (x)
#define JS_TYPE_TO_LOCAL_FUNCTION_TEMPLATE(x) (x)
#define JS_TYPE_TO_LOCAL_CONTEXT(x) (x)->GetRaw()
#define JS_TYPE_TO_LOCAL_ARRAY(x) (x)

#define JS_GET_CONTEXT_GLOBAL(x) MozJS::Value(JS::CurrentGlobalOrNull(x), x)
#define JS_GET_THREAD_ID() com->threadId

#define JS_INDEX_SET(obj, index, value)                                        \
  do {                                                                         \
    JS::RootedValue __local_value(JS_GET_STATE_MARKER(), value.GetRawValue()); \
    obj.SetIndex(index, __local_value);                                        \
  } while (0)

#define JS_NAME_SET(obj, name, value)                                          \
  do {                                                                         \
    JS::RootedValue __local_value(JS_GET_STATE_MARKER(), value.GetRawValue()); \
    obj.SetProperty(name, __local_value);                                      \
  } while (0)

// JS_STRING_ID has better performance for SET, GET, HAS
// type of calls. If you are confusing, you can simply
// use STD_TO_STRING for every instance
#define JS_STRING_ID(x) x

#define JS_NAME_SET_HIDDEN(obj, name, value) \
  JS_NAME_SET(obj, name, value)  // TODO(obastemur) implement hidden

#define JS_NAME_DELETE(obj, name) (obj).DeleteProperty(name)
#define JS_CLASSNAME_SET(obj, name)  // do nothing for MozJS

#define JS_ACCESSOR_SET(obj, name, getter, setter)                  \
  do {                                                              \
    JS_LOCAL_STRING str_name = name;                                \
    JS_LOCAL_VALUE null_value = JS_NULL();                          \
    (obj).DefineGetterSetter(str_name, getter, setter, null_value); \
  } while (0)

#define JS_NAMEDPROPERTYHANDLER_SET(obj, getter, setter, a, b, c, d) \
  (obj)->SetNamedPropertyHandler(getter, setter, a, b, c, d)

#define JS_IS_EMPTY(x) (x).IsEmpty()
#define JS_IS_NULL_OR_UNDEFINED(x) ((x).IsNull() || (x).IsUndefined())
#define JS_IS_FUNCTION(x) (x).IsFunction()
#define JS_IS_NUMBER(x) (x).IsNumber()
#define JS_IS_BOOLEAN(x) (x).IsBoolean()
#define JS_IS_STRING(x) (x).IsString()
#define JS_IS_DATE(x) (x).IsDate()
#define JS_IS_INT32(x) (x).IsInt32()
#define JS_IS_UINT32(x) (x).IsUint32()
#define JS_IS_TRUE(x) (x).IsTrue()
#define JS_IS_FALSE(x) (x).IsFalse()
#define JS_IS_REGEXP(x) (x).IsRegExp()
#define JS_IS_NULL(x) (x).IsNull()
#define JS_IS_UNDEFINED(x) (x).IsUndefined()
#define JS_IS_OBJECT(x) (x).IsObject()
#define JS_IS_ARRAY(x) (x).IsArray()
#define JS_IS_NEARDEATH(x) (x).IsNearDeath()

#define INTEGER_TO_STD(x) (x).IntegerValue()
#define INT32_TO_STD(x) (x).IntegerValue()
#define UINT32_TO_STD(x) (x).IntegerValue()
#define STRING_TO_STD(x) *jxcore::JXString(x, JS_GET_STATE_MARKER())
#define BOOLEAN_TO_STD(x) (x).BooleanValue()
#define NUMBER_TO_STD(x) (x).NumberValue()

#define JS_HAS_NAME(x, y) (x).Has(y)
#define JS_GET_INDEX(x, y) (x).GetIndex(y)
#define JS_GET_NAME(from, name) (from).Get(name)
#define JS_GET_NAME_HIDDEN(x, y) (x).Get(y)
#define JS_GET_FUNCTION(x) (x)
#define JS_GET_CONSTRUCTOR(x) (x).GetConstructor()
#define JS_NEW_DEFAULT_INSTANCE(x) (x).NewInstance(0)

#define JS_VALUE_TO_OBJECT(x) x
#define JS_VALUE_TO_STRING(x) x.ToString()
#define JS_VALUE_TO_NUMBER(x) x
#define JS_VALUE_TO_INTEGER(x) x
#define JS_VALUE_TO_BOOLEAN(x) x
#define JS_STRING_TO_ERROR_VALUE(x) MozJS::Exception::Error(x).GetErrorObject()

#define UTF8_TO_STRING(str) \
  MOZ_T_STRING::FromUTF8(JS_GET_STATE_MARKER(), str, 0)
#define STD_TO_STRING(str) MOZ_T_STRING::FromSTD(JS_GET_STATE_MARKER(), str, 0)

#define STD_TO_STRING_WITH_LENGTH(str, l) \
  MOZ_T_STRING::FromSTD(JS_GET_STATE_MARKER(), str, l)

#define UTF8_TO_STRING_WITH_LENGTH(str, l) \
  MOZ_T_STRING::FromUTF8(JS_GET_STATE_MARKER(), str, l)

#define STD_TO_BOOLEAN(bl) MOZ_T_VALUE::FromBoolean(JS_GET_STATE_MARKER(), bl)
#define STD_TO_INTEGER(nt) MOZ_T_VALUE::FromInteger(JS_GET_STATE_MARKER(), nt)
#define STD_TO_UNSIGNED(nt) MOZ_T_VALUE::FromUnsigned(JS_GET_STATE_MARKER(), nt)
#define STD_TO_NUMBER(nt) MOZ_T_VALUE::FromDouble(JS_GET_STATE_MARKER(), nt)
#define JS_NEW_EMPTY_STRING() STD_TO_STRING("")

#define JS_OBJECT_FIELD_COUNT(obj) (obj).InternalFieldCount()
#define JS_SET_POINTER_DATA(host, data) (host).SetPrivate(data)
#define JS_GET_POINTER_DATA(host) (host).GetPointerFromInternalField()

#define JS_SET_INDEXED_EXTERNAL(host, b, c, d) \
  (host).SetIndexedPropertiesToExternalArrayData(b, c, d)

#define JS_GET_EXTERNAL_ARRAY_DATA(x) \
  (x).GetIndexedPropertiesExternalArrayData()

#define JS_GET_EXTERNAL_ARRAY_DATA_LENGTH(x) \
  (x).GetIndexedPropertiesExternalArrayDataLength()

#define JS_GET_EXTERNAL_ARRAY_DATA_TYPE(x) \
  (MozJS::ExternalArrayType)(x).GetIndexedPropertiesExternalArrayDataType()

#define JS_PREDEFINED_STRING(name) #name

#define JS_ADJUST_EXTERNAL_MEMORY(x)
#define JS_NEW_INSTANCE(a, b, c) (a).NewInstance(b, c)
#define JS_GET_STRING_LENGTH(a) (a).StringLength()
#define JS_GET_ARRAY_LENGTH(a) (a).ArrayLength()

#define JS_RETAINED_OBJECT_INFO node::RetainedBufferInfo

#define JS_TYPE_AS_FUNCTION(x) (x)
#define JS_TYPE_AS_STRING(x) (x)
#define JS_TYPE_AS_OBJECT(x) (x)
#define JS_TYPE_AS_ARRAY(x) (x)

#define JS_TRY_CATCH_TYPE MozJS::TryCatch
#define JS_TRY_CATCH(x) JS_TRY_CATCH_TYPE x(JS_GET_STATE_MARKER())

#define JS_COMPARE_BY_CONSTRUCTOR(obj, cons) \
  (obj).GetConstructor().StrictEquals(cons)

#define JS_STRICT_EQUALS(a,b) a.StrictEquals(b)

#define JS_HAS_INSTANCE(obj, to_check) (obj).HasInstance(to_check)
#define JS_METHOD_CALL(obj, method, argc, args) (obj).Call(method, argc, args)
#define JS_METHOD_CALL_NO_PARAM(obj, method) (obj).Call(method)
#define JS_LOCAL_MESSAGE JS_LOCAL_VALUE

#define JS_COMPILE_AND_RUN(a, b) \
  MOZ_T_OBJECT::CompileAndRun(JS_GET_STATE_MARKER(), a, b)

#define JS_SCRIPT_COMPILE(source, filename) \
  MozJS::Script::Compile(JS_GET_STATE_MARKER(), source, filename)

#define JS_SCRIPT_RUN(script) script.Run()
#define JS_NATIVE_RETURN_TYPE bool
#define JS_GET_ERROR_VALUE(x) x.GetErrorObject()

#define JS_CALL_PARAMS(name, count, ...) JS::Value name[count] = {__VA_ARGS__}

// MozJS only
/*
 *  {                                                                        \
      const JS::AutoCheckCannotGC gc;                                        \
      __data = JS_GetArrayBufferData(__arr_buf, gc);                         \
    }                                                                        \
 */
#define JS_BIND_NEW_ARRAY_BUFFER(obj, name, type, size, target, target_type) \
  do {                                                                       \
    JSObject *__arr_buf =                                                    \
        JS_NewArrayBuffer(JS_GET_STATE_MARKER(), sizeof(type) * size);       \
    JS::RootedObject __rt_obj_arr(JS_GET_STATE_MARKER(), __arr_buf);         \
    JSObject *__obj_arr = JS_New##name##ArrayWithBuffer(                     \
        JS_GET_STATE_MARKER(), __rt_obj_arr, 0, size);                       \
    uint8_t *__data;                                                         \
    __data = JS_GetArrayBufferData(__arr_buf);                               \
    target = (target_type *)__data;                                          \
    obj = MozJS::Value(__obj_arr, JS_GET_STATE_MARKER());                    \
  } while (0)

#endif  // SRC_JX_PROXY_MOZILLA_MOZTYPES_H_
