// Copyright & License details are available under JXCORE_LICENSE file

#include "process_wrap.h"
#include "pipe_wrap.h"
#include "tty_wrap.h"
#include "tcp_wrap.h"
#include "udp_wrap.h"

#include <string.h>
#include <stdlib.h>

namespace node {

JS_METHOD(ProcessWrap, New) {
  // This constructor should not be exposed to public javascript.
  // Therefore we assert that we are not trying to call this as a
  // normal function.
  assert(args.IsConstructCall());
  JS_CLASS_NEW_INSTANCE(obj, Process);

  ProcessWrap* wrap = new ProcessWrap(obj);
  assert(wrap);

  RETURN_POINTER(obj);
}
JS_METHOD_END

void ProcessWrap::ParseStdioOptions(commons* com, JS_LOCAL_OBJECT js_options,
                                    uv_process_options_t* options) {
  JS_DEFINE_STATE_MARKER(com);
  JS_LOCAL_ARRAY stdios =
      JS_TYPE_AS_ARRAY(JS_GET_NAME(js_options, JS_PREDEFINED_STRING(stdio)));
  int len = JS_GET_ARRAY_LENGTH(stdios);
  options->stdio = new uv_stdio_container_t[len];
  options->stdio_count = len;

  for (int i = 0; i < len; i++) {
    JS_LOCAL_OBJECT stdio = JS_TYPE_AS_OBJECT(JS_GET_INDEX(stdios, i));
    JS_LOCAL_VALUE type = JS_GET_NAME(stdio, JS_PREDEFINED_STRING(type));

    jxcore::JXString str_type(type);

    if (!strcmp(*str_type, "ignore")) {
      options->stdio[i].flags = UV_IGNORE;
    } else if (!strcmp(*str_type, "pipe")) {
      options->stdio[i].flags = static_cast<uv_stdio_flags>(
          UV_CREATE_PIPE | UV_READABLE_PIPE | UV_WRITABLE_PIPE);
      options->stdio[i]
          .data.stream = reinterpret_cast<uv_stream_t*>(PipeWrap::Unwrap(
          JS_TYPE_AS_OBJECT(JS_GET_NAME(stdio, JS_PREDEFINED_STRING(handle))))
                                                            ->UVHandle());
    } else if (!strcmp(*str_type, "wrap")) {
      uv_stream_t* stream = NULL;
      JS_LOCAL_VALUE wrapType =
          JS_GET_NAME(stdio, JS_PREDEFINED_STRING(wrapType));
      jxcore::JXString str_wrap(wrapType);

      if (!strcmp(*str_wrap, "pipe")) {
        stream = reinterpret_cast<uv_stream_t*>(PipeWrap::Unwrap(
            JS_TYPE_AS_OBJECT(JS_GET_NAME(stdio, JS_PREDEFINED_STRING(handle))))
                                                    ->UVHandle());
      } else if (!strcmp(*str_wrap, "tty")) {
        stream = reinterpret_cast<uv_stream_t*>(TTYWrap::Unwrap(
            JS_TYPE_AS_OBJECT(JS_GET_NAME(stdio, JS_PREDEFINED_STRING(handle))))
                                                    ->UVHandle());
      } else if (!strcmp(*str_wrap, "tcp")) {
        stream = reinterpret_cast<uv_stream_t*>(TCPWrap::Unwrap(
            JS_TYPE_AS_OBJECT(JS_GET_NAME(stdio, JS_PREDEFINED_STRING(handle))))
                                                    ->UVHandle());
      } else if (!strcmp(*str_wrap, "udp")) {
        stream = reinterpret_cast<uv_stream_t*>(UDPWrap::Unwrap(
            JS_TYPE_AS_OBJECT(JS_GET_NAME(stdio, JS_PREDEFINED_STRING(handle))))
                                                    ->UVHandle());
      }
      assert(stream != NULL);

      options->stdio[i].flags = UV_INHERIT_STREAM;
      options->stdio[i].data.stream = stream;
    } else {
      int fd = static_cast<int>(
          INTEGER_TO_STD(JS_GET_NAME(stdio, JS_PREDEFINED_STRING(fd))));

      options->stdio[i].flags = UV_INHERIT_FD;
      options->stdio[i].data.fd = fd;
    }
  }
}

JS_METHOD_NO_COM(ProcessWrap, Spawn) {
  if (!commons::CanSysExec()) {
    THROW_EXCEPTION("This process is restricted for calling system commands");
  }

  ENGINE_UNWRAP(ProcessWrap);

  JS_LOCAL_OBJECT js_options = JS_VALUE_TO_OBJECT(GET_ARG(0));

  uv_process_options_t options;
  memset(&options, 0, sizeof(uv_process_options_t));

  options.exit_cb = OnExit;

  // options.uid
  JS_LOCAL_VALUE uid_v = JS_GET_NAME(js_options, JS_PREDEFINED_STRING(uid));
  if (JS_IS_INT32(uid_v)) {
    int32_t uid = INT32_TO_STD(uid_v);
    if (uid & ~((uv_uid_t) ~0)) {
      THROW_EXCEPTION("options.uid is out of range");
    }
    options.flags |= UV_PROCESS_SETUID;
    options.uid = (uv_uid_t)uid;
  } else if (!JS_IS_UNDEFINED(uid_v) && !JS_IS_NULL(uid_v)) {
    THROW_EXCEPTION("options.uid should be a number");
  }

  // options.gid
  JS_LOCAL_VALUE gid_v = JS_GET_NAME(js_options, JS_PREDEFINED_STRING(gid));
  if (JS_IS_INT32(gid_v)) {
    int32_t gid = INT32_TO_STD(gid_v);
    if (gid & ~((uv_gid_t) ~0)) {
      THROW_EXCEPTION("options.gid is out of range");
    }
    options.flags |= UV_PROCESS_SETGID;
    options.gid = (uv_gid_t)gid;
  } else if (!JS_IS_UNDEFINED(gid_v) && !JS_IS_NULL(gid_v)) {
    THROW_EXCEPTION("options.gid should be a number");
  }

  // options.file
  JS_LOCAL_VALUE file_v = JS_GET_NAME(js_options, JS_PREDEFINED_STRING(file));
  jxcore::JXString file;
  if (JS_IS_STRING(file_v)) {
    file.SetFromHandle(file_v);
    if (file.length() == 0) {
      THROW_EXCEPTION("Bad argument");
    }
    options.file = *file;
  } else {
    THROW_EXCEPTION("Bad argument");
  }

  // options.args
  JS_LOCAL_VALUE argv_v = JS_GET_NAME(js_options, JS_PREDEFINED_STRING(args));
  if (!JS_IS_EMPTY(argv_v) && JS_IS_ARRAY(argv_v)) {
    JS_LOCAL_ARRAY js_argv = JS_CAST_ARRAY(argv_v);
    int argc = JS_GET_ARRAY_LENGTH(js_argv);
    // Heap allocate to detect errors. +1 is for NULL.
    options.args = new char* [argc + 1];
    for (int i = 0; i < argc; i++) {
      JS_LOCAL_VALUE arg_val = JS_GET_INDEX(js_argv, i);
      jxcore::JXString arg(arg_val);
      arg.DisableAutoGC();
      options.args[i] = *arg;
    }
    options.args[argc] = NULL;
  }

  // options.cwd
  JS_LOCAL_VALUE cwd_v = JS_GET_NAME(js_options, JS_PREDEFINED_STRING(cwd));

  jxcore::JXString cwd;
  if (JS_IS_STRING(cwd_v)) {
    cwd.SetFromHandle(cwd_v);
    if (cwd.length() > 0) {
      options.cwd = *cwd;
    }
  }

  // options.env
  JS_LOCAL_VALUE env_v =
      JS_GET_NAME(js_options, JS_PREDEFINED_STRING(envPairs));
  if (!JS_IS_EMPTY(env_v) && JS_IS_ARRAY(env_v)) {
    JS_LOCAL_ARRAY env = JS_CAST_ARRAY(env_v);
    int envc = JS_GET_ARRAY_LENGTH(env);
    options.env = new char* [envc + 1];  // Heap allocated to detect errors.
    for (int i = 0; i < envc; i++) {
      JS_LOCAL_VALUE p_val = JS_GET_INDEX(env, i);
      jxcore::JXString pair(p_val);
      pair.DisableAutoGC();
      options.env[i] = *pair;
    }
    options.env[envc] = NULL;
  }

  // options.stdio
  ParseStdioOptions(com, js_options, &options);

  // options.windows_verbatim_arguments
  if (JS_IS_TRUE(JS_GET_NAME(js_options,
                             JS_PREDEFINED_STRING(windowsVerbatimArguments)))) {
    options.flags |= UV_PROCESS_WINDOWS_VERBATIM_ARGUMENTS;
  }

  // options.detached
  if (JS_IS_TRUE(JS_GET_NAME(js_options, JS_PREDEFINED_STRING(detached)))) {
    options.flags |= UV_PROCESS_DETACHED;
  }

  int r = uv_spawn_jx(wrap->com->loop, &wrap->process_, &options);

  if (r) {
    SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));
  } else {
    wrap->SetHandle((uv_handle_t*)&wrap->process_);
    assert(wrap->process_.data == wrap);
    JS_NAME_SET(wrap->object_, JS_PREDEFINED_STRING(pid),
                STD_TO_INTEGER(wrap->process_.pid));
  }

  if (options.args) {
    for (int i = 0; options.args[i]; i++) {
#ifdef JS_ENGINE_V8
      free(options.args[i]);
#elif JS_ENGINE_MOZJS
      JS_free(__contextORisolate, options.args[i]);
#endif
    }
    delete[] options.args;
  }

  if (options.env) {
    for (int i = 0; options.env[i]; i++) {
#ifdef JS_ENGINE_V8
      free(options.env[i]);
#elif JS_ENGINE_MOZJS
      JS_free(__contextORisolate, options.env[i]);
#endif
    }
    delete[] options.env;
  }

  delete[] options.stdio;

  RETURN_PARAM(STD_TO_INTEGER(r));
}
JS_METHOD_END

JS_METHOD_NO_COM(ProcessWrap, Kill) {
  ENGINE_UNWRAP(ProcessWrap);

  int signal = args.GetInteger(0);

  int r = uv_process_kill(&wrap->process_, signal);

  if (r) SetCOMErrno(com, uv_last_error(com->loop));

  RETURN_PARAM(STD_TO_INTEGER(r));
}
JS_METHOD_END

void ProcessWrap::OnExit(uv_process_t* handle, int exit_status,
                         int term_signal) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  ProcessWrap* wrap = static_cast<ProcessWrap*>(handle->data);
  assert(wrap);
  assert(&wrap->process_ == handle);

  JS_LOCAL_VALUE argv[2] = {STD_TO_INTEGER(exit_status),
                            STD_TO_STRING(signo_string(term_signal))};

  if (exit_status == -1) {
    SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));
  }

  MakeCallback(wrap->com, wrap->object_, STD_TO_STRING("onexit"),
               ARRAY_SIZE(argv), argv);
}

}  // namespace node

NODE_MODULE(node_process_wrap, node::ProcessWrap::Initialize)
