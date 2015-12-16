// Copyright & License details are available under JXCORE_LICENSE file

#include "node.h"
#include "node_buffer.h"
#include "wrappers/handle_wrap.h"
#include "slab_allocator.h"
#include "stream_wrap.h"
#include "pipe_wrap.h"
#include "tcp_wrap.h"
#include "udp_wrap.h"
#include "node_counters.h"
#include "node_http_parser.h"

#include <stdlib.h>  // abort()
#include <limits.h>  // INT_MAX

namespace node {
typedef class ReqWrap<uv_shutdown_t> ShutdownWrap;

class WriteWrap : public ReqWrap<uv_write_t> {
 public:
  void* operator new(size_t size, char* storage) { return storage; }

  // This is just to keep the compiler happy. It should never be called, since
  // we don't use exceptions in node.
  void operator delete(void* ptr, char* storage) { assert(0 && "DO NOT USE"); }

 protected:
  // People should not be using the non-placement new and delete operator on a
  // WriteWrap. Ensure this never happens.
  void* operator new(size_t size) {
    assert(0 && "DO NOT USE");
    return NULL;
  }
  void operator delete(void* ptr) { assert(0 && "DO NOT USE"); }
};

static void DeleteSlabAllocator(void*) {
  ENGINE_LOG_THIS("StreamWrap", "DeleteSlabAllocator");
  commons* com = node::commons::getInstance();
  delete com->s_slab_allocator;
  com->s_slab_allocator = NULL;
}

void StreamWrap::Initialize(JS_HANDLE_OBJECT target) {
  ENGINE_LOG_THIS("StreamWrap", "Initialize");
  JS_ENTER_SCOPE_COM();

  if (com->stream_initialized) return;
  com->stream_initialized = true;

  com->s_slab_allocator = new SlabAllocator(SLAB_SIZE, com);
  AtExit(DeleteSlabAllocator, NULL);

  HandleWrap::Initialize(target);
}

StreamWrap::StreamWrap(JS_HANDLE_OBJECT object, uv_stream_t* stream)
    : HandleWrap(object, (uv_handle_t*)stream) {
  ENGINE_LOG_THIS("StreamWrap", "StreamWrap");
  stream_ = stream;

  if (stream) {
    stream->data = this;
  }
}

JS_GETTER_CLASS_METHOD(StreamWrap, GetFD) {
  ENGINE_LOG_THIS("StreamWrap", "GetFD");
#if defined(_WIN32)
  RETURN_GETTER_PARAM(JS_NULL());
#else
  StreamWrap* wrap = static_cast<StreamWrap*>(JS_GET_POINTER_DATA(caller));
  int fd = -1;
  if (wrap != NULL && wrap->stream_ != NULL) fd = wrap->stream_->io_watcher.fd;
  RETURN_GETTER_PARAM(STD_TO_INTEGER(fd));
#endif
}
JS_GETTER_METHOD_END

void StreamWrap::SetHandle(uv_handle_t* h) {
  ENGINE_LOG_THIS("StreamWrap", "SetHandle");
  HandleWrap::SetHandle(h);
  stream_ = reinterpret_cast<uv_stream_t*>(h);
  stream_->data = this;
}

void StreamWrap::UpdateWriteQueueSize(const commons* com) {
  ENGINE_LOG_THIS("StreamWrap", "UpdateWriteQueueSize");
  JS_ENTER_SCOPE_WITH(com->node_isolate);
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_OBJECT obj = JS_OBJECT_FROM_PERSISTENT(object_);
  JS_NAME_SET(obj, JS_STRING_ID("writeQueueSize"),
              STD_TO_INTEGER(stream_->write_queue_size));
}

JS_METHOD_NO_COM(StreamWrap, ReadStart) {
  ENGINE_UNWRAP(StreamWrap);

  bool ipc_pipe =
      wrap->stream_->type == UV_NAMED_PIPE && ((uv_pipe_t*)wrap->stream_)->ipc;

  int r;
  if (ipc_pipe) {
    r = uv_read2_start(wrap->stream_, OnAlloc, OnRead2);
  } else {
    r = uv_read_start(wrap->stream_, OnAlloc, OnRead);
  }

  // Error starting the tcp.
  if (r) SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));

  RETURN_PARAM(STD_TO_INTEGER(r));
}
JS_METHOD_END

JS_METHOD_NO_COM(StreamWrap, ReadStop) {
  ENGINE_UNWRAP(StreamWrap);

  int r = uv_read_stop(wrap->stream_);

  // Error starting the tcp.
  if (r) SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));

  RETURN_PARAM(STD_TO_INTEGER(r));
}
JS_METHOD_END

uv_buf_t StreamWrap::OnAlloc(uv_handle_t* handle, size_t suggested_size) {
  ENGINE_LOG_THIS("StreamWrap", "OnAlloc");
  StreamWrap* wrap = static_cast<StreamWrap*>(handle->data);
  assert(wrap->stream_ == reinterpret_cast<uv_stream_t*>(handle));
  commons* com = wrap->com;
  JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(wrap->object_);
  char* buf = com->s_slab_allocator->Allocate(objl, suggested_size);
  return uv_buf_init(buf, suggested_size);
}

template <class WrapType, class UVType>
static JS_LOCAL_OBJECT AcceptHandle(uv_stream_t* pipe) {
  ENGINE_LOG_THIS("StreamWrap", "AcceptHandle");
  JS_ENTER_SCOPE();

  JS_LOCAL_OBJECT wrap_obj;
  WrapType* wrap;
  UVType* handle;

  wrap_obj = WrapType::Instantiate();
  if (JS_IS_EMPTY(wrap_obj)) return JS_LOCAL_OBJECT();

  wrap = static_cast<WrapType*>(JS_GET_POINTER_DATA(wrap_obj));
  handle = wrap->UVHandle();

  if (uv_accept(pipe, reinterpret_cast<uv_stream_t*>(handle))) {
    error_console("Failed command - uv_accept at StreapWrap::AcceptHandle\n");
    abort();
  }

  return JS_LEAVE_SCOPE(wrap_obj);
}

void StreamWrap::OnReadCommon(uv_stream_t* handle, ssize_t nread, uv_buf_t buf,
                              uv_handle_type pending) {
  StreamWrap* wrap = static_cast<StreamWrap*>(handle->data);

  // We should not be getting this callback if someone as already called
  // uv_close() on the handle.
  assert(JS_IS_EMPTY((wrap->object_)) == false);

  commons* com = wrap->com;
  JS_ENTER_SCOPE_WITH(com->node_isolate);
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(wrap->object_);
  if (nread < 0) {
    // If libuv reports an error or EOF it *may* give us a buffer back. In that
    // case, return the space to the slab.
    if (buf.base != NULL) {
      com->s_slab_allocator->Shrink(objl, buf.base, 0);
    }

    SetCOMErrno(com, uv_last_error(com->loop));
    MakeCallback(com, objl, JS_PREDEFINED_STRING(onread), 0, NULL);

    return;
  }

  assert(buf.base != NULL);

  JS_LOCAL_OBJECT slab = com->s_slab_allocator->Shrink(objl, buf.base, nread);

  if (nread == 0) return;
  assert(static_cast<size_t>(nread) <= buf.len);

  int argc = 3;
  char* buffer_data = BUFFER__DATA(slab);
  size_t started = buf.base - buffer_data;

#ifdef JS_ENGINE_MOZJS
  jsval argv[5] = {JS_CORE_REFERENCE(slab), JS::Int32Value(started),
                   JS::Int32Value(nread)};
#elif defined(JS_ENGINE_V8)
  // both works for MozJS but above is faster
  JS_LOCAL_VALUE argv[5] = {slab, STD_TO_INTEGER(started),
                            STD_TO_INTEGER(nread)};
#endif

  JS_LOCAL_OBJECT pending_obj;
  if (pending == UV_TCP) {
    pending_obj = AcceptHandle<TCPWrap, uv_tcp_t>(handle);
  } else if (pending == UV_NAMED_PIPE) {
    pending_obj = AcceptHandle<PipeWrap, uv_pipe_t>(handle);
  } else if (pending == UV_UDP) {
    pending_obj = AcceptHandle<UDPWrap, uv_udp_t>(handle);
  } else {
    assert(pending == UV_UNKNOWN_HANDLE);
  }

  if (!JS_IS_EMPTY(pending_obj)) {
    argv[3] = JS_CORE_REFERENCE(pending_obj);
    argc++;
  }

  if (wrap->stream_->type == UV_TCP) {
    NODE_COUNT_NET_BYTES_RECV(nread);
  } else if (wrap->stream_->type == UV_NAMED_PIPE) {
    NODE_COUNT_PIPE_BYTES_RECV(nread);
  }

  if (JS_HAS_NAME(objl, JS_PREDEFINED_STRING(owner))) {
    JS_LOCAL_OBJECT owner =
        JS_VALUE_TO_OBJECT(JS_GET_NAME(objl, JS_PREDEFINED_STRING(owner)));

    if (JS_HAS_NAME(owner, JS_PREDEFINED_STRING(parser))) {
      JS_LOCAL_VALUE this_parser =
          JS_GET_NAME(owner, JS_PREDEFINED_STRING(parser));

      if (!JS_IS_EMPTY(this_parser) && !JS_IS_NULL(this_parser)) {
        size_t bl = BUFFER__LENGTH(slab);
        int ret_val = 0;
#if defined(JS_ENGINE_V8)
        JS_LOCAL_OBJECT parser_object = JS_VALUE_TO_OBJECT(this_parser);
        JS_HANDLE_VALUE val = ExecuteDirect(com, parser_object, buffer_data, bl,
                                            started, nread, &ret_val);
        if (ret_val > 0 || ret_val == -2) {
          JS_LOCAL_OBJECT ret = JS_VALUE_TO_OBJECT(val);
#elif defined(JS_ENGINE_MOZJS)
        JS_HANDLE_VALUE ret = ExecuteDirect(com, this_parser, buffer_data, bl,
                                            started, nread, &ret_val);
        if (ret_val > 0 || ret_val == -2) {
#endif
          if (argc == 3) {  // interface compatibility
            argv[3] = JS_CORE_REFERENCE(ret);
            argc++;
          }

          argv[4] = JS_CORE_REFERENCE(ret);
          argc++;
        }
      }
    }
  }

  MakeCallback(com, objl, JS_PREDEFINED_STRING(onread), argc, argv);
}

void StreamWrap::OnRead(uv_stream_t* handle, ssize_t nread, uv_buf_t buf) {
  ENGINE_LOG_THIS("StreamWrap", "OnRead");
  OnReadCommon(handle, nread, buf, UV_UNKNOWN_HANDLE);
}

void StreamWrap::OnRead2(uv_pipe_t* handle, ssize_t nread, uv_buf_t buf,
                         uv_handle_type pending) {
  ENGINE_LOG_THIS("StreamWrap", "OnRead2");
  OnReadCommon(reinterpret_cast<uv_stream_t*>(handle), nread, buf, pending);
}

JS_METHOD_NO_COM(StreamWrap, WriteBuffer) {
  ENGINE_UNWRAP(StreamWrap);

  // The first argument is a buffer.
  assert(args.Length() >= 1);
  JS_LOCAL_OBJECT buffer_obj = JS_VALUE_TO_OBJECT(args.GetItem(0));
  size_t offset = 0;
  size_t length = BUFFER__LENGTH(buffer_obj);
  char* storage = new char[sizeof(WriteWrap)];
  WriteWrap* req_wrap = new (storage) WriteWrap();
  req_wrap->Init(wrap->com);

  JS_LOCAL_OBJECT objr = JS_OBJECT_FROM_PERSISTENT(req_wrap->object_);
  JS_NAME_SET_HIDDEN(objr, JS_PREDEFINED_STRING(buffer), buffer_obj);

  uv_buf_t buf;
  buf.base = BUFFER__DATA(buffer_obj) + offset;
  buf.len = length;

  int r =
      uv_write(&req_wrap->req_, wrap->stream_, &buf, 1, StreamWrap::AfterWrite);

  req_wrap->Dispatched();

  JS_NAME_SET(objr, JS_PREDEFINED_STRING(bytes), STD_TO_INTEGER(length));

  wrap->UpdateWriteQueueSize(wrap->com);

  if (r) {
    SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));
    req_wrap->~WriteWrap();

    delete[] storage;
    RETURN_PARAM(JS_NULL());
  } else {
    if (wrap->stream_->type == UV_TCP) {
      NODE_COUNT_NET_BYTES_SENT(length);
    } else if (wrap->stream_->type == UV_NAMED_PIPE) {
      NODE_COUNT_PIPE_BYTES_SENT(length);
    }

    RETURN_PARAM(objr);
  }
}
JS_METHOD_END

#define STR_WRITEIMP                                 \
  template <enum encoding encoding>                  \
  JS_NATIVE_RETURN_TYPE StreamWrap::WriteStringImpl( \
      jxcore::PArguments& args) {
STR_WRITEIMP
JS_ENTER_SCOPE_WITH(args.GetIsolate());
ENGINE_UNWRAP(StreamWrap);
JS_DEFINE_STATE_MARKER(com);
{
  int r;

  if (args.Length() < 1) THROW_TYPE_EXCEPTION("Not enough arguments");

  JS_HANDLE_STRING arg0 = args.GetAsString(0);
  size_t xl = JS_GET_STRING_LENGTH(arg0);

  // Compute the size of the storage that the string will be flattened into.
  // For UTF8 strings that are very long, go ahead and take the hit for
  // computing their actual size, rather than tripling the storage.
  bool isBuffer = Buffer::jxHasInstance(arg0, wrap->com);
  size_t storage_size;
  if (encoding == UTF8 && xl > 65535) {
    storage_size = StringBytes::JXSize(arg0, encoding, isBuffer, xl);
  } else {
    storage_size = StringBytes::JXStorageSize(arg0, encoding, isBuffer, xl);
  }

#ifdef JS_ENGINE_V8
  if (storage_size > INT_MAX) {
#elif defined(JS_ENGINE_MOZJS)
  if (storage_size > LONG_MAX) {
#endif
    uv_err_t err;
    err.code = UV_ENOBUFS;
    SetCOMErrno(wrap->com, err);
    RETURN_PARAM(JS_NULL());
  }

  char* storage = new char[sizeof(WriteWrap) + storage_size + 15];
  WriteWrap* req_wrap = new (storage) WriteWrap();

  req_wrap->Init(wrap->com);

  char* data = reinterpret_cast<char*>(
      ROUND_UP(reinterpret_cast<uintptr_t>(storage) + sizeof(WriteWrap), 16));

  size_t data_size;
  data_size =
      StringBytes::JXWrite(data, storage_size, arg0, encoding, isBuffer);

  assert(data_size <= storage_size);

  uv_buf_t buf;
  buf.base = data;
  buf.len = data_size;

  JS_LOCAL_OBJECT objr = JS_OBJECT_FROM_PERSISTENT(req_wrap->object_);
  bool ipc_pipe =
      wrap->stream_->type == UV_NAMED_PIPE && ((uv_pipe_t*)wrap->stream_)->ipc;

  if (!ipc_pipe) {
    r = uv_write(&req_wrap->req_, wrap->stream_, &buf, 1,
                 StreamWrap::AfterWrite);

  } else {
    uv_handle_t* send_handle = NULL;

    if (args.IsObject(1)) {
      JS_LOCAL_OBJECT send_handle_obj = JS_VALUE_TO_OBJECT(args.GetItem(1));
      assert(send_handle_obj->InternalFieldCount() > 0);
      HandleWrap* send_handle_wrap =
          static_cast<HandleWrap*>(JS_GET_POINTER_DATA(send_handle_obj));
      send_handle = send_handle_wrap->GetHandle();

      // Reference StreamWrap instance to prevent it from being garbage
      // collected before `AfterWrite` is called.
      commons* com = send_handle_wrap->com;
      assert(!JS_IS_EMPTY((req_wrap->object_)));
      com->handle_has_symbol_ = true;
      JS_NAME_SET(objr, JS_PREDEFINED_STRING(handle), send_handle_obj);
    }

    r = uv_write2(&req_wrap->req_, wrap->stream_, &buf, 1,
                  reinterpret_cast<uv_stream_t*>(send_handle),
                  StreamWrap::AfterWrite);
  }

  req_wrap->Dispatched();
  JS_NAME_SET(objr, JS_PREDEFINED_STRING(bytes), STD_TO_INTEGER(data_size));

  wrap->UpdateWriteQueueSize(wrap->com);

  if (r) {
    SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));
    req_wrap->~WriteWrap();
    delete[] storage;
    RETURN_PARAM(JS_NULL());
  } else {
    if (wrap->stream_->type == UV_TCP) {
      NODE_COUNT_NET_BYTES_SENT(buf.len);
    } else if (wrap->stream_->type == UV_NAMED_PIPE) {
      NODE_COUNT_PIPE_BYTES_SENT(buf.len);
    }

    RETURN_PARAM(objr);
  }
}
JS_METHOD_END

JS_METHOD_NO_COM(StreamWrap, WriteAsciiString) {
  RETURN_FROM(WriteStringImpl<ASCII>(args));
}
JS_METHOD_END

JS_METHOD_NO_COM(StreamWrap, WriteUtf8String) {
  RETURN_FROM(WriteStringImpl<UTF8>(args));
}
JS_METHOD_END

JS_METHOD_NO_COM(StreamWrap, WriteUcs2String) {
  RETURN_FROM(WriteStringImpl<UCS2>(args));
}
JS_METHOD_END

void StreamWrap::AfterWrite(uv_write_t* req, int status) {
  ENGINE_LOG_THIS("StreamWrap", "AfterWrite");
  WriteWrap* req_wrap = (WriteWrap*)req->data;
  StreamWrap* wrap = (StreamWrap*)req->handle->data;

  node::commons* com = wrap->com;
  JS_ENTER_SCOPE_WITH(com->node_isolate);
  JS_DEFINE_STATE_MARKER(com);

  // The wrap and request objects should still be there.
  assert(JS_IS_EMPTY((req_wrap->object_)) == false);
  assert(JS_IS_EMPTY((wrap->object_)) == false);

  JS_LOCAL_OBJECT objr = JS_OBJECT_FROM_PERSISTENT(req_wrap->object_);
  // Unref handle property
  if (com->handle_has_symbol_) {
    JS_NAME_DELETE(objr, JS_PREDEFINED_STRING(handle));
    com->handle_has_symbol_ = false;
  }

  if (status) {
    SetCOMErrno(com, uv_last_error(com->loop));
  }

  wrap->UpdateWriteQueueSize(com);

#ifdef JS_ENGINE_V8
  JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(wrap->object_);
  __JS_LOCAL_VALUE argv[] = {STD_TO_INTEGER(status),
                             JS_TYPE_TO_LOCAL_VALUE(objl),
                             JS_TYPE_TO_LOCAL_VALUE(objr)};
#elif defined(JS_ENGINE_MOZJS)
  __JS_LOCAL_VALUE argv[] = {JS::Int32Value(status),
                             JS_CORE_REFERENCE(wrap->object_),
                             JS_CORE_REFERENCE(req_wrap->object_)};
#endif
  MakeCallback(wrap->com, objr, JS_PREDEFINED_STRING(oncomplete), 3, argv);

  req_wrap->~WriteWrap();
  delete[] reinterpret_cast<char*>(req_wrap);
}

JS_METHOD_NO_COM(StreamWrap, Shutdown) {
  ENGINE_UNWRAP(StreamWrap);

  ShutdownWrap* req_wrap = new ShutdownWrap(wrap->com);

  int r = uv_shutdown(&req_wrap->req_, wrap->stream_, AfterShutdown);

  req_wrap->Dispatched();

  if (r) {
    SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));
    delete req_wrap;
    RETURN_PARAM(JS_NULL());
  } else {
    JS_LOCAL_OBJECT objr = JS_OBJECT_FROM_PERSISTENT(req_wrap->object_);
    RETURN_PARAM(objr);
  }
}
JS_METHOD_END

void StreamWrap::AfterShutdown(uv_shutdown_t* req, int status) {
  ENGINE_LOG_THIS("StreamWrap", "AfterShutdown");
  ReqWrap<uv_shutdown_t>* req_wrap = (ReqWrap<uv_shutdown_t>*)req->data;
  StreamWrap* wrap = (StreamWrap*)req->handle->data;

  // The wrap and request objects should still be there.
  assert(JS_IS_EMPTY((req_wrap->object_)) == false);
  assert(JS_IS_EMPTY((wrap->object_)) == false);

  JS_ENTER_SCOPE_WITH(wrap->com->node_isolate);
  JS_DEFINE_STATE_MARKER(wrap->com);

  if (status) {
    SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));
  }

  JS_LOCAL_OBJECT objr = JS_OBJECT_FROM_PERSISTENT(req_wrap->object_);
#ifdef JS_ENGINE_V8
  JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(wrap->object_);
  node::commons* com = wrap->com;
  __JS_LOCAL_VALUE argv[3] = {STD_TO_INTEGER(status),
                              JS_TYPE_TO_LOCAL_VALUE(objl),
                              JS_TYPE_TO_LOCAL_VALUE(objr)};
#elif defined(JS_ENGINE_MOZJS)
  __JS_LOCAL_VALUE argv[3] = {JS::Int32Value(status),
                              (wrap->object_.GetRawValue()),
                              (objr.GetRawValue())};
#endif

  MakeCallback(wrap->com, objr, JS_PREDEFINED_STRING(oncomplete),
               ARRAY_SIZE(argv), argv);

  delete req_wrap;
}
}  // namespace node
