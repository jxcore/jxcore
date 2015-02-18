/* Copyright (c) 2014, Oguz Bastemur (oguz@bastemur.com)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "node.h"
#include "JXString.h"
#include "wrappers/node_buffer.h"
#include "string_bytes.h"
#include <stdio.h>
#include <stdlib.h>

namespace jxcore {

static inline size_t base64_decoded_size_fast(size_t size) {
  size_t remainder = size % 4;

  size = (size / 4) * 3;
  if (remainder) {
    if (size == 0 && remainder == 1) {
      // special case: 1-byte input cannot be decoded
      size = 0;
    } else {
      // non-padded input, add 1 or 2 extra bytes
      size += 1 + (remainder == 3);
    }
  }

  return size;
}

static inline size_t base64_decoded_size(const char *src, size_t size) {
  if (size == 0) return 0;

  if (src[size - 1] == '=') size--;
  if (size > 0 && src[size - 1] == '=') size--;

  return base64_decoded_size_fast(size);
}

#define STRING_SIZE(result, val, encoding)                                     \
  {                                                                            \
    size_t data_size = 0;                                                      \
                                                                               \
    if (encoding == node::BUFFER || encoding == node::BINARY) {                \
      if (node::Buffer::jxHasInstance(val, com)) result = BUFFER__LENGTH(val); \
    }                                                                          \
                                                                               \
    JS_LOCAL_STRING str = val->ToString();                                     \
                                                                               \
    switch (encoding) {                                                        \
      case node::UTF8:                                                         \
        if (!str->MayContainNonAscii())                                        \
          data_size = str->Length();                                           \
        else                                                                   \
          data_size = str->Utf8Length();                                       \
        break;                                                                 \
                                                                               \
      case node::BINARY:                                                       \
      case node::BUFFER:                                                       \
      case node::ASCII:                                                        \
        data_size = str->Length();                                             \
        break;                                                                 \
                                                                               \
      case node::UCS2:                                                         \
        data_size = str->Length() * sizeof(uint16_t);                          \
        break;                                                                 \
                                                                               \
      case node::BASE64: {                                                     \
        v8::String::AsciiValue value(str);                                     \
        data_size = base64_decoded_size(*value, value.length());               \
        break;                                                                 \
      }                                                                        \
                                                                               \
      case node::HEX:                                                          \
        data_size = str->Length() / 2;                                         \
        break;                                                                 \
                                                                               \
      default:                                                                 \
        assert(0 && "unknown encoding");                                       \
        break;                                                                 \
    }                                                                          \
    result = data_size;                                                        \
  }

#define COSTRUCT_ME(_value)                                    \
  if (JS_IS_EMPTY(_value)) {                                  \
    str_ = NULL;                                               \
    is_set_ = false;                                           \
    length_ = 0;                                               \
    return;                                                    \
  }                                                            \
                                                               \
  JS_ENTER_SCOPE_COM();                                        \
                                                               \
  JS_LOCAL_STRING val_ = _value->ToString();                   \
                                                               \
  size_t len;                                                  \
  STRING_SIZE(len, val_, node::UTF8);                          \
  len++;                                                       \
                                                               \
  char *cstr = static_cast<char *>(calloc(sizeof(char), len)); \
                                                               \
  int flags = node::WRITE_UTF8_FLAGS;                          \
  flags |= ~v8::String::NO_NULL_TERMINATION;                   \
                                                               \
  length_ = val_->WriteUtf8(cstr, len, 0, flags);              \
                                                               \
  str_ = reinterpret_cast<char *>(cstr);                       \
  is_set_ = true;

static void cpystr(char **dest, const char *src, const int ln) {
  if (src != NULL) {
    *dest = (char *)malloc(sizeof(char) * (ln + 1));
    memcpy(*dest, src, ln);
    char *s = *dest;
    *(s + ln) = '\0';
  } else {
    *dest = NULL;
  }
}

JXString::JXString() {
  autogc_ = true;
  str_ = NULL;
  is_set_ = false;
  length_ = 0;
}

char *JXString::operator*() { return str_; }

const char *JXString::operator*() const { return str_; }

JXString::JXString(JS_HANDLE_VALUE_CARRY value, void *_) {
  autogc_ = true;
  COSTRUCT_ME(value);
}

void JXString::set_std(const char *other, void *_) {
  if (is_set_) {
    free(str_);
  } else {
    is_set_ = true;
  }

  length_ = strlen(other);
  cpystr(&str_, other, length_);
}

void JXString::set_handle(JS_HANDLE_VALUE str, bool _) {
  if (is_set_) {
    free(str_);
    is_set_ = false;
  }

  COSTRUCT_ME(str);
}

JXString::JXString(const char *str, void *_) {
  autogc_ = true;
  if (str != NULL) {
    is_set_ = true;
    length_ = strlen(str);
    cpystr(&str_, str, length_);
  } else {
    is_set_ = false;
    length_ = 0;
    str_ = NULL;
  }
}

JXString::~JXString() {
  if (is_set_ && autogc_) free(str_);
}

int JXString::length() const { return length_; }

JS_HANDLE_STRING JXString::ToJSString() {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);
  return JS_LEAVE_SCOPE(STD_TO_STRING(str_));
}
}  // namespace jxcore
