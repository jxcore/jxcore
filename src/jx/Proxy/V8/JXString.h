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

#ifndef SRC_JX_PROXY_V8_JXSTRING_H_
#define SRC_JX_PROXY_V8_JXSTRING_H_
#include "PMacro.h"

namespace jxcore {

class JXString {
  char* str_;
  bool is_set_;
  int length_;
  bool autogc_;

 public:
  void set_std(const char* other, void* _ = NULL);
  void set_handle(JS_HANDLE_VALUE value, bool get_ascii = false);

  JXString();
  explicit JXString(const char* str, void* _ = NULL);
  explicit JXString(JS_HANDLE_VALUE_CARRY value, void* _iso = NULL);
  ~JXString();

  char* operator*();
  const char* operator*() const;

  int length() const;

  JS_HANDLE_STRING ToJSString();

  void DisableAutoGC() { autogc_ = false; }
};
}  // namespace jxcore
#endif  // SRC_JX_PROXY_V8_JXSTRING_H_
