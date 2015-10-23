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
#include "commons.h"

std::thread::id NodeUtils::g_mainThreadId;
UWPAddOn UWPAddOn::s_instance;

bool UWPAddOn::EnsureCoInitialized() {
  if (!_coInitialized) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    _coInitialized = SUCCEEDED(hr);

    if (!_coInitialized) {
      JS_DEFINE_COM_AND_MARKER();
      JS_THROW_EXCEPTION(STD_TO_STRING("CoInitializeEx failed"));
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

void UWPAddOn::Init(JS_HANDLE_OBJECT_REF target) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  NodeUtils::g_mainThreadId =
      std::this_thread::get_id();  // Capture entering thread id

  if (!s_instance.EnsureCoInitialized()) {
    return;
  }

  JsErrorCode err = JsSetProjectionEnqueueCallback(
      [](JsProjectionCallback jsCallback, JsProjectionCallbackContext jsContext,
         void *context) {
        Async::RunOnMain([jsCallback, jsContext]() { jsCallback(jsContext); });
      },
      /*projectionEnqueueContext*/ nullptr);

  if (err != JsNoError) {
    THROW_EXCEPTION("JsSetProjectionEnqueueCallback failed");
    return;
  }
  
  JS_METHOD_SET(target, "projectNamespace", ProjectNamespace);
  JS_METHOD_SET(target, "close", Close);
}

wchar_t *GetWC(const char *c)
{
    const size_t cSize = mbstowcs(NULL, c, 0) + 1;
    wchar_t* wc = new wchar_t[cSize];
    mbstowcs (wc, c, cSize);

    return wc;
}

JS_METHOD(UWPAddOn, ProjectNamespace) {
  if (!args.IsString(0)) {
    THROW_EXCEPTION("Argument must be a string");
  }

  if (!s_instance.EnsureCoInitialized()) {
    RETURN();
  }

  jxcore::JXString name;
  args.GetString(0, &name);

  wchar_t *wname = GetWC(*name);
  bool err = JsProjectWinRTNamespace(wname) !=
      JsNoError;
  delete wname;
  if (err) {
    THROW_EXCEPTION("JsProjectWinRTNamespace failed");
  }

  // Keep Node alive once successfully projected a UWP namespace
  s_instance.EnsureKeepAlive();
}
JS_METHOD_END

JS_METHOD(UWPAddOn, Close) {
  s_instance.ReleaseKeepAlive();
  s_instance.EnsureCoUninitialized();
}
JS_METHOD_END

NODE_MODULE(uwp, UWPAddOn::Init);
