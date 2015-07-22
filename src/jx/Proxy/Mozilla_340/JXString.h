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

#ifndef SRC_JX_PROXY_MOZILLA_JXSTRING_H_
#define SRC_JX_PROXY_MOZILLA_JXSTRING_H_
#ifdef JS_ENGINE_MOZJS
#include "MozJS/MozJS.h"
#include "PMacro.h"

namespace jxcore {

class JXString {
  char *str_;
  size_t length_;
  size_t utf8_length_;
  bool autogc_;
  JSString *value_;
  bool ascii_char_set_;

  void SetFromHandle();
  void GetASCII();

  // MOZJS Specific
  JSContext *ctx_;

 public:
  JXCORE_PUBLIC void SetFromSTD(const char *other, const int lenght,
                                JSContext *ctx = NULL);
  JXCORE_PUBLIC void SetFromHandle(const JS_HANDLE_VALUE_REF str,
                                   bool get_ascii = false);

  JXCORE_PUBLIC JXString();
  JXCORE_PUBLIC JXString(const char *str, JSContext *ctx);

  JXCORE_PUBLIC explicit JXString(const JS_HANDLE_VALUE_REF str,
                                  void *ctx = NULL);
  JXCORE_PUBLIC ~JXString();

  JXCORE_PUBLIC char *operator*();
  JXCORE_PUBLIC const char *operator*() const;

  JXCORE_PUBLIC void Dispose();
  JXCORE_PUBLIC void DisableAutoGC() { autogc_ = false; }

  JXCORE_PUBLIC size_t Utf8Length() const;
  JXCORE_PUBLIC size_t length() const;

  // MozJS specific
  JXCORE_PUBLIC JXString(JSString *str, JSContext *ctx, bool autogc = true,
                         bool get_ascii = false);

  JXCORE_PUBLIC void SetFromHandle(JSString *str, JSContext *ctx);

  // TODO(obastemur) v8 like (move this into MozJS::Value)
  JXCORE_PUBLIC size_t
      WriteUtf8(char *buf, const size_t buflen, int *chars_written);
  JXCORE_PUBLIC void GetUTF8LetterAt(const size_t index,
                                     MozJS::auto_str *chars);
};

}  // namespace jxcore
#endif
#endif  // SRC_JX_PROXY_MOZILLA_JXSTRING_H_
