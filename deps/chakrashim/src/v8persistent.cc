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

#include "v8chakra.h"

namespace v8 {

void CALLBACK Utils::WeakReferenceCallbackWrapperCallback(JsRef ref,
                                                          void *data) {
  chakrashim::WeakReferenceCallbackWrapper *callbackWrapper =
    reinterpret_cast<chakrashim::WeakReferenceCallbackWrapper*>(data);
  WeakCallbackData<Value, void> callbackData(
    Isolate::GetCurrent(),
    static_cast<Value*>(ref),
    callbackWrapper->parameters);
  callbackWrapper->callback(callbackData);
}

namespace chakrashim {

static void CALLBACK DummyObjectBeforeCollectCallback(JsRef ref, void *data) {
  // Do nothing, only used to revive an object temporarily
}

void ClearObjectWeakReferenceCallback(JsValueRef object, bool revive) {
  if (jsrt::IsolateShim::GetCurrent()->IsDisposing()) {
    return;
  }

  JsSetObjectBeforeCollectCallback(
    object, nullptr, revive ? DummyObjectBeforeCollectCallback : nullptr);
}

void SetObjectWeakReferenceCallback(
    JsValueRef object,
    WeakCallbackData<Value, void>::Callback callback,
    void* parameters,
    std::shared_ptr<WeakReferenceCallbackWrapper>* weakWrapper) {
  if (jsrt::IsolateShim::GetCurrent()->IsDisposing()) {
    return;
  }

  if (callback == nullptr || object == JS_INVALID_REFERENCE) {
    return;
  }

  if (!*weakWrapper) {
    *weakWrapper = std::make_shared<WeakReferenceCallbackWrapper>();
  }

  WeakReferenceCallbackWrapper *callbackWrapper = (*weakWrapper).get();
  callbackWrapper->parameters = parameters;
  callbackWrapper->callback = callback;

  JsSetObjectBeforeCollectCallback(
    object, callbackWrapper,
    v8::Utils::WeakReferenceCallbackWrapperCallback);
}

}  // namespace chakrashim
}  // namespace v8
