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

#pragma once

//
// This file is derived from https://github.com/saary/node-async-helper
//

#include "node.h"
#include <functional>

namespace NodeUtils {

class Async {
 private:
  class TokenData {
   private:
    std::function<void()> func;
    friend Async;

   public:
    static uv_async_t* NewAsyncToken() {
      uv_async_t* asyncHandle = new uv_async_t;
      uv_async_init(uv_default_loop(), asyncHandle, AsyncCb);
      asyncHandle->data = new TokenData();

      return asyncHandle;
    }
  };

 public:
  static DWORD threadId;

  static uv_async_t* GetAsyncToken() {
    return TokenData::NewAsyncToken();
  }

  static void ReleaseAsyncToken(uv_async_t* handle) {
    TokenData* tokenData = static_cast<TokenData*>(handle->data);
    uv_close(reinterpret_cast<uv_handle_t*>(handle), AyncCloseCb);
    delete tokenData;
  }

  static void RunOnMain(uv_async_t* async, std::function<void()> func) {
    TokenData* tokenData = static_cast<TokenData*>(async->data);
    tokenData->func = func;
    uv_async_send(async);
  }

  static void RunOnMain(std::function<void()> func) {
    if (threadId == GetCurrentThreadId()) {
      func();
    } else {
      uv_async_t *async = GetAsyncToken();
      RunOnMain(async, func);
    }
  }

 private:
  // called after the async handle is closed in order to free it's memory
  static void AyncCloseCb(uv_handle_t* handle) {
    if (handle != nullptr) {
      uv_async_t* async = reinterpret_cast<uv_async_t*>(handle);
      delete async;
    }
  }

  // Called by run on main in case we are not running on the main thread
  static void AsyncCb(uv_async_t* handle, int _) {
    TokenData* tokenData = static_cast<TokenData*>(handle->data);
    tokenData->func();
    ReleaseAsyncToken(handle);
  }
};

}  // namespace NodeUtils
