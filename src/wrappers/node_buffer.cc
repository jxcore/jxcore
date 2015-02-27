// Copyright & License details are available under JXCORE_LICENSE file

#include "node_buffer.h"
#include "string_bytes.h"

#include <assert.h>
#include <string.h>  // memcpy
#include <limits.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

namespace node {

#define SLICE_ARGS(start_arg, end_arg)                                      \
  if (!args.IsInteger(start_arg) || !args.IsInteger(end_arg)) {             \
    THROW_TYPE_EXCEPTION("Bad argument. expects(integer, integer)");        \
  }                                                                         \
  int32_t start = args.GetInt32(start_arg);                                 \
  int32_t end = args.GetInt32(end_arg);                                     \
  if (start < 0 || end < 0) {                                               \
    THROW_TYPE_EXCEPTION("Bad argument. expects parameters bigger than 0"); \
  }                                                                         \
  if (!(start <= end)) {                                                    \
    THROW_EXCEPTION("Must have start <= end");                              \
  }                                                                         \
  if ((size_t)end > parent->length_) {                                      \
    THROW_EXCEPTION("end cannot be longer than parent.length");             \
  }

JS_HANDLE_OBJECT Buffer::New(JS_HANDLE_STRING string) {
  ENGINE_LOG_THIS("Buffer", "New1");
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  // get Buffer from global scope.
  JS_LOCAL_OBJECT global = JS_GET_GLOBAL();
  JS_LOCAL_VALUE bv = JS_GET_NAME(global, JS_PREDEFINED_STRING(Buffer));
  assert(JS_IS_FUNCTION(bv));
  JS_LOCAL_FUNCTION b = JS_CAST_FUNCTION(bv);

  JS_LOCAL_VALUE argv[1] = {JS_TYPE_TO_LOCAL_VALUE(string)};
  JS_LOCAL_OBJECT instance = JS_NEW_INSTANCE(b, 1, argv);

  return JS_LEAVE_SCOPE(instance);
}

Buffer* Buffer::New(size_t length, node::commons* com) {
  ENGINE_LOG_THIS("Buffer", "New2");
  JS_ENTER_SCOPE();
  if (com == NULL) com = node::commons::getInstance();
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_VALUE arg = STD_TO_UNSIGNED(length);
  JS_LOCAL_FUNCTION fnc = JS_GET_FUNCTION(com->bf_constructor_template);
  JS_LOCAL_OBJECT b = JS_NEW_INSTANCE(fnc, 1, &arg);

  if (JS_IS_EMPTY(b)) return NULL;

  return ObjectWrap::Unwrap<Buffer>(b);
}

Buffer* Buffer::New(const char* data, size_t length, node::commons* com) {
  ENGINE_LOG_THIS("Buffer", "New3");
  JS_ENTER_SCOPE();
  if (com == NULL) com = node::commons::getInstance();
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_VALUE arg = STD_TO_UNSIGNED(0);
  JS_LOCAL_FUNCTION fnc = JS_GET_FUNCTION(com->bf_constructor_template);
  JS_LOCAL_OBJECT obj = JS_NEW_INSTANCE(fnc, 1, &arg);

  Buffer* buffer = ObjectWrap::Unwrap<Buffer>(obj);
  buffer->Replace(const_cast<char*>(data), length, NULL, NULL);

  return buffer;
}

Buffer* Buffer::New(char* data, size_t length, free_callback callback,
                    void* hint, node::commons* com) {
  ENGINE_LOG_THIS("Buffer", "New4");
  JS_ENTER_SCOPE();
  if (com == NULL) com = node::commons::getInstance();
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_VALUE arg = STD_TO_UNSIGNED(0);
  JS_LOCAL_FUNCTION fnc = JS_GET_FUNCTION(com->bf_constructor_template);
  JS_LOCAL_OBJECT obj = JS_NEW_INSTANCE(fnc, 1, &arg);

  Buffer* buffer = ObjectWrap::Unwrap<Buffer>(obj);
  buffer->Replace(data, length, callback, hint);

  return buffer;
}

JS_METHOD(Buffer, New) {
  if (!args.IsConstructCall()) {
    RETURN_PARAM(FromConstructorTemplateX(com->bf_constructor_template, args));
  }

  int64_t length = args.GetUInteger(0);

  if (length < 0)
    THROW_TYPE_EXCEPTION("Buffer::New Bad argument (expects positive integer)");

  if (length > Buffer::kMaxLength) {
    THROW_RANGE_EXCEPTION("Buffer::New Bad argument (length > kMaxLength)");
  }

  JS_CLASS_NEW_INSTANCE(obj, SlowBuffer);

  new Buffer(com, obj, length);

  RETURN_PARAM(obj);
}
JS_METHOD_END

Buffer::Buffer(node::commons* com, JS_HANDLE_OBJECT_REF wrapper, size_t length)
    : ObjectWrap() {
  Wrap(wrapper);

  com_ = com;
  disposing_ = false;
  length_ = 0;
  callback_ = NULL;
#ifdef JS_ENGINE_V8
  handle_.SetWrapperClassId(BUFFER_CLASS_ID);
#endif

  Replace(NULL, length, NULL, NULL);
}

Buffer::~Buffer() {
  disposing_ = true;
#ifdef JS_ENGINE_MOZJS
  if (com_->instance_status_ == JXCORE_INSTANCE_EXITED ||
      node::commons::process_status_ == JXCORE_INSTANCE_EXITED)
    return;
#endif
  Replace(NULL, 0, NULL, NULL);
}

// if replace doesn't have a callback, data must be copied
// const_cast in Buffer::New requires this
void Buffer::Replace(char* data, size_t length, free_callback callback,
                     void* hint) {
  ENGINE_LOG_THIS("Buffer", "Replace");
  JS_ENTER_SCOPE();
  node::commons* com = com_;
  JS_DEFINE_STATE_MARKER(com);

  if (callback_) {
    callback_(data_, callback_hint_);
  } else if (length_) {
#ifdef JS_ENGINE_V8
    delete[] data_;
    JS_ADJUST_EXTERNAL_MEMORY(-static_cast<intptr_t>(sizeof(Buffer) + length_));
#elif defined(JS_ENGINE_MOZJS)
    JS_free(JS_GET_STATE_MARKER(), data_);
#endif
  }

  length_ = length;
  callback_ = callback;
  callback_hint_ = hint;

  if (callback_) {
    data_ = data;
  } else if (length_) {
#ifdef JS_ENGINE_V8
    data_ = new char[length_];
    JS_ADJUST_EXTERNAL_MEMORY(sizeof(Buffer) + length_);
#elif defined(JS_ENGINE_MOZJS)
    data_ = (char*)JS_malloc(JS_GET_STATE_MARKER(), sizeof(char) * length_);
#endif
    if (data) memcpy(data_, data, length_);
  } else {
    data_ = NULL;
  }

  if (disposing_) return;

  JS_LOCAL_VALUE val_len = STD_TO_UNSIGNED(length_);
  JS_SET_INDEXED_EXTERNAL(handle_, data_, ENGINE_NS::kExternalUnsignedByteArray,
                          length_);

  JS_NAME_SET(handle_, JS_PREDEFINED_STRING(length), val_len);
}

#define SLICER(name, encoding)                            \
  JS_METHOD_NO_COM(Buffer, name) Buffer* parent =         \
      ObjectWrap::Unwrap<Buffer>(args.This());            \
  SLICE_ARGS(0, 1);                                       \
                                                          \
  const char* src = parent->data_ + start;                \
  size_t slen = (end - start);                            \
  RETURN_PARAM(StringBytes::Encode(src, slen, encoding)); \
  JS_METHOD_END

SLICER(BinarySlice, BINARY)
SLICER(AsciiSlice, ASCII)
SLICER(Utf8Slice, UTF8)
SLICER(Ucs2Slice, UCS2)
SLICER(HexSlice, HEX)
SLICER(Base64Slice, BASE64)

JS_METHOD_NO_COM(Buffer, Fill) {
  if (!args.IsInteger(0) || !args.IsInteger(1) || !args.IsInteger(2)) {
    THROW_TYPE_EXCEPTION(
        "Buffer.fill(value, start,end) requires (int, int, int)");
  }

  int value = (char)args.GetInteger(0);

  Buffer* parent = ObjectWrap::Unwrap<Buffer>(args.This());

  int32_t start = (int32_t)args.GetInteger(1);
  int32_t end = (int32_t)args.GetInteger(2);
  if (start < 0 || end < 0) {
    THROW_EXCEPTION(
        "Buffer.fill(value, start,end) -> start or end can not be smaller than "
        "0");
  }

  if (!(start <= end)) {
    THROW_EXCEPTION("Buffer.fill(value, start,end) -> start must be <= end");
  }

  if ((size_t)end > parent->length_) {
    THROW_EXCEPTION(
        "Buffer.fill(value, start,end) -> end can not be bigger than parent's "
        "length");
  }

  memset((void*)(parent->data_ + start), value, end - start);
}
JS_METHOD_END

JS_METHOD(Buffer, Copy) {
  Buffer* source = ObjectWrap::Unwrap<Buffer>(args.This());

  if (!Buffer::jxHasInstance(GET_ARG(0), com)) {
    THROW_TYPE_EXCEPTION("First argument should be a Buffer");
  }

  JS_LOCAL_VALUE target = GET_ARG(0);
  char* target_data = BUFFER__DATA(target);
  size_t target_length = BUFFER__LENGTH(target);

  size_t target_start = args.IsUndefined(1) ? 0 : args.GetUInteger(1);
  size_t source_start = args.IsUndefined(2) ? 0 : args.GetUInteger(2);
  size_t source_end =
      args.IsUndefined(3) ? source->length_ : args.GetUInteger(3);

  if (source_end < source_start) {
    THROW_RANGE_EXCEPTION("sourceEnd < sourceStart");
  }

  // Copy 0 bytes; we're done
  if (source_end == source_start) {
    RETURN_PARAM(STD_TO_INTEGER(0));
  }

  if (target_start >= target_length) {
    THROW_RANGE_EXCEPTION("targetStart out of bounds");
  }

  if (source_start >= source->length_) {
    THROW_RANGE_EXCEPTION("sourceStart out of bounds");
  }

  if (source_end > source->length_) {
    THROW_RANGE_EXCEPTION("sourceEnd out of bounds");
  }

  size_t to_copy =
      MIN(MIN(source_end - source_start, target_length - target_start),
          source->length_ - source_start);

  // need to use slightly slower memmove is the ranges might overlap
  memmove((void*)(target_data + target_start),
          (const void*)(source->data_ + source_start), to_copy);

  RETURN_PARAM(STD_TO_INTEGER(to_copy));
}
JS_METHOD_END

#define WRITER(name, encoding)                                              \
  JS_METHOD(Buffer, name) Buffer* buffer =                                  \
      ObjectWrap::Unwrap<Buffer>(args.This());                              \
                                                                            \
  if (!args.IsString(0)) {                                                  \
    THROW_TYPE_EXCEPTION("Argument must be a string");                      \
  }                                                                         \
                                                                            \
  JS_HANDLE_STRING str = args.GetAsString(0);                               \
                                                                            \
  int length = JS_GET_STRING_LENGTH(str);                                   \
                                                                            \
  if (length == 0) {                                                        \
    JS_LOCAL_VALUE val_zero = STD_TO_INTEGER(0);                            \
    JS_NAME_SET(JS_GET_CONSTRUCTOR((com->bf_constructor_template)),         \
                JS_PREDEFINED_STRING(_charsWritten), val_zero);             \
    RETURN_POINTER(val_zero);                                               \
  }                                                                         \
                                                                            \
  if (encoding == HEX && length % 2 != 0) {                                 \
    THROW_TYPE_EXCEPTION("Invalid hex string");                             \
  }                                                                         \
                                                                            \
  size_t offset = args.GetInt32(1);                                         \
  size_t max_length =                                                       \
      args.IsUndefined(2) ? buffer->length_ - offset : args.GetUInteger(2); \
  max_length = MIN(buffer->length_ - offset, max_length);                   \
                                                                            \
  if (max_length == 0) {                                                    \
    /* shortcut: nothing to write anyway */                                 \
    JS_LOCAL_VALUE val = STD_TO_INTEGER(0);                                 \
    JS_NAME_SET(JS_GET_CONSTRUCTOR((com->bf_constructor_template)),         \
                JS_PREDEFINED_STRING(_charsWritten), val);                  \
    RETURN_POINTER(val);                                                    \
  }                                                                         \
                                                                            \
  if (encoding == UCS2) {                                                   \
    max_length = max_length / 2;                                            \
  }                                                                         \
                                                                            \
  if (offset >= buffer->length_) {                                          \
    THROW_TYPE_EXCEPTION("Offset is out of bounds");                        \
  }                                                                         \
                                                                            \
  char* start = buffer->data_ + offset;                                     \
  int chars_written;                                                        \
  size_t written =                                                          \
      StringBytes::Write(start, max_length, str, encoding, &chars_written); \
                                                                            \
  JS_LOCAL_VALUE val_chars_written = STD_TO_INTEGER(chars_written);         \
  JS_NAME_SET(JS_GET_CONSTRUCTOR((com->bf_constructor_template)),           \
              JS_PREDEFINED_STRING(_charsWritten), val_chars_written);      \
                                                                            \
  JS_LOCAL_VALUE val_written = STD_TO_INTEGER(written);                     \
  RETURN_POINTER(val_written);                                              \
  JS_METHOD_END

WRITER(Base64Write, BASE64)
WRITER(BinaryWrite, BINARY)
WRITER(Utf8Write, UTF8)
WRITER(Ucs2Write, UCS2)
WRITER(HexWrite, HEX)
WRITER(AsciiWrite, ASCII)

static bool is_big_endian() {
  const union {
    uint8_t u8[2];
    uint16_t u16;
  } u = {{0, 1}};
  return u.u16 == 1 ? true : false;
}

static void swizzle(char* buf, size_t len) {
  char t;
  for (size_t i = 0; i < len / 2; ++i) {
    t = buf[i];
    buf[i] = buf[len - i - 1];
    buf[len - i - 1] = t;
  }
}

#define BufferReadFloat(name, T, ENDIANNESS)                                 \
  JS_METHOD(Buffer, name) double offset_tmp = args.GetNumber(0);             \
  int64_t offset = static_cast<int64_t>(offset_tmp);                         \
  bool doAssert = !args.GetBoolean(1);                                       \
                                                                             \
  if (doAssert) {                                                            \
    if (offset_tmp != offset || offset < 0) {                                \
      THROW_TYPE_EXCEPTION("offset is not uint");                            \
    }                                                                        \
    size_t len =                                                             \
        static_cast<size_t>(JS_GET_EXTERNAL_ARRAY_DATA_LENGTH(args.This())); \
                                                                             \
    if (offset + sizeof(T) > len) {                                          \
      THROW_RANGE_EXCEPTION("Trying to read beyond buffer length");          \
    }                                                                        \
  }                                                                          \
                                                                             \
  T val;                                                                     \
  char* data = static_cast<char*>(JS_GET_EXTERNAL_ARRAY_DATA(args.This()));  \
  char* ptr = data + offset;                                                 \
                                                                             \
  memcpy(&val, ptr, sizeof(T));                                              \
  if (ENDIANNESS != is_big_endian()) {                                       \
    swizzle(reinterpret_cast<char*>(&val), sizeof(T));                       \
  }                                                                          \
  RETURN_PARAM(STD_TO_NUMBER(val));                                          \
  JS_METHOD_END

BufferReadFloat(ReadFloatLE, float, false);
BufferReadFloat(ReadFloatBE, float, true);
BufferReadFloat(ReadDoubleLE, double, false);
BufferReadFloat(ReadDoubleBE, double, true);

#define BufferWriteFloat(name, T, ENDIANNESS)                                \
  JS_METHOD(Buffer, name) bool doAssert = !args.GetBoolean(2);               \
                                                                             \
  if (doAssert) {                                                            \
    if (!args.IsNumber(0)) THROW_TYPE_EXCEPTION("value not a number");       \
    if (!args.IsUnsigned(1)) THROW_TYPE_EXCEPTION("offset is not uint");     \
  }                                                                          \
                                                                             \
  T val = static_cast<T>(args.GetNumber(0));                                 \
  size_t offset = args.GetUInteger(1);                                       \
  char* data = static_cast<char*>(JS_GET_EXTERNAL_ARRAY_DATA(args.This()));  \
  char* ptr = data + offset;                                                 \
                                                                             \
  if (doAssert) {                                                            \
    size_t len =                                                             \
        static_cast<size_t>(JS_GET_EXTERNAL_ARRAY_DATA_LENGTH(args.This())); \
    if (offset + sizeof(T) > len || offset + sizeof(T) < offset)             \
      THROW_RANGE_EXCEPTION("Trying to write beyond buffer length");         \
  }                                                                          \
                                                                             \
  memcpy(ptr, &val, sizeof(T));                                              \
  if (ENDIANNESS != is_big_endian()) {                                       \
    swizzle(ptr, sizeof(T));                                                 \
  }                                                                          \
  JS_METHOD_END

BufferWriteFloat(WriteFloatLE, float, false);
BufferWriteFloat(WriteFloatBE, float, true);
BufferWriteFloat(WriteDoubleLE, double, false);
BufferWriteFloat(WriteDoubleBE, double, true);

// var nbytes = Buffer.byteLength("string", "utf8")
JS_METHOD(Buffer, ByteLength) {
  if (!args.IsString(0)) {
    THROW_TYPE_EXCEPTION("Argument must be a string");
  }

  JS_LOCAL_STRING s = JS_VALUE_TO_STRING(args.GetAsString(0));

  enum encoding e;
  if (!args.IsString(1)) {
    e = UTF8;
  } else {
    jxcore::JXString str;
    args.GetString(1, &str);
    e = ParseEncoding(*str, str.length(), UTF8);
  }

  RETURN_PARAM(STD_TO_INTEGER(StringBytes::Size(s, e)));
}
JS_METHOD_END

JS_METHOD(Buffer, Utf8Length) {
  if (!args.IsString(0)) {
    THROW_TYPE_EXCEPTION("Argument must be a string");
  }

  RETURN_PARAM(STD_TO_INTEGER(args.GetUTF8Length(0)));
}
JS_METHOD_END

JS_METHOD(Buffer, MakeFastBuffer) {
  JS_LOCAL_OBJECT fitem = JS_VALUE_TO_OBJECT(GET_ARG(0));
  if (!Buffer::jxHasInstance(fitem, com)) {
    THROW_TYPE_EXCEPTION("First argument must be a Buffer.");
  }

  Buffer* buffer = ObjectWrap::Unwrap<Buffer>(fitem);
  JS_LOCAL_OBJECT fast_buffer = JS_VALUE_TO_OBJECT(GET_ARG(1));
  uint32_t offset = args.GetUInteger(2);
  uint32_t length = args.GetUInteger(3);

  if (offset > buffer->length_) {
    THROW_RANGE_EXCEPTION("offset out of range");
  }

  if (offset + length > buffer->length_) {
    THROW_RANGE_EXCEPTION("length out of range");
  }

  // Check for wraparound. Safe because offset and length are unsigned.
  if (offset + length < offset) {
    THROW_RANGE_EXCEPTION("offset or length out of range");
  }

  JS_SET_INDEXED_EXTERNAL(fast_buffer, buffer->data_ + offset,
                          ENGINE_NS::kExternalUnsignedByteArray, length);
}
JS_METHOD_END

bool Buffer::jxHasInstance(JS_HANDLE_VALUE val, commons* com) {
#if defined(JS_ENGINE_MOZJS)
  // TODO(obastemur) currently SM identifies ExternalArray as Buffer
  // do we need to change it for embedders ? or they should use TypedArrays
  // whenever they want to implement Buffer like custom proxy classes.
  // embedders can use Buffers on native land without an issue.
  //
  // P.S. The below V8 implementation also works for SM but not needed.

  return val.HasBufferSignature();
#elif defined(JS_ENGINE_V8)
  if (!JS_IS_OBJECT(val)) return false;
  JS_LOCAL_OBJECT obj = JS_VALUE_TO_OBJECT(val);

  ENGINE_NS::ExternalArrayType type = JS_GET_EXTERNAL_ARRAY_DATA_TYPE(obj);
  if (type != ENGINE_NS::kExternalUnsignedByteArray) return false;

  // Also check for SlowBuffers that are empty.
  if (JS_HAS_INSTANCE(com->bf_constructor_template, obj)) return true;

  assert(!JS_IS_EMPTY((com->fast_buffer_constructor)));
  return JS_COMPARE_BY_CONSTRUCTOR(obj, com->fast_buffer_constructor);
#endif
}

bool Buffer::HasInstance(JS_HANDLE_VALUE val) {
  commons* com = node::commons::getInstance();
  return jxHasInstance(val, com);
}

JS_METHOD(Buffer, SetFastBufferConstructor) {
  assert(args.IsFunction(0));

  JS_LOCAL_FUNCTION fnc = TO_LOCAL_FUNCTION(args.GetAsFunction(0));
  com->fast_buffer_constructor = JS_NEW_PERSISTENT_FUNCTION(fnc);
}
JS_METHOD_END

const char RetainedBufferInfo::label[] = "Buffer";

RetainedBufferInfo::RetainedBufferInfo(Buffer* buffer) : buffer_(buffer) {}

void RetainedBufferInfo::Dispose() {
  buffer_ = NULL;
  delete this;
}

bool RetainedBufferInfo::IsEquivalent(JS_RETAINED_OBJECT_INFO* other) {
  return label == other->GetLabel() &&
         buffer_ == static_cast<RetainedBufferInfo*>(other)->buffer_;
}

intptr_t RetainedBufferInfo::GetHash() {
  return reinterpret_cast<intptr_t>(buffer_);
}

const char* RetainedBufferInfo::GetLabel() { return label; }

intptr_t RetainedBufferInfo::GetSizeInBytes() {
  return Buffer::Length(buffer_);
}

#ifdef JS_ENGINE_V8
JS_RETAINED_OBJECT_INFO* WrapperInfo(uint16_t class_id,
                                     JS_HANDLE_VALUE wrapper) {
  assert(class_id == BUFFER_CLASS_ID);

  Buffer* buffer = Buffer::Unwrap<Buffer>(JS_TYPE_AS_OBJECT(wrapper));
  return new RetainedBufferInfo(buffer);
}
#endif

}  // namespace node

NODE_MODULE(node_buffer, node::Buffer::Initialize)
