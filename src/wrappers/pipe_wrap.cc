// Copyright & License details are available under JXCORE_LICENSE file

#include "node.h"
#include "node_buffer.h"
#include "jx/commons.h"
#include "wrappers/handle_wrap.h"
#include "stream_wrap.h"
#include "pipe_wrap.h"

namespace node {

// TODO(?) share with TCPWrap?
typedef class ReqWrap<uv_connect_t> ConnectWrap;

uv_pipe_t* PipeWrap::UVHandle() { return &handle_; }

JS_LOCAL_OBJECT PipeWrap::Instantiate() {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);
  assert(!JS_IS_EMPTY((com->pipeConstructor)));
  return JS_LEAVE_SCOPE(JS_NEW_DEFAULT_INSTANCE(com->pipeConstructor));
}

PipeWrap* PipeWrap::Unwrap(JS_LOCAL_OBJECT obj) {
  assert(!JS_IS_EMPTY(obj));
  assert(JS_OBJECT_FIELD_COUNT(obj) > 0);
  return static_cast<PipeWrap*>(JS_GET_POINTER_DATA(obj));
}

JS_METHOD(PipeWrap, New) {
  // This constructor should not be exposed to public javascript.
  // Therefore we assert that we are not trying to call this as a
  // normal function.

  assert(args.IsConstructCall());
  JS_CLASS_NEW_INSTANCE(obj, Pipe);

  PipeWrap* wrap = new PipeWrap(obj, args.GetBoolean(0));
  assert(wrap);

  RETURN_POINTER(obj);
}
JS_METHOD_END

PipeWrap::PipeWrap(JS_HANDLE_OBJECT object, bool ipc)
    : StreamWrap(object, (uv_stream_t*)&handle_) {
  int r = uv_pipe_init(com->loop, &handle_, ipc);
  assert(r == 0);  // How do we proxy this error up to javascript?
  // Suggestion: uv_pipe_init() returns void.
  handle_.data = static_cast<void*>(this);
  UpdateWriteQueueSize(com);
}

JS_METHOD_NO_COM(PipeWrap, Bind) {
  ENGINE_UNWRAP(PipeWrap);

  JS_LOCAL_VALUE arg0 = GET_ARG(0);
  jxcore::JXString name(arg0);

  int r = uv_pipe_bind(&wrap->handle_, *name);

  // Error starting the pipe.
  if (r) SetErrno(uv_last_error(com->loop));

  RETURN_PARAM(STD_TO_INTEGER(r));
}
JS_METHOD_END

#ifdef _WIN32
JS_METHOD_NO_COM(PipeWrap, SetPendingInstances) {
  ENGINE_UNWRAP(PipeWrap);

  int instances = args.GetInt32(0);

  uv_pipe_pending_instances(&wrap->handle_, instances);
}
JS_METHOD_END
#endif

JS_METHOD_NO_COM(PipeWrap, Listen) {
  ENGINE_UNWRAP(PipeWrap);

  int backlog = args.GetInt32(0);

  int r = uv_listen((uv_stream_t*)&wrap->handle_, backlog, OnConnection);

  // Error starting the pipe.
  if (r) SetErrno(uv_last_error(wrap->com->loop));

  RETURN_PARAM(STD_TO_INTEGER(r));
}
JS_METHOD_END

// TODO(?) maybe share with TCPWrap?
void PipeWrap::OnConnection(uv_stream_t* handle, int status) {
  JS_ENTER_SCOPE();

  PipeWrap* wrap = static_cast<PipeWrap*>(handle->data);
  assert(&wrap->handle_ == (uv_pipe_t*)handle);

  commons* com = wrap->com;

  // We should not be getting this callback if someone as already called
  // uv_close() on the handle.
  assert(JS_IS_EMPTY((wrap->object_)) == false);

  if (status != 0) {
    SetErrno(uv_last_error(com->loop));
    MakeCallback(wrap->com, wrap->object_, JS_PREDEFINED_STRING(onconnection),
                 0, NULL);
    return;
  }

  // Instanciate the client javascript object and handle.
  JS_LOCAL_OBJECT client_obj =
      JS_NEW_DEFAULT_INSTANCE(wrap->com->pipeConstructor);

  // Unwrap the client javascript object.
  assert(JS_OBJECT_FIELD_COUNT(client_obj) > 0);
  PipeWrap* client_wrap =
      static_cast<PipeWrap*>(JS_GET_POINTER_DATA(client_obj));

  if (uv_accept(handle, (uv_stream_t*)&client_wrap->handle_)) return;

  // Successful accept. Call the onconnection callback in JavaScript land.
  __JS_LOCAL_VALUE argv[1] = {JS_CORE_REFERENCE(client_obj)};

  MakeCallback(wrap->com, wrap->object_, JS_PREDEFINED_STRING(onconnection),
               ARRAY_SIZE(argv), argv);
}

// TODO(?) Maybe share this with TCPWrap?
void PipeWrap::AfterConnect(uv_connect_t* req, int status) {
  ConnectWrap* req_wrap = (ConnectWrap*)req->data;
  PipeWrap* wrap = (PipeWrap*)req->handle->data;

  JS_ENTER_SCOPE();
  JS_DEFINE_STATE_MARKER(wrap->com);

  // The wrap and request objects should still be there.
  assert(JS_IS_EMPTY((req_wrap->object_)) == false);
  assert(JS_IS_EMPTY((wrap->object_)) == false);

  bool readable, writable;

  if (status) {
    SetErrno(uv_last_error(wrap->com->loop));
    readable = writable = 0;
  } else {
    readable = uv_is_readable(req->handle) != 0;
    writable = uv_is_writable(req->handle) != 0;
  }

  JS_LOCAL_BOOLEAN b_readable = STD_TO_BOOLEAN(readable);
  JS_LOCAL_BOOLEAN b_writable = STD_TO_BOOLEAN(writable);

  JS_LOCAL_VALUE argv[5] = {STD_TO_INTEGER(status),
                            JS_TYPE_TO_LOCAL_OBJECT(wrap->object_),
                            JS_TYPE_TO_LOCAL_OBJECT(req_wrap->object_),
                            b_readable,
                            b_writable};

  MakeCallback(wrap->com, req_wrap->object_, wrap->com->pstr_oncomplete,
               ARRAY_SIZE(argv), argv);

  delete req_wrap;
}

JS_METHOD_NO_COM(PipeWrap, Open) {
  ENGINE_UNWRAP(PipeWrap);

  if (uv_pipe_open(&wrap->handle_, args.GetInt32(0))) {
    uv_err_t err = uv_last_error(wrap->handle_.loop);
    JS_LOCAL_VALUE _err = UVException(err.code, "uv_pipe_open");
    THROW_EXCEPTION_OBJECT(_err);
  }
}
JS_METHOD_END

JS_METHOD_NO_COM(PipeWrap, Connect) {
  ENGINE_UNWRAP(PipeWrap);

  JS_LOCAL_VALUE arg0 = GET_ARG(0);
  jxcore::JXString name(arg0);

  ConnectWrap* req_wrap = new ConnectWrap(wrap->com);

  uv_pipe_connect(&req_wrap->req_, &wrap->handle_, *name, AfterConnect);

  req_wrap->Dispatched();

  RETURN_PARAM(req_wrap->object_);
}
JS_METHOD_END

}  // namespace node

NODE_MODULE(node_pipe_wrap, node::PipeWrap::Initialize)
