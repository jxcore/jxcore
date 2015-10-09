// Copyright Microsoft. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and / or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#include "v8.h"
#include "v8chakra.h"
#include "jsrtutils.h"

namespace v8 {

using namespace jsrt;

class ExternalType {
};

struct ObjectTemplateData : public TemplateData {
 public:
  Persistent<String> className;
  NamedPropertyGetterCallback namedPropertyGetter;
  NamedPropertySetterCallback namedPropertySetter;
  NamedPropertyQueryCallback namedPropertyQuery;
  NamedPropertyDeleterCallback namedPropertyDeleter;
  NamedPropertyEnumeratorCallback namedPropertyEnumerator;
  Persistent<Value> namedPropertyInterceptorData;
  IndexedPropertyGetterCallback indexedPropertyGetter;
  IndexedPropertySetterCallback indexedPropertySetter;
  IndexedPropertyQueryCallback indexedPropertyQuery;
  IndexedPropertyDeleterCallback indexedPropertyDeleter;
  IndexedPropertyEnumeratorCallback indexedPropertyEnumerator;
  Persistent<Value> indexedPropertyInterceptorData;
  int internalFieldCount;
  bool supportsOverrideToString;

  ObjectTemplateData()
      : namedPropertyGetter(nullptr),
        namedPropertySetter(nullptr),
        namedPropertyQuery(nullptr),
        namedPropertyDeleter(nullptr),
        namedPropertyEnumerator(nullptr),
        indexedPropertyGetter(nullptr),
        indexedPropertySetter(nullptr),
        indexedPropertyQuery(nullptr),
        indexedPropertyDeleter(nullptr),
        indexedPropertyEnumerator(nullptr),
        className(),
        namedPropertyInterceptorData(),
        indexedPropertyInterceptorData(),
        internalFieldCount(0),
        supportsOverrideToString(false) {
    HandleScope scope;
    properties = Persistent<Object>(Object::New());
  }

  bool AreInterceptorsRequired() {
    return namedPropertyDeleter != nullptr ||
      namedPropertyEnumerator != nullptr ||
      namedPropertyGetter != nullptr ||
      namedPropertyQuery != nullptr ||
      namedPropertySetter != nullptr ||
      indexedPropertyDeleter != nullptr ||
      indexedPropertyEnumerator != nullptr ||
      indexedPropertyGetter != nullptr ||
      indexedPropertyQuery != nullptr ||
      indexedPropertySetter != nullptr;
  }

  static void CALLBACK FinalizeCallback(_In_opt_ void *data) {
    if (data != nullptr) {
      ObjectTemplateData* templateData =
        reinterpret_cast<ObjectTemplateData*>(data);
      templateData->className.Dispose();
      templateData->properties.Dispose();
      templateData->namedPropertyInterceptorData.Dispose();
      templateData->indexedPropertyInterceptorData.Dispose();
      delete templateData;
    }
  }
};

struct ObjectEnumerationData {
 public:
  Persistent<Array> array;
  int length;
  int index;

  explicit ObjectEnumerationData(Handle<Array> array)
      : array(array),
        index(-1),
        length(array->Length()) {
  }
};

ObjectData::ObjectData(ObjectTemplate* objectTemplate,
                       ObjectTemplateData *templateData)
    : objectTemplate(objectTemplate),
      namedPropertyGetter(templateData->namedPropertyGetter),
      namedPropertySetter(templateData->namedPropertySetter),
      namedPropertyQuery(templateData->namedPropertyQuery),
      namedPropertyDeleter(templateData->namedPropertyDeleter),
      namedPropertyEnumerator(templateData->namedPropertyEnumerator),
      namedPropertyInterceptorData(templateData->namedPropertyInterceptorData),
      indexedPropertyGetter(templateData->indexedPropertyGetter),
      indexedPropertySetter(templateData->indexedPropertySetter),
      indexedPropertyQuery(templateData->indexedPropertyQuery),
      indexedPropertyDeleter(templateData->indexedPropertyDeleter),
      indexedPropertyEnumerator(templateData->indexedPropertyEnumerator),
      indexedPropertyInterceptorData(
        templateData->indexedPropertyInterceptorData),
      internalFieldCount(templateData->internalFieldCount) {
  if (internalFieldCount > 0) {
    internalFields = new void *[internalFieldCount];
  }
}

// Callbacks used with proxies:
JsValueRef CALLBACK GetCallback(_In_ JsValueRef callee,
                                _In_ bool isConstructCall,
                                _In_ JsValueRef *arguments,
                                _In_ unsigned short argumentCount,
                                _In_opt_ void *callbackState) {
  void* externalData;
  JsValueRef result;

  JsValueRef object = arguments[1];
  JsValueRef prop = arguments[2];

  // intercept "__getSelf__" call from Object::InternalFieldHelper
  {
    JsValueType valueType;
    if (JsGetValueType(prop, &valueType) == JsNoError &&
        valueType == JsValueType::JsSymbol) {
      JsPropertyIdRef selfSymbolPropertyIdRef =
        jsrt::IsolateShim::GetCurrent()->GetSelfSymbolPropertyIdRef();
      JsPropertyIdRef idRef;
      if (JsGetPropertyIdFromSymbol(prop, &idRef) == JsNoError &&
          selfSymbolPropertyIdRef == idRef) {
        return object;  // return proxy target
      }
    }
  }

  if (JsGetExternalData(object, &externalData) != JsNoError) {
    return GetUndefined();
  }

  ObjectData *objectData = reinterpret_cast<ObjectData*>(externalData);

  bool isPropIntType;
  unsigned int index;
  if (TryParseUInt32(prop, &isPropIntType, &index) != JsNoError) {
    return GetUndefined();
  }

  if (isPropIntType) {
    if (objectData->indexedPropertyGetter) {
      // indexed array properties were set
      PropertyCallbackInfo<Value> info(
        *objectData->indexedPropertyInterceptorData,
        reinterpret_cast<Object*>(object),
        /*holder*/reinterpret_cast<Object*>(object));
      objectData->indexedPropertyGetter(index, info);
      return reinterpret_cast<JsValueRef>(info.GetReturnValue().Get());
    } else {
      if (GetIndexedProperty(object, index, &result) != JsNoError) {
        return GetUndefined();
      }

      return result;
    }
  } else {
    if (objectData->namedPropertyGetter) {
      PropertyCallbackInfo<Value> info(
        *objectData->namedPropertyInterceptorData,
        reinterpret_cast<Object*>(object),
        /*holder*/reinterpret_cast<Object*>(object));
      objectData->namedPropertyGetter(reinterpret_cast<String*>(prop), info);
      return reinterpret_cast<JsValueRef>(info.GetReturnValue().Get());
    } else {
      // use default JS behavior, on the prototype..
      if (jsrt::GetProperty(object, prop, &result) != JsNoError) {
        return GetUndefined();
      }
      return result;
    }
  }
}

JsValueRef CALLBACK SetCallback(_In_ JsValueRef callee,
                                _In_ bool isConstructCall,
                                _In_ JsValueRef *arguments,
                                _In_ unsigned short argumentCount,
                                _In_opt_ void *callbackState) {
  void* externalData;

  JsValueRef object = arguments[1];
  JsValueRef prop = arguments[2];
  JsValueRef value = arguments[3];

  if (JsGetExternalData(object, &externalData) != JsNoError) {
    return GetFalse();
  }

  ObjectData *objectData = reinterpret_cast<ObjectData*>(externalData);

  bool isPropIntType;
  unsigned int index;
  if (TryParseUInt32(prop, &isPropIntType, &index) != JsNoError) {
    return GetFalse();
  }

  if (isPropIntType) {
    if (objectData->indexedPropertySetter) {
      PropertyCallbackInfo<Value> info(
        *objectData->indexedPropertyInterceptorData,
        reinterpret_cast<Object*>(object),
        /*holder*/reinterpret_cast<Object*>(object));
      objectData->indexedPropertySetter(
        index, reinterpret_cast<Value*>(value), info);
      return reinterpret_cast<JsValueRef>(info.GetReturnValue().Get());
    } else {  // use default JS behavior
      if (jsrt::SetIndexedProperty(object, index, value) != JsNoError) {
        return GetFalse();
      }

      return GetTrue();
    }
  } else {
    if (objectData->namedPropertySetter) {
      PropertyCallbackInfo<Value> info(
        *objectData->namedPropertyInterceptorData,
        reinterpret_cast<Object*>(object),
        /*holder*/reinterpret_cast<Object*>(object));
      objectData->namedPropertySetter(
        reinterpret_cast<String*>(prop), reinterpret_cast<Value*>(value), info);
      return reinterpret_cast<JsValueRef>(info.GetReturnValue().Get());
    } else {  // use default JS behavior
      if (jsrt::SetProperty(object, prop, value) != JsNoError) {
        return GetFalse();
      }

      return GetTrue();
    }
  }
}

JsValueRef CALLBACK DeletePropertyCallback(_In_ JsValueRef callee,
                                           _In_ bool isConstructCall,
                                           _In_ JsValueRef *arguments,
                                           _In_ unsigned short argumentCount,
                                           _In_opt_ void *callbackState) {
  void* externalData;

  JsValueRef object = arguments[1];
  JsValueRef prop = arguments[2];

  JsValueRef result;

  if (JsGetExternalData(object, &externalData) != JsNoError) {
    return GetFalse();
  }

  ObjectData *objectData = reinterpret_cast<ObjectData*>(externalData);

  bool isPropIntType;
  unsigned int index;
  if (TryParseUInt32(prop, &isPropIntType, &index) != JsNoError) {
    return GetFalse();
  }

  if (isPropIntType) {
    if (objectData->indexedPropertyDeleter != nullptr) {
      PropertyCallbackInfo<Boolean> info(
        *objectData->indexedPropertyInterceptorData,
        reinterpret_cast<Object*>(object),
        /*holder*/reinterpret_cast<Object*>(object));
      objectData->indexedPropertyDeleter(index, info);
      return reinterpret_cast<JsValueRef>(info.GetReturnValue().Get());
    } else {
      if (DeleteIndexedProperty(object, index) != JsNoError) {
        return GetFalse();
      }

      return GetTrue();
    }
  } else {
    if (objectData->namedPropertyDeleter != nullptr) {
      PropertyCallbackInfo<Boolean> info(
        *objectData->namedPropertyInterceptorData,
        reinterpret_cast<Object*>(object),
        /*holder*/reinterpret_cast<Object*>(object));
      objectData->namedPropertyDeleter(reinterpret_cast<String*>(prop), info);
      return reinterpret_cast<JsValueRef>(info.GetReturnValue().Get());
    } else {  // use default JS behavior
      if (jsrt::DeleteProperty(object, prop, &result) != JsNoError) {
        return GetFalse();
      }

      return result;
    }
  }
}

JsValueRef HasPropertyHandler(_In_ JsValueRef *arguments,
                              _In_ unsigned short argumentCount,
                              _In_ bool checkInPrototype) {
  // algorithm is as follows:
  // 1. try to get the property descriptor - if it's not zero - return true
  // 2. list all of the properties using the enumerator, check if it's there
  // 3. call the target has method

  void* externalData;

  JsValueRef object = arguments[1];
  JsValueRef prop = arguments[2];

  if (JsGetExternalData(object, &externalData) != JsNoError) {
    return GetFalse();
  }

  ObjectData *objectData = reinterpret_cast<ObjectData*>(externalData);

  bool isPropIntType;
  unsigned int index;
  if (TryParseUInt32(prop, &isPropIntType, &index) != JsNoError) {
    return GetFalse();
  }

  if (isPropIntType) {
    if (objectData->indexedPropertyQuery != nullptr) {
      HandleScope scope;
      PropertyCallbackInfo<Integer> info(
        *objectData->indexedPropertyInterceptorData,
        reinterpret_cast<Object*>(object),
        /*holder*/reinterpret_cast<Object*>(object));
      objectData->indexedPropertyQuery(index, info);
      JsValueRef queryResult =
        reinterpret_cast<JsValueRef>(info.GetReturnValue().Get());

      bool isQueryResultZero;
      if (IsZero(queryResult, &isQueryResultZero) != JsNoError) {
        return GetFalse();
      }

      if (!isQueryResultZero) {
        return GetTrue();
      }
    }

    if (objectData->indexedPropertyEnumerator != nullptr) {
      PropertyCallbackInfo<Array> info(
        *objectData->indexedPropertyInterceptorData,
        reinterpret_cast<Object*>(object),
        /*holder*/reinterpret_cast<Object*>(object));
      objectData->indexedPropertyEnumerator(info);
      JsValueRef indexedProperties =
        reinterpret_cast<JsValueRef>(info.GetReturnValue().Get());

      bool valueInIndexedProps;
      if (jsrt::IsValueInArray(indexedProperties,
                               prop, &valueInIndexedProps) != JsNoError) {
        return GetFalse();
      }

      if (valueInIndexedProps) {
        return GetTrue();
      }
    }

    if (checkInPrototype) {
      bool hasProperty;
      if (jsrt::HasIndexedProperty(object, index, &hasProperty) != JsNoError) {
        return GetFalse();
      }


      return hasProperty ? GetTrue() : GetFalse();
    } else {
      JsValueRef result;
      jsrt::HasOwnProperty(object, prop, &result);
      return result;
    }
  } else {  // named property...
    if (objectData->namedPropertyQuery != nullptr) {
      HandleScope scope;
      PropertyCallbackInfo<Integer> info(
        *objectData->namedPropertyInterceptorData,
        reinterpret_cast<Object*>(object),
        /*holder*/reinterpret_cast<Object*>(object));
      objectData->namedPropertyQuery(reinterpret_cast<String*>(prop), info);
      JsValueRef queryResult =
        reinterpret_cast<JsValueRef>(info.GetReturnValue().Get());

      bool isQueryResultZero;
      if (IsZero(queryResult, &isQueryResultZero) != JsNoError) {
        return GetFalse();
      }

      if (!isQueryResultZero) {
        return GetTrue();
      }
    }

    if (objectData->namedPropertyEnumerator != nullptr) {
      PropertyCallbackInfo<Array> info(
        *objectData->namedPropertyInterceptorData,
        reinterpret_cast<Object*>(object),
        /*holder*/reinterpret_cast<Object*>(object));
      objectData->namedPropertyEnumerator(info);
      JsValueRef namedProperties =
        reinterpret_cast<JsValueRef>(info.GetReturnValue().Get());

      bool valueInNamedProps;
      if (jsrt::IsCaseInsensitiveStringValueInArray(namedProperties,
                                            prop,
                                            &valueInNamedProps) != JsNoError) {
        return GetFalse();
      }

      if (valueInNamedProps) {
        return GetTrue();
      }
    }

    if (checkInPrototype) {
      bool hasProperty;
      if (jsrt::HasProperty(object, prop, &hasProperty) != JsNoError) {
        return GetFalse();
      }

      return hasProperty ? GetTrue() : GetFalse();
    } else {
      JsValueRef result;
      if (jsrt::HasOwnProperty(object, prop, &result) != JsNoError) {
          return GetFalse();
      }
      return result;
    }
  }
}

JsValueRef CALLBACK HasCallback(_In_ JsValueRef callee,
                                _In_ bool isConstructCall,
                                _In_ JsValueRef *arguments,
                                _In_ unsigned short argumentCount,
                                _In_opt_ void *callbackState) {
  return HasPropertyHandler(arguments, argumentCount, false);
}

JsValueRef GetPropertiesHandler(_In_ JsValueRef* arguments,
                                _In_ unsigned int argumentsCount,
                                bool getFromPrototype) {
  HandleScope scope;
  void* externalData;

  JsValueRef object = arguments[1];

  if (JsGetExternalData(object, &externalData) != JsNoError) {
    return GetUndefined();
  }

  ObjectData *objectData = reinterpret_cast<ObjectData*>(externalData);
  JsValueRef indexedProperties;
  if (objectData->indexedPropertyEnumerator != nullptr) {
    PropertyCallbackInfo<Array> info(
      *objectData->indexedPropertyInterceptorData,
      reinterpret_cast<Object*>(object),
      /*holder*/reinterpret_cast<Object*>(object));
    objectData->indexedPropertyEnumerator(info);
    indexedProperties =
      reinterpret_cast<JsValueRef>(info.GetReturnValue().Get());
  } else if (getFromPrototype) {  // get indexed properties from object
    if (GetEnumerableIndexedProperties(object,
                                       &indexedProperties) != JsNoError) {
      return GetUndefined();
    }
  } else {
    if (GetIndexedOwnKeys(object, &indexedProperties) != JsNoError) {
      return GetUndefined();
    }
  }

  JsValueRef namedProperties;
  if (objectData->namedPropertyEnumerator != nullptr) {
    PropertyCallbackInfo<Array> info(
      *objectData->namedPropertyInterceptorData,
      reinterpret_cast<Object*>(object),
      /*holder*/reinterpret_cast<Object*>(object));
    objectData->namedPropertyEnumerator(info);
    namedProperties = reinterpret_cast<JsValueRef>(info.GetReturnValue().Get());
  } else if (getFromPrototype) {  // get named properties from object
    if (GetEnumerableNamedProperties(object, &namedProperties) != JsNoError) {
      return GetUndefined();
    }
  } else {
    if (GetNamedOwnKeys(object, &namedProperties) != JsNoError) {
      return GetUndefined();
    }
  }

  JsValueRef conatenatedArray;
  if (ConcatArray(indexedProperties,
                  namedProperties, &conatenatedArray) != JsNoError) {
    return GetUndefined();
  }

  return conatenatedArray;
}

JsValueRef GetPropertiesEnumeratorHandler(
    _In_ JsValueRef* arguments, _In_ unsigned int argumentsCount) {
  JsValueRef properties =
    GetPropertiesHandler(arguments, argumentsCount, /*getFromPrototype*/true);

  JsValueRef result;
  if (CreateEnumerationIterator(properties, &result) != JsNoError) {
    return GetUndefined();
  }

  return result;
}

JsValueRef CALLBACK EnumerateCallback(_In_ JsValueRef callee,
                                      _In_ bool isConstructCall,
                                      _In_ JsValueRef *arguments,
                                      _In_ unsigned short argumentCount,
                                      _In_opt_ void *callbackState) {
  return GetPropertiesEnumeratorHandler(arguments, argumentCount);
}

JsValueRef CALLBACK OwnKeysCallback(_In_ JsValueRef callee,
                                    _In_ bool isConstructCall,
                                    _In_ JsValueRef *arguments,
                                    _In_ unsigned short argumentCount,
                                    _In_opt_ void *callbackState) {
  return GetPropertiesHandler(arguments, argumentCount, false);
}

JsValueRef CALLBACK GetOwnPropertyDescriptorCallback(
    _In_ JsValueRef callee,
    _In_ bool isConstructCall,
    _In_ JsValueRef *arguments,
    _In_ unsigned short argumentCount,
    _In_opt_ void *callbackState) {
  // algorithm is as follows:
  // For indexed and named properties:
  //   1. If there are no getters & property query callbacks - call the objects
  //   method 2. If there is a query getter - use it, also call the getter on
  //   the proxy itself in order to have the interception work 3. if the value
  //   is null and query result is zero - return undefined, else - return the
  //   property descriptor object

  void* externalData;
  JsValueRef object = arguments[1];
  JsValueRef prop = arguments[2];
  JsValueRef descriptor;

  if (JsGetExternalData(object, &externalData) != JsNoError) {
    return GetUndefined();
  }

  ObjectData *objectData = reinterpret_cast<ObjectData*>(externalData);

  bool isPropIntType;
  unsigned int index;
  if (TryParseUInt32(prop, &isPropIntType, &index) != JsNoError) {
    return GetUndefined();
  }

  if (isPropIntType) {
    if (objectData->indexedPropertyQuery == nullptr &&
        objectData->indexedPropertyGetter == nullptr) {
      // default case - no interceptors are defined
      if (GetOwnPropertyDescriptor(object, prop, &descriptor) != JsNoError) {
        return GetUndefined();
      }
      return descriptor;
    }

    int queryResultInt = 0;

    if (objectData->indexedPropertyQuery != nullptr) {
      HandleScope scope;
      PropertyCallbackInfo<Integer> info(
        *objectData->indexedPropertyInterceptorData,
        reinterpret_cast<Object*>(object),
        /*holder*/reinterpret_cast<Object*>(object));
      objectData->indexedPropertyQuery(index, info);
      JsValueRef queryResult =
        reinterpret_cast<JsValueRef>(info.GetReturnValue().Get());

      double queryResultDouble;
      if (JsNumberToDouble(queryResult, &queryResultDouble) != JsNoError) {
        return GetUndefined();
      }
      queryResultInt = static_cast<int>(queryResultDouble);
    }

    JsValueRef value;
    if (objectData->indexedPropertyGetter != nullptr) {
      PropertyCallbackInfo<Value> info(
        *objectData->indexedPropertyInterceptorData,
        reinterpret_cast<Object*>(object),
        /*holder*/reinterpret_cast<Object*>(object));
      objectData->indexedPropertyGetter(index, info);
      value = reinterpret_cast<JsValueRef>(info.GetReturnValue().Get());
    } else {
      if (GetIndexedProperty(object, index, &value) != JsNoError) {
        return GetUndefined();
      }
    }

    bool isUndefined;
    if (IsUndefined(value, &isUndefined) != JsNoError) {
      return GetUndefined();
    }

    // no value & no property descriptor - we treat the property descriptor as
    // undefined
    if (isUndefined && queryResultInt == 0) {
      return GetUndefined();
    }

    v8::PropertyAttribute attributes =
      static_cast<v8::PropertyAttribute>(queryResultInt);
    if (CreatePropertyDescriptor(attributes, value,
                                 JS_INVALID_REFERENCE, JS_INVALID_REFERENCE,
                                 &descriptor) != JsNoError) {
      return GetUndefined();
    }

    return descriptor;

  } else {  // named property...
    // first we go for the default case - no getter and no query:
    if (objectData->namedPropertyQuery == nullptr &&
        objectData->namedPropertyGetter == nullptr) {
      if (GetOwnPropertyDescriptor(object, prop, &descriptor) != JsNoError) {
        return GetUndefined();
      }

      return descriptor;
    }

    // query the property descriptor if there is such, and then get the value
    // from the proxy in order to go through the interceptor
    int queryResultInt = 0;
    if (objectData->namedPropertyQuery != nullptr) {
      HandleScope scope;
      PropertyCallbackInfo<Integer> info(
        *objectData->namedPropertyInterceptorData,
        reinterpret_cast<Object*>(object),
        /*holder*/reinterpret_cast<Object*>(object));
      objectData->namedPropertyQuery(reinterpret_cast<String*>(prop), info);
      JsValueRef queryResult =
        reinterpret_cast<JsValueRef>(info.GetReturnValue().Get());

      double queryResultDouble;
      if (JsNumberToDouble(queryResult, &queryResultDouble) != JsNoError) {
        return GetUndefined();
      }

      queryResultInt = static_cast<int>(queryResultDouble);
    }

    JsValueRef value;

    if (objectData->namedPropertyGetter != nullptr) {
      PropertyCallbackInfo<Value> info(
        *objectData->namedPropertyInterceptorData,
        reinterpret_cast<Object*>(object),
        /*holder*/reinterpret_cast<Object*>(object));
      objectData->namedPropertyGetter(reinterpret_cast<String*>(prop), info);
      value = reinterpret_cast<JsValueRef>(info.GetReturnValue().Get());
    } else {
      if (GetProperty(object, prop, &value) != JsNoError) {
        return GetUndefined();
      }
    }

    bool isUndefined;
    if (IsUndefined(value, &isUndefined) != JsNoError) {
      return GetUndefined();
    }

    // no value & no property descriptor - we treat the property descriptor as
    // undefined
    if (isUndefined && queryResultInt == 0) {
      return GetUndefined();
    }

    v8::PropertyAttribute attributes =
      static_cast<v8::PropertyAttribute>(queryResultInt);
    if (CreatePropertyDescriptor(attributes, value,
                                 JS_INVALID_REFERENCE, JS_INVALID_REFERENCE,
                                 &descriptor) != JsNoError) {
      return GetUndefined();
    }

    return descriptor;
  }
}

void CALLBACK FinalizeCallback(void *data) {
  if (data != nullptr) {
    ObjectData* objectData = reinterpret_cast<ObjectData*>(data);
    if (objectData->internalFieldCount > 0) {
      delete objectData->internalFields;
    }
    objectData->namedPropertyInterceptorData.Dispose();
    objectData->indexedPropertyInterceptorData.Dispose();

    IsolateShim::GetCurrent()->UnregisterJsValueRefContextShim(
      objectData->objectInstance);
    delete objectData;
  }
}

Local<ObjectTemplate> ObjectTemplate::New(Isolate* isolate) {
  JsValueRef objectTemplateRef;
  ObjectTemplateData* templateData = new ObjectTemplateData();

  JsErrorCode error = JsCreateExternalObject(
    templateData, ObjectTemplateData::FinalizeCallback, &objectTemplateRef);
  if (error != JsNoError) {
    delete templateData;
    return Local<ObjectTemplate>();
  }

  return Local<ObjectTemplate>::New(
    static_cast<ObjectTemplate*>(objectTemplateRef));
}

JsValueRef CALLBACK GetSelf(_In_ JsValueRef callee,
                            _In_ bool isConstructCall,
                            _In_ JsValueRef *arguments,
                            _In_ unsigned short argumentCount,
                            _In_opt_ void *callbackState) {
  return reinterpret_cast<JsValueRef>(callbackState);
}

Local<Object> ObjectTemplate::NewInstance() {
  return NewInstance(Local<Object>());
}

Local<Object> ObjectTemplate::NewInstance(Handle<Object> prototype) {
  void* externalData;
  if (JsGetExternalData(this, &externalData) != JsNoError) {
    return Local<Object>();
  }

  ObjectTemplateData *objectTemplateData =
    reinterpret_cast<ObjectTemplateData*>(externalData);

  ObjectData *objectData = new ObjectData(this, objectTemplateData);

  JsValueRef newInstanceRef = JS_INVALID_REFERENCE;
  if (JsCreateExternalObject(objectData,
                             FinalizeCallback, &newInstanceRef) != JsNoError) {
    return Local<Object>();
  }

  if (!prototype.IsEmpty()) {
    if (JsSetPrototype(newInstanceRef,
                       reinterpret_cast<JsValueRef>(*prototype)) != JsNoError) {
      return Local<Object>();
    }
  }


  // In case the object should support index or named properties interceptors,
  // we will use Proxies We will also support in case there is an overrdien
  // toString method on the intercepted object (like Buffer), by returning an
  // object which has the proxy as it prorotype - This is needed due to the fact
  // that proxy implements comparison to other by reference in case the proxy is
  // in the left side of the comparison, and does not call a toString method
  // (like different objects) when compared to string For Node, such comparision
  // is used for Buffers.

  if (objectTemplateData->AreInterceptorsRequired()) {
    JsValueRef newInstanceTargetRef = newInstanceRef;

    JsNativeFunction proxyConf[ProxyTraps::TrapCount] = {};

    if (objectTemplateData->indexedPropertyGetter != nullptr ||
        objectTemplateData->namedPropertyGetter != nullptr)
      proxyConf[ProxyTraps::GetTrap] = GetCallback;

    if (objectTemplateData->indexedPropertySetter != nullptr ||
        objectTemplateData->namedPropertySetter != nullptr)
      proxyConf[ProxyTraps::SetTrap] = SetCallback;

    if (objectTemplateData->indexedPropertyDeleter != nullptr ||
        objectTemplateData->namedPropertyDeleter != nullptr)
      proxyConf[ProxyTraps::DeletePropertyTrap] = DeletePropertyCallback;

    if (objectTemplateData->indexedPropertyEnumerator != nullptr ||
        objectTemplateData->namedPropertyEnumerator != nullptr) {
      proxyConf[ProxyTraps::EnumerateTrap] = EnumerateCallback;
      proxyConf[ProxyTraps::OwnKeysTrap] = OwnKeysCallback;
    }

    if (objectTemplateData->indexedPropertyEnumerator != nullptr ||
        objectTemplateData->namedPropertyEnumerator != nullptr ||
        objectTemplateData->indexedPropertyQuery != nullptr ||
        objectTemplateData->namedPropertyQuery != nullptr) {
      proxyConf[ProxyTraps::HasTrap] = HasCallback;
    }

    if (objectTemplateData->indexedPropertyQuery != nullptr ||
        objectTemplateData->namedPropertyQuery != nullptr ||
        objectTemplateData->indexedPropertyGetter != nullptr ||
        objectTemplateData->namedPropertyGetter != nullptr) {
      proxyConf[ProxyTraps::GetOwnPropertyDescriptorTrap] =
        GetOwnPropertyDescriptorCallback;
    }

    JsErrorCode error =
      jsrt::CreateProxy(newInstanceTargetRef, proxyConf, &newInstanceRef);

    if (error != JsNoError) {
      return Local<Object>();
    }

    // this trick is needed in order to support the equals operator correctly:
    if (objectTemplateData->supportsOverrideToString) {
      JsValueRef proxyRef = newInstanceRef;

      error = JsCreateObject(&newInstanceRef);

      if (error != JsNoError) {
        return Local<Object>();
      }

      error = JsSetPrototype(newInstanceRef, proxyRef);

      if (error != JsNoError) {
        return Local<Object>();
      }
    }
  }

  // clone the object template into the new instance
  JsValueRef propertyNames;
  if (JsGetOwnPropertyNames(objectTemplateData->EnsureProperties(),
                            &propertyNames) != JsNoError) {
    return Local<Object>();
  }

  unsigned int length;
  if (GetArrayLength(propertyNames, &length) != JsNoError) {
    return Local<Object>();
  }

  for (unsigned int index = 0; index < length; index++) {
    JsValueRef indexValue;
    if (JsIntToNumber(index, &indexValue) != JsNoError) {
      return Local<Object>();
    }

    JsValueRef propertyNameValue;
    if (JsGetIndexedProperty(propertyNames,
                             indexValue, &propertyNameValue) != JsNoError) {
      return Local<Object>();
    }

    const wchar_t *propertyName;
    size_t propertyNameLength;
    if (JsStringToPointer(propertyNameValue,
                          &propertyName, &propertyNameLength) != JsNoError) {
      return Local<Object>();
    }

    JsPropertyIdRef propertyId;
    if (JsGetPropertyIdFromName(propertyName, &propertyId) != JsNoError) {
      return Local<Object>();
    }

    JsValueRef propertyDescriptor;
    if (JsGetOwnPropertyDescriptor(objectTemplateData->EnsureProperties(),
                                   propertyId,
                                   &propertyDescriptor) != JsNoError) {
      return Local<Object>();
    }

    bool result;
    if (JsDefineProperty(newInstanceRef,
                         propertyId,
                         propertyDescriptor,
                         &result) != JsNoError) {
      return Local<Object>();
    }
  }

  objectData->objectInstance = newInstanceRef;
  IsolateShim::GetCurrent()->RegisterJsValueRefContextShim(newInstanceRef);
  return Local<Object>::New(static_cast<Object*>(newInstanceRef));
}

void ObjectTemplate::SetAccessor(Handle<String> name,
                                 AccessorGetterCallback getter,
                                 AccessorSetterCallback setter,
                                 Handle<Value> data,
                                 AccessControl settings,
                                 PropertyAttribute attribute,
                                 Handle<AccessorSignature> signature) {
  void* externalData;
  if (JsGetExternalData(this, &externalData) != JsNoError) {
    return;
  }

  ObjectTemplateData *objectTemplateData =
    reinterpret_cast<ObjectTemplateData*>(externalData);
  objectTemplateData->EnsureProperties()->SetAccessor(
    name, getter, setter, data, settings, attribute, signature);
}

void ObjectTemplate::SetNamedPropertyHandler(
    NamedPropertyGetterCallback getter,
    NamedPropertySetterCallback setter,
    NamedPropertyQueryCallback query,
    NamedPropertyDeleterCallback remover,
    NamedPropertyEnumeratorCallback enumerator,
    Handle<Value> data) {
  void* externalData;
  if (JsGetExternalData(this, &externalData) != JsNoError) {
    return;
  }

  ObjectTemplateData *objectTemplateData =
    reinterpret_cast<ObjectTemplateData*>(externalData);

  objectTemplateData->namedPropertyGetter = getter;
  objectTemplateData->namedPropertySetter = setter;
  objectTemplateData->namedPropertyQuery = query;
  objectTemplateData->namedPropertyDeleter = remover;
  objectTemplateData->namedPropertyEnumerator = enumerator;
  objectTemplateData->namedPropertyInterceptorData = Persistent<Value>(data);
}

void ObjectTemplate::SetIndexedPropertyHandler(
    IndexedPropertyGetterCallback getter,
    IndexedPropertySetterCallback setter,
    IndexedPropertyQueryCallback query,
    IndexedPropertyDeleterCallback remover,
    IndexedPropertyEnumeratorCallback enumerator,
    Handle<Value> data) {
  void* externalData;
  if (JsGetExternalData(this, &externalData) != JsNoError) {
    return;
  }

  ObjectTemplateData *objectTemplateData =
    reinterpret_cast<ObjectTemplateData*>(externalData);

  objectTemplateData->indexedPropertyGetter = getter;
  objectTemplateData->indexedPropertySetter = setter;
  objectTemplateData->indexedPropertyQuery = query;
  objectTemplateData->indexedPropertyDeleter = remover;
  objectTemplateData->indexedPropertyEnumerator = enumerator;
  objectTemplateData->indexedPropertyInterceptorData = Persistent<Value>(data);
}


void ObjectTemplate::SetAccessCheckCallbacks(
    NamedSecurityCallback named_callback,
    IndexedSecurityCallback indexed_callback,
    Handle<Value> data,
    bool turned_on_by_default) {
  // CHAKRA-TODO
}

void ObjectTemplate::SetInternalFieldCount(int value) {
  void* externalData;
  if (JsGetExternalData(this, &externalData) != JsNoError) {
    return;
  }

  ObjectTemplateData *objectTemplateData =
    reinterpret_cast<ObjectTemplateData*>(externalData);
  objectTemplateData->internalFieldCount = value;
}

void ObjectTemplate::SetClassName(Handle<String> className) {
  void* externalData;
  if (JsGetExternalData(this, &externalData) != JsNoError) {
    return;
  }

  ObjectTemplateData *objectTemplateData =
    reinterpret_cast<ObjectTemplateData*>(externalData);
  objectTemplateData->className = Persistent<String>(className);
}

namespace chakrashim {
Handle<String> InternalMethods::GetClassName(ObjectTemplate* objectTemplate) {
  void* externalData;
  if (objectTemplate == nullptr ||
      JsGetExternalData(objectTemplate, &externalData) != JsNoError) {
    return Handle<String>();
  }

  ObjectTemplateData* objectTemplateData =
    reinterpret_cast<ObjectTemplateData*>(externalData);
  return objectTemplateData->className;
}
}  // namespace chakrashim

void ObjectTemplate::SetSupportsOverrideToString() {
  void* externalData;
  if (JsGetExternalData(this, &externalData) != JsNoError) {
    return;
  }

  ObjectTemplateData *objectTemplateData =
    reinterpret_cast<ObjectTemplateData*>(externalData);
  objectTemplateData->supportsOverrideToString = true;
}

}  // namespace v8
