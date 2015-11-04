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
#include "jsrt.h"
#include "jsrtUtils.h"
#include <math.h>

namespace v8 {

using jsrt::IsOfGlobalType;

bool Value::IsUndefined() const {
  JsValueType type;
  if (JsGetValueType((JsValueRef)this, &type) != JsNoError) {
    return false;
  }

  return (type == JsValueType::JsUndefined);
}

bool Value::IsNull() const {
  JsValueType type;
  if (JsGetValueType((JsValueRef)this, &type) != JsNoError) {
    return false;
  }

  return (type == JsValueType::JsNull);
}

bool Value::IsTrue() const {
  bool isTrue;
  if (JsEquals(*True(), (JsValueRef)this, &isTrue) != JsNoError) {
    return false;
  }

  return isTrue;
}

bool Value::IsFalse() const {
  bool isFalse;
  if (JsEquals(*False(), (JsValueRef)this, &isFalse) != JsNoError) {
    return false;
  }

  return isFalse;
}

bool Value::IsString() const {
  JsValueType type;
  if (JsGetValueType((JsValueRef)this, &type) != JsNoError) {
    return false;
  }
  return (type == JsValueType::JsString);
}

bool Value::IsFunction() const {
  JsValueType type;
  if (JsGetValueType((JsValueRef)this, &type) != JsNoError) {
    return false;
  }

  return (type == JsValueType::JsFunction);
}

bool Value::IsArray() const {
  JsValueType type;
  if (JsGetValueType((JsValueRef)this, &type) != JsNoError) {
    return false;
  }

  return (type == JsValueType::JsArray);
}

bool Value::IsObject() const {
  JsValueType type;
  if (JsGetValueType((JsValueRef)this, &type) != JsNoError) {
    return false;
  }

  return (type == JsValueType::JsObject || type == JsValueType::JsFunction ||
      type == JsValueType::JsError);
}

bool Value::IsExternal() const {
  return External::IsExternal(this);
}

bool Value::IsTypedArray() const {
  JsValueType type;
  if (JsGetValueType((JsValueRef)this, &type) != JsNoError) {
    return false;
  }
  return (type == JsValueType::JsTypedArray);
}

bool Value::IsBoolean() const {
  JsValueType type;
  if (JsGetValueType((JsValueRef)this, &type) != JsNoError) {
    return false;
  }

  return (type == JsValueType::JsBoolean);
}

bool Value::IsNumber() const {
  JsValueType type;
  if (JsGetValueType((JsValueRef)this, &type) != JsNoError) {
    return false;
  }

  return (type == JsValueType::JsNumber);
}

bool Value::IsInt32() const {
  if (!IsNumber()) {
    return false;
  }

  double value = NumberValue();

  // check that the value is smaller than int 32 bit maximum
  if (value > INT_MAX || value < INT_MIN) {
    return false;
  }

  double second;

  return (modf(value, &second) == 0.0);
}

bool Value::IsUint32() const {
  if (!IsNumber()) {
    return false;
  }

  double value = NumberValue();
  // check that the value is smaller than 32 bit maximum
  if (value > UINT_MAX) {
    return false;
  }


  double second;
  // CHAKRA-TODO: nadavbar: replace this with trunc. Not used for since for some
  // reason my math.h file does not contain it Probably a version problem
  return (modf(value, &second) == 0.0 && value >= 0.0);
}

bool Value::IsDate() const {
  bool result;
  if (IsOfGlobalType((JsValueRef)this, L"Date", &result) != JsNoError) {
    return false;
  }

  return result;
}

bool Value::IsBooleanObject() const {
  return IsOfGlobalType((JsValueRef)this, L"Boolean");
}

bool Value::IsNumberObject() const {
  return IsOfGlobalType((JsValueRef)this, L"Number");
}

bool Value::IsStringObject() const {
  return IsOfGlobalType((JsValueRef)this, L"String");
}

bool Value::IsNativeError() const {
  return IsOfGlobalType((JsValueRef)this, L"Error")
    || IsOfGlobalType((JsValueRef)this, L"EvalError")
    || IsOfGlobalType((JsValueRef)this, L"RangeError")
    || IsOfGlobalType((JsValueRef)this, L"ReferenceError")
    || IsOfGlobalType((JsValueRef)this, L"SyntaxError")
    || IsOfGlobalType((JsValueRef)this, L"TypeError")
    || IsOfGlobalType((JsValueRef)this, L"URIError");
}

bool Value::IsRegExp() const {
  bool result;
  if (IsOfGlobalType((JsValueRef)this, L"RegExp", &result) != JsNoError) {
    return false;
  }

  return result;
}

Local<Boolean> Value::ToBoolean() const {
  JsValueRef value;
  if (JsConvertValueToBoolean((JsValueRef)this, &value) != JsNoError) {
    return Local<Boolean>();
  }

  return Local<Boolean>::New(static_cast<Boolean*>(value));
}

Local<Number> Value::ToNumber() const {
  JsValueRef value;
  if (JsConvertValueToNumber((JsValueRef)this, &value) != JsNoError) {
    return Local<Number>();
  }

  return Local<Number>::New(static_cast<Number*>(value));
}

Local<String> Value::ToString() const {
  JsValueRef value;
  if (JsConvertValueToString((JsValueRef)this, &value) != JsNoError) {
    return Local<String>();
  }

  return Local<String>::New(static_cast<String*>(value));
}

Local<Object> Value::ToObject() const {
  JsValueRef value;
  if (JsConvertValueToObject((JsValueRef)this, &value) != JsNoError) {
    return Local<Object>();
  }

  return Local<Object>::New(static_cast<Object*>(value));
}

Local<Integer> Value::ToInteger() const {
  int64_t value = this->IntegerValue();

  JsValueRef integerRef;

  if (JsIntToNumber(static_cast<int>(value), &integerRef) != JsNoError) {
    return Local<Integer>();
  }

  return Local<Integer>::New(static_cast<Integer*>(integerRef));
}


Local<Uint32> Value::ToUint32() const {
  Local<Integer> jsValue =
    Integer::NewFromUnsigned(Isolate::GetCurrent(), this->Uint32Value());
  return Local<Uint32>(static_cast<Uint32*>(*jsValue));
}

Local<Int32> Value::ToInt32() const {
  Local<Integer> jsValue =
    Integer::New(Isolate::GetCurrent(), this->Int32Value());
  return Local<Int32>(static_cast<Int32*>(*jsValue));
}

bool Value::BooleanValue() const {
  JsValueRef ref;
  if (IsBoolean()) {
    ref = (JsValueRef)this;
  } else {
    if (JsConvertValueToBoolean((JsValueRef)this, &ref) != JsNoError) {
      return false;
    }
  }

  bool value;

  if (JsBooleanToBool((JsValueRef)this, &value) != JsNoError) {
    return false;
  }

  return value;
}

double Value::NumberValue() const {
  JsValueRef ref;
  if (IsNumber()) {
    ref = (JsValueRef)this;
  } else {
    if (JsConvertValueToNumber((JsValueRef)this, &ref) != JsNoError) {
      return 0;
    }
  }

  double value;

  if (JsNumberToDouble(ref, &value) != JsNoError) {
    return false;
  }

  return value;
}

int64_t Value::IntegerValue() const {
  return (int64_t)NumberValue();
}

uint32_t Value::Uint32Value() const {
  return (uint32_t)Int32Value();
}

int32_t Value::Int32Value() const {
  JsValueRef ref;
  if (JsConvertValueToNumber((JsValueRef)this, &ref) != JsNoError) {
    return 0;
  }

  int intValue;
  if (JsNumberToInt(ref, &intValue) != JsNoError) {
    return 0;
  }

  return intValue;
}

bool Value::Equals(Handle<Value> that) const {
  bool equals;
  if (JsEquals((JsValueRef)this, *that, &equals) != JsNoError) {
    return false;
  }

  return equals;
}

bool Value::StrictEquals(Handle<Value> that) const {
  bool strictEquals;
  if (JsStrictEquals((JsValueRef)this, *that, &strictEquals) != JsNoError) {
    return false;
  }

  return strictEquals;
}

}  // namespace v8
