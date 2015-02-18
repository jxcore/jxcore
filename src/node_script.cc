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

  JS_PERSISTENT_SCRIPT script_;
};

void CloneObject(JS_STATE_MARKER, JS_HANDLE_OBJECT recv,
                 JS_HANDLE_VALUE_REF source, JS_HANDLE_VALUE_REF target) {
  JS_HANDLE_VALUE args[] = {source, target};

// Init
#ifdef JS_ENGINE_V8
  JS_ENTER_SCOPE_COM();
  if (JS_IS_EMPTY((com->cloneObjectMethod))) {
    JS_LOCAL_FUNCTION cloneObjectMethod_ = JS_CAST_FUNCTION(JS_COMPILE_AND_RUN(
#elif defined(JS_ENGINE_MOZJS)
  MozJS::Value global = JS_GET_CONTEXT_GLOBAL(__contextORisolate);
  JS_LOCAL_FUNCTION cloneObjectMethod_ = MozJS::Value::CompileAndRun(
      __contextORisolate,
#endif
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
        STD_TO_STRING("binding:script")
#ifdef JS_ENGINE_V8
        ));
    com->cloneObjectMethod = JS_NEW_PERSISTENT_FUNCTION(cloneObjectMethod_);
  }
  JS_METHOD_CALL(com->cloneObjectMethod, recv, 2, args);
#elif defined(JS_ENGINE_MOZJS)
      ,
      &global);
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
  context_ = JS_NEW_EMPTY_CONTEXT();
}

WrappedContext::~WrappedContext() { JS_DISPOSE_PERSISTENT_CONTEXT(context_); }

JS_LOCAL_OBJECT WrappedContext::NewInstance(commons *com) {
  if (com == NULL) com = node::commons::getInstance();
  JS_LOCAL_OBJECT context =
      JS_NEW_DEFAULT_INSTANCE(JS_GET_FUNCTION(com->wc_constructor_template));
  return context;
}

JS_PERSISTENT_CONTEXT WrappedContext::GetV8Context() { return context_; }

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

#ifdef JS_ENGINE_MOZJS
static JSObject *CrossCompartmentClone(MozJS::Value &orig_obj,
                                       JSContext *newc) {
  JSContext *context = orig_obj.ctx_;
  MozJS::Value target_sandbox(jxcore::NewTransplantObject(context), context);
  JSObject *cs_sandbox =
      jxcore::CrossCompartmentCopy(context, newc, target_sandbox, false);

  CloneObject(context, target_sandbox, orig_obj, target_sandbox);
  return cs_sandbox;
}
#endif

template <WrappedScript::EvalInputFlags input_flag,
          WrappedScript::EvalContextFlags context_flag,
          WrappedScript::EvalOutputFlags output_flag>
JS_NATIVE_RETURN_TYPE WrappedScript::EvalMachine(jxcore::PArguments &args,
#ifdef JS_ENGINE_V8
                                                 JS_HANDLE_OBJECT _this)
#elif defined(JS_ENGINE_MOZJS)
                                                 const JS_HANDLE_OBJECT &_this)
#endif
{
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
#ifdef JS_ENGINE_V8
  bool display_error = false;
  // This hack doesn't needed for MozJS
  if (args.Length() > display_error_index &&
      args.IsBoolean(display_error_index) &&
      args.GetBoolean(display_error_index) == true) {
    display_error = true;
  }
#endif

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
#ifdef JS_ENGINE_V8
  ENGINE_NS::Context::Scope context_scope(context);

  // New and user context share code. DRY it up.
  if (context_flag == userContext || context_flag == newContext) {
    // Copy everything from the passed in sandbox (either the persistent
    // context for runInContext(), or the sandbox arg to runInNewContext()).
    CloneObject(JS_GET_STATE_MARKER(), args.This(), sandbox,
                context->Global()->GetPrototype());
  }
#define LEAVE_COMPARTMENT(x)
#endif
  // Catch errors
  JS_TRY_CATCH(try_catch);
#if defined(JS_ENGINE_MOZJS)
  JSObject *fake_sandbox = NULL;
  JS_LOCAL_OBJECT sandbox_backup;

  // we can't check comp != nullptr
  // if there is no oldCompartment, it will return null
  // after entering the new one anyways.
  // so we need a flag here
  bool entered_compartment_ = false;
  JSCompartment *comp = NULL;
  if (context_flag == userContext || context_flag == newContext) {
    if (context_flag == newContext) {
      sandbox_backup = JS_LOCAL_OBJECT(
          jxcore::NewTransplantObject(__contextORisolate), __contextORisolate);
    }
  }
#define LEAVE_COMPARTMENT(copy_result)                                        \
  if (entered_compartment_) {                                                 \
    JS_LOCAL_VALUE result_backup;                                             \
    if (copy_result) {                                                        \
      result_backup = result;                                                 \
      result = JS_LOCAL_VALUE(jxcore::NewTransplantObject(context), context); \
      JS_NAME_SET(result, JS_STRING_ID("result"), result_backup);             \
    }                                                                         \
    MozJS::Value source_sandbox(fake_sandbox, context);                       \
    JSObject *cs_sandbox =                                                    \
        CrossCompartmentClone(source_sandbox, __contextORisolate);            \
    JSObject *result_sandbox = nullptr;                                       \
    if (copy_result) {                                                        \
      result_sandbox = CrossCompartmentClone(result, __contextORisolate);     \
    }                                                                         \
    JS_LeaveCompartment(context, comp);                                       \
    MozJS::Value comp_sandbox(cs_sandbox, __contextORisolate);                \
    CloneObject(__contextORisolate, sandbox, comp_sandbox, sandbox);          \
    if (result_sandbox != nullptr) {                                          \
      result = JS_LOCAL_OBJECT(result_sandbox, __contextORisolate);           \
      result = JS_GET_NAME(result, JS_STRING_ID("result"));                   \
    }                                                                         \
  }
#endif

  JS_HANDLE_VALUE result;
  JS_HANDLE_SCRIPT script;

  if (input_flag == compileCode) {
// well, here WrappedScript::New would suffice in all cases, but maybe
// Compile has a little better performance where possible
#ifdef JS_ENGINE_V8
    script = output_flag == returnResult
                 ? JS_SCRIPT_COMPILE(code, filename)
                 : ENGINE_NS::Script::New(code, filename);
#elif defined(JS_ENGINE_MOZJS)
    JS_LOCAL_VALUE globals;
    if (context_flag == userContext || context_flag == newContext) {
      globals = sandbox;
    } else {
      globals = jxcore::getGlobal(com->threadId);
    }
    bool asm_js = false;
    if (args.IsInteger(args.Length() - 1))
      asm_js = args.GetInt32(args.Length() - 1) == 1;
    script = MozJS::Script::Compile(context, globals, code, filename, asm_js);
#endif
    if (JS_IS_EMPTY(script)) {
#ifdef JS_ENGINE_V8
      // v8 Only hack
      if (display_error) DisplayExceptionLine(try_catch);

      // Hack because I can't get a proper stacktrace on SyntaxError
      return try_catch.ReThrow();
#elif defined(JS_ENGINE_MOZJS)
      JS_LOCAL_VALUE err_val = try_catch.Exception();
      THROW_EXCEPTION_OBJECT(err_val);
#endif
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
#ifdef JS_ENGINE_V8
    result = JS_SCRIPT_RUN(script);
#elif defined(JS_ENGINE_MOZJS)

    if (context_flag == userContext || context_flag == newContext) {

      if (!sandbox_backup.IsEmpty()) {
        fake_sandbox = jxcore::CrossCompartmentCopy(__contextORisolate, context,
                                                    sandbox_backup, true);
        CloneObject(__contextORisolate, sandbox_backup, sandbox,
                    sandbox_backup);

        comp = JS_EnterCompartment(context, fake_sandbox);
        try_catch.ctx_ = context;
        entered_compartment_ = true;
        JSScript *new_script = jxcore::TransplantScript(
            script.ctx_, context, script.GetRawScriptPointer());
        MozJS::Script script_pr(new_script, context);

        JS_LOCAL_OBJECT obj_fs(fake_sandbox, context);
        result = script_pr.Run(obj_fs);
      } else {
        script.ctx_ = context;
        result = script.Run(sandbox);
        script.ctx_ = __contextORisolate;
      }
    } else {
      result = script.Run();
    }

#endif
    if (JS_IS_EMPTY(result)) {
#ifdef JS_ENGINE_V8
      if (display_error) DisplayExceptionLine(try_catch);
      return try_catch.ReThrow();
#elif defined(JS_ENGINE_MOZJS)
      JS_LOCAL_VALUE err_val;
      bool catched_error = try_catch.HasCaught();
      if (!catched_error) {
        LEAVE_COMPARTMENT(false);

        jxcore::JXString fname(filename);
        std::string str = "Uncaught exception at ";
        str += *fname;

        err_val = ENGINE_NS::Exception::Error(STD_TO_STRING(str.c_str()))
                      .GetErrorObject();
        JS_NAME_SET(err_val, "stack", filename);

        THROW_EXCEPTION_OBJECT(err_val);
      }

      err_val = try_catch.Exception();
      JSObject *new_error = CrossCompartmentClone(err_val, __contextORisolate);
      LEAVE_COMPARTMENT(false);
      err_val = JS_LOCAL_OBJECT(new_error, __contextORisolate);
      THROW_EXCEPTION_OBJECT(err_val);
#endif
    }
  } else {
    WrappedScript *n_script = ObjectWrap::Unwrap<WrappedScript>(_this);
    if (!n_script) {
      LEAVE_COMPARTMENT(false);
      THROW_EXCEPTION("Must be called as a method of Script.");
    }
    n_script->script_ = JS_NEW_PERSISTENT_SCRIPT(script);
    result = _this;
  }

#ifdef JS_ENGINE_V8
  if (context_flag == userContext || context_flag == newContext) {
    // success! copy changes back onto the sandbox object.
    CloneObject(JS_GET_STATE_MARKER(), args.This(),
                context->Global()->GetPrototype(), sandbox);
  }

  return result == args.This() ? result : JS_LEAVE_SCOPE(result);
#elif defined(JS_ENGINE_MOZJS)
  LEAVE_COMPARTMENT(true);
  RETURN_PARAM(result);
#endif
}

DECLARE_CLASS_INITIALIZER(InitEvals) {
  JS_ENTER_SCOPE();

  WrappedContext::Initialize(target);
  WrappedScript::Initialize(target);
}

}  // namespace node

NODE_MODULE(node_evals, node::InitEvals)
