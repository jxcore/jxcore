// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_NODE_BUFFER_H_
#define SRC_WRAPPERS_NODE_BUFFER_H_

#include "node.h"
#include "node_object_wrap.h"
#include <assert.h>
#include "jx/commons.h"

#define BUFFER__DATA(x) (char*) JS_GET_EXTERNAL_ARRAY_DATA(x)
#define BUFFER__LENGTH(x) (size_t) JS_GET_EXTERNAL_ARRAY_DATA_LENGTH(x)

namespace node {

#define BUFFER_CLASS_ID (0xBABE)

class Buffer;

#ifdef JS_ENGINE_V8
#define JS_RETAIN_VIRTUAL virtual
class RetainedBufferInfo : public JS_RETAINED_OBJECT_INFO {
#elif defined(JS_ENGINE_MOZJS)
#define JS_RETAIN_VIRTUAL
class RetainedBufferInfo {
#endif

 public:
  explicit RetainedBufferInfo(Buffer* buffer);
  JS_RETAIN_VIRTUAL void Dispose();
  JS_RETAIN_VIRTUAL bool IsEquivalent(JS_RETAINED_OBJECT_INFO* other);
  JS_RETAIN_VIRTUAL intptr_t GetHash();
  JS_RETAIN_VIRTUAL const char* GetLabel();
  JS_RETAIN_VIRTUAL intptr_t GetSizeInBytes();

 private:
  Buffer* buffer_;
  static const char label[];
};

#undef JS_RETAIN_VIRTUAL

#ifdef JS_ENGINE_V8
JS_RETAINED_OBJECT_INFO* WrapperInfo(uint16_t class_id,
                                     JS_HANDLE_VALUE wrapper);
/* A buffer is a chunk of memory stored outside the V8 heap, mirrored by an
 * object in javascript. The object is not totally opaque, one can access
 * individual bytes with [] and slice it into substrings or sub-buffers
 * without copying memory.
 */
#endif

class NODE_EXTERN Buffer : public ObjectWrap {
  node::commons *com_;
 public:
  static const unsigned int kMaxLength = 0x3fffffff;

  static bool HasInstance(JS_HANDLE_VALUE val);
  static bool jxHasInstance(JS_HANDLE_VALUE val, commons* com);

  static inline char* Data(JS_HANDLE_VALUE val) {
#ifdef JS_ENGINE_V8
    assert(JS_IS_OBJECT(val));
#endif
    void* data = JS_GET_EXTERNAL_ARRAY_DATA(val);
    return static_cast<char*>(data);
  }

  static inline char* Data(Buffer* b) { return Buffer::Data(b->handle_); }

  static inline size_t Length(JS_HANDLE_VALUE val) {
    assert(JS_IS_OBJECT(val));
    int len = JS_GET_EXTERNAL_ARRAY_DATA_LENGTH(val);
    return static_cast<size_t>(len);
  }

  static inline size_t Length(Buffer* b) { return Buffer::Length(b->handle_); }

  // This is verbose to be explicit with inline commenting
  static inline bool IsWithinBounds(size_t off, size_t len, size_t max) {
    // Asking to seek too far into the buffer
    // check to avoid wrapping in subsequent subtraction
    if (off > max) return false;

    // Asking for more than is left over in the buffer
    if (max - off < len) return false;

    // Otherwise we're in bounds
    return true;
  }

  ~Buffer();

  typedef void (*free_callback)(char* data, void* hint);

  // C++ API for constructing fast buffer
  static JS_HANDLE_OBJECT New(JS_HANDLE_STRING string);

  // public constructor
  static Buffer* New(size_t length, node::commons* com = NULL);
  // public constructor - data is copied
  static Buffer* New(const char* data, size_t len, node::commons* com = NULL);
  // public constructor
  static Buffer* New(char* data, size_t length, free_callback callback,
                     void* hint, node::commons* com = NULL);

 private:
  static DEFINE_JS_METHOD(New);
  static DEFINE_JS_METHOD(SetFastBufferConstructor);
  static DEFINE_JS_METHOD(BinarySlice);
  static DEFINE_JS_METHOD(AsciiSlice);
  static DEFINE_JS_METHOD(Base64Slice);
  static DEFINE_JS_METHOD(Utf8Slice);
  static DEFINE_JS_METHOD(Ucs2Slice);
  static DEFINE_JS_METHOD(HexSlice);

  static DEFINE_JS_METHOD(BinaryWrite);
  static DEFINE_JS_METHOD(Base64Write);
  static DEFINE_JS_METHOD(AsciiWrite);
  static DEFINE_JS_METHOD(Utf8Write);
  static DEFINE_JS_METHOD(Ucs2Write);
  static DEFINE_JS_METHOD(HexWrite);
  static DEFINE_JS_METHOD(ReadFloatLE);
  static DEFINE_JS_METHOD(ReadFloatBE);
  static DEFINE_JS_METHOD(ReadDoubleLE);
  static DEFINE_JS_METHOD(ReadDoubleBE);
  static DEFINE_JS_METHOD(WriteFloatLE);
  static DEFINE_JS_METHOD(WriteFloatBE);
  static DEFINE_JS_METHOD(WriteDoubleLE);
  static DEFINE_JS_METHOD(WriteDoubleBE);
  static DEFINE_JS_METHOD(ByteLength);
  static DEFINE_JS_METHOD(Utf8Length);
  static DEFINE_JS_METHOD(MakeFastBuffer);
  static DEFINE_JS_METHOD(Fill);
  static DEFINE_JS_METHOD(Copy);

  Buffer(node::commons *com, JS_HANDLE_OBJECT_REF wrapper, size_t length);
  void Replace(char* data, size_t length, free_callback callback, void* hint);

  bool disposing_;
  size_t length_;
  char* data_;
  free_callback callback_;
  void* callback_hint_;

 public:
  INIT_NAMED_CLASS_MEMBERS(SlowBuffer, Buffer) {
    com->bf_constructor_template =
        JS_NEW_PERSISTENT_FUNCTION_TEMPLATE(constructor);

    SET_INSTANCE_METHOD("binarySlice", Buffer::BinarySlice, 0);
    SET_INSTANCE_METHOD("asciiSlice", Buffer::AsciiSlice, 0);
    SET_INSTANCE_METHOD("base64Slice", Buffer::Base64Slice, 0);
    SET_INSTANCE_METHOD("ucs2Slice", Buffer::Ucs2Slice, 0);
    SET_INSTANCE_METHOD("hexSlice", Buffer::HexSlice, 0);
    SET_INSTANCE_METHOD("utf8Slice", Buffer::Utf8Slice, 0);
    SET_INSTANCE_METHOD("utf8Write", Buffer::Utf8Write, 0);
    SET_INSTANCE_METHOD("asciiWrite", Buffer::AsciiWrite, 3);
    SET_INSTANCE_METHOD("binaryWrite", Buffer::BinaryWrite, 3);
    SET_INSTANCE_METHOD("base64Write", Buffer::Base64Write, 3);
    SET_INSTANCE_METHOD("ucs2Write", Buffer::Ucs2Write, 3);
    SET_INSTANCE_METHOD("hexWrite", Buffer::HexWrite, 3);
    SET_INSTANCE_METHOD("readFloatLE", Buffer::ReadFloatLE, 0);
    SET_INSTANCE_METHOD("readFloatBE", Buffer::ReadFloatBE, 0);
    SET_INSTANCE_METHOD("readDoubleLE", Buffer::ReadDoubleLE, 0);
    SET_INSTANCE_METHOD("readDoubleBE", Buffer::ReadDoubleBE, 0);
    SET_INSTANCE_METHOD("writeFloatLE", Buffer::WriteFloatLE, 0);
    SET_INSTANCE_METHOD("writeFloatBE", Buffer::WriteFloatBE, 0);
    SET_INSTANCE_METHOD("writeDoubleLE", Buffer::WriteDoubleLE, 0);
    SET_INSTANCE_METHOD("writeDoubleBE", Buffer::WriteDoubleBE, 0);
    SET_INSTANCE_METHOD("fill", Buffer::Fill, 0);
    SET_INSTANCE_METHOD("copy", Buffer::Copy, 0);

    SET_CLASS_METHOD("byteLength", Buffer::ByteLength, 1);
    SET_CLASS_METHOD("Utf8Length", Buffer::Utf8Length, 0);
    SET_CLASS_METHOD("makeFastBuffer", Buffer::MakeFastBuffer, 0);

    JS_NAME_SET(target, JS_STRING_ID("setFastBufferConstructor"),
                JS_GET_FUNCTION(
                    JS_NEW_FUNCTION_CALL_TEMPLATE(SetFastBufferConstructor)));
#ifdef JS_ENGINE_V8
    ENGINE_NS::HeapProfiler::DefineWrapperClass(BUFFER_CLASS_ID, WrapperInfo);
#endif
  }
  END_INIT_NAMED_MEMBERS(SlowBuffer)
};

}  // namespace node
#endif  // SRC_WRAPPERS_NODE_BUFFER_H_
