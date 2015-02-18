// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_STREAM_WRAP_H_
#define SRC_WRAPPERS_STREAM_WRAP_H_

#include "node.h"
#include "wrappers/handle_wrap.h"
#include "string_bytes.h"
#include "jx/commons.h"

namespace node {

class StreamWrap : public HandleWrap {
 public:
  uv_stream_t* GetStream() { return stream_; }

  static void Initialize(JS_HANDLE_OBJECT target);

  static JS_DEFINE_GETTER_METHOD(GetFD);

  // JavaScript functions
  static DEFINE_JS_METHOD(ReadStart);
  static DEFINE_JS_METHOD(ReadStop);
  static DEFINE_JS_METHOD(Shutdown);

  static DEFINE_JS_METHOD(WriteBuffer);
  static DEFINE_JS_METHOD(WriteAsciiString);
  static DEFINE_JS_METHOD(WriteUtf8String);
  static DEFINE_JS_METHOD(WriteUcs2String);

 protected:
  StreamWrap(JS_HANDLE_OBJECT object, uv_stream_t* stream);
  virtual void SetHandle(uv_handle_t* h);
  void StateChange() {}
  // void UpdateWriteQueueSize();
  void UpdateWriteQueueSize(const commons* com);

 private:
  static inline char* NewSlab(JS_HANDLE_OBJECT global,
                              JS_HANDLE_OBJECT wrap_obj);

  // Callbacks for libuv
  static void AfterWrite(uv_write_t* req, int status);
  static uv_buf_t OnAlloc(uv_handle_t* handle, size_t suggested_size);
  static void AfterShutdown(uv_shutdown_t* req, int status);

  static void OnRead(uv_stream_t* handle, ssize_t nread, uv_buf_t buf);
  static void OnRead2(uv_pipe_t* handle, ssize_t nread, uv_buf_t buf,
                      uv_handle_type pending);
  static void OnReadCommon(uv_stream_t* handle, ssize_t nread, uv_buf_t buf,
                           uv_handle_type pending);

  template <enum encoding encoding>
  static JS_NATIVE_RETURN_TYPE WriteStringImpl(jxcore::PArguments& args);

  size_t slab_offset_;
  uv_stream_t* stream_;
};

}  // namespace node

#endif  // SRC_WRAPPERS_STREAM_WRAP_H_
