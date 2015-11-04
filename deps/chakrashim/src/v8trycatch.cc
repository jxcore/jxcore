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
#include <cassert>

namespace v8 {

TryCatch::TryCatch()
    : error(JS_INVALID_REFERENCE),
      rethrow(false),
      verbose(false) {
  jsrt::IsolateShim * isolateShim = jsrt::IsolateShim::GetCurrent();
  prev = isolateShim->tryCatchStackTop;
  isolateShim->tryCatchStackTop = this;
}

TryCatch::~TryCatch() {
  if (!rethrow) {
    GetAndClearException();
  }

  jsrt::IsolateShim::GetCurrent()->tryCatchStackTop = prev;
}

bool TryCatch::HasCaught() const {
  if (error == JS_INVALID_REFERENCE) {
    const_cast<TryCatch*>(this)->GetAndClearException();
  }

  if (error != JS_INVALID_REFERENCE) {
    return true;
  }

  bool hasException;
  JsErrorCode errorCode = JsHasException(&hasException);
  if (errorCode != JsNoError) {
    if (errorCode == JsErrorInDisabledState) {
      return true;
    }
    // CHAKRA-TODO: report error
    assert(false);
    return false;
  }

  return hasException;
}

bool TryCatch::HasTerminated() const {
  return jsrt::IsolateShim::GetCurrent()->IsExeuctionDisabled();
}

void TryCatch::GetAndClearException() {
  bool hasException;
  JsErrorCode errorCode = JsHasException(&hasException);
  if (errorCode != JsNoError) {
    // CHAKRA-TODO: report error
    assert(errorCode == JsErrorInDisabledState);
    return;
  }

  if (hasException) {
    JsValueRef exceptionRef;
    errorCode = JsGetAndClearException(&exceptionRef);
    if (errorCode != JsNoError) {
      // CHAKRA-TODO: report error
      assert(errorCode == JsErrorInDisabledState);
      return;
    }
    error = exceptionRef;
  }
}

Handle<Value> TryCatch::ReThrow() {
  if (error == JS_INVALID_REFERENCE) {
    GetAndClearException();
  }

  if (error == JS_INVALID_REFERENCE) {
    return Local<Value>();
  }

  if (JsSetException(error) != JsNoError) {
    return Handle<Value>();
  }
  rethrow = true;

  return Local<Value>::New(static_cast<Value *>(error));
}

Local<Value> TryCatch::Exception() const {
  if (error == JS_INVALID_REFERENCE) {
    const_cast<TryCatch*>(this)->GetAndClearException();
  }

  if (error == JS_INVALID_REFERENCE) {
    return Local<Value>();
  }

  return Local<Value>::New(static_cast<Value *>(error));
}

Local<Value> TryCatch::StackTrace() const {
  if (error == JS_INVALID_REFERENCE) {
    const_cast<TryCatch*>(this)->GetAndClearException();
  }

  if (error == JS_INVALID_REFERENCE) {
    return Local<Value>();
  }

  JsPropertyIdRef stack;
  if (JsGetPropertyIdFromName(L"stack", &stack) != JsNoError) {
    return Local<Value>();
  }

  JsValueRef trace;
  if (JsGetProperty(error, stack, &trace) != JsNoError) {
    return Local<Value>();
  }

  return Local<Value>::New(static_cast<Value *>(trace));
}

Local<v8::Message> TryCatch::Message() const {
  // return an empty ref for now, so no nulls/empty messages will be printed
  // should be changed once we understand how to retreive the info for each
  // errror message
  return Local<v8::Message>();
}

void TryCatch::SetVerbose(bool value) {
  this->verbose = value;
}

void TryCatch::CheckReportExternalException() {
  // This is only used by Function::Call. If caller explictly uses a TryCatch
  // and SetVerbose, we'll report the external exception message.
  if (prev != nullptr && prev->verbose) {
    jsrt::IsolateShim::GetCurrent()->ForEachMessageListener([this](
        void * messageListener) {
      ((v8::MessageCallback)messageListener)(Message(), Exception());
    });
  } else {
    rethrow = true;  // Otherwise leave the exception as is
  }
}

}  // namespace v8
