// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_EXTERN_STRING_H_
#define SRC_JX_EXTERN_STRING_H_

#include "../JSEngine.h"

#include <string.h> /* strdup() */
#if !defined(_MSC_VER)
#include <strings.h> /* strcasecmp() */
#else
#define strcasecmp _stricmp
#endif
#include <stdlib.h> /* free() */

namespace node {

template <typename ResourceType, typename TypeName>
class ExternString : public ResourceType {
 public:
  ~ExternString() {
    delete[] data_;
    int64_t change_in_bytes = -static_cast<int64_t>(length_);
    isolate()->AdjustAmountOfExternalAllocatedMemory(change_in_bytes);
  }

  const TypeName* data() const { return data_; }

  size_t length() const { return length_; }

  static v8::Local<v8::String> NewFromCopy(v8::Isolate* isolate,
                                           const TypeName* data,
                                           size_t length) {
    v8::EscapableHandleScope scope(isolate);

    if (length == 0) return scope.Escape(v8::String::Empty(isolate));

    TypeName* new_data = new TypeName[length];
    memcpy(new_data, data, length * sizeof(*new_data));

    return scope.Escape(
        ExternString<ResourceType, TypeName>::New(isolate, new_data, length));
  }

  // uses "data" for external resource, and will be free'd on gc
  static v8::Local<v8::String> New(v8::Isolate* isolate, const TypeName* data,
                                   size_t length) {
    v8::EscapableHandleScope scope(isolate);

    if (length == 0) return scope.Escape(v8::String::Empty(isolate));

    ExternString* h_str =
        new ExternString<ResourceType, TypeName>(isolate, data, length);
    v8::Local<v8::String> str = v8::String::NewExternal(isolate, h_str);
    isolate->AdjustAmountOfExternalAllocatedMemory(length);

    return scope.Escape(str);
  }

  inline v8::Isolate* isolate() const { return isolate_; }

 private:
  ExternString(v8::Isolate* isolate, const TypeName* data, size_t length)
      : isolate_(isolate), data_(data), length_(length) {}
  v8::Isolate* isolate_;
  const TypeName* data_;
  size_t length_;
};

#define EXTERN_APEX 0xFBEE9

enum Endianness {
  kLittleEndian,  // _Not_ LITTLE_ENDIAN, clashes with endian.h.
  kBigEndian
};

inline enum Endianness GetEndianness() {
  // Constant-folded by the compiler.
  const union {
    uint8_t u8[2];
    uint16_t u16;
  } u = {{1, 0}};
  return u.u16 == 1 ? kLittleEndian : kBigEndian;
}

inline bool IsLittleEndian() { return GetEndianness() == kLittleEndian; }

inline bool IsBigEndian() { return GetEndianness() == kBigEndian; }

static inline v8::Local<v8::String> STD_TO_STRING_WITH_LENGTH_(
    v8::Isolate* i, const char* str, unsigned length) {
  return v8::String::NewFromUtf8(i, str, v8::String::kNormalString, length);
}

static inline v8::Local<v8::String> STD_TO_STRING_WITH_LENGTH_(
    v8::Isolate* i, const uint16_t* str, unsigned length) {
  return node::ExternString<v8::String::ExternalStringResource, uint16_t>::
      NewFromCopy(i, str, length);
}
}
#endif
