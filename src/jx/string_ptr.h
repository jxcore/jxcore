// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_STRING_PTR_H_
#define SRC_JX_STRING_PTR_H_

#include "Proxy/JSEngine.h"

#include <string.h> /* strdup() */
#if !defined(_MSC_VER)
#include <strings.h> /* strcasecmp() */
#else
#define strcasecmp _stricmp
#endif
#include <stdlib.h> /* free() */

namespace node {

#define STRING_PTR_HOST 2048
// helper class for the Parser
struct StringPtr {
  StringPtr() {
    on_heap_ = false;
    Reset();
  }

  ~StringPtr() { Reset(); }

  // If str_ does not point to a heap string yet, this function makes it do
  // so. This is called at the end of each http_parser_execute() so as not
  // to leak references. See issue #2438 and test-http-parser-bad-ref.js.
  void Save() {}

  void Reset() {
    if (on_heap_) {
      if (str_ != host) delete[] str_;
      on_heap_ = false;
    }

    str_ = NULL;
    backup_ = NULL;
    size_ = 0;
    field_marker_ = -1;
  }

  void Update(const char *str, size_t size, const bool field) {
    if (field && size > 0 && field_marker_ == -1) {
      const int field_number = FindMakeUp(str, size);
      if (field_number > 0) {
        backup_ = str;
        field_marker_ = field_number;
        return;
      }
    }

    if (size + size_ < STRING_PTR_HOST && field_marker_ == -1) {
      memcpy(host + size_, str, size);
      size_ += size;
    } else {
      if (!on_heap_) {
        if (field_marker_ > 0) {
          field_marker_ = -1;
          str_ = backup_;
        } else {
          str_ = host;
        }
      }
      UpdateOld(str, size);
    }
  }

  void UpdateOld(const char *str, size_t size) {
    if (str_ == NULL) {
      str_ = str;
    } else if (on_heap_ || str_ + size_ != str) {
      // Non-consecutive input, make a copy on the heap.
      // TODO(?) Use slab allocation, O(n) allocs is bad.
      char *s = new char[size_ + size];
      memcpy(s, str_, size_);
      memcpy(s + size_, str, size);

      if (on_heap_)
        delete[] str_;
      else
        on_heap_ = true;

      str_ = s;
    }
    size_ += size;
  }

  inline bool compare(const char *org, const size_t org_len, const char *tag1,
                      const char *tag2, const size_t tag_len) const {
    if (org_len != tag_len) {
      return false;
    }

    for (size_t i = 0; i < org_len; i++) {
      if (tag1[i] != org[i] && tag2[i] != org[i]) {
        return false;
      }
    }

    return true;
  }

  // fast mapping for popular fields
  int FindMakeUp(const char *field, const size_t size) const {
    if (size < 4 || size > 24) return 0;

    if (size < 7) {
      if (size == 4) {
        if (compare(field, size, "host", "Host", 4)) {
          return 1;
        }
        if (compare(field, size, "link", "Link", 4)) {
          return 11;
        }
      } else if (size == 6) {
        if (compare(field, size, "accept", "Accept", 6)) {
          return 3;
        }
        if (compare(field, size, "cookie", "Cookie", 6)) {
          return 6;
        }
        if (compare(field, size, "pragma", "Pragma", 6)) {
          return 12;
        }
      }
    } else if (size < 14) {
      if (size == 10) {
        if (compare(field, size, "user-agent", "User-Agent", 10)) {
          return 2;
        }
        if (compare(field, size, "connection", "Connection", 10)) {
          return 7;
        }
        if (compare(field, size, "set-cookie", "Set-Cookie", 10)) {
          return 9;  // must stay 9!!
        }
      } else if (size == 13) {
        if (compare(field, size, "cache-control", "Cache-Control", 13)) {
          return 8;
        }
      }
    } else if (size < 16) {
      if (size == 14) {
        if (compare(field, size, "accept-charset", "Accept-Charset", 14)) {
          return 10;
        }
      } else if (size == 15) {
        if (compare(field, size, "accept-language", "Accept-Language", 15)) {
          return 4;
        }
        if (compare(field, size, "accept-encoding", "Accept-Encoding", 15)) {
          return 5;
        }
      }
    } else if (size < 18) {
      if (size == 16) {
        if (compare(field, size, "www-authenticate", "WWW-Authenticate", 16)) {
          return 13;
        }
      } else if (size == 18) {
        if (compare(field, size, "proxy-authenticate", "Proxy-Authenticate",
                    18)) {
          return 14;
        }
      }
    } else {
      if (size == 22) {
        if (compare(field, size, "sec-websocket-protocol",
                    "Sec-Websocket-Protocol", 22)) {
          return 16;
        }
      } else if (size == 24) {
        if (compare(field, size, "sec-websocket-extensions",
                    "Sec-Websocket-Extensions", 24)) {
          return 15;
        }
      }
    }
    return 0;
  }

  inline JS_LOCAL_VALUE MakeUp(const char *field) const {
    ENGINE_LOG_THIS("string_ptr", "MakeUp");
    JS_DEFINE_STATE_MARKER(com);
    JS_LOCAL_ARRAY obj = JS_NEW_ARRAY_WITH_COUNT(2);
    if (size_ > 3 && size_ < 25) {  // h,u,a,c,p,l,w,s
      int marker = FindMakeUp(field, size_);
      if (marker > 0) {
        JS_INDEX_SET(obj, 0, STD_TO_INTEGER(marker));
        return obj;
      }
    }

    JS_INDEX_SET(obj, 0, STD_TO_INTEGER(0));
    JS_INDEX_SET(obj, 1, UTF8_TO_STRING_WITH_LENGTH(field, size_));
    return obj;
  }

  JS_LOCAL_VALUE ToString(const bool field) const {
    ENGINE_LOG_THIS("string_ptr", "ToString");
    JS_DEFINE_STATE_MARKER(com);
    if (!field) {
      if (size_ < STRING_PTR_HOST && size_ > 0) {
        return UTF8_TO_STRING_WITH_LENGTH(host, size_);
      }

      if (str_)
        return UTF8_TO_STRING_WITH_LENGTH(str_, size_);
      else
        return JS_NEW_EMPTY_STRING();
    } else {
      if (field_marker_ > 0) {
        JS_LOCAL_ARRAY obj = JS_NEW_ARRAY_WITH_COUNT(1);
        JS_INDEX_SET(obj, 0, STD_TO_INTEGER(field_marker_));
        return obj;
      }

      if (size_ < STRING_PTR_HOST && size_ > 0) return MakeUp(host);

      if (str_)
        return MakeUp(str_);
      else
        return JS_NEW_EMPTY_STRING();
    }
  }

  const char *str_;
  const char *backup_;
  bool on_heap_;
  size_t size_;

  char host[STRING_PTR_HOST];

  int field_marker_;
  commons *com;
};

}  // namespace node

#endif  // SRC_JX_STRING_PTR_H_
