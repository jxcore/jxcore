/*
Copyright (c) 2014 Nubisa, Inc.
Copyright (c) 2005 JSON.org
*/
#ifndef SRC_JX_PROXY_MOZILLA_MOZJS_UTF_MAN_H_
#define SRC_JX_PROXY_MOZILLA_MOZJS_UTF_MAN_H_

#define UTF8_END -1
#define UTF8_ERROR -2

// force Char16_t
#include "Isolate.h"

int CheckUnicode(const char p[], unsigned length);
bool ConvertCharToChar16(const char *src, jschar *dst, size_t srclen,
                         size_t *dstlenp);

#endif  // SRC_JX_PROXY_MOZILLA_MOZJS_UTF_MAN_H_
