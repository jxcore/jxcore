#include <android/log.h>
#include <dlfcn.h>

/*
 * http://stackoverflow.com/questions/28413530/api-to-get-android-system-properties-is-removed-in-arm64-platforms
 *
 * It's useful API for native apps, just as it is for Java apps, it originates from the native side
 * (see http://rxwen.blogspot.com/2010/01/android-property-system.html), 
 * and other Android system code uses it, so it's unlikely to go away soon.
 */

#if (__ANDROID_API__ >= 21)
/* 
 * Android 'L' makes __system_property_get a non-global symbol.
 * Here we provide a stub which loads the symbol from libc via dlsym.
 */
typedef int (*PFN_SYSTEM_PROP_GET)(const char *, char *);
int __system_property_get(const char* name, char* value)
{
    static PFN_SYSTEM_PROP_GET __real_system_property_get = 0;
    if (!__real_system_property_get) {
        /* libc.so should already be open, get a handle to it. */
        void *handle = dlopen("libc.so", RTLD_NOLOAD);
        if (!handle) {
            __android_log_print(ANDROID_LOG_ERROR, "foobar", "Cannot dlopen libc.so: %s.\n", dlerror());
        } else {
            __real_system_property_get = (PFN_SYSTEM_PROP_GET)dlsym(handle, "__system_property_get");
        }
        if (!__real_system_property_get) {
            __android_log_print(ANDROID_LOG_ERROR, "foobar", "Cannot resolve __system_property_get(): %s.\n", dlerror());
        }
    }
    return (*__real_system_property_get)(name, value);
} 
#endif // __ANDROID_API__ >= 21