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

#define STRING_SIZE(result, val)           \
  {                                        \
    JS_LOCAL_STRING str = val->ToString(); \
    if (!str->MayContainNonAscii())        \
      result = str->Length();              \
    else                                   \
      result = str->Utf8Length();          \
  }

#define COSTRUCT_ME(_value)                              \
  if (JS_IS_EMPTY(_value)) {                             \
    str_ = NULL;                                         \
    length_ = 0;                                         \
    return;                                              \
  }                                                      \
                                                         \
  JS_LOCAL_STRING val_ = _value->ToString();             \
                                                         \
  size_t len;                                            \
  STRING_SIZE(len, val_);                                \
  len++;                                                 \
                                                         \
  str_ = static_cast<char *>(calloc(sizeof(char), len)); \
  int flags = node::WRITE_UTF8_FLAGS;                    \
  flags |= ~v8::String::NO_NULL_TERMINATION;             \
                                                         \
  length_ = val_->WriteUtf8(str_, len, 0, flags);

static char *cpystr(const char *src, const int ln) {
  char *dest;
  if (src != NULL) {
    dest = (char *)malloc(sizeof(char) * (ln + 1));
    memcpy(dest, src, ln);
    *(dest + ln) = '\0';
  } else {
    dest = NULL;
  }

  return dest;
}

JXString::JXString() {
  autogc_ = true;
  str_ = NULL;
  length_ = 0;
}

char *JXString::operator*() { return str_; }

const char *JXString::operator*() const { return str_; }

JXString::JXString(JS_HANDLE_VALUE value, void *_) {
  autogc_ = true;
  COSTRUCT_ME(value);
}

void JXString::SetFromSTD(const char *other, const int length, void *_) {
  if (str_ != NULL) {
    free(str_);
  }

  length_ = length;
  str_ = cpystr(other, length_);
}

void JXString::SetFromHandle(JS_HANDLE_VALUE str, bool _) {
  if (str_ != NULL) {
    free(str_);
  }

  COSTRUCT_ME(str);
}

JXString::JXString(const char *str, void *_) {
  autogc_ = true;
  if (str != NULL) {
    // TODO(obastemur) make this utf-16 compatible
    length_ = strlen(str);
    str_ = cpystr(str, length_);
  } else {
    length_ = 0;
    str_ = NULL;
  }
}

JXString::~JXString() {
  if (autogc_) {
    Dispose();
  }
}

void JXString::Dispose() {
  if (str_ != NULL && length_ != 0) {
    free(str_);
    str_ = NULL;
    length_ = 0;
  }
}

}  // namespace jxcore
