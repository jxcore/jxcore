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

#include "uwp.h"

std::thread::id NodeUtils::g_mainThreadId;
UWPAddOn UWPAddOn::s_instance;

bool UWPAddOn::EnsureCoInitialized() {
  if (!_coInitialized) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    _coInitialized = SUCCEEDED(hr);

    if (!_coInitialized) {
      Nan::ThrowError("CoInitializeEx failed");
    }
  }

  return _coInitialized;
}

void UWPAddOn::EnsureCoUninitialized() {
  if (_coInitialized) {
    CoUninitialize();
    _coInitialized = false;
  }
}

bool UWPAddOn::EnsureKeepAlive() {
  if (_keepAliveToken == nullptr) {
    // Open an active async handler to keep Node alive
    _keepAliveToken = Async::GetAsyncToken();
  }

  return _keepAliveToken != nullptr;
}

void UWPAddOn::ReleaseKeepAlive() {
  if (_keepAliveToken != nullptr) {
    auto token = _keepAliveToken;
    _keepAliveToken = nullptr;

    Async::ReleaseAsyncToken(token);
  }
}

void UWPAddOn::Init(Handle<Object> target) {
  Nan::HandleScope scope;

  NodeUtils::g_mainThreadId =
    std::this_thread::get_id();  // Capture entering thread id

  if (!s_instance.EnsureCoInitialized()) {
    return;
  }

  JsErrorCode err = JsSetProjectionEnqueueCallback(
    [](JsProjectionCallback jsCallback,
       JsProjectionCallbackContext jsContext, void *context) {
    Async::RunOnMain([jsCallback, jsContext]() {
      jsCallback(jsContext);
    });
  },
    /*projectionEnqueueContext*/nullptr);
  if (err != JsNoError) {
    Nan::ThrowError("JsSetProjectionEnqueueCallback failed");
    return;
  }

  Nan::SetMethod(target, "projectNamespace", ProjectNamespace);
  Nan::SetMethod(target, "close", Close);
}

NAN_METHOD(UWPAddOn::ProjectNamespace) {
  Nan::HandleScope scope;

  if (!info[0]->IsString()) {
    Nan::ThrowTypeError("Argument must be a string");
    return;
  }

  if (!s_instance.EnsureCoInitialized()) {
    return;
  }

  String::Value name(info[0]);
  if (JsProjectWinRTNamespace(reinterpret_cast<wchar_t*>(*name)) != JsNoError) {
    Nan::ThrowError("JsProjectWinRTNamespace failed");
    return;
  }

  // Keep Node alive once successfully projected a UWP namespace
  s_instance.EnsureKeepAlive();
  return;
}

NAN_METHOD(UWPAddOn::Close) {
  Nan::HandleScope scope;

  s_instance.ReleaseKeepAlive();
  s_instance.EnsureCoUninitialized();
  return;
}

NODE_MODULE(uwp, UWPAddOn::Init);
