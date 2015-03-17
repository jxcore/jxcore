// Copyright & License details are available under JXCORE_LICENSE file

#include "jx_instance.h"
#include "job_store.h"
#include "extend.h"
#include "job.h"
#include "../wrappers/thread_wrap.h"
#include "../jxcore.h"

#if !defined(_MSC_VER)
#include <strings.h>
#else
#define snprintf _snprintf
#endif

namespace jxcore {

#ifdef JS_ENGINE_MOZJS
void _GCOnMozJS(JSRuntime *rt, JSGCStatus status, void *data) {
  // TODO(obastemur) GC is happening so?
}

bool _JSEngineInterrupt(JSContext *ctx) {
  int tid = JS_GetThreadId(ctx);
  node::commons *com = node::commons::getInstanceByThreadId(tid);
  if (com->should_interrupt_) {
    com->should_interrupt_ = false;
    return false;
  }

  // TODO(obastemur) This also triggers when CPU hits 100%. do anything ?
  return true;
}
#endif

void JXInstance::runScript(void *x) {

  customLock(CSLOCK_NEWINSTANCE);

  if (node::commons::process_status_ != node::JXCORE_INSTANCE_ALIVE) {
    customUnlock(CSLOCK_NEWINSTANCE);
    return;
  }

  int threadId = Job::getNewThreadId();

  Job::fillTasks(threadId);

  node::commons *com = node::commons::newInstance(threadId + 1);
  int resetCount = 0;

#ifdef JS_ENGINE_V8
  v8::Isolate *isolate = com->node_isolate;
#elif defined(JS_ENGINE_MOZJS)
  ENGINE_NS::Isolate *isolate = com->node_isolate;
  JSContext *ctx = isolate->ctx_;
  JSRuntime *rt = JS_GetRuntime(ctx);

  JS_SetInterruptCallback(rt, _JSEngineInterrupt);
  JS_SetGCCallback(rt, _GCOnMozJS, NULL);
#endif
  {
#ifdef JS_ENGINE_V8
    v8::Locker locker(isolate);
    v8::Isolate::Scope isolateScope(isolate);
    v8::HandleScope handle_scope;
    JS_DEFINE_STATE_MARKER(com);
    JS_LOCAL_OBJECT_TEMPLATE _global = JS_NEW_OBJECT_TEMPLATE();

    v8::Handle<v8::Context> context = JS_NEW_CONTEXT(NULL, _global);
    v8::Context::Scope context_scope(context);

    v8_typed_array::AttachBindings(context->Global());
    JS_LOCAL_OBJECT global = context->Global();
#elif defined(JS_ENGINE_MOZJS)
    JSAutoRequest ar(ctx);
    JS::RootedObject _global(ctx, jxcore::NewGlobalObject(ctx));
    assert(_global != NULL);
    JSAutoCompartment ac(ctx, _global);
    JS_LOCAL_OBJECT global(_global, ctx);
    JS_DEFINE_STATE_MARKER(com);
    JS_SetErrorReporter(ctx, node::OnFatalError);
#endif

    uv_idle_t *t = com->tick_spinner;
    uv_idle_init(com->loop, t);

    uv_check_init(com->loop, com->check_immediate_watcher);
    uv_unref((uv_handle_t *)com->check_immediate_watcher);
    uv_idle_init(com->loop, com->idle_immediate_dummy);

    JS_LOCAL_OBJECT inner = JS_NEW_EMPTY_OBJECT();

    JS_METHOD_SET(inner, "compiler", Compiler);
    JS_METHOD_SET(inner, "callBack", CallBack);
    JS_METHOD_SET(inner, "refWaitCounter", refWaitCounter);
    JS_METHOD_SET(inner, "setThreadOnHold", setThreadOnHold);

    JS_NAME_SET(global, JS_STRING_ID("tools"), inner);

    node::SetupProcessObject(threadId + 1);
    JS_HANDLE_OBJECT process_l = com->getProcess();

    customUnlock(CSLOCK_NEWINSTANCE);

    JXEngine::InitializeProxyMethods(com);

    node::Load(process_l);

    uv_run_jx(com->loop, UV_RUN_DEFAULT, node::commons::CleanPinger,
              threadId + 1);

    if (!com->expects_reset)
      node::EmitExit(process_l);
    else
      resetCount = com->waitCounter;

    node::RunAtExit();

    com->Dispose();
  }

#ifdef JS_ENGINE_V8
  com->node_isolate->Dispose();
#elif defined(JS_ENGINE_MOZJS)
  JS_DestroyContext(ctx);

  com->instance_status_ = node::JXCORE_INSTANCE_EXITED;

  // SM can't do GC under GC. we need the destroy other contexts separately
  std::list<JSContext *>::iterator itc = com->free_context_list_.begin();

  while (itc != com->free_context_list_.end()) {
    JS_DestroyContext(*itc);
    itc++;
  }
  com->free_context_list_.clear();

  JS_DestroyRuntime(rt);
  com->node_isolate->Dispose();
#endif

  reduceThreadCount();
  node::removeCommons();
  Job::removeTasker(threadId);

  if (com->expects_reset) {
    char mess[64];
    int ln = snprintf(mess, sizeof(mess),
                      "{\"threadId\":%d , \"resetMe\":true, \"counter\":%d}",
                      threadId, resetCount);
    mess[ln] = '\0';
    jxcore::SendMessage(0, mess, strlen(mess), false);
  }

  delete com;
}

static void handleJob(node::commons *com, Job *j,
                      const JS_HANDLE_FUNCTION &runner) {
  JS_ENTER_SCOPE();
  JS_DEFINE_STATE_MARKER(com);

  JS_HANDLE_ARRAY arr = JS_NEW_ARRAY_WITH_COUNT(3);
  JS_INDEX_SET(arr, 0, STD_TO_INTEGER(j->taskId));
  JS_INDEX_SET(arr, 1, STD_TO_INTEGER(j->cbId));

  if (j->hasParam) {
    JS_INDEX_SET(arr, 2, UTF8_TO_STRING(j->param));
  } else {
    JS_INDEX_SET(arr, 2, JS_UNDEFINED());
  }

  JS_HANDLE_VALUE argv[1] = {arr};
  JS_HANDLE_VALUE result;
  JS_TRY_CATCH(try_catch);

  JS_HANDLE_OBJECT glob = JS_GET_GLOBAL();
  result = JS_METHOD_CALL(runner, glob, 1, argv);
  if (try_catch.HasCaught()) {
    if (try_catch.CanContinue()) node::ReportException(try_catch, true);
    result = JS_UNDEFINED();
  }

  if (!JS_IS_UNDEFINED(result) && !JS_IS_NULL(result)) {
    jxcore::JXString param1(result);
    SendMessage(0, *param1, param1.length(), false);
  } else {
    SendMessage(0, "null", 4, false);
  }
}

static void handleTasks(node::commons *com, const JS_HANDLE_FUNCTION &func,
                        const JS_HANDLE_FUNCTION &runner, const int threadId) {
  JS_ENTER_SCOPE();
  JS_DEFINE_STATE_MARKER(com);

  std::queue<int> tasks;
  Job::getTasks(&tasks, threadId);

  while (!tasks.empty()) {
    JS_HANDLE_ARRAY arr = JS_NEW_ARRAY_WITH_COUNT(2);
    JS_INDEX_SET(arr, 0, STD_TO_INTEGER(tasks.front()));
    Job *j = getTaskDefinition(tasks.front());

    tasks.pop();

    if (node::commons::process_status_ != node::JXCORE_INSTANCE_ALIVE) {
      break;
    }

    assert(j->script != NULL && "Something is wrong job->script shouldn't be null!");

    JS_INDEX_SET(arr, 1, UTF8_TO_STRING(j->script));
    JS_HANDLE_VALUE argv[1] = {arr};
    {
      JS_TRY_CATCH(try_catch);

      func->Call(JS_GET_GLOBAL(), 1, argv);
      if (try_catch.HasCaught()) {
        if (try_catch.CanContinue()) {
          node::ReportException(try_catch, true);
        }
      }
    }

    if (j->cbId == -2) {
      handleJob(com, j, runner);
    }
  }
}

JS_METHOD(JXInstance, Compiler) {
  int threadId = args.GetInteger(0);

  JS_LOCAL_OBJECT func_value = JS_VALUE_TO_OBJECT(GET_ARG(1));
  assert(JS_IS_FUNCTION(func_value));
  JS_LOCAL_FUNCTION func = JS_CAST_FUNCTION(func_value);

  JS_LOCAL_OBJECT runner_value = JS_VALUE_TO_OBJECT(GET_ARG(2));
  assert(JS_IS_FUNCTION(runner_value));
  JS_LOCAL_FUNCTION runner = JS_CAST_FUNCTION(runner_value);

  int was = 0;
  int succ = 0;
  int mn = 0;

  int directions[2];
  if (threadId % 2 == 0) {  // reverse lookups
    directions[0] = 0;
    directions[1] = 1;
  } else {
    directions[0] = 1;
    directions[1] = 0;
  }

  handleTasks(com, func, runner, threadId);
start:
  Job *j = getJob(directions[mn]);
  if (j != NULL) {
    succ++;

    handleTasks(com, func, runner, threadId);
    handleJob(com, j, runner);

    decreaseJobCount();

    j->Dispose();
    if (!j->hasScript) j = NULL;

    goto start;
  }

  mn++;
  if (mn < 2) goto start;

  if (node::commons::process_status_ != node::JXCORE_INSTANCE_ALIVE) {
    RETURN_PARAM(STD_TO_INTEGER(-1));
  } else {
    if (succ > 0) {
      was++;
      succ = 0;
      mn = 0;
      goto start;
    }
    RETURN_PARAM(STD_TO_INTEGER(was));
  }
}
JS_METHOD_END

JS_METHOD(JXInstance, setThreadOnHold) {
  if (args.Length() == 1) {
    if (args.IsInteger(0)) {
      com->threadOnHold = args.GetInteger(0);
      if (com->threadOnHold == 0) {
        node::ThreadWrap::EmitOnMessage(com->threadId);
      }
    }
  }
}
JS_METHOD_END

JS_METHOD(JXInstance, refWaitCounter) {
  if (args.Length() == 1) {
    if (args.IsInteger(0)) {
      com->waitCounter = args.GetInteger(0);
    }
  }
}
JS_METHOD_END

JS_METHOD(JXInstance, CallBack) {
  if (args.Length() == 1) {
    jxcore::JXString jxs;
    int ln = args.GetString(0, &jxs);
    jxcore::SendMessage(0, *jxs, ln, false);
  }
}
JS_METHOD_END
}  // namespace jxcore
