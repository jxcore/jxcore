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
using jsrt::ContextShim;

Local<Value> Date::New(Isolate * isolate, double time) {
  JsValueRef dateConstructor = IsolateShim::FromIsolate(isolate)
                                ->GetCurrentContextShim()->GetDateConstructor();
  JsValueRef newDateRef;
  JsValueRef numberRef;

  if (JsDoubleToNumber(time, &numberRef) != JsNoError) {
    return Local<Value>();
  }

  JsValueRef args[] = { nullptr, numberRef };

  if (JsConstructObject(dateConstructor,
                        args, _countof(args), &newDateRef) != JsNoError) {
    return Local<Value>();
  }

  return Local<Date>::New(static_cast<Date*>(newDateRef));
}

// Not Implemented
Date *Date::Cast(v8::Value *obj) {
  if (!obj->IsDate()) {
    // CHAKRA-TODO: what should we return in this case?
    // just exit and print?
    return nullptr;
  }

  return static_cast<Date*>(obj);
}

}  // namespace v8
