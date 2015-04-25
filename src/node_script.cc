// Copyright & License details are available under JXCORE_LICENSE file

#include "node.h"
#include "node_script.h"
#include <assert.h>
#include "jx/commons.h"
#include "jx/memory_store.h"

namespace node {

class WrappedContext : ObjectWrap {
 public:
  INIT_NAMED_CLASS_MEMBERS(Context, WrappedContext) {
    com->wc_constructor_template =
        JS_NEW_PERSISTENT_FUNCTION_TEMPLATE(constructor);
  }
  END_INIT_NAMED_MEMBERS(Context)

  static DEFINE_JS_METHOD(New);

  JS_PERSISTENT_CONTEXT GetV8Context();
  static JS_LOCAL_OBJECT NewInstance(commons *com = NULL);
  static bool InstanceOf(JS_HANDLE_VALUE value, commons *com = NULL);

 protected:
  WrappedContext();
  ~WrappedContext();

  JS_PERSISTENT_CONTEXT context_;
};

class WrappedScript : ObjectWrap {
 public:
  INIT_NAMED_CLASS_MEMBERS(NodeScript, WrappedScript) {
    SET_INSTANCE_METHOD("createContext", WrappedScript::CreateContext, 0);
    SET_INSTANCE_METHOD("runInContext", WrappedScript::RunInContext, 0);
    SET_INSTANCE_METHOD("runInThisContext", WrappedScript::RunInThisContext, 0);
    SET_INSTANCE_METHOD("runInNewContext", WrappedScript::RunInNewContext, 0);

    SET_CLASS_METHOD("createContext", WrappedScript::CreateContext, 0);
    SET_CLASS_METHOD("runInContext", WrappedScript::CompileRunInContext, 0);
    SET_CLASS_METHOD("runInThisContext", WrappedScript::CompileRunInThisContext,
                     0);
    SET_CLASS_METHOD("runInNewContext", WrappedScript::CompileRunInNewContext,
                     0);

    com->ws_constructor_template =
        JS_NEW_PERSISTENT_FUNCTION_TEMPLATE(constructor);
  }
  END_INIT_NAMED_MEMBERS(NodeScript)

  enum EvalInputFlags {
    compileCode,
    unwrapExternal
  };
  enum EvalContextFlags {
    thisContext,
    newContext,
    userContext
  };
  enum EvalOutputFlags {
    returnResult,
    wrapExternal
  };

  template <EvalInputFlags input_flag, EvalContextFlags context_flag,
            EvalOutputFlags output_flag>
  static JS_NATIVE_RETURN_TYPE EvalMachine(jxcore::PArguments &args,
#ifdef JS_ENGINE_V8
                                           JS_HANDLE_OBJECT _this);
#elif defined(JS_ENGINE_MOZJS)
                                           const JS_HANDLE_OBJECT &_this);
#endif

 protected:
  WrappedScript() : ObjectWrap() {}
  ~WrappedScript();

  static DEFINE_JS_METHOD(New);
  static DEFINE_JS_METHOD(CreateContext);
  static DEFINE_JS_METHOD(RunInContext);
  static DEFINE_JS_METHOD(RunInThisContext);
  static DEFINE_JS_METHOD(RunInNewContext);
  static DEFINE_JS_METHOD(CompileRunInContext);
  static DEFINE_JS_METHOD(CompileRunInThisContext);
  static DEFINE_JS_METHOD(CompileRunInNewContext);

#ifdef JS_ENGINE_V8
  JS_PERSISTENT_SCRIPT script_;
#elif defined(JS_ENGINE_MOZJS)
  jxcore::MemoryScript script_;
#endif
};

void CloneObject(JS_STATE_MARKER, JS_HANDLE_OBJECT recv,
                 JS_HANDLE_VALUE_REF source, JS_HANDLE_VALUE_REF target) {
  JS_HANDLE_VALUE args[] = {source, target};

// Init
#ifdef JS_ENGINE_V8
  JS_ENTER_SCOPE_COM();
  if (JS_IS_EMPTY((com->cloneObjectMethod))) {
    JS_LOCAL_FUNCTION cloneObjectMethod_ = JS_CAST_FUNCTION(JS_COMPILE_AND_RUN(
        STD_TO_STRING(
            "(function(source, target) {\n"
            "if(!target) target = global;\n"
            "if(Array.isArray(source)){for(var o in "
            "source){target[o]=source[o];}return;}\n"
            "Object.getOwnPropertyNames(source).forEach(function(key) {\n"
            "try {\n"
            "var desc = Object.getOwnPropertyDescriptor(source, key);\n"
            "if (desc.value === source) desc.value = target;\n"
            "Object.defineProperty(target, key, desc);\n"
            "} catch (e) {\n"
            " // Catch sealed properties errors\n"
            "}\n"
            "});\n"
            "})"),
        STD_TO_STRING("binding:script")));
    com->cloneObjectMethod = JS_NEW_PERSISTENT_FUNCTION(cloneObjectMethod_);
  }
  JS_METHOD_CALL(com->cloneObjectMethod, recv, 2, args);

#elif defined(JS_ENGINE_MOZJS)
  MozJS::Value global = JS_GET_CONTEXT_GLOBAL(__contextORisolate);
  JS_LOCAL_FUNCTION cloneObjectMethod_ = MozJS::Value::CompileAndRun(
      __contextORisolate,
      STD_TO_STRING(
          "(function(source, target) {\n"
          "  if (!target) target = global;\n"
          "    if (Array.isArray(source)) {\n"
          "      for(var o in source) {\n"
          "        target[o]=source[o];\n"
          "      }\n"
          "    return;\n"
          "  }\n"
          "Object.getOwnPropertyNames(source).forEach(function(key) {\n"
          "  try {\n"
          "    var desc = Object.getOwnPropertyDescriptor(source, key);\n"
          "    if (desc.value === source) desc.value = target;\n"
          "      Object.defineProperty(target, key, desc);\n"
          "  } catch (e) {\n"
          "     // Catch sealed properties errors\n"
          "  }\n"
          "});\n"
          "})"),
      STD_TO_STRING("binding:script"), &global);
  JS_METHOD_CALL(cloneObjectMethod_, recv, 2, args);
#endif
}

bool WrappedContext::InstanceOf(JS_HANDLE_VALUE value, commons *com) {
  if (com == NULL) com = node::commons::getInstance();

  return !JS_IS_EMPTY(value) &&
         JS_HAS_INSTANCE(com->wc_constructor_template, value);
}

JS_METHOD(WrappedContext, New) {
  JS_CLASS_NEW_INSTANCE(obj, Context);
  WrappedContext *t = new WrappedContext();
  t->Wrap(obj);

  RETURN_PARAM(obj);
}
JS_METHOD_END

WrappedContext::WrappedContext() : ObjectWrap() {
#ifdef JS_ENGINE_V8
  context_ = JS_NEW_EMPTY_CONTEXT();
#endif
}

WrappedContext::~WrappedContext() {
#ifdef JS_ENGINE_V8
  JS_DISPOSE_PERSISTENT_CONTEXT(context_);
#endif
}

JS_LOCAL_OBJECT WrappedContext::NewInstance(commons *com) {
  if (com == NULL) com = node::commons::getInstance();
  JS_LOCAL_OBJECT context =
      JS_NEW_DEFAULT_INSTANCE(JS_GET_FUNCTION(com->wc_constructor_template));
  return context;
}

JS_PERSISTENT_CONTEXT WrappedContext::GetV8Context() {
#ifdef JS_ENGINE_V8
  return context_;
#elif defined(JS_ENGINE_MOZJS)
  assert(0 && "This function should not be called for MozJS implementation");
  return NULL;
#endif
}

JS_METHOD(WrappedScript, New) {
  if (!args.IsConstructCall()) {
    RETURN_PARAM(FromConstructorTemplateX(com->ws_constructor_template, args));
  }

  JS_CLASS_NEW_INSTANCE(obj, NodeScript);
  WrappedScript *t = new WrappedScript();
  t->Wrap(obj);

#ifdef JS_ENGINE_V8
  JS_HANDLE_VALUE param =
      WrappedScript::EvalMachine<compileCode, thisContext, wrapExternal>(args,
                                                                         obj);
  RETURN_PARAM(param);
#elif defined(JS_ENGINE_MOZJS)
  bool param =
      WrappedScript::EvalMachine<compileCode, thisContext, wrapExternal>(args,
                                                                         obj);
  return (param);
#endif
}
JS_METHOD_END

WrappedScript::~WrappedScript() { script_.Dispose(); }

JS_METHOD(WrappedScript, CreateContext) {
  JS_LOCAL_OBJECT context = WrappedContext::NewInstance(com);

  if (args.Length() > 0) {
    if (args.IsObject(0)) {
      JS_LOCAL_OBJECT sandbox = JS_VALUE_TO_OBJECT(args.GetItem(0));

      CloneObject(JS_GET_STATE_MARKER(), args.This(), sandbox, context);
    } else {
      THROW_TYPE_EXCEPTION(
          "createContext() accept only object as first argument.");
    }
  }

  RETURN_POINTER(context);
}
JS_METHOD_END

JS_METHOD(WrappedScript, RunInContext) {
  JS_HANDLE_OBJECT holder = args.Holder();
  JS_NATIVE_RETURN_TYPE param =
      WrappedScript::EvalMachine<unwrapExternal, userContext, returnResult>(
          args, holder);
  return JS_LEAVE_SCOPE(param);
}
JS_METHOD_END

JS_METHOD(WrappedScript, RunInThisContext) {
  JS_HANDLE_OBJECT holder = args.Holder();
  JS_NATIVE_RETURN_TYPE param =
      WrappedScript::EvalMachine<unwrapExternal, thisContext, returnResult>(
          args, holder);
  return JS_LEAVE_SCOPE(param);
}
JS_METHOD_END

JS_METHOD(WrappedScript, RunInNewContext) {
  JS_HANDLE_OBJECT holder = args.Holder();
  JS_NATIVE_RETURN_TYPE param =
      WrappedScript::EvalMachine<unwrapExternal, newContext, returnResult>(
          args, holder);
  return JS_LEAVE_SCOPE(param);
}
JS_METHOD_END

JS_METHOD(WrappedScript, CompileRunInContext) {
  JS_HANDLE_OBJECT holder = args.Holder();
  JS_NATIVE_RETURN_TYPE param =
      WrappedScript::EvalMachine<compileCode, userContext, returnResult>(
          args, holder);
  return JS_LEAVE_SCOPE(param);
}
JS_METHOD_END

JS_METHOD(WrappedScript, CompileRunInThisContext) {
  JS_HANDLE_OBJECT holder = args.Holder();
  JS_NATIVE_RETURN_TYPE param =
      WrappedScript::EvalMachine<compileCode, thisContext, returnResult>(
          args, holder);
  return JS_LEAVE_SCOPE(param);
}
JS_METHOD_END

JS_METHOD(WrappedScript, CompileRunInNewContext) {
  JS_HANDLE_OBJECT holder = args.Holder();
  JS_NATIVE_RETURN_TYPE param =
      WrappedScript::EvalMachine<compileCode, newContext, returnResult>(args,
                                                                        holder);
  return JS_LEAVE_SCOPE(param);
}
JS_METHOD_END

#ifdef JS_ENGINE_V8
template <WrappedScript::EvalInputFlags input_flag,
          WrappedScript::EvalContextFlags context_flag,
          WrappedScript::EvalOutputFlags output_flag>
JS_NATIVE_RETURN_TYPE WrappedScript::EvalMachine(jxcore::PArguments &args,
                                                 JS_HANDLE_OBJECT _this) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  if (input_flag == compileCode && args.Length() < 1) {
    THROW_TYPE_EXCEPTION("needs at least 'code' argument.");
  }

  const int sandbox_index = input_flag == compileCode ? 1 : 0;
  if (context_flag == userContext &&
      !WrappedContext::InstanceOf(args.GetItem(sandbox_index), com)) {
    THROW_TYPE_EXCEPTION("needs a 'context' argument.");
  }

  JS_LOCAL_STRING code;
  if (input_flag == compileCode) {
    code = JS_VALUE_TO_STRING(args.GetItem(0));
  }

  JS_LOCAL_OBJECT sandbox;
  if (context_flag == newContext) {
    sandbox = args.IsObject(sandbox_index)
                  ? JS_VALUE_TO_OBJECT(args.GetItem(sandbox_index))
                  : JS_NEW_EMPTY_OBJECT();
  } else if (context_flag == userContext) {
    sandbox = JS_VALUE_TO_OBJECT(args.GetItem(sandbox_index));
  }

  const int filename_index =
      sandbox_index + (context_flag == thisContext ? 0 : 1);
  JS_LOCAL_STRING filename =
      args.Length() > filename_index
          ? JS_VALUE_TO_STRING(args.GetItem(filename_index))
          : STD_TO_STRING("evalmachine.<anonymous>");

  const int display_error_index = args.Length() - 1;
  bool display_error = false;
  // This hack doesn't needed for MozJS
  if (args.Length() > display_error_index &&
      args.IsBoolean(display_error_index) &&
      args.GetBoolean(display_error_index) == true) {
    display_error = true;
  }

  JS_HANDLE_CONTEXT context = JS_CURRENT_CONTEXT();

  JS_LOCAL_ARRAY keys;
  if (context_flag == newContext) {
    // Create the new context
    // Context::New returns a Persistent<Context>, but we only need it for this
    // function. Here we grab a temporary handle to the new context, assign it
    // to a local handle, and then dispose the persistent handle. This ensures
    // that when this function exits the context will be disposed.
    JS_PERSISTENT_CONTEXT tmp = JS_NEW_EMPTY_CONTEXT();
    context = JS_NEW_LOCAL_CONTEXT(tmp);
    JS_DISPOSE_PERSISTENT_CONTEXT(tmp);
  } else if (context_flag == userContext) {
    // Use the passed in context
    WrappedContext *nContext = ObjectWrap::Unwrap<WrappedContext>(sandbox);
    context = JS_TYPE_TO_LOCAL_CONTEXT(nContext->GetV8Context());
  }
  ENGINE_NS::Context::Scope context_scope(context);

  // New and user context share code. DRY it up.
  if (context_flag == userContext || context_flag == newContext) {
    // Copy everything from the passed in sandbox (either the persistent
    // context for runInContext(), or the sandbox arg to runInNewContext()).
    CloneObject(JS_GET_STATE_MARKER(), args.This(), sandbox,
                context->Global()->GetPrototype());
  }
  // Catch errors
  JS_TRY_CATCH(try_catch);
  JS_HANDLE_VALUE result;
  JS_HANDLE_SCRIPT script;

  if (input_flag == compileCode) {
    // well, here WrappedScript::New would suffice in all cases, but maybe
    // Compile has a little better performance where possible
    script = output_flag == returnResult
                 ? JS_SCRIPT_COMPILE(code, filename)
                 : ENGINE_NS::Script::New(code, filename);
    if (JS_IS_EMPTY(script)) {
      // v8 Only hack
      if (display_error) DisplayExceptionLine(try_catch);

      // Hack because I can't get a proper stacktrace on SyntaxError
      return try_catch.ReThrow();
    }
  } else {
    WrappedScript *n_script = ObjectWrap::Unwrap<WrappedScript>(_this);
    if (!n_script) {
      THROW_EXCEPTION("Must be called as a method of Script.");
    } else if (JS_IS_EMPTY((n_script->script_))) {
      THROW_EXCEPTION(
          "'this' must be a result of previous "
          "new Script(code) call.");
    }

    script = n_script->script_;
  }

  if (output_flag == returnResult) {
    result = JS_SCRIPT_RUN(script);
    if (JS_IS_EMPTY(result)) {
      if (display_error) DisplayExceptionLine(try_catch);
      return try_catch.ReThrow();
    }
  } else {
    WrappedScript *n_script = ObjectWrap::Unwrap<WrappedScript>(_this);
    if (!n_script) {
      THROW_EXCEPTION("Must be called as a method of Script.");
    }
    n_script->script_ = JS_NEW_PERSISTENT_SCRIPT(script);
    result = _this;
  }

  if (context_flag == userContext || context_flag == newContext) {
    // success! copy changes back onto the sandbox object.
    CloneObject(JS_GET_STATE_MARKER(), args.This(),
                context->Global()->GetPrototype(), sandbox);
  }

  return result == args.This() ? result : JS_LEAVE_SCOPE(result);
}
#elif defined(JS_ENGINE_MOZJS)
static void CrossCompartmentClone(MozJS::Value &orig_obj, JSContext *newc,
                                  JS::MutableHandleObject retval) {
  JSContext *context = orig_obj.GetContext();
  JS::RootedObject rt_ts(context);
  jxcore::NewTransplantObject(context, &rt_ts);
  MozJS::Value target_sandbox(rt_ts, context);

  jxcore::CrossCompartmentCopy(context, newc, target_sandbox, false, retval);

  CloneObject(context, target_sandbox, orig_obj, target_sandbox);
}

template <WrappedScript::EvalInputFlags input_flag,
          WrappedScript::EvalContextFlags context_flag,
          WrappedScript::EvalOutputFlags output_flag>
JS_NATIVE_RETURN_TYPE WrappedScript::EvalMachine(
    jxcore::PArguments &args, const JS_HANDLE_OBJECT_REF _this) {
  JSContext *__contextORisolate = args.GetContext();
  node::commons *com =
      node::commons::getInstanceByThreadId(JS_GetThreadId(__contextORisolate));

  if (input_flag == compileCode && args.Length() < 1) {
    THROW_TYPE_EXCEPTION("needs at least 'code' argument.");
  }

  const int sandbox_index = input_flag == compileCode ? 1 : 0;
  if (context_flag == userContext &&
      !WrappedContext::InstanceOf(args.GetItem(sandbox_index), com)) {
    THROW_TYPE_EXCEPTION("needs a 'context' argument.");
  }

  JS_LOCAL_STRING code;
  if (input_flag == compileCode) {
    code = JS_VALUE_TO_STRING(args.GetItem(0));
  }

  JS_LOCAL_OBJECT sandbox;
  if (context_flag == newContext) {
    sandbox = args.IsObject(sandbox_index)
                  ? JS_VALUE_TO_OBJECT(args.GetItem(sandbox_index))
                  : JS_NEW_EMPTY_OBJECT();
  } else if (context_flag == userContext) {
    sandbox = JS_VALUE_TO_OBJECT(args.GetItem(sandbox_index));
  }

  const int filename_index =
      sandbox_index + (context_flag == thisContext ? 0 : 1);
  JS_LOCAL_STRING filename =
      args.Length() > filename_index
          ? JS_VALUE_TO_STRING(args.GetItem(filename_index))
          : STD_TO_STRING("evalmachine.<anonymous>");

  JS_HANDLE_CONTEXT context = __contextORisolate;
  MozJS::Isolate *iso = NULL;

  if (context_flag == newContext || context_flag == userContext) {
    iso = JS_NEW_EMPTY_CONTEXT();
    context = iso->GetRaw();
    JS_SetErrorReporter(context, node::OnFatalError);
  }

  // Catch errors
  JS_TRY_CATCH(try_catch);

  JSObject *fake_sandbox = NULL;
  JSCompartment *comp = NULL;

#define LEAVE_COMPARTMENT(copy_result)                                      \
  if (fake_sandbox != NULL) {                                               \
    JS_LOCAL_VALUE result_backup;                                           \
    if (copy_result) {                                                      \
      result_backup = result;                                               \
      JS::RootedObject rt_ts(context);                                      \
      jxcore::NewTransplantObject(context, &rt_ts);                         \
      result = JS_LOCAL_VALUE(rt_ts, context);                              \
      JS_NAME_SET(result, JS_STRING_ID("result"), result_backup);           \
    }                                                                       \
    MozJS::Value source_sandbox(fake_sandbox, context);                     \
    JS::RootedObject cs_sandbox(__contextORisolate);                        \
    CrossCompartmentClone(source_sandbox, __contextORisolate, &cs_sandbox); \
    JS::RootedObject result_sandbox(__contextORisolate);                    \
    if (copy_result) {                                                      \
      CrossCompartmentClone(result, __contextORisolate, &result_sandbox);   \
    }                                                                       \
    JS_LeaveCompartment(context, comp);                                     \
    MozJS::Value comp_sandbox(cs_sandbox, __contextORisolate);              \
    CloneObject(__contextORisolate, sandbox, comp_sandbox, sandbox);        \
    if (result_sandbox != nullptr) {                                        \
      result = JS_LOCAL_OBJECT(result_sandbox, __contextORisolate);         \
      result = JS_GET_NAME(result, JS_STRING_ID("result"));                 \
    }                                                                       \
    JS_DestroyContext(context);                                             \
    iso->Dispose();                                                         \
  }

  JS_HANDLE_VALUE result;
  jxcore::MemoryScript mscript;

  if (input_flag == compileCode) {
    // well, here WrappedScript::New would suffice in all cases, but maybe
    // Compile has a little better performance where possible
    JS_LOCAL_VALUE globals;
    if (context_flag == userContext || context_flag == newContext) {
      globals = sandbox;
    } else {
      globals = jxcore::getGlobal(com->threadId);
    }

    JS_HANDLE_SCRIPT script =
        MozJS::Script::Compile(__contextORisolate, globals, code, filename);

    if (JS_IS_EMPTY(script)) {
      JS_LOCAL_VALUE err_val = try_catch.Exception();
      THROW_EXCEPTION_OBJECT(err_val);
    }

    mscript = jxcore::GetScriptMemory(__contextORisolate,
                                      script.GetRawScriptPointer());
  } else {
    WrappedScript *n_script = ObjectWrap::Unwrap<WrappedScript>(_this);
    if (!n_script) {
      THROW_EXCEPTION("Must be called as a method of Script.");
    }

    if (JS_IS_EMPTY((n_script->script_))) {
      THROW_EXCEPTION(
          "'this' must be a result of previous "
          "new Script(code) call.");
    }

    mscript = n_script->script_;
  }

  if (output_flag == returnResult) {
    std::string ex_message = "", ex_stack = "";
    if (context_flag == userContext || context_flag == newContext) {
      JS::RootedObject rt_ts(__contextORisolate);
      jxcore::NewTransplantObject(__contextORisolate, &rt_ts);
      JS_LOCAL_OBJECT sandbox_backup(rt_ts, __contextORisolate);

      JS::RootedObject prep(__contextORisolate);
      jxcore::CrossCompartmentCopy(__contextORisolate, context, sandbox_backup,
                                   true, &prep);

      fake_sandbox = prep.get();

      CloneObject(__contextORisolate, sandbox_backup, sandbox, sandbox_backup);

      comp = JS_EnterCompartment(context, fake_sandbox);
      try_catch.ctx_ = context;
      JSScript *new_script = jxcore::GetScript(context, mscript);
      MozJS::Script script_pr(new_script, context);

      JS_LOCAL_OBJECT obj_fs(fake_sandbox, context);
      result = script_pr.Run(obj_fs);

      if (!JS_IS_EMPTY(com->temp_exception_)) {
        JS::RootedValue rt_ex(context, com->temp_exception_.GetRawValue());
        JS_LOCAL_VALUE msg_ = com->temp_exception_.Get("message");
        if (!msg_.IsEmpty()) ex_message += STRING_TO_STD(msg_);

        JS_LOCAL_VALUE stk_ = com->temp_exception_.Get("stack");
        if (!stk_.IsEmpty()) ex_stack += STRING_TO_STD(stk_);

        com->temp_exception_.Clear();
      }
    } else {
      MozJS::Script script(jxcore::GetScript(context, mscript), context);
      result = script.Run();
    }

    if (JS_IS_EMPTY(result)) {
      JS_LOCAL_VALUE err_val;
      bool catched_error = try_catch.HasCaught();
      if (!catched_error) {
        LEAVE_COMPARTMENT(false);

        jxcore::JXString fname(filename);
        std::string str = "Uncaught exception at ";
        str += *fname;
        if (ex_message.length() != 0) {
          str += "\n\t";
          str += ex_message;
        }

        err_val = ENGINE_NS::Exception::Error(STD_TO_STRING(str.c_str()))
                      .GetErrorObject();

        if (ex_stack.length() == 0) {
          JS_NAME_SET(err_val, "stack", filename);
        } else {
          JS_LOCAL_STRING stack_str = UTF8_TO_STRING(ex_stack.c_str());
          JS_NAME_SET(err_val, "stack", stack_str);
        }

        THROW_EXCEPTION_OBJECT(err_val);
      }

      err_val = try_catch.Exception();
      JS::RootedObject new_error(__contextORisolate);
      CrossCompartmentClone(err_val, __contextORisolate, &new_error);
      LEAVE_COMPARTMENT(false);
      err_val = JS_LOCAL_OBJECT(new_error, __contextORisolate);
      THROW_EXCEPTION_OBJECT(err_val);
    }
  } else {
    result = _this;
    WrappedScript *n_script = ObjectWrap::Unwrap<WrappedScript>(_this);
    if (!n_script) {
      LEAVE_COMPARTMENT(false);
      THROW_EXCEPTION("Must be called as a method of Script.");
    }

    n_script->script_ = mscript;
  }

  LEAVE_COMPARTMENT(true);
  RETURN_PARAM(result);
}
#endif

DECLARE_CLASS_INITIALIZER(InitEvals) {
  JS_ENTER_SCOPE();

  WrappedContext::Initialize(target);
  WrappedScript::Initialize(target);
}

}  // namespace node

NODE_MODULE(node_evals, node::InitEvals)
