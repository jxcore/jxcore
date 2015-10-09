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
#include <memory>

namespace v8 {

__declspec(thread) JsSourceContext currentContext;

Local<Script> Script::Compile(Handle<String> source, ScriptOrigin* origin) {
  return Compile(source, origin->ResourceName());
}

// Create a object to hold the script infomration
static JsErrorCode CreateScriptObject(JsValueRef sourceRef,
                                      JsValueRef filenameRef,
                                      JsValueRef scriptFunction,
                                      JsValueRef * scriptObject) {
  JsErrorCode error = JsCreateObject(scriptObject);
  if (error != JsNoError) {
    return error;
  }

  error = jsrt::SetProperty(*scriptObject, L"source", sourceRef);
  if (error != JsNoError) {
    return error;
  }

  error = jsrt::SetProperty(*scriptObject, L"filename", filenameRef);
  if (error != JsNoError) {
    return error;
  }

  return jsrt::SetProperty(*scriptObject, L"function", scriptFunction);
}

// Compiled script object, bound to the context that was active when this
// function was called. When run it will always use this context.
Local<Script> Script::Compile(Handle<String> source, Handle<String> file_name) {
  JsErrorCode error;
  JsValueRef filenameRef;
  const wchar_t* filename;
  error = jsrt::ToString(*file_name, &filenameRef, &filename);

  if (error == JsNoError) {
    JsValueRef sourceRef;
    const wchar_t *script;
    error = jsrt::ToString(*source, &sourceRef, &script);

    if (error == JsNoError) {
      JsValueRef scriptFunction;
      error = JsParseScript(script,
        currentContext++, filename, &scriptFunction);

      if (error == JsNoError) {
        JsValueRef scriptObject;
        error = CreateScriptObject(sourceRef,
          filenameRef,
          scriptFunction,
          &scriptObject);

        if (error == JsNoError) {
          return Local<Script>::New(static_cast<Script *>(scriptObject));
        }
      }
    }
  }

  jsrt::SetOutOfMemoryErrorIfExist(error);

  return Local<Script>();
}

Local<Value> Script::Run() {
  JsValueRef scriptFunction;
  if (jsrt::GetProperty(this, L"function", &scriptFunction) != JsNoError) {
    return Local<Value>();
  }

  JsValueRef result;
  JsErrorCode errorCode = JsCallFunction(scriptFunction, nullptr, 0, &result);

  jsrt::SetOutOfMemoryErrorIfExist(errorCode);

  if (errorCode != JsNoError) {
    return Local<Value>();
  }

  return Local<Value>::New(static_cast<Value *>(result));
}

static void CALLBACK UnboundScriptFinalizeCallback(void * data) {
  JsValueRef * unboundScriptData = static_cast<JsValueRef *>(data);
  jsrt::IsolateShim::GetCurrent()->UnregisterJsValueRefContextShim(
    *unboundScriptData);
  delete unboundScriptData;
}
Local<UnboundScript> Script::GetUnboundScript() {
  // Chakra doesn't support unbound script, the script object contains all the
  // information to recompile

  JsValueRef * unboundScriptData = new JsValueRef;
  JsValueRef unboundScriptRef;
  if (JsCreateExternalObject(unboundScriptData,
                             UnboundScriptFinalizeCallback,
                             &unboundScriptRef) != JsNoError) {
    delete unboundScriptData;
    return Local<UnboundScript>();
  }

  if (jsrt::SetProperty(unboundScriptRef, L"script", this) != JsNoError) {
    delete unboundScriptData;
    return Local<UnboundScript>();
  }
  *unboundScriptData = unboundScriptRef;

  // CHAKRA-REVIEW: Since chakra doesn't allow access of object from another
  // context, we need to keep track of the context the unbound script is
  jsrt::IsolateShim::GetCurrent()->RegisterJsValueRefContextShim(
    unboundScriptRef);
  return Local<UnboundScript>(static_cast<UnboundScript*>(unboundScriptRef));
}

Local<Script> UnboundScript::BindToCurrentContext() {
  jsrt::ContextShim * contextShim =
    jsrt::IsolateShim::GetCurrent()->GetJsValueRefContextShim(this);
  if (contextShim == jsrt::ContextShim::GetCurrent()) {
    JsValueRef scriptRef;
    if (jsrt::GetProperty(this, L"script", &scriptRef) != JsNoError) {
      return Local<Script>();
    }
    // Same context, we can reuse the same script object
    return Local<Script>(static_cast<Script *>(scriptRef));
  }

  // Create a script object in another context
  const wchar_t * source;
  size_t sourceLength;
  const wchar_t * filename;
  size_t filenameLength;

  {
    jsrt::ContextShim::Scope scope(contextShim);
    JsValueRef scriptRef;
    if (jsrt::GetProperty(this, L"script", &scriptRef) != JsNoError) {
      return Local<Script>();
    }

    JsValueRef originalSourceRef;
    if (jsrt::GetProperty(scriptRef,
                          L"source", &originalSourceRef) != JsNoError) {
      return Local<Script>();
    }
    JsValueRef originalFilenameRef;
    if (jsrt::GetProperty(scriptRef,
                          L"filename", &originalFilenameRef) != JsNoError) {
      return Local<Script>();
    }
    if (JsStringToPointer(originalSourceRef,
                          &source, &sourceLength) != JsNoError) {
      return Local<Script>();
    }
    if (JsStringToPointer(originalFilenameRef,
                          &filename, &filenameLength) != JsNoError) {
      return Local<Script>();
    }
  }

  JsValueRef scriptFunction;
  if (JsParseScript(source,
                    currentContext++, filename, &scriptFunction) != JsNoError) {
    return Local<Script>();
  }

  JsValueRef sourceRef;
  if (JsPointerToString(source, sourceLength, &sourceRef) != JsNoError) {
    return Local<Script>();
  }

  JsValueRef filenameRef;
  if (JsPointerToString(filename, filenameLength, &filenameRef) != JsNoError) {
    return Local<Script>();
  }

  JsValueRef scriptObject;
  if (CreateScriptObject(sourceRef,
                         filenameRef,
                         scriptFunction,
                         &scriptObject) != JsNoError) {
    return Local<Script>();
  }

  return Local<Script>(static_cast<Script*>(scriptObject));
}

Local<UnboundScript> ScriptCompiler::CompileUnbound(
    Isolate* isolate, Source* source, CompileOptions options) {
  Local<Script> script = Compile(isolate, source, options);
  if (script.IsEmpty()) {
    return Local<UnboundScript>();
  }
  return script->GetUnboundScript();
}

Local<Script> ScriptCompiler::Compile(
    Isolate* isolate, Source* source, CompileOptions options) {
  return Script::Compile(source->source_string, source->resource_name);
}

}  // namespace v8
