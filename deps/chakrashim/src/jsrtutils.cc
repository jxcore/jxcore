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

#include "jsrtutils.h"
#include <functional>
#include <algorithm>

namespace jsrt {

const wchar_t * DEFAULT_EXTENRAL_DATA_NAME = L"__external__";

JsErrorCode GetProperty(_In_ JsValueRef ref,
                        _In_ JsValueRef propName,
                        _Out_ JsValueRef *result) {
  JsPropertyIdRef idRef;
  JsErrorCode error;

  size_t strLength;
  const  wchar_t *strPtr;

  error = JsStringToPointer(propName, &strPtr, &strLength);
  if (error != JsNoError) {
    return error;
  }

  error = JsGetPropertyIdFromName(strPtr, &idRef);
  if (error != JsNoError) {
    return error;
  }

  error = JsGetProperty(ref, idRef, result);

  return error;
}

JsErrorCode GetProperty(_In_ JsValueRef ref,
                        _In_ const wchar_t *propertyName,
                        _Out_ JsValueRef *result) {
  JsPropertyIdRef idRef;
  JsErrorCode error;

  error = JsGetPropertyIdFromName(propertyName, &idRef);

  if (error != JsNoError) {
    return error;
  }

  error = JsGetProperty(ref, idRef, result);

  return error;
}

JsErrorCode SetProperty(_In_ JsValueRef ref,
                        _In_ const wchar_t* propName,
                        _In_ JsValueRef propValue) {
  JsPropertyIdRef idRef;
  JsErrorCode error;

  error = JsGetPropertyIdFromName(propName, &idRef);
  if (error != JsNoError) {
    return error;
  }

  error = JsSetProperty(ref, idRef, propValue, false);

  return error;
}

JsErrorCode SetProperty(_In_ JsValueRef ref,
                        _In_ JsValueRef propName,
                        _In_ JsValueRef propValue) {
  JsPropertyIdRef idRef;
  JsErrorCode error;

  size_t strLength;
  const  wchar_t *strPtr;

  error = JsStringToPointer(propName, &strPtr, &strLength);
  if (error != JsNoError) {
    return error;
  }

  error = JsGetPropertyIdFromName(strPtr, &idRef);
  if (error != JsNoError) {
    return error;
  }

  error = JsSetProperty(ref, idRef, propValue, false);

  return error;
}

JsErrorCode DeleteProperty(_In_ JsValueRef ref,
                           _In_ JsValueRef propName,
                           _Out_ JsValueRef* result) {
  JsPropertyIdRef idRef;
  JsErrorCode error;

  size_t strLength;
  const  wchar_t *strPtr;

  error = JsStringToPointer(propName, &strPtr, &strLength);
  if (error != JsNoError) {
    return error;
  }

  error = JsGetPropertyIdFromName(strPtr, &idRef);
  if (error != JsNoError) {
    return error;
  }

  error = JsDeleteProperty(ref, idRef, false, result);

  return error;
}

JsErrorCode CallProperty(
  _In_ JsValueRef ref,
  _In_ JsPropertyIdRef idRef,
  _In_reads_(argumentCount) JsValueRef *arguments,
  _In_ unsigned short argumentCount,
  _Out_ JsValueRef *result) {
  JsValueRef propertyRef;
  JsErrorCode error;

  error = JsGetProperty(ref, idRef, &propertyRef);
  if (error != JsNoError) {
    return error;
  }

  error = JsCallFunction(propertyRef, arguments, argumentCount, result);
  return error;
}

JsErrorCode CallProperty(
  _In_ JsValueRef ref,
  _In_ const wchar_t *propertyName,
  _In_reads_(argumentCount) JsValueRef *arguments,
  _In_ unsigned short argumentCount,
  _Out_ JsValueRef *result) {
  JsPropertyIdRef idRef;
  JsErrorCode error;

  error = JsGetPropertyIdFromName(propertyName, &idRef);
  if (error != JsNoError) {
    return error;
  }

  return CallProperty(ref, idRef, arguments, argumentCount, result);
}

JsErrorCode GetPropertyOfGlobal(_In_ const wchar_t *propertyName,
                                _Out_ JsValueRef *ref) {
  JsErrorCode error = JsNoError;
  JsValueRef globalRef;

  error = JsGetGlobalObject(&globalRef);

  if (error != JsNoError) {
    return error;
  }

  error = GetProperty(globalRef, propertyName, ref);

  return error;
}

JsErrorCode SetPropertyOfGlobal(_In_ const wchar_t *propertyName,
                                _In_ JsValueRef ref) {
  JsErrorCode error = JsNoError;
  JsValueRef globalRef;

  error = JsGetGlobalObject(&globalRef);

  if (error != JsNoError) {
    return error;
  }

  JsPropertyIdRef propertyIdRef;
  error = JsGetPropertyIdFromName(propertyName, &propertyIdRef);
  if (error != JsNoError) {
    return error;
  }

  error = JsSetProperty(globalRef, propertyIdRef, ref, false);

  return error;
}

JsValueRef GetTrue() {
  return ContextShim::GetCurrent()->GetTrue();
}

JsValueRef GetFalse() {
  return ContextShim::GetCurrent()->GetFalse();
}

JsValueRef GetUndefined() {
  return ContextShim::GetCurrent()->GetUndefined();
}

JsValueRef GetNull() {
  return ContextShim::GetCurrent()->GetNull();
}

JsErrorCode GetArrayLength(_In_ JsValueRef arrayRef,
                           _Out_ unsigned int *arraySize) {
  JsErrorCode error;

  JsPropertyIdRef arrayLengthPropertyIdRef =
    IsolateShim::GetCurrent()->GetCachedPropertyIdRef(
      CachedPropertyIdRef::length);
  JsValueRef lengthRef;

  error = JsGetProperty(arrayRef, arrayLengthPropertyIdRef, &lengthRef);
  if (error != JsNoError) {
    return error;
  }

  double sizeInDouble;
  error = JsNumberToDouble(lengthRef, &sizeInDouble);
  *arraySize = static_cast<unsigned int>(sizeInDouble);

  return error;
}

JsErrorCode InstanceOf(_In_ JsValueRef first,
                       _In_ JsValueRef second,
                       _Out_ bool *result) {
  JsValueRef instanceOfFunction =
    ContextShim::GetCurrent()->GetInstanceOfFunction();
  JsValueRef args[] = { nullptr, first, second };
  JsValueRef resultRef;
  JsErrorCode error = JsCallFunction(instanceOfFunction,
                                     args, _countof(args), &resultRef);

  if (error != JsNoError) {
    return error;
  }

  error = JsBooleanToBool(resultRef, result);

  return error;
}

JsErrorCode InstanceOfGlobalType(_In_ JsValueRef first,
                                 _In_ const wchar_t* typeName,
                                 _Out_ bool *result) {
  JsValueRef typeRef;
  JsErrorCode error;
  error = jsrt::GetPropertyOfGlobal(typeName, &typeRef);
  if (error != JsNoError) {
    return error;
  }

  error = jsrt::InstanceOf(first, typeRef, result);
  return error;
}

JsErrorCode CloneObject(_In_ JsValueRef source,
                        _In_ JsValueRef target,
                        _In_ bool clonePrototype) {
  JsValueRef cloneObjectFunction =
    ContextShim::GetCurrent()->GetCloneObjectFunction();
  JsValueRef args[] = { nullptr, source, target };
  JsValueRef resultRef;
  JsErrorCode error = JsCallFunction(cloneObjectFunction,
                                     args, _countof(args), &resultRef);
  if (error != JsNoError) {
    return error;
  }

  if (clonePrototype) {
    JsValueRef prototypeRef;
    JsErrorCode error = JsGetPrototype(source, &prototypeRef);
    if (error != JsNoError) {
      return error;
    }
    error = JsSetPrototype(target, prototypeRef);
  }

  return error;
}

JsErrorCode HasOwnProperty(_In_ JsValueRef object,
                           _In_ JsValueRef prop,
                           _Out_ JsValueRef *result) {
  JsValueRef args[] = { object, prop };
  return jsrt::CallProperty(
      object, L"hasOwnProperty", args, _countof(args), result);
}

JsErrorCode IsValueInArray(
  _In_ JsValueRef arrayRef,
  _In_ JsValueRef valueRef,
  _In_ std::function<JsErrorCode(JsValueRef, JsValueRef, bool*)> comperator,
  _Out_ bool* result) {
  JsErrorCode error;
  unsigned int length;
  *result = false;
  error = GetArrayLength(arrayRef, &length);
  if (error != JsNoError) {
    return error;
  }

  for (unsigned int index = 0; index < length; index++) {
    JsValueRef indexValue;
    error = JsIntToNumber(index, &indexValue);
    if (error != JsNoError) {
      return error;
    }

    JsValueRef itemRef;
    error = JsGetIndexedProperty(arrayRef, indexValue, &itemRef);
    if (error != JsNoError) {
      return error;
    }

    if (comperator != nullptr) {
      error = comperator(valueRef, itemRef, result);
    } else {
      error = JsEquals(itemRef, valueRef, result);
    }

    if (error != JsNoError) {
      return error;
    }

    if (*result)
      return JsNoError;
  }

  return JsNoError;
}

JsErrorCode IsValueInArray(_In_ JsValueRef arrayRef,
                           _In_ JsValueRef valueRef,
                           _Out_ bool* result) {
  return IsValueInArray(arrayRef, valueRef, nullptr, result);
}

JsErrorCode IsCaseInsensitiveStringValueInArray(_In_ JsValueRef arrayRef,
                                                _In_ JsValueRef valueRef,
                                                _Out_ bool* result) {
  return IsValueInArray(arrayRef, valueRef, [=](
      JsValueRef first, JsValueRef second, bool* areEqual) -> JsErrorCode {
    JsValueType type;
    *areEqual = false;

    JsErrorCode error = JsGetValueType(first, &type);
    if (error != JsNoError) {
      return error;
    }
    if (type != JsString) {
      return JsNoError;
    }

    error = JsGetValueType(second, &type);
    if (error != JsNoError) {
      return error;
    }
    if (type != JsString) {
      return JsNoError;
    }

    const wchar_t* firstPtr;
    size_t firstLength;

    error = JsStringToPointer(first, &firstPtr, &firstLength);
    if (error != JsNoError) {
      return error;
    }

    const wchar_t* secondPtr;
    size_t secondLength;

    error = JsStringToPointer(second, &secondPtr, &secondLength);
    if (error != JsNoError) {
      return error;
    }

    size_t maxCount = min(firstLength, secondLength);
    *areEqual = (_wcsnicmp(firstPtr, secondPtr, maxCount) == 0);
    return JsNoError;
  },
                        result);
}

JsErrorCode GetOwnPropertyDescriptor(_In_ JsValueRef ref,
                                     _In_ JsValueRef prop,
                                     _Out_ JsValueRef* result) {
  JsValueRef getOwnPropertyDescriptorFunction =
    ContextShim::GetCurrent()->GetGetOwnPropertyDescriptorFunction();
  JsValueRef args[] = { nullptr, ref, prop };
  JsErrorCode error = JsCallFunction(getOwnPropertyDescriptorFunction,
                                     args, _countof(args), result);
  return error;
}

JsErrorCode IsZero(_In_ JsValueRef value,
                   _Out_ bool *result) {
  return JsEquals(value, ContextShim::GetCurrent()->GetZero(), result);
}

JsErrorCode IsUndefined(_In_ JsValueRef value,
                        _Out_ bool *result) {
  return JsEquals(value, GetUndefined(), result);
}

JsErrorCode GetEnumerableNamedProperties(_In_ JsValueRef object,
                                         _Out_ JsValueRef *result) {
  JsValueRef getEnumerableNamedPropertiesFunction =
    ContextShim::GetCurrent()->GetGetEnumerableNamedPropertiesFunction();
  JsValueRef args[] = { nullptr, object };
  JsErrorCode error = JsCallFunction(getEnumerableNamedPropertiesFunction,
                                     args, _countof(args), result);
  return error;
}

JsErrorCode GetEnumerableIndexedProperties(_In_ JsValueRef object,
                                           _Out_ JsValueRef *result) {
  JsValueRef getEnumerableIndexedPropertiesFunction =
    ContextShim::GetCurrent()->GetGetEnumerableIndexedPropertiesFunction();
  JsValueRef args[] = { nullptr, object };
  JsErrorCode error = JsCallFunction(getEnumerableIndexedPropertiesFunction,
                                     args, _countof(args), result);
  return error;
}

JsErrorCode GetIndexedOwnKeys(_In_ JsValueRef object,
                              _Out_ JsValueRef *result) {
  JsValueRef getIndexedOwnKeysFunction =
    ContextShim::GetCurrent()->GetGetIndexedOwnKeysFunction();
  JsValueRef args[] = { nullptr, object };
  JsErrorCode error = JsCallFunction(getIndexedOwnKeysFunction,
                                     args, _countof(args), result);
  return error;
}

JsErrorCode GetNamedOwnKeys(_In_ JsValueRef object,
                            _Out_ JsValueRef *result) {
  JsValueRef getNamedOwnKeysFunction =
    ContextShim::GetCurrent()->GetGetNamedOwnKeysFunction();
  JsValueRef args[] = { nullptr, object };
  JsErrorCode error = JsCallFunction(getNamedOwnKeysFunction,
                                     args, _countof(args), result);
  return error;
}

JsErrorCode ConcatArray(_In_ JsValueRef first,
                        _In_ JsValueRef second,
                        _Out_ JsValueRef *result) {
  JsValueRef args[] = { first, second };

  return CallProperty(first, L"concat", args, _countof(args), result);
}

JsErrorCode CreateEnumerationIterator(_In_ JsValueRef enumeration,
                                      _Out_ JsValueRef *result) {
  JsValueRef createEnumerationIteratorFunction =
    ContextShim::GetCurrent()->GetCreateEnumerationIteratorFunction();
  JsValueRef args[] = { nullptr, enumeration };
  JsErrorCode error = JsCallFunction(createEnumerationIteratorFunction,
                                     args, _countof(args), result);
  return error;
}

JsErrorCode CreatePropertyDescriptorsEnumerationIterator(
    _In_ JsValueRef enumeration,
    _Out_ JsValueRef *result) {
  JsValueRef function = ContextShim::GetCurrent()
                    ->GetCreatePropertyDescriptorsEnumerationIteratorFunction();
  JsValueRef args[] = { nullptr, enumeration };
  JsErrorCode error = JsCallFunction(function,
                                     args, _countof(args), result);
  return error;
}

JsErrorCode GetPropertyNames(_In_ JsValueRef object,
                             _Out_ JsValueRef *namesArray) {
  JsValueRef getPropertyNamesFunction =
    ContextShim::GetCurrent()->GetGetPropertyNamesFunction();
  JsValueRef args[] = { nullptr, object };
  JsErrorCode error = JsCallFunction(getPropertyNamesFunction,
                                     args, _countof(args), namesArray);
  return error;
}

JsErrorCode GetExternalDataDefaultName(
                                       _Out_ const wchar_t **defaultName) {
  *defaultName = DEFAULT_EXTENRAL_DATA_NAME;
  return JsNoError;
}

JsErrorCode AddExternalData(_In_ JsValueRef ref,
                            _In_ JsPropertyIdRef externalDataPropertyId,
                            _In_ void *data,
                            _In_ JsFinalizeCallback onObjectFinalize) {
  JsErrorCode error;

  JsValueRef externalObjectRef;
  error = JsCreateExternalObject(data, onObjectFinalize, &externalObjectRef);
  if (error != JsNoError) {
    return error;
  }

  error = DefineProperty(ref,
                         externalDataPropertyId,
                         PropertyDescriptorOptionValues::False,
                         PropertyDescriptorOptionValues::False,
                         PropertyDescriptorOptionValues::False,
                         externalObjectRef,
                         JS_INVALID_REFERENCE, JS_INVALID_REFERENCE);
  return error;
}

JsErrorCode AddExternalData(_In_ JsValueRef ref,
                            _In_ const wchar_t *externalDataName,
                            _In_ void *data,
                            _In_ JsFinalizeCallback onObjectFinalize) {
  JsErrorCode error;
  JsPropertyIdRef idRef;
  error = JsGetPropertyIdFromName(externalDataName, &idRef);
  if (error != JsNoError) {
    return error;
  }

  return AddExternalData(ref, idRef, data, onObjectFinalize);
}

JsErrorCode AddExternalData(_In_ JsValueRef ref,
                            _In_ void *data,
                            _In_ JsFinalizeCallback onObjectFinalize) {
  JsErrorCode error;
  JsPropertyIdRef defaultExternalDataPropertyIdRef;
  error = JsGetPropertyIdFromName(DEFAULT_EXTENRAL_DATA_NAME,
                                  &defaultExternalDataPropertyIdRef);
  if (error != JsNoError) {
    return error;
  }


  return AddExternalData(
    ref, defaultExternalDataPropertyIdRef, data, onObjectFinalize);
}

JsErrorCode HasExternalData(_In_ JsValueRef ref,
                            JsPropertyIdRef propertyId,
                            _Out_ bool *result) {
  return JsHasProperty(ref, propertyId, result);
}

JsErrorCode HasExternalData(_In_ JsValueRef ref,
                            _In_ const wchar_t *externalDataName,
                            _Out_ bool *result) {
  JsErrorCode error;
  JsPropertyIdRef idRef;
  error = JsGetPropertyIdFromName(externalDataName, &idRef);
  if (error != JsNoError) {
    return error;
  }

  return HasExternalData(ref, idRef, result);
}

JsErrorCode HasExternalData(_In_ JsValueRef ref,
                            _Out_ bool *result) {
  JsErrorCode error;
  JsPropertyIdRef defaultExternalDataPropertyIdRef;
  error = JsGetPropertyIdFromName(DEFAULT_EXTENRAL_DATA_NAME,
                                  &defaultExternalDataPropertyIdRef);
  if (error != JsNoError) {
    return error;
  }

  return HasExternalData(ref, defaultExternalDataPropertyIdRef, result);
}

JsErrorCode GetExternalData(_In_ JsValueRef ref,
                            _In_ JsPropertyIdRef idRef,
                            _Out_ void **data) {
  JsErrorCode error;
  bool hasProperty;
  error = HasExternalData(ref, idRef, &hasProperty);
  if (error != JsNoError) {
    return error;
  }

  if (!hasProperty) {
    *data = nullptr;
    return JsNoError;
  }

  JsValueRef externalObject;
  error = JsGetProperty(ref, idRef, &externalObject);
  if (error != JsNoError) {
    return error;
  }

  error = JsGetExternalData(externalObject, data);
  return error;
}

JsErrorCode GetExternalData(_In_ JsValueRef ref,
                            _In_ const wchar_t *externalDataName,
                            _Out_ void **data) {
  JsErrorCode error;
  JsPropertyIdRef propertyIdRef;
  error = JsGetPropertyIdFromName(externalDataName, &propertyIdRef);
  if (error != JsNoError) {
    return error;
  }

  error = GetExternalData(ref, propertyIdRef, data);
  return error;
}

JsErrorCode GetExternalData(_In_ JsValueRef ref,
                            _Out_ void **data) {
  JsErrorCode error;
  JsPropertyIdRef defaultExternalDataPropertyIdRef;
  error = JsGetPropertyIdFromName(DEFAULT_EXTENRAL_DATA_NAME,
                                  &defaultExternalDataPropertyIdRef);
  if (error != JsNoError) {
    return error;
  }

  error = GetExternalData(ref, defaultExternalDataPropertyIdRef, data);
  return error;
}

JsErrorCode CreateFunctionWithExternalData(
    _In_ JsNativeFunction nativeFunction,
    void* data,
    _In_ JsFinalizeCallback onObjectFinalize,
    _Out_ JsValueRef *function) {
  JsErrorCode error;
  error = JsCreateFunction(nativeFunction, nullptr, function);
  if (error != JsNoError) {
    return error;
  }

  error = AddExternalData(*function, data, onObjectFinalize);
  return error;
}

JsErrorCode ToString(_In_ JsValueRef ref,
                     _Out_ JsValueRef * strRef,
                     _Out_ const wchar_t** str,
                     _In_ bool alreadyString) {
  // just a dummy here
  size_t size;
  JsErrorCode error;

  // call convert only if needed
  if (alreadyString) {
    *strRef = ref;
  } else {
    error = JsConvertValueToString(ref, strRef);
    if (error != JsNoError) {
      return error;
    }
  }

  error = JsStringToPointer(*strRef, str, &size);
  return error;
}

PropertyDescriptorOptionValues GetPropertyDescriptorOptionValue(bool b) {
  return b ?
    PropertyDescriptorOptionValues::True :
    PropertyDescriptorOptionValues::False;
}

JsErrorCode CreatePropertyDescriptor(
    _In_ PropertyDescriptorOptionValues writable,
    _In_ PropertyDescriptorOptionValues enumerable,
    _In_ PropertyDescriptorOptionValues configurable,
    _In_ JsValueRef value,
    _In_ JsValueRef getter,
    _In_ JsValueRef setter,
    _Out_ JsValueRef *descriptor) {
  JsErrorCode error;
  error = JsCreateObject(descriptor);
  if (error != JsNoError) {
    return error;
  }

  IsolateShim * isolateShim = IsolateShim::GetCurrent();
  ContextShim * contextShim = isolateShim->GetCurrentContextShim();
  JsValueRef trueRef = contextShim->GetTrue();
  JsValueRef falseRef = contextShim->GetFalse();

  // set writable
  if (writable != PropertyDescriptorOptionValues::None) {
    JsPropertyIdRef writablePropertyIdRef =
      isolateShim->GetCachedPropertyIdRef(CachedPropertyIdRef::writable);
    JsValueRef writableRef =
      (writable == PropertyDescriptorOptionValues::True) ? trueRef : falseRef;
    error = JsSetProperty(*descriptor,
                          writablePropertyIdRef, writableRef, false);
    if (error != JsNoError) {
      return error;
    }
  }

  // set enumerable
  if (enumerable != PropertyDescriptorOptionValues::None) {
    JsPropertyIdRef enumerablePropertyIdRef =
      isolateShim->GetCachedPropertyIdRef(CachedPropertyIdRef::enumerable);
    JsValueRef enumerableRef =
      (enumerable == PropertyDescriptorOptionValues::True) ? trueRef : falseRef;
    error = JsSetProperty(*descriptor,
                          enumerablePropertyIdRef, enumerableRef, false);
    if (error != JsNoError) {
      return error;
    }
  }

  // set configurable
  if (configurable != PropertyDescriptorOptionValues::None) {
    JsPropertyIdRef configurablePropertyIdRef =
      isolateShim->GetCachedPropertyIdRef(CachedPropertyIdRef::configurable);
    JsValueRef configurableRef =
      (configurable == PropertyDescriptorOptionValues::True) ?
        trueRef : falseRef;
    error = JsSetProperty(*descriptor,
                          configurablePropertyIdRef, configurableRef, false);
    if (error != JsNoError) {
      return error;
    }
  }

  // set value
  if (value != JS_INVALID_REFERENCE) {
    JsPropertyIdRef valuePropertyIdRef =
      isolateShim->GetCachedPropertyIdRef(CachedPropertyIdRef::value);
    error = JsSetProperty(*descriptor, valuePropertyIdRef, value, false);
    if (error != JsNoError) {
      return error;
    }
  }

  // set getter if needed
  if (getter != JS_INVALID_REFERENCE) {
    JsPropertyIdRef getterPropertyIdRef =
      isolateShim->GetCachedPropertyIdRef(CachedPropertyIdRef::get);
    error = JsSetProperty(*descriptor, getterPropertyIdRef, getter, false);
    if (error != JsNoError) {
      return error;
    }
  }

  // set setter if needed
  if (setter != JS_INVALID_REFERENCE) {
    JsPropertyIdRef setterPropertyIdRef =
      isolateShim->GetCachedPropertyIdRef(CachedPropertyIdRef::set);
    error = JsSetProperty(*descriptor, setterPropertyIdRef, setter, false);
    if (error != JsNoError) {
      return error;
    }
  }

  return JsNoError;
}

JsErrorCode CreatePropertyDescriptor(_In_ v8::PropertyAttribute attributes,
                                     _In_ JsValueRef value,
                                     _In_ JsValueRef getter,
                                     _In_ JsValueRef setter,
                                     _Out_ JsValueRef *descriptor) {
  return CreatePropertyDescriptor(
    GetPropertyDescriptorOptionValue(!(attributes & v8::ReadOnly)),
    GetPropertyDescriptorOptionValue(!(attributes & v8::DontEnum)),
    GetPropertyDescriptorOptionValue(!(attributes & v8::DontDelete)),
    value,
    JS_INVALID_REFERENCE,
    JS_INVALID_REFERENCE,
    descriptor);
}

JsErrorCode DefineProperty(_In_ JsValueRef object,
                           _In_ JsPropertyIdRef propertyIdRef,
                           _In_ PropertyDescriptorOptionValues writable,
                           _In_ PropertyDescriptorOptionValues enumerable,
                           _In_ PropertyDescriptorOptionValues configurable,
                           _In_ JsValueRef value,
                           _In_ JsValueRef getter,
                           _In_ JsValueRef setter) {
  JsValueRef descriptor;
  JsErrorCode error;
  error = CreatePropertyDescriptor(
    writable, enumerable, configurable, value, getter, setter, &descriptor);
  if (error != JsNoError) {
    return error;
  }

  bool result;
  error = JsDefineProperty(object, propertyIdRef, descriptor, &result);

  if (error == JsNoError && !result) {
    return JsErrorInvalidArgument;
  }
  return error;
}

JsErrorCode DefineProperty(_In_ JsValueRef object,
                           _In_ const wchar_t * propertyName,
                           _In_ PropertyDescriptorOptionValues writable,
                           _In_ PropertyDescriptorOptionValues enumerable,
                           _In_ PropertyDescriptorOptionValues configurable,
                           _In_ JsValueRef value,
                           _In_ JsValueRef getter,
                           _In_ JsValueRef setter) {
  JsErrorCode error;
  JsPropertyIdRef propertyIdRef;

  error = JsGetPropertyIdFromName(propertyName, &propertyIdRef);
  if (error != JsNoError) {
    return error;
  }

  error = DefineProperty(
    object, propertyIdRef, writable, enumerable, configurable, value,
    getter, setter);
  return error;
}

// CHAKRA-TODO: cache the property ids in a hash table?
JsErrorCode GetPropertyIdFromJsString(_In_ JsValueRef stringRef,
                                      _Out_ JsPropertyIdRef *idRef) {
  JsErrorCode error;
  const wchar_t *propertyName;
  size_t propertyNameSize;

  error = JsStringToPointer(stringRef, &propertyName, &propertyNameSize);
  if (error != JsNoError) {
    return error;
  }

  error = JsGetPropertyIdFromName(propertyName, idRef);
  return error;
}

JsErrorCode GetPropertyIdFromValue(_In_ JsValueRef valueRef,
                                   _Out_ JsPropertyIdRef *idRef) {
  JsErrorCode error;
  JsValueRef stringRef;
  error = JsConvertValueToString(valueRef, &stringRef);

  if (error != JsNoError) {
    return error;
  }

  error = GetPropertyIdFromJsString(stringRef, idRef);
  return error;
}

JsErrorCode GetObjectConstructor(_In_ JsValueRef objectRef,
                                 _Out_ JsValueRef *constructorRef) {
  JsErrorCode error;
  JsPropertyIdRef constructorPropertyIdRef;
  error = JsGetPropertyIdFromName(L"constructor", &constructorPropertyIdRef);
  if (error != JsNoError) {
    return error;
  }

  error = JsGetProperty(objectRef, constructorPropertyIdRef, constructorRef);
  return error;
}

JsErrorCode SetIndexedProperty(_In_ JsValueRef object,
                               _In_ int index,
                               _In_ JsValueRef value) {
  JsErrorCode error;
  JsValueRef indexRef;
  error = JsIntToNumber(index, &indexRef);
  if (error != JsNoError) {
    return error;
  }

  error = JsSetIndexedProperty(object, indexRef, value);
  return error;
}

JsErrorCode GetIndexedProperty(_In_ JsValueRef object,
                               _In_ int index,
                               _In_ JsValueRef *value) {
  JsErrorCode error;
  JsValueRef indexRef;
  error = JsIntToNumber(index, &indexRef);
  if (error != JsNoError) {
    return error;
  }

  error = JsGetIndexedProperty(object, indexRef, value);
  return error;
}

JsErrorCode DeleteIndexedProperty(_In_ JsValueRef object,
                                  _In_ int index) {
  JsErrorCode error;
  JsValueRef indexRef;
  error = JsIntToNumber(index, &indexRef);
  if (error != JsNoError) {
    return error;
  }

  error = JsDeleteIndexedProperty(object, indexRef);
  return error;
}

JsErrorCode HasProperty(_In_ JsValueRef object,
                        _In_ JsValueRef prop,
                        _Out_ bool *result) {
  JsPropertyIdRef idRef;
  JsErrorCode error;

  size_t strLength;
  const  wchar_t *strPtr;

  error = JsStringToPointer(prop, &strPtr, &strLength);
  if (error != JsNoError) {
    return error;
  }

  error = JsGetPropertyIdFromName(strPtr, &idRef);
  if (error != JsNoError) {
    return error;
  }

  error = JsHasProperty(object, idRef, result);

  return error;
}

JsErrorCode HasIndexedProperty(_In_ JsValueRef object,
                               _In_ int index,
                               _Out_ bool *result) {
  JsErrorCode error;
  JsValueRef indexRef;
  error = JsIntToNumber(index, &indexRef);
  if (error != JsNoError) {
    return error;
  }

  error = JsHasIndexedProperty(object, indexRef, result);
  return error;
}

JsErrorCode IsOfGlobalType(
  _In_ JsValueRef objectRef,
  _In_ const wchar_t *typeName,
  _Out_ bool *result
  ) {
  // the JS equivlant to what we do is: this.constructor.prototype ==
  // object.prototype
  JsErrorCode error;
  JsValueRef valRef;

  error = GetPropertyOfGlobal(typeName, &valRef);
  if (error != JsNoError) {
    return error;
  }

  error = InstanceOf(objectRef, valRef, result);

  return error;
}

bool IsOfGlobalType(_In_ JsValueRef objectRef,
                    _In_ const wchar_t *typeName) {
  bool result;
  return IsOfGlobalType(objectRef, typeName, &result) == JsNoError && result;
}

JsErrorCode SetConstructorName(_In_ JsValueRef objectRef,
                               _In_ const wchar_t * name) {
  JsValueRef stringRef;
  JsErrorCode error = JsPointerToString(name, wcslen(name), &stringRef);
  if (error != JsNoError) {
    return error;
  }

  return SetConstructorName(objectRef, stringRef);
}

JsErrorCode SetConstructorName(_In_ JsValueRef objectRef,
                               _In_ JsValueRef nameStringRef) {
  JsErrorCode error;
  JsValueRef constructorObj = JS_INVALID_REFERENCE;

  error = GetProperty(objectRef, L"constructor", &constructorObj);
  if (error != JsNoError) {
    return error;
  }

  if (constructorObj == JS_INVALID_REFERENCE) {
    return JsErrorInvalidArgument;
  }

  JsPropertyIdRef namePropertyIdRef;
  error = JsGetPropertyIdFromName(L"name", &namePropertyIdRef);
  if (error != JsNoError) {
    return error;
  }

  error = JsSetProperty(constructorObj,
                        namePropertyIdRef, nameStringRef, false);
  return error;
}

// used for debugging

JsErrorCode StringifyObject(_In_ JsValueRef object,
                            _Out_ const wchar_t **stringifiedObject) {
  JsValueRef jsonObj = JS_INVALID_REFERENCE;
  JsErrorCode error;
  error = GetPropertyOfGlobal(L"JSON", &jsonObj);
  if (error != JsNoError) {
    return error;
  }

  JsValueRef args[] = { jsonObj, object };
  JsValueRef jsonResult;
  error = jsrt::CallProperty(jsonObj,
                             L"stringify", args, _countof(args), &jsonResult);
  if (error != JsNoError) {
    return error;
  }

  size_t stringLength;
  return JsStringToPointer(jsonResult, stringifiedObject, &stringLength);
}

JsErrorCode GetConstructorName(_In_ JsValueRef objectRef,
                               _Out_ const wchar_t **name) {
  JsErrorCode error;
  JsValueRef constructorObj = JS_INVALID_REFERENCE;

  error = GetProperty(objectRef, L"constructor", &constructorObj);
  if (error != JsNoError) {
    return error;
  }

  if (constructorObj == JS_INVALID_REFERENCE) {
    return JsErrorInvalidArgument;
  }

  JsValueRef stringRef = JS_INVALID_REFERENCE;
  error = GetProperty(constructorObj, L"name", &stringRef);
  if (error != JsNoError) {
    return error;
  }
  if (stringRef == JS_INVALID_REFERENCE) {
    return JsErrorInvalidArgument;
  }

  size_t dummyLength;
  error = JsStringToPointer(stringRef, name, &dummyLength);

  return error;
}

void Unimplemented(const char * message) {
  fprintf(stderr, "FATAL ERROR: '%s' unimplemented", message);
  __debugbreak();
  abort();
}

void Fatal(const char * format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, "FATAL ERROR: ");
  vfprintf(stderr, format, args);
  va_end(args);

#ifdef DEBUG
  __debugbreak();
#endif

  abort();
}

void SetOutOfMemoryErrorIfExist(_In_ JsErrorCode errorCode) {
  if (errorCode == JsErrorOutOfMemory) {
    const wchar_t txt[] = L"process out of memory";
    JsValueRef msg, err;
    if (JsPointerToString(txt, _countof(txt) - 1, &msg) == JsNoError &&
      JsCreateError(msg, &err) == JsNoError) {
      JsSetException(err);
    }
  }
}

}  // namespace jsrt
