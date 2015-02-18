/*
Copyright (c) 2014 Nubisa, Inc.
Copyright (c) 2005 JSON.org
*/
#ifndef SRC_JX_PROXY_MOZILLA_MOZJS_UTF_MAN_H_
#define SRC_JX_PROXY_MOZILLA_MOZJS_UTF_MAN_H_

#define UTF8_END -1
#define UTF8_ERROR -2

int utf8_to_utf16(char16_t w[], const char p[], unsigned length);

#endif  // SRC_JX_PROXY_MOZILLA_MOZJS_UTF_MAN_H_
