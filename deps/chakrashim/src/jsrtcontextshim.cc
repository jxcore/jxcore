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

#include "jsrtutils.h"
#include "v8chakra.h"
#include "chakra_natives.h"
#include <algorithm>
#include <memory>

namespace jsrt {

ContextShim::Scope::Scope(ContextShim * contextShim) {
  contextShim->GetIsolateShim()->PushScope(this, contextShim);
}

ContextShim::Scope::~Scope() {
  this->contextShim->GetIsolateShim()->PopScope(this);
}

ContextShim * ContextShim::New(IsolateShim * isolateShim, bool exposeGC,
                               JsValueRef globalObjectTemplateInstance) {
  JsContextRef context;
  if (JsCreateContext(isolateShim->GetRuntimeHandle(), &context) != JsNoError) {
    return nullptr;
  }

  // AddRef on globalObjectTemplateInstance if specified. Save and use later.
  if (globalObjectTemplateInstance != JS_INVALID_REFERENCE &&
      JsAddRef(globalObjectTemplateInstance, nullptr) != JsNoError) {
    return nullptr;
  }

  return new ContextShim(isolateShim, context, exposeGC,
                         globalObjectTemplateInstance);
}

ContextShim::ContextShim(IsolateShim * isolateShim,
                         JsContextRef context,
                         bool exposeGC,
                         JsValueRef globalObjectTemplateInstance)
    : isolateShim(isolateShim),
      context(context),
      initialized(false),
      exposeGC(exposeGC),
      globalObjectTemplateInstance(globalObjectTemplateInstance),
      promiseContinuationFunction(JS_INVALID_REFERENCE),
      instanceOfFunction(JS_INVALID_REFERENCE),
      cloneObjectFunction(JS_INVALID_REFERENCE),
      isUintFunction(JS_INVALID_REFERENCE),
      getPropertyNamesFunction(JS_INVALID_REFERENCE),
      getOwnPropertyDescriptorFunction(JS_INVALID_REFERENCE),
      getEnumerableNamedPropertiesFunction(JS_INVALID_REFERENCE),
      getEnumerableIndexedPropertiesFunction(JS_INVALID_REFERENCE),
      createEnumerationIteratorFunction(JS_INVALID_REFERENCE),
      createPropertyDescriptorsEnumerationIteratorFunction(
        JS_INVALID_REFERENCE),
      getNamedOwnKeysFunction(JS_INVALID_REFERENCE),
      getIndexedOwnKeysFunction(JS_INVALID_REFERENCE),
      forEachNonConfigurablePropertyFunction(JS_INVALID_REFERENCE),
      testFunctionTypeFunction(JS_INVALID_REFERENCE),
      createTargetFunction(JS_INVALID_REFERENCE) {
  memset(globalConstructor, 0, sizeof(globalConstructor));
  memset(globalPrototypeFunction, 0, sizeof(globalPrototypeFunction));
  memset(throwAccessorErrorFunctions, 0, sizeof(throwAccessorErrorFunctions));
}

ContextShim::~ContextShim() {
  if (globalObjectTemplateInstance != JS_INVALID_REFERENCE) {
    JsRelease(globalObjectTemplateInstance, nullptr);
  }

  // Mark each existing CrossContextMapInfo* that it no longer needs to
  // unregister from this ContextShim.
  std::for_each(crossContextObjects.begin(), crossContextObjects.end(),
                [](const auto& pair) {
    auto& v = pair.second;
    std::for_each(v.begin(), v.end(), [](CrossContextMapInfo* info) {
      info->fromContext = nullptr;
    });
  });
}

bool ContextShim::CheckConfigGlobalObjectTemplate() {
  if (globalObjectTemplateInstance != JS_INVALID_REFERENCE) {
    // Only need to config once. Discard globalObjectTemplateInstance
    JsValueRef target = globalObjectTemplateInstance;
    JsRelease(globalObjectTemplateInstance, nullptr);
    globalObjectTemplateInstance = JS_INVALID_REFERENCE;

    JsValueRef newProto = JS_INVALID_REFERENCE;
    {
      ContextShim* fromContext =
        IsolateShim::GetCurrent()->GetJsValueRefContextShim(target);
      ContextShim::Scope from(fromContext);
      newProto = MarshalJsValueRefToContext(target, fromContext, this);
    }

    JsValueRef glob, oldProto;
    return newProto != JS_INVALID_REFERENCE &&
      JsGetGlobalObject(&glob) == JsNoError &&
      JsGetPrototype(glob, &oldProto) == JsNoError &&
      JsSetPrototype(newProto, oldProto) == JsNoError &&
      JsSetPrototype(glob, newProto) == JsNoError;
  }

  return true;
}

IsolateShim * ContextShim::GetIsolateShim() {
  return isolateShim;
}

JsContextRef ContextShim::GetContextRef() {
  return context;
}

ContextShim * ContextShim::GetCurrent() {
  return IsolateShim::GetCurrent()->GetCurrentContextShim();
}

void* ContextShim::GetAlignedPointerFromEmbedderData(int index) {
  if (index >= 0 &&
      static_cast<std::vector<void*>::size_type>(index) < embedderData.size()) {
    return embedderData.at(index);
  }
  return nullptr;
}

bool ContextShim::KeepAlive(JsValueRef value) {
  // use an object set as a proeprty of the global object to keep object alive
  // for the duration of the context
  JsValueRef builtInIndex;
  if (JsIntToNumber(builtInCount, &builtInIndex) != JsNoError) {
    return false;
  }
  if (JsSetIndexedProperty(keepAliveObject, builtInIndex, value) != JsNoError) {
    return false;
  }
  builtInCount++;
  return true;
}

template <typename Fn>
bool ContextShim::InitializeBuiltIn(JsValueRef * builtInValue, Fn getBuiltIn) {
  JsValueRef value;
  if (getBuiltIn(&value) != JsNoError) {
    return false;
  }
  if (!KeepAlive(value)) {
    return false;
  }
  *builtInValue = value;
  return true;
}

bool ContextShim::InitializeBuiltIn(JsValueRef * builtInValue,
                                    const wchar_t* globalName) {
  return InitializeBuiltIn(builtInValue,
                           [=](JsValueRef * value) {
    return jsrt::GetProperty(globalObject, globalName, value);
  });
}

static const wchar_t* s_globalTypeNames[] = {
#define DEFTYPE(T) L#T,
#include "jsrtcontextcachedobj.inc"
  L""
};

bool ContextShim::InitializeGlobalTypes() {
  for (int i = 0; i < GlobalType::_TypeCount; i++) {
    if (!InitializeBuiltIn(&globalConstructor[i], s_globalTypeNames[i])) {
      return false;
    }
  }
  return true;
}

bool ContextShim::InitializeGlobalPrototypeFunctions() {
  IsolateShim* iso = GetIsolateShim();
  JsPropertyIdRef prototypeIdRef =
    iso->GetCachedPropertyIdRef(CachedPropertyIdRef::prototype);

  auto init = [=](GlobalPrototypeFunction index,
                  GlobalType type, JsPropertyIdRef functionIdRef) {
    // Cache the builtin function on the type's prototype
    JsValueRef prototype;
    if (!InitializeBuiltIn(&globalPrototypeFunction[index],
                           [=, &prototype](JsValueRef * value) {
      JsErrorCode error = JsGetProperty(
        globalConstructor[type], prototypeIdRef, &prototype);
      if (error != JsNoError) {
        return error;
      }

      return JsGetProperty(prototype, functionIdRef, value);
    })) {
      return false;
    }

    // Replace the builtin function with a cross context shim function
    JsValueRef function;
    if (JsCreateFunction(jsrt::PrototypeFunctionCrossContextShim,
                         reinterpret_cast<void*>(index),
                         &function) != JsNoError) {
      return false;
    }
    return JsSetProperty(prototype, functionIdRef, function,
                         false) == JsNoError;
  };

  struct TypeMethodPair {
    GlobalType type;
    JsPropertyIdRef functionIdRef;
  };

  const TypeMethodPair pairs[GlobalPrototypeFunction::_FunctionCount] = {
#define DEFMETHOD(T, M) { \
    GlobalType::##T, \
    iso->GetCachedPropertyIdRef(CachedPropertyIdRef::##M) },
#include "jsrtcontextcachedobj.inc"
  };

  for (int i = 0; i < _countof(pairs); i++) {
    if (!init(static_cast<GlobalPrototypeFunction>(i),
              pairs[i].type, pairs[i].functionIdRef)) {
      return false;
    }
  }

  return true;
}

// Replace (cached) Object.prototype.toString with a shim to support
// ObjectTemplate class name. Called after InitializeGlobalPrototypeFunctions().
bool ContextShim::InitializeObjectPrototypeToStringShim() {
  JsValueRef function;
  if (!InitializeBuiltIn(&function, [=](JsValueRef * value) {
    JsErrorCode error = JsCreateFunction(
      v8::chakrashim::InternalMethods::ObjectPrototypeToStringShim,
      GetGlobalPrototypeFunction(GlobalPrototypeFunction::Object_toString),
      value);
    if (error != JsNoError) {
      return error;
    }

    return JsNoError;
  })) {
    return false;
  }

  globalPrototypeFunction[GlobalPrototypeFunction::Object_toString] = function;
  return true;
}

bool ContextShim::InitializeBuiltIns() {
  // No need to keep the global object alive, the context will implicitly
  if (JsGetGlobalObject(&globalObject) != JsNoError) {
    return false;
  }

  JsValueRef newKeepAliveObject;
  // Create an object to keep these built in alive along with the global object
  builtInCount = 0;
  if (JsCreateObject(&newKeepAliveObject) != JsNoError) {
    return false;
  }
  keepAliveObject = newKeepAliveObject;
  // true and false is needed by DefineProperty to create the property
  // descriptor
  if (!InitializeBuiltIn(&trueRef, JsGetTrueValue)) {
    return false;
  }
  if (!InitializeBuiltIn(&falseRef, JsGetFalseValue)) {
    return false;
  }
  if (!InitializeBuiltIn(&undefinedRef, JsGetUndefinedValue)) {
    return false;
  }
  if (!InitializeBuiltIn(&nullRef, JsGetNullValue)) {
    return false;
  }
  if (!InitializeBuiltIn(&zero,
                         [](JsValueRef * value) {
                           return JsIntToNumber(0, value);
                         })) {
    return false;
  }

  if (!InitializeGlobalTypes()) {
    return false;
  }
  if (!InitializeGlobalPrototypeFunctions()) {
    return false;
  }
  if (!InitializeObjectPrototypeToStringShim()) {
    return false;
  }

  if (!InitializeBuiltIn(&getOwnPropertyDescriptorFunction,
                         [this](JsValueRef * value) {
                           return GetProperty(GetObjectConstructor(),
                                              L"getOwnPropertyDescriptor",
                                              value);
                         })) {
    return false;
  }

  if (!InitializeReflect()) {
    return false;
  }

  if (DefineProperty(globalObject,
                     GetIsolateShim()->GetKeepAliveObjectSymbolPropertyIdRef(),
                     PropertyDescriptorOptionValues::False,
                     PropertyDescriptorOptionValues::False,
                     PropertyDescriptorOptionValues::False,
                     newKeepAliveObject,
                     JS_INVALID_REFERENCE, JS_INVALID_REFERENCE) != JsNoError) {
    return false;
  }

  if (!InitializeProxyOfGlobal()) {
    return false;
  }

  return true;
}

static JsValueRef CALLBACK ProxyOfGlobalGetPrototypeOfCallback(
    _In_ JsValueRef callee,
    _In_ bool isConstructCall,
    _In_ JsValueRef *arguments,
    _In_ unsigned short argumentCount,
    _In_opt_ void *callbackState) {
  // Return the target (which is the global object)
  return arguments[1];
}

bool ContextShim::InitializeReflect() {
  if (!InitializeBuiltIn(&reflectObject,
                         [](JsValueRef * value) {
                           return GetPropertyOfGlobal(L"Reflect", value);
                         })) {
    return false;
  }

  for (unsigned int i = 0; i < ProxyTraps::TrapCount; i++) {
    if (!InitializeBuiltIn(&reflectFunctions[i],
        [this, i](JsValueRef * value) {
          return JsGetProperty(reflectObject,
              this->GetIsolateShim()->GetProxyTrapPropertyIdRef((ProxyTraps)i),
              value);
        })) {
      return false;
    }
  }
  return true;
}

bool ContextShim::InitializeProxyOfGlobal() {
  return InitializeBuiltIn(&proxyOfGlobal,
                           [this](JsValueRef * value) {
    // V8 Global is actually proxy where the actual global is it's prototype.
    // We will fake it here.
    JsNativeFunction proxyTraps[ProxyTraps::TrapCount] = {};
    proxyTraps[GetPrototypeOfTrap] = ProxyOfGlobalGetPrototypeOfCallback;
    return CreateProxy(globalObject, proxyTraps, value);
  });
}

bool ContextShim::EnsureInitialized() {
  if (initialized) {
    return true;
  }

  if (jsrt::InitializePromise() != JsNoError) {
    return false;
  }

  if (exposeGC && !ExposeGc()) {
    return false;
  }

  if (!InitializeBuiltIns()) {
    return false;
  }

  if (!ExecuteChakraShimJS()) {
    return false;
  }

  initialized = true;

  // Following is a special one-time initialization that needs to marshal
  // objects to this context. Do it after marking initialized = true.
  if (!CheckConfigGlobalObjectTemplate()) {
    return false;
  }

  return true;
}

bool ContextShim::ExposeGc() {
  JsValueRef collectGarbageRef;
  if (jsrt::GetPropertyOfGlobal(L"CollectGarbage",
                                &collectGarbageRef) != JsNoError) {
    return false;
  }

  if (jsrt::SetPropertyOfGlobal(L"gc", collectGarbageRef) != JsNoError) {
    return false;
  }

  return true;
}

bool ContextShim::ExecuteChakraShimJS() {
  wchar_t buffer[_countof(chakra_shim_native)];

  if (StringConvert::CopyRaw<char, wchar_t>(chakra_shim_native,
      _countof(chakra_shim_native),
      buffer,
      _countof(chakra_shim_native)) != JsNoError) {
    return false;
  }
  JsValueRef getInitFunction;
  if (JsParseScript(buffer,
                    JS_SOURCE_CONTEXT_NONE,
                    L"chakra_shim.js",
                    &getInitFunction) != JsNoError) {
    return false;
  }
  JsValueRef initFunction;
  if (JsCallFunction(getInitFunction, nullptr, 0, &initFunction) != JsNoError) {
    return false;
  }
  JsValueRef result;
  JsValueRef arguments[] = { this->keepAliveObject };
  return JsCallFunction(
    initFunction, arguments, _countof(arguments), &result) == JsNoError;
}

bool ContextShim::RegisterCrossContextObject(JsValueRef fakeTarget,
                                             const CrossContextMapInfo& info) {
  // Ensure fakeTarget lifetime encloses proxy lifetime
  if (JsSetProperty(fakeTarget,
                    isolateShim->GetProxySymbolPropertyIdRef(),
                    info.proxy, false) != JsNoError) {
    return false;
  }

  try {
    CrossContextMapInfo* mapInfoCopy = new CrossContextMapInfo(info);

    // Install a finalizer to clean up the map entry when proxy/fakeTarget are
    // collected by GC.
    JsValueRef finalizeObj;
    if (JsCreateExternalObject(mapInfoCopy,
                               CrossContextFakeTargetFinalizeCallback,
                               &finalizeObj) != JsNoError) {
      delete mapInfoCopy;
      return false;
    }

    if (JsSetProperty(fakeTarget,
                      isolateShim->GetFinalizerSymbolPropertyIdRef(),
                      finalizeObj, false) != JsNoError) {
      return false;
    }

    crossContextObjects[info.object].push_back(mapInfoCopy);
    return true;  // success
  } catch (const std::bad_alloc&) {
    return false;
  }
}

bool ContextShim::UnregisterCrossContextObject(
    const CrossContextMapInfo& info) {
  auto i = crossContextObjects.find(info.object);
  if (i != crossContextObjects.end()) {
    std::vector<CrossContextMapInfo*>& v = i->second;
    auto i2 = std::remove_if(v.begin(), v.end(),
                             [info](const CrossContextMapInfo* x) -> bool {
      return x->toContext == info.toContext;
    });

    if (i2 != v.end()) {
      v.erase(i2, v.end());
      if (v.size() == 0) {
        crossContextObjects.erase(i);
      }

      return true;
    }
  }

  assert(false);  // not found in map
  return false;
}

bool ContextShim::TryGetCrossContextObject(JsValueRef object,
                                           ContextShim* toContext,
                                           JsValueRef* proxy) {
  auto i = crossContextObjects.find(object);
  if (i != crossContextObjects.end()) {
    std::vector<CrossContextMapInfo*>& v = i->second;
    auto i2 = std::find_if(v.begin(), v.end(),
                           [=](const CrossContextMapInfo* x) -> bool {
      return x->toContext == toContext;
    });

    if (i2 != v.end()) {
      *proxy = (*i2)->proxy;
      return true;
    }
  }

  return false;
}

void CALLBACK ContextShim::CrossContextFakeTargetFinalizeCallback(
    void *callbackState) {
  CrossContextMapInfo* info = static_cast<CrossContextMapInfo*>(callbackState);

  ContextShim* fromContext = info->fromContext;
  if (fromContext != nullptr) {
    fromContext->UnregisterCrossContextObject(*info);
  }

  delete info;
}

void ContextShim::SetAlignedPointerInEmbedderData(int index, void * value) {
  if (index < 0) {
    return;
  }

  try {
    auto minSize = static_cast<std::vector<void*>::size_type>(index) + 1;
    if (embedderData.size() < minSize) {
      embedderData.resize(minSize);
    }
    embedderData[index] = value;
  } catch (const std::exception&) {
  }
}

JsValueRef ContextShim::GetPromiseContinuationFunction() {
  if (promiseContinuationFunction == JS_INVALID_REFERENCE) {
    JsValueRef process;
    if (jsrt::GetPropertyOfGlobal(L"process", &process) != JsNoError) {
      return JS_INVALID_REFERENCE;
    }

    JsValueRef nextTick;
    if (jsrt::GetProperty(process, L"nextTick", &nextTick) != JsNoError) {
      return JS_INVALID_REFERENCE;
    }

    // CHAKRA-REVIEW: Do we need to root this?
    promiseContinuationFunction = nextTick;  // save in context data
  }
  return promiseContinuationFunction;
}

JsValueRef ContextShim::GetUndefined() {
  return undefinedRef;
}

JsValueRef ContextShim::GetTrue() {
  return trueRef;
}

JsValueRef ContextShim::GetFalse() {
  return falseRef;
}

JsValueRef ContextShim::GetNull() {
  return nullRef;
}

JsValueRef ContextShim::GetZero() {
  return zero;
}

JsValueRef ContextShim::GetObjectConstructor() {
  return globalConstructor[GlobalType::Object];
}

JsValueRef ContextShim::GetBooleanObjectConstructor() {
  return globalConstructor[GlobalType::Boolean];
}

JsValueRef ContextShim::GetNumberObjectConstructor() {
  return globalConstructor[GlobalType::Number];
}

JsValueRef ContextShim::GetStringObjectConstructor() {
  return globalConstructor[GlobalType::String];
}

JsValueRef ContextShim::GetDateConstructor() {
  return globalConstructor[GlobalType::Date];
}

JsValueRef ContextShim::GetRegExpConstructor() {
  return globalConstructor[GlobalType::RegExp];
}

JsValueRef ContextShim::GetProxyConstructor() {
  return globalConstructor[GlobalType::Proxy];
}

JsValueRef ContextShim::GetGetOwnPropertyDescriptorFunction() {
  return getOwnPropertyDescriptorFunction;
}

JsValueRef ContextShim::GetStringConcatFunction() {
  return globalPrototypeFunction[GlobalPrototypeFunction::String_concat];
}

JsValueRef ContextShim::GetGlobalPrototypeFunction(
    GlobalPrototypeFunction index) {
  return globalPrototypeFunction[index];
}

JsValueRef ContextShim::GetProxyOfGlobal() {
  return proxyOfGlobal;
}

JsValueRef ContextShim::GetReflectObject() {
  return reflectObject;
}

JsValueRef ContextShim::GetReflectFunctionForTrap(ProxyTraps trap) {
  return reflectFunctions[trap];
}

JsValueRef ContextShim::GetCachedShimFunction(CachedPropertyIdRef id,
                                              JsValueRef* func) {
  if (*func == JS_INVALID_REFERENCE) {
    // chakra_shim.js has initialize the function an put it in keepAliveObject.
    JsErrorCode error = JsGetProperty(keepAliveObject,
                  GetIsolateShim()->GetCachedPropertyIdRef(id),
                  func);
    CHAKRA_VERIFY(error == JsNoError);
  }

  return *func;
}

JsValueRef ContextShim::GetInstanceOfFunction() {
  return GetCachedShimFunction(CachedPropertyIdRef::isInstanceOf,
                               &instanceOfFunction);
}

JsValueRef ContextShim::GetCloneObjectFunction() {
  return GetCachedShimFunction(CachedPropertyIdRef::cloneObject,
                               &cloneObjectFunction);
}

JsValueRef ContextShim::GetForEachNonConfigurablePropertyFunction() {
  return GetCachedShimFunction(
    CachedPropertyIdRef::forEachNonConfigurableProperty,
    &forEachNonConfigurablePropertyFunction);
}

JsValueRef ContextShim::GetGetPropertyNamesFunction() {
  return GetCachedShimFunction(CachedPropertyIdRef::getPropertyNames,
                               &getPropertyNamesFunction);
}

JsValueRef ContextShim::GetGetEnumerableNamedPropertiesFunction() {
  return GetCachedShimFunction(
    CachedPropertyIdRef::getEnumerableNamedProperties,
    &getEnumerableNamedPropertiesFunction);
}

JsValueRef ContextShim::GetGetEnumerableIndexedPropertiesFunction() {
  return GetCachedShimFunction(
    CachedPropertyIdRef::getEnumerableIndexedProperties,
    &getEnumerableIndexedPropertiesFunction);
}

JsValueRef ContextShim::GetCreateEnumerationIteratorFunction() {
  return GetCachedShimFunction(CachedPropertyIdRef::createEnumerationIterator,
                               &createEnumerationIteratorFunction);
}

JsValueRef
ContextShim::GetCreatePropertyDescriptorsEnumerationIteratorFunction() {
  return GetCachedShimFunction(
    CachedPropertyIdRef::createPropertyDescriptorsEnumerationIterator,
    &createPropertyDescriptorsEnumerationIteratorFunction);
}

JsValueRef ContextShim::GetGetNamedOwnKeysFunction() {
  return GetCachedShimFunction(CachedPropertyIdRef::getNamedOwnKeys,
                               &getNamedOwnKeysFunction);
}

JsValueRef ContextShim::GetGetIndexedOwnKeysFunction() {
  return GetCachedShimFunction(CachedPropertyIdRef::getIndexedOwnKeys,
                               &getIndexedOwnKeysFunction);
}

void ContextShim::EnsureThrowAccessorErrorFunctions() {
  if (throwAccessorErrorFunctions[0] == JS_INVALID_REFERENCE) {
    JsValueRef arr = JS_INVALID_REFERENCE;
    GetCachedShimFunction(CachedPropertyIdRef::throwAccessorErrorFunctions,
                          &arr);
    for (int i = 0; i < THROWACCESSORERRORFUNCTIONS; i++) {
      CHAKRA_VERIFY(jsrt::GetIndexedProperty(
        arr, i, &throwAccessorErrorFunctions[i]) == JsNoError);
    }
  }
}

bool ContextShim::FindThrowAccessorErrorFunction(JsValueRef func, int* index) {
  EnsureThrowAccessorErrorFunctions();

  JsValueRef* end = throwAccessorErrorFunctions + THROWACCESSORERRORFUNCTIONS;
  JsValueRef* p = std::find(throwAccessorErrorFunctions, end, func);
  if (p != end) {
    *index = p - throwAccessorErrorFunctions;
    return true;
  }

  return false;
}

JsValueRef ContextShim::GetThrowAccessorErrorFunction(int index) {
  CHAKRA_ASSERT(index < THROWACCESSORERRORFUNCTIONS);
  EnsureThrowAccessorErrorFunctions();
  return throwAccessorErrorFunctions[index];
}

JsValueRef ContextShim::GetTestFunctionTypeFunction() {
  return GetCachedShimFunction(CachedPropertyIdRef::testFunctionType,
                               &testFunctionTypeFunction);
}

JsValueRef ContextShim::GetCreateTargetFunction() {
  return GetCachedShimFunction(CachedPropertyIdRef::createTargetFunction,
                               &createTargetFunction);
}

}  // namespace jsrt

namespace v8 {
namespace chakrashim {

// This shim wraps Object.prototype.toString to supports ObjectTemplate class
// name.
JsValueRef CALLBACK InternalMethods::ObjectPrototypeToStringShim(
  JsValueRef callee,
  bool isConstructCall,
  JsValueRef *arguments,
  unsigned short argumentCount,
  void *callbackState) {
  if (argumentCount >= 1) {
    using namespace v8;
    Isolate* iso = Isolate::GetCurrent();
    HandleScope scope(iso);

    Object* obj = static_cast<Object*>(arguments[0]);
    Local<String> str = InternalMethods::GetClassName(obj);
    if (!str.IsEmpty()) {
      str = String::Concat(String::NewFromUtf8(iso, "[object "), str);
      str = String::Concat(str, String::NewFromUtf8(iso, "]"));
      return *str;
    }
  }

  JsValueRef function = callbackState;
  JsValueRef result;
  if (JsCallFunction(function,
                     arguments, argumentCount, &result) != JsNoError) {
    return JS_INVALID_REFERENCE;
  }
  return result;
}

}  // namespace chakrashim
}  // namespace v8
