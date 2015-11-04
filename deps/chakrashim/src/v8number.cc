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

namespace v8 {

double Number::Value() const {
  return NumberValue();
}

Local<Number> Number::New(Isolate* isolate, double value) {
  JsValueRef ref;

  if (JsDoubleToNumber(value, &ref) != JsNoError) {
    return Local<Number>();
  }

  return Local<Number>::New(static_cast<Number*>(ref));
}

Number *Number::Cast(v8::Value *obj) {
  if (!obj->IsNumber()) {
    // CHAKRA-TODO: report an error here!
    // CHAKRA-TODO: What is the best behavior here? Should we return a pointer
    // to undefined/null instead?
    return nullptr;
  }

  return static_cast<Number*>(obj);
}

}  // namespace v8
