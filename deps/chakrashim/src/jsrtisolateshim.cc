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
#include <assert.h>
#include <vector>
#include <algorithm>

namespace jsrt {

/* static */ __declspec(thread) IsolateShim * IsolateShim::s_currentIsolate;
/* static */ IsolateShim * IsolateShim::s_isolateList = nullptr;

IsolateShim::IsolateShim(JsRuntimeHandle runtime)
    : runtime(runtime),
      contextScopeStack(nullptr),
      selfSymbolPropertyIdRef(JS_INVALID_REFERENCE),
      crossContextTargetSymbolPropertyIdRef(JS_INVALID_REFERENCE),
      keepAliveObjectSymbolPropertyIdRef(JS_INVALID_REFERENCE),
      proxySymbolPropertyIdRef(JS_INVALID_REFERENCE),
      finalizerSymbolPropertyIdRef(JS_INVALID_REFERENCE),
      cachedPropertyIdRefs(),
      isDisposing(false),
      tryCatchStackTop(nullptr) {
  // CHAKRA-TODO: multithread locking for s_isolateList?
  this->prevnext = &s_isolateList;
  this->next = s_isolateList;
  s_isolateList = this;
  memset(embeddedData, 0, sizeof(embeddedData));
}

IsolateShim::~IsolateShim() {
  // Nothing to do here, Dispose already did everything
  assert(runtime == JS_INVALID_REFERENCE);
  assert(this->next == nullptr);
  assert(this->prevnext == nullptr);
}

/* static */ v8::Isolate * IsolateShim::New() {
  // CHAKRA-TODO: Disable multiple isolate for now until it is fully implemented
  if (s_isolateList != nullptr) {
    CHAKRA_UNIMPLEMENTED_("multiple isolate");
    return nullptr;
  }

  JsRuntimeHandle runtime;
  JsErrorCode error =
    JsCreateRuntime(JsRuntimeAttributeAllowScriptInterrupt, nullptr, &runtime);
  if (error != JsNoError) {
    return nullptr;
  }

  return ToIsolate(new IsolateShim(runtime));
}

/* static */ IsolateShim * IsolateShim::FromIsolate(v8::Isolate * isolate) {
  return reinterpret_cast<jsrt::IsolateShim *>(isolate);
}

/* static */ v8::Isolate * IsolateShim::ToIsolate(IsolateShim * isolateShim) {
  // v8::Isolate has no data member, so we can just pretend
  return reinterpret_cast<v8::Isolate *>(isolateShim);
}

/* static */ v8::Isolate * IsolateShim::GetCurrentAsIsolate() {
  return ToIsolate(s_currentIsolate);
}

/* static */ IsolateShim *IsolateShim::GetCurrent() {
  assert(s_currentIsolate);
  return s_currentIsolate;
}

void IsolateShim::Enter() {
  // CHAKRA-TODO: we don't support multiple isolate currently, this also doesn't
  // support reentrence
  assert(s_currentIsolate == nullptr);
  s_currentIsolate = this;
}

void IsolateShim::Exit() {
  // CHAKRA-TODO: we don't support multiple isolate currently, this also doesn't
  // support reentrence
  assert(s_currentIsolate == this);
  s_currentIsolate = nullptr;
}

JsRuntimeHandle IsolateShim::GetRuntimeHandle() {
  return runtime;
}

bool IsolateShim::Dispose() {
  isDisposing = true;
  {
    // Disposing the runtime may cause finalize call back to run
    // Set the current IsolateShim scope
    v8::Isolate::Scope scope(ToIsolate(this));
    if (JsDisposeRuntime(runtime) != JsNoError) {
      return false;
    }
  }

  // CHAKRA-TODO: multithread locking for s_isolateList?
  if (this->next) {
    this->next->prevnext = this->prevnext;
  }
  *this->prevnext = this->next;

  runtime = JS_INVALID_REFERENCE;
  this->next = nullptr;
  this->prevnext = nullptr;

  delete this;
  return true;
}

bool IsolateShim::IsDisposing() {
  return isDisposing;
}

void CALLBACK IsolateShim::JsContextBeforeCollectCallback(
    _In_ JsRef contextRef, _In_opt_ void *data) {
  IsolateShim * isolateShim = reinterpret_cast<IsolateShim *>(data);
  ContextShim * contextShim = isolateShim->contextShimMap[contextRef];
  isolateShim->contextShimMap.erase(contextRef);
  delete contextShim;
}

bool IsolateShim::NewContext(JsContextRef * context, bool exposeGC,
                             JsValueRef globalObjectTemplateInstance) {
  ContextShim * contextShim =
    ContextShim::New(this, exposeGC, globalObjectTemplateInstance);
  if (contextShim == nullptr) {
    return false;
  }
  JsContextRef contextRef = contextShim->GetContextRef();
  if (JsSetObjectBeforeCollectCallback(contextRef,
                                this,
                                JsContextBeforeCollectCallback) != JsNoError) {
    delete contextShim;
    return false;
  }
  contextShimMap[contextRef] = contextShim;
  *context = contextRef;
  return true;
}

ContextShim * IsolateShim::GetContextShim(JsContextRef contextRef) {
  assert(contextRef != JS_INVALID_REFERENCE);
  return contextShimMap[contextRef];
}

bool IsolateShim::GetMemoryUsage(size_t * memoryUsage) {
  return (JsGetRuntimeMemoryUsage(runtime, memoryUsage) == JsNoError);
}

void IsolateShim::DisposeAll() {
  // CHAKRA-TODO: multithread locking for s_isolateList?
  IsolateShim * curr = s_isolateList;
  s_isolateList = nullptr;
  while (curr) {
    // CHAKRA-TODO: Handle error?
    curr->Dispose();
    curr = curr->next;
  }
}

void IsolateShim::PushScope(
    ContextShim::Scope * scope, JsContextRef contextRef) {
  PushScope(scope, GetContextShim(contextRef));
}

void IsolateShim::PushScope(
    ContextShim::Scope * scope, ContextShim * contextShim) {
  scope->contextShim = contextShim;
  scope->previous = this->contextScopeStack;
  this->contextScopeStack = scope;

  // CHAKRA-TODO: Error handling?
  JsSetCurrentContext(contextShim->GetContextRef());

  // CHAKRA-TODO: Error handling?
  if (!contextShim->EnsureInitialized()) {
    Fatal("Failed to initialize context");
  }
}

void IsolateShim::PopScope(ContextShim::Scope * scope) {
  assert(this->contextScopeStack == scope);
  ContextShim::Scope * prevScope = scope->previous;
  if (prevScope != nullptr) {
    // Marshal the pending exception
    JsValueRef exception = JS_INVALID_REFERENCE;
    bool hasException;
    if (scope->contextShim != prevScope->contextShim &&
        JsHasException(&hasException) == JsNoError &&
        hasException &&
        JsGetAndClearException(&exception) == JsNoError) {
      exception = MarshalJsValueRefToContext(
        exception, scope->contextShim, prevScope->contextShim);
    }

    JsSetCurrentContext(prevScope->contextShim->GetContextRef());

    // CHAKRA-TODO: Error handling?
    if (exception != JS_INVALID_REFERENCE) {
      JsSetException(exception);
    }
  } else {
    JsSetCurrentContext(JS_INVALID_REFERENCE);
  }
  this->contextScopeStack = prevScope;
}

ContextShim * IsolateShim::GetCurrentContextShim() {
  return this->contextScopeStack->contextShim;
}

JsPropertyIdRef IsolateShim::GetSelfSymbolPropertyIdRef() {
  return EnsurePrivateSymbol(&selfSymbolPropertyIdRef);
}

JsPropertyIdRef IsolateShim::GetCrossContextTargetSymbolPropertyIdRef() {
  return EnsurePrivateSymbol(&crossContextTargetSymbolPropertyIdRef);
}

JsPropertyIdRef IsolateShim::GetKeepAliveObjectSymbolPropertyIdRef() {
  // CHAKRA-TODO: has a bug with symbols and proxy, just a real property name
  return GetCachedPropertyIdRef(CachedPropertyIdRef::__keepalive__);
  // return EnsurePrivateSymbol(&keepAliveObjectSymbolPropertyIdRef);
}

JsPropertyIdRef IsolateShim::GetProxySymbolPropertyIdRef() {
  return EnsurePrivateSymbol(&proxySymbolPropertyIdRef);
}

JsPropertyIdRef IsolateShim::GetFinalizerSymbolPropertyIdRef() {
  return EnsurePrivateSymbol(&finalizerSymbolPropertyIdRef);
}

JsPropertyIdRef IsolateShim::EnsurePrivateSymbol(
    JsPropertyIdRef * propertyIdRefPtr) {
  assert(this->GetCurrentContextShim() != nullptr);
  JsPropertyIdRef propertyIdRef = *propertyIdRefPtr;
  if (propertyIdRef == JS_INVALID_REFERENCE) {
    JsValueRef newSymbol;
    if (JsCreateSymbol(JS_INVALID_REFERENCE, &newSymbol) == JsNoError) {
      if (JsGetPropertyIdFromSymbol(newSymbol, &propertyIdRef) == JsNoError) {
        JsAddRef(propertyIdRef, nullptr);
        *propertyIdRefPtr = propertyIdRef;
      }
    }
  }
  return propertyIdRef;
}

static wchar_t const *
const s_cachedPropertyIdRefNames[CachedPropertyIdRef::Count] = {
#define DEF(x) L#x,
#include "jsrtcachedpropertyidref.inc"
};

JsPropertyIdRef IsolateShim::GetCachedPropertyIdRef(
    CachedPropertyIdRef cachedPropertyIdRef) {
  JsPropertyIdRef propertyIdRef = cachedPropertyIdRefs[cachedPropertyIdRef];
  if (propertyIdRef == JS_INVALID_REFERENCE) {
    if (JsGetPropertyIdFromName(s_cachedPropertyIdRefNames[cachedPropertyIdRef],
                                &propertyIdRef) == JsNoError) {
      JsAddRef(propertyIdRef, nullptr);
      cachedPropertyIdRefs[cachedPropertyIdRef] = propertyIdRef;
    }
  }
  return propertyIdRef;
}

JsPropertyIdRef IsolateShim::GetProxyTrapPropertyIdRef(ProxyTraps trap) {
  return GetCachedPropertyIdRef(GetProxyTrapCachedPropertyIdRef(trap));
}

void IsolateShim::RegisterJsValueRefContextShim(JsValueRef valueRef) {
  jsValueRefToContextShimMap[valueRef] = this->GetCurrentContextShim();
}

void IsolateShim::UnregisterJsValueRefContextShim(JsValueRef valueRef) {
  jsValueRefToContextShimMap.erase(valueRef);
}

ContextShim * IsolateShim::GetJsValueRefContextShim(JsValueRef valueRef) {
  auto i = jsValueRefToContextShimMap.find(valueRef);
  return i != jsValueRefToContextShimMap.end() ? i->second : nullptr;
}

ContextShim * IsolateShim::GetObjectContext(JsValueRef valueRef) {
  // CHAKRA-REVIEW: Chakra doesn't have an API to tell what context an object is
  // in. HACK: Go thru the list of context and see which one works
  JsValueType valueType;
  if (this->contextScopeStack != nullptr) {
    if (JsGetValueType(valueRef, &valueType) == JsNoError) {
      return this->GetCurrentContextShim();
    }
  }

  auto i = this->contextShimMap.begin();
  for (; i != this->contextShimMap.end(); i++) {
    ContextShim * contextShim = (*i).second;
    ContextShim::Scope scope(contextShim);
    if (JsGetValueType(valueRef, &valueType) == JsNoError) {
      return contextShim;
    }
  }
  return nullptr;
}

void IsolateShim::DisableExecution() {
  // CHAKRA: Error handling?
  JsDisableRuntimeExecution(this->GetRuntimeHandle());
}

void IsolateShim::EnableExecution() {
  // CHAKRA: Error handling?
  JsEnableRuntimeExecution(this->GetRuntimeHandle());
}

bool IsolateShim::IsExeuctionDisabled() {
  bool isDisabled;
  if (JsIsRuntimeExecutionDisabled(this->GetRuntimeHandle(),
                                   &isDisabled) == JsNoError) {
    return isDisabled;
  }
  return false;
}

bool IsolateShim::AddMessageListener(void * that) {
  try {
    messageListeners.push_back(that);
    return true;
  } catch (...) {
    return false;
  }
}

void IsolateShim::RemoveMessageListeners(void * that) {
  auto i = std::remove(messageListeners.begin(), messageListeners.end(), that);
  messageListeners.erase(i, messageListeners.end());
}

void IsolateShim::SetData(uint32_t slot, void* data) {
  if (slot >= _countof(this->embeddedData)) {
    CHAKRA_UNIMPLEMENTED_("Invalid embedded data index");
  }
  embeddedData[slot] = data;
}

void* IsolateShim::GetData(uint32_t slot) {
  return slot < _countof(this->embeddedData) ? embeddedData[slot] : nullptr;
}

}  // namespace jsrt
