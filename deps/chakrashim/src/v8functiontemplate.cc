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
#include "jsrtutils.h"

namespace v8 {

using jsrt::IsolateShim;
using jsrt::ContextShim;

Object* TemplateData::EnsureProperties() {
  if (properties.IsEmpty()) {
    CreateProperties();
    CHAKRA_VERIFY(!properties.IsEmpty());
  }

  return *properties;
}

void TemplateData::CreateProperties() {
  CHAKRA_ASSERT(false);
}

struct FunctionCallbackData {
  FunctionCallback callback;
  Persistent<Value> data;
  Persistent<Signature> signature;
  Persistent<ObjectTemplate> instanceTemplate;
  Persistent<Object> prototype;

  FunctionCallbackData(FunctionCallback aCallback,
                       Handle<Value> aData,
                       Handle<Signature> aSignature)
      : callback(aCallback),
        data(Persistent<Value>(aData)),
        signature(Persistent<Signature>(aSignature)),
        instanceTemplate(),
        prototype() {
    HandleScope scope;
    instanceTemplate =
      Persistent<ObjectTemplate>(ObjectTemplate::New(Isolate::GetCurrent()));
  }

  static void CALLBACK FinalizeCallback(_In_opt_ void *data) {
    if (data != nullptr) {
      FunctionCallbackData* templateData =
        reinterpret_cast<FunctionCallbackData*>(data);
      templateData->data.Dispose();
      templateData->signature.Dispose();
      templateData->instanceTemplate.Dispose();
      templateData->prototype.Dispose();
      delete templateData;
    }
  }

  bool CheckSignature(Local<Object> thisPointer,
                      JsValueRef *arguments,
                      unsigned short argumentCount,
                      Local<Object>* holder) {
    if (signature.IsEmpty()) {
      *holder = thisPointer;
      return true;
    }

    Local<FunctionTemplate> receiver =
      reinterpret_cast<FunctionTemplate*>(*signature);
    return chakrashim::CheckSignature(receiver, thisPointer, holder);
  }

  static JsValueRef CALLBACK FunctionInvoked(_In_ JsValueRef callee,
                                             _In_ bool isConstructCall,
                                             _In_ JsValueRef *arguments,
                                             _In_ unsigned short argumentCount,
                                             void *callbackState) {
    HandleScope scope;

    JsValueRef functionCallbackDataRef = JsValueRef(callbackState);
    void* externalData;
    if (JsGetExternalData(functionCallbackDataRef,
                          &externalData) != JsNoError) {
      // This should never happen
      return JS_INVALID_REFERENCE;
    }

    FunctionCallbackData *callbackData =
      reinterpret_cast<FunctionCallbackData*>(externalData);

    Local<Object> thisPointer;
    ++arguments;  // skip the this argument

    if (isConstructCall) {
      thisPointer =
        callbackData->instanceTemplate->NewInstance(callbackData->prototype);
      if (thisPointer.IsEmpty()) {
        return JS_INVALID_REFERENCE;
      }
    } else {
      thisPointer = Local<Object>::New(static_cast<Object*>(arguments[-1]));
    }

    if (callbackData->callback != nullptr) {
      Local<Object> holder;
      if (!callbackData->CheckSignature(*thisPointer,
                                        arguments, argumentCount, &holder)) {
        return JS_INVALID_REFERENCE;
      }

      FunctionCallbackInfo<Value> args(
        reinterpret_cast<Value**>(arguments),
        argumentCount - 1,
        thisPointer,
        holder,
        isConstructCall,
        Local<Function>::New(static_cast<Function*>(callee)));

      callbackData->callback(args);
      Handle<Value> result = args.GetReturnValue().Get();

      // if this is a regualr function call return the result, otherwise this is
      // a constructor call return the new instance
      if (!isConstructCall) {
        return *result;
      } else if (!result.IsEmpty()) {
        if (!result->Equals(Undefined()) && !result->Equals(Null())) {
          return *result;
        }
      }
    }

    // no callback is attach just return the new instance
    return *thisPointer;
  }
};

struct FunctionTemplateData : public TemplateData {
  FunctionCallbackData * callbackData;
  Persistent<ObjectTemplate> prototypeTemplate;
  JsValueRef functionTemplate;

  explicit FunctionTemplateData(FunctionCallbackData * callbackData)
      : prototypeTemplate() {
    this->callbackData = callbackData;
    this->functionTemplate = JS_INVALID_REFERENCE;
    this->prototypeTemplate =
      Persistent<ObjectTemplate>(ObjectTemplate::New(Isolate::GetCurrent()));
  }

  // Create the function lazily so that we can use the class name
  virtual void CreateProperties() {
    JsValueRef funcCallbackObjectRef;
    JsErrorCode error = JsCreateExternalObject(
      callbackData,
      FunctionCallbackData::FinalizeCallback,
      &funcCallbackObjectRef);
    if (error != JsNoError) {
      return;
    }

    JsValueRef function;
    {
      Handle<String> className = chakrashim::InternalMethods::GetClassName(
        *callbackData->instanceTemplate);
      if (!className.IsEmpty()) {
        error = JsCreateNamedFunction(*className,
                                      FunctionCallbackData::FunctionInvoked,
                                      funcCallbackObjectRef, &function);
      } else {
        error = JsCreateFunction(FunctionCallbackData::FunctionInvoked,
                                 funcCallbackObjectRef, &function);
      }

      if (error != JsNoError) {
        return;
      }
    }

    this->properties = function;
  }

  static void CALLBACK FinalizeCallback(_In_opt_ void *data) {
    if (data != nullptr) {
      FunctionTemplateData * templateData =
        reinterpret_cast<FunctionTemplateData*>(data);
      if (templateData->properties.IsEmpty()) {
        // function not created, delete callbackData explictly
        delete templateData->callbackData;
      }
      templateData->properties.Dispose();
      templateData->prototypeTemplate.Dispose();
      IsolateShim::GetCurrent()->UnregisterJsValueRefContextShim(
        templateData->functionTemplate);
      delete templateData;
    }
  }
};

Local<FunctionTemplate> FunctionTemplate::New(Isolate* isolate,
                                              FunctionCallback callback,
                                              v8::Handle<Value> data,
                                              v8::Handle<Signature> signature,
                                              int length) {
  FunctionCallbackData* callbackData =
    new FunctionCallbackData(callback, data, signature);
  FunctionTemplateData * templateData =
    new FunctionTemplateData(callbackData);
  JsValueRef functionTemplateRef;
  JsErrorCode error = JsCreateExternalObject(
    templateData, FunctionTemplateData::FinalizeCallback, &functionTemplateRef);
  if (error != JsNoError) {
    delete callbackData;
    delete templateData;
    return Local<FunctionTemplate>();
  }
  templateData->functionTemplate = functionTemplateRef;
  IsolateShim::FromIsolate(isolate)->RegisterJsValueRefContextShim(
    functionTemplateRef);
  return Local<FunctionTemplate>::New(
    static_cast<FunctionTemplate*>(functionTemplateRef));
}

Local<Function> FunctionTemplate::GetFunction() {
  void* externalData;
  if (JsGetExternalData(this, &externalData) != JsNoError) {
    return Local<Function>();
  }

  FunctionTemplateData *functionTemplateData =
    reinterpret_cast<FunctionTemplateData*>(externalData);
  FunctionCallbackData * functionCallbackData =
    functionTemplateData->callbackData;

  Local<Function> function =
    static_cast<Function*>(functionTemplateData->EnsureProperties());

  if (functionCallbackData->prototype.IsEmpty()) {
    functionCallbackData->prototype = Persistent<Object>(
      functionTemplateData->prototypeTemplate->NewInstance());

    if (functionCallbackData->prototype.IsEmpty() ||
        jsrt::SetProperty(*functionCallbackData->prototype,
                          L"constructor", *function) != JsNoError) {
      return Local<Function>();
    }

    function->Set(String::New(L"prototype"), functionCallbackData->prototype);
  }

  return function;
}

Local<ObjectTemplate> FunctionTemplate::InstanceTemplate() {
  void *externalData;
  if (JsGetExternalData(this, &externalData) != JsNoError) {
    return Local<ObjectTemplate>();
  }

  FunctionTemplateData *functionTemplateData =
    reinterpret_cast<FunctionTemplateData*>(externalData);
  FunctionCallbackData * functionCallbackData =
    functionTemplateData->callbackData;
  return Local<ObjectTemplate>::New(*functionCallbackData->instanceTemplate);
}

Local<ObjectTemplate> FunctionTemplate::PrototypeTemplate() {
  void *externalData;
  if (JsGetExternalData(this, &externalData) != JsNoError) {
    return Local<ObjectTemplate>();
  }

  FunctionTemplateData *functionTemplateData =
    reinterpret_cast<FunctionTemplateData*>(externalData);

  if (functionTemplateData->prototypeTemplate.IsEmpty()) {
    functionTemplateData->prototypeTemplate =
      Persistent<ObjectTemplate>(ObjectTemplate::New(Isolate::GetCurrent()));
  }

  // The V8 specs are silent on what's supposed to happen here if the function
  // has been created. If you try and modify the prototype template, what's
  // supposed to happen given that the prototype object must have already been
  // created?

  return Local<ObjectTemplate>::New(*functionTemplateData->prototypeTemplate);
}

void FunctionTemplate::SetClassName(Handle<String> name) {
  void *externalData;
  if (JsGetExternalData(this, &externalData) != JsNoError) {
    return;
  }

  FunctionTemplateData *functionTemplateData =
    reinterpret_cast<FunctionTemplateData*>(externalData);
  FunctionCallbackData * functionCallbackData =
    functionTemplateData->callbackData;
  functionCallbackData->instanceTemplate->SetClassName(name);
}

void FunctionTemplate::SetHiddenPrototype(bool value) {
  // CHAKRA-TODO
}

bool FunctionTemplate::HasInstance(Handle<Value> object) {
  return ContextShim::ExecuteInContextOf<bool>(this, [&]() {
    void *externalData;
    if (JsGetExternalData(this, &externalData) != JsNoError) {
      return false;
    }

    FunctionTemplateData *functionTemplateData =
      reinterpret_cast<FunctionTemplateData*>(externalData);
    FunctionCallbackData * functionCallbackData =
      functionTemplateData->callbackData;

    bool result;
    if (jsrt::InstanceOf(*object, *GetFunction(), &result) != JsNoError) {
      return false;
    }

    return result;
  });
}

}  // namespace v8
