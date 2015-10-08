// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_V8_V8TYPES_H_
#define SRC_JX_PROXY_V8_V8TYPES_H_

#define JS_CORE_REFERENCE(x) x

// !! V8_T_ members are private macros
#define V8_T_VALUE v8::Value
#define V8_T_OBJECT v8::Object
#define V8_T_OBJECT_TEMPLATE v8::ObjectTemplate
#define V8_T_STRING v8::String
#define V8_T_SCRIPT v8::Script
#define V8_T_ARRAY v8::Array
#define V8_T_BOOLEAN v8::Boolean
#define V8_T_INTEGER v8::Integer
#define V8_T_NUMBER v8::Number
#define V8_T_FUNCTION v8::Function
#define V8_T_FUNCTION_TEMPLATE v8::FunctionTemplate
#define V8_T_UNDEFINED v8::Undefined
#define V8_T_CONTEXT v8::Context
#define V8_T_NULL v8::Null

#define JS_V8_ARGUMENT v8::Arguments

// #public
#define V8_T_LOCAL(x) v8::Local<x>
#define V8_T_HANDLE(x) v8::Handle<x>
#define V8_T_PERSISTENT(x) v8::Persistent<x>

#define JS_UNDEFINED() V8_T_LOCAL(V8_T_VALUE)::New(V8_T_UNDEFINED())
#define JS_NULL() V8_T_LOCAL(V8_T_VALUE)::New(V8_T_NULL(JS_GET_STATE_MARKER()))

#define JS_NEW_ERROR_VALUE(x) ENGINE_NS::Exception::Error(x)
#define JS_NEW_OBJECT_TEMPLATE() V8_T_OBJECT_TEMPLATE::New()

#define JS_NEW_EMPTY_OBJECT() V8_T_OBJECT::New()
#define JS_NEW_EMPTY_STRING() V8_T_STRING::Empty()
#define _JS_NEW_INTEGER(x) V8_T_INTEGER::New(x, JS_GET_STATE_MARKER())
#define _JS_NEW_UNSIGNED(x) V8_T_INTEGER::NewFromUnsigned(x, __contextORisolate)
#define _JS_NEW_NUMBER(x) V8_T_NUMBER::New(x)
#define _JS_NEW_BOOLEAN(x) V8_T_BOOLEAN::New(x)

#define JS_PERSISTENT_VALUE V8_T_PERSISTENT(V8_T_VALUE)
#define JS_PERSISTENT_VALUE_REF JS_PERSISTENT_VALUE
#define JS_PERSISTENT_OBJECT V8_T_PERSISTENT(V8_T_OBJECT)
#define JS_PERSISTENT_OBJECT_TEMPLATE V8_T_PERSISTENT(V8_T_OBJECT_TEMPLATE)
#define JS_PERSISTENT_ARRAY V8_T_PERSISTENT(V8_T_ARRAY)
#define JS_PERSISTENT_STRING V8_T_PERSISTENT(V8_T_STRING)
#define JS_PERSISTENT_FUNCTION V8_T_PERSISTENT(V8_T_FUNCTION)
#define JS_PERSISTENT_FUNCTION_TEMPLATE V8_T_PERSISTENT(V8_T_FUNCTION_TEMPLATE)
#define JS_PERSISTENT_CONTEXT V8_T_PERSISTENT(v8::Context)
#define JS_PERSISTENT_SCRIPT V8_T_PERSISTENT(V8_T_SCRIPT)

#define JS_HANDLE_CONTEXT V8_T_HANDLE(V8_T_CONTEXT)
#define JS_HANDLE_OBJECT V8_T_HANDLE(V8_T_OBJECT)
#define JS_HANDLE_OBJECT_REF JS_HANDLE_OBJECT
#define JS_HANDLE_OBJECT_TEMPLATE V8_T_HANDLE(V8_T_OBJECT_TEMPLATE)
#define JS_HANDLE_STRING V8_T_HANDLE(V8_T_STRING)
#define JS_HANDLE_STRING_CARRY JS_HANDLE_STRING
#define JS_HANDLE_VALUE V8_T_HANDLE(V8_T_VALUE)
#define JS_HANDLE_VALUE_REF JS_HANDLE_VALUE
#define JS_HANDLE_ARRAY V8_T_HANDLE(V8_T_ARRAY)
#define JS_HANDLE_INTEGER V8_T_HANDLE(V8_T_INTEGER)
#define JS_HANDLE_INTEGER_ V8_T_HANDLE(V8_T_INTEGER)()
#define JS_HANDLE_BOOLEAN V8_T_HANDLE(V8_T_BOOLEAN)
#define JS_HANDLE_FUNCTION V8_T_HANDLE(V8_T_FUNCTION)
#define JS_HANDLE_FUNCTION_REF JS_HANDLE_FUNCTION
#define JS_HANDLE_FUNCTION_TEMPLATE V8_T_HANDLE(V8_T_FUNCTION_TEMPLATE)
#define JS_HANDLE_SCRIPT V8_T_HANDLE(V8_T_SCRIPT)

#define JS_LOCAL_OBJECT V8_T_LOCAL(V8_T_OBJECT)
#define JS_LOCAL_OBJECT_TEMPLATE V8_T_LOCAL(V8_T_OBJECT_TEMPLATE)
#define JS_LOCAL_STRING V8_T_LOCAL(V8_T_STRING)
#define JS_LOCAL_SCRIPT V8_T_LOCAL(V8_T_SCRIPT)
#define JS_LOCAL_VALUE V8_T_LOCAL(V8_T_VALUE)
#define JS_LOCAL_ARRAY V8_T_LOCAL(V8_T_ARRAY)
#define JS_LOCAL_INTEGER V8_T_LOCAL(V8_T_INTEGER)
#define JS_LOCAL_BOOLEAN V8_T_LOCAL(V8_T_BOOLEAN)
#define JS_LOCAL_FUNCTION V8_T_LOCAL(V8_T_FUNCTION)
#define JS_LOCAL_FUNCTION_TEMPLATE V8_T_LOCAL(V8_T_FUNCTION_TEMPLATE)
#define TO_LOCAL_FUNCTION(x) JS_LOCAL_FUNCTION::New(x)
#define JS_LOCAL_CONTEXT V8_T_LOCAL(V8_T_CONTEXT)

#define JS_NEW_SCRIPT(a, b) ENGINE_NS::Script::New(a, b)

#define JS_NEW_ARRAY() V8_T_ARRAY::New(JS_GET_STATE_MARKER())
#define JS_NEW_ARRAY_WITH_COUNT(n) V8_T_ARRAY::New(JS_GET_STATE_MARKER(), n)
#define JS_NEW_FUNCTION_TEMPLATE(x) V8_T_FUNCTION_TEMPLATE::New(x)
#define JS_NEW_FUNCTION_CALL_TEMPLATE(x) V8_T_FUNCTION_TEMPLATE::New(x)
#define JS_NEW_EMPTY_FUNCTION_TEMPLATE() V8_T_FUNCTION_TEMPLATE::New()
#define JS_NEW_CONTEXT(x, a, b)   \
  v8::Persistent<v8::Context> x = \
      V8_T_CONTEXT::New(NULL, JS_NEW_OBJECT_TEMPLATE())
#define JS_NEW_EMPTY_CONTEXT() V8_T_CONTEXT::New()
#define JS_NEW_PERSISTENT_CONTEXT(y, x) y = v8::Persistent<v8::Context>::New(x)

#define JS_CAST_VALUE(x) JS_LOCAL_VALUE::Cast(x)
#define JS_CAST_OBJECT(x) JS_LOCAL_OBJECT::Cast(x)
#define JS_CAST_ARRAY(x) JS_LOCAL_ARRAY::Cast(x)
#define JS_CAST_STRING(x) JS_LOCAL_STRING::Cast(x)
#define JS_CAST_FUNCTION(x) JS_LOCAL_FUNCTION::Cast(x)
#define JS_CAST_FUNCTION_TEMPLATE(x) JS_LOCAL_FUNCTION_TEMPLATE::Cast(x)

#define JS_CLEAR_PERSISTENT_(x) \
  x.Dispose();                  \
  x.Clear()

#define JS_CLEAR_PERSISTENT(x) \
  if (!x.IsEmpty()) {          \
    JS_CLEAR_PERSISTENT_(x);   \
  }

#define JS_NEW_PERSISTENT_OBJECT(a, x) a = JS_PERSISTENT_OBJECT::New(x)
#define JS_NEW_PERSISTENT_OBJECT_TEMPLATE(x) \
  JS_PERSISTENT_OBJECT_TEMPLATE::New(x)
#define JS_NEW_PERSISTENT_ARRAY(y, x) y = JS_PERSISTENT_ARRAY::New(x)
#define JS_NEW_EMPTY_PERSISTENT_OBJECT(x) \
  JS_NEW_PERSISTENT_OBJECT(x, JS_NEW_EMPTY_OBJECT())
#define JS_NEW_PERSISTENT_STRING(a, b) a = JS_PERSISTENT_STRING::New(b)
#define JS_NEW_PERSISTENT_FUNCTION(a, b) a = JS_PERSISTENT_FUNCTION::New(b)
#define JS_NEW_PERSISTENT_FUNCTION_TEMPLATE(a, b) \
  a = JS_PERSISTENT_FUNCTION_TEMPLATE::New(b)
#define JS_NEW_PERSISTENT_SCRIPT(y, x) y = JS_PERSISTENT_SCRIPT::New(x)
#define JS_DISPOSE_PERSISTENT_CONTEXT(x) (x).Dispose()
#define JS_NEW_PERSISTENT_VALUE(a, x) a = JS_PERSISTENT_VALUE::New(x)
#define JS_NEW_EMPTY_PERSISTENT_FUNCTION_TEMPLATE(a) a = JS_PERSISTENT_FUNCTION_TEMPLATE()

#define JS_TYPE_TO_LOCAL_VALUE(x) JS_LOCAL_VALUE::New(x)
#define JS_TYPE_TO_LOCAL_OBJECT(x) JS_LOCAL_OBJECT::New(x)
#define JS_TYPE_TO_LOCAL_STRING(x) JS_LOCAL_STRING::New(x)
#define JS_TYPE_TO_LOCAL_FUNCTION(x) JS_LOCAL_FUNCTION::New(x)
#define JS_TYPE_TO_LOCAL_FUNCTION_TEMPLATE(x) JS_LOCAL_FUNCTION_TEMPLATE::New(x)
#define JS_TYPE_TO_LOCAL_CONTEXT(x) (*x)
#define JS_NEW_LOCAL_CONTEXT(x) v8::Local<v8::Context>::New(x)
#define JS_TYPE_TO_LOCAL_ARRAY(x) JS_LOCAL_ARRAY::New(x)
#define JS_TYPE_TO_LOCAL_SCRIPT(x) JS_LOCAL_SCRIPT::New(x)
#define JS_OBJECT_FROM_PERSISTENT(x) x->ToObject()

#define JS_GET_CONTEXT_GLOBAL(x) x->Global()
#define JS_GET_THREAD_ID() com->threadId

#define JS_STRING_ID(X) STD_TO_STRING(X)
#define JS_INDEX_SET(name, index, value) \
  (name)->Set(STD_TO_INTEGER(index), value)
#define JS_NAME_SET(obj, name, value) (obj)->Set(name, value)
#define JS_NAME_SET_HIDDEN(obj, name, value) (obj)->SetHiddenValue(name, value)
#define JS_NAME_DELETE(obj, name) (obj)->Delete(name)
#define JS_CLASSNAME_SET(obj, name) (obj)->SetClassName(name)
#define JS_ACCESSOR_SET(obj, name, getter, setter) \
  (obj)->SetAccessor(name, getter, setter)
#define JS_NAMEDPROPERTYHANDLER_SET(obj, getter, setter, a, b, c, d) \
  (obj)->SetNamedPropertyHandler(getter, setter, a, b, c, d)

#define JS_IS_EMPTY(x) (x).IsEmpty()
#define JS_IS_NULL_OR_UNDEFINED(x) ((x)->IsNull() || (x)->IsUndefined())
#define JS_IS_FUNCTION(x) (x)->IsFunction()
#define JS_IS_NUMBER(x) (x)->IsNumber()
#define JS_IS_BOOLEAN(x) (x)->IsBoolean()
#define JS_IS_STRING(x) (x)->IsString()
#define JS_IS_DATE(x) (x)->IsDate()
#define JS_IS_INT32(x) (x)->IsInt32()
#define JS_IS_UINT32(x) (x)->IsUint32()
#define JS_IS_TRUE(x) (x)->IsTrue()
#define JS_IS_FALSE(x) (x)->IsFalse()
#define JS_IS_REGEXP(x) (x)->IsRegExp()
#define JS_IS_NULL(x) (x)->IsNull()
#define JS_IS_UNDEFINED(x) (x)->IsUndefined()
#define JS_IS_OBJECT(x) (x)->IsObject()
#define JS_IS_ARRAY(x) (x)->IsArray()
#define JS_IS_NEARDEATH(x) (x)->IsNearDeath()

#define INTEGER_TO_STD(x) (x)->IntegerValue()
#define INT32_TO_STD(x) (x)->Int32Value()
#define UINT32_TO_STD(x) (x)->Uint32Value()
#define STRING_TO_STD(x) *jxcore::JXString(x)
#define BOOLEAN_TO_STD(x) (x)->BooleanValue()
#define NUMBER_TO_STD(x) (x)->NumberValue()

#define JS_HAS_NAME(x, y) (x)->Has(y)
#define JS_GET_INDEX(x, y) (x)->Get(y)
#define JS_GET_NAME(x, y) (x)->Get(y)
#define JS_GET_NAME_HIDDEN(x, y) (x)->GetHiddenValue(y)
#define JS_GET_FUNCTION(x) (x)->GetFunction()
#define JS_GET_CONSTRUCTOR(x) (x)->GetFunction()

#define JS_VALUE_TO_OBJECT(x) x->ToObject()
#define JS_VALUE_TO_STRING(x) x->ToString()
#define JS_VALUE_TO_NUMBER(x) x->ToNumber()
#define JS_VALUE_TO_INTEGER(x) x->ToInteger()
#define JS_VALUE_TO_BOOLEAN(x) x->ToBoolean()
#define JS_STRING_TO_ERROR_VALUE(x) v8::Exception::Error(x)

#define UTF8_TO_STRING(str) V8_T_STRING::New(JS_GET_STATE_MARKER(), str)
#define STD_TO_STRING(str) V8_T_STRING::New(JS_GET_STATE_MARKER(), str)
#define STD_TO_STRING_WITH_LENGTH(str, l) \
  V8_T_STRING::New(JS_GET_STATE_MARKER(), str, l)
#define UTF8_TO_STRING_WITH_LENGTH(str, l) \
  V8_T_STRING::New(JS_GET_STATE_MARKER(), str, l)
#define STD_TO_BOOLEAN(bl) v8::Local<v8::Boolean>::New(_JS_NEW_BOOLEAN(bl))
#define STD_TO_INTEGER(nt) _JS_NEW_INTEGER(nt)
#define STD_TO_UNSIGNED(nt) _JS_NEW_UNSIGNED(nt)
#define STD_TO_NUMBER(n) _JS_NEW_NUMBER(n)

#define JS_OBJECT_FIELD_COUNT(obj) obj->InternalFieldCount()
#define JS_SET_POINTER_DATA(host, data) host->SetPointerInInternalField(0, data)
#define JS_GET_POINTER_DATA(host) host->GetPointerFromInternalField(0)
#define JS_SET_INDEXED_EXTERNAL(host, b, c, d) \
  host->SetIndexedPropertiesToExternalArrayData(b, c, d)
#define JS_GET_EXTERNAL_ARRAY_DATA(x) \
  (x)->ToObject()->GetIndexedPropertiesExternalArrayData()
#define JS_GET_EXTERNAL_ARRAY_DATA_LENGTH(x) \
  (x)->ToObject()->GetIndexedPropertiesExternalArrayDataLength()
#define JS_GET_EXTERNAL_ARRAY_DATA_TYPE(x) \
  (x)->GetIndexedPropertiesExternalArrayDataType()

#define JS_PREDEFINED_STRING(name) JS_VALUE_TO_STRING(com->pstr_##name)

#define JS_REMOVE_EXTERNAL_MEMORY(data_, length_) \
  delete[] data_;                                 \
  v8::V8::AdjustAmountOfExternalAllocatedMemory(  \
      -static_cast<intptr_t>(sizeof(Buffer) + length_))

#define JS_ADD_EXTERNAL_MEMORY(data_, length_) \
  data_ = new char[length_];                   \
  v8::V8::AdjustAmountOfExternalAllocatedMemory(sizeof(Buffer) + length_)

#define JS_ADJUST_EXTERNAL_MEMORY(k) \
  v8::V8::AdjustAmountOfExternalAllocatedMemory(k)

#define JS_NEW_INSTANCE(a, b, c) (a)->NewInstance(b, c)
#define JS_NEW_DEFAULT_INSTANCE(x) (x)->NewInstance()
#define JS_GET_STRING_LENGTH(a) (a)->Length()
#define JS_GET_ARRAY_LENGTH(a) (a)->Length()

#define JS_RETAINED_OBJECT_INFO v8::RetainedObjectInfo

#define JS_TYPE_AS_FUNCTION(x) (x).As<v8::Function>()
#define JS_TYPE_AS_STRING(x) (x).As<v8::String>()
#define JS_TYPE_AS_OBJECT(x) (x).As<v8::Object>()
#define JS_TYPE_AS_ARRAY(x) (x).As<v8::Array>()

#define JS_TRY_CATCH(x) v8::TryCatch x
#define JS_TRY_CATCH_TYPE v8::TryCatch

#define JS_COMPARE_BY_CONSTRUCTOR(obj, cons) \
  obj->GetConstructor()->StrictEquals(cons)
#define JS_HAS_INSTANCE(obj, to_check) obj->HasInstance(to_check)

#define JS_STRICT_EQUALS(a, b) a->StrictEquals(b)

#define JS_METHOD_CALL(obj, method, argc, args) obj->Call(method, argc, args)
#define JS_METHOD_CALL_NO_PARAM(obj, method) obj->Call(method, 0, NULL)

#define JS_LOCAL_MESSAGE v8::Handle<v8::Message>

#define JS_COMPILE_AND_RUN(a, b) v8::Script::Compile(a, b)->Run()
#define JS_SCRIPT_COMPILE(source, filename) \
  v8::Script::Compile(source, filename)
#define JS_SCRIPT_RUN(script) script->Run()

#define JS_NATIVE_RETURN_TYPE JS_HANDLE_VALUE

#define JS_GET_ERROR_VALUE(x) x

#define JS_CALL_PARAMS(name, count, ...) \
  JS_LOCAL_VALUE name[count] = {__VA_ARGS__}

#endif  // SRC_JX_PROXY_V8_V8TYPES_H_
