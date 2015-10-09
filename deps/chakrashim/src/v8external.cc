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
#include "jsrtutils.h"

namespace v8 {

using jsrt::IsolateShim;

const wchar_t * EXTERNAL_PROP_NAME = L"__isexternal__";

Local<Value> External::Wrap(void* data) {
  return External::New(Isolate::GetCurrent(), data);
}

inline void* External::Unwrap(Handle<v8::Value> obj) {
  if (!obj->IsExternal()) {
    return nullptr;
  }

  return obj.As<External>()->Value();
}

Local<External> External::New(Isolate* isolate, void* value) {
  JsValueRef externalRef;

  if (JsCreateExternalObject(value, nullptr, &externalRef) != JsNoError) {
    return Local<External>();
  }

  JsValueRef trueRef =
    IsolateShim::FromIsolate(isolate)->GetCurrentContextShim()->GetTrue();
  if (JsGetTrueValue(&trueRef) != JsNoError) {
    return Local<External>();
  }

  if (jsrt::DefineProperty(externalRef,
                           EXTERNAL_PROP_NAME,
                           jsrt::PropertyDescriptorOptionValues::False,
                           jsrt::PropertyDescriptorOptionValues::False,
                           jsrt::PropertyDescriptorOptionValues::False,
                           trueRef,
                           JS_INVALID_REFERENCE,
                           JS_INVALID_REFERENCE) != JsNoError) {
    return Local<External>();
  }

  return Local<External>::New(static_cast<External*>(externalRef));
}

bool External::IsExternal(const v8::Value* value) {
  if (!value->IsObject()) {
    return false;
  }

  JsPropertyIdRef propIdRef;
  if (JsGetPropertyIdFromName(EXTERNAL_PROP_NAME, &propIdRef) != JsNoError) {
    return false;
  }

  bool hasProp;
  if (JsHasProperty((JsValueRef)value, propIdRef, &hasProp) != JsNoError) {
    return false;
  }

  return hasProp;
}

External* External::Cast(v8::Value* obj) {
  if (!obj->IsExternal()) {
    return nullptr;
  }

  return static_cast<External*>(obj);
}

void* External::Value() const {
  void* data;
  if (JsGetExternalData((JsValueRef)this, &data) != JsNoError) {
    return nullptr;
  }

  return data;
}

}  // namespace v8
