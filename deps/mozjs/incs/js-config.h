/*
 * js-config.h
 */

#ifndef JS_CONFIG_H_
#define JS_CONFIG_H_

#if !defined(JS_NUNBOX32) && !defined(JS_PUNBOX64)
error !
#endif

#undef JS_GC_ZEAL

#ifdef __APPLE__
#define JS_HAVE_MACHINE_ENDIAN_H 1
#endif

#if defined(__IOS__) && defined(JS_CPU_X86)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#endif /* JS_CONFIG_H_ */
