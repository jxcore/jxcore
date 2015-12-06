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
#include "jsrtutils.h"

#include <memory>

namespace v8 {

using jsrt::IsolateShim;
using jsrt::ContextShim;
using jsrt::PropertyDescriptorOptionValues;
using jsrt::DefineProperty;

Local<Object> Function::NewInstance() const {
  return NewInstance(0, nullptr);
}

Local<Object> Function::NewInstance(int argc, Handle<Value> argv[]) const {
  jsrt::JsArguments<> args(argc + 1);
  args[0] = jsrt::GetUndefined();

  if (argc > 0) {
    for (int i = 0; i < argc; i++) {
      args[i + 1] = *argv[i];
    }
  }

  JsValueRef newInstance;
  if (JsConstructObject((JsValueRef)this,
                        args, argc + 1, &newInstance) != JsNoError) {
    return Local<Object>();
  }

  return Local<Object>::New(newInstance);
}

Local<Value> Function::Call(Handle<Value> recv,
                            int argc, Handle<Value> argv[]) {
  jsrt::JsArguments<> args(argc + 1);
  args[0] = *recv;

  for (int i = 0; i < argc; i++) {
    args[i + 1] = *argv[i];
  }

  JsValueRef result;
  {
    TryCatch tryCatch;
    JsErrorCode error =
      JsCallFunction((JsValueRef)this, args, argc + 1, &result);

    jsrt::SetOutOfMemoryErrorIfExist(error);

    if (error == JsNoError) {
      return Local<Value>::New(static_cast<Value*>(result));
    }
    if (error != JsErrorInvalidArgument) {
      tryCatch.CheckReportExternalException();
      return Local<Value>();
    }
  }

  // Invalid argument may mean some of the object are from another context
  // Check and marshal and call again.

  // NOTE: Ideally, we will never run into this situation where we will
  // also marshal correctly and use object from the same context.
  // But CopyProperty in node_contextify.cc violate that so, we have this
  // to paper over it.
  IsolateShim * isolateShim = IsolateShim::GetCurrent();
  ContextShim * currentContextShim = isolateShim->GetCurrentContextShim();
  if (currentContextShim == nullptr) {
    return Local<Value>();
  }

  for (int i = 0; i < argc + 1; i++) {
    JsValueRef valueRef = args[i];
    ContextShim * objectContextShim = isolateShim->GetObjectContext(valueRef);
    if (currentContextShim == objectContextShim) {
      continue;
    }
    if (objectContextShim != nullptr) {
      ContextShim::Scope scope(objectContextShim);
      args[i] = MarshalJsValueRefToContext(
        valueRef, objectContextShim, currentContextShim);
    } else {
      // Can't find a context
      return Local<Value>();
    }
  }

  {
      TryCatch tryCatch;
      JsErrorCode error = JsCallFunction((JsValueRef)this,
          args, argc + 1, &result);

      jsrt::SetOutOfMemoryErrorIfExist(error);

      if (error != JsNoError) {
          tryCatch.CheckReportExternalException();
          return Local<Value>();
      }
    return Local<Value>::New(result);
  }
}

void Function::SetName(Handle<String> name) {
  JsErrorCode error = DefineProperty((JsValueRef)this,
                 IsolateShim::GetCurrent()->GetCachedPropertyIdRef(
                   jsrt::CachedPropertyIdRef::name),
                 PropertyDescriptorOptionValues::False,
                 PropertyDescriptorOptionValues::False,
                 PropertyDescriptorOptionValues::False,
                 (JsValueRef)*name,
                 JS_INVALID_REFERENCE,
                 JS_INVALID_REFERENCE);
  CHAKRA_ASSERT(error == JsNoError);
}

Function *Function::Cast(Value *obj) {
  CHAKRA_ASSERT(obj->IsFunction());
  return static_cast<Function*>(obj);
}

}  // namespace v8
