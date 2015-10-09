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
#include <cassert>
#include <memory>

namespace v8 {

using std::unique_ptr;
using namespace jsrt;

const wchar_t *HIDDEN_VALUES_TABLE_PROPERTY_NAME = L"__hiddenvalues__";

enum AccessorType {
  Setter = 0,
  Getter = 1
};

typedef struct AcessorExternalDataType {
  Persistent<Value> data;
  Persistent<AccessorSignature> signature;
  AccessorType type;
  union {
    AccessorGetterCallback getter;
    AccessorSetterCallback setter;
  };
  Persistent<String> propertyName;

  bool CheckSignature(Local<Object> thisPointer, Local<Object>* holder) {
    if (signature.IsEmpty()) {
      *holder = thisPointer;
      return true;
    }

    Local<FunctionTemplate> receiver =
      reinterpret_cast<FunctionTemplate*>(*signature);
    return chakrashim::CheckSignature(receiver, thisPointer, holder);
  }
} AccessorExternalData;

struct InternalFieldDataStruct {
  void** internalFieldPointers;
  int size;
};

bool Object::Set(
    Handle<Value> key, Handle<Value> value, PropertyAttribute attribs) {
  return Set(key, value, attribs, /*force*/false);
}

bool Object::Set(Handle<Value> key,
                 Handle<Value> value,
                 PropertyAttribute attribs,
                 bool force) {
  JsPropertyIdRef idRef;

  if (GetPropertyIdFromValue((JsValueRef)*key, &idRef) != JsNoError) {
    return false;
  }

  // Do it faster if there are no property attributes
  if (!force && attribs == None) {
    if (JsSetProperty((JsValueRef)this,
                      idRef, (JsValueRef)*value, false) != JsNoError) {
      return false;
    }
  } else {  // we have attributes just use it
    PropertyDescriptorOptionValues writable =
      PropertyDescriptorOptionValues::False;

    if ((attribs & ReadOnly) == 0) {
      writable = PropertyDescriptorOptionValues::True;
    }

    PropertyDescriptorOptionValues enumerable =
      PropertyDescriptorOptionValues::False;

    if ((attribs & DontEnum) == 0) {
      enumerable = PropertyDescriptorOptionValues::True;
    }

    PropertyDescriptorOptionValues configurable =
      PropertyDescriptorOptionValues::False;

    if ((attribs & DontDelete) == 0) {
      configurable = PropertyDescriptorOptionValues::True;
    }


    if (DefineProperty((JsValueRef)this,
                       idRef,
                       writable,
                       enumerable,
                       configurable,
                       (JsValueRef)*value,
                       JS_INVALID_REFERENCE,
                       JS_INVALID_REFERENCE) != JsNoError) {
      return false;
    }
  }

  return true;
}

bool Object::ForceSet(
    Handle<Value> key, Handle<Value> value, PropertyAttribute attribs) {
  return Set(key, value, attribs, /*force*/true);
}

bool Object::Set(uint32_t index, Handle<Value> value) {
  if (SetIndexedProperty((JsValueRef)this,
                         index, (JsValueRef)(*value)) != JsNoError) {
    return false;
  }

  return true;
}

// CHAKRA-TODO: maybe add an overload that gets a Handle<String> ?
// CHAKRA-TODO: what if an integer is passsed here?
Local<Value> Object::Get(Handle<Value> key) {
  JsPropertyIdRef idRef;

  if (GetPropertyIdFromValue((JsValueRef)*key, &idRef) != JsNoError) {
    return Local<Value>();
  }

  JsValueRef valueRef;
  if (JsGetProperty((JsValueRef)this, idRef, &valueRef) != JsNoError) {
    return Local<Value>();
  }

  return Local<Value>::New(static_cast<Value*>(valueRef));
}


Local<Value> Object::Get(uint32_t index) {
  JsValueRef valueRef;

  if (GetIndexedProperty((JsValueRef)this, index, &valueRef) != JsNoError) {
    return Local<Value>();
  }

  // CHAKRA-TODO: allocate new local here? or just return Local(value)?
  return Local<Value>::New(static_cast<Value*>(valueRef));
}

bool Object::Has(Handle<String> key) {
  JsValueRef propertyIdRef;
  if (GetPropertyIdFromJsString((JsValueRef)*key,
                                &propertyIdRef) != JsNoError) {
    return false;
  }

  bool result;
  if (JsHasProperty((JsValueRef)this, propertyIdRef, &result) != JsNoError) {
    return false;
  }

  return result;
}

bool Object::Delete(Handle<String> key) {
  JsPropertyIdRef idRef;

  if (GetPropertyIdFromJsString((JsValueRef)*key, &idRef) != JsNoError) {
    return false;
  }

  JsValueRef resultRef;
  if (JsDeleteProperty((JsValueRef)this,
                       idRef, false, &resultRef) != JsNoError) {
    return false;
  }

  // get result:
  JsValueRef booleanRef;
  if (JsConvertValueToBoolean(resultRef, &booleanRef) != JsNoError) {
    return false;
  }

  bool result;
  if (JsBooleanToBool(booleanRef, &result) != JsNoError) {
    return false;
  }

  return result;
}

bool Object::Delete(uint32_t index) {
  if (DeleteIndexedProperty((JsValueRef)this, index) != JsNoError) {
    return false;
  }
  return true;
}

Local<Array> Object::GetPropertyNames() {
  JsValueRef arrayRef;

  if (jsrt::GetPropertyNames((JsValueRef)this, &arrayRef) != JsNoError) {
    return Local<Array>();
  }

  return Local<Array>::New(static_cast<Array*>(arrayRef));
}

Local<Array> Object::GetOwnPropertyNames() {
  JsValueRef arrayRef;

  if (JsGetOwnPropertyNames((JsValueRef)this, &arrayRef) != JsNoError) {
    return Local<Array>();
  }

  return Local<Array>::New(static_cast<Array*>(arrayRef));
}

bool Object::HasOwnProperty(Handle<String> key) {
  JsValueRef result;
  if (jsrt::HasOwnProperty((JsValueRef)this,
                           (JsValueRef)*key, &result) != JsNoError) {
    return false;
  }

  bool boolValue;
  if (JsBooleanToBool(result, &boolValue) != JsNoError) {
    return false;
  }

  return boolValue;
}

Local<String> v8::Object::GetConstructorName() {
  JsValueRef constructor;
  if (jsrt::GetObjectConstructor((JsValueRef)this,
                                 &constructor) != JsNoError) {
    return Local<String>();
  }

  JsValueRef name;
  if (GetProperty(constructor, L"name", &name) != JsNoError) {
    return Local<String>();
  }

  return Local<String>::New(static_cast<String*>(name));
}

JsValueRef CALLBACK AccessorHandler(JsValueRef callee,
                                    bool isConstructCall,
                                    JsValueRef *arguments,
                                    unsigned short argumentCount,
                                    void *callbackState) {
  void *externalData;
  JsValueRef result = JS_INVALID_REFERENCE;

  if (JsGetUndefinedValue(&result) != JsNoError) {
    return JS_INVALID_REFERENCE;
  }

  if (GetExternalData(callee, &externalData) != JsNoError) {
    return result;
  }

  if (externalData == nullptr) {
    return result;
  }

  AccessorExternalData *accessorData =
    static_cast<AccessorExternalData*>(externalData);
  Local<Value> dataLocal = Local<Value>::New(accessorData->data);

  JsValueRef thisRef = JS_INVALID_REFERENCE;
  if (argumentCount > 0) {
    thisRef = arguments[0];
  }
  // this is ok since the first argument will stay on the stack as long as we
  // are in this function
  Local<Object> thisLocal(static_cast<Object*>(thisRef));

  Local<Object> holder;
  if (!accessorData->CheckSignature(thisLocal, &holder)) {
    return JS_INVALID_REFERENCE;
  }

  Local<String> propertyNameLocal =
    Local<String>::New(accessorData->propertyName);
  switch (accessorData->type) {
    case Setter:
    {
      assert(argumentCount == 2);
      PropertyCallbackInfo<void> info(dataLocal, thisLocal, holder);
      accessorData->setter(
        propertyNameLocal, static_cast<Value*>(arguments[1]), info);
      break;
    }
    case Getter:
    {
      PropertyCallbackInfo<Value> info(dataLocal, thisLocal, holder);
      accessorData->getter(propertyNameLocal, info);
      result = info.GetReturnValue().Get();
      break;
    }
     default:
      break;
  }

  return result;
}

void CALLBACK AcessorExternalObjectFinalizeCallback(void *data) {
  if (data != nullptr) {
    AccessorExternalData *accessorData =
      static_cast<AccessorExternalData*>(data);
    delete accessorData;
  }
}

bool Object::SetAccessor(Handle<String> name,
                         AccessorGetterCallback getter,
                         AccessorSetterCallback setter,
                         v8::Handle<Value> data,
                         AccessControl settings,
                         PropertyAttribute attributes) {
  return SetAccessor(
    name,
    getter,
    setter,
    data,
    settings,
    attributes,
    Handle<AccessorSignature>());
}

bool Object::SetAccessor(Handle<String> name,
                         AccessorGetterCallback getter,
                         AccessorSetterCallback setter,
                         v8::Handle<Value> data,
                         AccessControl settings,
                         PropertyAttribute attributes,
                         Handle<AccessorSignature> signature) {
  JsValueRef getterRef = JS_INVALID_REFERENCE;
  JsValueRef setterRef = JS_INVALID_REFERENCE;

  JsPropertyIdRef idRef;
  if (GetPropertyIdFromJsString((JsValueRef)*name, &idRef) != JsNoError) {
    return false;
  }

  if (getter != nullptr) {
    AccessorExternalData *externalData = new AccessorExternalData();
    externalData->type = Getter;
    externalData->propertyName = Persistent<String>::New(name);
    externalData->getter = getter;
    externalData->data = Persistent<Value>::New(data);
    externalData->signature = Persistent<AccessorSignature>::New(signature);

    if (CreateFunctionWithExternalData(AccessorHandler,
                                       externalData,
                                       AcessorExternalObjectFinalizeCallback,
                                       &getterRef) != JsNoError) {
      delete externalData;
      return false;
    }
  }
  if (setter != nullptr) {
    AccessorExternalData *externalData = new AccessorExternalData();
    externalData->type = Setter;
    externalData->propertyName = Persistent<String>::New(name);
    externalData->setter = setter;
    externalData->data = Persistent<Value>::New(data);
    externalData->signature = Persistent<AccessorSignature>::New(signature);

    if (CreateFunctionWithExternalData(AccessorHandler,
                                       externalData,
                                       AcessorExternalObjectFinalizeCallback,
                                       &setterRef) != JsNoError) {
      delete externalData;
      return false;
    }
  }

  // writable is not supported for some reason..

  PropertyDescriptorOptionValues enumerable =
    PropertyDescriptorOptionValues::False;

  if ((attributes & DontEnum) == 0) {
    enumerable = PropertyDescriptorOptionValues::True;
  }

  PropertyDescriptorOptionValues configurable =
    PropertyDescriptorOptionValues::False;

  if ((attributes & DontDelete) == 0) {
    configurable = PropertyDescriptorOptionValues::True;
  }

  // CHAKRA-TODO: we ignore  AccessControl for now..

  if (DefineProperty((JsValueRef)this,
                     idRef,
                     PropertyDescriptorOptionValues::None,
                     enumerable,
                     configurable,
                     JS_INVALID_REFERENCE,
                     getterRef,
                     setterRef) != JsNoError) {
    return false;
  }

  return true;
}

Local<Value> Object::GetPrototype() {
  JsValueRef protoypeRef;

  if (JsGetPrototype((JsValueRef)this, &protoypeRef) != JsNoError) {
    return Local<Value>();
  }

  // CHAKRA-TODO: allocate new local here? or just return Local(value)?
  return Local<Value>::New(static_cast<Value*>(protoypeRef));
}

bool Object::SetPrototype(Handle<Value> prototype) {
  if (JsSetPrototype((JsValueRef)this, *prototype) != JsNoError) {
    return false;
  }

  return true;
}

Local<Value> Object::GetConstructor() {
  JsValueRef constructorRef;

  if (GetObjectConstructor((JsValueRef)this, &constructorRef) != JsNoError) {
    return Local<Value>();
  }

  return Local<Value>::New(static_cast<Value*>(constructorRef));
}

// Create an object that will hold the hidden values
bool Object::SetHiddenValue(Handle<String> key, Handle<Value> value) {
  JsPropertyIdRef hiddenValuesTablePropertyIdRef;
  if (JsGetPropertyIdFromName(HIDDEN_VALUES_TABLE_PROPERTY_NAME,
                              &hiddenValuesTablePropertyIdRef) != JsNoError) {
    return false;
  }

  bool hasHiddenValues;
  if (JsHasProperty((JsValueRef)this,
                    hiddenValuesTablePropertyIdRef,
                    &hasHiddenValues) != JsNoError) {
    return false;
  }

  JsValueRef hiddenValuesTable;
  if (!hasHiddenValues) {
    if (JsCreateObject(&hiddenValuesTable) != JsNoError) {
      return false;
    }

    if (DefineProperty((JsValueRef)this,
                       hiddenValuesTablePropertyIdRef,
                       PropertyDescriptorOptionValues::True,
                       PropertyDescriptorOptionValues::False,
                       PropertyDescriptorOptionValues::False,
                       hiddenValuesTable,
                       JS_INVALID_REFERENCE,
                       JS_INVALID_REFERENCE) != JsNoError) {
      return false;
    }
  } else {
    if (JsGetProperty((JsValueRef)this,
                      hiddenValuesTablePropertyIdRef,
                      &hiddenValuesTable) != JsNoError) {
      return false;
    }
  }

  JsPropertyIdRef keyIdRef;

  if (GetPropertyIdFromJsString((JsValueRef)*key, &keyIdRef) != JsNoError) {
    return false;
  }

  if (JsSetProperty(hiddenValuesTable,
                    keyIdRef, (JsValueRef)*value, false) != JsNoError) {
    return false;
  }

  return true;
}

Local<Value> Object::GetHiddenValue(Handle<String> key) {
  JsPropertyIdRef hiddenValuesTablePropertyIdRef;
  if (JsGetPropertyIdFromName(HIDDEN_VALUES_TABLE_PROPERTY_NAME,
                              &hiddenValuesTablePropertyIdRef) != JsNoError) {
    return Local<Value>();
  }

  bool hasHiddenValues;

  if (JsHasProperty((JsValueRef)this,
                    hiddenValuesTablePropertyIdRef,
                    &hasHiddenValues) != JsNoError) {
    return Local<Value>();
  }

  if (!hasHiddenValues) {
    return Local<Value>();
  }

  JsValueRef hiddenValuesTable;

  if (JsGetProperty((JsValueRef)this,
                    hiddenValuesTablePropertyIdRef,
                    &hiddenValuesTable) != JsNoError) {
    return Local<Value>();
  }

  JsPropertyIdRef keyIdRef;
  if (GetPropertyIdFromJsString((JsValueRef)*key, &keyIdRef) != JsNoError) {
    return Local<Value>();
  }

  bool hasKey;
  if (JsHasProperty(hiddenValuesTable, keyIdRef, &hasKey) != JsNoError) {
    return Local<Value>();
  }

  if (!hasKey) {
    return Local<Value>();
  }

  JsValueRef result;
  if (JsGetProperty(hiddenValuesTable, keyIdRef, &result) != JsNoError) {
    return Local<Value>();
  }

  return Local<Value>::New(static_cast<Value*>(result));
}

ObjectTemplate* Object::GetObjectTemplate() {
  ObjectData *objectData = nullptr;
  return GetObjectData(&objectData) == JsNoError && objectData != nullptr ?
    *objectData->objectTemplate : nullptr;
}

JsErrorCode Object::GetObjectData(ObjectData** objectData) {
  *objectData = nullptr;

  return ContextShim::ExecuteInContextOf<JsErrorCode>(this, [=]() {
    Local<Object> obj(static_cast<Object*>(this));

    if (obj->IsUndefined()) {
      return JsNoError;
    }

    JsErrorCode error;
    JsValueRef self = this;
    {
      JsPropertyIdRef selfSymbolIdRef =
        jsrt::IsolateShim::GetCurrent()->GetSelfSymbolPropertyIdRef();
      if (selfSymbolIdRef != JS_INVALID_REFERENCE) {
        JsValueRef result;
        error = JsGetProperty(this, selfSymbolIdRef, &result);
        if (error != JsNoError) {
          return error;
        }

        if (!Local<Value>(static_cast<Value*>(result))->IsUndefined()) {
          self = result;
        }
      }
    }

    return JsGetExternalData(self, reinterpret_cast<void **>(objectData));
  });
}

JsErrorCode Object::InternalFieldHelper(void ***internalFields, int *count) {
  struct ObjectData *objectData = nullptr;

  JsErrorCode error = GetObjectData(&objectData);
  if (error != JsNoError || objectData == nullptr) {
    *internalFields = nullptr;
    *count = 0;
    return error;
  }

  *internalFields = objectData->internalFields;
  *count = objectData->internalFieldCount;
  return JsNoError;
}

int Object::InternalFieldCount() {
  int result;
  void **arrayRef;

  if (InternalFieldHelper(&arrayRef, &result) != JsNoError) {
    return 0;
  }

  return result;
}

void *Object::GetAlignedPointerFromInternalField(int index) {
  if (index < 0) {
    return nullptr;
  }

  int length;
  void **arrayRef;

  if (InternalFieldHelper(&arrayRef, &length) != JsNoError) {
    return nullptr;
  }

  if (index >= length || arrayRef == nullptr) {
    return nullptr;
  }

  return arrayRef[index];
}

void Object::SetAlignedPointerInInternalField(int index, void *value) {
  if (index < 0) {
    return;
  }

  int length;
  void **arrayRef;

  if (InternalFieldHelper(&arrayRef, &length) != JsNoError) {
    return;
  }

  if (index >= length || arrayRef == nullptr) {
    return;
  }

  arrayRef[index] = value;
}

static JsTypedArrayType ConvertArrayType(ExternalArrayType array_type) {
  switch (array_type) {
    case ExternalArrayType::kExternalInt8Array:
      return JsTypedArrayType::JsArrayTypeInt8;
    case ExternalArrayType::kExternalUint8Array:
      return JsTypedArrayType::JsArrayTypeUint8;
    case ExternalArrayType::kExternalInt16Array:
      return JsTypedArrayType::JsArrayTypeInt16;
    case ExternalArrayType::kExternalUint16Array:
      return JsTypedArrayType::JsArrayTypeUint16;
    case ExternalArrayType::kExternalInt32Array:
      return JsTypedArrayType::JsArrayTypeInt32;
    case ExternalArrayType::kExternalUint32Array:
      return JsTypedArrayType::JsArrayTypeUint32;
    case ExternalArrayType::kExternalFloat32Array:
      return JsTypedArrayType::JsArrayTypeFloat32;
    case ExternalArrayType::kExternalFloat64Array:
      return JsTypedArrayType::JsArrayTypeFloat64;
    case ExternalArrayType::kExternalUint8ClampedArray:
      return JsTypedArrayType::JsArrayTypeUint8Clamped;
  }

  assert(false);
  return JsTypedArrayType();
}

static ExternalArrayType ConvertArrayType(JsTypedArrayType type) {
  switch (type) {
    case JsTypedArrayType::JsArrayTypeInt8:
      return ExternalArrayType::kExternalInt8Array;
    case JsTypedArrayType::JsArrayTypeUint8:
      return ExternalArrayType::kExternalUint8Array;
    case JsTypedArrayType::JsArrayTypeInt16:
      return ExternalArrayType::kExternalInt16Array;
    case JsTypedArrayType::JsArrayTypeUint16:
      return ExternalArrayType::kExternalUint16Array;
    case JsTypedArrayType::JsArrayTypeInt32:
      return ExternalArrayType::kExternalInt32Array;
    case JsTypedArrayType::JsArrayTypeUint32:
      return ExternalArrayType::kExternalUint32Array;
    case JsTypedArrayType::JsArrayTypeFloat32:
      return ExternalArrayType::kExternalFloat32Array;
    case JsTypedArrayType::JsArrayTypeFloat64:
      return ExternalArrayType::kExternalFloat64Array;
    case JsTypedArrayType::JsArrayTypeUint8Clamped:
      return ExternalArrayType::kExternalUint8ClampedArray;
  }

  assert(false);
  return ExternalArrayType();
}

void Object::SetIndexedPropertiesToExternalArrayData(
    void *data, ExternalArrayType array_type, int number_of_elements) {
  JsTypedArrayType type = ConvertArrayType(array_type);
  JsSetIndexedPropertiesToExternalData(
    (JsValueRef)this, data, type, number_of_elements);
}

bool Object::HasIndexedPropertiesInExternalArrayData() {
  bool has;
  if (JsHasIndexedPropertiesExternalData((JsValueRef)this, &has) != JsNoError) {
    return false;
  }

  return has;
}

void *Object::GetIndexedPropertiesExternalArrayData() {
  void *data;
  JsTypedArrayType type;
  unsigned int number_of_elements;

  if (JsGetIndexedPropertiesExternalData((JsValueRef)this,
                                         &data,
                                         &type,
                                         &number_of_elements) != JsNoError) {
    return nullptr;
  }

  return data;
}

ExternalArrayType Object::GetIndexedPropertiesExternalArrayDataType() {
  void *data;
  JsTypedArrayType type;
  unsigned int number_of_elements;

  if (JsGetIndexedPropertiesExternalData((JsValueRef)this,
                                         &data,
                                         &type,
                                         &number_of_elements) != JsNoError) {
    return ExternalArrayType();
  }

  return ConvertArrayType(type);
}

int Object::GetIndexedPropertiesExternalArrayDataLength() {
  void *data;
  JsTypedArrayType type;
  unsigned int number_of_elements;

  if (JsGetIndexedPropertiesExternalData((JsValueRef)this,
                                         &data,
                                         &type,
                                         &number_of_elements) != JsNoError) {
    return 0;
  }

  return number_of_elements;
}

Local<Object> Object::Clone() {
  JsValueRef constructor;
  if (GetObjectConstructor((JsValueRef)this,
                           &constructor) != JsNoError) {
    return Local<Object>();
  }

  JsValueRef obj;
  if (JsCallFunction(constructor, nullptr, 0, &obj) != JsNoError) {
    return Local<Object>();
  }

  if (jsrt::CloneObject((JsValueRef)this, obj) != JsNoError) {
    return Local<Object>();
  }

  return Local<Object>::New(static_cast<Object*>(obj));
}

Local<Context> Object::CreationContext() {
  jsrt::ContextShim * contextShim =
    jsrt::IsolateShim::GetCurrent()->GetObjectContext(this);
  if (contextShim == nullptr) {
    return Local<Context>();
  }
  return Local<Context>::New(
    static_cast<Context *>(contextShim->GetContextRef()));
}

Local<Value> Object::GetRealNamedProperty(Handle<String> key) {
  // CHAKRA-TODO: how to skip interceptors?
  return this->Get(key);
}

Local<Object> Object::New(Isolate* isolate) {
  JsValueRef newObjectRef;
  if (JsCreateObject(&newObjectRef) != JsNoError) {
    return Local<Object>();
  }

  return Local<Object>::New(static_cast<Object*>(newObjectRef));
}

Object *Object::Cast(Value *obj) {
  if (!obj->IsObject()) {
    // CHAKRA-TODO: report error?
    return nullptr;
  }

  return static_cast<Object*>(obj);
}

}  // namespace v8
