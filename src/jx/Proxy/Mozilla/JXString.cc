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

#include "JXString.h"
#include "../EngineLogger.h"
#include <stdio.h>
#include <stdlib.h>

namespace jxcore {

static char empty_str = '\0';

static char *cpystr(JSContext *ctx, const char *src, const int ln) {
  char *dest;
  if (src != NULL) {
    dest = (char *)JS_malloc(ctx, sizeof(char) * (ln + 1));
    memcpy(dest, src, ln);
    *(dest + ln) = '\0';
  } else {
    dest = NULL;
  }

  return dest;
}

JXString::JXString() {
  str_ = NULL;
  ctx_ = NULL;
  length_ = 0;
  utf8_length_ = 0;
  autogc_ = true;
  value_ = NULL;
  ctx_ = NULL;
  ascii_char_set_ = false;
}

JXString::JXString(const JS_HANDLE_VALUE_REF str, void *ctx) {
  str_ = NULL;
  autogc_ = true;
  ctx_ = str.GetContext();
  length_ = 0;
  value_ = str.GetRawStringPointer();
  ascii_char_set_ = false;
  SetFromHandle();
}

char *JXString::operator*() { return str_; }

const char *JXString::operator*() const { return str_; }

JXString::JXString(JSString *str, JSContext *ctx, bool autogc, bool get_ascii) {
  autogc_ = autogc;
  str_ = NULL;
  value_ = str;
  ctx_ = ctx;
  length_ = 0;
  ascii_char_set_ = get_ascii;
  if (!ascii_char_set_)
    SetFromHandle();
  else
    GetASCII();
}

void JXString::SetFromSTD(const char *other, JSContext *iso) {
  if (ctx_ == NULL) {
    assert(iso != NULL);
    ctx_ = iso;
  }

  if (str_ != NULL) {
    JS_free(ctx_, str_);
    str_ = NULL;
  }

  length_ = strlen(other);
  if (length_ != 0) {
    str_ = cpystr(ctx_, other, length_);
  } else {
    str_ = &empty_str;
  }
  utf8_length_ = length_;
}

void JXString::SetFromHandle() {
  assert(value_ != nullptr);
  if (length_ != 0) {
    JS_free(ctx_, str_);
    str_ = NULL;
    length_ = 0;
  }

  if (value_ != nullptr) {
    JS::RootedString rs_value_(ctx_, value_);
    str_ = JS_EncodeStringToUTF8AndLength(ctx_, rs_value_, utf8_length_);
    if (str_ != nullptr) {
      length_ = JS_GetStringEncodingLengthUnsafe(value_);
      ascii_char_set_ = false;
      return;
    }
  }

  str_ = &empty_str;
  utf8_length_ = 0;
  length_ = 0;
}

void JXString::GetUTF8LetterAt(const size_t index, MozJS::auto_str *chars) {
  chars->ctx_ = ctx_;
  if (ascii_char_set_) {
    chars->str_ = (char *)JS_malloc(ctx_, sizeof(char));
    chars->str_[0] = str_[index];
    chars->length_ = 1;
  } else {
    size_t n = 0;
    int len = 0;
    for (size_t i = 0; i <= index && n < utf8_length_; i++) {
      char ch = str_[n];
      if ((ch & 0x80) == 0) {
        len = 1;
        n++;
      } else if ((ch & 0xE0) == 0xC0) {
        len = 2;
        n += 2;
      } else if ((ch & 0xF0) == 0xE0) {
        len = 3;
        n += 3;
      } else if ((ch & 0xF8) == 0xF0) {
        len = 4;
        n += 4;
      } else {
        flush_console("Unrecognized UTF-8 String\n");
        abort();
      }
    }
    n -= len;
    chars->str_ = (char *)JS_malloc(ctx_, sizeof(char) * len);
    memcpy(chars->str_, str_ + n, len);
    chars->length_ = len;
  }
}

size_t JXString::WriteUtf8(char *buf, const size_t buflen, int *chars_written) {
  // ENGINE_LOG_THIS("String", "WriteUtf8");
  size_t len = utf8_length_;
  int cw = length_;
  if (utf8_length_ <= buflen) {
    memcpy(buf, str_, len);
  } else if (utf8_length_ == length_) {
    memcpy(buf, str_, buflen);
    len = buflen;
  } else {
    size_t total_written = 0;
    cw = 0;
    for (; cw < length_; cw++) {
      MozJS::auto_str out;
      GetUTF8LetterAt(cw, &out);
      if (out.length_ > buflen - total_written) break;
      memcpy(buf + total_written, out.str_, out.length_);
      total_written += out.length_;
    }
    len = total_written;
  }
  if (chars_written != NULL) *chars_written = cw;
  return len;
}

void JXString::GetASCII() {
  if (length_ != 0) {
    JS_free(ctx_, str_);
    str_ = NULL;
    length_ = 0;
  }

  if (value_ != nullptr) {
    ascii_char_set_ = true;
    length_ = JS_GetStringEncodingLength(ctx_, value_);
  }

  if (length_ > 0) {
    utf8_length_ = length_;
    str_ = JS_EncodeString(ctx_, value_);
  } else {
    length_ = 0;
    utf8_length_ = length_;
    str_ = &empty_str;
  }
}

void JXString::SetFromHandle(const JS_HANDLE_VALUE_REF str, bool get_ascii) {
  value_ = str.GetRawStringPointer();
  ctx_ = str.GetContext();
  if (!get_ascii)
    SetFromHandle();
  else
    GetASCII();
}

void JXString::SetFromHandle(JSString *str, JSContext *ctx) {
  value_ = str;
  ctx_ = ctx;
  SetFromHandle();
}

JXString::JXString(const char *str, JSContext *iso) {
  ctx_ = iso;
  str_ = NULL;
  autogc_ = true;
  length_ = 0;
  utf8_length_ = 0;
  value_ = NULL;
  ascii_char_set_ = false;
  if (str != NULL) {
    length_ = strlen(str);
    if (length_) str_ = cpystr(ctx_, str, length_);
  }

  if (length_ == 0) {
    str_ = &empty_str;
  }
}

void JXString::Dispose() {
  if (length_ != 0) {
    JS_free(ctx_, str_);
    str_ = NULL;
  }
}

JXString::~JXString() {
  if (autogc_) Dispose();
}

size_t JXString::length() const { return length_; }

size_t JXString::Utf8Length() const {
  if (str_ == NULL) return 0;

  return utf8_length_;
}

}  // namespace jxcore
