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

using jsrt::ContextShim;

Local<Value> StringObject::New(Handle<String> value) {
  JsValueRef stringObjectConstructor =
    ContextShim::GetCurrent()->GetStringObjectConstructor();

  JsValueRef args[] = { nullptr, *value };
  JsValueRef newStringObjectRef;
  if (JsConstructObject(stringObjectConstructor,
                        args,
                        _countof(args),
                        &newStringObjectRef) != JsNoError) {
    return Local<Value>();
  }

  return Local<StringObject>::New(
    static_cast<StringObject*>(newStringObjectRef));
}

StringObject *StringObject::Cast(v8::Value *obj) {
  if (!obj->IsBooleanObject()) {
    // CHAKRA-TODO: what should we return in this case?
    // just exit and print?
    return nullptr;
  }

  return static_cast<StringObject*>(obj);
}

}  // namespace v8
