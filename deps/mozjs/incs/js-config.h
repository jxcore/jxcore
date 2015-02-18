/*
 * js-config.h
 */

#ifndef JS_CONFIG_H_
#define JS_CONFIG_H_

#if !defined(JS_NUNBOX32) && !defined(JS_PUNBOX64)
error!
#endif

#undef JS_GC_ZEAL

#ifdef __APPLE__
#define JS_HAVE_MACHINE_ENDIAN_H 1
#endif

#endif /* JS_CONFIG_H_ */
